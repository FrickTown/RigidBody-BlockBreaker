#include "box2d/collision.h"
#include "raylib.h"
#include "box2d/box2d.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "assets.h"
#include "rlgl.h"
#include "entities.h"
#include "arena.h"
#include "interface.h"
#include "levels.h"

Texture TextureLibrary[TextureEnumSize] = { 0 };
Sound SoundLibrary[SoundEnumSize] = { nullptr };
InterfaceAssets interfaceAssets = { 0 };

#if defined(PLATFORM_WEB)
	#include <emscripten/emscripten.h>
#endif

float InvLerp(float a, float b, float t) {
	return (t - a) / (b - a);
}

void Update(void);
void DrawFrame(void);
void InitWorld(void);
void UnloadAssets(void);

void CoreLoop(void){
	Update();
	DrawFrame();
}

#define BOX_COUNT 10

int width = 1920, height = 1080;
int main(void)
{
	srand(time(nullptr));
	InitWindow(width, height, "box2d-raylib");
	InitAudioDevice();
	LoadAssetLibraries();
	interfaceAssets = LoadInterfaceAssets();
	InitWorld();
	#if defined(PLATFORM_WEB)
		emscripten_set_main_loop(CoreLoop, 0, 1);
	#else
		SetTargetFPS(60);   // Set our game to run at 60 frames-per-second
	//--------------------------------------------------------------------------------------

		// Main game loop
		while (!WindowShouldClose())    // Detect window close button or ESC key
		{
			CoreLoop();
		}
	#endif

	UnloadAssetLibraries();
	UnloadInterfaceAssets(interfaceAssets);

	CloseAudioDevice();
	CloseWindow();

	return 0;
}

constexpr bool DEBUG = true;

float lengthUnitsPerMeter;
b2WorldId worldId;
Level level;
Camera2D camera = { 0 };
Vector2 screenOrigin, screenMax;
Rectangle screenBounds;

GameState gameState = {
	.paused = false,
	.state = GAME_ACTIVE,
	.score = 0
};

PauseMenu* pauseMenu;

Entity boxEntities[BOX_COUNT] = { 0 };
Entity leftWall, rightWall, ceiling, limit, deathZone;
Ball ballEntity;
Paddle paddle;

Vector2 mousePosition;
Vector2 mouseDelta;

Vector2 mouseInWorld;
b2Vec2 mVec;
b2Vec2 paddleTarget;

bool holdingEntity = false;
Entity* lastHeldEntity;

BallRayCastContext context = {0};
b2Vec2 origin = { 0 };
b2Vec2 translation = { 0 };
b2Vec2 VectorsToDraw[10] = { 0 };

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
	screenBounds = (Rectangle){screenOrigin.x, screenOrigin.y, screenMax.x - screenOrigin.x, screenMax.y - screenOrigin.y};

	b2Vec2 staticsExtent = { 0.5f * TextureLibrary[t_block_idle].width, 0.5f * TextureLibrary[t_block_idle].height };

	// Defining the solid walls
	b2Vec2 wallExtent = {TextureLibrary[t_wall_left].width * 0.5f, (screenMax.y - screenOrigin.y) / 2};
	b2Vec2 lPos = {screenOrigin.x + staticsExtent.x, screenOrigin.y + (screenMax.y - screenOrigin.y) / 2};
	leftWall = CreateSolid(lPos, wallExtent, nullptr, PURPLE, worldId);
	b2Vec2 rPos = {screenMax.x - staticsExtent.x, screenOrigin.y + (screenMax.y - screenOrigin.y) / 2};
	rightWall = CreateSolid(rPos, wallExtent, nullptr, PURPLE, worldId);

	// Defining the solid ceiling
	b2Vec2 ceilPos = {screenOrigin.x + ((screenMax.x - screenOrigin.x) / 2), screenOrigin.y + TextureLibrary[t_ceiling_left].height * 0.5f};
	b2Vec2 ceilExtent = {((screenMax.x - screenOrigin.x) / 2), TextureLibrary[t_ceiling_left].height * 0.5f };
	ceiling = CreateSolid(ceilPos, ceilExtent, nullptr, WHITE, worldId);

	b2Vec2 boxExtent = { 0.5f * TextureLibrary[t_box].width, 0.5f * TextureLibrary[t_box].height };
	for (int i = 0; i < BOX_COUNT; ++i)
	{
		float y = height - boxExtent.y - 100.0f - (2.5f * i + 2.0f) * boxExtent.y - 20.0f;
		float x = 0.5f * width + (3.0f * i - 3.0f) * boxExtent.x;
		Entity box = CreatePhysicsBox((b2Vec2){x, y}, boxExtent, &TextureLibrary[t_box], worldId);
		boxEntities[i] = box;
	}

	

	// Create the paddle
	paddle = CreatePaddle(
		(b2Vec2){
			width / 2.0f, 
			height * 0.95f
		}, 
		1.2f * lengthUnitsPerMeter, 
		0.4f * lengthUnitsPerMeter,
		BLUE, 
		worldId
		);

	// Create the paddle's movement limit
	// TODO: Make special function for this
	b2Vec2 limPos = {width / 2.0f, height * 0.85f + TextureLibrary[t_limit].height / 2.0f};
	b2Vec2 limExtent = {width, 16};
	limit = CreateSolid(
		limPos,
		limExtent,
		nullptr, WHITE, worldId
		);
	
	b2Filter filt = { 0 };
	filt.categoryBits = BALLTHRU;
	filt.maskBits = PADDLE | TARGET;
	b2Shape_SetFilter(limit.shapeId, filt);

	// Establish the inner bounds of the arena
	// TODO: Clean this up (move to arena.c?)
	float innerWidth = (rPos.x - rightWall.extent.x) - (lPos.x + leftWall.extent.x);
	float innerHeight =  (limPos.y - limExtent.y) - (ceilPos.y + ceilExtent.y);
	Vector2 innerOrigin = {(lPos.x + leftWall.extent.x), (ceilPos.y + ceilExtent.y)};

	// Establish the death zone
	// TODO: Allow levels to define their specific death zone
	float dzHeight = 100;
	b2Vec2 dzExtent = {innerWidth / 2, dzHeight};
	b2Vec2 dzPos = {innerOrigin.x + dzExtent.x, screenMax.y - dzExtent.y};
	deathZone = CreateDeathZone(dzPos, dzExtent, nullptr, WHITE, worldId);

	mousePosition = GetMousePosition();
	mouseDelta = GetMouseDelta();

	Vector2 center = {
		screenMax.x - (screenMax.x - screenOrigin.x) / 2, 
		screenMax.y - (screenMax.y - screenOrigin.y) / 2
	};
	Rectangle pauseMenuBounds = {
		center.x - innerWidth  / 8, 
		center.y - innerHeight  / 2, 
		innerWidth / 4, 
		innerHeight
	};
	pauseMenu = CreatePauseMenu(
		&gameState, 
		pauseMenuBounds, 
		&interfaceAssets);
	//printf("Available Width / Height: %.3f / %.3f", innerWidth, innerHeight);
	level = LoadLevel(levelRooms, innerOrigin, worldId);

	//TODO: Investigate further why ballspawn is not where it should be
	Vector2 ballTest = GetWorldToScreen2D(
		(Vector2){
			level.ballSpawn.x, 
			level.ballSpawn.y
		}, camera);
	ballEntity = CreateBall(
		(b2Vec2){ballTest.x, ballTest.y},
		0.3f * lengthUnitsPerMeter,
		&TextureLibrary[t_ball],
		PURPLE,
		worldId
		);
}

void Update(void) {
	mouseInWorld = GetScreenToWorld2D(mousePosition, camera);
	if (IsKeyPressed(KEY_P))
	{
		gameState.paused = !gameState.paused;
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

	mouseInWorld = GetScreenToWorld2D(mousePosition, camera);
	mVec = (b2Vec2){mouseInWorld.x, mouseInWorld.y};

	memset(VectorsToDraw, 0, sizeof VectorsToDraw);

	int VecIndex = 0;

	paddle.tilt = 0;
	Color color = RED;
	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
		// Mouse left allows picking up a box
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !holdingEntity) {
			paddle.tilt = -1;
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
			paddle.tilt = 1;
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
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		PauseMenuHandleClick(pauseMenu, mouseInWorld);
	}

	// Logic for killing a ball if it hits death zone
	b2Vec2 ballPos = b2Body_GetPosition(ballEntity.bodyId);
	b2Vec2 ballToDeath = b2Body_GetLocalPoint(deathZone.bodyId, ballPos);
	if (fabsf(ballToDeath.x) <= deathZone.extent.x && fabsf(ballToDeath.y) <= deathZone.extent.y) {
		ResetBall(&ballEntity);
	}

	// Prevent high-velocity shots by having cursor above limit
	paddleTarget = mVec;
	if (paddleTarget.y < height / 2.0f) {
		paddleTarget.y = height / 2.0f;
	}
	b2Vec2 paddlePos = b2Body_GetPosition(paddle.bodyId);
	float limitBottom = b2Body_GetPosition(limit.bodyId).y + limit.extent.y;
	if (paddleTarget.y <= paddlePos.y && paddle.touchingLimit && paddle.timeDelta > 0.1f) {
		paddleTarget.y = limitBottom + paddle.extent.y;
	}

	// Raycast Collision

	// Reset the ballcast result 
	memset(&context, 0, sizeof(context));
	
	context.targetShapeId = paddle.shapeId;
	origin = b2Body_GetPosition(ballEntity.bodyId);
	translation = b2MulSV(100000, b2Normalize(b2Body_GetLinearVelocity(ballEntity.bodyId)));
	//translation.x = translation.x * (1.0f / 60.0f);
	//translation.y = translation.y * (1.0f / 60.0f);
	b2QueryFilter filter = {RAY, PADDLE};
	b2ShapeProxy ballProx = b2MakeProxy(&ballPos, 1, ballEntity.radius);
	b2World_CastShape(worldId, &ballProx, translation, filter, BallRayResultFcn, &context);

	if (DEBUG) {
		//b2World_CastRay(worldId, origin, translation, filter, &BallRayResultFcn, &context);
		if (context.shapeId.index1 == paddle.shapeId.index1)
			printf("Context: ShapeID: %d, Point: (%.2f, %.2f), Normal: (%.2f, %.2f), Frac: (%.8f) \n", context.shapeId.index1, context.point.x, context.point.y, context.normal.x, context.normal.y, context.fraction);
	}

	// Set the paddle velocity required to approach the cursor the next timestep
	UpdatePaddle(&paddle, paddleTarget);

	// Get the magnitude of the paddle velocity
	b2Vec2 paddleVelocity = b2Body_GetLinearVelocity(paddle.bodyId);
	float paddleSpeed = sqrt(paddleVelocity.x * paddleVelocity.x + paddleVelocity.y * paddleVelocity.y);

	// If paddle is moving fast enough, perform extra collision checks to prevent tunneling
	if(paddleSpeed > 2000.0f){
		printf("Paddlespeed: %.4f \n", paddleSpeed);
		CheckBallPaddleCollision(&ballEntity, &paddle, &context);
	}

	if (gameState.paused == false)
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
		if (DEBUG) printf("ShapeIDA: %lu, ShapeIDB: %lu\n", b2Shape_GetFilter(hitEvent->shapeIdA).categoryBits, b2Shape_GetFilter(hitEvent->shapeIdB).categoryBits);
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
						//b2MassData massData = b2Body_GetMassData(targetBody);
						//massData.mass = 5.0f;
						//b2Body_SetMassData(targetBody, massData);
						b2Body_SetType(targetBody, b2_dynamicBody);
						b2Vec2 ballVel = b2Body_GetLinearVelocity(ballEntity.bodyId);
						ballVel.x *= -1;
						ballVel.y *= -1;
						b2Body_SetLinearVelocity(targetBody, ballVel);
						gameState.score += 20;
					}
					else if (level.targets[j].state == 1){
						//b2DestroyBody(level.targets[j].bodyId);
						b2Body_Disable(targetBody);
						gameState.score += 30;
					}
				}

			}
			int r = rand() % 4;
			SetSoundVolume(SoundLibrary[s_target_1 + r], 0.75f);
			PlaySound(SoundLibrary[s_target_1 + r]);
		}
		// IF BALL (2) collides with PADDLE (22)
		if (abs((int)(shapeACategory - shapeBCategory)) == 20) {
			b2ShapeId shape;
			if (shapeACategory == BALL)
				shape = hitEvent->shapeIdA;
			else
				shape = hitEvent->shapeIdB;

			// Audio level determination
			b2BodyId ballBody = b2Shape_GetBody(shape);
			b2Vec2 vel = b2Body_GetLinearVelocity(ballBody);
			float volMod = sqrt((pow(vel.x, 2) + pow(vel.y, 2))) * 1;
			float vol = InvLerp(0, 10000, volMod);
			vol = 1.0f / pow(vol, -0.5);
			//printf("Volume: %.2f\n", volMod);
			int r = rand() % 3;
			SetSoundVolume(SoundLibrary[s_paddle_1 + r], vol);
			PlaySound(SoundLibrary[s_paddle_1 + r]);
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
}

void DrawFrame(void){
	// #############
	// Drawing logic
	// #############

	ClearBackground(DARKGRAY);
	BeginDrawing();

	char debugText[32];
	snprintf(debugText, sizeof(debugText), "Rot: %.3f", camera.rotation);
	DrawText(debugText, 0, 0, 25, BLACK);
	BeginMode2D(camera);
	// Draw cursor
	DrawCircle((int)mouseInWorld.x, (int)mouseInWorld.y, 10.0f, RED);


	// Draw force-field vectors (debugging)
	if (DEBUG) {
		for (int i = 0; i < BOX_COUNT; ++i) {
			b2Vec2* vec = VectorsToDraw + i;
			if (vec->x != 0 && vec->y != 0) {
				rlSetLineWidth(3);
				DrawLine(mVec.x, mVec.y, vec->x, vec->y, BLUE);
			}
		}
	}

	// Draw outer bounds

	DrawBackground(&camera);
	//DrawEntity(&rightWall);

	// Ray-casting for landing
	if (context.shapeId.index1 == paddle.shapeId.index1) {
		Vector2 hitPoint = { origin.x + translation.x * context.fraction,  origin.y + translation.y * context.fraction};
		DrawLine(origin.x, origin.y, hitPoint.x, hitPoint.y, RED);
		DrawLine(hitPoint.x,  hitPoint.y, hitPoint.x + context.normal.x, hitPoint.y + context.normal.y, BLUE);
        b2Vec2 adjustment = b2MulSV(ballEntity.radius, context.normal);
        b2Vec2 adjPos = b2Add(context.point, adjustment);
		rlSetLineWidth(3.0f);
		DrawLine(context.point.x, context.point.y, adjPos.x, adjPos.y, PINK);
	}

	DrawWalls(&camera);
	DrawCeiling(&camera);

	// Draw physics-based boxes
	for (int i = 0; i < BOX_COUNT; ++i)
	{
		DrawEntity(boxEntities + i);
	}

	DrawLevel(&level);

	DrawBall(&ballEntity);
	DrawEntity(&deathZone);
	DrawPaddle(&paddle);
	//DrawEntity(&limit);
	DrawLimit(&limit);
	if (gameState.paused)
		DrawPauseMenu(pauseMenu);
	DrawCircle((int)paddleTarget.x, (int)paddleTarget.y, 10.0f, PURPLE);
	DrawHUD(&gameState, &interfaceAssets, screenBounds);
	EndMode2D();
	EndDrawing();
}