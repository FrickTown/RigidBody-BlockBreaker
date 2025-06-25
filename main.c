#include "raylib.h"
#include "box2d/box2d.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "rlgl.h"
#include "cmake-build-debug/_deps/box2d-src/src/contact.h"
#include "entities.h"

// This shows how to use Box2D v3 with raylib.
// It also show how to use Box2D with pixel units.


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
	DrawTextureEx(entity->texture, ps, RAD2DEG * radians, 1.0f, WHITE);

	// I used these circles to ensure the coordinates are correct
	//DrawCircleV(ps, 5.0f, BLACK);
	//p = b2Body_GetWorldPoint(entity->bodyId, (b2Vec2){0.0f, 0.0f});
	//ps = (Vector2){ p.x, p.y };
	//DrawCircleV(ps, 5.0f, BLUE);
	//p = b2Body_GetWorldPoint(entity->bodyId, (b2Vec2){ entity->extent.x, entity->extent.y });
	//ps = (Vector2){ p.x, p.y };
	//DrawCircleV(ps, 5.0f, RED);
}

float degreesToRadians(float degrees) {
	return degrees * (M_PI / 180);
}

#define GROUND_COUNT 14
#define BOX_COUNT 10
#define TARGET_COUNT 10


int main(void)
{

	int width = 1920, height = 1080;
	InitWindow(width, height, "box2d-raylib");



	//HideCursor();
	Camera2D camera = { 0 };
	camera.target = (Vector2){ width/2.0f, height/2.0f };
	camera.offset = (Vector2){ width/2.0f, height/2.0f };
	camera.rotation = 0.0f;
	camera.zoom = 0.5f;

	SetTargetFPS(60);

	// 128 pixels per meter is a appropriate for this scene. The boxes are 128 pixels wide.
	float lengthUnitsPerMeter = 128.0f;
	b2SetLengthUnitsPerMeter(lengthUnitsPerMeter);

	b2WorldDef worldDef = b2DefaultWorldDef();

	// Realistic gravity is achieved by multiplying gravity by the length unit.
	worldDef.gravity.y = 9.8f * lengthUnitsPerMeter;
	worldDef.enableSleep = false;
	worldDef.hitEventThreshold = 2.0f * lengthUnitsPerMeter;
	b2WorldId worldId = b2CreateWorld(&worldDef);

	Texture groundTexture = LoadTexture("ground.png");
	Texture boxTexture = LoadTexture("box.png");
	Texture targetTexture = LoadTexture("ground.png");
	targetTexture.height = targetTexture.height * 0.5f;
	targetTexture.width = targetTexture.width * 0.5f;

	b2Vec2 groundExtent = { 0.5f * groundTexture.width, 0.5f * groundTexture.height };
	b2Vec2 boxExtent = { 0.5f * boxTexture.width, 0.5f * boxTexture.height };
	b2Vec2 targetExtent = { 0.5f * targetTexture.width, 0.5f * targetTexture.height };


	// These polygons are centered on the origin and when they are added to a body they
	// will be centered on the body position.
	b2Polygon groundPolygon = b2MakeBox(groundExtent.x, groundExtent.y);
	b2Polygon boxPolygon = b2MakeBox(boxExtent.x, boxExtent.y);
	b2Polygon targetPolygon = b2MakeBox(targetExtent.x, targetExtent.y);

	//b2Polygon ball = b2MakeRoundedBox(5.0f, 5.0f, 100.0f);

	Entity groundEntities[GROUND_COUNT] = { 0 };
	for (int i = 0; i < GROUND_COUNT; ++i)
	{
		Entity* entity = groundEntities + i;
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.position = (b2Vec2){ (2.0f * i + 2.0f) * groundExtent.x, height - groundExtent.y - 100.0f};

		// I used this rotation to test the world to screen transformation
		//bodyDef.rotation = b2MakeRot(0.25f * b2_pi * i);

		entity->bodyId = b2CreateBody(worldId, &bodyDef);
		entity->extent = groundExtent;
		entity->texture = groundTexture;
		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.filter.categoryBits = GROUND;
		b2CreatePolygonShape(entity->bodyId, &shapeDef, &groundPolygon);
	}

	Entity boxEntities[BOX_COUNT] = { 0 };
	int boxIndex = 0;
	for (int i = 0; i < BOX_COUNT; ++i)
	{
		float y = height - groundExtent.y - 100.0f - (2.5f * i + 2.0f) * boxExtent.y - 20.0f;

		float x = 0.5f * width + (3.0f * i - 3.0f) * boxExtent.x;
		assert(boxIndex < BOX_COUNT);

		Entity* entity = boxEntities + boxIndex;
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = b2_dynamicBody;
		bodyDef.position = (b2Vec2){ x, y };
		entity->bodyId = b2CreateBody(worldId, &bodyDef);
		entity->texture = boxTexture;
		entity->extent = boxExtent;
		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.filter.categoryBits = BOX;
		entity->shapeId = b2CreatePolygonShape(entity->bodyId, &shapeDef, &boxPolygon);
		boxIndex += 1;
	}

	Entity targetEntities[TARGET_COUNT] = { 0 };
	int targetIndex = 0;
	for (int i = 0; i < TARGET_COUNT; ++i) {
		Entity* entity = targetEntities + i;
		float y = (0) - targetExtent.y + (targetExtent.y * 2 * (i % 2));
		float x = ((width / 2.0f) - (targetExtent.x * 2) * TARGET_COUNT / 2.0f) + i * (targetExtent.x * 2.0f);

		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = b2_staticBody;
		bodyDef.position = (b2Vec2){ x, y };
		entity->bodyId = b2CreateBody(worldId, &bodyDef);
		entity->texture = targetTexture;
		entity->extent = targetExtent;
		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.enableContactEvents = true;
		shapeDef.enableHitEvents = true;
		shapeDef.filter.categoryBits = TARGET;
		entity->shapeId = b2CreatePolygonShape(entity->bodyId, &shapeDef, &targetPolygon);
		targetIndex += 1;
	}

	Ball ballEntity = CreateBall(
		(b2Vec2){width / 2.0f, height / 5.0f},
		2.0f * lengthUnitsPerMeter,
		RED,
		worldId
		);


	Entity paddleEntity = { 0 };
	b2BodyDef paddleBodyDef = b2DefaultBodyDef();
	paddleBodyDef.type = b2_dynamicBody;
	paddleBodyDef.position = b2Vec2_zero;
	paddleEntity.bodyId = b2CreateBody(worldId, &paddleBodyDef);
	paddleEntity.texture = targetTexture;


	bool pause = false;
	Vector2 mousePosition = GetMousePosition();
	Vector2 lastMousePosition = GetMousePosition();
	Vector2 mouseDelta = GetMouseDelta();

	bool holdingEntity = false;
	Entity* lastHeldEntity;

	while (!WindowShouldClose())
	{
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
						printf("(%.2f, %.2f)\n", pob.x, pob.y);
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
			printf("(%.2f, %.2f)\n", mouseDelta.x * 1280.0f, mouseDelta.y * 1280.0f);
			b2Vec2 releaseVelocity = {mouseDelta.x * 32.0f, mouseDelta.y * 32.0f};
			//releaseVelocity = (b2MakeRot(degreesToRadians(camera.rotation)), releaseVelocity);
			//b2Body_SetLinearVelocity(lastHeldEntity->bodyId, releaseVelocity);
		}

		if (pause == false)
		{
			float deltaTime = GetFrameTime();
			b2World_Step(worldId, deltaTime, 4);
		}

		// #################
		// Target hit logic
		// #################
		b2ContactEvents contactEvents = b2World_GetContactEvents(worldId);
		if (contactEvents.beginCount > 0 || contactEvents.endCount > 0 || contactEvents.hitCount > 0 )
			printf("Contact begin: %d, Contact end: %d, Hits: %d\n", contactEvents.beginCount, contactEvents.endCount, contactEvents.hitCount);

		for (int i = 0; i < contactEvents.hitCount; ++i)
		{
			b2ContactHitEvent* hitEvent = contactEvents.hitEvents + i;
			printf("ShapeIDA: %lu, ShapeIDB: %lu\n", b2Shape_GetFilter(hitEvent->shapeIdA).categoryBits, b2Shape_GetFilter(hitEvent->shapeIdB).categoryBits);
			uint64_t shapeACategory = b2Shape_GetFilter(hitEvent->shapeIdA).categoryBits;
			uint64_t shapeBCategory = b2Shape_GetFilter(hitEvent->shapeIdB).categoryBits;
			if (abs((int)(shapeACategory - shapeBCategory)) == 1) {
				b2ShapeId shape;
				if (shapeACategory == BALL)
					shape = hitEvent->shapeIdB;
				else
					shape = hitEvent->shapeIdA;
				b2BodyId targetBody = b2Shape_GetBody(shape);
				b2Body_Disable(targetBody);
			}
			if (hitEvent->approachSpeed > 10.0f)
			{
				// play sound
			}
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

		// Draw static ground
		for (int i = 0; i < GROUND_COUNT; ++i)
		{
			DrawEntity(groundEntities + i);
		}

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
		/*for (int i = 0; i < BALL_TRACERS; i++) {
			b2Vec2 ballHist = ballHistory[i];
			float histRad = ballRadius * (1.0f / BALL_TRACERS) * ((float)BALL_TRACERS - (float)i);
			//printf("(%.2f, %.2f), ", ballHist.x, ballHist.y);
			DrawCircle(ballHist.x, ballHist.y, histRad, c);
		}*/
		//printf("\n");
		EndMode2D();
		EndDrawing();

		// #############
		// Ball tracer logic
		// #############
		/*for (int k = BALL_TRACERS - 1; k > 0; k--){
			ballHistory[k] = ballHistory[k-1];
		}
		ballHistory[0] = ballPos;*/
	}

	UnloadTexture(groundTexture);
	UnloadTexture(boxTexture);

	CloseWindow();

	return 0;
}