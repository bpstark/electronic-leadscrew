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


#ifndef __TABLES_H
#define __TABLES_H

#include "F28x_Project.h"
#include "Configuration.h"
#include "ControlPanel.h"
#include "Message.h"

typedef struct GEAR
{
    MESSAGE message;
    Uint64 numerator;
    Uint64 denominator;
} GEAR;

typedef struct FEED_THREAD
{
    Uint16 display[4];
    union LED_REG leds;
    Uint64 numerator;
    Uint64 denominator;
} FEED_THREAD;

template<typename T> class Table
{
private:
    const T *table;
    Uint16 selectedRow;
    Uint16 numRows;

public:
    Table(const T *table, Uint16 numRows, Uint16 defaultSelection)
    {
        this->table = table;
        this->numRows = numRows;
        this->selectedRow = defaultSelection;
    }

    const T* current(void)
    {
        return &table[selectedRow];
    }

    const T* next(void)
    {
        this->selectedRow = (this->selectedRow + 1) % this->numRows;
        return this->current();
    }

    const T* previous(void)
    {
        if( this->selectedRow > 0 )
        {
            --this->selectedRow;
        }
        else
        {
            this->selectedRow = this->numRows-1;
        }
        return this->current();
    }
};

typedef Table<FEED_THREAD> FeedTable;
typedef Table<GEAR> GearTable;


class FeedTableFactory
{
private:
    FeedTable inchThreads;
    FeedTable inchFeeds;
    FeedTable metricThreads;
    FeedTable metricFeeds;

public:
    FeedTableFactory(void);

    FeedTable *getFeedTable(bool metric, bool thread);
};

class GearTableFactory
{
private:
    GearTable table;
public:
    GearTableFactory(void);
    inline GearTable* getGearTable(void)
    {
        return &(this->table);
    }
};


#endif // __TABLES_H
