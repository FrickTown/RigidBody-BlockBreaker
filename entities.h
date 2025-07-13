//
// Created by frick on 2025-06-25.
//

#ifndef ENTITIES_H
#define ENTITIES_H

#include "box2d/types.h"
#include "raylib.h"
constexpr int BALL_TRACERS = 35;

enum CATS {
    TARGET = 0x0001,
    BALL = 0x0002,
    GROUND = 0x0004,
    BOX = 0x0008,
    PADDLE = 0x0016,
    BALLTHRU = 0x0020,
    RAY = 0x0040,
    DEATH = 0x0080,
};

typedef struct Entity {
    b2BodyId bodyId;
    b2BodyDef bodyDef;
    b2ShapeId shapeId;
    b2Vec2 extent;
    Texture texture;
    Color color;
} Entity;

Entity CreateSolid(b2Vec2 pos, b2Vec2 extent, Texture* texture, Color color, b2WorldId worldId);
Entity CreateDeathZone(b2Vec2 pos, b2Vec2 extent, Texture* texture, Color color, b2WorldId worldId);
Entity CreatePhysicsBox(b2Vec2 pos, b2Vec2 extent, Texture* texture, b2WorldId worldId);
void DrawEntity(const Entity* entity);

typedef struct Ball {
    b2Circle circle;
    b2BodyId bodyId;
    b2BodyDef bodyDef;
    b2ShapeId shapeId;
    b2Vec2 spawn;
    float radius;
    Color color;
    b2ShapeProxy proxy;
    Texture* texture;
    b2Vec2 ballHistory[BALL_TRACERS];
} Ball;

typedef struct BallRayCastContext
{
    b2ShapeId shapeId;
    b2ShapeId targetShapeId;
    b2Vec2 point;
    b2Vec2 normal;
    float fraction;
} BallRayCastContext;

b2CastResultFcn BallRayResultFcn;

Ball CreateBall(b2Vec2 pos, float radius, Texture* texture, Color color, b2WorldId worldId);
void DrawBall(Ball* ball);
void ResetBall(Ball* ball);

typedef struct Paddle {
    b2BodyId bodyId;
    b2BodyDef bodyDef;
    b2ShapeId shapeId;
    b2Vec2 extent;
    Texture* textures;
    Color color;
    bool touchingLimit;
    float lastTouchTime;
    float timeDelta;
    b2ShapeProxy proxy;
    int tilt;
} Paddle;

Paddle CreatePaddle(b2Vec2 spawn, float halfWidth, float halfHeight, Color color, b2WorldId worldId);
void CheckBallPaddleCollision(Ball* ball, Paddle* paddle, BallRayCastContext* context);
void UpdatePaddle(Paddle* paddle, b2Vec2 pos);
void DrawPaddle(Paddle* paddle);

typedef struct Target {
    b2BodyId bodyId;
    b2BodyDef bodyDef;
    b2ShapeId shapeId;
    b2Vec2 extent;
    b2ShapeProxy proxy;
    float scale;
    int state;
} Target;

Target CreateTarget(b2Vec2 spawn, float scale, Color color, b2WorldId worldId);
void UpdateTarget(Target* target);
void DrawTarget(Target* target);

#endif //ENTITIES_H
