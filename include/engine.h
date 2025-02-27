#ifndef ENGINE_H
#define ENGINE_H

extern uint8_t horizon_line;

void run_loop(void);
void engine_interrupt_handler(void);
void engine_player_control(bool enable);
void engine_unblock(void);
void engine_clear_keyboard(void);
void engine_allowinput(bool allowed);
void engine_statusline(bool enable);

#endif