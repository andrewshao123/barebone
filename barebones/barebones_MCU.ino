#include <Servo.h>
#include <Wire.h> //I2C Arduino Library
#define address 0x1E //0011110b, I2C 7bit address of HMC5883 compass

float newData[200]; //We will know the size of the float array

int start = 0; //start keeping track of next float in string to be parsed
int data_index = 0; //data_index keeps track of which index to place float in float array

float verticalSpeed = 0; //forward/backward, joystick element 1
float horizontalSpeed = 0; //right/left; joystick element 0
float rotationSpeed = 0; //rotate right/left about centre axis; joystick element 3
float dampingFactor = 1; //Damping factor to control speed range
float forw_backFactor = 1; //Dampen either forward or backward wheels to make forward & 
                           //backward motors move at same speed; APPLY TO FASTER MOTOR PAIR

//**********PARAMETERS TO BE SET AT BEGINNING**********
float armSpeed = 1;
float clawSpeed = 1;
float sliderSpeed = 1;
int pincerSpeed = 1;
int shovelSpeed = 1;

Servo myservo1;  //Front left wheel
Servo myservo2;  //Back left wheel
Servo myservo3; //Front right wheel
Servo myservo4; //Back right wheel

void setup(){
  
  //6 is right back wheel, 7 is left back wheel, 8 is right front wheel, 9 is left front wheel
  //PWM control wheels

  myservo1.attach(9); //Front left wheel
  //myservo2.attach(7); //Back left wheel
  myservo3.attach(8); //Front right wheel
  //myservo4.attach(6); //Back right wheel

  Serial.begin(9600); //Serial output to computer
  Serial1.begin(9600); //Serial communication with RPi
  //delay(100);
}

void loop(){
  //Process string and build float array
  if(Serial1.available() > 0){
    data_index = 0;
    start = 0;
    String string = Serial1.readStringUntil('\n');
    Serial.println(string);
    //String string = "0.5,1.0,0.1,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0"; //Test String
    //Serial.println(string);

    for(int i = 0; i < string.length(); i++){
      if(string.substring(i, i+1) == "," || i == string.length()-1){
        if (string.substring(i, i+1) == ","){ //Check for comma to denote end of float in string
          newData[data_index] = string.substring(start, i).toFloat();
        }
        else{ //Account for last float in string
          newData[data_index] = string.substring(start, string.length()).toFloat();
        }
        start = i+1;
        data_index++;
      }
    }
    
  horizontalSpeed = newData[0]; //Left/right joystick
  verticalSpeed = newData[1]; //Forward/backward joystick
  rotationSpeed = newData[2]; //Twist joystick
  dampingFactor = (newData[3]+1)/2; //Takes a number from [-1:1] and converts it to [0:1] to control speed range 
  }
  
  moveRover(verticalSpeed, horizontalSpeed, rotationSpeed);
}

//Takes number in range [-1:1] and converts to dampingFactor*[31:59]
int custom_map(float num){
  int adjustedSpeed;
  
  if (num >= 0){
    adjustedSpeed = 69*num*dampingFactor+90; 
  }
  else if (num <= 0){
    adjustedSpeed = 59*num*dampingFactor+90;
  }
  return adjustedSpeed;
}

void actuate_motor(Servo servo, float joystickSpeed){
  servo.write(custom_map(joystickSpeed));
}

void moveRover(float verticalSpeed, float horizontalSpeed, float rotationSpeed){
  float turningSpeed = diagonalSpeed(verticalSpeed, horizontalSpeed);
  
  if(horizontalSpeed != 0 || verticalSpeed != 0){  //Continuous forward/backward/turning movement
    if (horizontalSpeed > 0){  //first and fourth quadrant
      actuate_motor(myservo3, turningSpeed); //Front left wheel moves faster than right wheels
      actuate_motor(myservo4, turningSpeed); //Back left wheel moves faster than right wheels
      actuate_motor(myservo1, turningSpeed*(1-abs(horizontalSpeed))); //Right wheel moves slower wrt left wheel
      actuate_motor(myservo2, turningSpeed*(1-abs(horizontalSpeed))); //Right wheel moves slower wrt left wheel
    }
    else{  //second and third quadrant
      actuate_motor(myservo3, turningSpeed*(1-abs(horizontalSpeed))); //Left wheel moves slower wrt right wheel
      actuate_motor(myservo4, turningSpeed*(1-abs(horizontalSpeed))); //Left wheel moves slower wrt right wheel
      actuate_motor(myservo1, turningSpeed); //Right wheel moves faster
      actuate_motor(myservo2, turningSpeed); //Right wheel moves faster
    }
  }
  else{  //Rotate rover about centre axis; only rotate on the spot if the joystick doesn't go forward or backward
    actuate_motor(myservo1, rotationSpeed);
    actuate_motor(myservo2, rotationSpeed);
    actuate_motor(myservo3, -rotationSpeed);
    actuate_motor(myservo4, -rotationSpeed);
  }
}

float diagonalSpeed(float vy, float vx){
  float vz = sqrt(vx*vx+vy*vy);
  
  return (vy >=0 ? vz:-vz)/sqrt(2);  //Normalize speed for range [-1:1] and direction of vy
}
