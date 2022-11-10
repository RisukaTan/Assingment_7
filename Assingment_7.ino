#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TimerOne.h>
#include <EEPROM.h>
#define BA 2
#define BB 3
#define Buzzer 4
#define OLED_RESET -1

Adafruit_SSD1306 OLED(OLED_RESET);

enum State{ None, Save, StopWatch, Alarm, AlarmHour, SetMin, SetHr };
int8_t second,minute,hour,tempMin,tempHour;
int8_t currentState = State::None,tempState = State::None;
int32_t deltaTime,blinkTime,SoundBlink;
bool blink,startStopWatch = false,isPlooked = false;
int64_t startSecond;
int8_t plookHour = -1,plookMinute = -1;

void setup()
{
    Serial.begin(9600);
    OLED.begin(SSD1306_SWITCHCAPVCC, 0x3c);
    OLED.setTextColor(WHITE);

    pinMode(BA, INPUT_PULLUP);
    pinMode(BB, INPUT_PULLUP);
    pinMode(Buzzer, OUTPUT);

    Timer1.initialize(1000000);
    Timer1.attachInterrupt(oneSec);


    if (EEPROM.read(0) < 60)
        EEPROM.get(0, second);
    if (EEPROM.read(1) < 60)
        EEPROM.get(1, minute);
    if (EEPROM.read(2) < 24)
        EEPROM.get(2, hour);

    if (EEPROM.read(3) < 60)
        EEPROM.get(3, plookMinute);
    if (EEPROM.read(4) < 24)
        EEPROM.get(4, plookHour);
}
int s;
void modePressed()
{
    const State states[] = {None, Save, StopWatch, Alarm, SetMin, SetHr};
    s = (s + 1) % 6;
    tempState = states[s];
}
void selectPressed()
{
    if (tempState != currentState)
    {
        currentState = tempState;
        if (currentState == State::StopWatch)
        {
            startStopWatch = true;
            startSecond = 0;
        }
        else if (currentState == State::Save)
        {
            EEPROM.update(0, second);
            EEPROM.update(1, minute);
            EEPROM.update(2, hour);
        }
        return;
    }
    // Confirm (tempState == currentState)
    switch (currentState)
    {
    case State::None:
        if (isPlooked)
        {
            plookHour = -1;
            plookMinute = -1;
            EEPROM.update(3, plookMinute);
            EEPROM.update(4, plookHour);
            isPlooked = false;
            noTone(Buzzer);
        }
        break;
    case State::Save:
        EEPROM.update(0, second);
        EEPROM.update(1, minute);
        EEPROM.update(2, hour);
        break;
    case State::StopWatch:
        startStopWatch = !startStopWatch;
        break;
    case State::Alarm:
        plookMinute = tempMin;
        currentState = State::AlarmHour;
        tempState = State::AlarmHour;
        break;
    case State::AlarmHour:
        plookHour = tempHour;
        EEPROM.update(3, plookMinute);
        EEPROM.update(4, plookHour);
        currentState = State::None;
        tempState = State::Alarm;
        isPlooked = false;
        break;
    case State::SetMin:
        minute = tempMin;
        currentState = State::None;
        break;
    case State::SetHr:
        hour = tempHour;
        currentState = State::None;
        break;
    default:
        break;
    }
}
int32_t bounce;
void pressButton(int pin, void (*callback)())
{
    if (!digitalRead(pin))
    {
        if (millis() - bounce > 300)
        {
            callback();
        }
        bounce = millis();
    }
}
void oneSec()
{
    second++;
    if (second >= 60)
    {
        second = 0;
        minute++;
    }
    if (minute >= 60)
    {
        minute = 0;
        hour++;
    }
    if (hour >= 24)
    {
        hour = 0;
        minute = 0;
        second = 0;
    }
    if (currentState == State::StopWatch && startStopWatch)
        startSecond++;
}
void setLight()
{
    OLED.ssd1306_command(SSD1306_SETCONTRAST);
    OLED.ssd1306_command(255 - (map(analogRead(A1), 0, 1023, 0, 255)));
}
void printMode(const char *str)
{
    OLED.setTextSize(1);
    OLED.setCursor(35, 20);
    OLED.print(str);
}
void loop()
{
    deltaTime = millis() - deltaTime;
    blinkTime += deltaTime;
    SoundBlink += deltaTime;
    setLight();
    SoundPlook();
    pressButton(BA, modePressed);   // Mode
    pressButton(BB, selectPressed); // Select

    tempMin = map(analogRead(A0), 0, 1023, 0, 60);
    tempHour = map(analogRead(A0), 0, 1023, 0, 24);

    OLED.clearDisplay();
    if (tempState == State::StopWatch)
        printMode("StopWatch");
    else if (tempState == State::Alarm || tempState == State::AlarmHour)
        printMode("Alarm");
    else if (tempState == State::SetMin)
        printMode("Set Minute");
    else if (tempState == State::SetHr)
        printMode("Set Hour");
    else if (tempState == State::Save)
        printMode("Save Time");

    OLED.setTextSize(2);
    if (currentState == State::StopWatch)
    {
        int swSec = startSecond % 60;
        int swMin = startSecond / 60;
        OLED.setCursor(35, 1);
        if (swMin < 10)
            OLED.print("0");
        OLED.print(swMin);
        OLED.print(":");
        if (swSec < 10)
            OLED.print("0");
        OLED.print(swSec);
    }
    else if (currentState == State::Alarm)
    {
        OLED.setCursor(35, 1);
        if (plookHour < 10)
            OLED.print("0");
        if (plookHour < 10)
            OLED.print("0");
        else
            OLED.print(plookHour);
        OLED.print(":");
        if (blinkTime > 1000)
        {
            blink = !blink;
            blinkTime = 0;
        }
        if (blink)
        {
            if (tempMin < 10)
                OLED.print("0");
            OLED.print(tempMin);
        }
    }
    else if (currentState == State::AlarmHour)
    {
        OLED.setCursor(35, 1);
        if (blinkTime > 1000)
        {
            blink = !blink;
            blinkTime = 0;
        }
        if (blink)
        {
            if (tempHour < 10)
                OLED.print("0");
            OLED.print(tempHour);
        }
        else
        {
            OLED.print("  ");
        }
        OLED.print(":");
        if (plookMinute < 10)
            OLED.print("0");
        OLED.print(plookMinute);
    }
    else if (currentState == State::SetMin)
    {
        OLED.setCursor(35, 1);
        if (hour < 10)
            OLED.print("0");
        OLED.print(hour);
        OLED.print(":");
        if (blinkTime > 1000)
        {
            blink = !blink;
            blinkTime = 0;
        }
        if (blink)
        {
            if (tempMin < 10)
                OLED.print("0");
            OLED.print(tempMin);
        }
    }
    else if (currentState == State::SetHr)
    {
        OLED.setCursor(35, 1);
        if (blinkTime > 1000)
        {
            blink = !blink;
            blinkTime = 0;
        }
        if (blink)
        {
            if (tempHour < 10)
                OLED.print("0");
            OLED.print(tempHour);
        }
        else
        {
            OLED.print("  ");
        }
        OLED.print(":");
        if (minute < 10)
            OLED.print("0");
        OLED.print(minute);
    }
    else // None
    {
        OLED.setCursor(35, 1);
        if (hour < 10)
            OLED.print("0");
        OLED.print(hour);
        OLED.print(":");
        if (minute < 10)
            OLED.print("0");
        OLED.print(minute);
    }
    OLED.display();
}
void SoundPlook()
{
    if (hour == plookHour && minute == plookMinute)
    {
        isPlooked = true;
        if (SoundBlink > 1000)
        {
            SoundBlink = 0;
            tone(Buzzer,100);
            delay(100);
            noTone(Buzzer);
        }
    }
}
