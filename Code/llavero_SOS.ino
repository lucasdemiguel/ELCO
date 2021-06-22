#include "BluetoothSerial.h"
#include <Arduino.h>
#include <esp_bt.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;

// GPIO34 wakeup ESP32 chip

// constants won't change. They're used here to set pin numbers:
const int buttonPin = 34;    // the number of the pushbutton pin
const int ledPin = 2;      // the number of the LED pin
const int ANALOGPILA = 35;


//To edit:
int num = 5;
int deadline = 10000; //en ms

// Variables:
int ledState = HIGH;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
int numeroPulsaciones = num;

unsigned long tiempo_actual = 0;
unsigned long tiempo_aux = 0;
unsigned long tiempo_sleep = 0;
unsigned long tiempo_sleep_aux = 0;
unsigned long tiempo_bat = 0;
unsigned long tiempo_bat_aux = 0;

double analogValor = 0;
double voltaje = 0;
double conv_factor = 0.000805; //3.3V/4095
String infoBateria;
// Umbrales
double maximo = 3;
double alto = 2.25;
double medio = 1.5;
double bajo = 0.75;
double minimo = 0.1;

int Alarm_COD = 1;
int Bat_COD = 2;


// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(ANALOGPILA, INPUT);

  // set initial LED state
  digitalWrite(ledPin, ledState);

  //set BT
  SerialBT.begin("Keepy BT"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
}

void loop() {
  pulsador();
  
  //Deep Sleep
  deepSleep();

  //Battery level
  battery();
}

void newPulse() {
  tiempo_actual = millis();
  if (((tiempo_actual - tiempo_aux) > 2*deadline)) { //si pasan 20s se resetea el numero de pulsaciones
    tiempo_aux = tiempo_actual;
    numeroPulsaciones = num;
  } else {
    Serial.println(numeroPulsaciones);
    numeroPulsaciones--;
    if (numeroPulsaciones < 1) {       
      enviarMensaje(Alarm_COD);
      numeroPulsaciones=num;
    }
  }
}

void enviarMensaje(int code) {
  if(code==Alarm_COD){   
    SerialBT.print("10"); 
    Serial.println("ALARMA"); 
    //SerialBT.println("ALARMA"); 
  } 
  else if(code==Bat_COD){   
    /*Serial.print("Voltaje: ");
    Serial.print(voltaje);
    Serial.print(" V -> ");*/
    Serial.println(infoBateria);
    
    /*SerialBT.print("Voltaje: ");
    SerialBT.print(voltaje);
    SerialBT.print(" V -> ");*/
    SerialBT.println(infoBateria); 
  }
}

void pulsador(){
  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;      
      if (buttonState == HIGH) {        
        newPulse();
      }
    }
  }

  // set the LED:
  ledState = buttonState;
  digitalWrite(ledPin, ledState);

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
}

//Function that prints the reason by which ESP32 has been awaken from sleep
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}

void deepSleep(){
  tiempo_sleep = millis();
  if (((tiempo_sleep - tiempo_sleep_aux) > 180*deadline)){
    tiempo_sleep_aux = tiempo_sleep;
    
    //Print the wakeup reason for ESP32
    print_wakeup_reason();

    //turn off BT
    //esp_bt_controller_disable();
    //Configure the wake up source
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_34,1); //1 = High, 0 = Low
    //Go to sleep now
    Serial.println("Going to sleep now");
    delay(1000);
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
  } 
}

void battery(){
  tiempo_bat = millis();
  if (((tiempo_bat - tiempo_bat_aux) > 3*deadline)){
    tiempo_bat_aux = tiempo_bat;
    // Leemos valor de la entrada analógica
    analogValor = analogRead(ANALOGPILA); 
    // Obtenemos el voltaje
    voltaje = conv_factor * analogValor;
 
    if (voltaje >= maximo){
      infoBateria = "70";//Nivel de carga Maximo
    }else if (voltaje < maximo && voltaje > alto){
      infoBateria = "60";//Nivel de carga Alto
    }else if (voltaje < alto && voltaje > medio){
      infoBateria = "50";//Nivel de carga Medio
    }else if (voltaje < medio && voltaje > bajo){
      infoBateria = "40";//Nivel de carga Bajo
    }else if (voltaje < bajo && voltaje > minimo){
      infoBateria = "30";//Nivel de carga al Mínimo, Cargue el dispositivo
    }else{
      infoBateria = "20"; //Batería agotada, Cargue el dispositivo
    }
  
    enviarMensaje(Bat_COD);
  }
}
