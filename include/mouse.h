void mouse_init(void);
void mouse_hide(void);
void mouse_show(void);
void mouse_irq(void);

extern volatile uint16_t mouse_xpos;
extern volatile uint16_t mouse_ypos;
extern volatile uint8_t mouse_prvx;
extern volatile uint8_t mouse_prvy;
