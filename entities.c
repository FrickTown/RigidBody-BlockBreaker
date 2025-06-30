//
// Created by frick on 2025-06-25.
//

#include "raylib.h"
#include "box2d/box2d.h"
#include "entities.h"

#include <stdio.h>
#include <sys/types.h>

/*  #########################
 *         BALL ENTITY
 *  CONSTRUCTOR AND FUNCTIONS
 *  #########################
*/

Ball CreateBall(b2Vec2 pos, float radius, Color color, b2WorldId worldId) {
    b2Circle circle = {b2Vec2_zero, radius};
    b2BodyDef ballBodyDef = b2DefaultBodyDef();
    ballBodyDef.type = b2_dynamicBody;
    ballBodyDef.position = pos;
    ballBodyDef.isBullet = true;
    b2BodyId bodyId = b2CreateBody(worldId, &ballBodyDef);
    b2ShapeDef ballShapeDef = b2DefaultShapeDef();
    ballShapeDef.enableContactEvents = true;
    ballShapeDef.enableHitEvents = true;
    ballShapeDef.filter.categoryBits = BALL;
    ballShapeDef.filter.maskBits = PADDLE | GROUND | BOX | TARGET;
    b2ShapeId shapeId = b2CreateCircleShape(bodyId, &ballShapeDef, &circle);
    b2Shape_SetRestitution(shapeId, 0.95f);
    b2ShapeProxy ballProxy = b2MakeProxy(&circle.center, 1, radius);
    Ball ball = {
        bodyId,
        ballBodyDef,
        shapeId,
        pos,
        radius,
        color,
        ballProxy,
{[0 ... (BALL_TRACERS - 1)] = pos}
    };

    return ball;
}

void CheckBallPaddleCollision(Ball* ball, Paddle* paddle) {

    b2Sweep sweepA, sweepB;
    sweepA.c1 = b2Body_GetPosition(ball->bodyId);
    sweepA.q1 = b2NormalizeRot(b2Body_GetRotation(ball->bodyId));
    sweepA.c2 = b2MulAdd(b2Body_GetPosition(ball->bodyId), GetFrameTime(), b2Body_GetLinearVelocity(ball->bodyId)); // extrapolated pos
    b2Rot preRot = b2Body_GetRotation(ball->bodyId);
    b2Rot rotDelta = b2MakeRot(GetFrameTime() * b2Body_GetAngularVelocity(ball->bodyId));
    b2Rot finalRot = b2NormalizeRot((b2Rot){preRot.c + rotDelta.c, preRot.s + rotDelta.s});
    sweepA.q2 = finalRot;
    sweepA.localCenter = b2Body_GetLocalCenterOfMass(ball->bodyId);

    sweepB.c1 = b2Body_GetPosition(paddle->bodyId);
    sweepB.q1 = b2NormalizeRot(b2Body_GetRotation(paddle->bodyId));
    sweepB.c2 = b2MulAdd(b2Body_GetPosition(paddle->bodyId), GetFrameTime(), b2Body_GetLinearVelocity(paddle->bodyId)); // extrapolated pos
    b2Rot padPreRot = b2Body_GetRotation(paddle->bodyId);
    b2Rot padRotDelta = b2MakeRot(GetFrameTime() * b2Body_GetAngularVelocity(paddle->bodyId));
    b2Rot padFinalRot = b2NormalizeRot((b2Rot){padPreRot.c + padRotDelta.c, padPreRot.s + padRotDelta.s});
    sweepB.q2 = padFinalRot;
    sweepB.localCenter = b2Body_GetLocalCenterOfMass(paddle->bodyId);

    b2TOIInput input = { 0 };
    input.proxyA = ball->proxy;
    input.proxyB = paddle->proxy;
    input.sweepA = sweepA;
    input.sweepB = sweepB;
    input.maxFraction = 1.0f;

    b2TOIOutput output = b2TimeOfImpact(&input);
    if ((output.state == b2_toiStateHit || output.state == b2_toiStateOverlapped) && output.fraction < 1.0f) {
        b2Vec2 ballPos = b2Body_GetPosition(ball->bodyId);
        b2Rot ballRot = b2Body_GetRotation(ball->bodyId);
        b2Vec2 paddlePos = b2Body_GetPosition(paddle->bodyId);
        b2Body_SetTransform(ball->bodyId, (b2Vec2){ballPos.x, paddlePos.y - paddle->extent.y - ball->radius}, ballRot);
    }
}

void DrawBall(Ball* ball) {
    // Draw the ball
    b2Vec2 ballPos = b2Body_GetPosition(ball->bodyId);
    Color c = ball->color;
    c.a = 0;
    c.b = (c.b / BALL_TRACERS);
    uint8_t blueDelta = c.b;
    uint8_t alphaDelta = 255 / (BALL_TRACERS);
    // #############
    // Ball tracer logic
    // #############
    for (int k = BALL_TRACERS - 1; k > 0; k--){
        b2Vec2 ballHist = ball->ballHistory[k];
        float histRad = ball->radius * (1.0f / BALL_TRACERS) * ((float)BALL_TRACERS - (float)k);
        DrawCircle((int)ballHist.x, (int)ballHist.y, histRad, c);
        c.b += blueDelta;
        c.a += (alphaDelta);
        ball->ballHistory[k] = ball->ballHistory[k-1];
    }
    ball->ballHistory[0] = ballPos;
    DrawCircle(ballPos.x, ballPos.y, ball->radius, c);
    DrawCircleLines(ballPos.x, ballPos.y, ball->radius, WHITE);
}

void ResetBall(Ball* ball) {
    b2Body_SetLinearVelocity(ball->bodyId, b2Vec2_zero);
    b2Body_SetTransform(ball->bodyId,
        ball->spawn,
        b2Body_GetRotation(ball->bodyId));
}

/*  #########################
 *        PADDLE ENTITY
 *  CONSTRUCTOR AND FUNCTIONS
 *  #########################
*/

Paddle CreatePaddle(b2Vec2 spawn, float halfWidth, float halfHeight, Texture* texture, Color color, b2WorldId worldId) {
    Paddle paddle = { 0 };
    paddle.touchingLimit = false;
    paddle.lastTouchTime = 0;
    paddle.timeDelta = 0.0f;
    paddle.color = color;

    b2Vec2 extent;
    //b2Hull paddleHull = b2ComputeHull()

    b2Polygon polygon;
    if (texture != nullptr) {
        paddle.texture = *texture;
        extent = (b2Vec2){ 0.5f * texture->width, 0.5f * texture->height };
        polygon = b2MakeBox(extent.x, extent.y);
    }
    else {
        paddle.texture.id = -1;
        extent = (b2Vec2){ halfWidth, halfHeight };
        polygon = b2MakeBox(halfWidth, halfHeight);
    }
    paddle.extent = extent;
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = spawn;
    //bodyDef.isBullet = true;

    b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
    paddle.bodyId = bodyId;
    paddle.bodyDef = bodyDef;
    //b2MotionLocks motionLocks = { 0 };
    //motionLocks.angularZ = true;
    //b2Body_SetMotionLocks(bodyId, motionLocks);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.enableContactEvents = true;
    shapeDef.enableHitEvents = true;
    shapeDef.filter.categoryBits = PADDLE;
    b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
    paddle.shapeId = shapeId;

    b2ShapeProxy proxy = b2MakeProxy(polygon.vertices, polygon.count, 0);
    paddle.proxy = proxy;

    return paddle;
}

void UpdatePaddle(Paddle* paddle, b2Vec2 pos) {
    b2Transform target = {
        pos,
        //b2Body_GetRotation(paddle->bodyId)
        b2MakeRot(0)
    };
    b2Body_SetTargetTransform(paddle->bodyId, target, 0.1f);
    if (paddle->touchingLimit) {
        float nowTime = GetTime();
        paddle->timeDelta = nowTime - paddle->lastTouchTime;
    }
}

void DrawPaddle(Paddle* paddle) {
    if (!b2Body_IsEnabled(paddle->bodyId))
        return;

    b2Vec2 p = b2Body_GetWorldPoint(paddle->bodyId, (b2Vec2) { -paddle->extent.x, -paddle->extent.y });
    b2Rot rotation = b2Body_GetRotation(paddle->bodyId);
    float radians = b2Rot_GetAngle(rotation);
    Vector2 ps = {p.x, p.y};

    if (paddle->texture.id == -1) {
        Rectangle rect = {p.x, p.y, paddle->extent.x * 2, paddle->extent.y * 2};
        DrawRectanglePro(rect, (Vector2){0, 0}, RAD2DEG * radians, paddle->color);
    }
    else {
        DrawTextureEx(paddle->texture, ps, RAD2DEG * radians, 1.0f, WHITE);
    }
}

/*  #########################
 *       WORLD ENTITIES
 *        CONSTRUCTORS
 *  #########################
*/

/**
 * Creates a solid box given a point and a texture.
 * @param pos The spawn-point for the solid box
 * @param texture The texture to map to the box
 * @param worldId The world to spawn the box in
 * @return An Entity in the form of a static block
 */
Entity CreateSolid(b2Vec2 pos, b2Vec2 extent, Texture* texture, Color color, b2WorldId worldId) {
    Entity entity = { 0 };
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.position = pos;
    bodyDef.type = b2_staticBody;
    entity.bodyDef = bodyDef;
    b2Polygon groundPolygon = b2MakeBox(extent.x, extent.y);


    entity.bodyId = b2CreateBody(worldId, &bodyDef);
    entity.extent = extent;
    if (texture != nullptr) {
        entity.texture = *texture;
    }
    else {
        entity.texture.id = -1;
    }
    entity.color = color;
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.filter.categoryBits = GROUND;
    b2ShapeId shapeId = b2CreatePolygonShape(entity.bodyId, &shapeDef, &groundPolygon);
    entity.shapeId = shapeId;
    return entity;
}

Entity CreatePhysicsBox(b2Vec2 pos, b2Vec2 extent, Texture* texture, b2WorldId worldId) {
    b2Polygon boxPolygon = b2MakeBox(extent.x, extent.y);
    Entity box = { 0 };
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = pos;
    box.bodyId = b2CreateBody(worldId, &bodyDef);
    box.texture = *texture;
    box.extent = extent;
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.filter.categoryBits = BOX;
    box.shapeId = b2CreatePolygonShape(box.bodyId, &shapeDef, &boxPolygon);
    return box;
}

Entity CreateTarget(b2Vec2 pos, b2Vec2 extent, Texture* texture, b2WorldId worldId) {
    Entity entity = { 0 };
    b2Polygon targetPolygon = b2MakeBox(extent.x, extent.y);
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = pos;
    entity.bodyId = b2CreateBody(worldId, &bodyDef);
    entity.texture = *texture;
    entity.extent = extent;
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.enableContactEvents = true;
    shapeDef.enableHitEvents = true;
    shapeDef.filter.categoryBits = TARGET;
    entity.shapeId = b2CreatePolygonShape(entity.bodyId, &shapeDef, &targetPolygon);
    return entity;
}