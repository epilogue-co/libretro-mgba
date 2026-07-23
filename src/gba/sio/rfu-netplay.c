/* Copyright (c) 2026 Epilogue
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <mgba/internal/gba/sio/rfu-netplay.h>

#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/io.h>

#include <stdlib.h>

#define GBA_RFU_BRIDGE_POLL_CYCLES 16384

static bool _init(struct GBASIODriver* driver);
static void _deinit(struct GBASIODriver* driver);
static void _reset(struct GBASIODriver* driver);
static uint32_t _driverId(const struct GBASIODriver* driver);
static bool _loadState(struct GBASIODriver* driver, const void* state, size_t size);
static void _saveState(struct GBASIODriver* driver, void** state, size_t* size);
static bool _handlesMode(struct GBASIODriver* driver, enum GBASIOMode mode);
static int _connectedDevices(struct GBASIODriver* driver);
static uint16_t _writeRCNT(struct GBASIODriver* driver, uint16_t value);
static uint16_t _writeSIOCNT(struct GBASIODriver* driver, uint16_t value);
static bool _start(struct GBASIODriver* driver);
static uint32_t _finishNormal32(struct GBASIODriver* driver);
static void _pollEvent(struct mTiming* timing, void* context, uint32_t cyclesLate);

void GBASIORFUNetplayNodeCreate(struct GBASIORFUNetplayNode* node) {
	node->d.init = _init;
	node->d.deinit = _deinit;
	node->d.reset = _reset;
	node->d.driverId = _driverId;
	node->d.loadState = _loadState;
	node->d.saveState = _saveState;
	node->d.setMode = NULL;
	node->d.handlesMode = _handlesMode;
	node->d.connectedDevices = _connectedDevices;
	node->d.deviceId = NULL;
	node->d.writeSIOCNT = _writeSIOCNT;
	node->d.writeRCNT = _writeRCNT;
	node->d.start = _start;
	node->d.finishMultiplayer = NULL;
	node->d.finishNormal8 = NULL;
	node->d.finishNormal32 = _finishNormal32;
	node->pollEvent.context = node;
	node->pollEvent.name = "GBA SIO Private RFU Bridge Poll";
	node->pollEvent.callback = _pollEvent;
	node->pollEvent.priority = 0x81;
	node->interface = NULL;
	node->runtime = NULL;
}

void GBASIORFUNetplaySetRuntime(struct GBASIORFUNetplayNode* node,
		const struct retro_epilogue_serial_interface* interface, void* runtime) {
	node->interface = interface;
	node->runtime = runtime;
}

static bool _init(struct GBASIODriver* driver) {
	struct GBASIORFUNetplayNode* node = (struct GBASIORFUNetplayNode*) driver;
	mTimingDeschedule(&driver->p->p->timing, &node->pollEvent);
	mTimingSchedule(&driver->p->p->timing, &node->pollEvent, GBA_RFU_BRIDGE_POLL_CYCLES);
	return node->interface && node->runtime;
}

static void _deinit(struct GBASIODriver* driver) {
	struct GBASIORFUNetplayNode* node = (struct GBASIORFUNetplayNode*) driver;
	mTimingDeschedule(&driver->p->p->timing, &node->pollEvent);
}

static void _reset(struct GBASIODriver* driver) {
	struct GBASIORFUNetplayNode* node = (struct GBASIORFUNetplayNode*) driver;
	node->interface->gba_reset(node->runtime);
	mTimingDeschedule(&driver->p->p->timing, &node->pollEvent);
	mTimingSchedule(&driver->p->p->timing, &node->pollEvent, GBA_RFU_BRIDGE_POLL_CYCLES);
}

static uint32_t _driverId(const struct GBASIODriver* driver) {
	UNUSED(driver);
	return 0x45505231;
}

static bool _loadState(struct GBASIODriver* driver, const void* state, size_t size) {
	struct GBASIORFUNetplayNode* node = (struct GBASIORFUNetplayNode*) driver;
	return node->interface->load_state(node->runtime, state, size);
}

static void _saveState(struct GBASIODriver* driver, void** state, size_t* size) {
	struct GBASIORFUNetplayNode* node = (struct GBASIORFUNetplayNode*) driver;
	*size = node->interface->state_size(node->runtime);
	*state = *size ? malloc(*size) : NULL;
	if (*size && !node->interface->save_state(node->runtime, *state, *size)) {
		free(*state);
		*state = NULL;
		*size = 0;
	}
}

static bool _handlesMode(struct GBASIODriver* driver, enum GBASIOMode mode) {
	UNUSED(driver);
	return mode == GBA_SIO_NORMAL_32;
}

static int _connectedDevices(struct GBASIODriver* driver) {
	UNUSED(driver);
	return 1;
}

static uint16_t _writeSIOCNT(struct GBASIODriver* driver, uint16_t value) {
	struct GBASIORFUNetplayNode* node = (struct GBASIORFUNetplayNode*) driver;
	return node->interface->gba_write_siocnt(node->runtime, value);
}

static uint16_t _writeRCNT(struct GBASIODriver* driver, uint16_t value) {
	struct GBASIORFUNetplayNode* node = (struct GBASIORFUNetplayNode*) driver;
	return node->interface->gba_write_rcnt(node->runtime, value, driver->p->mode == GBA_SIO_GPIO);
}

static bool _start(struct GBASIODriver* driver) {
	struct GBASIORFUNetplayNode* node = (struct GBASIORFUNetplayNode*) driver;
	struct GBASIO* sio = driver->p;
	uint32_t sent = sio->p->memory.io[GBA_REG(SIODATA32_LO)] |
		((uint32_t) sio->p->memory.io[GBA_REG(SIODATA32_HI)] << 16);
	return node->interface->gba_start(node->runtime, sent, sio->mode == GBA_SIO_NORMAL_32);
}

void GBASIORFUNetplayEnsurePolling(struct GBASIORFUNetplayNode* node) {
	if (!node->interface || !node->runtime || !node->d.p) {
		return;
	}
	struct mTiming* timing = &node->d.p->p->timing;
	if (!mTimingIsScheduled(timing, &node->pollEvent)) {
		mTimingSchedule(timing, &node->pollEvent, GBA_RFU_BRIDGE_POLL_CYCLES);
	}
}

void GBASIORFUNetplayApplyAction(struct GBASIORFUNetplayNode* node,
		struct retro_epilogue_serial_action action) {
	if (action.flags & RETRO_EPILOGUE_SERIAL_ACTION_RAISE_SI) {
		node->d.p->siocnt |= 0x0004;
	}
	if (action.flags & RETRO_EPILOGUE_SERIAL_ACTION_COMPLETE) {
		GBASIONormal32FinishTransfer(node->d.p, action.data32, 0);
	}
}

static uint32_t _finishNormal32(struct GBASIODriver* driver) {
	struct GBASIORFUNetplayNode* node = (struct GBASIORFUNetplayNode*) driver;
	struct retro_epilogue_serial_action action = node->interface->gba_finish_normal32(node->runtime);
	if (action.flags & RETRO_EPILOGUE_SERIAL_ACTION_RAISE_SI) {
		driver->p->siocnt |= 0x0004;
	}
	return action.data32;
}

static void _pollEvent(struct mTiming* timing, void* context, uint32_t cyclesLate) {
	struct GBASIORFUNetplayNode* node = context;
	struct retro_epilogue_serial_action action = node->interface->poll(
		node->runtime, node->d.p->siocnt, GBA_RFU_BRIDGE_POLL_CYCLES,
		GBA_ARM7TDMI_FREQUENCY);
	if (action.flags & RETRO_EPILOGUE_SERIAL_ACTION_RAISE_SI) {
		node->d.p->siocnt |= 0x0004;
	}
	if (action.flags & RETRO_EPILOGUE_SERIAL_ACTION_COMPLETE) {
		GBASIONormal32FinishTransfer(node->d.p, action.data32, cyclesLate);
	}
	mTimingSchedule(timing, &node->pollEvent, GBA_RFU_BRIDGE_POLL_CYCLES - cyclesLate);
}
