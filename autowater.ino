#include <stdio.h>
#include <string.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Timer.h>
//#include "Gsender.h"
#include "HTTPSRedirect.h"
#include "sscanf.h"



//*-- Hardware Serial
#define _baudrate	115200
String inputString = "";         // a String to hold incoming data
boolean stringComplete = false;  // whether the string is complete

//Ryan begin
#define ledPin 2 // GPIO2
#define Spray_Enable_Pin 13 // GPIO13
#define buttonPin 12 // GPIO12

//thingspeak begin
#define HOST_thingspeak 	"api.thingspeak.com" // ThingSpeak IP Address: 184.106.153.149
#define PORT_thingspeak 	80
String GET = "GET /update?key=5PFXFEU54JZ3VD2P";
String FIELD1 = "&field1=";
String FIELD2 = "&field2=";
String FIELD3 = "&field3=";
String FIELD4 = "&field4=";
String FIELD5 = "&field5=";
String FIELD6 = "&field6=";
//thingspeak end

#define power_on_time  60 * 2 // 10 mins 
#define spray_time 60 * 1 // 1 min
#define sleep_time 60 * 60 * 8 // 4 hours

#define Timer0_period 1    //Seconds
Timer t_Timer_0;

enum System_Status
{
	system_init,
	power_on,
	spray,
	sleep,
	spray2,
	sleep2,
	manual
};

int e_system_status = 0;
int LED_Status=0;
int upload_period = 0;
int get_time_period = 0;
int power_status_counter=0;
int buttonState = 0;
int VOL_check_flag = 0;
int time_update_count=0;
int read_from_google=0;
int water_time_1=9;
int water_time_2=15;
int water_enable=0;
int water_enable_temp=0;
int current_time=0;
int water_period=60;
int Spray_frequency=0;
int spray_data[52];
int intData;


int save_flag = 0;
int int_date = 0xFF;
int int_hour = 0xFF;
int int_year,int_minute,int_second,int_month;
char c_theDate[50],c_week[5],c_month[5];
char inChar;
bool WiFi_connected=true;
bool sleep_flag=false;
int current_time_temp=0;


#define server_id        spray_data[0]
#define server_PW        spray_data[1]
#define server_Power     spray_data[2]
#define server_spray_now spray_data[3]

typedef enum {
	data_frequency,
	data_time_1,
	data_time_2,
	data_period 	
}spray_data_type;

typedef enum {
	D1S2,//1��2��
	D1S1,//1��1��
	D2S1,//2��1��
	D3S1,//3��1�� 
	D4S1,//4��1�� 
	D5S1,
	D6S1,
	D7S1,
	D_Max
}spray_frequency_type;

typedef struct
{
  byte date;	// 1 byte
  byte spraytime;	// 1 byte 
  int boot_count;	// 2 byte 
  char eep_ssid[32];
  char eep_pass[64];
  unsigned int id;
  int eep_intData;
} TDIC_SystemDictionary;								// 6 bytes

TDIC_SystemDictionary  SystemData;
int eeAddress = 0; //EEPROM address to start reading from
//Ryan end

ESP8266WebServer server(80);

const char* ssid = "test";
const char* passphrase = "test";
String st;
String content;
int statusCode;


//Google
const char* host_google = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
//const char *GScriptId = "AKfycbwR6D4-TIARID4xyeCWYmwkcKAlXyvFh-ReYHxO-Cqua-UKozI";  //auto water test
const char *GScriptId = "AKfycbwowtJ8_bnR2VugFBCZJLMAS8fZNhI1fiDQ71y_2puaCZwYC2BU";  //APP test2





const int httpsPort = 443;

// http://askubuntu.com/questions/156620/how-to-verify-the-ssl-fingerprint-by-command-line-wget-curl/
// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
// www.grc.com doesn't seem to get the right fingerprint
// SHA1 fingerprint of the certificate
//const char* fingerprint = "94 2F 19 F7 A8 B4 5B 09 90 34 36 B2 2A C4 7F 17 06 AC 6A 2E";
//const char* fingerprint = "F0 5C 74 77 3F 6B 25 D7 3B 66 4D 43 2F 7E BC 5B E9 28 86 AD";
//const char* fingerprint2 = "94 64 D8 75 DE 5D 3A E6 3B A7 B6 15 52 72 CC 51 7A BA 2B BE";
const char* fingerprint = "07 EB DC 3D D6 DC BF C2 DD 7D B1 95 6F 38 A1 09 1A 66 8F 33";

// Write to Google Spreadsheet
//String url = String("/macros/s/") + GScriptId + "/exec?value=Hello";
// Fetch Google Calendar events for 1 week ahead
String url2 = String("/macros/s/") + GScriptId + "/exec?cal";
// Read from Google Spreadsheet
String url3 = String("/macros/s/") + GScriptId + "/exec?read";

String url_google = String("/macros/s/") + GScriptId + "/exec?";

void setup() {
  Serial.begin(_baudrate);
  Serial.flush();
  delay(1000);
  
  pinMode(ledPin, OUTPUT);	
  pinMode(Spray_Enable_Pin, OUTPUT);	
  digitalWrite(Spray_Enable_Pin, HIGH);	
  pinMode(buttonPin, INPUT);	 
  t_Timer_0.every(Timer0_period*1000, Timer0_callback_1s); 
  get_time_period = 3;
  upload_period = 60;
  WiFi_connected=true;
  sleep_flag=false;
	
  e_system_status = system_init;
  power_status_counter = power_on_time;
	
  buttonState = 0;
  save_flag = 0;
  time_update_count = 60; 
  read_from_google = 60;
  
  
  EEPROM.begin(512);
  delay(10);
  
  Serial.println("Reading EEPROM ");
  EEPROM.get(eeAddress, SystemData);	  

  
  Serial.println();
  Serial.print("Hardware ID =");Serial.println(SystemData.id);
  if (SystemData.id>1000)
  {
	  SystemData.id = 2;
	  Serial.print("Hardware ID =");Serial.println(SystemData.id);
  }
/*  
  Serial.println();
  Serial.println("Startup");
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
*/	
/*
  String esid(SystemData.eep_ssid);// = (String)SystemData.eep_ssid;
  Serial.print("SSID: ");
  Serial.println(esid);
*/  
/*  
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
*/
/*	
  String epass(SystemData.eep_pass);// = (String)SystemData.eep_pass;
  Serial.print("PASS: ");
  Serial.println(epass);  
*/
/*  
  if ( esid.length() > 1 ) {
      WiFi.begin(esid.c_str(), epass.c_str());
      if (testWifi()) {
        launchWeb(0);
        return;
      } 
	  WiFi.disconnect();
      WiFi.begin(esid.c_str(), epass.c_str());
      if (testWifi()) {
        launchWeb(0);
        return;
      } 	  
  }
*/  

  Serial.flush();

  if (true==wifi_connect())
  {
	  get_time_form_google_sheet();
	  connect_to_google(0);
	  launchWeb(0);
      return;	  
  }
  
  WiFi.disconnect();

  if (true==wifi_connect())
  {
	  get_time_form_google_sheet();
	  connect_to_google(0);
	  launchWeb(0);
      return;	  
  }  
  
  WiFi_connected = false;
  setupAP();
  
}
//====================================================================================================================
bool wifi_connect(void)
{
  String esid(SystemData.eep_ssid);// = (String)SystemData.eep_ssid;
  Serial.print("SSID: ");
  Serial.println(esid);
  
  String epass(SystemData.eep_pass);// = (String)SystemData.eep_pass;
  Serial.print("PASS: ");
  Serial.println(epass);  

   if ( esid.length() > 1 ) 
   {
      WiFi.begin(esid.c_str(), epass.c_str());
      return testWifi();     
   } 
	
}
//====================================================================================================================
bool testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");  
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) { return true; } 
    delay(1000);
    Serial.print(WiFi.status());    
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
} 

void launchWeb(int webtype) {
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer(webtype);
  // Start the server
  server.begin();
  Serial.println("Server started"); 
}

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println(""); 
  st = "<ol>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += "<li>";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      st += "</li>";
    }
  st += "</ol>";
  delay(100);
  WiFi.softAP(ssid, passphrase, 6);
  Serial.println("softap");
  launchWeb(1);
  Serial.println("over");
}

void createWebServer(int webtype)
{
  if ( webtype == 1 ) {
    server.on("/", []() {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
        content += ipStr;
        content += "<p>";
        content += st;
        content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
        content += "</html>";
        server.send(200, "text/html", content);  
    });
    server.on("/setting", []() {
        String qsid = server.arg("ssid");
        String qpass = server.arg("pass");
        if (qsid.length() > 0 && qpass.length() > 0) {
          Serial.println("clearing eeprom");
          //for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
		  EEPROM_clean_network();
          Serial.println(qsid);
          Serial.println("");
          Serial.println(qpass);
          Serial.println("");
            
          Serial.println("writing eeprom ssid:");
		  strcpy(SystemData.eep_ssid, qsid.c_str());
		  strcpy(SystemData.eep_pass, qpass.c_str());
		  EEPROM.put(eeAddress, SystemData);
		  /*
          for (int i = 0; i < qsid.length(); ++i)
            {
              EEPROM.write(i, qsid[i]);
              Serial.print("Wrote: ");
              Serial.println(qsid[i]); 
            }
          Serial.println("writing eeprom pass:"); 
          for (int i = 0; i < qpass.length(); ++i)
            {
              EEPROM.write(32+i, qpass[i]);
              Serial.print("Wrote: ");
              Serial.println(qpass[i]); 
            }
          */			
          EEPROM.commit();
          content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
          statusCode = 200;
        } else {
          content = "{\"Error\":\"404 not found\"}";
          statusCode = 404;
          Serial.println("Sending 404");
        }
        server.send(statusCode, "application/json", content);
    });
  } else if (webtype == 0) {
    server.on("/", []() {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      server.send(200, "application/json", "{\"IP\":\"" + ipStr + "\"}");
    });
    server.on("/cleareeprom", []() {
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Clearing the EEPROM</p></html>";
      server.send(200, "text/html", content);
      Serial.println("clearing eeprom");
      //for (int i = 0; i < sizeof(TDIC_SystemDictionary); ++i) { EEPROM.write(i, 0); }
	  EEPROM_clean_network();
      //EEPROM.commit();
    });
  }
}

void loop() {
    server.handleClient();
    t_Timer_0.update();
	uart_command();
	remote_water();
	sync_time(); //sync time from google every 10 minutes
	upload_cloud();
	EEP_save();

	
	if (int_date>31) return;
	
    Time_update(); // update time every 1 minute
	
	current_time = int_hour*100 + int_minute;	

	if (current_time<600)
	{
	    read_from_google = 60;
		get_time_period = 600;
		upload_period = 60;
		if (current_time_temp!=current_time)
		{
		    current_time_temp = current_time;
			Serial.println(current_time);
			Serial.println("sleep mode");
		}
        sleep_flag = true;
	    return;
	}

	if (true == sleep_flag)
	{
		Serial.println("clean");
		SystemData.date = 0;
		SystemData.spraytime = 0;
		SystemData.boot_count = 0;
		EEPROM.put(eeAddress, SystemData);
		EEPROM.commit();
		Serial.println("wait 10 seconds then reboot");
	    delay(10*1000);
	    Serial.println("========reboot=======");
		ESP.restart();			
	}

	//Serial.print("e_system_status = "); Serial.println(e_system_status); 
	switch (e_system_status)
	{
		case system_init:
		    e_system_status = power_on;
			SystemData.boot_count++;
			save_flag =1;
			upload_period = 0;
			if (SystemData.date!=int_date)
			{
				SystemData.date = int_date;
				SystemData.spraytime = 0;
				SystemData.boot_count = 0;			
			}									
		break;
		
		case power_on:		

			if (server_Power==0) return;
			
			switch (Spray_frequency)
			{
				case D1S1:
					water_time_2 = 2500;
				break;
				case D2S1:
				case D3S1:
				case D4S1:
				case D5S1:
				case D6S1:
				case D7S1:
					 water_time_2 = 2500;
					 if ((intData-SystemData.eep_intData)<Spray_frequency) return;
				break;	
			}		
			
			if (current_time<water_time_1)
			{

			}
			else if (current_time>=water_time_1)
			{
				if (SystemData.spraytime == 0)
				{
					e_system_status = spray;
					power_status_counter = water_period;
					digitalWrite(Spray_Enable_Pin, LOW);	
					upload_period = 0; 
				}
				else
				{
					e_system_status = sleep;
				}
			}
			else
			{
				e_system_status = sleep;
			}
				
		break;
			
		case spray: 
			if (0==power_status_counter)
			{
				e_system_status = sleep;
				digitalWrite(Spray_Enable_Pin, HIGH);
				SystemData.spraytime = SystemData.spraytime+1;
				SystemData.eep_intData = intData;
				save_flag = 1;												
				upload_period = 0;          
			}		    
			
		break;
			
		case sleep: 
		    
			if (current_time<water_time_2)
			{
				
			}
			else if (current_time>=water_time_2)
			{
				if (SystemData.spraytime < 2)
				{
					e_system_status = spray2;
					power_status_counter = water_period;
					digitalWrite(Spray_Enable_Pin, LOW);
					upload_period = 0;     
				}
				else
				{
					e_system_status = sleep2;
				}
				
			}
		
		break;		
			
		case spray2:			
			if (0==power_status_counter)
			{
				e_system_status = sleep2;
				digitalWrite(Spray_Enable_Pin, HIGH);
				SystemData.spraytime = SystemData.spraytime+1;
				save_flag = 1;																								
				upload_period = 0;     
			}		    
		
		break;		
		
		case sleep2:

		break;			
		
		case manual:
			if (0==power_status_counter)
			{
				e_system_status = power_on;
				digitalWrite(Spray_Enable_Pin, HIGH);											
				upload_period = 0;          
			}		    		
		break;
			
		default:
			e_system_status = power_on;
			power_status_counter = power_on_time;
			digitalWrite(Spray_Enable_Pin, HIGH);
		break;			
		
	}		  
  
}

//======================================================================================================================
void Timer0_callback_1s()
{
    VOL_check_flag = 1;
	
	if (0==LED_Status)
	{
		LED_Status = 1;
	}
	else
	{
		LED_Status = 0;
	}
     
    if (upload_period>0) upload_period--;	 
	if (get_time_period>0) get_time_period--;
	if (power_status_counter>0) power_status_counter--;
	if (time_update_count>0) time_update_count--;
	if (read_from_google>0) read_from_google--;
	
	

	if (e_system_status == spray)
	{
		LED_Status = 1;
	}
	else if (e_system_status == sleep)
	{
		//LED_Status = 1;
	}
	
	digitalWrite(ledPin, LED_Status);	


/*	
	Serial.print("e_system_status = "); Serial.print(e_system_status); 
	Serial.println();
	
	 Serial.print(power_status_counter); 
	 Serial.println();
*/	
	//Serial.print("upload_period = "); Serial.print(upload_period); 
	//Serial.println();

    //Serial.println("Timer0_callback_1s");
	
}
//====================================================================================================================
void upload_to_thingspeak() {
  WiFiClient client;
  
   //initialize your AT command string
  //String cmd = "AT+CIPSTART=\"TCP\",\"";
  String cmd;
  String temp = "20";
  String humid ="30";
  String s_stray ="0";   
  
  char c_hour[5],c_minute[5],c_vol[5],c_boot_count[5],c_status[5],c_spraytime[5];  
  //String s_hour,s_minute; 
  
  Serial.println( "===update_to_thingspeak====" );  
  
  sprintf(c_hour,"%d",int_hour);
  sprintf(c_minute,"%d",int_minute);
  sprintf(c_boot_count,"%d",SystemData.boot_count);
  sprintf(c_status,"%d",e_system_status);
  sprintf(c_spraytime,"%d",SystemData.spraytime);
       
      
  String s_hour(c_hour);
  String s_minute(c_minute);
  String s_vol(c_vol);
  String s_boot_count(c_boot_count);
  String s_status(c_status);
  String s_stray_time(c_spraytime);    
 
  
  Serial.println(int_hour);
  Serial.println(c_hour);
  
  if ((e_system_status==spray)||(e_system_status==spray2))
  {
	  s_stray ="1";  
  }
  else
  {
      s_stray ="0";  
  }	  

  Serial.print("SystemData.spraytime = "); Serial.println(SystemData.spraytime);  
  Serial.print("SystemData.boot_count = "); Serial.println(SystemData.boot_count);  
 
	if( !client.connect( HOST_thingspeak, PORT_thingspeak ) )
	{
		Serial.println( "connection failed" );
		return;
	}
	else
	{

		//build GET command, ThingSpeak takes Post or Get commands for updates, I use a Get
		cmd = GET;
		cmd += FIELD1;
		cmd += s_hour;
		cmd += FIELD2;
		cmd += s_minute;//s_hour;humid
		cmd += FIELD3;
		cmd += s_stray;
		cmd += FIELD4;
		cmd += s_stray_time;
		cmd += FIELD5;
		cmd += s_boot_count;
		cmd += FIELD6;
		cmd += s_status; 
		 
		//cmd += "\r\n";

		String getStr = cmd + " HTTP/1.1\r\n";

	    client.print( getStr );
	    client.print( "Host: api.thingspeak.com\n" );
	    client.print( "Connection: close\r\n\r\n" );
	    
      	delay(10);
      	//
      	// ?��??�端伺�??��??��?訊息，�?式碼?�以寫在?�裡�?
      	//


      	client.stop();

	}
	

  
}
//====================================================================================================================
void upload_cloud()
{
	if (0==upload_period)
	{
		upload_period = 60;
		upload_to_thingspeak();
		//Serial.println(getTime());
	}
	
}
//======================================================================================================================
void sync_time()
{
	//String s_date;
	if (0==get_time_period)
	{
		get_time_period = 600;
		get_time_form_google_sheet();
/*		
		//upload_period = 10;
		strcpy(c_theDate, getTime().c_str());	
		Serial.println(c_theDate);

        if ((sscanf(c_theDate, " %3s , %d %s %d %d:%d:%d", &c_week,&int_date,&c_month,&int_year,&int_hour,&int_minute,&int_second))==7)
	    {
		   int_hour=int_hour+8;
		   if (int_hour>=24)
		   {
			   int_hour = int_hour-24;
			   int_date = int_date + 1;			   
		   }   
		   time_update_count = 60;
		   Serial.println( "getTime OK========" ); //success! 
 		   Serial.println(c_week); 
		   Serial.println(int_date); 
		   Serial.println(c_month); 
		   Serial.println(int_year); 
		   Serial.println(int_hour); 
		   Serial.println(int_minute); 
		   Serial.println(int_second); 	        			   
	    }
	    else
	    {
		   Serial.println( "==============NOK" );
	    }	     	  		
*/		
	}
		
}
//======================================================================================================================
String getTime() 
{
	WiFiClient client;
	while (!!!client.connect("google.com", 80)) 
	{
		Serial.println("connection failed, retrying...");
    }

	client.print("HEAD / HTTP/1.1\r\n\r\n");

	while(!!!client.available()) {
		yield();
	}

	while(client.available())
	{
		if (client.read() == '\n') { 
			if (client.read() == 'D') { 
				if (client.read() == 'a') { 
					if (client.read() == 't') { 
						if (client.read() == 'e') { 
							if (client.read() == ':') { 
								client.read();
								String theDate = client.readStringUntil('\r');
								client.stop();
								return theDate;
							}
						}
					}
				}
			}
		}
	}
}
//======================================================================================================================
void get_time_form_google_sheet(void)
{
	connect_to_google(2);
}
//======================================================================================================================
void EEPROM_clean_network()
{
	//Serial.println("EEPROM_clean_network...");
	memset (SystemData.eep_ssid, 0, sizeof(SystemData.eep_ssid));
	memset (SystemData.eep_pass, 0, sizeof(SystemData.eep_pass));
    //Serial.println(SystemData.eep_ssid);
	//Serial.println(SystemData.eep_pass);	
	EEPROM.put(eeAddress, SystemData);
	EEPROM.commit();
}
//====================================================================================================================
void uart_command()
{
  char command[50];
  char ch;
  unsigned int id;
  
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:

    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\r') 
	{
        stringComplete = true;
        Serial.println();
    }
	else
	{
		Serial.print(inChar);
		inputString += inChar;		
	}
	
  }
  
    if (stringComplete) 
    {
    
        stringComplete = false;
		
        if (inputString=="reboot")
        {
			Serial.println("========reboot=======");
			ESP.restart();	
        }		
		
		strcpy(command, inputString.c_str());
        inputString = "";	
	
		switch (command[0])
		{
			case 'h':
				 Serial.println("High");
				 digitalWrite(Spray_Enable_Pin, LOW);			
			break;

			case 'l':
				 Serial.println("Low");
				 digitalWrite(Spray_Enable_Pin, HIGH);			
			break;
			
			case 's':
				upload_period = 0;
				Serial.println("spray");
				e_system_status = spray;
				power_status_counter = water_period;
				digitalWrite(Spray_Enable_Pin, LOW);	        			
			break;
			
			case 'c':
				Serial.println("clean");
				SystemData.date = 0;
				SystemData.spraytime = 0;
				SystemData.boot_count = 0;
				EEPROM.put(eeAddress, SystemData);
				EEPROM.commit();			
			break;
			
			case 'n':
				if ((sscanf(command, "%s %d", &ch,&id))==2)
				{
					SystemData.id = id;
					save_flag = 1;
					Serial.print("New HW ID = "); Serial.println(SystemData.id);
				}
					
			break;			

			default:
			    Serial.println("h : Spray_Enable_Pin = high");
				Serial.println("l : Spray_Enable_Pin = low");
				Serial.println("s : Spray now");
				Serial.println("c : clean spay data ");
				Serial.println("n id : set HW ID ");
			break;
			
		}


    }  

	
}
//====================================================================================================================
void connect_to_google(int cell)
{
   String cmd;
   String cmd_all;  
   char c_id[10];   
   char  *cPtr;
   int wLen,j,i;
   HTTPSRedirect* client = nullptr;
   

   sprintf(c_id,"%u",SystemData.id);
   String s_id(c_id);
   
  if (false==WiFi_connected) return;
     
  //HTTPSRedirect client(httpsPort);
  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");  
  
  Serial.print("Connecting to ");
  Serial.println(host_google);

  bool flag = false;
  for ( i=0; i<5; i++){
    //int retval = client.connect(host_google, httpsPort);
	int retval = client->connect(host_google, httpsPort);
    if (retval == 1) {
       flag = true;
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  Serial.print("flag = ");
  Serial.println(flag);
	
  if (!flag){
    Serial.print("Could not connect to server: ");
    Serial.println(host_google);
    Serial.println("Exiting...");
	delete client;
	client = nullptr;
    return;
  }


  Serial.flush();
/*
  if (client.verify(fingerprint, host_google)) {
    Serial.println("Certificate match.");
  } else {
    Serial.println("Certificate mis-match");
  }
*/
  // Note: setup() must finish within approx. 1s, or the the watchdog timer
  // will reset the chip. Hence don't put too many requests in setup()
  // ref: https://github.com/esp8266/Arduino/issues/34
  
  //Serial.print("Requesting URL: ");
  //Serial.println(url);
  
/* 
  if (!client.connected())
    client.connect(host_google, httpsPort);
*/

  Serial.println("read form google sheet");
 
 
  switch (cell)
  {
  
	  case 0:	      
	      Serial.print("s_id = ");Serial.println(s_id); 
	      cmd = "cmd=read&id="+s_id;
	      cmd_all = url_google + cmd;	  
		  //url_read = url_read + s_id;	  
          Serial.println(cmd_all); 
		  
		  //client.printRedir(cmd_all, host_google, googleRedirHost); 
		  client->GET(cmd_all, host_google);
		  cPtr = Getdata_form_google();
		  wLen = strlen(cPtr);
		  Serial.print("wLen = ");Serial.println(wLen); 
		  
		  Serial.print("cPtr = ");
          Serial.println(cPtr);
		  
     
		  if ((sscanf(cPtr, "%d,", &spray_data[0]))==1)
		  {
				Serial.print("spray_data[0] = ");Serial.println(spray_data[0]); 
		  }
		  
		  j=1;
		  
		  for(i=0; i<wLen; i++)
		  {	
			  if(*cPtr == ',')
			  {
				  if (j<=50)
				  {
					  if ((sscanf(cPtr, ",%d,", &spray_data[j]))==1)
					  {      
						//rial.print("i = ");Serial.println(i); 
						Serial.print("j = ");Serial.print(j); 
						Serial.print("spray_data[] = ");Serial.println(spray_data[j]); 
						j++;
					  }					  
				  }
				  else if (j==51)
				  {
					  if ((sscanf(cPtr, ",%d", &spray_data[j]))==1)
					  {      
						//Serial.print("i = ");Serial.println(i); 
						Serial.print("j = ");Serial.print(j); 
						Serial.print("spray_data[] = ");Serial.println(spray_data[j]); 
						j++;
					  }					  					  
				  }
					  

				   cPtr++;
			  }
			  else
			  {
				  cPtr++;
			  }	
		   }	 
		   
		   Serial.print("int_month = ");Serial.println(int_month); 
		   
		   Spray_frequency = spray_data[int_month*4+data_frequency];
		   water_time_1 = spray_data[int_month*4+data_time_1];
		   water_time_2 = spray_data[int_month*4+data_time_2];
		   water_period = spray_data[int_month*4+data_period];

		   if (D1S2 != Spray_frequency)
		   {
			   water_time_2 = 2500;
		   }	
			
    	   Serial.print("Spray_frequency = ");Serial.println(Spray_frequency);    
		   Serial.print("water_time_1 = ");Serial.println(water_time_1); 
		   Serial.print("water_time_2 = ");Serial.println(water_time_2); 
		   Serial.print("water_period = ");Serial.println(water_period); 
		   
		   Serial.print("server_spray_now = ");Serial.println(server_spray_now); 
		   Serial.print("water_enable = ");Serial.println(water_enable); 
	 
	
		  if (server_spray_now!=water_enable)
		  {
			  water_enable = server_spray_now;
			  if (e_system_status == system_init) break;
	          Serial.println("spray");
			  e_system_status = manual;
			  power_status_counter = water_period;
			  digitalWrite(Spray_Enable_Pin, LOW);	   
   			  upload_period = 0;
		  }		  
		  
	  break;
	  
	  case 1:
		
	  break;
	  
	  case 2:
	  
	      cmd = "cmd=time";
	      cmd_all = url_google + cmd;	
		  //client.printRedir(cmd_all, host_google, googleRedirHost);  
		  if (client->GET(cmd_all, host_google))
		  	{    
		  	    Serial.print("client->GET OK ");
			}  
		  else{
		  	    Serial.print("client->GET Error ");
			}
		  
          if ((sscanf(Getdata_form_google(), "%d-%d-%d %d:%d",&int_year,&int_month,&int_date,&int_hour,&int_minute))==5)
	      {
			   Serial.print("int_year = ");Serial.println(int_year); 
			   Serial.print("int_month = ");Serial.println(int_month); 
			   Serial.print("int_date = ");Serial.println(int_date); 
			   Serial.print("int_hour = ");Serial.println(int_hour); 
			   Serial.print("int_minute = ");Serial.println(int_minute); 

		  }	
		  
		  delay(1000);
	      cmd = "cmd=time&id=0";
	      cmd_all = url_google + cmd;	
		  //client->printRedir(cmd_all, host_google, googleRedirHost);  		  
		  if (client->GET(cmd_all, host_google))
			{	 
				Serial.print("client->GET OK");
			}  
		  else{
				Serial.print("client->GET Error");
			}
          
		  if ((sscanf(Getdata_form_google(), "%d",&intData))==1)
	      {
			   Serial.print("intData!!! = ");Serial.println(intData); 
			   Serial.print("ABS!!! = ");Serial.println(abs(intData-SystemData.eep_intData)); 
			   if (abs(intData-SystemData.eep_intData)>=D_Max)
			   {
				   SystemData.eep_intData = intData;
				   save_flag = 1;
			   }			   
		  }
		  		  
	  break;

  }

 	delete client;
	client = nullptr;
	
}
//====================================================================================================================
void remote_water(void)
{
  if (read_from_google>0) return;  
  read_from_google = 60;  
  connect_to_google(0);	
}
//====================================================================================================================
void EEP_save(void)
{
    if (1==save_flag)
	{
		save_flag = 0;
		EEPROM.put(eeAddress, SystemData);
		EEPROM.commit();
	}		
}
//====================================================================================================================
void Time_update(void)
{
	if (0==time_update_count)
	{
		upload_period = 10;
		time_update_count = 60;
		int_minute++;
		if (60==int_minute)
		{
			int_minute = 0;
			int_hour++;
		}		
		Serial.print(int_hour); Serial.print(":"); Serial.println(int_minute);	
		Serial.print("Spray_frequency = "); Serial.println(Spray_frequency); 
		Serial.print("water_time_1 = "); Serial.println(water_time_1); 
		Serial.print("water_time_2 = "); Serial.println(water_time_2); 
		Serial.print("current_time = "); Serial.println(current_time); 
		Serial.print("e_system_status = "); Serial.println(e_system_status); 
		

	}	
}
//====================================================================================================================
