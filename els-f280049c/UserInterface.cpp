// Clough42 Electronic Leadscrew
// https://github.com/clough42/electronic-leadscrew
//
// MIT License
//
// Copyright (c) 2019 James Clough
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include "UserInterface.h"

const MESSAGE STARTUP_MESSAGE_2 =
{
  .message = { LETTER_E, LETTER_L, LETTER_S, DASH, ONE | POINT, FOUR | POINT, ZERO, ZERO },
  .displayTime = UI_REFRESH_RATE_HZ * 1.5
};

const MESSAGE STARTUP_MESSAGE_1 =
{
 .message = { LETTER_C, LETTER_L, LETTER_O, LETTER_U, LETTER_G, LETTER_H, FOUR, TWO },
 .displayTime = UI_REFRESH_RATE_HZ * 1.5,
 .next = &STARTUP_MESSAGE_2
};

const MESSAGE SETTINGS_MESSAGE_2 =
{
 .message = { LETTER_S, LETTER_E, LETTER_T, LETTER_T, LETTER_I, LETTER_N, LETTER_G, LETTER_S },
 .displayTime = UI_REFRESH_RATE_HZ * .5
};

const MESSAGE SETTINGS_MESSAGE_1 =
{
 .message = { BLANK, BLANK, BLANK, LETTER_N, LETTER_O, BLANK, BLANK, BLANK },
 .displayTime = UI_REFRESH_RATE_HZ * .5,
 .next = &SETTINGS_MESSAGE_2
};

extern const MESSAGE BACKLOG_PANIC_MESSAGE_2;
const MESSAGE BACKLOG_PANIC_MESSAGE_1 =
{
 .message = { LETTER_T, LETTER_O, LETTER_O, BLANK, LETTER_F, LETTER_A, LETTER_S, LETTER_T },
 .displayTime = UI_REFRESH_RATE_HZ * .5,
 .next = &BACKLOG_PANIC_MESSAGE_2
};
const MESSAGE BACKLOG_PANIC_MESSAGE_2 =
{
 .message = { BLANK, LETTER_R, LETTER_E, LETTER_S, LETTER_E, LETTER_T, BLANK, BLANK },
 .displayTime = UI_REFRESH_RATE_HZ * .5,
 .next = &BACKLOG_PANIC_MESSAGE_1
};



const Uint16 VALUE_BLANK[4] = { BLANK, BLANK, BLANK, BLANK };

UserInterface :: UserInterface(ControlPanel *controlPanel, Core *core, FeedTableFactory *feedTableFactory, GearTable *gearTable):
    controlPanel_(controlPanel)
    , core_(core)
    , feedTableFactory_(feedTableFactory)
    , metric_(false)
    , thread_(false)
    , reverse_(false)
    , feedTable_(NULL)
    , gearTable_(gearTable)
{
    this->keys_.all = 0xff;

    // initialize the core so we start up correctly
    core->setReverse(this->reverse_);
    core->setFeed(loadFeedTable());
    core->setGear(gearTable_->current());

    setMessage(&STARTUP_MESSAGE_1);
}

const FEED_THREAD *UserInterface::loadFeedTable()
{
    this->feedTable_ = this->feedTableFactory_->getFeedTable(this->metric_, this->thread_);
    return this->feedTable_->current();
}

LED_REG UserInterface::calculateLEDs()
{
    // get the LEDs for this feed
    LED_REG leds = feedTable_->current()->leds;

    if( this->core_->isPowerOn() )
    {
        // and add a few of our own
        leds.bit.POWER = 1;
        leds.bit.REVERSE = this->reverse_;
        leds.bit.FORWARD = ! this->reverse_;
    }
    else
    {
        // power is off
        leds.all = 0;
    }

    return leds;
}

void UserInterface :: setMessage(const MESSAGE *message)
{
    this->message_ = message;
    this->messageTime_ = message->displayTime;
}

void UserInterface :: overrideMessage( void )
{
    if( this->message_ != NULL )
    {
        if( this->messageTime_ > 0 ) {
            this->messageTime_--;
            controlPanel_->setMessage(this->message_->message);
        }
        else {
            this->message_ = this->message_->next;
            if( this->message_ == NULL )
                controlPanel_->setMessage(NULL);
            else
                this->messageTime_ = this->message_->displayTime;
        }
    }
}

void UserInterface :: clearMessage( void )
{
    this->message_ = NULL;
    this->messageTime_ = 0;
    controlPanel_->setMessage(NULL);
}

void UserInterface :: panicStepBacklog( void )
{
    setMessage(&BACKLOG_PANIC_MESSAGE_1);
}

void UserInterface :: loop( void )
{
    // read the RPM up front so we can use it to make decisions
    Uint16 currentRpm = core_->getRPM();

    // display an override message, if there is one
    overrideMessage();

    // read keypresses from the control panel
    keys_ = controlPanel_->getKeys();

    // respond to keypresses
    if( currentRpm == 0 )
    {
        // these keys should only be sensitive when the machine is stopped
        if( keys_.bit.POWER ) {
            this->core_->setPowerOn(!this->core_->isPowerOn());
            clearMessage();
        }

        // these should only work when the power is on
        if( this->core_->isPowerOn() ) {
            if( keys_.bit.IN_MM )
            {
                this->metric_ = ! this->metric_;
                core_->setFeed(loadFeedTable());
            }
            if( keys_.bit.FEED_THREAD )
            {
                this->thread_ = ! this->thread_;
                core_->setFeed(loadFeedTable());
            }
            if( keys_.bit.FWD_REV )
            {
                this->reverse_ = ! this->reverse_;
                core_->setReverse(this->reverse_);
            }
            if( keys_.bit.SET )
            {
#ifdef USE_GEARBOX
                core_->setGear(gearTable_->next());
                setMessage(&(gearTable_->current()->message));
#else
                setMessage(&SETTINGS_MESSAGE_1)
#endif
            }
        }
    }

#ifdef IGNORE_ALL_KEYS_WHEN_RUNNING
    if( currentRpm == 0 )
        {
#endif // IGNORE_ALL_KEYS_WHEN_RUNNING

        // these should only work when the power is on
        if( this->core_->isPowerOn() ) {
            // these keys can be operated when the machine is running
            if( keys_.bit.UP )
            {
                core_->setFeed(feedTable_->next());
            }
            if( keys_.bit.DOWN )
            {
                core_->setFeed(feedTable_->previous());
            }
        }

#ifdef IGNORE_ALL_KEYS_WHEN_RUNNING
    }
#endif // IGNORE_ALL_KEYS_WHEN_RUNNING

    // update the control panel
    controlPanel_->setLEDs(calculateLEDs());
    controlPanel_->setValue(feedTable_->current()->display);
    controlPanel_->setRPM(currentRpm);

    if( ! core_->isPowerOn() )
    {
        controlPanel_->setValue(VALUE_BLANK);
    }

    controlPanel_->refresh();
}
