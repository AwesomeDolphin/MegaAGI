#ifndef ENGINE_H
#define ENGINE_H

void run_loop(void);
void engine_interrupt_handler(void);
void engine_player_control(bool enable);
void engine_unblock(void);
void engine_horizon(uint8_t horizon_line);
void engine_clear_keyboard(void);

#endif