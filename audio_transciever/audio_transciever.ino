#define LOG_OUT 1 // use the log output function
#define FHT_N 128 // set to 256 point fht

#include <FHT.h>
#include <SPI.h>
#include "RF24.h"


#define CS_PIN      2
#define CLCK_PIN    3
#define DIN         4
#define CE_PIN      7
#define CSN_PIN     8
#define SUBTR_CONST 650
#define LIGHT_COUNT  60
#define PACKET_SIZE  11

int max_sum = 2000;

int over_counter = 0;
int under_counter = 0;
int zero_counter = 0;
int skips = 0;
uint8_t send_buf[PACKET_SIZE];
int32_t sum = 0;
unsigned long then;
unsigned long now;
bool skip_fht = false;
uint8_t light_height;
RF24 radio (8,7); // CE, CS
byte txaddr[][6] = {"1Node"};

void set_up_adc(){
  pinMode(CS_PIN,OUTPUT);
  pinMode(CLCK_PIN,OUTPUT);
  pinMode(DIN,INPUT);
  PORTD |= 0b00000100;
}

uint16_t fast_analog_read(){
  uint16_t adc_read = 0;
  PORTD ^= 0b00000100;

  for(int i = 0; i < 6;i++){
    PORTD ^= 0b00001000;
  }

  for(int i = 11; i >= 0; i--){
    PORTD ^= 0b00001000;
    PORTD ^= 0b00001000;
    adc_read |= ((0X01 & (PIND >> 4)) << i);
  }
  PORTD ^= 0b00000100;

  return adc_read;
}


void get_fht(){
  uint16_t maximum = 0;
  uint16_t minimum = 10000;
  uint16_t adc_read;
  for(int i = 0; i < FHT_N; i++){
     adc_read = fast_analog_read();
     fht_input[i] = adc_read;
     minimum = min(adc_read, minimum);
     maximum = max(adc_read, maximum);
  }
  uint16_t difference = maximum - minimum;
  //Serial.println(difference);
  if(difference <= 3){
    skip_fht = true;
  }
  else{
    fht_window(); // window the data for better frequency response
    fht_reorder(); // reorder the data before doing the fht
    fht_run(); // process the data in the fht
    fht_mag_log(); // take the output of the fht
  }
}
void setup() {
  SPI.begin();
  Serial.begin(115200);
  radio.begin();
  radio.openWritingPipe(txaddr[0]);
  set_up_adc();
  then = micros();
}



void loop() {

  for(int j = 0; j < (PACKET_SIZE - 1); j++){
    get_fht();
    sum = 0;
    for(int i = 2; i < 64; i ++){
      sum += fht_log_out[i];
    }
    sum -= SUBTR_CONST;
    if(skip_fht){
      skip_fht = false;
      sum = 0;
    }
    //Serial.print("sum: ");Serial.println(sum);
      if(sum > 0){
        if(skips > 0){
          light_height = 0;
          skips --;
        }
        else{
          float ratio = (float)sum / (float)max_sum;
          //Serial.print("ratio: ");Serial.println(ratio);
          light_height = (uint8_t) ((float)LIGHT_COUNT * ratio);
          if(light_height > LIGHT_COUNT){
            light_height = LIGHT_COUNT;
          }
          if(light_height > 53){
            over_counter ++;
          }
          else{
            over_counter = 0;
          }

          if(light_height < 15){
            under_counter ++;
          }
          else{
            under_counter = 0;
          }
        }
    }
    else{
      light_height = 0;
      zero_counter ++;
      if(zero_counter >= 14){
        skips = 5;
      }
    }

    send_buf[j] = light_height;

  }
  now = micros();
  send_buf[PACKET_SIZE - 1] = (now - then) / (PACKET_SIZE - 1);

  /*for(int i = 0 ;i<4;i++){
    for(int j = 0; j < send_buf[i];j++){
      Serial.print("*");
    }
    Serial.println();
  }*/

  if(over_counter > 10){
    max_sum += 200;
  }
  if(under_counter > 20){
    max_sum -= 100;
  }
  radio.write(&send_buf, sizeof(send_buf));

  then = micros();

}
