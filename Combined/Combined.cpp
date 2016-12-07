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
int posX[36]={0},posY[36]={0};

int dice=0;
int myPosition=1;
int oppPosition=1;
int myOldPosition=1;
int oppOldPosition=1;
int turn=0;
int nextRoll=6;
int winner=0;
int pickDice=1;
int maxValue=0;
bool doubleDice=false;
bool quit=false;
bool useSpecial=false;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
lcd_image_t map_image = { "snake.lcd", 128, 107 };
lcd_image_t map_image5 = { "snakes.lcd", 128, 130 };
<<<<<<< HEAD
lcd_image_t main_menu_image = { "sal.lcd", 128, 130 };
=======
lcd_image_t map_image6 = { "sal.lcd", 128, 130 };
>>>>>>> 1f62df2b9dd1353ef8f05c4a79587ab848b565f1

int current_screen = 0;

int snakeposition[5]={27, 21, 17, 19, 0};
int snakemoveto[5]={1, 9, 4, 7, 0};
int ladderposition[5]={11, 5, 20, 3, 0};
int laddermoveto[5]={26, 8, 29, 22, 0};
int items[4]={1,1,1,1};
//==========================================================================

bool wait_on_serial3( uint8_t nbytes, long timeout ) {
  unsigned long deadline = millis() + timeout;//wraparound not a problem
  while (Serial3.available()<nbytes && (timeout<0 || millis()<deadline))
  {
    delay(1); // be nice, no busy loop
  }
  return Serial3.available()>=nbytes;
}
void uint32_to_serial3(uint32_t num) {
  Serial3.write((char) (num >> 0));
  Serial3.write((char) (num >> 8));
  Serial3.write((char) (num >> 16));
  Serial3.write((char) (num >> 24));
}
uint32_t uint32_from_serial3() {
  uint32_t num = 0;
  num = num | ((uint32_t) Serial3.read()) << 0;
  num = num | ((uint32_t) Serial3.read()) << 8;
  num = num | ((uint32_t) Serial3.read()) << 16;
  num = num | ((uint32_t) Serial3.read()) << 24;
  return num;
}
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
uint32_t server(uint32_t skey){
  enum Server { LISTEN=1, WFK, WFA, WFK2, WFA2, DataExchange };
  Server curr_state = LISTEN;

  char CR ='C';
  char ACK = 'A';
  uint32_t ckey;
  while (true){
    if(curr_state == LISTEN){
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
  // tft.fillRect(0,132,128,140,ST7735_BLACK);
  for (int i=0; i<5; i++){
    if (currentPosition == snakeposition[i]){
      tft.setCursor(40, 146);
      tft.print("SNAKED!");
      Serial.print("Snaked! Move from: ");
      Serial.print(currentPosition);
      Serial.print(" to ");
      Serial.println(snakeposition[i]);
      currentPosition=snakemoveto[i];
    }
  }
  for (int i=0; i<5; i++){
    if (currentPosition == ladderposition[i]){
      tft.setCursor(40, 146);
      tft.print("LADDERED!");
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
  int a=0,b=0,c=0,d=0;
  if(board_no == 0){
    a=4;
    b=107;
    c=5;
    d=0;
  }
  else if(board_no == 1){
    a=5;
    b=130;
    c=6;
    d=1;
  }
  Serial.println("a");
  int cc=0;
  for(int j=a; j>=0; j--){
    if(j%2+d==1){
      for (int i=5; i>=0; i--){
        posX[cc] = (floor(i*((128/6)))+(floor(128/12)-(2-i)));
        posY[cc] = (floor(j*((b/c)))+(floor(b/(c*2))-(3-j)));
        cc++;
      }
    }
    else{
      for (int i=0; i<6; i++){
        posX[cc] = (floor(i*((128/6)))+(floor(128/12)-(2-i)));
        posY[cc] = (floor(j*((b/c)))+(floor(b/(c*2))-(3-j)));
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
  if(board_no==0){
    int movePosX=posX[oldposition-1];
    lcd_image_draw(&map_image, &tft,
      posX[oldposition-1]+diff, posY[oldposition-1], // upper-left corner of parrot picture
      posX[oldposition-1]+diff, posY[oldposition-1], // upper-left corner of the screen
      5, 5); // draw all rows and columns of the parrot
    tft.fillRect(posX[position-1]+diff, posY[position-1], 5, 5, playerColor);
  }
  else if(board_no==1){
    lcd_image_draw(&map_image5, &tft,
      posX[oldposition-1]+diff, posY[oldposition-1], // upper-left corner of parrot picture
      posX[oldposition-1]+diff, posY[oldposition-1], // upper-left corner of the screen
      5, 5); // draw all rows and columns of the parrot
    tft.fillRect(posX[position-1]+diff, posY[position-1] , 5, 5, playerColor);
  }
}
void Instructions(){
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
      return;
    }
  }
}

void choose_image(){
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
<<<<<<< HEAD
        tft.fillTriangle(98, 148, 94, 146, 94, 150, ST7735_GREEN);
        if(select!=3){
          tft.fillTriangle(121, 62, 121, 58, 125, 60, ST7735_GREEN);
        }
=======

      tft.fillTriangle(98, 148, 94, 146, 94, 150, ST7735_GREEN);
      if(select!=3){
      tft.fillTriangle(121, 62, 121, 58, 125, 60, ST7735_GREEN);
    }
>>>>>>> 1f62df2b9dd1353ef8f05c4a79587ab848b565f1
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
<<<<<<< HEAD
          tft.fillTriangle(0, 60, 4, 58, 4, 62, ST7735_GREEN);
        }
=======
        tft.fillTriangle(0, 60, 4, 58, 4, 62, ST7735_GREEN);
      }
>>>>>>> 1f62df2b9dd1353ef8f05c4a79587ab848b565f1
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
<<<<<<< HEAD
          tft.fillTriangle(0, 60, 4, 58, 4, 62, 0x77fd04);
          tft.fillTriangle(121, 62, 121, 58, 125, 60, 0x77fd04);
=======
         tft.fillTriangle(0, 60, 4, 58, 4, 62, 0x77fd04);
         tft.fillTriangle(121, 62, 121, 58, 125, 60, 0x77fd04);
>>>>>>> 1f62df2b9dd1353ef8f05c4a79587ab848b565f1
        }
      }
      if(refresh == 1 && select==1){

        tft.setCursor(80, 144);
        tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
        tft.print("1");
        lcd_image_draw(&map_image, &tft,
          0, 0, // upper-left corner of picture
          10, 4, // upper-left corner of the screen
<<<<<<< HEAD
          107, 107
        ); // draw all rows and columns of the pICTURE

        tft.drawRect(8,2,111,111,tft.Color565(0x00, 0x00, 0xff));
        tft.fillTriangle(0, 60, 4, 58, 4, 62, 0x77fd04);
        tft.fillTriangle(121, 62, 121, 58, 125, 60, 0x77fd04);
        refresh=0;
=======
          107, 107); // draw all rows and columns of the pICTURE

          tft.drawRect(8,2,111,111,tft.Color565(0x00, 0x00, 0xff));
          tft.fillTriangle(0, 60, 4, 58, 4, 62, 0x77fd04);
          tft.fillTriangle(121, 62, 121, 58, 125, 60, 0x77fd04);
          refresh=0;
>>>>>>> 1f62df2b9dd1353ef8f05c4a79587ab848b565f1
      }
      else if(refresh == 1 && select==2){

        tft.setCursor(80, 144);
        tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
        tft.print("2");
        lcd_image_draw(&map_image5, &tft,
          0, 23, // upper-left corner of parrot picture
          10, 4, // upper-left corner of the screen
<<<<<<< HEAD
          107, 107
        ); // draw all rows and columns of the parrot

        tft.drawRect(8,2,111,111,tft.Color565(0x00, 0x00, 0xff));
        tft.fillTriangle(0, 60, 4, 58, 4, 62, 0x77fd04);
        tft.fillTriangle(121, 62, 121, 58, 125, 60, 0x77fd04);
        refresh=0;
      }
=======
          107, 107); // draw all rows and columns of the parrot

          tft.drawRect(8,2,111,111,tft.Color565(0x00, 0x00, 0xff));
          tft.fillTriangle(0, 60, 4, 58, 4, 62, 0x77fd04);
          tft.fillTriangle(121, 62, 121, 58, 125, 60, 0x77fd04);
          refresh=0;
      }

>>>>>>> 1f62df2b9dd1353ef8f05c4a79587ab848b565f1
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
        return;
      }
      if(invSelect==0 && select==3){
        selected=0;
        return;
      }
      if(invSelect==0 && select==3){
        selected=0;
        return 0;
      }
    }
  }
}
void chooseMap(){
  tft.fillScreen(0);
  tft.setCursor(30, 134);
  tft.setTextColor(ST7735_WHITE);
  tft.print("Choose a Map!");
  lcd_image_draw(&map_image, &tft,
    0, 0, // upper-left corner of picture
    10, 4, // upper-left corner of the screen
<<<<<<< HEAD
    107, 107
  ); // draw all rows and columns of the pICTURE
  tft.drawRect(8,2,111,111,tft.Color565(0x00, 0x00, 0xff));
  tft.fillTriangle(0, 60, 4, 58, 4, 62, 0x77fd04);
  tft.fillTriangle(121, 62, 121, 58, 125, 60, 0x77fd04);
=======
    107, 107); // draw all rows and columns of the pICTURE
    tft.drawRect(8,2,111,111,tft.Color565(0x00, 0x00, 0xff));
    tft.fillTriangle(0, 60, 4, 58, 4, 62, 0x77fd04);
    tft.fillTriangle(121, 62, 121, 58, 125, 60, 0x77fd04);
>>>>>>> 1f62df2b9dd1353ef8f05c4a79587ab848b565f1
  choose_image();
  return;
}
void cleanScreen(){
  tft.fillRect(0,132,128,160,ST7735_BLACK);
}
void initializeScreen(){
  if(board_no == 0){
    lcd_image_draw(&map_image, &tft,
      0, 0, // upper-left corner of parrot picture
      0, 0, // upper-left corner of the screen
      128, 107
    ); // draw all rows and columns of the parrot
    maxValue=30;
  }
  if(board_no == 1){
    lcd_image_draw(&map_image5, &tft,
      0, 0, // upper-left corner of parrot picture
      0, 0, // upper-left corner of the screen
      128, 130
    ); // draw all rows and columns of the parrot
    maxValue=36;
  }
  updateMap(1,1,1);
  updateMap(2,1,1);
}
void preCaution(){
  tft.fillScreen(0);
  tft.setCursor(15, 40);
  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.print("Connecting Snakes");
  tft.setCursor(20, 56);
  tft.print("Please Wait...");
  int ledpin = 13;
  pinMode(ledpin, OUTPUT);
  if (digitalRead(ledpin) == LOW){
    tft.setCursor(15, 92);
    tft.print("Press the button");
    tft.setCursor(20, 108);
    tft.print("when the server");
    tft.setCursor(50, 122);
    tft.print("is On");
    while(true){
      bool invSelect = digitalRead(JOY_SEL);
      if(invSelect==0){
        return;
      }
    }
  }
  else{
    tft.setCursor(8, 90);
    tft.println("Waiting for Client...");
  }
}
void initializeMenu(){
  // tft.setTextColor(tft.Color565(0xff, 0x00, 0x00));
  // tft.setTextWrap(false);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(8, 146);
  tft.print("Roll");
  tft.setCursor(42, 146);
  tft.print("Items");
  tft.setCursor(80, 146);
  tft.print("Forfeit");
  tft.setCursor(8, 146);
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
  tft.print("Roll");
}
void updateItemMenu(int a){
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  cleanScreen();
  tft.setTextSize(1);
  tft.setCursor(35, 134);
  tft.print("Item to Use?");
  if (a==1){
    // tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
    tft.setCursor(30, 146);
    tft.print("2x Dice:");
    tft.setCursor(95, 146);
    tft.print(items[a-1]);
  }
  else if (a==2){
    // tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
    tft.setCursor(30, 146);
    tft.print("2x Value:");
    tft.setCursor(95, 146);
    tft.print(items[a-1]);
  }
  else if (a==3){
    // tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
    tft.setCursor(20, 146);
    tft.print("Choose Dice:");
    tft.setCursor(95, 146);
    tft.print(items[a-1]);
  }
  else if (a==4){
    tft.setCursor(5, 146);
    tft.print("Return to Main Menu");
  }
}
void chooseDice(){
  int press=1;
  delay(500);
  cleanScreen();
  tft.setCursor(2,132);
  tft.print("What would you like to Roll?");
  tft.setCursor(60,146);
  tft.print(pickDice);
  while(true){
    bool invSelect = digitalRead(JOY_SEL);
    int h = analogRead(JOY_HORIZ_ANALOG);
    // // Serial.println("scan_joystick");
    if(h > 550){
      if(press < 6){
        press++;
        pickDice+=1;
        tft.setCursor(60,146);
        tft.print(pickDice);
      }
      delay(200);
    }
    else if(h < 490){
      if(press > 1){
        press--;
        pickDice-=1;
        tft.setCursor(60,146);
        tft.print(pickDice);
      }
      delay(200);
    }
    if(invSelect==0){
      useSpecial=true;
      return;
    }
  }
}
int itemMenu(){
  int press=1;
  cleanScreen();
  updateItemMenu(1);
  while(true){
    bool invSelect = digitalRead(JOY_SEL);
    int h = analogRead(JOY_HORIZ_ANALOG);
    // // Serial.println("scan_joystick");
    if(h > 550){
      if(press < 4){
        press++;
        updateItemMenu(press);
      }
    }
    else if(h < 490){
      if(press > 1){
        press--;
        updateItemMenu(press);
      }
    }
    if(invSelect==0){
      if(press == 1){
        if(items[0]>=1){
          items[0]-=1;
          nextRoll=12;
          delay(500);
          cleanScreen();
          return 0;
        }
      }
      else if(press == 2){
        if(items[1]>=1){
          items[1]-=1;
          doubleDice=true;
          delay(500);
          cleanScreen();
          return 0;
        }
      }
      else if(press == 3){
        if(items[2]>=1){
          items[2]-=1;
          chooseDice();
          delay(500);
          cleanScreen();
          return 0;
        }
      }
      else if(press == 4){
        delay(500);
        return 1;
      }
    }
  }
}
void updateMenu(int a){
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(8, 146);
  tft.print("Roll");
  tft.setCursor(42, 146);
  tft.print("Items");
  tft.setCursor(80, 146);
  tft.print("Forfeit");
  if (a==1){
    tft.setCursor(8, 146);
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
    tft.print("Roll");
  }
  else if(a==2){
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
    tft.setCursor(42, 146);
    tft.print("Items");
  }
  else if(a==3){
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
    tft.setCursor(80, 146);
    tft.print("Forfeit");
  }
  delay(250);
}
void updateQuitMenu(int a){
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, 134);
  tft.print("Really want to Quit?");
  tft.setCursor(35, 146);
  tft.print("Yes");
  tft.setCursor(75, 146);
  tft.print("No");
  if (a==1){
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
    tft.setCursor(35, 146);
    tft.print("Yes");
  }
  else if(a==2){
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
    tft.setCursor(75, 146);
    tft.print("No");
  }
}
void quitMenu(){
  int press=1;
  cleanScreen();
  updateQuitMenu(1);
  while(true){
    bool invSelect = digitalRead(JOY_SEL);
    int h = analogRead(JOY_HORIZ_ANALOG);
    // // Serial.println("scan_joystick");
    if(h > 550){
      if(press < 2){
        press++;
        updateQuitMenu(press);
      }
    }
    else if(h < 490){
      if(press > 1){
        press--;
        updateQuitMenu(press);
      }
    }
    if(invSelect==0){
      if(press == 1){
        quit=true;
        return;
      }
      else if(press == 2){
        return;
      }
    }
  }
}
void chooseMenu(){
  int press=1;
  cleanScreen();
  updateMenu(1);
  while(true){
    bool invSelect = digitalRead(JOY_SEL);
    int h = analogRead(JOY_HORIZ_ANALOG);
    // // Serial.println("scan_joystick");
    if(h > 550){
      if(press < 3){
        press++;
        updateMenu(press);
      }
    }
    else if(h < 490){
      if(press > 1){
        press--;
        updateMenu(press);
      }
    }
    if(invSelect==0){
      if(press == 1){
        return;
      }
      else if(press == 2){
        int aa=itemMenu();
        if(aa==0){
          cleanScreen();
          return;
        }
      }
      else if(press == 3){
        quitMenu();
        cleanScreen();
        if(quit==true){
          return;
        }
        updateMenu(press);
      }
    }
  }
}
void updateMyPosition(){
  dice = rollDice(nextRoll);
  nextRoll=6;
  if(doubleDice==true){
    dice*=2;
    doubleDice=false;
  }
  if(useSpecial==true){
    dice=pickDice;
  }
  tft.fillRect(0,132,128,160,ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setCursor(25, 132);
  tft.print("You Rolled:");
  tft.setCursor(100, 132);
  tft.print(dice);
  Serial3.write(dice);
  Serial.print("I Rolled: ");
  Serial.println(dice);
  myOldPosition = myPosition;
  myPosition+=dice;
  if(myPosition>maxValue){
    myPosition=maxValue;
  }
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
}
void updateOppPosition(){
  dice=Serial3.read();
  // Serial3.write(dice);
  if(dice>12){
    winner=1;
    selected=4;
    return;
  }
  cleanScreen();
  tft.setCursor(10, 132);
  tft.print("Opponent Rolled:");
  tft.setCursor(110, 132);
  tft.print(dice);
  Serial.print("Opponent Rolled: ");
  Serial.println(dice);
  oppOldPosition = oppPosition;
  oppPosition+=dice;
  if(oppPosition>maxValue){
    oppPosition=maxValue;
  }
  updateMap(2,oppPosition,oppOldPosition);
  delay(2000);
  oppOldPosition = oppPosition;
  oppPosition = checkSnakeLadder(oppPosition);
  updateMap(2,oppPosition,oppOldPosition);
  Serial.print("Opponent Position: ");
  Serial.println(oppPosition);
  Serial.println("Dumping Buffers...");
  oppOldPosition=oppPosition;
}
void getSLPosition(){
  int a[5]={12,14,17,31,35};
  int b[5]={2,11,4,19,22};
  int c[5]={3,5,15,18,21};
  int d[5]={16,7,25,20,32};
  if(board_no == 1){
    for(int i=0; i<5; i++){
      snakeposition[i]=a[i];
      snakemoveto[i]=b[i];
      ladderposition[i]=c[i];
      laddermoveto[i]=d[i];
    }
  }
}
void showTurn(){
  tft.fillScreen(0);
  tft.setCursor(25, 50);
  tft.setTextSize(2);
  tft.print("You Go: ");
  tft.setCursor(60, 75);
  tft.setTextSize(3);
  tft.print(turn);
  tft.setTextSize(1);
  delay(1000);
}
void check_game_end(int position){
  if(board_no==0 && position>=30){
    winner=turn;
  }
  else if(board_no==1 && position>=36){
    winner=turn;
  }
}
int gameMain(){
  preCaution();
  dumpBuffer();
  establishCommunication();
  turn=decideOrder();
  calculateMapCoordinate();
  getSLPosition();
  showTurn();
  initializeScreen();
  // initializeMenu();
  dumpBuffer();

  //Main Game Loop
  while (true){
    dumpBuffer();
    if(turn == 1){
      delay(2000);
      cleanScreen();
      chooseMenu();
      if(quit==true){
        winner=2;
        quit=false;
        selected=4;
        return 0;
      }
      updateMyPosition();
      check_game_end(myPosition);
      if(winner!=0){
        Serial.print("turn: ");
        Serial.print(turn);
        Serial.print(" Winner: ");
        Serial.println(winner);
        selected=4;
        return 0;
      }
      // Serial.print("In Turn: ");
      // Serial.println(turn);
      turn=2;
      dumpBuffer();
    }
    if(turn == 2){
      cleanScreen();
      Serial.print("In Turn: ");
      Serial.println(turn);
      // TODO: wait
      Serial.println("Waiting for opponent to roll...");

      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      tft.setTextWrap(true);
      tft.setCursor(0, 132);
      tft.println("Waiting for opponent to roll...");
      dumpBuffer();
      while(true){
        if(Serial3.available()>0){
          updateOppPosition();
          break;
        }
      }
      check_game_end(oppPosition);
      if(winner!=0){
        selected=4;
        return 0;
      }
      turn=1;
      dumpBuffer();
    }
  }
}
void gameEndScreen(){
  tft.fillScreen(0);
  tft.setTextSize(2);
  if(winner==1){
    tft.setCursor(25, 65);
    tft.print("You WON");
  }
  else{
    tft.setCursor(20, 65);
    tft.print("You LOST");
  }
  tft.setTextSize(1);
  tft.setCursor(10, 110);
  tft.print("Click to return to");
  tft.setCursor(40, 120);
  tft.print("main menu");
  winner=0;
  selected=0;
  while(true){
    bool invSelect = digitalRead(JOY_SEL);
    if(invSelect==0){
      return;
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
    else if(selected==4){
      gameEndScreen();
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
}
int menu(){
  selected=1;
  // prev_selected=1;
  tft.fillScreen(0);
  int t_col=1;
  int t_row=1;
<<<<<<< HEAD
  lcd_image_draw(&main_menu_image, &tft,
    0, 0, // upper-left corner of parrot picture
    0, 0, // upper-left corner of the screen
    128, 160
  ); // draw all rows and columns of the parrot
  tft.setTextColor(tft.Color565(0xff, 0x00, 0x00));
  tft.setTextWrap(false);
  tft.setTextSize(2);
=======
  lcd_image_draw(&map_image6, &tft,
    0, 0, // upper-left corner of parrot picture
    0, 0, // upper-left corner of the screen
    128, 160); // draw all rows and columns of the parrot

  tft.setTextColor(tft.Color565(0xff, 0x00, 0x00));

  tft.setTextSize(2);

  tft.setTextWrap(false);
  
>>>>>>> 1f62df2b9dd1353ef8f05c4a79587ab848b565f1
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
    choose();
    // gameMain();
    // initializeScreen();
    // gameEndScreen();
  }
  Serial.print("returned in main function");
  Serial3.end();
  Serial.end();
  return 0;
}
