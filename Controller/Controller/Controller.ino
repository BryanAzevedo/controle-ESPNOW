#include <ESP8266WiFi.h>
#include <dht.h>
dht DHT;
#define DHT11_PIN D1  //Estabelece D1 como o pino pra receber as informações do sensor 

extern "C" {
#include <espnow.h>  
#include <user_interface.h>
}
 uint8_t remoteMac [] = {0x36,0x33,0x33,0x33,0x33,0x33}; //Endereço do Slave 1 que vai receber os dados
 uint8_t mac [] = {0x36,0x35,0x35,0x35,0x35,0x35};      // Enedereço da própria placa contolador que está enviando os dados

 #define WIFI_CHANNEL 6 //Define o canal de comunicação 
 
 int sleep = 1; //Variavel auxiliar para modo sleep  

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
 
//=======================================

void initVariant(){                     //Função para configurar o modo Wifi 
  WiFi.mode (WIFI_AP);                  // Estabelece o modo Wifi como Acesses Point 
  wifi_set_macaddr(SOFTAP_IF, &mac[0]); //Selecino o Macadress de indice 0 (0x36)    
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin (115200); Serial.println();
  Serial.println ("Iniciando o controlador");
  pinMode(14,INPUT); //D5
  pinMode(2,OUTPUT); //Led interno
  digitalWrite(2,LOW); 

  Serial.printf ("Slave mac: %02x%02x%02x%02x%02x%02x", remoteMac[0], remoteMac[1], remoteMac[2], remoteMac[3], remoteMac[4], remoteMac[5]);
  Serial.printf (", channel: %i\n", WIFI_CHANNEL);

   InitESPNow (); //Inicia o ESPnow 
   
  Serial.println ("Configurações iniciais terminadas"); 
  
}

//=======================================
void InitESPNow () {
  
  if (esp_now_init () == 0){
    Serial.println("ESPNow inciou com sucesso");
  }
  else {
    Serial.println("A iniciação do ESPNow falhou");
    ESP.restart();
  }
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO); // Está usaando o combo mode 
  esp_now_add_peer(remoteMac, ESP_NOW_ROLE_COMBO, WIFI_CHANNEL, NULL, 0); //Adiciona o macadress a rede 
  esp_now_register_send_cb(sendstatus);  // Confere se os dados saíram da placa
  esp_now_register_recv_cb(receiveCallBackFunction);// Confere se os dados chegaram ao destino 
  
}

void loop() {
  
  sendData();                          //Função que envia os dados constantemente
  
  if(digitalRead(14) == LOW){         // Compara o valor e envia Baixo para Slave1
    DataStruct.botao = LOW; 
    digitalWrite(2,LOW);     
    }
    
   if(digitalRead(14) == HIGH) {      //Compara o valor e envia Alto para Slave1 
    DataStruct.botao = HIGH; 
    digitalWrite(2,HIGH);  
    }  
  delay(500);

  if(!sleep){                        //Após receber que os dados foram enviados com sucesso entra em modo sleep
    ESP.deepSleep(1 * 60000000);     //Dorme por 1 Minuto (Deep-Sleep em Micro segundos) 
  }


}
//=======================================

//Definição da função sendData
void sendData (){
  DataStruct.botao = digitalRead(D5); // Faz leitura do pino 

//Faz a leitura e envio da temperatura e umidade

  int chk = DHT.read11(DHT11_PIN);
  if(chk == DHTLIB_OK){               //Confere se a leitura funcionou 
   DataStruct.temp = DHT.temperature; //Guarda a medição da temperatura na estrutura de dados
   DataStruct.hum = DHT.humidity;     //Guarda a medição da umidade na estrutura de dados
   Serial.print("Temperatura: ");
   Serial.println(DataStruct.temp);
   Serial.print("Umidade: ");
   Serial.println(DataStruct.hum);
  }

  uint8_t DataStructSend[sizeof(DataStruct)];
  memcpy(DataStructSend, &DataStruct, sizeof (DataStruct));
  esp_now_send(remoteMac, DataStructSend, sizeof(DataStruct));
  Serial.print ("O estado do botao e: "); Serial.println(DataStruct.botao);
  delay (30);
 
}

//=======================================

//Definição das funções de check 
void sendstatus (uint8_t* remoteMac, uint8_t sendStatus){       // Definição da função sendstatus que confere se os dados foram enviados
  Serial. printf ("Envio terminado, status = %i\n", sendStatus);// Se os dados saíram da placa mostra "0" se não mostra "1"
  sleep=sendStatus;                                             // Avisa se pode entrar no modo sleep
  Serial.printf("Modo sleep: %i\n",sleep);  
}

void receiveCallBackFunction (uint8_t *senderMac, uint8_t *incomingData, uint8_t len) { //(Macadress do slave 1, Resposta do Slave 1, comprimento dos dados    ) 
   memcpy(&DataStruct, incomingData, sizeof(DataStruct));
   Serial.print("Resposta do Slave1 ");
   Serial.print("MacAddr: "); 
   for (byte n = 0; n < 6; n++){
     Serial.print (senderMac[n], HEX); 
   }
   Serial.print("Comprimento da Menssagem: ");
   Serial.println(len);
   Serial.print("Mensagem recebida do Slave1: ");
   Serial.println(DataStruct.receivedData); 
}












  
