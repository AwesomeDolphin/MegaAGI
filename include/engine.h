#ifndef ENGINE_H
#define ENGINE_H

#include "main.h"

void run_loop(void);
void engine_display_dialog(uint8_t __far *message_string);
void engine_show_object(uint8_t view_num);
void engine_interrupt_handler(void);
void engine_clear_keyboard(void);
void engine_allowinput(bool allowed);
void engine_statusline(bool enable);
void engine_dialog_close(void);

#endif