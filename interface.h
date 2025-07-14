//
// Created by frick on 2025-07-02.
//

#ifndef INTERFACE_H
#define INTERFACE_H
#include <raylib.h>
#include <stdint.h>
#include "assets.h"

enum GameStates {
    MAIN_MENU,
    GAME_ACTIVE,
    GAME_WIN,
    GAME_OVER
};

typedef struct GameState {
    int paused;
    uint64_t state;
    int score;
} GameState;

typedef struct Container {
    Rectangle bounds;
    Color color;

} Container;

typedef struct Button {
    Container* parent;
    Font* font;
    Rectangle bounds;
    Texture texture;
    char text[16];
    Texture textAsImg;
    Vector2 textPosition;
    void (*callback)(int*);
    int* callbackArg;
} Button;

typedef struct PauseMenu {
    GameState* gameState;
    Rectangle bounds;
    Container background;
    Container foreground;
    int buttonCount;
    Button buttons[];
} PauseMenu;

PauseMenu* CreatePauseMenu(GameState* gameState, Rectangle bounds);
void DrawPauseMenu(PauseMenu* pauseMenu);
void PauseMenuHandleClick(PauseMenu* pauseMenu, Vector2 mousePos);

void DrawHUD(GameState* gameState, Rectangle screenBounds);

#endif //INTERFACE_H
