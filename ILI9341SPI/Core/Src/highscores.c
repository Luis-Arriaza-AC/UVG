#include "highscores.h"
#include <string.h>

#include "ff.h"

//Encryptacion y checksum para evitar cheaters que carguen partidas modificadas
#define SCORE_MAGIC 0x4D425348UL /* "MBSH" */
typedef struct {
    uint32_t magic;
    uint32_t scores[HIGH_SCORE_COUNT];
    uint32_t checksum;
} ScoreFile;
static FATFS scoreFs;
static uint8_t mounted;

static uint32_t Checksum(const ScoreFile *data) {
    uint32_t value = data->magic ^ 0xA55A3CC3UL;
    for (unsigned i = 0; i < HIGH_SCORE_COUNT; ++i)
        value = (value << 5) ^ (value >> 27) ^ data->scores[i];
    return value;
}

void HighScores_Init(void) {
    mounted = (uint8_t)(f_mount(&scoreFs, "", 1) == FR_OK);
}

void HighScores_Load(uint32_t scores[HIGH_SCORE_COUNT]) {
    memset(scores, 0, HIGH_SCORE_COUNT * sizeof(scores[0]));
    if (!mounted) return;
    FIL file;
    UINT readBytes = 0;
    ScoreFile data;
    if (f_open(&file, "highscores.dat", FA_READ) != FR_OK) return;
    FRESULT result = f_read(&file, &data, sizeof(data), &readBytes);
    (void)f_close(&file);
    if (result == FR_OK && readBytes == sizeof(data) &&
        data.magic == SCORE_MAGIC && data.checksum == Checksum(&data)) {
        memcpy(scores, data.scores, sizeof(data.scores));
    }
}

uint8_t HighScores_Insert(uint32_t scores[HIGH_SCORE_COUNT], uint32_t score) {
    if (score == 0U) return 0U;
    for (unsigned i = 0; i < HIGH_SCORE_COUNT; ++i) {
        if (score <= scores[i]) continue;
        for (unsigned j = HIGH_SCORE_COUNT - 1U; j > i; --j)
            scores[j] = scores[j - 1U];
        scores[i] = score;
        return (uint8_t)(i + 1U);
    }
    return 0U;
}

uint8_t HighScores_Save(const uint32_t scores[HIGH_SCORE_COUNT]) {
    if (!mounted) return 0U;
    FIL file;
    UINT written = 0;
    ScoreFile data = {SCORE_MAGIC, {0}, 0};
    memcpy(data.scores, scores, sizeof(data.scores));
    data.checksum = Checksum(&data);
    if (f_open(&file, "highscores.dat", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
        return 0U;
    FRESULT result = f_write(&file, &data, sizeof(data), &written);
    if (result == FR_OK) result = f_sync(&file);
    (void)f_close(&file);
    return (uint8_t)(result == FR_OK && written == sizeof(data));
}

uint8_t HighScores_HasSdSupport(void) {
    return mounted;
}
