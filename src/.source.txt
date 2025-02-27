            .extern _Zp
            .extern irq_handler
            .extern _InterruptChain
            .extern viewing_screen

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

