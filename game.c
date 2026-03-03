#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Tunables
#define TRAIL_LEN   60
#define MAX_ENEMIES 12
#define MAX_PART    160

static float clampf(float x, float a, float b){ return x<a?a:(x>b?b:x); }
static float randf(float a, float b){ return a + (b-a)*((float)GetRandomValue(0,10000)/10000.0f); }

// ----- Audio: generate beep without GenWaveSine -----
static Sound MakeBeep(float freqHz, float sec, float vol)
{
    const int sampleRate = 22050;
    int sampleCount = (int)(sec * sampleRate);
    if (sampleCount < 1) sampleCount = 1;

    short *data = (short*)MemAlloc((unsigned int)sampleCount * sizeof(short));
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / (float)sampleRate;
        float env = 1.0f - ((float)i / (float)sampleCount);     // quick fade-out
        float s = sinf(2.0f*PI*freqHz*t) * env * vol;
        int v = (int)(s * 32767.0f);
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        data[i] = (short)v;
    }

    Wave w = {0};
    w.frameCount = sampleCount;
    w.sampleRate = sampleRate;
    w.sampleSize = 16;
    w.channels   = 1;
    w.data       = data;

    Sound snd = LoadSoundFromWave(w);
    UnloadWave(w); // frees data
    return snd;
}

// ----- Particles -----
static void Emit(Game *g, Vector2 p, Color c, int count, float spd)
{
    for(int k=0;k<count;k++){
        Particle *pt = &g->part[g->partHead];
        g->partHead = (g->partHead + 1) % g->partMax;

        float ang = randf(0, 6.28318f);
        float s = randf(0.2f, 1.0f) * spd;

        pt->p = p;
        pt->v = (Vector2){ cosf(ang)*s, sinf(ang)*s };
        pt->r = randf(2, 6);
        pt->life = 1.0f;
        pt->c = c;
    }
}

static void UpdateParticles(Game *g, float dt)
{
    for(int i=0;i<g->partMax;i++){
        Particle *pt = &g->part[i];
        if(pt->life <= 0) continue;
        pt->life -= dt * 1.8f;
        pt->p.x += pt->v.x * dt;
        pt->p.y += pt->v.y * dt;
        pt->v.x *= (1.0f - dt*1.5f);
        pt->v.y *= (1.0f - dt*1.5f);
        if(pt->life < 0) pt->life = 0;
    }
}

static void DrawParticles(const Game *g)
{
    for(int i=0;i<g->partMax;i++){
        const Particle *pt = &g->part[i];
        if(pt->life <= 0) continue;
        unsigned char a = (unsigned char)(pt->c.a * pt->life);
        Color c = (Color){ pt->c.r, pt->c.g, pt->c.b, a };
        DrawCircleV(pt->p, pt->r + 6, (Color){ c.r, c.g, c.b, (unsigned char)(a*0.15f) });
        DrawCircleV(pt->p, pt->r, c);
    }
}

// ----- Gameplay helpers -----
static void ResetPlayer(Game *g)
{
    g->pl.p = (Vector2){ W*0.25f, H*0.5f };
    g->pl.v = (Vector2){ 0,0 };
    g->pl.r = 16;
    g->pl.trailLen = TRAIL_LEN;
    g->pl.ti = 0;
    for(int i=0;i<g->pl.trailLen;i++) g->pl.trail[i] = g->pl.p;
}

static void SpawnScoreOrb(Game *g)
{
    g->orb.p = (Vector2){ randf(80,W-80), randf(80,H-80) };
    g->orb.r = 14;
    g->orb.pulse = 0;
}

static void SpawnPowerOrb(Game *g)
{
    g->power.p = (Vector2){ randf(120,W-120), randf(120,H-120) };
    g->power.r = 12;
    g->power.pulse = 0;
}

static void SpawnEnemies(Game *g)
{
    g->nE = 4;
    for(int i=0;i<g->maxE;i++){
        g->e[i].p = (Vector2){ randf(100,W-100), randf(100,H-100) };
        g->e[i].v = (Vector2){ randf(-140,140), randf(-140,140) };
        g->e[i].r = randf(10,18);
        g->e[i].phase = randf(0,6.28f);
    }
}

static void StartPlay(Game *g)
{
    g->s = PLAY;
    g->score = 0;

    g->flash = 0;
    g->shield = 0;
    g->shake = 0;
    g->zoomPulse = 0;
    g->slow = 0;

    ResetPlayer(g);
    SpawnEnemies(g);
    SpawnScoreOrb(g);
    SpawnPowerOrb(g);

    // small “start” punch
    g->zoomPulse = 0.12f;
}

static void UpdatePlayer(Game *g, float dt)
{
    Player *pl = &g->pl;

    Vector2 in = {0,0};
    if(IsKeyDown(KEY_RIGHT)||IsKeyDown(KEY_D)) in.x += 1;
    if(IsKeyDown(KEY_LEFT) ||IsKeyDown(KEY_A)) in.x -= 1;
    if(IsKeyDown(KEY_DOWN) ||IsKeyDown(KEY_S)) in.y += 1;
    if(IsKeyDown(KEY_UP)   ||IsKeyDown(KEY_W)) in.y -= 1;

    float len = sqrtf(in.x*in.x + in.y*in.y);
    if(len>0.001f){ in.x/=len; in.y/=len; }

    const float accel = 950.0f;
    const float maxSp = 360.0f;
    const float fric  = 7.0f;

    pl->v.x += in.x*accel*dt;
    pl->v.y += in.y*accel*dt;

    pl->v.x -= pl->v.x*fric*dt;
    pl->v.y -= pl->v.y*fric*dt;

    float sp = sqrtf(pl->v.x*pl->v.x + pl->v.y*pl->v.y);
    if(sp>maxSp){
        float k = maxSp/sp;
        pl->v.x *= k; pl->v.y *= k;
    }

    pl->p.x += pl->v.x*dt;
    pl->p.y += pl->v.y*dt;

    pl->p.x = clampf(pl->p.x, pl->r+10, W-pl->r-10);
    pl->p.y = clampf(pl->p.y, pl->r+10, H-pl->r-10);

    pl->trail[pl->ti] = pl->p;
    pl->ti = (pl->ti+1)%pl->trailLen;
}

static void UpdateEnemies(Game *g, float dt)
{
    int target = 4 + g->score/3;
    if(target>g->maxE) target=g->maxE;
    g->nE = target;

    float speedMul = 1.0f + 0.06f*g->score;

    for(int i=0;i<g->nE;i++){
        Enemy *e = &g->e[i];
        e->phase += dt*(1.0f+0.2f*i);
        float wig = sinf(e->phase)*18.0f;

        e->p.x += e->v.x*dt*speedMul;
        e->p.y += (e->v.y*dt*speedMul) + wig*dt;

        if(e->p.x < 50 || e->p.x > W-50) e->v.x *= -1;
        if(e->p.y < 50 || e->p.y > H-50) e->v.y *= -1;

        e->p.x = clampf(e->p.x, 50, W-50);
        e->p.y = clampf(e->p.y, 50, H-50);
    }
}

static int HitPlayer(const Game *g)
{
    for(int i=0;i<g->nE;i++){
        float dx = g->pl.p.x - g->e[i].p.x;
        float dy = g->pl.p.y - g->e[i].p.y;
        float rr = g->pl.r + g->e[i].r + 2;
        if(dx*dx + dy*dy <= rr*rr) return 1;
    }
    return 0;
}

static void UpdateOrbs(Game *g, float dt)
{
    g->orb.pulse += dt*4.5f;
    g->power.pulse += dt*4.0f;

    // score orb
    {
        float dx = g->pl.p.x - g->orb.p.x;
        float dy = g->pl.p.y - g->orb.p.y;
        float rr = g->pl.r + g->orb.r;

        if(dx*dx + dy*dy <= rr*rr){
            g->score += 1;

            // juicy feedback
            g->flash = 1.0f;
            g->shake = 10.0f;
            g->zoomPulse = 0.08f;

            if (g->audioReady) PlaySound(g->sPickup);
            Emit(g, g->orb.p, (Color){255,0,255,220}, 28, 220);

            SpawnScoreOrb(g);
            if(g->score % 5 == 0) SpawnPowerOrb(g);
        }
    }

    // power orb (shield)
    {
        float dx = g->pl.p.x - g->power.p.x;
        float dy = g->pl.p.y - g->power.p.y;
        float rr = g->pl.r + g->power.r;

        if(dx*dx + dy*dy <= rr*rr){
            g->shield = 3.5f;
            g->zoomPulse = 0.10f;

            if (g->audioReady) PlaySound(g->sPower);
            Emit(g, g->power.p, (Color){0,255,255,220}, 44, 260);

            SpawnPowerOrb(g);
        }
    }
}

// ----- Drawing (world) -----
static void DrawBackground(float t)
{
    ClearBackground((Color){ 5,6,12,255 });

    // moving streaks
    for(int i=-10;i<20;i++){
        float x = fmodf(i*120.0f + t*90.0f, W+300.0f) - 300.0f;
        DrawLineEx((Vector2){x,0}, (Vector2){x+220,H}, 2.0f, (Color){0,255,255,18});
    }

    // grid
    for(int x=0;x<W;x+=60) DrawLine(x,0,x,H,(Color){255,0,255,12});
    for(int y=0;y<H;y+=60) DrawLine(0,y,W,y,(Color){0,255,255,10});

    DrawRectangleLinesEx((Rectangle){12,12,W-24,H-24}, 2, (Color){255,0,255,40});
}

static void DrawPlayer(const Game *g)
{
    const Player *pl = &g->pl;

    // trail glow
    for(int i=0;i<pl->trailLen;i++){
        int idx = (pl->ti + i) % pl->trailLen;
        float r = pl->r - i*0.22f; if(r<3) r=3;
        Vector2 p = pl->trail[idx];
        DrawCircleV(p, r+10, (Color){0,255,255,18});
        DrawCircleV(p, r+6,  (Color){0,255,255,32});
        DrawCircleV(p, r+2,  (Color){0,255,255,52});
    }

    // shield ring
    if(g->shield > 0){
        float pulse = (sinf(g->t*9.0f)+1.0f)*0.5f;
        DrawCircleV(pl->p, pl->r+22 + pulse*3, (Color){0,255,255,40});
        DrawCircleV(pl->p, pl->r+18 + pulse*2, (Color){0,255,255,70});
    }

    DrawCircleV(pl->p, pl->r+10, (Color){255,255,255,24});
    DrawCircleV(pl->p, pl->r+6,  (Color){0,255,255,70});
    DrawCircleV(pl->p, pl->r+1,  (Color){255,255,255,180});
    DrawCircleV(pl->p, pl->r-6,  (Color){0,255,255,220});
}

static void DrawEnemies(const Game *g)
{
    for(int i=0;i<g->nE;i++){
        const Enemy *e = &g->e[i];
        float pulse = (sinf(g->t*7.0f + i) + 1.0f)*0.5f;

        DrawCircleV(e->p, e->r+12, (Color){255,0,140, (unsigned char)(18 + 18*pulse)});
        DrawCircleV(e->p, e->r+6,  (Color){255,0,140, (unsigned char)(45 + 25*pulse)});
        DrawCircleV(e->p, e->r,    (Color){255,60,200, (unsigned char)(150 + 80*pulse)});
        DrawCircleV(e->p, e->r*0.5f, (Color){255,255,255,120});
    }
}

static void DrawOrb(const Orb *o, Color glowC, const char *label)
{
    float p = (sinf(o->pulse)+1.0f)*0.5f;
    float glow = o->r + 14 + p*8;

    DrawCircleV(o->p, glow,      (Color){glowC.r, glowC.g, glowC.b, 26});
    DrawCircleV(o->p, o->r+6,    (Color){glowC.r, glowC.g, glowC.b, 70});
    DrawCircleV(o->p, o->r,      (Color){255,255,255,220});
    DrawText(label, (int)(o->p.x-22), (int)(o->p.y-42), 16, (Color){glowC.r, glowC.g, glowC.b, 210});
}

static void DrawHUD(const Game *g)
{
    char b[128];
    snprintf(b,sizeof(b),"Score: %d", g->score);
    DrawText(b, 20, 18, 26, (Color){0,255,255,220});

    snprintf(b,sizeof(b),"Best: %d", g->best);
    DrawText(b, 20, 48, 20, (Color){255,0,255,200});

    if(g->shield > 0){
        snprintf(b,sizeof(b),"Shield: %.1fs", g->shield);
        DrawText(b, 20, 74, 18, (Color){0,255,255,190});
    }

    DrawText("Move: WASD/Arrows | P: Pause | Pink=Score | Cyan=Shield", 20, H-28, 18, (Color){255,255,255,120});
}

static void DrawMenu(void)
{
    DrawText("NEON DRIFT COURIER", 180, 180, 54, (Color){0,255,255,240});
    DrawText("Press ENTER / SPACE to Start", 320, 260, 22, (Color){255,255,255,180});
    DrawText("Pink ORB = score | Cyan ORB = shield power-up", 260, 300, 18, (Color){255,0,255,180});
    DrawText("P = Pause (during game)", 380, 330, 18, (Color){255,255,255,140});
}

static void DrawPause(void)
{
    DrawRectangle(0,0,W,H,(Color){0,0,0,130});
    DrawText("PAUSED", 430, 230, 44, (Color){0,255,255,230});
    DrawText("Press P to resume", 400, 290, 20, (Color){255,255,255,180});
    DrawText("Press M for menu", 415, 320, 18, (Color){255,255,255,140});
}

static void DrawOver(const Game *g)
{
    DrawText("GAME OVER", 350, 180, 58, (Color){255,0,140,240});
    char b[128];
    snprintf(b,sizeof(b),"Score: %d   Best: %d", g->score, g->best);
    DrawText(b, 360, 250, 22, (Color){255,255,255,190});
    DrawText("Press ENTER / SPACE to Retry", 320, 300, 20, (Color){0,255,255,200});
    DrawText("Press M for Menu", 420, 330, 18, (Color){255,255,255,150});
}

// ----- Public API -----
void GameInit(Game *g)
{
    g->maxE = MAX_ENEMIES;
    g->partMax = MAX_PART;

    g->e = (Enemy*)MemAlloc(sizeof(Enemy) * g->maxE);
    g->part = (Particle*)MemAlloc(sizeof(Particle) * g->partMax);
    g->pl.trail = (Vector2*)MemAlloc(sizeof(Vector2) * TRAIL_LEN);

    g->partHead = 0;
    for(int i=0;i<g->partMax;i++) g->part[i].life = 0;

    // audio
    g->audioReady = 0;
    if (IsAudioDeviceReady()) {
        g->sPickup = MakeBeep(880.0f, 0.08f, 0.85f);
        g->sPower  = MakeBeep(520.0f, 0.12f, 0.85f);
        g->sHit    = MakeBeep(180.0f, 0.18f, 0.85f);
        g->audioReady = 1;
    }

    g->best = 0;
    g->t = 0;

    g->s = MENU;
    g->score = 0;
    g->flash = 0;
    g->shield = 0;
    g->shake = 0;
    g->zoomPulse = 0;
    g->slow = 0;

    ResetPlayer(g);
    SpawnEnemies(g);
    SpawnScoreOrb(g);
    SpawnPowerOrb(g);
}

void GameUpdate(Game *g, float dt)
{
    // cinematic slow motion
    if (g->slow > 0) {
        g->slow -= dt;
        if (g->slow < 0) g->slow = 0;
        dt *= 0.35f;
    }

    g->t += dt;

    if(g->flash > 0){ g->flash -= dt*2.6f; if(g->flash < 0) g->flash = 0; }
    if(g->shield > 0){ g->shield -= dt; if(g->shield < 0) g->shield = 0; }

    // decay FX
    g->shake -= g->shake * 10.0f * dt;
    if (g->shake < 0.05f) g->shake = 0;

    g->zoomPulse -= g->zoomPulse * 8.0f * dt;
    if (g->zoomPulse < 0.001f) g->zoomPulse = 0;

    UpdateParticles(g, dt);

    if(g->s==MENU){
        if(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) StartPlay(g);
        return;
    }

    if(g->s==OVER){
        if(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) StartPlay(g);
        if(IsKeyPressed(KEY_M)) g->s = MENU;
        return;
    }

    if(g->s==PAUSE){
        if(IsKeyPressed(KEY_P)) g->s = PLAY;
        if(IsKeyPressed(KEY_M)) g->s = MENU;
        return;
    }

    // PLAY
    if(IsKeyPressed(KEY_P)) { g->s = PAUSE; return; }

    UpdatePlayer(g, dt);
    UpdateEnemies(g, dt);
    UpdateOrbs(g, dt);

    if(HitPlayer(g)){
        if(g->shield > 0){
            // shield absorbs
            g->shield = 0;
            g->flash = 1.0f;
            g->shake = 14.0f;
            g->zoomPulse = 0.12f;
            g->slow = 0.18f;

            if(g->audioReady) PlaySound(g->sPower);
            Emit(g, g->pl.p, (Color){0,255,255,220}, 70, 340);
        } else {
            if(g->audioReady) PlaySound(g->sHit);
            Emit(g, g->pl.p, (Color){255,0,140,220}, 90, 360);

            g->flash = 1.0f;
            g->shake = 18.0f;
            g->zoomPulse = 0.16f;
            g->slow = 0.25f;

            if(g->score > g->best) g->best = g->score;
            g->s = OVER;
        }
    }
}

void GameDraw(Game *g)
{
    // Camera2D for cinematic feel (shake + zoom)
    Camera2D cam = {0};
    cam.target = (Vector2){ W*0.5f, H*0.5f };
    cam.offset = (Vector2){ W*0.5f, H*0.5f };

    float zx = 1.0f + g->zoomPulse;
    cam.zoom = zx;

    float sx = 0, sy = 0;
    if (g->shake > 0) {
        sx = randf(-g->shake, g->shake);
        sy = randf(-g->shake, g->shake);
    }
    cam.target.x += sx;
    cam.target.y += sy;

    BeginMode2D(cam);

    DrawBackground(g->t);

    if(g->s==MENU){
        EndMode2D();
        DrawMenu();
        return;
    }

    DrawOrb(&g->orb,   (Color){255,0,255,255}, "ORB");
    DrawOrb(&g->power, (Color){0,255,255,255}, "CYAN");
    DrawEnemies(g);
    DrawParticles(g);
    DrawPlayer(g);

    EndMode2D();

    // HUD in screen space (not affected by camera)
    DrawHUD(g);

    if(g->s==PAUSE) DrawPause();
    if(g->s==OVER)  DrawOver(g);

    if(g->flash > 0){
        DrawRectangle(0,0,W,H,(Color){0,255,255,(unsigned char)(g->flash*70)});
    }
}

void GameUnload(Game *g)
{
    if(g->audioReady){
        UnloadSound(g->sPickup);
        UnloadSound(g->sPower);
        UnloadSound(g->sHit);
        g->audioReady = 0;
    }

    if (g->pl.trail) MemFree(g->pl.trail);
    if (g->e) MemFree(g->e);
    if (g->part) MemFree(g->part);

    g->pl.trail = NULL;
    g->e = NULL;
    g->part = NULL;
}
