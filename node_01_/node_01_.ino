

#include <DHT.h>
#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>

#define DEBUG 1
#define SOLO  A0
#define LED_R 3
#define LED_G 2
RF24 radio(7, 8);               // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 01;   // Endereço desse nó em formato octal 
const uint16_t node00 = 00;      // Endereço de outros nós da rede


DHT dht(12, DHT22); //Define o pino e tipo de sensor DHT utilizado

uint32_t time_count = 0;

typedef struct node01 //TAD criado para armazenar os dados dos sensores
{
  float dht_temp;
  float dht_umid;
  uint32_t umid_solo;
  uint8_t motor_stat = false;

} t_node01;

t_node01 node_01;

int valor_solo;

void setup()
{
  Serial.begin(9600);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, HIGH);
  SPI.begin();
  radio.begin();
  network.begin(90, this_node);  //(channel, node address)
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS) ;
  dht.begin(); //inicializa o DHT22
  delay(1000);

}

void loop()
{
  valor_solo = analogRead(SOLO);

  node_01.umid_solo = map(valor_solo, 0, 1023, 100, 0);
  //leitura de temperatura e umidade do DHT22
  node_01.dht_temp = dht.readTemperature();
  node_01.dht_umid = dht.readHumidity();

  Serial.print("temp = ");
  Serial.println(node_01.dht_temp);
  Serial.print("umid = ");
  Serial.println(node_01.dht_umid);
  Serial.print("solo = ");
  Serial.print(node_01.umid_solo);
  Serial.println("%");
  Serial.print("Sistema de Irrigação = ");
  if ( node_01.motor_stat)
    Serial.println("Ligado");
  else
    Serial.println("Desligado");
  delay(1000);

  network.update();
  //===== Leitura =====//
  while ( network.available() )
  { // existe algum comando para receber?
    Serial.print("DATA Received from = ");
    RF24NetworkHeader header;
    network.peek(header);
    Serial.println(header.from_node, OCT);
    uint8_t motor_ctrl;
    network.read(header, &motor_ctrl, sizeof(motor_ctrl)); // Lê os dados recebido
    if (motor_ctrl == true)
    {
      Serial.println("Turning motor ON");
      digitalWrite(LED_G, LOW); // Liga o Sistema de irrigacao
      digitalWrite(LED_R, HIGH); 
      node_01.motor_stat = true;
    }
    else
    {
      Serial.println("Turning motor OFF");
      digitalWrite(LED_G, HIGH); // Desliga o Sistema de irrigacao
      digitalWrite(LED_R, LOW); // 
      node_01.motor_stat = false;
    }
  }
  //===== Envio =====//
  RF24NetworkHeader header(node00);     //enviando dados coletados para node00
  bool ok = network.write(header, &node_01, sizeof(node_01)); 
  if (ok)
    Serial.println("enviado com sucesso");
  else
    Serial.println("envio falhou");
}
