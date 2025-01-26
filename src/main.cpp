/**
 * @file main.cpp
 * @author Cyrus Brunner (cyrusbuilt at gmail dot com)
 * @brief 
 * @version 1.3
 * @date 2022-10-21
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef ESP32
	#error "This firmware is only intended to run on ESP32 MCUs."
#endif

#include "Arduino.h"
#include "AtxController.h"
#include "AtxTask.h"
#include "fabgl.h"

// Forward declarations
void handleAtxPowerOn();
void handleAtxPowerOff();
void handleAtxPowerInit();

// Global vars
fabgl::BitmappedDisplayController*  DisplayController;
fabgl::PS2Controller                PS2Controller;
fabgl::Terminal                     Terminal;
fabgl::SerialPort                   SerialPort;
fabgl::SerialPortTerminalConnector  SerialPortTerminalConnector;
AtxController atxController;

#include "confdialog.h"

#define INPUT_QUEUE_SIZE 2048     // 2048 good to pass vttest
#define KB_READER_STACK_SIZE 3000 // more stack is required for the UI (used inside Terminal.onVirtualKey)

void disableWatchdogs() {
	Serial.println(F("INIT: boot0 - Disable watchdogs. "));
	disableCore0WDT();
  	delay(100); // experienced crashes without this delay!
  	disableCore1WDT();
}

void bootScreen() {
	if (ConfDialogApp::getBootInfo() == BOOTINFO_ENABLED) {
    	Terminal.write("* *  CyBorg VGA BIOS - Integrated Terminal              * *\r\n");
    	Terminal.write("* *  2022 by Cyrus Brunner - cyrusbuilt@gmail.com       * *\r\n");
		Terminal.write("* *  Based on AnsiTerminal using FabGL                  * *\r\n");
		Terminal.write("* *  2019-2022 by Fabrizio Di Vittorio - www.fabgl.com  * *\r\n\n");
    	Terminal.printf("Version            : %d.%d\r\n", TERMVERSION_MAJ, TERMVERSION_MIN);
    	Terminal.printf("Screen Size        : %d x %d\r\n", DisplayController->getViewPortWidth(), DisplayController->getViewPortHeight());
    	Terminal.printf("Terminal Size      : %d x %d\r\n", Terminal.getColumns(), Terminal.getRows());
    	Terminal.printf("Keyboard Layout    : %s\r\n", PS2Controller.keyboard()->isKeyboardAvailable() ? SupportedLayouts::names()[ConfDialogApp::getKbdLayoutIndex()] : "No Keyboard");
    	Terminal.printf("Mouse              : %s\r\n", PS2Controller.mouse()->isMouseAvailable() ? "Yes" : "No");
    	Terminal.printf("Terminal Type      : %s\r\n", SupportedTerminals::names()[(int)ConfDialogApp::getTermType()]);
    	Terminal.printf("Free Memory        : %d bytes\r\n", heap_caps_get_free_size(MALLOC_CAP_32BIT));
    	Terminal.write("\r\nPress F12 to change terminal configuration and CTRL-ALT-F12 to reset settings\r\n\n");
  	} else if (ConfDialogApp::getBootInfo() == BOOTINFO_TEMPDISABLED) {
    	preferences.putInt(PREF_BOOTINFO, BOOTINFO_ENABLED);
  	}
}

void configureKeyboardEvents() {
	// onVirtualKey is triggered whenever a key is pressed or released
  	Terminal.onVirtualKeyItem = [&](VirtualKeyItem * vkItem) {
    	if (vkItem->vk == VirtualKey::VK_F12) {
      		// CTRL ALT F12 -> show reboot dialog
      		// F12          -> show configuration dialog
      		if (vkItem->CTRL && (vkItem->LALT || vkItem->RALT)) {
        		Terminal.deactivate();
        		preferences.clear();

        		// show reboot dialog
        		auto rebootApp = new RebootDialogApp;
        		rebootApp->run(DisplayController);
      		}
			else if (!vkItem->CTRL && !vkItem->LALT && !vkItem->RALT && !vkItem->down) {
        		// releasing F12 key to open configuration dialog
        		Terminal.deactivate();
        		if (PS2Controller.mouse()) {
          			PS2Controller.mouse()->emptyQueue();  // avoid previous mouse movements to be showed on UI
				}
        		
				auto dlgApp = new ConfDialogApp;
        		dlgApp->run(DisplayController);
        		
        		// this is required, because the terminal may not cover the entire screen
        		Terminal.canvas()->reset();
        		Terminal.canvas()->setBrushColor(dlgApp->getBGColor());
        		Terminal.canvas()->fillRectangle(dlgApp->frameRect);        
        		delete dlgApp;

        		Terminal.keyboard()->emptyVirtualKeyQueue();
        		Terminal.activate();
        		
        		vkItem->vk = VirtualKey::VK_NONE;
      		}
    	}
		else if (vkItem->vk == VirtualKey::VK_BREAK && !vkItem->down) {
      		// BREAK (CTRL PAUSE) -> short break (TX low for 3.5 s)
      		// SHIFT BREAK (SHIFT CTRL PAUSE) -> long break (TX low for 0.233 ms)
      		SerialPort.sendBreak(true);
      		vTaskDelay((vkItem->SHIFT ? 3500 : 233) / portTICK_PERIOD_MS);
      		SerialPort.sendBreak(false);
      		vkItem->vk = VirtualKey::VK_NONE;
    	}
  	};

  	// onUserSequence is triggered whenever a User Sequence has been received (ESC + '_#' ... '$'), where '...' is sent here
  	Terminal.onUserSequence = [&](char const * seq) {
		Serial.print(F("DEBUG: User sequence: "));
		Serial.println(seq);
		// TODO implement custom sequences for the following:
		// 1) Soft-off
		// 2) Reset MCP23017
		// 3) Full system reset?

    	// 'R': change resolution (for example: ESC + "_#R512x384x64$")
    	for (int i = 0; i < RESOLUTIONS_COUNT; ++i) {
      		if (strcmp(RESOLUTIONS_CMDSTR[i], seq) == 0) {
        		// found resolution string
        		preferences.putInt(PREF_TEMPRESOLUTION, i);
        		if (ConfDialogApp::getBootInfo() == BOOTINFO_ENABLED) {
          			preferences.putInt(PREF_BOOTINFO, BOOTINFO_TEMPDISABLED);
				}

				// TODO this will effectively reboot the whole machine. Do we want to do this?
        		ESP.restart();
      		}
		}
  	};
}

void bootStage2() {
	Serial.println(F("INIT: boot2 - ViCREM init."));

	// Wait about 3 seconds to allow the display to wake up and show the
	// VGA BIOS info before booting the main system.
	delay(5000);

	// Turn on the I/O expander and send RUN signal to Northbridge.
	AtxController::singleton->signalInit();
}

void bootStage1() {
	Serial.println(F("INIT: boot1 - Terminal init."));
	Terminal.keyboardReaderTaskStackSize = KB_READER_STACK_SIZE;
	Terminal.inputQueueSize = INPUT_QUEUE_SIZE;
	
	preferences.begin("CyBorg", false);

  	ConfDialogApp::checkVersion();

	// because mouse is optional, don't re-try if it is not found (to speed-up boot)
  	fabgl::Mouse::quickCheckHardware();

	// Keyboard on PS/2 Port 0, mouse on Port 1.
	PS2Controller.begin(PS2Preset::KeyboardPort0_MousePort1);

	ConfDialogApp::setupDisplay();
  	ConfDialogApp::loadConfiguration();

	Terminal.clear();
  	Terminal.enableCursor(true);

	bootScreen();

	configureKeyboardEvents();
}

void handleAtxPowerOn() {
	// We have full power. Moving on to full boot...
	Serial.println(F("INIT: boot0 - ATX PSU initialized."));
	bootStage1();
	bootStage2();
}

void handleAtxPowerOff() {
	// We lost power. Restart.
	Serial.println(F("INFO: ATX PSU shutdown detected."));
	ESP.restart();
}

void handleAtxPowerInit() {
	Serial.println(F("INIT: boot0 - ATX PSU initializing..."));
}

void bootStage0() {
	Serial.begin(115200);
	Serial.print(F("CyBorg SB Firmware v"));
	Serial.print(TERMVERSION_MAJ);
	Serial.print(F("."));
	Serial.println(TERMVERSION_MIN);
	Serial.println(F("INIT: boot0 - Southbridge init."));
	delay(500);
	disableWatchdogs();
	AtxController::singleton->onPowerInit(handleAtxPowerInit);
	AtxController::singleton->onPowerOn(handleAtxPowerOn);
	AtxController::singleton->onPowerOff(handleAtxPowerOff);
	initAtxController();
}

void setup() {
	bootStage0();
}

void loop() {
	// the terminal job is done using UART interrupts
  	vTaskDelete(NULL);
}