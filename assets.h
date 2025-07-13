#ifndef ASSETS_H
#define ASSETS_H
#include "raylib.h"

enum TextureEnum {
    t_block_idle,
    t_box,
    t_ball,
    t_wall_left,
    t_wall_left_top,
    t_wall_right,
    t_wall_right_top,
    t_ceiling_left,
    t_ceiling_mid,
    t_ceiling_right,
    t_bg_ground,
    t_bg_view,
    t_bg_sky,
    t_target_rest,
    t_target_awake,
    t_limit,
    t_limit_end,
    t_paddle_left,
    t_paddle_mid,
    t_paddle_right,
    TextureEnumSize
};

enum SoundEnum {
    s_paddle_1,
    s_paddle_2,
    s_paddle_3,
    s_target_1,
    s_target_2,
    s_target_3,
    s_target_4,
    SoundEnumSize
};

typedef struct InterfaceAssets {
    Texture numbers[10];
    Texture ui[7];
    Sound sounds[9];
    Font menuFont;

} InterfaceAssets;

void LoadAssetLibraries(void);
void UnloadAssetLibraries(void);

InterfaceAssets LoadInterfaceAssets(void);
void UnloadInterfaceAssets(InterfaceAssets assets);

#endif //ASSETS_H