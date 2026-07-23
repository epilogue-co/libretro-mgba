/* Copyright (c) 2026 Epilogue
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef GBA_SIO_RFU_NETPLAY_H
#define GBA_SIO_RFU_NETPLAY_H

#include <mgba-util/common.h>

CXX_GUARD_START

#include <mgba/core/timing.h>
#include <mgba/internal/gba/sio.h>
#include <libretro_epilogue.h>

struct GBASIORFUNetplayNode {
	struct GBASIODriver d;
	struct mTimingEvent pollEvent;
	const struct retro_epilogue_serial_interface* interface;
	void* runtime;
};

void GBASIORFUNetplayNodeCreate(struct GBASIORFUNetplayNode* node);
void GBASIORFUNetplaySetRuntime(struct GBASIORFUNetplayNode* node,
	const struct retro_epilogue_serial_interface* interface, void* runtime);
void GBASIORFUNetplayEnsurePolling(struct GBASIORFUNetplayNode* node);
void GBASIORFUNetplayApplyAction(struct GBASIORFUNetplayNode* node,
	struct retro_epilogue_serial_action action);

CXX_GUARD_END

#endif
