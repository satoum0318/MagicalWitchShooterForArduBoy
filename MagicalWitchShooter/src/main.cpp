#include <Arduboy2.h>

Arduboy2 arduboy;

// Game states
enum GameState {
  OPENING,
  PLAYING,
  ENDING
};

GameState gameState = OPENING;


// 16x16スプライトデータ
const unsigned char PROGMEM witchSprite[] = {
  0x18, 0x3C, 0x7E, 0x7E, 0x3C, 0x5A, 0x18, 0x00
};
const unsigned char PROGMEM skeletonSprite[] = {
  0x3C, 0x7E, 0xFF, 0xDB, 0xFF, 0x66, 0x3C, 0x3C
};

const unsigned char PROGMEM powerupSprite[] = {
  0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x3C, 0x18
};

const unsigned char PROGMEM bossSprite[] = {
  0x3C, 0x3C, 0x7E, 0x7E, 0xFF, 0xFF, 0xDB, 0xDB,
  0xFF, 0xFF, 0x66, 0x66, 0x3C, 0x3C, 0x3C, 0x3C,
  0x3C, 0x3C, 0x7E, 0x7E, 0xFF, 0xFF, 0xDB, 0xDB,
  0xFF, 0xFF, 0x66, 0x66, 0x3C, 0x3C, 0x3C, 0x3C
};

const unsigned char PROGMEM bulletSprite[] = {
  0x06, 0x0F, 0x0F, 0x06
};

const unsigned char PROGMEM barrierSprite[] = {
  0xFF, 0xFF, 0x81, 0xFF, 0xBD, 0xFF, 0xA1, 0xFF,
  0xA1, 0xFF, 0xBD, 0xFF, 0x81, 0xFF, 0xFF, 0xFF
};

// Player
struct Player {
  int x;
  int y;
  int health;
  bool hasBarrier;
  unsigned long barrierTime;
} player;

// Bullet
const int MAX_BULLETS = 5;
struct Bullet {
  int x;
  int y;
  bool active;
} bullets[MAX_BULLETS];

// Enemy
const int MAX_ENEMIES = 3;
struct Enemy {
  int x;
  int y;
  bool active;
} enemies[MAX_ENEMIES];

// Powerup
struct PowerUp {
  int x;
  int y;
  bool active;
} powerup;

// Boss
struct Boss {
  int x;
  int y;
  int health;
  bool active;
} boss;

// Score
int score = 0;

// Play time
unsigned long gameTime = 0;

// Function prototypes
void resetGame();
void drawOpening();
void updateGame();
void drawGame();
void drawEnding();
bool collide(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(60);
  arduboy.initRandomSeed();
  resetGame();
}

void loop() {
  if (!arduboy.nextFrame()) return;

  arduboy.pollButtons();
  arduboy.clear();

  switch (gameState) {
    case OPENING:
      drawOpening();
      if (arduboy.justPressed(A_BUTTON)) {
        gameState = PLAYING;
        gameTime = millis();
      }
      break;
    case PLAYING:
      updateGame();
      drawGame();
      if (player.health <= 0 || (millis() - gameTime) > 60000) {
        gameState = ENDING;
      }
      break;
    case ENDING:
      drawEnding();
      if (arduboy.justPressed(A_BUTTON)) {
        resetGame();
        gameState = OPENING;
      }
      break;
  }

  arduboy.display();
}

void updateGame() {
  // Move player
  if (arduboy.pressed(UP_BUTTON) && player.y > 0) player.y--;
  if (arduboy.pressed(DOWN_BUTTON) && player.y < HEIGHT - 16) player.y++;

  // Shoot
  if (arduboy.justPressed(A_BUTTON)) {
    for (int i = 0; i < MAX_BULLETS; i++) {
      if (!bullets[i].active) {
        bullets[i].x = player.x + 16;
        bullets[i].y = player.y + 6;
        bullets[i].active = true;
        break;
      }
    }
  }

  // Guard
  if (arduboy.justPressed(B_BUTTON) && !player.hasBarrier) {
    player.hasBarrier = true;
    player.barrierTime = millis();
  }

  // Check guard time
  if (player.hasBarrier && millis() - player.barrierTime > 3000) {
    player.hasBarrier = false;
  }

  // Move bullets
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      bullets[i].x += 2;
      if (bullets[i].x > WIDTH) bullets[i].active = false;
    }
  }

  // Generate enemies
  if (random(100) < 5) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (!enemies[i].active) {
        enemies[i].x = WIDTH;
        enemies[i].y = random(HEIGHT - 8);
        enemies[i].active = true;
        break;
      }
    }
  }

  // Move enemies
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      enemies[i].x -= 1;
      if (enemies[i].x < 0) enemies[i].active = false;
    }
  }


  // Generate boss
  if (score >= 50 && !boss.active) {
    boss.active = true;
    boss.x = WIDTH;
    boss.y = HEIGHT / 2;
    boss.health = 10;  // ボスの体力を20に設定
  }

  // Move boss
  if (boss.active) {
    boss.x -= 1;
    if (boss.x < 0) boss.x = WIDTH;

  // 
  if (random(100) < 10) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (!enemies[i].active) {
          enemies[i].x = boss.x;
          enemies[i].y = boss.y;
          enemies[i].active = true;
          break;
        }
      }
    }
  }
  // Generate powerup
  if (!powerup.active && random(1000) < 5) {
    powerup.x = WIDTH;
    powerup.y = random(HEIGHT - 8);
    powerup.active = true;
  }

  // Move powerup
  if (powerup.active) {
    powerup.x -= 1;
    if (powerup.x < 0) powerup.active = false;
  }

  // Collision detection: bullets and enemies
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      for (int j = 0; j < MAX_ENEMIES; j++) {
        if (enemies[j].active) {
          if (collide(bullets[i].x, bullets[i].y, 4, 4, enemies[j].x, enemies[j].y, 8, 8)) {
            bullets[i].active = false;
            enemies[j].active = false;
            score += 10;
          }
        }
      }
    }
  }

  // Collision detection: player and enemies
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      if (collide(player.x, player.y, 16, 16, enemies[i].x, enemies[i].y, 8, 8)) {
        enemies[i].active = false;
        if (!player.hasBarrier) {
          player.health--;
        }
      }
    }
  }

  // Collision detection: player and powerup
  if (powerup.active) {
    if (collide(player.x, player.y, 16, 16, powerup.x, powerup.y, 8, 8)) {
      powerup.active = false;
      player.health = min(player.health + 1, 5);  // Max health is 5
    }
  }

  // Boss logic (to be implemented)
  if (score >= 100 && !boss.active) {
    boss.active = true;
    boss.x = WIDTH;
    boss.y = HEIGHT / 2 - 8;
    boss.health = 10;
  }

  if (boss.active) {
    // Move boss
    if (boss.x > WIDTH - 32) boss.x--;

    // Boss attack pattern (to be implemented)

    // Collision detection: bullets and boss
    for (int i = 0; i < MAX_BULLETS; i++) {
      if (bullets[i].active) {
        if (collide(bullets[i].x, bullets[i].y, 4, 4, boss.x, boss.y, 16, 16)) {
          bullets[i].active = false;
          boss.health--;
          if (boss.health <= 0) {
            boss.active = false;
            score += 100;
          }
        }
      }
    }

    // Collision detection: player and boss
    if (collide(player.x, player.y, 16, 16, boss.x, boss.y, 16, 16)) {
      if (!player.hasBarrier) {
        player.health--;
      }
    }
  }
}

void drawGame() {
  // Draw player (witch)
  arduboy.drawBitmap(player.x, player.y, witchSprite, 8, 8, WHITE);

  // Draw barrier
  if (player.hasBarrier) {
    arduboy.drawBitmap(player.x - 8, player.y - 8, barrierSprite, 16, 16, WHITE);
  }

  // Draw bullets
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      arduboy.drawBitmap(bullets[i].x, bullets[i].y, bulletSprite, 4, 4, WHITE);
    }
  }

  // Draw enemies (skeletons)
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      arduboy.drawBitmap(enemies[i].x, enemies[i].y, skeletonSprite, 8, 8, WHITE);
    }
  }

  // Draw powerup
  if (powerup.active) {
    arduboy.drawBitmap(powerup.x, powerup.y, powerupSprite, 8, 8, WHITE);
  }

  // Draw boss
  if (boss.active) {
    arduboy.drawBitmap(boss.x, boss.y, bossSprite, 16, 16, WHITE);
  }

  // Draw score and health
  arduboy.setCursor(0, 0);
  arduboy.print(F("Score: "));
  arduboy.print(score);
  arduboy.setCursor(70, 0);
  arduboy.print(F("HP: "));
  arduboy.print(player.health);
}

void drawOpening() {
  arduboy.setCursor(10, 20);
  arduboy.print(F("Witch Shooter"));
  arduboy.setCursor(10, 40);
  arduboy.print(F("Press A to start"));
}

void drawEnding() {
  arduboy.setCursor(10, 20);
  arduboy.print(F("Game Over"));
  arduboy.setCursor(10, 30);
  arduboy.print(F("Score: "));
  arduboy.print(score);
  arduboy.setCursor(10, 40);
  arduboy.print(F("Press A to restart"));
}

bool collide(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
  return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

void resetGame() {
  player.x = 10;
  player.y = HEIGHT / 2 - 8;
  player.health = 3;
  player.hasBarrier = false;
  score = 0;
  for (int i = 0; i < MAX_BULLETS; i++) {
    bullets[i].active = false;
  }
  for (int i = 0; i < MAX_ENEMIES; i++) {
    enemies[i].active = false;
  }
  powerup.active = false;
  boss.active = false;
}