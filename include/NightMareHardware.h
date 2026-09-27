#pragma once

#include <NightMare/HardwareProfile.h>

#include <board.h>

namespace NMHardware
{

/*
 * Sherlock physical topology (hardware configuration v2).
 *
 * The host assembly "main" is the ESP32-S3 on the mw-overnight v1.0 board, which also carries the
 * DS18B20 and the DHT11. The WS2812 strip and the TCRT5000 drawer sensors are separate physical
 * units wired to the board, so each is its own root assembly and every wire that leaves the board
 * goes contact-to-contact through a connector.
 *
 * Only the signal connections are described. Drawer sensors appear once their pin is set in
 * board.h.
 */

#define NM_SHERLOCK_STR_(x) #x
#define NM_SHERLOCK_GPIO(pin) "GPIO" NM_SHERLOCK_STR_(pin)

inline Profile projectProfile()
{
    /* ---- Main board: mw-overnight v1.0 ---------------------------------------------------- */

    static const Terminal mcuTerminals[] = {
        {NM_SHERLOCK_GPIO(ONE_WIRE_BUS)},
        {NM_SHERLOCK_GPIO(DHT_PIN)},
        {NM_SHERLOCK_GPIO(LED_STRIP_PIN)},
#if DRAWER_1_PIN >= 0
        {NM_SHERLOCK_GPIO(DRAWER_1_PIN)},
#endif
#if DRAWER_2_PIN >= 0
        {NM_SHERLOCK_GPIO(DRAWER_2_PIN)},
#endif
#if DRAWER_3_PIN >= 0
        {NM_SHERLOCK_GPIO(DRAWER_3_PIN)},
#endif
    };
    static const Terminal temperatureTerminals[] = {{"DQ"}};
    static const Terminal dhtTerminals[] = {{"DATA"}};

    static const Device mainDevices[] = {
        {"mcu", mcuTerminals, sizeof(mcuTerminals) / sizeof(mcuTerminals[0]),
         "ESP32-S3", "mcu", "ESP32-S3"},
        {"temperature", temperatureTerminals, 1, "Temperature sensor", "sensor", "DS18B20"},
        {"dht", dhtTerminals, 1, "Humidity sensor", "sensor", "DHT11"},
    };

    static const ConnectorContact ledContacts[] = {{"DIN"}};
    static const ConnectorContact drawerContacts[] = {{"DO"}};
    static const Connector mainConnectors[] = {
        {"j_led", ledContacts, 1, "LED strip", ConnectorKind::Generic},
#if DRAWER_1_PIN >= 0
        {"j_drawer_1", drawerContacts, 1, "Drawer 1 sensor", ConnectorKind::Generic},
#endif
#if DRAWER_2_PIN >= 0
        {"j_drawer_2", drawerContacts, 1, "Drawer 2 sensor", ConnectorKind::Generic},
#endif
#if DRAWER_3_PIN >= 0
        {"j_drawer_3", drawerContacts, 1, "Drawer 3 sensor", ConnectorKind::Generic},
#endif
    };

    static const Connection mainConnections[] = {
        {{"", EndpointKind::DeviceTerminal, "mcu", NM_SHERLOCK_GPIO(ONE_WIRE_BUS)},
         {"", EndpointKind::DeviceTerminal, "temperature", "DQ"}},
        {{"", EndpointKind::DeviceTerminal, "mcu", NM_SHERLOCK_GPIO(DHT_PIN)},
         {"", EndpointKind::DeviceTerminal, "dht", "DATA"}},
        {{"", EndpointKind::DeviceTerminal, "mcu", NM_SHERLOCK_GPIO(LED_STRIP_PIN)},
         {"", EndpointKind::ConnectorContact, "j_led", "DIN"}},
#if DRAWER_1_PIN >= 0
        {{"", EndpointKind::DeviceTerminal, "mcu", NM_SHERLOCK_GPIO(DRAWER_1_PIN)},
         {"", EndpointKind::ConnectorContact, "j_drawer_1", "DO"}},
#endif
#if DRAWER_2_PIN >= 0
        {{"", EndpointKind::DeviceTerminal, "mcu", NM_SHERLOCK_GPIO(DRAWER_2_PIN)},
         {"", EndpointKind::ConnectorContact, "j_drawer_2", "DO"}},
#endif
#if DRAWER_3_PIN >= 0
        {{"", EndpointKind::DeviceTerminal, "mcu", NM_SHERLOCK_GPIO(DRAWER_3_PIN)},
         {"", EndpointKind::ConnectorContact, "j_drawer_3", "DO"}},
#endif
    };

    /* ---- WS2812 strip ---------------------------------------------------------------------- */

    static const Terminal stripTerminals[] = {{"DIN"}};
    static const Device stripDevices[] = {
        {"strip", stripTerminals, 1, "LED strip", "led", "WS2812"}};
    static const Connector stripConnectors[] = {
        {"lead", ledContacts, 1, "Strip lead", ConnectorKind::DirectWire}};
    static const Connection stripConnections[] = {
        {{"", EndpointKind::DeviceTerminal, "strip", "DIN"},
         {"", EndpointKind::ConnectorContact, "lead", "DIN"}},
    };

    /* ---- TCRT5000 drawer sensor (one definition, one instance per drawer) ------------------ */

    static const Terminal sensorTerminals[] = {{"DO"}};
    static const Device sensorDevices[] = {
        {"sensor", sensorTerminals, 1, "Reflective sensor", "sensor", "TCRT5000"}};
    static const Connector sensorConnectors[] = {
        {"lead", drawerContacts, 1, "Sensor lead", ConnectorKind::DirectWire}};
    static const Connection sensorConnections[] = {
        {{"", EndpointKind::DeviceTerminal, "sensor", "DO"},
         {"", EndpointKind::ConnectorContact, "lead", "DO"}},
    };

    static const HardwareDefinition definitions[] = {
        {"mw-overnight-v1", AssemblyKind::CustomBoard, "mw-overnight", "mw-overnight:v1.0",
         nullptr,
         {nullptr, 0, mainDevices, sizeof(mainDevices) / sizeof(mainDevices[0]),
          mainConnectors, sizeof(mainConnectors) / sizeof(mainConnectors[0]),
          mainConnections, sizeof(mainConnections) / sizeof(mainConnections[0])}},
        {"ws2812-strip", AssemblyKind::External, "WS2812 LED strip", "WS2812", nullptr,
         {nullptr, 0, stripDevices, 1, stripConnectors, 1, stripConnections, 1}},
        {"tcrt5000-module", AssemblyKind::Module, "TCRT5000 sensor module", "TCRT5000", nullptr,
         {nullptr, 0, sensorDevices, 1, sensorConnectors, 1, sensorConnections, 1}},
    };

    static const Assembly roots[] = {
        {"main", "mw-overnight-v1", "Main board"},
        {"led_strip", "ws2812-strip", "LED strip"},
#if DRAWER_1_PIN >= 0
        {"drawer_1", "tcrt5000-module", "Drawer 1 sensor"},
#endif
#if DRAWER_2_PIN >= 0
        {"drawer_2", "tcrt5000-module", "Drawer 2 sensor"},
#endif
#if DRAWER_3_PIN >= 0
        {"drawer_3", "tcrt5000-module", "Drawer 3 sensor"},
#endif
    };

    // Cross-assembly wiring: contact to contact only.
    static const Connection connections[] = {
        {{"main", EndpointKind::ConnectorContact, "j_led", "DIN"},
         {"led_strip", EndpointKind::ConnectorContact, "lead", "DIN"}},
#if DRAWER_1_PIN >= 0
        {{"main", EndpointKind::ConnectorContact, "j_drawer_1", "DO"},
         {"drawer_1", EndpointKind::ConnectorContact, "lead", "DO"}},
#endif
#if DRAWER_2_PIN >= 0
        {{"main", EndpointKind::ConnectorContact, "j_drawer_2", "DO"},
         {"drawer_2", EndpointKind::ConnectorContact, "lead", "DO"}},
#endif
#if DRAWER_3_PIN >= 0
        {{"main", EndpointKind::ConnectorContact, "j_drawer_3", "DO"},
         {"drawer_3", EndpointKind::ConnectorContact, "lead", "DO"}},
#endif
    };

    return {
        "main",
        definitions,
        sizeof(definitions) / sizeof(definitions[0]),
        roots,
        sizeof(roots) / sizeof(roots[0]),
        connections,
        sizeof(connections) / sizeof(connections[0]),
    };
}

} // namespace NMHardware
