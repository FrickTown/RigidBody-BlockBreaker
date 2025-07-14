PATH_TO_RAYLIB=/home/frick/Dev/C-Stuff/raylib/build/raylib
PATH_TO_BOX2D=/home/frick/Dev/C-Stuff/box2d-3.1.1
BOX2D_SRC=$(PATH_TO_BOX2D)/src/
BOX2D_INCLUDE=$(PATH_TO_BOX2D)/include/

all:
	emcc -o web_build/game.html main.c entities.c arena.c levels.c interface.c assets.c --preload-file assets -std=c23 -Os -Wall $(PATH_TO_RAYLIB)/libraylib.a -I. -I$(BOX2D_SRC) -I$(BOX2D_INCLUDE) -I$(PATH_TO_RAYLIB)/include/ $(PATH_TO_BOX2D)/build/src/CMakeFiles/box2d.dir/*.o -L. -L$(PATH_TO_RAYLIB)/libraylib.a -L$(PATH_TO_BOX2D)/build/src/libbox2dd.a -s EXPORTED_RUNTIME_METHODS=ccall -s USE_GLFW=3 --shell-file ./html_templates/minshell.html -DPLATFORM_WEB -lembind

clean:
	rm ./web_build/*