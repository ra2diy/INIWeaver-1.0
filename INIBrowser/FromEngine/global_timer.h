#pragma once

/***********************************************************************************************
 ***                                 ??????????——源代码                                    ***
 ***********************************************************************************************
 *                                                                                             *
 *                       文件名 : global_timer.h                                               *
 *                                                                                             *
 *                     编写名单 : Old Sovok                                                    *
 *                                                                                             *
 *                     新建日期 : 2022/3/23                                                    *
 *                                                                                             *
 *                     最近编辑 : 2022/3/30 Old Sovok                                          *
 *                                                                                             *
 *                     单元测试 : 2022/3/30 Old Sovok                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * 内容：                                                             
 *   namespace Timer -- WOTT的计时器所在命名空间       
 *     inttime_t (在 types.h) -- 时间刻类型
 *     timer_t -- 时间点类型                            
 *     dura_t -- 时间段类型                                                 
 *     ClocksPerSec -- 获得一秒的时间段长度                               
 *     ClocksPerMSec -- 获得一毫秒的时间段长度                               
 *     SecondToTick -- 用秒数生成时间刻
 *     MilliToTick -- 用毫秒数生成时间刻
 *     ToDuration -- 用时间刻生成时间段
 *     ToTimeInt -- 用时间段生成时间刻
 *     ClockNow -- 获取当前时间点
 *     SetSpeedRatio -- 设置速度倍率（> 1 = 加速 ，< 1 = 减速）
 *     mode_t -- 计时器种类（正计时、倒计时）
 *     TimerClass -- 计时器类
 *       Set -- 重设运行参数
 *       Reset -- 计时器复位
 *       Pause -- 暂停计时
 *       Resume -- 继续计时
 *       GetClock -- 获取计时器的时间段（正计时 = 计时时间 ， 倒计时 = 剩余时间）
 *       GetSecond -- 获取计时器秒数（正计时 = 计时时间 ， 倒计时 = 剩余时间）
 *       GetMilli -- 获取计时器毫秒数（正计时 = 计时时间 ， 倒计时 = 剩余时间）
 *       TimeUp -- 是否时间到了
 *       BeginTime -- 获取起始时间点
 *       CountLength -- 获取（倒计时）时间段长度
 *       PauseMode -- 是否正在暂停
 *     GlobalTimer -- 全局计时器
 *       RealTimer -- 获取本局实际流逝时间计时器
 *       MapTimer -- 获取本局地图触发计时器
 *       Init -- 初始化2个计时器
 *       ResetGlobalTimer -- 复位地图触发计时器
 *       SaveGameTimer -- 对外赋值2个计时器用于保存
 *       FrameFlow -- 帧计数器步进
 *       SavePlayFrames -- 对外赋值帧计数器用于保存
 *       GetPlayFrames -- 获取帧计数器
 *       QuitGameReset -- 退出游戏时的计时器、帧计数器重置
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "types.h"
#include<ratio>
#include<chrono>

namespace ETimer
{
	typedef std::chrono::time_point<std::chrono::steady_clock> timer_t;
	typedef std::chrono::steady_clock::duration dura_t;
	const static dura_t __DurationZero = dura_t::zero();
	typedef int64_t inttime_t;
	typedef int64_t counter_t;
	const static inttime_t ClockTicksPerSec = std::chrono::steady_clock::duration::period::den / (std::chrono::steady_clock::duration::period::num);
	const static inttime_t ClockTicksPerMSec = ClockTicksPerSec / 1000;

    constexpr dura_t ClocksPerSec() { return dura_t((inttime_t)((double)ClockTicksPerSec)); }
    constexpr dura_t ClocksPerMSec() { return dura_t((inttime_t)((double)ClockTicksPerMSec)); }
    constexpr inttime_t SecondToTick(const inttime_t _clock) { return _clock * ClockTicksPerSec; }
    constexpr inttime_t MilliToTick(const inttime_t _clock) { return _clock * ClockTicksPerMSec; }
	constexpr dura_t ToDuration(const inttime_t _clock) { return dura_t(_clock); }
	constexpr inttime_t ToTimeInt(const dura_t& _clock) { return (inttime_t)_clock.count(); }
    constexpr dura_t SecondToDuration(const inttime_t _clock) { return ToDuration(SecondToTick(_clock)); }
    constexpr dura_t MilliToDuration(const inttime_t _clock) { return ToDuration(MilliToTick(_clock)); }
    static timer_t ClockNow() { return std::chrono::steady_clock::now(); }

	enum mode_t
	{
		T_COUNT,
		T_COUNTDOWN
	};
	class TimerClass
	{
		timer_t Begin, RBegin;
		timer_t PauseTime;
		dura_t Length;
		mode_t Mode;
		bool InPause;//sizeof TimerClass = 32
		void ReleasePause(timer_t To)
		{
			Begin += To - PauseTime;
			PauseTime = To;
		}
	public:
		//CONSTRUCTOR
		TimerClass(timer_t _Begin = ClockNow())
		{
			Begin = RBegin = _Begin;
			Mode = T_COUNT;
			Length = __DurationZero;
			PauseTime = ClockNow();
			InPause = false;
		}
		TimerClass(dura_t _Length, timer_t _Begin = ClockNow())
		{
			Begin = RBegin = _Begin;
			Mode = T_COUNTDOWN;
			Length = _Length;
			PauseTime = ClockNow();
			InPause = false;
		}
		//ACTION
		void Set(timer_t _Begin = ClockNow())
		{
			Begin = RBegin = _Begin;
			Mode = T_COUNT;
			Length = __DurationZero;
			PauseTime = ClockNow();
			InPause = false;
		}
		void Set(dura_t _Length, timer_t _Begin = ClockNow())
		{
			Begin = RBegin = _Begin;
			Mode = T_COUNTDOWN;
			Length = _Length;
			PauseTime = ClockNow();
			InPause = false;
		}
		void Reset()
		{
			Begin = RBegin = ClockNow();
			PauseTime = ClockNow();
			InPause = false;
		}
		void Pause()
		{
			if (InPause)return;
			PauseTime = ClockNow();
			InPause = true;
		}
		void Resume()
		{
			if (!InPause)return;
			timer_t Now = ClockNow();
			ReleasePause(Now);
			PauseTime = Now;
			InPause = false;
		}
		//QUERY
		dura_t GetClock()
		{
			timer_t Now = ClockNow();
			if (InPause)
			{
				ReleasePause(Now);
			}
			if (Mode == T_COUNT)
			{
				return (Now - Begin);
			}
			else if (Mode == T_COUNTDOWN)
			{
				return (Begin + Length) - Now;
			}
			else return __DurationZero;
		}
		inttime_t GetSecond()
		{
			return (inttime_t)((double)GetClock().count() / (double)ClockTicksPerSec);
		}
		inttime_t GetMilli()
		{
			return (inttime_t)(((double)GetClock().count() - (double)Length.count()) / (double)ClockTicksPerMSec);
		}
		bool TimeUp()
		{
			if (Mode == T_COUNTDOWN)
			{
				return ((inttime_t)((double)GetClock().count() - (double)Length.count()) <= 0);
			}
			else return false;
		}
		timer_t BeginTime() { return RBegin; }
		dura_t CountLength() { return Length; }
		bool PauseMode() { return InPause; }
	};

}
