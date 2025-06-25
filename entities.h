//
// Created by frick on 2025-06-25.
//

#ifndef ENTITIES_H
#define ENTITIES_H

constexpr int BALL_TRACERS = 35;

enum CATS {
    TARGET = 0x0001,
    BALL = 0x0002,
    GROUND = 0x0004,
    BOX = 0x0008,
    PADDLE = 0x0016
};

typedef struct Entity {
    b2BodyId bodyId;
    b2BodyDef bodyDef;
    b2ShapeId shapeId;
    b2Vec2 extent;
    Texture texture;
} Entity;

typedef struct Ball {
    b2BodyId bodyId;
    b2BodyDef bodyDef;
    b2ShapeId shapeId;
    b2Vec2 spawn;
    float radius;
    Color color;
    b2Vec2 *ballHistory[BALL_TRACERS];
} Ball;

Ball CreateBall(b2Vec2 pos, float radius, Color color, b2WorldId worldId);

void DrawBall(Ball* ball);

void ResetBall(Ball* ball);

#endif //ENTITIES_H
