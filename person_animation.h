#ifndef PERSON_ANIMATION_H
#define PERSON_ANIMATION_H

// Public API used by smart_home_pro.cpp
void personInit();
void personUpdate(float dt);   // call every frame from onTimer
void personDraw();             // call from display() AFTER drawHouse()
void personGoToRoom(int room); // 0=Living, 1=Bedroom, 2=Kitchen, 3=Bathroom
bool personIsBusy();           // true while walking

void personLeaveHome(int exit); // 0 = front door, 1 = garage/back door

#endif