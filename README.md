# `RoboMatch`
A Memory Game built on the Arduino Platform. *Project made for TEJ4M*

See a live demo of RoboMatch on YouTube! (Click to see video)

[![Demo of RoboMatch](https://user-images.githubusercontent.com/47889571/144732060-391cb0a3-d17b-4996-81e2-7f85a5beb02e.png)](https://www.youtube.com/watch?v=cjLqTAt5Vvc)

Alternatively, you can play around with the code online at https://wokwi.com/arduino/projects/316891890985206336

## `Build Instructions`

This project uses PlatformIO, and is designed for an Arduino UNO. The libraries that this project depends on are included in the `platformio.ini` file. Simply connect your Arduino, and upload the script with PIO.

## `Parts List`

**Required Components:**
- 1x I2C (or compatible) LCD
- 1x 10K Ohm Potentiometer
- 1x Push Button
- 1x 10K Ohm Resistor (for the button)

**Optional Components:**
- 1x Piezo Buzzer
- 1x 100 Ohm Resistor (for the buzzer)

## `Project Report`

### `Build & Design Process`

#### Game Idea

The game is based on the generic card matching memory game. I decided to replicate this game due to its simplicity, but also because it is fun to play. The basic premise of the game is to pick matching cards out of a set of unknown cards. Once the player picks all matching pairs, they win the game.

The player will interact with the game with a Potentiometer (As a joystick) and a button to select the letter.

#### Build Process

1. First I designed the game on Wokwi (See the online demo for this)
2. I transferred the design onto a physical breadboard.

#### Coding the Game

I wanted the game to handle input, render the pixels on the screen, count down the time remaining, and to run other tasks at the same time. This is why I decided enlist the help of the Task Scheduler Library. With the use of this library, I am able to schedule tasks with relative precision. 

#### Troubleshooting

| Problem | Attempted Solution | Resolution |
| ------- | ------------------ | ---------- |
| LCD display does not output correctly | Tried re-connecting the wires | Found that the I2C interface required a special library |
| Task Scheduler not running properly | Tried re-creating the tasks (Game tick task, render task, input scan task, etc) | Found out that I double-initialized the task scheduler. |
| The potentiometer was not inputting on a linear scale | Tried with a different potentiometer | Resolved the problem by manually correcting the non-linear input. |
| LCD Display has poor refresh rate | N/A | Refrained from clearing the display. Instead, I overrode the existing pixels. This effectively doubled the refresh rate |


## `Disclaimer`

I am not very experienced in electronics, so take these instructions as general *recommendations*. I highly recommend you to first do some googling around before trying this. This short repo description is neither rigorous nor free from error. Your mileage may vary & I am not responsible for any damages that may happen.
