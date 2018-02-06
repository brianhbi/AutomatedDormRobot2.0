#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>

#define ESP_RX            3
#define ESP_TX            2
#define MP3_TX            4
#define MP3_RX            5

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
#define REMINDER_BRIGHTNESS 5

#define BLUETOOTH_PIN     8
#define LDC_BRIGHTNESS_PIN  9
#define ALARM_OFF_BUTTON  7
#define TIME_DATA         0
#define ALARM_DATA        1
#define MP3_DATA          2
#define REMINDER_DATA     3

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

#define CMD_NEXT_SONG     0X01
#define CMD_PREV_SONG     0X02
#define CMD_PLAY_W_INDEX  0X03
#define CMD_VOLUME_UP     0X04
#define CMD_VOLUME_DOWN   0X05
#define CMD_SET_VOLUME    0X06

#define CMD_SNG_CYCL_PLAY 0X08
#define CMD_SEL_DEV       0X09
#define CMD_SLEEP_MODE    0X0A
#define CMD_WAKE_UP       0X0B
#define CMD_RESET         0X0C
#define CMD_PLAY          0X0D
#define CMD_PAUSE         0X0E
#define CMD_PLAY_FOLDER_FILE 0X0F

#define CMD_STOP_PLAY     0X16
#define CMD_FOLDER_CYCLE  0X17
#define CMD_SHUFFLE_PLAY  0x18
#define CMD_SET_SNGL_CYCL 0X19

#define CMD_SET_DAC       0X1A
#define DAC_ON            0X00
#define DAC_OFF           0X01

#define CMD_PLAY_W_VOL    0X22
#define CMD_PLAYING_N     0x4C
#define CMD_QUERY_STATUS      0x42
#define CMD_QUERY_VOLUME      0x43
#define CMD_QUERY_FLDR_TRACKS 0x4e
#define CMD_QUERY_TOT_TRACKS  0x48
#define CMD_QUERY_FLDR_COUNT  0x4f

#define BRIGHTNESS_PIN      9
#define DEV_TF            0X02

SoftwareSerial esp_serial(ESP_RX,ESP_TX);
SoftwareSerial mp3_serial(MP3_RX,MP3_TX);
LiquidCrystal_I2C lcd(0X3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

bool update_lcd = false;
int hour = 0, min = 0, year = 0, month = 0, day = 0;
int reminder_data[6] = {0,0,0,0,0,255};
int mp3_data[6] = {0,0,0,0,0,0};
int alarm_data[3] = {0,0,0};
int time_data[5] = {0,0,0,0,0};
int volume = 20;
bool play_once = false;
String reminder_message;
String ip_address;
bool do_once = false;
bool mp3_between = false;
bool is_playing = false;
String mp3Answer;
uint8_t alarm_number = 1;
bool mp3_awake = false;
unsigned long then = 0;
unsigned long now;


void wait_for_esp(int delay_time){
  delay(delay_time);
  int i = 10;
  while(!esp_serial.available()){
    delay(800);
    lcd.setCursor(i,3);
    lcd.print(".");
    Serial.print(".");
    if(i < 19){
      i++;
    }
  }
  Serial.println();
}

void get_esp_out(){
  if(esp_serial.available()){
    int type = (int) esp_serial.peek();
    int buffer_length;

    switch(type){
      case IP_TYPE:
        buffer_length = esp_serial.available();
        break;
      case TIME_TYPE:
        buffer_length = 6;
        break;
      case ALARM_TYPE:
        buffer_length = 4;
        break;
      case MP3_TYPE:
        buffer_length = 7;
        break;
      case REMINDER_TYPE:
        Serial.println(esp_serial.read());
        buffer_length = 7 + ((int)esp_serial.peek());
        break;
      case 6:
        buffer_length = 2;
        break;
      default:
        while(esp_serial.available()>0){
          esp_serial.read();
        }
        Serial.println("Invalid Type Indicator");
    }

    byte buffer[buffer_length];
    esp_serial.readBytes(buffer,buffer_length);

    switch (type) {
      case IP_TYPE:
        Serial.println("IP ADDRESS");
        update_ip_address(buffer,buffer_length);
        break;
      case TIME_TYPE:
        Serial.println("TIME");
        update_time_data(buffer,buffer_length);
        break;
      case ALARM_TYPE:
        Serial.println("ALARM");
        update_alarm_data(buffer,buffer_length);
        break;
      case MP3_TYPE:
        Serial.println("MP3");
        update_mp3_data(buffer,buffer_length);
        break;
      case REMINDER_TYPE:
        update_reminder_data(buffer,buffer_length);
        break;
      case BLUETOOTH_TYPE:
        Serial.println("BLUETOOTH");
        Serial.println(buffer[1]);
        if(buffer[1] == 1){
          digitalWrite(8,HIGH);
        }
        else{
          digitalWrite(8,LOW);
        }
    }
  }
}

void update_ip_address(byte buffer[],int buffer_length){
  for(int i = 1; i < buffer_length;i++){
    ip_address = String(ip_address + String((char) buffer[i]));
  }
  Serial.print("IP Address: ");
  Serial.println(ip_address);
  Serial.println();
}

void send_segment_command(int dig0,int dig1,int dig2,int dig3){
  Wire.beginTransmission(0x08);
  Wire.write((byte) dig0);
  Wire.write((byte) dig1);
  Wire.write((byte) dig2);
  Wire.write((byte) dig3);
  Wire.endTransmission();
}

void write_time(int military_hour, int military_min){
  int hour,min;
  if(military_hour > 12){
    hour = military_hour - 12;
  }
  else if(military_hour == 0){
    hour = 12;
  }
  else{
    hour = military_hour;
  }

  min = military_min;
  byte digit0, digit1, digit2, digit3;

  if(hour < 10){
    digit0 = 0;
    digit1 = (byte) hour;
  }
  else{
    digit0 = (byte) (hour / 10);
    digit1 = (byte) (hour % 10);
  }

  if(min < 10){
    digit2 = 0;
    digit3 = (byte) min;
  }
  else{
    digit2 = (byte) (min / 10);
    digit3 = (byte) (min % 10);
  }

  send_segment_command(digit0, digit1, digit2, digit3);
}

void update_lcd_date(){
  lcd.setCursor(0,0);
  lcd.print("                    ");
  lcd.setCursor(0,1);
  lcd.print("                    ");
  lcd.setCursor(0,0);
  lcd.print("Today is:");
  lcd.setCursor(0,1);
  String month_name;
  switch(month){
    case 1:
      month_name = "January";
      break;
    case 2:
      month_name = "February";
      break;
    case 3:
      month_name = "March";
      break;
    case 4:
      month_name = "April";
      break;
    case 5:
      month_name = "May";
      break;
    case 6:
      month_name = "June";
      break;
    case 7:
      month_name = "July";
      break;
    case 8:
      month_name = "August";
      break;
    case 9:
      month_name = "September";
      break;
    case 10:
      month_name = "October";
      break;
    case 11:
      month_name = "November";
      break;
    case 12:
      month_name = "December";
      break;
  }

  lcd.print(month_name);
  lcd.print(" ");
  lcd.print(day);
}

void update_time_data(byte buffer[],int buffer_length){
  int new_day;
  year = ((int) buffer[YEAR]) + 2000;
  month = (int) buffer[MONTH];
  new_day = (int) buffer[DAY];
  hour = (int) buffer[HOUR];
  min = (int) buffer[MIN];

  write_time(hour,min);
  if(new_day != day){
    update_lcd = true;
    day = new_day;
  }


  //  Serial.println("updatint date");


  Serial.print("Year: ");Serial.println(year);
  Serial.print("Month: ");Serial.println(month);
  Serial.print("Day: ");Serial.println(day);
  Serial.print("Hour: "); Serial.println(hour);
  Serial.print("Min: ");Serial.println(min);
  Serial.println();
}

void send_mp3_command(int8_t command, int16_t dat,bool empty_return=false){
  int8_t Send_buf[10];

  Send_buf[0] = 0x7e;   //
  Send_buf[1] = 0xff;   //
  Send_buf[2] = 0x06;   // Len
  Send_buf[3] = command;//
  Send_buf[4] = 0x01;   // 0x00 NO, 0x01 feedback
  Send_buf[5] = (int8_t)(dat >> 8);  //datah
  Send_buf[6] = (int8_t)(dat);       //datal
  Send_buf[7] = 0xef;   //
  Serial.print("Sending: ");
  for (uint8_t i = 0; i < 8; i++)
  {
    mp3_serial.write(Send_buf[i]) ;
  }
  Serial.println();
  delay(50);
}

void empty_mp3_available(){
  if(!mp3_serial.isListening()){
    mp3_serial.listen();
  }
  if(mp3_serial.available()){
    while(mp3_serial.available()){
      while(mp3_serial.available() % 10 != 0){}
      for(int i = 0; i < 10; i++){
        mp3_serial.read();
      }
    //  delay(50);
    }
  }
}

void get_mp3_out(byte buffer[]){
  if(mp3_serial.available() && mp3_awake){
    while(mp3_serial.available() % 10 != 0){}
    for(int i = 0; i < 10; i++){
      buffer[i] = mp3_serial.read();
    }
    if(buffer[0] == 0x7E && buffer[9] == 0xEF){
      Serial.println("data recieved");
    }
    else{
      Serial.println("bullshit recieved");
    }
  }
}

void update_alarm_data(byte buffer[],int buffer_length){
  alarm_data[ALARM_STATE] = (int) buffer[ALARM_STATE+1];
  alarm_data[ALARM_HOUR] = (int) buffer[ALARM_HOUR+1];
  alarm_data[ALARM_MIN] = (int) buffer[ALARM_MIN+1];

  Serial.print("state: ");Serial.println(alarm_data[ALARM_STATE]);
  Serial.print("hour: ");Serial.println(alarm_data[ALARM_HOUR]);
  Serial.print("min :");Serial.println(alarm_data[ALARM_MIN]);
  Serial.println();
  play_once = true;
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

void play_alarm(){
  if(!mp3_awake){
    execute_mp3_command(WAKE_UP);
  }
    send_mp3_command(CMD_PLAY_W_INDEX, alarm_number);
}

void check_alarm(){
  if(alarm_data[ALARM_STATE] && play_once){
    if(hour == alarm_data[ALARM_HOUR] && min == alarm_data[ALARM_MIN]){
      play_alarm();
      send_mp3_command(CMD_SET_VOLUME, 20);
      volume = 20;
      unsigned long prev = millis();
      unsigned long now;
      while(digitalRead(7)){
        now = millis();
        if(now - prev >= 2000 && volume < 30){
          Serial.println("up");
          execute_mp3_command(VOL_UP);
          prev = now;
        }
      }
      execute_mp3_command(SLEEP);
      mp3_awake = false;
    }
  }
  play_once = false;
}

void update_mp3_data(byte buffer[],int buffer_length){
  mp3_data[MP3_COMMAND] = (int) buffer[MP3_COMMAND+1];
  mp3_data[MP3_AUTOMATION] =(int) buffer[MP3_AUTOMATION+1];
  mp3_data[MP3_ON_HOUR] = (int) buffer[MP3_ON_HOUR+1];
  mp3_data[MP3_ON_MIN] = (int) buffer[MP3_ON_MIN+1];
  mp3_data[MP3_OFF_HOUR] = (int) buffer[MP3_OFF_HOUR+1];
  mp3_data[MP3_OFF_MIN] = (int) buffer[MP3_OFF_MIN+1];

  Serial.print("command: "); Serial.println(mp3_data[MP3_COMMAND]);
  Serial.print("automation: "); Serial.println(mp3_data[MP3_AUTOMATION]);
  Serial.print("on hour: "); Serial.println(mp3_data[MP3_ON_HOUR]);
  Serial.print("on min: "); Serial.println(mp3_data[MP3_ON_MIN]);
  Serial.print("off hour: "); Serial.println(mp3_data[MP3_OFF_HOUR]);
  Serial.print("off min: "); Serial.println(mp3_data[MP3_OFF_MIN]);
  Serial.println();

  if(mp3_data[MP3_COMMAND]){
    execute_mp3_command((int) mp3_data[MP3_COMMAND]);
  }
  do_once = true;
}

void execute_mp3_command(int command){
  switch(command){
    case PLAY:
      send_mp3_command(CMD_PLAY,0);
      avoid_alarm();
      digitalWrite(6,LOW);
      is_playing = true;
      break;
    case PAUSE:
      send_mp3_command(CMD_PAUSE,0);
      is_playing = false;
      digitalWrite(6,HIGH);
      break;
    case NEXT_SONG:
      send_mp3_command(CMD_NEXT_SONG,0);
      is_playing = true;
      avoid_alarm();
      digitalWrite(6,LOW);
      break;
    case PREVIOUS_SONG:
      send_mp3_command(CMD_PREV_SONG,0);
      is_playing = true;
      avoid_alarm();
      digitalWrite(6,LOW);
      break;
    case WAKE_UP:
      send_mp3_command(CMD_WAKE_UP,0);
      is_playing = false;
      mp3_awake = true;
      break;
    case SLEEP:
      send_mp3_command(CMD_SLEEP_MODE,0);
      is_playing = false;
      mp3_awake = false;
      digitalWrite(6,HIGH);
      break;
    case VOL_UP:
      send_mp3_command(CMD_VOLUME_UP,0);
      if(volume < 30){
        volume ++;
      }
      break;
    case VOL_DOWN:
      if(volume > 15){
        send_mp3_command(CMD_VOLUME_DOWN,0);
        volume --;
      }
      break;
  }
}

void avoid_alarm(){
  empty_mp3_available();
  send_mp3_command(CMD_PLAYING_N,0);
  if(!mp3_serial.isListening()){
    mp3_serial.listen();
  }
  while(!mp3_serial.available()){}
  uint8_t buffer[10];
  get_mp3_out(buffer);
  if(buffer[6] == alarm_number){
    Serial.println("dodged alarm");
    send_mp3_command(CMD_NEXT_SONG, 0);
  }
}

void keep_playing(){
  if(is_playing){
    int x = analogRead(A0);
  //  Serial.println(x);
    if(x >= 615){
      Serial.println("next song");
      digitalWrite(6,HIGH);
      delay(2);
      execute_mp3_command(PLAY);
    }
  }
}

void update_reminder_data(byte buffer[],int buffer_length){
  reminder_data[REMINDER_STATE] = (int) buffer[REMINDER_STATE+1];
  reminder_data[REMINDER_ON_HOUR] = (int) buffer[REMINDER_ON_HOUR+1];
  reminder_data[REMINDER_ON_MIN] = (int) buffer[REMINDER_ON_MIN+1];
  reminder_data[REMINDER_OFF_HOUR] = (int) buffer[REMINDER_OFF_HOUR+1];
  reminder_data[REMINDER_OFF_MIN] = (int) buffer[REMINDER_OFF_MIN+1];
  reminder_data[REMINDER_BRIGHTNESS] = (int) buffer[REMINDER_BRIGHTNESS+1];
  reminder_message = "";
  for(int i = REMINDER_MESSAGE+1; i < buffer_length; i++){
    reminder_message = String(reminder_message + String((char) buffer[i]));
  }
  reminder_message = reminder_message.substring(1);

  analogWrite(BRIGHTNESS_PIN, reminder_data[REMINDER_BRIGHTNESS]);

  Serial.print("reminder state: ");Serial.println(reminder_data[REMINDER_STATE]);
  Serial.print("reminder on hour: ");Serial.println(reminder_data[REMINDER_ON_HOUR]);
  Serial.print("reminder on min: ");Serial.println(reminder_data[REMINDER_ON_MIN]);
  Serial.print("reminder off hour: ");Serial.println(reminder_data[REMINDER_OFF_HOUR]);
  Serial.print("reminder off min ");Serial.println(reminder_data[REMINDER_OFF_MIN]);
  Serial.print("reminder message: ");Serial.println(reminder_message);
  Serial.print("reminder brightness ");Serial.println(reminder_data[REMINDER_BRIGHTNESS]);
  Serial.println();
}

void check_mp3_automation(){
  if(mp3_data[MP3_AUTOMATION]){
    if(do_once && between(hour, min, mp3_data[MP3_ON_HOUR], mp3_data[MP3_ON_MIN], mp3_data[MP3_OFF_HOUR], mp3_data[MP3_OFF_MIN])){
      execute_mp3_command(WAKE_UP);
      do_once = false;
      mp3_between = true;
      execute_mp3_command(PLAY);
    }
    if(mp3_between){
      if(!between(hour, min, mp3_data[MP3_ON_HOUR], mp3_data[MP3_ON_MIN], mp3_data[MP3_OFF_HOUR], mp3_data[MP3_OFF_MIN])){
        execute_mp3_command(SLEEP);
        mp3_between = false;
      }
    }
  }
}

void transmit_audio_data(){

}
void setup() {
  Serial.begin(9600);
  esp_serial.begin(9600);
  mp3_serial.begin(9600);
  Wire.begin();
  pinMode(6,OUTPUT);
  pinMode(BRIGHTNESS_PIN, OUTPUT);
  analogWrite(BRIGHTNESS_PIN, reminder_data[REMINDER_BRIGHTNESS]);
  pinMode(A0,INPUT);
  pinMode(ALARM_OFF_BUTTON,INPUT_PULLUP);
  pinMode(BLUETOOTH_PIN,OUTPUT);
  digitalWrite(BLUETOOTH_PIN,LOW); // CHANGE TO LOW
  digitalWrite(6,LOW);
  mp3_awake = true;

  lcd.begin(20,4);
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Product of");
  lcd.setCursor(0,1);
  lcd.print("MASTER SHENWEI");
  lcd.setCursor(0,3);
  delay(500);
  lcd.print("Connecting");
  esp_serial.listen();
  wait_for_esp(0);
  if(!esp_serial.isListening()){
    esp_serial.listen();
  }
  get_esp_out();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("IP Adrs:");
  lcd.setCursor(0,1);
  lcd.print(ip_address);
  send_mp3_command(CMD_SEL_DEV, DEV_TF);
  send_mp3_command(CMD_SHUFFLE_PLAY,0);
  send_mp3_command(CMD_SET_VOLUME, 20);
  empty_mp3_available();

}

void loop() {
  if(!esp_serial.isListening()){
    esp_serial.listen();
  }
  get_esp_out();
  keep_playing();
  check_alarm();
  check_mp3_automation();
  if(update_lcd){
    update_lcd_date();
    update_lcd = false;
  }

}
