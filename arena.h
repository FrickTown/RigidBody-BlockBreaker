//
// Created by frick on 2025-07-01.
//

#ifndef ARENA_H
#define ARENA_H
#include "entities.h"
#include <raylib.h>

void DrawWalls(Camera2D* camera);
void DrawCeiling(Camera2D* camera);
void DrawBackground(Camera2D* camera);
void DrawLimit(Entity* limit);
void DrawDeathZone(Entity* deathZone);
#endif //ARENA_H
