REDIRECT_STDOUT_TO(Serial)
#include "Arduino_GigaDisplay_GFX.h"
#include "Arduino_GigaDisplayTouch.h"
#include <Arduino_GigaDisplay.h>
//#include "C:\Users\taran\OneDrive\Documents\Arduino\NavTimer\NavTimerGPS.h"
#include "NavTimerGPS.h"
#include "NavTimerSD.h"


//Clock to guide pilots doing the navigation challenge
//Need to add:
//  -Button to start timer
//  -Timer for total time
//  -Timer for leg time


// Define button dimensions and positions
#define BUTTON_WIDTH 70
#define BUTTON_HEIGHT 50
#define BUTTON_MARGIN 10
#define BUTTON_START_X 540
#define BUTTON_START_Y 100

//#define DISPLAY_BLACK 0x0000
#define DISPLAY_WHITE 0xffff
#define DISPLAY_BLUE 0xAAAA
#define DISPLAY_GREEN 0x5555

#define BLACK 0x0000
#define WHITE 0xffff
GigaDisplay_GFX display;
Arduino_GigaDisplayTouch touchDetector;
int threshold = 500;
int lastTouch;

GigaDisplayRGB rgb;  //create rgb object

int numLegs = 0;
int num_of_current_leg = 1;
//IN SECONDS
//int total_time_left;
//int leg_time_left;
String screen = "Start";
int sec_passed;
bool timer_started = false;
bool stop_timer_confirmation = false;
bool add_new_leg_distance = true;

struct Leg {
  int leg_time;
  double leg_time_left;
  struct Leg* next_leg;
  struct Leg* previous_leg;
  double leg_distance;
  double leg_distance_decimal;
};

struct Leg first_leg;
struct Leg* first_leg_ptr = &first_leg;
struct Leg* current_leg = &first_leg;

const int ROWS = 4;
const int COLS = 3;
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {0x7F, '0', '0'}
};

int enter_time = 1;
int hrs = 0;
int mins = 0;
int secs = 0;

double distance = 0;
double distance_decimal = 0;
int enter_distance = 1;
double speed[5];
double speed_flown[5];
int speed_index = 0;
int time_at_speed_check;
int time_at_last_speed_check;
double distance_at_last_speed_check = 0;
double distance_at_last_speed_check_decimal = 0;

uint8_t Current_GPS_Hrs;
uint8_t Current_GPS_Mins;
uint8_t Current_GPS_Secs;
uint8_t Prev_GPS_Hrs;
uint8_t Prev_GPS_Mins;
uint8_t Prev_GPS_Secs;


void setup() {
  Serial.begin(9600);
  GPS_Setup();
  //SD_Setup();
  //Serial.println(F("Touch Paint!"));

 //rgb.begin();  //init the library

  display.begin();
  display.fillScreen(BLACK);
  display.setRotation(1);
  /*rgb.on(255, 0, 0)
  delay(1000)
  rgb.on(0, 255, 0)
  delay(1000)
  rgb.on(0, 0, 255)
  delay(1000)
  rgb.on(255,255,255)
  */
  display.setCursor(10,10);
  display.setTextSize(5);
  display.print("Hello World!");

  display.setCursor(10, 80);
  display.print("Total Time Left: ");

  display.setCursor(10, 160);
  display.print("Time Left in Leg: ");

  //display.drawRect(600, 200, 100, 80, 0xffff);
  display.fillRect(650, 200, 100, 80, 0xffff);
  display.setTextColor(BLACK);
  display.setCursor(660,220);
  display.setTextSize(3);
  display.print("Start");

  if (touchDetector.begin()) {
    //Serial.print("Touch controller init - OK");
  } else {
    //Serial.print("Touch controller init - FAILED");
    while (1) {
      rgb.on(255, 0, 0);
      delay(1000);
      rgb.off();
      delay(1000);
    }
  }
}



bool isButtonPressed(int buttonX, int buttonY) {
  //TSPoint touch = ts.getPoint();
  //pinMode(YP, OUTPUT);
  //pinMode(XM, OUTPUT);
  uint8_t contacts;
  GDTpoint_t points[5];
  contacts = touchDetector.getTouchPoints(points);
  //X AND Y ARE FLIPPED DUE TO ROTATION
    int touchX = points[0].x;
    int touchY = points[0].y;
    if (touchX >= buttonX && touchX <= buttonX + BUTTON_WIDTH &&
        touchY >= buttonY && touchY <= buttonY + BUTTON_HEIGHT) {
      return true;
    }
    return false;
}

//MAIN LOOP
void loop() {

  uint8_t contacts;
  GDTpoint_t points[5];
  contacts = touchDetector.getTouchPoints(points);
  Check_GPS();
  //millis();
  if (contacts > 0 && (millis() - lastTouch > threshold)) {

    if (contacts == 0) {
      rgb.off();
      return;
    }
    rgb.on(0, 32, 0);

    // Retrieve a point
    //THE POINTS ARE FLIPPED FOR TOUCHING
    // X IS THE UNROTATED X COORDINATE, Y IS THE UNROTATED Y COORDINATE
    int touch_x = points[0].y;
    int touch_y = 480 - points[0].x;

  //START BUTTON
    if (screen.equals("Start") && touch_x > 650 && touch_x < 800 && touch_y > 100 && touch_y < 300) {//((touch_x > 400 && touch_x < 600) && (touch_y > 200 && touch_y < 300)) {
      GPS_Active_Screen();
      Add_Leg_Screen();
      screen = "add_leg";
      //delay(1000);
      display.setCursor(20, 360);
      //delay(300);
      //while (true) {
        //delay(3000);
        //touch_x = points[0].y;
        //touch_y = points[0].x;
        //if (isButtonPressed(touch_x, touch_y)) {
          //display.fillScreen(BLACK);
        //}
      //}
    }

    //if(screen.equals("add_leg")) {
    //  display.fillScreen(BLACK);
    //}

  //KEYPAD INPUTS

  //ADD LEG SCREEN
    else if (screen.equals("add_leg")) {
      for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
          int buttonX = BUTTON_START_X + (BUTTON_WIDTH + BUTTON_MARGIN) * col;
          int buttonY = BUTTON_START_Y + (BUTTON_HEIGHT + BUTTON_MARGIN) * row;

          if ((touch_x > buttonX && touch_x < (buttonX + BUTTON_WIDTH)) && (touch_y > buttonY && touch_y < (buttonY + BUTTON_HEIGHT))) {
            //delay(200);
            //display.fillRect(buttonX, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT, WHITE);
            //display.setCursor(180, 360);
            //display.print(keys[row][col]);
            Time_Entry(keys[row][col], enter_time);
            //delay(200);
          }
          /*if (isButtonPressed(buttonX, buttonY)) {
            display.fillScreen(BLACK);
          }*/
        }
      }

      //BUTTON TO DETECT INPUT ON HOURS
      if ((touch_x > 20 && touch_x < 70) && (touch_y > 350 && touch_y < 410)) {
        display.setCursor(20, 360);
        enter_time = 1;
      }
      //BUTTON TO DETECT INPUT ON MINUTES
      if ((touch_x > 170 && touch_x < 220) && (touch_y > 350 && touch_y < 410)) {
        display.setCursor(170, 360);
        enter_time = 2;
      }
      //BUTTON TO DETECT INPUT ON SECONDS
      if ((touch_x > 320 && touch_x < 370) && (touch_y > 350 && touch_y < 410)) {
        display.setCursor(320, 360);
        enter_time = 3;
      }

      //BUTTON TO GO BACK TO THE TIMER SCREEN
      if ((touch_x > 400 && touch_x < 550) && (touch_y > 20 && touch_y < 100)) {
        screen = "timer";
        Timer_Screen();
      }

      //BUTTON TO FINISH ADDING NEW LEG
      if ((touch_x > 500 && touch_x < 800) && (touch_y > 360 && touch_y < 460)) {
        //display.print("balls");

        //  -------------
        screen = "confirmation";
        //screen = "timer";
        //  -------------
        
        //Timer_Screen();
        
        Confirm_Leg_Screen();
        //Timer_Screen();
        //display.print(time);
      }
    }

    //TIMER SCREEN
    else if (screen.equals("timer")) {

      //UPDATE GPS SATELLITE LOCK
      /*display.setCursor(500, 20);
      display.print(" ");
      display.setCursor(500, 20);
      if (GPS_Get_Satellites() == 0) {
        display.print(" No Lock");
      }
      else{
        display.print(" ");
        display.print(GPS_Get_Satellites());
        display.print("1");
        Serial.print(GPS_Get_Satellites());
        //display.print(" Satellites Locked");
      }*/


      //ADD LEG BUTTON
      if (touch_x > 650 && touch_x < 800 && touch_y > 200 && touch_y < 300) {
        Add_Leg_Screen();
        screen = "add_leg";
        display.setCursor(20, 360);
      }
      //REMOVE LEG BUTTON
      else if (touch_x > 650 && touch_x < 800 && touch_y > 100 && touch_y < 200) {
        Confirm_Remove_Leg_Screen();
        screen = "confirm_remove_leg";
        /*//If it is not the first, nor the last leg
        if (current_leg->previous_leg != NULL) {
          if (current_leg->next_leg != NULL) {
            current_leg->previous_leg->next_leg = current_leg->next_leg;
            current_leg->next_leg->previous_leg = current_leg->previous_leg;
            current_leg = current_leg->previous_leg;
          }
          //If it is the last leg
          else {
            current_leg->previous_leg->next_leg = NULL;
            current_leg = current_leg->previous_leg;
          }
          numLegs--;
          num_of_current_leg--;
        }
        //If it is the first leg
        else if (current_leg->next_leg != NULL) {
            current_leg->next_leg->previous_leg = NULL;
            current_leg = current_leg->next_leg;
            numLegs--;
            first_leg_ptr = current_leg;
            //first_leg = *current_leg;
        }
        Timer_Screen();*/
      }
      //NEXT LEG BUTTON
      else if (touch_x > 600 && touch_x < 800 && touch_y > 300 && touch_y < 380) {
        if (current_leg->next_leg != NULL) {
          //current_leg->next_leg->leg_time_left =
          current_leg = current_leg->next_leg;
          num_of_current_leg++;
          Speed_Clear();
          Timer_Screen();
        }
        
      }
      //PREVIOUS LEG BUTTON
      else if (touch_x > 600 && touch_x < 800 && touch_y > 400 && touch_y < 480) {
        if (current_leg->previous_leg != NULL) {
          current_leg = current_leg->previous_leg;
          num_of_current_leg--;
          Speed_Clear();
          Timer_Screen();
        }
      }
      //START AND STOP TIME BUTTON
      else if (touch_x > 250 && touch_x < 450 && touch_y > 360 && touch_y < 440) {
        if (!timer_started) {
          sec_passed = millis();
          timer_started = true;
          Return_Time(&Current_GPS_Hrs, &Current_GPS_Mins, &Current_GPS_Secs);
          Return_Time(&Prev_GPS_Hrs, &Prev_GPS_Mins, &Prev_GPS_Secs);
          Timer_Screen();
        }
        else {
          //timer_started = false;
          //Timer_Screen();
          screen = "confirm_stop_timer";
          Confirm_Stop_Timer_Screen();
          //delay(1000);
        }
      }
      //BUTTON TO SPEED CHECK
      else if (touch_x > 20 && touch_x < 220 && touch_y > 360 && touch_y < 440) {
        if (distance_at_last_speed_check == 0 && distance_at_last_speed_check_decimal == 0) {
          distance_at_last_speed_check = current_leg->leg_distance;
          distance_at_last_speed_check_decimal = current_leg->leg_distance_decimal;
        }
        add_new_leg_distance = false;
        screen = "add_distance";
        time_at_speed_check = Get_Current_Sum_Time();
        distance = 0;
        distance_decimal = 0;
        enter_distance = 1;
        Add_Distance_Screen();
      }
    }
    //delay(100);



    //if (/*screen.equals("add_leg") && */touch_x > 540 && touch_x < 610 && touch_y > 100 && touch_y < 150) {
    //  display.fillScreen(WHITE);
    //}

    /*if (touch_y > 200 && touch_y < 300) {
      display.setTextColor(BLACK);
      display.setCursor(660,220);
      display.setTextSize(3);
      display.print("wah wah wee wah");
    }*/

    /*if (touch_y > 100 && touch_y < 200) {
      display.fillScreen(BLACK);
      display.fillRect(650, 200, 100, 80, 0xffff);
      display.setTextColor(BLACK);
      display.setCursor(660,220);
      display.setTextSize(3);
      display.print("Start");
    }*/

    //Confirmation screen
    else if (screen.equals("confirmation")) {
      if (touch_x > 400 && touch_x < 700 && touch_y > 200 && touch_y < 300) {
        //TOTAL TIME IN SECONDS
        int time = secs + (60 * (mins + (60 * hrs)));
        if(numLegs == 0) {
          first_leg = New_Leg(time, time, NULL, NULL);
        }
        else {
         // struct Leg* temp_leg = Find_Last_Leg(&first_leg);
          struct Leg* temp_leg = Find_Last_Leg(first_leg_ptr);
          //display.print(temp_leg->leg_time);
          temp_leg->next_leg = (struct Leg*) malloc(sizeof(struct Leg));
          *temp_leg->next_leg = New_Leg(time, time, NULL, temp_leg);
          //first_leg.next_leg = (struct Leg*) malloc(sizeof(struct Leg));
          //*first_leg.next_leg = New_Leg(time, time, NULL, &first_leg);
          //temp_leg->next_leg->leg_time = time;
          //temp_leg->next_leg->leg_time_left = time;
          //temp_leg->next_leg->next_leg = NULL;
          //temp_leg->next_leg->previous_leg = temp_leg;
        }
        //screen = "timer";
        //Timer_Screen();
        add_new_leg_distance = true;
        screen = "add_distance";
        Add_Distance_Screen();
      }
      else if (touch_x > 100 && touch_x < 400 && touch_y > 200 && touch_y < 300) {
        screen = "add_leg";
        Add_Leg_Screen();
      }
    }

    else if (screen.equals("confirm_stop_timer")) {
      //Button to confirm stop
      if (touch_x > 400 && touch_x < 700 && touch_y > 200 && touch_y < 300) {
        timer_started = false;
        screen = "timer";
        Timer_Screen();
      }
      //Back button
      else if (touch_x > 100 && touch_x < 400 && touch_y > 200 && touch_y < 300) {
        screen = "timer";
        Timer_Screen();
      }
      //RESET BUTTON
      else if (touch_x > 250 && touch_x < 550 && touch_y > 350 && touch_y < 450) {
        timer_started = false;
        current_leg->leg_time_left = (int)current_leg->leg_time;
        screen = "timer";
        Timer_Screen();
      }
    }
    //SCREEN FOR CONFIRMING REMOVAL OF A LEG
    else if (screen.equals("confirm_remove_leg")) {
      //Button to confirm removing leg
      if (touch_x > 400 && touch_x < 700 && touch_y > 200 && touch_y < 300) {
          //If it is not the first, nor the last leg
          if (current_leg->previous_leg != NULL) {
            if (current_leg->next_leg != NULL) {
              current_leg->previous_leg->next_leg = current_leg->next_leg;
              current_leg->next_leg->previous_leg = current_leg->previous_leg;
              current_leg = current_leg->previous_leg;
            }
            //If it is the last leg
            else {
              current_leg->previous_leg->next_leg = NULL;
              current_leg = current_leg->previous_leg;
            }
            numLegs--;
            num_of_current_leg--;
          }
          //If it is the first leg
          else if (current_leg->next_leg != NULL) {
              current_leg->next_leg->previous_leg = NULL;
              current_leg = current_leg->next_leg;
              numLegs--;
              first_leg_ptr = current_leg;
              //first_leg = *current_leg;
          }
          screen = "timer";
          Timer_Screen();
      }
      //Back button
      else if (touch_x > 100 && touch_x < 400 && touch_y > 200 && touch_y < 300) {
        screen = "timer";
        Timer_Screen();
      }
    }

    //ADD DISTANCE SCREEN
    else if (screen.equals("add_distance")) {
      for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
          int buttonX = BUTTON_START_X + (BUTTON_WIDTH + BUTTON_MARGIN) * col;
          int buttonY = BUTTON_START_Y + (BUTTON_HEIGHT + BUTTON_MARGIN) * row;

          if ((touch_x > buttonX && touch_x < (buttonX + BUTTON_WIDTH)) && (touch_y > buttonY && touch_y < (buttonY + BUTTON_HEIGHT))) {
            //Time_Entry(keys[row][col], enter_time);
            Distance_Entry(keys[row][col], enter_distance);
          }
          /*if (isButtonPressed(buttonX, buttonY)) {
            display.fillScreen(BLACK);
          }*/
        }
      }

      //BUTTON TO DETECT INPUT ON MILES
      if ((touch_x > 170 && touch_x < 240) && (touch_y > 350 && touch_y < 410)) {
        display.setCursor(180, 360);
        enter_distance = 1;
      }
      //BUTTON TO DETECT INPUT ON DISTANCE DECIMAL
      if ((touch_x > 270 && touch_x < 340) && (touch_y > 350 && touch_y < 410)) {
        display.setCursor(280, 360);
        enter_distance = 2;
      }

      //BUTTON TO GO BACK TO THE TIMER SCREEN
      if ((touch_x > 400 && touch_x < 550) && (touch_y > 20 && touch_y < 100)) {
        screen = "timer";
        Timer_Screen();
      }

      //BUTTON TO FINISH ADDING NEW DISTANCE
      if ((touch_x > 500 && touch_x < 800) && (touch_y > 360 && touch_y < 460)) {
        //display.print("balls");
        //if (speed_index == 4) {
        //  speed_index = 0;
        //}
        if (add_new_leg_distance) {
          Find_Last_Leg(first_leg_ptr)->leg_distance = distance;
          Find_Last_Leg(first_leg_ptr)->leg_distance_decimal = distance_decimal;
        }
        else {
          Ground_Speed_Push_Back();
        }
        //speed[speed_index] = (distance + (distance_decimal / 10.0)) / ((time_at_speed_check/60.0)/60.0);
        //speed_index++;
        //  -------------
        screen = "timer";
        //screen = "timer";
        //  -------------
        
        //Timer_Screen();
        
        Timer_Screen();
        //Timer_Screen();
        //display.print(time);
      }
    }

    lastTouch = millis();
  }
//END OF THE LOOP FOR TRACKING TOUCH INPUTS
  if (screen.equals("timer")) {
    display.setTextSize(3);
    display.setCursor(630, 20);
    display.print(" ");
    display.setCursor(630, 20);
    if (GPS_Get_Satellites() == 0) {
      display.print(" No Lock");
    }
    else{
      display.print("");
      display.print(GPS_Get_Satellites());
      //Serial.print(GPS_Get_Satellites());
      display.print(" Active");
    }
  }
  //CHECK IF TIMER IS ACTIVE AT THE END OF THE MAIN LOOP
  if (screen.equals("timer") && timer_started) { 
    if ((millis() - sec_passed) >= 1000) {
      //Serial.print(millis() - sec_passed);
      float lat;
      float lon;
      Return_Time(&Current_GPS_Hrs, &Current_GPS_Mins, &Current_GPS_Secs);
      Return_Position(&lat, &lon);
      /*
      if (current_leg->leg_time_left <= 0) {
        if (current_leg->next_leg != NULL) {
          current_leg = current_leg->next_leg;
        }
        else {
          timer_started = false;
        }
      }
      else {*/
        //display.fillRect(80, 150, 340, 50, BLACK);
        //display.setTextColor(0xffff, BLACK);
        display.setTextSize(3);
        //current_leg->leg_time_left -= 1;
        //first_leg.leg_time_left -= 1;
        //LEG TIME LEFT - (CURRENT GPS TIME - LAST GPS TIME)
        //LAST GPS = CURRENT GPS
        if (Prev_GPS_Secs == Current_GPS_Secs) {
          first_leg_ptr->leg_time_left -= 1;
          Prev_GPS_Secs++;

        }
        else{
          first_leg_ptr->leg_time_left -= ((60*60*Current_GPS_Hrs) + (60*Current_GPS_Mins) + Current_GPS_Secs) - ((60*60*Prev_GPS_Hrs) + (60*Prev_GPS_Mins) + Prev_GPS_Secs);
          Prev_GPS_Hrs = Current_GPS_Hrs;
          Prev_GPS_Mins = Current_GPS_Mins;
          Prev_GPS_Secs = Current_GPS_Secs;
        }
        //SD_WritePosition(current_leg->leg_time_left, num_of_current_leg, lat, lon);

  //THIS CODE CHANGES THE WAY THE LEG TIME LEFT IS DISPLAYED, IT WILL NOW DISPLAY TOTAL TIME LEFT FROM THE SUM OF ALL THE LEGS UP TO THE CURRENT LEG
  //so the time is stored in each leg, it needs to be stored in the first leg and every time displayed is the first leg_time_left + legs_time_left up until the current_leg
        Leg* temp_leg = first_leg_ptr;  //&first_leg;
        double total_leg_time = 0;
        while (temp_leg != current_leg) {
          total_leg_time += temp_leg->leg_time_left;
          temp_leg = temp_leg->next_leg;
        }
        total_leg_time += current_leg->leg_time_left;

        display.setCursor(80, 250);

        display.print( (int)((total_leg_time/60.0)/60.0));
        //display.print((current_leg->leg_time_left/60)/60);
        display.print("Hrs  ");

        display.setCursor(230, 250);
        display.print( (int)(total_leg_time/60.0)%60);
        //display.print((current_leg->leg_time_left/60)%60);
        display.print("Mins  ");

        display.setCursor(380, 250);
        display.print( (int)total_leg_time%60);
        //display.print((current_leg->leg_time_left%60));
        display.print("Secs  ");
        sec_passed = millis();
      //}
      
    }
  }


}

//TIMER SCREEN
void Timer_Screen() {
  display.setTextColor(0xffff, BLACK);
  display.fillScreen(BLACK);
  display.setCursor(20, 20);
  display.setTextSize(3);
  display.print(numLegs);
  display.print(" legs left");

  //DISPLAY NUMBER OF SATELLITES LOCKED
  display.setCursor(470, 20);
  display.print("GPS Lock:");

  //struct Leg* tempLeg = first_leg_ptr;  //&first_leg;
  struct Leg* tempLeg = current_leg;
  int i = 0;
  while(i < 5) {
    if (i < numLegs) {
      

      display.setCursor(20, 50 +(30*i));
      display.print("Leg ");
      //display.print(i+1);
      display.print(num_of_current_leg+i);
      display.print(": ");

      display.print( (int)((tempLeg->leg_time_left/60.0)/60.0));
      display.print("Hrs  ");

      display.print( ((int)(tempLeg->leg_time_left/60.0))%60);
      display.print("Mins   ");

      display.print( ((int)(tempLeg->leg_time_left)) %60);
      display.print("Secs");

      if (tempLeg->next_leg != NULL) {
        tempLeg = tempLeg->next_leg;
      }
      //BREAK CONDITION
      else {
        i = 5;
      }
    }
    i++;
  }

  //SAME CODE AS ABOVE
  Leg* temp_leg = first_leg_ptr;  //&first_leg;
  double total_leg_time = 0;
  while (temp_leg != current_leg) {
    total_leg_time += temp_leg->leg_time_left;
    temp_leg = temp_leg->next_leg;
  }
  total_leg_time += current_leg->leg_time_left;

  display.setCursor(20, 200);
  display.print("Time Left in Leg ");
  display.print(num_of_current_leg);
  display.print(": ");
  
  //display.print(((temp_leg->leg_time)))
  display.setCursor(80, 250);
  display.print((int)((total_leg_time/60.0)/60.0));
  //display.print((current_leg->leg_time_left/60)/60);
  display.print("Hrs ");

  display.setCursor(230, 250);
  display.print( ((int)(total_leg_time/60.0)) %60);
  //display.print((current_leg->leg_time_left/60)%60);
  display.print("Mins ");

  display.setCursor(380, 250);
  display.print( ((int)(total_leg_time) %60));
  //display.print((current_leg->leg_time_left%60));
  display.print("Secs ");

  display.fillRect(650, 200, 150, 80, 0xffff);
  display.setTextColor(BLACK);
  display.setCursor(670,220);
  display.setTextSize(3);
  display.print("Add Leg");

  display.fillRect(650, 100, 150, 80, 0xffff);
  display.setCursor(670,120);
  display.print("Remove Leg");

  display.fillRect(600, 300, 200, 80, 0xffff);
  display.setCursor(620, 320);
  display.print("Next Leg");

  display.fillRect(600, 400, 200, 80, 0xffff);
  display.setCursor(620, 420);
  display.print("Prev Leg");

  if (timer_started) {
    display.fillRect(250, 360, 200, 80, 0xffff);
    display.setCursor(270, 380);
    display.print("Stop Timer");
  }
  else {
    display.fillRect(250, 360, 200, 80, 0xffff);
    display.setCursor(270, 380);
    display.print("Start Timer");
  }

  display.fillRect(20, 360, 200, 80, 0xffff);
  display.setCursor(20, 380);
  display.print("Gnd Spd Chk");

  display.setTextColor(0xffff);
  display.setTextSize(2);
  display.setCursor(40, 260);
  display.print("GSR / GSP");
  for (int i = 0; i < 4; i++) {
    if (speed[i] > 0) {
      display.setCursor(20, 280+(20*i));
      display.print("Speed Check ");
      if (speed_index < 4)
        display.print(i+1);
      else
        display.print(speed_index - (3 - i));
      display.print(": ");
      display.print(speed[i]);
      display.print(" kts / ");
      display.print(speed_flown[i]);
      display.print(" kts");
    }
  }

  display.setTextColor(0xffff, BLACK);
  display.setTextSize(4);
}

//ADD LEG SCREEN
void Add_Leg_Screen() {
  hrs = 0;
  mins = 0;
  secs = 0;
  enter_time = 1;

  display.setTextColor(0xffff, BLACK);
  display.fillScreen(BLACK);
  display.setCursor(20, 20);
  display.setTextSize(4);
  display.print("Leg ");
  display.print(numLegs+1);

  display.setCursor(20, 120);
  display.print("How long is this leg? ");
  
  display.drawRect(20, 350, 50, 60, 0xffff);
  display.setCursor(20, 360);
  display.print("0");
  display.setCursor(80, 360);
  display.print("Hrs");

  display.drawRect(170, 350, 50, 60, 0xffff);
  display.setCursor(170, 360);
  display.print("0");
  display.setCursor(230, 360);
  display.print("Mins");

  display.drawRect(320, 350, 50, 60, 0xffff);
  display.setCursor(320, 360);
  display.print("0");
  display.setCursor(380, 360);
  display.print("Secs");

  display.drawRect(500, 360, 300, 100, DISPLAY_BLUE);
  display.setCursor(520, 385);
  display.print("Finish Leg");

  display.drawRect(400, 20, 150, 80, DISPLAY_BLUE);
  display.setCursor(420, 30);
  display.print("Back");

  for (int row = 0; row < ROWS; row++) {
    for (int col = 0; col < COLS; col++) {
      int buttonX = BUTTON_START_X + (BUTTON_WIDTH + BUTTON_MARGIN) * col;
      int buttonY = BUTTON_START_Y + (BUTTON_HEIGHT + BUTTON_MARGIN) * row;
      
      display.fillRect(buttonX, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT, DISPLAY_BLUE);
      display.setCursor(buttonX + 20, buttonY + 15);
      display.print(keys[row][col]);
    }
  }
  display.setCursor(180, 360);
}

void Confirm_Leg_Screen() {
  display.fillScreen(BLACK);
  display.drawRect(400, 200, 300, 100, DISPLAY_BLUE);
  display.setCursor(420, 225);
  display.print("Confirm Leg");
  
  display.drawRect(80, 200, 300, 100, DISPLAY_BLUE);
  display.setCursor(100, 225);
  display.print("Back");

  display.setCursor (20, 50);
  display.print("Leg: ");
  display.print(numLegs+1);

  display.setCursor(150, 120);
  display.print("Hrs: ");
  display.print(hrs);
  display.print(" Mins: ");
  display.print(mins);
  display.print(" Secs: ");
  display.print(secs);
}

//FUNCTION FOR SCREEN TO CONFIRM STOPPING TIMER
void Confirm_Stop_Timer_Screen() {
  display.fillScreen(BLACK);
  display.setCursor(300, 120);
  display.print("Press again to confirm");

  display.setCursor(420, 225);
  display.fillRect(400, 200, 300, 100, 0xFFFF);
  display.print("Stop Timer");

  display.fillRect(250, 350, 300, 100, 0xFFFF);
  display.setCursor(270, 370);
  display.print("Reset");

  display.fillRect(80, 200, 300, 100, 0XFFFF);
  display.setCursor(100, 225);
  display.print("Back");
}
//FUNCTION TO CREATE AND RETURN A LEG
struct Leg New_Leg(int total_time, double total_time_left, struct Leg* next_leg, struct Leg* previous_leg) {
  struct Leg leg;
  leg.leg_time = total_time;
  leg.leg_time_left = total_time_left;
  leg.next_leg = next_leg;
  leg.previous_leg = previous_leg;
  numLegs++;
  return leg;
}
//FUNCTION TO HANDLE KEYPAD INPUTS ON THE ADD LEG SCREEN
void Time_Entry(char val, int unit) {
  int num = val - '0';
  switch (unit) {
    case 1:
    //CLEAR THE BOX
      //display.setCursor(20, 360);
      //display.print(" ");
    //ADD THE DIGIT TO HOURS
    display.drawRect(20, 350, 50, 60, DISPLAY_BLUE);
    display.drawRect(170, 350, 50, 60, 0xffff);
    display.drawRect(320, 350, 50, 60, 0xffff);
      hrs = hrs * 10;
      hrs += num;
      if (hrs > 100) {
        hrs = 0;
        display.setCursor(20, 360);
        display.print("  ");
        display.setCursor(20, 360);
      }
      else {
        display.print(num);
      }
      
    //PRINT NEW NUMBER
      //display.setCursor(20, 360);
      break;
    case 2:
      //display.setCursor(170, 360);
      display.drawRect(170, 350, 50, 60, DISPLAY_BLUE);
      display.drawRect(20, 350, 50, 60, 0xffff);
      display.drawRect(320, 350, 50, 60, 0xffff);
      mins *= 10;
      mins += num;
      if (mins > 60) {
        mins = 0;
        display.setCursor(170, 360);
        display.print("  ");
        display.setCursor(170, 360);
      }
      else {
        display.print(num);
      }
      break;
    case 3:
      //display.setCursor(320, 350);
      display.drawRect(320, 350, 50, 60, DISPLAY_BLUE);
      display.drawRect(170, 350, 50, 60, 0xffff);
      display.drawRect(20, 350, 50, 60, 0xffff);
      secs *= 10;
      secs += num;
      if (secs > 60) {
        secs = 0;
        display.setCursor(320, 360);
        display.print("  ");
        display.setCursor(320, 360);
      }
      else {
        display.print(num);
      }
      break;
  }
}

//FUNCTION TO FIND THE LAST LEG OF THE LINKED LISTS
struct Leg* Find_Last_Leg(struct Leg* leg) {
  if ((leg->next_leg) != NULL) {
    return Find_Last_Leg(leg->next_leg);
  }
  else {
    return leg;
  }
}

//FUNCTION TO DO SPEED CHECK
void Add_Distance_Screen() {

  display.setTextColor(0xffff, BLACK);
  display.fillScreen(BLACK);
  display.setCursor(20, 20);
  display.setTextSize(4);
  display.print("Ground Speed Check");

  display.setTextSize(3);
  display.setCursor(20, 120);
  display.print("How much distance is left? ");

  display.setTextSize(4);
  display.drawRect(170, 350, 70, 60, 0xffff);
  display.setCursor(180, 360);
  display.print("0");
  
  display.setCursor(250, 360);
  display.print(".");
  display.drawRect(270, 350, 70, 60, 0xffff);
  display.setCursor(350, 360);
  display.print("NM");

  display.setTextSize(2);
  display.drawRect(500, 360, 300, 100, DISPLAY_BLUE);
  display.setCursor(520, 385);
  display.print("Finish Adding Distance");

  display.setTextSize(4);
  display.drawRect(400, 20, 150, 80, DISPLAY_BLUE);
  display.setCursor(420, 30);
  display.print("Back");

  for (int row = 0; row < ROWS; row++) {
    for (int col = 0; col < COLS; col++) {
      int buttonX = BUTTON_START_X + (BUTTON_WIDTH + BUTTON_MARGIN) * col;
      int buttonY = BUTTON_START_Y + (BUTTON_HEIGHT + BUTTON_MARGIN) * row;
      
      display.fillRect(buttonX, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT, DISPLAY_BLUE);
      display.setCursor(buttonX + 20, buttonY + 15);
      display.print(keys[row][col]);
    }
  }
  display.setCursor(180, 360);
}

void Distance_Entry(int num, int unit) {
  num = num - '0';
  switch (unit) {
    case 1:
      display.drawRect(170, 350, 70, 60, DISPLAY_BLUE);
      distance *= 10;
      distance += num;
      if (distance > 200) {
        distance = 0;
        display.setCursor(180, 360);
        display.print("  ");
        display.setCursor(180, 360);
      }
      else {
        display.print(num);
      }
      break;
    case 2:
      display.drawRect(270, 350, 70, 60, DISPLAY_BLUE);
      distance_decimal *=10;
      distance_decimal += num;
      if (distance_decimal > 9) {
        distance_decimal = 0;
        display.setCursor(280, 360);
        display.print(" ");
        display.setCursor(280, 360);
      }
      else {
        display.print(num);
      }
      break;
  }
}

void Speed_Clear() {
  for (int i = 0; i < 5; i++) {
    speed[i] = 0;
    speed_flown[i] = 0;
  }
  distance_at_last_speed_check = current_leg->leg_distance;
  distance_at_last_speed_check_decimal = current_leg->leg_distance_decimal;
  time_at_last_speed_check = Get_Current_Sum_Time();
  //speed[0] = (current_leg->leg_distance + (current_leg->leg_distance_decimal / 10.0)) / ((current_leg->leg_time/60.0)/60.0);
  speed_index = 0;
}

void Ground_Speed_Push_Back() {
  if (speed_index > 3) {
    for (int i = 1; i < 4; i++) {
      speed[i-1] = speed[i];
      speed_flown[i-1] = speed_flown[i];
    }
    speed[3] = (distance + (distance_decimal / 10.0)) / ((time_at_speed_check/60.0)/60.0);
    speed_flown[3] = ((distance_at_last_speed_check + (distance_at_last_speed_check_decimal / 10.0)) - ((distance + (distance_decimal / 10.0)))) / (((time_at_last_speed_check - time_at_speed_check)/60.0)/60.0);
    // ((current_leg->leg_distance + (current_leg->leg_distance_decimal / 10.0)) - (distance + (distance_decimal / 10.0))) / (((Get_Current_Total_Time() - time_at_speed_check)/60.0)/60.0);
    //((current_leg->leg_distance + (current_leg->leg_distance_decimal/10.0)) - (distance + (distance_decimal / 10.0))) / (((current_leg->leg_time - current_leg->leg_time_left)/60.0)/60.0);
  }
  else {
    speed[speed_index] = (distance + (distance_decimal / 10.0)) / ((time_at_speed_check/60.0)/60.0);
    speed_flown[speed_index] = ((distance_at_last_speed_check + (distance_at_last_speed_check_decimal / 10.0)) - ((distance + (distance_decimal / 10.0)))) / (((time_at_last_speed_check - time_at_speed_check)/60.0)/60.0);
    // ((current_leg->leg_distance + (current_leg->leg_distance_decimal / 10.0)) - (distance + (distance_decimal / 10.0))) / (((Get_Current_Total_Time() - time_at_speed_check)/60.0)/60.0);
    //((current_leg->leg_distance + (current_leg->leg_distance_decimal/10.0)) - (distance + (distance_decimal / 10.0))) / (((current_leg->leg_time - current_leg->leg_time_left)/60.0)/60.0);
  }
  distance_at_last_speed_check = distance;
  distance_at_last_speed_check_decimal = distance_decimal;
  time_at_last_speed_check = time_at_speed_check;
  speed_index++;
}

//SCREEN TO WAIT FOR GPS LOCK
void GPS_Active_Screen() {
  display.setTextColor(0xffff, BLACK);
  display.fillScreen(BLACK);
  display.setCursor(200, 200);
  display.setTextSize(3);

  display.print("Waiting for GPS Lock...");

  GPS_Active();
}

//FUNCTION TO WAIT FOR GPS LOCK BEFORE BEGINNING PROGRAM
void GPS_Active() {
  display.print(fix.satellites);
  while (!GPS_Fix()) {
    while (millis() - sec_passed < 1000) {

    }
    Check_GPS();
    display.print(" ");
    display.print(fix.satellites);
    //Serial.print(" ");
    //Serial.print(fix.satellites);
    sec_passed = millis();
  }
}

//Function mainly for the ground speed check, gets the total time for the current leg
int Get_Current_Sum_Time() {
  int total_time = 0;
  struct Leg* tempLeg = first_leg_ptr;
  for (int i = 0; i < num_of_current_leg; i++) {
    total_time += tempLeg->leg_time_left;
    tempLeg = tempLeg->next_leg;
  }
  return total_time;
}

int Get_Current_Total_Time() {
  int total_leg_time = 0;
  struct Leg* tempLeg = first_leg_ptr;
  for (int i = 0; i < num_of_current_leg; i++) {
    total_leg_time += tempLeg->leg_time;
    tempLeg = tempLeg->next_leg;
  }
  return total_leg_time;
}

double Get_Current_Sum_Distance() {
  double total_distance = 0;
  struct Leg* tempLeg = first_leg_ptr;
  for (int i = 0; i < num_of_current_leg; i++) {
    total_distance += tempLeg->leg_distance + (tempLeg->leg_distance_decimal / 10.0);
    tempLeg = tempLeg->next_leg;
  }
  return total_distance;
}
/*
double Get_Speed_Flown() {
  total distance - distance left to go = distance traveled
  total time - time left = time taken
  distance traveled / time taken = avg speed
}*/

//FUNCTION FOR SCREEN TO CONFIRM REMOVING A LEG
void Confirm_Remove_Leg_Screen() {
  display.fillScreen(BLACK);
  display.setCursor(300, 120);
  display.print("Press again to confirm");

  display.setCursor(420, 225);
  display.fillRect(400, 200, 300, 100, 0xFFFF);
  display.print("Remove Leg");

  display.fillRect(80, 200, 300, 100, 0XFFFF);
  display.setCursor(100, 225);
  display.print("Back");
}