GBDK_HOME = ../../../
LCC	= /opt/gbdk/bin/lcc
#LCCFLAGS += -Wl-j -Wm-yoA -Wm-ya4 -autobank -Wb-ext=.rel -Wb-v -Wl-yt0x1B -mgbz80:gb -Wm-yC -debug
LCCFLAGS += -Wl-j -mgbz80:gb -Wm-yC -debug -Wf--std-sdcc2x -Wf--nogcse -Wf--nolabelopt -Wf--noinvariant -Wf--noloopreverse -Wf--no-peep -Wf--no-peep-return -Wf--allow-unsafe-read -Wf--nostdlibcall
 

all:	l2d.gb

obj/%.o:	src/%.c Makefile
	$(LCC) $(LCCFLAGS) -c -o $@ $<

# Compile and link single file in one pass
l2d.gb:	obj/main.o obj/level0.o obj/level1.o obj/splash_tiles.o obj/util.o obj/splash_tilemap.o obj/splash_sprites.o obj/splash_title_map.o obj/splash_title_tiles.o obj/level_select_tiles.o obj/level_select_arrows_tilemap.o
	$(LCC) $(LCCFLAGS) -o $@ $^

clean:
	rm -f *.o *.lst *.map *.gb *~ *.rel *.cdb *.ihx *.lnk *.sym *.asm *.noi

CFLAGS2 = -Wa-l -Wl-m -Wl-j -debug -Wm-yc