#include "person_animation.h"
#include <GL/glut.h>
#include <cmath>
#include <queue>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---- Layout constants (must match smart_home_pro.cpp) --------------
static const float HX = 20.f, HY = 110.f;
static const float HW = 720.f, HH = 370.f;
static const float MX = HX + HW * 0.5f;
static const float MY = HY + HH * 0.5f;

// ---- Externals (non-const globals only) ----------------------------
extern bool  lightOn[4];
extern float lightBright[4];
extern bool  fanOn[2];        // 0=Kitchen, 1=Bathroom
extern float fanSpeed[2];
extern bool  acOn;
extern bool  tvOn;
extern bool  doorOpen[2];     // 0=Front, 1=Garage/Back

// ---- Internal state ------------------------------------------------
struct Vec2 { float x, y; };

static Vec2  pPos    = {0,0};
static Vec2  pTarget = {0,0};
static bool  pActive = false;
static int   pCurrentRoom = -1;
static std::queue<int> pQueue;
static float pWalkPhase = 0.f;
static float pSpeed = 90.f;     // px / sec

static bool  pOutside    = true;
static int   pExitChoice = 0;   // 0 = front, 1 = garage/back

// Outside spawn / despawn points
static Vec2 frontOutside() { return { HX - 60.f, HY + 30.f }; }
static Vec2 backOutside()  { return { HX + HW + 60.f, HY + 30.f }; }

// Room centers (must match drawHouse layout)
static Vec2 roomCenter(int room) {
    float rw = HW/2.f, rh = HH/2.f;
    // 0 Living top-left, 1 Bedroom top-right,
    // 2 Kitchen bottom-left, 3 Bathroom bottom-right
    switch(room){
        case 0: return { HX + rw*0.5f, MY + rh*0.5f };
        case 1: return { MX + rw*0.5f, MY + rh*0.5f };
        case 2: return { HX + rw*0.5f, HY + rh*0.5f };
        case 3: return { MX + rw*0.5f, HY + rh*0.5f };
    }
    return {HX, HY};
}

// ---- Per-room AUTO-ON ----------------------------------------------
// Comment / uncomment lines inside each block to control what
// turns on when the person enters that room.
static void applyRoomAutoOn(int room) {
    switch(room){
        case 0: // LIVING ROOM
            lightOn[0] = true;
            tvOn       = true;
            // acOn    = true;
            break;
        case 1: // BEDROOM
            lightOn[1] = true;
            acOn       = true;
            // tvOn   = true;
            break;
        case 2: // KITCHEN
            lightOn[2] = true;
            fanOn[0]   = true;
            // tvOn = true;
            break;
        case 3: // BATHROOM
            lightOn[3] = true;
            fanOn[1]   = true;
            break;
    }
}

// ---- Per-room AUTO-OFF ---------------------------------------------
// Comment any line you want to KEEP on after he leaves the room.
static void applyRoomAutoOff(int room) {
    switch(room){
        case 0: // LIVING ROOM
            lightOn[0] = false;
            tvOn       = false;
            break;
        case 1: // BEDROOM
            lightOn[1] = false;
            acOn       = false;
            break;
        case 2: // KITCHEN
            lightOn[2] = false;
            fanOn[0]   = false;
            break;
        case 3: // BATHROOM
            lightOn[3] = false;
            fanOn[1]   = false;
            break;
    }
}

// ---- Public API ----------------------------------------------------
void personInit() {
    pPos        = frontOutside();
    pTarget     = pPos;
    pActive     = false;
    pCurrentRoom= -1;
    pOutside    = true;
    pExitChoice = 0;
    while(!pQueue.empty()) pQueue.pop();
}

void personGoToRoom(int room) {
    if(room < 0 || room > 3) return;
    pQueue.push(room);
    if(!pActive){
        // Open the front door ONLY if entering from outside
        if(pOutside){
            doorOpen[0] = true;
            pExitChoice = 0;
        }
        pTarget = roomCenter(pQueue.front());
        pActive = true;
    }
}

// exit: 0 = front door, 1 = garage / back door
void personLeaveHome(int exit) {
    pExitChoice = (exit == 1) ? 1 : 0;
    pQueue.push(-1);  // sentinel = "leave the house"
    if(!pActive){
        pActive = true;
        pTarget = (pExitChoice == 1) ? backOutside() : frontOutside();
        if(pExitChoice == 1) doorOpen[1] = true;
        else                 doorOpen[0] = true;
    }
}

bool personIsBusy() { return pActive || !pQueue.empty(); }

void personUpdate(float dt) {
    if(!pActive) return;

    float dx = pTarget.x - pPos.x;
    float dy = pTarget.y - pPos.y;
    float dist = sqrtf(dx*dx + dy*dy);

    if(dist < 2.f){
        // Arrived at current target
        if(!pQueue.empty()){
            int room = pQueue.front();
            pQueue.pop();

            if(room == -1){
                // Leaving the house
                if(pCurrentRoom != -1) applyRoomAutoOff(pCurrentRoom);
                pCurrentRoom = -1;
                pOutside = true;
                pActive = false;
                if(pExitChoice == 1) doorOpen[1] = false;
                else                 doorOpen[0] = false;
                return;
            }

            // Just stepped inside the house — close entry door behind him
            if(pOutside){
                doorOpen[0] = false;
                pOutside = false;
            }
            // Auto-off old room before entering a new one
            if(pCurrentRoom != -1 && pCurrentRoom != room)
                applyRoomAutoOff(pCurrentRoom);

            pCurrentRoom = room;
            applyRoomAutoOn(room);
        }

        // Set next target
        if(!pQueue.empty()){
            int next = pQueue.front();
            if(next == -1){
                pTarget = (pExitChoice == 1) ? backOutside() : frontOutside();
                if(pExitChoice == 1) doorOpen[1] = true;
                else                 doorOpen[0] = true;
            } else {
                pTarget = roomCenter(next);
            }
        } else {
            pActive = false;   // idle inside the house
        }
        return;
    }

    float step = pSpeed * dt;
    if(step > dist) step = dist;
    pPos.x += dx/dist * step;
    pPos.y += dy/dist * step;
    pWalkPhase += dt * 8.f;
}

// ---- Drawing -------------------------------------------------------
static void fillCirc(float cx, float cy, float r){
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for(int i = 0; i <= 24; i++){
        float a = 2.f * M_PI * i / 24.f;
        glVertex2f(cx + r*cosf(a), cy + r*sinf(a));
    }
    glEnd();
}

void personDraw() {
    float x = pPos.x, y = pPos.y;
    float swing = sinf(pWalkPhase) * (pActive ? 4.f : 0.f);

    // Shadow
    glColor4f(0, 0, 0, 0.35f);
    fillCirc(x, y - 14, 8);

    // Legs
    glColor3f(0.15f, 0.20f, 0.45f);
    glBegin(GL_QUADS);
      glVertex2f(x-5+swing, y-12); glVertex2f(x-1+swing, y-12);
      glVertex2f(x-1+swing, y-2);  glVertex2f(x-5+swing, y-2);
      glVertex2f(x+1-swing, y-12); glVertex2f(x+5-swing, y-12);
      glVertex2f(x+5-swing, y-2);  glVertex2f(x+1-swing, y-2);
    glEnd();

    // Body
    glColor3f(0.20f, 0.55f, 0.85f);
    glBegin(GL_QUADS);
      glVertex2f(x-7, y-2);  glVertex2f(x+7, y-2);
      glVertex2f(x+7, y+10); glVertex2f(x-7, y+10);
    glEnd();

    // Head
    glColor3f(0.95f, 0.80f, 0.65f);
    fillCirc(x, y + 15, 6);

    // Hair
    glColor3f(0.15f, 0.10f, 0.08f);
    glBegin(GL_TRIANGLE_FAN);
      glVertex2f(x, y + 18);
      for(int i = 0; i <= 12; i++){
        float a = M_PI * i / 12.f;
        glVertex2f(x + 6*cosf(a), y + 15 + 6*sinf(a));
      }
    glEnd();

    // Arms (swing opposite to legs)
    glColor3f(0.95f, 0.80f, 0.65f);
    glLineWidth(3.f);
    glBegin(GL_LINES);
      glVertex2f(x-7, y+8); glVertex2f(x-10 - swing*0.6f, y+0);
      glVertex2f(x+7, y+8); glVertex2f(x+10 + swing*0.6f, y+0);
    glEnd();
    glLineWidth(1.f);
}