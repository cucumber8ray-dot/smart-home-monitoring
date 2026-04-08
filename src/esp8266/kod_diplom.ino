/*
  UDPSendReceive.pde:
Программа управляет 2 реле, получает данные от 2 датчиков DS18B20 и 1 датчика тока
Прием и передача данных от Raspberry Pi Zero W
*/

#include <microDS18B20.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define WEMOS_A0     A0
#ifndef STASSID

#define STASSID "tplink"
#define STAPSK "111222"
#endif

#define DS_PIN 5 // пин для термометров

// Уникальные адреса датчиков - считать можно в примере address_read

uint8_t s1_addr[] = {0x28, 0xC5, 0x6A, 0x0F, 0x02, 0x00, 0x00, 0x13};
uint8_t s2_addr[] = {0x28, 0xAB, 0xCE, 0x15, 0x04, 0x00, 0x00, 0x1D};


MicroDS18B20<DS_PIN, s1_addr> sensor1;  // Создаем термометр с адресацией

MicroDS18B20<DS_PIN, s2_addr> sensor2;  // Создаем термометр с адресацией

unsigned int localPort = 8888;  // local port to listen on
int nn,nn1,i;
char aaaa, sim;
int val,val0, read_dat ;
long  regim1, regim2,temp1_old,temp2_old, temp1, temp2,temp11, temp12, z_temp1, z_temp2, nom,sost=0,counter0 = 0,err1=0,err2=0;
unsigned long n_dat_1=1,n_dat_2=1, sost1, sost2,intVar,intVar_old, nom_a, millis0, millis1,millis2,millis3, tim, rab_sek=0,temp1_out,temp2_out;
String stringVar = "1234567890777";
float voltage,tim_r;

IPAddress ip(192, 168, 3, 235); // this 3 lines for a fix IP-address
IPAddress gateway(192, 168, 3, 1);
IPAddress subnet(255, 255, 255, 0);
//volatile long counter = 0;



// buffers for receiving and sending data
char  packetBuffer[UDP_TX_PACKET_MAX_SIZE + 1];  // buffer to hold incoming packet,
char packetBuffer2[UDP_TX_PACKET_MAX_SIZE + 1];

char ReplyBuffer[] = "777_acknowledged\r\n";        // a string to send back
String Buffer, Buffer2, Buffer3, myStr[20];

WiFiUDP Udp;

void setup() {
pinMode(WEMOS_A0, INPUT);

pinMode(D5, OUTPUT);
pinMode(D6, OUTPUT);
pinMode(D7, OUTPUT);


 sensor1.setResolution(12); 
 sensor2.setResolution(12); 
  
  Serial.print('7');Serial.print('8');Serial.print('9');
  Serial.begin(115200);
  WiFi.config(ip, gateway, subnet);
  WiFi.mode(WIFI_STA);

  // WiFi.config(ip);
  WiFi.begin(STASSID, STAPSK);

  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("UDP server on port %d\n", localPort);

  Udp.begin(localPort);
  millis3=millis2=millis1=millis();
 regim1=1; regim2=1;  z_temp1=600; z_temp2=610;

  
  
}

void loop() {

  sensor1.requestTemp();      // Запрашиваем преобразование температуры
  sensor2.requestTemp();
 

// замер тока

val=0;  
nom_a=0;
millis0= millis();
millis1=  millis0; 
while ((millis1<(millis0+20))&(millis1>=millis0)) {millis1= millis(); val =val+ analogRead(WEMOS_A0);++nom_a; };
if (nom_a>0) {val= val /nom_a;}  
if (val>=3) {val= val -3;}  
if (val>=999)  {val= 999;} 


//прием – передача

// проверка запроса от Raspberry Pi Zero W

  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.printf("Received packet of size %d from %s:%d\n    (to %s:%d, free heap = %d B)\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort(), Udp.destinationIP().toString().c_str(), Udp.localPort(), ESP.getFreeHeap());

    // read the packet into packetBufffer

for (i=1; i<30; i++) {packetBuffer[i]=' ';};
//Serial.print("buf1 - ");
//Serial.println(packetBuffer);
    int n = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
//Serial.print("buf2 - ");
//Serial.println(packetBuffer);

 for (i=0; i<14; i++) {stringVar[i] = ' ';}

 for (i=0; i<10; i++) {stringVar[i] = packetBuffer[i+1];}
intVar_old=intVar;
intVar=stringVar.toInt();

Serial.print(intVar_old);Serial.print(" - ");Serial.println(intVar);
if (intVar_old == intVar) {
 
// упаковка данных
 z_temp1=(intVar / 1000000);
 z_temp2=((intVar - z_temp1*1000000) /10000);
 regim1=(intVar - z_temp1*1000000- z_temp2*10000)/1000;
 regim2=(intVar - z_temp1*1000000- z_temp2*10000-regim1*1000)/100;
n_dat_1=(intVar - z_temp1*1000000- z_temp2*10000-regim1*1000-regim2*100)/10;
n_dat_2=(intVar - z_temp1*1000000- z_temp2*10000-regim1*1000-regim2*100-n_dat_1*10);
 
z_temp1=z_temp1*10;
z_temp2=z_temp2*10;
                              
if (err1==0){temp1_out=temp1;} else temp1_out=999;
if (err2==0){temp2_out=temp2;} else temp2_out=999;
 Buffer2=" "+String((sost2*2+sost1)*1000000000+temp1_out*1000000+temp2_out*1000+val);
packetBuffer[0]=19;

// подготовка буфера строки

  for (i=0; i<20; i++) {packetBuffer[i]=' ';}  // заполнение пробелами

  for (i=1; i<20; i++) {packetBuffer[i]=Buffer2[i];} 
 
 // packetBuffer[18]='\r'; // возврат каретки
  //packetBuffer[19]='\n'; // перевод 

packetBuffer[0]=19;
Serial.println(packetBuffer);

  // send a reply, to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
 //  Udp.write(Buffer);
    Udp.write(packetBuffer);
    Udp.endPacket();  
  } 

//завершение прием - передача

if((millis()>(millis2+1000))||(millis()<millis2)) {millis2= millis(); // ожидаем результат 1 с 

  err1=err2=0;
  
  temp1_old=temp1;
  if (sensor1.readTemp()) {temp1=round(10*sensor1.getTemp())+300;if (temp1>998) temp1=998;if (temp1<=0) temp2=0;}  else  {  temp1=temp1_old;  err1=1;};

  temp2_old=temp2;
  if (sensor2.readTemp()) {temp2=round(10*sensor2.getTemp())+300;if (temp2>998) temp2=998;if (temp2<=0) temp2=0;}  else  {  temp2=temp2_old;  err2=1;}; 
                                               };



if((millis()>(millis3+60000))||(millis()<millis3)) {millis3= millis(); // ожидаем 60 с и управляем

if (n_dat_1==1){temp11=temp1;};   
if (n_dat_1==2){temp11=temp2;};

if ((temp11<z_temp1)&(regim1==0)) {sost1=1;} 
if ((temp11>z_temp1)&(regim1==0)) {sost1=0;} 
if ((temp11<z_temp1)&(regim1==1)) {sost1=0;} 
if ((temp11>z_temp1)&(regim1==1)) {sost1=1;} 

if (n_dat_2==1){temp12=temp1;};   
if (n_dat_2==2){temp12=temp2;};

if ((temp12<z_temp2)&(regim2==0)) {sost2=1;} 
if ((temp12>z_temp2)&(regim2==0)) {sost2=0;} 
if ((temp12<z_temp2)&(regim2==1)) {sost2=0;} 
if ((temp12>z_temp2)&(regim2==1)) {sost2=1;} 


if (sost1==1) {digitalWrite(D6, HIGH);} else {digitalWrite(D6, LOW);}; // реле 1

if (sost2==1) {digitalWrite(D7, HIGH);} else {digitalWrite(D7, LOW);}; // реле 2               


                                                     };

}
