//
// Created by frick on 2025-07-01.
//

#include "levels.h"
#include <contact.h>
#include <string.h>

typedef struct TargetData {
    float xPos;
    float yPos;
};

// TODO: Gör majoriteten av paddle-området till en killzone (optional för vissa banor)
// Eventuellt kan du göra det med en killzone som har en float height som kan specificeras vid loadlevel.
Level LoadLevel(int* levelData, Vector2 origin, Texture2D solidTexture, Texture2D targetTextures[], b2WorldId world) {
    Level level = { 0 };
    level.targetCount = 0;
    Target targets[128] = { 0 };
    level.entityCount = 0;
    Entity entities[128] = { 0 };


    for (int i = 0; i < LEVELSIZE; i++) {
        float yPos = origin.y + (i / LEVELWIDTH) * TILESIZE;
        float xPos = origin.x + TILESIZE + (i % LEVELWIDTH) * TILESIZE;
        b2Vec2 pos = {xPos, yPos};
        int* currentID = levelData + i;
        switch (*currentID) {
            case 0:
                break;
            case 1:
                entities[level.entityCount++] = CreateSolid(pos, (b2Vec2){solidTexture.width * 0.5f, solidTexture.height * 0.5f }, &solidTexture, WHITE, world);
                break;
            case 2:
                targets[level.targetCount++] = CreateTarget(pos, targetTextures, 1.0f, WHITE, world);
        }
    }
    memcpy(level.targets, targets, sizeof(Target) * level.targetCount);
    memcpy(level.entities, entities, sizeof(Entity) * level.entityCount);
    return level;
}

void DrawLevel(Level* level) {

    for (int i = 0; i < level->targetCount; i++) {
        DrawTarget(&(level->targets[i]));
    }
    for (int i = 0; i < level->entityCount; i++) {
        DrawEntity(&(level->entities[i]));
    }
}
