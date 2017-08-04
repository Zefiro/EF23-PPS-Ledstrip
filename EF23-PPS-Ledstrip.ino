/*
 * (c) 2017 Zefiro
 */

#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>

#include "Bounce2.h"
Bounce bouncerPushButton = Bounce();
Bounce bouncerRotaryButton = Bounce();

#define LEDSTRIP_PIN 2

#define ROTARY_PUSH_PIN 13
#define ROTARY_A_PIN 14
#define ROTARY_B_PIN 12
#define ROTARY_DEBOUNCE_MS 3
#define ROTARY_PUSH_DEBOUNCE_MS 50

#define PUSH_BUTTON_PIN 4

#define NUM_LEDS 59

byte rotaryALast, rotaryBLast;
int rotaryPos = 30;
unsigned long rotaryALastMillis, rotaryBLastMillis;
unsigned long rotaryPushedMillis;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS + 1, LEDSTRIP_PIN, NEO_GRBW + NEO_KHZ800);

struct {
	byte id;
	byte param1;
	byte param2;
	byte param3;
	byte param4;
	byte brightness;
	byte mode;
} effect;


byte effectMaxMode, selectionMode;

// 0 = pushed (react on rise), 1 = pushed & saved to eeprom (ignore on rise)
byte selectionModeChangeState;

void setup() {
  pinMode(ROTARY_A_PIN, INPUT_PULLUP);
  pinMode(ROTARY_B_PIN, INPUT_PULLUP);
  pinMode(ROTARY_PUSH_PIN, INPUT_PULLUP);
  pinMode(PUSH_BUTTON_PIN, INPUT_PULLUP);
  rotaryALast = digitalRead(ROTARY_A_PIN);
  rotaryBLast = digitalRead(ROTARY_B_PIN);
  bouncerPushButton.attach(PUSH_BUTTON_PIN);
  bouncerRotaryButton.attach(ROTARY_PUSH_PIN);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  Serial.begin(115200);
  Serial.println("\nPPS 2017\n");
  EEPROM.begin(64);
  loadEEPROM();
}

short checkRotary() {
  short rotation = 0;
  // read rotary encoder
  unsigned long currentMillis = millis();
  byte rotaryACurrent = digitalRead(ROTARY_A_PIN);
  if (rotaryACurrent == rotaryALast) {
    rotaryALastMillis = currentMillis;
  } else if (rotaryALastMillis + ROTARY_DEBOUNCE_MS <= currentMillis) {
    rotaryALast = rotaryACurrent;
    rotaryALastMillis = currentMillis;
  }
  byte rotaryBCurrent = digitalRead(ROTARY_B_PIN);
  if (rotaryBCurrent == rotaryBLast) {
    rotaryBLastMillis = currentMillis;
  } else if (rotaryBLastMillis + ROTARY_DEBOUNCE_MS <= currentMillis) {
    rotaryBLast = rotaryBCurrent;
    rotaryBLastMillis = currentMillis;
    // trigger on flank (rotaryA already processed/debounced)
    if (rotaryBLast == HIGH) {
      if (rotaryALast == LOW) {
        rotation = -1;
      } else {
        rotation = 1;
      }
    }
  }
  return rotation;
}

void loop() {
    bouncerPushButton.update();
    bouncerRotaryButton.update();
    int rotation = checkRotary();
    if (rotation) {
      switch(selectionMode) {
        case 0:
          effect.id = constrain(effect.id + rotation, 0, 2);
          Serial.printf("Effect: %d\n", effect.id);
		  initEffect();
          break;
        case 1:
          effect.brightness = constrain(effect.brightness + rotation * 5, 0, 255);
          Serial.printf("Brightness: %d\n", effect.brightness);
          break;
        case 2:
          effect.param1 = constrain(effect.param1 + rotation, 0, 255);
          Serial.printf("Param1: %d\n", effect.param1);
          break;
        case 3:
          effect.param2 = constrain(effect.param2 + rotation, 0, 255);
          Serial.printf("Param2: %d\n", effect.param2);
          break;
        case 4:
          effect.param3 = constrain(effect.param3 + rotation, 0, 255);
          Serial.printf("Param3: %d\n", effect.param3);
          break;
        case 5:
          effect.param4 = constrain(effect.param4 + rotation, 0, 255);
          Serial.printf("Param4: %d\n", effect.param4);
          break;
      }
    }

    if (bouncerRotaryButton.fell()) {
		rotaryPushedMillis = millis();
		selectionModeChangeState = 0;
	}
	if (bouncerRotaryButton.read() == 0 && selectionModeChangeState == 0 && rotaryPushedMillis + 3000 < millis()) {
		selectionModeChangeState = 1;
		saveEEPROM();
	}
	if (bouncerRotaryButton.rose() && selectionModeChangeState == 0) {
        selectionMode = selectionMode < 5 ? selectionMode + 1 : 0;
		Serial.printf("selectionMode changed: %d\n", selectionMode);
    }
    if (bouncerPushButton.rose()) {
        effect.mode = effect.mode < effectMaxMode ? effect.mode + 1 : 0;
		Serial.printf("effectMode: %d (0..%d)\n", effect.mode, effectMaxMode);
    }

    calcSelectionColor();
    calcColors();
    strip.show();  
}

void saveEEPROM() {
	char signature[3] = "Z!";
	int addr = 0;
	EEPROM.put(addr, signature);
	addr += sizeof(signature);
	EEPROM.put(addr, sizeof(effect));
	addr += sizeof(sizeof(effect));
	EEPROM.put(addr, effect);
    EEPROM.commit();
	Serial.println("Saved to EEPROM");
}

void loadEEPROM() {
	char signature[3];
	size_t size;
	int addr = 0;
	EEPROM.get(addr, signature);
	addr += sizeof(signature);
	EEPROM.get(addr, size);
	addr += sizeof(size);
	if (strcmp(signature, "Z!") == 0 && size == sizeof(effect)) {
		EEPROM.get(addr, effect);
		initEffect();
		EEPROM.get(addr, effect);
		Serial.println("Loaded from EEPROM");
		Serial.printf("Id: %d (p1: %d, p2: %d) b=%d\n", effect.id, effect.param1, effect.param2, effect.brightness);
	} else {
		Serial.printf("EEPROM content invalid: '%s' (%d != %d)\n", signature, size, sizeof(effect));
	}
}

void initEffect() {
	switch(effect.id) {
		case 0:
			effectMaxMode = 0;
			break;
		case 1:
			effect.param1 = 10;
			effect.param2 = 20;
			effect.param3 = 10;
			effect.param4 = 0;
		    effect.brightness = 50;
			effectMaxMode = 1;
			break;
		case 2:
			effect.param1 = 0;
		    effect.brightness = 50;
			effectMaxMode = 1;
			break;
	}
}

void calcSelectionColor() {
  uint32_t c = 0;
  switch(selectionMode) {
    case 0:
      c = strip.Color(5, 0, 0, 0);
      break;
    case 1:
      c = strip.Color(0, 0, 0, 5);
      break;
    case 2:
    case 3:
    case 4:
    case 5:
      c = strip.Color(0, 5, 0, 0);
      break;
  }
  strip.setPixelColor(0, c);
}

void calcColors() {
  switch(effect.id) {
    case 0:
      fillColor(0);
      break;
    case 1:
	  effectAlarm();
      break;
	case 2:
	  effectCockpit();
	  break;
  }
}

void effectAlarm() {
	switch(effect.mode) {
		case 0:
			fillColor(0);
			break;
		case 1:
			uint32_t off = strip.Color(0, 0, 0, 0);
			uint32_t on = strip.Color(effect.brightness, 0, 0, constrain(effect.param4, 0, 16) * effect.brightness / 16);
			int shift = millis() * effect.param1 / 500;
			for(uint16_t i=1; i<strip.numPixels(); i++) {
    			bool isOn = (i + shift) % (effect.param2+1) < effect.param3;
				strip.setPixelColor(i, isOn ? on : off);
			}
			break;
	}
}

void effectCockpit() {
	switch(effect.mode) {
		case 0:
			fillColor(0);
			break;
		case 1:
		    effect.param1 = constrain(effect.param1, 0, 20);
			byte w = (int) effect.brightness * effect.param1 / 20;
			byte b = (int) effect.brightness * (20 - effect.param1) / 20;
			uint32_t c = strip.Color(0, 0, b, w);
			fillColor(c);
			break;
	}
}

void fillColor(uint32_t c) {
  for(uint16_t i=1; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
}
