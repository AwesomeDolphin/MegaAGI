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
        .extern mouseypointerthing

potax:  .equlab 0xd620
potay:  .equlab 0xd621
sprxpos:.equlab 0xd000
sprypos:.equlab 0xd001
spropos:.equlab 0xd010
spren:  .equlab 0xd015
sprcol: .equlab 0xd027
sprx64: .equlab 0xd057
spr16:  .equlab 0xd06b
sprptr: .equlab 0xd06c
palsel: .equlab 0xd070

        .section zzpage,bss
mouse_zp1:
        .space 1,0
mouse_bits:
        .space 1,0
        .public mouse_prvx
mouse_prvx:
        .space 1,0
mouse_prvy:
        .public mouse_prvy
        .space 1,0

        .section data,data
        .public mouse_xpos
mouse_xpos:
        .word 0x0000
        .public mouse_ypos
mouse_ypos:
        .word 0x0000

        .section extradata,bss
        .public hw_spriteptrs
        .align 16
hw_spriteptrs:
        .space 16,0

        .section ls_spritetext
        .public mouse_init
mouse_init:
        lda #.byte0 hw_spriteptrs
        ldx #.byte1 hw_spriteptrs
        ldy #.byte2 hw_spriteptrs
        ldz #0x00
        stq zp:_Zp+4

        lda #.byte0 mouseypointerthing
        sta mouse_zp1
        lda #.byte1 mouseypointerthing
        lsr a
        ror zp:mouse_zp1
        lsr a
        ror zp:mouse_zp1
        lsr a
        ror zp:mouse_zp1
        lsr a
        ror zp:mouse_zp1
        lsr a
        ror zp:mouse_zp1
        lsr a
        ror zp:mouse_zp1
        clc
        adc #0x04
        ldz #0x01
        sta [zp:_Zp+4],z
        lda mouse_zp1
        dez
        sta [zp:_Zp+4],z

        lda #.byte0 hw_spriteptrs
        sta sprptr
        lda #.byte1 hw_spriteptrs
        sta sprptr+1
        lda #.byte2 hw_spriteptrs
        ora #0x80
        sta sprptr+2
        lda #0x00
        sta spropos
        sta sprcol
        sta sprxpos
        sta sprypos
        lda #0x01
        sta sprx64
        sta spr16
        sta spren
        sta mouse_bits

        rts

        .public mouse_hide
mouse_hide:
        lda #0x00
        sta spren
        rts

        .public mouse_show
mouse_show:
        lda mouse_bits
        sta spren
        rts
        
        .section code
        .public mouse_irq
mouse_irq:
        ldy mouse_prvx
        lda potax
        jsr movechk$
        sty mouse_prvx

        clc
        adc mouse_xpos
        sta mouse_xpos
        txa 
        adc mouse_xpos+1
        sta mouse_xpos+1

        bpl +
        lda #0x00
        sta mouse_xpos
        sta mouse_xpos+1

+:      beq ++
        lda mouse_xpos
        cmp #0x40
        bcc ++
        lda #0x3f
        sta mouse_xpos

++:     ldy mouse_prvy
        lda potay
        jsr movechk$
        sty mouse_prvy
        eor #0xff
        sec
        adc mouse_ypos
        sta mouse_ypos
        txa
        eor #0xff
        adc #0x00

        bpl +++
        lda #0x00
        sta mouse_ypos
        bra ++++
        
+++:    lda mouse_ypos
        cmp #0xc8
        bcc ++++
        lda #0xc7
        sta mouse_ypos

++++:   
        lda mouse_xpos
        clc
        adc #0x18
        sta sprxpos
        lda mouse_xpos+1
        adc #0x00
        sta spropos

        lda mouse_ypos
        clc
        adc #0x32
        sta sprypos

        rts

movechk$:
        sty mouse_zp1
        tay
        sec
        sbc mouse_zp1
        ldx #0x00
        asl a
        asr a
        asr a
        bpl +
        ldx #0xff
+:      rts
