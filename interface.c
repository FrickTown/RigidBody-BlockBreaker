//
// Created by frick on 2025-07-02.
//

#include "interface.h"
#include "raylib.h"
#include "sys/types.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Texture TextureLibrary[TextureEnumSize];
extern Sound SoundLibrary[SoundEnumSize];
extern Font menuFont;
/**
 * Transform a Rectangle's values, from percentage of parent's position and size, to world coordinates.
 * @param parent
 * @param rect
 * @return A Rectangle with world coordinates (in pixels)
 */
Rectangle RelToAbs(Rectangle parent, Rectangle rect) {
    Vector2 units = {parent.width / 100.0f, parent.height / 100.0f};
    Rectangle out = {
        parent.x + rect.x * units.x, parent.y + rect.y * units.y,
        rect.width * units.x, rect.height  * units.y
    };
    return out;
}

/**
 * Transform from percentage of parent's position and size to world coordinates.
 * Uses only the y and width and height of the child, which will be centered along the x-axis inside its parent.
 * @param parent The parent's world coordinates and size in pixel values.
 * @param childDimensions The width and height in percentages of its parent's dimensions.
 * @return A Rectangle with world coordinates (in pixels)
 */
Rectangle RelToAbsCentered(Rectangle parent, Rectangle childRectangle) {
    Vector2 units = {parent.width / 100.0f, parent.height / 100.0f};
    float width = childRectangle.width * units.x;
    float height = childRectangle.height * units.y;
    float xPos = (parent.x + parent.width / 2) - width / 2;
    Rectangle out = {xPos, childRectangle.y, width, height};
    return out;
}

/**
 * Create a blank, solid-color square, taking up a proportion of its parent's bounds.
 * @param parentBounds The parent's world coordinates and size in pixel values.
 * @param containerBounds The dimensions and positions of this container, in percentages of its parent.
 * @param color The color of this solid square
 * @return A Container struct
 */
Container CreateContainer(Rectangle parentBounds, Rectangle containerBounds, Color color) {
    Rectangle relativeRect = RelToAbs(parentBounds, containerBounds);
    Container container = {
        .bounds = relativeRect,
        .color = color
    };
    return container;
}

Button CreateButton(Container* parent, bool centered, Rectangle buttonBounds, Font* font, const char text[16], Texture* texture, void* callback, int* callbackArg) {
    Rectangle relativeRect;
    if (centered)
        relativeRect = RelToAbsCentered(parent->bounds, buttonBounds);
    else
        relativeRect = RelToAbs(parent->bounds, buttonBounds);

    Image imageText = ImageTextEx(*font, text, 75, 0.1f, WHITE);
    Texture img = LoadTextureFromImage(imageText);
    Vector2 textPos = {
        (relativeRect.x + (relativeRect.width / 2)) - img.width / 2,
        (relativeRect.y + (relativeRect.height / 2)) - img.height / 2};

    Texture modTex = *texture;
    modTex.width = relativeRect.width;
    modTex.height = relativeRect.height;
    Button button = {
        .parent = parent,
        .bounds = relativeRect,
        .texture = modTex,
        .textAsImg = img,
        .textPosition = textPos,
        .callback = callback,
        .callbackArg = callbackArg
    };
    memcpy(button.text, text, 16);

    return button;
}

void DrawButton(Button* button) {
    DrawTextureEx(button->texture, (Vector2){ button->bounds.x, button->bounds.y }, 0, 1.0f, WHITE);
    DrawTextureEx(button->textAsImg, button->textPosition, 0, 1.0f, WHITE);
}

void TogglePause(int* pause) {
    *pause = !*pause;
}

void CallCallback(void (*func)(int*),int* arg) {
    (*func)(arg);
}

PauseMenu* CreatePauseMenu(GameState* gameState, Rectangle bounds) {

    int buttonCount = 1;
    PauseMenu* menu = malloc(sizeof(PauseMenu) + buttonCount * sizeof(Button));
    menu->buttonCount = buttonCount;

    Rectangle backgroundDimensions = {100, 100, 100, 100};
    Container background = CreateContainer(bounds, backgroundDimensions, RED);
    Rectangle foregroundDimensions = {5, 5, 90, 90};
    Container foreground = CreateContainer(bounds, foregroundDimensions, LIGHTGRAY);

    Rectangle buttonDimensions = {0, 0, 50, 10};

    Button resume = CreateButton(&foreground, true, buttonDimensions, &menuFont, "Resume", &TextureLibrary[t_ui_button_color], TogglePause, &(gameState->paused));
    menu->buttons[0] = resume;

    menu->gameState = gameState;
    menu->bounds = bounds;
    menu->background = background;
    menu->foreground = foreground;

    return menu;
}

void DrawPauseMenu(PauseMenu* pauseMenu) {
    DrawRectanglePro(pauseMenu->bounds, (Vector2){ 0, 0 }, 0, RED);
    DrawRectanglePro(pauseMenu->foreground.bounds, (Vector2){ 0, 0 }, 0, GRAY);
    for (int i = 0; i < pauseMenu->buttonCount; i++) {
        DrawButton(&pauseMenu->buttons[i]);
    }
}

void PauseMenuHandleClick(PauseMenu* menu, Vector2 mousePos) {
    printf("Mouse: (%.3f, %.3f) | Box: (%.3f -> %.3f; %.3f -> %.3f)\n",
    mousePos.x, mousePos.y,
    menu->bounds.x, menu->bounds.y, menu->bounds.x + menu->bounds.width, menu->bounds.y + menu->bounds.height);
    if (CheckCollisionPointRec(mousePos, menu->bounds)) {
        for (int i = 0; i < menu->buttonCount; i++) {
            Button button = menu->buttons[i];
            printf("    Mouse: (%.3f, %.3f) | Box: (%.3f -> %.3f; %.3f -> %.3f)\n",
            mousePos.x, mousePos.y,
            button.bounds.x, button.bounds.y, button.bounds.x + button.bounds.width, button.bounds.y + button.bounds.height);
            if (CheckCollisionPointRec(mousePos, button.bounds)) {
                CallCallback(button.callback, button.callbackArg);
            }
        }
    }
}

int * toArray(int number)
{
    int n;
    if(number == 0)
        n = 1;
    else
        n = log10(number) + 1;
    int i;
    int *numberArray = calloc(n, sizeof(int));
    for ( i = 0; i < n; ++i, number /= 10 )
    {
        numberArray[i] = number % 10;
    }
    return numberArray;
}


void DrawHUD(GameState *gameState, Rectangle screenBounds) {
    Vector2 scorePadding = {20, 20};
    DrawTexture(TextureLibrary[t_ui_coin], screenBounds.x + scorePadding.x, screenBounds.y + scorePadding.y, WHITE);


    int* asArray = toArray(gameState->score);
    int numberLength = 1;
    if(gameState->score > 0)
        numberLength = log10(gameState->score) + 1;
    int j = numberLength;
    for (int i = 0; i < numberLength; i++) {
        DrawTexture(TextureLibrary[t_ui_number_0 + asArray[i]], screenBounds.x + scorePadding.x + 100 * j, screenBounds.y + scorePadding.y, WHITE);
        j--;
    }
    free(asArray);
}