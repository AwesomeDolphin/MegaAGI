#ifndef LOGIC_H
#define LOGIC_H

#include <stdbool.h>
extern uint8_t __far *logic_vars;
extern uint8_t __far *logic_flags;
extern uint16_t object_data_offset;
extern bool debug;
typedef struct logic_info {
    uint16_t offset;
    uint8_t __far *text_offset;
} logic_info_t;

void logic_strcpy_far_near(uint8_t *dest_string, uint8_t __far *src_string);
void logic_strcpy_far_far(uint8_t __far *dest_string, uint8_t __far *src_string);
void logic_set_flag(uint8_t flag);
void logic_reset_flag(uint8_t flag);
void logic_load(uint8_t logic_num);
void logic_run(void);
void logic_purge(uint16_t freed_offset);
void logic_init(void);

#endif
