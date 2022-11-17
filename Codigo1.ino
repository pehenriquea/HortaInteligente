#include <arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Ultrasonic.h>

#define DHTPIN 32
#define DHTTYPE DHT11 // DHT 11
//Define Relé/BLINK

#define pinoR 15
#define pinoG 16
#define pinoB 17
#define bombinha 23

//Define Sensor Proximidade
#define trig 18
#define echo 19

/* Definicoes para o MQTT */
#define TOPICO_PUBLISH_TEMPERATURA   "topico_publica_temp"
#define TOPICO_PUBLISH_UMIDADE_SOLO  "topico_publica_umidade_solo"
#define TOPICO_PUBLISH_NIVEL_AGUA    "topico_publica_nivel_agua"
#define TOPICO_PUBLISH_UMIDADE       "topico_publica_umidade"
#define TOPICO_SUBSCRIBE_BOMBA_AGUA  "topico_publica_bomba_agua"
#define ID_MQTT  "Horta_Inteligente_mqtt"     //id mqtt (para identificação de sessão)

//Casa Pedro
/*const char* SSID     = "Justweb-Pedro"; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = "36393525"; // Senha da rede WI-FI que deseja se conectar
const char* BROKER_MQTT = "test.mosquitto.org";
int BROKER_PORT = 1883; // Porta do Broker MQTT*/

//Casa Isabela
const char* SSID     = "2.4GNETVIRTUACASA6"; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = "Scarabelli"; // Senha da rede WI-FI que deseja se conectar
const char* BROKER_MQTT = "test.mosquitto.org";
int BROKER_PORT = 1883; // Porta do Broker MQTT

//Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

Ultrasonic ultrasonic(trig,echo);
DHT dht(DHTPIN, DHTTYPE);

int distance;
int leitura;

/* Prototypes */
void initWiFi(void);
void initMQTT(void);
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT(void);
void reconnectWiFi(void);
void VerificaConexoesWiFIEMQTT(void);
void umidadeTemp();
void sensorDeUmidade();
void valorUmidadeETemperatura();
void setColor(int vermelho, int verde, int azul);

int sensorUmidadeSolo = 34;
int valorLimiteUmidade = 2457;           // valor numérico da tensão de comparação do sensor / valor máximo = 4095
int valorPorcentagem = 0;

/*
   Implementações
*/
/* Função: inicializa e conecta-se na rede WI-FI desejada
   Parâmetros: nenhum
   Retorno: nenhum
*/
void initWiFi(void)
{
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");
  reconnectWiFi();
}
/* Função: inicializa parâmetros de conexão MQTT(endereço do broker, porta e seta função de callback)
   Parâmetros: nenhum
   Retorno: nenhum
*/
void initMQTT(void)
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
  MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

/* Função: função de callback
           esta função é chamada toda vez que uma informação de
           um dos tópicos subescritos chega)
   Parâmetros: nenhum
   Retorno: nenhum
*/
void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
  String msg;

  /* obtem a string do payload recebido */
  for (int i = 0; i < length; i++)
  {
    char c = (char)payload[i];
    msg += c;
  }
  Serial.print("Chegou a seguinte string via MQTT: ");
  Serial.println(msg);

  /* toma ação dependendo da string recebida */
  if (msg.equals("L"))
  {
    digitalWrite(bombinha,HIGH);
  }
  if (msg.equals("D"))
  {
    digitalWrite(bombinha, LOW);
  }

}
/*  Função:  reconecta-se  ao  broker  MQTT  (caso  ainda  não  esteja  conectado  ou  em  caso  de  a 
conexão cair)
  em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
  Parâmetros: nenhum
  Retorno: nenhum
*/
void reconnectMQTT()
{
  while (!MQTT.connected())
  {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT))
    {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe(TOPICO_SUBSCRIBE_BOMBA_AGUA);  //topicos inscritos para receber informações
    }
    else
    {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Havera nova tentativa de conexao em 2s");
      delay(2000);
    }
  }  
}
/* Função: verifica o estado das conexões WiFI e ao broker MQTT.
           Em caso de desconexão (qualquer uma das duas), a conexão
           é refeita.
   Parâmetros: nenhum Retorno: nenhum
*/
void VerificaConexoesWiFIEMQTT(void)
{
  if (!MQTT.connected())
    reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
    reconnectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}
/* Função: reconecta-se ao WiFi
   Parâmetros: nenhum
   Retorno: nenhum
*/
void reconnectWiFi(void)
{
  //se já está conectado a rede WI-FI, nada é feito.
  //Caso contrário, são efetuadas tentativas de conexão
  if (WiFi.status() == WL_CONNECTED)
    return;
  WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(SSID);
  Serial.println("\nIP obtido: ");
  Serial.println(WiFi.localIP());
}

void ultrassonico(){
  char distancia_str[13] = {0};
  distance = ultrasonic.read();
  Serial.print("Distance in CM: ");
  Serial.println(distance);
  delay(100);

  if(distance < 100){
    sprintf(distancia_str, "Nível de água baixo");
    //led RGB  vermelho
     setColor(255,0,0);
  } else if(distance > 100 && distance < 200){
    sprintf(distancia_str, "Nível de água médio");
     setColor(0,0,255);
  }else{
    sprintf(distancia_str, "Nível de água bom");
    setColor(0,255,0);
  }

  /*  Publica a temperatura */
  MQTT.publish(TOPICO_PUBLISH_NIVEL_AGUA, distancia_str);
}


void sensorDeUmidade ()
{
  char umidade_solo_str[13] = {0};
  int valorSensorUmidadeSolo = analogRead(sensorUmidadeSolo);    // fazendo a leitura do Sensor de umidade do solo
  Serial.print(" Sensor de umidade do solo = ");                 // imprime mensagem
  Serial.println(valorSensorUmidadeSolo);                          // imprime o valor do sensor de umidade do solo

  Serial.print("Umidade em porcentagem: ");
  valorPorcentagem = 100 - ((valorSensorUmidadeSolo * 100) / 4095);
  Serial.print(valorPorcentagem);
  Serial.println("%");

  //converte a temperatura e a umidade lida para string
  sprintf(umidade_solo_str, "%d%%", valorPorcentagem);
 
  Serial.println(umidade_solo_str);

  /*  Publica a temperatura */
  MQTT.publish(TOPICO_PUBLISH_UMIDADE_SOLO, umidade_solo_str);
}

void valorUmidadeETemperatura(){
  char temperatura_str[13] = {0};
  char umidade_str[13] = {0};
  long h = dht.readHumidity();
  long t = dht.readTemperature();

  if (isnan(t)||isnan(h)) {
    Serial.println("Failed to read from DHT");
  } else {
    Serial.print("Temperatura: ");
    Serial.print(t);
    Serial.println("*C");
    Serial.print("Umidade: ");
    Serial.print(h);
    Serial.println("%");
  }

  //converte a temperatura e a umidade lida para string
  sprintf(temperatura_str, "%dC", t);
  sprintf(umidade_str, "%d%%", h);

  /*  Publica a temperatura */
  MQTT.publish(TOPICO_PUBLISH_TEMPERATURA, temperatura_str);
  MQTT.publish(TOPICO_PUBLISH_UMIDADE, umidade_str);

  delay(2000);
}

void setColor(int vermelho, int verde, int azul){
 digitalWrite(pinoR, vermelho); //DEFINE O BRILHO DO LED DE ACORDO COM O VALOR INFORMADO PELA VARIÁVEL 'vermelho'
 digitalWrite(pinoG, verde); //DEFINE O BRILHO DO LED DE ACORDO COM O VALOR INFORMADO PELA VARIÁVEL 'verde'
 digitalWrite(pinoB, azul); //DEFINE O BRILHO DO LED DE ACORDO COM O VALOR INFORMADO PELA VARIÁVEL 'azul'
}

void setup() {
  delay(1000);
  Serial.println("Disciplina IoT: acesso a nuvem via ESP32");
  delay(1000);
  initWiFi();
  initMQTT();
  pinMode(sensorUmidadeSolo, INPUT);
  dht.begin();
  pinMode(pinoR, OUTPUT);
  pinMode(pinoG, OUTPUT);
  pinMode(pinoB, OUTPUT);
  pinMode(bombinha, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  VerificaConexoesWiFIEMQTT();
  valorUmidadeETemperatura();
  sensorDeUmidade();
  MQTT.loop();
  delay(500);
}
