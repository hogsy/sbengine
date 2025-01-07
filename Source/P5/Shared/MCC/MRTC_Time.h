static void SwapLE(fp64& _Var); // (used by CMTime_Generic and CMTime_fp64)


class CMTimerFuncs_OS
{
public:
	M_INLINE static uint64 Clock()
	{
		return MGetOSClock();
	}
	M_INLINE static uint64 Frequency()
	{
		return MGetOSFrequencyInt();
	}
	M_INLINE static fp32 FrequencyFloat()
	{
		return MGetOSFrequencyFp();
	}
	M_INLINE static fp32 FrequencyRecp()
	{
		return MGetOSFrequencyRecp();
	}
};

class CMTimerFuncs_CPU
{
public:
	M_INLINE static uint64 Clock()
	{
		return MGetCPUClock();
	}
	M_INLINE static uint64 Frequency()
	{
		return MGetCPUFrequencyInt();
	}
	M_INLINE static fp32 FrequencyFloat()
	{
		return MGetCPUFrequencyFp();
	}
	M_INLINE static fp32 FrequencyRecp()
	{
		return MGetCPUFrequencyRecp();
	}
};

template <class t_CTimeFuncs = CMTimerFuncs_CPU>
class TCMStopTimer
{
public:
	int64 m_StartTime;
	int64 m_EndTime;

	TCMStopTimer(fp32 _Duration)
	{
		Start(_Duration);
	}

	void Start(fp32 _Duration)
	{
		m_StartTime = t_CTimeFuncs::Clock();
		m_EndTime = m_StartTime + int64(_Duration * t_CTimeFuncs::FrequencyFloat());
	}

	void Reset(fp32 _Duration)
	{
		m_StartTime = m_EndTime;
		m_EndTime = m_StartTime + int64(_Duration * t_CTimeFuncs::FrequencyFloat());
	}

	bool Done()
	{
		return t_CTimeFuncs::Clock() >= m_EndTime;
	}
};

typedef TCMStopTimer<> CMStopTimer;


template <class t_CTimeFuncs = CMTimerFuncs_OS>
class CMTime_Generic
{
private:
	int64 m_Time;
public:
	CMTime_Generic()
	{
		m_Time = 0;
	}

	CMTime_Generic(CMTime_Generic<t_CTimeFuncs> const&_Time)
	{
		m_Time = _Time.m_Time;
	}

	bool operator != (CMTime_Generic<t_CTimeFuncs> const&_Time)
	{
		return m_Time != _Time.m_Time;
	}

	CMTime_Generic<t_CTimeFuncs> &operator = (CMTime_Generic<t_CTimeFuncs> const&_Time)
	{
		m_Time = _Time.m_Time;
		return *this;
	}

	CMTime_Generic<t_CTimeFuncs> operator + (CMTime_Generic<t_CTimeFuncs> const&_Time) const
	{
		CMTime_Generic<t_CTimeFuncs> Time;
		Time.m_Time = m_Time + _Time.m_Time;
		return Time;
	}

	CMTime_Generic<t_CTimeFuncs> operator - (CMTime_Generic<t_CTimeFuncs> const&_Time) const
	{
		CMTime_Generic<t_CTimeFuncs> Time;
		Time.m_Time = m_Time - _Time.m_Time;
		return Time;
	}

	CMTime_Generic<t_CTimeFuncs> &operator += (CMTime_Generic<t_CTimeFuncs> const&_Time)
	{
		m_Time += _Time.m_Time;
		return *this;
	}

	CMTime_Generic<t_CTimeFuncs> &operator -= (CMTime_Generic<t_CTimeFuncs> const&_Time)
	{
		m_Time -= _Time.m_Time;
		return *this;
	}

	void Reset()
	{
		m_Time = 0;
	}

	bool IsReset() const
	{
		return m_Time == 0;
	}

	static CMTime_Generic<t_CTimeFuncs> CreateInvalid()
	{
		CMTime_Generic<t_CTimeFuncs> Invalid;
		Invalid.MakeInvalid();
		return Invalid;
	}

	void MakeInvalid()
	{
		m_Time = -1;
	}

	bool IsInvalid() const
	{
		return m_Time == -1;
	}

	void ResetMax()
	{
		m_Time = 0x7fffffffffffffffLL;
	}

	void Snapshot()
	{
		m_Time = t_CTimeFuncs::Clock();
	}

	void Start()
	{
		m_Time -= t_CTimeFuncs::Clock();
	}

	void Stop()
	{
		m_Time += t_CTimeFuncs::Clock();
	}

	void Add(CMTime_Generic<t_CTimeFuncs> const & _Time)
	{
		m_Time += _Time.m_Time;
	}

	fp32 GetTime() const
	{
		return (fp32)m_Time * t_CTimeFuncs::FrequencyRecp();
	}

	fp64 GetTimefp64() const
	{
		return (fp64)m_Time * fp64(t_CTimeFuncs::FrequencyRecp());
	}

	int64 GetCycles() const
	{
		return m_Time;
	}

	int Compare(CMTime_Generic<t_CTimeFuncs> const&_Time) const
	{		
		if (m_Time < _Time.m_Time)
			return -1;
		else if (m_Time > _Time.m_Time)
			return 1;
		return 0;
	}

	int Compare(fp32 _Time) const
	{
		return Compare( CreateFromSeconds( _Time ) );
	}

	// Default error margin is 100 micro seconds
	bool AlmostEqual(const CMTime_Generic<t_CTimeFuncs>& _Time, fp32 _Epsilon = 0.0001f) const
	{
		int64 nDiff = m_Time - _Time.m_Time;
		int64 nEpsilon = _Epsilon * t_CTimeFuncs::FrequencyFloat();
		return (nDiff > -nEpsilon  && nDiff < nEpsilon);
	}

	CMTime_Generic<t_CTimeFuncs> Modulus(fp32 _Modulus) const
	{
		CMTime_Generic<t_CTimeFuncs> Ret;
		CMTime_Generic<t_CTimeFuncs> Modul = CMTime_Generic<t_CTimeFuncs>::CreateFromSeconds( _Modulus );
		Ret.m_Time = m_Time % Modul.m_Time;
		return Ret;
	}

	CMTime_Generic<t_CTimeFuncs> Modulus(CMTime_Generic<t_CTimeFuncs> _Modulus) const
	{
		CMTime_Generic<t_CTimeFuncs> Ret;
		Ret.m_Time = m_Time % _Modulus.m_Time;
		return Ret;
	}

	CMTime_Generic<t_CTimeFuncs> ModulusScaled(fp32 _Scale, fp32 _Modulus) const
	{
		CMTime_Generic<t_CTimeFuncs> Ret;
		_Modulus = _Modulus * (1.0f / _Scale);

		CMTime_Generic<t_CTimeFuncs> Modul = CMTime_Generic<t_CTimeFuncs>::CreateFromSeconds( _Modulus );
		int64 Int64Scale = (_Scale * 65536.0f);
		// This isn't really safe, should be done with 128 bit ints

		Ret.m_Time = (((m_Time % Modul.m_Time)) * Int64Scale) >> 16;
		return Ret;
	}

	CMTime_Generic<t_CTimeFuncs> Scale(fp32 _Scale) const
	{
		CMTime_Generic<t_CTimeFuncs> Ret;

		int64 Int64Scale = (_Scale * 65536.0f);
		// This isn't really safe, should be done with 128 bit ints

		Ret.m_Time = (m_Time * Int64Scale) >> 16;
		return Ret;
	}


	fp32 GetTimeFraction(fp32 _Modulus) const
	{
		return GetTimeModulus(_Modulus) * (1.0f / _Modulus);
	}

	int GetNumModulus(fp32 _Modulus) const
	{
		int64 Int64Modulus = _Modulus * t_CTimeFuncs::FrequencyFloat();

		return m_Time / Int64Modulus;
	}

	int GetNumTicks(fp32 _TicksPerSec) const
	{
		int64 Int64Scale = (_TicksPerSec * 65536.0f);
		// This isn't really safe, should be done with 128 bit ints
		int64 Ticks = (m_Time * Int64Scale) >> 16;

		return Ticks / t_CTimeFuncs::Frequency();
	}

	fp32 GetTimeModulus(fp32 _Modulus) const
	{
		int64 Int64Modulus = _Modulus * t_CTimeFuncs::FrequencyFloat();

		int64 Time = m_Time % Int64Modulus;

		return (fp32)Time * t_CTimeFuncs::FrequencyRecp();
	}

	fp32 GetTimeModulusScaled(fp32 _Scale, fp32 _Modulus) const
	{
		_Modulus = _Modulus * (1.0f / _Scale);

		int64 Int64Modulus = _Modulus * t_CTimeFuncs::FrequencyFloat();

		int64 Time = m_Time % Int64Modulus;

		fp32 Modded = (fp32)Time * t_CTimeFuncs::FrequencyRecp();
		return Modded * _Scale;
	}

	fp32 GetTimeSqrModulusScaled(fp32 _Scale, fp32 _Modulus) const
	{
		_Scale *= GetTime();
		_Modulus = _Modulus * (1.0f / _Scale);

		int64 Int64Modulus = _Modulus * t_CTimeFuncs::FrequencyFloat();

		int64 TimeSqr = m_Time;

		int64 Time = TimeSqr % Int64Modulus;

		fp32 Modded = (fp32)Time * t_CTimeFuncs::FrequencyRecp();
		return Modded * _Scale;
	}

	CMTime_Generic<t_CTimeFuncs> Max(CMTime_Generic<t_CTimeFuncs> const&_Time) const
	{
		CMTime_Generic<t_CTimeFuncs> Ret;
		Ret.m_Time = m_Time > _Time.m_Time ? m_Time : _Time.m_Time;
		return Ret;
	}

	CMTime_Generic<t_CTimeFuncs> Min(CMTime_Generic<t_CTimeFuncs> const&_Time) const
	{
		CMTime_Generic<t_CTimeFuncs> Ret;
		Ret.m_Time = m_Time < _Time.m_Time ? m_Time : _Time.m_Time;
		return Ret;
	}

	static CMTime_Generic<t_CTimeFuncs> GetCPU()
	{
		CMTime_Generic<t_CTimeFuncs> Ret;
		Ret.m_Time = t_CTimeFuncs::Clock();
		return Ret;
	}

	static CMTime_Generic<t_CTimeFuncs> CreateFromSeconds(fp32 _Seconds)
	{
		CMTime_Generic<t_CTimeFuncs> Ret;

		Ret.m_Time = _Seconds * t_CTimeFuncs::FrequencyFloat();

		return Ret;
	}

	static CMTime_Generic<t_CTimeFuncs> CreateFromTicks(int64 _Ticks, fp32 _TickLength, fp32 _TickFraction = 0.0f)
	{
		CMTime_Generic<t_CTimeFuncs> Ret;

		Ret.m_Time = _TickLength * t_CTimeFuncs::FrequencyFloat();
		Ret.m_Time *= _Ticks;
		Ret.m_Time += (_TickFraction * _TickLength) * t_CTimeFuncs::FrequencyFloat();

		return Ret;
	}

	// This should be rewritten to pack in 1 MHz int64 precission or something to be platform independent (if its to slow to use fp64 here)
	void Pack(uint8 *&_pPtr) const
	{
		fp64 Time = (fp64)m_Time * fp64(t_CTimeFuncs::FrequencyRecp());
		::SwapLE(Time);
		(*(fp64*)_pPtr) = Time;
		_pPtr += sizeof(Time);
	}

	void Unpack(const uint8 *&_pPtr)
	{
		fp64 Time = (*(fp64*)_pPtr);
		::SwapLE(Time);
		m_Time = (Time * fp64(t_CTimeFuncs::FrequencyFloat()));
		_pPtr += sizeof(Time);
	}

	void Write(class CCFile* _pFile) const
	{
		fp64 Time = (fp64)m_Time * fp64(t_CTimeFuncs::FrequencyRecp());
		File_WriteLE(_pFile, Time);
	}

	void Read(class CCFile* _pFile)
	{
		fp64 Time;
		File_ReadLE(_pFile, Time);
		m_Time = (Time * fp64(t_CTimeFuncs::FrequencyFloat()));
	}

	void GetHMS(int &_h, int &_m, int &_s, fp32 &_rest) const
	{
		CMTime_Generic<t_CTimeFuncs> T = *this;
		_h = T.GetNumModulus(3600);
		T -= CreateFromSeconds(_h * 3600);

		_m = T.GetNumModulus(60);
		T -= CreateFromSeconds(_m * 60);

		_rest = T.GetTime();
		_s = (int)_rest;
		_rest -= _s;
	}

	/* // CStr is not know yet. Probably no problem in including, but that's for another day -JA
	CStr GetHMSString() const
	{
		int h, m, s;
		fp32 rest;
		GetHMS(h, m, s, rest);
		return CStrF("%.2i:%.2i:%.2i.%.2i", h, m, s, RoundToInt(rest * 100));
	}*/
};

template <class t_CTimeFuncs = CMTimerFuncs_OS>
class CMTime_fp64
{
private:
	fp64 m_Time;
public:
	CMTime_fp64()
	{
		m_Time = 0;
	}
	CMTime_fp64(CMTime_fp64<t_CTimeFuncs> const &_Time)
	{
		m_Time = _Time.m_Time;
	}
	bool operator != (CMTime_fp64<t_CTimeFuncs> const &_Time)
	{
		return m_Time != _Time.m_Time;
	}

	CMTime_fp64<t_CTimeFuncs> &operator = (CMTime_fp64<t_CTimeFuncs> const  &_Time)
	{
		m_Time = _Time.m_Time;
		return *this;
	}

	CMTime_fp64<t_CTimeFuncs> operator + (CMTime_fp64<t_CTimeFuncs> const  &_Time) const
	{
		CMTime_fp64 Time;
		Time.m_Time = m_Time + _Time.m_Time;
		return Time;
	}

	CMTime_fp64<t_CTimeFuncs> operator - (CMTime_fp64<t_CTimeFuncs> const  &_Time) const
	{
		CMTime_fp64<t_CTimeFuncs> Time;
		Time.m_Time = m_Time - _Time.m_Time;
		return Time;
	}

	CMTime_fp64<t_CTimeFuncs> &operator += (CMTime_fp64<t_CTimeFuncs> const  &_Time)
	{
		m_Time += _Time.m_Time;
		return *this;
	}

	CMTime_fp64<t_CTimeFuncs> &operator -= (CMTime_fp64<t_CTimeFuncs> const  &_Time)
	{
		m_Time -= _Time.m_Time;
		return *this;
	}

	void Reset()
	{
		m_Time = 0;
	}

	bool IsReset() const
	{
		return m_Time == 0;
	}

	static CMTime_fp64<t_CTimeFuncs> CreateInvalid()
	{
		CMTime_fp64<t_CTimeFuncs> Invalid;
		Invalid.MakeInvalid();
		return Invalid;
	}

	void MakeInvalid()
	{
		m_Time = -1.0;
	}

	bool IsInvalid() const
	{
		return m_Time == -1.0;
	}

	void ResetMax()
	{
		m_Time = 0x7fffffffffffffffLL;
	}

	void Snapshot()
	{
		m_Time = (fp64)t_CTimeFuncs::Clock() * (fp64)t_CTimeFuncs::FrequencyRecp();
	}

	void Start()
	{
		m_Time -= (fp64)t_CTimeFuncs::Clock() * (fp64)t_CTimeFuncs::FrequencyRecp();
	}

	void Stop()
	{
		m_Time += (fp64)t_CTimeFuncs::Clock() * (fp64)t_CTimeFuncs::FrequencyRecp();
	}

	void Add(CMTime_fp64<t_CTimeFuncs> const & _Time)
	{
		m_Time += _Time.m_Time;
	}

	fp32 GetTime() const
	{
		return m_Time;
	}

	fp64 GetTimefp64() const
	{
		return m_Time;
	}

	int64 GetCycles() const
	{
		return (int64)(m_Time * (fp64)t_CTimeFuncs::FrequencyFloat());
	}

	int Compare(CMTime_fp64<t_CTimeFuncs> const  &_Time) const
	{		
		if (m_Time < _Time.m_Time)
			return -1;
		else if (m_Time > _Time.m_Time)
			return 1;
		return 0;
	}

	int Compare(fp32 _Time) const
	{
		return Compare( CreateFromSeconds( _Time ) );
	}

	CMTime_fp64<t_CTimeFuncs> Modulus(fp32 _Modulus) const
	{
		CMTime_fp64<t_CTimeFuncs> Ret;
		Ret.m_Time = fmod(m_Time, (fp64)_Modulus);
		return Ret;
	}

	CMTime_fp64<t_CTimeFuncs> Modulus(CMTime_fp64<t_CTimeFuncs> _Modulus) const
	{
		CMTime_fp64<t_CTimeFuncs> Ret;
		Ret.m_Time = fmod(m_Time, _Modulus.m_Time);
		return Ret;
	}

	CMTime_fp64<t_CTimeFuncs> ModulusScaled(fp32 _Scale, fp32 _Modulus) const
	{
		CMTime_fp64<t_CTimeFuncs> Ret;
		Ret.m_Time = fmod(m_Time * _Scale, (fp64)_Modulus);
		return Ret;
	}

	CMTime_fp64<t_CTimeFuncs> Scale(fp32 _Scale) const
	{
		CMTime_fp64<t_CTimeFuncs> Ret;
		Ret.m_Time = m_Time * _Scale;
		return Ret;
	}


	fp32 GetTimeFraction(fp32 _Modulus) const
	{
		return GetTimeModulus(_Modulus) * (1.0f / _Modulus);
	}

	int GetNumModulus(fp32 _Modulus) const
	{
		return (int)(m_Time / _Modulus);
	}

	int GetNumTicks(fp32 _TicksPerSec) const
	{
		return (int)(m_Time * _TicksPerSec);
	}

	fp32 GetTimeModulus(fp32 _Modulus) const
	{
		return fmod(m_Time, (fp64)_Modulus);
	}

	fp32 GetTimeModulusScaled(fp32 _Scale, fp32 _Modulus) const
	{
		return fmod(m_Time * _Scale, (fp64)_Modulus);
	}

	fp32 GetTimeSqrModulusScaled(fp32 _Scale, fp32 _Modulus) const
	{
		return fmod(m_Time * m_Time * _Scale, (fp64)_Modulus);
	}

	bool AlmostEqual(const CMTime_fp64<t_CTimeFuncs>& _Time, fp32 _Epsilon = 0.0001f) const
	{
		fp64 fDiff = m_Time - _Time.m_Time;
		return (fDiff > -_Epsilon  && fDiff < _Epsilon);
	}

	CMTime_fp64<t_CTimeFuncs> Max(CMTime_fp64<t_CTimeFuncs> const  &_Time) const
	{
		CMTime_fp64<t_CTimeFuncs> Ret;
		Ret.m_Time = m_Time > _Time.m_Time ? m_Time : _Time.m_Time;
		return Ret;
	}

	CMTime_fp64<t_CTimeFuncs> Min(CMTime_fp64<t_CTimeFuncs> const  &_Time) const
	{
		CMTime_fp64<t_CTimeFuncs> Ret;
		Ret.m_Time = m_Time < _Time.m_Time ? m_Time : _Time.m_Time;
		return Ret;
	}

	static CMTime_fp64<t_CTimeFuncs> GetCPU()
	{
		CMTime_fp64<t_CTimeFuncs> Ret;
		Ret.m_Time = (fp64)t_CTimeFuncs::Clock() * (fp64)t_CTimeFuncs::FrequencyRecp();
		return Ret;
	}

	static CMTime_fp64<t_CTimeFuncs> CreateFromSeconds(fp32 _Seconds)
	{
		CMTime_fp64<t_CTimeFuncs> Ret;

		Ret.m_Time = _Seconds;

		return Ret;
	}

	static CMTime_fp64<t_CTimeFuncs> CreateFromTicks(int64 _Ticks, fp32 _TickLength, fp32 _TickFraction = 0.0f)
	{
		CMTime_fp64<t_CTimeFuncs> Ret;
		Ret.m_Time = _TickLength * _Ticks + (_TickFraction * _TickLength);
		return Ret;
	}

	// This should be rewritten to pack in 1 MHz int64 precission or something to be platform independent (if its to slow to use fp64 here)
	void Pack(uint8*& _pPtr) const
	{
		fp64 Tmp = m_Time;
		::SwapLE(Tmp);
		memcpy(_pPtr, &Tmp, sizeof(Tmp));
		_pPtr += sizeof(Tmp);
	}

	void Unpack(const uint8*& _pPtr)
	{
		memcpy(&m_Time, _pPtr, sizeof(m_Time));
		::SwapLE(m_Time);
		_pPtr += sizeof(m_Time);
	}

	void Write(class CCFile* _pFile) const
	{
		File_WriteLE(_pFile, m_Time);
	}

	void Read(class CCFile* _pFile)
	{
		File_ReadLE(_pFile, m_Time);
	}

};

#if defined(PLATFORM_CONSOLE) || !defined(M_Profile)
typedef	CMTime_fp64<>	CMTime;
#else
typedef	CMTime_Generic<>	CMTime;
#endif

typedef	CMTime_Generic<>	CMTimeCPU;


template <typename t_CTimerClass>
class TCMTimeScope
{
public:
	t_CTimerClass &m_Time;
	
	TCMTimeScope(t_CTimerClass &_Time, bool _bReset = false) : m_Time(_Time)
	{
		if (_bReset)
			m_Time.Reset();

		m_Time.Start();
	}

	~TCMTimeScope()
	{
		m_Time.Stop();
	}
};

typedef TCMTimeScope<CMTime> CMTimeScope;


#	define TStartAdd(timevar) {timevar.Start();}
#	define TMeasureC(timevar, _Class) TCMTimeScope<_Class> TimeScope(timevar);
#	define TMeasureResetC(timevar, _Class) TCMTimeScope<_Class> TimeScope(timevar, 1);

#	define TMeasure(timevar) CMTimeScope TimeScope(timevar);
#	define TMeasureReset(timevar) CMTimeScope TimeScope(timevar, 1);
#	define TStart(timevar) {(timevar).Reset(); (timevar).Start();}
#	define TStop(timevar) {(timevar).Stop();}
#	define TCycles(name, timevar) CStrF("%s %d c", name, (timevar).GetCycles())
#	define TString(name, timevar) CStrF("%s %0.2f ms", name, (timevar).GetTime()*1000.0)
#	define TMbPerSec(timevar, bytes) CStrF("%0.2f Mb/s", (fp32(bytes)/1000000)/((timevar).GetTime()))
#	define TxPerSec(timevar, tick, unit) CStrF("%0.2f%s", fp32(tick)/((timevar).GetTime()), unit)

#ifdef M_Profile
#	define MIncProfile(var) ++var
#	define MAddProfile(var, add) var += add;
#	define TProfileDef(timevar) CMTime timevar
#	define TStartAddProfile(timevar) TStartAdd(timevar)
#	define TMeasureProfile(timevar) TMeasure(timevar)
#	define TMeasureResetProfile(timevar) TMeasureReset(timevar)
#	define TStartProfile(timevar) TStart(timevar)
#	define TStopProfile(timevar) TStop(timevar)
#	define TAddProfile(timevar, time) timevar.Add(time)
#else
#	define MIncProfile(var) ((void)0)
#	define MAddProfile(var, add) ((void)0)
#	define TMeasureProfile(timevar) ((void)0);
#	define TMeasureResetProfile(timevar) ((void)0);
#	define TProfileDef(timevar) ((void)0)
#	define TStartAddProfile(timevar) ((void)0)
#	define TStartProfile(timevar) ((void)0)
#	define TStopProfile(timevar) ((void)0)
#	define TAddProfile(timevar, time) ((void)0)
#endif
