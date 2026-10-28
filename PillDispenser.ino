#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <EEPROM.h> //salva os alarmes mesmo sem alimentação

// Libs auxiliares
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Pinos
const uint8_t PINO_BOTAO = 2;
const uint8_t PINO_BUZZER = 12;
const uint8_t PINO_LUZ = 5;
const uint8_t PINOS_MOTOR[4] = {8, 9, 10, 11};

// LCD
LiquidCrystal lcd(A0, A1, A2, A3, 3, 4);

// RTC
RTC_DS3231 rtc;

// Motor 28BYJ-48 + ULN2003: aproximadamente 2048 passos por volta
const int PASSOS_POR_VOLTA = 2048;
const int PASSOS_POR_DOSE = 256;
Stepper motor(PASSOS_POR_VOLTA, 8, 9, 10, 11);

// Interação
const unsigned long DEBOUNCE_MS = 35;
const unsigned long SEGURAR_MS = 2000; // tempo pressionado ate ativacao do buzzer
const unsigned long AJUSTAR_RTC_MS = 5000; //tempo pressionado para mudar o RTC
const unsigned long INATIVIDADE_MS = 60000;
const unsigned long CONFIRMACAO_MS = 5000;
const unsigned long PISCAR_MS = 350;
const unsigned long ATUALIZAR_RTC_MS = 200;

const char *const DIAS[] = {"Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sab"};
// *char é um dado de 1 byte ou uma matriz de textos;

//lista de modos possiveis
enum Modo {
  PASSIVO,
  AL_MINUTOS, AL_HORAS, AL_DIA,
  RTC_ANO, RTC_MES, RTC_DIA, RTC_HORA, RTC_MINUTOS,
  MENSAGEM, ALERTA
};
Modo modo = PASSIVO;

// EEPROM
const uint8_t MAX_ALARMES = 24;
const uint16_t ASSINATURA = 0x4D44;
const uint8_t VERSAO = 1;
const uint32_t MINUTO_NUNCA = 0xFFFFFFFFUL;
// *uint8_t representa um número inteiro sem sinal, de 8 bits, podendo armazenar valores de 0 a 255;

// Struct agrupa informacoes numa estrutura de dados unica
struct Alarme {
  uint8_t diaSemana;    // 0=Domingo ... 6=Sabado
  uint8_t hora;         // 0 a 23
  uint8_t minuto;       // 0, 15, 30 ou 45
};

struct DadosSalvos {
  uint16_t assinatura;
  uint8_t versao;
  uint8_t quantidade;
  Alarme alarmes[MAX_ALARMES];
  uint32_t ultimoMinutoDisparado; // Evita segunda dose no mesmo minuto, ate apos reset
  uint16_t crc;
};
DadosSalvos dados = {};

uint16_t calcularCRC() {
  uint16_t crc = 0xFFFF;
  const uint8_t *p = reinterpret_cast<const uint8_t *>(&dados);
  const size_t n = offsetof(DadosSalvos, crc);
  for (size_t i = 0; i < n; ++i) {
    crc ^= uint16_t(p[i]) << 8;
    for (uint8_t b = 0; b < 8; ++b)
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
  }
  return crc;
}

void gravarEEPROM() {
  dados.crc = calcularCRC();
  EEPROM.put(0, dados); // so altera os bytes que mudaram
}

void carregarEEPROM() {
  EEPROM.get(0, dados);
  bool valido = dados.assinatura == ASSINATURA &&
                dados.versao == VERSAO &&
                dados.quantidade <= MAX_ALARMES &&
                dados.crc == calcularCRC();
  if (valido) {
    for (uint8_t i = 0; i < dados.quantidade; ++i) {
      Alarme a = dados.alarmes[i];
      if (a.diaSemana > 6 || a.hora > 23 || a.minuto > 45 || a.minuto % 15 != 0)
        valido = false;
    }
  }
  if (!valido) {
    memset(&dados, 0, sizeof(dados));
    dados.assinatura = ASSINATURA;
    dados.versao = VERSAO;
    dados.ultimoMinutoDisparado = MINUTO_NUNCA;
    gravarEEPROM();
  }
}

// Estado de edicao
Alarme edicao = {0, 0, 0};
uint16_t anoEdit = 2000;
uint8_t mesEdit = 1, diaEdit = 1, horaEdit = 0, minutoEdit = 0;
char mensagem1[17] = "";
char mensagem2[17] = "";
unsigned long inicioMensagem = 0;
unsigned long ultimaInteracao = 0;

// Estado de leitura
DateTime agora(2000, 1, 1, 0, 0, 0);
bool horarioConfiavel = false;
unsigned long ultimaLeitura = 0;
unsigned long ultimoDesenhoPassivo = 0;

// Botao e telas
bool leituraAnterior = HIGH;
bool botaoEstavel = HIGH;
unsigned long instanteMudanca = 0;
unsigned long inicioPressao = 0;
bool ignorarSoltura = false;
char linhaAnterior1[17] = "";
char linhaAnterior2[17] = "";

bool editandoAlarme() {
  return modo >= AL_MINUTOS && modo <= AL_DIA;
}
bool editandoRTC() {
  return modo >= RTC_ANO && modo <= RTC_MINUTOS;
}

void entrarModo(Modo novo) {
  modo = novo;
  ultimaInteracao = millis();
  linhaAnterior1[0] = '\0';
  linhaAnterior2[0] = '\0';
}

void liberarMotor() {
  for (uint8_t i = 0; i < 4; ++i)
    digitalWrite(PINOS_MOTOR[i], LOW);
  // Sem torque de sustentacao quando parado.
}

uint8_t diasNoMes(uint16_t ano, uint8_t mes) {
  const uint8_t tabela[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (mes == 2 && ((ano % 4 == 0 && ano % 100 != 0) || ano % 400 == 0))
    return 29;
  return tabela[mes - 1];
}

void limitarDia() {
  uint8_t maxDia = diasNoMes(anoEdit, mesEdit);
  if (diaEdit > maxDia) diaEdit = maxDia;
}

void mostrarLinhas(const char *t1, const char *t2) {
  char l1[17], l2[17];
  memset(l1, ' ', 16); memset(l2, ' ', 16);
  l1[16] = '\0'; l2[16] = '\0';
  for (uint8_t i = 0; i < 16 && t1[i]; ++i) l1[i] = t1[i];
  for (uint8_t i = 0; i < 16 && t2[i]; ++i) l2[i] = t2[i];
  if (strcmp(l1, linhaAnterior1) != 0) {
    lcd.setCursor(0, 0);
    lcd.print(l1);
    strcpy(linhaAnterior1, l1);
  }
  if (strcmp(l2, linhaAnterior2) != 0) {
    lcd.setCursor(0, 1);
    lcd.print(l2);
    strcpy(linhaAnterior2, l2);
  }
}

void desenharLCD() {
  // Evita consultar a temperatura do RTC milhares de vezes por segundo.
  if (modo == PASSIVO) {
    unsigned long ms = millis();
    if (ms - ultimoDesenhoPassivo < 500) return;
    ultimoDesenhoPassivo = ms;
  }
  char primeira[24] = "";
  char segunda[24] = "";

  switch (modo) {
    case PASSIVO: {
      if (!horarioConfiavel) {
        mostrarLinhas("RTC SEM HORARIO", "Segure 5 segundos");
        return;
      }
      snprintf(primeira, sizeof(primeira), "%s %02u/%02u/%04u",
               DIAS[agora.dayOfTheWeek()], agora.day(), agora.month(), agora.year());
      float temperatura = rtc.getTemperature();
      int decimos = int(temperatura * 10.0f + (temperatura < 0 ? -0.5f : 0.5f));
      int valorAbsoluto = abs(decimos);
      char strTemp[9];
      snprintf(strTemp, sizeof(strTemp), "%s%d.%d", decimos < 0 ? "-" : "",
               valorAbsoluto / 10, valorAbsoluto % 10);
      snprintf(segunda, sizeof(segunda), "%02u:%02u:%02u %s%cC",
               agora.hour(), agora.minute(), agora.second(), strTemp, (char)223);
      break;
    }
    case AL_MINUTOS:
      strcpy(primeira, "ALARME: MINUTOS");
      snprintf(segunda, sizeof(segunda), "--:%02u ---", edicao.minuto);
      break;
    case AL_HORAS:
      strcpy(primeira, "ALARME: HORAS");
      snprintf(segunda, sizeof(segunda), "%02u:%02u ---", edicao.hora, edicao.minuto);
      break;
    case AL_DIA:
      strcpy(primeira, "ALARME: DIA");
      snprintf(segunda, sizeof(segunda), "%02u:%02u %s", edicao.hora,
               edicao.minuto, DIAS[edicao.diaSemana]);
      break;
    case RTC_ANO:     strcpy(primeira, "AJUSTAR: ANO");    break;
    case RTC_MES:     strcpy(primeira, "AJUSTAR: MES");    break;
    case RTC_DIA:     strcpy(primeira, "AJUSTAR: DIA");    break;
    case RTC_HORA:    strcpy(primeira, "AJUSTAR: HORA");   break;
    case RTC_MINUTOS: strcpy(primeira, "AJUSTAR: MINUTO"); break;
    case MENSAGEM:
      mostrarLinhas(mensagem1, mensagem2);
      return;
    case ALERTA:
      mostrarLinhas("PEGUE A PILULA", "Pressione botao");
      return;
  }

  if (editandoRTC()) {
    snprintf(segunda, sizeof(segunda), "%04u/%02u/%02u %02u:%02u",
             anoEdit, mesEdit, diaEdit, horaEdit, minutoEdit);
  }
  mostrarLinhas(primeira, segunda);
}

void mostrarMensagem(const char *t1, const char *t2) {
  strncpy(mensagem1, t1, 16); mensagem1[16] = '\0';
  strncpy(mensagem2, t2, 16); mensagem2[16] = '\0';
  entrarModo(MENSAGEM);
  inicioMensagem = millis();
}

void iniciarAlarme() {
  edicao.diaSemana = 0;
  edicao.hora = 0;
  edicao.minuto = 0;
  entrarModo(AL_MINUTOS);
}

void iniciarAjusteRTC() {
  // Aproveita a data atual se valida, para reduzir os cliques.
  // Caso contrario, comeca em 2000/01/01 00:00.
  if (horarioConfiavel) {
    anoEdit = agora.year();
    if (anoEdit < 2000 || anoEdit > 2067) anoEdit = 2000;
    mesEdit = agora.month();
    diaEdit = agora.day();
    horaEdit = agora.hour();
    minutoEdit = agora.minute();
  } else {
    anoEdit = 2000;
    mesEdit = 1;
    diaEdit = 1;
    horaEdit = 0;
    minutoEdit = 0;
  }
  entrarModo(RTC_ANO);
}

void cliqueCurto() {
  switch (modo) { // Switch funciona como uma corrente de else-if
    case AL_MINUTOS: edicao.minuto = (edicao.minuto + 15) % 60; break; // Operador % calcula resto de divisao
    case AL_HORAS:   edicao.hora = (edicao.hora + 1) % 24; break;
    case AL_DIA:     edicao.diaSemana = (edicao.diaSemana + 1) % 7; break;
    case RTC_ANO:
      anoEdit = (anoEdit >= 2067) ? 2000 : anoEdit + 1;
      limitarDia();
      break;
    case RTC_MES:
      mesEdit = (mesEdit == 12) ? 1 : mesEdit + 1;
      limitarDia();
      break;
    case RTC_DIA:
      diaEdit = (diaEdit >= diasNoMes(anoEdit, mesEdit)) ? 1 : diaEdit + 1;
      break;
    case RTC_HORA:    horaEdit = (horaEdit + 1) % 24; break;
    case RTC_MINUTOS: minutoEdit = (minutoEdit + 1) % 60; break;
    default: break;
  }
}

void salvarAlarme() {
  for (uint8_t i = 0; i < dados.quantidade; ++i) {
    Alarme a = dados.alarmes[i];
    if (a.diaSemana == edicao.diaSemana && a.hora == edicao.hora &&
        a.minuto == edicao.minuto) {
      mostrarMensagem("ALARME REPETIDO", "Nao foi duplicado");
      return;
    }
  }
  if (dados.quantidade >= MAX_ALARMES) {
    mostrarMensagem("MEMORIA LOTADA", "Maximo 24 alarmes");
    return;
  }
  dados.alarmes[dados.quantidade++] = edicao;
  gravarEEPROM();
  char resumo[17];
  snprintf(resumo, sizeof(resumo), "%s %02u:%02u", DIAS[edicao.diaSemana],
           edicao.hora, edicao.minuto);
  mostrarMensagem("ALARME SALVO!", resumo);
  Serial.print(F("Alarme salvo: "));
  Serial.println(resumo);
}

void salvarAjusteRTC() {
  rtc.adjust(DateTime(anoEdit, mesEdit, diaEdit, horaEdit, minutoEdit, 0));
  horarioConfiavel = true;
  agora = rtc.now();
  // Requisito: acertar a data apaga todos os alarmes.
  dados.quantidade = 0;
  memset(dados.alarmes, 0, sizeof(dados.alarmes));
  dados.ultimoMinutoDisparado = MINUTO_NUNCA;
  gravarEEPROM();
  entrarModo(PASSIVO);
  Serial.println(F("RTC ajustado; todos os alarmes apagados."));
}

void confirmacaoLonga() {
  switch (modo) {
    case AL_MINUTOS: entrarModo(AL_HORAS); break;
    case AL_HORAS:   entrarModo(AL_DIA); break;
    case AL_DIA:     salvarAlarme(); break;
    case RTC_ANO:     entrarModo(RTC_MES); break;
    case RTC_MES:     entrarModo(RTC_DIA); break;
    case RTC_DIA:     entrarModo(RTC_HORA); break;
    case RTC_HORA:    entrarModo(RTC_MINUTOS); break;
    case RTC_MINUTOS: salvarAjusteRTC(); break;
    default: break;
  }
}

// Decide a troca de modo
void aoSoltar(unsigned long duracao) {
  ultimaInteracao = millis();
  if (ignorarSoltura) {
    ignorarSoltura = false;
    return;
  }
  if (modo == ALERTA) {
    entrarModo(PASSIVO);
    Serial.println(F("Aviso de pilula reconhecido."));
    return;
  }
  if (modo == MENSAGEM) return;
  if (modo == PASSIVO) {
    if (duracao >= AJUSTAR_RTC_MS) iniciarAjusteRTC();
    else if (duracao >= SEGURAR_MS) iniciarAlarme();
  } else if (editandoAlarme() || editandoRTC()) {
    if (duracao >= SEGURAR_MS) confirmacaoLonga();
    else cliqueCurto();
  }
}

// Realiza leitura do botao
void lerBotao(unsigned long ms) {
  bool leitura = digitalRead(PINO_BOTAO);
  if (leitura != leituraAnterior) instanteMudanca = ms;
  leituraAnterior = leitura;
  if ((ms - instanteMudanca) >= DEBOUNCE_MS && leitura != botaoEstavel) {
    botaoEstavel = leitura;
    ultimaInteracao = ms;

    if (botaoEstavel == LOW) { // Botao pressionado
      inicioPressao = ms;
    } else {
      aoSoltar(ms - inicioPressao); // Botao solto
    }

  }
}

void atualizarBuzzerELuz(unsigned long ms) {
  bool bip = false;
  if (modo == ALERTA) {
    bip = ((ms / PISCAR_MS) % 2) == 0;
    digitalWrite(PINO_LUZ, bip ? HIGH : LOW);
  } else {
    bool segurando = botaoEstavel == LOW && (modo == PASSIVO ||
    editandoAlarme() || editandoRTC());
    if (segurando) {
      unsigned long tempo = ms - inicioPressao;
      if (tempo >= SEGURAR_MS &&
          (modo != PASSIVO || tempo < AJUSTAR_RTC_MS))
        bip = true;
    }
    digitalWrite(PINO_LUZ, modo == PASSIVO ? LOW : HIGH);
  }
  digitalWrite(PINO_BUZZER, bip ? HIGH : LOW);
}

void verificarHorario(unsigned long ms) {
  if (ms - ultimaLeitura < ATUALIZAR_RTC_MS) return;
  ultimaLeitura = ms;
  if (!horarioConfiavel) return;
  agora = rtc.now();
  if (!agora.isValid()) {
    horarioConfiavel = false;
    return;
  }
  if (modo == ALERTA) return;

  const uint32_t minutoAtual = agora.unixtime() / 60UL;
  if (minutoAtual == dados.ultimoMinutoDisparado) return;
  for (uint8_t i = 0; i < dados.quantidade; ++i) {
    const Alarme &a = dados.alarmes[i];
    if (a.diaSemana == agora.dayOfTheWeek() &&
        a.hora == agora.hour() && a.minuto == agora.minute()) {
      // Registra antes de mover: um reset nao deve soltar outra pilula
      // no mesmo minuto. Nao garante dispensacao se faltar energia.
      dados.ultimoMinutoDisparado = minutoAtual;
      gravarEEPROM();
      ignorarSoltura = (botaoEstavel == LOW);
      entrarModo(ALERTA);
      digitalWrite(PINO_BUZZER, LOW);
      digitalWrite(PINO_LUZ, LOW);
      Serial.println(F("Horario do alarme: movimentando motor."));
      motor.step(PASSOS_POR_DOSE); // chamada bloqueante (~0,75 s a 10 RPM)
      //resulta em queda de tensao
      liberarMotor();
      // Recupera o LCD se a queda de tensao desconfigurou o controlador.
      // Nao corrige alimentacao insuficiente ou reinicio do Arduino!
      lcd.begin(16, 2);
      linhaAnterior1[0] = '\0';
      linhaAnterior2[0] = '\0';
      ultimaInteracao = millis();
      return;
    }
  }
}

void setup() {

  Serial.begin(9600);

  pinMode(PINO_BUZZER, OUTPUT);

  pinMode(PINO_LUZ, OUTPUT);

  pinMode(PINO_BOTAO, INPUT_PULLUP); // PULLUP faz com que o botao leia LOW quando pressionado

  digitalWrite(PINO_BUZZER, LOW);

  digitalWrite(PINO_LUZ, LOW);

  motor.setSpeed(4); // Essa velocidade permite a rotacao correta

  liberarMotor();

  lcd.begin(16, 2);

  mostrarLinhas("INICIANDO...", "");

  carregarEEPROM();

  if (!rtc.begin()) {
    mostrarLinhas("ERRO: RTC", "Verifique fios");
    Serial.println(F("ERRO: RTC nao encontrado."));
    while (true) {
      digitalWrite(PINO_BUZZER, LOW);
      delay(100);
    }
  }

  horarioConfiavel = !rtc.lostPower();
  agora = rtc.now();
  if (!agora.isValid()) horarioConfiavel = false;
  entrarModo(PASSIVO);
}

void loop() {

  unsigned long ms = millis(); //quantos milissegundos desde o inicio da execucao

  lerBotao(ms);

  verificarHorario(ms);

  ms = millis(); // motor.step() pode ter bloqueado o programa

  if (modo != PASSIVO && ms - ultimaInteracao >= INATIVIDADE_MS) {
    
    entrarModo(PASSIVO);
    
    Serial.println(F("Tempo de inatividade: voltando ao modo passivo."));
  }

  if (modo == MENSAGEM && ms - inicioMensagem >= CONFIRMACAO_MS) {
    entrarModo(PASSIVO);
  }

  atualizarBuzzerELuz(ms);

  desenharLCD();

}
