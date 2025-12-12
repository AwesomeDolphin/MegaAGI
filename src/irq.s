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

VECTOR:     .equlab 0xff8d
LINESTEPL:  .equlab 0xd058
CHRXSCL:    .equlab 0xd05a
CHRCOUNT:   .equlab 0xd05e
SCRNPTRMSB: .equlab 0xd061

            .section code
irq_tbl:
            .word _nmi_rt
            .word _nmi_rt
            .word engine_interrupt_handler

            .public hook_irq
hook_irq:  
            ldz #0x00
            ldy #0x05
            ldx #0x7f
            lda #0xfa
            stq my_quad
            ldz #0x05
            ldx #0x05
hook_loop1
            lda irq_tbl,x
            sta 0xfffa,x
            sta [my_quad],z
            dex
            dez
            bpl hook_loop1

            ldx #0xff
            stx my_quad + 1
            ldz #0x05
            ldx #0x05
hook_loop2
            lda irq_tbl,x
            sta [my_quad],z
            dex
            dez
            bpl hook_loop2
            cli
		rts

            .public _nmi_rt
_nmi_rt:
            rti
