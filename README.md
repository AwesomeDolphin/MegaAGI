This version can play several screens of KQ1 (AGI 2 version from GOG). I'm not even testing other AGI games, and KQ1 was originally an AGI 1 game. It is likely not as demanding as some of the later AGI 2 releases.

To build the D81, you will need to do the following:
	c1541 -format "agi,a1" d81 agi.d81
	c1541 -attach agi.d81 -write agi.prg agi.c65
	c1541 -attach agi.d81 -write nographics.raw nographics,s
	c1541 -attach agi.d81 -write volumes/LOGDIR logdir,s
	c1541 -attach agi.d81 -write volumes/PICDIR picdir,s
	c1541 -attach agi.d81 -write volumes/SNDDIR snddir,s
	c1541 -attach agi.d81 -write volumes/VIEWDIR viewdir,s
	c1541 -attach agi.d81 -write volumes/VOL.0 vol.0,s
	c1541 -attach agi.d81 -write volumes/VOL.1 vol.1,s
	c1541 -attach agi.d81 -write volumes/VOL.2 vol.2,s
	c1541 -attach agi.d81 -write volumes/WORDS.TOK words.tok,s

You will need to get the game data files from GOG.

Eventually, you will get a crash. The entire inventory system is not implemented, so trying to pick something up will get a fault. Plus, several important logic statements are not implemented and will crash, such as random.

