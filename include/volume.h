#ifndef VOLUME_H
#define VOLUME_H

typedef enum volobj_kind {
    voLogic,
    voPic,
    voSound,
    voView,
} volobj_kind_t;

void load_directory_files(void);
void load_volume_files(void);
uint16_t load_volume_object(volobj_kind_t kind, uint8_t volobj_num, uint16_t *object_length);
uint8_t __huge *locate_volume_object(volobj_kind_t kind, uint8_t volobj_num, uint16_t *object_length);

#endif