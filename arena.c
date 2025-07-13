//
// Created by frick on 2025-07-01.
//

#include <raylib.h>
#include <raymath.h>
#include <box2d/box2d.h>
#include <box2d/math_functions.h>

#include "entities.h"
#include "interface.h"

extern Texture TextureLibrary[TextureEnumSize];
extern Sound SoundLibrary[SoundEnumSize];

void DrawWalls(Camera2D* camera) {
    int width = 1920, height = 1080;
    Vector2 screenOrigin = GetScreenToWorld2D((Vector2){0, 0}, *camera);
    Vector2 screenMax = GetScreenToWorld2D((Vector2){width, height}, *camera);
    int blockSize = TextureLibrary[t_wall_left].width;
    int wallHeight = (screenMax.y - screenOrigin.y / blockSize);
    float xPosL = screenOrigin.x;
    float xPosR = screenMax.x - blockSize;
    for (int i = 1; i < wallHeight; i++) {
        float yPos = screenOrigin.y + i * blockSize;
        Vector2 posL = {xPosL, yPos};
        Vector2 posR = {xPosR, yPos};
        if (i == 1)
        {
            DrawTextureEx(
                TextureLibrary[t_wall_left_top], 
                posL, 
                0, 
                1.0f, 
                WHITE);
            DrawTextureEx(
                TextureLibrary[t_wall_right_top], 
                posR, 
                0, 
                1.0f, 
                WHITE);
        }
        else {
            DrawTextureEx(
                TextureLibrary[t_wall_left], 
                posL, 
                0, 
                1.0f, 
                WHITE);
            DrawTextureEx(
                TextureLibrary[t_wall_right], 
                posR, 
                0, 
                1.0f, 
                WHITE);
        }
    }
}

void DrawCeiling(Camera2D* camera) {
    int width = 1920, height = 1080;
    Vector2 screenOrigin = GetScreenToWorld2D((Vector2){0, 0}, *camera);
    Vector2 screenMax = GetScreenToWorld2D((Vector2){width, height}, *camera);
    int blockSize = TextureLibrary[t_ceiling_left].width;
    int ceilWidth = (screenMax.x - screenOrigin.x / blockSize);
    float yPos = screenOrigin.y;
    for (int i = 0; i < ceilWidth; i++) {
        float xPos = screenOrigin.x + i * blockSize;
        DrawTextureEx(TextureLibrary[t_ceiling_mid], (Vector2){xPos, yPos}, 0, 1.0f, WHITE);
    }
}

void DrawBackground(Camera2D* camera) {
    int width = 1920, height = 1080;
    Vector2 screenOrigin = GetScreenToWorld2D((Vector2){0, 0}, *camera);
    Vector2 screenMax = GetScreenToWorld2D((Vector2){width, height}, *camera);
    Vector2 screenCenter = GetScreenToWorld2D((Vector2){width/2.0f, height/2.0f}, *camera);

    int scaleFactor = 2;
    int blockSize = TextureLibrary[t_bg_ground].width * scaleFactor;
    int backgroundWidth = (screenMax.x - screenCenter.x / blockSize);
    Color tint = {125, 150, 175, 255};

    for (int j = 0; j < backgroundWidth; j++) {
        DrawTextureEx(
            TextureLibrary[t_bg_ground], 
            (Vector2){
                screenOrigin.x + blockSize * j, 
                screenCenter.y + 0.5 * blockSize
            }, 
            0, 
            scaleFactor, tint);
        DrawTextureEx(
            TextureLibrary[t_bg_view], 
            (Vector2){
                screenOrigin.x + blockSize * j, 
                screenCenter.y - 0.5 * blockSize
            }, 
            0, 
            scaleFactor, tint);
        DrawTextureEx(
            TextureLibrary[t_bg_sky], 
            (Vector2){
                screenOrigin.x + blockSize * j, 
                screenCenter.y - 1.5 * blockSize
            }, 
            0, 
            scaleFactor, tint);
    }

}

void DrawLimit(Entity* limit) {
    float texWidth = TextureLibrary[t_limit].width;
    b2Vec2 limitPos = b2Body_GetPosition(limit->bodyId);
    float yPos = limitPos.y - TextureLibrary[t_limit].height / 2.0f;
    float x0 = limitPos.x - limit->extent.x + texWidth;
    float x1 = limitPos.x + limit->extent.x - texWidth * 2.0f;
    DrawTextureEx(TextureLibrary[t_limit_end], (Vector2){x0 + texWidth, yPos + texWidth}, 180, 1.0f, WHITE);
    DrawTextureEx(TextureLibrary[t_limit_end], (Vector2){x1, yPos}, 0, 1.0f, WHITE);
    float limitWidth = (limit->extent.x * 2 - texWidth * 2) / texWidth;
    for (int i = 1; i < limitWidth - 1; i++) {
        DrawTextureEx(TextureLibrary[t_limit], (Vector2){x0 + i * texWidth, yPos}, 0, 1.0f, WHITE);
    }
}

void DrawDeathZone(Entity* deathZone) {

}