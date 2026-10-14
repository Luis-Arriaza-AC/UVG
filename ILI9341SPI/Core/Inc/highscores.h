#ifndef INC_HIGHSCORES_H_
#define INC_HIGHSCORES_H_

#include <stdint.h>

//Cantidad de highscores maximas
#define HIGH_SCORE_COUNT 5U


void HighScores_Init(void);
void HighScores_Load(uint32_t scores[HIGH_SCORE_COUNT]);
uint8_t HighScores_Insert(uint32_t scores[HIGH_SCORE_COUNT], uint32_t score);
uint8_t HighScores_Save(const uint32_t scores[HIGH_SCORE_COUNT]);
uint8_t HighScores_HasSdSupport(void);

#endif
