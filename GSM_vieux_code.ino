#include <GPRS_Shield_Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#define PIN_TX    2
#define PIN_RX    3
#define BAUDRATE  9600

#include <dht11.h> 
dht11 DHT11; 
#define DHT11PIN 4 


#define MESSAGE_LENGTH 160
char message[MESSAGE_LENGTH];
int messageIndex = 0;

char phone[16];
char datetime[24];

GPRS gprsTest(PIN_TX,PIN_RX,BAUDRATE);

char* privateTelephoneNumber[]={"+33781223900","+33682784555","END"};
//char* privateTelephoneNumber[]={"+33781223909","+33682784555","END"};

char* SMScommand[]={"temperature", "arret", "hors-gel", "confort","?","END"};
//penser à les recopier ds le switch

String mdp="mdp";
int tailleMdp=3;

const int relai_1=6,relai_2=7;
const int led=13;

void setup() {  
  Serial.begin(9600);
  
  pinMode(relai_1,OUTPUT);
  pinMode(relai_2,OUTPUT);
  pinMode(led,OUTPUT);
  while(!gprsTest.init()) {
      Serial.print("initialisation error\r\n");
      digitalWrite(led,HIGH);
      delay(200);
      digitalWrite(led,LOW);
      delay(200);
      digitalWrite(led,HIGH);
      delay(200);
      digitalWrite(led,LOW);
      delay(200);
      digitalWrite(led,HIGH);
      delay(200);
  }
  delay(3000);  
  Serial.println("initialisation OK");
  digitalWrite(led,HIGH);
}

void loop() {
   messageIndex = gprsTest.isSMSunread();
   if (messageIndex > 0) {
      gprsTest.readSMS(messageIndex, message, MESSAGE_LENGTH, phone, datetime);           
      gprsTest.deleteSMS(messageIndex);
      
      Serial.println("nouveau message... Check en cours...");      

      int returnCPNE=checkPhoneNumberExist(phone,message);
      if(returnCPNE!=0) {
        String messageS=message;
        if(returnCPNE==2){
          messageS=messageS.substring(tailleMdp);
        }
                
        Serial.print("numero entrant : ");
        Serial.println(phone); 

        if(checkOrExecCommand(messageS,phone,1)==false){
          Serial.print("command undefined : ");
          Serial.println(messageS);
          gprsTest.sendSMS(phone,"commande indefini");
          Serial.print("erreur envoye à : ");
          Serial.println(phone);
        }
        else{
          Serial.print("commande : ");
          Serial.println(messageS);

          if(checkOrExecCommand(messageS,phone,2)){
            gprsTest.sendSMS(phone,"mode execute sans erreur");
          }
          
          Serial.print("message recu : ");
          Serial.println(messageS);   
        }
        
        /*Serial.print("Datetime: ");
        Serial.println(datetime);  */
      }
   }
}

//0 telephone et mdp incorrect
//1 reussite
//2 mdp reussit

int checkPhoneNumberExist(String nb, String msg){
  int z=0;
  bool PTNNotfinish=true;
  
  while(PTNNotfinish){
    if(privateTelephoneNumber[z]=="END"){
      Serial.println("max tableau OK 1 ");
      PTNNotfinish=false;
    }
    else
      z++;
  }
        
  for(int b=0;b<z;b++){
    if(nb==privateTelephoneNumber[b])
      return 1;
  }
  if(msg.substring(0,tailleMdp)==mdp){
    Serial.println("mdp OK");
    return 2;
  }
  else{
    Serial.print("numero inexistant et mdp incorrect, mdp testé : ");
    Serial.println(msg.substring(0,50));

    return 0;
  }
}
//mode 1 =check command, mode 2 = exec command
bool checkOrExecCommand(String msg,String telNB,int mode){
  int z=0;
  bool SMScommandNotFinish=true;
  
  while(SMScommandNotFinish){
    if(SMScommand[z]=="END"){
      Serial.println("max tableau OK 2 ");
      SMScommandNotFinish=false;
    }
    else
      z++;
  }
        
  for(int b=0;b<z;b++){
    if(msg==SMScommand[b]){
      if(mode==1)
        return true;
        
      else if(mode==2){
                  
        if(msg== "temperature"){


          int chk = DHT11.read(DHT11PIN); // Lecture du capteur
        
          Serial.print("Etat du capteur: ");
        
          switch (chk){
            case DHTLIB_OK: 
                        
            break;
            case DHTLIB_ERROR_CHECKSUM: 
                        return false;
            break;
            case DHTLIB_ERROR_TIMEOUT: 
                        return false;
            break;
            default: 
                        return false;
            break;
          }

          char stringInChar[50]; 
        
          float humi=DHT11.humidity;
          float temp=DHT11.temperature;

          String tempS=ConvertionFloatToString(temp);
          String humiS=ConvertionFloatToString(humi);

          String temperature="temperature : "+tempS+"C, humidite : "+humiS+"/100";

          for(int compteur=0;compteur<temperature.length();compteur++){
            stringInChar[compteur]=temperature[compteur];
          }
           
          Serial.println(stringInChar);
          gprsTest.sendSMS(phone,stringInChar);
          return true;
        }
        
        else if(msg=="arret"){
          digitalWrite(relai_1,LOW);
          digitalWrite(relai_2,LOW);
          gprsTest.sendSMS(phone,"mode arret active");
          return true;
        }
        else if(msg=="hors-gel"){
          digitalWrite(relai_1,HIGH);
          digitalWrite(relai_2,HIGH);
          gprsTest.sendSMS(phone,"mode hors gel active");
          return true;
        }
        else if(msg=="confort"){
          digitalWrite(relai_1,HIGH);
          digitalWrite(relai_2,LOW);
          gprsTest.sendSMS(phone,"mode confort active");
          return true;
        }
        else if(msg=="?"){
          gprsTest.sendSMS(phone,"commande possible : temperature,arret,hors-gel,confort,?");
          return true;
        }
        else
          return false;
      }
    }
  }
  return false;
}

String ConvertionFloatToString(float Val){
 char buffer[4];
 String str = dtostrf(Val, 2, 2, buffer);
 return str;
}

//
//supprimer msg carte sim !!!

