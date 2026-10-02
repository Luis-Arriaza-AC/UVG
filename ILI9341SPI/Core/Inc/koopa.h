/* Koopa: frames 16x16 extraidos por filas del atlas RGB565 48x48. */
#ifndef INC_KOOPA_H_
#define INC_KOOPA_H_
#include <stdint.h>
#define KOOPA_W 16
#define KOOPA_H 16
#define KOOPA_FRAME_COUNT 7
#define KOOPA_WALK_FRAMES 4
#define KOOPA_ATTACK_FRAME 6
#define KOOPA_TRANSPARENT_COLOR 0x4208
extern const uint16_t koopaFrames[KOOPA_FRAME_COUNT][KOOPA_W * KOOPA_H];
#endif
