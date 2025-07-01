#include "raylib.h"
#include "box2d/box2d.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "rlgl.h"
#include "contact.h"
#include "entities.h"
#include "raymath.h"
#include "arena.h"
#include "levels.h"
#if defined(PLATFORM_WEB)
	#include <emscripten/emscripten.h>
#endif

float InvLerp(float a, float b, float t) {
	return (t - a) / (b - a);
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

void UpdateDrawFrame(void);
void InitWorld(void);
void LoadAssets(void);
void UnloadAssets(void);

#define BOX_COUNT 10

bool pause = false;

int width = 1920, height = 1080;
int main(void)
{
	srand(time(NULL));
	InitWindow(width, height, "box2d-raylib");
	InitAudioDevice();
	LoadAssets();
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

	UnloadAssets();

	CloseAudioDevice();
	CloseWindow();

	return 0;
}

Texture groundTexture, boxTexture, targetTexture, ballTexture;
char* paddleTexturePaths[][3] = {
	"paddleL.png", "paddleMid.png", "paddleR.png"
};

Texture wallTextures[4] = { 0 };
Texture ceilTextures[3] = { 0 };
Texture backgroundTextures[3] = { 0 };
Texture limitTextures[2] = { 0 };
Texture targetTextures[2] = { 0 };

Sound paddleSounds[3] = { nullptr };
Sound targetSounds[4] = { nullptr };

void LoadAssets(void) {
	groundTexture = LoadTexture("assets/ground.png");
	boxTexture = LoadTexture("assets/box.png");
	targetTexture = LoadTexture("assets/ground.png");
	ballTexture = LoadTexture("assets/ball.png");
	targetTexture.height = targetTexture.height * 0.5f;
	targetTexture.width = targetTexture.width * 0.5f;

	paddleSounds[0] = LoadSound("assets/paddle1.ogg");
	paddleSounds[1] = LoadSound("assets/paddle2.ogg");
	paddleSounds[2] = LoadSound("assets/paddle3.ogg");

	targetSounds[0] = LoadSound("assets/target1.ogg");
	targetSounds[1] = LoadSound("assets/target2.ogg");
	targetSounds[2] = LoadSound("assets/target3.ogg");
	targetSounds[3] = LoadSound("assets/target4.ogg");

	wallTextures[0] = LoadTexture("assets/wallL.png");
	wallTextures[1] = LoadTexture("assets/wallLTop.png");
	wallTextures[2] = LoadTexture("assets/wallR.png");
	wallTextures[3] = LoadTexture("assets/wallRTop.png");

	ceilTextures[0] = LoadTexture("assets/ceilL.png");
	ceilTextures[1] = LoadTexture("assets/ceilMid.png");
	ceilTextures[2] = LoadTexture("assets/ceilR.png");

	backgroundTextures[0] = LoadTexture("assets/bg_ground.png");
	backgroundTextures[1] = LoadTexture("assets/bg_view.png");
	backgroundTextures[2] = LoadTexture("assets/bg_sky.png");

	limitTextures[0] = LoadTexture("assets/limit.png");
	limitTextures[1] = LoadTexture("assets/limit_end.png");

	targetTextures[0] = LoadTexture("assets/target_rest.png");
	targetTextures[1] = LoadTexture("assets/target_awake.png");
}

constexpr bool DEBUG = false;

float lengthUnitsPerMeter;
b2WorldId worldId;
Level level;
Camera2D camera = { 0 };
Vector2 screenOrigin, screenMax;

Entity boxEntities[BOX_COUNT] = { 0 };
Entity leftWall, rightWall, ceiling, limit;
Ball ballEntity;
Paddle paddle;

Vector2 mousePosition;
Vector2 lastMousePosition;
Vector2 mouseDelta;

bool holdingEntity = false;
Entity* lastHeldEntity;

void UnloadAssets(void) {
	UnloadTexture(groundTexture);
	UnloadTexture(boxTexture);
	for (int i = 0; i < 3; i++) {
		UnloadSound(paddleSounds[i]);
		UnloadSound(targetSounds[i]);
		UnloadTexture(paddle.textures[i]);
	}
	UnloadSound(targetSounds[3]);
}

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


	// Top-left and bottom-right vectors
	screenOrigin = GetScreenToWorld2D((Vector2){0, 0}, camera);
	screenMax = GetScreenToWorld2D((Vector2){width, height}, camera);

	b2Vec2 staticsExtent = { 0.5f * groundTexture.width, 0.5f * groundTexture.height };

	// Defining the solid walls
	b2Vec2 wallExtent = {wallTextures[0].width * 0.5f, (screenMax.y - screenOrigin.y) / 2};
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

	ballEntity = CreateBall(
		(b2Vec2){width / 2.0f, height / 5.0f},
		0.3f * lengthUnitsPerMeter,
		&ballTexture,
		PURPLE,
		worldId
		);

	paddle = CreatePaddle(
		(b2Vec2){width / 2.0f, height * 0.95f}, 1.2f * lengthUnitsPerMeter, 0.4f * lengthUnitsPerMeter,
		paddleTexturePaths, 3, 0.8f, BLUE, worldId
		);

	b2Vec2 limPos = {width / 2.0f, height * 0.85f + limitTextures[0].height / 2.0f};
	b2Vec2 limExtent = {width, 16};
	limit = CreateSolid(
		limPos,
		limExtent,
		nullptr, WHITE, worldId
		);

	b2Filter filt = { 0 };
	filt.categoryBits = BALLTHRU;
	filt.maskBits = PADDLE;
	b2Shape_SetFilter(limit.shapeId, filt);

	mousePosition = GetMousePosition();
	lastMousePosition = GetMousePosition();
	mouseDelta = GetMouseDelta();

	holdingEntity = false;

	float innerWidth = (rPos.x - rightWall.extent.x) - (lPos.x + leftWall.extent.x);
	float innerHeight =  (limPos.y - limExtent.y) - (ceilPos.y + ceilExtent.y);
	printf("Available Width / Height: %.3f / %.3f", innerWidth, innerHeight);
	Vector2 innerOrigin = {(lPos.x + leftWall.extent.x), (ceilPos.y + ceilExtent.y)};
	level = LoadLevel(levelVuve, innerOrigin, targetTextures, worldId);
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
	// Collision logic
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
			for (int j = 0; j < level.targetCount; j++) {
				if (level.targets[j].bodyId.index1 == targetBody.index1) {
					if (level.targets[j].state == 0) {
						level.targets[j].state = 1;
						b2MassData massData = b2Body_GetMassData(targetBody);
						massData.mass = 0.01f;
						b2Body_SetMassData(targetBody, massData);
						b2Body_SetType(targetBody, b2_dynamicBody);
					}
					else if (level.targets[j].state == 1)
						//b2DestroyBody(level.targets[j].bodyId);
						b2Body_Disable(targetBody);
				}

			}
			int r = rand() % 3;
			SetSoundVolume(targetSounds[r], 1.0f);
			PlaySound(targetSounds[r]);
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
			int r = rand() % 3;
			SetSoundVolume(paddleSounds[r], vol);
			PlaySound(paddleSounds[r]);
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
	b2Vec2 origin = { 0 };
	b2Vec2 translation = { 0 };
	if (DEBUG) {
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
	}

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

	// Ray-casting for landing
	if (DEBUG) {
		Vector2 hitPoint = { origin.x + translation.x * context.fraction,  origin.y + translation.y * context.fraction};
		DrawLine(origin.x, origin.y, hitPoint.x, hitPoint.y, color);
		DrawLine(hitPoint.x,  hitPoint.y, hitPoint.x + context.normal.x, hitPoint.y + context.normal.y, BLUE);
	}

	// Draw outer bounds

	DrawBackground(backgroundTextures, &camera);
	DrawEntity(&rightWall);
	DrawWalls(wallTextures, &camera);
	DrawCeiling(ceilTextures,  &camera);

	// Draw physics-based boxes
	for (int i = 0; i < BOX_COUNT; ++i)
	{
		DrawEntity(boxEntities + i);
	}

	DrawLevel(&level);

	DrawBall(&ballEntity);
	DrawPaddle(&paddle);

	//DrawEntity(&limit);
	DrawLimit(limitTextures, &limit);

	DrawCircle((int)paddleTarget.x, (int)paddleTarget.y, 10.0f, PURPLE);
	EndMode2D();
	EndDrawing();
}