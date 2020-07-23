#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include "ThingSpeak.h"
#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#include <UniversalTelegramBot.h>

RF24 radio(2, 15);               // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 00;   // Endereço desse nó em formato octal 
const uint16_t node01 = 01;      // Endereço de outros nós da rede
const uint16_t node011 = 011;

bool flag_node00 = false;
bool flag_node01 = false;
bool flag_node011 = false;
bool flag_node02 = false;
int time_node01 = 0;
int time_node011 = 0;
int time_node012 = 0;
int time_node022 = 0;
uint8_t motor_ctrl = 0;

//constantes e variáveis globais
#define SSID_REDE     "nome da sua rede"
#define SENHA_REDE    "senha da rede"
#define INTERVALO_ENVIO_THINGSPEAK  15000
#define BOT_TOKEN      "token do bot"

char EnderecoAPIThingSpeak[] = "api.thingspeak.com";
String ChaveEscritaThingSpeak =  "XXXXXXXXXX" //WRITE API KEY do Thingspeak
unsigned long myChannelNumber = 1234567890;         //número do canal criado no Thingspeak
long lastConnectionTime;

WiFiClient client;               //thingspeak
WiFiClientSecure clientSecure;   //telegram bot
UniversalTelegramBot bot(BOT_TOKEN, clientSecure);

//Intervalo para leitura de novas mensagens do telegram
int botRequestDelay = 2000;
unsigned long lastTimeBotRan;

void send2ThingSpeak(String StringDados);

typedef struct node01
{
  float dht_temp;
  float dht_umid;
  uint32_t umid_solo;
  uint8_t motor_stat;
} t_node01;

t_node01 node_01;

typedef struct node011
{
  float temp;
  uint8_t fire_stat = false;
  uint32_t gas;
} t_node011;

t_node011 node_011;

void handleNewMessages(int numNewMessages) {
  Serial.print("Mensagem Recebida do Telegram:");
  Serial.println(String(numNewMessages));
  char msg[200];
  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    Serial.print("chat_id = ");
    Serial.println(chat_id);
    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

   
    if (text == "/node1")
    {
      sprintf(msg,"-----------STAT Node 01-----------\n");
      sprintf(msg + strlen(msg), " Temperatura = %.2f °C \n Umidade do Ar = %.2f %% \n", node_01.dht_temp, node_01.dht_umid);
      if (node_01.umid_solo < 40)
        sprintf(msg + strlen(msg), "Umidade do Solo = Seco");
      else
        sprintf(msg + strlen(msg), "Umidade do Solo = Úmido\n");
      if(node_01.motor_stat)
        sprintf(msg + strlen(msg), "\nSistema de Irrigação está LIGADO\n");
      else
      {
        sprintf(msg + strlen(msg), "- Favor ligar o sistema de irrigação!\n");
        sprintf(msg + strlen(msg), "Sistema de Irrigação está DESLIGADO\n");
      }
      bot.sendMessage(chat_id, msg, "");
    }

    if (text == "/node3")
    {
      sprintf(msg,"-----------STAT Node 03-----------\n");
      sprintf(msg+ strlen(msg), "Temperature = %.2f °C\n", node_011.temp);
      if (node_011.fire_stat == 0)
        sprintf(msg + strlen(msg), "Node 3 Está pegando FOGO!!");
      else
        sprintf(msg + strlen(msg), "Node 3 está ok.");
      bot.sendMessage(chat_id, msg, "");

    }

    if (text == "/web")
    {
      String web = "Acesse o link: https://thingspeak.com/channels/1088833";
      bot.sendMessage(chat_id, web, "");
    }
    if (text == "/start")
    {
      String welcome = "Olá, " + from_name + ".\n";
      String Ed = "Edward";
      if (from_name == "Edward")
        Ed = "Você";

      welcome += "Eu sou o Bot do Telegram criado por " + Ed + ".\nEstou aqui para te auxiliar na interação com o Sistema de Rede de Sensores sem Fio";
      welcome += " desenvolvida para o Projeto da Disciplina de IoT (CE-289)\n";
      welcome += "Os comandos disponíveis (por enquanto) são:\n";
      welcome += "/node1  : Receber dados do node01\n";
      welcome += "/node3 : Receber dados do node011\n";
      welcome += "/web : Receber o link da plataforma Web Thingspeak\n";
      welcome += "/wateron : Ligar o sistema de irrigação do node01 \n";
      welcome += "/wateroff : Desligar o sistema de irrigação do node01  \n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
    if (text == "/wateron")
    {
      bot.sendMessage(chat_id, "Ligando Sistema de Irrigação do node01", "");
      RF24NetworkHeader header2(node01);
      motor_ctrl = 1;
      bool ok = network.write(header2, &motor_ctrl, sizeof(motor_ctrl));
    }
    if (text == "/wateroff")
    {
      bot.sendMessage(chat_id, "Desligando Sistema de Irrigação do node01", "");
      RF24NetworkHeader header2(node01);
      motor_ctrl = 0;
      bool ok = network.write(header2, &motor_ctrl, sizeof(motor_ctrl));
    }
  }
}

void send2ThingSpeak(String StringDados)
{
  if (client.connect(EnderecoAPIThingSpeak, 80))
  {

    //faz a requisição HTTP ao ThingSpeak
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + ChaveEscritaThingSpeak + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(StringDados.length());
    client.print("\n\n");
    client.print(StringDados);

    lastConnectionTime = millis();
    Serial.println("- Informações enviadas ao ThingSpeak!");
  }
}
void setup()
{
  //Configuração da UART
  Serial.begin(9600);
  Serial.println("Boot Node00 Gateway");
  SPI.begin();
  radio.begin();
  network.begin(90, this_node); //(channel, node address)
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS) ;
  lastConnectionTime = 0;
  //Inicia o WiFi
  clientSecure.setInsecure();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println("Conectando-se à rede WiFi...");
  delay(1000);
  WiFi.begin(SSID_REDE, SENHA_REDE);

  //Espera a conexão no router
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  ThingSpeak.begin(client);  // Inicializa ThingSpeak
  delay(1000);
}

void loop()
{
  char postStr[500] = {0};
  network.update();
  while ( network.available()) // existe algum dado para receber?
  { 
    RF24NetworkHeader header;
    network.peek(header);
    Serial.print("\nDados Recebido do nó:");
    Serial.println(header.from_node, OCT);
    if (header.from_node == 01) // recebeu dados do node01
    {
      network.read(header, &node_01, sizeof(node_01)); // Lê os dados recebido
      Serial.print("Temperatura = ");
      Serial.println(node_01.dht_temp);
      Serial.print("Umidade do Ar = ");
      Serial.println(node_01.dht_umid);
      Serial.print("Umidade do solo = ");
      Serial.print(node_01.umid_solo);
      Serial.println("%");
      flag_node01 = true;
    }
    else if (header.from_node == 011) // recebeu dados do node011
    {
      network.read(header, &node_011, sizeof(node_011)); // Lê os dados recebido
      Serial.print("Temperatura = ");
      Serial.println(node_011.temp);
      Serial.print("Status Sensor de Fogo = ");
      if(node_011.fire_stat)
        Serial.println("ok");
      else
        Serial.println("not ok! Nó 3 está pegando FOGO!!");
      flag_node011 = true;
   
      if ( node_011.fire_stat == false) //detectou que há indício de incêndio no node011
      {
        //Envia uma mensagem pra alertar o usuário
        bot.sendMessage("940406680", "Mensagem Automática! \n \xF0\x9F\x94\xA5 Node03 is on FIREEEE!! \xF0\x9F\x94\xA5 ");
      }
    }

    //verifica se está conectado no WiFi e se é o momento de enviar dados ao ThingSpeak
    if (!client.connected() &&
        (millis() - lastConnectionTime > INTERVALO_ENVIO_THINGSPEAK))
    {
      if (flag_node01 == true) //há dados do node01 para serem enviados ao ThingSpeak
      {
        ThingSpeak.setField(1, node_01.dht_temp);
        ThingSpeak.setField(2, node_01.dht_umid);
        ThingSpeak.setField(3, 1);
        sprintf(postStr, "field1=%.2f&field2=%.2f&field3=%d", node_01.dht_temp, node_01.dht_umid, 1);
        flag_node01 = false;
      }
      if (flag_node011 == true) ////há dados do node011 para serem enviados ao ThingSpeak
      {
        ThingSpeak.setField(4, 1);
        ThingSpeak.setField(5, node_011.fire_stat);
        ThingSpeak.setField(6, node_011.temp);
        sprintf(postStr + strlen(postStr), "&field4=%d&field5=%d&field6=%.2f", 1, node_011.fire_stat, node_011.temp);
        flag_node011 = false;
      }
      send2ThingSpeak(postStr);
    }

    if (millis() > lastTimeBotRan + botRequestDelay) //Verifica se há comando recebido do Telegram
    {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

      while (numNewMessages) {
        Serial.println("Comando Recebido");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }

      lastTimeBotRan = millis();
    }
    delay(1000);
  }
}
