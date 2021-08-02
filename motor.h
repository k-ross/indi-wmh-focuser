#pragma once

#include <chrono>

enum class BoardRevision
{
    Original, Rev21
};

class Motor
{
protected:
    BoardRevision _revision = BoardRevision::Original;
    
    void _Delay(int microseconds)
    {
        if (microseconds == 0)
            return;

        auto start = std::chrono::high_resolution_clock::now();
        for(;;)
        {
            auto later = std::chrono::high_resolution_clock::now();
            auto micros = std::chrono::duration_cast<std::chrono::microseconds>(later - start);
            if (micros.count() >= microseconds)
                break;
        }
    }
    
public:
    enum class Direction
    {
        Forward, Backward
    };
    
    Motor() {}
    virtual ~Motor() {}
    
    void SetBoardRevision(BoardRevision revision) { _revision = revision; }
    
    virtual void Enable(Direction dir) = 0;
    virtual void Disable() = 0;
    virtual void SingleStep(int stepDelayMicroseconds) = 0;
};
