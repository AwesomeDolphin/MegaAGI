#ifndef LOGIC_H
#define LOGIC_H

extern uint8_t __far *logic_vars;
extern uint8_t __far *logic_flags;

void logic_set_flag(uint8_t flag);
void logic_reset_flag(uint8_t flag);
void logic_load(uint8_t logic_num);
void logic_run(uint8_t logic_num);
void logic_purge(uint16_t freed_offset);
void logic_init(void);

#endif
