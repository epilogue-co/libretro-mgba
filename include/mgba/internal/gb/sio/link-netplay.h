/* Copyright (c) 2026 Epilogue
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef GB_SIO_LINK_NETPLAY_H
#define GB_SIO_LINK_NETPLAY_H

#include <mgba-util/common.h>

CXX_GUARD_START

#include <mgba/core/timing.h>
#include <mgba/internal/gb/sio.h>
#include <libretro_epilogue.h>

struct GBSIONetplayNode {
	struct GBSIODriver d;
	struct mTimingEvent pollEvent;
	const struct retro_epilogue_serial_interface* interface;
	void* runtime;
	bool attached;
	bool sessionActive;
	bool scWriteInProgress;
};

void GBSIONetplayNodeCreate(struct GBSIONetplayNode* node);
void GBSIONetplaySetRuntime(struct GBSIONetplayNode* node,
	const struct retro_epilogue_serial_interface* interface, void* runtime);
void GBSIONetplaySetSessionActive(struct GBSIONetplayNode* node, bool active);
void GBSIONetplayEnsurePolling(struct GBSIONetplayNode* node);
void GBSIONetplayApplyAction(struct GBSIONetplayNode* node,
	struct retro_epilogue_serial_action action);

CXX_GUARD_END

#endif
