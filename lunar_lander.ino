// -----------------------------------------------------------
// MEGGY JR: LUNAR LANDER LANDSCAPE SCROLL (V14 - Final Code)
// -----------------------------------------------------------

// 1. INCLUDE THE MEGGY SIMPLE LIBRARY 
#include <MeggyJrSimple.h>

// 2. DEFINE CONSTANTS
// We use numerical values for all colors.
#define COLOR_MOUNTAIN 3    
#define COLOR_LANDER   1    
#define COLOR_THRUST   2    
#define COLOR_SKY      0    

#define MAX_HEIGHT     7    
#define SCREEN_WIDTH   8    
#define SCREEN_HEIGHT  8    
#define SCROLL_DELAY  50    

// Horizontal Physics Constants
#define MAX_SCROLL_VELOCITY  1.5  
#define SCROLL_ACCEL         0.15 
#define SCROLL_INERTIA_DECAY 0.05 

// Vertical Physics Constants
#define MAX_VELOCITY       1.0  
#define CONTINUOUS_GRAVITY -0.01 // Pulls DOWN (Y decreases towards 0)
#define THRUST_ACCEL       0.05 // Pushes UP (Y increases towards 7)

// Safe Landing & Tone Constants
#define SAFE_LANDING_SPEED 0.3  // Descent rate threshold (must be > -0.3)
#define HAPPY_TONE_FREQ    1000 // High-pitched tone frequency
#define HAPPY_TONE_DUR     300  // Duration of the happy tone in milliseconds
#define CRASH_TONE_FREQ    100  // Low-pitched tone for rough landing
#define CRASH_TONE_DUR     100  

// 3. GLOBAL VARIABLES
#define LANDSCAPE_SIZE 100 

int mountainHeights[LANDSCAPE_SIZE]; 
float landscapeOffset;              
float scrollSpeed;                  

const int dotX = 3;                 
float dotY;                         
float dotVelocityY;                 

bool flickerState = false;       // Toggles between true/false for flicker effect
bool isMuted = false;            // NEW: Tracks if sound is muted
bool landedTonePlayed = false;   // NEW: Tracks if the tone has played for the current landing

// -----------------------------------------------------------
// HELPER FUNCTION: GENERATE NEW LANDSCAPE DATA
// -----------------------------------------------------------

void generateLandscape() {
  mountainHeights[0] = random(1, 3);
  
  for (int x = 1; x < LANDSCAPE_SIZE; x++) {
    int lastHeight = mountainHeights[x - 1]; 
    int heightChange = random(-2, 3); 
    int newHeight = lastHeight + heightChange;

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

  // Initialize new state variables
  isMuted = false;
  landedTonePlayed = false;
}

// -----------------------------------------------------------
// LOOP FUNCTION
// -----------------------------------------------------------
void loop() {
  
  CheckButtonsDown(); 
  
  // --- A. MUTE TOGGLE ---
  if (Button_B) {
    isMuted = !isMuted; // Toggle the mute state
    if (!isMuted) {
      // Confirmation beep for Unmute
      Tone_Start(800, 50); 
    }
  }

  // --- Check for Airborne State ---
  int currentLandscapeIndex = ((int)landscapeOffset + dotX) % LANDSCAPE_SIZE;
  int groundHeight = mountainHeights[currentLandscapeIndex]; 
  
  bool isAirborne = (dotY > (float)groundHeight);

  // If the lander is flying, reset the tone flag so it can play on the next landing
  if (isAirborne) {
    landedTonePlayed = false;
  }
  
  // ===================================
  // STEP 1: APPLY FORCES AND ACCELERATION
  // ===================================
  
  // --- HORIZONTAL SCROLL CONTROL (GATED) ---
  if (isAirborne) {
      
    // 1. ACCELERATION
    if (Button_Left) {
      scrollSpeed = scrollSpeed - SCROLL_ACCEL; 
    }
    if (Button_Right) {
      scrollSpeed = scrollSpeed + SCROLL_ACCEL; 
    }
    
    // 2. INERTIA DECAY
    if (scrollSpeed > 0.0) {
      scrollSpeed = scrollSpeed - SCROLL_INERTIA_DECAY;
      if (scrollSpeed < 0.0) scrollSpeed = 0.0; 
    } else if (scrollSpeed < 0.0) {
      scrollSpeed = scrollSpeed + SCROLL_INERTIA_DECAY;
      if (scrollSpeed > 0.0) scrollSpeed = 0.0; 
    }

    // 3. CLAMP horizontal scroll speed
    if (scrollSpeed > MAX_SCROLL_VELOCITY) {
      scrollSpeed = MAX_SCROLL_VELOCITY;
    } else if (scrollSpeed < -MAX_SCROLL_VELOCITY) {
      scrollSpeed = -MAX_SCROLL_VELOCITY;
    }
    
  } else {
    // If landed, stop all horizontal movement immediately
    scrollSpeed = 0.0;
  }
  
  // --- B. CONTINUOUS VERTICAL FORCES ---
  
  // 1. GRAVITY 
  dotVelocityY = dotVelocityY + CONTINUOUS_GRAVITY;
  
  // 2. THRUST 
  if (Button_A) {
    dotVelocityY = dotVelocityY + THRUST_ACCEL;
    
    // Play sound ONLY if unmuted
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
    
    // If the tone has NOT been played for this landing yet:
    if (!landedTonePlayed) {
        
        // Check for SAFE LANDING
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
        
        // Mark the tone as played so it doesn't repeat until lift-off
        landedTonePlayed = true;
    }
    
    // Stop the lander
    dotY = (float)groundHeight;
    dotVelocityY = 0.0; 
  }

  // 3. CLAMPING (Top and Bottom Edges)
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
    
    for (int y = 0; y < height; y++) {
      DrawPx(screenX, y, COLOR_MOUNTAIN);
    }
  }
  
  // --- DRAW PLAYER DOT ---
  // Draw the thruster flicker one pixel below the lander
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