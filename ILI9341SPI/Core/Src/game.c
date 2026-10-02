/*
 * game.c
 *
 * Mario + Koopa: caminar, screenwrap y reinicio tras contacto.
 * El driver ILI9341 y la fisica/movimiento de Mario se conservan.
 */
#include "game.h"
#include "ili9341.h"
#include "koopa.h"
#include "stage.h"

// ---- Flags de entrada (ver HAL_GPIO_EXTI_Callback en main.c) ----
volatile uint8_t g_moveLeft = 0;
volatile uint8_t g_moveRight = 0;
volatile uint8_t g_jumpRequest = 0;

// Y del jugador cuando está parado sobre el "piso" (borde inferior de pantalla)
static const int16_t FLOOR_Y = STAGE_FLOOR_Y - PLAYER_H;

typedef struct {
	int16_t x, y;
	int16_t prevX, prevY;
	int16_t vy;
	uint8_t onGround;
} Player;

static Player player;
static uint8_t facingLeft = 0; // 0 = viendo a la derecha, 1 = viendo a la izquierda

/* Ajustes del enemigo. El atlas original mira hacia la izquierda. */
#define KOOPA_MOVE_SPEED 2
#define KOOPA_WALK_FRAME_MS 120U
#define DEATH_FREEZE_MS 2000U
#ifndef KOOPA_SPAWN_FROM_LEFT
#define KOOPA_SPAWN_FROM_LEFT 0 /* 0: nace derecha; 1: nace izquierda */
#endif

typedef struct {
    int16_t x, y;
    int8_t direction;
    uint8_t frame;
    uint32_t animationTick;
} Koopa;
static Koopa koopa;
static uint8_t deathFrozen;
static uint32_t deathTick;

static void SpawnKoopa(uint8_t fromLeft) {
    koopa.x = fromLeft ? 0 : SCREEN_WIDTH - KOOPA_W;
    koopa.y = STAGE_FLOOR_Y - KOOPA_H;
    koopa.direction = fromLeft ? 1 : -1;
    koopa.frame = 0;
    koopa.animationTick = HAL_GetTick();
}

static void UpdateKoopa(uint32_t now) {
    koopa.x += koopa.direction * KOOPA_MOVE_SPEED;
    if (koopa.x < 0) koopa.x += SCREEN_WIDTH;
    if (koopa.x >= SCREEN_WIDTH) koopa.x -= SCREEN_WIDTH;
    uint32_t steps = (uint32_t)(now - koopa.animationTick) / KOOPA_WALK_FRAME_MS;
    if (steps != 0U) {
        koopa.frame = (uint8_t)((koopa.frame + steps) % KOOPA_WALK_FRAMES);
        koopa.animationTick += steps * KOOPA_WALK_FRAME_MS;
    }
}

/* Contacto AABB, incluyendo el segmento que aparece al otro lado del wrap.
 * Se respeta la visibilidad original de Mario en los bordes. */
static uint8_t KoopaTouchesMario(void) {
    if (player.x < 0 || player.x + PLAYER_W > SCREEN_WIDTH) return 0;
    if (player.y >= koopa.y + KOOPA_H || player.y + PLAYER_H <= koopa.y) return 0;
    for (int copy = 0; copy < 2; ++copy) {
        int x = koopa.x - copy * SCREEN_WIDTH;
        if (player.x < x + KOOPA_W && player.x + PLAYER_W > x) return 1;
    }
    return 0;
}

// ---- Movimiento horizontal: wrap de pantalla (izq/der) ----
static void WrapPlayerX(void) {
	if (player.x >= SCREEN_WIDTH) {
		// salió completamente por la derecha -> reaparece entrando por la izquierda
		player.x = -PLAYER_W;
	} else if (player.x + PLAYER_W <= 0) {
		// salió completamente por la izquierda -> reaparece entrando por la derecha
		player.x = SCREEN_WIDTH;
	}
}

// SetWindows/FillRect reciben coordenadas unsigned int, así que nunca hay que
// llamarlas con x negativa o fuera del ancho de pantalla. Por eso, mientras
// el sprite está "entrando" o "saliendo" por el borde (wrap), simplemente no
// se dibuja hasta que vuelve a estar completamente dentro de la pantalla.
static uint8_t IsOnScreenX(int x) {
	return (x >= 0) && (x + PLAYER_W <= SCREEN_WIDTH);
}

/* Composicion de regiones sucias: se envia el resultado final, nunca una
 * pantalla intermedia con los personajes borrados. Sin framebuffer completo.
 * Uso exclusivo desde el bucle principal; no llamar desde interrupciones. */
#define TILE 16
#define TILE_COLS ((SCREEN_WIDTH + TILE - 1) / TILE)
#define TILE_ROWS ((SCREEN_HEIGHT + TILE - 1) / TILE)
static uint8_t dirty[TILE_ROWS][TILE_COLS];
static uint8_t pixels[SCREEN_WIDTH * TILE * 2];
extern SPI_HandleTypeDef hspi1;

static void MarkRect(int x, int y, int w, int h) {
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

static void MarkKoopa(int x) {
    MarkRect(x, koopa.y, KOOPA_W, KOOPA_H);
    MarkRect(x - SCREEN_WIDTH, koopa.y, KOOPA_W, KOOPA_H);
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
        !SendCommand(0x2C) || !SendBytes(pixels, (uint16_t)(w*h*2))) return;
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

static uint16_t ScenePixel(int x, int y) {
    uint16_t color = Stage_Pixel(x, y);
    if (!deathFrozen && IsOnScreenX(player.x) &&
        x >= player.x && x < player.x+PLAYER_W &&
        y >= player.y && y < player.y+PLAYER_H) {
        int sx = x-player.x;
        if (facingLeft) sx = PLAYER_W-1-sx;
        uint16_t p = marioSprite[(y-player.y)*PLAYER_W+sx];
        if (p != MARIO_TRANSPARENT_COLOR) color = p;
    }
    int sx = x-koopa.x;
    if (sx < 0) sx += SCREEN_WIDTH;
    if (sx < KOOPA_W && y >= koopa.y && y < koopa.y+KOOPA_H) {
        if (koopa.direction > 0) sx = KOOPA_W-1-sx;
        uint16_t p = koopaFrames[koopa.frame][(y-koopa.y)*KOOPA_W+sx];
        if (p != KOOPA_TRANSPARENT_COLOR) color = p;
    }
    return color;
}

static void RenderDirty(void) {
    for (int row = 0; row < TILE_ROWS; ++row) {
        int col = 0;
        while (col < TILE_COLS) {
            if (!dirty[row][col]) { ++col; continue; }
            int first = col;
            while (col < TILE_COLS && dirty[row][col]) dirty[row][col++] = 0;
            int x = first*TILE, y = row*TILE;
            int w = col*TILE-x, h = TILE;
            if (x+w > SCREEN_WIDTH) w = SCREEN_WIDTH-x;
            if (y+h > SCREEN_HEIGHT) h = SCREEN_HEIGHT-y;
            int i = 0;
            for (int py = y; py < y+h; ++py) {
                for (int px = x; px < x+w; ++px) {
                    uint16_t p = ScenePixel(px, py);
                    pixels[i++] = (uint8_t)(p >> 8);
                    pixels[i++] = (uint8_t)p;
                }
            }
            BlitRegion(x, y, w, h);
        }
    }
}

void Game_Init(void) {
    deathFrozen = 0;
    g_jumpRequest = 0;
    facingLeft = 0;
    SpawnKoopa(KOOPA_SPAWN_FROM_LEFT);
	player.x = (SCREEN_WIDTH - PLAYER_W) / 2;
	player.y = FLOOR_Y;
	player.prevX = player.x;
	player.prevY = player.y;
	player.vy = 0;
	player.onGround = 1;

    MarkRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    RenderDirty();
}

void Game_Update(void) {
    if (deathFrozen) {
        g_jumpRequest = 0; /* No acumular saltos durante la pausa. */
        if ((uint32_t)(HAL_GetTick() - deathTick) >= DEATH_FREEZE_MS)
            Game_Init();
        return;
    }
    int oldKoopaX = koopa.x;
    UpdateKoopa(HAL_GetTick());
	player.prevX = player.x;
	player.prevY = player.y;

	// ---- Movimiento horizontal ----
	uint8_t prevFacing = facingLeft;
	if (g_moveLeft && !g_moveRight) {
		facingLeft = 0;
	} else if (g_moveRight && !g_moveLeft) {
		facingLeft = 1;
	}
	if (g_moveLeft) {
		player.x -= MOVE_SPEED;
	}
	if (g_moveRight) {
		player.x += MOVE_SPEED;
	}
	WrapPlayerX();

	// ---- Salto (solo si está en el piso) ----
	if (g_jumpRequest) {
		if (player.onGround) {
			player.vy = JUMP_VELOCITY;
			player.onGround = 0;
		}
		g_jumpRequest = 0; // se consume siempre, para no "guardar" saltos en el aire
	}

	// ---- Gravedad ----
	player.vy += GRAVITY;
	if (player.vy > MAX_FALL_SPEED) {
		player.vy = MAX_FALL_SPEED;
	}
	player.y += player.vy;

	// ---- Piso = límite inferior de la pantalla ----
	if (player.y >= FLOOR_Y) {
		player.y = FLOOR_Y;
		player.vy = 0;
		player.onGround = 1;
	}

    /* Marcar posiciones viejas y nuevas; componer ambas antes de enviar. */
    MarkKoopa(oldKoopaX);
    MarkKoopa(koopa.x);
    uint8_t moved = (player.x != player.prevX) || (player.y != player.prevY);
    if (moved || facingLeft != prevFacing) {
        if (IsOnScreenX(player.prevX))
            MarkRect(player.prevX, player.prevY, PLAYER_W, PLAYER_H);
        if (IsOnScreenX(player.x))
            MarkRect(player.x, player.y, PLAYER_W, PLAYER_H);
    }
    if (KoopaTouchesMario()) {
        if (IsOnScreenX(player.prevX))
            MarkRect(player.prevX, player.prevY, PLAYER_W, PLAYER_H);
        deathFrozen = 1;
        koopa.frame = KOOPA_ATTACK_FRAME;
        g_jumpRequest = 0;
        RenderDirty();
        deathTick = HAL_GetTick();
        return;
    }
    RenderDirty();
}
