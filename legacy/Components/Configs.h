#pragma once
#include <Arduino.h>
#include <Core/StateStore.h>

// Light automation settings, persisted through NightMare's PersistentSettings. They are exposed to
// the network as ManagedState resources in main.cpp; this struct only holds and stores them.

#define CONFIG_KEY_USE_EXTERNAL_LDR "sherlock.use_ext_ldr"
#define CONFIG_KEY_TIME_AUTOMATION "sherlock.time_auto"
#define CONFIG_KEY_WAKE_TIME "sherlock.wake"
#define CONFIG_KEY_SLEEP_TIME "sherlock.sleep"

struct Configs
{
    bool use_external_ldr = true;
    bool use_time_automation = true;
    String time_to_wake = "19:00";
    String time_to_sleep = "22:00";

    /// @brief Parses "HH:MM" into minutes since midnight.
    /// @return False when the text is not a valid 24-hour time.
    static bool parseClock(const String &text, int &minutesOfDay)
    {
        int colon = text.indexOf(':');
        if (colon < 1 || colon > 2 || text.length() != (unsigned)colon + 3)
            return false;
        for (unsigned i = 0; i < text.length(); i++)
        {
            if (i != (unsigned)colon && !isDigit(text[i]))
                return false;
        }
        int hours = text.substring(0, colon).toInt();
        int minutes = text.substring(colon + 1).toInt();
        if (hours > 23 || minutes > 59)
            return false;
        minutesOfDay = hours * 60 + minutes;
        return true;
    }

    /// @brief Loads stored values; anything missing or malformed keeps its default.
    void load()
    {
        use_external_ldr = PersistentSettings.get(CONFIG_KEY_USE_EXTERNAL_LDR, use_external_ldr ? "1" : "0") == "1";
        use_time_automation = PersistentSettings.get(CONFIG_KEY_TIME_AUTOMATION, use_time_automation ? "1" : "0") == "1";
        int unused;
        String wake = PersistentSettings.get(CONFIG_KEY_WAKE_TIME, time_to_wake);
        if (parseClock(wake, unused))
            time_to_wake = wake;
        String sleep = PersistentSettings.get(CONFIG_KEY_SLEEP_TIME, time_to_sleep);
        if (parseClock(sleep, unused))
            time_to_sleep = sleep;
    }

    // Each set() rewrites the settings file, so every setter persists only its own key.

    void setUseExternalLdr(bool value)
    {
        use_external_ldr = value;
        PersistentSettings.set(CONFIG_KEY_USE_EXTERNAL_LDR, value ? "1" : "0");
    }

    void setUseTimeAutomation(bool value)
    {
        use_time_automation = value;
        PersistentSettings.set(CONFIG_KEY_TIME_AUTOMATION, value ? "1" : "0");
    }

    void setWakeTime(const String &value)
    {
        time_to_wake = value;
        PersistentSettings.set(CONFIG_KEY_WAKE_TIME, value);
    }

    void setSleepTime(const String &value)
    {
        time_to_sleep = value;
        PersistentSettings.set(CONFIG_KEY_SLEEP_TIME, value);
    }
};

extern Configs Configuration;
