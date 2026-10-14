/*
 * graphics.h
 *
 * Recursos graficos RGB565
 */
#ifndef INC_GRAPHICS_H_
#define INC_GRAPHICS_H_

#include <stdint.h>

#define MARIO_W 16
#define MARIO_H 32
#define MARIO_FRAME_COUNT 9
#define MARIO_IDLE_FRAME 0
#define MARIO_WALK_FIRST_FRAME 1
#define MARIO_WALK_FRAME_COUNT 5
#define MARIO_JUMP_FRAME 6
#define MARIO_DEATH_FIRST_FRAME 7
#define MARIO_DEATH_FRAME_COUNT 2
#define MARIO_TRANSPARENT_COLOR 0x4208

#define KOOPA_W 16
#define KOOPA_H 16
#define KOOPA_FRAME_COUNT 7
#define KOOPA_WALK_FRAMES 4
#define KOOPA_ATTACK_FRAME 6
#define KOOPA_TRANSPARENT_COLOR 0x4208
#define CRAB_FRAME_COUNT 8
#define FIRE_FRAME_COUNT 3
#define FREEZIE_FRAME_COUNT 12
#define FREEZIE_W 16
#define FREEZIE_H 16
#define FREEZIE_WALK_FRAMES 6
#define FREEZIE_DEATH_FIRST_FRAME 6
#define FREEZIE_DEATH_FRAMES 6
#define FIRE_W 16
#define FIRE_H 16
#define ENEMY_TRANSPARENT_COLOR 0x4208

#define TITLE_W 183
#define TITLE_H 81
#define TITLE_FRAME_COUNT 6

#define LIFE_W 8
#define LIFE_H 6
#define LIFE_TRANSPARENT_COLOR 0x4208

#define STAGE_TILE_COUNT 25
#define STAGE_TILE_W 8
#define STAGE_TILE_H 8

extern const uint16_t marioFrames[MARIO_FRAME_COUNT][MARIO_W * MARIO_H];
extern const uint16_t koopaFrames[KOOPA_FRAME_COUNT][KOOPA_W * KOOPA_H];
extern const uint16_t stageTiles[STAGE_TILE_COUNT][STAGE_TILE_W * STAGE_TILE_H];
extern const uint16_t titleFrames[TITLE_FRAME_COUNT][TITLE_W * TITLE_H];
extern const uint16_t lifeSprite[LIFE_W * LIFE_H];


extern const uint16_t crabFrames[CRAB_FRAME_COUNT][16 * 16];
extern const uint16_t fireFrames[FIRE_FRAME_COUNT][FIRE_W * FIRE_H];
extern const uint16_t blueFireFrames[FIRE_FRAME_COUNT][FIRE_W * FIRE_H];
extern const uint16_t freezieFrames[FREEZIE_FRAME_COUNT][FREEZIE_W * FREEZIE_H];
#endif /* INC_GRAPHICS_H_ */
