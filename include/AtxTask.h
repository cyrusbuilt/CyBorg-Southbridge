#ifndef _ATX_TASK_H
#define _ATX_TASK_H

#include <Arduino.h>
#include "AtxController.h"

void initAtxController();
void AtxTask(void *pvParameter);

#endif