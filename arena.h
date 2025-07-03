//
// Created by frick on 2025-07-01.
//

#ifndef ARENA_H
#define ARENA_H
#include <raylib.h>

void DrawWalls(Texture* textures, Camera2D* camera);
void DrawCeiling(Texture textures[], Camera2D* camera);
void DrawBackground(Texture textures[], Camera2D* camera);
void DrawLimit(Texture textures[], Entity* limit);
void DrawDeathZone(Texture textures[], Entity* deathZone);
#endif //ARENA_H
