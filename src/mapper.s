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

        .extern _Zp
        .extern irq_handler
        .extern _InterruptChain
        .extern viewing_screen
        .extern engine_interrupt_handler
        .extern my_quad

hmem_picdraw:   .equ 0x01
hmem_parser:    .equ 0x02
hmem_gui:       .equ 0x03
hmem_gamesave:  .equ 0x04
hmem_volume:    .equ 0x05

bank_ell:       .equ 0x01
bank_elh:       .equ 0x02
bank_eeh:       .equ 0x03
bank_sprite:    .equ 0x04
bank_edd:       .equ 0x05

        .section banked_bss,bss
        .public himem_mapbank
himem_mapbank:
        .space 1,0
bank_current:
        .space 1,0
bank_previous:
        .space 1,0

        .section code
        .public select_graphics0_mem
select_graphics0_mem:  
		lda #0xC0
		ldx #0xC4
		ldy #0xC0
		ldz #0x34
		map
		eom
		rts

        .public select_graphics1_mem
select_graphics1_mem:  
		lda #0x40
		ldx #0xC5
		ldy #0x40
		ldz #0x35
		map
		eom
		rts


        .public select_engine_logiclow_mem
select_engine_logiclow_mem:
        lda bank_current
        sta bank_previous
        lda #bank_ell
        sta bank_current

		lda #0x60
		ldx #0x23
		ldy #0x80
		ldz #0x32
		map
		eom
		rts

        .public select_engine_logichigh_mem
select_engine_logichigh_mem:  
        lda bank_current
        sta bank_previous
        lda #bank_elh
        sta bank_current
        
		lda #0x60
		ldx #0x23
		ldy #0xC0
		ldz #0x32
		map
		eom
		rts

        .public select_engine_enginehigh_mem
select_engine_enginehigh_mem:  
        lda bank_current
        sta bank_previous
        lda #bank_eeh
        sta bank_current
        
		lda #0x60
		ldx #0x23
		ldy #0x20
		ldz #0x33
		map
		eom
		rts

        .public select_sprite_mem
select_sprite_mem:  
        lda bank_current
        sta bank_previous
        lda #bank_sprite
        sta bank_current
        
		lda #0x00
		ldx #0x00
		ldy #0xC0
		ldz #0x32
		map
		eom
		rts

        .public select_engine_diskdriver_mem
select_engine_diskdriver_mem:  
        lda bank_current
        sta bank_previous
        lda #bank_edd
        sta bank_current
        
		lda #0x60
		ldx #0x23
		ldy #0x00
		ldz #0x00
		map
		eom
		rts

        .public select_nokernel_mem
select_nokernel_mem:  
		lda #0x60
		ldx #0x23
		ldy #0x00
		ldz #0x00
		map
		eom
        lda #0x20
        trb 0xd030
		rts

        .public select_kernel_mem
select_kernel_mem:  
		lda #0x00
		ldx #0x00
		ldy #0x00
		ldz #0x83
		map
		eom
        lda #0x20
        tsb 0xd030
		rts

        .public select_previous_bank
select_previous_bank:
        lda bank_previous
        cmp #bank_ell
        bne +
        jmp select_engine_logiclow_mem
+       cmp #bank_elh
        bne +
        jmp select_engine_logichigh_mem
+       cmp #bank_eeh
        bne +
        jmp select_engine_enginehigh_mem
+       cmp #bank_sprite
        bne +
        jmp select_sprite_mem
+       cmp #bank_edd
        bne +
        jmp select_engine_diskdriver_mem
+       rts

		.public select_picdraw_mem
select_picdraw_mem:
        lda himem_mapbank
        cmp #hmem_picdraw
        beq +
		sta 0xd707
        
        .byte 0x80
        .byte 0x80
        .byte 0x81
        .byte 0x00
        .byte 0x00
        .byte 0x00
        .word 0x1ff0
        .word 0x4000
        .byte 0x01
        .word 0xe000
        .byte 0x00
        .byte 0x00
        .byte 0x00

        lda #hmem_picdraw
        sta himem_mapbank

+:		rts

		.public select_parser_mem
select_parser_mem:
        lda himem_mapbank
        cmp #hmem_parser
        beq +
		sta 0xd707
        
        .byte 0x80
        .byte 0x80
        .byte 0x81
        .byte 0x00
        .byte 0x00
        .byte 0x00
        .word 0x1ff0
        .word 0x6000
        .byte 0x01
        .word 0xe000
        .byte 0x00
        .byte 0x00
        .byte 0x00

        lda #hmem_parser
        sta himem_mapbank

+:		rts

		.public select_gui_mem
select_gui_mem:
        lda himem_mapbank
        cmp #hmem_gui
        beq +
		sta 0xd707
        
        .byte 0x80
        .byte 0x80
        .byte 0x81
        .byte 0x00
        .byte 0x00
        .byte 0x00
        .word 0x1ff0
        .word 0xa000
        .byte 0x01
        .word 0xe000
        .byte 0x00
        .byte 0x00
        .byte 0x00

        lda #hmem_gui
        sta himem_mapbank
+:		rts

		.public select_gamesave_mem
select_gamesave_mem:
        lda himem_mapbank
        cmp #hmem_gamesave
        beq +
		sta 0xd707
        
        .byte 0x80
        .byte 0x80
        .byte 0x81
        .byte 0x00
        .byte 0x00
        .byte 0x00
        .word 0x1ff0
        .word 0x8000
        .byte 0x01
        .word 0xe000
        .byte 0x00
        .byte 0x00
        .byte 0x00

        lda #hmem_gamesave
        sta himem_mapbank
+:		rts

		.public select_volume_mem
select_volume_mem:
        lda himem_mapbank
        cmp #hmem_volume
        beq +
		sta 0xd707
        
        .byte 0x80
        .byte 0x80
        .byte 0x81
        .byte 0x00
        .byte 0x00
        .byte 0x00
        .word 0x1ff0
        .word 0x0000
        .byte 0x02
        .word 0xe000
        .byte 0x00
        .byte 0x00
        .byte 0x00

        lda #hmem_volume
        sta himem_mapbank
+:		rts
