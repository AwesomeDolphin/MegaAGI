@echo off
cd logoassets
C:\Users\death\Documents\C65\JettMonsters\tools\tilemizer.py -b4 -m=3000 -o=logo -t="Mega65_Logo.png"
cd ..
java -jar D:\KickAssembler\KickAss65CE02-5.25.jar -bytedumpfile startup.bytes -showmem -odir . -vicesymbols -log build/startup.klog logosrc/main.asm
