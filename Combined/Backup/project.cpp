#include<Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SD.h>

#include "lcd_image.h"
// Define the pins
#define SD_CS   5
#define TFT_CS  6
#define TFT_DC  7
#define TFT_RST 8
#define JOY_SEL 9
#define JOY_VERT_ANALOG 0
#define JOY_HORIZ_ANALOG 1
// Initialize the screen
int selected=0;
int board_no=0;
int curr_joy_select =1;
int posX[30],posY[30];
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
lcd_image_t map_image = { "snake.lcd", 128, 107 };
lcd_image_t map_image1 = { "snake1.lcd", 80, 59 };
lcd_image_t map_image2 = { "snake2.lcd", 128, 81 };
lcd_image_t map_image3 = { "dice.lcd", 40, 39 };
lcd_image_t map_image4 = { "ladder.lcd", 30, 60 };
lcd_image_t map_image5 = { "snakes.lcd", 128, 130 };

int current_screen = 0;

int snakeposition[10]={27, 21, 17, 19, 0, 0, 0, 0, 0, 0};
int snakemoveto[10]={1, 9, 4, 7, 0, 0, 0, 0, 0, 0};
int ladderposition[10]={11, 5, 20, 3, 0, 0, 0, 0, 0, 0};
int laddermoveto[10]={26, 8, 29, 22, 0, 0, 0, 0, 0, 0};
//==========================================================================
//==========================================================================
//==========================================================================
//==========================================================================
//==========================================================================
//==========================================================================
bool wait_on_serial3( uint8_t nbytes, long timeout ) {
  unsigned long deadline = millis() + timeout;//wraparound not a problem
  while (Serial3.available()<nbytes && (timeout<0 || millis()<deadline))
  {
    delay(1); // be nice, no busy loop
  }
  return Serial3.available()>=nbytes;
}
/** Writes an uint32_t to Serial3, starting from the least-significant
* and finishing with the most significant byte.
*/
void uint32_to_serial3(uint32_t num) {
  Serial3.write((char) (num >> 0));
  Serial3.write((char) (num >> 8));
  Serial3.write((char) (num >> 16));
  Serial3.write((char) (num >> 24));
}
/** Reads an uint32_t from Serial3, starting from the least-significant
* and finishing with the most significant byte.
*/
uint32_t uint32_from_serial3() {
  uint32_t num = 0;
  num = num | ((uint32_t) Serial3.read()) << 0;
  num = num | ((uint32_t) Serial3.read()) << 8;
  num = num | ((uint32_t) Serial3.read()) << 16;
  num = num | ((uint32_t) Serial3.read()) << 24;
  return num;
}
/*============================================================
Client State Machine Function
============================================================*/
uint32_t client(uint32_t ckey){
  enum Client { START=1, WaitingForAck, DataExchange  };
  Client curr_state = START;

  uint32_t skey;
  char ACK = 'A';
  while (true){
    if (curr_state == START){
      Serial.println("Start");
      Serial3.write('C');
      uint32_to_serial3(ckey);
      curr_state = WaitingForAck;
    }
    else if ((curr_state == WaitingForAck) && (wait_on_serial3(1,1000) != 0) && (Serial3.read() == ACK)){
      Serial.println("WFA");
      if(wait_on_serial3(4,1000) != 0){
        skey = uint32_from_serial3();
        Serial3.write(ACK);
        curr_state = DataExchange;
      }
      else{ curr_state = START; }
    }
    else if( curr_state == DataExchange ){
      return skey;
    }
    else{ curr_state = START;
    }
  }
}
/*============================================================
Server State Machine Function
============================================================*/
uint32_t server(uint32_t skey){
  // int startTime=millis();
  enum Server { LISTEN=1, WFK, WFA, WFK2, WFA2, DataExchange };
  Server curr_state = LISTEN;

  char CR ='C';
  char ACK = 'A';
  uint32_t ckey;
  while (true){
    if(curr_state == LISTEN){
      // Serial.println(currTime-startTime);
      // Serial.println("Listening...");
      if((wait_on_serial3(1,-1) !=0) && Serial3.read() == CR){
        curr_state = WFK;
      }
    }
    else if(curr_state == WFK){
      // Serial.println("Waiting For Key...");
      if(wait_on_serial3(1,1000) != 0){
        ckey=uint32_from_serial3();
        Serial3.write(ACK);
        uint32_to_serial3(skey);
        curr_state = WFA;
      }
      else{
        curr_state = LISTEN;
      }
    }
    else if (curr_state == WFA && wait_on_serial3(1,1000)){
      // Serial.println("Waiting For Ack...");
      while((Serial3.available()>0) && curr_state == WFA){
        char aa = Serial3.read();
        if(aa == ACK){
          curr_state = DataExchange;
        }
        else if(aa == CR){
          curr_state = WFK;
        }
      }
    }
    else if(curr_state == WFK2 ){
      // Serial.println("Waiting For Key...");
      if(wait_on_serial3(4,1000) != 0){
        ckey=uint32_from_serial3();
        uint32_to_serial3(skey);
        curr_state = WFA2;
      }
      else{
        curr_state = LISTEN;
      }
    }
    else if (curr_state == WFA2 && wait_on_serial3(1,1000)){
      // Serial.println("Waiting For Ack...");
      while((Serial3.available()>0) && curr_state == WFA){
        char aa = Serial3.read();
        if(aa == ACK){
          curr_state = DataExchange;
        }
        else if(aa == CR){
          curr_state = WFK;
        }
      }
    }
    else if(curr_state == DataExchange){
      Serial.println("DataExchange!");
      return ckey;
    }
    else{
      curr_state = LISTEN;
    }
  }
}

/* ===============================================================
Function that Generates a Random Key
Variable Description:
analog_pin: declares the port that will be used for analog_pin
counter: will be used shift the number left from 0 to 31
=============================================================== */
uint32_t random_key(){
  int analog_pin=1;
  uint32_t counter = 31;
  uint32_t private_key=0;

  int val;
  for(int i=0; i<32; ++i){
    val = analogRead(analog_pin);
    if(val & 1){
      private_key = private_key + (1ul << counter );
    }
    counter--;
    delay(50);
  }
  return private_key;
}

/* ===============================================================
Function that calculates the modular where we are removing the
limitation where we can calculate 32bit * 32bit
"a * b % m where correctly with a and m requiring as many as 31 bits
=============================================================== */
uint32_t mul_mod( uint32_t a, uint32_t b, uint32_t m){
  uint32_t result=0;
  uint32_t p = b % m;
  for(int i=0; i<32; i++){
    if(a & (1ul<<i)){
      result= (result + p) % m;
    }
    p= (p * 2) % m;
  }
  return result;
}

/* ===============================================================
Function that calculates the modular

a: base, nonnegative integer
b: exponent, nonnegative integer, a=b=0 not allowed
m: positive integer, m<2^16
=============================================================== */
uint32_t fast_pow_mod( uint32_t a, uint32_t b, uint32_t m){
  uint32_t result = 1 % m;
  uint32_t p = a % m;
  for (int i=0; i<32; i++){
    if ((b & (1ul<<i)) != 0){
      // result = (result*p) % m;
      result = mul_mod(result, p, m);
    }
    // p = (p*p) % m;
    p = mul_mod( p, p, m);
  }
  return result;
}


/** Implements the Park-Miller algorithm with 32 bit integer arithmetic
* @return ((current_key * 48271)) mod (2^31 - 1);
* This is linear congruential generator, based on the multiplicative
* group of integers modulo m = 2^31 - 1.
* The generator has a long period and it is relatively efficient.
* Most importantly, the generator's modulus is not a power of two
* (as is for the built-in rng),
* hence the keys mod 2^{s} cannot be obtained
* by using a key with s bits.
* Based on:
* http://www.firstpr.com.au/dsp/rand31/rand31-park-miller-carta.cc.txt
*/
uint32_t next_key(uint32_t current_key) {
  const uint32_t modulus = 0x7FFFFFFF; // 2^31-1
  const uint32_t consta = 48271;  // we use that consta<2^16
  uint32_t lo = consta*(current_key & 0xFFFF);
  uint32_t hi = consta*(current_key >> 16);
  lo += (hi & 0x7FFF)<<16;
  lo += hi>>15;
  if (lo > modulus) lo -= modulus;
  return lo;
}
/* ===============================================================
Main Function
=============================================================== */
int establishCommunication() {
  Serial.println("Beginning Game...");
  Serial.println("Establishing Secure Communication...");
  uint32_t prime = 2147483647;
  uint32_t generator = 16807;
  uint32_t private_key = random_key();
  // Serial.println("");
  Serial.println("Generating Random Key for Key Exchange... ");
  // Serial.println(private_key);
  // Serial.println("");

  //Generates your Public Key
  uint32_t public_key = fast_pow_mod(generator, private_key, prime);
  // Serial.print("Public Key: ");
  // Serial.println(public_key);

  // Get partner's Public Key
  // Serial.println("Can you please enter the key recieved from your partner? ");
  uint32_t partners_public_key;
  int ledpin = 13;
  pinMode(ledpin, OUTPUT);
  if (digitalRead(ledpin) == LOW){
    Serial.println("Starting Secure Communication as Client...");
    partners_public_key = client(public_key);
  }
  else{
    Serial.println("Starting Secure Communication as Server...");
    partners_public_key = server(public_key);
  }
  // Get the shared Secret Key
  uint32_t shared_secret = fast_pow_mod( partners_public_key, private_key, prime );
  // Serial.print("Shared: ");
  // Serial.println(shared_secret);

  /* There are sometimes some buffers that mess up the secret key calculation
   so that we need to dumb all the buffers
   We wait until all the Serial3 dumbs all its buffers*/
  Serial.println("Dumping Buffers...");
  while(Serial3.available()>0){
    Serial.print(Serial3.read());
    delay(1);
  }
  Serial.println("Buffer Dumped!");
  Serial.println("Communication Established! Enjoy Your Game!");
  return 0;
}
//==========================================================================
//==========================================================================
//==========================================================================
//==========================================================================
//==========================================================================
//==========================================================================
int rollDice(int max){
  int analog_pin=7;
  int counter = 2;
  int val, diceValue;

  for(int i=0; i<20; ++i){
    val = analogRead(analog_pin);
    if(val & 1){
      diceValue = diceValue + (1ul << counter );
    }
    counter--;
    delay(50);
  }
  diceValue = ((diceValue+(rand()%100))%max)+1;
  return diceValue;
}

int decideOrder(){
  int playorder;

  Serial.println("Randomly deciding who will go first..");
  delay(1000);
  int ledpin = 13;
  pinMode(ledpin, OUTPUT);
  if (digitalRead(ledpin) == LOW){
    Serial.println("Client!");
    playorder=rollDice(2);
    if(playorder==1){
      Serial3.write(2);
    }
    else if(playorder == 2){
      Serial3.write(1);
    }
  }
  else{
    while(true){
      if(Serial3.available()>0){
        playorder=Serial3.read();
        break;
      }
    }
  }
  return playorder;
}

void dumpBuffer(){
  while(Serial3.available()>0){
    Serial.print(Serial3.read());
    delay(1);
  }
}

int checkSnakeLadder(int currentPosition){
  for (int i=0; i<10; i++){
    if (currentPosition == snakeposition[i]){
      Serial.print("Snaked! Move from: ");
      Serial.print(currentPosition);
      Serial.print(" to ");
      Serial.println(snakeposition[i]);
      currentPosition=snakemoveto[i];
    }
  }
  for (int i=0; i<10; i++){
    if (currentPosition == ladderposition[i]){
      Serial.print("Laddered! Move from: ");
      Serial.print(currentPosition);
      Serial.print(" to ");
      Serial.println(laddermoveto[i]);
      currentPosition=laddermoveto[i];
    }
  }
  return currentPosition;
}

int menu();

void calculateMapCoordinate(){
  int cc=0;
  for(int j=4; j>=0; j--){
    if(j%2==1){
      for (int i=5; i>=0; i--){
        posX[cc] = (floor(i*((128/6)))+(floor(128/12)-(6-i)));
        posY[cc] = (floor(j*((107/5)))+(floor(107/10)-(5-j)));
        cc++;
      }
    }
    else{
      for (int i=0; i<6; i++){
        posX[cc] = (floor(i*((128/6)))+(floor(128/12)-(6-i)));
        posY[cc] = (floor(j*((107/5)))+(floor(107/10)-(5-j)));
        cc++;
      }
    }
  }
}

void updateMap(int a, int position, int oldposition){
  int diff,playerColor;
  if (a==1){
    diff=3;
    playerColor=(tft.Color565(0xff, 0x00, 0xff));
  }
  else if (a==2){
    diff=-3;
    playerColor=(tft.Color565(0x00, 0x00, 0x00));
  }
  if(board_no==0){
    lcd_image_draw(&map_image, &tft,
      posX[oldposition-1]+=diff, posY[oldposition-1], // upper-left corner of parrot picture
      posX[oldposition-1]+=diff, posY[oldposition-1], // upper-left corner of the screen
      5, 5); // draw all rows and columns of the parrot
    tft.drawRect(posX[position-1]+=diff, posY[position-1], 5,5, playerColor);
    tft.fillRect(posX[position-1]+=diff, posY[position-1] ,5, 5, playerColor);
  }
  else if(board_no==1){
    lcd_image_draw(&map_image5, &tft,
      posX[oldposition-1]+=diff, posY[oldposition-1], // upper-left corner of parrot picture
      posX[oldposition-1]+=diff, posY[oldposition-1], // upper-left corner of the screen
      5, 5); // draw all rows and columns of the parrot
    tft.drawRect(posX[position-1]+=diff, posY[position-1], 5, 5, ST7735_WHITE);
    tft.fillRect(posX[position-1]+=diff, posY[position-1] , 5, 5, ST7735_WHITE);
  }
}

int Instructions(){
  tft.fillScreen(0);
  tft.setCursor(1, 1);
  tft.setTextColor(ST7735_WHITE);
  tft.print("Rules of the Game");
  tft.setCursor(1, 130);
  tft.setTextColor(ST7735_GREEN);
  tft.print("********************");
  tft.setCursor(1, 140);
  tft.print("Press joystick to go \n back to menu");
  while(true){
    bool invSelect = digitalRead(JOY_SEL);
    if(invSelect==0){
      selected=0;
      return 0;
    }
  }
}

int choose_image(){
  while(true){
    tft.setCursor(40, 142);
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.print("Map #: ");
    tft.setCursor(80, 142);
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.print("1");
    int h = analogRead(JOY_HORIZ_ANALOG);
    int select=1,prev_select=1;
    if(h > 500){
      // Serial.println("v1");
      if(select!=prev_select){
        board_no=1;
        tft.setCursor(80, 142);
        tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
        tft.print("2");
        lcd_image_draw(&map_image5, &tft,
          0, 0, // upper-left corner of parrot picture
          0, 0, // upper-left corner of the screen
          128, 130); // draw all rows and columns of the parrot
        prev_select=select;
        }
    }
    if(h < 450){
      tft.setCursor(80, 142);
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      tft.print("1");
      if(select!=prev_select){
        lcd_image_draw(&map_image, &tft,
          0, 0, // upper-left corner of parrot picture
          0, 0, // upper-left corner of the screen
          128, 107); // draw all rows and columns of the parrot
          board_no=0;
          // int w=128;
          // int h=23;
          // tft.drawRect(0, 107, w, h, ST7735_BLACK);
          // tft.fillRect(0, 107, w, h, ST7735_BLACK);
          // tft.drawRect(100, 100, 10,10, ST7735_WHITE);
          // tft.fillRect(100, 100 , 10, 10, ST7735_WHITE);
          // tft.drawRect(100, 100, 10,10, ST7735_WHITE);
          // tft.fillRect(100, 100 , 10, 10, ST7735_WHITE);
        }
      prev_select=select;
    }
    bool invSelect = digitalRead(JOY_SEL);
    if(invSelect==0){
      selected=3;
      return 0;
    }
  }
}
int chooseMap(){
  tft.fillScreen(0);
  tft.setCursor(30, 134);
  tft.setTextColor(ST7735_WHITE);
  tft.print("Choose a Map!");

  lcd_image_draw(&map_image, &tft,
    0, 0, // upper-left corner of picture
    0, 0, // upper-left corner of the screen
    128, 107); // draw all rows and columns of the pICTURE
  choose_image();
  return 0;
}


int gameMain(){
  dumpBuffer();
  int dice;
  int myPosition=1;
  int oppPosition=1;
  int myOldPosition=1;
  int oppOldPosition=1;
  establishCommunication();
  calculateMapCoordinate();
  int turn=decideOrder();
  Serial.print("My Order is: ");
  Serial.println(turn);
  dumpBuffer();

  //Main Game Loop
  while (true){
    dumpBuffer();
    if(turn == 2){
      Serial.print("In Turn: ");
      Serial.println(turn);
      // TODO: wait
      Serial.println("Waiting for opponent to roll...");
      dumpBuffer();
      while(true){
        if(Serial3.available()>0){
          dice=Serial3.read();
          Serial3.write(dice);
          Serial.print("Opponent Rolled: ");
          Serial.println(dice);
          oppPosition+=dice;
          updateMap(2,oppPosition,oppOldPosition);
          delay(2000);
          oppOldPosition = oppPosition;
          oppPosition = checkSnakeLadder(oppPosition);
          updateMap(2,oppPosition,oppOldPosition);
          Serial.print("Opponent Position: ");
          Serial.println(oppPosition);
          turn=1;
          Serial.println("Dumping Buffers...");
          oppOldPosition=oppPosition;
          break;
        }
      }
      dumpBuffer();
    }
    else if(turn == 1){
      Serial.print("In Turn: ");
      Serial.println(turn);
      delay(2000);
      Serial.println("Roll the Dice by Clicking!");
      bool joy_select = digitalRead(JOY_SEL);
      while (joy_select==1){
        joy_select = digitalRead(JOY_SEL);
        if (joy_select == 0){
          Serial.println("CLICKED!");
          dice = rollDice(6);
          break;
        }
      }
      Serial3.write(dice);
      Serial.print("I Rolled: ");
      Serial.println(dice);
      myPosition+=dice;
      updateMap(1,myPosition,myOldPosition);
      delay(2000);
      myOldPosition = myPosition;
      myPosition = checkSnakeLadder(myPosition);
      updateMap(1,myPosition,myOldPosition);
      Serial.print("My Position: ");
      Serial.println(myPosition);
      myOldPosition=myPosition;
      turn=2;
      dumpBuffer();
    }
  }
}
void choose(){
  while(true){
    if(selected==0){
      menu();
    }
    else if(selected==1){
      chooseMap();
    }
    else if(selected==2){
      Instructions();
    }
    else if(selected==3){
      gameMain();
    }
  }
}

int checkCursor(){

  while(true){
    int v = analogRead(JOY_VERT_ANALOG);
    // Serial.println("scan_joystick");
    if(v > 510){
      // Serial.println("v1");
      // if(selected==prev_selected){
      selected=2;
      tft.setCursor(38, 114);
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      tft.print("Start Game");

      tft.setCursor(32, 132);
      tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
      tft.print("Instructions");
      // prev_selected=selected;
      // }
    }
    if(v < 490){
      // if(selected==prev_selected){
      tft.setCursor(32, 132);
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      tft.print("Instructions");

      tft.setCursor(38, 114);
      tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
      tft.print("Start Game");
      selected=1;
      // prev_selected=selected;
      // }
    }
    bool invSelect = digitalRead(JOY_SEL);
    if(invSelect==0){
      return 0;
    }
  }
  // checkJoySelect();
  // Serial.print(invSelect);
}
int menu(){
  selected=1;
  // prev_selected=1;
  tft.fillScreen(0);
  int t_col=1;
  int t_row=1;
  lcd_image_draw(&map_image1, &tft,
    0, 0, // upper-left corner of parrot picture
    48, 1, // upper-left corner of the screen
    80, 50); // draw all rows and columns of the parrot

  tft.setTextColor(tft.Color565(0xff, 0x00, 0x00));
  lcd_image_draw(&map_image2, &tft,
    0, 0, // upper-left corner of parrot picture
    0, 79, // upper-left corner of the screen
    128, 81); // draw all rows and columns of the parrot
  tft.setTextSize(2);
  lcd_image_draw(&map_image3, &tft,
    0, 0, // upper-left corner of parrot picture
    2, 70, // upper-left corner of the screen
    40, 39); // draw all rows and columns of the parrot
  tft.setTextWrap(false);
  lcd_image_draw(&map_image4, &tft,
    0, 0, // upper-left corner of parrot picture
    0, 0, // upper-left corner of the screen
    30, 60); // draw all rows and columns of the parrot
  tft.setCursor(t_col*16, t_row*32);
  tft.print("SNAKE");
  tft.setCursor(t_col*56, t_row*48);
  tft.print("&&");
  tft.setCursor(t_col*42, t_row*64);
  tft.setTextWrap(false);

  tft.print("LADDERS");
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(38, t_row*114);
  tft.print("Start Game");
  tft.setCursor(32, t_row*132);
  tft.print("Instructions");
  tft.setCursor(38, 114);
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
  tft.print("Start Game");
  checkCursor();
  return 0;
}

int main(){
  init();
  Serial.begin(9600);
  Serial3.begin(9600);
  tft.initR(INITR_BLACKTAB);
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed!");
    return 0;
  }
  Serial.println("OK!");
  pinMode(JOY_SEL, INPUT);
  digitalWrite(JOY_SEL, HIGH); // enables pull-up resistor - required!
  while(true){
    choose();
  }
  Serial.print("returned in main function");
  Serial3.end();
  Serial.end();
  return 0;
}
