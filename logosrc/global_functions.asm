.macro basic_stub(target) {
	.byte $16,$20			// End of command marker (first byte after the 00 terminator)
	.byte $0a,$00			// 10
	.byte $fe,$02,$30,$3a	// BANK 0:
	.byte $9e				// SYS
	.text toIntString(target)
	.byte $3a, $8f			// :REM
	.fill 20, $14
	.text "BAS"
	.byte $00
	.byte $00,$00			// End of basic terminators
}

.macro is_real_hw() {
	lda UARTMISC_REALHW_ADDR
	bit #UARTMISC_REALHW_MASK
}

.macro run_dma_job (job_pointer) {
	lda #(job_pointer >> 16)
	sta $d702
	sta $d704
	lda #>job_pointer
	sta $d701
	lda #<job_pointer
	sta $d705
}

.macro dma_fill_job (source_byte, destination, length, chain) {
	.byte $0a
	.byte $00
	.if (chain) {
		.byte $07
	} else {
		.byte $03
	}
	
	.word length
	.word source_byte
	.byte $00
	.word destination & $FFFF
	.byte ((destination >> 16) & $0F)
	
	.if (chain) {
		.word $0000
	}
}

.macro dma_copy_job (source, destination, length, chain, backwards, io_src, io_dest) {
	.byte $0a
	.byte $00 //No more options
	.if(chain) {
		.byte $04 //Copy and chain
	} else {
		.byte $00 //Copy and last request
	}	
	
	.var back_byte = 0
	.if(backwards) {
		.eval back_byte = $40
		.eval source = source + length - 1
		.eval destination = destination + length - 1
	}
	
	.var io_src_byte = 0
	.if (io_src) {
		.eval io_src_byte = $80
	}
	
	.var io_dest_byte = 0
	.if (io_dest) {
		.eval io_dest_byte = $80
	}
	
	.word length //Size of Copy

	.word source & $ffff
	.byte (source >> 16) + back_byte + io_src_byte

	.word destination & $ffff
	.byte ((destination >> 16) & $0f)  + back_byte + io_dest_byte
	.if(chain) {
		.word $0000
	}
}

.macro dma_attic_copy_job(source, destination, length, chain, backwards) {
	.byte $0a
	.byte $80, (source & %1111111100000000000000000000) >> 20
	.byte $81, (destination & %1111111100000000000000000000) >> 20
	.byte $00 //No more options
	.if(chain) {
		.byte $04 //Copy and chain
	} else {
		.byte $00 //Copy and last request
	}	
	
	.var back_byte = 0
	.if(backwards) {
		.eval back_byte = $40
		.eval source = source + length - 1
		.eval destination = destination + length - 1
	}
	.word length //Size of Copy

	.word source & $ffff
	.byte ((source & $ff0000) >> 16) + back_byte

	.word destination & $ffff
	.byte ((destination >> 16) & $0f)  + back_byte
	.if(chain) {
		.word $0000
	}
}
