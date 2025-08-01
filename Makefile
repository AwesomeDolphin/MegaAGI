VPATH = src
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

# Common source files
ASM_SRCS = simplefile.s irq.s
C_SRCS = main.c ncm.c pic.c volume.c sound.c view.c engine.c interrupt.c memmanage.c sprite.c logic.c parser.c init.c gamesave.c dialog.c
C1541 = c1541
INC = -I./include

# Object files
OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)
OBJS_DEBUG = $(ASM_SRCS:%.s=obj/%-debug.o) $(C_SRCS:%.c=obj/%-debug.o)

# GIT repository information
ifneq "$(wildcard $(ROOT_DIR)/.git )" "" #check if local repo exist
#Create variable GIT_MSG with:
#1) git sha and additionally dirty flag
GIT_MSG := sha:$(shell git --git-dir=$(ROOT_DIR)/.git --work-tree=$(ROOT_DIR) --no-pager describe --tags --always --dirty)
else
GIT_MSG := Can't find git repo
endif
#add user define macro (-D) to gcc
CFLAGS += -DGIT_MSG=\"$(strip "$(GIT_MSG)")\"

obj/%.o: %.s
	as6502 --target=mega65 --list-file=$(@:%.o=%.clst) -o $@ $<

obj/%.o: %.c
	cc6502 --target=mega65 -Wall -Werror -O2 $(INC) --list-file=$(@:%.o=%.clst) $(CFLAGS) -o $@ $<

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

mega65-agi.d81: logosrc\agi.lgo
	$(C1541) -format "mega65,agi" d81 mega65-agi.d81
	$(C1541) -attach mega65-agi.d81 -write logosrc\agi.lgo agi.c65
	$(C1541) -attach mega65-agi.d81 -write COPYING copying,s
	$(C1541) -attach mega65-agi.d81 -write inits.raw inits,s
	$(C1541) -attach mega65-agi.d81 -write midmem.raw midmem,s
	$(C1541) -attach mega65-agi.d81 -write himem.raw himem,s
	$(C1541) -attach mega65-agi.d81 -write ultmem.raw ultmem,s
	
agi.d81: logosrc\agi.lgo
	$(C1541) -format "agi,a1" d81 agi.d81
	$(C1541) -attach agi.d81 -write logosrc\agi.lgo agi.c65
	$(C1541) -attach agi.d81 -write COPYING copying,s
	$(C1541) -attach agi.d81 -write inits.raw inits,s
	$(C1541) -attach agi.d81 -write midmem.raw midmem,s
	$(C1541) -attach agi.d81 -write himem.raw himem,s
	$(C1541) -attach agi.d81 -write ultmem.raw ultmem,s
	$(C1541) -attach agi.d81 -write kq1/LOGDIR logdir,s
	$(C1541) -attach agi.d81 -write kq1/PICDIR picdir,s
	$(C1541) -attach agi.d81 -write kq1/SNDDIR snddir,s
	$(C1541) -attach agi.d81 -write kq1/VIEWDIR viewdir,s
	$(C1541) -attach agi.d81 -write kq1/VOL.0 vol.0,s
	$(C1541) -attach agi.d81 -write kq1/VOL.1 vol.1,s
	$(C1541) -attach agi.d81 -write kq1/VOL.2 vol.2,s
	$(C1541) -attach agi.d81 -write kq1/WORDS.TOK words.tok,s
	$(C1541) -attach agi.d81 -write kq1/OBJECT object,s

agisystem: mega65-agi.d81 agi.d81

clean:
	-rm $(OBJS) $(OBJS:%.o=%.clst) $(OBJS_DEBUG) $(OBJS_DEBUG:%.o=%.clst)
	-rm agi.elf agi.prg agi-mega65.cmap agi-debug.cmap
