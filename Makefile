VPATH = src

# Common source files
ASM_SRCS = simplefile.s irq.s
C_SRCS = main.c ncm.c pic.c volume.c sound.c view.c engine.c interrupt.c memmanage.c sprite.c logic.c parser.c init.c gamesave.c dialog.c
C1541 = c1541
INC = -I./include

# Object files
OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)
OBJS_DEBUG = $(ASM_SRCS:%.s=obj/%-debug.o) $(C_SRCS:%.c=obj/%-debug.o)

obj/%.o: %.s
	as6502 --target=mega65 --list-file=$(@:%.o=%.clst) -o $@ $<

obj/%.o: %.c
	cc6502 --target=mega65 -Wall -Werror -O2 $(INC) --list-file=$(@:%.o=%.clst) -o $@ $<

obj/%-debug.o: %.s
	as6502 --target=mega65 --debug --list-file=$(@:%.o=%.clst) -o $@ $<

obj/%-debug.o: %.c
	cc6502 --target=mega65 --debug --list-file=$(@:%.o=%.clst) -o $@ $<

agi.prg:  mega65-agi.scm $(OBJS)
	ln6502 --target=mega65 -o $@ $^ --raw-multiple-memories --output-format=prg --list-file=agi-mega65.cmap

agi.elf: $(OBJS_DEBUG)
	ln6502 --target=mega65 --debug -o $@ $^ --list-file=agi-debug.cmap --semi-hosted

agi.exo: agi.prg
	exomizer sfx basic -n -t 65 -o agi.exo agi.prg

logosrc\agi.lgo: agi.exo
	build-logo.cmd

agi.d81: logosrc\agi.lgo
	$(C1541) -format "agi,a1" d81 agi.d81
	$(C1541) -attach agi.d81 -write logosrc\agi.lgo agi.c65
	$(C1541) -attach agi.d81 -write COPYING copying,s
	$(C1541) -attach agi.d81 -write inits.raw inits,s
	$(C1541) -attach agi.d81 -write midmem.raw midmem,s
	$(C1541) -attach agi.d81 -write himem.raw himem,s
	$(C1541) -attach agi.d81 -write ultmem.raw ultmem,s
	$(C1541) -attach agi.d81 -write volumes/LOGDIR logdir,s
	$(C1541) -attach agi.d81 -write volumes/PICDIR picdir,s
	$(C1541) -attach agi.d81 -write volumes/SNDDIR snddir,s
	$(C1541) -attach agi.d81 -write volumes/VIEWDIR viewdir,s
	$(C1541) -attach agi.d81 -write volumes/VOL.0 vol.0,s
	$(C1541) -attach agi.d81 -write volumes/VOL.1 vol.1,s
	$(C1541) -attach agi.d81 -write volumes/VOL.2 vol.2,s
	$(C1541) -attach agi.d81 -write volumes/WORDS.TOK words.tok,s
	$(C1541) -attach agi.d81 -write volumes/OBJECT object,s

clean:
	-rm $(OBJS) $(OBJS:%.o=%.clst) $(OBJS_DEBUG) $(OBJS_DEBUG:%.o=%.clst)
	-rm agi.elf agi.prg agi-mega65.cmap agi-debug.cmap
