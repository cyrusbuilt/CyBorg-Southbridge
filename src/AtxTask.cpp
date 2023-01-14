#include "AtxTask.h"

void initAtxController() {
	Serial.print(F("INIT: boot0 - Initializing ATX power management... "));
	TaskHandle_t taskHandle = AtxController::singleton->atxTask;
	xTaskCreate(&AtxTask, "ATX Controller", 2048, NULL, 2, &taskHandle);
	Serial.println(F("DONE"));
}

void AtxTask(void *pvParameter) {
	AtxController::singleton->begin();
	for (;;) {
		AtxController::singleton->loop();
	}
}