#include <ShiftySherwin.h>
#include <TinyWireS.h>

#define DIGIT0    0
#define DIGIT1    1
#define DIGIT2    2
#define DIGIT3    3

int delayTime = 0;
uint8_t number_display[4] = {1,2,0,0};
byte register_bits[10] = {0b00111111, 0b00000110, 0b01011011, 0b01001111,0b01100110,
                          0b01101101,0b01111101,0b00000111,0b011111111,0b01100111};
ShiftySherwin segments(2);
void setup() {
  segments.shiftSetup(1,3,4);
  TinyWireS.begin(8);
  TinyWireS.onReceive(receiveEvent);
  segments.clearRegisters();
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);
}

void loop() {

  TinyWireS_stop_check();
  segments.twoByteWrite(1,(((uint16_t) register_bits[number_display[DIGIT0]]) << 8) | ((uint16_t) 0b00000001));
  delay(delayTime);

  segments.twoByteWrite(1,(((uint16_t) register_bits[number_display[DIGIT1]]) << 8) | ((uint16_t) 0b00000010));
  delay(delayTime);

  segments.twoByteWrite(1,(((uint16_t) register_bits[number_display[DIGIT2]]) << 8) | ((uint16_t) 0b00000100));
  delay(delayTime);

  segments.twoByteWrite(1,(((uint16_t) register_bits[number_display[DIGIT3]]) << 8) | ((uint16_t) 0b00001000));
  delay(delayTime);
}

void receiveEvent(int how_many){
  while(TinyWireS.available() != 4){}
  for(int i = 0; i < 4; i++){
    number_display[i] = (int) TinyWireS.receive();
  }
}
