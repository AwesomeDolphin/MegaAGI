#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "gfx.h"
#include "logic.h"
#include "memmanage.h"
#include "parser.h"

#define MAX_WORD_LENGTH 24
#define ALPHABET_SIZE 26
#define XOR_VALUE 0x7f

#pragma clang section bss="midmembss" data="midmemdata" rodata="midmemrodata" text="midmemtext"

// Structure to represent our dictionary reference
typedef struct dictionary {
    uint16_t __far * letter_offsets;  // Points to the 26 16-bit offsets in memory
    uint8_t __far * data;             // Points to the raw dictionary data
} dictionary_t;

dictionary_t dict;
uint8_t parser_word_index;
uint16_t parser_word_numbers[20];
bool parser_debug;

// Function to find a word in the dictionary and collect matching word numbers
bool parser_find_word(const char* target) {
    // Check if first letter is in range a-z
    char first_letter = target[0];
    int offset_index = first_letter - 'a';
    uint16_t offset = dict.letter_offsets[offset_index];
    
    // If offset is 0, no words starting with this letter
    if (offset == 0) {
        return false;
    }

    if (parser_debug) {
        gfx_print_ascii(0,0,(uint8_t *)target);
    }
    
    // Set current position in dictionary
    uint8_t __far * current_pos = dict.data + offset;

    // Buffer to reconstruct words
    char current_word[MAX_WORD_LENGTH] = {0};
    int current_word_len = 0;
    
    int target_len = strlen(target);
    
    bool first = true;
    // Iterate through words in this section
    while (1) {
        // Read prefix length
        uint8_t prefix_len = *current_pos++;
        
        // If we've reached the next letter section or end of data
        if ((prefix_len == 0x00) && (!first)) {
            break;
        }
        first = false;

        // Truncate current word to prefix length
        current_word_len = prefix_len;
        
        // Read the rest of the word (XORed with 0x7F)
        int i = prefix_len;
        bool copying = true;
        while (copying) {
            uint8_t c = *current_pos++;
            
            // Check if we've reached end of word
            if (c & 0x80) { // 0x7F XOR 0x7F = 0
                c = c & 0x7f;
                copying = false;
            }
            
            // De-XOR the character and add to current word
            current_word[i++] = c ^ XOR_VALUE;
            
            // Prevent buffer overflow
            if (i >= MAX_WORD_LENGTH - 1) {
                break;
            }
        }
        
        // Null-terminate the reconstructed word
        current_word[i] = '\0';
        current_word_len = i;
        
        // Read the word number (2 bytes)
        uint16_t word_number = *current_pos << 8;
        word_number = *(current_pos + 1);
        current_pos += 2;

        if (parser_debug) {
            gfx_print_ascii(0,1,(uint8_t *)current_word);
            gfx_print_ascii(0, 2, (uint8_t *)"%d", i);
            gfx_print_ascii(0,3,(uint8_t *)"%d", word_number);
        }

        // OPTIMIZATION 3: Early length check
        if (current_word_len != target_len) {
            if (parser_debug) {
                gfx_print_ascii(0,4,(uint8_t *)"O3");
                while(ASCIIKEY == 0) {
                    // Wait for key release
                }
                ASCIIKEY = 0; // Clear key
            }
            continue;
        }
        
        int comp_result = strcmp(current_word, target);
        // Compare with target
        if (comp_result == 0) {
            // Match found, store word number if not 0
            if (word_number > 0) {
                parser_word_numbers[parser_word_index] = word_number;
                if (parser_debug) {
                    gfx_print_ascii(0,4,(uint8_t *)"M: %d %d", word_number, parser_word_index);
                    while(ASCIIKEY == 0) {
                        // Wait for key release
                    }
                    ASCIIKEY = 0; // Clear key
                }
                parser_word_index++;
            }
            return true;
        }
    }
    
    return false;
}

void parser_cook_string(char *target) {
    uint8_t len = strlen(target);

    char *destination = target;

    for (uint8_t i = 0; i < len; i++) {
        if ((target[i] >= 65) && (target[i] <= 90)) {
            *destination = target[i] + 32;
            destination++;
        } else if ((target[i] >= 97) && (target[i] <= 122)) {
            *destination = target[i];
            destination++;
        } else if ((target[i] >= 48) && (target[i] <= 57)) {
            *destination = target[i];
            destination++;
        } else if (target[i] == 32) {
            *destination = target[i];
            destination++;
            while(target[i+1] == 32) {
                i++;
            }
        }
    }
}

bool parser_decode_string(char *target) {
    parser_cook_string(target);
    uint8_t index = 0;
    parser_word_index = 0;
    char *token = strtok(target, " ");
    logic_vars[9] = 0;
    while (token != NULL) {
        if (!parser_find_word(token)){
            logic_vars[9] = index + 1;
            break;
        }
        index++;
        token = strtok(NULL, " ");
    }
    logic_set_flag(2);
    logic_reset_flag(4);
    return true;
}

// Initialize dictionary reference to pre-loaded memory
void parser_init(void) {
    parser_debug = false;
   // Point to the 26 letter offsets at the start of the dictionary
    dict.letter_offsets = (uint16_t __far *) (chipmem2_base+1);
    
    // Dictionary data starts right after the offsets
    dict.data = chipmem2_base + 1;

    for (int i = 0; i < 52; i += 2) {
        uint8_t temp = dict.data[i];
        dict.data[i] = dict.data[i + 1];
        dict.data[i + 1] = temp;
    }
}

