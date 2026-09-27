#include "Sensors.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHTesp.h>
#include <NightMare.h>
#include <board.h>

/// The DHT11 needs at least a second between reads; the room does not change faster than this.
#define CLIMATE_INTERVAL_MS 5000
/// Bit-banged on this task, with interrupts off for the read, so it stays off the LED task's core.
#define SENSORS_TASK_STACK 4096
#define SENSORS_TASK_PRIORITY 1
#define SENSORS_TASK_CORE 0

namespace
{
    const int8_t DrawerPins[DRAWER_COUNT] = {DRAWER_1_PIN, DRAWER_2_PIN};

    OneWire oneWire(ONE_WIRE_BUS);
    DallasTemperature ds18(&oneWire);
    DeviceAddress ds18Address;
    bool ds18Found = false;
    DHTesp dht;

    StaticSemaphore_t mutexStorage;
    SemaphoreHandle_t mutex = nullptr;
    SensorReadings shared;
    volatile uint16_t rawLevels[DRAWER_COUNT] = {};
    DrawerChangeCallback drawerCallback = nullptr;
    TaskHandle_t task = nullptr;

    // Sampling-task state: nothing else touches these.
    bool drawerOpen[DRAWER_COUNT] = {};
    uint8_t drawerAgree[DRAWER_COUNT] = {};

    struct Lock
    {
        Lock() { xSemaphoreTake(mutex, portMAX_DELAY); }
        ~Lock() { xSemaphoreGive(mutex); }
        Lock(const Lock &) = delete;
        Lock &operator=(const Lock &) = delete;
    };

    bool readDrawer(uint8_t i)
    {
        #define threshhold 5
        int sum = 0;
        for (uint8_t j = 0; j < 10; j++)
            sum += readDrawer(i);
        return sum > threshhold;
    }

    uint8_t openMask()
    {
        uint8_t mask = 0;
        for (uint8_t i = 0; i < DRAWER_COUNT; i++)
            mask |= drawerOpen[i] ? (1u << i) : 0;
        return mask;
    }

    void publishDrawers()
    {
        const uint8_t mask = openMask();
        {
            Lock lock;
            shared.openDrawers = mask;
        }
        if (drawerCallback != nullptr)
            drawerCallback(mask);
    }

    /// The first reading has no history to debounce against, so it is taken as it is.
    void primeDrawers()
    {
        for (uint8_t i = 0; i < DRAWER_COUNT; i++)
        {
            if (DrawerPins[i] < 0)
                continue;
            rawLevels[i] = readDrawer(i);
            drawerOpen[i] = rawLevels[i] == DRAWER_OPEN_LEVEL;
        }
        publishDrawers();
    }

    void sampleDrawers()
    {
        bool changed = false;
        for (uint8_t i = 0; i < DRAWER_COUNT; i++)
        {
            if (DrawerPins[i] < 0)
                continue;
            const uint16_t raw = readDrawer(i);
            rawLevels[i] = raw;
            const bool wantOpen = raw == DRAWER_OPEN_LEVEL;
            if (wantOpen == drawerOpen[i])
            {
                drawerAgree[i] = 0;
            }
            else if (++drawerAgree[i] >= DRAWER_DEBOUNCE_SAMPLES)
            {
                drawerAgree[i] = 0;
                drawerOpen[i] = wantOpen;
                changed = true;
            }
        }
        if (changed)
            publishDrawers();
    }

    void sampleClimate()
    {
        SensorReadings next;
        bool ds18Ok = false;
        float tempC = 0;
        if (ds18Found)
        {
            // Read the conversion requested last time round, then start the next one: the bus is
            // never held for the ~400 ms an 11-bit conversion takes.
            tempC = ds18.getTempC(ds18Address);
            ds18Ok = tempC != DEVICE_DISCONNECTED_C;
            ds18.requestTemperaturesByAddress(ds18Address);
        }

        const TempAndHumidity reading = dht.getTempAndHumidity();
        const bool dhtOk = dht.getStatus() == DHTesp::ERROR_NONE;
        if (!dhtOk)
            LOG_DEBUG("Sensors", "DHT: %s", dht.getStatusString());

        Lock lock;
        // A failed read keeps the last good value: valid stays true once a reading exists.
        if (ds18Ok)
        {
            shared.temperatureValid = true;
            shared.temperature = tempC;
        }
        if (dhtOk)
        {
            shared.dhtValid = true;
            shared.dhtTemperature = reading.temperature;
            shared.dhtHumidity = reading.humidity;
        }
    }

    void run(void *)
    {
        uint32_t lastClimateMs = millis();
        TickType_t wake = xTaskGetTickCount();
        for (;;)
        {
            sampleDrawers();
            if (millis() - lastClimateMs >= CLIMATE_INTERVAL_MS)
            {
                lastClimateMs = millis();
                sampleClimate();
            }
            vTaskDelayUntil(&wake, pdMS_TO_TICKS(DRAWER_SAMPLE_MS));
        }
    }
}

bool Sensors_begin()
{
    if (task != nullptr)
        return true;
    mutex = xSemaphoreCreateMutexStatic(&mutexStorage);

    ds18.begin();
    ds18Found = ds18.getAddress(ds18Address, 0);
    if (ds18Found)
    {
        ds18.setResolution(ds18Address, 11);
        ds18.setWaitForConversion(false);
        ds18.requestTemperaturesByAddress(ds18Address);
    }
    else
        LOG_ERROR("Sensors", "No DS18B20 found on GPIO%d", ONE_WIRE_BUS);

    dht.setup(DHT_PIN, DHTesp::DHT11);

    for (uint8_t i = 0; i < DRAWER_COUNT; i++)
    {
        if (DrawerPins[i] >= 0)
            pinMode(DrawerPins[i], INPUT);
    }
    primeDrawers();

    if (xTaskCreatePinnedToCore(run, "sensors", SENSORS_TASK_STACK, nullptr, SENSORS_TASK_PRIORITY,
                                &task, SENSORS_TASK_CORE) != pdPASS)
    {
        LOG_ERROR("Sensors", "could not start the sampling task");
        return false;
    }
    return true;
}

void Sensors_onDrawerChange(DrawerChangeCallback callback)
{
    drawerCallback = callback;
}

SensorReadings Sensors_get()
{
    if (mutex == nullptr)
        return SensorReadings();
    Lock lock;
    return shared;
}

bool Sensors_drawerMonitored(uint8_t index)
{
    return index < DRAWER_COUNT && DrawerPins[index] >= 0;
}

uint16_t Sensors_drawerRaw(uint8_t index)
{
    return index < DRAWER_COUNT ? rawLevels[index] : 0;
}

bool Sensors_ds18Connected()
{
    return ds18Found;
}
