//
// Created by frick on 2025-07-02.
//

#include "assets.h"

#include <raylib.h>

Texture groundTexture, boxTexture, targetTexture, ballTexture;
char* paddleTexturePaths[][3] = {
    "paddleL.png", "paddleMid.png", "paddleR.png"
};

Texture wallTextures[4] = { 0 };
Texture ceilTextures[3] = { 0 };
Texture backgroundTextures[3] = { 0 };
Texture limitTextures[2] = { 0 };
Texture targetTextures[2] = { 0 };

struct GameSounds gameSounds;

void LoadAssets(void) {
    groundTexture = LoadTexture("assets/block_idle.png");
    boxTexture = LoadTexture("assets/box.png");
    targetTexture = LoadTexture("assets/ground.png");
    ballTexture = LoadTexture("assets/ball.png");
    targetTexture.height = targetTexture.height * 0.5f;
    targetTexture.width = targetTexture.width * 0.5f;

    gameSounds = {
        {
            LoadSound("assets/paddle1.ogg");
            LoadSound("assets/paddle2.ogg");
            LoadSound("assets/paddle3.ogg");

        }
    };

    targetSounds[0] = LoadSound("assets/target1.ogg");
    targetSounds[1] = LoadSound("assets/target2.ogg");
    targetSounds[2] = LoadSound("assets/target3.ogg");
    targetSounds[3] = LoadSound("assets/target4.ogg");

    wallTextures[0] = LoadTexture("assets/wallL.png");
    wallTextures[1] = LoadTexture("assets/wallLTop.png");
    wallTextures[2] = LoadTexture("assets/wallR.png");
    wallTextures[3] = LoadTexture("assets/wallRTop.png");

    ceilTextures[0] = LoadTexture("assets/ceilL.png");
    ceilTextures[1] = LoadTexture("assets/ceilMid.png");
    ceilTextures[2] = LoadTexture("assets/ceilR.png");

    backgroundTextures[0] = LoadTexture("assets/bg_ground.png");
    backgroundTextures[1] = LoadTexture("assets/bg_view.png");
    backgroundTextures[2] = LoadTexture("assets/bg_sky.png");

    limitTextures[0] = LoadTexture("assets/limit.png");
    limitTextures[1] = LoadTexture("assets/limit_end.png");

    targetTextures[0] = LoadTexture("assets/target_rest.png");
    targetTextures[1] = LoadTexture("assets/target_awake.png");
}

void UnloadAssets(void) {

}