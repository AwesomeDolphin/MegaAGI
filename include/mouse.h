void mouse_init(void);
void mouse_hide(void);
void mouse_show(void);
void mouse_irq(void);

extern uint16_t mouse_xpos;
extern uint16_t mouse_ypos;
extern uint8_t mouse_prvx;
extern uint8_t mouse_prvy;
