/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
*/
//

#include "MenuChangeObserver.h"


void MenuChangeObserver::activeItemHasChanged(MenuItem *newActive) {

    Serial.println("Active menu item changed.. ");

    if (LOCK_TO_BUTTON) {
        if (newActive != stopIcon) {
            // move it back...
            menuManager->activateMenuItem(stopIcon);
        }
    }
    // todo.. block out the title bar.. but only when on the home screen?
    if (newActive == timerRow ) {
        Serial.println("Tried to move to TimerRow... Move back to LeftConstraint ");
        menuManager->activateMenuItem(leftConstraint);
    }
}



MenuChangeObserver::MenuChangeObserver(MenuManager *menuManager, MenuItem *timerRow, MenuItem *leftConstraint)
        : menuManager(menuManager), timerRow(timerRow), leftConstraint(leftConstraint) {

}




void MenuChangeObserver::structureHasChanged() {
Serial.println("menu tree structure changed");
}

bool MenuChangeObserver::menuEditStarting(MenuItem* item)  {
Serial.println("Editing started");
return true; // allow editing to start
}

void MenuChangeObserver::menuEditEnded(MenuItem* item)  {
Serial.println("Edit committed");
}

void MenuChangeObserver::constrainToStopButton(MenuItem *stopButton) {
    LOCK_TO_BUTTON = true;
    stopIcon = stopButton;
}

void MenuChangeObserver::resetConstraint() {
    LOCK_TO_BUTTON = false;

}
