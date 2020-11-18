#include <ESP8266WiFi.h>
#include <dht.h>
dht DHT;
#define DHT11_PIN D1

extern "C" {
#include <espnow.h>  
#include <user_interface.h>
#include <stdlib.h>
}

uint8_t mac [] = {0x36, 0x34, 0x34, 0x34, 0x34, 0x34}; //O endereço do Slave2, a prórpia placa



//=======================================

#define WIFI_CHANNEL 6 

void initVariant(){                     //Função para configurar o modo Wifi 
  WiFi.mode (WIFI_AP);                  // Estabelece o modo Wifi como Acesses Point 
  wifi_set_macaddr(SOFTAP_IF, &mac[0]); //Selecino o Macadress de indice 0 (0x36) 
}

//=======================================

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
  
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  pinMode(14,INPUT); //D5
  pinMode(2,OUTPUT); //Led interno
  digitalWrite(2,LOW); 
  
  if(esp_now_init()!=0){ //Confere se a iniciação da placa está normal 
    Serial.println("A iniciação do Slave2 falhou");
    while(true){};       //Mantém a placa em loop para o usuário reniciar a placa 
  }
   esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
   esp_now_register_recv_cb(receiveFunction);// Confere os dados recebidos
} 
//====================================
   


void loop() {

esp_now_register_recv_cb(receiveFunction); //Recebe os dados 
indiceCalor();                             //Calcula o indice de calor com os dados recebidos 

//Coloca na tela os valores recebidos 
if((DataStruct.hum>0)&&(DataStruct.temp>0)){
  if((DataStruct.hum2>0)&&(DataStruct.temp2>0)){

   
  //Serial.print("Temperatura no ponto 1: ");
   Serial.print(DataStruct.temp);
   Serial.print("  ");
   //Serial.print("  Umidade no ponto 1: ");
   Serial.print(DataStruct.hum);
   Serial.print("  ");

   //Serial.print("  Temperatura no ponto 2: ");
   Serial.print(DataStruct.temp2);
   Serial.print("  ");
   //Serial.print("  Umidade no ponto 2: ");
   Serial.print(DataStruct.hum2);
   Serial.print("  ");

   //Serial.print("  Temperatura média: ");
   Serial.print(DataStruct.temp_total);
    Serial.print("  ");
   //Serial.print("  Umidade média: ");
   Serial.print(DataStruct.hum_total);
   Serial.print("  ");
   //Serial.print("  Sensação térmica: ");
   Serial.println(indiceCalor());
   //Espera novos dados
   
}
}


//Botão de teste de funcionamento
if(DataStruct.botao == LOW){
  digitalWrite(2,LOW); 
  }
if(DataStruct.botao == HIGH){
  digitalWrite(2,HIGH); 
  }

delay (1000); 
  

}
//====================================
//Declaração da função de contagem de tempo
int Tempo(int minuto){                 

  delay((minuto * 60000000));
  
}
//Declaração da função que calcula o Indice de Calor
float indiceCalor(){
  float TC = DataStruct.temp_total;
  float TF = (TC+32)* 1.8; 
  float UR = DataStruct.hum_total/100;
  double IF,IC;
  float TF2 = pow(TF,2);
  float UR2 = pow(UR,2);
 
  IF = ((2.04901523*TF)+(10.14333127*UR)+(-0.22475541*TF*UR)+(-0.00683783*TF2)+(-0.05481717*UR2)+(0.00122874*TF2*UR)+(0.00085282*TF*UR2)+(-0.00000199*TF2*UR2))-42,379;                
  IC = ((5.0/9.0)*(IF-32.0));
  return IC -2; 
  
}
//Declaração da função que recebe os dados e os coloca na estrutura de dados 
void receiveFunction (uint8_t *senderMac, uint8_t *incomingData, uint8_t len){ //(Macadress do slave 1, Resposta do Slave 1, comprimento dos dados)
   memcpy(&DataStruct, incomingData, sizeof(DataStruct)); //Salva os dados da estrutura para enviar ao Slave2
   uint8_t dataReceived [sizeof(DataStruct)];
   memcpy(dataReceived, &DataStruct, sizeof(DataStruct));
  
}
