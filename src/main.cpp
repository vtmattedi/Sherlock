#include <Arduino.h>
#include <ArduinoJson.h>
#include <NightMare.h>
#include <board.h>

#include "Light/LightController.h"
#include "Sensors/Sensors.h"

namespace
{
    static const ActionArgMetadata SetArgs[] = {
        {"on", NetValueType::BOOLEAN, false},
    };
    ManagedAction setAction("set", SetArgs);

    ManagedSensor<float> temperature("temperature");
    ManagedSensor<float> dhtTemperature("dht_temperature");
    ManagedSensor<float> dhtHumidity("dht_humidity");
    ManagedSensor<bool> drawers[DRAWER_COUNT] = {
        ManagedSensor<bool>("drawer_1"),
        ManagedSensor<bool>("drawer_2"),
    };

    // Config<T> values are runtime configuration declarations. Automatic control is disabled by
    // default, while drawer handling is enabled.
    Config<TimeType> autoShowTime("autos:show:time", TimeType(18, 0));
    Config<ColourType> autoShowColor("auto:show:color", ColourType(255, 255, 0));
    Config<TimeType> autoHideTime("auto:hide:time", TimeType(6, 0));
    Config<bool> autoEnabled("auto:enable", false);
    Config<bool> drawersEnabled("drawers:enable", true);

    volatile bool configsDirty = true;
    volatile uint32_t configsChangedMs = 0;

    bool parsePowerRequest(const String &payload, bool &hasValue, bool &on)
    {
        String text = payload;
        text.trim();
        if (text.length() == 0)
        {
            hasValue = false;
            return true;
        }

        JsonDocument document;
        if (!deserializeJson(document, text) && document.is<JsonObject>())
        {
            JsonVariantConst value = document["on"];
            if (value.isNull())
            {
                hasValue = false;
                return document.as<JsonObjectConst>().size() == 0;
            }
            if (!value.is<bool>())
                return false;
            hasValue = true;
            on = value.as<bool>();
            return true;
        }

        text.toLowerCase();
        if (text == "toggle")
        {
            hasValue = false;
            return true;
        }
        if (text == "on" || text == "true" || text == "1")
        {
            hasValue = true;
            on = true;
            return true;
        }
        if (text == "off" || text == "false" || text == "0")
        {
            hasValue = true;
            on = false;
            return true;
        }
        return false;
    }

    ActionResult onSet(ManagedAction &, const String &payload)
    {
        bool hasValue = false;
        bool on = false;
        if (!parsePowerRequest(payload, hasValue, on))
            return {false, "expected no payload, on/off, or {\"on\":true|false}"};

        if (hasValue)
            gLight.setOn(on);
        else
            on = gLight.toggle();

        return {true, on ? "ON" : "OFF"};
    }

    bool onConfigChange(const String &key, const String &rawValue)
    {
        if (key == "auto:show:color")
        {
            ColourType color;
            if (!NetCodec<ColourType>::decode(rawValue, color))
                return false;
        }
        configsChangedMs = millis();
        configsDirty = true;
        return true;
    }

    bool bindResources()
    {
        setAction.onInvoke = onSet;

        bool ok = gLight.bindResources();
        ok = gResourcesManager.bindResource(&setAction) && ok;
        ok = gResourcesManager.bindResource(&temperature) && ok;
        ok = gResourcesManager.bindResource(&dhtTemperature) && ok;
        ok = gResourcesManager.bindResource(&dhtHumidity) && ok;
        for (uint8_t i = 0; i < DRAWER_COUNT; ++i)
            ok = gResourcesManager.bindResource(&drawers[i]) && ok;
        return ok;
    }

    template <typename Resource, typename T>
    void setIfChanged(Resource &resource, bool &initialized, const T &value)
    {
        if (initialized && resource.getValue() == value)
            return;
        if (resource.setValue(value))
            initialized = true;
    }

    void syncResources()
    {
        const SensorReadings readings = Sensors_get();
        static bool temperatureInitialized;
        static bool dhtTemperatureInitialized;
        static bool dhtHumidityInitialized;
        static bool drawerInitialized[DRAWER_COUNT];
        if (readings.temperatureValid)
            setIfChanged(temperature, temperatureInitialized, readings.temperature);
        if (readings.dhtValid)
        {
            setIfChanged(dhtTemperature, dhtTemperatureInitialized, readings.dhtTemperature);
            setIfChanged(dhtHumidity, dhtHumidityInitialized, readings.dhtHumidity);
        }
        for (uint8_t i = 0; i < DRAWER_COUNT; ++i)
            setIfChanged(drawers[i], drawerInitialized[i],
                         static_cast<bool>(readings.openDrawers & (1u << i)));
    }

    uint16_t minutesOfDay(const TimeType &time)
    {
        return static_cast<uint16_t>(time.hour()) * 60 + time.minute();
    }

    bool insideAutomaticWindow()
    {
        const time_t now = NightMare::Time::now();
        struct tm local;
        localtime_r(&now, &local);
        const uint16_t current = static_cast<uint16_t>(local.tm_hour) * 60 + local.tm_min;
        const uint16_t show = minutesOfDay(autoShowTime.value());
        const uint16_t hide = minutesOfDay(autoHideTime.value());

        if (show == hide)
            return true;
        if (show < hide)
            return current >= show && current < hide;
        return current >= show || current < hide;
    }

    void runAutomation(bool force)
    {
        static bool initialized;
        static bool lastWantedOn;

        if (!autoEnabled.value() || !NightMare::Time::valid())
        {
            initialized = false;
            return;
        }

        const bool wantedOn = insideAutomaticWindow();
        if (!force && initialized && wantedOn == lastWantedOn)
            return;

        initialized = true;
        lastWantedOn = wantedOn;
        if (wantedOn)
        {
            gLight.setColor(autoShowColor.value());
            gLight.setOn(true);
        }
        else
            gLight.setOn(false);
    }

    void applyConfigs(bool immediate = false)
    {
        if (!configsDirty)
            return;
        // ConfigManager calls the change handler immediately before committing the decoded value.
        // MQTT/console ingress may run on another task, so defer normal application until that commit
        // is certainly complete.
        if (!immediate && millis() - configsChangedMs < 20)
            return;
        configsDirty = false;
        gLight.setDrawersEnabled(drawersEnabled.value());
        runAutomation(true);
    }

    void onDrawerChange(uint8_t openMask)
    {
        gLight.setOpenDrawers(openMask);
    }

    void onWifiConnected(bool)
    {
        gLight.setColor(autoShowColor.value());
        gLight.setOn(false);
        gLight.blinkConnected();
        // Schedule a timeout to turn off the light after 10 seconds.
    }

    void onOta(OTA_INFO info, int data)
    {
        switch (info)
        {
        case OTA_START:
            gLight.showOtaProgress(0);
            break;
        case OTA_PROGRESS:
            gLight.showOtaProgress(static_cast<uint8_t>(constrain(data, 0, 100)));
            break;
        case OTA_ERROR:
            gLight.showOtaFailure();
            break;
        case OTA_END:
            gLight.showOtaSuccess();
            break;
        }
    }
} // namespace

#define VOLTAGE_INPUT_PIN 35
#define VOLTAGE_INPUT_INTERVAL_MS 5000
#define VOLTAGE_INPUT_SAMPLES 3
#define VOLTAGE_REF 3.3f
#define ADC_RESOLUTION 4095.0f
#define R1 3900.0f
#define R2 1000.0f
#define VOLTAGE_CAL 0.9923f
ManagedSensor<float> mvInput("Board Voltage");
Config<int> voltageInputPeriod("voltage:period", VOLTAGE_INPUT_INTERVAL_MS, false);
void readVoltageInput()
{
    int raw_read = 0;
    uint32_t mvSum = 0;

    for (int i = 0; i < VOLTAGE_INPUT_SAMPLES; i++)
    {
        mvSum += analogReadMilliVolts(VOLTAGE_INPUT_PIN);
        delayMicroseconds(100);
    }

    float adcVoltage =
        (mvSum / (float)VOLTAGE_INPUT_SAMPLES);

    float voltage2 =
        adcVoltage * ((R1 + R2) / R2) * VOLTAGE_CAL / 1000.0f; // Convert to volts

    mvInput.setValue(voltage2);
}

void setup()
{
    introNightMareESP();

    if (!gLight.begin())
        LOG_ERROR("Sherlock", "could not start the LED strip");

    configManager().setChangeHandler(onConfigChange);
    WiFi_onConnected(onWifiConnected);
    onOTAEvent(onOta);
    Sensors_onDrawerChange(onDrawerChange);
    Sensors_begin();

    if (!bindResources())
        LOG_ERROR("Sherlock", "one or more resources failed to bind");

    applyConfigs(true);
    syncResources();
    pinMode(VOLTAGE_INPUT_PIN, INPUT);
    bool ok = gResourcesManager.bindResource(&mvInput);
    LOG("Sherlock", "Millivolt input resource setup: %s", OK_LOG(ok));
    startNightMareESP();
    gLight.showStartup();
    gScheduler.everyMonotonic("voltage_input", readVoltageInput, voltageInputPeriod.value());
    voltageInputPeriod.onWrite = [](Config<int> &config, const int &requested)
    {
        if (requested < 5 || requested > 60000)
            return false;
        bool ok = gScheduler.remove("voltage_input");
        if (!ok)
        {
            LOG_ERROR("Sherlock", "failed to remove voltage_input job for period change");
            return false;
        }
        ok = gScheduler.everyMonotonic("voltage_input", readVoltageInput, requested);
        if (!ok)
        {
            LOG_ERROR("Sherlock", "failed to add voltage_input job for period change");
            return false;
        }
        return true;
    };
}

void loop()
{
    tickNightMareESP();
    applyConfigs();

    static uint32_t lastAutomationMs;
    if (millis() - lastAutomationMs >= 1000)
    {
        lastAutomationMs = millis();
        runAutomation(false);
    }

    syncResources();
    delay(20);
}
