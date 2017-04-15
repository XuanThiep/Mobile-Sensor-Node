#include "Energia.h"

#line 1 "C:/Users/Xuan Thiep/Documents/MEGA/Projects/MSP43X Projects/Mobile Node Using SIM 800L/Mobile Node Using SIM 800L.ino"






















#include <driverlib.h>
#include "atof.h"
#include "GSM_MQTT.h"
#include "hdc1080.h"
#include "stdlib.h"
#include "stdio.h"



#define         NODE_ID                 "SIM1"

#define         LED_STATUS_PORT         GPIO_PORT_P1
#define         LED_STATUS_OK           GPIO_PIN1
#define         LED_STATUS_ERROR        GPIO_PIN0

#define         GPS_POWER_PORT          GPIO_PORT_P4
#define         GPS_POWER               GPIO_PIN3

#define         TURN_ON(led)            GPIO_setOutputHighOnPin(LED_STATUS_PORT,led)
#define         TURN_OFF(led)           GPIO_setOutputLowOnPin(LED_STATUS_PORT,led)

#define         TURN_ON_GPS()           GPIO_setOutputHighOnPin(GPS_POWER_PORT,GPS_POWER)
#define         TURN_OFF_GPS()          GPIO_setOutputLowOnPin(GPS_POWER_PORT,GPS_POWER)

#define         GPS_Serial              Serial




void setup();
void loop();
void float2string(char* string,double number);

#line 52
char gps_buffer[150],lat_degree[2],lat_minute[7],long_degree[3],long_minute[7];

unsigned int gps_index=0;
unsigned char count_comma=0;
bool gps_done = false;
bool invalid_data = false;

unsigned char lat_start_index,lat_end_index,long_end_index,star_index;
float latitude,longitude;

hdc1080 sensor_hdc1080;
float temp;
uint8_t humi;

char latitude_arr[12],longitude_arr[12],*temp_arr;
char publish_message_arr[100];
char clientid[10];




void    Request_GPS_Data(void);
unsigned char calculate_checksum(char* array,unsigned char start_index,unsigned char end_index);
unsigned char hexString2int(char *hex,unsigned char _length);
char* f2s(float f, int p);
void empty_buffer(char* buf,unsigned char length);


void GSM_MQTT::AutoConnect(void)
{
    




    sprintf(clientid,"_SIM_%d",random(1000));
    connect(clientid, 1, 1, MQTT_USER, MQTT_PASSWORD, 1, 0, 0, 0, "", "");
    





































}


void GSM_MQTT::OnConnect(void)
{
    



#ifdef  USING_DEBUG
    subscribe(0, _generateMessageID(), "inTopic", 1);
#endif
    











#ifdef  USING_DEBUG
    publish(0, 0, 0, _generateMessageID(), "inTopic", "Hello World I am Client A hi hi");
#endif
    

















}


void GSM_MQTT::OnMessage(char *Topic, int TopicLength, char *Message, int MessageLength)
{
    




    






#ifdef  USING_DEBUG
    UART_DEBUG.println(TopicLength);
    UART_DEBUG.println(Topic);
    UART_DEBUG.println(MessageLength);
    UART_DEBUG.println(Message);
#endif

}

GSM_MQTT MQTT(30);




void setup()
{

    
    GPIO_setAsOutputPin(LED_STATUS_PORT,LED_STATUS_OK|LED_STATUS_ERROR);

    
    GPIO_setAsOutputPin(GPS_POWER_PORT,GPS_POWER);

    
    TURN_ON_GPS();

    
    TURN_OFF(LED_STATUS_OK);
    TURN_ON(LED_STATUS_ERROR);

    
    GPS_Serial.begin(115200);

    
    sensor_hdc1080.Init(Temperature_Resolution_14_bit, Humidity_Resolution_14_bit);

    
    MQTT.begin();



}

void loop()
{
    if (MQTT.available())
    {
        TURN_OFF(LED_STATUS_ERROR);
        TURN_ON(LED_STATUS_OK);

        Request_GPS_Data();

        float2string(latitude_arr,(double) latitude);
        float2string(longitude_arr,(double) longitude);

        sensor_hdc1080.Start_measurement(&temp,&humi);
        temp_arr = f2s(temp, 2);

        sprintf(publish_message_arr,"%s,%s,%s,%s,%d",NODE_ID,latitude_arr,longitude_arr,temp_arr,humi);

        
        MQTT.publish(0, 1, 1,MQTT._generateMessageID(), "inTopic", publish_message_arr);

        MQTT.processing();

        
        sleep(30000);
    }
    else
    {
        TURN_OFF(LED_STATUS_OK);
        TURN_ON(LED_STATUS_ERROR);


    }

    MQTT.processing();


  
}













void    Request_GPS_Data(void)
{
    
    TURN_ON_GPS();

    gps_done=false;
    invalid_data = false;
    gps_index=count_comma=0;


   while(gps_done==false)
   {
       while(GPS_Serial.available())
       {
           gps_buffer[gps_index] = GPS_Serial.read();

           if(gps_index == 5)
           {
               
               if((gps_buffer[0] != '$')||(gps_buffer[1] != 'G')||(gps_buffer[2] != 'P')||(gps_buffer[3] != 'R')||(gps_buffer[4] != 'M')||(gps_buffer[5] != 'C'))
               {
                   
                   while(1)
                   {
                       
                       if(GPS_Serial.available())
                       {
                           gps_buffer[0] = GPS_Serial.read();
                           if(gps_buffer[0] == '\n')
                           {
                               break;
                           }
                       }
                   }
                   gps_index = 0;
               }
               else
               {
                   gps_index++;
               }
           }
           else
           {
               if(gps_index > 5)
               {
                   switch(gps_buffer[gps_index])
                   {
                       case ',':
                           count_comma++;
                           switch (count_comma)
                           {
                           case 3:
                               lat_start_index = gps_index;
                               break;
                           case 4:
                               lat_end_index = gps_index;
                               break;
                           case 6:
                               long_end_index = gps_index;
                               break;
                           }
                           break;
                       case '*':
                           star_index = gps_index;
                           break;
                       case '\n':
                           
                           if((gps_buffer[lat_start_index-1] == 'A')&&(calculate_checksum(gps_buffer, 1, star_index-1) == hexString2int(gps_buffer + star_index +1,2)))
                           {

                               memmove(lat_degree,gps_buffer+lat_start_index+1,2);
                               memmove(lat_minute,gps_buffer+lat_start_index+3,7);

                               memmove(long_degree,gps_buffer+lat_end_index+3,3);
                               memmove(long_minute,gps_buffer+lat_end_index+6,7);

                               latitude = (lat_degree[0]-'0')*10 + (lat_degree[1]-'0') + atof(lat_minute)/60.0;
                               longitude = (long_degree[0]-'0')*100+(long_degree[1]-'0')*10 + (long_degree[2]-'0') + atof(long_minute)/60.0;

                               gps_done = true;

                           }
                           else
                           {
                               invalid_data = true;
                           }
                           break;
                   }
               }

               if(invalid_data == false)
               {
                   gps_index++;
               }
               else
               {
                   invalid_data = false;
                   gps_index=count_comma=0;

               }
           }
       }
   }

   
   TURN_OFF_GPS();

}












unsigned char calculate_checksum(char* array,unsigned char start_index,unsigned char end_index)
{
    unsigned char i;
    unsigned char result = *(array+start_index);

    for(i=start_index+1; i<=end_index;i++)
    {
        result^=*(array+i);
    }
    return result;
}











unsigned char hexString2int(char *hex,unsigned char _length)
{
    unsigned char val = 0,i;
    unsigned char _byte;
    for(i=0;i<_length;i++)
    {
        
        _byte = *hex++;
        
        if (_byte >= '0' && _byte <= '9') _byte = _byte - '0';
        else if (_byte >= 'a' && _byte <='f') _byte = _byte - 'a' + 10;
        else if (_byte >= 'A' && _byte <='F') _byte = _byte - 'A' + 10;
        
        val = (val << 4) | (_byte & 0x0F);
    }
    return val;
}











void float2string(char* string,double number)
{
    unsigned long d1,d2,d3,d4,d5,d6;
    unsigned int integer = (int)number;

    d1    = integer/100;
    d2    = (integer -d1*100)/10;
    d3    = integer%10;

    *(string+0) = d1+'0';
    *(string+1) = d2+'0';
    *(string+2) = d3+'0';

    unsigned long decimal = (number-(double)integer)*1000000;
    d1 = decimal/100000;
    d2 = (decimal - d1*100000)/10000;
    d3 = (decimal - d1*100000 - d2*10000)/1000;
    d4 = (decimal - d1*100000 - d2*10000 - d3*1000)/100;
    d5 = (decimal - d1*100000 - d2*10000 - d3*1000 -d4*100)/10;
    d6 = (decimal - d1*100000 - d2*10000 - d3*1000 -d4*100 -d5*10);

    *(string+3) = '.';
    *(string+4) = d1 + '0';
    *(string+5) = d2 + '0';
    *(string+6) = d3 + '0';
    *(string+7) = d4 + '0';
    *(string+8) = d5 + '0';
    *(string+9) = d6 + '0';

}











char* f2s(float f, int p)
{
  char * pBuff;                         
  const int iSize = 10;                 
  static char sBuff[iSize][20];         
  static int iCount = 0;                
  pBuff = sBuff[iCount];                
  if(iCount >= iSize -1)
  {               
    iCount = 0;                         
  }
  else{
    iCount++;                           
  }
  return dtostrf(f, 0, p, pBuff);       
}










void empty_buffer(char* buf,unsigned char length)
{
    for(unsigned char i=0;i<length;i++)
    {
        *(buf+i) = 0;
    }
}



