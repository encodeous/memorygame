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
#include <algorithm>
#include <TaskScheduler.h>
#include <string>
#include <functional>
using namespace std;
#pragma endregion
#pragma region CONSTANTS
#define __a ' ',
#define __b __a __a __a __a
const char EMPTY_LINE[] = {__b __b __b __b};
Scheduler sc;
LiquidCrystal_I2C lcd(0x27,16,2);
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
        B00000
};
byte HEART[] = {
        B00000,
        B00000,
        B01010,
        B11111,
        B11111,
        B01110,
        B00100,
        B00000
};

#pragma endregion
#pragma region UTILITIES
li mapi(double pct, li min, li max){
    return (max - min) * pct + min;
}
double read_pct(){
    return analogRead(IPT_POT) / 1023.0;
}
#pragma endregion
// simple scheduled scrolling text
int ts_ln = 0, ts_cur = 0, ts_wtsl, ts_cc = 0;
string curstr;
Task textScroller;
void ts_callback(){
    if(ts_cc-- != 0){
        lcd.setCursor(0, ts_ln);
        char buf[16];
        for(int i = 0; i < 16; i++){
            buf[i] = curstr[i + ts_cur];
        }
        lcd.print(buf);
        ts_cur++;
        textScroller.delay(ts_wtsl);
        textScroller.setCallback(ts_callback);
        sc.addTask(textScroller);
    }
}
void fs_scrolling_text(const string& str, int ln, int duration){
    if(str.length() <= 16){
        lcd.setCursor(0, ln);
        char buf[16];
        memcpy(buf, EMPTY_LINE, sizeof(buf));
        memcpy(buf, str.c_str(), str.length() * sizeof(char));
        lcd.printstr(buf);
    }else{
        double mspc = duration / (str.length() - 15.0);
        lcd.setCursor(0, ln);
        char buf[16];
        for(int i = 0; i < 16; i++){
            buf[i] = curstr[i];
        }
        lcd.print(buf);
        ts_cur = 1;
        ts_ln = ln;
        ts_cc = str.length() - 16;
        ts_wtsl = mspc;
        textScroller.delay(ts_wtsl);
        textScroller.setCallback(ts_callback);
        sc.addTask(textScroller);
    }
}
void endGame();
struct game{
    char grid[26];
    bool picked[26];
    int cur = 0;
    int csel = -1;
    int time = 0; // this shows how much time is remaining in 500ms units
    int lives = 0;
    bool is_active = false;
    bool odd_timer = false;
    game(){}
    void init(int difficulty){
        is_active = true;
        char c3[3][13];
        for(int i = 0; i < 13; i++){
            c3[0][i] = 'A' + i;
            c3[1][i] = i;
            c3[2][i] = i + 13;
        }
        random_shuffle(c3[0], c3[0] + 13);
        random_shuffle(c3[1], c3[1] + 13);
        random_shuffle(c3[2], c3[2] + 13);
        for(int i = 0; i < 13; i++){
            grid[c3[1][i]] = c3[0][i];
            grid[c3[2][i]] = c3[0][i];
        }
        time = 240 * 2;
        lives = 21 - mapi(difficulty * difficulty / (20 * 20), 1, 20);
    }
    void render(){

    }
    void render_pre(){

    }
    void win(){

    }
    void lose(string reason){

    }
    void on_click(){

    }
};
game g;
void inputScan();
void screenRefresh();
void beginGame(int delay = 5000);
Task inputTask(5, TASK_FOREVER, &inputScan), refreshTask(15, TASK_FOREVER, &screenRefresh);
Task gameTask;
int curDifficulty = 1;
void endGame(bool hasWon){
    gameTask.disable();
    if(hasWon) curDifficulty++;
    beginGame(3000);
    g.is_active = false;
}
int lastInp = 0;
bool isDown = false;
void inputScan(){
    if(!g.is_active) return;
    g.cur = mapi(read_pct(), 0, 26 - 1);
    if(millis() - lastInp >= 20){
        // ready for input
        lastInp = millis();
        if(digitalRead(IPT_BUT) && !isDown){
            isDown = true;
            g.on_click();
        }else{
            isDown = false;
        }
    }
}
void screenRefresh(){
    if(!g.is_active) return;

}
void timer(){
    g.odd_timer = !g.odd_timer;
    g.time--;
    if(g.time <= 0){
        g.lose("Out of time!");
    }
}
#pragma region GAME LOADING
void startGame(){
    sc.addTask(gameTask);
    gameTask.setIterations(TASK_FOREVER);
    gameTask.setInterval(500);
    gameTask.setCallback(timer);
}
void prepGame(){
    g.init(curDifficulty);
    g.render_pre();
    gameTask.delay(3000);
    gameTask.setCallback(startGame);
    sc.addTask(gameTask);
}
void gameLoader(){
    lcd.clear();
    lcd.printstr("Difficulty: ");
    lcd.println(curDifficulty);
    lcd.print("Remember These");
    gameTask.delay(2000);
    gameTask.setCallback(prepGame);
}
void beginGame(int delay){
    gameTask.delay(delay);
    gameTask.setCallback(gameLoader);
    sc.addTask(gameTask);
}
#pragma endregion
void setup()
{
    sc.addTask(inputTask);
    sc.addTask(refreshTask);
    lcd.init();
    lcd.backlight();
    pinMode(BUZZER, OUTPUT);
    randomSeed(analogRead(A1));
    srand(analogRead(A1));
    lcd.printstr("Robo Match v1.0");
    fs_scrolling_text("Memory Game Developed by Adam Chen - git.io/robomatch", 1, 5000);
    beginGame();
}
void loop(){ sc.execute(); }