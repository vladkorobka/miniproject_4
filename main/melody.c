#include "melody.h"

const note_t feed_melody[] = {
    { NOTE_G5, 100 }, { NOTE_C6, 100 }, { NOTE_E6, 100 },
    { NOTE_G6, 250 },
    { REST,    80 },
    { NOTE_E6, 120 }, { NOTE_G6, 450 },
    { REST,    120 },
};

const size_t feed_melody_len = sizeof(feed_melody) / sizeof(feed_melody[0]);

int feed_melody_duration_ms(void)
{
    int total = MEOW_UP_MS + MEOW_DOWN_MS;
    for (size_t i = 0; i < feed_melody_len; i++) {
        total += feed_melody[i].ms + NOTE_GAP_MS;
    }
    return total;
}
