#include <../lib/pubsubclient-2.8/src/PubSubClient.h>
#include <../include/Creds/WifiCred.h>
#include <../include/Creds/HiveMQCred.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include "esp_task_wdt.h"
#include <LittleFS.h>
#include <WebServer.h>
#include "DHTesp.h"
#include <FastLED.h>
#include <../lib/Time-master/TimeLib.h>
#include <NightMareTCP.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
StaticJsonDocument<256> DocJson;
#define DEVICE_NAME "Sherlock"
#define SERVER_HOST "Feymann"
#define FASTLED_INTERNAL
#define EXTERNAL_LDR_TOPIC "Adler/sensors/ldr"
#define LDR_THERSHHOLD 3800
#define LDR_ON_LOW true
NightMareTCPServer tcpServer(100);
NightMareTCPClient tcpClient(DEVICE_NAME, false);
void handle_gateway(const char *, String, bool);

#define COMPILE_SERIAL
#define USE_OTA

// DS1820 Stuff
#define ONE_WIRE_BUS 16
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);
DeviceAddress sensorAddress;
DHTesp dht;
#define dht_type DHTesp::DHT11
#define dht_pin 17
void MqttSend(String topic, String message, bool insertOwner = true, bool retained = false);
#define serverPort 80 // Web server port (default is 80)
WebServer server(serverPort);
double _current_temp = 0;
double _dht_temp = 0;
double _dht_hum = 0;
String _dht_info = "";
void HiveMQ_Callback(const char *, String);

// TCP Definitions
WiFiClientSecure hive_client;
PubSubClient HiveMQ(hive_client);

#define LED_STRIP_PIN 18
#define LED_STRIP_SIZE 102
CRGB leds[LED_STRIP_SIZE] = {0};

enum function
{
    Solid,
    ShowHandles,
    lightShow,
    randomLights,
    rainbow,
    laser
};

function getFnbyName(String fnName)
{
    if (fnName == "Solid")
        return Solid;
    if (fnName == "ShowHandles")
        return ShowHandles;
    if (fnName == "lightShow")
        return lightShow;
    if (fnName == "randomLights")
        return randomLights;
    if (fnName == "rainbow")
        return rainbow;
    if (fnName == "laser")
        return laser;
    return Solid;
}

String getfnName(int fnValue)
{
    switch (fnValue)
    {
    case Solid:
        return "Solid";
    case ShowHandles:
        return "ShowHandles";
    case lightShow:
        return "lightShow";
    case randomLights:
        return "randomLights";
    case rainbow:
        return "rainbow";
    case laser:
        return "laser";
    default:
        return "Unknown";
    }
}

String getFns()
{
    String enumString = "";
    bool done = false;
    for (int i = 0; !done; i++)
    {
        String current_fn = getfnName(i);
        if (current_fn == "Unknown")
            done = true;
        else
        {
            enumString += current_fn;
            enumString += ",";
            enumString += i;
            enumString += "+";
        }
    }
    return enumString;
}

enum direction
{
    backward = -1,
    stop = 0,
    foward = 1,
};

direction opposite(direction currentDirection)
{
    switch (currentDirection)
    {
    case backward:
        return foward;
    case foward:
        return backward;
    default:
        return stop;
    }
}

enum option
{
    none,
    blink,
    fade,
};

struct LedHandler
{
#define NUM_LEDS 102
private:
    unsigned long _lastMillis = 0;
    int _Index1 = 0;
    int _Index2 = 0;
    int _start_index = 0;
    int _end_index = NUM_LEDS;
    int _num_of_leds = NUM_LEDS;
    int _interval = 25;
    bool _helperFlag = false;
    int _helperInt0 = 0;
    int _helperInt1 = 0;
    byte _helperByte = 0;
    bool _halt = false;
    bool _finite = false;
    bool _show = true;
    uint32_t _finite_timestamp = 0;

    int _old_interval = 25;
    CRGB _old_baseColor = CRGB::Red;
    CRGB _old_baseColor2 = CRGB::Blue;
    function _old_fn = function::Solid;
    direction _old_movement = direction::stop;
    option _old_options = option::none;

public:
    function current_fn = function::Solid;
    CRGB baseColor = CRGB::GreenYellow;
    CRGB baseColor2 = CRGB::Blue;
    direction movement = direction::stop;
    option options = option::none;
    void setBoundries(int start, int finish)
    {
        for (size_t i = 0; i < NUM_LEDS; i++)
        {
            leds[i] = 0;
        }

        if (finish < start)
        {
            int middleman = finish;
            finish = start;
            start = middleman;
        }
        if (start < 0)
            start = 0;
        if (finish >= NUM_LEDS)
            finish = NUM_LEDS - 1;

        _start_index = start;
        _end_index = finish;
        _num_of_leds = _end_index - _start_index + 1;
    }
    void setInterval(int newInterval, int beatsPerMinute = 0)
    {
        if (newInterval < 10)
        {
            newInterval = 10;
        }
        _interval = newInterval;
        Serial.printf("new interval: %d\n", newInterval);
    }
    void setFinite(uint32_t time_to_finish_ms)
    {
        _finite = true;
        _finite_timestamp = millis() + time_to_finish_ms;
    }
    void setOption(option newOption)
    {
        options = newOption;
    }
    void setDirection(direction newDirection)
    {
        movement = newDirection;
    }
    void goToLastMode()
    {
        direction temp_mov = _old_movement;
        option temp_opt = _old_options;

        setMode(_old_fn, _old_baseColor, _old_baseColor2);
        setDirection(temp_mov);
        setOption(_old_options);
    }
    void setMode(function new_fn)
    {
        setMode(new_fn, baseColor);
    }
    void setMode(function new_fn, CRGB newColor1, CRGB newColor2 = CRGB::Black)
    {
        _halt = false;
        _old_fn = current_fn;
        _old_baseColor = baseColor;
        _old_baseColor2 = baseColor2;
        _old_movement = movement;
        _old_options = options;
        options = none;
        movement = stop;
        current_fn = new_fn;
        baseColor = newColor1;
        baseColor2 = newColor2;
        _lastMillis = 0;
        _helperFlag = false;
        _helperInt0 = 0;
        _helperInt1 = 0;
        _helperByte = 0;
        _show = true;
        FastLED.clear();
    }
    void showProgbar(byte percentage, CRGB ValueColor = CRGB::DarkCyan, CRGB BackColor = CRGB::DarkGoldenrod)
    {
        _halt = true;
        byte percentIndex = round(percentage * _end_index / 100);
        for (size_t i = _start_index; i <= _end_index; i++)
        {
            if (i <= percentIndex)
                leds[i] = ValueColor;
            else
                leds[i] = BackColor;
        }
        FastLED.show();
    }

    void partialMove(direction dir, int startIndex, int endIndex, bool loop = true)
    {
        // Serial.printf("Start P.M, d = %d, s = %d, e = %d,\n", dir, startIndex, endIndex);
        // Serial.println("");
        if (dir == foward)
        {
            CRGB first = leds[startIndex];

            for (size_t i = startIndex; i < endIndex; i++)
            {
                // Serial.printf("[%d] - > [%d]\n", i, i + 1);
                leds[i] = leds[i + 1];
            }
            if (loop)
                leds[endIndex] = first;
        }
        else if (dir == backward)
        {
            CRGB last = leds[endIndex];

            for (size_t i = 0; i < endIndex - startIndex; i++)
            {
                // Serial.printf("[%d] - > [%d]\n", endIndex - i, endIndex - (i + 1));
                leds[endIndex - i] = leds[endIndex - (i + 1)];
            }
            if (loop)
                leds[startIndex] = last;
        }

        return;
    }
    void move(direction dir, bool loop = true)
    {
        // partialMove(dir, _start_index, _end_index, loop);
        // return;
        if (dir == foward)
        {
            CRGB first = leds[_start_index];

            for (size_t i = _start_index; i < _end_index; i++)
            {
                leds[i] = leds[i + 1];
            }
            if (loop)
                leds[_end_index] = first;
        }
        else if (dir == backward)
        {
            CRGB first = leds[_end_index];

            for (size_t i = 0; i < _end_index - _start_index; i++)
            {
                leds[_end_index - i] = leds[_end_index - (i + 1)];
            }
            if (loop)
                leds[_start_index] = first;
        }
    }

    uint32_t getHex(CRGB rgb)
    {
        return rgb.red * 0x10000 + rgb.green * 0x100 + rgb.blue;
    }
    void run()
    {
        if (millis() >= _finite_timestamp && _finite)
            goToLastMode();
        if (millis() < _lastMillis + _interval || _halt)
            return;
        _lastMillis = millis();
        if (current_fn == Solid)
        {
            if (options == fade)
            {
                _show = true;
                if (_helperInt0 == 0)
                {
                    for (size_t i = _start_index; i < _end_index; i++)
                    {
                        leds[i] = baseColor;
                    }
                }
                else
                {
                    fadeToBlackBy(leds, NUM_LEDS, 16);
                }
                _helperInt0++;
                if (_helperInt0 == 16)
                    _helperInt0 = 0;
            }
            else if (options == blink)
            {
                _show = true;
                _helperFlag = !_helperFlag;
                for (size_t i = _start_index; i < _end_index; i++)
                {
                    leds[i] = _helperFlag ? baseColor : baseColor2;
                }
            }
            else
            {
                _show = false;
                for (size_t i = _start_index; i < _end_index; i++)
                {
                    if (leds[i] != baseColor)
                    {
                        leds[i] = baseColor;
                        _show = true;
                    }
                }
            }
        }
        else if (current_fn == lightShow)
        {
            fadeToBlackBy(leds, _end_index, 32);
            leds[_start_index + _helperInt0] = _helperFlag ? baseColor : baseColor2;
            leds[_end_index - _helperInt0] = _helperFlag ? baseColor2 : baseColor;
            blur1d(leds, _end_index, 64);
            _helperInt0++;
            if (_start_index + _helperInt0 > _end_index)
            {
                _helperInt0 = 0;
                _helperFlag = !_helperFlag;
                if (_helperFlag = true)
                {
                    baseColor = CHSV(random8(), 255, 255);
                    baseColor2 = CHSV(random8(), 255, 255);
                }
            }
        }
        else if (current_fn == ShowHandles)
        {
            fadeToBlackBy(leds, _num_of_leds, 8);
            _helperInt0++;
            if (_helperInt0 >= 8)
                move(movement);
            if (_helperInt0 >= 40)
            {
                uint8_t newhue = random8();
                _helperInt0 = 0;
                for (size_t i = 0; i < 10; i++)
                {
                    leds[19 + i] = CHSV(newhue, 255, 255);

                    leds[50 + i] = CHSV(newhue + 85, 255, 255);

                    leds[83 + i] = CHSV(newhue + 170, 255, 255);
                }
            }
        }
        else if (current_fn == randomLights)
        {
            fadeToBlackBy(leds, _num_of_leds, 8);
            move(movement);
            leds[random(7, 102)] = CHSV(random8(), 255, 255);
        }
        else if (current_fn == rainbow)
        {
            int midIndex = (_end_index - _start_index) / 2;
            partialMove(movement, _start_index, midIndex);
            partialMove(opposite(movement), midIndex + 1, _end_index);
            if (_helperInt0 == 0)
            {
                int8_t startHue = random8();
                for (size_t i = _start_index; i <= _end_index; i++)
                {
                    leds[i] = CHSV(startHue + i * 4, 255, 255);
                }
                blur1d(leds, NUM_LEDS, 32);
                _helperInt0++;
            }
            else
            {
                if (movement == stop)
                    _show = false;
                else
                    _show = true;
            }
        }
        else if (current_fn == laser)
        {
            move(movement == stop ? backward : movement, false);
            if (_helperInt0 == 0)
                _helperByte = random(1, 8);

            if (movement == foward)
            {
                leds[_end_index] = _helperInt0 < _helperByte ? baseColor : baseColor2;
            }
            else
            {
                leds[_start_index] = _helperInt0 < _helperByte ? baseColor : baseColor2;
            }
            //  blur1d(leds, 102, 16);

            _helperInt0++;
            if (_helperInt0 >= random(32, 64))
                _helperInt0 = 0;
        }
        if (_show)
            FastLED.show();
    }
};

LedHandler ledController;

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

    bool use_time_automation = true;
    String time_to_wake = "19:00";
    String time_to_sleep = "22:00";
    bool time_automation_wake_flag = false;
    bool time_automation_sleep_flag = false;
    uint32_t last_state_sent = 0;

    uint16_t last_external_ldr = 4095;
    bool use_external_ldr = true;
    String external_ldr_topic = EXTERNAL_LDR_TOPIC;
    bool current_light_state = false;

    String toString()
    {
        String result = "time_synced: " + String(time_synced) + ";boot_time: " + String(boot_time) + ";life: " + String(millis() - boot_time);
        return result;
    }
} control_variables;

struct Timers_Last_Values
{
    uint temperature = 0;
    uint sync_time = 0;
    uint mqtt_publish = 0;
    uint mqtt_reconnect = 0;
    uint Tasks = 0;
    uint soft_ldr;
    uint Automation_resume = 0;
};
Timers_Last_Values Timers;

void send_state_to_mqtt()
{
    DocJson.clear();
    DocJson["Automation"] = control_variables.user_override ? Timers.Automation_resume : 0;
    DocJson["State"] = control_variables.current_light_state;
    DocJson["Color"] = String(ledController.getHex(ledController.baseColor)).c_str();
    DocJson["Brightness"] = control_variables.on_brightness;
    String msg;
    serializeJson(DocJson, msg);
    MqttSend("/state", msg);
    control_variables.last_state_sent = now();
}

/// @brief Handles Led turning on and off and syncs thourghout the network
/// @param turnon Wheater or not you want to turn on the lights
/// @param force If you want to force the state or if is from an automatic sensor value
/// @param user_input If it was an user input and thefore the system should handle it as such or an automatic one
void handleLedAutomation(bool turnon, bool force = false, bool user_input = false)
{
    if (control_variables.updating)
        return;

    if (user_input)
    {
        control_variables.user_override = true;
        Timers.Automation_resume = now() + 60 * 60; // resume in one hour
    }
    if (!(control_variables.user_override && !force))
    {
        FastLED.setBrightness(control_variables.on_brightness * turnon);
        control_variables.current_light_state = turnon;
    }

    send_state_to_mqtt();
    // MqttSend("/debug", String(turnon) + " : " + String(force) + " : " + String(user_input) + " @ " + String(control_variables.on_brightness));
    FastLED.show();
}

bool CompareTime(String begin_time, String end_time)
{
    bool begin = false;
    bool end = false;
    bool end_is_bigger = false;
    int _hour = atoi(begin_time.substring(0, begin_time.indexOf(":")).c_str());
    int _minute = atoi(begin_time.substring(begin_time.indexOf(":") + 1).c_str());

    if ((hour() > _hour) || (hour() >= _hour && minute() >= _minute))
        begin = true;

    int _hour_end = atoi(end_time.substring(0, end_time.indexOf(":")).c_str());
    if (_hour_end > _hour)
        end_is_bigger = true;
    _minute = atoi(end_time.substring(end_time.indexOf(":") + 1).c_str());
    if ((hour() < _hour_end) || (hour() <= _hour_end && minute() <= _minute))
        end = true;

    if (end_is_bigger)
        return begin && end;
    return begin || end;
}
struct ColorTask
{
    String label = "";
    CRGB baseColor = 00;
    CRGB baseColor2 = 00;
    function fn = Solid;
    option op = none;
    direction movement = stop;
    int interval = -1;
    int _hour = -1;
    int _minute = -1;
    bool done = false;
    bool Compare()
    {
        if (done)
            return false;
        if (hour() == _hour && minute() == _minute)
            return true;
        return false;
    }

    void Execute()
    {
        if (baseColor == CRGB(0))
            ledController.setMode(fn);
        else if (baseColor2 == CRGB(0))
            ledController.setMode(fn, baseColor);
        else
            ledController.setMode(fn, baseColor, baseColor2);

        if (interval > 0)
            ledController.setInterval(interval);
        ledController.setDirection(movement);
        ledController.setOption(op);
        done = true;
    }
    void Rearm()
    {
        done = false;
    }

    void reset()
    {
        label = "";
        baseColor = 0;
        baseColor2 = 0;
        fn = Solid;
        op = none;
        movement = stop;
        interval = -1;
        _hour = -1;
        _minute = -1;
        done = false;
    }
};

/// @brief Gets the MQTT topic in the proper format ie. with 'DEVICE_NAME/' before the topic.
/// @param topic The topic without the owner prefix.
/// @return The topic with the owner prefix.
String MqttTopic(String topic)
{
#ifndef DEVICE_NAME
#error Please define device name
#endif
    String topicWithOwner = "";
    topicWithOwner += DEVICE_NAME;
    if (topic != "" || topic != NULL)
    {
        if (topic[0] != '/')
            topicWithOwner += "/";
        topicWithOwner += topic;
    }
    return topicWithOwner;
}

/// @brief Sends an message to the Mqtt Server
/// @param topic The topic to the message
/// @param message The message
/// @param insertOwner Use the MqttTopic function to insert device's name before the topic.
/// @param retained Retained message or normal
void MqttSend(String topic, String message, bool insertOwner, bool retained)
{
    if (insertOwner)
        topic = MqttTopic(topic);
    if (control_variables.tcp_state)
        tcpClient.send("topic::" + topic + "payload::" + message);
    else if (control_variables.mqtt_state)
        HiveMQ.publish(topic.c_str(), message.c_str(), retained);
}

#pragma region OTA
void startOTA()
{
    String type;
    // is_updating = true;
    //  caso a atualização esteja sendo gravada na memória flash externa, então informa "flash"
    if (ArduinoOTA.getCommand() == 0)
        type = "flash";
    else                     // caso a atualização seja feita pela memória interna (file system), então informa "filesystem"
        type = "filesystem"; // U_SPIFFS
    // exibe mensagem junto ao tipo de gravação
    Serial.println("Start updating " + type);
    control_variables.updating = true;
}
// exibe mensagem
void endOTA()
{
    Serial.println("\nEnd");
    ledController.setMode(Solid, CRGB::Green);
    ledController.setOption(blink);
    ledController.setInterval(50);
    control_variables.updating = false;
}
// exibe progresso em porcentagem
void progressOTA(unsigned int progress, unsigned int total)
{
    byte update_progress = round((float)progress / total * 100);
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
    ledController.showProgbar(update_progress, CRGB::DarkGreen);
}

void errorOTA(ota_error_t error)
{
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
        Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
        Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
        Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
        Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
        Serial.println("End Failed");
    ledController.setMode(Solid, CRGB::Red);
    ledController.setOption(blink);
    ledController.setInterval(200);
    ledController.setFinite(2000);
    control_variables.updating = false;
}
#pragma endregion

void manual(int start, int finish = -1)
{
#define MCOLOR CRGB::Amethyst

    Serial.printf("manual %d - %d\n", start, finish);
    if (start > finish && finish >= 0)
    {
        int t = finish;
        finish = start;
        start = t;
    }
    if (start < 0 || start >= NUM_LEDS || finish >= NUM_LEDS)
        return;
    for (size_t i = 0; i < NUM_LEDS; i++)
    {
        leds[i] = CRGB::Black;
    }
    if (finish <= 0 || start == finish)
        leds[start] = MCOLOR;
    else
    {
        for (size_t i = start; i <= finish; i++)
        {
            leds[i] = MCOLOR;
        }
    }
    FastLED.show();
}

String HandleTcpClientMessage(String msg)
{
    int index = msg.indexOf(",");
    if (index > 0)
    {
        handle_gateway(msg.substring(0, index).c_str(), msg.substring(index + 1), false);
    }
    else
        Serial.printf("{%s}\n", msg.c_str());
    return "";
}

String HandleTcpServertMessage(String msg, byte index)
{
    Serial.print("[");
    Serial.print(tcpServer.clients[index].client->remoteIP());
    Serial.printf("]:%d ", tcpServer.clients[index].client->remotePort());
    Serial.print((char)39);
    Serial.print(msg);
    Serial.println((char)39);
    int msgAsInt = atoi(msg.c_str());
    if (msg == "mqtt")
    {
        String s = "mqtt = ";
        s += HiveMQ.state();
        s += ";";
        s += MqttTopic("#");
        s += " tcp:";
        s += tcpClient.connected();
        s += "  | [soft]: ";
        s += control_variables.mqtt_state;
        s += "  | ";
        s += control_variables.tcp_state;
        return s;
    }
    if (msg == "f")
    {
        ledController.movement = foward;
        return "foward";
    }
    else if (msg == "b")
    {
        ledController.movement = backward;
        return "backward";
    }
    else if (msg == "s")
    {
        ledController.movement = direction::stop;
        return "stop";
    }
    else if (msg == "debug")
    {
        control_variables.tcp_debug = !control_variables.tcp_debug;
        return String("debug: [" + String(control_variables.tcp_debug) + "]");
    }
    int _index = msg.indexOf("c::");
    if (_index >= 0)
    {
        String sub = msg.substring(_index + 3, msg.indexOf(";"));
        ledController.setMode(Solid, CHSV(map(sub.toInt(), 0, 359, 0, 255), 255, 255));
        return "new color: " + String(map(sub.toInt(), 0, 359, 0, 255));
    }

    return msg;
}

void handleIndex()
{
    String msg = R"==( 
     <!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Sherlock</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
        }

        #color-picker {
            margin-top: 20px;
        }

        #animation-slider {
            width: 80%;
            margin-top: 20px;
        }

        #color-box {
            width: 100px;
            height: 100px;
            margin: 20px auto;
            transition: background-color 0.5s ease, opacity 0.5s ease;
        }
    </style>
</head>

<body>
    <h1>I am SherLOCKED</h1>

    <label for="color-picker">Choose a color:</label>
    <input type="color" id="color-picker">

    <p>Selected Color: <span id="selected-color">#000000</span></p>

    <label for="animation-slider">Speed:</label>
    <input type="range" id="animation-slider" min="10" max="1000" value="500" onchange="sendSpeed(this.value)">

    <p>Choose Animation:</p>
    <label for="fade-toggle">Fade</label>
    <input type="radio" id="fade-toggle" name="animation-type" value="fade" checked>
    <label for="blink-toggle">Blink</label>
    <input type="radio" id="blink-toggle" name="animation-type" value="blink">
    <label for="none-toggle">None</label>
    <input type="radio" id="none-toggle" name="animation-type" value="none">
    <select id="LedAnimations" onchange="sendAnimation()">
    </select>
    <div>
        <button onclick="sendMov(1)">Fowards</button>
        <button onclick="sendMov(0)">Stop</button>
        <button onclick="sendMov(-1)">Backwards</button>
    </div>
    <div id="color-box"></div>

    <script>
        function fetchData() {
            fetch('/general?get-modes=1')
                .then(response => response.text())
                .then(data => {
                    var list = [];
                    var pairs = data.split("+");
                    pairs.forEach(pair => {
                        if (pair != "") {
                            var obj = {
                                'name': pair.split(",")[0],
                                'value': pair.split(",")[1]
                            };
                            list.push(obj);
                        }

                    });
                    setSensors(list);
                })
                .catch(error => {
                    console.error('Error fetching data:', error);
                });
        }
        function setSensors(sensorlist) {

            const topicDiv = document.getElementById("LedAnimations");
            topicDiv.innerHTML = "";
            sensorlist.forEach(sensor => {
                if (sensor.value != "") {
                    var opt = document.createElement('option');
                    opt.value = sensor.value;
                    opt.innerHTML = sensor.name;
                    topicDiv.appendChild(opt);
                }
            });


        }
        // Attach the fetchData function to the window.onload event
        document.addEventListener('DOMContentLoaded', fetchData);
        const colorPicker = document.getElementById('color-picker');
        const selectedColor = document.getElementById('selected-color');
        const animationSlider = document.getElementById('animation-slider');
        const fadeToggle = document.getElementById('fade-toggle');
        const blinkToggle = document.getElementById('blink-toggle');
        const noneToggle = document.getElementById('none-toggle');
        const colorBox = document.getElementById('color-box');

        colorPicker.addEventListener('input', () => {
            sendColor();
            const color = colorPicker.value;
            selectedColor.textContent = color;
            selectedColor.style.color = color;
            colorBox.style.backgroundColor = color;
        });

        function fadeOut() {
            colorBox.style.opacity = 0;
        }

        function fadeIn() {
            colorBox.style.opacity = 1;
        }

        let animationInterval = null;

        function startAnimation() {

            const animationType = document.querySelector('input[name="animation-type"]:checked').value;
            if (animationType === 'fade') {
                sendOpt(2);
                clearInterval(animationInterval);
                animationInterval = setInterval(() => {
                    fadeOut();
                    setTimeout(fadeIn, animationSlider.value / 2);
                }, animationSlider.value);
            } else if (animationType === 'blink') {
                sendOpt(1);
                clearInterval(animationInterval);
                animationInterval = setInterval(() => {
                    colorBox.style.backgroundColor = 'transparent';
                    setTimeout(() => {
                        colorBox.style.backgroundColor = colorPicker.value;
                    }, animationSlider.value / 2);
                }, animationSlider.value * 2);
            }
            else {
                sendOpt(0);
                clearInterval(animationInterval);
            }
        }

        function sendColor() {
            const color = colorPicker.value;
            var str = `/general?color=${color.substr(1)}`;
            console.log(str);
            fetch(str).then(response => response.text())
                .then(data => {
                    console.log(data)
                })

        }
        function sendAnimation() {
            const selectElement = document.getElementById("LedAnimations");

            // Get the selected <option> element
            const selectedOption = selectElement.options[selectElement.selectedIndex];

            // Get the value of the selected <option>
            const selectedValue = selectedOption.value;
            var str = `/general?animation=${selectedValue}`;
            console.log(str);
            fetch(str).then(response => response.text())
                .then(data => {
                    console.log(data)
                })

        }
        function sendMov(value) {
            var str = `/general?dir=${value}`;
            console.log(str);
            fetch(str).then(response => response.text())
                .then(data => {
                    console.log(data)
                })
        }
        function sendOpt(value) {
            var str = `/general?opt=${value}`;
            console.log(str);
            fetch(str).then(response => response.text())
                .then(data => {
                    console.log(data)
                })
        }
        function mapValue(value, fromMin, fromMax, toMin, toMax) {
            // First, normalize the value from the original range to a 0-1 range
            const normalizedValue = (value - fromMin) / (fromMax - fromMin);

            // Then, map the normalized value to the new range
            const mappedValue = normalizedValue * (toMax - toMin) + toMin;

            return mappedValue;
        }
        function sendSpeed(value) {
            value = mapValue(value, 15, 1000, 1000, 15);
            var str = `/general?speed=${value}`;
            console.log(str);
            fetch(str).then(response => response.text())
                .then(data => {
                    console.log(data)
                })
        }
        fadeToggle.addEventListener('change', startAnimation);
        blinkToggle.addEventListener('change', startAnimation);
        noneToggle.addEventListener('change', startAnimation);
    </script>
</body>

</html>
   )==";
    server.send(200, "text/html", msg);
}

void handleGeneral()
{
    if (server.hasArg("color"))
    {
        String msg = server.arg("color");
        uint32_t color = strtoul(msg.c_str(), NULL, HEX);
        ledController.baseColor = color;
        server.send(200, "text/plain", msg);
    }
    if (server.hasArg("dir"))
    {
        int dir = atoi(server.arg("dir").c_str());
        ledController.setDirection(static_cast<direction>(dir));
        server.send(200, "text/plain", String(dir));
    }
    if (server.hasArg("opt"))
    {
        int opt = atoi(server.arg("opt").c_str());
        ledController.setOption(static_cast<option>(opt));
        server.send(200, "text/plain", String(opt));
    }
    if (server.hasArg("get-modes"))
    {
        server.send(200, "text/plain", getFns());
    }
    if (server.hasArg("speed"))
    {
        uint32_t speed = atoi(server.arg("speed").c_str());
        if (speed != 0)
        {
            ledController.setInterval(speed);
        }
        server.send(200, "text/plain", String(speed));
    }
    if (server.hasArg("animation"))
    {
        int newfn = atoi(server.arg("animation").c_str());
        if (newfn != 0 || server.arg("animation") == "0")
        {
            ledController.setMode(static_cast<function>(newfn));
        }
        server.send(200, "text/plain", String(newfn));
    }
}
// Callback for payload at the mqtt topic

void handle_gateway(const char *topic, String incommingMessage, bool mqtt)
{
    // String incommingMessage = "";
    // for (int i = 0; i < length; i++)
    //     incommingMessage += (char)payload[i];

    if (strcmp(topic, "All/control") == 0)
    {
        if (incommingMessage.equals("get_state"))
        {
            handleLedAutomation(control_variables.current_light_state);
        }
    }

    if (strcmp(topic, "Sherlock/Hue") == 0)
    {
        int _newHue = incommingMessage.toInt();
        if (_newHue > 0 || incommingMessage == "0")
        {
            ledController.setMode(Solid, CHSV(map(_newHue, 0, 359, 0, 255), 255, 255));
            MqttSend("/console/currentcolor", String(ledController.getHex(ledController.baseColor)));
        }
    }
    if (strcmp(topic, "Sherlock/Color") == 0)
    {
        uint32_t _newColor = strtoul(incommingMessage.c_str(), NULL, 10);
        if (_newColor > 0 || incommingMessage == "0")
        {
            ledController.setMode(Solid, CRGB(_newColor));
            MqttSend("/console/currentcolor", incommingMessage);
        }
    }
    if (strcmp(topic, "Sherlock/Mode") == 0)
    {
        MqttSend("/debug/mode", incommingMessage + " " + getfnName(getFnbyName(incommingMessage)));
        ledController.setMode(getFnbyName(incommingMessage));
    }
    if (String(topic) == control_variables.external_ldr_topic)
    {
        control_variables.last_external_ldr = incommingMessage.toInt();
        if (control_variables.use_external_ldr)
        {
            handleLedAutomation(!(control_variables.last_external_ldr > 3800));
        }
    }
    if (strcmp(topic, "Sherlock/Brightness") == 0)
    {
        MqttSend("/debug/bright", "old: " + String(control_variables.on_brightness) + " -> " + incommingMessage);
        control_variables.on_brightness = incommingMessage.toInt();
        handleLedAutomation(control_variables.current_light_state);
    }
    if (strcmp(topic, "Sherlock/Lights") == 0)
    {
        if (incommingMessage.equals("auto"))
        {
            control_variables.user_override = false;
            // it is properly calling true on low light
            handleLedAutomation(!(control_variables.last_external_ldr > 3800), true);
        }
        else if (incommingMessage[0] == 'u' && incommingMessage.length() > 1)
        {
            bool on = incommingMessage[1] == '1';
            if (incommingMessage[1] == '2')
                on = !control_variables.current_light_state;
            handleLedAutomation(on, true, true);
        }
/// maybe obsolete now
#ifdef USE_LDR
        else if (incommingMessage == "0")
            handleLedAutomation(true);
        else if (incommingMessage == "1")
            handleLedAutomation(false);
#endif
    }
    if (strcmp(topic, "Sherlock/console/in") == 0)
    {
        String result = "";
        if (incommingMessage == "ldr_external")
        {
            control_variables.use_external_ldr = !control_variables.use_external_ldr;
            result = "External LDR is now:";
            result += control_variables.use_external_ldr ? " Enabled." : " Disabled.";
            if (control_variables.use_external_ldr)
            {
                result += "   @'";
                result += control_variables.external_ldr_topic;
                result += "'.";
            }
        }
        if (incommingMessage == "time_automation")
        {
            control_variables.use_time_automation = !control_variables.use_time_automation;
            result = "Time Automation is now:";
            result += control_variables.use_time_automation ? " Enabled." : " Disabled.";
        }
        if (incommingMessage == "test_time")
        {
            bool light_time_state = CompareTime(control_variables.time_to_wake, control_variables.time_to_sleep);
            result += "\nlight_time_state is now:";
            result += light_time_state ? " true." : " false.";
            result += "Time Automation is now:";
            result += control_variables.use_time_automation ? " Enabled." : " Disabled.";
        }
        MqttSend("/console/out", result);
    }

    if (strcmp(topic, "Sherlock/setinterval") == 0)
    {
        int incoming_asInt = atoi(incommingMessage.c_str());
        ledController.setInterval(incoming_asInt);
    }
    if (strcmp(topic, "Sherlock/setwaketime") == 0)
    {
        String result = "Set Wake Time result was: ";
        bool pass = false;
        int pos = incommingMessage.indexOf(":");
        if (pos > 0)
        {
            String _hour_Str = incommingMessage.substring(0, pos);
            String _min_Str = incommingMessage.substring(pos + 1);
            if ((atoi(_hour_Str.c_str()) != 0 || _hour_Str == "00") && (atoi(_min_Str.c_str()) != 0 || _min_Str == "00"))
            {
                pass = true;
                control_variables.time_to_wake = incommingMessage;
                result += "sucess";
            }
        }
        if (!pass)
            result += false;

        MqttSend("/console/out", result);
    }
    if (strcmp(topic, "Sherlock/setsleeptime") == 0)
    {
        String result = "Set Sleep Time result was: ";
        bool pass = false;
        int pos = incommingMessage.indexOf(":");
        if (pos > 0)
        {
            String _hour_Str = incommingMessage.substring(0, pos);
            String _min_Str = incommingMessage.substring(pos + 1);
            if ((atoi(_hour_Str.c_str()) != 0 || _hour_Str == "00") && (atoi(_min_Str.c_str()) != 0 || _min_Str == "00"))
            {
                pass = true;
                control_variables.time_to_wake = incommingMessage;
                result += "sucess";
            }
        }
        if (!pass)
            result += false;

        MqttSend("/console/out", result);
    }

    if (control_variables.tcp_debug)
    {
        String msg = mqtt ? "[MQTT] - " : "[TCP] -";
        msg += "topic:'";
        msg += topic;
        msg += "',payload:'";
        msg += incommingMessage;
        msg += "'.";
        Serial.println(msg);
        tcpServer.broadcast(msg);
    }
}

bool getTime()
{
    bool result = false;
#ifdef COMPILE_SERIAL
    Serial.println("Syncing Time Online");
#endif
    WiFiClient client;
    HTTPClient http;
    http.begin(client, "http://worldtimeapi.org/api/timezone/America/Bahia.txt"); // HTTP
    int httpCode = http.GET();
    // httpCode will be negative on error
    if (httpCode > 0)
    {
        // HTTP header has been send and Server response header has been handled
        // file found at server
        if (httpCode == HTTP_CODE_OK)
        {
#ifdef COMPILE_SERIAL
            Serial.printf("[HTTP] OK... code: %d\n", httpCode);
#endif
            String payload = http.getString();
            char str[payload.length() + 1];
            strcpy(str, payload.c_str());
            char *pch;
            pch = strtok(str, ":\n");
            int i = 0;
            int raw_offset = 0;
            while (pch != NULL)
            {
                i++;
                if (i == 23)
                {
                    raw_offset = atoi(pch);
                }
                if (i == 27)
                {
                    setTime(atoi(pch) + raw_offset);
                }
                // printf("%d: %s\n", i, pch);
                pch = strtok(NULL, ":\n");
            }
#ifdef COMPILE_SERIAL
            String msg = "Time Synced ";
            msg += millis();
            msg += "ms from boot.";
            Serial.println(msg);
#endif
            result = true;
        }
        else
        {
#ifdef COMPILE_SERIAL
            Serial.printf("[HTTP] Error code: %d\n", httpCode);
#endif
        }
    }
    else
    {
#ifdef COMPILE_SERIAL
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
#endif
    }
    http.end();
    return result;
}

void LED_TASK(void *pvParameters)
{
    for (;;)
    {
        ledController.run();
        delay(5);
        if (control_variables.updating)
            delay(995);
    }
}

int getRGB(CRGB c)
{
    int s = 0;
    s += c.r * 0x10000;
    s += c.g * 0x100;
    s += c.b * 0x1;
    return s;
}
void printLEDs()
{
    Serial.println("------------------------------------------------------------------------------------------------------------------------------------");
    for (size_t i = 6; i < 15; i++)
    {
        Serial.printf("[%2d] = %2d", i, getRGB(leds[i]));
        for (size_t j = 1; j <= 9; j++)
        {
            int k = i + 10 * j;
            if (k < 102)
            {
                Serial.printf(",  [%2d] = %2d", k, getRGB(leds[k]));
            }
        }
        Serial.println();
    }
    Serial.println("------------------------------------------------------------------------------------------------------------------------------------");
}

void HandleMqtt(char *topic, byte *payload, unsigned int length)
{
    String incommingMessage = "";
    for (int i = 0; i < length; i++)
        incommingMessage += (char)payload[i];
    handle_gateway(topic, incommingMessage, true);
}

/// @brief Attempts to connect, reconnect to the MQTT broker .
/// @return True if successful or false if not.
bool MQTT_Reconnect()
{
    String clientID = DEVICE_NAME;
    clientID += String(random(), HEX);
    if (HiveMQ.connect(clientID.c_str(), MQTT_USER, MQTT_PASSWD))
    {
        bool sub = HiveMQ.subscribe("#");
        // HiveMQ.subscribe("Adler/sensors/LDR");
#ifdef COMPILE_SERIAL
        Serial.printf("MQTT Connected subscribe was: %s\n", sub ? "success" : "failed");
#endif
        return true;
    }
    else
    {
#ifdef COMPILE_SERIAL
        Serial.printf("Can't Connect to MQTT Error Code : %d\n", HiveMQ.state());
#endif
        return false;
    }
}

void mqtt_reconect_task()
{
    if (HiveMQ.connected())
        return;
    control_variables.mqtt_state = MQTT_Reconnect();
}
void tcp_reconnect()
{
    if (control_variables.updating)
        return;
    // Serial.printf("[RAW] TCP: (%d) || MQTT: (%d) \n", tcpClient.connected(), HiveMQ.connected());

    if (!tcpClient.connected())
    {
        IPAddress server_ip = MDNS.queryHost(SERVER_HOST);
        if (server_ip != IPAddress())
        {

            if (tcpClient.connect(server_ip))
            {
                tcpClient.setMode(TransmissionMode::SizeColon);
                tcpClient.send("REQ_UPDATES");
                if (HiveMQ.connected())
                {
                    HiveMQ.disconnect();
                    control_variables.mqtt_state = false;
                }
                control_variables.tcp_state = true;
            }
            else
            {
                control_variables.tcp_state = false;
            }
        }
        else
        {
            control_variables.tcp_state = false;
        }
    }
    if (!control_variables.tcp_state)
        mqtt_reconect_task();

    if (control_variables.tcp_debug)
    {
        String msg = "[debug][tcp_reconnect]: ";
        msg += " TCP: label {";
        msg += control_variables.tcp_state;
        msg += "} RAW {";
        msg += tcpClient.connected();
        msg += "} || MQTT:  label {";
        msg += control_variables.mqtt_state;
        msg += "} RAW {";
        msg += HiveMQ.connected();
        msg += "}";
        tcpServer.broadcast(msg);
    }
}

void setup()
{
    pinMode(ONE_WIRE_BUS, INPUT);
    pinMode(LED_STRIP_PIN, OUTPUT);
    pinMode(2, OUTPUT);
    FastLED.addLeds<WS2812, LED_STRIP_PIN, GRB>(leds, LED_STRIP_SIZE); // GRB ordering is typical
    ledController.setBoundries(6, 101);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 2000);
    FastLED.setBrightness(control_variables.on_brightness);
    control_variables.current_light_state = true;

    Serial.begin(115200);

    tempSensor.begin();
    tempSensor.getAddress(sensorAddress, 0);
    tempSensor.setResolution(sensorAddress, 11);
    dht.setup(dht_pin, dht_type);

    int sensors_found = tempSensor.getDeviceCount();
    Serial.printf("DS18 SENSORS: %d\n", sensors_found);
    for (uint8_t i = 0; i < sensors_found; i++)
    {
        Serial.printf("[%d] 0x", i);
        DeviceAddress addr;
        tempSensor.getAddress(addr, i);
        for (uint8_t j = 0; j < 8; j++)
        {
            if (addr[j] < 0x10)
                Serial.print("0");
            Serial.print(addr[j], HEX);
        }
        Serial.println("");
    }
    Serial.println();
    Serial.print("Getting Internet Acess: ");
    Serial.printf("%s : %s\n", WIFISSID, WIFIPASSWD);

    // Wifi Setup
    WiFi.mode(WIFI_STA); // Important to be explicitly connected as client
    WiFi.begin(WIFISSID, WIFIPASSWD);

    uint8_t counter = 0;
    bool color = true;
    bool notconnected = false;
    while (WiFi.status() != WL_CONNECTED && !notconnected)
    {

        if (millis() % 100 == 0)
        {
            counter++;
            Serial.print(".");
        }

        if (counter == 10)
        {
            FastLED.showColor(color ? CRGB::DarkGreen : CRGB::Blue);
            color = !color;
            Serial.print(WiFi.status());
            Serial.println();
            counter = 0;
        }

        yield();
        if (millis() > 15000)
        {
            uint not_the_same_dumb_fucking = millis();
            Serial.print("Reseting at: ");
            Serial.print(not_the_same_dumb_fucking);
            Serial.print("ms / ");
            Serial.print(not_the_same_dumb_fucking / 1000);
            Serial.print("sec / ");
            Serial.print(not_the_same_dumb_fucking / 60000);
            Serial.println("min");
            notconnected = true;
            ESP.restart();
        }
    }

    ledController.setMode(Solid, CRGB::Green, CRGB::BlanchedAlmond);
    ledController.setOption(blink);
    ledController.setInterval(150);

    if (!notconnected)
    {

        Serial.println();
        Serial.print("Connection took (ms) : ");
        Serial.println(millis());
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        if (!MDNS.begin(DEVICE_NAME))
        {
            Serial.println("Error starting mDNS");
            return;
        }

        MDNS.addService("http", "tcp", 80);
        ArduinoOTA.setHostname(DEVICE_NAME);
        ArduinoOTA.onStart(startOTA);
        ArduinoOTA.onEnd(endOTA);
        ArduinoOTA.onProgress(progressOTA);
        ArduinoOTA.onError(errorOTA);
        ArduinoOTA.begin();

        server.begin();

        Serial.println("HTTP server started");

        hive_client.setInsecure();
        HiveMQ.setServer(MQTT_URL, MQTT_PORT);
        HiveMQ.setCallback(HandleMqtt);
        control_variables.time_synced = getTime();
        control_variables.boot_time = millis();

        // Starts the TCP server (used for local debug mostly)
        tcpServer.setMessageHandler(HandleTcpServertMessage);
        tcpServer.begin();

        // Starts the TCP Client (used for local integration)
        tcpClient.setMessageHandler(HandleTcpClientMessage);
        tcpClient.client->setTimeout(1);
        tcpClient.setCllientInfo("LEDController", "led controller for under the bed");
        tcp_reconnect(); // handle conectivity, prefers tcp (local server) to mqtt (HiveMQTT)

        // Start webserver
        server.on("/", handleIndex);
        server.on("/general", handleGeneral);
        server.begin();
    }
    xTaskCreatePinnedToCore(
        LED_TASK,    // Function to implement the task /
        "LEDhandle", // Name of the task /
        4096,        //* Stack size in words /
        NULL,        //* Task input parameter /
        32,          //* Priority of the task /
        NULL,        //* Task handle. /
        1);

    ledController.setMode(function::Solid, CRGB::Green);
    ledController.setInterval(250);
}

void loop()
{
    ArduinoOTA.handle();
    tcpServer.handleServer();
    server.handleClient();
    HiveMQ.loop();
    tcpClient.handleClient();

    if (Serial.available() > 0)
    {
        String _input = "";
        while (Serial.available() > 0)
        {
            char c = Serial.read();
            // Control bytes from PIO implementation of serial monitor.
            if (c != 10 && c != 13)
            {
                _input += c;
            }
        }
        // tcpClient.send(_input);
    }

    if (now() - Timers.temperature > 2)
    {
        Timers.temperature = now();
        tempSensor.requestTemperatures();
        _current_temp = tempSensor.getTempCByIndex(0);
        _dht_hum = dht.getHumidity();
        _dht_temp = dht.getTemperature();
        _dht_info = dht.getStatusString();
        Serial.printf("DS18: %.2f; DHT temp: %.2f hum: %.2f  status: %s\n", _current_temp, _dht_temp, _dht_hum, _dht_info);
    }
    if (now() - Timers.sync_time > 300 && !control_variables.time_synced)
    {
        Timers.sync_time = now();
        control_variables.time_synced = getTime();
    }
    if (now() - Timers.mqtt_publish > control_variables.mqtt_update)
    {
        Timers.mqtt_publish = now();
        MqttSend("/sensors/Temperature", String(_current_temp));
        if (_dht_info == "OK")
        {
            MqttSend("/sensors/dhtTemp", String(_dht_temp));
            MqttSend("/sensors/dhtHum", String(_dht_hum));
        }
    }
    if (now() - Timers.soft_ldr > 5)
    {
        tcp_reconnect();
        Timers.soft_ldr = now();

        // IPAddress ldr_ip = MDNS.queryHost(LDR_HOST);
        // if (ldr_ip != IPAddress())
        // {
        //     tcpClient.connect(ldr_ip);
        // }
        // else
        // {
        //     Serial.printf("Error: %s was not found by MDNS.\n", LDR_HOST);
        // }
    }
    if (now() >= Timers.Automation_resume && control_variables.user_override)
    {
        control_variables.user_override = false;
        handleLedAutomation(!(control_variables.last_external_ldr > 3800));
    }
    if (control_variables.use_time_automation && !control_variables.user_override)
    {
        // Turn on the lights if time is between wake and sleep hours
        bool light_time_state = CompareTime(control_variables.time_to_wake, control_variables.time_to_sleep);
        if (light_time_state != control_variables.current_light_state)
        {
            handleLedAutomation(light_time_state);
        };
    }
}
