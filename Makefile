CC	= lcc -Wa-l -Wl-m -Wl-j

all:	blappy_firds.gb

obj/%.o:	src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile and link single file in one pass
blappy_firds.gb:	obj/blappy_firds.o obj/tile_data.o obj/pipes_map.o
	$(CC) -Wm-yC -o $@ $^

clean:
	rm -f *.o *.lst *.map *.gb *~ *.rel *.cdb *.ihx *.lnk *.sym *.asm *.noi

