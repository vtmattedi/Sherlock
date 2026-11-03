# Sherlock

NightMare lighting and sensor firmware for the **mw-overnight v1.0** board.
Sherlock controls a WS2812 strip, highlights open drawer handles, and publishes
temperature, humidity, drawer state and board voltage over Wi-Fi/MQTT.

## Hardware

The current PlatformIO environment is `mw-overnight`, using the Arduino framework
and the **classic ESP32** target `esp32dev`. Some hardware-profile comments still
identify an ESP32-S3; the build configuration and pin assignments below describe
the current firmware. Confirm the physical MCU before changing the target.

| Connection | GPIO / layout |
| --- | --- |
| DS18B20 temperature sensor | GPIO16 |
| DHT11 temperature/humidity sensor | GPIO17 |
| WS2812 data, GRB order | GPIO18; 133 pixels, visible range 35-132 |
| Drawer 1 digital sensor | GPIO26; HIGH means open |
| Drawer 2 digital sensor | GPIO27; HIGH means open |
| Drawer 3 | No sensor configured (`-1`); not published |
| Board-voltage ADC | GPIO35; 3.9 kohm / 1 kohm divider, calibration 0.9923 |

Pin assignments and strip geometry live in [include/board.h](include/board.h).
The voltage input is defined in [src/main.cpp](src/main.cpp).
The network hardware description is in
[include/NightMareHardware.h](include/NightMareHardware.h).

## Lighting and sensing

The normal strip has independently writable power, colour and brightness. When
drawer handling is enabled and a drawer opens, its nine-pixel handle zone lights
white and takes precedence over the normal strip display. Closing the drawers
restores the normal display. Drawer samples run every 20 ms and require two
agreeing samples before accepting a change; climate sensors are read every five
seconds. Failed climate reads preserve the last good value.

Optional automatic lighting uses local time: the default window is 18:00 to
06:00, with yellow as its colour. Automation is disabled by default and waits
for a valid clock. Equal start/end times mean an all-day window. A Wi-Fi
connection sets the configured colour, turns normal lighting off, and blinks
green three times. OTA progress uses blue, success green, and failure red.

## Network Resources

Resources use `<device>/resource/<name>/state`; writable states accept requests
at the corresponding `/set` topic. `<device>` is the configured/adopted identity.

| Name | Type | Purpose |
| --- | --- | --- |
| `led:power` | Writable boolean | Normal strip power |
| `led:color` | Writable `ColourType` | Normal strip colour |
| `led:brightness` | Writable uint8 | Brightness, 0-255 |
| `temperature` | Float sensor | DS18B20 temperature in Celsius |
| `dht_temperature` | Float sensor | DHT11 temperature in Celsius |
| `dht_humidity` | Float sensor | Relative humidity in percent |
| `drawer_1`, `drawer_2` | Boolean sensors | True means open |
| `Board Voltage` | Float sensor | Divider-corrected supply voltage in volts |

NightMareNetwork also provides device discovery, manifests, configuration,
telemetry and its serial/MQTT console.

## Runtime configuration

Use the NightMare console's `CONFIG LIST`, `CONFIG GET <name>` and
`CONFIG SET <name> <value>` commands. Accepted configuration values are persisted
by the framework.

| Name | Default | Purpose |
| --- | --- | --- |
| `auto:enable` | `false` | Enable scheduled lighting |
| `auto:show:time` | 18:00 | Start of the lighting window |
| `auto:hide:time` | 06:00 | End of the lighting window |
| `auto:show:color` | Yellow (255, 255, 0) | Automatic lighting colour |
| `drawers:enable` | `true` | Enable drawer sampling and handle lighting |
| `voltage:period` | 5000 ms | Voltage sampling interval; positive changes up to 60000 ms are accepted, nonpositive changes disable sampling |

Feature selection and the default UTC-3 timezone are in
[include/NightMareConfig.h](include/NightMareConfig.h). The OTA feature there and
`NM_ENABLE_OTA` in `platformio.ini` must stay in step so dependency discovery
includes ArduinoOTA.

## Setup and build

From this repository, create the local credentials file:

```powershell
Copy-Item include/creds.example.h include/creds.h
```

Fill in the Wi-Fi credentials, MQTT endpoints/account and trusted broker root
certificate. `creds.h`, the old `include/Creds/` directory and local `.env` files
are ignored. Keep real credentials out of commits and use the example as the
template.

Build from the umbrella folder:

```powershell
pio run -d devices/Sherlock -e mw-overnight
```

The current upload protocol is ESP OTA. When uploading deliberately, supply the
device's actual address with `--upload-port`; the commented address in
`platformio.ini` is not an active setting. First-time USB flashing requires the
appropriate serial upload configuration. No upload is needed to compile.

Serial monitoring runs at 115200 baud. The pre-build script generates
`include/Version.h` and uses `src/Version/.version_no_increment` to avoid repeated
version increments until an upload. Commit the tracked version counter only for
a deliberate version bump.

NightMareNetwork is fetched from GitHub through `lib_deps`. Shared-library work
belongs in the umbrella's `lib/nightmarenetwork`, followed by a dependency update;
do not edit `.pio/libdeps`.

## Source layout

- `src/main.cpp`: framework startup, resource wiring, automation and voltage ADC.
- `src/Light/`: strip driver, state, drawer zones and OTA indicators.
- `src/Sensors/`: climate acquisition and debounced drawer sampling task.
- `src/Version/`: generated-version support.
- `legacy/`: earlier animations and configuration, excluded from the active build.
