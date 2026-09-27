#pragma once

/* Нота для пасивного зумера: частота і тривалість. hz = REST - пауза. */
typedef struct {
    int hz;
    int ms;
} note_t;

#define REST        0

/* Частоти нот (Гц), рівномірна темперація, A4 = 440. П'єзо найгучніший
 * близько 2-4 кГц, тому мелодії краще писати в 5-6 октаві. */
#define NOTE_C5     523
#define NOTE_E5     659
#define NOTE_G5     784
#define NOTE_C6     1047
#define NOTE_E6     1319
#define NOTE_G6     1568

/* Діапазон, який зумер відтворює чутно, а LEDC (10 біт, APB 80 МГц) - точно. */
#define NOTE_HZ_MIN 200
#define NOTE_HZ_MAX 4000

/* Тиша між нотами, щоб сусідні однакові ноти не зливались в одну. */
#define NOTE_GAP_MS 20
