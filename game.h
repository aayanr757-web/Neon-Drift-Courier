#ifndef GAME_H
#define GAME_H

#include "raylib.h"

#define W 1000
#define H 600

typedef enum { MENU, PLAY, PAUSE, OVER } State;

typedef struct {
    Vector2 p, v;
    float r;
    Vector2 *trail;
    int trailLen;
    int ti;
} Player;

typedef struct {
    Vector2 p, v;
    float r;
    float phase;
} Enemy;

typedef struct {
    Vector2 p;
    float r;
    float pulse;
} Orb;

typedef struct {
    Vector2 p, v;
    float r;
    float life;
    Color c;
} Particle;

typedef struct {
    State s;

    Player pl;

    Enemy *e;
    int nE;
    int maxE;

    Orb orb;       // score orb
    Orb power;     // shield orb

    int score, best;
    float t;

    // FX
    float flash;
    float shield;      // seconds
    float shake;       // screen shake intensity
    float zoomPulse;   // camera zoom pulse
    float slow;        // slow-mo timer (seconds)

    // particles
    Particle *part;
    int partMax;
    int partHead;

    // audio
    int audioReady;
    Sound sPickup;
    Sound sHit;
    Sound sPower;

} Game;

void GameInit(Game *g);
void GameUpdate(Game *g, float dt);
void GameDraw(Game *g);
void GameUnload(Game *g);

#endif
