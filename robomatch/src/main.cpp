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
enum C_CHAR{
    CHECK,
    HRT,
    ARROW_RIGHT,
    SELECTION_BOX,
    SELECTION_BOX2
}cc;
byte __cc01[] = {
        B00000,
        B00001,
        B00011,
        B10110,
        B11100,
        B01000,
        B00000,
        B00000};
byte __cc02[] = {
        B00000,
        B00000,
        B01010,
        B11111,
        B11111,
        B01110,
        B00100,
        B00000};
byte __cc03[] = {
        B00000,
        B00100,
        B00110,
        B11111,
        B11111,
        B00110,
        B00100,
        B00000
};
byte __cc04[] = {
        B11011,
        B10001,
        B00000,
        B00000,
        B00000,
        B00000,
        B10001,
        B11011
};
byte __cc05[] = {
        B11101,
        B10001,
        B10000,
        B10000,
        B00001,
        B00001,
        B10001,
        B10111
};
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
int curDifficulty = 1;
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
        char buf[17];
        buf[16] = '\0';
        memcpy(buf, EMPTY_LINE, sizeof(EMPTY_LINE));
        memcpy(buf, str.c_str(), str.length() * sizeof(char));
        lcd.printstr(buf);
    }
    else
    {
        duration -= 100;
        double mspc = duration / (str.length() - 15.0);
        Serial.println(mspc);
        Serial.println(str.c_str());
        lcd.setCursor(0, ln);
        curstr = str;
        char buf[17];
        for (int i = 0; i < 16; i++)
        {
            buf[i] = curstr[i];
        }
        buf[16] = '\0';
        lcd.printstr(buf);
        ts_cur = 1;
        ts_ln = ln;
        ts_cc = str.length() - 16;
        ts_wtsl = mspc;
        Serial.println(ts_wtsl);
        textScroller.setCallback(ts_callback);
        textScroller.restartDelayed(100 + ts_wtsl * TASK_MILLISECOND);
    }
}
void endGame(bool hasWon);
struct game
{
    char grid[27];
    bool picked[26];
    int clearSelT = 0;
    int cur = 0;
    int pcsel = -1;
    int csel = -1;
    int time = 0; // this shows how much time is remaining in 500ms units
    int lives = 0;
    int remChar = 0;
    bool is_active = false;
    game() {}
    void init(int difficulty)
    {
        grid[26] = '\0';
        //remChar = 1;
        remChar = 13 * 2;
        pcsel = -1;
        csel = -1;
        clearSelT = 0;
        memset(picked, 0, sizeof(picked));
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
        time = 600 * 2;
        //lives = 1;
        lives = 100 - mapi(difficulty / 20.0, 1, 99);
    }
    void render_char(char buf[], const char src[], int ln){
        for(int i = 0; i < 13; i++){
            Serial.println(src[i + ln * 13]);
            buf[i + 3] = src[i + ln * 13];
        }
    }
    long lastMoved = 0;
    void render_char_mask(char& val, int idx){
        if(csel == idx || pcsel == idx){
            if(time % 2 == 0) val = SELECTION_BOX2;
        }
        if(cur == idx){
            if(time % 2 == 0 || millis() - lastMoved <= 500) val = SELECTION_BOX;
        }
    }
    void render_char_masked(char buf[], const char src[], int ln){
        for(int i = 0; i < 13; i++){
            if(picked[i + ln * 13]){
                buf[i + 3] = src[i + ln * 13];
            }else{
                buf[i + 3] = 255;
            }
            render_char_mask(buf[i+3], ln * 13 + i);
        }
    }
    void render()
    {
        char buf1[17];
        buf1[16] = '\0';
        buf1[0] = HRT;
        buf1[1] = buf1[2] = ' ';
        String s = String(lives, DEC);
        memcpy(buf1 + 1, s.c_str(), sizeof(char) * s.length());
        char buf2[17];
        buf2[16] = '\0';
        buf2[0] = buf2[1] = buf2[2] = ' ';
        String s2 = String(time / 2, DEC);
        memcpy(buf2, s2.c_str(), sizeof(char) * s2.length());
        render_char_masked(buf1, grid, 0);
        render_char_masked(buf2, grid, 1);
        lcd.home();
        lcd.print(buf1);
        lcd.setCursor(0, 1);
        lcd.print(buf2);
    }
    void render_pre()
    {
        char buf1[17];
        buf1[16] = '\0';
        buf1[0] = buf1[1] = buf1[2] = 2;
        char buf2[17];
        buf2[16] = '\0';
        buf2[0] = buf2[1] = buf2[2] = 2;
        render_char(buf1, grid, 0);
        render_char(buf2, grid, 1);
        lcd.clear();
        lcd.home();
        lcd.print(buf1);
        Serial.println(buf1);
        lcd.setCursor(0, 1);
        lcd.print(buf2);
        Serial.println(buf2);
    }
    void win()
    {
        if(curDifficulty == 20){
            is_active = false;
            lcd.clear();
            lcd.home();
            lcd.printstr("You won!");
            fs_scrolling_text("You have completed all of the levels! Restart to reset", 1, 5000);
        }else{
            endGame(true);
            lcd.clear();
            lcd.home();
            lcd.printstr("Good job!");
            fs_scrolling_text("Going to the next level!", 1, 2500);
        }
    }
    void lose(String reason)
    {
        endGame(false);
        lcd.clear();
        lcd.home();
        lcd.printstr(reason.c_str());
        fs_scrolling_text("Restarting the level...", 1, 2500);
    }
    void on_click()
    {
        if(clearSelT != 0) return;
        if(picked[cur]) return;
        if(csel != -1){
            picked[csel] = true;
            picked[cur] = true;
            if(grid[csel] != grid[cur]){
                clearSelT = 6;
                lives--;
                pcsel = cur;
            }else{
                remChar -= 2;
                csel = -1;
                pcsel = -1;
            }
        }else{
            csel = cur;
            picked[csel] = true;
        }
        check_game();
    }
    void check_game(){
        if(lives <= 0){
            lose("Out of lives!");
        }
        if(remChar <= 0){
            win();
        }
    }
    void tick(){
        time--;
        if(clearSelT > 0){
            clearSelT--;
            if(clearSelT == 0){
                picked[csel] = false;
                picked[pcsel] = false;
                csel = -1;
                pcsel = -1;
            }
        }
        if (time <= 0)
        {
            lose("Out of time!");
        }
    }
};
game g;
void inputScan();
void screenRefresh();
void beginGame(int delay = 6000);
Task inputTask(15 * TASK_MILLISECOND, TASK_FOREVER, &inputScan, &sc, true), refreshTask(15 * TASK_MILLISECOND, TASK_FOREVER, &screenRefresh, &sc, true);
Task gameTask(1000 * TASK_MILLISECOND, TASK_ONCE, NULL, &sc, false);
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
    int a = mapi(read_pct(), 0, 26 - 1);
    if(g.cur != a){
        g.cur = a;
        g.lastMoved = millis();
    }
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
    g.tick();
}
#pragma region GAME LOADING
void startGame()
{
    g.is_active = true;
    lcd.clear();
    gameTask.setIterations(TASK_FOREVER);
    gameTask.setInterval(500 * TASK_MILLISECOND);
    gameTask.setCallback(timer);
    gameTask.restart();
}
void prepGame()
{
    lcd.clear();
    g.init(curDifficulty);
    g.render_pre();
    gameTask.setCallback(startGame);
    gameTask.restartDelayed((5000 - mapi(curDifficulty / 20.0, 0, 4000)) * TASK_MILLISECOND);
}
void gameLoader()
{
    lcd.clear();
    lcd.printstr("Difficulty: ");
    lcd.print(curDifficulty);
    fs_scrolling_text("Remember these letters!", 1, 2500);
    gameTask.setCallback(prepGame);
    gameTask.restartDelayed(3000 * TASK_MILLISECOND);
}
void beginGame(int delay)
{
    gameTask.setCallback(gameLoader);
    gameTask.enableDelayed(delay);
}
#pragma endregion
//#define DEBUGGING
void setup() {
    Serial.begin(9600);
    inputTask.enable();
    refreshTask.enable();
    lcd.init();
    lcd.backlight();
    lcd.createChar(HRT, __cc02);
    lcd.createChar(CHECK, __cc01);
    lcd.createChar(ARROW_RIGHT, __cc03);
    lcd.createChar(SELECTION_BOX, __cc04);
    lcd.createChar(SELECTION_BOX2, __cc05);
    pinMode(BUZZER, OUTPUT);
    randomSeed(analogRead(A1));
    srand(analogRead(A1));
#ifdef DEBUGGING
    prepGame();
#else
    lcd.home();
    lcd.printstr("Robo Match v1.6");
    fs_scrolling_text("Memory Game Developed by Adam Chen - git.io/robomatch", 1, 4000);
    beginGame();
#endif
}
void loop() {
    sc.execute();
}