        .extern _Zp

CHKIN:  .equlab 0xffc6
OPEN:   .equlab 0xffc0
CLRCHN: .equlab 0xffcc
SETLFS: .equlab 0xffba
SETNAM: .equlab 0xffbd
CHRIN:  .equlab 0xffcf
CHROUT: .equlab 0xffd2
READST: .equlab 0xffb7
CLOSE:  .equlab 0xffc3

        .section code
        .public simpleopen
simpleopen:
        ldx zp:_Zp+0
        ldy zp:_Zp+1
        jsr SETNAM
        lda #0x02
        ldx #0x08
        ldy #0x02
        jsr SETLFS
        jsr OPEN
        jmp CLRCHN

        .public simpleread
simpleread:
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

        .public simplewrite
simplewrite:
        jmp CHROUT

        .public simpleprint
simpleprint:
        ldy #0x00
print$:
        lda (zp:_Zp+0),y
        beq doneprint$
        jsr CHROUT
        iny
        
        jmp print$
doneprint$:
        rts

        .public simpleclose
simpleclose:
        lda #0x02
        jmp CLOSE