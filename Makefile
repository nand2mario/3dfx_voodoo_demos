INCLUDES = -I. -Iglide -Ivoodoo
CC = g++
CFLAGS = $(shell sdl2-config --cflags) -g -std=c++17
LDFLAGS = $(shell sdl2-config --libs)
OBJS = glide.o voodoo_emu.o display.o texture.o
TARGETS = triangle cube teapot view3df texcube

all: $(TARGETS)

$(TARGETS): %: %.o $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

glide.o: glide/glide.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

texture.o: glide/texture.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

voodoo_emu.o: voodoo/voodoo_emu.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

display.o: voodoo/display.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f *.o $(TARGETS)

