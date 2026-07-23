/* Copyright (c) 2026 Epilogue
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <mgba/internal/gb/sio/link-netplay.h>

#include <mgba/internal/gb/gb.h>
#include <mgba/internal/gb/io.h>

#define GBSIO_NETPLAY_POLL_CYCLES 4096

static bool _GBSIONetplayNodeInit(struct GBSIODriver* driver);
static void _GBSIONetplayNodeDeinit(struct GBSIODriver* driver);
static void _GBSIONetplayNodeWriteSB(struct GBSIODriver* driver, uint8_t value);
static uint8_t _GBSIONetplayNodeWriteSC(struct GBSIODriver* driver, uint8_t value);
static void _GBSIONetplayPollEvent(struct mTiming* timing, void* context, uint32_t cyclesLate);
static void _completeWithPending(struct GBSIONetplayNode* node, uint8_t pendingSB);

void GBSIONetplayNodeCreate(struct GBSIONetplayNode* node) {
	node->d.init = _GBSIONetplayNodeInit;
	node->d.deinit = _GBSIONetplayNodeDeinit;
	node->d.writeSB = _GBSIONetplayNodeWriteSB;
	node->d.writeSC = _GBSIONetplayNodeWriteSC;
	node->pollEvent.context = node;
	node->pollEvent.name = "GB SIO Private Serial Bridge Poll";
	node->pollEvent.callback = _GBSIONetplayPollEvent;
	node->pollEvent.priority = 0x81;
	node->interface = NULL;
	node->runtime = NULL;
	node->attached = false;
	node->sessionActive = false;
	node->scWriteInProgress = false;
}

void GBSIONetplaySetSessionActive(struct GBSIONetplayNode* node, bool active) {
	node->sessionActive = active;
}

void GBSIONetplaySetRuntime(struct GBSIONetplayNode* node,
		const struct retro_epilogue_serial_interface* interface, void* runtime) {
	node->interface = interface;
	node->runtime = runtime;
}

static bool _GBSIONetplayNodeInit(struct GBSIODriver* driver) {
	struct GBSIONetplayNode* node = (struct GBSIONetplayNode*) driver;
	mTimingDeschedule(&driver->p->p->timing, &node->pollEvent);
	mTimingSchedule(&driver->p->p->timing, &node->pollEvent, GBSIO_NETPLAY_POLL_CYCLES);
	node->attached = true;
	return true;
}

static void _GBSIONetplayNodeDeinit(struct GBSIODriver* driver) {
	struct GBSIONetplayNode* node = (struct GBSIONetplayNode*) driver;
	mTimingDeschedule(&driver->p->p->timing, &node->pollEvent);
	node->attached = false;
	node->scWriteInProgress = false;
}

static void _GBSIONetplayNodeWriteSB(struct GBSIODriver* driver, uint8_t value) {
	struct GBSIONetplayNode* node = (struct GBSIONetplayNode*) driver;
	if (node->interface && node->runtime) {
		node->interface->gb_write_sb(node->runtime, value);
	}
}

static uint8_t _GBSIONetplayNodeWriteSC(struct GBSIODriver* driver, uint8_t value) {
	struct GBSIONetplayNode* node = (struct GBSIONetplayNode*) driver;
	if (!node->interface || !node->runtime) {
		return value;
	}
	if (node->sessionActive && GBRegisterSCIsEnable(value) && GBRegisterSCIsShiftClock(value)) {
		mTimingDeschedule(&driver->p->p->timing, &driver->p->event);
	}
	node->scWriteInProgress = true;
	struct retro_epilogue_serial_action action = node->interface->gb_write_sc(node->runtime, value);
	node->scWriteInProgress = false;
	GBSIONetplayApplyAction(node, action);
	return value;
}

static void _completeWithPending(struct GBSIONetplayNode* node, uint8_t pendingSB) {
	if (!node->attached) {
		return;
	}
	struct GBSIO* sio = node->d.p;
	if (!node->scWriteInProgress && !GBRegisterSCIsEnable(sio->p->memory.io[GB_REG_SC])) {
		return;
	}
	sio->pendingSB = pendingSB;
	sio->remainingBits = 8;
	mTimingDeschedule(&sio->p->timing, &sio->event);
	mTimingSchedule(&sio->p->timing, &sio->event, 0);
}

void GBSIONetplayEnsurePolling(struct GBSIONetplayNode* node) {
	if (!node->attached) {
		return;
	}
	struct mTiming* timing = &node->d.p->p->timing;
	if (!mTimingIsScheduled(timing, &node->pollEvent)) {
		mTimingSchedule(timing, &node->pollEvent, GBSIO_NETPLAY_POLL_CYCLES);
	}
}

void GBSIONetplayApplyAction(struct GBSIONetplayNode* node,
		struct retro_epilogue_serial_action action) {
	if (action.flags & RETRO_EPILOGUE_SERIAL_ACTION_COMPLETE) {
		_completeWithPending(node, (uint8_t) action.data32);
	}
	if (action.flags & RETRO_EPILOGUE_SERIAL_ACTION_SHIFT_SB) {
		if (node->attached) {
			node->d.p->p->memory.io[GB_REG_SB] = (uint8_t) action.data32;
		}
	}
}

static void _GBSIONetplayPollEvent(struct mTiming* timing, void* context, uint32_t cyclesLate) {
	struct GBSIONetplayNode* node = context;
	if (node->interface && node->runtime) {
		const uint32_t timingHz = node->d.p->p->model == GB_MODEL_SGB
			? SGB_SM83_FREQUENCY * 2 : CGB_SM83_FREQUENCY;
		GBSIONetplayApplyAction(node, node->interface->poll(node->runtime,
			node->d.p->p->memory.io[GB_REG_SC], GBSIO_NETPLAY_POLL_CYCLES, timingHz));
	}
	mTimingSchedule(timing, &node->pollEvent, GBSIO_NETPLAY_POLL_CYCLES - cyclesLate);
}
