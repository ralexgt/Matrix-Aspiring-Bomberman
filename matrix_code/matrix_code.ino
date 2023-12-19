#include <LiquidCrystal.h>
#include <LedControl.h>
#include <EEPROM.h>

const byte eepromLcdBrightness = 0;
const byte eepromMatrixBrightness = 4;
// easy, medium, hard highscores
const byte eepromHighscores[3] = {8, 12, 16};
const byte eepromDifficulty = 20;

// joystick pins
const byte pinSW = 0;
const byte pinX = A0;
const byte pinY = A1; 

const byte pinBomb = 2;

// driver/matrix pins
const byte dinPin = 12;
const byte clockPin = 11;
const byte loadPin = 10;
const byte matrixSize = 8;

const byte rs = 9;
const byte en = 8;
const byte d4 = 7;
const byte d5 = 6;
const byte d6 = 5;
const byte d7 = 4;
const byte lcdBrightnessPin = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
int intensity = 0;
byte intensityMapped = 0; 

LedControl lc = LedControl(dinPin, clockPin, loadPin, 1); 
byte matrixBrightness = EEPROM.read(eepromLcdBrightness);
byte matrix[matrixSize][matrixSize] = {
  {1, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};

byte bombChar[8] = {
	0b00000,
	0b00001,
	0b00010,
	0b01110,
	0b01010,
	0b01010,
	0b01110,
	0b00000
};

// player coordinates
int playerX = 0;
int playerY = 0;

// joystick debounce
const int minThreshold = 200;
const int maxThreshold = 900;
bool joyMoved = false;
const int joystickDebounce = 300;
unsigned long lastJoyMove = 0;

// joystick inputs
int xValue = 0;
int yValue = 0;

unsigned long currentSlowBlinkMillis = 0;
unsigned long currentBombBlinkMillis = 0;
unsigned long lastSlowBlinkMillis = 0;
unsigned long lastBombBlinkMillis = 0;
const int slowBlinkingTime = 800;
const int bombBlinkingTime = 100;
byte playerState = 1;

unsigned long lastBombInterruptTime = 0;
unsigned long buttonsDebounce = 300;

byte gameStarted = 0;
unsigned long gameStartTime = 0;

// how many walls are left during game
int gameWallCount = 0;

bool displayIsOn = 1;
bool inMenu = 1;
bool isGameFinished = 0;
bool pressedSW = 1;
bool inSettings = 0;
bool inBrightnessMatrix = 0;
bool inBrightnessLCD = 0;
bool inDifficulty = 0;

int timer = 0;
int timeLimit = 0;
int score = 0;
int ammo = 0;

void setup() {
  EIFR = (1 << INTF1) | (1 << INTF0);

  pinMode(pinX, INPUT);
  pinMode(pinY, INPUT);
  pinMode(pinSW, INPUT_PULLUP);
  pinMode(pinBomb, INPUT_PULLUP);
  
  Serial.begin(9600);

  lc.shutdown(0, false);
  lc.setIntensity(0, matrixBrightness);
  lc.clearDisplay(0);

  fullMatrix();
  if(EEPROM.read(eepromDifficulty) == 0){
    EEPROM.update(eepromDifficulty, 1);
  }

  analogWrite(lcdBrightnessPin, EEPROM.read(eepromLcdBrightness));
  lcd.begin(16, 2);
  lcd.createChar(0, bombChar);
  bootingScreen();

  attachInterrupt(digitalPinToInterrupt(pinBomb), placeBombInterrupt, FALLING);
}

void loop() {
  readJoystickValues();
  //if the joystick was moved after the debouncing time take the input.
  joystickMove();
  updateDisplay();
  if(gameStarted){
    // continiously update game board
    // player's LED
    playerBlink(playerX, playerY);
    // show timer, bombs and walls left on lcd
    timer = timeLimit - (millis() - gameStartTime) / 1000;
    lcd.setCursor(12, 0);
    lcd.write(byte(0));
    if(ammo == -1){
      lcd.print("inf");
    }
    else{
      if(ammo >= 10){
        lcd.print(ammo);
      }
      else if(ammo > 0){
        lcd.print("0");
        lcd.print(ammo);
      }
      if(ammo == 0){
        endGame();
      }
    }
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    if(timer >= 100){
      lcd.print(timer);
    }
    else if(timer >= 10){
      lcd.print("0");
      lcd.print(timer);
    }
    else if(timer > 0){
      lcd.print("00");
      lcd.print(timer);
    }
    if(gameWallCount >= 10){
      lcd.setCursor(14, 1);
      lcd.print(gameWallCount);
    }
    if(gameWallCount < 10){
      lcd.setCursor(14, 1);
      lcd.print("0");
      lcd.print(gameWallCount);
    }
    if(timer == 0){
      endGame();
    }
  }
  pressedSW = digitalRead(pinSW);
  if(!pressedSW && isGameFinished){
    fullMatrix();
    // restart to menu
    if(inMenu == 0 && isGameFinished == 1){
      inMenu = 1;
      displayMenu();
    }
  }
}

// matrix
void fullMatrix(){
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      lc.setLed(0, row, col, 1);
    }
  }
}

void updateDisplay(){
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
    lc.setLed(0, row, col, matrix[row][col]);
    }
  }
}


// movement and player
void readJoystickValues(){
  xValue = analogRead(pinX);
  yValue = analogRead(pinY);
}
// handle joystick inputs
void joystickMove(){
  unsigned long currentTime = millis();
  // check if the joystick was moved beyond thresholds and the debounce time has passed
  if(!joyMoved && currentTime - lastJoyMove >= joystickDebounce && (xValue < minThreshold || xValue > maxThreshold || yValue < minThreshold || yValue > maxThreshold)){
    matrix[playerX][playerY] = 0;
    joyMoved = true;
    lastJoyMove = millis(); 
    playerMovement(playerX, playerY);
    matrix[playerX][playerY] = 1;
  }
    else if(xValue > minThreshold && xValue < maxThreshold && yValue > minThreshold && yValue < maxThreshold){
    joyMoved = false;
  } 
}

// when joystick was moved, move player position anywhere but not out of display bounds
void playerMovement(int &playerX, int &playerY){
  if(yValue > maxThreshold && playerY < matrixSize - 1 && !matrix[playerX][playerY+1]){
      playerY = playerY + 1;
  }
  if(yValue < minThreshold && playerY > 0 && !matrix[playerX][playerY-1]){
      playerY = playerY - 1;
  }
  if(xValue > maxThreshold && playerX > 0 && !matrix[playerX-1][playerY]){
      playerX = playerX - 1;
    }
  if(xValue < minThreshold && playerX < matrixSize - 1 && !matrix[playerX+1][playerY]){
      playerX = playerX + 1;
  }
}

void playerBlink(int coordX, int coordY){
  currentSlowBlinkMillis = millis();
  if(currentSlowBlinkMillis - lastSlowBlinkMillis >= slowBlinkingTime){
    playerState = !playerState;
    matrix[coordX][coordY] = playerState;
    lastSlowBlinkMillis = millis();
  }
}


void startGameplay() {
  if(!gameStarted && inMenu){
    score = 0;
    if(EEPROM.read(eepromDifficulty) == 1){
      timeLimit = 300;
    }
    if(EEPROM.read(eepromDifficulty) == 2){
      timeLimit = 180;
    }
    if(EEPROM.read(eepromDifficulty) == 3){
      timeLimit = 60;
    }
    randomizeWalls();
    gameStarted = 1;
    isGameFinished = 0;
    inMenu = 0;
    gameStartTime = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Game on!");
  }
}

// gameplay
void randomizeWalls(){
  int count = 0;
  byte randomNum = -1;
  int maxWalls = 0;
  if(EEPROM.read(eepromDifficulty) == 1){
    maxWalls = 18;
    ammo = -1;
  }
  if(EEPROM.read(eepromDifficulty) == 2){
    maxWalls =26;
    ammo = 15;
  }
  if(EEPROM.read(eepromDifficulty) == 3){
    maxWalls = 34;
  }
  //int count = 0;
  for (int row = 0; row < matrixSize && count < maxWalls; row++) {
    for (int col = 0; col < matrixSize && count < maxWalls; col++) {
      if((row == 0 && col == 0) || (row == 0 && col == 1) || (row == 1 && col == 0) || (row == 1 && col == 1)){
        matrix[row][col] = 0;
      }
      else{
        randomNum = random(0, 2);
        // 1 in 2 chances to place a wall
        if(randomNum){
          matrix[row][col] = 0;
        }
        if(!randomNum){
          count++;
          matrix[row][col] = 2;
        }
      }
    }
  }
  gameWallCount = count;
  if(EEPROM.read(eepromDifficulty) == 3){
    ammo = gameWallCount / 2;
  }
  playerX = 0;
  playerY = 0;
}
// check if there are walls left and set the game as finished if no
void gameFinished(){
  gameWallCount = 0;
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      if(matrix[row][col] == 2){
        gameWallCount++;
      }
    }
  }
  if(!gameWallCount){
    Serial.println("Level finished!");
    score += 5;
    randomizeWalls();
  }
}

void endGame(){
  Serial.println("Game finished!");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Score: ");
  lcd.print(score);
  lcd.setCursor(0, 1);
  lcd.print("Joystick - menu");
  Serial.println("cleared display");
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
        matrix[row][col] = 0;
    }
  }
  // smile on matrix
  matrix[5][2] = 1;
  matrix[5][3] = 1;
  matrix[5][4] = 1;
  matrix[5][5] = 1;
  matrix[4][1] = 1;
  matrix[4][6] = 1;
  matrix[1][2] = 1;
  matrix[1][5] = 1;
  matrix[2][2] = 1;
  matrix[2][5] = 1;
  if(EEPROM.read(eepromDifficulty) == 1 && score > EEPROM.read(eepromHighscores[0])){
    EEPROM.update(eepromHighscores[0], score);
    Serial.println("New highscore!");
    }
  if(EEPROM.read(eepromDifficulty) == 2 && score > EEPROM.read(eepromHighscores[1])){
    EEPROM.update(eepromHighscores[1], score);
    Serial.println("New highscore!");
  }
  if(EEPROM.read(eepromDifficulty) == 3 && score > EEPROM.read(eepromHighscores[2])){
    EEPROM.update(eepromHighscores[2], score); 
    Serial.println("New highscore!");
  }
  isGameFinished = 1;
  gameStarted = 0;
}

void placeBombInterrupt(){
  if(inMenu){
    return;
  }
  static unsigned long bombInterruptTime = 0;
  bombInterruptTime = micros();
  if(bombInterruptTime - lastBombInterruptTime > buttonsDebounce * 1000 && gameStarted == 1){
    ammo = ammo - 1;
    // if the value goes beyond matrix borders do not destroy walls on the other side
    if(playerY + 1 < matrixSize && matrix[playerX][playerY + 1] == 2){
      matrix[playerX][playerY + 1] = 0; 
      score = score + EEPROM.read(eepromDifficulty);
    }
    else{
      matrix[playerX][playerY + 1] = matrix[playerX][playerY + 1];
    }
    if(playerY - 1 >= 0 && matrix[playerX][playerY - 1] == 2){
      matrix[playerX][playerY - 1] = 0; 
      score = score + EEPROM.read(eepromDifficulty);
    }
    else{
      matrix[playerX][playerY - 1] = matrix[playerX][playerY - 1];
    }
    if(playerX + 1 < matrixSize && matrix[playerX + 1][playerY] == 2){
      matrix[playerX + 1][playerY] = 0;
      score = score + EEPROM.read(eepromDifficulty);
    }
    else{
      matrix[playerX + 1][playerY] = matrix[playerX + 1][playerY];
    }
    if(playerX - 1 >= 0 && matrix[playerX - 1][playerY] == 2){
      matrix[playerX - 1][playerY] = 0; 
      score = score + EEPROM.read(eepromDifficulty);
    }
    else{
      matrix[playerX - 1][playerY] = matrix[playerX - 1][playerY];
    }
    if(playerX + 1 < matrixSize && playerY + 1 < matrixSize && matrix[playerX + 1][playerY + 1] == 2){
      matrix[playerX + 1][playerY + 1] = 0; 
      score = score + EEPROM.read(eepromDifficulty);
    }
    else{
      matrix[playerX + 1][playerY + 1] = matrix[playerX + 1][playerY + 1];
    }
    if(playerX + 1 < matrixSize && playerY - 1 >= 0 && matrix[playerX + 1][playerY - 1] == 2){
      matrix[playerX + 1][playerY - 1] = 0; 
      score = score + EEPROM.read(eepromDifficulty);
    }
    else{
      matrix[playerX + 1][playerY - 1] = matrix[playerX + 1][playerY - 1];
    }
    if(playerX - 1 >= 0 && playerY - 1 >= 0 && matrix[playerX - 1][playerY - 1] == 2){
      matrix[playerX - 1][playerY - 1] = 0; 
      score = score + EEPROM.read(eepromDifficulty);
    }
    else{
      matrix[playerX - 1][playerY - 1] = matrix[playerX - 1][playerY - 1]; 
    }
    if(playerX - 1 >= 0 && playerY + 1 < matrixSize && matrix[playerX - 1][playerY + 1] == 2){
      matrix[playerX - 1][playerY + 1] = 0; 
      score = score + EEPROM.read(eepromDifficulty);
    }
    else{
      matrix[playerX - 1][playerY + 1];
    }
    // check walls count after exploding a bomb
    gameFinished();
    if(ammo == 0){
      endGame();
    }
    lastBombInterruptTime = millis();
  }
}


// lcd and menu on serial
void bootingScreen(){
  const int bootingTime = 10000;
  const int lcdBlinkingInterval = 500; 
  unsigned long currentMillis = millis();
  unsigned long previousMillis = currentMillis;
  Serial.println("Game is booting up!");
  while(currentMillis <= bootingTime){
    if(currentMillis - previousMillis >= lcdBlinkingInterval){
      previousMillis = currentMillis;
      if(displayIsOn){
        lcd.noDisplay();
      }
      else{
        lcd.display();
        lcd.setCursor(3, 0);
        lcd.print("Booting up");
        lcd.setCursor(3, 1);
        lcd.print("the game...");
      }
      displayIsOn = !displayIsOn;
    }
    currentMillis = millis();
  }
  displayIsOn = 1;
  lcd.display();
  lcd.clear();

  displayMenu();
}

void displayMenu(){
  Serial.println("Main Menu:");
  Serial.println("1. Start game");
  Serial.println("2. Highscores");
  Serial.println("3. Settings");
  Serial.println("4. About");
  Serial.println("5. How to play");
  Serial.println("Enter your choice:");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Menu displayed");
  lcd.setCursor(0, 1);
  lcd.print("on Serial");
  inMenu = 1;
  inSettings = 0;
  handleMenu();
}

void handleMenu(){
  if(!inMenu){
    return;
  }
  while(!Serial.available()){
    // we wait in the specific menu until a decision was made
  }
  if(Serial.available() > 0){
    int option = Serial.parseInt();

    switch (option) {
      case 1:
        Serial.println("Gameplay started!");
        startGameplay();
        break;
      case 2:
        handleHighscores();
        break;
      case 3:
        displaySettingsMenu();  
        handleSettingsMenu();   
        break;
      case 4:
        // Display about section on Serial Monitor
        displayAbout();
        break;
      case 5:
        // Display how to section on Serial Monitor
        displayHowTo();
        break;
      default:
        Serial.println("Invalid option. Please enter an option displayed on the menu.");
        displayMenu();
        break;
    }
  }
}

void handleHighscores(){
  Serial.print("Easy: highscore - ");
  Serial.println(EEPROM.read(eepromHighscores[0]));
  Serial.print("Medium: highscore - ");
  Serial.println(EEPROM.read(eepromHighscores[1]));
  Serial.print("Hard: highscore - ");
  Serial.println(EEPROM.read(eepromHighscores[2]));
  Serial.println();
  displayMenu();
}

void displaySettingsMenu(){
  Serial.println("Settings Menu:");
  Serial.println("1. Choose difficulty");
  Serial.println("2. LCD brightness control");
  Serial.println("3. Matrix brightness control");
  Serial.println("4. Back to main menu");
  Serial.println("Enter your option: ");
  inSettings = 1;
  handleSettingsMenu();
}

void handleSettingsMenu(){
  if(!inMenu || !inSettings){
    return;
  }
  while(!Serial.available()){
    // we wait in the specific menu until a decision was made
  }
  if(Serial.available() > 0){
    int option = Serial.parseInt();

    switch (option) {
      case 1:
        Serial.println("Choose difficulty level (1 - 3 | easy -> hard): ");
        inDifficulty = 1;
        handleDifficulty();
        break;
      case 2:
        Serial.println("Set new LCD brightness (0 - 5): ");
        inBrightnessLCD = 1;
        handleBrightness();
        break;
      case 3:
        Serial.println("Set new Matrix brightness (0 - 5): ");
        inBrightnessMatrix = 1;
        handleBrightness();
        break;
      case 4:
        // Display about section on lcd and on Serial Monitor
        Serial.println("Back to main menu.");
        displayMenu();
        break;
      default:
        Serial.println("Invalid option. Please enter an option displayed on the menu.");
        displaySettingsMenu();
        break;
    }
  }
}

void handleDifficulty(){
  while(!Serial.available()){
    // we wait in the specific menu until a decision was made
  }
  if(Serial.available() > 0){
    intensity = Serial.parseInt();
    Serial.println(intensity);
    if(intensity <= 3 && intensity >= 1){
      if(inDifficulty){
        EEPROM.update(eepromDifficulty, intensity);
        inDifficulty = 0;
        displayMenu();
      }
    }
    else{
      Serial.println("Invalid input.");
      Serial.println("Enter 1 = easy | 2 = medium | 3 = hard: ");
      handleDifficulty();
    }
  }
}

void handleBrightness(){
  while(!Serial.available()){
    // we wait in the specific menu until a decision was made
  }
  if(Serial.available() > 0){
    intensity = Serial.parseInt();
    Serial.println(intensity);
    if(intensity <= 5 && intensity >= 0){
      if(inBrightnessLCD){
        intensityMapped = map(intensity, 0, 5, 0, 255);
        EEPROM.update(eepromLcdBrightness, intensityMapped);
        analogWrite(lcdBrightnessPin, EEPROM.read(eepromLcdBrightness));
        inBrightnessLCD = 0;
      }
      else if(inBrightnessMatrix){
        intensityMapped = map(intensity, 0, 5, 0, 15);
        EEPROM.update(eepromMatrixBrightness, intensityMapped);
        lc.setIntensity(0, intensityMapped);
        inBrightnessMatrix = 0;
      }
      inSettings = 0;
      displayMenu();
    }
    else{
      Serial.println("Invalid input.");
      Serial.println("Enter new brightness: ");
      handleBrightness();
    }
  }
}

void displayAbout(){
  if(!inMenu){
    return;
  }
  Serial.println("School project:");
  Serial.println("Matrix (aspiring) bomberman game ");
  Serial.println("How many points can you get?");
  Serial.println();
  displayMenu();
}

void displayHowTo(){
  if(!inMenu){
    return;
  }
  // Serial.println("This is bomberman! (eh)");
  // Serial.print(" and the push button to place a bomb.");
  // Serial.println("Use the joystick to move");
  // Serial.print("Don't worry, you won't expl");
  // Serial.println("ode. However the walls 1 space near you will."); 
  // Serial.println("There are 3 difficulties you can choose from:");
  // Serial.print(" - easy: time limit - 5 minutes;");
  // Serial.println("bombs per level - infinite; ~16 walls per level");
  // Serial.print(" - medium: time limit - 3 minutes;");
  // Serial.println(" bombs per level - 15 + 1 for each level cleared; ~25 walls");
  // Serial.print(" - hard: time limit - 1 minute; ");
  // Serial.println("bombs per level - walls/2; ~32 walls");
  // Serial.println("Points: - wall destroyed = +1p * difficulty");
  // Serial.println("	- level cleared = +5p ");
  // Serial.print("Highscores are displayed in the");
  // Serial.println(" menu (check highscore), for each difficulty!");
  // Serial.println();
  Serial.println("bomb area - 1 space, player can't die, clear all walls");
  displayMenu();
}