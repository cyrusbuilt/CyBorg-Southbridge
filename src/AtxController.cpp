#include "AtxController.h"
#include "ButtonEvent.h"

AtxController* AtxController::singleton = nullptr;

AtxController::AtxController() {
	if (AtxController::singleton == nullptr) {
		AtxController::singleton = this;
	}
	_currentState = SystemState::OFF;
	_lastState = SystemState::OFF;
	_lastPwrOkState = LOW;
	_buttonWasDown = false;
	_mutex = nullptr;
}

AtxController::~AtxController() {
	if (_mutex) {
		end();
	}

	// TODO Since we're a singleton, we should probably dispose our own instance here?
}

void onButtonDown(ButtonInformation *sender) {
	//Serial.println(F("DEBUG: Button down!"));
	AtxController::singleton->setButtonWasDown(true);
}

void onButtonUp(ButtonInformation *sender) {
	//Serial.println(F("DEBUG: Button up!"));
	if (AtxController::singleton->getState() == SystemState::OFF
		&& AtxController::singleton->buttonWasDown()) {
		Serial.println(F("DEBUG: Powering up!"));
		digitalWrite(PIN_PS_ON, LOW);
	}

	AtxController::singleton->setButtonWasDown(false);
}

void onButtonHold(ButtonInformation *sender) {
	Serial.println(F("DEBUG: Button hold!"));
	if (AtxController::singleton->getState() != SystemState::OFF) {
		Serial.println(F("DEBUG: Dropping power!"));
		digitalWrite(PIN_PS_ON, HIGH);
	}
}

void AtxController::begin() {
	pinMode(PIN_LED_STANDBY, OUTPUT);
	digitalWrite(PIN_LED_STANDBY, HIGH);
	pinMode(PIN_PS_ON, OUTPUT);
	digitalWrite(PIN_PS_ON, HIGH);
	pinMode(PIN_IO_EX_ON, OUTPUT);
	digitalWrite(PIN_IO_EX_ON, LOW);
	pinMode(PIN_RUN, OUTPUT);
	digitalWrite(PIN_RUN, LOW);
	pinMode(PIN_PWR_OK, INPUT);
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
	_lastState = SystemState::OFF;
	_lastPwrOkState = LOW;
	_buttonWasDown = false;
	// TODO should we delete the atxTask too?
	// As long as it is running, loop() will continue to get called. Luckily, we guard against that, but still.
}

void AtxController::loop() {
	if (!_mutex) {
		return;
	}

	xSemaphoreTake(_mutex, portMAX_DELAY);
	ButtonEvent.loop();
	
	int pwrOkState = digitalRead(PIN_PWR_OK);
	if (digitalRead(PIN_PS_ON) == LOW && pwrOkState == LOW) {
		_currentState = SystemState::INIT;
	}

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
	if (_currentState != _lastState) {
		switch (_currentState) {
			case SystemState::ON:
				digitalWrite(PIN_LED_STANDBY, LOW);
				if (systemPowerOn != NULL) {
					systemPowerOn();
				}
				break;
			case SystemState::OFF:
				digitalWrite(PIN_LED_STANDBY, HIGH);
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
	}

	_lastState = _currentState;
	xSemaphoreGive(_mutex);
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

bool AtxController::buttonWasDown() {
	return _buttonWasDown;
}

void AtxController::setButtonWasDown(bool wasDown) {
	_buttonWasDown = wasDown;
}

void AtxController::signalInit() {
	digitalWrite(PIN_IO_EX_ON, HIGH);
	digitalWrite(PIN_RUN, HIGH);
}
