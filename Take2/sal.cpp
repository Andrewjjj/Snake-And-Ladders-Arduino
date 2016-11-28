#include <Arduino.h>

#define JOY_SEL 9

int current_screen = 0;
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

  enum Server { LISTEN=1, WFK, WFA, WFK2, WFA2, DataExchange };
  Server curr_state = LISTEN;

  char CR ='C';
  char ACK = 'A';
  uint32_t ckey;
  while (true){
    if(curr_state == LISTEN){
      Serial.println("Listening...");
      if((wait_on_serial3(1,-1) !=0) && Serial3.read() == CR){
        curr_state = WFK;
      }
    }
    else if(curr_state == WFK){
      Serial.println("Waiting For Key...");
      if(wait_on_serial3(4,1000) != 0){
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
      Serial.println("Waiting For Ack...");
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
      Serial.println("Waiting For Key...");
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
      Serial.println("Waiting For Ack...");
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
  int my_position=0;
  int opponent_position=0;
  int playorder;

  Serial.println("Randomly deciding who will go first..");
  delay(2000);
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
int gameMain(){
  establishCommunication();
  int turn=decideOrder();
  Serial.print("My Order is: ");
  Serial.println(playorder);

  //Main Game Loop
  while (true){

  }
  // int diceValue;
  // int my_position=0;
  // int opponent_position=0;
  //
  // Serial.println("Roll the die do see who goes first");
  // Serial.println("Click the Button to Roll");
  // bool joy_select = digitalRead(JOY_SEL);
  // while (joy_select==1){
  //   joy_select = digitalRead(JOY_SEL);
  //   if (joy_select == 0){
  //     Serial.println("CLICKED!");
  //     diceValue = rollDice();
  //     break;
  //   }
  // }
  // Serial.print("Your Die value is: ");
  //
  // Serial.println(diceValue);
  return 0;
}

void setup(){
  Serial.begin(9600);
  Serial3.begin(9600);
  pinMode(JOY_SEL, INPUT);
  digitalWrite(JOY_SEL, HIGH);
}
int main(){
  init();
  setup();
  // while(true){
    gameMain();
  // }
  Serial.end();
  Serial3.end();
}
