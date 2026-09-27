#pragma once

void servo_init(void);

/* Встановлює кут 0-180 градусів. */
void servo_set_angle(int angle_deg);

/* Знімає навантаження з серво (duty = 0), щоб не грілось у нерухомому стані. */
void servo_detach(void);
