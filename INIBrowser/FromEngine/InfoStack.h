#pragma once

#include<vector>
#include<mutex>

template<class T>
class InfoStack
{
public:
    typedef std::vector<T> Cont;
private:
    Cont Store;
    std::mutex Lock;
public:
    Cont& GetStoreRaw()
    {
        return Store;
    }
    Cont GetCopy()
    {
        Lock.lock();
        Cont Ret = Store;
        Lock.unlock();
        return Ret;
    }
    void GetCopy(Cont& Target)
    {
        Lock.lock();
        Target = Store;
        Lock.unlock();
    }
    void SetCont(const Cont& Source)
    {
        Lock.lock();
        Store = Source;
        Lock.unlock();
    }
    bool Empty()
    {
        bool E;
        Lock.lock();
        E = Store.empty();
        Lock.unlock();
        return E;
    }
    void Clear()
    {
        Lock.lock();
        Store.clear();
        Lock.unlock();
    }
    Cont Release()
    {
        Lock.lock();
        Cont Ret;
        std::swap(Ret, Store);
        Lock.unlock();
        return Ret;
    }
    void Release(Cont& Target)
    {
        while (!Lock.try_lock())std::this_thread::sleep_for(std::chrono::milliseconds(0));//Wait for other threads
        Target.clear();
        std::swap(Target, Store);
        Lock.unlock();
    }
    void Push(const T& Ct)
    {
        Lock.lock();
        Store.push_back(Ct);
        Lock.unlock();
    }
};
