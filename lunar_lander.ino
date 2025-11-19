// -----------------------------------------------------------
// MEGGY JR: LUNAR LANDER LANDSCAPE SCROLL (FINAL VERSION)
// -----------------------------------------------------------

// 1. INCLUDE THE MEGGY SIMPLE LIBRARY 
#include <MeggyJrSimple.h>

// 2. DEFINE CONSTANTS
// We use numerical values for all colors.
#define COLOR_MOUNTAIN 3    
#define COLOR_LANDER   1    
#define COLOR_THRUST   2    
#define COLOR_SKY      0    

#define MAX_HEIGHT     6    // Max height is 6 pixels high
#define SCREEN_WIDTH   8    
#define SCREEN_HEIGHT  8    
#define SCROLL_DELAY  50    

// Horizontal Physics Constants (Simplified)
#define SCROLL_TOGGLE_SPEED 0.3 // Fixed speed for scroll toggle

// Vertical Physics Constants
#define MAX_VELOCITY       1.0  
#define CONTINUOUS_GRAVITY -0.01 // Pulls DOWN (Y decreases towards 0)
#define THRUST_ACCEL       0.05 // Pushes UP (Y increases towards 7)

// Safe Landing & Tone Constants
#define SAFE_LANDING_SPEED 0.3  // Descent rate threshold
#define HAPPY_TONE_FREQ    1000 
#define HAPPY_TONE_DUR     300  
#define CRASH_TONE_FREQ    100  
#define CRASH_TONE_DUR     100  

// 3. GLOBAL VARIABLES
#define LANDSCAPE_SIZE 100 

int mountainHeights[LANDSCAPE_SIZE]; 
float landscapeOffset;              
float scrollSpeed;                  

const int dotX = 3;                 
float dotY;                         
float dotVelocityY;                 

bool flickerState = false;       
bool isMuted = false;            
bool landedTonePlayed = false;   

// -----------------------------------------------------------
// HELPER FUNCTION: GENERATE NEW LANDSCAPE DATA
// -----------------------------------------------------------

void generateLandscape() {
  mountainHeights[0] = random(1, 3);
  
  for (int x = 1; x < LANDSCAPE_SIZE; x++) {
    int lastHeight = mountainHeights[x - 1]; 
    int heightChange = random(-2, 3); 
    int newHeight = lastHeight + heightChange;

    // CLAMPING: Check against MAX_HEIGHT
    if (newHeight > MAX_HEIGHT) {
      newHeight = MAX_HEIGHT;
    } else if (newHeight < 1) {
      newHeight = 1; 
    }
    
    mountainHeights[x] = newHeight;
  }
}

// -----------------------------------------------------------
// SETUP FUNCTION
// -----------------------------------------------------------
void setup() {
  MeggyJrSimpleSetup();
  
  randomSeed(analogRead(0)); 
  generateLandscape();
  
  landscapeOffset = LANDSCAPE_SIZE / 2.0; 
  scrollSpeed = 0.0; 
  dotY = SCREEN_HEIGHT * 0.75; 
  dotVelocityY = 0.0; 

  isMuted = false;
  landedTonePlayed = false;
}

// -----------------------------------------------------------
// LOOP FUNCTION
// -----------------------------------------------------------
void loop() {
  
  CheckButtonsDown(); 
  
  // --- MUTE TOGGLE ---
  if (Button_B) {
    isMuted = !isMuted; 
    // Play confirmation sound only if unmuted
    if (!isMuted) {
      Tone_Start(800, 50); 
    }
  }

  // --- Check for Airborne State ---
  int currentLandscapeIndex = ((int)landscapeOffset + dotX) % LANDSCAPE_SIZE;
  int groundHeight = mountainHeights[currentLandscapeIndex]; 
  
  bool isAirborne = (dotY > (float)groundHeight);

  // If the lander lifts off, reset the tone flag
  if (isAirborne) {
    landedTonePlayed = false;
  }
  
  // ===================================
  // STEP 1: APPLY FORCES AND ACCELERATION
  // ===================================
  
  // --- A. HORIZONTAL SCROLL CONTROL (TOGGLE LOGIC) ---
  if (isAirborne) {
      
    // Right button pressed: 
    if (Button_Right) {
        // 1. BRAKE: If mountains are moving RIGHT (> 0.0), pressing RIGHT stops.
        if (scrollSpeed > 0.0) { 
            scrollSpeed = 0.0;
        } else {
            // 2. MOVE: Otherwise, set mountains to scroll LEFT (negative speed).
            scrollSpeed = -SCROLL_TOGGLE_SPEED; 
        }
    }
    
    // Left button pressed: 
    if (Button_Left) {
        // 1. BRAKE: If mountains are moving LEFT (< 0.0), pressing LEFT stops.
        if (scrollSpeed < 0.0) {
            scrollSpeed = 0.0;
        } else {
            // 2. MOVE: Otherwise, set mountains to scroll RIGHT (positive speed).
            scrollSpeed = SCROLL_TOGGLE_SPEED;
        }
    }
    
  } else {
    // If landed, stop all horizontal movement
    scrollSpeed = 0.0;
  }
  
  // --- B. CONTINUOUS VERTICAL FORCES ---
  
  // 1. GRAVITY 
  dotVelocityY = dotVelocityY + CONTINUOUS_GRAVITY;
  
  // 2. THRUST 
  if (Button_A) {
    dotVelocityY = dotVelocityY + THRUST_ACCEL;
    
    // Check mute state before playing tone
    if (!isMuted) {
      Tone_Start(500, SCROLL_DELAY); 
    }
    flickerState = !flickerState;
  } else {
    flickerState = false;
  }
  
  // CLAMP vertical velocity
  if (dotVelocityY > MAX_VELOCITY) {
    dotVelocityY = MAX_VELOCITY;
  } else if (dotVelocityY < -MAX_VELOCITY) {
    dotVelocityY = -MAX_VELOCITY;
  }
  
  // ===================================
  // STEP 2: APPLY VELOCITY AND COLLISION CHECK
  // ===================================

  // 1. Apply velocity to position
  dotY = dotY + dotVelocityY; 
  
  // 2. Collision Check 
  if ( dotY < (float)groundHeight ) {
    
    // Check if the landing tone has NOT been played yet
    if (!landedTonePlayed) {
        
        // SAFE LANDING check
        if (dotVelocityY < 0.0 && dotVelocityY > -SAFE_LANDING_SPEED) {
            if (!isMuted) {
                // SUCCESS! Play the happy tone.
                Tone_Start(HAPPY_TONE_FREQ, HAPPY_TONE_DUR); 
            }
        } else if (dotVelocityY < -SAFE_LANDING_SPEED) {
             // CRASH!
            if (!isMuted) {
                Tone_Start(CRASH_TONE_FREQ, CRASH_TONE_DUR); 
            }
        }
        
        // Mark tone as played
        landedTonePlayed = true;
    }
    
    // Stop the lander
    dotY = (float)groundHeight;
    dotVelocityY = 0.0; 
  }

  // 3. CLAMPING (Screen Edges)
  if (dotY <= 0.0) {
    dotY = 0.0; 
    dotVelocityY = 0.0; 
  }
  if (dotY >= SCREEN_HEIGHT - 1) {
    dotY = SCREEN_HEIGHT - 1; 
    dotVelocityY = 0.0; 
  }
  
  // ===================================
  // STEP 3: APPLY SCROLL SPEED TO OFFSET
  // ===================================
  
  landscapeOffset = landscapeOffset + scrollSpeed; 
  
  // WRAP-AROUND for endless scrolling
  if (landscapeOffset < 0.0) {
    landscapeOffset = landscapeOffset + LANDSCAPE_SIZE;
  }
  if (landscapeOffset >= LANDSCAPE_SIZE) {
    landscapeOffset = landscapeOffset - LANDSCAPE_SIZE;
  }
  
  // ===================================
  // STEP 4: DRAW THE SCENE
  // ===================================
  
  ClearSlate(); 
  
  // --- DRAW MOUNTAINS ---
  for (int screenX = 0; screenX < SCREEN_WIDTH; screenX++) {
    int landscapeIndex = ((int)landscapeOffset + screenX) % LANDSCAPE_SIZE;
    int height = mountainHeights[landscapeIndex];
    
    // Draw from the base (y=0) up to the mountain's height (y=height-1).
    for (int y = 0; y < height; y++) {
      DrawPx(screenX, y, COLOR_MOUNTAIN);
    }
  }
  
  // --- DRAW PLAYER DOT ---
  // Draw the thruster flicker
  if (Button_A && (int)dotY > 0) {
      if (flickerState) {
          DrawPx(dotX, (int)dotY - 1, COLOR_THRUST);
      }
  }
  
  // Draw the lander itself
  DrawPx(dotX, (int)dotY, COLOR_LANDER);
  
  // ===================================
  // STEP 5: DISPLAY AND DELAY
  // ===================================
  
  DisplaySlate();
  
  delay(SCROLL_DELAY);
}