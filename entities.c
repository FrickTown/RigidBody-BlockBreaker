//
// Created by frick on 2025-06-25.
//

#include "raylib.h"
#include "box2d/box2d.h"
#include "entities.h"

#include <sys/types.h>

Ball createBall(b2Vec2 pos, float radius, Color color, b2WorldId worldId) {
    b2Circle circle = {b2Vec2_zero, radius};
    b2BodyDef ballBodyDef = b2DefaultBodyDef();
    ballBodyDef.type = b2_dynamicBody;
    ballBodyDef.position = pos;
    b2BodyId bodyId = b2CreateBody(worldId, &ballBodyDef);
    b2ShapeDef ballShapeDef = b2DefaultShapeDef();
    ballShapeDef.enableContactEvents = true;
    ballShapeDef.enableHitEvents = true;
    ballShapeDef.filter.categoryBits = BALL;
    b2ShapeId shapeId = b2CreateCircleShape(bodyId, &ballShapeDef, &circle);
    b2Vec2 ballHist[BALL_TRACERS] = {[0 ... (BALL_TRACERS - 1)] = pos};

    Ball ball = {
        bodyId,
        ballBodyDef,
        shapeId,
        pos,
        radius,
        color,
        ballHist
    };

    return ball;
}

void DrawBall(Ball* ball) {
    // Draw the ball
    b2Vec2 ballPos = b2Body_GetPosition(ball->bodyId);
    DrawCircle(ballPos.x, ballPos.y, ball->radius, ball->color);
    Color c = ball->color;
    c.a = 0;
    c.b = 255;
    u_char reduction = 255 / (BALL_TRACERS);
    // #############
    // Ball tracer logic
    // #############
    for (int k = BALL_TRACERS - 1; k > 0; k--){
        b2Vec2 *ballHist = ball->ballHistory[k];
        float histRad = ball->radius * (1.0f / BALL_TRACERS) * ((float)BALL_TRACERS - (float)k);
        DrawCircle((int)ballHist->x, (int)ballHist->y, histRad, c);
        c.b -= reduction;
        c.a += (reduction);
        ball->ballHistory[k] = ball->ballHistory[k-1];
    }
    ball->ballHistory[0] = &ballPos;
}

void ResetBall(Ball* ball) {
    b2Body_SetLinearVelocity(ball->bodyId, b2Vec2_zero);
    b2Body_SetTransform(ball->bodyId,
        ball->spawn,
        b2Body_GetRotation(ball->bodyId));
}