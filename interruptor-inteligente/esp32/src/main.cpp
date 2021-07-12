//Required HTTPClientESP32Ex library to be installed  https://github.com/mobizt/HTTPClientESP32Ex

#include <WiFi.h>
#include "FirebaseESP32.h"

#define FIREBASE_HOST "" //Change to your Firebase RTDB project ID e.g. Your_Project_ID.firebaseio.com
#define FIREBASE_AUTH "" //Change to your Firebase RTDB secret password
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

FirebaseData firebaseData1;
FirebaseData firebaseData2;

#define LED_conexaoB 22
#define LED_conexaoG 23
#define lencol_pino 27
#define botao_pino T4

bool disableLED = true;
bool outputState = false;

//Variáveis para o botão
bool currentState = false;
bool lastState = false;
int tempo = 0;
int contagem = 1;

String path = "/lencolEletrico/OnOff";
String nodeID = "on";

// Função q só troca o led caso esteja permitido
void toggleLED(int led, int state)
{
  if (disableLED)
  {
    Serial.printf("Vou desligar o led azul\n");
    digitalWrite(led, true); //Bota como true pq o led é cátodo comum
  }
  else
  {
    Serial.printf("Vou colocar o led para %d\n", state);
    digitalWrite(led, state);
  }
}

void streamCallback(StreamData data)
{

  if (data.dataType() == "boolean")
  {
    if (data.boolData())
      Serial.println("Define a saída como alta");
    else
      Serial.println("Define a saída como baixa");

    bool value = data.boolData();
    digitalWrite(lencol_pino, value);
    toggleLED(LED_conexaoB, !value); //O valor é negado pq o led é cátodo comum
    outputState = value;
  }
}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
  {
    Serial.println();
    Serial.println("Stream timeout, resume streaming...");
    Serial.println();
  }
}

void setup()
{

  Serial.begin(115200);

  pinMode(lencol_pino, OUTPUT);

  //Definição de saida e entrada de cada pino
  pinMode(LED_conexaoB, OUTPUT);
  pinMode(LED_conexaoG, OUTPUT);
  pinMode(lencol_pino, OUTPUT);

  //Desliga o LED de status de conexão do Wifi
  digitalWrite(LED_conexaoB, HIGH);
  digitalWrite(LED_conexaoG, LOW);

  Serial.println();
  Serial.println();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }

  //Desliga o led verde pq já conectou no wifi
  digitalWrite(LED_conexaoB, LOW);

  Serial.println();
  Serial.print("Conectado com IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  if (!Firebase.beginStream(firebaseData1, path + "/" + nodeID))
  {
    Serial.println("Não iniciou a stream");
    Serial.println("MOTIVO: " + firebaseData1.errorReason());
    Serial.println();
  }

  Firebase.setStreamCallback(firebaseData1, streamCallback, streamTimeoutCallback);
  digitalWrite(LED_conexaoB, HIGH);
  digitalWrite(LED_conexaoG, HIGH);
}

// Callback do clique triplo
void tripleClick()
{
  disableLED = !disableLED;
  Serial.printf("\n clique triplo %d\n\n", disableLED);
  toggleLED(LED_conexaoB, !outputState);
}

// Callback do clique longo
void longClick()
{
  Serial.printf("\n long clique %d\n\n", outputState);
  Firebase.setBool(firebaseData2, path + "/" + nodeID, !outputState);
  digitalWrite(lencol_pino, !outputState);
}

// Função que faz toda a avaliação do botão
void botao()
{
  int botao = touchRead(botao_pino) > 30 ? false : true;

  if (botao && !currentState) //Verifica a borda de subida
  {
    if ((millis() - tempo) < 700) //Compara com o tempo do último clique para saber se é um clique rápido
    {
      contagem++;
      if (contagem == 2)
      {
        tripleClick();
        contagem = 0;
      }
    }
    else
    {
      contagem = 0;
    }

    tempo = millis();
    currentState = true;
  }

  if (!botao && currentState) // borda de descida
  {
    if ((millis() - tempo) > 1000)
      longClick();
    char teste[100];
    sprintf(teste, "Botão solto - %d milissegundos\n", millis() - tempo);
    tempo = millis();
    currentState = false;
    Serial.printf(teste);
  }
}

void loop()
{
  botao();
}