#ifndef MAINMENUSTATE_H
#define MAINMENUSTATE_H

#include "common.h"
#include "gamestate.h"

class MainMenuState: public GameState {
public:
    MainMenuState();
    virtual ~MainMenuState();
    void main_loop();
};

#endif // MAINMENUSTATE_H