#include <Servo.h>
#include <DHT.h> // required library for DHT humidity sensor 

// Pin Definitions 
// Assigning pins to variable makes the code easier to update if you move a wire 
#define DHTPIN 4 // Data pin for the DHT sensor
#define DHTTYPE DHT11 

const int trigPin = 9; // sends out ultra sonic ping 
const int echoPin = 10; // Listens for the to bounce back 
const int servoPin = 11; // Signal wire for the servo motor
const int leftBtn = 2; // Input pin for the decrease angle 
const int rightBtn = 3; // Input pin for the increase angle 

// Object Initialization
DHT dht(DHTPIN, DHTTYPE);
Servo myServo; // create the servo object to control the motor
int servoPos = 90; // Initial position variable 

void setup() {
  // put your setup code here, to run once:
  // start serial communication at 9600 bits per second 
  // has to match python baud rate 
  Serial.begin(9600);

  dht.begin(); // start dht sensor

  // Define pin modes sensors are inputs actuators are outputs 
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // INPUT_PULLUP uses arduino internal resistor to keep the pin high 
  // until the button is pressed, which pulls it to LOW (GND)
  pinMode(leftBtn, INPUT_PULLUP);
  pinMode(rightBtn, INPUT_PULLUP);

  myServo.attach(servoPin); // Connect the servo object to physical pin 11
  myServo.write(servoPos); // move servo to the starting position 
  
}

void loop() {
  // put your main code here, to run repeatedly:
  // ultrasonic distance calculation 
  // clear the trigPin, then trigger a 10-microsecond pule
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Masure the duration of the return pulse in microsecnonds 
  long duration = pulseIn(echoPin, HIGH);
  // convert time into distance (cm) based on the speed of sound 
  float distance = duration * 0.034 / 2;

  // humidity 
  float humidity = dht.readHumidity();

  // check if sensor is disconnected or failing NAN 
  // we set it to 0.0 so the python script doesnt crash during string spliting 
  if (isnan(humidity)) {
    humidity = 0.0;
  }
  // Transform: Handle Manual Inputs and Servo Logic
  String btnStatus = "None"; // default status if no buttons are pressed 
  
  // If the pin is LOW  the button is being pressed (due to INPUT_PULLUP)
  if (digitalRead(leftBtn) == LOW ) {
    servoPos = max(0, servoPos -5); // subtract 5 degrees but dont go below zero 
    btnStatus = "Left";
    delay(50); // small 'debounce' delay to prevent multiple inputs per press 

  } else if (digitalRead(rightBtn) == LOW) {
    servoPos = min(180, servoPos + 5); // add 5 degrees
    btnStatus = "Right";
    delay(50);
  }
  myServo.write(servoPos); // move the physical motor to the updated position 
  // load and transmit data string to python 
  // send commma seperated values (csv) string that python can split 
  // Final string looks like: "25.4,55.0,90,Left"
  Serial.print(distance); // Field 0; Distance
  Serial.print(","); // Separator
  Serial.print(humidity); // separator
  Serial.print(",");
  Serial.print(servoPos); // field servo angle
  Serial.print(","); // separator
  Serial.println(btnStatus); // button press 

  // wait 2 seconds between logs DHT sensors need time stablize 
  delay(2000);


}
