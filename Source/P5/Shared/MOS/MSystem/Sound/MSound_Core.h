
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030606:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef _INC_MSOUND_CORE
#define _INC_MSOUND_CORE

#include "MSound.h"
#include "MSound_Codec.h"
#include "MSound_CoreDualStream.h"

class CSC_LipSyncManager
{
public:

	CSC_LipSyncManager()
	{
		m_bAllowLSDRead = false;
		LSD_Clear();
	}

	//
	class CLipSyncHolder
	{
	public:
//		int16 m_WaveID;
		int16 m_Index:15;
		int16 m_bLoaded:1;
	};

	TThinArray<CLipSyncHolder> m_lLipSyncLUT;
	TThinArray<uint32> m_lLipSyncStartLUT;
	TThinArray<fp32> m_lLipSyncTimeCodes;
	TThinArray<uint8> m_lData;
	int32 m_UsedLSDSize;
	bool m_bAllowLSDRead;

	void LSD_Add(int16 _WaveID, CWaveContext *_pWaveContext);
	void LSD_Clear();
	void LSD_Load(uint16 *_pPreCacheOrder, int _nIds, CWaveContext *_pWaveContext);

	// Interface
	void *LSD_Get(int16 _WaveID);
	fp32 Timecode_Get(int16 _WaveID);
	void Wave_PrecacheFlush();
	void Wave_Precache(int _WaveID, CWaveContext *_pWaveContext);
	void Wave_PrecacheBegin();
	void Wave_PrecacheEnd(uint16 *_pPreCacheOrder, int _nIds, CWaveContext *_pWaveContext);

};


#endif // _INC_MSOUND_CORE
