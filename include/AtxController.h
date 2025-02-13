#ifndef _ATX_CONTROLLER_H
#define _ATX_CONTROLLER_H

#include <Arduino.h>

#define PIN_LED_STANDBY 13
#define PIN_PS_ON 14
#define PIN_RUN 12
#define PIN_PWR_OK 36
#define PIN_PWR_BTN 39

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
	bool buttonWasDown();
	void setButtonWasDown(bool wasDown);

	static AtxController *singleton;
	TaskHandle_t atxTask;

private:
	int _lastPwrOkState;
	volatile SemaphoreHandle_t _mutex;
	volatile SystemState _currentState;
	volatile SystemState _lastState;
	volatile bool _buttonWasDown;
	void (*systemPowerOn)();
	void (*systemPowerOff)();
	void (*systemPowerInit)();
};

#endif