#include <Adafruit_TCS34725.h>
#include <Adafruit_NeoPixel.h>
#include <CurieBLE.h>

#define NUMPIXELS      7
#define LED_DATA_PIN   6
#define BUTTON_PIN     2
#define BLE_CONNECTED  13

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);

// eye-recognized gamma color
byte gammatable[256];
uint32_t currentColor;

BLEPeripheral blePeripheral;
BLEService bleService("a56ada00-ed09-11e5-9c97-0002a5d5c51b");

BLECharacteristic currentColorChar("a56ada04-ed09-11e5-9c97-0002a5d5c51b", BLERead | BLENotify, 12);

void setup() {

  //while (!Serial); // arduino 101: wait for Serial to be init

  Serial.begin(9600);
  Serial.println("Color Picker Running!");

  pixels.begin();

  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1); // halt!
  }

  pinMode(BUTTON_PIN, INPUT);
  
  // thanks PhilB for this gamma table!
  // it helps convert RGB colors to what humans see
  for (int i=0; i<256; i++) {
    float x = i;
    x /= 255;
    x = pow(x, 2.5);
    x *= 255;

    gammatable[i] = x;
    //Serial.println(gammatable[i]);
  }


  blePeripheral.setLocalName("Arduino101");
  blePeripheral.setAdvertisedServiceUuid(bleService.uuid());

  blePeripheral.addAttribute(bleService);
  blePeripheral.addAttribute(currentColorChar);

  currentColor = 0;

  setCurrentColor();
  
  blePeripheral.begin();
  digitalWrite(BLE_CONNECTED, LOW);

}


void loop() {
  
  uint16_t clear, red, green, blue;
  BLECentral bleCentral = blePeripheral.central();
  if(bleCentral && bleCentral.connected()){
    digitalWrite(BLE_CONNECTED, HIGH);  
  }
  else{
    digitalWrite(BLE_CONNECTED, LOW);  
  }

  if (digitalRead(BUTTON_PIN) == HIGH){

    delay(60);  // takes 50ms to read
    tcs.getRawData(&red, &green, &blue, &clear);
    
    // Figure out some basic hex code for visualization
    uint32_t sum = clear;
    float r, g, b;
    r = red; r /= sum;
    g = green; g /= sum;
    b = blue; b /= sum;
    r *= 256; g *= 256; b *= 256;
  
    /*Serial.print("\tR:\t"); Serial.print(gammatable[(int)r]);
    Serial.print("\tG:\t"); Serial.print(gammatable[(int)g]);
    Serial.print("\tB:\t"); Serial.print(gammatable[(int)b]);
    
    Serial.print("\t");
    Serial.print((int)r, HEX); Serial.print((int)g, HEX); Serial.print((int)b, HEX);
    Serial.println();*/
    currentColor = pixels.Color(gammatable[(int)r],gammatable[(int)g],gammatable[(int)b]);      
    setCurrentColor();
    for(int i=0;i<NUMPIXELS;i++){
  
      //pixels.setPixelColor(i, pixels.Color(r,g,b)); // uncorrected rgb values
      pixels.setPixelColor(i, currentColor);

      pixels.show(); // This sends the updated pixel color to the hardware.
      
    }    
  }
}

void setCurrentColor(){
  unsigned char bytes[] = {(currentColor & 0xFF0000) >> 16, (currentColor & 0x00FF00) >> 8, currentColor & 0x0000FF};  
  currentColorChar.setValue(bytes, 3);
}

