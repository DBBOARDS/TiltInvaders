/*-----------------------------------------------------------------------------------------------------------
dbBoardsTiltInvaders.ino

  Summary:
    This pogram uses the game controller hardware outlined in the Tilt Invaders Showcase at DBboards.com.
    To start or restart the game, click the middle button. The LEDs will give you a green, yellow, red 
    countodown. Then your first enemy will appear as two green dots. Your ship is represented by the middle 
    LED always. In order to destroy the enemy before its green, yellow, red countdown ends and you lose, you 
    need to tilt the controller towards the direction of the LED to aim at it. Then you can fire with the
    left or right buttons. Be careful, if you miss, you cannot fire again until your weapon has reloaded.
    After the buzzer rings three times. If you hit your target the vibe motor will shake the controller and
    a new random enemy will appear which will countdown faster then the last. To win, destroy all
    the enemy targets without getting hit.
  
  Utilizing:
    Sparkfun's ADXL345 Library https://github.com/sparkfun/SparkFun_ADXL345_Arduino_Library
    Adafruit NeoPixel Library: https://github.com/adafruit/Adafruit_NeoPixel
   
  Programmer:
    Duncan Brandt @ DB Boards, LLC
    Created: Feb 9, 2018
  
  Development Environment Specifics:
    Arduino 1.6.11
  
  Hardware Specifications:
    DB Boards SPI ADXL345, DB3000
    Arduino Drawing Board (UNO) DB1000
    WS2812 Strip 25 DB7002
    DB Button Board Kit DB5002

  Beerware License:
    This program is free, open source, and public domain. The program is distributed as is and is not
    guaranteed. However, if you like the code and find it useful I would happily take a beer should you 
    ever catch me at the local.
*///---------------------------------------------------------------------------------------------------------
#include <SparkFun_ADXL345.h>         // https://github.com/sparkfun/SparkFun_ADXL345_Arduino_Library
ADXL345 adxl = ADXL345(10);           // USE FOR SPI COMMUNICATION, ADXL345(chipSelectPin);
int x,y,z;                            // Variable used to store accelerometer data
//-----------------------------------------------------------------------------------------------------------
#include <Adafruit_NeoPixel.h>        // https://github.com/adafruit/Adafruit_NeoPixel
#define PIN 1                         // Pin 1 for communication with LED strip
// 25  Number of pixels in strip, NEO_GRB  Pixels are wired for GRB bitstream, NEO_KHZ800  800 KHz bitstream
Adafruit_NeoPixel strip = Adafruit_NeoPixel(25, PIN, NEO_GRB + NEO_KHZ800);
int buzz = 14, vibe = 15, led = 16;   // Digital pins for buzzer, vibe motor, and LED
int btn1 = 5, btn2 = 3, btn3 = 6;     // Button pins 1-3 = left-right
//-----------------------------------------------------------------------------------------------------------
// Variables for location and color of target, and game state booleans
int enemy = 0, ship = 0, enemyTimer = 0, enemyCnt = 90, colorCnt = 0, reloadCnt = 0;
boolean gameOver = false, winner = false, game = false, reload = false;

//-----------------------------------------------------------------------------------------------------------
void setup() {                        // The setup program runs only once after power-up or reset
  strip.begin();                      // Activate the LED strand
  strip.setBrightness(100);           // For our battery application we will limit the brightness to 1Amp max
  strip.show();                       // Initialize all pixels to 'off'
  adxl.powerOn();                     // Power on the ADXL345
  adxl.setRangeSetting(4);            // Range settings 2g(highest sensetivity), 4g, 8g or 16g(lowest sensetivity)
  adxl.setSpiBit(0);                  // Configure the device to be in 4 wire SPI mode
  pinMode(buzz, OUTPUT);              // Setup the buzzer pin as an output
  pinMode(vibe, OUTPUT);              // Setup the vibration motor pin as an ouput
  pinMode(led, OUTPUT);               // Setup the LED pin as an output
  pinMode(btn1, INPUT);               // Setup btn1 as an input
  pinMode(btn2, INPUT);               // Setup btn2 as an input
  pinMode(btn3, INPUT);               // Setup btn3 as an input
  randomSeed(analogRead(5));          // Use floating Analog pin to generate random sequence
  while(digitalRead(btn2));           // Do not start the game until the middle button is pressed
  startGame();                        // Show the countdown sequence and enter the game loop
}
//-----------------------------------------------------------------------------------------------------------
void loop() {                         // Loop runs all the updates and checks according to game state
  if(winner) celebrate();             // If the user won the game, run the celebration sequence
  else if(gameOver) loser();          // If the ship was destroyed, run the loser sequence
  else{                               // Otherwise, keep updating the game
    adxl.readAccel(&x, &y, &z);       // Read the accelerometer values and store them in x, y, and z
    updatePosition();                 // Calculates the ship and enemy locations and color
    if(reload){                       // If the weapon was just fired
      if(reloadCnt < 3){              // Repeat three times
        shotsFired();                 // Make the weapon sound
        reloadCnt++;                  // Add one to the repeat timer
      }
      else{                           // If the repeat cycle is done
        reloadCnt = 0;                // Prep the timer for the next shot
        reload = false;               // Stop making the weapon noises
      }
    }
    else checkTriggers();             // If the weapon is ready, fire and check to see if the enemy is hit
  }
}
//-----------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------
void updatePosition(){               // Map the enemy based on the controllers tilt
  ship = constrain(x, -100, 100);    // Constraing the values to within about 150 degrees
  ship = map(ship, -100, 100, 0, 24);// Adjust the tilt to be represented on the 25 LED strand
  if(enemy == 0){                    // If there is no enemy
    enemy = random(1, 24);           // Creat a random location on the LEDs for a new enemy
  }
  drawEnemy();                       // Update the position and color of the enemy
  delay(paceOfPlay);                 // This is the only major game delay responcible for speed        
}
//-----------------------------------------------------------------------------------------------------------
void drawEnemy(){                    // Update the enemy
  int enemyNow;                      // The tilt adjusted location of the enemy
  if(ship - enemy > 12) enemyNow = 1;// If the enemy's location is below the visible LED strand force it to 1
  else if(ship - enemy < -12) enemyNow = 24;                // If above, force it to 24, the last LED
  else enemyNow = enemy - (ship-13); // Otherwise, draw the enemy minus the ships tilt
  zeroStrip();                       // Clear the last ship location off the LEDs
  if(colorCnt < enemyCnt/3){         // If we are less then a third of the way through the count
    strip.setPixelColor(enemyNow, strip.Color(0,200,0));    // Draw the enemy green
    strip.setPixelColor(enemyNow-1, strip.Color(0,200,0));  // Draw the enemy larger
  }
  else if(colorCnt < 2*enemyCnt/3){  // Or if we are less then two thirds of the way through the count
    strip.setPixelColor(enemyNow, strip.Color(100,100,0));  // Draw the enemy yellow
    strip.setPixelColor(enemyNow-1, strip.Color(100,100,0));// Draw the enemy larger
  }
  else if(colorCnt < 3*enemyCnt/3){  // Or if we are almost done with the count timer
    strip.setPixelColor(enemyNow, strip.Color(200,0,0));    // Draw the enemy red
    strip.setPixelColor(enemyNow-1, strip.Color(200,0,0));  // Draw the enemy larger
  }
  else gameOver = true;              // If the count ran out, game over
  colorCnt++;                        // Progress the enemy timer by one
  strip.show();                      // Display the current enemy on the LED strip
}
//-----------------------------------------------------------------------------------------------------------
void shotsFired(){                   // Make the weopon sound
  tone(buzz, 523, 100);              // Play a high C
  delay(150);                        // Wait time and a half for the sound of the weapon
}
//-----------------------------------------------------------------------------------------------------------
void celebrate(){                    // Sound the victory sequence
  zeroStrip();                       // Clear the LEDs
  strip.show();                      // Turn them all off
  tone(buzz, 262, 100);              // Play a C note
  delay(100);                        // Wait for the note to finish to begin the next one
  tone(buzz, 294, 100);              // Play a D note
  delay(100);                        // Wait for the note to finish to begin the next one
  tone(buzz, 330, 100);              // Play a E note
  delay(100);                        // Wait for the note to finish to begin the next one
  tone(buzz, 349, 100);              // Play a F note
  delay(100);                        // Wait for the note to finish to begin the next one
  tone(buzz, 392, 100);              // Play a G note
  delay(100);                        // Wait for the note to finish to begin the next one
  tone(buzz, 440, 100);              // Play a A note
  delay(100);                        // Wait for the note to finish to begin the next one
  tone(buzz, 494, 100);              // Play a B note
  delay(100);                        // Wait for the note to finish to begin the next one
  tone(buzz, 523, 300);              // Play a High C note
  delay(300);                        // Make a small break after the tones
  for(int a = 0; a < 30; a++){       // Run the sequence 30 times
    for(int b = 0; b < 24; b++){     // For the entire strip
      strip.setPixelColor(b, strip.Color(80,80,80));        // Turn on LED white
      b++;                           // Skip ahead inside the for loop
      strip.setPixelColor(b, strip.Color(0,0,0));           // Turn the next LED off
    }
    strip.show();                    // Show the every other sequence
    delay(300);                      // Wait briefly
    for(int c = 0; c < 24; c++){     // Walk through every LED
      strip.setPixelColor(c, strip.Color(0,0,0));           // Turn the white LED off
      c++;                           // Skip ahead in the for loop
      strip.setPixelColor(c, strip.Color(80,80,80));        // Turn the off LED white
    }
    strip.show();                    // Show the switched every other sequence
    delay(300);                      // Pause briefly
  }
}
//-----------------------------------------------------------------------------------------------------------
void loser(){                        // Sound the loser sequence
  zeroStrip();                       // Clear LED locations
  strip.show();                      // Turn all off
  tone(buzz, 523, 800);              // Play a slow High C
  delay(1000);                       // Rest briefly after the note finishes
  tone(buzz, 392, 800);              // Play a slow G
  delay(1000);                       // Rest briefly after the note finishes
  tone(buzz, 262, 800);              // Play a slow C
  delay(1000);                       // Rest briefly after the note finishes
  for(int x = 0; x < 5; x++){        // Go through the sequence 5 times
    for(int a = 0; a < 5; a++){      // Go through the 5 middle LEDs
      strip.setPixelColor(11+a, strip.Color(100,0,0));      // Turn them red
    }
    strip.show();                    // Show the 5 Red LEDs
    delay(1000);                     // Wait 1 second
    zeroStrip();                     // Clear the strip
    strip.show();                    // Turn the LEDs off
    delay(1000);                     // Wait 1 second
    if(!digitalRead(btn2)){          // If the user is holding the middle button
      gameOver = false;              // The game is not over
      reloadCnt = 0;                 // Clear any reload timer
      reload = false;                // Ready weapon
      x = 5;                         // Exit loop
      startGame();                   // Run start game countdown
    }
  }
}
//-----------------------------------------------------------------------------------------------------------
void zeroStrip(){                    // Change all LED locations to 0
  for(int i = 0; i < 25; i++){       // For each LED
    strip.setPixelColor(i, strip.Color(0,0,0));             // Set the LED to off
  }
}
//-----------------------------------------------------------------------------------------------------------
void checkTriggers(){                // See if the weopon is being fired
  if(!digitalRead(btn1) || !digitalRead(btn3)){             // If the left or right button are pressed
    if(ship == enemy || ship == enemy-1){                   // If the ship is locked on to the enemy
      enemy = 0;                     // Destory the enemy
      colorCnt = 0;                  // Restart the enemy timer
      if(enemyCnt > 30) enemyCnt -= 3;                      // If it's not over, lessen the enemy timer
      else winner = true;            // Otherwise sound the victory
      digitalWrite(vibe, HIGH);      // Shake the contollor
      delay(200);                    // Signal the hit briefly
      digitalWrite(vibe, LOW);       // Turn the vibe motor back off
    }
    else reload = true;              // If the user missed, start the reload timer
  }
}
//-----------------------------------------------------------------------------------------------------------
void startGame(){                    // Countdown the start of the game
  for(int a = 0; a < 5; a++){        // For the middle five LEDs
    strip.setPixelColor(11+a, strip.Color(0,200,0));        // Turn them green
  }
  strip.show();                      // Show the green
  delay(500);                        // For half of a second
  zeroStrip();                       // Clear strip
  strip.show();                      // Turn strip off
  delay(500);                        // For half of a second
  for(int a = 0; a < 5; a++){        // For the middle five LEDs
    strip.setPixelColor(11+a, strip.Color(100,100,0));      // Turn them yellow
  }
  strip.show();                      // Show the yellow
  delay(500);                        // For half of a second
  zeroStrip();                       // Clear strip
  strip.show();                      // Turn strip off
  delay(500);                        // For half of a second
  for(int a = 0; a < 5; a++){
    strip.setPixelColor(11+a, strip.Color(200,0,0));        // Turn them red
  }
  strip.show();                      // Show the red
  delay(500);                        // For half of a second
  zeroStrip();                       // Clear strip
  strip.show();                      // Turn strip off
  delay(500);                        // For half of a second
}
//-----------------------------------------------------------------------------------------------------------
