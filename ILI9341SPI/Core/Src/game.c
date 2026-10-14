/*
 * game.c
 *
 * Lógica completa del videojuego. Gameplay, títlescree, reception uart, música, renderizado.
 */
#include "game.h"
#include "ili9341.h"
#include "graphics.h"
#include "stage.h"
#include "sonidos.h"
#include "highscores.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

//Controles actualizados desde la interrupcion USART3
volatile uint8_t g_moveLeft = 0;
volatile uint8_t g_moveRight = 0;
volatile uint8_t g_jumpRequest = 0;
static uint8_t serialRx[2];
static volatile uint8_t serialJumpHeld = 0;
extern UART_HandleTypeDef huart3;

// Detección de jugador en suelo/plataforma
static const int16_t FLOOR_Y = STAGE_FLOOR_Y - PLAYER_H;

/* Hitbox de Mario: el sprite sigue siendo 16x32.
 * Se recorta 1 px a izquierda/derecha y se usan solo los 19 px inferiores para mejorar gamefeel. */
#define MARIO_HITBOX_X_OFFSET 1
#define MARIO_HITBOX_Y_OFFSET (PLAYER_H - 19)
#define MARIO_HITBOX_W (PLAYER_W - 2)
#define MARIO_HITBOX_H 19

typedef struct {
	int16_t x, y;
	int16_t prevX, prevY;
	int16_t vy;
    int16_t verticalRemainder;
	uint8_t onGround;
} Player;

static Player player;
static uint8_t facingLeft = 0; // 0 = viendo a la derecha, 1 = viendo a la izquierda

/* Ajustes del enemigo. El sprite original mira hacia la izquierda. Todos siguen la misma base de koopa (exeptuando sprites) */
#define KOOPA_MOVE_SPEED 1
#define KOOPA_WALK_FRAME_MS 120U
#define KOOPA_VULNERABLE_FRAME_MS 160U
#define KOOPA_VULNERABLE_DURATION_MS 4000U
#define KOOPA_BUMP_VELOCITY (-512)
#define KOOPA_DEFEAT_VELOCITY (-768)
#define KOOPA_GRAVITY 48
#define KOOPA_MAX_FALL 1024
#define KOOPA_TOP_PIPE_Y 48
#define KOOPA_TOP_LEFT_MOUTH_X 32
#define KOOPA_TOP_RIGHT_MOUTH_X (SCREEN_WIDTH - 32 - KOOPA_W)
#define KOOPA_VULNERABLE_FIRST_FRAME (KOOPA_FRAME_COUNT - 3) /* penultimos 2: frames 5 y 6 del sprite*/
#define MAX_KOOPAS 4
#define LEVEL_TOTAL_KOOPAS 15
#define KOOPA_SPAWN_INTERVAL_MS 5000U
#ifndef KOOPA_SPAWN_FROM_LEFT
#define KOOPA_SPAWN_FROM_LEFT 0 /* 0: spawnea derecha; 1: spawnea izquierda */
#define DEATH_JUMP_VELOCITY (-1152)

//Definiciones para freezeos de pantalla, temporizadores, etc.
#define LEVEL_CLEAR_FREEZE_MS 1000U
#define LEVEL_TIME_SECONDS 200U
#define COMBO_WINDOW_MS 1000U
#define TIME_BONUS_POINTS 50U
#define BONUS_STEP_MS 20U
#define TITLE_FRAME_MS 700U
#define MARIO_WALK_FRAME_MS 90U
#define MARIO_DEATH_FRAME_MS 180U
#define DEATH_FREEZE_MS 500U
#define DEATH_POST_MUSIC_PAUSE_MS 1000U


#endif

typedef enum {
    KOOPA_NORMAL = 0,
    KOOPA_BUMPED,
    KOOPA_VULNERABLE,
    KOOPA_DEFEATED,
    KOOPA_RESPAWN
} KoopaState;

//Structo para manejar enemigos
typedef struct {
    int16_t x, y;
    int16_t vy;
    int16_t verticalRemainder;
    int16_t bumpGroundY;
    int8_t direction;
    uint8_t frame;
    uint8_t spawnPipe;
    KoopaState state;
    uint32_t animationTick;
    uint32_t vulnerableTick;
    uint8_t active;
    uint8_t isCrab;
    uint8_t isFreezie;
    uint8_t angry;
    uint8_t fellFromPlatform;
    uint16_t horizontalRemainder;
} Koopa;
static Koopa koopas[MAX_KOOPAS];
static uint8_t levelSpawned;
static uint8_t levelDefeated;
static uint32_t lastSpawnTick;
static uint8_t levelClearFreeze;
static uint8_t levelClearPending;
static int8_t levelClearKoopaIndex;
static uint32_t levelClearTick;
//Flags para proceso de muerte
typedef enum {
    DEATH_NONE = 0,
    DEATH_FREEZE,
    DEATH_FALL,
    DEATH_POST_MUSIC_PAUSE
} DeathState;
static DeathState deathState;
static uint32_t deathTick;
static uint8_t marioFrame;
static uint32_t marioAnimationTick;

//Flags para lógica para organicación del nivel
typedef enum { GAME_TITLE = 0, GAME_GAMEOVER, GAME_LEVEL_INTRO, GAME_PLAYING, GAME_TIME_BONUS, GAME_LEVEL_FREEZE } GameMode;
static GameMode gameMode = GAME_TITLE;
static uint8_t lives = 3;
static uint32_t score = 0;
static uint32_t levelStartTick = 0;
static uint16_t timeRemaining = LEVEL_TIME_SECONDS;
static uint32_t lastDefeatTick = 0;
static uint8_t comboCount = 0;
static uint32_t bonusTick = 0;
static uint32_t titleTick = 0;
static uint8_t titleFrame = 0;
static uint8_t titleRenderFrame = 0;

//Animación en la pantalla de título y gameover
typedef struct {
    int16_t x;
    uint8_t mario, koopa, crab, freezie, fire, blueFire;
} TitleDemo;
static TitleDemo titleDemo, titleDemoRender;
static uint32_t titleDemoTick;
static uint16_t titleDemoPhase;
static uint8_t hudDirty = 1;
static uint32_t levelFreezeTick = 0;
static uint32_t highScores[HIGH_SCORE_COUNT];
static uint32_t finalScore;
static uint8_t gameOverNewTop1;
static uint8_t gameOverTextDirty;
static uint8_t highScoresNeedSave;

//Interactuables del nivel
#define POW_X 112
#define POW_Y 240
#define POW_SIZE 16
#define SHAKE_MS 360U
#define SHAKE_STEP_MS 60U
#define FIRE_SPAWN_MS 15000U
#define FIRE_FRAME_MS 120U
static uint16_t levelNumber = 1;
static uint8_t spawnedKoopas, spawnedCrabs, spawnedFreezies;
static int16_t iceVX, iceRemainder;
static uint8_t lifeSlots = 3;
static uint8_t powAvailable = 1;
static uint8_t shaking;
static uint32_t shakeTick;
static int8_t shakeX, shakeY, renderShakeX, renderShakeY;
static uint32_t lastFireTick;

//Bola de fuego
typedef struct {
    int16_t x, y;
    int8_t direction;
    uint8_t active, frame;
    uint16_t remainder;
    uint32_t animationTick;
} Fireball;
static Fireball fireball;

//Agregar hitbox al suelo/plataforma
static uint8_t WorldSolid(int x, int y) {
    return Stage_IsSolid(x, y) || (powAvailable && x >= POW_X &&
        x < POW_X+POW_SIZE && y >= POW_Y && y < POW_Y+POW_SIZE);
}
//Lógica de spawneo de enemigos
static uint32_t SpawnInterval(void) {
    int remaining = LEVEL_TOTAL_KOOPAS - levelDefeated;
    return remaining <= 6 ? 2000U : remaining <= 10 ? 3000U : KOOPA_SPAWN_INTERVAL_MS;
}
//Asignar sprites de vulnerables
static uint8_t VulnerableFirst(const Koopa *k) {
    if (k->isFreezie) return FREEZIE_DEATH_FIRST_FRAME;
    return k->isCrab ? 6U : KOOPA_VULNERABLE_FIRST_FRAME;
}

//lógica de animación de caminar
static uint8_t WalkFirst(const Koopa *k) { return k->isCrab && k->angry ? 3U : 0U; }
static uint8_t WalkCount(const Koopa *k) { return k->isFreezie ? FREEZIE_WALK_FRAMES : (k->isCrab ? 3U : KOOPA_WALK_FRAMES); }

//Aletoriedad y semilla
static uint32_t enemyRandom = 0x51A7C3E9U;
static uint32_t EnemyRandom(void) {
    enemyRandom ^= enemyRandom << 13;
    enemyRandom ^= enemyRandom >> 17;
    enemyRandom ^= enemyRandom << 5;
    return enemyRandom;
}

//Loopea enemigos de regreso encima del mapa por las tuberías
static void RespawnKoopaFromTopPipe(Koopa *k, uint32_t now) {
    k->spawnPipe = (uint8_t)(EnemyRandom() & 1U);
    if (k->spawnPipe == 0U) {
        k->x = KOOPA_TOP_LEFT_MOUTH_X;
        k->direction = 1;
    } else {
        k->x = KOOPA_TOP_RIGHT_MOUTH_X;
        k->direction = -1;
    }
    k->y = KOOPA_TOP_PIPE_Y;
    k->vy = 0;
    k->verticalRemainder = 0;
    k->bumpGroundY = k->y;
    k->frame = WalkFirst(k);
    k->horizontalRemainder = 0;
    k->state = KOOPA_NORMAL;
    k->animationTick = now;
    k->vulnerableTick = 0;
    k->fellFromPlatform = 0;
    k->active = 1;
}


//Verificar cuantos enemigos activos hay
static Koopa *FindFreeKoopa(void) {
    for (int i = 0; i < MAX_KOOPAS; ++i)
        if (!koopas[i].active) return &koopas[i];
    return 0;
}

//Lógica de spawneo según nivel
static void SpawnLevelKoopa(uint32_t now) {
    Koopa *k = FindFreeKoopa();
    if (!k || levelSpawned >= LEVEL_TOTAL_KOOPAS) return;
    int crabLeft = 0, koopaLeft = 0, freezieLeft = 0;
    if (levelNumber >= 3U) { crabLeft = 10 - spawnedCrabs; freezieLeft = 5 - spawnedFreezies; }
    else if (levelNumber == 2U) { crabLeft = 5 - spawnedCrabs; koopaLeft = 10 - spawnedKoopas; }
    else koopaLeft = 15 - spawnedKoopas;
    int totalLeft = crabLeft + koopaLeft + freezieLeft;
    uint32_t pick = totalLeft > 0 ? EnemyRandom() % (uint32_t)totalLeft : 0U;
    k->isCrab = (pick < (uint32_t)crabLeft);
    k->isFreezie = (!k->isCrab && pick >= (uint32_t)(crabLeft + koopaLeft));
    k->angry = 0; k->fellFromPlatform = 0;
    if (k->isCrab) ++spawnedCrabs; else if (k->isFreezie) ++spawnedFreezies; else ++spawnedKoopas;
    RespawnKoopaFromTopPipe(k, now);
    ++levelSpawned;
    lastSpawnTick = now;
}

//Enemgigo esta tocando el suelo
static uint8_t KoopaGroundBlocked(const Koopa *k, int x, int y) {
    int footY = y + KOOPA_H;
    if (footY >= STAGE_FLOOR_Y) return 1;
    if (footY < 0) return 0;
    for (int px = x; px < x + KOOPA_W; ++px) {
        if (px >= 0 && px < SCREEN_WIDTH && WorldSolid(px, footY)) return 1;
    }
    (void)k;
    return 0;
}

//Gravedad para enemigos
static void MoveKoopaNormalY(Koopa *k) {
    k->vy += KOOPA_GRAVITY;
    if (k->vy > KOOPA_MAX_FALL) k->vy = KOOPA_MAX_FALL;
    int total = k->verticalRemainder + k->vy;
    int dy = total / PHYSICS_SCALE;
    k->verticalRemainder = (int16_t)(total - dy * PHYSICS_SCALE);
    if (dy <= 0) return;
    for (int step = 0; step < dy; ++step) {
        if (KoopaGroundBlocked(k, k->x, k->y)) {
            k->vy = 0;
            k->verticalRemainder = 0;
            return;
        }
        ++k->y;
    }
}

//Actualizar la ia de los enemigos (caminado, tp hacia el tope, screen wrap horizontal, etc)
static void UpdateKoopa(Koopa *k, uint32_t now) {
    if (!k->active) return;
    if (k->state == KOOPA_NORMAL) {
        k->horizontalRemainder += k->isCrab && k->angry ? 1300U : 1000U;
        int dx = k->horizontalRemainder / 1000U;
        k->horizontalRemainder %= 1000U;
        uint8_t wasGrounded = KoopaGroundBlocked(k, k->x, k->y);
        k->x += k->direction * dx;
        MoveKoopaNormalY(k);
        uint8_t nowGrounded = KoopaGroundBlocked(k, k->x, k->y);
        if (k->isFreezie) {
            if (wasGrounded && !nowGrounded) k->fellFromPlatform = 1;
            else if (k->fellFromPlatform && nowGrounded) {
                k->direction = (int8_t)-k->direction;
                k->fellFromPlatform = 0;
            }
        }
        if (k->y >= 240) {
            if (k->x + KOOPA_W <= 0 || k->x >= SCREEN_WIDTH) {
                RespawnKoopaFromTopPipe(k, now);
                return;
            }
        } else {
            if (k->x + KOOPA_W <= 0) k->x = SCREEN_WIDTH - 1;
            else if (k->x >= SCREEN_WIDTH) k->x = 1 - KOOPA_W;
        }
        uint32_t steps = (uint32_t)(now - k->animationTick) / KOOPA_WALK_FRAME_MS;
        if (steps != 0U) {
            k->frame = (uint8_t)(WalkFirst(k) +
                ((k->frame - WalkFirst(k) + steps) % WalkCount(k)));
            k->animationTick += steps * KOOPA_WALK_FRAME_MS;
        }
        return;
    }

    if (k->state == KOOPA_BUMPED || k->state == KOOPA_DEFEATED) {
        k->vy += KOOPA_GRAVITY;
        if (k->vy > KOOPA_MAX_FALL) k->vy = KOOPA_MAX_FALL;
        int total = k->verticalRemainder + k->vy;
        int dy = total / PHYSICS_SCALE;
        k->verticalRemainder = (int16_t)(total - dy * PHYSICS_SCALE);
        k->y += (int16_t)dy;
        if (k->state == KOOPA_DEFEATED) {
            k->x += k->direction * 2;
            if (k->y >= SCREEN_HEIGHT || k->x + KOOPA_W < -64 || k->x > SCREEN_WIDTH + 64) {
                k->active = 0;
                return;
            }
        } else if (k->vy >= 0 && k->y >= k->bumpGroundY) {
            k->y = k->bumpGroundY;
            k->state = KOOPA_VULNERABLE;
            k->vy = 0;
            k->verticalRemainder = 0;
            k->animationTick = now;
            k->vulnerableTick = now;
        }
    }

    if (k->state == KOOPA_VULNERABLE &&
        (uint32_t)(now - k->vulnerableTick) >= KOOPA_VULNERABLE_DURATION_MS) {
        k->state = KOOPA_NORMAL;
        k->vy = 0;
        k->verticalRemainder = 0;
        k->angry = 0;
        k->horizontalRemainder = 0;
        k->frame = 0;
        k->animationTick = now;
        k->vulnerableTick = 0;
        return;
    }

    if (k->state == KOOPA_VULNERABLE || k->state == KOOPA_BUMPED || k->state == KOOPA_DEFEATED) {
        uint32_t steps = (uint32_t)(now - k->animationTick) / KOOPA_VULNERABLE_FRAME_MS;
        if (steps != 0U) {
            uint8_t animCount = (k->isFreezie && k->state == KOOPA_DEFEATED) ? FREEZIE_DEATH_FRAMES : 2U;
            k->frame = (uint8_t)(VulnerableFirst(k) +
                ((k->frame - VulnerableFirst(k) + steps) % animCount));
            k->animationTick += steps * KOOPA_VULNERABLE_FRAME_MS;
        }
    }
}

// Lógica de contancto con mario cuando son vulnerables
static uint8_t KoopaTouchesMario(const Koopa *k) {
    if (!k->active || k->state == KOOPA_DEFEATED) return 0;
    int marioLeft = player.x + MARIO_HITBOX_X_OFFSET;
    int marioTop = player.y + MARIO_HITBOX_Y_OFFSET;
    int marioRight = marioLeft + MARIO_HITBOX_W;
    int marioBottom = marioTop + MARIO_HITBOX_H;
    if (marioTop >= k->y + KOOPA_H || marioBottom <= k->y) return 0;
    return marioLeft < k->x + KOOPA_W && marioRight > k->x;
}

//Screen wrap horizontal
static int WrapX(int x) {
    x %= SCREEN_WIDTH;
    return x < 0 ? x + SCREEN_WIDTH : x;
}
static void WrapPlayerX(void) { player.x = (int16_t)WrapX(player.x); }

/* Composicion de regiones sucias: se envia el resultado final, nunca una
 * pantalla intermedia con los personajes borrados. Sin framebuffer completo.
 * Uso exclusivo desde el bucle principal; no llamar desde interrupciones. */
#define TILE 8
#define TILE_COLS ((SCREEN_WIDTH + TILE - 1) / TILE)
#define TILE_ROWS ((SCREEN_HEIGHT + TILE - 1) / TILE)
static uint8_t dirty[TILE_ROWS][TILE_COLS];
static uint8_t sendingDirty[TILE_ROWS][TILE_COLS];
static uint8_t dmaInFlight = 0;
static uint8_t batchActive = 0;
static volatile uint8_t dmaComplete = 0;
static uint32_t lastBatchTick = 0;
static uint32_t dmaStartTick = 0;
static int scanRow = 0, scanCol = 0;
#define RENDER_MAX_H 32
static uint8_t pixels[SCREEN_WIDTH * RENDER_MAX_H * 2];
extern SPI_HandleTypeDef hspi1;

static void MarkRect(int x, int y, int w, int h) {
    /* Cubrir tambien el desplazamiento de camara del lote en vuelo. */
    if (gameMode != GAME_TITLE && (shaking || renderShakeX || renderShakeY)) {
        x -= 2; y -= 2; w += 4; h += 4;
    }
    int right = x + w, bottom = y + h;
    if (right <= 0 || bottom <= 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (right > SCREEN_WIDTH) right = SCREEN_WIDTH;
    if (bottom > SCREEN_HEIGHT) bottom = SCREEN_HEIGHT;
    for (int row = y / TILE; row <= (bottom - 1) / TILE; ++row)
        for (int col = x / TILE; col <= (right - 1) / TILE; ++col)
            dirty[row][col] = 1;
}

/* Reflejar y dividir el rectangulo cuando cruza el borde de pantalla. */
static void MarkTitleDemo(void) {
    static const int offsets[6] = {0,40,80,120,148,176};
    for (int i = 0; i < 6; ++i) {
        int x = WrapX(titleDemo.x - offsets[i]);
        int h = i == 0 ? MARIO_H : 16;
        MarkRect(SCREEN_WIDTH - x - 16, STAGE_FLOOR_Y - h, 16, h);
        MarkRect(2 * SCREEN_WIDTH - x - 16, STAGE_FLOOR_Y - h, 16, h);
    }
}

//l+ogica de la title screen y sus animaciones con los personajes
static void UpdateTitleDemo(uint32_t now) {
    uint32_t steps = (uint32_t)(now - titleDemoTick) / 32U;
    if (!steps) return;
    MarkTitleDemo();
    titleDemoTick += steps * 32U;
    titleDemo.x = (int16_t)((titleDemo.x + steps % SCREEN_WIDTH) % SCREEN_WIDTH);
    titleDemoPhase = (uint16_t)((titleDemoPhase + (steps % 225U) * 32U) % 7200U);
    titleDemo.mario = MARIO_WALK_FIRST_FRAME + (titleDemoPhase / MARIO_WALK_FRAME_MS) % MARIO_WALK_FRAME_COUNT;
    titleDemo.koopa = (titleDemoPhase / KOOPA_WALK_FRAME_MS) % KOOPA_WALK_FRAMES;
    titleDemo.crab = (titleDemoPhase / KOOPA_WALK_FRAME_MS) % 3U;
    titleDemo.freezie = (titleDemoPhase / KOOPA_WALK_FRAME_MS) % FREEZIE_WALK_FRAMES;
    titleDemo.fire = (titleDemoPhase / FIRE_FRAME_MS) % FIRE_FRAME_COUNT;
    titleDemo.blueFire = titleDemo.fire;
    MarkTitleDemo();
}


//Prioridad del dibjuado en la title screen y orden de las animaciones. Se extendio la tuberías para ocultar completamente a mario
static uint16_t TitleFloorPixel(int visualX, int y) {
    uint16_t color = y >= STAGE_FLOOR_Y ? Stage_Pixel(visualX, y) : COLOR_BG;
    static const int offsets[6] = {0,40,80,120,148,176};
    for (int i = 0; i < 6; ++i) {
        int sx = WrapX(visualX - (titleDemoRender.x - offsets[i]));
        int h = i == 0 ? MARIO_H : 16;
        int sy = y - (STAGE_FLOOR_Y - h);
        if (sx >= 16 || sy < 0 || sy >= h) continue;
        uint16_t p;
        if (i == 0) p = marioFrames[titleDemoRender.mario][sy*16 + 15-sx];
        else if (i == 1) p = koopaFrames[titleDemoRender.koopa][sy*16 + 15-sx];
        else if (i == 2) p = crabFrames[titleDemoRender.crab][sy*16 + 15-sx];
        else if (i == 3) p = freezieFrames[titleDemoRender.freezie][sy*16 + 15-sx];
        else if (i == 4) p = fireFrames[titleDemoRender.fire][sy*16 + 15-sx];
        else p = blueFireFrames[titleDemoRender.blueFire][sy*16 + 15-sx];
        if (p != MARIO_TRANSPARENT_COLOR) color = p;
    }
    if (y >= STAGE_FLOOR_Y - 16 && Stage_IsPipe(visualX, y)) color = Stage_Pixel(visualX, y);
    if (y >= STAGE_FLOOR_Y - 32 && y < STAGE_FLOOR_Y - 16 &&
        Stage_IsPipe(visualX, y + 16)) {
        color = COLOR_BG;
    }
    return color;
}


static void MarkKoopaAt(int x, int y) { MarkRect(x, y, KOOPA_W, KOOPA_H); }

/* Para desplazamientos cortos (caidas/caminata) una sola region cubre la
 * posicion vieja y la nueva. En teleports/wrap se marcan por separado para
 * no invalidar una franja enorme de la pantalla. */
static void MarkKoopaMove(int oldX, int oldY, int newX, int newY) {
    int dx = newX - oldX;
    int dy = newY - oldY;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    if (dx <= KOOPA_W && dy <= KOOPA_H) {
        int left = oldX < newX ? oldX : newX;
        int top = oldY < newY ? oldY : newY;
        int rightOld = oldX + KOOPA_W;
        int rightNew = newX + KOOPA_W;
        int bottomOld = oldY + KOOPA_H;
        int bottomNew = newY + KOOPA_H;
        int right = rightOld > rightNew ? rightOld : rightNew;
        int bottom = bottomOld > bottomNew ? bottomOld : bottomNew;
        MarkRect(left, top, right - left, bottom - top);
    } else {
        MarkKoopaAt(oldX, oldY);
        MarkKoopaAt(newX, newY);
    }
}

/* HAL habilita SPI y espera el final de cada bloque antes de cambiar DC/CS. */
static uint8_t SendBytes(uint8_t *data, uint16_t length) {
    if (HAL_SPI_Transmit(&hspi1, data, length, 100U) == HAL_OK) return 1;
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
    Error_Handler();
    return 0;
}

static uint8_t SendCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
    if (!SendBytes(&cmd, 1)) return 0;
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
    return 1;
}

static void BlitRegion(int x, int y, int w, int h) {
    uint8_t column[4] = {(uint8_t)(x >> 8), (uint8_t)x,
        (uint8_t)((x+w-1) >> 8), (uint8_t)(x+w-1)};
    uint8_t page[4] = {(uint8_t)(y >> 8), (uint8_t)y,
        (uint8_t)((y+h-1) >> 8), (uint8_t)(y+h-1)};
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    if (!SendCommand(0x2A) || !SendBytes(column, 4) ||
        !SendCommand(0x2B) || !SendBytes(page, 4) ||
        !SendCommand(0x2C)) return;
    dmaComplete = 0;
    dmaInFlight = 1;
    dmaStartTick = HAL_GetTick();
    if (HAL_SPI_Transmit_DMA(&hspi1, pixels, (uint16_t)(w*h*2)) != HAL_OK) {
        dmaInFlight = 0;
        HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
        Error_Handler();
    }
}

static uint16_t ScenePixel(int x, int y) {
    /* El montaje actual refleja X: compensar solo TITLE/HUD, no gameplay. */
    int visualX = SCREEN_WIDTH - 1 - x;
    if (gameMode == GAME_TITLE) {
        int tx = visualX - (SCREEN_WIDTH - TITLE_W) / 2;
        int ty = y - (SCREEN_HEIGHT - TITLE_H) / 2;
        if (tx < 0 || tx >= TITLE_W || ty < 0 || ty >= TITLE_H)
            return TitleFloorPixel(visualX, y);
        return titleFrames[titleRenderFrame][ty * TITLE_W + tx];
    }
    if (gameMode == GAME_GAMEOVER)
        return TitleFloorPixel(visualX, y);
    int screenY = y;
    if (screenY >= 32) {
        x += renderShakeX;
        y += renderShakeY;
        if (x < 0 || x >= SCREEN_WIDTH || y < 32 || y >= SCREEN_HEIGHT) return COLOR_BG;
    }
    uint16_t color = Stage_Pixel(x, y);
    if (powAvailable && x >= POW_X && x < POW_X+16 && y >= POW_Y && y < POW_Y+16) {
        /* Compensar el espejo del montaje solo en el texto del POW. */
        int px = 15-(x-POW_X), py = y-POW_Y;
        unsigned tile = 10U + (unsigned)(px/8) + (unsigned)(py/8)*12U;
        color = stageTiles[tile][(py%8)*8 + px%8];
    }
    /* Enemigos primero: despues se repinta cualquier pixel de tuberia para
     * que puedan entrar/salir visualmente por detras de ella. */
    for (int ki = 0; ki < MAX_KOOPAS; ++ki) {
        Koopa *k = &koopas[ki];
        if (!k->active) continue;
        int ksx = x - k->x;
        if (ksx >= 0 && ksx < KOOPA_W && y >= k->y && y < k->y + KOOPA_H) {
            if (k->direction > 0) ksx = KOOPA_W - 1 - ksx;
            uint16_t p = k->isFreezie ? freezieFrames[k->frame][(y-k->y)*KOOPA_W+ksx]
                         : k->isCrab ? crabFrames[k->frame][(y-k->y)*KOOPA_W+ksx]
                                     : koopaFrames[k->frame][(y-k->y)*KOOPA_W+ksx];
            if (p != KOOPA_TRANSPARENT_COLOR) color = p;
        }
    }
    if (fireball.active && x >= fireball.x && x < fireball.x+FIRE_W &&
        y >= fireball.y && y < fireball.y+FIRE_H) {
        int sx = x-fireball.x;
        if (fireball.direction < 0) sx = FIRE_W-1-sx;
        uint16_t p = (levelNumber >= 3U ? blueFireFrames : fireFrames)[fireball.frame][(y-fireball.y)*FIRE_W+sx];
        if (p != ENEMY_TRANSPARENT_COLOR) color = p;
    }
    if (Stage_IsPipe(x, y)) color = Stage_Pixel(x, y);

    int marioX = WrapX(x - player.x);
    if (marioX < PLAYER_W && y >= player.y && y < player.y+PLAYER_H) {
        int sx = facingLeft ? marioX : PLAYER_W-1-marioX;
        uint16_t p = marioFrames[marioFrame][(y-player.y)*PLAYER_W+sx];
        if (p != MARIO_TRANSPARENT_COLOR) color = p;
    }
    /* Mario tambien pasa por detras de las tuberias: repintarlas al final
     * evita que la gorra sobresalga durante la entrada/salida. */
    if (Stage_IsPipe(x, y)) color = Stage_Pixel(x, y);

    /* HUD de vidas: al perder una se elimina la mas a la izquierda. */
    if (screenY < 32) color = COLOR_BG; /* Franja reservada al HUD. */
    if (screenY < LIFE_H) {
        /* Una ventana de hasta 24 iconos; contador interno hasta 255 vidas. */
        int slots = lifeSlots > 24U ? 24 : lifeSlots;
        int shown = lives > slots ? slots : lives;
        int firstX = (slots - shown) * (LIFE_W + 2);
        for (int li = 0; li < shown; ++li) {
            int lx = firstX + li * (LIFE_W + 2);
            if (visualX >= lx && visualX < lx + LIFE_W) {
                uint16_t p = lifeSprite[y * LIFE_W + (visualX - lx)];
                if (p != LIFE_TRANSPARENT_COLOR) color = p;
            }
        }
    }
    return color;
}

// El callback NO cambia CS: la transferencia DMA puede terminar antes de que el ultimo bit haya salido del SPI. Se finaliza en el bucle principal.
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) dmaComplete = 1;
}
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) Error_Handler();
}

//Renderizado rapido sucio para no lagear el juego
static void RenderDirty(void) {
    uint32_t now = HAL_GetTick();
    if (dmaInFlight) {
        if (!dmaComplete) {
            if ((uint32_t)(now - dmaStartTick) > 250U) Error_Handler();
            return;
        }
        /* Esperar a que el ultimo bit se haya transmitido antes de soltar CS. */
        if (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_BSY)) return;
        HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
        dmaInFlight = 0;
        dmaComplete = 0;
    }
    if (!batchActive) {
        if ((uint32_t)(now - lastBatchTick) < 32U) return;
        int any = 0;
        for (int r = 0; r < TILE_ROWS; ++r)
            for (int c = 0; c < TILE_COLS; ++c) {
                sendingDirty[r][c] = dirty[r][c];
                any |= dirty[r][c];
                dirty[r][c] = 0;
            }
        if (!any) return;
        titleRenderFrame = titleFrame;
        titleDemoRender = titleDemo;
        renderShakeX = shakeX;
        renderShakeY = shakeY;
        batchActive = 1;
        scanRow = scanCol = 0;
        lastBatchTick = now;
    }
    /* Enviar como maximo UNA region por llamada; el DMA transmite mientras
     * Game_Update sigue calculando fisica y marcando el siguiente frame. */
    for (int row = scanRow; row < TILE_ROWS; ++row) {
        int col = (row == scanRow) ? scanCol : 0;
        while (col < TILE_COLS) {
            if (!sendingDirty[row][col]) { ++col; continue; }
            int first = col;
            while (col < TILE_COLS && sendingDirty[row][col]) ++col;
            int last = col;
            int lastRow = row + 1;
            while (lastRow < TILE_ROWS && (lastRow-row)*TILE < RENDER_MAX_H) {
                int allDirty = 1;
                for (int c = first; c < last; ++c)
                    if (!sendingDirty[lastRow][c]) { allDirty = 0; break; }
                if (!allDirty) break;
                ++lastRow;
            }
            for (int r = row; r < lastRow; ++r)
                for (int c = first; c < last; ++c) sendingDirty[r][c] = 0;
            scanRow = row;
            scanCol = col;
            int x = first*TILE, y = row*TILE;
            int w = last*TILE-x, h = (lastRow-row)*TILE;
            if (x+w > SCREEN_WIDTH) w = SCREEN_WIDTH-x;
            if (y+h > SCREEN_HEIGHT) h = SCREEN_HEIGHT-y;
            int i = 0;
            for (int py = y; py < y+h; ++py)
                for (int px = x; px < x+w; ++px) {
                    uint16_t p = ScenePixel(px,py);
                    pixels[i++] = (uint8_t)(p >> 8);
                    pixels[i++] = (uint8_t)p;
                }
            if (gameMode != GAME_TITLE && y < 32) hudDirty = 1;
            BlitRegion(x,y,w,h);
            return;
        }
        scanCol = 0;
    }
    batchActive = 0;
}

//Logica de derrota de enemigos y combo de puntuacion. Si se derrotan en menos de un segundo varios enemigos seguidos se acumulan mas puntos
static void HitEnemy(Koopa *k, uint32_t now, uint8_t forceVulnerable) {
    if (k->isFreezie) {
        k->state = KOOPA_DEFEATED;
        k->vy = KOOPA_DEFEAT_VELOCITY;
        k->verticalRemainder = 0;
        k->frame = FREEZIE_DEATH_FIRST_FRAME;
        k->animationTick = now;
        ++levelDefeated;
        if (comboCount != 0U && (uint32_t)(now-lastDefeatTick) <= COMBO_WINDOW_MS) ++comboCount;
        else comboCount = 1U;
        score += 100U * comboCount; lastDefeatTick = now; hudDirty = 1;
        if (levelDefeated >= LEVEL_TOTAL_KOOPAS) { levelClearPending=1; levelClearKoopaIndex=(int8_t)(k-koopas); }
    } else if (k->isCrab && !k->angry && !forceVulnerable) {
        k->angry = 1;
        k->frame = 3;
        k->animationTick = now;
        k->horizontalRemainder = 0;
    } else {
        k->state = KOOPA_BUMPED;
        k->bumpGroundY = k->y;
        k->vy = KOOPA_BUMP_VELOCITY;
        k->verticalRemainder = 0;
        k->frame = VulnerableFirst(k);
        k->animationTick = now;
        k->vulnerableTick = 0;
    }
    MarkKoopaAt(k->x, k->y);
}

//Lógica del bloque pow y screenshake al usarlo
static void TriggerPow(uint32_t now) {
    if (!powAvailable) return;
    /* Evaluar apoyo antes de quitar el bloque: incluye enemigos sobre POW. */
    for (int i = 0; i < MAX_KOOPAS; ++i) {
        Koopa *k = &koopas[i];
        if (k->active && k->state == KOOPA_NORMAL && k->vy >= 0 &&
            k->x < SCREEN_WIDTH && k->x+KOOPA_W > 0 &&
            k->y < SCREEN_HEIGHT && k->y+KOOPA_H > 0 &&
            KoopaGroundBlocked(k, k->x, k->y)) HitEnemy(k, now, 1);
    }
    powAvailable = 0;
    shaking = 1;
    shakeTick = now;
    shakeX = 2; shakeY = -2;
    MarkRect(0, 32, SCREEN_WIDTH, SCREEN_HEIGHT-32);
}
static void UpdateShake(uint32_t now) {
    if (!shaking) return;
    uint32_t elapsed = (uint32_t)(now-shakeTick);
    int8_t x=0, y=0;
    if (elapsed < SHAKE_MS) {
        x = (elapsed/SHAKE_STEP_MS)&1U ? -2 : 2;
        y = (elapsed/SHAKE_STEP_MS)&1U ? 2 : -2;
    } else shaking = 0;
    if (x != shakeX || y != shakeY) {
        shakeX=x; shakeY=y;
        MarkRect(0,32,SCREEN_WIDTH,SCREEN_HEIGHT-32);
    }
}

//Colision con plataformas de mario
static int16_t MarioPlatformSurfaceY(void) {
    int cx = WrapX(player.x + PLAYER_W / 2);
    int start = player.y + PLAYER_H;
    if (start < 32) start = 32;
    for (int y = start; y < STAGE_FLOOR_Y; ++y)
        if (WorldSolid(cx, y)) return (int16_t)y;
    return STAGE_FLOOR_Y;
}
/* Recorrer el borde completo detecta tambien tiles entre las esquinas. */
static uint8_t HorizontalBlocked(int x, int y, int direction) {
    int hitLeft = x + MARIO_HITBOX_X_OFFSET;
    int hitTop = y + MARIO_HITBOX_Y_OFFSET;
    int edge = direction > 0 ? hitLeft + MARIO_HITBOX_W - 1 : hitLeft;
    for (int py = hitTop; py < hitTop + MARIO_HITBOX_H; ++py)
        if (WorldSolid(WrapX(edge), py)) return 1;
    return 0;
}

static uint8_t VerticalBlocked(int x, int y, int direction) {
    int hitLeft = x + MARIO_HITBOX_X_OFFSET;
    int hitTop = y + MARIO_HITBOX_Y_OFFSET;
    int edge = direction > 0 ? hitTop + MARIO_HITBOX_H - 1 : hitTop;
    if (edge < 0 || edge >= STAGE_FLOOR_Y) return 1;
    for (int px = hitLeft; px < hitLeft + MARIO_HITBOX_W; ++px)
        if (WorldSolid(WrapX(px), edge)) return 1;
    return 0;
}
static void MovePlayerX(int dx) {
    int direction = dx > 0 ? 1 : -1;
    for (int step = 0; step < (dx > 0 ? dx : -dx); ++step) {
        if (HorizontalBlocked(player.x + direction, player.y, direction)) break;
        player.x += direction;
    }
    WrapPlayerX();
}

//Lógica de bolas de juego, rutina de spawneo y seleccion de piso.
static void UpdateFireball(uint32_t now) {
    int oldX=fireball.x, oldY=fireball.y;
    uint8_t oldFrame=fireball.frame, wasActive=fireball.active;
    if (fireball.active) {
        fireball.remainder += 224U; /* 0.875 px/tick: 1.75x la velocidad anterior. */
        fireball.x += fireball.direction*(fireball.remainder/256U);
        fireball.remainder %= 256U;
        fireball.frame = (uint8_t)(((uint32_t)(now-fireball.animationTick)/FIRE_FRAME_MS)%FIRE_FRAME_COUNT);
        if (fireball.x >= SCREEN_WIDTH || fireball.x+FIRE_W <= 0) fireball.active=0;
    }
    if (levelNumber >= 2 && !levelClearPending && !fireball.active &&
        (uint32_t)(now-lastFireTick) >= FIRE_SPAWN_MS) {
        static const int16_t surfaces[3]={304,240,160};
        fireball.direction=(EnemyRandom()&1U)?1:-1;
        fireball.x=fireball.direction>0?1-FIRE_W:SCREEN_WIDTH-1;
        /* Nivel 3: la bola azul aparece siempre en la plataforma de Mario. */
        int16_t surface = levelNumber >= 3U ? MarioPlatformSurfaceY() : surfaces[EnemyRandom()%3U];
        fireball.y=surface-20-FIRE_H;
        fireball.frame=0;fireball.remainder=0;fireball.animationTick=now;
        fireball.active=1;lastFireTick=now;
    }
    if (wasActive && (!fireball.active || oldX!=fireball.x || oldFrame!=fireball.frame))
        MarkRect(oldX,oldY,FIRE_W,FIRE_H);
    if (fireball.active && (!wasActive || oldX!=fireball.x || oldFrame!=fireball.frame))
        MarkRect(fireball.x,fireball.y,FIRE_W,FIRE_H);
}
static uint8_t FireTouchesMario(void) {
    int left=player.x+MARIO_HITBOX_X_OFFSET, top=player.y+MARIO_HITBOX_Y_OFFSET;
    return fireball.active && left < fireball.x+FIRE_W && left+MARIO_HITBOX_W > fireball.x &&
        top < fireball.y+FIRE_H && top+MARIO_HITBOX_H > fireball.y;
}
//Borrar enemigos fuera de pantalla al ser derrotados
static uint8_t DefeatedEnemiesGone(void) {
    for (int i=0;i<MAX_KOOPAS;++i) if (koopas[i].active) return 0;
    return 1;
}

//logica de golpear a los enemigos El golpe solo afecta 32 px horizontales, centrados donde Mario pego.
static void BumpKoopaOnPlatform(int hitX, int hitY) {
    int left, right, top;
    if (!Stage_GetPlatformSpan(WrapX(hitX), hitY, &left, &right, &top)) return;

    /* */
    int hitCenter = WrapX(hitX);
    uint32_t now = HAL_GetTick();
    for (int i = 0; i < MAX_KOOPAS; ++i) {
        Koopa *k = &koopas[i];
        if (!k->active || k->state != KOOPA_NORMAL) continue;
        int koopaBottom = k->y + KOOPA_H;
        if (koopaBottom < top - 1 || koopaBottom > top + 1) continue;
        if (k->x + KOOPA_W <= left || k->x >= right) continue;

        int koopaCenter = WrapX(k->x + KOOPA_W / 2);
        int dx = koopaCenter - hitCenter;
        if (dx > SCREEN_WIDTH / 2) dx -= SCREEN_WIDTH;
        if (dx < -SCREEN_WIDTH / 2) dx += SCREEN_WIDTH;
        if (dx < -16 || dx >= 16) continue;

        if (KoopaGroundBlocked(k, k->x, k->y) && k->vy >= 0) HitEnemy(k, now, 0);
    }
}

//salto y gravedad de mario
static void MovePlayerY(void) {
    int total = player.verticalRemainder + player.vy;
    int dy = total / PHYSICS_SCALE;
    player.verticalRemainder = (int16_t)(total - dy * PHYSICS_SCALE);
    int direction = dy > 0 ? 1 : -1;
    int distance = dy > 0 ? dy : -dy;
    player.onGround = 0;
    for (int step = 0; step < distance; ++step) {
        if (VerticalBlocked(player.x, player.y + direction, direction)) {
            if (direction < 0) {
                int hitX = player.x + MARIO_HITBOX_X_OFFSET + MARIO_HITBOX_W / 2;
                int hitY = player.y + direction + MARIO_HITBOX_Y_OFFSET;
                int left=player.x+MARIO_HITBOX_X_OFFSET;
                uint8_t hitPow=powAvailable && hitY >= POW_Y && hitY < POW_Y+POW_SIZE &&
                    left < POW_X+POW_SIZE && left+MARIO_HITBOX_W > POW_X;
                if (hitPow) TriggerPow(HAL_GetTick());
                else BumpKoopaOnPlatform(hitX, hitY);
            }
            player.vy = 0;
            player.verticalRemainder = 0;
            if (direction > 0) player.onGround = 1;
            return;
        }
        player.y += direction;
    }
    if (player.vy >= 0)
        player.onGround = VerticalBlocked(player.x, player.y + 1, 1);
}

//Reseteo de nivel al iniciar el juego o morir. resetea todas las banderas
static void ResetLevelState(void) {
    uint32_t now = HAL_GetTick();
    deathState = DEATH_NONE;
    g_jumpRequest = 0;
    g_moveLeft = 0;
    g_moveRight = 0;
    facingLeft = 0;
    marioFrame = MARIO_IDLE_FRAME;
    marioAnimationTick = now;
    enemyRandom ^= now + 0x9E3779B9U;
    for (int i = 0; i < MAX_KOOPAS; ++i) koopas[i].active = 0;
    levelSpawned = 0;
    levelDefeated = 0;
    spawnedKoopas = spawnedCrabs = spawnedFreezies = 0;
    iceVX = iceRemainder = 0;
    fireball.active = 0;
    lastFireTick = now;
    shaking=0;shakeX=shakeY=0;
    Stage_SetLevel(levelNumber);
    lastSpawnTick = now;
    levelClearFreeze = 0;
    levelClearPending = 0;
    levelClearKoopaIndex = -1;
    levelClearTick = 0;
    player.x = (SCREEN_WIDTH - PLAYER_W) / 2;
    player.y = FLOOR_Y;
    player.prevX = player.x;
    player.prevY = player.y;
    player.vy = 0;
    player.verticalRemainder = 0;
    player.onGround = 1;
    levelStartTick = now;
    timeRemaining = LEVEL_TIME_SECONDS;
    comboCount = 0;
    lastDefeatTick = 0;
    hudDirty = 1;
    MarkRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

//Inicialicación de nivel y sfx
static void StartLevel(void) {
    ResetLevelState();
    gameMode = GAME_LEVEL_INTRO;
    Sonidos_PlayLevelStart();
}

//Pantalla de título.
static void EnterTitle(void) {
    gameMode = GAME_TITLE;
    lives = 3;
    lifeSlots = 3;
    levelNumber = 1;
    Stage_SetLevel(1);
    powAvailable = 1;
    fireball.active = 0;
    shaking=0;shakeX=shakeY=0;
    score = 0;
    comboCount = 0;
    g_moveLeft = g_moveRight = g_jumpRequest = 0;
    titleFrame = 0;
    titleTick = HAL_GetTick();
    titleRenderFrame = 0;
    titleDemo = (TitleDemo){96, MARIO_WALK_FIRST_FRAME, 0, 0, 0, 0, 0};
    titleDemoRender = titleDemo;
    titleDemoTick = titleTick;
    titleDemoPhase = 0;
    Sonidos_PlayTitle();
    memset(dirty, 0, sizeof(dirty));
    memset(sendingDirty, 0, sizeof(sendingDirty));
    batchActive = 0;
    MarkRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

//Pantalla de gameover
static void EnterGameOver(void) {
    gameMode = GAME_GAMEOVER;
    finalScore = score;
    uint8_t rank = HighScores_Insert(highScores, finalScore);
    gameOverNewTop1 = (uint8_t)(rank == 1U);
    highScoresNeedSave = (uint8_t)(rank != 0U);
    gameOverTextDirty = 1;
    g_moveLeft = g_moveRight = g_jumpRequest = 0;
    shaking = 0;
    shakeX = shakeY = 0;
    Stage_SetLevel(1);
    titleDemo = (TitleDemo){96, MARIO_WALK_FIRST_FRAME, 0, 0, 0, 0, 0};
    titleDemoRender = titleDemo;
    titleDemoTick = HAL_GetTick();
    titleDemoPhase = 0;
    memset(dirty, 0, sizeof(dirty));
    memset(sendingDirty, 0, sizeof(sendingDirty));
    batchActive = 0;
    MarkRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

static void LoseLifeAndRestart(void) {
    if (lives > 0) --lives;
    if (lives == 0) EnterGameOver();
    else StartLevel();
}

static void UpdateTimer(uint32_t now) {
    uint32_t elapsed = (uint32_t)(now - levelStartTick) / 1000U;
    uint16_t newTime = elapsed >= LEVEL_TIME_SECONDS ? 0U : (uint16_t)(LEVEL_TIME_SECONDS - elapsed);
    if (newTime != timeRemaining) {
        timeRemaining = newTime;
        hudDirty = 1;
    }
}

//Inicialización
void Game_Init(void) {
    Sonidos_Init();
    HighScores_Init();
    HighScores_Load(highScores);
    EnterTitle();
}

void Game_SerialInit(void) {
    g_moveLeft = 0;
    g_moveRight = 0;
    g_jumpRequest = 0;
    serialJumpHeld = 0;
    (void)HAL_UART_Receive_IT(&huart3, serialRx, sizeof(serialRx));
}


//Lógica de controles por medio de usart3 ESP32
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart != &huart3) return;

    uint8_t button = serialRx[0];
    uint8_t pressed = serialRx[1];
    if (pressed == '0' || pressed == '1') {
        uint8_t down = (uint8_t)(pressed == '1');
        if (button == 'I') {
            g_moveRight = down;
        } else if (button == 'D') {
            g_moveLeft = down;
        } else if (button == 'A') {
            if (down && !serialJumpHeld) g_jumpRequest = 1;
            serialJumpHeld = down;
        }
    }
    (void)HAL_UART_Receive_IT(&huart3, serialRx, sizeof(serialRx));
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart == &huart3)
        (void)HAL_UART_Receive_IT(&huart3, serialRx, sizeof(serialRx));
}

//Lógica principal de actualización. Engloba todo el juego
void Game_Update(void) {
    Sonidos_Update();
    uint32_t modeNow = HAL_GetTick();
    UpdateShake(modeNow);
    if (gameMode == GAME_GAMEOVER) {
        UpdateTitleDemo(modeNow);
        if (g_moveLeft || g_moveRight || g_jumpRequest) {
            g_moveLeft = g_moveRight = g_jumpRequest = 0;
            EnterTitle();
        }
        return;
    }
    if (gameMode == GAME_TITLE) {
        UpdateTitleDemo(modeNow);
        uint32_t steps = (uint32_t)(modeNow - titleTick) / TITLE_FRAME_MS;
        if (steps != 0U) {
            titleFrame = (uint8_t)((titleFrame + steps) % TITLE_FRAME_COUNT);
            titleTick += steps * TITLE_FRAME_MS;
            /* Ventana en coordenadas del controlador, reflejada en X. */
            MarkRect(SCREEN_WIDTH - (SCREEN_WIDTH - TITLE_W)/2 - TITLE_W,
                     (SCREEN_HEIGHT - TITLE_H)/2, TITLE_W, TITLE_H);
        }
        if (g_moveLeft || g_moveRight || g_jumpRequest) {
            g_moveLeft = g_moveRight = g_jumpRequest = 0;
            StartLevel();
        }
        return;
    }
    if (gameMode == GAME_LEVEL_INTRO) {
        /* Nivel completamente congelado mientras suena el jingle de inicio. */
        if (!Sonidos_IsPlaying()) {
            gameMode = GAME_PLAYING;
            levelStartTick = modeNow;
            hudDirty = 1;
        }
        return;
    }
    if (gameMode == GAME_TIME_BONUS) {
        if (timeRemaining > 0 && (uint32_t)(modeNow - bonusTick) >= BONUS_STEP_MS) {
            uint32_t steps = (uint32_t)(modeNow - bonusTick) / BONUS_STEP_MS;
            if (steps > timeRemaining) steps = timeRemaining;
            timeRemaining = (uint16_t)(timeRemaining - steps);
            score += steps * TIME_BONUS_POINTS;
            bonusTick += steps * BONUS_STEP_MS;
            hudDirty = 1;
        }
        if (timeRemaining == 0) {
            gameMode = GAME_LEVEL_FREEZE;
            levelFreezeTick = modeNow;
        }
        return;
    }
    if (gameMode == GAME_LEVEL_FREEZE) {
        if ((uint32_t)(modeNow - levelFreezeTick) >= LEVEL_CLEAR_FREEZE_MS) {
            if (levelNumber >= 3U) {
                EnterTitle();
            } else {
                if (lives < 255U) ++lives;
                if (lives > lifeSlots) lifeSlots = lives;
                ++levelNumber;
                powAvailable = 1;
                StartLevel();
            }
        }
        return;
    }
    UpdateTimer(modeNow);
    if (timeRemaining == 0 && deathState == DEATH_NONE) {
        deathState = DEATH_FREEZE;
        deathTick = modeNow;
        Sonidos_PlayDeath();
        marioFrame = MARIO_DEATH_FIRST_FRAME;
    }
    if (levelClearFreeze) {
        if ((uint32_t)(HAL_GetTick() - levelClearTick) >= LEVEL_CLEAR_FREEZE_MS) Game_Init();
        return;
    }
    if (deathState != DEATH_NONE) {
        uint32_t now = HAL_GetTick();
        g_jumpRequest = 0; /* No acumular saltos mientras Mario muere. */

        if (deathState == DEATH_FREEZE) {
            /* Primeros 0.5 s: Mario queda completamente quieto. */
            if ((uint32_t)(now - deathTick) < DEATH_FREEZE_MS) return;

            deathState = DEATH_FALL;
            deathTick = now;
            player.vy = DEATH_JUMP_VELOCITY;
            player.verticalRemainder = 0;
            player.onGround = 0;
            marioFrame = MARIO_DEATH_FIRST_FRAME;
            marioAnimationTick = now;
        }

        /* El timestamp de la pausa nace al terminar realmente DeathMus. */
        if (deathState == DEATH_FALL &&
            !(Sonidos_IsPlaying() && Sonidos_Current() == SONIDO_MUERTE)) {
            deathState = DEATH_POST_MUSIC_PAUSE;
            deathTick = now;
        }

        // Pequeño salto de muerte y caída atravesando todo el escenario.
        player.prevX = player.x;
        player.prevY = player.y;
        uint8_t prevMarioFrame = marioFrame;

        player.vy += GRAVITY;
        if (player.vy > MAX_FALL_SPEED) player.vy = MAX_FALL_SPEED;
        int total = player.verticalRemainder + player.vy;
        int dy = total / PHYSICS_SCALE;
        player.verticalRemainder = (int16_t)(total - dy * PHYSICS_SCALE);
        player.y += (int16_t)dy;

        marioFrame = (uint8_t)(MARIO_DEATH_FIRST_FRAME +
            ((now - marioAnimationTick) / MARIO_DEATH_FRAME_MS) % MARIO_DEATH_FRAME_COUNT);

        MarkRect(player.prevX, player.prevY, PLAYER_W, PLAYER_H);
        MarkRect(player.prevX - SCREEN_WIDTH, player.prevY, PLAYER_W, PLAYER_H);
        MarkRect(player.x, player.y, PLAYER_W, PLAYER_H);
        MarkRect(player.x - SCREEN_WIDTH, player.y, PLAYER_W, PLAYER_H);
        if (marioFrame != prevMarioFrame) {
            MarkRect(player.x, player.y, PLAYER_W, PLAYER_H);
            MarkRect(player.x - SCREEN_WIDTH, player.y, PLAYER_W, PLAYER_H);
        }
        // Tras DeathMus hay una pausa completa no bloqueante de 1000 ms.
        if (deathState == DEATH_POST_MUSIC_PAUSE &&
            (uint32_t)(now - deathTick) >= DEATH_POST_MUSIC_PAUSE_MS) {
            LoseLifeAndRestart();
        }
        return;
    }
    uint32_t now = HAL_GetTick();
    UpdateFireball(now);
    int oldKoopaX[MAX_KOOPAS], oldKoopaY[MAX_KOOPAS];
    uint8_t oldKoopaActive[MAX_KOOPAS], oldKoopaFrame[MAX_KOOPAS];
    int8_t oldKoopaDirection[MAX_KOOPAS];
    for (int i = 0; i < MAX_KOOPAS; ++i) {
        oldKoopaX[i] = koopas[i].x;
        oldKoopaY[i] = koopas[i].y;
        oldKoopaActive[i] = koopas[i].active;
        oldKoopaFrame[i] = koopas[i].frame;
        oldKoopaDirection[i] = koopas[i].direction;
        UpdateKoopa(&koopas[i], now);
    }
    if (levelSpawned < LEVEL_TOTAL_KOOPAS &&
        (uint32_t)(now - lastSpawnTick) >= SpawnInterval() && FindFreeKoopa()) {
        SpawnLevelKoopa(now);
    }
	player.prevX = player.x;
	player.prevY = player.y;

	//Movimiento horizontal
	uint8_t prevFacing = facingLeft;
    uint8_t prevMarioFrame = marioFrame;
	if (g_moveLeft && !g_moveRight) {
		facingLeft = 1;
	} else if (g_moveRight && !g_moveLeft) {
		facingLeft = 0;
	}
    if (levelNumber >= 3U && player.onGround) {
        const int16_t ICE_ACCEL = 48, ICE_FRICTION = 20, ICE_MAX = MOVE_SPEED * PHYSICS_SCALE;
        int input = (g_moveRight ? 1 : 0) - (g_moveLeft ? 1 : 0);
        if (input) {
            iceVX += (int16_t)(input * ICE_ACCEL);
            if (iceVX > ICE_MAX) iceVX = ICE_MAX;
            if (iceVX < -ICE_MAX) iceVX = -ICE_MAX;
        } else if (iceVX > 0) { iceVX = iceVX > ICE_FRICTION ? iceVX-ICE_FRICTION : 0; }
        else if (iceVX < 0) { iceVX = iceVX < -ICE_FRICTION ? iceVX+ICE_FRICTION : 0; }
        int totalX = iceRemainder + iceVX;
        int iceDx = totalX / PHYSICS_SCALE;
        iceRemainder = (int16_t)(totalX - iceDx * PHYSICS_SCALE);
        if (iceDx) MovePlayerX(iceDx);
    } else {
        iceVX = iceRemainder = 0;
        MovePlayerX((g_moveRight ? MOVE_SPEED : 0) - (g_moveLeft ? MOVE_SPEED : 0));
    }
    // Al abandonar un borde se pierde apoyo antes de aceptar otro salto
    player.onGround = (player.vy >= 0) && VerticalBlocked(player.x, player.y + 1, 1);

	//Salto solo si está en el piso
	if (g_jumpRequest) {
		if (player.onGround) {
			player.vy = JUMP_VELOCITY;
            player.verticalRemainder = 0;
			player.onGround = 0;
		}
		g_jumpRequest = 0;
	}

	//Gravedad
	player.vy += GRAVITY;
	if (player.vy > MAX_FALL_SPEED) {
		player.vy = MAX_FALL_SPEED;
	}
    MovePlayerY();

    //Animacion de Mario
    if (!player.onGround) {
        marioFrame = MARIO_JUMP_FRAME;
    } else if ((g_moveLeft != 0U) != (g_moveRight != 0U)) {
        uint32_t now = HAL_GetTick();
        uint32_t steps = (uint32_t)(now - marioAnimationTick) / MARIO_WALK_FRAME_MS;
        if (marioFrame < MARIO_WALK_FIRST_FRAME ||
            marioFrame >= MARIO_WALK_FIRST_FRAME + MARIO_WALK_FRAME_COUNT) {
            marioFrame = MARIO_WALK_FIRST_FRAME;
            marioAnimationTick = now;
        } else if (steps != 0U) {
            marioFrame = (uint8_t)(MARIO_WALK_FIRST_FRAME +
                ((marioFrame - MARIO_WALK_FIRST_FRAME + steps) % MARIO_WALK_FRAME_COUNT));
            marioAnimationTick += steps * MARIO_WALK_FRAME_MS;
        }
    } else {
        marioFrame = MARIO_IDLE_FRAME;
        marioAnimationTick = HAL_GetTick();
    }

    //Solo invalidar enemigos cuyo resultado visual realmente cambio. Antes se redibujaban los cuatro slots activos en TODOS los frames,
    //incluso estando quietos; eso consumia SPI innecesariamente.
    for (int i = 0; i < MAX_KOOPAS; ++i) {
        Koopa *k = &koopas[i];
        if (!oldKoopaActive[i] && !k->active) continue;
        if (oldKoopaActive[i] && !k->active) {
            MarkKoopaAt(oldKoopaX[i], oldKoopaY[i]);
            continue;
        }
        if (!oldKoopaActive[i] && k->active) {
            MarkKoopaAt(k->x, k->y);
            continue;
        }
        if (oldKoopaX[i] != k->x || oldKoopaY[i] != k->y) {
            MarkKoopaMove(oldKoopaX[i], oldKoopaY[i], k->x, k->y);
        } else if (oldKoopaFrame[i] != k->frame || oldKoopaDirection[i] != k->direction) {
            MarkKoopaAt(k->x, k->y);
        }
    }
    uint8_t moved = (player.x != player.prevX) || (player.y != player.prevY);
    if (moved || facingLeft != prevFacing || marioFrame != prevMarioFrame) {
        MarkRect(player.prevX, player.prevY, PLAYER_W, PLAYER_H);
        MarkRect(player.prevX - SCREEN_WIDTH, player.prevY, PLAYER_W, PLAYER_H);
        MarkRect(player.x, player.y, PLAYER_W, PLAYER_H);
        MarkRect(player.x - SCREEN_WIDTH, player.y, PLAYER_W, PLAYER_H);
    }

    for (int i = 0; i < MAX_KOOPAS; ++i) {
        Koopa *k = &koopas[i];
        if (!KoopaTouchesMario(k)) continue;
        if (k->state == KOOPA_VULNERABLE || k->state == KOOPA_BUMPED) {
            k->state = KOOPA_DEFEATED;
            k->direction = facingLeft ? -1 : 1;
            k->vy = KOOPA_DEFEAT_VELOCITY;
            k->verticalRemainder = 0;
            k->frame = VulnerableFirst(k);
            k->animationTick = HAL_GetTick();
            ++levelDefeated;
            uint32_t defeatNow = HAL_GetTick();
            if (comboCount != 0U && (uint32_t)(defeatNow - lastDefeatTick) <= COMBO_WINDOW_MS) ++comboCount;
            else comboCount = 1U;
            score += 100U * comboCount;
            lastDefeatTick = defeatNow;
            hudDirty = 1;
            MarkKoopaAt(k->x, k->y);
            if (levelDefeated >= LEVEL_TOTAL_KOOPAS) {
                /* La victoria queda pendiente. El freeze NO empieza aqui:
                 * esperamos a que este ultimo Koopa pateado salga por
                 * completo de pantalla y sea desactivado por UpdateKoopa(). */
                levelClearPending = 1;
                levelClearKoopaIndex = (int8_t)i;
            }
        } else if (k->state == KOOPA_NORMAL) {
            MarkRect(player.prevX, player.prevY, PLAYER_W, PLAYER_H);
            MarkRect(player.prevX - SCREEN_WIDTH, player.prevY, PLAYER_W, PLAYER_H);
            deathState = DEATH_FREEZE;
            deathTick = HAL_GetTick();
            Sonidos_PlayDeath();
            marioFrame = MARIO_DEATH_FIRST_FRAME;
            k->frame = k->isCrab ? WalkFirst(k) : KOOPA_ATTACK_FRAME;
            g_jumpRequest = 0;
            MarkRect(player.x, player.y, PLAYER_W, PLAYER_H);
            MarkRect(player.x - SCREEN_WIDTH, player.y, PLAYER_W, PLAYER_H);
            MarkKoopaAt(k->x, k->y);
            return;
        }
    }
    if (FireTouchesMario()) {
        deathState=DEATH_FREEZE;deathTick=now;
        Sonidos_PlayDeath();
        marioFrame=MARIO_DEATH_FIRST_FRAME;g_jumpRequest=0;
        MarkRect(player.x,player.y,PLAYER_W,PLAYER_H);
        MarkRect(player.x-SCREEN_WIDTH,player.y,PLAYER_W,PLAYER_H);
    }
}

void Game_Render(void) {
    RenderDirty();
    if (gameMode == GAME_TITLE) return;
    if (gameMode == GAME_GAMEOVER) {
        if (!dmaInFlight && !batchActive && highScoresNeedSave) {
            (void)HighScores_Save(highScores);
            highScoresNeedSave = 0;
        }
        if (!dmaInFlight && !batchActive && gameOverTextDirty) {
            char line[24];
            LCD_PrintHUD("<< HIGH SCORES >>", 52, 48, 1, 0xFFFF, COLOR_BG);
            for (unsigned i = 0; i < HIGH_SCORE_COUNT; ++i) {
                snprintf(line, sizeof(line), "%uo : %08lu", i + 1U,
                         (unsigned long)highScores[i]);
                LCD_PrintHUD(line, 68, 76 + (int)i * 20, 1, 0xFFFF, COLOR_BG);
            }
            LCD_PrintHUD("----------------", 56, 184, 1, 0xFFFF, COLOR_BG);
            snprintf(line, sizeof(line), "Final Score: %08lu", (unsigned long)finalScore);
            LCD_PrintHUD(line, 40, 208, 1, 0xFFFF, COLOR_BG);
            if (gameOverNewTop1)
                LCD_PrintHUD("NEW HIGHSCORE!!", 60, 232, 1, 0xFFFF, COLOR_BG);
            gameOverTextDirty = 0;
        }
        return;
    }

    int pendingPixels = 0;
    if (levelClearPending && !dmaInFlight && !batchActive) {
        for (int r = 0; r < TILE_ROWS; ++r)
            for (int c = 0; c < TILE_COLS; ++c) pendingPixels |= dirty[r][c];
    }
    if (!pendingPixels && !dmaInFlight && !batchActive && levelClearPending &&
        levelClearKoopaIndex >= 0 && DefeatedEnemiesGone()) {
        levelClearPending = 0;
        gameMode = GAME_TIME_BONUS;
        /* Todas las vidas restantes, antes de conceder la vida del siguiente nivel. */
        if (lives > 1U) score += 1000U * (lives - 1U);
        bonusTick = HAL_GetTick();
        hudDirty = 1;
    }

    /* LCD_Print usa SPI directamente: solo llamarla con DMA completamente libre. */
    if (!dmaInFlight && !batchActive && hudDirty) {
        char scoreText[20];
        char timeText[16];
        snprintf(scoreText, sizeof(scoreText), "SCORE %06lu", (unsigned long)score);
        snprintf(timeText, sizeof(timeText), "TIME %03u", (unsigned)timeRemaining);
        LCD_PrintHUD(scoreText, 0, 8, 1, 0xFFFF, COLOR_BG);
        LCD_PrintHUD(timeText, 0, 20, 1, 0xFFFF, COLOR_BG);
        hudDirty = 0;
    }
}

