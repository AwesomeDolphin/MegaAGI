#ifndef SOUND_H
#define SOUND_H
void sound_play(uint8_t sound_num, uint8_t flag_at_end);
void sound_stop(void);
void wait_sound(void);
void sound_interrupt_handler(void);

#endif