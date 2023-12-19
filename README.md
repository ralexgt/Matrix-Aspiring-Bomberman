# **Introduction To Robotics (2023 - 2024)**

_Introduction to Robotics - Faculty of Mathematics and Computer Science, University of Bucharest - 3rd year | 1st semester_ </br> </br>
Repository for the first project - Matrix game (Bomberman) - including requirements, implementation, source code, setup images and/or videos.

## [Matrix Game - Aspiring Bomberman](#ab) <a name="ab"></a>

- ### **_Credits_**

  "EIFR = (1 << INTF1) | (1 << INTF0);" - directly from https://forum.arduino.cc/t/left-over-variables-on-reset-or-strange-interrupt-behaviour/328693/9 - interrupts acted wrong/weird before

- ### **_Description_**

  Bomberman is a classic when it comes to video games. This is my implementation of it. Designed to be simple and easy, there is not much to it but I tried to give the feeling of "playing".
  The player (a blinking LED) goes around an 8x8 map and places bombs with a radius of 1 LED. Walls in the radius are destroyed instantly, but the player is always safe. There are 3 difficulties,
  easy (more like a tutorial), medium (balanced) and hard (less time, bombs depending on the level).
  </br></br>

- ### **_Task Requirements_**

  Make game on a 8x8 logical matrix. It must be fun and intuitive, and
  also remember that it is your game! Pick something that you like and have
  fun creating it. More components can be used. It should be intuitive
  and straight down obvious how to use it.

- ### **_Menu_**

  1. Start game
  2. Highscores
  3. Settings </br>

     - Difficulty </br>
     - LCD brightness </br>
     - Matrix brightness </br>
     - Back to menu

  4. About
  5. How to play

- ### **_Components_** </br>

  - Arduino Uno Board;
  - Joystick;
  - 8x8 LED Matrix;
  - MAX7219;
  - Button
  - Resistors and capacitors as needed;
  - Breadboard and connecting wires;
    </br></br>

- ### **_Setup picture_**

  <img src="./matrix_setup/matrix_setup_img.jpg" width="500" height="300">
  </br></br>

- ### **_Functionality video:_**
  <a href="https://youtu.be/ZSNIxkh37bg"> Introduction to Robotics - Matrix Project (Bomberman) [YouTube] </a>

</br></br>
