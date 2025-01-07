#ifndef __INC_MThreadManager
#define __INC_MThreadManager

class CMThread : public MRTC_Thread_Core
{
public:
	CDA_Link m_ThreadList;
	void Thread_Sleep();

};

#ifdef PLATFORM_DOLPHIN

class CMThreadManager
{
public:


};

#else
class CMThreadManager
{
public:

};

#endif

#endif // __INC_MThreadManager
