#pragma once

// NightMareNetwork feature selection for Sherlock. Features.h picks this file up through
// __has_include, so it has to stay in include/ -- which platformio.ini adds to the library's
// include path with -I "${platformio.include_dir}".

// VERSION / BUILD_TIMESTAMP come from the version pre-script. Including it here also lets
// introNightMareESP() print them, since the library only sees macros this file brings in.
#include <Version.h>

#define NM_FIRMWARE_VERSION VERSION

#define NM_ENABLE_SETTINGS 1
#define NM_ENABLE_RESOURCES 1
#define NM_ENABLE_NETWORK 1
#define NM_ENABLE_CONSOLE 1
#define NM_ENABLE_WIFI 1
#define NM_ENABLE_MQTT 1
#define NM_ENABLE_TELEMETRY 1
#define NM_ENABLE_SCHEDULER 1
#define NM_ENABLE_JOBS 1
#define NM_ENABLE_TIME_SYNC 1
// Sherlock is reflashed over the air; the LED strip shows the progress.
#define NM_ENABLE_OTA 1
// The web UI is Sherlock's own WebServer on port 80.
#define NM_ENABLE_HTTP 0
#define NM_ENABLE_WEBSOCKET 0
#define NM_ENABLE_LVGL 0

// POSIX TZ for UTC-3 (America/Bahia, no DST): the sign is inverted in POSIX notation.
#define NM_TIMEZONE "<-03>3"

#define NM_CONSOLE_BUILTINS 1
#define NM_CONSOLE_SERIAL 1

#define NM_SCHEDULER_OWN_TASK 1
#define NM_LOG_LEVEL NM_LOG_LEVEL_INFO
