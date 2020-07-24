#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DS18B20   9
#define VCC       6
#define FIRE_IN   3
#define FIRE_OUT  5

OneWire oneWire(DS18B20);
DallasTemperature sensors(&oneWire);

RF24 radio(7, 8);               // nRF24L01 (CE,CSN)

RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 011;   //  Endereço desse nó em formato octal 
const uint16_t master00 = 00;    //  Endereço do nó gateway em formato octal 
const unsigned long interval = 1000;  //Intervalo de envio de dados para o gateway
unsigned long last_sent;            // When did we last send?


typedef struct node011 
{
  float temp;
  uint8_t fire_stat = false;
}
t_node011;

t_node011 node_011;

void fire_trigger() //Verifica se o botão foi acionado
{
  node_011.fire_stat = digitalRead(FIRE_IN);
  Serial.print("fire_stat = ");
  Serial.println(node_011.fire_stat);
}

void setup()
{
  Serial.begin(9600);

  Serial.println("Node 011 BOOT");
  pinMode(VCC,OUTPUT);
  pinMode(FIRE_OUT, OUTPUT);
  pinMode(FIRE_IN, INPUT_PULLUP);
  digitalWrite(FIRE_OUT, HIGH);
  digitalWrite(VCC, HIGH);
  attachInterrupt(digitalPinToInterrupt(FIRE_IN), fire_trigger, CHANGE);
  node_011.fire_stat = digitalRead(FIRE_IN);
  sensors.begin();
  SPI.begin();
  
  radio.begin();
  network.begin(90, this_node);  //(channel, node address)
  radio.setDataRate(RF24_250KBPS );
  radio.setPALevel(RF24_PA_MAX);
}

void loop()
{
  
  sensors.requestTemperatures();
  node_011.temp = sensors.getTempCByIndex(0);
  network.update();

  //===== Envio =====//
  unsigned long now = millis();
  if (now - last_sent >= interval)
  { 
    last_sent = now;
    Serial.print("Temp = ");
    Serial.print(node_011.temp);
    Serial.println("°C");

    RF24NetworkHeader header(master00);   // (Address where the data is going)
    bool ok = network.write(header, &node_011, sizeof(node_011)); // Send the data
    if(ok)
      Serial.println("enviado com sucesso");
    else
    Serial.println("envio falhou");
  }
  if (node_011.fire_stat == false)
  {
    Serial.println("Node 011 is on FIRE!");
  }
  else
  {
    Serial.println("Node 011 is ok");
  }

  delay(1000);
}
