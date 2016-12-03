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
lcd_image_t map_image5 = { "snakes.lcd", 128, 130 };
lcd_image_t map_image6 = { "sal.lcd", 128, 130 };

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
        posX[cc] = (floor(i*((128/6)))+(floor(128/12)-(2-i)));
        posY[cc] = (floor(j*((107/5)))+(floor(107/10)-(3-j)));
        cc++;
      }
    }
    else{
      for (int i=0; i<6; i++){
        posX[cc] = (floor(i*((128/6)))+(floor(128/12)-(2-i)));
        posY[cc] = (floor(j*((107/5)))+(floor(107/10)-(3-j)));
        cc++;
      }
    }
  }
}

void updateMap(int a, int position, int oldposition){
  int diff=0,playerColor;
  if (a==1){
    diff=4;
    playerColor=(tft.Color565(0xff, 0x00, 0xff));
  }
  else if (a==2){
    diff=-4;
    playerColor=(tft.Color565(0x00, 0x00, 0x00));
  }
  Serial.println("Pos:");
  Serial.println(position);
  Serial.println(oldposition);
  Serial.print("First: ");
  Serial.println(posX[oldposition-1]+diff);
  Serial.print("Second: ");
  Serial.println(posX[position-1]+diff);
  if(board_no==0){
    int movePosX=posX[oldposition-1];
    // while(movePosX!=posX[position-1]){
    //   tft.fillRect(movePosX, posY[position-1], 5, 5, playerColor);
    //   lcd_image_draw(&map_image, &tft,
    //     movePosX-5, posY[oldposition-1], // upper-left corner of parrot picture
    //     movePosX-5, posY[oldposition-1], // upper-left corner of the screen
    //     5, 5); // draw all rows and columns of the parrot
    //   movePosX++;
    //   delay(30);
    // }
    lcd_image_draw(&map_image, &tft,
      posX[oldposition-1]+diff, posY[oldposition-1], // upper-left corner of parrot picture
      posX[oldposition-1]+diff, posY[oldposition-1], // upper-left corner of the screen
      5, 5); // draw all rows and columns of the parrot
    // tft.drawRect(posX[position-1]+=diff, posY[position-1], 5, 5, playerColor);
    tft.fillRect(posX[position-1]+diff, posY[position-1], 5, 5, playerColor);
  }
  else if(board_no==1){
    lcd_image_draw(&map_image5, &tft,
      posX[oldposition-1]+diff, posY[oldposition-1], // upper-left corner of parrot picture
      posX[oldposition-1]+diff, posY[oldposition-1], // upper-left corner of the screen
      5, 5); // draw all rows and columns of the parrot
    // tft.drawRect(posX[position-1]+=diff, posY[position-1], 5, 5, playerColor);
    tft.fillRect(posX[position-1]+diff, posY[position-1] , 5, 5, playerColor);
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
    tft.setCursor(40, 144);
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.print("Map #: ");
    tft.setCursor(80, 144);
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.print("1");
    tft.fillTriangle(28, 148, 32, 146, 32, 150, 0x77fd04);
    tft.fillTriangle(98, 148, 94, 146, 94, 150, 0x77fd04);
    int select=1,prev_select=1,refresh=0;
    while(true){
      // int v = analogRead(JOY_VERT_ANALOG);
      // // Serial.println("scan_joystick");
      // if(v > 550){
      //   // Serial.println("v1");
      //   // if(selected==prev_selected){
      //   selected=2;
      //   tft.setCursor(38, 114);
      //   tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      //   tft.print("Start Game");
      //
      //   tft.setCursor(32, 132);
      //   tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
      //   tft.print("Instructions");
      //
      //   tft.fillTriangle(26, 118, 24, 116, 24, 120, ST7735_BLACK);
      //   tft.fillTriangle(110, 118, 112, 116, 112, 120, ST7735_BLACK);
      //   tft.fillTriangle(26, 136, 24, 134, 24, 138, 0x77fd04);
      //   tft.fillTriangle(110, 136, 112, 134, 112, 138, 0x77fd04);
      //   // prev_selected=selected;
      //   // }
      // }
      // if(v < 490){
      //   // if(selected==prev_selected){
      //   tft.setCursor(32, 132);
      //   tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      //   tft.print("Instructions");
      //
      //   tft.setCursor(38, 114);
      //   tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
      //   tft.print("Start Game");
      //   selected=1;
      //   tft.fillTriangle(26, 118, 24, 116, 24, 120, 0x77fd04);
      //   tft.fillTriangle(110, 118, 112, 116, 112, 120, 0x77fd04);
      //   tft.fillTriangle(26, 136, 24, 134, 24, 138, ST7735_BLACK);
      //   tft.fillTriangle(110, 136, 112, 134, 112, 138, ST7735_BLACK);
      //   // prev_selected=selected;
      //   // }
      // }
      int h = analogRead(JOY_HORIZ_ANALOG);
      if(h > 550){

      tft.fillTriangle(98, 148, 94, 146, 94, 150, ST7735_GREEN);
      if(select!=3){
      tft.fillTriangle(121, 62, 121, 58, 125, 60, ST7735_GREEN);
    }
        if(select==prev_select && select<3){
          board_no+=1;
          select+=1;
          refresh=1;
          prev_select=select;
        }
      }
      else if(h < 450){

        tft.fillTriangle(28, 148, 32, 146, 32, 150, ST7735_GREEN);
        if(select!=3){
        tft.fillTriangle(0, 60, 4, 58, 4, 62, ST7735_GREEN);
      }
        if(select==prev_select && select>1){
          board_no-=1;
          select-=1;
          refresh=1;
          prev_select=select;
        }
      }
      else{

        tft.fillTriangle(28, 148, 32, 146, 32, 150, 0x77fd04);
        tft.fillTriangle(98, 148, 94, 146, 94, 150, 0x77fd04);
        if(select!=3){
         tft.fillTriangle(0, 60, 4, 58, 4, 62, 0x77fd04);
         tft.fillTriangle(121, 62, 121, 58, 125, 60, 0x77fd04);
        }
      }
      if(refresh == 1 && select==1){

        tft.setCursor(80, 144);
        tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
        tft.print("1");
        lcd_image_draw(&map_image, &tft,
          0, 0, // upper-left corner of picture
          10, 4, // upper-left corner of the screen
          107, 107); // draw all rows and columns of the pICTURE

          tft.drawRect(8,2,111,111,tft.Color565(0x00, 0x00, 0xff));
          tft.fillTriangle(0, 60, 4, 58, 4, 62, 0x77fd04);
          tft.fillTriangle(121, 62, 121, 58, 125, 60, 0x77fd04);
          refresh=0;
      }
      else if(refresh == 1 && select==2){

        tft.setCursor(80, 144);
        tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
        tft.print("2");
        lcd_image_draw(&map_image5, &tft,
          0, 23, // upper-left corner of parrot picture
          10, 4, // upper-left corner of the screen
          107, 107); // draw all rows and columns of the parrot

          tft.drawRect(8,2,111,111,tft.Color565(0x00, 0x00, 0xff));
          tft.fillTriangle(0, 60, 4, 58, 4, 62, 0x77fd04);
          tft.fillTriangle(121, 62, 121, 58, 125, 60, 0x77fd04);
          refresh=0;
      }

      else if(refresh == 1 && select==3){
        tft.setCursor(80, 144);
        tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
        tft.print("3");
        tft.fillRect(0,0,128,130,ST7735_BLACK);
        tft.setCursor(10, 60);
        tft.setTextSize(2);
        tft.setTextColor(ST7735_WHITE);
        tft.print("Return to");
        tft.setCursor(10, 76);
        tft.print("Main Menu");
        tft.setTextSize(1);
        refresh=0;
      }
      bool invSelect = digitalRead(JOY_SEL);
      if(invSelect==0 && select!=3){
        selected=3;
        return 0;
      }
      if(invSelect==0 && select==3){
        selected=0;
        return 0;
      }
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
    10, 4, // upper-left corner of the screen
    107, 107); // draw all rows and columns of the pICTURE
    tft.drawRect(8,2,111,111,tft.Color565(0x00, 0x00, 0xff));
    tft.fillTriangle(0, 60, 4, 58, 4, 62, 0x77fd04);
    tft.fillTriangle(121, 62, 121, 58, 125, 60, 0x77fd04);
  choose_image();
  return 0;
}

void initializeScreen(){
  lcd_image_draw(&map_image, &tft,
    0, 0, // upper-left corner of parrot picture
    0, 0, // upper-left corner of the screen
    128, 130); // draw all rows and columns of the parrot
  updateMap(1,1,1);
  updateMap(2,1,1);
}
int gameMain(){
  dumpBuffer();
  tft.fillScreen(0);
  tft.setCursor(15, 60);
  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.print("Connecting Snakes");
  tft.setCursor(20, 76);
  tft.print("Please Wait...");
  int dice;
  int myPosition=1;
  int oppPosition=1;
  int myOldPosition=1;
  int oppOldPosition=1;
  // establishCommunication();
  calculateMapCoordinate();
  int turn=1;
  // decideOrder();
  tft.fillScreen(0);
  tft.setCursor(25, 50);
  tft.setTextSize(2);
  tft.print("You Go: ");
  tft.setCursor(60, 75);
  tft.setTextSize(3);
  tft.print(turn);
  tft.setCursor(5, 120);
  tft.setTextSize(1);
  tft.print("Your game will begin Shortly");
  // delay(3000);
  initializeScreen();
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
          oppOldPosition = oppPosition;
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
      myOldPosition = myPosition;
      myPosition+=dice;
        Serial.print("Current Pos: ");
        Serial.println(myPosition);
        Serial.print("Prev Pos: ");
        Serial.println(myOldPosition);
      updateMap(1,myPosition,myOldPosition);
      delay(2000);
      myOldPosition = myPosition;
      myPosition = checkSnakeLadder(myPosition);
        Serial.print("Current Pos: ");
        Serial.println(myPosition);
        Serial.print("Prev Pos: ");
        Serial.println(myOldPosition);
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

  tft.fillTriangle(26, 118, 24, 116, 24, 120, 0x77fd04);
  tft.fillTriangle(110, 118, 112, 116, 112, 120, 0x77fd04);

  while(true){
    int v = analogRead(JOY_VERT_ANALOG);
    // Serial.println("scan_joystick");
    if(v > 550){
      // Serial.println("v1");
      // if(selected==prev_selected){
      selected=2;
      tft.setCursor(38, 114);
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      tft.print("Start Game");

      tft.setCursor(32, 132);
      tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
      tft.print("Instructions");

      tft.fillTriangle(26, 118, 24, 116, 24, 120, ST7735_BLACK);
      tft.fillTriangle(110, 118, 112, 116, 112, 120, ST7735_BLACK);
      tft.fillTriangle(26, 136, 24, 134, 24, 138, 0x77fd04);
      tft.fillTriangle(110, 136, 112, 134, 112, 138, 0x77fd04);
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
      tft.fillTriangle(26, 118, 24, 116, 24, 120, 0x77fd04);
      tft.fillTriangle(110, 118, 112, 116, 112, 120, 0x77fd04);
      tft.fillTriangle(26, 136, 24, 134, 24, 138, ST7735_BLACK);
      tft.fillTriangle(110, 136, 112, 134, 112, 138, ST7735_BLACK);
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
  lcd_image_draw(&map_image6, &tft,
    0, 0, // upper-left corner of parrot picture
    0, 0, // upper-left corner of the screen
    128, 160); // draw all rows and columns of the parrot

  tft.setTextColor(tft.Color565(0xff, 0x00, 0x00));

  tft.setTextSize(2);

  tft.setTextWrap(false);
  
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
tft.fillScreen(0);
  while(true){
    // choose();
    gameMain();
    // lcd_image_draw(&map_image, &tft,
    //   20, 20, // upper-left corner of parrot picture
    //   20, 20, // upper-left corner of the screen
    //   5, 5); // draw all rows and columns of the parrot
    // // tft.drawRect(posX[position-1]+=diff, posY[position-1], 5, 5, playerColor);
    //   delay(500);
    // tft.fillRect(20, 20, 5, 5, ST7735_GREEN);
    // delay(500);

  }
  Serial.print("returned in main function");
  Serial3.end();
  Serial.end();
  return 0;
}
