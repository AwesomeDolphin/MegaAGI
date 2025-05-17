#ifndef PARSER_H
#define PARSER_H

extern uint8_t __far *logic_vars;
extern uint8_t __far *logic_flags;

extern uint8_t parser_word_index;
extern uint16_t parser_word_numbers[20];

bool parser_decode_string(char *target);
void parser_init(void);

#endif
