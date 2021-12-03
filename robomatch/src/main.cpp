/*
 * MIT License
 *
 * Copyright (c) 2021 Adam Chen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma region IMPORTS
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TaskScheduler.h>

using namespace std;
#pragma endregion
#pragma region CONSTANTS
#define __a ' ',
#define __b __a __a __a __a
const char EMPTY_LINE[] = {__b __b __b __b};
Scheduler sc;
LiquidCrystal_I2C lcd(0x3F, 16, 2);
#define IPT_POT A0
#define IPT_BUT 4
#define BUZZER 5
#define li long int
#pragma endregion
#pragma region CUSTOM CHARS
byte CHECK_MARK[] = {
        B00000,
        B00001,
        B00011,
        B10110,
        B11100,
        B01000,
        B00000,
        B00000};
byte HEART[] = {
        B00000,
        B00000,
        B01010,
        B11111,
        B11111,
        B01110,
        B00100,
        B00000};

#pragma endregion
#pragma region UTILITIES
template <typename T>
void swap (T *a, T *b)
{
    T temp = *a;
    *a = *b;
    *b = temp;
}
template <typename T>
void randomize (T begin[], T end[])
{
    for(T* i = end - 1; i > begin; i--){
        long j = random(0, end - begin);
        swap(i, &begin[j]);
    }
}
li mapi(double pct, li min, li max)
{
    return (max - min) * pct + min;
}
double read_pct()
{
    return analogRead(IPT_POT) / 1023.0;
}
#pragma endregion
// simple scheduled scrolling text
int ts_ln = 0, ts_cur = 0, ts_wtsl, ts_cc = 0;
String curstr;
Task textScroller(0, TASK_ONCE, NULL, &sc, false);
void ts_callback() {
    Serial.println("EXEC: ts_callback");
    if (ts_cc-- != 0) {
        lcd.setCursor(0, ts_ln);
        char buf[16];
        for (int i = 0; i < 16; i++) {
            buf[i] = curstr[i + ts_cur];
        }
        Serial.println(buf);
        lcd.print(buf);
        ts_cur++;
        textScroller.restartDelayed(ts_wtsl * TASK_MILLISECOND);
    }
}
void fs_scrolling_text(const String &str, int ln, int duration)
{
    if (str.length() <= 16)
    {
        lcd.setCursor(0, ln);
        char buf[16];
        memcpy(buf, EMPTY_LINE, sizeof(buf));
        memcpy(buf, str.c_str(), str.length() * sizeof(char));
        lcd.printstr(buf);
    }
    else
    {
        double mspc = duration / (str.length() - 15.0);
        Serial.println(mspc);
        Serial.println(str.c_str());
        lcd.setCursor(0, ln);
        curstr = str;
        char buf[16];
        for (int i = 0; i < 16; i++)
        {
            buf[i] = curstr[i];
        }
        lcd.printstr(buf);
        ts_cur = 1;
        ts_ln = ln;
        ts_cc = str.length() - 16;
        ts_wtsl = mspc;
        Serial.println(ts_wtsl);
        textScroller.setCallback(ts_callback);
        textScroller.enableDelayed(ts_wtsl * TASK_MILLISECOND);
    }
}
void endGame();
struct game
{
    char grid[26];
    bool picked[26];
    int cur = 0;
    int csel = -1;
    int time = 0; // this shows how much time is remaining in 500ms units
    int lives = 0;
    bool is_active = false;
    bool odd_timer = false;
    game() {}
    void init(int difficulty)
    {
        is_active = true;
        char c3[3][13];
        for (int i = 0; i < 13; i++)
        {
            c3[0][i] = 'A' + i;
            c3[1][i] = i;
            c3[2][i] = i + 13;
        }
        randomize(c3[0], c3[0] + 13);
        randomize(c3[1], c3[1] + 13);
        randomize(c3[2], c3[2] + 13);
        for (int i = 0; i < 13; i++)
        {
            grid[c3[1][i]] = c3[0][i];
            grid[c3[2][i]] = c3[0][i];
        }
        time = 240 * 2;
        lives = 21 - mapi(difficulty * difficulty / (20 * 20), 1, 20);
    }
    void render_char(char buf[], const char src[]){
        Serial.print(src);
        for(int i = 0; i < 13; i++){
            buf[i + 3] = src[i];
            buf[i + 19] = src[i + 13];
        }
    }
    void render()
    {
        char buf[32];
        buf[0] = 0;

        buf[1] = 'A';
        buf[2] = 'Y';

        buf[16] = 'O';
        buf[17] = 'U';
        buf[18] = 'T';
        render_char(buf, grid);
        lcd.home();
        lcd.printstr(buf);
    }
    void render_pre()
    {
        char buf[32];
        buf[0] = 'L';
        buf[1] = 'A';
        buf[2] = 'Y';
        buf[16] = 'O';
        buf[17] = 'U';
        buf[18] = 'T';
        render_char(buf, grid);
        lcd.home();
        lcd.printstr(buf);
    }
    void win()
    {
    }
    void lose(String reason)
    {
    }
    void on_click()
    {
    }
};
game g;
void inputScan();
void screenRefresh();
void beginGame(int delay = 6000);
Task inputTask(100 * TASK_MILLISECOND, TASK_FOREVER, &inputScan, &sc, true), refreshTask(100 * TASK_MILLISECOND, TASK_FOREVER, &screenRefresh, &sc, true);
Task gameTask(1000 * TASK_MILLISECOND, TASK_ONCE, NULL, &sc, false);
int curDifficulty = 1;
void endGame(bool hasWon)
{
    gameTask.disable();
    gameTask.setInterval(TASK_ONCE);
    if (hasWon)
        curDifficulty++;
    beginGame(3000);
    g.is_active = false;
}
int lastInp = 0;
bool isDown = false;
void inputScan()
{
    if (!g.is_active)
        return;
    g.cur = mapi(read_pct(), 0, 26 - 1);
    if (millis() - lastInp >= 20)
    {
        // ready for input
        lastInp = millis();
        if (digitalRead(IPT_BUT) && !isDown)
        {
            isDown = true;
            g.on_click();
        }
        else
        {
            isDown = false;
        }
    }
}
void screenRefresh()
{
    if (!g.is_active)
        return;
    g.render();
}
void timer()
{
    g.odd_timer = !g.odd_timer;
    g.time--;
    if (g.time <= 0)
    {
        g.lose("Out of time!");
    }
}
#pragma region GAME LOADING
void startGame()
{
    lcd.clear();
    gameTask.setIterations(TASK_FOREVER);
    gameTask.setInterval(500 * TASK_MILLISECOND);
    gameTask.setCallback(timer);
    gameTask.restart();
}
void prepGame()
{
    g.init(curDifficulty);
    g.render_pre();
    gameTask.setCallback(startGame);
    gameTask.restartDelayed(3000 * TASK_MILLISECOND);
}
void gameLoader()
{
    lcd.clear();
    lcd.printstr("Difficulty: ");
    lcd.print(curDifficulty);
    gameTask.setCallback(prepGame);
    gameTask.restartDelayed(3000 * TASK_MILLISECOND);
}
void beginGame(int delay)
{
    gameTask.setCallback(gameLoader);
    gameTask.enableDelayed(delay);
}
#pragma endregion
void setup() {
    Serial.begin(9600);
    inputTask.enable();
    refreshTask.enable();
    lcd.init();
    lcd.backlight();
    lcd.createChar(0, HEART);
    lcd.createChar(1, CHECK_MARK);
    pinMode(BUZZER, OUTPUT);
    randomSeed(analogRead(A1));
    srand(analogRead(A1));
    lcd.home();
    lcd.printstr("Robo Match v1.1");
    fs_scrolling_text("Memory Game Developed by Adam Chen - git.io/robomatch", 1, 5000);
    beginGame();
}
void loop() {
    sc.execute();
}