#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <mega65.h>
#include <stdbool.h>

#include "engine.h"
#include "sound.h"

__attribute__((kernal_interrupt))
void irq_handler(void) {
    engine_interrupt_handler();
    sound_interrupt_handler();
}