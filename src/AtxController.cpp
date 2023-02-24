#include "AtxController.h"
#include "ButtonEvent.h"

AtxController* AtxController::singleton = nullptr;

AtxController::AtxController() {
	if (AtxController::singleton == nullptr) {
		AtxController::singleton = this;
	}
	_currentState = SystemState::OFF;
	// _buttonState = HIGH;
	// _lastButtonState = HIGH;
	// _lastDebounceTime = 0;
	_mutex = nullptr;
}

AtxController::~AtxController() {
	if (_mutex) {
		end();
	}

	// TODO Since we're a singleton, we should probably dispose our own instance here?
}

void onButtonDown(ButtonInformation *sender) {
	Serial.println(F("DEBUG: Button down!"));
	// TODO we probably don't need this one.
}

void onButtonUp(ButtonInformation *sender) {
	Serial.println(F("DEBUG: Button up!"));
	if (AtxController::singleton->getState() == SystemState::OFF) {
		Serial.println(F("DEBUG: Powering up!"));
		digitalWrite(PIN_PS_ON, LOW);
	}
}

void onButtonHold(ButtonInformation *sender) {
	Serial.println(F("DEBUG: Button hold!"));
	if (AtxController::singleton->getState() != SystemState::OFF) {
		Serial.println(F("DEBUG: Dropping power!"));
		digitalWrite(PIN_PS_ON, HIGH);
	}
}

void AtxController::begin() {
	pinMode(PIN_PS_ON, OUTPUT);
	digitalWrite(PIN_PS_ON, HIGH);
	pinMode(PIN_IO_EX_ON, OUTPUT);
	digitalWrite(PIN_IO_EX_ON, LOW);
	pinMode(PIN_RUN, OUTPUT);
	digitalWrite(PIN_RUN, LOW);
	pinMode(PIN_PWR_OK, INPUT);
	//pinMode(PIN_PWR_BTN, INPUT_PULLUP);
	ButtonEvent.addButton(PIN_PWR_BTN, onButtonDown, onButtonUp, onButtonHold, PWR_OFF_DELAY, NULL, 0);
	_mutex = xSemaphoreCreateMutex();
}

void AtxController::end() {
	vSemaphoreDelete(_mutex);
	digitalWrite(PIN_IO_EX_ON, LOW);
	digitalWrite(PIN_RUN, LOW);
	// TODO should we go ahead drive PIN_PS_ON high and drop ATX power?
	_mutex = nullptr;
	_currentState = SystemState::OFF;
	// _buttonState = HIGH;
	// _lastButtonState = HIGH;
	// _lastDebounceTime = 0;
	// TODO should we delete the atxTask too?
	// As long as it is running, loop() will continue to get called. Luckily, we guard against that, but still.
}

void AtxController::loop() {
	if (!_mutex) {
		return;
	}

	xSemaphoreTake(_mutex, portMAX_DELAY);
	
	// int powerBtnRaw = digitalRead(PIN_PWR_BTN);
	// if (powerBtnRaw != _lastButtonState) {
	// 	_lastDebounceTime = millis();
	// }

	// unsigned long checkDiff = millis() - _lastDebounceTime;
	// if (checkDiff > DEBOUNCE_DELAY) {
	// 	_buttonState = powerBtnRaw;
	// 	if (_buttonState == HIGH) {
	// 		// Button was pressed and then released.
	// 		if (_currentState != SystemState::ON) {
	// 			// Turn power on.
	// 			digitalWrite(PIN_PS_ON, LOW);
	// 			_currentState = SystemState::INIT;
	// 		}
	// 	}
	// 	else {
	// 		// Button is being held
	// 		if (checkDiff > PWR_OFF_DELAY && _currentState != SystemState::OFF) {
	// 			// Drop power.
	// 			digitalWrite(PIN_PS_ON, HIGH);
	// 		}
	// 	}
	// }

	// _lastButtonState = powerBtnRaw;
	
	int pwrOkState = digitalRead(PIN_PWR_OK);
	if (pwrOkState != _lastPwrOkState) {
		// We changed power states.
		if (pwrOkState == HIGH && _currentState != SystemState::ON) {
			// We have full ATX power.
			_currentState = SystemState::ON;
		}
		else if (pwrOkState == LOW && _currentState == SystemState::ON) {
			_currentState = SystemState::OFF;
		}
	}

	_lastPwrOkState = pwrOkState;
	switch (_currentState) {
		case SystemState::ON:
			if (systemPowerOn != NULL) {
				systemPowerOn();
			}
			break;
		case SystemState::OFF:
			if (systemPowerOff != NULL) {
				systemPowerOff();
			}
			break;
		case SystemState::INIT:
			if (systemPowerInit != NULL) {
				systemPowerInit();
			}
			break;
		default:
			break;
	}

	xSemaphoreGive(_mutex);
	ButtonEvent.loop();
}

SystemState AtxController::getState() {
	return _currentState;
}

void AtxController::onPowerOn(void (*systemPowerOn)()) {
	this->systemPowerOn = systemPowerOn;
}

void AtxController::onPowerOff(void (*systemPowerOff)()) {
	this->systemPowerOff = systemPowerOff;
}

void AtxController::onPowerInit(void (*systemPowerInit)()) {
	this->systemPowerInit = systemPowerInit;
}

void AtxController::signalInit() {
	digitalWrite(PIN_IO_EX_ON, HIGH);
	digitalWrite(PIN_RUN, HIGH);
}