

#include "RFBump.h"
#include "..\Global.h"
#include "global_tool_func.h"

void IBRF_Bump::SendToF(const IBR_ToFMessage& Msg)
{
    RFStack.Push(Msg);
    RFNewInfo.store(true);
}
void IBRF_Bump::SendToR(const IBF_ToRMessage& Msg)
{
    FRStack.Push(Msg);
}
void IBRF_Bump::IBR_SendToFDelayed(const IBR_ToFMessage& Msg, uint64_t DelayMicros)
{
    RFLock.lock();
    RFDelayStack.Push(Msg);
    RFDelay.Push(DelayMicros + GetSysTimeMicros());
    RFLock.unlock();
}
void IBRF_Bump::IBF_SendToRDelayed(const IBF_ToRMessage& Msg, uint64_t DelayMicros)
{
    FRLock.lock();
    FRDelayStack.Push(Msg);
    FRDelay.Push(DelayMicros + GetSysTimeMicros());
    FRLock.unlock();
}
IBRF_Bump::RToFStack::Cont IBRF_Bump::IBF_TakeAway()
{
    RToFStack::Cont Ret;
    RFStack.Release(Ret);
    RFNewInfo.store(false);
    return Ret;
}
IBRF_Bump::FToRStack::Cont IBRF_Bump::IBR_TakeAway()
{
    FToRStack::Cont Ret;
    FRStack.Release(Ret);
    return Ret;
}
bool IBRF_Bump::IBF_TimeToTake()
{
    return RFNewInfo.load();
}
void IBRF_Bump::IBF_Process(const RToFStack::Cont& Copy)
{
    for (auto Msg : Copy)
    {
        FInterruptLock.lock();
        Msg();
        FInterruptLock.unlock();
    }
}
void IBRF_Bump::IBF_ProcessDelay()
{
    FRLock.lock();
    std::vector<IBF_ToRMessage> DStack = FRDelayStack.Release();
    std::vector<uint64_t> DTime = FRDelay.Release();
    FRLock.unlock();

    std::vector<IBF_ToRMessage> RStack;
    std::vector<uint64_t> RTime;
    uint64_t Time = GetSysTimeMicros();
    for (int i = 0; i < (int)DTime.size(); i++)
    {
        if (DTime[i] <= Time)//推出时间 <= 当前时间 推出
        {
            FRStack.Push(DStack[i]);
        }
        else
        {
            RStack.push_back(DStack[i]);
            RTime.push_back(DTime[i]);
        }
    }

    FRLock.lock();
    std::vector<IBF_ToRMessage> DStackA = FRDelayStack.Release();
    std::vector<uint64_t> DTimeA = FRDelay.Release();
    if (!DStackA.empty())for (auto v : DStackA)DStack.push_back(v);
    if (!DTimeA.empty())for (auto v : DTimeA)DTime.push_back(v);
    FRDelayStack.SetCont(DStack);
    FRDelay.SetCont(DTime);
    FRLock.unlock();
}
void IBRF_Bump::IBF_AutoProc()
{
    IBF_ProcessDelay();
    if (IBF_TimeToTake())
    {
        IBF_Process(IBF_TakeAway());
    }
}
void IBRF_Bump::IBF_ForceProc()
{
    if (IBF_TimeToTake())
    {
        for (auto Msg : IBF_TakeAway())Msg();
    }
}
void IBRF_Bump::IBR_Process(const FToRStack::Cont& Copy)
{
    for (auto Msg : Copy)
    {
        RInterruptLock.lock();
        if (Msg.Target == nullptr)
        {
            Msg.Action();
        }
        else
        {
            Msg.Target->Push(Msg.Action);
        }
        RInterruptLock.unlock();
    }
}
void IBRF_Bump::IBR_ProcessDelay()
{
    RFLock.lock();
    std::vector<IBR_ToFMessage> DStack = RFDelayStack.Release();
    std::vector<uint64_t> DTime = RFDelay.Release();
    RFLock.unlock();

    std::vector<IBR_ToFMessage> RStack;
    std::vector<uint64_t> RTime;
    uint64_t Time = GetSysTimeMicros();
    for (int i = 0; i < (int)DTime.size(); i++)
    {
        if (DTime[i] <= Time)//推出时间 <= 当前时间 推出
        {
            RFStack.Push(DStack[i]);
        }
        else
        {
            RStack.push_back(DStack[i]);
            RTime.push_back(DTime[i]);
        }
    }

    RFLock.lock();
    std::vector<IBR_ToFMessage> DStackA = RFDelayStack.Release();
    std::vector<uint64_t> DTimeA = RFDelay.Release();
    if (!DStackA.empty())for (auto v : DStackA)DStack.push_back(v);
    if (!DTimeA.empty())for (auto v : DTimeA)DTime.push_back(v);
    RFDelayStack.SetCont(DStack);
    RFDelay.SetCont(DTime);
    RFLock.unlock();
}
void IBRF_Bump::IBR_AutoProc()
{
    IBR_ProcessDelay();
    IBR_Process(IBR_TakeAway());
}
void IBRF_Bump::IBR_ForceProc()
{
    for (auto Msg : IBR_TakeAway())
    {
        if (Msg.Target == nullptr) Msg.Action();
        else Msg.Target->Push(Msg.Action);
    }
}


void IBRF_Bump::IBG_Disable_RInterruptF(bool Cond)
{
    FInterruptDisabled.store(Cond);
}
void IBRF_Bump::IBG_Disable_FInterruptR(bool Cond)
{
    RInterruptDisabled.store(Cond);
}
