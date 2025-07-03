//
// Created by frick on 2025-07-02.
//

#ifndef INTERFACE_H
#define INTERFACE_H
#include <raylib.h>

typedef struct Container {
    Rectangle bounds;
    Color color;
} Container;

typedef struct Button {
    Rectangle bounds;
    Texture* texture;
    void* callback;
}Button;


typedef struct PauseMenu {
    Rectangle bounds;
    Container background;
    Container foreground;
    Button resume;
    Texture textures[2];
}PauseMenu;

PauseMenu CreatePauseMenu(Rectangle bounds, Texture textures[]);
void DrawPauseMenu(PauseMenu* pauseMenu);
#endif //INTERFACE_H
