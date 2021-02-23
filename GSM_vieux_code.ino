#include <SoftwareSerial.h>
#include <dht.h>

#define SERIALPCBAUDRATE 19200
#define SERIALGSMBAUDRATE 19200
#define POWERKEY 9

#define PINRELAI1 5
#define PINRELAI2 6

#define MAXBAUDRATEMSG 128

#define DHT11_PIN 2
dht DHT;

HardwareSerial &gsm = Serial1;

//0=ARRET, 1=HORS-GEL, 2=CONFORT
int arduinoState = 0;

const boolean DEBUG = true;

const int COMMANDESIZE = 7;
const char COMMANDE[][9] = {"TEMP","ARRET", "HORS-GEL", "CONFORT","??","STATUS","REBOOT"};

const int NUMBERSIZE = 3;      //FIRST NUMBER IS RECEIVING SMS AT EVERY START
const char KNOWNNUMBER[][13] = {"+***********","+***********","+***********"};
//12

const char OKRETURN[3] = "OK";

int len(char inText[]){
  int length = 0;

  while (inText[length] != 0)
    length++;

  return length;
}

/* ### SEND COMMAND TO GSM + READ RESPONSE ### */
void sendCMD(char commandeReturnString[], char cmd[], const int timeForReading, const int maxSizeString){
  if(cmd=="ctrlz")
    gsm.println((char)26);
  else
    gsm.println(cmd);

  //START READING RESOPONSE
  if(DEBUG)
    Serial.println("START READ SERIAL");

  long int timeBefore=millis();

  int counterCommande=0;

  while( (timeBefore+timeForReading) > millis()){
    while (gsm.available() && counterCommande<maxSizeString){
      counterCommande++;
      char c = gsm.read();
      if(commandeReturnString!=0)
        commandeReturnString[counterCommande]=c;
    }
  }
  for(int a=counterCommande; a<maxSizeString; a++)
    commandeReturnString[counterCommande]=NULL;

 /* if(DEBUG)
    Serial.println("commande return : "+commandeReturnString);*/
}

int indexOf(char str[], const char toFind[]){
  delay(50);

  int lengthSTRtofind=strlen(toFind);

  for(int a=0; a<strlen(str)-lengthSTRtofind; a++){
    boolean find=true;
    int b;
    for(b=0; b<lengthSTRtofind; b++)
      if(str[a+b]!=toFind[b])
        find=false;
    if(find==true)
      return a;
  }
  return -1;
}

/* ### FIND STRING IN STRING => boolean ### */
boolean findStr(char str[], char const toFind[]){
  //toFind max length = 20;

  int posToFind=indexOf(str,toFind);
  if(posToFind>=0)
    return true;
  else
    return false;
}

/* ### SHUTDOWN GSM => EXPECT THAN GSM IS POWER ON ### */
boolean shutdown(){
  char commande[8]="AT+CPOF";

  char returnCMD[MAXBAUDRATEMSG];
  sendCMD(returnCMD,commande,2000, MAXBAUDRATEMSG);

  boolean isShutdown=findStr(returnCMD,OKRETURN);

  delay(10000);

  if(isShutdown)
    manage(0, 72, "shutdown => OK","");
  else
    manage(0, 85, "error while shutdown, expected is already shutdown","");
}

/* ### RESTART GSM ###*/
void restart(){
  delay(1000);
  do{
    shutdown();
    digitalWrite(POWERKEY, LOW);
    delay(1000);               
    digitalWrite(POWERKEY, HIGH);
    delay(15000);
  } while(!checkIfSerialRespond());
  delay(1000);
}

/* ### TRY TO COMMUNICATE WITH GSM ###*/
boolean checkIfSerialRespond(){
  char commande[3]="AT";

  char returnCMD[MAXBAUDRATEMSG];
  sendCMD(returnCMD,commande,2000,MAXBAUDRATEMSG);

  return findStr(returnCMD,"OK");
}

/* ### MANAGE ERROR ### */
void manage(int action, int lineNumber, char msg[], char tel[]){
  if(DEBUG){
    Serial.print(lineNumber);
    Serial.print(", ");
    Serial.println(msg);
  }
  if(action>0){
    switch(action){
      case 1:
        Serial.println("RESTART GSM");
        restart();
      break;
      case 2:
        Serial.print("SEND SMS TO : ");
        Serial.print(tel);
        Serial.print(", MSG : ");
        Serial.println(msg);
        sendSms(tel,msg);
      break;
      case 3:
        Serial.print("SEND SMS TO : ");
        Serial.print(tel);
        Serial.print(", MSG : ");
        Serial.print(msg);
        Serial.println("+ DELETE SMS & RESTART");
        sendSms(tel,msg);
        deleteAllSms();
        restart();
      break;
      case 4:
        Serial.println("DELETE SMS + restart");
        deleteAllSms();
        restart();
      break;
      case 5:
        sendSms(tel,msg);
        deleteAllSms();
      break;
      default:
        Serial.println("UNDEFINED ERROR NUMBER");
      break;
    }
  }
}
//https://www.circuitbasics.com/wp-content/uploads/2015/10/DHTLib.zip
/* ### CONVERT STRING TO ASCII DECIMAL (SMS ARE IN ASCII DECIMAL SO YOU NEED TO CONVERT YOUR COMMAND BEFORE COMPARING ###*/
/*char *convertStrToAsciiDec(char str){  // ONLY FOR UNICODE
  char returnStr[len(str)*3];
  //TODO CHECK IF LENGTH IS CORRSPONDING !!!
  for(int a=0; a<len(str); a++){
    returnStr+="00"+String(str[a],HEX);
  }
  returnStr.toUpperCase();
  return returnStr;
}*/

void *getTempHum(int &chk, int &hum, int &tem){
  chk = DHT.read11(DHT11_PIN);
  hum=(int)DHT.humidity;
  tem=(int)DHT.temperature;
}

/* ### EXECUTE COMMAND ### */
void executeCommand(char SMSreturn[], int commandeNB, int maxLength){
  //const String COMMANDE[COMMANDESIZE] = {"TEMP","ARRET", "HORS-GEL", "CONFORT","??","STATUS","REBOOT"};

  switch(commandeNB){
    case 0:
      Serial.println("code 0");

      int ack,hum,temp;
      getTempHum(ack, hum, temp);

      snprintf(SMSreturn, maxLength,"ACK : %d, HUM : %d, TEMP : %d",ack,hum,temp);

    break;
    case 1:
      Serial.println("code 1");
      digitalWrite(PINRELAI1,LOW);
      digitalWrite(PINRELAI2,LOW);

      arduinoState=0;

      snprintf(SMSreturn, maxLength,"MODE ARRET ACTIF");
    break;
    case 2:
      Serial.println("code 2");
      digitalWrite(PINRELAI1,HIGH);
      digitalWrite(PINRELAI2,HIGH);

      arduinoState=1;

      snprintf(SMSreturn, maxLength,"MODE HORS-GEL ACTIF");
    break;
    case 3:
      Serial.println("code 3");
      digitalWrite(PINRELAI1,HIGH);
      digitalWrite(PINRELAI2,LOW);

      arduinoState=2;

      snprintf(SMSreturn, maxLength,"MODE CONFORT ACTIF");
    break;
    case 4:
      Serial.println("code 4");
    /*  SMSreturn="COMMANDE : ";
      for(int commandeNb=0; commandeNb<COMMANDESIZE; commandeNb++){
        SMSreturn+=COMMANDE[commandeNb];
        if(commandeNb<COMMANDESIZE-1)
          SMSreturn+=", ";
      }*/

      //TODO CHANGE THAT TO BE DONE AUTOMATICALLY, PARCOURIR TABLEAU ET CONCATENER CHAINE DE CARACTERE
      snprintf(SMSreturn, maxLength,"TEMP,ARRET,HORS-GEL,CONFORT,??,STATUS,REBOOT");
    break;
    case 5:
      Serial.println("code 5");
      snprintf(SMSreturn, maxLength,"STATUS : %d",arduinoState);
    break;
    case 6:
      Serial.println("code 6");
      restart();
      snprintf(SMSreturn, maxLength,"GSM IS REBOOTED");
    break;
    default:
      snprintf(SMSreturn, maxLength,"COMMANDE NOT FOUND");
    break;
  }
}

/* ### CHECK IF PHONE NUMBER IS DECLARED ### */
boolean checkPhoneNumber(char undefinedNb[12]){
  //TODO CHECK IF SEND CHAR NUMBER IS GOOD 12 LENGTH
  //TODO : CHECK THAN PHONE NUMBER IS 12 CHAR (OR 13 FOR LAST DEFAULT VALUE \o)

  for(int phoneNB=0; phoneNB<NUMBERSIZE; phoneNB++){
    boolean returnState=true;

    for(int nb=0; nb<12; nb++)
      if(KNOWNNUMBER[phoneNB][nb] != undefinedNb[nb])
        returnState=false;

    if(returnState)
      return true;
  }
  return false;
}

void subChar(char returnSTR[], char str[], int start, int charLength){
  int c=0;
  for(int a=start; a<start+charLength; a++){
    returnSTR[c]=str[a];  // C more lisible than a-start
    c++;
  }
}

/* ### READ SMS ###*/
void readSms(){
  Serial.println("CHECK STEP 1");

  char smsSTR[MAXBAUDRATEMSG];
  sendCMD(smsSTR,"AT+CMGL=\"ALL\"",3000,MAXBAUDRATEMSG);

  Serial.println("CHECK STEP 1.5");

  if(findStr(smsSTR,"CMGL")){
    int telPos=indexOf(smsSTR,"+33");
    Serial.println("CHECK STEP 2, MSG : ");

    Serial.println("SMS STR : ");
    Serial.println(smsSTR);

    if(telPos>-1 && strlen(smsSTR)>(telPos+12) ){
      char telephone[12];
      subChar(telephone,smsSTR,telPos,12);

      boolean isDefined=checkPhoneNumber(telephone);
      if(isDefined){

        Serial.println("CHECK STEP 3");

        boolean commandeFind=false;

        for(int commandeNb=0; commandeNb<COMMANDESIZE; commandeNb++){

        //  String commandCourante=convertStrToAsciiDec(COMMANDE[commandeNb]);   //ONLY FOR UNICODE

      //    if(findStr(smsSTR,commandCourante) || findStr(smsSTR,COMMANDE[commandeNb])){      //ONLY FOR UNICODE
          if(findStr(smsSTR,COMMANDE[commandeNb])){
            Serial.println("CHECK STEP 4");

            commandeFind=true;

            char smsSTRtoSEND[MAXBAUDRATEMSG];
            executeCommand(smsSTRtoSEND,commandeNb,MAXBAUDRATEMSG);

            Serial.println("CHECK STEP 5");

            Serial.println(smsSTRtoSEND);

            manage(5,247,smsSTRtoSEND,telephone);
          }
        }
        if(commandeFind==false)
          manage(3,170,"UNDEFINED COMMAND => REBOOT GSM",telephone);
      }
      else
        manage(4,179,"UNDEFINED NUMBER => REBOOT GSM","");

    }
    else{
      Serial.println(telPos);
      Serial.println(strlen(smsSTR));
      manage(4,180,"UNDEFINED COMMAND RETURN DURING READING","");

    }
  }
  //else{
    //NO SMS
  //}
}

/* ### DELETE ALL SMS ### */
boolean deleteAllSms(){
  char deleteSTR1[MAXBAUDRATEMSG];
  sendCMD(deleteSTR1,"AT+CMGD=0,4",3000,MAXBAUDRATEMSG);

  delay(2000);

  char deleteSTR2[MAXBAUDRATEMSG];
  sendCMD(deleteSTR2,"AT+CMGD=1,4",3000,MAXBAUDRATEMSG);

  if(findStr(deleteSTR1,OKRETURN)!= true || findStr(deleteSTR2,OKRETURN)!= true)
    manage(1,205,"ERROR DURING DELETEING SMS","");
}

void resetString(char str[]){
  for(int a=0; a<sizeof(str); a++)
    str[a]=NULL;
}

void appendStr(char str1[], char str2[], int a0, int b0, int bmax){
  for(int b=b0; b<bmax; b++)
    str1[a0+b-b0]=str2[b];
}

/* ### SEND SMS ### */
boolean sendSms(char numero[], char msg[]){

  //TODO CHECK IF NUMBER IS GOOD 12 LENGTH
  char returnCMD[MAXBAUDRATEMSG];
  sendCMD(returnCMD,"AT+CMGF=1",2000,MAXBAUDRATEMSG);

  delay(100);
  if(findStr(returnCMD,"OK")){

    char commandPhone[22];
    appendStr(commandPhone,"AT+CMGS=\"+33",0,0,12);
    appendStr(commandPhone,numero,12,3,13);
    commandPhone[21]='"';

    Serial.println(commandPhone);

    resetString(returnCMD);

    sendCMD(returnCMD,commandPhone,2000,MAXBAUDRATEMSG);
    delay(100);

    resetString(returnCMD);

    sendCMD(returnCMD,msg,2000,MAXBAUDRATEMSG);
    delay(100);

    resetString(returnCMD);

    sendCMD(returnCMD,"ctrlz",2000,MAXBAUDRATEMSG);
    delay(100);

    if(findStr(returnCMD,OKRETURN))
      manage(0,226,"SMS ENVOYE","");
    else
      manage(1,229,returnCMD,"");
  }
  else
    manage(1,299,returnCMD,"");
}

/* ### SETUP ### */
void setup(){

  //INIT SERIAL
  Serial.begin(SERIALPCBAUDRATE);
  gsm.begin(SERIALGSMBAUDRATE);

  if(DEBUG)
    Serial.println("START SETUP");

  //INIT GSM POWERPIN
  pinMode(POWERKEY, OUTPUT);

  //INIT HEATER PIN
  pinMode(PINRELAI1,OUTPUT);
  pinMode(PINRELAI2,OUTPUT);

  digitalWrite(PINRELAI1,LOW);
  digitalWrite(PINRELAI2,LOW);

  arduinoState=0;

  delay(300);

  while(!checkIfSerialRespond())
    manage(1,279,"GSM NOT RESPONDING","");

  //manage(2,354,"GSM STARTED","+***********");
}

/* ### VOID LOOP ###*/
void loop(){

  Serial.print(".");

  if(checkIfSerialRespond()){
    Serial.println("start reading SMS");
    readSms();
  }
  else
    manage(1,279,"GSM NOT RESPONDING","");

  delay(5000);
}
