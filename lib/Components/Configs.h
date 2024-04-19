#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#define EXTERNAL_LDR_TOPIC "Adler/sensors/ldr"
#define CONFIG_FILENAME "/configs.cfg" // the default name of the configuration file.

extern void MqttSend(String,String,bool,bool);


struct Internals
{
    bool time_synced = false;
    uint boot_time = 0;
    bool LittleFS_Mounted = false;
    bool updating = false;
    byte on_brightness = 127;
    byte oldBrightness = 0;
    bool user_override = false;
    bool tcp_state = false;
    uint mqtt_update = 30;
    uint last_mqtt_reconnect_attemp = 0;
    bool mqtt_state = false;
    bool tcp_debug = false;
    bool time_automation_wake_flag = false;
    bool time_automation_sleep_flag = false;
    uint32_t last_state_sent = 0;
    uint16_t last_external_ldr = 4095;
    bool current_light_state = false;

    String toString()
    {
        String result = "time_synced: " + String(time_synced) + ";boot_time: " + String(boot_time) + ";life: " + String(millis() - boot_time);
        return result;
    }
} control_variables;

struct Configs
{
    bool use_external_ldr = true;
    String external_ldr_topic = EXTERNAL_LDR_TOPIC;
    bool use_time_automation = true;
    bool use_ldr_over_time = false;
    String time_to_wake = "19:00";
    String time_to_sleep = "22:00";

    String ToString()
    {
        String result;
        result += "external_ldr_topic:" + external_ldr_topic + ";";
        result += "use_external_ldr:" + String(use_external_ldr) + ";";
        result += "use_time_automation:" + String(use_time_automation) + ";";
        result += "use_ldr_over_time:" + String(use_ldr_over_time) + ";";
        result += "time_to_wake:" + time_to_wake + ";";
        result += "time_to_sleep:" + time_to_sleep + ";";
        return result;
    }

    /// @brief Loads the values with a previously saved configuration String.
    /// @param data a String that represents the configuration.
    void FromString(const String &data)
    {
        uint8_t pos = 0;
        while (pos < data.length())
        {
            int colonIndex = data.indexOf(':', pos);
            if (colonIndex == -1)
                break;

            int semicolonIndex = data.indexOf(';', colonIndex);
            if (semicolonIndex == -1)
                semicolonIndex = data.length();

            String key = data.substring(pos, colonIndex);
            String value = data.substring(colonIndex + 1, semicolonIndex);

            if (key.equals("external_ldr_topic"))
                external_ldr_topic = value;
            else if (key.equals("use_external_ldr"))
                use_external_ldr = (value.toInt() != 0);
            else if (key.equals("use_time_automation"))
                use_time_automation = (value.toInt() != 0);
            else if (key.equals("use_ldr_over_time"))
                use_ldr_over_time = (value.toInt() != 0);
            else if (key.equals("time_to_wake"))
                time_to_wake = value;
            else if (key.equals("time_to_sleep"))
                time_to_sleep = value;

            pos = semicolonIndex + 1;
        }

        String s = ToString();
#ifdef COMPILE_SERIAL
        s.replace(";", "\n");
        Serial.printf("--------NEW CONFIG--------\n%s\n---------------------", s.c_str());
#endif
    }

    /// @brief Saves the configuration to a file using LittleFS.
    /// @param filename The configuration filename.
    /// @return True if LittleFs was mount and therefore file was saved and false if LittleFS was not mounted.
    bool SaveToFile(const char *filename = CONFIG_FILENAME)
    {
        if (!control_variables.LittleFS_Mounted)
            return false;

        if (LittleFS.exists(filename))
            LittleFS.remove(filename);
        File configfile = LittleFS.open(filename, "w");
        configfile.print(this->ToString());
        configfile.flush();
        configfile.close();
        SendToMqtt();
        return true;
    }

    /// @brief Loads the configuration from a file using LittleFS.
    /// @param filename The configuration filename.
    /// @return False if the is no file with that name or LittleFS was not mounted. True otherwise.
    bool LoadFromFile(const char *filename = CONFIG_FILENAME)
    {
        if (!control_variables.LittleFS_Mounted)
            return false;
        if (!LittleFS.exists(filename))
            return false;
        File configfile = LittleFS.open(filename, "r");
        FromString(configfile.readString());
        SendToMqtt();
        return true;
    }

    void SendToMqtt()
    {
        MqttSend("/config", ToString(),true,false);
    }
} Configuration;
