#pragma once

#include"InfoStack.h"
#include <functional>
#include <atomic>

#ifndef _TEXT_UTF8
#define _TEXT_UTF8
#endif

typedef std::function<void()> StdMessage;
using IBR_ToFMessage = StdMessage;
struct IBF_ToRMessage
{
    StdMessage Action;
    InfoStack<StdMessage>* Target;
};


//但是它和IBD_RInterruptF、IBD_FInterruptR不同，不允许嵌套
template<typename T>
T IBG_Interrupt(std::mutex& mu, const std::function<T()>& msg)
{
    mu.lock();
    T Ret=msg();
    mu.unlock();
    return Ret;
}


class IBRF_Bump
{
public:
    typedef InfoStack<IBR_ToFMessage> RToFStack;
    typedef InfoStack<IBF_ToRMessage> FToRStack;
private:
    RToFStack RFStack;
    FToRStack FRStack;

    RToFStack RFDelayStack;
    FToRStack FRDelayStack;
    InfoStack<uint64_t> RFDelay;
    InfoStack<uint64_t> FRDelay;
    std::mutex RFLock, FRLock;

    std::atomic<bool> RFNewInfo;

public:
    std::mutex FInterruptLock, RInterruptLock;
    std::atomic<bool> FInterruptDisabled{ false }, RInterruptDisabled{ false };

    void IBG_Disable_RInterruptF(bool Cond);
    void IBG_Disable_FInterruptR(bool Cond);

    void SendToF(const IBR_ToFMessage& Msg);
    void SendToR(const IBF_ToRMessage& Msg);
    void IBR_SendToFDelayed(const IBR_ToFMessage& Msg, uint64_t DelayMicros);
    void IBF_SendToRDelayed(const IBF_ToRMessage& Msg, uint64_t DelayMicros);
    RToFStack::Cont IBF_TakeAway();
    FToRStack::Cont IBR_TakeAway();
    bool IBF_TimeToTake();
    void IBF_Process(const RToFStack::Cont& Copy);
    void IBF_ProcessDelay();
    void IBF_AutoProc();
    void IBF_ForceProc();
    void IBR_Process(const FToRStack::Cont& Copy);
    void IBR_ProcessDelay();
    void IBR_AutoProc();
    void IBR_ForceProc();
};


struct IBG_RInterruptF_RangeLock
{
    IBRF_Bump& Bump;
    bool DoLock{ false };
    IBG_RInterruptF_RangeLock() = delete;
    IBG_RInterruptF_RangeLock(IBRF_Bump& _Bump) : Bump(_Bump)
    {
        if (!_Bump.FInterruptDisabled.load())
        {
            _Bump.IBG_Disable_RInterruptF(true);
            while(!_Bump.FInterruptLock.try_lock())std::this_thread::sleep_for(std::chrono::milliseconds(0));
            DoLock = true;
        }
    }
    ~IBG_RInterruptF_RangeLock()
    {
        if (DoLock)
        {
            Bump.FInterruptLock.unlock();
            Bump.IBG_Disable_RInterruptF(false);
        }
    }
};

struct IBG_FInterruptR_RangeLock
{
    IBRF_Bump& Bump;
    bool DoLock{ false };
    IBG_FInterruptR_RangeLock() = delete;
    IBG_FInterruptR_RangeLock(IBRF_Bump& _Bump) : Bump(_Bump)
    {
        if (!_Bump.RInterruptDisabled.load())
        {
            _Bump.IBG_Disable_FInterruptR(true);
            while (!_Bump.RInterruptLock.try_lock())std::this_thread::sleep_for(std::chrono::milliseconds(0));
            DoLock = true;
        }
    }
    ~IBG_FInterruptR_RangeLock()
    {
        if (DoLock)
        {
            Bump.RInterruptLock.unlock();
            Bump.IBG_Disable_FInterruptR(false);
        }
    }
};
