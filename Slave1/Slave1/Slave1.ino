#include <ESP8266WiFi.h>
#include <dht.h>
dht DHT;
#define DHT11_PIN D1

extern "C" {
#include <espnow.h>  
#include <user_interface.h>
}
                                                        
uint8_t mac[] = {0x36,0x33,0x33,0x33,0x33,0x33};       // Enedereço da própria placa Slave1 que vai enviar os dados
uint8_t remoteMac[] = {0x36,0x34,0x34,0x34,0x34,0x34}; //Endereço do Slave2 que vai receber os dados
#define WIFI_CHANNEL 6                                 //Define o canal de comunicação 

int sleep = 1;                                         //Variavel auxiliar para modo sleep 

void initVariant(){                     //Função para configurar o modo Wifi 
  WiFi.mode (WIFI_AP);                  // Estabelece o modo Wifi como Acesses Point 
  wifi_set_macaddr(SOFTAP_IF, &mac[0]); //Selecino o Macadress de indice 0 (0x36)    
}

//========================================

//Estrutura de dados para enviar as informações
struct __attribute__((packed)) DATASRUCT {  // Todos os dados na estrutura serão enviados para Slave 1
   char receivedData [32];                  // Armazena dados recebidos em 32 bits
   char botao;                              // Variavel zero ou um para o estado do botao                 
   float temp;                              // Variavel de temperatura no ponto 1
   float temp2;                             // Variavel de temperatura no ponto 2
   float temp_total;                        // Variavel com a media da temperatura 
   float hum;                               // Variavel de umidade no ponto 1
   float hum2;                              // Variavel de umidade no ponto 2 
   float hum_total;                         // Variavel com a media das umidades 
   unsigned long time;    
} DataStruct;  

DATASRUCT replyData;                        //Estrutura de dados para Callback 

//========================================

void setup() {
  Serial.begin(115200); Serial.println();
  pinMode(14,INPUT); // Entada do botao D5
  pinMode(2,OUTPUT); //Led interno
  digitalWrite(2,LOW); 

  Serial.println("Iniciando o ESPNow Slave1");

      if (esp_now_init()!=0){
        Serial.println ("**A iniciação do Slave1 falhou");
        while(true) {};
      }

      esp_now_set_self_role(ESP_NOW_ROLE_COMBO);                              // Está usaando o combo mode 
      esp_now_add_peer(remoteMac, ESP_NOW_ROLE_COMBO, WIFI_CHANNEL, NULL, 0); //Adiciona o macadress a rede 
      esp_now_register_recv_cb(receiveCallBackFunction);                      // Confere se os dados chegaram ao destino
      strcpy(replyData.receivedData, "Os dados foram recebidos com sucesso");
      Serial.print("Essa menssagem vai ser enviada ao controlador "); Serial.println(replyData.receivedData);
      Serial.println("Esperando por uma conexão"); 
}

void loop() {
  
  esp_now_register_send_cb(sendstatus);
  delay(2000);
  sendData(); 
  
  if(DataStruct.botao == LOW){
    digitalWrite(2,LOW); 
  }
    if(DataStruct.botao == HIGH){
    digitalWrite(2,HIGH); 
  }

  if (!sleep){                   //Após receber que os dados foram enviados com sucesso entra em modo sleep
    ESP.deepSleep(1 * 60000000); //Dorme por 1 Minuto (Deep-Sleep em Micro segundos)
  }

}

//=========================================
//Definição da função receiveCallBackFunction
void receiveCallBackFunction (uint8_t *senderMac, uint8_t *incomingData, uint8_t len) { //(Macadress do slave 1, Resposta do Slave 1, comprimento dos dados)
  memcpy(&DataStruct, incomingData, sizeof(DataStruct)); //Salva os dados da estrutura para enviar ao Slave2
  uint8_t dataReceived [sizeof(DataStruct)];
  memcpy(dataReceived, &DataStruct, sizeof(DataStruct));
  sendReply(senderMac);                                 //Manda resposta ao controlador
  sendSlaveTwo(dataReceived, remoteMac);                //Manda os dados pro Slave2 
}

//Definição da função sendReply 
void sendReply(uint8_t *macAddr){
   esp_now_add_peer(macAddr, ESP_NOW_ROLE_COMBO, WIFI_CHANNEL, NULL, 0); //Adiciona o controlador como um peer
   uint8_t SendReply[sizeof(replyData)];
   memcpy(SendReply, &replyData, sizeof(replyData));
   esp_now_send(macAddr, SendReply, sizeof(replyData));                 //Envia a mensagem pro controlador
   Serial.println("sendReply enviou estes dados");                      //Mostra na tela quais dados foram enviados
   esp_now_del_peer(macAddr);    
}

//Definição da função sendSlaveTwo
void sendSlaveTwo(uint8_t *dataReceived, uint8_t *remoteMac){           //Recebe o endereço do Slave2 e os Dados do controlador
  esp_now_send(remoteMac, dataReceived, sizeof (DataStruct));           //Envia os dados ao endereço recebido do Slave2
  Serial.println ("Os dados foram enviados ao Slave2");
}
 void sendstatus(uint8_t *remoteMac, uint8_t sendStatus){
  Serial.printf ("Envio terminado, status = %i\n", sendStatus);         //Se os dados foram enviados para Slave1 e para o conrolador na tela aparecerá "00"
  sleep=sendStatus;                                                     //Avisa se pode entrar no modo sleep                                                                          
  Serial.printf("Modo sleep: %i\n",sleep); 
 }                                                                      

 //Definição da função de leitura do sensor 
void sensorRead (){
  int chk = DHT.read11(DHT11_PIN);
  if(chk == DHTLIB_OK){                //Confere se a leitura funcionou 
   DataStruct.temp2 = DHT.temperature; //Guarda a medição da temperatura na estrutura de dados
   DataStruct.hum2 = DHT.humidity;     //Guarda a medição da umidade na estrutura de dados
   Serial.print("Temperatura no ponto 2: ");
   Serial.println(DataStruct.temp2);
   Serial.print("Umidade no ponto 2: ");
   Serial.println(DataStruct.hum2);
}
}

//Definição da função que calcula a média das umidades e temperatura 
void sensorMedia(){
  DataStruct.temp_total = (DataStruct.temp2+DataStruct.temp)/2;
  DataStruct.hum_total = (DataStruct.hum2+DataStruct.hum)/2;  
}

//Definição da função que envia as novas leituras 
void sendData (){
  sensorRead();  // Faz leitura do ponto 2 para ser enviado ao Slave2
  sensorMedia(); // Faz a media das leituras do ponto 1 e do ponto 2 para ser enviado ao Slave2
  uint8_t DataStructSend[sizeof(DataStruct)];
  memcpy(DataStructSend, &DataStruct, sizeof (DataStruct));
  esp_now_send(remoteMac, DataStructSend, sizeof(DataStruct));
  delay(30); 
}
