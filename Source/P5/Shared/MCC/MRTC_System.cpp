
#ifdef	PLATFORM_PS2
#define CONSTANT_FRAME_RATE
#endif

#if defined(PLATFORM_WIN_PC)
	#include "MRTC_System_Win32.cpp"

#elif defined( PLATFORM_PS2 )
	#include "MRTC_System_PS2.cpp"

#elif defined(PLATFORM_PS3)
	#include "MRTC_System_PS3.cpp"

#elif defined( PLATFORM_DOLPHIN )
	#include "MRTC_System_Dolphin.cpp"

#elif defined(PLATFORM_XBOX)
	//
#else
	#error "ERROR!! You need to write an MRTC_System_.cpp file for this configuration"

#endif

const char* MRTC_SystemInfo::CPU_GetName(bool _bIncludeFeatures)
{
	if (_bIncludeFeatures)
		return m_CPUNameWithFeatures;
	else
		return m_CPUName;
}

void MRTC_SystemInfo::CPU_DisableFeatures(int _Flags)
{
	m_CPUFeaturesEnabled &= ~_Flags;
	CPU_CreateNames();
}

void MRTC_SystemInfo::CPU_EnableFeatures(int _Flags)
{
	m_CPUFeaturesEnabled |= _Flags;
	CPU_CreateNames();
}


#ifdef M_STATIC
MRTC_SystemInfo *MRTC_ObjectManager::m_pSystemInfo = NULL;
uint64 MRTC_ObjectManager::m_SystemInfoData[(sizeof(MRTC_SystemInfo) + 7) / 8];
#endif

MRTC_SystemInfo& MRTC_SystemInfo::MRTC_GetSystemInfo()
{
#ifdef M_STATIC
	if (MRTC_ObjectManager::m_pSystemInfo)
		return *MRTC_ObjectManager::m_pSystemInfo;
		
	MRTC_ObjectManager::m_pSystemInfo = new(MRTC_ObjectManager::m_SystemInfoData) MRTC_SystemInfo;
	return *MRTC_ObjectManager::m_pSystemInfo;
#else
	return MRTC_GetObjectManager()->m_SystemInfo;
#endif
}

void MRTC_SystemInfo::OS_HeapPush(int _MemoryType)
{
	M_ASSERT(0, "Not implemented.");
}

void MRTC_SystemInfo::OS_HeapPop()
{
	M_ASSERT(0, "Not implemented.");
}

