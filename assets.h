//
// Created by frick on 2025-07-02.
//

#ifndef ASSETS_H
#define ASSETS_H
#include <raylib.h>

Texture groundTexture, boxTexture, targetTexture, ballTexture;
char* paddleTexturePaths[][3];

struct GameTextures {
    Texture wallTextures[4];
    Texture ceilTextures[3];
    Texture backgroundTextures[3];
    Texture limitTextures[2];
    Texture targetTextures[2];
};

struct GameSounds {
    Sound paddleSounds[3];
    Sound targetSounds[4];
};

void LoadAssets(void);
void UnloadAssets(void);

#endif //ASSETS_H
