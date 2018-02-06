/*HTTP GET REQUEST NOTES:
* / :
*    function: goes to home page
*    arguments: none
*
* /setalarm:
*   function: sets alarm clock time
*   arguments: state   = (0 to turn off alarm clock)
*                        (1 to turn on alarm clock)
*              hour    = (hour of alarm in militar time 0-23)
*              min     = (minute of alarm 0-59)
*
* /setlamp:
*   function: control the lamp directly or set automation
*   arguments: state   = (0 to turn off music lamp)
*                        (1 to turn on music lamp)
*                        (2 to automate music lamp)
*              display = (0 for fire animation)
*                        (1 for music light show)
*              onhour  = (hour to turn on music lamp in military time 0-23)
*              onmin   = (minute to turn on music lamp 0-50)
*              offhour = (hour to turn off music lamp in military time 0-23)
*              offmin  = (minute to turn off music lamp 0-50)
*
* /mp3:
*   function: control the mp3 player directly or set automation
*   arguments: command = (1 to play song)
*                        (2 to pause song)
*                        (3 to go to next song)
*                        (4 to go to previous song)
*                        (5 to wake up)
*                        (6 to sleep)
*                        (7 to increase volume)
*                        (8 to decrease volume)
*              automation= (0 to turn off mp3 player automation)
*                          (1 to turn on mp3 player automation)
*              onhour  = (hour to turn on mp3 player in military time 0-23)
*              onmin   = (minute to turn on mp3 player 0-50)
*              offhour = (hour to turn off mp3 player in military time 0-23)
*              offmin  = (minute to turn off mp3 player 0-50)
*
* /reminder:
*   function: set automation on message on clock lcd screen
*   arguments: state   = (0 to hide automated message)
*                      = (1 to show automated message)
*              onhour  = (hour to display message in military time 0-23)
*              onmin   = (minute to display message 0-50)
*              offhour = (hour to hide message in military time 0-23)
*              offmin  = (minute to hide message 0-50)
*              brightness = (set brightness of lcd, pass in value 0-255)
*              message = (type out message. put '-' in each space and
*                          set maximum of 40 characters)
*
* /bluetooth:
*   function: turn bluetooth on or offmin
*   arguments: state   = (0 to turn off bluetooth)
*                      = (1 to turn on bluetooth)
*
* /lampcommand:
*   function: send command to music lamps
              sends the state then display setting.
*   arguments: none
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>

#define timer0_preload 40161290
#define DELAY_TIME        6
#define LAMP_STATE        0
#define LAMP_DISPLAY      1
#define LAMP_ON           1
#define LAMP_AUTOMATED    2
#define ALARM_STATE       0
#define OFF               0
#define ON                1
#define FIRE_MODE         0
#define MUSIC_MODE        1
#define LAMP_ON_HOUR      2
#define LAMP_ON_MIN       3
#define LAMP_OFF_HOUR     4
#define LAMP_OFF_MIN      5
#define ALARM_HOUR        1
#define ALARM_MIN         2
#define MP3_COMMAND       0
#define MP3_AUTOMATION    1
#define MP3_ON_HOUR       2
#define MP3_ON_MIN        3
#define MP3_OFF_HOUR      4
#define MP3_OFF_MIN       5
#define DO_NOTHING        0
#define PLAY              1
#define PAUSE             2
#define NEXT_SONG         3
#define PREVIOUS_SONG     4
#define WAKE_UP           5
#define SLEEP             6
#define VOL_UP            7
#define VOL_DOWN          8

#define REMINDER_STATE    0
#define REMINDER_ON_HOUR  1
#define REMINDER_ON_MIN   2
#define REMINDER_OFF_HOUR 3
#define REMINDER_OFF_MIN  4
#define REMINDER_MESSAGE  5

#define YEAR              1
#define MONTH             2
#define DAY               3
#define HOUR              4
#define MIN               5

#define IP_TYPE           5
#define TIME_TYPE         1
#define ALARM_TYPE        2
#define MP3_TYPE          3
#define REMINDER_TYPE     4
#define BLUETOOTH_TYPE    6

String payload;
String time_out;
String buffer[6];
String name_buffer[6];
int http_code;
int data_recieved[5];
int hour,min,sec;
int day, month, year;
int lamp_command;
String reminder_checks[7] = {"state","onhour","onmin","offhour","offmin","brightness","message"};
int reminder_data[6] = {0,0,0,0,0,255};
String reminder_message = " ";
String mp3_checks[6] = {"command","automation","onhour","onmin","offhour","offmin"};
int mp3_data[6] = {0,0,0,0,0,0};
int alarm_data[3] = {0,0,0};
String lamp_checks[6] = {"state","display","onhour","onmin","offhour","offmin"};
int lamp_data[6] = {0,0,0,0,0,0};
byte time_data[6];
String alarm_checks[3] = {"state","hour","min"};
String bluetooth_checks[1] = {"state"};
int bluetooth_data[1] = {0};
String ip_address;
bool send_alarm = false,send_mp3 = false,send_remind = false;
ESP8266WebServer server(80);
HTTPClient http;
unsigned long last_time = 0;
unsigned long this_time;

const char* ssid = "MasterShenwei";
const char* password = "sherwin210";

void start_up(const char* ssid, const char* password){
  WiFi.begin(ssid, password);
  //Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
//  Serial.println("");
//  Serial.print("Connected to ");
  //Serial.println(ssid);

  ip_address = String(WiFi.localIP());
  Serial.write((byte)IP_TYPE);
  Serial.print(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    //Serial.println("MDNS responder started");
  }
}

void show_home_page() {
  server.send(200, "text/plain", "LETS GET DIS SHIT STARTED!!!");
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void get_arguments(int argument_count){
  for(int i = 0; i < argument_count; i ++){
    buffer[i] = server.arg(i);
  }
  for(int i = 0; i < argument_count; i++){
    name_buffer[i] = server.argName(i);
  }
}

void check_arguments(String raw_arguments[], String raw_data[], String check_strings[], int variables[],int argument_count,int size){
  int k = 0;
  while(k < size){
    for(int i = 0; i < argument_count; i++){
      if(raw_arguments[i] == check_strings[k]){
        variables[k] = raw_data[i].toInt();
      }
    }
    k++;
  }
}

void send_data(int type, int data[],int end,int begin = 0){
  Serial.write((byte)type);
  for(int i = begin; i < end; i++){
    Serial.write((byte)data[i]);
  }
}

String time_string(int hour, int min){
  String message;
  message += hour;
  if(min < 9){
    message += ":0";
    message += min;
  }
  else{
    message += ":";
    message += min;
  }
  return message;
}

void update_alarm(){
  int argument_count =  server.args();
  if(argument_count > 0){
    get_arguments(argument_count);
    check_arguments(name_buffer,buffer,alarm_checks,alarm_data,argument_count,3);
    send_data(ALARM_TYPE,alarm_data,3);
  }
  String message = "alarm time set ";
  if(alarm_data[ALARM_STATE]){
    message += "on ";
  }
  else{
    message += "off ";
  }
    message += "at ";
  message += time_string(alarm_data[ALARM_HOUR], alarm_data[ALARM_MIN]);
  server.send(200,"text/plain", message);
}

void lamp_adjust(){
  int argument_count = server.args();
  if(argument_count > 0){
    get_arguments(argument_count);
    check_arguments(name_buffer,buffer,lamp_checks,lamp_data,argument_count,6);
  }
  String message = "lamp is ";
  switch(lamp_data[LAMP_STATE]){
    case OFF: message += "off ";break;
    case ON: message += "on ";break;
    case LAMP_AUTOMATED: message += "automated ";break;
  }
  message += "and set to ";
  switch(lamp_data[LAMP_DISPLAY]){
    case FIRE_MODE: message += "fire mode.\n\n";break;
    case MUSIC_MODE: message += "music mode.\n\n";break;
  }
  message += "turns on at ";
  message += time_string(lamp_data[LAMP_ON_HOUR], lamp_data[LAMP_ON_MIN]);
  message += "\nturns off at ";
  message += time_string(lamp_data[LAMP_OFF_HOUR], lamp_data[LAMP_ON_MIN]);

  server.send(200,"text/plain",message);
}

bool between(int current_hour, int current_min, int start_hour, int start_min, int end_hour, int end_min){
  int k = start_hour;
  int hour_difference = 0;
  int current_hour_difference = 0;
  while(k != end_hour){
    k++;
    hour_difference ++;
    if(k > 23){
      k = 0;
    }
  }
  k = current_hour;
  while(k != end_hour){
    k++;
    current_hour_difference ++;
    if(k > 23){
      k = 0;
    }
  }

  if(current_hour_difference < hour_difference && current_hour_difference != 0){
    return true;
  }
  else if(current_hour_difference == hour_difference){
    if(current_min >= start_min){
      return true;
    }
    else{
      return false;
    }
  }
  else if(current_hour_difference == 0){
    if(current_min <= end_min){
      return true;
    }
    else{
      return false;
    }
  }
  else{
      return false;
  }
}

void interpret_lamp_command(){
  if(lamp_data[LAMP_STATE] == OFF){
    lamp_command = OFF;
  }

  else if(lamp_data[LAMP_STATE] == ON){
    lamp_command = ON;
  }

  else if(lamp_data[LAMP_STATE] == LAMP_AUTOMATED){
    if(between(hour,min,lamp_data[LAMP_ON_HOUR],lamp_data[LAMP_ON_MIN],lamp_data[LAMP_OFF_HOUR],lamp_data[LAMP_OFF_MIN])){
      lamp_command = ON;
    }
    else{
      lamp_command = OFF;
    }
  }
}

void send_lamp_command(){
  interpret_lamp_command();
  String message;
  message += lamp_command;
  message += lamp_data[LAMP_DISPLAY];
  server.send(200,"text/plain",message);
}

void adjust_mp3(){
  int argument_count = server.args();
  if (argument_count > 0){
    get_arguments(argument_count);
    check_arguments(name_buffer,buffer,mp3_checks,mp3_data,argument_count,6);
    send_data(MP3_TYPE,mp3_data,6);
  }
  String message = "mp3 told to ";
  switch (mp3_data[MP3_COMMAND]) {
    case DO_NOTHING: message += "do nothing";break;
    case PLAY: message += "play song";break;
    case PAUSE: message += "pause song";break;
    case NEXT_SONG: message += "go to next song";break;
    case PREVIOUS_SONG: message += "go to previous song";break;
    case WAKE_UP: message += "wake up";break;
    case SLEEP: message += "go to sleep";break;
    case VOL_UP: message += "increase volume";break;
    case VOL_DOWN: message += "decrease volume";break;
    default: message += "do nothing";break;
  }
  message +="\n\nautomation is ";
  if(mp3_data[MP3_AUTOMATION]){
    message += "on";
  }
  else{
    message += "off";
  }
  message += "\nturns on at ";
  message += time_string(mp3_data[MP3_ON_HOUR], mp3_data[MP3_ON_MIN]);
  message += "\n turns off at ";
  message += time_string(mp3_data[MP3_OFF_HOUR],mp3_data[MP3_OFF_MIN]);

  server.send(200,"text/plain",message);
  mp3_data[MP3_COMMAND] = DO_NOTHING;
}

void set_bluetooth(){
  int argument_count =  server.args();
  if(argument_count > 0){
    get_arguments(argument_count);
    check_arguments(name_buffer,buffer,bluetooth_checks,bluetooth_data,argument_count,1);
    Serial.write(6);
    Serial.write((byte) bluetooth_data[0]);
  }
  String message = "bluetooth set ";
  if(bluetooth_data[0]){
    message += "on ";
  }
  else{
    message += "off ";
  }
  server.send(200,"text/plain", message);
}

void set_reminder(){
  int argument_count = server.args();
  if(argument_count > 0){
    get_arguments(argument_count);
    check_arguments(name_buffer,buffer,reminder_checks,reminder_data,argument_count,6);

    for(int i = 0; i < argument_count; i++){
      if(name_buffer[i] == reminder_checks[REMINDER_MESSAGE+1]){
        reminder_message = buffer[i];
      }
    }
    int mes_length = reminder_message.length();

    byte send_buffer[8];
    send_buffer[0] = REMINDER_TYPE;
    send_buffer[1] = mes_length;
    send_buffer[2] = reminder_data[REMINDER_STATE];
    send_buffer[3] = reminder_data[REMINDER_ON_HOUR];
    send_buffer[4] = reminder_data[REMINDER_ON_MIN];
    send_buffer[5] = reminder_data[REMINDER_OFF_HOUR];
    send_buffer[6] = reminder_data[REMINDER_OFF_MIN];
    send_buffer[7] = reminder_data[5];
    Serial.write(send_buffer,8);
    Serial.print(reminder_message);
  }
  String message = "reminder is: ";
  message += reminder_message;
  message += "\n\nreminder set ";
  if(reminder_data[REMINDER_STATE]){
    message += "on to";
  }
  else{
    message += "off to";
  }
  message += "\nshow at ";
  message += time_string(reminder_data[REMINDER_ON_HOUR],reminder_data[REMINDER_ON_MIN]);
  message += "\nhide at ";
  message += time_string(reminder_data[REMINDER_OFF_HOUR],reminder_data[REMINDER_OFF_MIN]);
  message += "\nbrightness set to ";
  message += (String) reminder_data[5];

  server.send(200,"text/plain",message);
}

void decode_time_out(String data_in){
  //Serial.println(data_in);
  buffer[0] = data_in.substring(1,5);
  buffer[1] = data_in.substring(6,8);
  buffer[2] = data_in.substring(9,11);
  buffer[3] = data_in.substring(12,14);
  buffer[4] = data_in.substring(15,17);
  buffer[5] = data_in.substring(18,20);

  time_data[YEAR] = (byte) (buffer[0].toInt() - 2000);
  time_data[MONTH] = (byte) buffer[1].toInt();
  time_data[DAY] = (byte) buffer[2].toInt();
  time_data[HOUR] =(byte) buffer[3].toInt();
  time_data[MIN] = (byte) buffer[4].toInt();
}

void setup(){
  Serial.begin(9600);
  start_up("MasterShenwei","sherwin210");

  server.on("/", show_home_page);
  server.on("/setlamp", lamp_adjust);
  server.on("/setalarm", update_alarm);
  server.on("/lampcommand", send_lamp_command);
  server.on("/mp3", adjust_mp3);
  server.on("/reminder", set_reminder);
  server.on("/bluetooth", set_bluetooth);
  server.onNotFound(handleNotFound);

  server.begin();
}

void loop(){
  this_time = millis();
  if(this_time - last_time >= 5000){
    http.begin("http://api.timezonedb.com/v2/get-time-zone?key=60K12JHZKLV3&by=zone&zone=America/Los_Angeles&format=json&fields=formatted");
    http_code = http.GET();
    if(http_code > 0){
      if(http_code == HTTP_CODE_OK){
          payload = http.getString();
          time_out = payload.substring(40);
          decode_time_out(time_out);
          time_data[0] = (byte) TIME_TYPE;
          Serial.write(time_data,6);

      }
    }
    http.end();
    last_time = this_time;
  }
  server.handleClient();
}
