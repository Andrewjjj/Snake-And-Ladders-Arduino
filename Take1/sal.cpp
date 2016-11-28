#include <Arduino.h>
//
//
// /*============================================================
// Client State Machine Function
// ============================================================*/
// int sendDice(int myDice){
//   enum Client { START=1, WaitingForAck, DataExchange  };
//   Client curr_state = START;
//
//   int oppDice;
//   while (true){
//     if (curr_state == START){
//       Serial.println("Start");
//       Serial3.write('C');
//       Serial3.write(myDice);
//       curr_state = WaitingForAck;
//     }
//     else if ((curr_state == WaitingForAck)){
//       if((wait_on_serial3(1,-1) != 0)){
//         if(Serial3.read() == 'C'){
//           Serial.println("C Recieved");
//           curr_state = WaitingForAck;
//         }
//         else if(Serial3.read() == 'A'){
//           Serial.println("A Recieved, Dataexchange");
//           curr_state = DataExchange;
//         }
//       }
//       else{
//         curr_state = START;
//       }
//     }
//     else if(curr_state == DataExchange){
//       return 0;
//     }
//     else{
//       curr_state = START;
//     }
//   }
// }
// /*============================================================
// Server State Machine Function
// ============================================================*/
// int recieveDice(){
//
//   enum Server { LISTEN=1, WFK, WFA, WFK2, WFA2, DataExchange };
//   Server curr_state = LISTEN;
//
//   char CR ='C';
//   char ACK = 'A';
//   int oppDice;
//   while (true){
//     if(curr_state == LISTEN){
//       Serial.println("Listening...");
//       if((wait_on_serial3(1,-1) !=0) && Serial3.read() == CR){
//         curr_state = WFK;
//       }
//     }
//     else if(curr_state == WFK){
//       Serial.println("Waiting For Key...");
//       if(wait_on_serial3(1,1000) != 0){
//         oppDice=Serial3.read();
//         Serial.print("opponent Dice Recieved: ");
//         Serial.println(oppDice);
//         if(oppDice >= 1 && oppDice <= 6){
//           Serial.println("Correct OppDice, going to DATAEXCHANGE");
//           for (int i=0; i<20; i++){
//             Serial.println("ACK SENT");
//             Serial3.write(ACK);
//           }
//           curr_state = DataExchange;
//         }
//         else{
//           Serial.println("WRONG OppDice, GOING BACK");
//           curr_state = LISTEN;
//         }
//       }
//       else{
//         curr_state = LISTEN;
//       }
//     }
//     else if(curr_state == DataExchange){
//       Serial.println("DataExchange!");
//       return oppDice;
//     }
//     else{
//       curr_state = LISTEN;
//     }
//   }
// }
//

#define JOY_SEL 9

int current_screen = 0;

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


bool wait_on_serial3( uint8_t nbytes, long timeout ) {
  unsigned long deadline = millis() + timeout;//wraparound not a problem
  while (Serial3.available()<nbytes && (timeout<0 || millis()<deadline))
  {
    delay(1); // be nice, no busy loop
  }
  return Serial3.available()>=nbytes;
}


int decideOrder(){
  int my_position=0;
  int opponent_position=0;
  int playorder;

  Serial.println("Randomly deciding who will go first..");
  delay(2000);
  // bool joy_select = digitalRead(JOY_SEL);
  // while (joy_select==1){
  //   joy_select = digitalRead(JOY_SEL);
  //   if (joy_select == 0){
  //     Serial.println("CLICKED!");
  //     myDice = rollDice();
  //     break;
  //   }
  // }
  // Serial.print("Your Die value is: ");
  // Serial.println(myDice);

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
  Serial.print("My Order is: ");
  Serial.println(playorder);
  return playorder;
}
int gameMain(){
  int turn=decideOrder();
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
