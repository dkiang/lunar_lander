// -----------------------------------------------------------
// MEGGY JR: LUNAR LANDER LANDSCAPE SCROLL (FINAL CODE)
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

// Horizontal Physics Constants
#define SCROLL_ACCEL_RATE    0.08 // Rate at which scroll speed changes when button is held
#define SCROLL_INERTIA_DECAY 0.05 // Gradual slowdown to zero
#define MAX_SCROLL_SPEED     0.5  // Maximum scroll speed limit

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

const int dotX = 3;                 // Lander's fixed X position
float dotY;                         
float dotVelocityY;                 

bool flickerState = false;       
bool isMuted = false;            
bool landedTonePlayed = false;   

// Variables for Button State Tracking
bool lastButtonBState = false; 

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

  isMuted = false;
  landedTonePlayed = false;
  
  CheckButtonsDown(); 
  lastButtonBState = Button_B;
}

// -----------------------------------------------------------
// LOOP FUNCTION
// -----------------------------------------------------------
void loop() {
  
  // Read current button held state
  CheckButtonsDown(); 
  
  // --- MUTE TOGGLE (Using internal debouncing for toggle logic) ---
  if (Button_B && !lastButtonBState) {
    isMuted = !isMuted; 
    if (!isMuted) {
      Tone_Start(800, 50); 
    }
  }
  lastButtonBState = Button_B; // Update the state of button B

  // --- Check for Airborne State ---
  int currentLandscapeIndex = ((int)landscapeOffset + dotX) % LANDSCAPE_SIZE;
  int groundHeight = mountainHeights[currentLandscapeIndex]; 
  
  bool isAirborne = (dotY > (float)groundHeight);

  if (isAirborne) {
    landedTonePlayed = false;
  }
  
  // ===================================
  // STEP 1: APPLY FORCES AND ACCELERATION
  // ===================================
  
  // --- A. HORIZONTAL SCROLL CONTROL ---
  if (isAirborne) {
      
    if (Button_Left) {
        // SCROLL RIGHT: Add positive acceleration (mountains move Right)
        scrollSpeed = scrollSpeed + SCROLL_ACCEL_RATE;
    }
    
    if (Button_Right) {
        // SCROLL LEFT: Add negative acceleration (mountains move Left)
        scrollSpeed = scrollSpeed - SCROLL_ACCEL_RATE;
    }
    
    // Gradual Slowdown (Decay/Inertia)
    if (!Button_Left && !Button_Right) {
      if (scrollSpeed > 0.0) {
        scrollSpeed = scrollSpeed - SCROLL_INERTIA_DECAY;
        if (scrollSpeed < 0.0) scrollSpeed = 0.0; 
      } else if (scrollSpeed < 0.0) {
        scrollSpeed = scrollSpeed + SCROLL_INERTIA_DECAY;
        if (scrollSpeed > 0.0) scrollSpeed = 0.0; 
      }
    }
    
    // CLAMP horizontal scroll speed
    if (scrollSpeed > MAX_SCROLL_SPEED) {
      scrollSpeed = MAX_SCROLL_SPEED;
    } else if (scrollSpeed < -MAX_SCROLL_SPEED) {
      scrollSpeed = -MAX_SCROLL_SPEED;
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
    
    if (!isMuted) {
      Tone_Start(500, SCROLL_DELAY); 
    }
    // Vertical thrust toggles the flicker state for all thrusters
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
    
    for (int y = 0; y < height; y++) {
      DrawPx(screenX, y, COLOR_MOUNTAIN);
    }
  }
  
  // --- DRAW PLAYER DOT AND THRUSTERS ---
  
  int landerY = (int)dotY;

  // Vertical Thruster (Flame BELOW lander)
  if (Button_A && landerY > 0) {
      if (flickerState) {
          DrawPx(dotX, landerY - 1, COLOR_THRUST);
      }
  }
  
  // Horizontal Thruster Flicker (Flames to the sides)
  if (isAirborne && flickerState) {

      // Flame for SCROLL RIGHT (Caused by Button_Left) -> Flame on LEFT side of lander
      if (Button_Left && dotX > 0) {
          DrawPx(dotX - 1, landerY, COLOR_THRUST);
      }
      
      // Flame for SCROLL LEFT (Caused by Button_Right) -> Flame on RIGHT side of lander
      if (Button_Right && dotX < SCREEN_WIDTH - 1) {
          DrawPx(dotX + 1, landerY, COLOR_THRUST);
      }
  }
  
  // Draw the lander itself (Drawn last to be on top of the thruster flames)
  DrawPx(dotX, landerY, COLOR_LANDER);
  
  // ===================================
  // STEP 5: DISPLAY AND DELAY
  // ===================================
  
  DisplaySlate();
  
  delay(SCROLL_DELAY);
}