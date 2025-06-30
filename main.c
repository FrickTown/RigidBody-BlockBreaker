#include "raylib.h"
#include "box2d/box2d.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "rlgl.h"
#include "contact.h"
#include "entities.h"
#include "raymath.h"
#if defined(PLATFORM_WEB)
	#include <emscripten/emscripten.h>
#endif


// This shows how to use Box2D v3 with raylib.
// It also show how to use Box2D with pixel units.

float InvLerp(float a, float b, float t) {
	return (t - a) / (b - a);
}

Vector2 ScreenOrigin(Camera2D camera) {
	return GetScreenToWorld2D((Vector2){0, 0}, camera);
}

typedef struct MyRayCastContext
{
	b2ShapeId shapeId;
	b2ShapeId targetShapeId;
	b2Vec2 point;
	b2Vec2 normal;
	float fraction;
} MyRayCastContext;

b2CastResultFcn PaddleBallCast;
float PaddleBallCast(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context ) {
	MyRayCastContext* myContext = context;
	myContext->shapeId = shapeId;
	myContext->point = point;
	myContext->normal = normal;
	myContext->fraction = fraction;
	if (shapeId.index1 == myContext->targetShapeId.index1) {
		return fraction;
	}
	return -1;
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

void UpdateDrawFrame(void);
void InitWorld(void);

#define GROUND_COUNT 14
#define BOX_COUNT 10
#define TARGET_COUNT 10

Texture groundTexture, boxTexture, targetTexture;
bool pause = true;

int width = 1920, height = 1080;
int main(void)
{
	InitWindow(width, height, "box2d-raylib");
	InitAudioDevice();
	InitWorld();
	#if defined(PLATFORM_WEB)
		emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
	#else
		SetTargetFPS(60);   // Set our game to run at 60 frames-per-second
	//--------------------------------------------------------------------------------------

		// Main game loop
		while (!WindowShouldClose())    // Detect window close button or ESC key
		{
			UpdateDrawFrame();
		}
	#endif

	UnloadTexture(groundTexture);
	UnloadTexture(boxTexture);

	CloseAudioDevice();
	CloseWindow();

	return 0;
}

constexpr bool DEBUG = false;

float lengthUnitsPerMeter;
b2WorldId worldId;
Camera2D camera = { 0 };
Vector2 screenOrigin, screenMax;

Texture groundTexture, boxTexture, targetTexture;
Sound paddleSound;

Entity targetEntities[TARGET_COUNT] = { 0 }, boxEntities[BOX_COUNT] = { 0 };
Entity leftWall, rightWall, ceiling, limit;
Ball ballEntity;
Paddle paddle;


Vector2 mousePosition;
Vector2 lastMousePosition;
Vector2 mouseDelta;

bool holdingEntity = false;
Entity* lastHeldEntity;

void InitWorld(void) {
	camera.target = (Vector2){ width/2.0f, height/2.0f };
	camera.offset = (Vector2){ width/2.0f, height/2.0f };
	camera.rotation = 0.0f;
	camera.zoom = 0.5f;

	// 128 pixels per meter is a appropriate for this scene. The boxes are 128 pixels wide.
	lengthUnitsPerMeter = 128.0f;
	b2SetLengthUnitsPerMeter(lengthUnitsPerMeter);

	b2WorldDef worldDef = b2DefaultWorldDef();

	// Realistic gravity is achieved by multiplying gravity by the length unit.
	worldDef.gravity.y = 9.8f * lengthUnitsPerMeter;
	worldDef.enableSleep = false;
	worldDef.hitEventThreshold = 2.0f * lengthUnitsPerMeter;
	worldId = b2CreateWorld(&worldDef);

	groundTexture = LoadTexture("assets/ground.png");
	boxTexture = LoadTexture("assets/box.png");
	targetTexture = LoadTexture("assets/ground.png");
	targetTexture.height = targetTexture.height * 0.5f;
	targetTexture.width = targetTexture.width * 0.5f;


	paddleSound = LoadSound("assets/paddle.ogg");

	// Top-left and bottom-right vectors
	screenOrigin = ScreenOrigin(camera);
	screenMax = GetScreenToWorld2D((Vector2){width, height}, camera);

	b2Vec2 staticsExtent = { 0.5f * groundTexture.width, 0.5f * groundTexture.height };

	// Defining the solid walls
	b2Vec2 wallExtent = {groundTexture.width, (screenMax.y - screenOrigin.y) / 2};
	b2Vec2 lPos = {screenOrigin.x + staticsExtent.x, screenOrigin.y + (screenMax.y - screenOrigin.y) / 2};
	leftWall = CreateSolid(lPos, wallExtent, nullptr, PURPLE, worldId);
	b2Vec2 rPos = {screenMax.x - staticsExtent.x, screenOrigin.y + (screenMax.y - screenOrigin.y) / 2};
	rightWall = CreateSolid(rPos, wallExtent, nullptr, PURPLE, worldId);

	// Defining the solid ceiling
	b2Vec2 ceilPos = {screenOrigin.x + ((screenMax.x - screenOrigin.x) / 2), screenOrigin.y + groundTexture.height * 0.5f};
	b2Vec2 ceilExtent = {((screenMax.x - screenOrigin.x) / 2), groundTexture.height * 0.5f };
	ceiling = CreateSolid(ceilPos, ceilExtent, nullptr, WHITE, worldId);

	b2Vec2 boxExtent = { 0.5f * boxTexture.width, 0.5f * boxTexture.height };
	for (int i = 0; i < BOX_COUNT; ++i)
	{
		float y = height - boxExtent.y - 100.0f - (2.5f * i + 2.0f) * boxExtent.y - 20.0f;
		float x = 0.5f * width + (3.0f * i - 3.0f) * boxExtent.x;
		Entity box = CreatePhysicsBox((b2Vec2){x, y}, boxExtent, &boxTexture, worldId);
		boxEntities[i] = box;
	}

	b2Vec2 targetExtent = { 0.5f * targetTexture.width, 0.5f * targetTexture.height };
	for (int i = 0; i < TARGET_COUNT; ++i) {
		float y = (0) - targetExtent.y + (targetExtent.y * 2 * (i % 2));
		float x = ((width / 2.0f) - (targetExtent.x * 2) * TARGET_COUNT / 2.0f) + i * (targetExtent.x * 2.0f);
		targetEntities[i] = CreateTarget((b2Vec2){x, y}, targetExtent, &targetTexture, worldId);
	}

	ballEntity = CreateBall(
		(b2Vec2){width / 2.0f, height / 5.0f},
		0.2f * lengthUnitsPerMeter,
		PURPLE,
		worldId
		);

	paddle = CreatePaddle(
		(b2Vec2){width / 2.0f, height * 0.95f}, 1.0f * lengthUnitsPerMeter, 0.2f * lengthUnitsPerMeter,
		nullptr, BLUE, worldId
		);

	limit = CreateSolid(
		(b2Vec2){width / 2.0f, height * 0.9f},
		(b2Vec2){width, 0.05f * lengthUnitsPerMeter},
		nullptr, BLACK, worldId
		);

	b2Filter filt = { 0 };
	filt.categoryBits = BALLTHRU;
	filt.maskBits = PADDLE;
	b2Shape_SetFilter(limit.shapeId, filt);

	mousePosition = GetMousePosition();
	lastMousePosition = GetMousePosition();
	mouseDelta = GetMouseDelta();

	holdingEntity = false;
}

void UpdateDrawFrame(void) {
	if (IsKeyPressed(KEY_P))
	{
		pause = !pause;
	}

	// Reset boxes and ball
	if (IsKeyPressed(KEY_R)) {
		for (int i = 0; i < BOX_COUNT; ++i) {
			Entity* entity = boxEntities + i;
			b2Body_SetLinearVelocity(entity->bodyId, b2Vec2_zero);
			b2Body_SetTransform(entity->bodyId,
				(b2Vec2){
					128 + (float)(width-64)/BOX_COUNT * (float)(i/(1+i%2)),
					i%2 * 128
				},
				b2Body_GetRotation(entity->bodyId));
		}
	}

	if (IsKeyPressed(KEY_T)) {
		ResetBall(&ballEntity);
	}

	if (IsKeyDown(KEY_D)) {
		camera.rotation += 5;
	}

	if (IsKeyDown(KEY_A)) {
		camera.rotation -= 5;
	}

	// ######################
	// Game and physics logic
	// #######################

	mousePosition = GetMousePosition();
	mouseDelta = GetMouseDelta();

	Vector2 mouseInWorld = GetScreenToWorld2D(mousePosition, camera);
	b2Vec2 mVec = {mouseInWorld.x, mouseInWorld.y};

	b2Vec2 VectorsToDraw[10] = {0};
	int VecIndex = 0;

	Color color = RED;
	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
		// Mouse left allows picking up a box
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !holdingEntity) {
			// Loop through all the boxes
			for (int i = 0; i < BOX_COUNT; ++i) {
				Entity* entity = boxEntities + i;
				b2Vec2 pob = b2Body_GetLocalPoint(entity->bodyId, mVec);

				// If the mouse coord as a local point (origin is center on box) is within the bounds of the box
				if (fabsf(pob.x) <= entity->extent.x && fabsf(pob.y) <= entity->extent.y) {
					// Debug color
					color = BLUE;
					// Set variables required for holding a box (logic is applied later)
					holdingEntity = true;
					lastHeldEntity = entity;
					break;
				}
			}
		}
		// Mouse right creates a radial force-field that pushes the boxes away based on the distance to the mouse
		else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
			for (int i = 0; i < BOX_COUNT; ++i) {
				Entity* entity = boxEntities + i;
				b2Vec2 pob = b2Body_GetLocalPoint(entity->bodyId, mVec);
				//b2Vec2 distVec = {pob.x * -1, pob.y * -1};
				b2Vec2 entityPos = b2Body_GetWorldCenterOfMass(entity->bodyId);
				b2Vec2 distVec = {entityPos.x - mVec.x, entityPos.y - mVec.y};
				float distMag = (float)sqrt(pow(distVec.x, 2) + pow(distVec.y, 2));
				float maxDistance = 256;
				if (distMag > maxDistance) {
					continue;
				}
				b2Vec2 distNorm = b2Normalize(distVec);
				b2Vec2 maxForce = {10, 10};
				b2Vec2 maxVec = b2Mul(distNorm, maxForce);
				b2Vec2 str = b2Lerp((b2Vec2){0, 0}, maxVec, distMag);
				VectorsToDraw[VecIndex++] = str;
				b2Body_SetLinearVelocity(entity->bodyId, str);
				//b2Body_ApplyForce(entity->bodyId, str, mVec, true);
				if (fabsf(pob.x) <= entity->extent.x && fabsf(pob.y) <= entity->extent.y) {
					color = BLUE;
					if (DEBUG) printf("(%.2f, %.2f)\n", pob.x, pob.y);
				}
			}
		}
	}

	// Logic for ensuring a held box follows the mouse cursor
	if (holdingEntity && lastHeldEntity != NULL) {
		//b2Body_SetTransform(lastHeldEntity->bodyId, mVec, b2Body_GetRotation(lastHeldEntity->bodyId));
		//b2Body_SetLinearVelocity(lastHeldEntity->bodyId, (b2Vec2){0, 0});
		b2Transform target = {
			mVec,
			b2Body_GetRotation(lastHeldEntity->bodyId)
		};
		b2Body_SetTargetTransform(lastHeldEntity->bodyId, target, 0.1f);
	}

	// Logic for releasing a held box
	if (IsMouseButtonUp(MOUSE_BUTTON_LEFT) && holdingEntity && lastHeldEntity != NULL) {
		holdingEntity = false;
		if (DEBUG) printf("(%.2f, %.2f)\n", mouseDelta.x * 1280.0f, mouseDelta.y * 1280.0f);
		//b2Vec2 releaseVelocity = {mouseDelta.x * 32.0f, mouseDelta.y * 32.0f};
		//releaseVelocity = (b2MakeRot(degreesToRadians(camera.rotation)), releaseVelocity);
		//b2Body_SetLinearVelocity(lastHeldEntity->bodyId, releaseVelocity);
	}

	// Prevent high-velocity shots by having cursor above limit
	b2Vec2 paddleTarget = mVec;
	if (paddleTarget.y < height / 2.0f) {
		paddleTarget.y = height / 2.0f;
	}
	b2Vec2 paddlePos = b2Body_GetPosition(paddle.bodyId);
	float limitBottom = b2Body_GetPosition(limit.bodyId).y + limit.extent.y;
	if (paddleTarget.y <= paddlePos.y && paddle.touchingLimit && paddle.timeDelta > 0.1f) {
		paddleTarget.y = limitBottom + paddle.extent.y;
	}

	UpdatePaddle(&paddle, paddleTarget);
	CheckBallPaddleCollision(&ballEntity, &paddle);

	if (pause == false)
	{
		float deltaTime = GetFrameTime();
		b2World_Step(worldId, deltaTime, 16);
	}

	// #################
	// Target hit logic
	// #################
	b2ContactEvents contactEvents = b2World_GetContactEvents(worldId);
	if (contactEvents.beginCount > 0 || contactEvents.endCount > 0 || contactEvents.hitCount > 0 )
		if (DEBUG) printf("Contact begin: %d, Contact end: %d, Hits: %d\n", contactEvents.beginCount, contactEvents.endCount, contactEvents.hitCount);

	for (int i = 0; i < contactEvents.hitCount; ++i)
	{
		b2ContactHitEvent* hitEvent = contactEvents.hitEvents + i;
		if (DEBUG) printf("ShapeIDA: %llu, ShapeIDB: %llu\n", b2Shape_GetFilter(hitEvent->shapeIdA).categoryBits, b2Shape_GetFilter(hitEvent->shapeIdB).categoryBits);
		uint64_t shapeACategory = b2Shape_GetFilter(hitEvent->shapeIdA).categoryBits;
		uint64_t shapeBCategory = b2Shape_GetFilter(hitEvent->shapeIdB).categoryBits;
		// If BALL (2) collides with TARGET (1)
		if (abs((int)(shapeACategory - shapeBCategory)) == 1) {
			b2ShapeId shape;
			if (shapeACategory == BALL)
				shape = hitEvent->shapeIdB;
			else
				shape = hitEvent->shapeIdA;
			b2BodyId targetBody = b2Shape_GetBody(shape);
			b2Body_Disable(targetBody);
		}
		// IF BALL (2) collides with PADDLE (22)
		if (abs((int)(shapeACategory - shapeBCategory)) == 20) {
			b2ShapeId shape;
			if (shapeACategory == BALL)
				shape = hitEvent->shapeIdA;
			else
				shape = hitEvent->shapeIdB;
			b2BodyId ballBody = b2Shape_GetBody(shape);
			b2Vec2 vel = b2Body_GetLinearVelocity(ballBody);
			float volMod = sqrt((pow(vel.x, 2) + pow(vel.y, 2))) * 1;
			float vol = InvLerp(0, 10000, volMod);
			vol = 1.0f / pow(vol, -0.5);
			//printf("Volume: %.2f\n", volMod);
			SetSoundVolume(paddleSound, vol);
			PlaySound(paddleSound);
		}
	}

	// BeginTouchEvents
	for (int i = 0; i < contactEvents.beginCount; ++i) {
		b2ContactBeginTouchEvent* beginEvent = contactEvents.beginEvents + i;
		uint64_t shapeACategory = b2Shape_GetFilter(beginEvent->shapeIdA).categoryBits;
		uint64_t shapeBCategory = b2Shape_GetFilter(beginEvent->shapeIdB).categoryBits;
		// IF PADDLE (22) touches LIMIT (32)
		if (abs((int)(shapeACategory - shapeBCategory)) == 10) {
			paddle.touchingLimit = true;
			paddle.lastTouchTime = GetTime();
		}
	}

	// EndTouchEvents
	for (int i = 0; i < contactEvents.endCount; ++i) {
		b2ContactEndTouchEvent* touchEvent = contactEvents.endEvents + i;
		uint64_t shapeACategory = b2Shape_GetFilter(touchEvent->shapeIdA).categoryBits;
		uint64_t shapeBCategory = b2Shape_GetFilter(touchEvent->shapeIdB).categoryBits;
		// IF PADDLE (22) touches LIMIT (32)
		if (abs((int)(shapeACategory - shapeBCategory)) == 10) {
			paddle.touchingLimit = false;
		}
	}

	// Raycast Collision
	MyRayCastContext context = {0};
	context.targetShapeId = paddle.shapeId;
	b2Vec2 origin = b2Body_GetPosition(ballEntity.bodyId);
	b2Vec2 translation = b2Body_GetLinearVelocity(ballEntity.bodyId);
	//translation.x = translation.x * (1.0f / 60.0f);
	//translation.y = translation.y * (1.0f / 60.0f);
	b2QueryFilter filter = {RAY, PADDLE};
	b2World_CastRay(worldId, origin, translation, filter, &PaddleBallCast, &context);
	if (context.shapeId.index1 != 0)
		if (DEBUG) printf("Context: ShapeID: %d, Point: (%.2f, %.2f), Normal: (%.2f, %.2f), Frac: (%.2f) \n", context.shapeId.index1, context.point.x, context.point.y, context.normal.x, context.normal.y, context.fraction);

	// #############
	// Drawing logic
	// #############

	lastMousePosition = mousePosition;
	ClearBackground(DARKGRAY);
	BeginDrawing();

	char debugText[32];
	snprintf(debugText, sizeof(debugText), "Rot: %.3f", camera.rotation);
	DrawText(debugText, 0, 0, 25, BLACK);
	BeginMode2D(camera);
	// Draw cursor
	DrawCircle((int)mouseInWorld.x, (int)mouseInWorld.y, 10.0f, color);


	// Draw force-field vectors (debugging)
	for (int i = 0; i < BOX_COUNT; ++i) {
		b2Vec2* vec = VectorsToDraw + i;
		if (vec->x != 0 && vec->y != 0) {
			rlSetLineWidth(3);
			DrawLine(mVec.x, mVec.y, vec->x, vec->y, color);
		}
	}

	Vector2 hitPoint = { origin.x + translation.x * context.fraction,  origin.y + translation.y * context.fraction};
	DrawLine(origin.x, origin.y, hitPoint.x, hitPoint.y, color);
	DrawLine(hitPoint.x,  hitPoint.y, hitPoint.x + context.normal.x, hitPoint.y + context.normal.y, BLUE);

	// Draw outer bounds

	DrawEntity(&leftWall);
	DrawEntity(&rightWall);
	DrawEntity(&ceiling);

	// Draw static targets
	for (int i = 0; i < TARGET_COUNT; ++i)
	{
		DrawEntity(targetEntities + i);
	}

	// Draw physics-based boxes
	for (int i = 0; i < BOX_COUNT; ++i)
	{
		DrawEntity(boxEntities + i);
	}

	DrawBall(&ballEntity);
	DrawPaddle(&paddle);
	DrawEntity(&limit);

	DrawCircle((int)paddleTarget.x, (int)paddleTarget.y, 10.0f, PURPLE);
	EndMode2D();
	EndDrawing();
}