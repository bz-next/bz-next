/* bzflag
 * Copyright (c) 1993-2021 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* interface header */
#include "ForceFeedback.h"

/* common interface headers */
#include "BzfJoystick.h"

/* local implementation headers */
#include "LocalPlayer.h"
#include "playing.h"

static BzfJoystick*     getJoystick();
static bool      useForceFeedback();


static BzfJoystick*     getJoystick()
{
    MainWindow *win = getMainWindow();
    if (win)
        return win->getJoystick();
    else
        return NULL;
}

static bool      useForceFeedback()
{
    BzfJoystick* js = getJoystick();

    /* There must be a joystick class, and we need an opened joystick device */
    if (!js)
        return false;
    if (!js->joystick())
        return false;

    /* Joystick must be the current input method */
    if (LocalPlayer::getMyTank()->getInputMethod() != LocalPlayer::Joystick)
        return false;

    /* Did the user enable rumble? */
    return BZDB.isTrue("rumble");
}

namespace ForceFeedback
{

void death()
{
    /* Nice long hard rumble for death */
    if (useForceFeedback())
        getJoystick()->ffRumble(1, 1.5f, 1.0f, 0.0f);
}

void shotFired()
{
    /* Tiny little kick for a normal shot being fired */
    if (useForceFeedback())
        getJoystick()->ffRumble(1, 0.1f, 0.0f, 1.0f);
}

void laserFired()
{
    /* Funky pulsating rumble for the laser.
     * (Only tested so far with the Logitech Wingman Cordless Rumblepad,
     *  some quirks in its driver may mean it's feeling a little different
     *  than it should)
     */
    if (useForceFeedback())
        getJoystick()->ffRumble(4, 0.02f, 1.0f, 1.0f);
}

void shockwaveFired()
{
    /* try to 'match' the shockwave sound */
    if (useForceFeedback())
        getJoystick()->ffRumble(1, 0.5f, 0.0f, 1.0f);
}
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
