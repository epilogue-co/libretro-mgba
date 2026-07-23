/* Copyright (c) 2026 Epilogue
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LIBRETRO_EPILOGUE_H
#define LIBRETRO_EPILOGUE_H

#include "libretro.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RETRO_ENVIRONMENT_GET_EPILOGUE_SERIAL_INTERFACE (0x100 | RETRO_ENVIRONMENT_PRIVATE)

#define RETRO_EPILOGUE_SERIAL_ABI_VERSION 1u
#define RETRO_EPILOGUE_SERIAL_BROADCAST 0xFFFFu

enum retro_epilogue_serial_device {
  RETRO_EPILOGUE_SERIAL_DEVICE_GB_LINK = 1,
  RETRO_EPILOGUE_SERIAL_DEVICE_GBA_RFU = 2,
};

enum retro_epilogue_serial_action_flags {
  RETRO_EPILOGUE_SERIAL_ACTION_NONE = 0,
  RETRO_EPILOGUE_SERIAL_ACTION_HOLD_TRANSFER = 1u << 0,
  RETRO_EPILOGUE_SERIAL_ACTION_COMPLETE = 1u << 1,
  RETRO_EPILOGUE_SERIAL_ACTION_RAISE_SI = 1u << 2,
  RETRO_EPILOGUE_SERIAL_ACTION_SHIFT_SB = 1u << 3,
};

struct retro_epilogue_serial_action {
  uint32_t flags;
  uint32_t data32;
};

typedef void (*retro_epilogue_serial_send_t)(void* context, int flags, const void* data, size_t len,
                                             uint16_t client_id);
typedef void (*retro_epilogue_serial_poll_receive_t)(void* context);

struct retro_epilogue_serial_transport {
  void* context;
  retro_epilogue_serial_send_t send;
  retro_epilogue_serial_poll_receive_t poll_receive;
};

struct retro_epilogue_serial_interface {
  uint32_t abi_version;
  size_t size;

  void* (*create)(enum retro_epilogue_serial_device device);
  void (*destroy)(void* instance);
  const char* (*protocol_version)(void* instance);

  struct retro_epilogue_serial_action (*session_start)(
      void* instance, uint16_t client_id, const struct retro_epilogue_serial_transport* transport);
  struct retro_epilogue_serial_action (*receive)(void* instance, const void* data, size_t len,
                                                 uint16_t client_id);
  struct retro_epilogue_serial_action (*session_stop)(void* instance);
  struct retro_epilogue_serial_action (*peer_disconnected)(void* instance, uint16_t client_id);
  struct retro_epilogue_serial_action (*poll)(void* instance, uint32_t serial_control,
                                              uint32_t elapsed_cycles, uint32_t clock_hz);

  void (*gb_write_sb)(void* instance, uint8_t value);
  struct retro_epilogue_serial_action (*gb_write_sc)(void* instance, uint8_t value);

  void (*gba_reset)(void* instance);
  uint16_t (*gba_write_siocnt)(void* instance, uint16_t value);
  uint16_t (*gba_write_rcnt)(void* instance, uint16_t value, bool gpio_mode);
  bool (*gba_start)(void* instance, uint32_t sent_value, bool normal32_mode);
  struct retro_epilogue_serial_action (*gba_finish_normal32)(void* instance);

  size_t (*state_size)(void* instance);
  bool (*load_state)(void* instance, const void* state, size_t size);
  bool (*save_state)(void* instance, void* state, size_t size);
};

struct retro_epilogue_serial_interface_query {
  uint32_t requested_abi_version;
  const struct retro_epilogue_serial_interface* serial_interface;
};

#ifdef __cplusplus
}
#endif

#endif
