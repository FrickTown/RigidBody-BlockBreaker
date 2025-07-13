#include "assets.h"
#include "raylib.h"

extern Texture TextureLibrary[TextureEnumSize];
extern Sound SoundLibrary[SoundEnumSize];

void LoadAssetLibraries(){
    TextureLibrary[t_block_idle] = LoadTexture("assets/block_idle.png");
	TextureLibrary[t_box] = LoadTexture("assets/box.png");
	TextureLibrary[t_ball] = LoadTexture("assets/ball.png");
    
	TextureLibrary[t_wall_left] = LoadTexture("assets/wallL.png");
	TextureLibrary[t_wall_left_top] = LoadTexture("assets/wallLTop.png");
	TextureLibrary[t_wall_right] = LoadTexture("assets/wallR.png");
	TextureLibrary[t_wall_right_top] = LoadTexture("assets/wallRTop.png");

	TextureLibrary[t_ceiling_left] = LoadTexture("assets/ceilL.png");
	TextureLibrary[t_ceiling_mid] = LoadTexture("assets/ceilMid.png");
	TextureLibrary[t_ceiling_right] = LoadTexture("assets/ceilR.png");

	TextureLibrary[t_bg_ground] = LoadTexture("assets/bg_ground.png");
	TextureLibrary[t_bg_view] = LoadTexture("assets/bg_view.png");
	TextureLibrary[t_bg_sky] = LoadTexture("assets/bg_sky.png");

	TextureLibrary[t_target_rest] = LoadTexture("assets/target_rest.png");
	TextureLibrary[t_target_awake] = LoadTexture("assets/target_awake.png");

	TextureLibrary[t_limit] = LoadTexture("assets/limit.png");
	TextureLibrary[t_limit_end] = LoadTexture("assets/limit_end.png");

	TextureLibrary[t_paddle_left] = LoadTexture("assets/paddleL.png");
	TextureLibrary[t_paddle_mid] = LoadTexture("assets/paddleMid.png");
	TextureLibrary[t_paddle_right] = LoadTexture("assets/paddleR.png");
    
    SoundLibrary[s_paddle_1] = LoadSound("assets/paddle1.ogg");
	SoundLibrary[s_paddle_2] = LoadSound("assets/paddle2.ogg");
	SoundLibrary[s_paddle_3] = LoadSound("assets/paddle3.ogg");
	SoundLibrary[s_target_1] = LoadSound("assets/target1.ogg");
	SoundLibrary[s_target_2] = LoadSound("assets/target2.ogg");
	SoundLibrary[s_target_3] = LoadSound("assets/target3.ogg");
	SoundLibrary[s_target_4] = LoadSound("assets/target4.ogg");
}

void UnloadAssetLibraries(){
    for(int i = 0; i < TextureEnumSize; i++){
        UnloadTexture(TextureLibrary[i]);
    }
    for(int i = 0; i < SoundEnumSize; i++){
        UnloadSound(SoundLibrary[i]);
    }
}