/****************************************************************************************
 *  @File           :   "SIM 800L Sensor Node.ino"
 *
 *  @Description    :   Main program for SIM800L Sensor Node using MCU MSP430F5529, SIM800L,
 *                      GPS-L70 and HDC1080
 *
 *  @Deatail        :   MCU MSP430F5529 reads GPS location from L70, temperature and humidity
 *                      from HDC1080 then push data via GPRS using SIM800L with MQTT Protocol.
 *
 *  @Date           :   18/03/2017
 *
 *  @Version        :   1.0
 *
 *  @Author         :   Thiepnx
 *
 *  @Infor              GitHub:     https://github.com/XuanThiep
 *                      Youtube:    https://www.youtube.com/channel/UCMnrf_QCcGwOv_TdvHxtTQQ/playlists
 *
 */


/*********************************** Include Library ***************************************/
#include <driverlib.h>
#include "atof.h"
#include "GSM_MQTT.h"
#include "hdc1080.h"
#include "stdlib.h"
#include "stdio.h"

/************************** Define For Connection And Useful Macro **************************/

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


/******************************* Declaring Global Variables ********************************/

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

/******************************* Declaring function prototype ******************************/


void    Request_GPS_Data(void);
unsigned char calculate_checksum(char* array,unsigned char start_index,unsigned char end_index);
unsigned char hexString2int(char *hex,unsigned char _length);
char* f2s(float f, int p);
void empty_buffer(char* buf,unsigned char length);


void GSM_MQTT::AutoConnect(void)
{
    /*
     Use this function if you want to use autoconnect(and auto reconnect) facility
     This function is called whenever TCP connection is established (or re-established).
     put your connect codes here.
     */
    sprintf(clientid,"_SIM_%d",random(1000));
    connect(clientid, 1, 1, MQTT_USER, MQTT_PASSWORD, 1, 0, 0, 0, "", "");
    /*    void connect(char *ClientIdentifier, char UserNameFlag, char PasswordFlag, char *UserName, char *Password, char CleanSession, char WillFlag, char WillQoS, char WillRetain, char *WillTopic, char *WillMessage);
          ClientIdentifier  :Is a string that uniquely identifies the client to the server.
                            :It must be unique across all clients connecting to a single server.(So it will be better for you to change that).
                            :It's length must be greater than 0 and less than 24
                            :Example "qwerty"
          UserNameFlag      :Indicates whether UserName is present
                            :Possible values (0,1)
                            :Default value 0 (Disabled)
          PasswordFlag      :Valid only when  UserNameFlag is 1, otherwise its value is disregarded.
                            :Indicates whether Password is present
                            :Possible values (0,1)
                            :Default value 0 (Disabled)
          UserName          :Mandatory when UserNameFlag is 1, otherwise its value is disregarded.
                            :The UserName corresponding to the user who is connecting, which can be used for authentication.
          Password          :alid only when  UserNameFlag and PasswordFlag are 1 , otherwise its value is disregarded.
                            :The password corresponding to the user who is connecting, which can be used for authentication.
          CleanSession      :If not set (0), then the server must store the subscriptions of the client after it disconnects.
                            :If set (1), then the server must discard any previously maintained information about the client and treat the connection as "clean".
                            :Possible values (0,1)
                            :Default value 1
          WillFlag          :This flag determines whether a WillMessage published on behalf of the client when client is disconnected involuntarily.
                            :If the WillFlag is set, the WillQoS, WillRetain, WillTopic, WilMessage fields are valid.
                            :Possible values (0,1)
                            :Default value 0 (Disables will Message)
          WillQoS           :Valid only when  WillFlag is 1, otherwise its value is disregarded.
                            :Determines the QoS level of WillMessage
                            :Possible values (0,1,2)
                            :Default value 0 (QoS 0)
          WillRetain        :Valid only when  WillFlag is 1, otherwise its value is disregarded.
                            :Determines whether the server should retain the Will message.
                            :Possible values (0,1)
                            :Default value 0
          WillTopic         :Mandatory when  WillFlag is 1, otherwise its value is disregarded.
                            :The Will Message will published to this topic (WillTopic) in case of involuntary client disconnection.
          WillMessage       :Mandatory when  WillFlag is 1, otherwise its value is disregarded.
                            :This message (WillMessage) will published to WillTopic in case of involuntary client disconnection.
     */

}


void GSM_MQTT::OnConnect(void)
{
    /*
     This function is called when mqqt connection is established.
     put your subscription publish codes here.
     */
#ifdef  USING_DEBUG
    subscribe(0, _generateMessageID(), "inTopic", 1);
#endif
    /*    void subscribe(char DUP, unsigned int MessageID, char *SubTopic, char SubQoS);
          DUP       :This flag is set when the client or server attempts to re-deliver a SUBSCRIBE message
                    :This applies to messages where the value of QoS is greater than zero (0)
                    :Possible values (0,1)
                    :Default value 0
          Message ID:The Message Identifier (Message ID) field
                    :Used only in messages where the QoS levels greater than 0 (SUBSCRIBE message is at QoS =1)
          SubTopic  :Topic names to which  subscription is needed
          SubQoS    :QoS level at which the client wants to receive messages
                    :Possible values (0,1,2)
                    :Default value 0
     */
#ifdef  USING_DEBUG
    publish(0, 0, 0, _generateMessageID(), "inTopic", "Hello World I am Client A hi hi");
#endif
    /*  void publish(char DUP, char Qos, char RETAIN, unsigned int MessageID, char *Topic, char *Message);
      DUP       :This flag is set when the client or server attempts to re-deliver a PUBLISH message
                :This applies to messages where the value of QoS is greater than zero (0)
                :Possible values (0,1)
                :Default value 0
      QoS       :Quality of Service
                :This flag indicates the level of assurance for delivery of a PUBLISH message
                :Possible values (0,1,2)
                :Default value 0
      RETAIN    :if the Retain flag is set (1), the server should hold on to the message after it has been delivered to the current subscribers.
                :When a new subscription is established on a topic, the last retained message on that topic is sent to the subscriber
                :Possible values (0,1)
                :Default value 0
      Message ID:The Message Identifier (Message ID) field
                :Used only in messages where the QoS levels greater than 0
      Topic     :Publishing topic
      Message   :Publishing Message
     */
}


void GSM_MQTT::OnMessage(char *Topic, int TopicLength, char *Message, int MessageLength)
{
    /*
    This function is called whenever a message received from subscribed topics
    put your subscription publish codes here.
     */

    /*
     Topic        :Name of the topic from which message is coming
     TopicLength  :Number of characters in topic name
     Message      :The containing array
     MessageLength:Number of characters in message
     */

#ifdef  USING_DEBUG
    UART_DEBUG.println(TopicLength);
    UART_DEBUG.println(Topic);
    UART_DEBUG.println(MessageLength);
    UART_DEBUG.println(Message);
#endif

}

GSM_MQTT MQTT(30);


/* Main Program */

void setup()
{

    /* Configure GPIO For Status Led */
    GPIO_setAsOutputPin(LED_STATUS_PORT,LED_STATUS_OK|LED_STATUS_ERROR);

    /* Configure GPIO For GPS Power Control */
    GPIO_setAsOutputPin(GPS_POWER_PORT,GPS_POWER);

    /* Turn on GPS power */
    TURN_ON_GPS();

    /* Turn off ok status led and turn on error status led */
    TURN_OFF(LED_STATUS_OK);
    TURN_ON(LED_STATUS_ERROR);

    /* Initialize UART for communicate with module gps L70 */
    GPS_Serial.begin(115200);

    /* Initialize HDC1080 Sensor with highest resolution */
    sensor_hdc1080.Init(Temperature_Resolution_14_bit, Humidity_Resolution_14_bit);

    /* Begin MQTT Protocol */
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

        /* Publish with Qos=1 and Retain =1 */
        MQTT.publish(0, 1, 1,MQTT._generateMessageID(), "inTopic", publish_message_arr);

        MQTT.processing();

        //Sleep in 30 seconds
        sleep(30000);
    }
    else
    {
        TURN_OFF(LED_STATUS_OK);
        TURN_ON(LED_STATUS_ERROR);


    }

    MQTT.processing();


  
}


/* Global Function Code */

/*  @Brief  :   Turn on module gps L70, then read gps data include latitude and longitude, after that
 *              turn off module gps L70 for reduce power consumption
 *
 *  @Para   :   None
 *
 *  @Return :   None
 *
 *  @Note   :   latitude and longitude value are stored in variable "latitude" and "longitude"
 */
void    Request_GPS_Data(void)
{
    /* Turn on VCC for L70R GPS Module */
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
               /* Check Message ID */
               if((gps_buffer[0] != '$')||(gps_buffer[1] != 'G')||(gps_buffer[2] != 'P')||(gps_buffer[3] != 'R')||(gps_buffer[4] != 'M')||(gps_buffer[5] != 'C'))
               {
                   /* Invalid response */
                   while(1)
                   {
                       /* Read out content of invalid message */
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
                           /* Check for valid data and Calculate Check Sum*/
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

   /* Turn Off module gps for reduce power consumption */
   TURN_OFF_GPS();

}


/*  @Brief  :   Calculate check sum for NMEA 0183 standard commands
 *
 *  @Para   :   +   array: This is pointer of NMEA Message
 *              +   start_index: Start index for calculate check sum
 *              +   end_index:  End index for calculate check sum
 *
 *  @Return :   Check sum value
 *
 *  @Note   :   None
 */
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


/*  @Brief  :   Convert hexa string to integer number
 *
 *  @Para   :   +   hex: This is pointer of hexa string
 *              +   _length: This is length of hexa string
 *
 *  @Return :   result of convert hexa string to integer number
 *
 *  @Note   :   None
 */
unsigned char hexString2int(char *hex,unsigned char _length)
{
    unsigned char val = 0,i;
    unsigned char _byte;
    for(i=0;i<_length;i++)
    {
        // get current character then increment
        _byte = *hex++;
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (_byte >= '0' && _byte <= '9') _byte = _byte - '0';
        else if (_byte >= 'a' && _byte <='f') _byte = _byte - 'a' + 10;
        else if (_byte >= 'A' && _byte <='F') _byte = _byte - 'A' + 10;
        // shift 4 to make space for new digit, and add the 4 bits of the new digit
        val = (val << 4) | (_byte & 0x0F);
    }
    return val;
}


/*  @Brief  :   Convert float number to string
 *
 *  @Para   :   +   string: Pointer to array store string
 *              +   number: number to convert
 *
 *  @Return :   None
 *
 *  @Note   :   Accuracy is 6 digits
 */
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


/*  @Brief  :   Convert float number to string with specific precision
 *
 *  @Para   :   +   f: number to convert
 *              +   p: specific precision
 *
 *  @Return :   Pointer to array store string
 *
 *  @Note   :   Allow accuracy <= 3 digits
 */
char* f2s(float f, int p)
{
  char * pBuff;                         // use to remember which part of the buffer to use for dtostrf
  const int iSize = 10;                 // number of bufffers, one for each float before wrapping around
  static char sBuff[iSize][20];         // space for 20 characters including NULL terminator for each float
  static int iCount = 0;                // keep a tab of next place in sBuff to use
  pBuff = sBuff[iCount];                // use this buffer
  if(iCount >= iSize -1)
  {               // check for wrap
    iCount = 0;                         // if wrapping start again and reset
  }
  else{
    iCount++;                           // advance the counter
  }
  return dtostrf(f, 0, p, pBuff);       // call the library function
}

/*  @Brief  :   Empty Buffer
 *
 *  @Para   :   +   buf: This is pointer of buffer
 *              +   length: lenght of buffer
 *
 *  @Return :   None
 *
 *  @Note   :   None
 */
void empty_buffer(char* buf,unsigned char length)
{
    for(unsigned char i=0;i<length;i++)
    {
        *(buf+i) = 0;
    }
}
