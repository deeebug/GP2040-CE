/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

// GP2040 Libraries
#include "gamepad.h"
#include "storagemanager.h"

#include "FlashPROM.h"
#include "CRC32.h"

#include <stdio.h>

#include "picolight.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"


bool __no_inline_not_in_flash_func(get_bootsel_button)() {
    const uint CS_PIN_INDEX = 1;

    // Must disable interrupts, as interrupt handlers may be in flash, and we
    // are about to temporarily disable flash access!
    uint32_t flags = save_and_disable_interrupts();

    // Set chip select to Hi-Z
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    // Note we can't call into any sleep functions in flash right now
    for (volatile int i = 0; i < 1000; ++i);

    // The HI GPIO registers in SIO can observe and control the 6 QSPI pins.
    // Note the button pulls the pin *low* when pressed.
    bool button_state = !(sio_hw->gpio_hi_in & (1u << CS_PIN_INDEX));

    // Need to restore the state of chip select, else we are going to have a
    // bad time when we return to code in flash!
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    restore_interrupts(flags);

    return button_state;
}

// MUST BE DEFINED for mpgs
uint32_t getMillis() {
	return to_ms_since_boot(get_absolute_time());
}

uint64_t getMicro() {
	return to_us_since_boot(get_absolute_time());
}

void Gamepad::setup()
{
	load(); // MPGS loads

	// Configure pin mapping
	f2Mask = (GAMEPAD_MASK_A1 | GAMEPAD_MASK_S2);
	BoardOptions boardOptions = Storage::getInstance().getBoardOptions();

	mapDpadUp    = new GamepadButtonMapping(boardOptions.pinDpadUp,    GAMEPAD_MASK_UP);
	mapDpadDown  = new GamepadButtonMapping(boardOptions.pinDpadDown,  GAMEPAD_MASK_DOWN);
	mapDpadLeft  = new GamepadButtonMapping(boardOptions.pinDpadLeft,  GAMEPAD_MASK_LEFT);
	mapDpadRight = new GamepadButtonMapping(boardOptions.pinDpadRight, GAMEPAD_MASK_RIGHT);
	mapButtonB1  = new GamepadButtonMapping(boardOptions.pinButtonB1,  GAMEPAD_MASK_B1);
	mapButtonB2  = new GamepadButtonMapping(boardOptions.pinButtonB2,  GAMEPAD_MASK_B2);
	mapButtonB3  = new GamepadButtonMapping(boardOptions.pinButtonB3,  GAMEPAD_MASK_B3);
	mapButtonB4  = new GamepadButtonMapping(boardOptions.pinButtonB4,  GAMEPAD_MASK_B4);
	mapButtonL1  = new GamepadButtonMapping(boardOptions.pinButtonL1,  GAMEPAD_MASK_L1);
	mapButtonR1  = new GamepadButtonMapping(boardOptions.pinButtonR1,  GAMEPAD_MASK_R1);
	mapButtonL2  = new GamepadButtonMapping(boardOptions.pinButtonL2,  GAMEPAD_MASK_L2);
	mapButtonR2  = new GamepadButtonMapping(boardOptions.pinButtonR2,  GAMEPAD_MASK_R2);
	mapButtonS1  = new GamepadButtonMapping(boardOptions.pinButtonS1,  GAMEPAD_MASK_S1);
	mapButtonS2  = new GamepadButtonMapping(boardOptions.pinButtonS2,  GAMEPAD_MASK_S2);
	mapButtonL3  = new GamepadButtonMapping(boardOptions.pinButtonL3,  GAMEPAD_MASK_L3);
	mapButtonR3  = new GamepadButtonMapping(boardOptions.pinButtonR3,  GAMEPAD_MASK_R3);
	mapButtonA1  = new GamepadButtonMapping(boardOptions.pinButtonA1,  GAMEPAD_MASK_A1);
	mapButtonA2  = new GamepadButtonMapping(boardOptions.pinButtonA2,  GAMEPAD_MASK_A2);
	
	gamepadMappings = new GamepadButtonMapping *[GAMEPAD_DIGITAL_INPUT_COUNT]
	{
		mapDpadUp,   mapDpadDown, mapDpadLeft, mapDpadRight,
		mapButtonB1, mapButtonB2, mapButtonB3, mapButtonB4,
		mapButtonL1, mapButtonR1, mapButtonL2, mapButtonR2,
		mapButtonS1, mapButtonS2, mapButtonL3, mapButtonR3,
		mapButtonA1, mapButtonA2
	};

	for (int i = 0; i < GAMEPAD_DIGITAL_INPUT_COUNT; i++)
	{
		gpio_init(gamepadMappings[i]->pin);             // Initialize pin
		gpio_set_dir(gamepadMappings[i]->pin, GPIO_IN); // Set as INPUT
		gpio_pull_up(gamepadMappings[i]->pin);          // Set as PULLUP
	}

	#ifdef PIN_SETTINGS
		gpio_init(PIN_SETTINGS);             // Initialize pin
		gpio_set_dir(PIN_SETTINGS, GPIO_IN); // Set as INPUT
		gpio_pull_up(PIN_SETTINGS);          // Set as PULLUP
	#endif
}

void Gamepad::process()
{
	memcpy(&rawState, &state, sizeof(GamepadState));
	MPGS::process();
}

int pushed = 0;
int raised = 1;
int yes = 0;

void Gamepad::read()
{
	// Need to invert since we're using pullups
	uint32_t values = ~gpio_get_all();

	// if (!pushed & get_bootsel_button())
	// 	pushed = 1;

	// if (pushed & !get_bootsel_button()) {
	// 	pushed = 0;
	// 	yes = (yes + 1) % 13;
	// }

	#ifdef PIN_SETTINGS
	state.aux = 0
		| ((values & (1 << PIN_SETTINGS)) ? (1 << 0) : 0)
	;
	#endif

	state.dpad = 0
		| ((values & mapDpadUp->pinMask)    ? (options.invertYAxis ? mapDpadDown->buttonMask : mapDpadUp->buttonMask) : 0)
		| ((values & mapDpadDown->pinMask)  ? (options.invertYAxis ? mapDpadUp->buttonMask : mapDpadDown->buttonMask) : 0)
		| ((values & mapDpadLeft->pinMask)  ? mapDpadLeft->buttonMask  : 0)
		| ((values & mapDpadRight->pinMask) ? mapDpadRight->buttonMask : 0)
	;

	// state.buttons = 0
	// 	| ((yes == 0)  ? mapButtonB1->buttonMask  : 0)
	// 	| ((yes == 1)  ? mapButtonB2->buttonMask  : 0)
	// 	| ((yes == 2)  ? mapButtonB3->buttonMask  : 0)
	// 	| ((yes == 3)  ? mapButtonB4->buttonMask  : 0)
	// 	| ((yes == 4)  ? mapButtonL1->buttonMask  : 0)
	// 	| ((yes == 5)  ? mapButtonR1->buttonMask  : 0)
	// 	| ((yes == 6)  ? mapButtonL2->buttonMask  : 0)
	// 	| ((yes == 7)  ? mapButtonR2->buttonMask  : 0)
	// 	| ((yes == 8)  ? mapButtonS1->buttonMask  : 0)
	// 	| ((yes == 9)  ? mapButtonS2->buttonMask  : 0)
	// 	| ((yes == 10)  ? mapButtonL3->buttonMask  : 0)
	// 	| ((yes == 11)  ? mapButtonR3->buttonMask  : 0)
	// 	| ((yes == 12)  ? mapButtonA1->buttonMask  : 0)
	// 	| ((yes == 13)  ? mapButtonA2->buttonMask  : 0)
	// ;

	bool bootsel = get_bootsel_button();
	light_up(!bootsel);

	state.buttons = 0
		| ((bootsel)  ? mapButtonA1->buttonMask  : 0)
		| ((values & mapButtonB2->pinMask)  ? mapButtonB2->buttonMask  : 0)
		| ((values & mapButtonB3->pinMask)  ? mapButtonB3->buttonMask  : 0)
		| ((values & mapButtonB4->pinMask)  ? mapButtonB4->buttonMask  : 0)
		| ((values & mapButtonL1->pinMask)  ? mapButtonL1->buttonMask  : 0)
		| ((values & mapButtonR1->pinMask)  ? mapButtonR1->buttonMask  : 0)
		| ((values & mapButtonL2->pinMask)  ? mapButtonL2->buttonMask  : 0)
		| ((values & mapButtonR2->pinMask)  ? mapButtonR2->buttonMask  : 0)
		| ((values & mapButtonS1->pinMask)  ? mapButtonS1->buttonMask  : 0)
		| ((values & mapButtonS2->pinMask)  ? mapButtonS2->buttonMask  : 0)
		| ((values & mapButtonL3->pinMask)  ? mapButtonL3->buttonMask  : 0)
		| ((values & mapButtonR3->pinMask)  ? mapButtonR3->buttonMask  : 0)
		| ((values & mapButtonA1->pinMask)  ? mapButtonB1->buttonMask  : 0)
		| ((values & mapButtonA2->pinMask)  ? mapButtonA2->buttonMask  : 0)
	;

	state.lx = GAMEPAD_JOYSTICK_MID;
	state.ly = GAMEPAD_JOYSTICK_MID;
	state.rx = GAMEPAD_JOYSTICK_MID;
	state.ry = GAMEPAD_JOYSTICK_MID;
	state.lt = 0;
	state.rt = 0;
}


/* Gamepad stuffs */
void GamepadStorage::start()
{
	EEPROM.start();
}

void GamepadStorage::save()
{
	EEPROM.commit();
}

GamepadOptions GamepadStorage::getGamepadOptions()
{
	GamepadOptions options;
	EEPROM.get(GAMEPAD_STORAGE_INDEX, options);

	uint32_t lastCRC = options.checksum;
	options.checksum = CHECKSUM_MAGIC;
	if (CRC32::calculate(&options) != lastCRC)
	{
		options.inputMode = InputMode::INPUT_MODE_XINPUT; // Default?
		options.dpadMode = DpadMode::DPAD_MODE_DIGITAL; // Default?
#ifdef DEFAULT_SOCD_MODE
		options.socdMode = DEFAULT_SOCD_MODE;
#else
		options.socdMode = SOCD_MODE_NEUTRAL;
#endif
		setGamepadOptions(options);
	}

	return options;
}

void GamepadStorage::setGamepadOptions(GamepadOptions options)
{
	options.checksum = 0;
	options.checksum = CRC32::calculate(&options);
	EEPROM.set(GAMEPAD_STORAGE_INDEX, options);
}

