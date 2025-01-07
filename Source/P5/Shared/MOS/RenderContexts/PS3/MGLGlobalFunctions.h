
#ifndef DInc_MGLGlobalFunctions_h
#define DInc_MGLGlobalFunctions_h

#ifdef M_Profile
//#define GL_FUNCTIMING
#endif

// -------------------------------------------------------------------
#ifdef GL_FUNCTIMING
typedef CMTime_Generic<CMTimerFuncs_CPU> CMGLTimer;

class CGLFunctionTimer
{
public:
	CGLFunctionTimer(const char *_pName);
	const char *m_pName;
	int64 m_Calls;
	CMGLTimer m_Timer;

	DIdsListLinkD_Link(CGLFunctionTimer, m_Link);
};

extern DIdsListLinkDA_List(CGLFunctionTimer, m_Link) g_GlobalTimers;



#define GLN_DeclareFunc(_Name) CGLFunctionTimer m_Time_##_Name;

#define GLN_ImplementConstructor(_Name) m_Time_##_Name(#_Name)

#define GLE_DeclareFunc(_Type, _Name)

#define GLC_Declare_P0(_Name, _Global) \
GLN_DeclareFunc(_Name) \
M_FORCEINLINE void f_##_Name()\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name();\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}
#define GLC_Declare_P1(_Name, _Global) \
GLN_DeclareFunc(_Name) \
template <typename t_0>\
M_FORCEINLINE void f_##_Name(t_0 _p0)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name(_p0);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}
#define GLC_Declare_P2(_Name, _Global) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1>\
M_FORCEINLINE void f_##_Name(t_0 _p0, t_1 _p1)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name(_p0, _p1);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}
#define GLC_Declare_P3(_Name, _Global) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2>\
M_FORCEINLINE void f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name(_p0, _p1, _p2);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}
#define GLC_Declare_P4(_Name, _Global) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3>\
M_FORCEINLINE void f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name(_p0, _p1, _p2, _p3);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}
#define GLC_Declare_P5(_Name, _Global) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4>\
M_FORCEINLINE void f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name(_p0, _p1, _p2, _p3, _p4);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}
#define GLC_Declare_P6(_Name, _Global) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4, typename t_5>\
M_FORCEINLINE void f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4, t_5 _p5)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name(_p0, _p1, _p2, _p3, _p4, _p5);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}
#define GLC_Declare_P7(_Name, _Global) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4, typename t_5, typename t_6>\
M_FORCEINLINE void f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4, t_5 _p5, t_6 _p6)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name(_p0, _p1, _p2, _p3, _p4, _p5, _p6);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}
#define GLC_Declare_P8(_Name, _Global) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4, typename t_5, typename t_6, typename t_7>\
M_FORCEINLINE void f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4, t_5 _p5, t_6 _p6, t_7 _p7)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name(_p0, _p1, _p2, _p3, _p4, _p5, _p6, _p7);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}
#define GLC_Declare_P9(_Name, _Global) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4, typename t_5, typename t_6, typename t_7, typename t_8>\
M_FORCEINLINE void f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4, t_5 _p5, t_6 _p6, t_7 _p7, t_8 _p8)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name(_p0, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}
#define GLC_Declare_P10(_Name, _Global) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4, typename t_5, typename t_6, typename t_7, typename t_8, typename t_9>\
M_FORCEINLINE void f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4, t_5 _p5, t_6 _p6, t_7 _p7, t_8 _p8, t_9 _p9)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name(_p0, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8, _p9);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}
#define GLC_Declare_P11(_Name, _Global) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4, typename t_5, typename t_6, typename t_7, typename t_8, typename t_9, typename t_10>\
M_FORCEINLINE void f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4, t_5 _p5, t_6 _p6, t_7 _p7, t_8 _p8, t_9 _p9, t_10 _p10)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name(_p0, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8, _p9, _p10);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}
#define GLC_Declare_P12(_Name, _Global) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4, typename t_5, typename t_6, typename t_7, typename t_8, typename t_9, typename t_10, typename t_11>\
M_FORCEINLINE void f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4, t_5 _p5, t_6 _p6, t_7 _p7, t_8 _p8, t_9 _p9, t_10 _p10, t_11 _p11)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name(_p0, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8, _p9, _p10, _p11);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}
#define GLC_Declare_P13(_Name, _Global) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4, typename t_5, typename t_6, typename t_7, typename t_8, typename t_9, typename t_10, typename t_11, typename t_12>\
M_FORCEINLINE void f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4, t_5 _p5, t_6 _p6, t_7 _p7, t_8 _p8, t_9 _p9, t_10 _p10, t_11 _p11, t_12 _p12)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name(_p0, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8, _p9, _p10, _p11, _p12);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}
#define GLC_Declare_P14(_Name, _Global) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4, typename t_5, typename t_6, typename t_7, typename t_8, typename t_9, typename t_10, typename t_11, typename t_12, typename t_13>\
M_FORCEINLINE void f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4, t_5 _p5, t_6 _p6, t_7 _p7, t_8 _p8, t_9 _p9, t_10 _p10, t_11 _p11, t_12 _p12, t_13 _p13)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Name(_p0, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8, _p9, _p10, _p11, _p12, _p13);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
}

// Functions

#define GLC_Declare_F0(_Name, _Global, _Returns) \
GLN_DeclareFunc(_Name) \
M_FORCEINLINE _Returns f_##_Name()\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Returns Ret = _Name();\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
	return Ret;\
}
#define GLC_Declare_F1(_Name, _Global, _Returns ) \
GLN_DeclareFunc(_Name) \
template <typename t_0>\
M_FORCEINLINE _Returns f_##_Name(t_0 _p0)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Returns Ret = _Name(_p0);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
	return Ret;\
}
#define GLC_Declare_F2(_Name, _Global, _Returns ) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1>\
M_FORCEINLINE _Returns f_##_Name(t_0 _p0, t_1 _p1)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Returns Ret = _Name(_p0, _p1);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
	return Ret;\
}
#define GLC_Declare_F3(_Name, _Global, _Returns ) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2>\
M_FORCEINLINE _Returns f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Returns Ret = _Name(_p0, _p1, _p2);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
	return Ret;\
}
#define GLC_Declare_F4(_Name, _Global, _Returns ) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3>\
M_FORCEINLINE _Returns f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Returns Ret = _Name(_p0, _p1, _p2, _p3);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
	return Ret;\
}
#define GLC_Declare_F5(_Name, _Global, _Returns ) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4>\
M_FORCEINLINE _Returns f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Returns Ret = _Name(_p0, _p1, _p2, _p3, _p4);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
	return Ret;\
}
#define GLC_Declare_F6(_Name, _Global, _Returns) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4, typename t_5>\
M_FORCEINLINE _Returns f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4, t_5 _p5)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Returns Ret = _Name(_p0, _p1, _p2, _p3, _p4, _p5);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
	return Ret;\
}
#define GLC_Declare_F7(_Name, _Global, _Returns) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4, typename t_5, typename t_6>\
M_FORCEINLINE _Returns f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4, t_5 _p5, t_6 _p6)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Returns Ret = _Name(_p0, _p1, _p2, _p3, _p4, _p5, _p6);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
	return Ret;\
}
#define GLC_Declare_F8(_Name, _Global, _Returns) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4, typename t_5, typename t_6, typename t_7>\
M_FORCEINLINE _Returns f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4, t_5 _p5, t_6 _p6, t_7 _p7)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Returns Ret = _Name(_p0, _p1, _p2, _p3, _p4, _p5, _p6, _p7);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
	return Ret;\
}
#define GLC_Declare_F9(_Name, _Global, _Returns) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4, typename t_5, typename t_6, typename t_7, typename t_8>\
M_FORCEINLINE _Returns f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4, t_5 _p5, t_6 _p6, t_7 _p7, t_8 _p8)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Returns Ret = _Name(_p0, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
	return Ret;\
}
#define GLC_Declare_F10(_Name, _Global, _Returns) \
GLN_DeclareFunc(_Name) \
template <typename t_0, typename t_1, typename t_2, typename t_3, typename t_4, typename t_5, typename t_6, typename t_7, typename t_8, typename t_9>\
M_FORCEINLINE _Returns f_##_Name(t_0 _p0, t_1 _p1, t_2 _p2, t_3 _p3, t_4 _p4, t_5 _p5, t_6 _p6, t_7 _p7, t_8 _p8, t_9 _p9)\
{\
	CMGLTimer Timer;\
	TStart(Timer)\
	_Returns Ret = _Name(_p0, _p1, _p2, _p3, _p4, _p5, _p6, _p7, _p8, _p9);\
	TStop(Timer)\
	m_Time_##_Name.m_Timer += Timer;\
	(_Global).m_GlobalTimer += Timer;\
	++m_Time_##_Name.m_Calls;\
	++(_Global).m_GlobalCalls;\
	return Ret;\
}


#define GLN_Declare_P0(_Name) GLC_Declare_P0(_Name, (*this))
#define GLN_Declare_P1(_Name) GLC_Declare_P1(_Name, (*this))
#define GLN_Declare_P2(_Name) GLC_Declare_P2(_Name, (*this))
#define GLN_Declare_P3(_Name) GLC_Declare_P3(_Name, (*this))
#define GLN_Declare_P4(_Name) GLC_Declare_P4(_Name, (*this))
#define GLN_Declare_P5(_Name) GLC_Declare_P5(_Name, (*this))
#define GLN_Declare_P6(_Name) GLC_Declare_P6(_Name, (*this))
#define GLN_Declare_P7(_Name) GLC_Declare_P7(_Name, (*this))
#define GLN_Declare_P8(_Name) GLC_Declare_P8(_Name, (*this))
#define GLN_Declare_P9(_Name) GLC_Declare_P9(_Name, (*this))
#define GLN_Declare_P10(_Name) GLC_Declare_P10(_Name, (*this))
#define GLN_Declare_P11(_Name) GLC_Declare_P11(_Name, (*this))
#define GLN_Declare_P12(_Name) GLC_Declare_P12(_Name, (*this))
#define GLN_Declare_P13(_Name) GLC_Declare_P13(_Name, (*this))
#define GLN_Declare_P14(_Name) GLC_Declare_P14(_Name, (*this))

#define GLN_Declare_F0(_Name, _Returns) GLC_Declare_F0(_Name, (*this), _Returns)
#define GLN_Declare_F1(_Name, _Returns) GLC_Declare_F1(_Name, (*this), _Returns)
#define GLN_Declare_F2(_Name, _Returns) GLC_Declare_F2(_Name, (*this), _Returns)
#define GLN_Declare_F3(_Name, _Returns) GLC_Declare_F3(_Name, (*this), _Returns)
#define GLN_Declare_F4(_Name, _Returns) GLC_Declare_F4(_Name, (*this), _Returns)
#define GLN_Declare_F5(_Name, _Returns) GLC_Declare_F5(_Name, (*this), _Returns) 
#define GLN_Declare_F6(_Name, _Returns) GLC_Declare_F6(_Name, (*this), _Returns)
#define GLN_Declare_F7(_Name, _Returns) GLC_Declare_F7(_Name, (*this), _Returns)
#define GLN_Declare_F8(_Name, _Returns) GLC_Declare_F8(_Name, (*this), _Returns)
#define GLN_Declare_F9(_Name, _Returns) GLC_Declare_F9(_Name, (*this), _Returns)
#define GLN_Declare_F10(_Name, _Returns) GLC_Declare_F10(_Name, (*this), _Returns)

#define GLE_Declare_P0(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P0(m_fp_##_Name, g_GlobalTiming)
#define GLE_Declare_P1(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P1(m_fp_##_Name, g_GlobalTiming)
#define GLE_Declare_P2(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P2(m_fp_##_Name, g_GlobalTiming)
#define GLE_Declare_P3(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P3(m_fp_##_Name, g_GlobalTiming)
#define GLE_Declare_P4(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P4(m_fp_##_Name, g_GlobalTiming)
#define GLE_Declare_P5(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P5(m_fp_##_Name, g_GlobalTiming)
#define GLE_Declare_P6(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P6(m_fp_##_Name, g_GlobalTiming)
#define GLE_Declare_P7(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P7(m_fp_##_Name, g_GlobalTiming)
#define GLE_Declare_P8(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P8(m_fp_##_Name, g_GlobalTiming)
#define GLE_Declare_P9(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P9(m_fp_##_Name, g_GlobalTiming)
#define GLE_Declare_P10(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P10(m_fp_##_Name, g_GlobalTiming)
#define GLE_Declare_P11(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P11(m_fp_##_Name, g_GlobalTiming)
#define GLE_Declare_P12(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P12(m_fp_##_Name, g_GlobalTiming)
#define GLE_Declare_P13(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P13(m_fp_##_Name, g_GlobalTiming)
#define GLE_Declare_P14(_Type, _Name) GLE_DeclareFunc(_Type, _Name) GLC_Declare_P14(m_fp_##_Name, g_GlobalTiming)

#define GLE_Declare_F0(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name) GLC_Declare_F0(m_fp_##_Name, g_GlobalTiming, _Returns)
#define GLE_Declare_F1(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name) GLC_Declare_F1(m_fp_##_Name, g_GlobalTiming, _Returns)
#define GLE_Declare_F2(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name) GLC_Declare_F2(m_fp_##_Name, g_GlobalTiming, _Returns)
#define GLE_Declare_F3(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name) GLC_Declare_F3(m_fp_##_Name, g_GlobalTiming, _Returns)
#define GLE_Declare_F4(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name) GLC_Declare_F4(m_fp_##_Name, g_GlobalTiming, _Returns)
#define GLE_Declare_F5(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name) GLC_Declare_F5(m_fp_##_Name, g_GlobalTiming, _Returns) 
#define GLE_Declare_F6(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name) GLC_Declare_F6(m_fp_##_Name, g_GlobalTiming, _Returns)
#define GLE_Declare_F7(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name) GLC_Declare_F7(m_fp_##_Name, g_GlobalTiming, _Returns)
#define GLE_Declare_F8(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name) GLC_Declare_F8(m_fp_##_Name, g_GlobalTiming, _Returns)
#define GLE_Declare_F9(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name) GLC_Declare_F9(m_fp_##_Name, g_GlobalTiming, _Returns)
#define GLE_Declare_F10(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name) GLC_Declare_F10(m_fp_##_Name, g_GlobalTiming, _Returns)

#define GLE_ImplementConstructor(_Name) GLN_ImplementConstructor(m_fp_##_Name)

#else
	#define GLN_DeclareFunc(_Name)

	#define GLE_DeclareFunc(_Type, _Name)

#define GLN_Declare_P0(_Name) GLN_DeclareFunc(_Name)
#define GLN_Declare_P1(_Name) GLN_DeclareFunc(_Name)
#define GLN_Declare_P2(_Name) GLN_DeclareFunc(_Name)
#define GLN_Declare_P3(_Name) GLN_DeclareFunc(_Name)
#define GLN_Declare_P4(_Name) GLN_DeclareFunc(_Name)
#define GLN_Declare_P5(_Name) GLN_DeclareFunc(_Name)
#define GLN_Declare_P6(_Name) GLN_DeclareFunc(_Name)
#define GLN_Declare_P7(_Name) GLN_DeclareFunc(_Name)
#define GLN_Declare_P8(_Name) GLN_DeclareFunc(_Name)
#define GLN_Declare_P9(_Name) GLN_DeclareFunc(_Name)
#define GLN_Declare_P10(_Name) GLN_DeclareFunc(_Name)
#define GLN_Declare_P11(_Name) GLN_DeclareFunc(_Name)
#define GLN_Declare_P12(_Name) GLN_DeclareFunc(_Name)
#define GLN_Declare_P13(_Name) GLN_DeclareFunc(_Name)
#define GLN_Declare_P14(_Name) GLN_DeclareFunc(_Name)

#define GLN_Declare_F0(_Name, _Returns) GLN_DeclareFunc(_Name)
#define GLN_Declare_F1(_Name, _Returns) GLN_DeclareFunc(_Name)
#define GLN_Declare_F2(_Name, _Returns) GLN_DeclareFunc(_Name)
#define GLN_Declare_F3(_Name, _Returns) GLN_DeclareFunc(_Name)
#define GLN_Declare_F4(_Name, _Returns) GLN_DeclareFunc(_Name)
#define GLN_Declare_F5(_Name, _Returns) GLN_DeclareFunc(_Name)
#define GLN_Declare_F6(_Name, _Returns) GLN_DeclareFunc(_Name)
#define GLN_Declare_F7(_Name, _Returns) GLN_DeclareFunc(_Name)
#define GLN_Declare_F8(_Name, _Returns) GLN_DeclareFunc(_Name)
#define GLN_Declare_F9(_Name, _Returns) GLN_DeclareFunc(_Name)
#define GLN_Declare_F10(_Name, _Returns) GLN_DeclareFunc(_Name)

#define GLE_Declare_P0(_Type, _Name) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_P1(_Type, _Name) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_P2(_Type, _Name) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_P3(_Type, _Name) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_P4(_Type, _Name) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_P5(_Type, _Name) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_P6(_Type, _Name) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_P7(_Type, _Name) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_P8(_Type, _Name) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_P9(_Type, _Name) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_P10(_Type, _Name) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_P11(_Type, _Name) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_P12(_Type, _Name) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_P13(_Type, _Name) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_P14(_Type, _Name) GLE_DeclareFunc(_Type, _Name)

#define GLE_Declare_F0(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_F1(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_F2(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_F3(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_F4(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_F5(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_F6(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_F7(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_F8(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_F9(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name)
#define GLE_Declare_F10(_Type, _Name, _Returns) GLE_DeclareFunc(_Type, _Name)

#endif

#ifdef GL_FUNCTIMING

#define GLN_P0(_Name)											g_GlobalTiming.f_##_Name()
#define GLN_P1(_Name, p0)										g_GlobalTiming.f_##_Name(p0)
#define GLN_P2(_Name, p0, p1)									g_GlobalTiming.f_##_Name(p0, p1)
#define GLN_P3(_Name, p0, p1, p2)								g_GlobalTiming.f_##_Name(p0, p1, p2)
#define GLN_P4(_Name, p0, p1, p2, p3)							g_GlobalTiming.f_##_Name(p0, p1, p2, p3)
#define GLN_P5(_Name, p0, p1, p2, p3, p4)						g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4)
#define GLN_P6(_Name, p0, p1, p2, p3, p4, p5)					g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4, p5)
#define GLN_P7(_Name, p0, p1, p2, p3, p4, p5, p6)				g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4, p5, p6)
#define GLN_P8(_Name, p0, p1, p2, p3, p4, p5, p6, p7)			g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4, p5, p6, p7)
#define GLN_P9(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8)		g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8)
#define GLN_P10(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)	g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)
#define GLN_P11(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)	g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
#define GLN_P12(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)	g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)
#define GLN_P13(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)	g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)
#define GLN_P14(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)	g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)

#define GLN_F0(_Name)											g_GlobalTiming.f_##_Name()
#define GLN_F1(_Name, p0)										g_GlobalTiming.f_##_Name(p0)
#define GLN_F2(_Name, p0, p1)									g_GlobalTiming.f_##_Name(p0, p1)
#define GLN_F3(_Name, p0, p1, p2)								g_GlobalTiming.f_##_Name(p0, p1, p2)
#define GLN_F4(_Name, p0, p1, p2, p3)							g_GlobalTiming.f_##_Name(p0, p1, p2, p3)
#define GLN_F5(_Name, p0, p1, p2, p3, p4)						g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4)
#define GLN_F6(_Name, p0, p1, p2, p3, p4, p5)					g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4, p5)
#define GLN_F7(_Name, p0, p1, p2, p3, p4, p5, p6)				g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4, p5, p6)
#define GLN_F8(_Name, p0, p1, p2, p3, p4, p5, p6, p7)			g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4, p5, p6, p7)
#define GLN_F9(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8)		g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8)
#define GLN_F10(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)	g_GlobalTiming.f_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)

#define GLE_P0(_Name)											f_m_fp_##_Name()
#define GLE_P1(_Name, p0)										f_m_fp_##_Name(p0)
#define GLE_P2(_Name, p0, p1)									f_m_fp_##_Name(p0, p1)
#define GLE_P3(_Name, p0, p1, p2)								f_m_fp_##_Name(p0, p1, p2)
#define GLE_P4(_Name, p0, p1, p2, p3)							f_m_fp_##_Name(p0, p1, p2, p3)
#define GLE_P5(_Name, p0, p1, p2, p3, p4)						f_m_fp_##_Name(p0, p1, p2, p3, p4)
#define GLE_P6(_Name, p0, p1, p2, p3, p4, p5)					f_m_fp_##_Name(p0, p1, p2, p3, p4, p5)
#define GLE_P7(_Name, p0, p1, p2, p3, p4, p5, p6)				f_m_fp_##_Name(p0, p1, p2, p3, p4, p5, p6)
#define GLE_P8(_Name, p0, p1, p2, p3, p4, p5, p6, p7)			f_m_fp_##_Name(p0, p1, p2, p3, p4, p5, p6, p7)
#define GLE_P9(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8)		f_m_fp_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8)
#define GLE_P10(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)	f_m_fp_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)
#define GLE_P11(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)	f_m_fp_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
#define GLE_P12(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)	f_m_fp_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)
#define GLE_P13(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)	f_m_fp_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)
#define GLE_P14(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)	f_m_fp_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)

#define GLE_F0(_Name)											f_m_fp_##_Name()
#define GLE_F1(_Name, p0)										f_m_fp_##_Name(p0)
#define GLE_F2(_Name, p0, p1)									f_m_fp_##_Name(p0, p1)
#define GLE_F3(_Name, p0, p1, p2)								f_m_fp_##_Name(p0, p1, p2)
#define GLE_F4(_Name, p0, p1, p2, p3)							f_m_fp_##_Name(p0, p1, p2, p3)
#define GLE_F5(_Name, p0, p1, p2, p3, p4)						f_m_fp_##_Name(p0, p1, p2, p3, p4)
#define GLE_F6(_Name, p0, p1, p2, p3, p4, p5)					f_m_fp_##_Name(p0, p1, p2, p3, p4, p5)
#define GLE_F7(_Name, p0, p1, p2, p3, p4, p5, p6)				f_m_fp_##_Name(p0, p1, p2, p3, p4, p5, p6)
#define GLE_F8(_Name, p0, p1, p2, p3, p4, p5, p6, p7)			f_m_fp_##_Name(p0, p1, p2, p3, p4, p5, p6, p7)
#define GLE_F9(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8)		f_m_fp_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8)
#define GLE_F10(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)	f_m_fp_##_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)

#else

#define GLN_P0(_Name)											_Name()
#define GLN_P1(_Name, p0)										_Name(p0)
#define GLN_P2(_Name, p0, p1)									_Name(p0, p1)
#define GLN_P3(_Name, p0, p1, p2)								_Name(p0, p1, p2)
#define GLN_P4(_Name, p0, p1, p2, p3)							_Name(p0, p1, p2, p3)
#define GLN_P5(_Name, p0, p1, p2, p3, p4)						_Name(p0, p1, p2, p3, p4)
#define GLN_P6(_Name, p0, p1, p2, p3, p4, p5)					_Name(p0, p1, p2, p3, p4, p5)
#define GLN_P7(_Name, p0, p1, p2, p3, p4, p5, p6)				_Name(p0, p1, p2, p3, p4, p5, p6)
#define GLN_P8(_Name, p0, p1, p2, p3, p4, p5, p6, p7)			_Name(p0, p1, p2, p3, p4, p5, p6, p7)
#define GLN_P9(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8)		_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8)
#define GLN_P10(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)	_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)
#define GLN_P11(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)	_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
#define GLN_P12(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)	_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)
#define GLN_P13(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)	_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)
#define GLN_P14(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)	_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)

#define GLN_F0(_Name)											_Name()
#define GLN_F1(_Name, p0)										_Name(p0)
#define GLN_F2(_Name, p0, p1)									_Name(p0, p1)
#define GLN_F3(_Name, p0, p1, p2)								_Name(p0, p1, p2)
#define GLN_F4(_Name, p0, p1, p2, p3)							_Name(p0, p1, p2, p3)
#define GLN_F5(_Name, p0, p1, p2, p3, p4)						_Name(p0, p1, p2, p3, p4)
#define GLN_F6(_Name, p0, p1, p2, p3, p4, p5)					_Name(p0, p1, p2, p3, p4, p5)
#define GLN_F7(_Name, p0, p1, p2, p3, p4, p5, p6)				_Name(p0, p1, p2, p3, p4, p5, p6)
#define GLN_F8(_Name, p0, p1, p2, p3, p4, p5, p6, p7)			_Name(p0, p1, p2, p3, p4, p5, p6, p7)
#define GLN_F9(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8)		_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8)
#define GLN_F10(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)	_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)

#define GLE_P0(_Name)											_Name()
#define GLE_P1(_Name, p0)										_Name(p0)
#define GLE_P2(_Name, p0, p1)									_Name(p0, p1)
#define GLE_P3(_Name, p0, p1, p2)								_Name(p0, p1, p2)
#define GLE_P4(_Name, p0, p1, p2, p3)							_Name(p0, p1, p2, p3)
#define GLE_P5(_Name, p0, p1, p2, p3, p4)						_Name(p0, p1, p2, p3, p4)
#define GLE_P6(_Name, p0, p1, p2, p3, p4, p5)					_Name(p0, p1, p2, p3, p4, p5)
#define GLE_P7(_Name, p0, p1, p2, p3, p4, p5, p6)				_Name(p0, p1, p2, p3, p4, p5, p6)
#define GLE_P8(_Name, p0, p1, p2, p3, p4, p5, p6, p7)			_Name(p0, p1, p2, p3, p4, p5, p6, p7)
#define GLE_P9(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8)		_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8)
#define GLE_P10(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)	_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)
#define GLE_P11(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)	_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
#define GLE_P12(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)	_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)
#define GLE_P13(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)	_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)
#define GLE_P14(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)	_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)

#define GLE_F0(_Name)											_Name()
#define GLE_F1(_Name, p0)										_Name(p0)
#define GLE_F2(_Name, p0, p1)									_Name(p0, p1)
#define GLE_F3(_Name, p0, p1, p2)								_Name(p0, p1, p2)
#define GLE_F4(_Name, p0, p1, p2, p3)							_Name(p0, p1, p2, p3)
#define GLE_F5(_Name, p0, p1, p2, p3, p4)						_Name(p0, p1, p2, p3, p4)
#define GLE_F6(_Name, p0, p1, p2, p3, p4, p5)					_Name(p0, p1, p2, p3, p4, p5)
#define GLE_F7(_Name, p0, p1, p2, p3, p4, p5, p6)				_Name(p0, p1, p2, p3, p4, p5, p6)
#define GLE_F8(_Name, p0, p1, p2, p3, p4, p5, p6, p7)			_Name(p0, p1, p2, p3, p4, p5, p6, p7)
#define GLE_F9(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8)		_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8)
#define GLE_F10(_Name, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)	_Name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)

#endif

#ifdef GL_FUNCTIMING
class CGLGlobalTiming
{
public:
#endif

// Normal gl functions

// Procedure 0
GLN_Declare_P0(glEnd)
#define glnEnd()				GLN_P0(glEnd)
GLN_Declare_P0(glEndList)
#define glnEndList()				GLN_P0(glEndList)
GLN_Declare_P0(glFinish)
#define glnFinish()				GLN_P0(glFinish)
GLN_Declare_P0(glFlush)
#define glnFlush()				GLN_P0(glFlush)
GLN_Declare_P0(glInitNames)
#define glnInitNames()				GLN_P0(glInitNames)
GLN_Declare_P0(glLoadIdentity)
#define glnLoadIdentity()				GLN_P0(glLoadIdentity)
GLN_Declare_P0(glPopAttrib)
#define glnPopAttrib()				GLN_P0(glPopAttrib)
GLN_Declare_P0(glPopClientAttrib)
#define glnPopClientAttrib()				GLN_P0(glPopClientAttrib)
GLN_Declare_P0(glPopMatrix)
#define glnPopMatrix()				GLN_P0(glPopMatrix)
GLN_Declare_P0(glPopName)
#define glnPopName()				GLN_P0(glPopName)
GLN_Declare_P0(glPushMatrix)
#define glnPushMatrix()				GLN_P0(glPushMatrix)

// Procedure 1
GLN_Declare_P1(glArrayElement)
#define glnArrayElement(p0)				GLN_P1(glArrayElement, p0)
GLN_Declare_P1(glBegin)
#define glnBegin(p0)				GLN_P1(glBegin, p0)
GLN_Declare_P1(glCallList)
#define glnCallList(p0)				GLN_P1(glCallList, p0)
GLN_Declare_P1(glClear)
#define glnClear(p0)				GLN_P1(glClear, p0)
GLN_Declare_P1(glClearDepth)
#define glnClearDepth(p0)				GLN_P1(glClearDepth, p0)
GLN_Declare_P1(glClearIndex)
#define glnClearIndex(p0)				GLN_P1(glClearIndex, p0)
GLN_Declare_P1(glClearStencil)
#define glnClearStencil(p0)				GLN_P1(glClearStencil, p0)
GLN_Declare_P1(glColor3bv)
#define glnColor3bv(p0)				GLN_P1(glColor3bv, p0)
GLN_Declare_P1(glColor3dv)
#define glnColor3dv(p0)				GLN_P1(glColor3dv, p0)
GLN_Declare_P1(glColor3fv)
#define glnColor3fv(p0)				GLN_P1(glColor3fv, p0)
GLN_Declare_P1(glColor3iv)
#define glnColor3iv(p0)				GLN_P1(glColor3iv, p0)
GLN_Declare_P1(glColor3sv)
#define glnColor3sv(p0)				GLN_P1(glColor3sv, p0)
GLN_Declare_P1(glColor3ubv)
#define glnColor3ubv(p0)				GLN_P1(glColor3ubv, p0)
GLN_Declare_P1(glColor3uiv)
#define glnColor3uiv(p0)				GLN_P1(glColor3uiv, p0)
GLN_Declare_P1(glColor3usv)
#define glnColor3usv(p0)				GLN_P1(glColor3usv, p0)
GLN_Declare_P1(glColor4bv)
#define glnColor4bv(p0)				GLN_P1(glColor4bv, p0)
GLN_Declare_P1(glColor4dv)
#define glnColor4dv(p0)				GLN_P1(glColor4dv, p0)
GLN_Declare_P1(glColor4fv)
#define glnColor4fv(p0)				GLN_P1(glColor4fv, p0)
GLN_Declare_P1(glColor4iv)
#define glnColor4iv(p0)				GLN_P1(glColor4iv, p0)
GLN_Declare_P1(glColor4sv)
#define glnColor4sv(p0)				GLN_P1(glColor4sv, p0)
GLN_Declare_P1(glColor4ubv)
#define glnColor4ubv(p0)				GLN_P1(glColor4ubv, p0)
GLN_Declare_P1(glColor4uiv)
#define glnColor4uiv(p0)				GLN_P1(glColor4uiv, p0)
GLN_Declare_P1(glColor4usv)
#define glnColor4usv(p0)				GLN_P1(glColor4usv, p0)
GLN_Declare_P1(glCullFace)
#define glnCullFace(p0)				GLN_P1(glCullFace, p0)
GLN_Declare_P1(glDepthFunc)
#define glnDepthFunc(p0)				GLN_P1(glDepthFunc, p0)
GLN_Declare_P1(glDepthMask)
#define glnDepthMask(p0)				GLN_P1(glDepthMask, p0)
GLN_Declare_P1(glDisable)
#define glnDisable(p0)				GLN_P1(glDisable, p0)
GLN_Declare_P1(glDrawBuffer)
#define glnDrawBuffer(p0)				GLN_P1(glDrawBuffer, p0)
GLN_Declare_P1(glEdgeFlag)
#define glnEdgeFlag(p0)				GLN_P1(glEdgeFlag, p0)
GLN_Declare_P1(glEdgeFlagv)
#define glnEdgeFlagv(p0)				GLN_P1(glEdgeFlagv, p0)
GLN_Declare_P1(glEnable)
#define glnEnable(p0)				GLN_P1(glEnable, p0)
GLN_Declare_P1(glEnableClientState)
#define glnEnableClientState(p0)				GLN_P1(glEnableClientState, p0)
GLN_Declare_P1(glEvalCoord1d)
#define glnEvalCoord1d(p0)				GLN_P1(glEvalCoord1d, p0)
GLN_Declare_P1(glEvalCoord1dv)
#define glnEvalCoord1dv(p0)				GLN_P1(glEvalCoord1dv, p0)
GLN_Declare_P1(glEvalCoord1f)
#define glnEvalCoord1f(p0)				GLN_P1(glEvalCoord1f, p0)
GLN_Declare_P1(glEvalCoord1fv)
#define glnEvalCoord1fv(p0)				GLN_P1(glEvalCoord1fv, p0)
GLN_Declare_P1(glEvalCoord2dv)
#define glnEvalCoord2dv(p0)				GLN_P1(glEvalCoord2dv, p0)
GLN_Declare_P1(glEvalCoord2fv)
#define glnEvalCoord2fv(p0)				GLN_P1(glEvalCoord2fv, p0)
GLN_Declare_P1(glEvalPoint1)
#define glnEvalPoint1(p0)				GLN_P1(glEvalPoint1, p0)
GLN_Declare_P1(glFrontFace)
#define glnFrontFace(p0)				GLN_P1(glFrontFace, p0)
GLN_Declare_P1(glIndexMask)
#define glnIndexMask(p0)				GLN_P1(glIndexMask, p0)
GLN_Declare_P1(glIndexd)
#define glnIndexd(p0)				GLN_P1(glIndexd, p0)
GLN_Declare_P1(glIndexdv)
#define glnIndexdv(p0)				GLN_P1(glIndexdv, p0)
GLN_Declare_P1(glIndexf)
#define glnIndexf(p0)				GLN_P1(glIndexf, p0)
GLN_Declare_P1(glIndexfv)
#define glnIndexfv(p0)				GLN_P1(glIndexfv, p0)
GLN_Declare_P1(glIndexi)
#define glnIndexi(p0)				GLN_P1(glIndexi, p0)
GLN_Declare_P1(glIndexiv)
#define glnIndexiv(p0)				GLN_P1(glIndexiv, p0)
GLN_Declare_P1(glIndexs)
#define glnIndexs(p0)				GLN_P1(glIndexs, p0)
GLN_Declare_P1(glIndexsv)
#define glnIndexsv(p0)				GLN_P1(glIndexsv, p0)
GLN_Declare_P1(glIndexub)
#define glnIndexub(p0)				GLN_P1(glIndexub, p0)
GLN_Declare_P1(glIndexubv)
#define glnIndexubv(p0)				GLN_P1(glIndexubv, p0)
GLN_Declare_P1(glLineWidth)
#define glnLineWidth(p0)				GLN_P1(glLineWidth, p0)
GLN_Declare_P1(glListBase)
#define glnListBase(p0)				GLN_P1(glListBase, p0)
GLN_Declare_P1(glLoadMatrixd)
#define glnLoadMatrixd(p0)				GLN_P1(glLoadMatrixd, p0)
GLN_Declare_P1(glLoadMatrixf)
#define glnLoadMatrixf(p0)				GLN_P1(glLoadMatrixf, p0)
GLN_Declare_P1(glLoadName)
#define glnLoadName(p0)				GLN_P1(glLoadName, p0)
GLN_Declare_P1(glLogicOp)
#define glnLogicOp(p0)				GLN_P1(glLogicOp, p0)
GLN_Declare_P1(glMatrixMode)
#define glnMatrixMode(p0)				GLN_P1(glMatrixMode, p0)
GLN_Declare_P1(glMultMatrixd)
#define glnMultMatrixd(p0)				GLN_P1(glMultMatrixd, p0)
GLN_Declare_P1(glMultMatrixf)
#define glnMultMatrixf(p0)				GLN_P1(glMultMatrixf, p0)
GLN_Declare_P1(glNormal3bv)
#define glnNormal3bv(p0)				GLN_P1(glNormal3bv, p0)
GLN_Declare_P1(glNormal3dv)
#define glnNormal3dv(p0)				GLN_P1(glNormal3dv, p0)
GLN_Declare_P1(glNormal3fv)
#define glnNormal3fv(p0)				GLN_P1(glNormal3fv, p0)
GLN_Declare_P1(glNormal3iv)
#define glnNormal3iv(p0)				GLN_P1(glNormal3iv, p0)
GLN_Declare_P1(glNormal3sv)
#define glnNormal3sv(p0)				GLN_P1(glNormal3sv, p0)
GLN_Declare_P1(glPassThrough)
#define glnPassThrough(p0)				GLN_P1(glPassThrough, p0)
GLN_Declare_P1(glPointSize)
#define glnPointSize(p0)				GLN_P1(glPointSize, p0)
GLN_Declare_P1(glPolygonStipple)
#define glnPolygonStipple(p0)				GLN_P1(glPolygonStipple, p0)
GLN_Declare_P1(glPushAttrib)
#define glnPushAttrib(p0)				GLN_P1(glPushAttrib, p0)
GLN_Declare_P1(glPushClientAttrib)
#define glnPushClientAttrib(p0)				GLN_P1(glPushClientAttrib, p0)
GLN_Declare_P1(glPushName)
#define glnPushName(p0)				GLN_P1(glPushName, p0)
GLN_Declare_P1(glRasterPos2dv)
#define glnRasterPos2dv(p0)				GLN_P1(glRasterPos2dv, p0)
GLN_Declare_P1(glRasterPos2fv)
#define glnRasterPos2fv(p0)				GLN_P1(glRasterPos2fv, p0)
GLN_Declare_P1(glRasterPos2iv)
#define glnRasterPos2iv(p0)				GLN_P1(glRasterPos2iv, p0)
GLN_Declare_P1(glRasterPos2sv)
#define glnRasterPos2sv(p0)				GLN_P1(glRasterPos2sv, p0)
GLN_Declare_P1(glRasterPos3dv)
#define glnRasterPos3dv(p0)				GLN_P1(glRasterPos3dv, p0)
GLN_Declare_P1(glRasterPos3fv)
#define glnRasterPos3fv(p0)				GLN_P1(glRasterPos3fv, p0)
GLN_Declare_P1(glRasterPos3iv)
#define glnRasterPos3iv(p0)				GLN_P1(glRasterPos3iv, p0)
GLN_Declare_P1(glRasterPos3sv)
#define glnRasterPos3sv(p0)				GLN_P1(glRasterPos3sv, p0)
GLN_Declare_P1(glRasterPos4dv)
#define glnRasterPos4dv(p0)				GLN_P1(glRasterPos4dv, p0)
GLN_Declare_P1(glRasterPos4fv)
#define glnRasterPos4fv(p0)				GLN_P1(glRasterPos4fv, p0)
GLN_Declare_P1(glRasterPos4iv)
#define glnRasterPos4iv(p0)				GLN_P1(glRasterPos4iv, p0)
GLN_Declare_P1(glShadeModel)
#define glnShadeModel(p0)				GLN_P1(glShadeModel, p0)
GLN_Declare_P1(glStencilMask)
#define glnStencilMask(p0)				GLN_P1(glStencilMask, p0)
GLN_Declare_P1(glTexCoord1d)
#define glnTexCoord1d(p0)				GLN_P1(glTexCoord1d, p0)
GLN_Declare_P1(glTexCoord1dv)
#define glnTexCoord1dv(p0)				GLN_P1(glTexCoord1dv, p0)
GLN_Declare_P1(glTexCoord1f)
#define glnTexCoord1f(p0)				GLN_P1(glTexCoord1f, p0)
GLN_Declare_P1(glTexCoord1fv)
#define glnTexCoord1fv(p0)				GLN_P1(glTexCoord1fv, p0)
GLN_Declare_P1(glTexCoord1i)
#define glnTexCoord1i(p0)				GLN_P1(glTexCoord1i, p0)
GLN_Declare_P1(glTexCoord1iv)
#define glnTexCoord1iv(p0)				GLN_P1(glTexCoord1iv, p0)
GLN_Declare_P1(glTexCoord1s)
#define glnTexCoord1s(p0)				GLN_P1(glTexCoord1s, p0)
GLN_Declare_P1(glTexCoord1sv)
#define glnTexCoord1sv(p0)				GLN_P1(glTexCoord1sv, p0)
GLN_Declare_P1(glTexCoord2dv)
#define glnTexCoord2dv(p0)				GLN_P1(glTexCoord2dv, p0)
GLN_Declare_P1(glTexCoord2fv)
#define glnTexCoord2fv(p0)				GLN_P1(glTexCoord2fv, p0)
GLN_Declare_P1(glTexCoord2iv)
#define glnTexCoord2iv(p0)				GLN_P1(glTexCoord2iv, p0)
GLN_Declare_P1(glTexCoord2sv)
#define glnTexCoord2sv(p0)				GLN_P1(glTexCoord2sv, p0)
GLN_Declare_P1(glTexCoord3dv)
#define glnTexCoord3dv(p0)				GLN_P1(glTexCoord3dv, p0)
GLN_Declare_P1(glTexCoord3fv)
#define glnTexCoord3fv(p0)				GLN_P1(glTexCoord3fv, p0)
GLN_Declare_P1(glTexCoord3iv)
#define glnTexCoord3iv(p0)				GLN_P1(glTexCoord3iv, p0)
GLN_Declare_P1(glTexCoord3sv)
#define glnTexCoord3sv(p0)				GLN_P1(glTexCoord3sv, p0)
GLN_Declare_P1(glTexCoord4dv)
#define glnTexCoord4dv(p0)				GLN_P1(glTexCoord4dv, p0)
GLN_Declare_P1(glTexCoord4fv)
#define glnTexCoord4fv(p0)				GLN_P1(glTexCoord4fv, p0)
GLN_Declare_P1(glTexCoord4iv)
#define glnTexCoord4iv(p0)				GLN_P1(glTexCoord4iv, p0)
GLN_Declare_P1(glTexCoord4sv)
#define glnTexCoord4sv(p0)				GLN_P1(glTexCoord4sv, p0)
GLN_Declare_P1(glRasterPos4sv)
#define glnRasterPos4sv(p0)				GLN_P1(glRasterPos4sv, p0)
GLN_Declare_P1(glReadBuffer)
#define glnReadBuffer(p0)				GLN_P1(glReadBuffer, p0)
GLN_Declare_P1(glVertex2dv)
#define glnVertex2dv(p0)				GLN_P1(glVertex2dv, p0)
GLN_Declare_P1(glVertex2fv)
#define glnVertex2fv(p0)				GLN_P1(glVertex2fv, p0)
GLN_Declare_P1(glVertex2iv)
#define glnVertex2iv(p0)				GLN_P1(glVertex2iv, p0)
GLN_Declare_P1(glVertex2sv)
#define glnVertex2sv(p0)				GLN_P1(glVertex2sv, p0)
GLN_Declare_P1(glVertex3dv)
#define glnVertex3dv(p0)				GLN_P1(glVertex3dv, p0)
GLN_Declare_P1(glVertex3fv)
#define glnVertex3fv(p0)				GLN_P1(glVertex3fv, p0)
GLN_Declare_P1(glVertex3iv)
#define glnVertex3iv(p0)				GLN_P1(glVertex3iv, p0)
GLN_Declare_P1(glVertex3sv)
#define glnVertex3sv(p0)				GLN_P1(glVertex3sv, p0)
GLN_Declare_P1(glVertex4dv)
#define glnVertex4dv(p0)				GLN_P1(glVertex4dv, p0)
GLN_Declare_P1(glVertex4fv)
#define glnVertex4fv(p0)				GLN_P1(glVertex4fv, p0)
GLN_Declare_P1(glVertex4iv)
#define glnVertex4iv(p0)				GLN_P1(glVertex4iv, p0)
GLN_Declare_P1(glVertex4sv)
#define glnVertex4sv(p0)				GLN_P1(glVertex4sv, p0)
GLN_Declare_P1(glGetPolygonStipple)
#define glnGetPolygonStipple(p0)				GLN_P1(glGetPolygonStipple, p0)
GLN_Declare_P1(glDisableClientState)
#define glnDisableClientState(p0)				GLN_P1(glDisableClientState, p0)


// Procedure 2
GLN_Declare_P2(glAccum)
#define glnAccum(p0, p1)					GLN_P2(glAccum, p0, p1)
GLN_Declare_P2(glAlphaFunc)
#define glnAlphaFunc(p0, p1)					GLN_P2(glAlphaFunc, p0, p1)
GLN_Declare_P2(glBindTexture)
#define glnBindTexture(p0, p1)					GLN_P2(glBindTexture, p0, p1)
GLN_Declare_P2(glBlendFunc)
#define glnBlendFunc(p0, p1)					GLN_P2(glBlendFunc, p0, p1)
GLN_Declare_P2(glClipPlane)
#define glnClipPlane(p0, p1)					GLN_P2(glClipPlane, p0, p1)
GLN_Declare_P2(glColorMaterial)
#define glnColorMaterial(p0, p1)					GLN_P2(glColorMaterial, p0, p1)
GLN_Declare_P2(glDeleteLists)
#define glnDeleteLists(p0, p1)					GLN_P2(glDeleteLists, p0, p1)
GLN_Declare_P2(glDeleteTextures)
#define glnDeleteTextures(p0, p1)					GLN_P2(glDeleteTextures, p0, p1)
GLN_Declare_P2(glDepthRange)
#define glnDepthRange(p0, p1)					GLN_P2(glDepthRange, p0, p1)
GLN_Declare_P2(glEdgeFlagPointer)
#define glnEdgeFlagPointer(p0, p1)					GLN_P2(glEdgeFlagPointer, p0, p1)
GLN_Declare_P2(glEvalCoord2d)
#define glnEvalCoord2d(p0, p1)					GLN_P2(glEvalCoord2d, p0, p1)
GLN_Declare_P2(glEvalCoord2f)
#define glnEvalCoord2f(p0, p1)					GLN_P2(glEvalCoord2f, p0, p1)
GLN_Declare_P2(glEvalPoint2)
#define glnEvalPoint2(p0, p1)					GLN_P2(glEvalPoint2, p0, p1)
GLN_Declare_P2(glFogf)
#define glnFogf(p0, p1)					GLN_P2(glFogf, p0, p1)
GLN_Declare_P2(glFogfv)
#define glnFogfv(p0, p1)					GLN_P2(glFogfv, p0, p1)
GLN_Declare_P2(glFogi)
#define glnFogi(p0, p1)					GLN_P2(glFogi, p0, p1)
GLN_Declare_P2(glFogiv)
#define glnFogiv(p0, p1)					GLN_P2(glFogiv, p0, p1)
GLN_Declare_P2(glGenTextures)
#define glnGenTextures(p0, p1)					GLN_P2(glGenTextures, p0, p1)
GLN_Declare_P2(glGetBooleanv)
#define glnGetBooleanv(p0, p1)					GLN_P2(glGetBooleanv, p0, p1)
GLN_Declare_P2(glGetClipPlane)
#define glnGetClipPlane(p0, p1)					GLN_P2(glGetClipPlane, p0, p1)
GLN_Declare_P2(glGetDoublev)
#define glnGetDoublev(p0, p1)					GLN_P2(glGetDoublev, p0, p1)
GLN_Declare_P2(glGetFloatv)
#define glnGetFloatv(p0, p1)					GLN_P2(glGetFloatv, p0, p1)
GLN_Declare_P2(glGetIntegerv)
#define glnGetIntegerv(p0, p1)					GLN_P2(glGetIntegerv, p0, p1)
GLN_Declare_P2(glGetPixelMapfv)
#define glnGetPixelMapfv(p0, p1)					GLN_P2(glGetPixelMapfv, p0, p1)
GLN_Declare_P2(glGetPixelMapuiv)
#define glnGetPixelMapuiv(p0, p1)					GLN_P2(glGetPixelMapuiv, p0, p1)
GLN_Declare_P2(glGetPixelMapusv)
#define glnGetPixelMapusv(p0, p1)					GLN_P2(glGetPixelMapusv, p0, p1)
GLN_Declare_P2(glHint)
#define glnHint(p0, p1)					GLN_P2(glHint, p0, p1)
GLN_Declare_P2(glLightModelf)
#define glnLightModelf(p0, p1)					GLN_P2(glLightModelf, p0, p1)
GLN_Declare_P2(glLightModelfv)
#define glnLightModelfv(p0, p1)					GLN_P2(glLightModelfv, p0, p1)
GLN_Declare_P2(glLightModeli)
#define glnLightModeli(p0, p1)					GLN_P2(glLightModeli, p0, p1)
GLN_Declare_P2(glLightModeliv)
#define glnLightModeliv(p0, p1)					GLN_P2(glLightModeliv, p0, p1)
GLN_Declare_P2(glLineStipple)
#define glnLineStipple(p0, p1)					GLN_P2(glLineStipple, p0, p1)
GLN_Declare_P2(glNewList)
#define glnNewList(p0, p1)					GLN_P2(glNewList, p0, p1)
GLN_Declare_P2(glPixelStoref)
#define glnPixelStoref(p0, p1)					GLN_P2(glPixelStoref, p0, p1)
GLN_Declare_P2(glPixelStorei)
#define glnPixelStorei(p0, p1)					GLN_P2(glPixelStorei, p0, p1)
GLN_Declare_P2(glPixelTransferf)
#define glnPixelTransferf(p0, p1)					GLN_P2(glPixelTransferf, p0, p1)
GLN_Declare_P2(glPixelTransferi)
#define glnPixelTransferi(p0, p1)					GLN_P2(glPixelTransferi, p0, p1)
GLN_Declare_P2(glPixelZoom)
#define glnPixelZoom(p0, p1)					GLN_P2(glPixelZoom, p0, p1)
GLN_Declare_P2(glPolygonMode)
#define glnPolygonMode(p0, p1)					GLN_P2(glPolygonMode, p0, p1)
GLN_Declare_P2(glPolygonOffset)
#define glnPolygonOffset(p0, p1)					GLN_P2(glPolygonOffset, p0, p1)
GLN_Declare_P2(glRasterPos2d)
#define glnRasterPos2d(p0, p1)					GLN_P2(glRasterPos2d, p0, p1)
GLN_Declare_P2(glRasterPos2f)
#define glnRasterPos2f(p0, p1)					GLN_P2(glRasterPos2f, p0, p1)
GLN_Declare_P2(glRasterPos2i)
#define glnRasterPos2i(p0, p1)					GLN_P2(glRasterPos2i, p0, p1)
GLN_Declare_P2(glRasterPos2s)
#define glnRasterPos2s(p0, p1)					GLN_P2(glRasterPos2s, p0, p1)
GLN_Declare_P2(glRectdv)
#define glnRectdv(p0, p1)					GLN_P2(glRectdv, p0, p1)
GLN_Declare_P2(glRectfv)
#define glnRectfv(p0, p1)					GLN_P2(glRectfv, p0, p1)
GLN_Declare_P2(glRectiv)
#define glnRectiv(p0, p1)					GLN_P2(glRectiv, p0, p1)
GLN_Declare_P2(glRectsv)
#define glnRectsv(p0, p1)					GLN_P2(glRectsv, p0, p1)
GLN_Declare_P2(glTexCoord2d)
#define glnTexCoord2d(p0, p1)					GLN_P2(glTexCoord2d, p0, p1)
GLN_Declare_P2(glTexCoord2f)
#define glnTexCoord2f(p0, p1)					GLN_P2(glTexCoord2f, p0, p1)
GLN_Declare_P2(glTexCoord2i)
#define glnTexCoord2i(p0, p1)					GLN_P2(glTexCoord2i, p0, p1)
GLN_Declare_P2(glTexCoord2s)
#define glnTexCoord2s(p0, p1)					GLN_P2(glTexCoord2s, p0, p1)
GLN_Declare_P2(glVertex2d)
#define glnVertex2d(p0, p1)					GLN_P2(glVertex2d, p0, p1)
GLN_Declare_P2(glVertex2f)
#define glnVertex2f(p0, p1)					GLN_P2(glVertex2f, p0, p1)
GLN_Declare_P2(glVertex2i)
#define glnVertex2i(p0, p1)					GLN_P2(glVertex2i, p0, p1)
GLN_Declare_P2(glVertex2s)
#define glnVertex2s(p0, p1)					GLN_P2(glVertex2s, p0, p1)
GLN_Declare_P2(glSelectBuffer)
#define glnSelectBuffer(p0, p1)					GLN_P2(glSelectBuffer, p0, p1)

// Procedure 3
GLN_Declare_P3(glCallLists)
#define glnCallLists(p0, p1, p2)				GLN_P3(glCallLists, p0, p1, p2)
GLN_Declare_P3(glColor3b)
#define glnColor3b(p0, p1, p2)				GLN_P3(glColor3b, p0, p1, p2)
GLN_Declare_P3(glColor3d)
#define glnColor3d(p0, p1, p2)				GLN_P3(glColor3d, p0, p1, p2)
GLN_Declare_P3(glColor3f)
#define glnColor3f(p0, p1, p2)				GLN_P3(glColor3f, p0, p1, p2)
GLN_Declare_P3(glColor3i)
#define glnColor3i(p0, p1, p2)				GLN_P3(glColor3i, p0, p1, p2)
GLN_Declare_P3(glColor3s)
#define glnColor3s(p0, p1, p2)				GLN_P3(glColor3s, p0, p1, p2)
GLN_Declare_P3(glColor3ub)
#define glnColor3ub(p0, p1, p2)				GLN_P3(glColor3ub, p0, p1, p2)
GLN_Declare_P3(glColor3ui)
#define glnColor3ui(p0, p1, p2)				GLN_P3(glColor3ui, p0, p1, p2)
GLN_Declare_P3(glColor3us)
#define glnColor3us(p0, p1, p2)				GLN_P3(glColor3us, p0, p1, p2)
GLN_Declare_P3(glDrawArrays)
#define glnDrawArrays(p0, p1, p2)				GLN_P3(glDrawArrays, p0, p1, p2)
GLN_Declare_P3(glEvalMesh1)
#define glnEvalMesh1(p0, p1, p2)				GLN_P3(glEvalMesh1, p0, p1, p2)
GLN_Declare_P3(glGetLightfv)
#define glnGetLightfv(p0, p1, p2)				GLN_P3(glGetLightfv, p0, p1, p2)
GLN_Declare_P3(glGetLightiv)
#define glnGetLightiv(p0, p1, p2)				GLN_P3(glGetLightiv, p0, p1, p2)
GLN_Declare_P3(glGetMapdv)
#define glnGetMapdv(p0, p1, p2)				GLN_P3(glGetMapdv, p0, p1, p2)
GLN_Declare_P3(glGetMapfv)
#define glnGetMapfv(p0, p1, p2)				GLN_P3(glGetMapfv, p0, p1, p2)
GLN_Declare_P3(glGetMapiv)
#define glnGetMapiv(p0, p1, p2)				GLN_P3(glGetMapiv, p0, p1, p2)
GLN_Declare_P3(glGetMaterialfv)
#define glnGetMaterialfv(p0, p1, p2)				GLN_P3(glGetMaterialfv, p0, p1, p2)
GLN_Declare_P3(glGetMaterialiv)
#define glnGetMaterialiv(p0, p1, p2)				GLN_P3(glGetMaterialiv, p0, p1, p2)
GLN_Declare_P3(glGetPointerv)
#define glnGetPointerv(p0, p1, p2)				GLN_P3(glGetPointerv, p0, p1, p2)
GLN_Declare_P3(glGetTexEnvfv)
#define glnGetTexEnvfv(p0, p1, p2)				GLN_P3(glGetTexEnvfv, p0, p1, p2)
GLN_Declare_P3(glGetTexEnviv)
#define glnGetTexEnviv(p0, p1, p2)				GLN_P3(glGetTexEnviv, p0, p1, p2)
GLN_Declare_P3(glGetTexGendv)
#define glnGetTexGendv(p0, p1, p2)				GLN_P3(glGetTexGendv, p0, p1, p2)
GLN_Declare_P3(glGetTexGenfv)
#define glnGetTexGenfv(p0, p1, p2)				GLN_P3(glGetTexGenfv, p0, p1, p2)
GLN_Declare_P3(glGetTexGeniv)
#define glnGetTexGeniv(p0, p1, p2)				GLN_P3(glGetTexGeniv, p0, p1, p2)
GLN_Declare_P3(glGetTexParameterfv)
#define glnGetTexParameterfv(p0, p1, p2)				GLN_P3(glGetTexParameterfv, p0, p1, p2)
GLN_Declare_P3(glGetTexParameteriv)
#define glnGetTexParameteriv(p0, p1, p2)				GLN_P3(glGetTexParameteriv, p0, p1, p2)
GLN_Declare_P3(glIndexPointer)
#define glnIndexPointer(p0, p1, p2)				GLN_P3(glIndexPointer, p0, p1, p2)
GLN_Declare_P3(glInterleavedArrays)
#define glnInterleavedArrays(p0, p1, p2)				GLN_P3(glInterleavedArrays, p0, p1, p2)
GLN_Declare_P3(glLightf)
#define glnLightf(p0, p1, p2)				GLN_P3(glLightf, p0, p1, p2)
GLN_Declare_P3(glLightfv)
#define glnLightfv(p0, p1, p2)				GLN_P3(glLightfv, p0, p1, p2)
GLN_Declare_P3(glLighti)
#define glnLighti(p0, p1, p2)				GLN_P3(glLighti, p0, p1, p2)
GLN_Declare_P3(glLightiv)
#define glnLightiv(p0, p1, p2)				GLN_P3(glLightiv, p0, p1, p2)
GLN_Declare_P3(glMapGrid1d)
#define glnMapGrid1d(p0, p1, p2)				GLN_P3(glMapGrid1d, p0, p1, p2)
GLN_Declare_P3(glMapGrid1f)
#define glnMapGrid1f(p0, p1, p2)				GLN_P3(glMapGrid1f, p0, p1, p2)
GLN_Declare_P3(glMaterialf)
#define glnMaterialf(p0, p1, p2)				GLN_P3(glMaterialf, p0, p1, p2)
GLN_Declare_P3(glMaterialfv)
#define glnMaterialfv(p0, p1, p2)				GLN_P3(glMaterialfv, p0, p1, p2)
GLN_Declare_P3(glMateriali)
#define glnMateriali(p0, p1, p2)				GLN_P3(glMateriali, p0, p1, p2)
GLN_Declare_P3(glMaterialiv)
#define glnMaterialiv(p0, p1, p2)				GLN_P3(glMaterialiv, p0, p1, p2)
GLN_Declare_P3(glNormal3b)
#define glnNormal3b(p0, p1, p2)				GLN_P3(glNormal3b, p0, p1, p2)
GLN_Declare_P3(glNormal3d)
#define glnNormal3d(p0, p1, p2)				GLN_P3(glNormal3d, p0, p1, p2)
GLN_Declare_P3(glNormal3f)
#define glnNormal3f(p0, p1, p2)				GLN_P3(glNormal3f, p0, p1, p2)
GLN_Declare_P3(glNormal3i)
#define glnNormal3i(p0, p1, p2)				GLN_P3(glNormal3i, p0, p1, p2)
GLN_Declare_P3(glNormal3s)
#define glnNormal3s(p0, p1, p2)				GLN_P3(glNormal3s, p0, p1, p2)
GLN_Declare_P3(glNormalPointer)
#define glnNormalPointer(p0, p1, p2)				GLN_P3(glNormalPointer, p0, p1, p2)
GLN_Declare_P3(glStencilFunc)
#define glnStencilFunc(p0, p1, p2)				GLN_P3(glStencilFunc, p0, p1, p2)
GLN_Declare_P3(glStencilOp)
#define glnStencilOp(p0, p1, p2)				GLN_P3(glStencilOp, p0, p1, p2)
GLN_Declare_P3(glPixelMapfv)
#define glnPixelMapfv(p0, p1, p2)				GLN_P3(glPixelMapfv, p0, p1, p2)
GLN_Declare_P3(glPixelMapuiv)
#define glnPixelMapuiv(p0, p1, p2)				GLN_P3(glPixelMapuiv, p0, p1, p2)
GLN_Declare_P3(glPixelMapusv)
#define glnPixelMapusv(p0, p1, p2)				GLN_P3(glPixelMapusv, p0, p1, p2)
GLN_Declare_P3(glTexParameterf)
#define glnTexParameterf(p0, p1, p2)				GLN_P3(glTexParameterf, p0, p1, p2)
GLN_Declare_P3(glTexParameterfv)
#define glnTexParameterfv(p0, p1, p2)				GLN_P3(glTexParameterfv, p0, p1, p2)
GLN_Declare_P3(glTexParameteri)
#define glnTexParameteri(p0, p1, p2)				GLN_P3(glTexParameteri, p0, p1, p2)
GLN_Declare_P3(glTexParameteriv)
#define glnTexParameteriv(p0, p1, p2)				GLN_P3(glTexParameteriv, p0, p1, p2)
GLN_Declare_P3(glRasterPos3d)
#define glnRasterPos3d(p0, p1, p2)				GLN_P3(glRasterPos3d, p0, p1, p2)
GLN_Declare_P3(glRasterPos3f)
#define glnRasterPos3f(p0, p1, p2)				GLN_P3(glRasterPos3f, p0, p1, p2)
GLN_Declare_P3(glRasterPos3i)
#define glnRasterPos3i(p0, p1, p2)				GLN_P3(glRasterPos3i, p0, p1, p2)
GLN_Declare_P3(glRasterPos3s)
#define glnRasterPos3s(p0, p1, p2)				GLN_P3(glRasterPos3s, p0, p1, p2)
GLN_Declare_P3(glScaled)
#define glnScaled(p0, p1, p2)				GLN_P3(glScaled, p0, p1, p2)
GLN_Declare_P3(glScalef)
#define glnScalef(p0, p1, p2)				GLN_P3(glScalef, p0, p1, p2)
GLN_Declare_P3(glTexCoord3d)
#define glnTexCoord3d(p0, p1, p2)				GLN_P3(glTexCoord3d, p0, p1, p2)
GLN_Declare_P3(glTexCoord3f)
#define glnTexCoord3f(p0, p1, p2)				GLN_P3(glTexCoord3f, p0, p1, p2)
GLN_Declare_P3(glTexCoord3i)
#define glnTexCoord3i(p0, p1, p2)				GLN_P3(glTexCoord3i, p0, p1, p2)
GLN_Declare_P3(glTexCoord3s)
#define glnTexCoord3s(p0, p1, p2)				GLN_P3(glTexCoord3s, p0, p1, p2)
GLN_Declare_P3(glTexEnvf)
#define glnTexEnvf(p0, p1, p2)				GLN_P3(glTexEnvf, p0, p1, p2)
GLN_Declare_P3(glTexEnvfv)
#define glnTexEnvfv(p0, p1, p2)				GLN_P3(glTexEnvfv, p0, p1, p2)
GLN_Declare_P3(glTexEnvi)
#define glnTexEnvi(p0, p1, p2)				GLN_P3(glTexEnvi, p0, p1, p2)
GLN_Declare_P3(glTexEnviv)
#define glnTexEnviv(p0, p1, p2)				GLN_P3(glTexEnviv, p0, p1, p2)
GLN_Declare_P3(glTexGend)
#define glnTexGend(p0, p1, p2)				GLN_P3(glTexGend, p0, p1, p2)
GLN_Declare_P3(glTexGendv)
#define glnTexGendv(p0, p1, p2)				GLN_P3(glTexGendv, p0, p1, p2)
GLN_Declare_P3(glTexGenf)
#define glnTexGenf(p0, p1, p2)				GLN_P3(glTexGenf, p0, p1, p2)
GLN_Declare_P3(glTexGenfv)
#define glnTexGenfv(p0, p1, p2)				GLN_P3(glTexGenfv, p0, p1, p2)
GLN_Declare_P3(glTexGeni)
#define glnTexGeni(p0, p1, p2)				GLN_P3(glTexGeni, p0, p1, p2)
GLN_Declare_P3(glTexGeniv)
#define glnTexGeniv(p0, p1, p2)				GLN_P3(glTexGeniv, p0, p1, p2)
GLN_Declare_P3(glTranslated)
#define glnTranslated(p0, p1, p2)				GLN_P3(glTranslated, p0, p1, p2)
GLN_Declare_P3(glTranslatef)
#define glnTranslatef(p0, p1, p2)				GLN_P3(glTranslatef, p0, p1, p2)
GLN_Declare_P3(glVertex3d)
#define glnVertex3d(p0, p1, p2)				GLN_P3(glVertex3d, p0, p1, p2)
GLN_Declare_P3(glVertex3f)
#define glnVertex3f(p0, p1, p2)				GLN_P3(glVertex3f, p0, p1, p2)
GLN_Declare_P3(glVertex3i)
#define glnVertex3i(p0, p1, p2)				GLN_P3(glVertex3i, p0, p1, p2)
GLN_Declare_P3(glVertex3s)
#define glnVertex3s(p0, p1, p2)				GLN_P3(glVertex3s, p0, p1, p2)
GLN_Declare_P3(glFeedbackBuffer)
#define glnFeedbackBuffer(p0, p1, p2)				GLN_P3(glFeedbackBuffer, p0, p1, p2)
GLN_Declare_P3(glPrioritizeTextures)
#define glnPrioritizeTextures(p0, p1, p2)				GLN_P3(glPrioritizeTextures, p0, p1, p2)

// Procedure 4
GLN_Declare_P4(glClearAccum)
#define glnClearAccum(p0, p1, p2, p3)				GLN_P4(glClearAccum, p0, p1, p2, p3)
GLN_Declare_P4(glClearColor)
#define glnClearColor(p0, p1, p2, p3)				GLN_P4(glClearColor, p0, p1, p2, p3)
GLN_Declare_P4(glColor4b)
#define glnColor4b(p0, p1, p2, p3)				GLN_P4(glColor4b, p0, p1, p2, p3)
GLN_Declare_P4(glColor4d)
#define glnColor4d(p0, p1, p2, p3)				GLN_P4(glColor4d, p0, p1, p2, p3)
GLN_Declare_P4(glColor4f)
#define glnColor4f(p0, p1, p2, p3)				GLN_P4(glColor4f, p0, p1, p2, p3)
GLN_Declare_P4(glColor4i)
#define glnColor4i(p0, p1, p2, p3)				GLN_P4(glColor4i, p0, p1, p2, p3)
GLN_Declare_P4(glColor4s)
#define glnColor4s(p0, p1, p2, p3)				GLN_P4(glColor4s, p0, p1, p2, p3)
GLN_Declare_P4(glColor4ub)
#define glnColor4ub(p0, p1, p2, p3)				GLN_P4(glColor4ub, p0, p1, p2, p3)
GLN_Declare_P4(glColor4ui)
#define glnColor4ui(p0, p1, p2, p3)				GLN_P4(glColor4ui, p0, p1, p2, p3)
GLN_Declare_P4(glColor4us)
#define glnColor4us(p0, p1, p2, p3)				GLN_P4(glColor4us, p0, p1, p2, p3)
GLN_Declare_P4(glColorMask)
#define glnColorMask(p0, p1, p2, p3)				GLN_P4(glColorMask, p0, p1, p2, p3)
GLN_Declare_P4(glColorPointer)
#define glnColorPointer(p0, p1, p2, p3)				GLN_P4(glColorPointer, p0, p1, p2, p3)
GLN_Declare_P4(glDrawElements)
#define glnDrawElements(p0, p1, p2, p3)				GLN_P4(glDrawElements, p0, p1, p2, p3)
GLN_Declare_P4(glGetTexLevelParameterfv)
#define glnGetTexLevelParameterfv(p0, p1, p2, p3)				GLN_P4(glGetTexLevelParameterfv, p0, p1, p2, p3)
GLN_Declare_P4(glGetTexLevelParameteriv)
#define glnGetTexLevelParameteriv(p0, p1, p2, p3)				GLN_P4(glGetTexLevelParameteriv, p0, p1, p2, p3)
GLN_Declare_P4(glRasterPos4d)
#define glnRasterPos4d(p0, p1, p2, p3)				GLN_P4(glRasterPos4d, p0, p1, p2, p3)
GLN_Declare_P4(glRasterPos4f)
#define glnRasterPos4f(p0, p1, p2, p3)				GLN_P4(glRasterPos4f, p0, p1, p2, p3)
GLN_Declare_P4(glRasterPos4i)
#define glnRasterPos4i(p0, p1, p2, p3)				GLN_P4(glRasterPos4i, p0, p1, p2, p3)
GLN_Declare_P4(glRasterPos4s)
#define glnRasterPos4s(p0, p1, p2, p3)				GLN_P4(glRasterPos4s, p0, p1, p2, p3)
GLN_Declare_P4(glRectd)
#define glnRectd(p0, p1, p2, p3)				GLN_P4(glRectd, p0, p1, p2, p3)
GLN_Declare_P4(glRectf)
#define glnRectf(p0, p1, p2, p3)				GLN_P4(glRectf, p0, p1, p2, p3)
GLN_Declare_P4(glRecti)
#define glnRecti(p0, p1, p2, p3)				GLN_P4(glRecti, p0, p1, p2, p3)
GLN_Declare_P4(glRects)
#define glnRects(p0, p1, p2, p3)				GLN_P4(glRects, p0, p1, p2, p3)
GLN_Declare_P4(glRotated)
#define glnRotated(p0, p1, p2, p3)				GLN_P4(glRotated, p0, p1, p2, p3)
GLN_Declare_P4(glRotatef)
#define glnRotatef(p0, p1, p2, p3)				GLN_P4(glRotatef, p0, p1, p2, p3)
GLN_Declare_P4(glScissor)
#define glnScissor(p0, p1, p2, p3)				GLN_P4(glScissor, p0, p1, p2, p3)
GLN_Declare_P4(glTexCoord4d)
#define glnTexCoord4d(p0, p1, p2, p3)				GLN_P4(glTexCoord4d, p0, p1, p2, p3)
GLN_Declare_P4(glTexCoord4f)
#define glnTexCoord4f(p0, p1, p2, p3)				GLN_P4(glTexCoord4f, p0, p1, p2, p3)
GLN_Declare_P4(glTexCoord4i)
#define glnTexCoord4i(p0, p1, p2, p3)				GLN_P4(glTexCoord4i, p0, p1, p2, p3)
GLN_Declare_P4(glTexCoord4s)
#define glnTexCoord4s(p0, p1, p2, p3)				GLN_P4(glTexCoord4s, p0, p1, p2, p3)
GLN_Declare_P4(glTexCoordPointer)
#define glnTexCoordPointer(p0, p1, p2, p3)				GLN_P4(glTexCoordPointer, p0, p1, p2, p3)
GLN_Declare_P4(glVertex4d)
#define glnVertex4d(p0, p1, p2, p3)				GLN_P4(glVertex4d, p0, p1, p2, p3)
GLN_Declare_P4(glVertex4f)
#define glnVertex4f(p0, p1, p2, p3)				GLN_P4(glVertex4f, p0, p1, p2, p3)
GLN_Declare_P4(glVertex4i)
#define glnVertex4i(p0, p1, p2, p3)				GLN_P4(glVertex4i, p0, p1, p2, p3)
GLN_Declare_P4(glVertex4s)
#define glnVertex4s(p0, p1, p2, p3)				GLN_P4(glVertex4s, p0, p1, p2, p3)
GLN_Declare_P4(glVertexPointer)
#define glnVertexPointer(p0, p1, p2, p3)				GLN_P4(glVertexPointer, p0, p1, p2, p3)
GLN_Declare_P4(glViewport)
#define glnViewport(p0, p1, p2, p3)				GLN_P4(glViewport, p0, p1, p2, p3)

// Procedure 5
GLN_Declare_P5(glCopyPixels)
#define glnCopyPixels(p0, p1, p2, p3, p4)				GLN_P5(glCopyPixels, p0, p1, p2, p3, p4)
GLN_Declare_P5(glDrawPixels)
#define glnDrawPixels(p0, p1, p2, p3, p4)				GLN_P5(glDrawPixels, p0, p1, p2, p3, p4)
GLN_Declare_P5(glEvalMesh2)
#define glnEvalMesh2(p0, p1, p2, p3, p4)				GLN_P5(glEvalMesh2, p0, p1, p2, p3, p4)
GLN_Declare_P5(glGetTexImage)
#define glnGetTexImage(p0, p1, p2, p3, p4)				GLN_P5(glGetTexImage, p0, p1, p2, p3, p4)

// Procedure 6
GLN_Declare_P6(glCopyTexSubImage1D)
#define glnCopyTexSubImage1D(p0, p1, p2, p3, p4, p5)		GLN_P6(glCopyTexSubImage1D, p0, p1, p2, p3, p4, p5)
GLN_Declare_P6(glFrustumf)
#define glnFrustumf(p0, p1, p2, p3, p4, p5)		GLN_P6(glFrustumf, p0, p1, p2, p3, p4, p5)
GLN_Declare_P6(glMap1d)
#define glnMap1d(p0, p1, p2, p3, p4, p5)		GLN_P6(glMap1d, p0, p1, p2, p3, p4, p5)
GLN_Declare_P6(glMap1f)
#define glnMap1f(p0, p1, p2, p3, p4, p5)		GLN_P6(glMap1f, p0, p1, p2, p3, p4, p5)
GLN_Declare_P6(glMapGrid2d)
#define glnMapGrid2d(p0, p1, p2, p3, p4, p5)		GLN_P6(glMapGrid2d, p0, p1, p2, p3, p4, p5)
GLN_Declare_P6(glMapGrid2f)
#define glnMapGrid2f(p0, p1, p2, p3, p4, p5)		GLN_P6(glMapGrid2f, p0, p1, p2, p3, p4, p5)
GLN_Declare_P6(glOrtho)
#define glnOrtho(p0, p1, p2, p3, p4, p5)		GLN_P6(glOrtho, p0, p1, p2, p3, p4, p5)

// Procedure 7
GLN_Declare_P7(glBitmap)
#define glnBitmap(p0, p1, p2, p3, p4, p5, p6)		GLN_P7(glBitmap, p0, p1, p2, p3, p4, p5, p6)
GLN_Declare_P7(glCopyTexImage1D)
#define glnCopyTexImage1D(p0, p1, p2, p3, p4, p5, p6)		GLN_P7(glCopyTexImage1D, p0, p1, p2, p3, p4, p5, p6)
GLN_Declare_P7(glReadPixels)
#define glnReadPixels(p0, p1, p2, p3, p4, p5, p6)		GLN_P7(glReadPixels, p0, p1, p2, p3, p4, p5, p6)
GLN_Declare_P7(glTexSubImage1D)
#define glnTexSubImage1D(p0, p1, p2, p3, p4, p5, p6)		GLN_P7(glTexSubImage1D, p0, p1, p2, p3, p4, p5, p6)


// Procedure 8
GLN_Declare_P8(glCopyTexImage2D)
#define glnCopyTexImage2D(p0, p1, p2, p3, p4, p5, p6, p7)		GLN_P8(glCopyTexImage2D, p0, p1, p2, p3, p4, p5, p6, p7)
GLN_Declare_P8(glCopyTexSubImage2D)
#define glnCopyTexSubImage2D(p0, p1, p2, p3, p4, p5, p6, p7)		GLN_P8(glCopyTexSubImage2D, p0, p1, p2, p3, p4, p5, p6, p7)
GLN_Declare_P8(glTexImage1D)
#define glnTexImage1D(p0, p1, p2, p3, p4, p5, p6, p7)		GLN_P8(glTexImage1D, p0, p1, p2, p3, p4, p5, p6, p7)

// Procedure 9
GLN_Declare_P9(glTexImage2D)
#define glnTexImage2D(p0, p1, p2, p3, p4, p5, p6, p7, p8)		GLN_P9(glTexImage2D, p0, p1, p2, p3, p4, p5, p6, p7, p8)
GLN_Declare_P9(glTexSubImage2D)
#define glnTexSubImage2D(p0, p1, p2, p3, p4, p5, p6, p7, p8)		GLN_P9(glTexSubImage2D, p0, p1, p2, p3, p4, p5, p6, p7, p8)

// Procedure 10
GLN_Declare_P10(glMap2d)
#define glnMap2d(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)		GLN_P10(glMap2d, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)
GLN_Declare_P10(glMap2f)
#define glnMap2f(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)		GLN_P10(glMap2f, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)


// Function 0
GLN_Declare_F0(glGetError, GLenum)
#define glnGetError()			GLN_F0(glGetError)

// Function 1
GLN_Declare_F1(glRenderMode, GLint)
#define glnRenderMode(p0)			GLN_F1(glRenderMode, p0)
GLN_Declare_F1(glIsEnabled, GLboolean)
#define glnIsEnabled(p0)			GLN_F1(glIsEnabled, p0)
GLN_Declare_F1(glIsList, GLboolean)
#define glnIsList(p0)				GLN_F1(glIsList, p0)
GLN_Declare_F1(glIsTexture, GLboolean)
#define glnIsTexture(p0)			GLN_F1(glIsTexture, p0)
GLN_Declare_F1(glGenLists, GLuint)
#define glnGenLists(p0)			GLN_F1(glGenLists, p0)
GLN_Declare_F1(glGetString, const GLubyte *)
#define glnGetString(p0)			GLN_F1(glGetString, p0)
GLN_Declare_F1(wglCreateContext, HGLRC)
#define wglnCreateContext(p0)	GLN_F1(wglCreateContext, p0)
GLN_Declare_F1(wglDeleteContext, BOOL )
#define wglnDeleteContext(p0)	GLN_F1(wglDeleteContext, p0)
GLN_Declare_F1(wglGetCurrentContext, HGLRC)
#define wglnGetCurrentContext(p0)	GLN_F1(wglGetCurrentContext, p0)
GLN_Declare_F1(wglGetCurrentDC, HDC  )
#define wglnGetCurrentDC(p0)	GLN_F1(wglGetCurrentDC, p0)
GLN_Declare_F1(wglGetProcAddress, PROC )
#define wglnGetProcAddress(p0)	GLN_F1(wglGetProcAddress, p0)
GLN_Declare_F1(SwapBuffers, BOOL )
#define nSwapBuffers(p0)	GLN_F1(SwapBuffers, p0)

// Function 2
GLN_Declare_F2(wglCreateLayerContext, HGLRC)
#define wglnCreateLayerContext(p0, p1)	GLN_F2(wglCreateLayerContext, p0, p1)
GLN_Declare_F2(wglMakeCurrent, BOOL )
#define wglnMakeCurrent(p0, p1)	GLN_F2(wglMakeCurrent, p0, p1)
GLN_Declare_F2(wglShareLists, BOOL )
#define wglnShareLists(p0, p1)	GLN_F2(wglShareLists, p0, p1)
GLN_Declare_F2(wglSwapLayerBuffers, BOOL )
#define wglnSwapLayerBuffers(p0, p1)	GLN_F2(wglSwapLayerBuffers, p0, p1)
GLN_Declare_F2(wglSwapMultipleBuffers, DWORD)
#define wglnSwapMultipleBuffers(p0, p1)	GLN_F2(wglSwapMultipleBuffers, p0, p1)

// Function 3
GLN_Declare_F3(glAreTexturesResident, GLboolean)
#define glnAreTexturesResident(p0, p1, p2)			GLN_F3(glAreTexturesResident, p0, p1, p2)
GLN_Declare_F3(wglCopyContext, BOOL )
#define wglnCopyContext(p0, p1, p2)	GLN_F3(wglCopyContext, p0, p1, p2)
GLN_Declare_F3(wglRealizeLayerPalette, BOOL )
#define wglnRealizeLayerPalette(p0, p1, p2)	GLN_F3(wglRealizeLayerPalette, p0, p1, p2)

// Function 4
GLN_Declare_F4(wglUseFontBitmapsA, BOOL )
#define wglnUseFontBitmapsA(p0, p1, p2, p3)	GLN_F4(wglUseFontBitmapsA, p0, p1, p2, p3)
GLN_Declare_F4(wglUseFontBitmapsW, BOOL )
#define wglnUseFontBitmapsW(p0, p1, p2, p3)	GLN_F4(wglUseFontBitmapsW, p0, p1, p2, p3)

// Function 5
GLN_Declare_F5(wglDescribeLayerPlane, BOOL )
#define wglnDescribeLayerPlane(p0, p1, p2, p3, p4)	GLN_F5(wglDescribeLayerPlane, p0, p1, p2, p3, p4)
GLN_Declare_F5(wglSetLayerPaletteEntries, int  )
#define wglnSetLayerPaletteEntries(p0, p1, p2, p3, p4)	GLN_F5(wglSetLayerPaletteEntries, p0, p1, p2, p3, p4)
GLN_Declare_F5(wglGetLayerPaletteEntries, int  )
#define wglnGetLayerPaletteEntries(p0, p1, p2, p3, p4)	GLN_F5(wglGetLayerPaletteEntries, p0, p1, p2, p3, p4)

// Function 8
GLN_Declare_F8(wglUseFontOutlinesA, BOOL )
#define wglnUseFontOutlinesA(p0, p1, p2, p3, p4, p5, p6, p7)	GLN_F8(wglUseFontOutlinesA, p0, p1, p2, p3, p4, p5, p6, p7)
GLN_Declare_F8(wglUseFontOutlinesW, BOOL )
#define wglnUseFontOutlinesW(p0, p1, p2, p3, p4, p5, p6, p7)	GLN_F8(wglUseFontOutlinesW, p0, p1, p2, p3, p4, p5, p6, p7)

#ifdef GL_FUNCTIMING
	CMGLTimer m_GlobalTimer;
	int64 m_GlobalCalls;

	int m_bPendingTrace;
	void Reset();

	class CSortTrace
	{
	public:
		static aint Compare(void *_pContext, CGLFunctionTimer *_pFirst, CGLFunctionTimer *_pSecond)
		{
			aint Comp = _pSecond->m_Timer.Compare(_pFirst->m_Timer);
			if (Comp)
				return Comp;

			if (_pFirst->m_Calls > _pSecond->m_Calls)
				return -1;
			else if (_pFirst->m_Calls < _pSecond->m_Calls)
				return 1;

			return 0;			
		}
	};
	void TraceTimes();

	CGLGlobalTiming():
GLN_ImplementConstructor(glEnd),
GLN_ImplementConstructor(glEndList),
GLN_ImplementConstructor(glFinish),
GLN_ImplementConstructor(glFlush),
GLN_ImplementConstructor(glInitNames),
GLN_ImplementConstructor(glLoadIdentity),
GLN_ImplementConstructor(glPopAttrib),
GLN_ImplementConstructor(glPopClientAttrib),
GLN_ImplementConstructor(glPopMatrix),
GLN_ImplementConstructor(glPopName),
GLN_ImplementConstructor(glPushMatrix),

// Procedure 1
GLN_ImplementConstructor(glArrayElement),
GLN_ImplementConstructor(glBegin),
GLN_ImplementConstructor(glCallList),
GLN_ImplementConstructor(glClear),
GLN_ImplementConstructor(glClearDepth),
GLN_ImplementConstructor(glClearIndex),
GLN_ImplementConstructor(glClearStencil),
GLN_ImplementConstructor(glColor3bv),
GLN_ImplementConstructor(glColor3dv),
GLN_ImplementConstructor(glColor3fv),
GLN_ImplementConstructor(glColor3iv),
GLN_ImplementConstructor(glColor3sv),
GLN_ImplementConstructor(glColor3ubv),
GLN_ImplementConstructor(glColor3uiv),
GLN_ImplementConstructor(glColor3usv),
GLN_ImplementConstructor(glColor4bv),
GLN_ImplementConstructor(glColor4dv),
GLN_ImplementConstructor(glColor4fv),
GLN_ImplementConstructor(glColor4iv),
GLN_ImplementConstructor(glColor4sv),
GLN_ImplementConstructor(glColor4ubv),
GLN_ImplementConstructor(glColor4uiv),
GLN_ImplementConstructor(glColor4usv),
GLN_ImplementConstructor(glCullFace),
GLN_ImplementConstructor(glDepthFunc),
GLN_ImplementConstructor(glDepthMask),
GLN_ImplementConstructor(glDisable),
GLN_ImplementConstructor(glDrawBuffer),
GLN_ImplementConstructor(glEdgeFlag),
GLN_ImplementConstructor(glEdgeFlagv),
GLN_ImplementConstructor(glEnable),
GLN_ImplementConstructor(glEnableClientState),
GLN_ImplementConstructor(glEvalCoord1d),
GLN_ImplementConstructor(glEvalCoord1dv),
GLN_ImplementConstructor(glEvalCoord1f),
GLN_ImplementConstructor(glEvalCoord1fv),
GLN_ImplementConstructor(glEvalCoord2dv),
GLN_ImplementConstructor(glEvalCoord2fv),
GLN_ImplementConstructor(glEvalPoint1),
GLN_ImplementConstructor(glFrontFace),
GLN_ImplementConstructor(glIndexMask),
GLN_ImplementConstructor(glIndexd),
GLN_ImplementConstructor(glIndexdv),
GLN_ImplementConstructor(glIndexf),
GLN_ImplementConstructor(glIndexfv),
GLN_ImplementConstructor(glIndexi),
GLN_ImplementConstructor(glIndexiv),
GLN_ImplementConstructor(glIndexs),
GLN_ImplementConstructor(glIndexsv),
GLN_ImplementConstructor(glIndexub),
GLN_ImplementConstructor(glIndexubv),
GLN_ImplementConstructor(glLineWidth),
GLN_ImplementConstructor(glListBase),
GLN_ImplementConstructor(glLoadMatrixd),
GLN_ImplementConstructor(glLoadMatrixf),
GLN_ImplementConstructor(glLoadName),
GLN_ImplementConstructor(glLogicOp),
GLN_ImplementConstructor(glMatrixMode),
GLN_ImplementConstructor(glMultMatrixd),
GLN_ImplementConstructor(glMultMatrixf),
GLN_ImplementConstructor(glNormal3bv),
GLN_ImplementConstructor(glNormal3dv),
GLN_ImplementConstructor(glNormal3fv),
GLN_ImplementConstructor(glNormal3iv),
GLN_ImplementConstructor(glNormal3sv),
GLN_ImplementConstructor(glPassThrough),
GLN_ImplementConstructor(glPointSize),
GLN_ImplementConstructor(glPolygonStipple),
GLN_ImplementConstructor(glPushAttrib),
GLN_ImplementConstructor(glPushClientAttrib),
GLN_ImplementConstructor(glPushName),
GLN_ImplementConstructor(glRasterPos2dv),
GLN_ImplementConstructor(glRasterPos2fv),
GLN_ImplementConstructor(glRasterPos2iv),
GLN_ImplementConstructor(glRasterPos2sv),
GLN_ImplementConstructor(glRasterPos3dv),
GLN_ImplementConstructor(glRasterPos3fv),
GLN_ImplementConstructor(glRasterPos3iv),
GLN_ImplementConstructor(glRasterPos3sv),
GLN_ImplementConstructor(glRasterPos4dv),
GLN_ImplementConstructor(glRasterPos4fv),
GLN_ImplementConstructor(glRasterPos4iv),
GLN_ImplementConstructor(glShadeModel),
GLN_ImplementConstructor(glStencilMask),
GLN_ImplementConstructor(glTexCoord1d),
GLN_ImplementConstructor(glTexCoord1dv),
GLN_ImplementConstructor(glTexCoord1f),
GLN_ImplementConstructor(glTexCoord1fv),
GLN_ImplementConstructor(glTexCoord1i),
GLN_ImplementConstructor(glTexCoord1iv),
GLN_ImplementConstructor(glTexCoord1s),
GLN_ImplementConstructor(glTexCoord1sv),
GLN_ImplementConstructor(glTexCoord2dv),
GLN_ImplementConstructor(glTexCoord2fv),
GLN_ImplementConstructor(glTexCoord2iv),
GLN_ImplementConstructor(glTexCoord2sv),
GLN_ImplementConstructor(glTexCoord3dv),
GLN_ImplementConstructor(glTexCoord3fv),
GLN_ImplementConstructor(glTexCoord3iv),
GLN_ImplementConstructor(glTexCoord3sv),
GLN_ImplementConstructor(glTexCoord4dv),
GLN_ImplementConstructor(glTexCoord4fv),
GLN_ImplementConstructor(glTexCoord4iv),
GLN_ImplementConstructor(glTexCoord4sv),
GLN_ImplementConstructor(glRasterPos4sv),
GLN_ImplementConstructor(glReadBuffer),
GLN_ImplementConstructor(glVertex2dv),
GLN_ImplementConstructor(glVertex2fv),
GLN_ImplementConstructor(glVertex2iv),
GLN_ImplementConstructor(glVertex2sv),
GLN_ImplementConstructor(glVertex3dv),
GLN_ImplementConstructor(glVertex3fv),
GLN_ImplementConstructor(glVertex3iv),
GLN_ImplementConstructor(glVertex3sv),
GLN_ImplementConstructor(glVertex4dv),
GLN_ImplementConstructor(glVertex4fv),
GLN_ImplementConstructor(glVertex4iv),
GLN_ImplementConstructor(glVertex4sv),
GLN_ImplementConstructor(glGetPolygonStipple),
GLN_ImplementConstructor(glDisableClientState),


// Procedure 2
GLN_ImplementConstructor(glAccum),
GLN_ImplementConstructor(glAlphaFunc),
GLN_ImplementConstructor(glBindTexture),
GLN_ImplementConstructor(glBlendFunc),
GLN_ImplementConstructor(glClipPlane),
GLN_ImplementConstructor(glColorMaterial),
GLN_ImplementConstructor(glDeleteLists),
GLN_ImplementConstructor(glDeleteTextures),
GLN_ImplementConstructor(glDepthRange),
GLN_ImplementConstructor(glEdgeFlagPointer),
GLN_ImplementConstructor(glEvalCoord2d),
GLN_ImplementConstructor(glEvalCoord2f),
GLN_ImplementConstructor(glEvalPoint2),
GLN_ImplementConstructor(glFogf),
GLN_ImplementConstructor(glFogfv),
GLN_ImplementConstructor(glFogi),
GLN_ImplementConstructor(glFogiv),
GLN_ImplementConstructor(glGenTextures),
GLN_ImplementConstructor(glGetBooleanv),
GLN_ImplementConstructor(glGetClipPlane),
GLN_ImplementConstructor(glGetDoublev),
GLN_ImplementConstructor(glGetFloatv),
GLN_ImplementConstructor(glGetIntegerv),
GLN_ImplementConstructor(glGetPixelMapfv),
GLN_ImplementConstructor(glGetPixelMapuiv),
GLN_ImplementConstructor(glGetPixelMapusv),
GLN_ImplementConstructor(glHint),
GLN_ImplementConstructor(glLightModelf),
GLN_ImplementConstructor(glLightModelfv),
GLN_ImplementConstructor(glLightModeli),
GLN_ImplementConstructor(glLightModeliv),
GLN_ImplementConstructor(glLineStipple),
GLN_ImplementConstructor(glNewList),
GLN_ImplementConstructor(glPixelStoref),
GLN_ImplementConstructor(glPixelStorei),
GLN_ImplementConstructor(glPixelTransferf),
GLN_ImplementConstructor(glPixelTransferi),
GLN_ImplementConstructor(glPixelZoom),
GLN_ImplementConstructor(glPolygonMode),
GLN_ImplementConstructor(glPolygonOffset),
GLN_ImplementConstructor(glRasterPos2d),
GLN_ImplementConstructor(glRasterPos2f),
GLN_ImplementConstructor(glRasterPos2i),
GLN_ImplementConstructor(glRasterPos2s),
GLN_ImplementConstructor(glRectdv),
GLN_ImplementConstructor(glRectfv),
GLN_ImplementConstructor(glRectiv),
GLN_ImplementConstructor(glRectsv),
GLN_ImplementConstructor(glTexCoord2d),
GLN_ImplementConstructor(glTexCoord2f),
GLN_ImplementConstructor(glTexCoord2i),
GLN_ImplementConstructor(glTexCoord2s),
GLN_ImplementConstructor(glVertex2d),
GLN_ImplementConstructor(glVertex2f),
GLN_ImplementConstructor(glVertex2i),
GLN_ImplementConstructor(glVertex2s),
GLN_ImplementConstructor(glSelectBuffer),

// Procedure 3
GLN_ImplementConstructor(glCallLists),
GLN_ImplementConstructor(glColor3b),
GLN_ImplementConstructor(glColor3d),
GLN_ImplementConstructor(glColor3f),
GLN_ImplementConstructor(glColor3i),
GLN_ImplementConstructor(glColor3s),
GLN_ImplementConstructor(glColor3ub),
GLN_ImplementConstructor(glColor3ui),
GLN_ImplementConstructor(glColor3us),
GLN_ImplementConstructor(glDrawArrays),
GLN_ImplementConstructor(glEvalMesh1),
GLN_ImplementConstructor(glGetLightfv),
GLN_ImplementConstructor(glGetLightiv),
GLN_ImplementConstructor(glGetMapdv),
GLN_ImplementConstructor(glGetMapfv),
GLN_ImplementConstructor(glGetMapiv),
GLN_ImplementConstructor(glGetMaterialfv),
GLN_ImplementConstructor(glGetMaterialiv),
GLN_ImplementConstructor(glGetPointerv),
GLN_ImplementConstructor(glGetTexEnvfv),
GLN_ImplementConstructor(glGetTexEnviv),
GLN_ImplementConstructor(glGetTexGendv),
GLN_ImplementConstructor(glGetTexGenfv),
GLN_ImplementConstructor(glGetTexGeniv),
GLN_ImplementConstructor(glGetTexParameterfv),
GLN_ImplementConstructor(glGetTexParameteriv),
GLN_ImplementConstructor(glIndexPointer),
GLN_ImplementConstructor(glInterleavedArrays),
GLN_ImplementConstructor(glLightf),
GLN_ImplementConstructor(glLightfv),
GLN_ImplementConstructor(glLighti),
GLN_ImplementConstructor(glLightiv),
GLN_ImplementConstructor(glMapGrid1d),
GLN_ImplementConstructor(glMapGrid1f),
GLN_ImplementConstructor(glMaterialf),
GLN_ImplementConstructor(glMaterialfv),
GLN_ImplementConstructor(glMateriali),
GLN_ImplementConstructor(glMaterialiv),
GLN_ImplementConstructor(glNormal3b),
GLN_ImplementConstructor(glNormal3d),
GLN_ImplementConstructor(glNormal3f),
GLN_ImplementConstructor(glNormal3i),
GLN_ImplementConstructor(glNormal3s),
GLN_ImplementConstructor(glNormalPointer),
GLN_ImplementConstructor(glStencilFunc),
GLN_ImplementConstructor(glStencilOp),
GLN_ImplementConstructor(glPixelMapfv),
GLN_ImplementConstructor(glPixelMapuiv),
GLN_ImplementConstructor(glPixelMapusv),
GLN_ImplementConstructor(glTexParameterf),
GLN_ImplementConstructor(glTexParameterfv),
GLN_ImplementConstructor(glTexParameteri),
GLN_ImplementConstructor(glTexParameteriv),
GLN_ImplementConstructor(glRasterPos3d),
GLN_ImplementConstructor(glRasterPos3f),
GLN_ImplementConstructor(glRasterPos3i),
GLN_ImplementConstructor(glRasterPos3s),
GLN_ImplementConstructor(glScaled),
GLN_ImplementConstructor(glScalef),
GLN_ImplementConstructor(glTexCoord3d),
GLN_ImplementConstructor(glTexCoord3f),
GLN_ImplementConstructor(glTexCoord3i),
GLN_ImplementConstructor(glTexCoord3s),
GLN_ImplementConstructor(glTexEnvf),
GLN_ImplementConstructor(glTexEnvfv),
GLN_ImplementConstructor(glTexEnvi),
GLN_ImplementConstructor(glTexEnviv),
GLN_ImplementConstructor(glTexGend),
GLN_ImplementConstructor(glTexGendv),
GLN_ImplementConstructor(glTexGenf),
GLN_ImplementConstructor(glTexGenfv),
GLN_ImplementConstructor(glTexGeni),
GLN_ImplementConstructor(glTexGeniv),
GLN_ImplementConstructor(glTranslated),
GLN_ImplementConstructor(glTranslatef),
GLN_ImplementConstructor(glVertex3d),
GLN_ImplementConstructor(glVertex3f),
GLN_ImplementConstructor(glVertex3i),
GLN_ImplementConstructor(glVertex3s),
GLN_ImplementConstructor(glFeedbackBuffer),
GLN_ImplementConstructor(glPrioritizeTextures),

// Procedure 4
GLN_ImplementConstructor(glClearAccum),
GLN_ImplementConstructor(glClearColor),
GLN_ImplementConstructor(glColor4b),
GLN_ImplementConstructor(glColor4d),
GLN_ImplementConstructor(glColor4f),
GLN_ImplementConstructor(glColor4i),
GLN_ImplementConstructor(glColor4s),
GLN_ImplementConstructor(glColor4ub),
GLN_ImplementConstructor(glColor4ui),
GLN_ImplementConstructor(glColor4us),
GLN_ImplementConstructor(glColorMask),
GLN_ImplementConstructor(glColorPointer),
GLN_ImplementConstructor(glDrawElements),
GLN_ImplementConstructor(glGetTexLevelParameterfv),
GLN_ImplementConstructor(glGetTexLevelParameteriv),
GLN_ImplementConstructor(glRasterPos4d),
GLN_ImplementConstructor(glRasterPos4f),
GLN_ImplementConstructor(glRasterPos4i),
GLN_ImplementConstructor(glRasterPos4s),
GLN_ImplementConstructor(glRectd),
GLN_ImplementConstructor(glRectf),
GLN_ImplementConstructor(glRecti),
GLN_ImplementConstructor(glRects),
GLN_ImplementConstructor(glRotated),
GLN_ImplementConstructor(glRotatef),
GLN_ImplementConstructor(glScissor),
GLN_ImplementConstructor(glTexCoord4d),
GLN_ImplementConstructor(glTexCoord4f),
GLN_ImplementConstructor(glTexCoord4i),
GLN_ImplementConstructor(glTexCoord4s),
GLN_ImplementConstructor(glTexCoordPointer),
GLN_ImplementConstructor(glVertex4d),
GLN_ImplementConstructor(glVertex4f),
GLN_ImplementConstructor(glVertex4i),
GLN_ImplementConstructor(glVertex4s),
GLN_ImplementConstructor(glVertexPointer),
GLN_ImplementConstructor(glViewport),

// Procedure 5
GLN_ImplementConstructor(glCopyPixels),
GLN_ImplementConstructor(glDrawPixels),
GLN_ImplementConstructor(glEvalMesh2),
GLN_ImplementConstructor(glGetTexImage),

// Procedure 6
GLN_ImplementConstructor(glCopyTexSubImage1D),
GLN_ImplementConstructor(glFrustumf),
GLN_ImplementConstructor(glMap1d),
GLN_ImplementConstructor(glMap1f),
GLN_ImplementConstructor(glMapGrid2d),
GLN_ImplementConstructor(glMapGrid2f),
GLN_ImplementConstructor(glOrtho),

// Procedure 7
GLN_ImplementConstructor(glBitmap),
GLN_ImplementConstructor(glCopyTexImage1D),
GLN_ImplementConstructor(glReadPixels),
GLN_ImplementConstructor(glTexSubImage1D),


// Procedure 8
GLN_ImplementConstructor(glCopyTexImage2D),
GLN_ImplementConstructor(glCopyTexSubImage2D),
GLN_ImplementConstructor(glTexImage1D),

// Procedure 9
GLN_ImplementConstructor(glTexImage2D),
GLN_ImplementConstructor(glTexSubImage2D),

// Procedure 10
GLN_ImplementConstructor(glMap2d),
GLN_ImplementConstructor(glMap2f),


// Function 0
GLN_ImplementConstructor(glGetError),

// Function 1
GLN_ImplementConstructor(glRenderMode),
GLN_ImplementConstructor(glIsEnabled),
GLN_ImplementConstructor(glIsList),
GLN_ImplementConstructor(glIsTexture),
GLN_ImplementConstructor(glGenLists),
GLN_ImplementConstructor(glGetString),
GLN_ImplementConstructor(wglCreateContext),
GLN_ImplementConstructor(wglDeleteContext),
GLN_ImplementConstructor(wglGetCurrentContext),
GLN_ImplementConstructor(wglGetCurrentDC),
GLN_ImplementConstructor(wglGetProcAddress),
GLN_ImplementConstructor(SwapBuffers),

// Function 2
GLN_ImplementConstructor(wglCreateLayerContext),
GLN_ImplementConstructor(wglMakeCurrent),
GLN_ImplementConstructor(wglShareLists),
GLN_ImplementConstructor(wglSwapLayerBuffers),
GLN_ImplementConstructor(wglSwapMultipleBuffers),

// Function 3
GLN_ImplementConstructor(glAreTexturesResident),
GLN_ImplementConstructor(wglCopyContext),
GLN_ImplementConstructor(wglRealizeLayerPalette),

// Function 4
GLN_ImplementConstructor(wglUseFontBitmapsA),
GLN_ImplementConstructor(wglUseFontBitmapsW),

// Function 5
GLN_ImplementConstructor(wglDescribeLayerPlane),
GLN_ImplementConstructor(wglSetLayerPaletteEntries),
GLN_ImplementConstructor(wglGetLayerPaletteEntries),

// Function 8
GLN_ImplementConstructor(wglUseFontOutlinesA),
GLN_ImplementConstructor(wglUseFontOutlinesW)
	{
	}

};
extern CGLGlobalTiming g_GlobalTiming;

#endif
#endif // DInc_MGLGlobalFunctions_h
