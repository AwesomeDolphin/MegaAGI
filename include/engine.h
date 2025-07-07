#ifndef ENGINE_H
#define ENGINE_H

#include "main.h"

extern uint8_t status_line_score;

void run_loop(void);
void engine_show_object(uint8_t view_num);
void engine_interrupt_handler(void);
void engine_clear_keyboard(void);
void engine_allowinput(bool allowed);
void engine_statusline(bool enable);

#endif