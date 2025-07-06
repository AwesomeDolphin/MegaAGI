void simpleopen(char *filename, uint8_t namelength, uint8_t device_number);
uint8_t simpleread(uint8_t *destination);
void simplewrite(uint8_t *source, uint8_t length);
void simpleprint(char *string);
void simpleclose(void);
void simplecmdchan(uint8_t *drive_command, uint8_t device_number);
void simpleerrchan(uint8_t *statusmessage, uint8_t device_number);
