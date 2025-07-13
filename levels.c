//
// Created by frick on 2025-07-01.
//

#include "levels.h"
#include <contact.h>
#include <string.h>
#include "assets.h"

extern Texture TextureLibrary[TextureEnumSize];
extern Sound SoundLibrary[SoundEnumSize];

// TODO: Gör majoriteten av paddle-området till en killzone (optional för vissa banor)
// Eventuellt kan du göra det med en killzone som har en float height som kan specificeras vid loadlevel.
Level LoadLevel(int* levelData, Vector2 origin, b2WorldId world) {
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
                entities[level.entityCount++] = CreateSolid(
                    pos, 
                    (b2Vec2){
                        TextureLibrary[t_block_idle].width * 0.5f, 
                        TextureLibrary[t_block_idle].height * 0.5f 
                    }, 
                    &TextureLibrary[t_block_idle], 
                    WHITE, 
                    world);
                break;
            case 2:
                targets[level.targetCount++] = CreateTarget(
                    pos, 
                    1.0f, 
                    WHITE, 
                    world);
            case 3:
                level.ballSpawn = pos;
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
