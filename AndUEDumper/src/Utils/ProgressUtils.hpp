#pragma once

#include <iostream>
#include <string>
#include <functional>

class SimpleProgressBar
{
private:
    int total;            // Total units of work
    int current;          // Current progress
    int width;            // Width of the progress bar in characters
    char completeChar;    // Character for completed portion
    char incompleteChar;  // Character for incomplete portion

public:
    SimpleProgressBar()
        : total(1), current(0), width(50), completeChar('#'), incompleteChar('-') {}

    SimpleProgressBar(int totalWork, int barWidth = 50, char compChar = '#', char incompChar = '-')
        : total(totalWork > 0 ? totalWork : 1), current(0), width(barWidth > 0 ? barWidth : 50), completeChar(compChar), incompleteChar(incompChar) {}

    inline int getTotal() const { return total; }
    inline int getCurrent() const { return current; }
    inline int getWidth() const { return width; }
    inline char getCompleteChar() const { return completeChar; }
    inline char getIncompleteChar() const { return incompleteChar; }

    inline void setTotal(int t) { total = t; }
    inline void setCurrent(int c)
    {
        if (c >= 0 && c <= total) current = c;
    }
    inline void setWidth(int w) { width = w; }
    inline void setCompleteChar(char c) { completeChar = c; }
    inline void setIncompleteChar(char c) { incompleteChar = c; }

    inline int getPercentage() const
    {
        if (current == 0 || total == 0) return 0;

        const float percentage = static_cast<float>(current) / total;
        return static_cast<int>(percentage * 100.0f);
    }

    inline bool isComplete() const { return current >= total; }

    inline SimpleProgressBar& operator++()
    {
        if (current < total)
        {
            ++current;
        }
        return *this;
    }

    inline SimpleProgressBar operator++(int)
    {
        SimpleProgressBar temp = *this;
        if (current < total)
        {
            ++current;
        }
        return temp;
    }

    void print() const;
};
