#include <GL/glut.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>
#include <algorithm>


//  WINDOW & LAYOUT :

const int WIN_W = 1100;
const int WIN_H = 740;

// House geometry
const float HX  = 20.f,  HY  = 110.f;
const float HW  = 720.f, HH  = 370.f;
const float MX  = HX + HW * 0.5f;   // mid vertical
const float MY  = HY + HH * 0.5f;   // mid horizontal

// Right sidebar
const float SB_X = HX + HW + 20.f;
const float SB_W = WIN_W - SB_X - 10.f;


//  GLOBAL TIME / ANIMATION :

float gTime      = 0.f;   // seconds elapsed
bool  nightMode  = false;


//  WEATHER :

enum WeatherType { WEATHER_SUNNY=0, WEATHER_CLOUDY, WEATHER_RAINY };
WeatherType weather = WEATHER_SUNNY;

// Rain drops
struct Drop { float x, y, speed; };
std::vector<Drop> rainDrops;

void initRain() {
    rainDrops.clear();
    for (int i = 0; i < 80; i++) {
        Drop d;
        d.x     = HX + (float)(rand() % (int)HW);
        d.y     = HY + (float)(rand() % (int)(HH + 100));
        d.speed = 3.f + (rand() % 40) * 0.1f;
        rainDrops.push_back(d);
    }
}


//  CHIMNEY SMOKE PARTICLES :

struct Particle { float x, y, vx, vy, life, maxLife, r; };
std::vector<Particle> smoke;

void spawnSmoke(float cx, float cy) {
    if (smoke.size() > 40) return;
    Particle p;
    p.x = cx + (rand()%6 - 3);
    p.y = cy;
    p.vx = (rand()%10 - 5) * 0.05f;
    p.vy = 0.6f + (rand()%10) * 0.05f;
    p.life = 0.f;
    p.maxLife = 60.f + rand()%40;
    p.r = 4.f + rand()%6;
    smoke.push_back(p);
}


//  DEVICE STATES :

// Lights: 0=Living, 1=Bedroom, 2=Kitchen, 3=Bathroom
bool  lightOn[4]    = {false,false,false,false};
float lightBright[4]= {1.f,1.f,1.f,1.f};  // 0.2–1.0

// Fans: 0=Living, 1=Kitchen
bool  fanOn[2]    = {false,false};
float fanSpeed[2] = {3.f,3.f};
float fanAngle[2] = {0.f,0.f};

// Doors: 0=Front, 1=Garage
bool  doorOpen[2]   = {false,false};
float doorAnim[2]   = {0.f,0.f};  // 0=closed, 1=open (interpolated)

// AC
bool  acOn        = false;
float acTemp      = 24.f;   // °C  (16–30)
float roomTemp[4] = {28.f,28.f,28.f,28.f}; // ambient room temps

// TV
bool  tvOn        = false;
float tvFlicker   = 0.f;

// Alarm
bool  alarmOn     = false;
float alarmPulse  = 0.f;

// Energy (fake watt consumption)
float totalWatts  = 0.f;

// Hover state for buttons
int   hoverBtn = -1;

// Active slider: 0=none, 1-4=light bright, 5=fan0spd, 6=fan1spd, 7=acTemp
int   activeSlider = 0;


//  UTILITY: compute power consumption :

void updatePower() {
    totalWatts = 0;
    for (int i=0;i<4;i++) if(lightOn[i]) totalWatts += 10.f * lightBright[i];
    for (int i=0;i<2;i++) if(fanOn[i])   totalWatts += 35.f * (fanSpeed[i]/5.f);
    if (acOn)  totalWatts += 900.f;
    if (tvOn)  totalWatts += 120.f;
    if (alarmOn) totalWatts += 5.f;
}


//  COLOUR HELPERS :

void col(float r,float g,float b,float a=1.f){ glColor4f(r,g,b,a); }

// Lerp colour
void colLerp(float r0,float g0,float b0, float r1,float g1,float b1, float t, float a=1.f){
    col(r0+(r1-r0)*t, g0+(g1-g0)*t, b0+(b1-b0)*t, a);
}


//  PRIMITIVE DRAWING :

void fillCircle(float cx,float cy,float r,int seg=48){
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx,cy);
    for(int i=0;i<=seg;i++){
        float a=2.f*M_PI*i/seg;
        glVertex2f(cx+r*cosf(a),cy+r*sinf(a));
    }
    glEnd();
}
void lineCircle(float cx,float cy,float r,int seg=48){
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<seg;i++){
        float a=2.f*M_PI*i/seg;
        glVertex2f(cx+r*cosf(a),cy+r*sinf(a));
    }
    glEnd();
}
void fillRect(float x,float y,float w,float h){
    glBegin(GL_QUADS);
    glVertex2f(x,y); glVertex2f(x+w,y);
    glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}
void lineRect(float x,float y,float w,float h){
    glBegin(GL_LINE_LOOP);
    glVertex2f(x,y); glVertex2f(x+w,y);
    glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}
void fillTri(float x1,float y1,float x2,float y2,float x3,float y3){
    glBegin(GL_TRIANGLES);
    glVertex2f(x1,y1);glVertex2f(x2,y2);glVertex2f(x3,y3);
    glEnd();
}
// Rounded rect (approximate with circles at corners)
void fillRoundRect(float x,float y,float w,float h,float r){
    fillRect(x+r,y,w-2*r,h);
    fillRect(x,y+r,r,h-2*r);
    fillRect(x+w-r,y+r,r,h-2*r);
    fillCircle(x+r,   y+r,   r);
    fillCircle(x+w-r, y+r,   r);
    fillCircle(x+r,   y+h-r, r);
    fillCircle(x+w-r, y+h-r, r);
}

void drawText(float x,float y,const char* s,void* font=GLUT_BITMAP_HELVETICA_12){
    glRasterPos2f(x,y);
    for(const char*c=s;*c;c++) glutBitmapCharacter(font,*c);
}
void drawTextLarge(float x,float y,const char* s){
    drawText(x,y,s,GLUT_BITMAP_HELVETICA_18);
}
void drawTextSmall(float x,float y,const char* s){
    drawText(x,y,s,GLUT_BITMAP_HELVETICA_10);
}


//  GLOW  (radial soft blend) :

void drawGlow(float cx,float cy,float r,float R,float G,float B,float peak=0.22f){
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    int N=10;
    for(int i=N;i>=1;i--){
        float t=(float)i/N;
        glColor4f(R,G,B,peak*(1.f-t));
        fillCircle(cx,cy,r*t);
    }
    glDisable(GL_BLEND);
}


//  SKY / BACKGROUND :

void drawSky(){
    // Gradient sky
    float t = nightMode ? 1.f : 0.f;
    glBegin(GL_QUADS);
    // top
    colLerp(0.53f,0.81f,0.98f, 0.04f,0.06f,0.14f, t);
    glVertex2f(0, WIN_H);
    glVertex2f(WIN_W, WIN_H);
    // bottom
    colLerp(0.87f,0.95f,1.f, 0.06f,0.10f,0.20f, t);
    glVertex2f(WIN_W, HY+HH+5);
    glVertex2f(0, HY+HH+5);
    glEnd();

    // Ground
    if(nightMode) col(0.05f,0.10f,0.07f);
    else          col(0.35f,0.58f,0.28f);
    fillRect(0, 0, WIN_W, HY);

    // Sun or Moon
    if(!nightMode){
        col(1.f,0.92f,0.3f);
        fillCircle(WIN_W-80, WIN_H-60, 30);
        drawGlow(WIN_W-80, WIN_H-60, 55, 1.f,0.9f,0.3f,0.12f);
    } else {
        col(0.95f,0.95f,0.85f);
        fillCircle(WIN_W-80, WIN_H-60, 22);
        // crescent cut
        col(0.04f,0.06f,0.14f);
        fillCircle(WIN_W-72, WIN_H-55, 18);
        // Stars
        srand(42);
        col(1.f,1.f,1.f,0.9f);
        for(int i=0;i<50;i++){
            float sx=(float)(rand()%(WIN_W-200));
            float sy=HY+HH+30+(float)(rand()%200);
            float sr=0.5f+(rand()%3)*0.5f;
            float flicker=0.6f+0.4f*sinf(gTime*2.f+i);
            col(1.f,1.f,0.9f,flicker);
            fillCircle(sx,sy,sr,6);
        }
        srand((int)time(nullptr)); // restore
    }

    // Weather
    if(weather==WEATHER_CLOUDY || weather==WEATHER_RAINY){
        for(int i=0;i<3;i++){
            float cx=150.f+i*200.f + 30.f*sinf(gTime*0.1f+i);
            float cy=WIN_H-80;
            float a=nightMode?0.45f:0.88f;
            col(0.85f,0.87f,0.9f,a);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
            fillCircle(cx,     cy,    28);
            fillCircle(cx+22,  cy+5,  22);
            fillCircle(cx-22,  cy+5,  22);
            fillCircle(cx+10,  cy+12, 20);
            fillCircle(cx-10,  cy+12, 20);
            glDisable(GL_BLEND);
        }
    }
    if(weather==WEATHER_RAINY){
        col(0.55f,0.75f,1.f,0.6f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(1.2f);
        glBegin(GL_LINES);
        for(auto&d:rainDrops){
            glVertex2f(d.x,   d.y);
            glVertex2f(d.x-2, d.y-12);
        }
        glEnd();
        glDisable(GL_BLEND);
        glLineWidth(1.f);
    }
}


//  LIGHT BULB :

void drawBulb(float cx,float cy,bool on,float bright=1.f){
    float bR=16.f;
    if(on){
        drawGlow(cx,cy-4,55*bright,1.f,0.9f,0.2f,0.25f*bright);
    }
    // Globe
    if(on){ float b=0.6f+0.4f*bright; col(b,b*0.95f,0.3f*b); }
    else  col(0.15f,0.22f,0.32f);
    fillCircle(cx,cy-4,bR);

    col(0.25f,0.4f,0.55f); glLineWidth(1.5f);
    lineCircle(cx,cy-4,bR);

    // Base
    if(on){ float b=0.7f*bright; col(b,b*0.8f,0.2f); }
    else  col(0.18f,0.28f,0.38f);
    glBegin(GL_QUADS);
    glVertex2f(cx-7,cy+bR-5); glVertex2f(cx+7,cy+bR-5);
    glVertex2f(cx+5,cy+bR+5); glVertex2f(cx-5,cy+bR+5);
    glEnd();

    // Filament
    glLineWidth(1.5f);
    if(on) col(1.f,0.8f,0.1f);
    else   col(0.28f,0.42f,0.55f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(cx-5,cy+bR-7);
    glVertex2f(cx-2,cy-3); glVertex2f(cx+2,cy-1);
    glVertex2f(cx+5,cy+bR-7);
    glEnd();

    // Rays
    if(on){
        float b=bright;
        col(1.f,0.92f,0.3f,0.7f*b);
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(1.2f);
        glBegin(GL_LINES);
        float angs[]={-90,-55,-125,0,-180,45,-45+180,22,-22+180};
        for(float ang:angs){
            float rad=ang*M_PI/180.f;
            glVertex2f(cx+(bR+3)*cosf(rad), cy-4+(bR+3)*sinf(rad));
            glVertex2f(cx+(bR+12)*cosf(rad),cy-4+(bR+12)*sinf(rad));
        }
        glEnd();
        glDisable(GL_BLEND);
    }
    glLineWidth(1.f);
}


//  FAN :

void drawFan(float cx,float cy,float ang,bool on,float spd=3.f){
    if(on) drawGlow(cx,cy,42,0.f,0.8f,1.f,0.18f);

    int nBlades=4;
    for(int i=0;i<nBlades;i++){
        float a=ang+i*(360.f/nBlades);
        glPushMatrix();
        glTranslatef(cx,cy,0);
        glRotatef(a,0,0,1);
        if(on){ float s=spd/5.f; col(0.f,0.5f+0.5f*s,0.8f+0.2f*s,0.85f); }
        else  col(0.18f,0.3f,0.44f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0,0);
        int sg=18; float bw=6.f,bh=19.f;
        for(int s=0;s<=sg;s++){
            float tt=(float)s/sg;
            glVertex2f(bw*cosf(tt*M_PI), -(bh+bh*sinf(tt*M_PI)));
        }
        glEnd();
        col(0.25f,0.45f,0.6f); glLineWidth(1.f);
        glBegin(GL_LINE_STRIP);
        for(int s=0;s<=sg;s++){
            float tt=(float)s/sg;
            glVertex2f(bw*cosf(tt*M_PI),-(bh+bh*sinf(tt*M_PI)));
        }
        glEnd();
        glPopMatrix();
    }
    // Rod
    col(0.2f,0.32f,0.44f);
    fillRect(cx-2,cy+18,4,10);
    // Hub
    if(on) col(0.f,0.9f,1.f);
    else   col(0.22f,0.38f,0.52f);
    fillCircle(cx,cy,7);
    col(0.1f,0.18f,0.26f);
    fillCircle(cx,cy,3.5f);
    glLineWidth(1.f);
}


//  DOOR  (animated via doorAnim float 0→1) :

void drawDoor(float x,float y,float w,float h,float anim,int side){
    // Frame
    col(0.12f,0.22f,0.34f); glLineWidth(2.5f);
    lineRect(x,y,w,h); glLineWidth(1.f);

    // Foreshortened panel width: anim=0 → w-4, anim=1 → w*0.15
    float pw = (w-4)*(1.f-anim) + (w*0.15f)*anim;
    float px2 = (side<0) ? x+2 : x+w-2-pw;

    // Panel fill
    if(anim>0.01f){
        // green tint proportional to open
        float g=anim;
        col(0.05f*(1-g)+0.02f*g, 0.12f*(1-g)+0.18f*g, 0.22f*(1-g)+0.12f*g);
    } else {
        col(0.07f,0.13f,0.22f);
    }
    fillRect(px2,y+2,pw,h-4);

    // Border
    if(anim>0.05f){ col(0.1f*anim,0.9f*anim+0.1f,0.35f*anim); }
    else          col(0.18f,0.32f,0.48f);
    glLineWidth(anim>0.05f?2.f:1.f);
    lineRect(px2,y+2,pw,h-4);
    glLineWidth(1.f);

    // Door panels (decorative insets) when mostly closed
    if(anim<0.5f){
        float alpha=1.f-anim*2.f;
        col(0.1f,0.2f,0.3f,alpha);
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        float iw=pw-8, ih=(h-20)/2.f;
        fillRect(px2+4, y+5,  iw, ih);
        fillRect(px2+4, y+5+ih+4, iw, ih);
        glDisable(GL_BLEND);
    }

    // Handle
    float hx2=(side<0)? px2+pw-7 : px2+5;
    if(anim>0.5f) col(0.1f,1.f,0.4f);
    else          col(0.3f,0.5f,0.65f);
    fillCircle(hx2, y+h/2.f, 3.5f);

    // Glow on frame when open
    if(anim>0.1f){
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        col(0.1f,1.f,0.4f,0.15f*anim);
        fillRect(x-3,y-3,w+6,h+6);
        glDisable(GL_BLEND);
    }
}


//  AIR CONDITIONER UNIT :

void drawAC(float x,float y,float w,float h,bool on,float temp){
    // Body
    if(on) col(0.06f,0.14f,0.28f);
    else   col(0.08f,0.13f,0.20f);
    fillRect(x,y,w,h);
    if(on) col(0.f,0.6f,1.f); else col(0.18f,0.3f,0.44f);
    glLineWidth(1.5f);lineRect(x,y,w,h);glLineWidth(1.f);

    // Vent slats
    col(0.12f,0.22f,0.34f);
    for(int i=0;i<4;i++){
        float sy=y+6+i*(h-12)/4.f;
        fillRect(x+6,sy,w-12,3);
    }

    // Indicator LED
    if(on){ col(0.f,0.9f,1.f); drawGlow(x+w-10,y+8,8,0.f,0.8f,1.f,0.3f); }
    else   col(0.2f,0.3f,0.4f);
    fillCircle(x+w-10,y+8,4);

    // Cold airflow lines when on
    if(on){
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        col(0.4f,0.85f,1.f,0.35f);
        glLineWidth(1.2f);
        for(int i=0;i<3;i++){
            float lx=x+10+i*12;
            float wave=4.f*sinf(gTime*2.f+i);
            glBegin(GL_LINE_STRIP);
            for(int j=0;j<8;j++){
                float jj=j*3.f;
                glVertex2f(lx+wave*sinf(j*0.8f), y+h+jj);
            }
            glEnd();
        }
        glDisable(GL_BLEND);
        glLineWidth(1.f);
    }

    // Temperature label
    char buf[16];
    if(on){ snprintf(buf,sizeof(buf),"%.0f°C",temp); col(0.f,0.9f,1.f); }
    else  { snprintf(buf,sizeof(buf),"OFF");          col(0.3f,0.45f,0.6f); }
    drawTextSmall(x+5,y+h-8,buf);
}


//  TELEVISION :

void drawTV(float x,float y,float w,float h,bool on,float t){
    // Outer bezel
    col(0.08f,0.12f,0.18f);
    fillRect(x,y,w,h);
    col(0.15f,0.25f,0.38f);
    glLineWidth(2.f);lineRect(x,y,w,h);glLineWidth(1.f);

    // Screen
    float sx=x+5,sy=y+5,sw=w-10,sh=h-18;
    if(on){
        // Flickering coloured screen
        float fl=0.85f+0.15f*sinf(t*7.3f);
        // Fake TV colour bars
        float barW=sw/7.f;
        float colours[7][3]={{1,1,1},{1,1,0},{0,1,1},{0,1,0},{1,0,1},{1,0,0},{0,0,1}};
        for(int i=0;i<7;i++){
            col(colours[i][0]*fl,colours[i][1]*fl,colours[i][2]*fl);
            fillRect(sx+i*barW,sy,barW,sh);
        }
        // Scan line
        float scanY=sy+fmod(t*60.f,sh);
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        col(1.f,1.f,1.f,0.12f);
        fillRect(sx,scanY,sw,3);
        glDisable(GL_BLEND);
    } else {
        col(0.04f,0.07f,0.12f);
        fillRect(sx,sy,sw,sh);
        // Reflection glint
        col(0.1f,0.18f,0.28f);
        fillRect(sx+5,sy+5,sw/3.f,3);
    }

    // Stand
    col(0.1f,0.16f,0.24f);
    fillRect(x+w/2.f-5, y-8, 10, 10);
    fillRect(x+w/2.f-15,y-10, 30, 4);

    // Power dot
    if(on) col(0.f,1.f,0.4f);
    else   col(0.3f,0.4f,0.5f);
    fillCircle(x+w-8,y+h-10,3);
}


//  ALARM INDICATOR :

void drawAlarm(float x,float y,bool on,float pulse){
    // Bell shape
    col(0.15f,0.25f,0.38f);
    fillCircle(x,y,16);
    if(on){
        float p=0.5f+0.5f*sinf(pulse*8.f);
        col(1.f,0.2f+0.4f*p,0.1f);
        drawGlow(x,y,30,1.f,0.2f,0.1f,0.3f*p);
    } else col(0.2f,0.35f,0.5f);
    fillCircle(x,y,14);

    // Bell shape detail
    col(0.1f,0.18f,0.28f);
    // Arc bottom
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x,y);
    for(int i=0;i<=12;i++){
        float a=M_PI+i*M_PI/12.f;
        glVertex2f(x+14*cosf(a),y+14*sinf(a));
    }
    glEnd();

    // Clapper
    if(on){ float shift=3.f*sinf(pulse*8.f); col(1.f,0.4f,0.1f); fillCircle(x+shift,y-8,4); }
    else  { col(0.2f,0.35f,0.5f); fillCircle(x,y-8,4); }

    // Label
    if(on) col(1.f,0.4f,0.1f);
    else   col(0.3f,0.5f,0.65f);
    drawTextSmall(x-12,y-22, on?"ALARM ON":"ALARM");
}


//  CHIMNEY SMOKE :

void drawSmoke(){
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for(auto&p:smoke){
        float life=p.life/p.maxLife;
        float a=0.25f*(1.f-life);
        float grey=0.5f+0.3f*life;
        col(grey,grey,grey,a);
        fillCircle(p.x,p.y,p.r*(0.5f+life));
    }
    glDisable(GL_BLEND);
}


//  HOUSE  (4 rooms) :

void drawHouse(){
    // ── Room fills ──
    // Room 0: Living (top-left), Room 1: Bedroom (top-right)
    // Room 2: Kitchen (bottom-left), Room 3: Bathroom (bottom-right)
    float rw=HW/2.f, rh=HH/2.f;
    struct RoomDef{ float x,y; int li; int fi; };
    RoomDef rooms[4]={
        {HX,      MY,     0,-1},
        {MX,      MY,     1,-1},
        {HX,      HY,     2, 0},
        {MX,      HY,     3, 1},
    };
    const char* rNames[4]={"LIVING ROOM","BEDROOM","KITCHEN","BATHROOM"};

    for(int i=0;i<4;i++){
        auto&r=rooms[i];
        // Base fill
        float ambient = nightMode ? 0.03f : 0.05f;
        float lo = lightOn[i] ? lightBright[i] : 0.f;
        col(ambient+0.10f*lo, ambient+0.08f*lo, ambient+0.03f*lo);
        fillRect(r.x, r.y, rw, rh);

        // Light glow overlay
        if(lightOn[i]){
            float cx2=r.x+rw/2.f, cy2=r.y+rh/2.f;
            drawGlow(cx2,cy2,rw*0.6f,1.f,0.88f,0.22f,0.20f*lightBright[i]);
        }
        // Fan glow overlay
        if(i==2&&fanOn[0]) drawGlow(r.x+rw/2.f,r.y+rh/2.f,rw*0.4f,0.f,0.8f,1.f,0.12f);
        if(i==3&&fanOn[1]) drawGlow(r.x+rw/2.f,r.y+rh/2.f,rw*0.4f,0.f,0.8f,1.f,0.12f);
    }

    // ── Outer walls ──
    col(0.12f,0.22f,0.35f); glLineWidth(3.f);
    lineRect(HX,HY,HW,HH); glLineWidth(1.f);

    // ── Internal walls ──
    col(0.10f,0.18f,0.28f); glLineWidth(1.8f);
    glBegin(GL_LINES);
    glVertex2f(MX,HY);     glVertex2f(MX,HY+HH);
    glVertex2f(HX,MY);     glVertex2f(HX+HW,MY);
    glEnd();
    glLineWidth(1.f);

    // ── Roof ──
    float rl=HX-25, rr=HX+HW+25, rb=HY+HH, rtop=rb+130;
    col(0.07f,0.13f,0.20f);
    fillTri(MX,rtop, rl,rb, rr,rb);
    col(0.14f,0.26f,0.40f); glLineWidth(2.5f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(rl,rb); glVertex2f(MX,rtop); glVertex2f(rr,rb);
    glEnd();
    glLineWidth(1.f);

    // Roof tiles (decorative lines)
    col(0.10f,0.18f,0.28f);
    glLineWidth(1.f);
    glBegin(GL_LINES);
    for(int i=1;i<=6;i++){
        float t=(float)i/7.f;
        float lx=rl+(MX-rl)*t, ly=rb+(rtop-rb)*t;
        float rx2=rr+(MX-rr)*t, ry2=rb+(rtop-rb)*t;
        glVertex2f(lx,ly); glVertex2f(rx2,ry2);
    }
    glEnd();

    // Chimney
    float chx=HX+HW*0.63f, chy=rb+30, chw=34, chh=70;
    col(0.09f,0.16f,0.25f);
    fillRect(chx,chy,chw,chh);
    col(0.16f,0.28f,0.42f);glLineWidth(1.5f);
    lineRect(chx,chy,chw,chh); glLineWidth(1.f);

    // Smoke from chimney
    drawSmoke();

    // Windows (decorative)
    auto drawWindow=[&](float wx,float wy,float ww,float wh,int room){
        float glow = lightOn[room]?lightBright[room]:0.f;
        if(glow>0.01f){ col(1.f,0.92f,0.5f,0.4f*glow); glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);fillRect(wx,wy,ww,wh);glDisable(GL_BLEND);}
        else if(nightMode){ col(0.05f,0.08f,0.15f);fillRect(wx,wy,ww,wh);}
        else { col(0.6f,0.85f,0.95f,0.5f);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);fillRect(wx,wy,ww,wh);glDisable(GL_BLEND);}
        col(0.18f,0.30f,0.44f);glLineWidth(1.5f);lineRect(wx,wy,ww,wh);
        // cross
        glBegin(GL_LINES);
        glVertex2f(wx+ww/2,wy);glVertex2f(wx+ww/2,wy+wh);
        glVertex2f(wx,wy+wh/2);glVertex2f(wx+ww,wy+wh/2);
        glEnd();glLineWidth(1.f);
    };
    // Two windows per upper-floor front (on roof area)
    drawWindow(HX+30,  rb+10, 50,40, 0);
    drawWindow(HX+HW-80, rb+10, 50,40, 1);

    // ── Room Labels ──
    for(int i=0;i<4;i++){
        auto&r=rooms[i];
        col(0.22f,0.38f,0.52f);
        drawTextSmall(r.x+6, r.y+rh-8, rNames[i]);
    }

    // ── Appliances in rooms ──

    // Bulbs (top-centre of each room)
    for(int i=0;i<4;i++){
        auto&r=rooms[i];
        drawBulb(r.x+rw/2.f, r.y+rh-36, lightOn[i], lightBright[i]);
    }

    // Fans (Kitchen=room2, Bathroom=room3)
    drawFan(rooms[2].x+rw/2.f, rooms[2].y+rh/2.f, fanAngle[0], fanOn[0], fanSpeed[0]);
    drawFan(rooms[3].x+rw/2.f, rooms[3].y+rh/2.f, fanAngle[1], fanOn[1], fanSpeed[1]);

    // AC unit on bedroom wall
    drawAC(rooms[1].x+10, rooms[1].y+rh/2.f-18, 70, 36, acOn, acTemp);

    // TV in Living Room
    drawTV(rooms[0].x+18, rooms[0].y+10, 80, 58, tvOn, gTime);

    // Alarm in upper-right corner
    drawAlarm(rooms[1].x+rw-22, rooms[1].y+22, alarmOn, alarmPulse);

    // ── Doors ──
    float dw=42, dh=68;
    // Front door: left wall of kitchen (room2)
    drawDoor(HX-dw, rooms[2].y+rh/2.f-dh/2.f, dw, dh, doorAnim[0], -1);
    // Garage door: right wall of bathroom (room3)
    drawDoor(HX+HW, rooms[3].y+rh/2.f-dh/2.f, dw, dh, doorAnim[1], +1);

    // Room temperatures
    for(int i=0;i<4;i++){
        auto&r=rooms[i];
        char tbuf[16]; snprintf(tbuf,sizeof(tbuf),"%.1f°C",roomTemp[i]);
        col(0.30f,0.52f,0.68f);
        drawTextSmall(r.x+6, r.y+12, tbuf);
    }
}


//  SIDEBAR PANEL :

struct Btn {
    float x,y,w,h;
    const char* label;
    bool* state;
    float onR,onG,onB;
    int   id;
};

// We build buttons dynamically in drawSidebar and store for hit-test
static Btn sideButtons[32];
static int nSideButtons=0;

// Slider descriptors
struct Slider{ float x,y,w; float*val; float mn,mx; const char*label; bool*active; int id; };
static Slider sliders[16];
static int nSliders=0;

void addBtn(float x,float y,float w,float h,const char*lbl,bool*st,float r,float g,float b,int id){
    if(nSideButtons>=32)return;
    sideButtons[nSideButtons++]={x,y,w,h,lbl,st,r,g,b,id};
}
void addSlider(float x,float y,float w,float*val,float mn,float mx,const char*lbl,bool*act,int id){
    if(nSliders>=16)return;
    sliders[nSliders++]={x,y,w,val,mn,mx,lbl,act,id};
}

void drawBtn(const Btn&b){
    bool on=*b.state;
    bool hov=(hoverBtn==b.id);

    // Shadow
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    col(0,0,0,0.3f); fillRect(b.x+2,b.y-2,b.w,b.h);
    glDisable(GL_BLEND);

    // Body
    if(on)       col(b.onR*0.22f,b.onG*0.22f,b.onB*0.22f);
    else if(hov) col(0.10f,0.18f,0.28f);
    else         col(0.07f,0.12f,0.20f);
    fillRect(b.x,b.y,b.w,b.h);

    // Border
    glLineWidth(on?2.f:1.5f);
    if(on)  col(b.onR,b.onG,b.onB);
    else    col(hov?0.3f:0.16f, hov?0.48f:0.28f, hov?0.65f:0.42f);
    lineRect(b.x,b.y,b.w,b.h);glLineWidth(1.f);

    // Label
    float lw=strlen(b.label)*6.5f;
    if(on) col(b.onR,b.onG,b.onB);
    else   col(hov?0.65f:0.42f,hov?0.8f:0.6f,hov?0.9f:0.72f);
    drawText(b.x+(b.w-lw)/2.f, b.y+b.h/2.f+4, b.label);

    // State badge
    const char*badge=on?"● ON":"○ OFF";
    float bw2=strlen(badge)*5.2f;
    if(on) col(b.onR,b.onG,b.onB,0.8f);
    else   col(0.28f,0.42f,0.56f);
    drawTextSmall(b.x+(b.w-bw2)/2.f, b.y+6, badge);
}

void drawSliderWidget(const Slider&s){
    bool active=*(s.active);
    float ratio=(*s.val - s.mn)/(s.mx - s.mn);

    // Track bg
    col(0.08f,0.14f,0.22f);
    fillRect(s.x, s.y+5, s.w, 5);
    // Fill
    if(active) col(0.f,0.75f,1.f);
    else        col(0.14f,0.26f,0.40f);
    fillRect(s.x, s.y+5, s.w*ratio, 5);

    // Knob
    float kx=s.x+s.w*ratio;
    if(active) col(0.f,0.9f,1.f);
    else        col(0.22f,0.42f,0.58f);
    fillCircle(kx,s.y+7.5f,8);
    col(0.06f,0.12f,0.20f);
    fillCircle(kx,s.y+7.5f,4);

    // Label
    col(0.3f,0.52f,0.68f);
    drawTextSmall(s.x, s.y-2, s.label);

    // Value
    char buf[12]; snprintf(buf,sizeof(buf),"%.1f",*s.val);
    col(active?0.f:0.3f, active?0.85f:0.5f, active?1.f:0.68f);
    drawTextSmall(s.x+s.w+5, s.y+10, buf);
}

void drawSidebar(){
    nSideButtons=0; nSliders=0;

    float x=SB_X, w=SB_W-5;
    float y=WIN_H-14;

    // Panel bg
    col(0.05f,0.09f,0.15f);
    fillRect(x-8,0,SB_W+8,WIN_H);
    col(0.10f,0.18f,0.28f);glLineWidth(1.5f);
    glBegin(GL_LINES);glVertex2f(x-8,0);glVertex2f(x-8,WIN_H);glEnd();
    glLineWidth(1.f);

    // Title
    col(0.f,0.75f,1.f);
    drawTextLarge(x, y-4,"CONTROLS");
    y-=22;
    col(0.12f,0.22f,0.35f);
    fillRect(x,y,w,1);
    y-=14;

    float bw=(w-4)/2.f, bh=26.f;

    // ── Lights ──
    col(0.3f,0.52f,0.68f);drawTextSmall(x,y,"── LIGHTS ──"); y-=16;
    const char*lnames[4]={"Living Rm","Bedroom","Kitchen","Bathroom"};
    for(int i=0;i<4;i++){
        float bx=x + (i%2)*(bw+4);
        if(i%2==0 && i>0) y-=bh+4;
        addBtn(bx,y-bh,bw,bh,lnames[i],&lightOn[i],1.f,0.88f,0.15f,10+i);
        drawBtn(sideButtons[nSideButtons-1]);
    }
    y-=bh+8;

    // Brightness sliders
    col(0.25f,0.42f,0.58f);drawTextSmall(x,y,"Brightness"); y-=18;
    for(int i=0;i<4;i++){
        addSlider(x,y-8,(w-14)/4.f*1.f,&lightBright[i],0.2f,1.f,"",&lightOn[i],20+i);
        // draw with mini label
        Slider&sl=sliders[nSliders-1];
        float ratio=(lightBright[i]-0.2f)/0.8f;
        col(lightOn[i]?0.08f:0.05f, lightOn[i]?0.14f:0.09f, lightOn[i]?0.22f:0.15f);
        fillRect(sl.x,sl.y+3,sl.w,4);
        if(lightOn[i]) col(1.f,0.88f,0.15f); else col(0.14f,0.26f,0.40f);
        fillRect(sl.x,sl.y+3,sl.w*ratio,4);
        if(lightOn[i]) col(1.f,0.88f,0.15f); else col(0.22f,0.42f,0.58f);
        fillCircle(sl.x+sl.w*ratio,sl.y+5,5);
        char buf[4]; snprintf(buf,sizeof(buf),"%d",(int)(lightBright[i]*100));
        col(0.3f,0.5f,0.65f); drawTextSmall(sl.x,sl.y-4,lnames[i]);
        sl.x+=(w-14)/4.f+4;
        // re-register correct x
        sliders[nSliders-1].x=x+(i)*((w)/4.f);
    }
    y-=18;

    col(0.12f,0.22f,0.35f);fillRect(x,y,w,1);y-=12;

    // ── Fans ──
    col(0.3f,0.52f,0.68f);drawTextSmall(x,y,"── FANS ──"); y-=16;
    addBtn(x,      y-bh, bw,bh,"Kitchen Fan",&fanOn[0],0.f,0.8f,1.f,30);
    addBtn(x+bw+4, y-bh, bw,bh,"Bathroom Fan",&fanOn[1],0.f,0.8f,1.f,31);
    drawBtn(sideButtons[nSideButtons-2]);
    drawBtn(sideButtons[nSideButtons-1]);
    y-=bh+6;

    // Fan speed sliders
    col(0.25f,0.42f,0.58f);drawTextSmall(x,y,"Fan Speed"); y-=16;
    addSlider(x,       y-4, (w-8)/2.f,&fanSpeed[0],1.f,5.f,"Kitchen",&fanOn[0],40);
    addSlider(x+(w+4)/2.f, y-4, (w-8)/2.f,&fanSpeed[1],1.f,5.f,"Bath",&fanOn[1],41);
    drawSliderWidget(sliders[nSliders-2]);
    drawSliderWidget(sliders[nSliders-1]);
    y-=22;

    col(0.12f,0.22f,0.35f);fillRect(x,y,w,1);y-=12;

    // ── Doors ──
    col(0.3f,0.52f,0.68f);drawTextSmall(x,y,"── DOORS ──"); y-=16;
    addBtn(x,      y-bh, bw,bh,"Front Door",&doorOpen[0],0.1f,1.f,0.4f,50);
    addBtn(x+bw+4, y-bh, bw,bh,"Garage Door",&doorOpen[1],0.1f,1.f,0.4f,51);
    drawBtn(sideButtons[nSideButtons-2]);
    drawBtn(sideButtons[nSideButtons-1]);
    y-=bh+8;

    col(0.12f,0.22f,0.35f);fillRect(x,y,w,1);y-=12;

    // ── Appliances ──
    col(0.3f,0.52f,0.68f);drawTextSmall(x,y,"── APPLIANCES ──"); y-=16;
    addBtn(x,      y-bh, bw,bh,"Air Cond.",&acOn,0.f,0.7f,1.f,60);
    addBtn(x+bw+4, y-bh, bw,bh,"Television",&tvOn,0.6f,0.3f,1.f,61);
    drawBtn(sideButtons[nSideButtons-2]);
    drawBtn(sideButtons[nSideButtons-1]);
    y-=bh+6;

    // AC temp slider
    col(0.25f,0.42f,0.58f);drawTextSmall(x,y,"AC Temp (°C)"); y-=16;
    addSlider(x,y-4,w-14,&acTemp,16.f,30.f,"",&acOn,70);
    drawSliderWidget(sliders[nSliders-1]);
    y-=22;

    col(0.12f,0.22f,0.35f);fillRect(x,y,w,1);y-=12;

    // ── Security ──
    col(0.3f,0.52f,0.68f);drawTextSmall(x,y,"── SECURITY ──"); y-=16;
    addBtn(x,y-bh,w,bh,"Security Alarm",&alarmOn,1.f,0.3f,0.1f,80);
    drawBtn(sideButtons[nSideButtons-1]);
    y-=bh+8;

    col(0.12f,0.22f,0.35f);fillRect(x,y,w,1);y-=12;

    // ── Scene ──
    col(0.3f,0.52f,0.68f);drawTextSmall(x,y,"── SCENE ──"); y-=16;
    addBtn(x,      y-bh, bw,bh,"Night Mode",&nightMode,0.4f,0.3f,0.9f,90);
    // Weather cycle button (single button, cycles)
    bool dummyW=false;
    addBtn(x+bw+4, y-bh, bw,bh,"Weather",&dummyW,0.3f,0.8f,0.5f,91);
    drawBtn(sideButtons[nSideButtons-2]);
    // Weather btn special draw
    {
        Btn b=sideButtons[nSideButtons-1];
        // custom label based on current weather
        const char*wlbl[3]={"☀ Sunny","⛅ Cloudy","🌧 Rainy"};
        col(0.07f,0.12f,0.20f);fillRect(b.x,b.y,b.w,b.h);
        col(0.16f,0.28f,0.42f);glLineWidth(1.5f);lineRect(b.x,b.y,b.w,b.h);glLineWidth(1.f);
        float lw2=strlen(wlbl[weather])*6.2f;
        col(0.3f,0.8f,0.5f);
        drawText(b.x+(b.w-lw2)/2.f,b.y+b.h/2.f+4,wlbl[weather]);
    }
    y-=bh+8;

    // ── Master ──
    col(0.12f,0.22f,0.35f);fillRect(x,y,w,1);y-=12;
    bool allOn=lightOn[0]&&lightOn[1]&&lightOn[2]&&lightOn[3]&&fanOn[0]&&fanOn[1];
    bool allOff=!lightOn[0]&&!lightOn[1]&&!lightOn[2]&&!lightOn[3]&&!fanOn[0]&&!fanOn[1];
    addBtn(x,      y-bh, bw,bh,"ALL ON",&allOn,0.f,1.f,0.5f,100);
    addBtn(x+bw+4, y-bh, bw,bh,"ALL OFF",&allOff,1.f,0.3f,0.2f,101);
    drawBtn(sideButtons[nSideButtons-2]);
    drawBtn(sideButtons[nSideButtons-1]);
}


//  TOP STATUS BAR :

void drawStatusBar(){
    float bx=HX, by=WIN_H-88, bw2=HW, bh=50;

    // Panel
    col(0.05f,0.09f,0.15f);
    fillRect(bx,by,bw2,bh);
    col(0.10f,0.20f,0.32f);
    glLineWidth(1.5f);lineRect(bx,by,bw2,bh);glLineWidth(1.f);

    // Title
    col(0.f,0.78f,1.f);
    drawTextLarge(bx+10, by+bh-14,"SMART HOME AUTOMATION SYSTEM");

    // Clock (simulated)
    time_t now=time(nullptr);
    struct tm*tm2=localtime(&now);
    char clk[32];
    strftime(clk,sizeof(clk),"%H:%M:%S  %a %d %b",tm2);
    col(0.4f,0.68f,0.8f);
    drawText(bx+bw2-200, by+bh-14, clk);

    // Active device count + power
    int active=0;
    for(int i=0;i<4;i++) if(lightOn[i]) active++;
    for(int i=0;i<2;i++) if(fanOn[i])   active++;
    if(acOn) active++; if(tvOn) active++; if(alarmOn) active++;

    updatePower();
    char info[64];
    snprintf(info,sizeof(info),"Devices ON: %d/9   Power: %.0f W",active,totalWatts);
    col(0.35f,0.62f,0.75f);
    drawText(bx+10,by+12,info);

    // Energy bar
    float maxW=1300.f;
    float ratio=totalWatts/maxW;
    col(0.08f,0.15f,0.24f);fillRect(bx+200,by+8,bw2-380,8);
    // colour: green→yellow→red
    if(ratio<0.5f)      col(0.1f,0.9f,0.3f);
    else if(ratio<0.8f) col(1.f,0.75f,0.1f);
    else                col(1.f,0.2f,0.1f);
    fillRect(bx+200,by+8,(bw2-380)*std::min(ratio,1.f),8);
    col(0.18f,0.32f,0.48f);lineRect(bx+200,by+8,bw2-380,8);

    // Weather & day/night icon
    col(0.3f,0.55f,0.7f);
    const char*wstr[3]={"Sunny","Cloudy","Rainy"};
    char wbuf[32]; snprintf(wbuf,sizeof(wbuf),"Weather: %s  |  %s",wstr[weather],nightMode?"Night":"Day");
    drawTextSmall(bx+bw2-200, by+12, wbuf);
}


//  KEYBOARD HINT BAR :

void drawHint(){
    col(0.14f,0.24f,0.36f);
    fillRect(0,0,WIN_W,18);
    col(0.2f,0.35f,0.5f);
    drawTextSmall(6,6,"1-4:Lights  5-6:Fans  7:Door1  8:Door2  9:AC  T:TV  A:Alarm  D/N:Day/Night  W:Weather  Q:Quit  All-On:0  All-Off:-");
}


//  DISPLAY :

void display(){
    glClearColor(0.04f,0.07f,0.12f,1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    drawSky();
    drawHouse();
    drawStatusBar();
    drawSidebar();
    drawHint();

    glutSwapBuffers();
}


//  TIMER  ~60 FPS :

void onTimer(int){
    gTime += 0.016f;

    // Fan animation
    for(int i=0;i<2;i++){
        if(fanOn[i]) fanAngle[i]+=fanSpeed[i]*2.8f;
        if(fanAngle[i]>=360.f) fanAngle[i]-=360.f;
    }

    // Door animation (smooth lerp)
    for(int i=0;i<2;i++){
        float target=doorOpen[i]?1.f:0.f;
        doorAnim[i]+=(target-doorAnim[i])*0.07f;
        if(doorAnim[i]<0.001f)doorAnim[i]=0.f;
        if(doorAnim[i]>0.999f)doorAnim[i]=1.f;
    }

    // Alarm pulse
    if(alarmOn) alarmPulse+=0.016f;

    // Chimney smoke
    float chx=HX+HW*0.63f+17;
    float chy=HY+HH+97;
    if((int)(gTime*10)%3==0) spawnSmoke(chx,chy);
    for(auto&p:smoke){ p.x+=p.vx; p.y+=p.vy; p.life+=1.f; }
    smoke.erase(std::remove_if(smoke.begin(),smoke.end(),[](const Particle&p){return p.life>=p.maxLife;}),smoke.end());

    // Rain drops
    if(weather==WEATHER_RAINY){
        for(auto&d:rainDrops){
            d.y-=d.speed;
            if(d.y<HY-10){ d.y=HY+HH+20; d.x=HX+(float)(rand()%(int)HW); }
        }
    }

    // Room temperature (AC cools rooms)
    for(int i=0;i<4;i++){
        float target = acOn ? acTemp : (nightMode?24.f:30.f);
        roomTemp[i]+=(target-roomTemp[i])*0.001f;
    }

    // TV flicker
    tvFlicker+=0.016f;

    glutPostRedisplay();
    glutTimerFunc(16,onTimer,0);
}


//  MASTER ON / OFF helpers :

void setAll(bool v){
    for(int i=0;i<4;i++) lightOn[i]=v;
    for(int i=0;i<2;i++) fanOn[i]=v;
    acOn=v; tvOn=v;
}


//  MOUSE :

void onMouse(int btn,int state,int mx,int my){
    float x=(float)mx, y=(float)(WIN_H-my);
    if(btn==GLUT_LEFT_BUTTON && state==GLUT_DOWN){
        // Buttons
        for(int i=0;i<nSideButtons;i++){
            Btn&b=sideButtons[i];
            if(x>=b.x&&x<=b.x+b.w&&y>=b.y&&y<=b.y+b.h){
                if(b.id==91){
                    // Weather cycle
                    weather=(WeatherType)((weather+1)%3);
                    if(weather==WEATHER_RAINY) initRain();
                } else if(b.id==100){
                    setAll(true);
                } else if(b.id==101){
                    setAll(false); alarmOn=false;
                } else {
                    *b.state=!*b.state;
                }
                return;
            }
        }
        // Sliders
        for(int i=0;i<nSliders;i++){
            Slider&s=sliders[i];
            if(x>=s.x-5&&x<=s.x+s.w+5&&y>=s.y-6&&y<=s.y+20){
                activeSlider=s.id;
                float v=s.mn+(x-s.x)/s.w*(s.mx-s.mn);
                *s.val=std::max(s.mn,std::min(s.mx,v));
                return;
            }
        }
    }
    if(btn==GLUT_LEFT_BUTTON && state==GLUT_UP)
        activeSlider=0;
}

void onMouseMove(int mx,int my){
    float x=(float)mx, y=(float)(WIN_H-my);
    // Hover
    hoverBtn=-1;
    for(int i=0;i<nSideButtons;i++){
        Btn&b=sideButtons[i];
        if(x>=b.x&&x<=b.x+b.w&&y>=b.y&&y<=b.y+b.h){ hoverBtn=b.id; break; }
    }
    // Dragging slider
    if(activeSlider!=0){
        for(int i=0;i<nSliders;i++){
            if(sliders[i].id==activeSlider){
                Slider&s=sliders[i];
                float v=s.mn+(x-s.x)/s.w*(s.mx-s.mn);
                *s.val=std::max(s.mn,std::min(s.mx,v));
                break;
            }
        }
    }
    glutPostRedisplay();
}

void onPassiveMove(int mx,int my){
    onMouseMove(mx,my);
}


//  KEYBOARD :

void onKeyboard(unsigned char key,int,int){
    switch(key){
        case 27: case 'q': case 'Q': exit(0);
        case '1': lightOn[0]=!lightOn[0]; break;
        case '2': lightOn[1]=!lightOn[1]; break;
        case '3': lightOn[2]=!lightOn[2]; break;
        case '4': lightOn[3]=!lightOn[3]; break;
        case '5': fanOn[0]=!fanOn[0];     break;
        case '6': fanOn[1]=!fanOn[1];     break;
        case '7': doorOpen[0]=!doorOpen[0];break;
        case '8': doorOpen[1]=!doorOpen[1];break;
        case '9': acOn=!acOn;             break;
        case 't': case 'T': tvOn=!tvOn;   break;
        case 'a': case 'A': alarmOn=!alarmOn; break;
        case 'd': case 'D': nightMode=false;  break;
        case 'n': case 'N': nightMode=true;   break;
        case 'w': case 'W':
            weather=(WeatherType)((weather+1)%3);
            if(weather==WEATHER_RAINY) initRain();
            break;
        case '0': setAll(true);  break;
        case '-': setAll(false); alarmOn=false; break;
    }
    glutPostRedisplay();
}


//  MAIN :

int main(int argc,char**argv){
    srand((unsigned)time(nullptr));
    initRain();

    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(WIN_W,WIN_H);
    glutInitWindowPosition(80,50);
    glutCreateWindow("Smart Home Automation Visualization  |  Computer Graphics Project");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.f);

    glutDisplayFunc(display);
    glutKeyboardFunc(onKeyboard);
    glutMouseFunc(onMouse);
    glutMotionFunc(onMouseMove);
    glutPassiveMotionFunc(onPassiveMove);
    glutTimerFunc(16,onTimer,0);

    glutMainLoop();
    return 0;
}