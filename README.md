This version can play KQ1 (AGI 2 version from GOG), at least to the point of
beating the game by some path. Not all paths to success may be fully debugged.
I'm not even testing other AGI games, and KQ1 was originally an AGI 1 game.
It is likely not as demanding as some of the later AGI 2 releases.

To build the D81 from scratch, you will need to do the following:
```
	c1541 -format "agi,a1" d81 agi.d81
	c1541 -attach agi.d81 -write agi.prg agi.c65
	c1541 -attach agi.d81 -write COPYING copying,s
	c1541 -attach agi.d81 -write inits.raw inits,s
	c1541 -attach agi.d81 -write midmem.raw midmem,s
	c1541 -attach agi.d81 -write himem.raw himem,s
	c1541 -attach agi.d81 -write ultmem.raw ultmem,s
	c1541 -attach agi.d81 -write volumes/LOGDIR logdir,s
	c1541 -attach agi.d81 -write volumes/PICDIR picdir,s
	c1541 -attach agi.d81 -write volumes/SNDDIR snddir,s
	c1541 -attach agi.d81 -write volumes/VIEWDIR viewdir,s
	c1541 -attach agi.d81 -write volumes/VOL.0 vol.0,s
	c1541 -attach agi.d81 -write volumes/VOL.1 vol.1,s
	c1541 -attach agi.d81 -write volumes/VOL.2 vol.2,s
	c1541 -attach agi.d81 -write volumes/WORDS.TOK words.tok,s
	c1541 -attach agi.d81 -write volumes/OBJECT object,s
```

To use builddisk.py:
```
	Place the mega65-agi.d81 file and the builddisk.py script in the same path.
	Ensure that the d64 python module is installed: pip install d64
	Run builddisk.py, providing the path to the files you downloaded from GOG.
	If you want the disk to autoboot, provide the switch '-a'. Otherwise, SHIFT-RUN/STOP
	will start the game.
```

You will need to get the game data files from GOG.

