/***************************************************************************
    MEGA65-AGI -- Sierra AGI interpreter for the MEGA65
    Copyright (C) 2025  Keith Henrickson

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
***************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <mega65.h>

#include "disk.h"
#include "gamesave.h"
#include "memmanage.h"
#include "mouse.h"

#pragma clang section bss = "diskbss" data = "diskdata" rodata = "diskrodata" text = "disktext"

 uint8_t disk_load_attic(char *name, uint32_t *data_size, uint8_t device)
{
    uint8_t __huge *data_destination = NULL;
    data_destination = attic_memory + atticmem_allocoffset;
    uint32_t local_data_size;

    static uint8_t buffer[256];
    strncpy((char *)buffer, name, 256-5);
    strcat((char *)buffer, ",S,R");
    size_t namelen = strlen((char *)buffer);

    // NO KERNAL CALLS ABOVE THIS LINE, NO NON-KERNAL CALLS BELOW THIS LINE
    disk_enter_kernal();

    kernal_open((char *)buffer, namelen, device);
    size_t bytes_read;
    local_data_size = 0;
    do
    {
        bytes_read = kernal_read(buffer);
        if (bytes_read > 0)
        {
            for (size_t idx = 0; idx < bytes_read; idx++)
            {
                data_destination[idx] = buffer[idx];
            }
            local_data_size += bytes_read;
            data_destination += bytes_read;
        }
    } while (bytes_read > 0);
    kernal_close();

    kernal_errchan(buffer, device);
    uint8_t errcode = simpleerrcode(buffer);

    // NO KERNAL CALLS BELOW THIS LINE, NO NON-KERNAL CALLS ABOVE THIS LINE
    disk_exit_kernal();

    if (errcode == 0) {
        *data_size = local_data_size;
        atticmem_alloc(*data_size);
    } else {
        *data_size = 0;
    }

    return errcode;
}

 uint8_t disk_save_attic(char *filename, uint32_t attic_offset, uint32_t data_size, uint8_t device) {
    static uint32_t chunk_size;
    static uint32_t i;
    static uint8_t idx;
    static uint8_t buffer[256];
    uint8_t __huge *data_source = NULL;
    data_source = attic_memory + attic_offset;

    strncpy((char *)buffer, filename, 256-5);
    strcat((char *)buffer, ",S,W");
    size_t namelen = strlen((char *)buffer);

    // NO KERNAL CALLS ABOVE THIS LINE, NO NON-KERNAL CALLS BELOW THIS LINE
    disk_enter_kernal();
    kernal_open((char *)buffer, namelen, device);

    for (i = 0; i < data_size; i += 250) {
        chunk_size = (data_size - i > 250) ? 250 : (data_size - i);
        for (idx = 0; idx < chunk_size; idx++)
        {
            buffer[idx] = data_source[idx];
        }
        data_source += chunk_size;
        if (!kernal_write(buffer, chunk_size)) {
            break;
        }
    }
    kernal_close();

    kernal_errchan(buffer, device);
    uint8_t errcode = simpleerrcode(buffer);

    // NO KERNAL CALLS BELOW THIS LINE, NO NON-KERNAL CALLS ABOVE THIS LINE
    disk_exit_kernal();

    return errcode;
}


