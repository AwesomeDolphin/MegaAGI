// original intro by deathy and spairhead under the unlicense
.cpu _45gs02
// MEGA65 platform PRG executable starting in MEGA65 mode.
.file [name="agi.lgo", type="prg", segments="Program"]

#import "global_functions.asm"
#import "mega65_io.asm"

.segmentdef Program [segments="Basic, Code, Data"]
.segmentdef Basic [start=$2001]

.segmentdef Code [start=$202a]
.segmentdef Data [startAfter="Code"]
.segment Basic
	basic_stub(main)
	
.segment Code
	.const line_length = 20
	.const line_count =  25
	.const base_char = CHARSET / $40
	.const zp = $02
	// TODO: Add in a customizable delay constant
	
	// mega sample constant values
	// 00. Senfsosse's Aid
	// 01. Angry 64 Boss
	// 02. Angry Boss
	// 03. Dutch 8-bit Bicycle
	.const sample_id = 3

.print "$" + toHexString(base_char)
	
main: {
    // enable 40MHz
	lda #65
    sta CPU_PORTDDR
		
	// Enable the VIC4
	lda #$47
	sta VIC4_KEY
	lda #$53
	sta VIC4_KEY
	
	// configure PAL/NTSC variation
	lda	VIC4_PALNTSC_ADDR
	and	#VIC4_PALNTSC_MASK
	bne	!+
	
	lda	#$1a
	sta	matrix_raster
	lda	#$01
	sta	matrix_raster + 1
	
	lda #VIC4_PALNTSC_MASK
	trb VIC4_PALNTSC_ADDR
	bra	!++
	
!:
	lda	#$e8
	sta	matrix_raster
	lda	#$00
	sta	matrix_raster + 1
	
	lda #VIC4_PALNTSC_MASK
	tsb VIC4_PALNTSC_ADDR

!:
	// disable raster interrupts
	lda #VIC4_FNRST_MASK
	trb VIC4_FNRST_ADDR
	
	//sta VIC4_FNRST_CMP_ADDR
	
	// disable 640 horizontal width
	lda #VIC3_H640_MASK
	trb VIC3_H640_ADDR

	// disable hot registers
	lda #VIC4_HOTREG_MASK
    trb VIC4_HOTREG_ADDR
	
	// use wide character lookup (i.e. character data anywhere in memory)
    lda #VIC4_CHR16_MASK
    sta VIC4_CHR16_ADDR

	// configure screen row length
	lda #line_length << 1
    sta VIC4_LINESTEPLSB
    lda #line_length
    sta VIC4_CHRCOUNT
    lda #$00
    sta VIC4_LINESTEPMSB
    sta VIC4_CHRCOUNT_ADDR
    lda #line_count
    sta VIC4_Number
	
	// load attributes map
	run_dma_job(fill_attribute_map)
	
	// clear the screen
	run_dma_job(clear_tile_map)
    
	// draw the logo itself
	ldx #base_char + 1
	ldy #$00
!:
	stx $09c4, y
	inx
	iny
	iny
	cpy #$10
	bne !-
	
	ldy #$00
!:
	stx $09ec, y
	inx
	iny
	iny
	cpy #$10
	bne !-
	
	ldy #$00
!:
	stx $0a14, y
	inx
	iny
	iny
	cpy #$10
	bne !-
	
	// load the color palette
    run_dma_job(load_color_palette_dma)
	
	// disable 400 vertical height
	lda #VIC3_V400_MASK
	trb VIC3_V400_ADDR
	
	lda #$02
	sta VIC2_BORDERCOL_ADDR
	sta VIC2_SCREENCOL_ADDR

	lda #VIC2_BLNK_MASK
	trb VIC2_BLNK_ADDR
	
	lda #$00

	sta $d405
	sta $d40c
	sta $d413
	sta $d404
	sta $d40b
	sta $d412

	sta $d425
	sta $d42c
	sta $d433
	sta $d424
	sta $d42b
	sta $d432
	
	sta $d445
	sta $d44c
	sta $d453
	sta $d444
	sta $d44b
	sta $d452

	sta $d465
	sta $d46c
	sta $d473
	sta $d464
	sta $d46b
	sta $d472

	lda #$07
	sta $d418
	sta $d438
	sta $d458
	sta $d478

	lda #$f0
	sta $d406
	sta $d426
	sta $d446
	sta $d466
	sta $d40d
	sta $d42d
	sta $d44d
	sta $d46d
	sta $d414
	sta $d434
	sta $d454
	sta $d474
	
	lda #$2a
	sta drawin_index
	sta drawin_delay
	
delay_loop:
    ldx matrix_raster
    lda matrix_raster + 1
!:	
    cpx VIC4_FNRASTERLSB
    bne !-
	lda VIC4_FN_RASTER_MSB_ADDR
	and #$07
	cmp matrix_raster + 2
	bne !-

	inx
		
!:	    
    cpx VIC4_FNRASTERLSB
    bne !-
	
	dec drawin_delay
	bpl delay_loop
	
	lda zp
	sta zp_backup
	lda zp+1
	sta zp_backup+1
	
	lda #$00
	sta zp
	lda #$d4
	sta zp+1

	// supersaw init SID voice control
	supersaw_ctl( $21 )
	
drawin_loop:
	jsr supersaw
	
    ldx matrix_raster
		
!:	    
    cpx VIC4_FNRASTERLSB
    bne !-
	lda VIC4_FN_RASTER_MSB_ADDR
	and #$07
	cmp matrix_raster + 1
    bne !-
	
	lda #VIC2_BLNK_MASK
	trb VIC2_BLNK_ADDR
	
	clc
    lda matrix_raster
	adc #$2a
	tax
    lda matrix_raster + 1
	adc #$00
	sta matrix_raster + 2
		
!:	    
    cpx VIC4_FNRASTERLSB
    bne !-
	lda VIC4_FN_RASTER_MSB_ADDR
	and #$07
	cmp matrix_raster + 2
    bne !-
	
	lda #VIC2_BLNK_MASK
	tsb VIC2_BLNK_ADDR

	
	// supersaw update frequencies loop
	ldx #$0b
!:
	lda frequencies_lo, x
	clc
	adc frequencies_delta_lo, x
	sta frequencies_lo, x
	
	lda frequencies_hi, x
	adc #$00
	sta frequencies_hi, x
	
	dex
	bpl !-


	clc
	lda matrix_raster
	adc #$01
	sta matrix_raster
	lda matrix_raster + 1
	adc #$00
	sta matrix_raster + 1
	
	dec drawin_index
	bpl drawin_loop
	
	// supersaw gate off SID voice control
	supersaw_ctl( $20 )
	
	lda #$2a
	sta drawin_delay
	
delay_loop2:
    ldx matrix_raster
    lda matrix_raster + 1
!:	    
    cpx VIC4_FNRASTERLSB
    bne !-
	lda VIC4_FN_RASTER_MSB_ADDR
	and #$07
	cmp matrix_raster + 2
    bne !-

	inx
		
!:	    
    cpx VIC4_FNRASTERLSB
    bne !-
	
	dec drawin_delay
	bpl delay_loop2
	
	// $d71c
	// enable audio dma
	lda #%10000000
	sta $d711
	
	// reset sample
	lda #$00
	sta $d720
	sta $d740
	
	// set sample start
	lda #<SAMPLE
	sta $d72a
	sta $d74a
	
	lda #>SAMPLE
	sta $d72b
	sta $d74b
	
	lda #$00
	sta $d72c
	sta $d74c
	
	// set sample end
	lda #<SAMPLE_END
	sta $d727
	sta $d747
	
	lda #>SAMPLE_END
	sta $d728
	sta $d748
	
	// the frequency calculation
	//
	// @gardners:
	// so 44K1 = ( 44100 x 2^24 ) / ( 40.5 x 10^6 )
	// 40.5 * 10 ^ 6 = 40,500,000 (of course)
	// = 18269
	// = $00475D, assuming I got it right.
	//
	// 8KHz = 8000 x 2 ^ 24 = 134,217,728,000
	// That / 40.5M = 3314.0179753086419753086419753086
	// = $000cf2
	//
	// but now, let us try 11KHz to see if that is the bug
	//
	// 11KHz = 11000 x 2 ^ 24 = 184,549,376,000
	// That / 40.5M = 4556.7747160493827160493827160494
	// = $0011cd
	
	lda #$cd
	sta $d724
	sta $d744
	
	lda #$11
	sta $d725
	sta $d745
	
	// set the volume
	lda #$41
	sta $d729
	sta $d749
	
	// and play
	lda #%10100010
	sta $d720
	sta $d740
	
	ldy #$b4
	
sample_wait:
    ldx matrix_raster
    cpx VIC4_FNRASTERLSB
    bne sample_wait
	lda VIC4_FN_RASTER_MSB_ADDR
	and #$07
	cmp matrix_raster + 1
	bne sample_wait

	inx
		
!:	
	lda matrix_raster + 1
    cpx VIC4_FNRASTERLSB
    bne !-
	
	dey
	bne sample_wait
	
	// bra inf
	lda $d720
	bit #%10000000
	bpl sample_wait
	
	lda #%00010000
	trb $d011

	run_dma_job(stub_to_screen_dma)
	
	jmp $0800
	
	// drawin should take 0.7 seconds - which conviently works out to round 
	// numbers in both NTSC and PAL.  In NTSC, that is 42 frames, and in PAL,
	// that is 35 frames.  However, the height of the logo is 21 pixels, which
	// means perfect timing is NTSC, so maybe instead we'll just make it slower
	// in PAL.
}

supersaw: {
	ldx #$0b
	
!:
	ldy frequencies_sid_reg_lo, x
	lda frequencies_lo, x
	sta (zp),y
	
	ldy frequencies_sid_reg_hi, x
	lda frequencies_hi, x
	sta (zp),y
	
	dex
	bpl !-
	
	rts
}

next_program: {
	run_dma_job(move_next_prog_dma)

	lda #%00010000
	tsb $d011
	
	jmp $2011
	
move_next_prog_dma:
	dma_copy_job(SAMPLE_END, $2001, NEXT_PROG_END - SAMPLE_END, false, false, false, false)
}
next_program_end:

.macro supersaw_ctl (val) {
	ldx #$0b
	
!:
	ldy frequencies_sid_reg_ctl, x
	lda #val
	sta (zp), y
	
	dex
	bpl !-
	
	lda #$2a
	sta drawin_delay
}

.segment Data
matrix_raster:
	.word $0000
	.byte $00
	
drawin_index:
	.byte $00
	
drawin_delay:
	.byte $00
	
zp_backup:
	.word $00
	
	// notes used in the drawin
	//
	// f#2		- $061f
	// to c-3	- $08a8	- step $00f
	//
	// f#3		- $0c3e
	// to c-4	- $1150	- step $01e
	//
	//
	// d#2		- $0526
	// to d#4	- $1496 - step $05e
	//
	// d#3		- $0a4b
	// to d#5	- $292d - step $0bc
	//
	//
	// c#0		- $0115
	// to c#4	- $1258 - step $069
	//
	// c#1		- $024b
	// to c#5	- $24af - step $0d1
frequencies_lo:
	.byte $3e,$4e,$20,$1f,$4b,$5b,$27,$26,$4b,$5b,$16,$15
	
frequencies_hi:
	.byte $0c,$0c,$0c,$06,$0a,$0a,$0a,$05,$02,$02,$02,$01
	
frequencies_delta_lo:
	.byte $1e,$20,$11,$0f,$bc,$be,$60,$5e,$d1,$d3,$6b,$69
//frequencies_delta_hi:
//	.byte $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00

	// SID start values
	// voice 1 freq lo - $00
	// voice 1 freq hi - $01
	// voice 1 ctrl	   - $04
	// voice 2 freq lo - $07
	// voice 2 freq hi - $08
	// voice 2 ctrl	   - $0b
	// voice 3 freq lo - $0e
	// voice 3 freq hi - $0f
	// voice 3 ctrl    - $12
frequencies_sid_reg_lo:
	.byte $00,$20,$40,$60,$07,$27,$47,$67,$0e,$2e,$4e,$6e
frequencies_sid_reg_hi:
	.byte $01,$21,$41,$61,$08,$28,$48,$68,$0f,$2f,$4f,$6f
frequencies_sid_reg_ctl:
	.byte $04,$24,$44,$64,$0b,$2b,$4b,$6b,$12,$32,$52,$72
	
load_color_palette_dma:
	dma_copy_job(CLUT, VIC3_PALRED, $300, false, false, false, true)
stub_to_screen_dma:
	dma_copy_job(next_program, $800, next_program_end - next_program, false, false, false, false)
	
fill_attribute_map:
	.byte $0a
	.byte $81, $ff
	.byte $85, $02
	.byte $00
	.byte $07
	.word line_length * line_count
	.word $08
	.byte $00
	.word $0000
	.byte $08
	.word $0000

	.byte $0a
	.byte $81, $ff
	.byte $85, $02
	.byte $00
	.byte $03
	.word line_length * line_count
	.word $0f
	.byte $00
	.word $0001
	.byte $08
	
clear_tile_map:
	.byte $0a
	.byte $85, $02
	.byte $00
	.byte $07
	.word line_length * line_count
	.word base_char & $ff
	.byte $00
	.word $0800
	.byte $00
	.word $0000

	.byte $0a
	.byte $85, $02
	.byte $00
	.byte $03
	.word line_length * line_count
	.word $0000
	.byte $00
	.word $0801
	.byte $00
	
CLUT:
	.const palette = LoadBinary("../logoassets/logo.clut")
	.fill palette.getSize(), palette.get(i)

.align $40
CHARSET:
	.const charset = LoadBinary("../logoassets/logo.chrs")
	.fill charset.getSize(), charset.get(i)

SAMPLE:
.if (sample_id == 0) {
	.const sample = LoadBinary("../logoassets/8x8_mega.raw")
	.fill sample.getSize(), sample.get(i)
}
.if (sample_id == 1) {
	.const sample = LoadBinary("../logoassets/voc_mega_c64_angry_boss.raw")
	.fill sample.getSize(), sample.get(i)
}
.if (sample_id == 2) {
	.const sample = LoadBinary("../logoassets/voc_mega_angry_boss.raw")
	.fill sample.getSize(), sample.get(i)
}
.if (sample_id == 3) {
	.const sample = LoadBinary("../logoassets/voc_mega_c64_8bitcycle.raw")
	.fill sample.getSize(), sample.get(i)
}
SAMPLE_END:

.const next_prog = LoadBinary("./agi.exo")
.fill next_prog.getSize() - 2, next_prog.get(i + 2)

NEXT_PROG_END:
