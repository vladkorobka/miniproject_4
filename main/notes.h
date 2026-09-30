#pragma once

typedef struct {
    int hz;
    int ms;
} note_t;

#define REST        0
#define NOTE_C5     523
#define NOTE_E5     659
#define NOTE_G5     784
#define NOTE_C6     1047
#define NOTE_E6     1319
#define NOTE_G6     1568
#define NOTE_HZ_MIN 200
#define NOTE_HZ_MAX 4000
#define NOTE_GAP_MS 20
