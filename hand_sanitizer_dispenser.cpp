/*
* Author: Miren Patel
* Purpose: To design a prototype of a touchless hand sanitizer dispenser using an ultra-sonic sensor along with a servo motor
* Extra Modules/ Functions in the File: Watchdog Reset Function which resets the watchdog timer
* Assignment: Project 3
********************************************
* Inputs: Waving of an object in front, in the appropriate range, of the ultra-sonic sensor 
* Outputs: Triggering the ultrasonic distance sensor that will signal the servo motor to turn on which will ultimately result into dispensing of the hand sanitizer 
           Green LED light is to indicate when the prototype is dispensing the sanitizer and Red LED light is to indicate when the prototype is not dispensing the sanitize. 
           In other words, whenever the system is in idle state it will have the Red LED turned on. 
           Whenever the system turns on, the red LED will turn off and the green LED will turn on. 
           After the dispensing, the system will go back to the idle state unless triggered. 

* Constraints: 
    • The dispenser cannot dispense sanitizer if the object, in front of the sensor, is farther than 3 inches.
    • It is not as fast as traditional automatic sanitizer dispensers.
    • Based on the design, only one specific hand sanitizer bottle would work.
    • The current design does not exert enough pressure for a full pump of the sanitizer so multiple dispenses might be needed to get enough sanitizer for one use.
********************************************
* References: UM2179 User Manual 
              RM0432 Reference Manual
              CSE 321 Lecture Slides and example .cpp files
              MBed Documentation and Website
              Datasheets are linked adjacent to code
 */

// Link to headers and other files
#include "mbed.h"

// Creating the Watchdog that resets the whole system every 30 seconds
Watchdog &watchMe = Watchdog::get_instance();

// For when the board button is pressed
InterruptIn pressed(BUTTON1); 

// Timer for controlling the delay
#define wdTimeout 30000 //Total of 30 seconds

// Initialization Thread and Queue
Thread t;
EventQueue e(32*EVENTS_EVENT_SIZE);

// Prototype for ISR
void reset();

// Setting up the Ultrasonic Distance Sensor (HC-SR04)
DigitalOut trigger(PB_8); //PB 8 is the Trigger
DigitalIn  echo(PB_9); //PB 9 is the Echo
int distance_measured = 0; //Setting up the variable that holds  the distance that is measured in inches
int overhead_timer_delay = 0; //There is a overhead timer delay that needs to be subtracted in order to get the accurate distance measured
Timer UDS_Timer; //Initializing the timer for the Ultrasonic Distance Sensor

// Setting up the Servo Motor (SG90)
PwmOut myServoMotor(PA_7); //Pulse Width Modification (PWM) is coming from pin PA 7
 
int main(){
    watchMe.start(wdTimeout); //Starting the Watchdog Timeout
    printf("Reset\n"); //Everytime the system resets, it prints out "Reset" which can be used to determine the functionality of the Watchdog Timer
    t.start(callback(&e, &EventQueue::dispatch_forever)); //Dispatch Forever
    pressed.rise(e.event(reset)); //Pressing the button calls the reset function

    // Enabling clock - E
    RCC->AHB2ENR |= 0x10;

    //Clock E
    GPIOE->MODER &= ~(0x3F30); // Port E will be used as Output
    GPIOE->MODER |= 0x0110; // (PE2 and PE4 are Outputs) These will used for Red (PE4) and Green(PE2) LEDs

    //Code to operate the Ultrasonic Sensor
    //This website was used for some of the code and the implementation for HC-SR04: https://os.mbed.com/components/HC-SR04/
    
    //This part of the code calculates the timer delay for the Ultrasonic Sensor which will be used to calculate the accurate distance
    UDS_Timer.reset(); //Reseting the Ultrasonic Sensor Timer
    UDS_Timer.start(); //Starting the Ultrasonic Sensor Timer
    while(echo == 2){}; //Minimum software polling delay is used to read the echo pin
    UDS_Timer.stop(); //Stoping the Ultrasonic Sensor Timer
    overhead_timer_delay = UDS_Timer.read_us(); //This gives us the overhead timer delay in microseconds
    // printf("%d uS\n\r", overhead_timer_delay); //Prints out the overhead timer delay. Only used for testing

    //This part of the code calculates the distance measured using the Ultrasonic Distance Sensor
    //Loop to read Sonar distance values, scale, and print
    while(true) { //Looping to read the distance
    // trigger sonar to send a ping
        trigger = 1; //Trigger the sensor and ask it to start collecting data
        UDS_Timer.reset(); //Reseting the Ultrasonic Sensor Timer
        wait_us(10); //Wait for 10 microseconds
        trigger = 0; //"Untrigger" the sensor and ask it to stop collecting data

        //Echo = 0 High and Echo = 1 is low
        while (echo==0) {}; //While the echo is high, start the Ultrasonic Sensor Timer
        UDS_Timer.start();

        while (echo==1) {}; //While the echo is low, stop the Ultrasonic Sensor Timer
        UDS_Timer.stop();

        //Considering the overhead timer delay to calculate the correct distance
        int difference = UDS_Timer.read_us()-overhead_timer_delay; //Calculating the difference between the read value and overhead timer delay
        distance_measured = difference/148.0; //According to the MBed website, to obtain the answer in inches, the difference is divided by 148

        //At this point, the system is in idle state because it is not doing anything
        //Well, the ultrasonic distance sensor is still measuring the distances constantly
        GPIOE->ODR |= 0x10; //Since it is in idle state, Red LED is turned ON
        GPIOE->ODR &= ~(0x4); //Green LED is turned OFF

        //As soon as the distance is less than 3 inches, the system is turned on and is in active state
        //In other words, it is time to trigger the servo motor
        if (distance_measured <= 3){
            printf("%d inches \n\r", distance_measured);
            GPIOE->ODR &= ~(0x10); //Since it is in active state, Red LED is turned OFF
            GPIOE->ODR |= 0x4; //Green LED is turned ON

            //Code to operate the Servo motor 
            //This website was used for the implementation for SG90 Servo Motor: https://os.mbed.com/components/RC-Servo/

            myServoMotor.period_ms(20); //Setting the period of the servo as mentioned in the SG90 Servo Motor datasheet.
            // Link for the datasheet: https://datasheetspdf.com/pdf-file/791970/TowerPro/SG90/1

            //This makes the servo rotate from 12 o' clock position to 6 o' clock position. In other words, 180 degrees 
            for(int i = 2500; i>=500; i = i - 2000){ 
                myServoMotor.pulsewidth_us(i); //The pulsewidth_us values were determined based on trial and error
                wait_us(500000); //Waiting for 0.5 seconds. The wait_us values were also determined based on trial and error
            }

            //This makes the servo rotate from 6 o' clock position to 12 o' clock position. In other words, 180 degrees
            //This brings the servo motor back to its initial position after one pump of sanitizer is dispensed
            for(int i = 500; i<=2500; i = i + 2000){ 
                myServoMotor.pulsewidth_us(i); //The pulsewidth_us values were determined based on trial and error
                wait_us(500000); //Waiting for 0.5 seconds. The wait_us values were also determined based on trial and error
            }
            //This wait_us value was determined based on trial and error.
            wait_us(1000000); //After every pump, it waits for 1 seconds before the second pump
        }
    }
    return 0; //Reduce any memory leaks
}

void reset(){ //Function to reset the Watchdog
    watchMe.kick(); //Reset the watchdog
}