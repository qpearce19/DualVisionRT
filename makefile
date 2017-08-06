all:
	gcc -o DualVisionRT main.c -lavutil -lavformat -lavcodec -lswscale -lz -lm -lswresample `sdl-config --cflags --libs`

clean:
	rm -rf DualVisionRT
