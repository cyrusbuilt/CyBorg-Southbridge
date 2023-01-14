#include "AtxController.h"

AtxController* AtxController::singleton = nullptr;

AtxController::AtxController() {
	if (AtxController::singleton == nullptr) {
		AtxController::singleton = this;
	}
	_buttonState = 0;
	_lastButtonState = 0;
	_lastDebounceTime = 0;
}

void AtxController::begin() {
	pinMode(PIN_PS_ON, OUTPUT);
	digitalWrite(PIN_PS_ON, HIGH);
	pinMode(PIN_IO_EX_ON, OUTPUT);
	digitalWrite(PIN_IO_EX_ON, LOW);
	pinMode(PIN_RUN, OUTPUT);
	digitalWrite(PIN_RUN, LOW);
	pinMode(PIN_PWR_OK, INPUT);
	pinMode(PIN_PWR_BTN, INPUT_PULLUP);
}

void AtxController::loop() {
	xSemaphoreTake(_mutex, portMAX_DELAY);
	
	int powerBtnRaw = digitalRead(PIN_PWR_BTN);
	if (powerBtnRaw != _lastButtonState) {
		_lastDebounceTime = millis();
	}

	unsigned long checkDiff = millis() - _lastDebounceTime;
	if (checkDiff > DEBOUNCE_DELAY) {
		_buttonState = powerBtnRaw;
		if (_buttonState == HIGH) {
			// Button was pressed and then released.
			if (_currentState != SystemState::ON) {
				// Turn power on.
				digitalWrite(PIN_PS_ON, LOW);
				_currentState = SystemState::INIT;
			}
		}
		else {
			// Button is being held
			if (checkDiff > PWR_OFF_DELAY && _currentState != SystemState::OFF) {
				// Drop power.
				digitalWrite(PIN_PS_ON, HIGH);
			}
		}
	}

	_lastButtonState = powerBtnRaw;
	
	int pwrOkState = digitalRead(PIN_PWR_OK);
	if (pwrOkState != _lastPwrOkState) {
		// We changed power states.
		if (pwrOkState == HIGH && _currentState != SystemState::ON) {
			// We have full ATX power.
			_currentState = SystemState::ON;
		}
		else if (pwrOkState == LOW && _currentState != SystemState::OFF) {
			_currentState = SystemState::OFF;
		}
	}

	_lastPwrOkState = pwrOkState;
	xSemaphoreGive(_mutex);
}

SystemState AtxController::getState() {
	return _currentState;
}

void AtxController::signalInit() {
	digitalWrite(PIN_IO_EX_ON, HIGH);
	digitalWrite(PIN_RUN, HIGH);
}