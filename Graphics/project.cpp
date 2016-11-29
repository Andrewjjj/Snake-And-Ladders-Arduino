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
// Initialize the screen
int selected=0;
int prev_selected=0;
int curr_joy_select =1;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
lcd_image_t map_image = { "snake.lcd", 128, 128 };
lcd_image_t map_image1 = { "snake1.lcd", 20, 18 };

int menu();
int Instructions(){
  tft.fillScreen(0);
  tft.setCursor(1, 1);
  tft.setTextColor(ST7735_WHITE);
  tft.print("Rules of the Game Bro");
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
int chooseMap(){
  tft.fillScreen(0);
  tft.setCursor(1, 108);
  tft.setTextColor(ST7735_WHITE);
  tft.print("lets play snake and \n\r ladders");
  lcd_image_draw(&map_image, &tft,
                 0, 0, // upper-left corner of parrot picture
                 0, 0, // upper-left corner of the screen
                 128, 107); // draw all rows and columns of the parrot
  while(true){
    bool invSelect = digitalRead(JOY_SEL);
    if(invSelect==0){
      selected=0;
      return 0;
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
      // playGame();
    }
  }
}

int checkCursor(){
  Serial.println("oooooo");


  while(true){
    int v = analogRead(JOY_VERT_ANALOG);
    // Serial.println("scan_joystick");
    if(v > 510){
      // Serial.println("v1");
      if(selected==prev_selected){
        selected=2;
        tft.setCursor(38, 114);
        tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
        tft.print("Start Game");

        tft.setCursor(32, 132);
        tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
        tft.print("Instructions");
        prev_selected=selected;
      }
    }
    if(v < 490){
      if(selected==prev_selected){
        tft.setCursor(32, 132);
        tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
        tft.print("Instructions");

        tft.setCursor(38, 114);
        tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
        tft.print("Start Game");
        selected=1;
        prev_selected=selected;
      }
    }
    bool invSelect = digitalRead(JOY_SEL);
    Serial.println(invSelect);
    if(invSelect==0){
      return 0;
    }
  }
  // checkJoySelect();
  // Serial.print(invSelect);
  Serial.println("hanji");
  Serial.println("hello");
}
int menu(){
  selected=0;
  prev_selected=0;
  tft.fillScreen(0);
  int t_col=1;
  int t_row=1;
  lcd_image_draw(&map_image1, &tft,
                 0, 0, // upper-left corner of parrot picture
                 108, 1, // upper-left corner of the screen
                 20, 18); // draw all rows and columns of the parrot
  tft.setTextColor(tft.Color565(0xff, 0x00, 0x00));
  tft.setTextSize(2);
  tft.setTextWrap(false);
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
  Serial.end();
  return 0;
}
