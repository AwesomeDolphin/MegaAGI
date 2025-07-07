#ifndef DIALOG_H
#define DIALOG_H

#include "main.h"

void dialog_print_ascii(uint8_t x, uint8_t y, bool reverse, uint8_t __far *formatstring, ...);
void dialog_clear_keyboard(void);
void dialog_show(uint8_t __far *message_string, bool accept_input);
void dialog_close(void);
bool dialog_proc(void);
void dialog_init(void);

#endif
