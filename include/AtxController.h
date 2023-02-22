#ifndef _ATX_CONTROLLER_H
#define _ATX_CONTROLLER_H

#include <Arduino.h>

#define PIN_PS_ON 14
#define PIN_IO_EX_ON 16
#define PIN_RUN 12
#define PIN_PWR_OK 36
#define PIN_PWR_BTN 39

#define DEBOUNCE_DELAY 100
#define PWR_OFF_DELAY 10000

enum class SystemState : uint8_t {
	OFF = 0,
	INIT = 1,
	ON = 2
};

class AtxController {
public:
	AtxController();
	~AtxController();
	void begin();
	void end();
	void loop();
	void signalInit();
	SystemState getState();
	void onPowerOn(void (*systemPowerOn)());
	void onPowerOff(void (*systemPowerOff)());
	void onPowerInit(void (*systemPowerInit)());

	static AtxController *singleton;
	TaskHandle_t atxTask;

private:
	volatile SemaphoreHandle_t _mutex;
	int _buttonState;
	int _lastButtonState;
	int _lastPwrOkState;
	unsigned long _lastDebounceTime;
	volatile SystemState _currentState;
	void (*systemPowerOn)();
	void (*systemPowerOff)();
	void (*systemPowerInit)();
};

#endif