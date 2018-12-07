#include <WiFi.h>
#include <IOXhop_FirebaseESP32.h>

// Set these to run example.
#define FIREBASE_HOST "curupira-8c123.firebaseio.com/"
#define FIREBASE_AUTH "H0OoYSzOFPOjXkxqTHfEgW3qPqfu8rV1hv3C0NwC"
#define WIFI_SSID "MDEO"
#define WIFI_PASSWORD "matcig123"

#define botao 8
#define buzzer 7
#define verde 2
#define amarelo 3
#define vermelho 4
#define GSM_Botao 15

int risco = 0;/*RISCO
                0 - Baixo
                1 - Médio
                2 - Alto
                3 - Muito alto
                4 - Extremo */
int flag_botao = 0;
char mens[160];
int tabela_int[3][3];//Id+gás+luz
float tabela_float[3][5];//+temperatura+umidade+velDoVento+pluviômetro+fwi
int vel_GSM;
bool fumaca = false;
int fwi_max = 0;

void setup() {
  Serial.begin(115200);
  pinMode(verde, OUTPUT);
  pinMode(amarelo, OUTPUT);
  pinMode(vermelho, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(botao,INPUT);
  
  pinMode(GSM_Botao, OUTPUT);
  GSM_begin();
  
  Serial.println("Velocidade do GSM: "+ String(vel_GSM));

  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("CONECTADO: ");
  Serial.println(WiFi.localIP());
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

int GSM_DEL_MSG(int number_msg){//deleta a mensagem cujo numero é passado
  char a0=0, a1=0, a2=0, a3=0, a4=0;
  delay(1000);
  Serial2.begin(vel_GSM);
  Serial2.write("AT+CMGD=");
  Serial2.write(number_msg+48);
  Serial2.write("\r\n");
  while((a1 != 'O' || a0 != 'K') && (a4 != 'E' || a3 != 'R' || a2 != 'R' || a1 != 'O' || a0 != 'R')){        
    if(Serial2.available()){
      a4= a3;
      a3= a2;
      a2= a1;
      a1= a0;
      a0 = Serial2.read() ;      
     // Serial.print(a0);
    }
  }
}
int GSM_READ_MSG(char *msg){ //retorna por referencia a 1º mensagem, e retorna o numero da mensagem lida (se retornar 0 é pq não tem mensagem)
  delay(1000);
  Serial2.begin(vel_GSM);
  Serial2.write("AT+CMGL=\"ALL\"\r\n");
  char a0=0, a1=0, a2=0, a3=0, a4=0;
  int count=0;
  int c2 = 0;
  int i = 0;
  int number_msg=0;
  while((a1 != 'O' || a0 != 'K') && (a4 != 'E' || a3 != 'R' || a2 != 'R' || a1 != 'O' || a0 != 'R')){        
    if(Serial2.available()){
      a4= a3;
      a3= a2;
      a2= a1;
      a1= a0;
      a0 = Serial2.read() ;      
     // Serial.print(a0);
      if(a0 == '\"'){
        count++;
      }
      if(count == 2 && a0 >= 48 && a0 <= 58)
        number_msg = a0 -48;
      if(count == 10){
        if(a0 == 13 || a0 == 10)
          c2++;
        else
          if(c2 == 2)          
            msg[i++] = a0;              
      }
    }
  }
  msg[i] = 0;
  if(count >= 10)
    return number_msg;
  else
    return 0;
}
int GSM_begin(){
  int teste = GSM_GET_SPEED();
  Serial.println("GSM_begin>GET_SPEED()");
  if (teste == 0){//Se está desligado
    GSM_ON_OFF();
  }
  else{
   delay(1000);
   return teste;
  }
  delay(1000); 
  return GSM_GET_SPEED();
}
int GSM_GET_SPEED(){//retorna a velocidade de comunicação do SIM900

  int vel[10] = {300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 74800, 115200};
  int i, j;
  char AT_resp[] = {65, 84, 13, 10, 13, 10, 79, 75, 13, 10};
  char str[15];
  for(i = 0; i < 10; i++){ 
    j = 0;
    Serial2.begin(vel[i]);
    if(GSM_AT()){
     
      vel_GSM = vel[i];
      return vel[i];
    }
  }
  return 0;
}
void GSM_ON_OFF(){//Altera o estado do botão do SIM900
  digitalWrite(15, HIGH);
  delay(2000);
  digitalWrite(15, LOW);    
  Serial.println("GSM_ON_OFF");
}
int GSM_AT(){//1 OK, 0 N respondeu 
  int i = 0;
  char AT_resp[] = {65, 84, 13, 10, 13, 10, 79, 75, 13, 10};
  char str[15];   
  Serial2.write("AT\r\n");
  delay(300);
  while(Serial2.available()){    
    if(Serial2.read() != AT_resp[i])
      break;      
    i++;
    delay(10);
  }  
  if(i == 10) 
    return 1;  
  return 0;   
}
void GSM_OFF(){//DEsliga o SIM900
  Serial2.begin(vel_GSM);
  Serial.print("Desligando GSM: ");
  if(GSM_AT()){
     GSM_ON_OFF(); 
     Serial.println("Desliguei.");     
  }else{
    Serial.println("Já está desligado.");
  }
}
void GSM_ON(){//Liga o SIM900
  Serial2.begin(vel_GSM);
  Serial.print("Ligando GSM: ");
  if(!GSM_AT()){
     GSM_ON_OFF(); 
     Serial.println("Liguei.");     
  }else{
    Serial.println("Já está Ligado.");
  }
}
int GSM_SEND_MSG(char* numero, char* mensagem){//Envia a mensagem "mensagem" para  o numero "numero" 
  if(GSM_SINAL()){//Se existir sinal
    
    Serial2.begin(vel_GSM);   
    Serial2.write("AT+CMGS=\"");
    Serial2.write(numero);
    Serial2.write("\"\r\n");   
    delay(700);
    Serial2.write(mensagem);    
    Serial2.write((char)26);
    delay(300);
    while(Serial2.available()){    
        Serial.print(char(Serial2.read()));       
    }  
    Serial.println();
    return 1;
  }
  return 0;
}
int GSM_SINAL(){//retorna o sinal da rede do SIM900 (não é mto confiavel, mas é o que temos)
  int i;
  int sinal=0;
  char si;
  char ch;
  Serial2.begin(vel_GSM);
  delay(1000);
  Serial2.write("AT+CSQ\r\n");
  delay(200);   
  for(i=0;Serial2.available();i++){
    ch = Serial2.read();
    Serial.print(ch);
    delay(10);
    if(i==16)
      sinal =  ch-48;    
    if(i==17 && ch != ',')    
      sinal = sinal*10+ch-48;    
  }  
  Serial.println("Sinal = "+String(sinal));
  if(sinal > 3 && sinal < 16){
    delay(1000);
    return 1;
  }    
  else
    return 0;  
}
void leds(){
    switch(risco)
  {
    case 0:
          digitalWrite(verde, HIGH);
          digitalWrite(amarelo, LOW);
          digitalWrite(vermelho, LOW);
          break;
    case 1:
          digitalWrite(verde, HIGH);
          digitalWrite(amarelo, HIGH);
          digitalWrite(vermelho, LOW);
          break;
    case 2:
          digitalWrite(verde, LOW);
          digitalWrite(amarelo, HIGH);
          digitalWrite(vermelho, HIGH);
          break;
    case 3:
          digitalWrite(verde, LOW);
          digitalWrite(amarelo, HIGH);
          digitalWrite(vermelho, HIGH);
          break;
    case 4:
          digitalWrite(verde, LOW);
          digitalWrite(amarelo, LOW);
          digitalWrite(vermelho, HIGH);
          break;   
  }
}


int processaMsg(){
  /*RETORNA OS VALORES por "referencia" (só que global...): 
    Id, gás e luz EM tabela_int;
    temperatura, umidade, velDoVento, pluviômetro e fwi EM tabela_float
    NESTA ORDEM...
    VALOR DE RETORNO DA FUNÇAO:
    Quantidade de nós presentes na mensagem;
   */
  return 0;
}

void loop() {
  if(GSM_READ_MSG(mens))
  {
      fwi_max = 0;
      fumaca = false;
      int j = processaMsg();//retorna a quantidede de nós recebidos
      for(int i = 0; i > j; i++){
        Firebase.setInt  ("node_"+String(tabela_int[i][0])+"/fumaca", tabela_int[i][1]);
        Firebase.setInt  ("node_"+String(tabela_int[i][0])+"/luminosidade", tabela_int[i][2]);
        Firebase.setFloat("node_"+String(tabela_int[i][0])+"/temperatura", tabela_float[i][0]);        
        Firebase.setFloat("node_"+String(tabela_int[i][0])+"/umidade", tabela_float[i][1]);
        Firebase.setFloat("node_"+String(tabela_int[i][0])+"/vento", tabela_float[i][2]);
        Firebase.setFloat("node_"+String(tabela_int[i][0])+"/pluviosidade", tabela_float[i][3]);
        Firebase.setFloat("node_"+String(tabela_int[i][0])+"/risco", tabela_float[i][4]);
        if(fwi_max < tabela_float[i][4])
          fwi_max = tabela_float[i][4];   
        if(tabela_int[i][1])
          fumaca = true;    
      } 
      if(fwi_max >= 0  && fwi_max < 11.2) //Baixo
        risco = 0;
      else if(fwi_max >=11.2  && fwi_max < 21.2)//Moderado
        risco = 1;
      else if(fwi_max >=21.3  && fwi_max < 38.0)//Alto
        risco = 2;
      else if(fwi_max >=38.0  && fwi_max < 50.0)//Critico
        risco = 3;
      else if(fwi_max >=38.0)   //Extremo
        risco = 4;
  }
  if(risco >= 2 || fumaca == true){//Risco alto, critico ou extremo -> Liga o Alarme
    if(digitalRead(botao)==HIGH){
      flag_botao = 1;
      digitalWrite(buzzer, LOW);//desliga buzzer
    }else if(flag_botao == 0){
      digitalWrite(buzzer, HIGH);//liga buzzer
    }
  }else
    flag_botao = 0;
  leds();  
}
