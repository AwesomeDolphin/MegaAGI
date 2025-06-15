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
		lda #0x00
		ldx #0x00
		ldy #0x00
		ldz #0xB0
		map
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

		eom
		rts

            .public _nmi_rt
_nmi_rt:
            rti

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
