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

SETBNK: .equlab 0xff6b
CHKIN:  .equlab 0xffc6
CHKOUT: .equlab 0xffc9
OPEN:   .equlab 0xffc0
CLRCHN: .equlab 0xffcc
SETLFS: .equlab 0xffba
SETNAM: .equlab 0xffbd
CHRIN:  .equlab 0xffcf
CHROUT: .equlab 0xffd2
READST: .equlab 0xffb7
CLOSE:  .equlab 0xffc3

        .section disktext
        .public kernal_open
kernal_open:
        ldx zp:_Zp+0
        ldy zp:_Zp+1
        jsr SETNAM
        lda #0x02
        ldx zp:_Zp+2
        ldy #0x02
        jsr SETLFS
        lda #0x00
        tax
        jsr SETBNK
        jsr OPEN
        jmp CLRCHN

        .public kernal_read
kernal_read:
        ldx #0x02
        jsr CHKIN
        ldy #0x00
readnext:
        jsr CHRIN
        tax
        jsr READST
        and #0x02
        bne readfail
        txa
        sta (zp:_Zp+0),y
        jsr READST
        iny
        and #0x40
        bne readeoi
        cpy #0xff
        bne readnext

readeoi:
        jsr CLRCHN
        tya
        rts

readfail:
        lda #0x00
        jmp CLRCHN

        .public kernal_write
kernal_write:
        sta zp:_Zp+2
        ldx #0x02
        jsr CHKOUT
        ldy #0x00
write$:
        lda (zp:_Zp+0),y
        jsr CHROUT
        iny
        cpy zp:_Zp+2
        bne write$
        lda #0x00
        jmp CLRCHN
        
        .public kernal_print
kernal_print:
        ldy #0x00
print$:
        lda (zp:_Zp+0),y
        beq doneprint$
        jsr CHROUT
        iny
        
        jmp print$
doneprint$:
        rts

        .public kernal_close
kernal_close:
        lda #0x02
        jmp CLOSE

        .public kernal_cmdchan
kernal_cmdchan:
        tax
        lda #0x0f
        tay
        jsr SETLFS
        lda #0x00
        jsr SETNAM
        jsr OPEN
        jsr CLRCHN

        ldx #0x0f
        jsr CHKOUT
        ldy #0x00
cmdnext:
        lda (zp:_Zp+0),y
        jsr CHROUT
        iny
        cpy #0xff
        beq cmddone
        cmp #0x0d
        bne cmdnext
cmddone:
        lda #0x0f
        jmp CLOSE

        .public kernal_errchan
kernal_errchan:
        tax
        lda #0x0f
        tay
        jsr SETLFS
        lda #0x00
        jsr SETNAM
        jsr OPEN
        jsr CLRCHN

        ldx #0x0f
        jsr CHKIN
        ldy #0x00
errnext:
        jsr CHRIN
        sta (zp:_Zp+0),y
        iny
        cpy #0xff
        beq errdone
        cmp #0x0d
        bne errnext
        dey
errdone:
        lda #0x00
        sta (zp:_Zp+0),y

        lda #0x0f
        jmp CLOSE

        .public simpleerrcode
simpleerrcode:
        ldy #0x00
        lda (zp:_Zp+0),y
        sec
        sbc #0x30
        asl a
        sta zp:_Zp+2
        asl a
        asl a
        clc
        adc zp:_Zp+2
        iny
        clc
        adc (zp:_Zp+0),y
        sec
        sbc #0x30
        rts

        .public disk_exit_kernal
disk_exit_kernal:
        sei
        lda #0x00
        ldx #0x00
        ldy #0x00
        ldz #0xB0
        map
        eom
        lda 0xd030
        and #0xdf
        sta 0xd030

        sta 0xd707
        .byte 0x80
        .byte 0x80
        .byte 0x00
        .byte 0x00
        .word 0x1e00
        .word 0x2200
        .byte 0x00
        .word 0x0200
        .byte 0x00
        .byte 0x00
        .byte 0x00

        rts

        .public disk_enter_kernal
disk_enter_kernal:  
        sta 0xd707
        
        .byte 0x80
        .byte 0x00
        .byte 0x81
        .byte 0x80
        .byte 0x00
        .byte 0x04
        .word 0x1e00
        .word 0x0200
        .byte 0x00
        .word 0x2200
        .byte 0x00
        .byte 0x00
        .byte 0x00

        .byte 0x80
        .byte 0x80
        .byte 0x81
        .byte 0x00
        .byte 0x00
        .byte 0x00
        .word 0x1e00
        .word 0x0200
        .byte 0x00
        .word 0x0200
        .byte 0x00
        .byte 0x00
        .byte 0x00

        sei
        lda #0x00
        ldx #0x00
        ldy #0x00
        ldz #0x83
        map
        eom
        lda 0xd030
        ora #0x20
        sta 0xd030

	rts
