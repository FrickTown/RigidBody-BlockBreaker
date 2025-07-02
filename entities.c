//
// Created by frick on 2025-06-25.
//

#include "raylib.h"
#include "box2d/box2d.h"
#include "entities.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

/*  #########################
 *         BALL ENTITY
 *  CONSTRUCTOR AND FUNCTIONS
 *  #########################
*/

Ball CreateBall(b2Vec2 pos, float radius, Texture* texture, Color color, b2WorldId worldId) {
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
        circle,
        bodyId,
        ballBodyDef,
        shapeId,
        pos,
        radius,
        color,
        ballProxy,
        texture,
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
        if (ballPos.y + ball->radius < paddlePos.y - paddle->extent.y
            && ballPos.x + ball->radius > paddlePos.x - paddle->extent.x
            && ballPos.x - ball->radius < paddlePos.x + paddle->extent.x) {
            b2Body_SetTransform(ball->bodyId, (b2Vec2){ballPos.x, paddlePos.y - paddle->extent.y - ball->circle.radius}, ballRot);
        }
        //else {
         //   b2Body_SetTransform(ball->bodyId, (b2Vec2){ballPos.x - paddle->extent.x - ball->circle.radius, paddlePos.y }, ballRot);
        //}
    }
}

void DrawBall(Ball* ball) {

    if (ball->circle.radius != ball->texture->width) {
        ball->texture->width = ball->circle.radius*2;
        ball->texture->height = ball->circle.radius*2;
    }
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
        float histRad = ball->circle.radius * (1.0f / BALL_TRACERS) * ((float)BALL_TRACERS - (float)k);
        DrawCircle((int)ballHist.x, (int)ballHist.y, histRad, c);
        c.b += blueDelta;
        c.a += (alphaDelta);
        ball->ballHistory[k] = ball->ballHistory[k-1];
    }
    ball->ballHistory[0] = ballPos;

    // Actual ball drawing
    // Drawing colored ball
    DrawCircle(ballPos.x, ballPos.y, ball->circle.radius, c);
    DrawCircleLines(ballPos.x, ballPos.y, ball->circle.radius, WHITE);

    // Drawing Texture
    b2Rot rotation = b2Body_GetRotation(ball->bodyId);
    float radians = b2Rot_GetAngle(rotation);
    b2Vec2 world = b2Body_GetWorldCenterOfMass(ball->bodyId);
    // Raylib rotates textures around the positional point it is given, meaning the top-left corner is the anchor.
    // Therefore, we need to compensate by drawing a vector from the ball's center to its "top-left" "corner",
    // rotating it by the rotation of the b2Body, and adding it to the position of the ball's center to
    // get the appropriate coordinates for Raylib to draw.
    b2Vec2 adj = b2RotateVector(rotation, (b2Vec2){-ball->radius, -ball->radius});
    DrawTextureEx(*(ball->texture), (Vector2){world.x + adj.x, world.y + adj.y}, RAD2DEG * radians, 1.0f, WHITE);
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

Paddle CreatePaddle(b2Vec2 spawn, float halfWidth, float halfHeight, char* texturePaths[][3], int numTex, float scale, Color color, b2WorldId worldId) {
    Paddle paddle = { 0 };
    paddle.touchingLimit = false;
    paddle.lastTouchTime = 0;
    paddle.timeDelta = 0.0f;
    paddle.color = color;

    b2Vec2 extent;
    b2Polygon polygon;
    if (texturePaths != nullptr) {
        Texture textures[3] = { 0 };
        for (int i = 0; i < numTex; i++) {

            char fullPath[32];
            char* path = *(*texturePaths + i);
            snprintf(fullPath, sizeof(fullPath), "%s/%s", "assets", path);
            Image img = LoadImage(fullPath);
            ImageResize(&img, img.width * scale, img.height * scale);
            textures[i] = LoadTextureFromImage(img);
            extent = (b2Vec2){ 0.5f * textures[i].width * 3, 0.5f * textures[i].height };
            polygon = b2MakeBox(extent.x, extent.y);
        }
        memcpy(paddle.textures, textures, numTex * sizeof(Texture));
    }
    else {
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
        b2MakeRot(PI/6 * paddle->tilt)
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
    b2Vec2 pos = b2Body_GetPosition(paddle->bodyId);
    b2Rot rotation = b2Body_GetRotation(paddle->bodyId);
    float radians = b2Rot_GetAngle(rotation);

    b2Vec2 toLeft = {-paddle->extent.x, -paddle->extent.y};
    b2Vec2 leftAdj = b2RotateVector(rotation, toLeft);
    b2Vec2 toMid = {-paddle->extent.y, -paddle->extent.y};
    b2Vec2 midAdj = b2RotateVector(rotation, toMid);
    b2Vec2 toRight = {paddle->extent.x / 3.0f, -paddle->extent.y};
    b2Vec2 rightAdj = b2RotateVector(rotation, toRight);

    if (paddle->textures == nullptr) {
        Rectangle rect = {p.x, p.y, paddle->extent.x * 2, paddle->extent.y * 2};
        DrawRectanglePro(rect, (Vector2){0, 0}, RAD2DEG * radians, paddle->color);
    }
    else {
        Texture tex1 = paddle->textures[0];
        DrawTextureEx(tex1, (Vector2){pos.x + leftAdj.x, pos.y + leftAdj.y}, RAD2DEG * radians, 1.0f, WHITE);
        Texture tex2 = paddle->textures[1];
        DrawTextureEx(tex2, (Vector2){pos.x + midAdj.x, pos.y + midAdj.y}, RAD2DEG * radians, 1.0f, WHITE);
        Texture tex3 = paddle->textures[2];
        DrawTextureEx(tex3, (Vector2){pos.x + rightAdj.x, pos.y + rightAdj.y}, RAD2DEG * radians, 1.0f, WHITE);
    }
}

/*  #########################
 *       TARGET ENTITY
 *  CONSTRUCTOR AND FUNCTIONS
 *  #########################
*/

Target CreateTarget(b2Vec2 spawn, Texture textures[], float scale, Color color, b2WorldId worldId) {
    b2Vec2 extent = {textures[0].width * 0.5f * scale, textures[0].height * 0.5f * scale};
    b2Polygon polygon = b2MakeBox(extent.x, extent.y);

    b2BodyDef targetBodyDef = b2DefaultBodyDef();
    targetBodyDef.type = b2_staticBody;
    targetBodyDef.position = spawn;
    targetBodyDef.gravityScale = 0;


    b2BodyId targetBodyId = b2CreateBody(worldId, &targetBodyDef);
    b2ShapeDef targetShapeDef = b2DefaultShapeDef();
    targetShapeDef.enableContactEvents = true;
    targetShapeDef.enableHitEvents = true;
    targetShapeDef.filter.categoryBits = TARGET;
    targetShapeDef.filter.maskBits = PADDLE | GROUND | BOX | BALL | BALLTHRU | TARGET;
    targetShapeDef.density = 0.1f;

    b2ShapeId shapeId = b2CreatePolygonShape(targetBodyId, &targetShapeDef, &polygon);
    b2Shape_SetRestitution(shapeId, 0.9);
    b2ShapeProxy proxy = b2MakeProxy(polygon.vertices, polygon.count, 0);

    Target target = {
        targetBodyId,
        targetBodyDef,
        shapeId,
        extent,
        proxy,
        scale,
        0,
        {0}
    };
    memcpy(target.textures, textures, 2 * sizeof(Texture));

    return target;
}

void UpdateTarget(Target* target) {

}

void DrawTarget(Target* target) {
    if (!b2Body_IsEnabled(target->bodyId))
        return;

    b2Vec2 p = b2Body_GetWorldPoint(target->bodyId, (b2Vec2) { -target->extent.x, -target->extent.y });
    b2Rot rotation = b2Body_GetRotation(target->bodyId);
    float radians = b2Rot_GetAngle(rotation);
    b2Vec2 adj = b2RotateVector(rotation, (b2Vec2){-target->extent.x, -target->extent.y});
    //Vector2 ps = {p.x + adj.x, p.y + adj.y};
    Vector2 ps = {p.x, p.y};
    DrawTextureEx(target->textures[target->state], ps, RAD2DEG * radians, target->scale, WHITE);
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
    b2Polygon groundPolygon = b2MakeBox(extent.x, extent.y);
    Entity entity = { 0 };
    entity.color = color;
    entity.extent = extent;

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.position = pos;
    bodyDef.type = b2_staticBody;
    entity.bodyDef = bodyDef;
    entity.bodyId = b2CreateBody(worldId, &bodyDef);

    if (texture != nullptr) {
        entity.texture = *texture;
    }
    else {
        entity.texture.id = -1;
    }
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.filter.categoryBits = GROUND;
    b2ShapeId shapeId = b2CreatePolygonShape(entity.bodyId, &shapeDef, &groundPolygon);
    b2Shape_SetFriction(shapeId, 0.0f);
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

void DrawEntity(const Entity* entity)
{
    if (!b2Body_IsEnabled(entity->bodyId))
        return;
    // The boxes were created centered on the bodies, but raylib draws textures starting at the top left corner.
    // b2Body_GetWorldPoint gets the top left corner of the box accounting for rotation.
    b2Vec2 p = b2Body_GetWorldPoint(entity->bodyId, (b2Vec2) { -entity->extent.x, -entity->extent.y });
    b2Rot rotation = b2Body_GetRotation(entity->bodyId);
    float radians = b2Rot_GetAngle(rotation);

    Vector2 ps = {p.x, p.y};
    if (entity->texture.id == -1) {
        Rectangle rect = {p.x, p.y, entity->extent.x * 2, entity->extent.y * 2};
        DrawRectanglePro(rect, (Vector2){0, 0}, RAD2DEG * radians, entity->color);
    }
    else {
        DrawTextureEx(entity->texture, ps, RAD2DEG * radians, 1.0f, WHITE);
    }

}