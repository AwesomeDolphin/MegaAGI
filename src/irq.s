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

vectab:     .space 56

            .align 256
            .public basepage
basepage:   .space 256

            .public hook_irq
hook_irq:  
		lda #0x00
		ldx #0x00
		ldy #0x00
		ldz #0xB0
		map
            ldz #0x00
            ldy #0x05
            ldx #0x7f
            lda #0xfe
            stq my_quad
            ldz #0x00
            lda #.byte0 engine_interrupt_handler
            sta 0xfffe
            sta [my_quad],z
            ldx #0xff
            stx my_quad + 1
            sta [my_quad],z
            inz
            lda #.byte1 engine_interrupt_handler
            sta 0xffff
            ldx #0x7f
            stx my_quad + 1
            sta [my_quad],z
            ldx #0xff
            stx my_quad + 1
            sta [my_quad],z
		eom
		rts

            ldx #.byte0 vectab
            ldy #.byte1 vectab
            sec
            jsr VECTOR

            lda vectab + 0
            sta _InterruptChain + 0
            lda vectab + 1
            sta _InterruptChain + 1

            lda #.byte0 _irq_rt
            sta vectab + 0
            lda #.byte1 _irq_rt
            sta vectab + 1

            sei
            ldx #.byte0 vectab
            ldy #.byte1 vectab
            clc
            jsr VECTOR

            lda #0x1b
            sta 0xd011

            cli

            rts

            .public unhook_irq
unhook_irq:
            ldx _InterruptChain + 0
            stx vectab + 0
            ldx _InterruptChain + 1
            stx vectab + 1

            sei
            ldx #.byte0 vectab
            ldy #.byte1 vectab
            clc
            jsr VECTOR
            cli

            lda #0x00
            sta zp:_Zp+0
            sta zp:_Zp+1
            rts

            .public _irq_rt
_irq_rt:
            jmp irq_handler

            .public select_graphics0_mem
select_graphics0_mem:  
		lda #0x00
		ldx #0x00
		ldy #0x80
		ldz #0xF4
		map
		eom
		rts
            .public select_graphics1_mem
select_graphics1_mem:  
		lda #0x00
		ldx #0x00
		ldy #0x00
		ldz #0xF5
		map
		eom
		rts
            .public select_execution_mem
select_execution_mem:  
		lda #0x00
		ldx #0x00
		ldy #0x00
		ldz #0xB0
		map
		eom
		rts
