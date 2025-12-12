              .rtmodel cstartup,"megaagi"

              .rtmodel version, "1"
              .rtmodel core, "*"

              ;; External declarations

            .section data_init_table
            .section stack
            .section cstack
            .section heap
            .section runtime
            .section runtime_startup
            .section code
            .section disk_driver
            .section disk_driver_autostart

              .extern main
              .extern _Zp, _Vsp, _Vfp

              .pubweak __program_root_section, __program_start

              .section programStart, root ; to be at address 0x801 for Commodore 64
__program_root_section:
              .word   nextLine
              .word   10            ; line number
              .byte   0x9e, " 8206", 0 ; SYS 806
nextLine:     .word   0             ; end of program

              .section startup, root, noreorder
__program_start:

              .section startup, root, noreorder
              lda     #.byte0(.sectionEnd cstack)
              sta     zp:_Vsp
              lda     #.byte1(.sectionEnd cstack)
              sta     zp:_Vsp+1

              sei
              
              lda #1
              trb 0xd703
              lda #2
              tsb 0xd703
              sta 0xd707
              .byte 0x00
              .byte 0x04
              .word (.sectionSize disk_driver_autostart)
              .word (.sectionStart disk_driver_autostart)
              .byte 0x00
              .word 0x8000
              .byte 0x00
              .byte 0x00
              .byte 0x00

              .byte 0x81
              .byte 0x80
              .byte 0x00
              .byte 0x04
              .word 0x1e00
              .word 0x0200
              .byte 0x00
              .word 0x0200
              .byte 0x00
              .byte 0x00
              .byte 0x00

              .byte 0x81
              .byte 0x80
              .byte 0x00
              .byte 0x04
              .word (.sectionSize runtime_startup)
              .word (.sectionStart runtime_startup)
              .byte 0x00
              .word 0x2200
              .byte 0x00
              .byte 0x00
              .byte 0x00

              .byte 0x81
              .byte 0x00
              .byte 0x00
              .byte 0x00
              .word (.sectionSize runtime_startup)
              .word (.sectionStart runtime_startup)
              .byte 0x00
              .word (.sectionStart code)
              .byte 0x00
              .byte 0x00
              .byte 0x00

              .section startup, noroot, noreorder

;;; Initialize data sections if needed.
              .section startup, noroot, noreorder
              .pubweak __data_initialization_needed
              .extern __initialize_sections
__data_initialization_needed:
              lda     #.byte0 (.sectionStart data_init_table)
              sta     zp:_Zp
              lda     #.byte1 (.sectionStart data_init_table)
              sta     zp:_Zp+1
              lda     #.byte0 (.sectionEnd data_init_table)
              sta     zp:_Zp+2
              lda     #.byte1 (.sectionEnd data_init_table)
              sta     zp:_Zp+3
              jsr     __initialize_sections

              .section startup, root, noreorder
              lda     #2
              sta     0xd641
              clv
              lda     #0            ; argc = 0
              sta     0xd030
              sta     zp:_Zp
              sta     zp:_Zp+1
              jsr     main
              lda     #0
              jsr     0xff32

;;; ***************************************************************************
;;;
;;; Keep track of the initial stack pointer so that it can be restores to make
;;; a return back on exit().
;;;
;;; ***************************************************************************

              .section zdata, bss
              .pubweak _InitialStack
_InitialStack:
              .space  1