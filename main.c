#include "raylib.h"
#include "game.h"

int main(void)
{
    InitWindow(W, H, "Neon Drift Courier (Elite)");
    SetTargetFPS(60);

    InitAudioDevice();

    Game g = {0};
    GameInit(&g);

    while(!WindowShouldClose())
    {
        float dt = GetFrameTime();
        if(dt > 0.05f) dt = 0.05f;

        GameUpdate(&g, dt);

        BeginDrawing();
        GameDraw(&g);
        EndDrawing();
    }

    GameUnload(&g);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
