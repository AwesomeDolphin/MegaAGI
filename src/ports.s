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
joyport2:  .equlab 0xdc00
joyport1:  .equlab 0xdc01

        .section banked_bss,bss
        .public joystick_direction
joystick_direction:
        .space 1,0
        .public joystick_fire
joystick_fire:
        .space 1,0
        .public mouse_leftclick
mouse_leftclick:
        .space 1,0

        .section code
        .public joyports_poll
joyports_poll:
        sei             ;disable IRQ to inhibit kybd
        ldy #0xff
        sty joyport2   ;set to not read any kybd inputs

        lda #0x02        ;disable MEGA65 keyboard UART high line
        tsb 0xd607       ;(otherwise Tab, Alt, Help, etc. register and JOY(1) events)
        tsb 0xd608

-       lda joyport2    ;read joystick values
        cmp joyport2    ;debounce
        bne -

        ldx #0x00
        bit #0x10
        bne +
        inx
+       stx joystick_fire
        and #0x0f
        sta joystick_direction

-       lda joyport1    ;read mouse values
        cmp joyport1    ;debounce
        bne -

        ldx #0x00
        bit #0x10
        bne +
        inx
+       stx mouse_leftclick

        lda #0x02        ;re-enable MEGA65 keyboard UART high line
        trb 0xd607
        trb 0xd608

        ldy #0x7f
        sty joyport2    ;reset kybd output lines
        cli

        rts

