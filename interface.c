//
// Created by frick on 2025-07-02.
//

#include "interface.h"

#include <string.h>

Rectangle RelToAbs(Rectangle parent, Rectangle rect) {
    Vector2 units = {parent.width / 100.0f, parent.height / 100.0f};
    Rectangle out = {
        parent.x + rect.x * units.x, parent.y + rect.y * units.y,
        rect.width * units.x, rect.height  * units.y
    };
    return out;
}

Container CreateContainer(Rectangle parentBounds, Rectangle containerBounds, Color color) {
    Rectangle relativeRect = RelToAbs(parentBounds, containerBounds);
    Container container = {
        .bounds = relativeRect,
        .color = color
    };
    return container;
}

Button CreateButton(Rectangle parentBounds, Rectangle containerBounds, Texture* texture, void* callback) {
    Rectangle relativeRect = RelToAbs(parentBounds, containerBounds);
    Button button = {
        .bounds = relativeRect,
        .texture = texture,
        .callback = callback,
    };
    return button;
}

void TogglePause(bool* pause) {
    *pause = !*pause;
}

PauseMenu CreatePauseMenu(Rectangle bounds, Texture textures[2]) {
    Rectangle backgroundDimensions = {100, 100, 100, 100};
    Rectangle foregroundDimensions = {5, 5, 90, 90};
    Rectangle buttonDimensions = {30, 40,50, 20};
    PauseMenu menu = {
        .bounds = bounds,
        .background = CreateContainer(bounds, backgroundDimensions, RED),
        .foreground = CreateContainer(bounds, foregroundDimensions, LIGHTGRAY),
        .resume = CreateButton(bounds, buttonDimensions, &textures[0], TogglePause)
    };
    memcpy(menu.textures, textures, 2 * sizeof(Texture));
    return menu;
}

void DrawPauseMenu(PauseMenu* pauseMenu) {
    DrawRectanglePro(pauseMenu->bounds, (Vector2){ 0, 0 }, 0, RED);
    DrawRectanglePro(pauseMenu->foreground.bounds, (Vector2){ 0, 0 }, 0, GRAY);
    DrawTextureEx(pauseMenu->textures[0], (Vector2){ pauseMenu->resume.bounds.x, pauseMenu->resume.bounds.y }, 0, 1.0f, WHITE);
}

