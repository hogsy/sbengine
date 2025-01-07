
#ifdef	PLATFORM_PS2

uint32 ScratchPad_Alloc( int _Bytes );
void ScratchPad_Free( int _Bytes );

M_INLINE int64 PS2DoubleMul( int64 _a, int64 _b )
{
	uint64 _lowmask = 0xffffffff;
	int bNegResult = 0;
	
	if( _a < 0 )
	{
		bNegResult ^= 1;
		_a = -_a;
	}
	if( _b < 0 )
	{
		bNegResult ^= 1;
		_b = -_b;
	}

	uint32 _a0, _b0;
	int32 _a1, _b1;
	
	int64 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	
	asm("
		and		_a0, _a, _lowmask
		and		_b0, _b, _lowmask
		dsrl32	_a1, _a, 0
		dsrl32	_b1, _b, 0
	
		multu	tmp2, _a0, _b1		// Since these are shifted we're only interested in the lo result
		multu	tmp3, _a1, _b0		// Since these are shifted we're only interested in the lo result
		multu	tmp1, _a0, _b0		// Here we are interested in both the hi and the lo result so we have to fetch the hi value
		mfhi	tmp4
		
		add		tmp5, tmp2, tmp3
		add		tmp5, tmp5, tmp4
		dsll32	tmp5, tmp5, 0
		
		daddu	tmp6, tmp5, tmp1
		");
		
	if( bNegResult )
		tmp6 = -tmp6;

	return tmp6;
}


M_INLINE int64 Time_GetModulusWork( int64 _Time, int64 _Size )
{
	int64 Counter = _Size;
	int bit = 0;
	
	if( _Time < Counter )
	{
		return _Time;
	}
	
	while( Counter < _Time )
	{
		Counter <<= 1;
		bit++;
	}
	
	// We always go 1 step to far.. compensate
	bit--;
	Counter	>>= 1;
	int modulus = 1 << bit;
	int64 Remainder = _Time - Counter;
	
	while( bit >= 0 && Remainder > 0 )
	{
		if( Counter <= Remainder )
		{
			modulus += 1 << bit;
			Remainder	-= Counter;
		}
		
		bit--;
		Counter	>>= 1;
	}
	
	return Remainder;
}


M_INLINE int64 Time_GetModulus( int64 _Time, int64 _Size )
{
	int Neg = 0;
	if( _Time < 0 )
	{
		Neg = 1;
		_Time = -_Time;
	}
	int64 Result = Time_GetModulusWork( _Time, _Size );
	if( Neg )
	{
		Result = -Result;
	}
	return Result;
}

////////////////////////////////////////////////////////
//      / operator for int64

M_INLINE int Time_GetModulusCountWork( int64 _Time, int64 _Size )
{
	int64 Counter = _Size;
	int bit = 0;
	
	if( _Time < Counter )
	{
		return 0;
	}
	
	while( Counter < _Time )
	{
		Counter <<= 1;
		bit++;
	}
	
	// We always go 1 step to far.. compensate
	bit--;
	Counter	>>= 1;
	int modulus = 1 << bit;
	int64 Remainder = _Time - Counter;
	
	while( bit >= 0 && Remainder > 0 )
	{
		if( Counter <= Remainder )
		{
			modulus += 1 << bit;
			Remainder	-= Counter;
		}
		
		bit--;
		Counter	>>= 1;
	}
	
	return modulus;
}

M_INLINE int Time_GetModulusCount( int64 _Time, int64 _Size )
{
	if( _Time < 0 )
	{
		return -Time_GetModulusCountWork( -_Time, _Size );
	}
	return Time_GetModulusCountWork( _Time, _Size );
}

////////////////////////////////////////////////////////
//      / and % operator for int64

M_INLINE int Time_GetModulusCountAndRemainderWork( int64 _Time, int64 _Size, int64& _Remainder )
{
	int64 Counter = _Size;
	int bit = 0;
	
	if( _Time < Counter )
	{
		_Remainder = _Time;
		return 0;
	}
	
	while( Counter < _Time )
	{
		Counter <<= 1;
		bit++;
	}
	
	// We always go 1 step to far.. compensate
	bit--;
	Counter	>>= 1;
	int modulus = 1 << bit;
	int64 Remainder = _Time - Counter;
	
	while( bit >= 0 && Remainder > 0 )
	{
		if( Counter <= Remainder )
		{
			modulus += 1 << bit;
			Remainder	-= Counter;
		}
		
		bit--;
		Counter	>>= 1;
	}
	
	_Remainder = Remainder;
	
	return modulus;
}

M_INLINE int Time_GetModulusCountAndRemainder( int64 _Time, int64 _Size, int64& _Remainder )
{
	int NegTime = 0;
	if( _Time < 0 )
	{
		NegTime = 1;
		_Time = -_Time;
	}
	int64 Result = Time_GetModulusCountAndRemainderWork( _Time, _Size, _Remainder );
	if( NegTime )
		Result = -Result;
	return Result;
}


class CMTime_PS2
{
private:
	uint32	m_aTime[2];
public:
	CMTime_PS2();
	CMTime_PS2( bool _bReset);

	CMTime_PS2(const CMTime_PS2 &_Time);

	bool operator != (const CMTime_PS2 &_Time);
	
	CMTime_PS2& operator = (const CMTime_PS2 &_Time);
	CMTime_PS2 operator + (const CMTime_PS2 &_Time) const;
	CMTime_PS2 operator - (const CMTime_PS2 &_Time) const;
	CMTime_PS2& operator += (const CMTime_PS2 &_Time);
	CMTime_PS2& operator -= (const CMTime_PS2 &_Time);

	void Reset();

	bool IsReset() const;
	static CMTime_PS2 CreateInvalid();
	void MakeInvalid();
	bool IsInvalid() const;
	void ResetMax();
	void Snapshot();
	void Start();
	void Stop();
	void Add(CMTime_PS2 &_Time);

	fp4 GetTime() const;

	int64 GetCycles() const;

	int Compare(const CMTime_PS2 &_Time) const;

	int Compare(fp4 _Time) const;

	CMTime_PS2 Modulus(fp4 _Modulus) const;
	CMTime_PS2 Modulus(const CMTime_PS2& _Modulus) const;
	CMTime_PS2 ModulusScaled(fp4 _Scale, fp4 _Modulus) const;
	CMTime_PS2 Scale(fp4 _Scale) const;

	fp4 GetTimeFraction(fp4 _Modulus) const;
	int GetNumModulus(fp4 _Modulus) const;
	int GetNumTicks(fp4 _TicksPerSec) const;
	fp4 GetTimeModulus(fp4 _Modulus) const;
	fp4 GetTimeModulusScaled(fp4 _Scale, fp4 _Modulus) const;
	fp4 GetTimeSqrModulusScaled(fp4 _Scale, fp4 _Modulus) const;

	CMTime_PS2 Max(const CMTime_PS2 &_Time) const;
	CMTime_PS2 Min(const CMTime_PS2 &_Time) const;

	static CMTime_PS2 GetCPU();
	static CMTime_PS2 CreateFromSeconds(fp4 _Seconds);
	static CMTime_PS2 CreateFromTicks(int _Ticks, fp4 _TickLength, fp4 _TickFraction = 0.0f);

	void Pack(uint8 *&_pPtr) const;
	void Unpack(const uint8 *&_pPtr);
	void Write(class CCFile *_pFile) const;
	void Read(class CCFile *_pFile);
};

//#define	DEBUG_PS2_TIME
#ifndef	DEBUG_PS2_TIME

union PS2TimeUnion
{
	int64 m_Time64;
	struct
	{
		uint32 lo;
		int32 hi;
	} m_Time32;
};

M_INLINE CMTime_PS2::CMTime_PS2()
{
	Reset();
}

M_INLINE CMTime_PS2::CMTime_PS2( bool _bReset )
{
	if( _bReset )
		Reset();
}

M_INLINE CMTime_PS2::CMTime_PS2(const CMTime_PS2 &_Time)
{
	m_aTime[0]	= _Time.m_aTime[0];
	m_aTime[1]	= _Time.m_aTime[1];
}

M_INLINE bool CMTime_PS2::operator != (const CMTime_PS2 &_Time)
{
	return ( m_aTime[0] != _Time.m_aTime[0] ) || ( m_aTime[1] != _Time.m_aTime[1] );
}

M_INLINE CMTime_PS2& CMTime_PS2::operator = (const CMTime_PS2 &_Time)
{
	m_aTime[0]	= _Time.m_aTime[0];
	m_aTime[1]	= _Time.m_aTime[1];
	return *this;
}

M_INLINE CMTime_PS2 CMTime_PS2::operator + (const CMTime_PS2 &_Time) const
{
	PS2TimeUnion Time1;
	PS2TimeUnion Time2;

	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= _Time.m_aTime[0];
	Time2.m_Time32.hi	= _Time.m_aTime[1];

	Time1.m_Time64	= Time1.m_Time64 + Time2.m_Time64;

	CMTime_PS2 Time( false );
	Time.m_aTime[0]	= Time1.m_Time32.lo;
	Time.m_aTime[1]	= Time1.m_Time32.hi;
	return Time;
}

M_INLINE CMTime_PS2 CMTime_PS2::operator - (const CMTime_PS2 &_Time) const
{
	PS2TimeUnion Time1;
	PS2TimeUnion Time2;

	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= _Time.m_aTime[0];
	Time2.m_Time32.hi	= _Time.m_aTime[1];
	
	Time1.m_Time64	= Time1.m_Time64 - Time2.m_Time64;

	CMTime_PS2 RetTime( false );
	RetTime.m_aTime[0] = Time1.m_Time32.lo;
	RetTime.m_aTime[1] = Time1.m_Time32.hi;

	return RetTime;
}

M_INLINE CMTime_PS2& CMTime_PS2::operator += (const CMTime_PS2 &_Time)
{
	PS2TimeUnion Time1;
	PS2TimeUnion Time2;

	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= _Time.m_aTime[0];
	Time2.m_Time32.hi	= _Time.m_aTime[1];

	Time1.m_Time64	= Time1.m_Time64 + Time2.m_Time64;

	m_aTime[0]	= Time1.m_Time32.lo;
	m_aTime[1]	= Time1.m_Time32.hi;

	return *this;
}

M_INLINE CMTime_PS2& CMTime_PS2::operator -= (const CMTime_PS2 &_Time)
{
	PS2TimeUnion Time1;
	PS2TimeUnion Time2;

	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= _Time.m_aTime[0];
	Time2.m_Time32.hi	= _Time.m_aTime[1];
	
	Time1.m_Time64	= Time1.m_Time64 - Time2.m_Time64;

	m_aTime[0]	= Time1.m_Time32.lo;
	m_aTime[1]	= Time1.m_Time32.hi;

	return *this;
}

M_INLINE void CMTime_PS2::Reset()
{
	m_aTime[0]	= 0;
	m_aTime[1]	= 0;
}

M_INLINE bool CMTime_PS2::IsReset() const
{
	return (m_aTime[0] == 0 ) && ( m_aTime[1] == 0 );
}

M_INLINE CMTime_PS2 CMTime_PS2::CreateInvalid()
{
	CMTime_PS2 Invalid( false );
	Invalid.MakeInvalid();
	return Invalid;
}

M_INLINE void CMTime_PS2::MakeInvalid()
{
	m_aTime[0]	= 0xffffffff;
	m_aTime[1]	= 0xffffffff;
}

M_INLINE bool CMTime_PS2::IsInvalid() const
{
	return ( m_aTime[0] == 0xffffffff ) && ( m_aTime[1] == 0xffffffff );
}

M_INLINE void CMTime_PS2::ResetMax()
{
	m_aTime[0]	= 0xffffffff;
	m_aTime[1]	= 0x7fffffff;
}

M_INLINE void CMTime_PS2::Snapshot()
{
	PS2TimeUnion Time;
	
	Time.m_Time64 = MGetCPUClock();

	m_aTime[0] = Time.m_Time32.lo;
	m_aTime[1] = Time.m_Time32.hi;
}

M_INLINE void CMTime_PS2::Start()
{
	PS2TimeUnion Time;
	
	Time.m_Time64 = -MGetCPUClock();

	m_aTime[0] = Time.m_Time32.lo;
	m_aTime[1] = Time.m_Time32.hi;
}

M_INLINE void CMTime_PS2::Stop()
{
	CMTime_PS2 moo( false );
	PS2TimeUnion Time;
	
	Time.m_Time64 = MGetCPUClock();
	moo.m_aTime[0] = Time.m_Time32.lo;
	moo.m_aTime[1] = Time.m_Time32.hi;

	*this	+= moo;
}

M_INLINE void CMTime_PS2::Add(CMTime_PS2 &_Time)
{
	*this += _Time;
}

M_INLINE fp4 CMTime_PS2::GetTime() const
{
	uint32 lo;
	int32 hi;

	fp4 Time0, Time1;
	fp4 Recp = MGetCPUFrequencyRecp();

	lo	= m_aTime[0];
	hi	= (int32)m_aTime[1];
	if( hi < 0 )
	{
		Recp = -Recp;
		lo	= -lo;
		hi	= -hi;
	}
	
	Time0 = lo * Recp;
	Time1 = hi * (Recp * (fp4)(2147483648.0f*2.0f));
	
	return Time0 + Time1;
}

M_INLINE int64 CMTime_PS2::GetCycles() const
{
	PS2TimeUnion Time;

	Time.m_Time32.lo	= m_aTime[0];
	Time.m_Time32.hi	= m_aTime[1];

	return Time.m_Time64;
}

M_INLINE int CMTime_PS2::Compare(const CMTime_PS2 &_Time) const
{

//	if (m_Time < _Time.m_Time)
//		return -1;
//	else if (m_Time > _Time.m_Time)
//		return 1;

	int32 hi0 = (int32)m_aTime[1];
	int32 hi1 = (int32)_Time.m_aTime[1];
	if( hi0 < hi1 )
		return -1;
	else if( hi0 > hi1 )
		return 1;
	else
	{
		uint32 lo0 = m_aTime[0];
		uint32 lo1 = _Time.m_aTime[0];
		if( ((int32)m_aTime[1]) < 0 )
		{
			// Time is negative
			lo0	= -lo0;
			lo1	= -lo1;
		}
		if( lo0 < lo1 )
			return -1;
		else if( lo0 > lo1 )
			return 1;
	}

	return 0;
}

M_INLINE int CMTime_PS2::Compare(fp4 _Time) const
{
	return Compare( CreateFromSeconds( _Time ) );
}

M_INLINE CMTime_PS2 CMTime_PS2::Modulus(fp4 _Modulus) const
{
	CMTime_PS2 Modul = CMTime_PS2::CreateFromSeconds( _Modulus );
	PS2TimeUnion Time1, Time2, RetTime;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= Modul.m_aTime[0];
	Time2.m_Time32.hi	= Modul.m_aTime[1];
	
	int NegMod = 0;
	if( Time2.m_Time64 < 0 )
	{
		NegMod = 1;
		Time2.m_Time64 = -Time2.m_Time64;
	}
	
//	RetTime.m_Time64 = Time1.m_Time64 % Time2.m_Time64;
	RetTime.m_Time64 = Time_GetModulus( Time1.m_Time64, Time2.m_Time64 );
	if( NegMod )
	{
		RetTime.m_Time64 = -RetTime.m_Time64;
	}

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0]	= RetTime.m_Time32.lo;
	Ret.m_aTime[1]	= RetTime.m_Time32.hi;
	
	return Ret;
}

M_INLINE CMTime_PS2 CMTime_PS2::Modulus(const CMTime_PS2& _Modulus) const
{
	PS2TimeUnion Time1, Time2, RetTime;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= _Modulus.m_aTime[0];
	Time2.m_Time32.hi	= _Modulus.m_aTime[1];

	int NegMod = 0;
	if( Time2.m_Time64 < 0 )
	{
		NegMod = 1;
		Time2.m_Time64 = -Time2.m_Time64;
	}
	
//	RetTime.m_Time64 = Time1.m_Time64 % Time2.m_Time64;
	RetTime.m_Time64 = Time_GetModulus( Time1.m_Time64, Time2.m_Time64 );
	if( NegMod )
	{
		RetTime.m_Time64 = -RetTime.m_Time64;
	}

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0]	= RetTime.m_Time32.lo;
	Ret.m_aTime[1]	= RetTime.m_Time32.hi;

	return Ret;
}

M_INLINE CMTime_PS2 CMTime_PS2::ModulusScaled(fp4 _Scale, fp4 _Modulus) const
{
	_Modulus = _Modulus * (1.0f / _Scale);

	CMTime_PS2 Modul = CMTime_PS2::CreateFromSeconds( _Modulus );
	int64 Int64Scale = (_Scale * 65536.0f);
	// This isn't really safe, should be done with 128 bit ints

	PS2TimeUnion Time1, Time2, RetTime;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= Modul.m_aTime[0];
	Time2.m_Time32.hi	= Modul.m_aTime[1];

//	RetTime.m_Time64 = (((Time1.m_Time64 % Time2.m_Time64)) * Int64Scale) >> 16;
	RetTime.m_Time64 = ((Time_GetModulus(Time1.m_Time64, Time2.m_Time64)) * Int64Scale) >> 16;

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0]	= RetTime.m_Time32.lo;
	Ret.m_aTime[1]	= RetTime.m_Time32.hi;

	return Ret;
}

M_INLINE CMTime_PS2 CMTime_PS2::Scale(fp4 _Scale) const
{
/*
	CMTime_PS2 Ret;

	int64 Int64Scale = (_Scale * 65536.0f);
	// This isn't really safe, should be done with 128 bit ints

	PS2TimeUnion Time1, RetTime;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];

	RetTime.m_Time64 = (Time1.m_Time64 * Int64Scale) >> 16;
	Ret.m_aTime[0]	= RetTime.m_Time32.lo;
	Ret.m_aTime[1]	= RetTime.m_Time32.hi;

	return Ret;
*/
	if( _Scale == 1.0f )
		return *this;

	int32 Scale = _Scale * 65536.0f;
	PS2TimeUnion Time1, RetTime;
	Time1.m_Time32.lo = m_aTime[0];
	Time1.m_Time32.hi = m_aTime[1];
	
	RetTime.m_Time64 = PS2DoubleMul( Time1.m_Time64, Scale ) >> 16;

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0] = RetTime.m_Time32.lo;
	Ret.m_aTime[1] = RetTime.m_Time32.hi;
	
	return Ret;
}

M_INLINE fp4 CMTime_PS2::GetTimeFraction(fp4 _Modulus) const
{
	return GetTimeModulus(_Modulus) * (1.0f / _Modulus);
}

M_INLINE int CMTime_PS2::GetNumModulus(fp4 _Modulus) const
{
	CMTime_PS2 Modulus = CMTime_PS2::CreateFromSeconds( _Modulus );

	PS2TimeUnion Time1, Time2, RetTime;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= Modulus.m_aTime[0];
	Time2.m_Time32.hi	= Modulus.m_aTime[1];

//	return Time1.m_Time64 / Int64Modulus;
	return Time_GetModulusCount( Time1.m_Time64, Time2.m_Time64 );
}

M_INLINE int CMTime_PS2::GetNumTicks(fp4 _TicksPerSec) const
{
	int64 Int64Scale = (_TicksPerSec * 65536.0f);
	// This isn't really safe, should be done with 128 bit ints
	PS2TimeUnion Time1;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];

	int64 Ticks = (Time1.m_Time64 * Int64Scale) >> 16;

//	return Ticks / MGetCPUFrequencyInt();
	return Time_GetModulusCount( Ticks, MGetCPUFrequencyInt() );
}

M_INLINE fp4 CMTime_PS2::GetTimeModulus(fp4 _Modulus) const
{
	CMTime_PS2 Modulus = CMTime_PS2::CreateFromSeconds( _Modulus );

	PS2TimeUnion Time1, Time2;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= Modulus.m_aTime[0];
	Time2.m_Time32.hi	= Modulus.m_aTime[1];

//	int64 Time = Time1.m_Time64 % Int64Modulus;
	Time1.m_Time64 = Time_GetModulus( Time1.m_Time64, Time2.m_Time64 );
	
	fp4 Recp = MGetCPUFrequencyRecp();;
	if( Time1.m_Time64 < 0 )
	{
		Time1.m_Time64	= -Time1.m_Time64;
		Recp = -Recp;
		
	}

	fp4 RetTime0 = Time1.m_Time32.lo * Recp;
	fp4 RetTime1 = Time1.m_Time32.hi * (Recp * (fp4)(2147483648.0f*2.0f));
	return (RetTime0 + RetTime1);
//	return (fp4)Time * MGetCPUFrequencyRecp();
}

M_INLINE fp4 CMTime_PS2::GetTimeModulusScaled(fp4 _Scale, fp4 _Modulus) const
{
	if( _Scale == 0.0f )
	{
		// Specialcase this fucker
		return 0.0f;
	}
	fp4 ScaleSign = 1.0f;
	if( _Scale < 0.0f )
	{
		ScaleSign = -ScaleSign;
		_Scale = -_Scale;
	}

	_Modulus = _Modulus * (1.0f / _Scale);

	const fp4 InvCPUFrequency = MGetCPUFrequencyRecp();
	
//	int64 Int64Modulus = _Modulus * CPUFrequency;
	CMTime_PS2 Modulus = CMTime_PS2::CreateFromSeconds( _Modulus );

	PS2TimeUnion Time1, Time2;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= Modulus.m_aTime[0];
	Time2.m_Time32.hi	= Modulus.m_aTime[1];

	int NegMod = 0;
	if( Time2.m_Time64 < 0 )
	{
		NegMod = 1;
		Time2.m_Time64 = -Time2.m_Time64;
	}

//	int64 Time = Time_GetModulus( Time1.m_Time64, Int64Modulus );
	int64 Time = Time_GetModulus( Time1.m_Time64, Time2.m_Time64 );
	if( NegMod )
	{
		Time = -Time;
	}

	int64 Remainder;
	int64 Count = Time_GetModulusCountAndRemainder( Time, MGetCPUFrequencyInt(), Remainder );

//	fp4 Modded = (fp4)Time * MGetCPUFrequencyRecp();
	fp4 Modded = (int)Count + ((fp4)((int)Remainder)) * InvCPUFrequency;
	return Modded * ScaleSign * _Scale;
}

M_INLINE fp4 CMTime_PS2::GetTimeSqrModulusScaled(fp4 _Scale, fp4 _Modulus) const
{
	if( _Scale == 0.0f )
	{
		// Specialcase this fucker
		return 0.0f;
	}
	fp4 ScaleSign = 1.0f;
	if( _Scale < 0.0f )
	{
		ScaleSign = -ScaleSign;
		_Scale = -_Scale;
	}

	_Scale *= GetTime();

	_Modulus = _Modulus * (1.0f / _Scale);

	const fp4 InvCPUFrequency = MGetCPUFrequencyRecp();

//	int64 Int64Modulus = _Modulus * CPUFrequency;
	CMTime_PS2 Modulus = CMTime_PS2::CreateFromSeconds( _Modulus );

	PS2TimeUnion Time1, Time2;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= Modulus.m_aTime[0];
	Time2.m_Time32.hi	= Modulus.m_aTime[1];

	int NegMod = 0;
	if( Time2.m_Time64 < 0 )
	{
		NegMod = 1;
		Time2.m_Time64 = -Time2.m_Time64;
	}

	int64 TimeSqr = Time1.m_Time64;

//	int64 Time = TimeSqr % Int64Modulus;
	int64 Time;
	Time = Time_GetModulus( TimeSqr, Time2.m_Time64 );

	if( NegMod )
	{
		Time = -Time;
	}

	int64 Remainder;
	int64 Count = Time_GetModulusCountAndRemainder( Time, MGetCPUFrequencyInt(), Remainder );

//	fp4 Modded = (fp4)Time * MGetCPUFrequencyRecp();
	fp4 Modded = (int)Count + ((fp4)((int)Remainder)) * InvCPUFrequency;
	return Modded * ScaleSign * _Scale;
}

M_INLINE CMTime_PS2 CMTime_PS2::Max(const CMTime_PS2 &_Time) const
{
	PS2TimeUnion Time1, Time2, RetTime;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= _Time.m_aTime[0];
	Time2.m_Time32.hi	= _Time.m_aTime[1];

	RetTime.m_Time64 = ::Max(Time1.m_Time64, Time2.m_Time64);

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0] = RetTime.m_Time32.lo;
	Ret.m_aTime[1] = RetTime.m_Time32.hi;

	return Ret;
}

M_INLINE CMTime_PS2 CMTime_PS2::Min(const CMTime_PS2 &_Time) const
{
	PS2TimeUnion Time1, Time2, RetTime;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= _Time.m_aTime[0];
	Time2.m_Time32.hi	= _Time.m_aTime[1];

	RetTime.m_Time64 = ::Min(Time1.m_Time64, Time2.m_Time64);

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0] = RetTime.m_Time32.lo;
	Ret.m_aTime[1] = RetTime.m_Time32.hi;

	return Ret;
}


M_INLINE CMTime_PS2 CMTime_PS2::GetCPU()
{
	PS2TimeUnion Time1;
	Time1.m_Time64 = MRTC_SystemInfo::CPU_GetCycles();

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0]	= Time1.m_Time32.lo;
	Ret.m_aTime[1]	= Time1.m_Time32.hi;
	return Ret;
}

M_INLINE CMTime_PS2 CMTime_PS2::CreateFromSeconds(fp4 _Seconds)
{
	int IntPart = (int)_Seconds;
	fp4 FracPart = _Seconds - (fp4)IntPart;
	int freqint = 294912000;
	fp4 freqfp4 = 294912000;
	uint64 _hi, _lo;
	fp4 tmp;
	asm("
		mult	IntPart, freqint
		mul.s	tmp, FracPart, freqfp4
		cvt.w.s	tmp,tmp
		mfhi	_hi
		mflo	_lo
		pextlw	_lo, _hi, _lo
		mfc1	_hi, tmp
		daddu	_lo, _lo, _hi
		");

	PS2TimeUnion Time1;
	Time1.m_Time64 = _lo;

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0]	= Time1.m_Time32.lo;
	Ret.m_aTime[1]	= Time1.m_Time32.hi;

	return Ret;
}

M_INLINE CMTime_PS2 CMTime_PS2::CreateFromTicks(int _Ticks, fp4 _TickLength, fp4 _TickFraction )
{
	fp4 freqfp4 = 294912000.0f;
	fp4 TickLenfp4;
	int TickLenint;
	int64 Fracint;

	fp4 tmp;
	int64 _hi, _lo;
	asm("
		mul.s	TickLenfp4, _TickLength, freqfp4

		cvt.w.s	tmp, TickLenfp4
		mfc1	TickLenint, tmp

		mul.s	tmp, TickLenfp4, _TickFraction
		mult	TickLenint, _Ticks
		cvt.w.s	tmp, tmp
		mfc1	Fracint, tmp
		mfhi	_hi
		mflo	_lo
		pextlw	_lo, _hi, _lo
		daddu	_lo, _lo, Fracint
		");

	PS2TimeUnion Time1;
	Time1.m_Time64 = _lo;

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0]	= Time1.m_Time32.lo;
	Ret.m_aTime[1]	= Time1.m_Time32.hi;
	
	return Ret;
}

M_INLINE void CMTime_PS2::Pack(uint8 *&_pPtr) const
{
	memcpy( _pPtr, &m_aTime, 8 );
	_pPtr += 8;
}

M_INLINE void CMTime_PS2::Unpack(const uint8 *&_pPtr)
{
	memcpy( &m_aTime, _pPtr, 8 );
	_pPtr += 8;
}

#endif	// DEBUG_PS2_TIME

typedef CMTime_PS2	CMTime;

#endif
