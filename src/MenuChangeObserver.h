/*
 * Copyright (c) 2023 Kevin Balthaser kevin@nybblebyte.com
 *
 * Observer to listen to changes in the menu manager.  We will use this to constrain navigation to where we want it
 * on the home screen.
 *
 *
*/


#ifndef KIWIBOARDFIRMWARE_MENUCHANGEOBSERVER_H
#define KIWIBOARDFIRMWARE_MENUCHANGEOBSERVER_H

#include <tcMenu.h>


class MenuChangeObserver : public MenuManagerObserver {

    public:
        MenuChangeObserver(MenuManager* menuManager, MenuItem *timerRow, MenuItem *leftConstraint);


        void activeItemHasChanged(MenuItem* newActive) override;
        void structureHasChanged() override;
        bool menuEditStarting(MenuItem* item) override;
        void menuEditEnded(MenuItem* item) override;

        /**
         * We are going to constrain navigation to this menu item until cleared.
         * @param stopButton
         */
        void constrainToStopButton(MenuItem * stopButton);


        /**
         * Reset navitation constraint (we are no longer locking to button)
         */
        void resetConstraint();

    private:
        MenuManager* menuManager;
        MenuItem* timerRow; // Ref to timer row.. we never want to be on this menu item..
        MenuItem* leftConstraint; // Left most constraint.. where to push them back to if they end up on timerrow..

        bool LOCK_TO_BUTTON = false; // If running.. constrain to current.
        MenuItem* stopIcon; // Where to constrain to when LOCK_TO_BUTTON

};


#endif //KIWIBOARDFIRMWARE_MENUCHANGEOBSERVER_H
