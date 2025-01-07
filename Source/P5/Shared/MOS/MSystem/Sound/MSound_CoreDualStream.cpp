#include "PCH.h"

#include "../MSystem.h"
#include "MSound_CoreDualStream.h"

// -------------------------------------------------------------------
//  CSC_DualStream
// -------------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(CSC_DualStream);


#ifdef PLATFORM_WIN_PC
#include <windows.h>
class COSInfoClass
{
public:
	int m_bHasInit;
	OSVERSIONINFO m_OSInfo;
	M_INLINE void Init()
	{
		if (!m_bHasInit)
		{
			m_OSInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetVersionEx(&m_OSInfo);
			m_bHasInit = true;
		}

	}

	M_INLINE bool IsNT()
	{
		Init();
		return (m_OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
	}
};
static COSInfoClass g_OsInfo = {0};
#elif defined(PLATFORM_XBOX)

class COSInfoClass
{
public:
	M_INLINE void Init(){}
	M_INLINE bool IsNT(){return true;}
};

static COSInfoClass g_OsInfo;

#endif
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Short_desscription
						
	Comments:			Longer_description_not_mandatory
\*____________________________________________________________________*/
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Short_desscription
\*____________________________________________________________________*/
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	TemplateClass:		Short_desscription
						
	Parameters:			
		_param1:		description
						
	Comments:			Longer_description_not_mandatory
\*____________________________________________________________________*/

CSC_DualStream::CSC_DualStream()
{
	MSCOPESHORT(CSC_DualStream_ctor);

	m_DualStreamThread.Create(this);

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) Error("-", "No System object.");

	m_DualStreamMasterVolume = pSys->GetEnvironment()->GetValuef("SND_DUALSTREAMVOLUME", 1.00f);

	m_bPausedUpdate = true;
	m_bAddedToConsole = false;
	m_pSC = NULL;

}

CSC_DualStream::~CSC_DualStream()
{
	if (m_bAddedToConsole)
		RemoveFromConsole();
}

void CSC_DualStream::KillThreads()
{
	m_DualStreamThread.Thread_Destroy();
}

void CSC_DualStream::Create(CSoundContext *_pSC)
{
	m_pSC = _pSC;

	m_DualStreamChannels = m_pSC->Chn_Alloc(12);

#ifdef SC_THREADS
	if (!m_DualStreamThread.Thread_IsCreated())
	{
		m_DualStreamThread.Thread_Create(NULL, 8192, MRTC_THREAD_PRIO_HIGHEST);
	}
#endif

	AddToConsole();
	m_bAddedToConsole = true;

}

// -------------------------------------------------------------------
void CSC_DualStream::Refresh()
{
	if (!m_pSC)
		return;
	{
		M_LOCK(m_DualStreamLock);

		if (m_lStreams.Len() && !m_bPausedUpdate)
		{
	//		bool bHasStreams = false;
			for (int i = 0; i < m_lStreams.Len(); ++i)
			{
				if (m_lStreams[i].m_OldNewBlend > 0.9999f)
				{
					if (m_lStreams[i].m_OldStream >= 0)
					{
						m_pSC->Voice_Destroy(m_lStreams[i].m_OldStream);
						m_lStreams[i].m_OldStream = -1;
					}
				}
	//			else if (m_lStreams[i].m_OldStream >= 0)
	//				bHasStreams = true;

	//			if(m_lStreams[i].m_Stream >= 0)
	//				bHasStreams = true;
			}

	//		if( !bHasStreams )
	//			m_lStreams.Destroy();
		}

		if (m_bPausedUpdate)
		{
			bint AllCanUnpause = true;
			for (int i = 0; i < m_lStreams.Len(); ++i)
			{
				CSC_DualStream::CStream &Stream = m_lStreams[i];
				if (Stream.m_Stream >= 0)
				{
					if (!m_pSC->Voice_ReadyForPlay(Stream.m_Stream))
						AllCanUnpause = false;

				}
			}
			if (AllCanUnpause)
			{
				m_bPausedUpdate = false;
				for (int i = 0; i < m_lStreams.Len(); ++i)
				{
					CSC_DualStream::CStream &Stream = m_lStreams[i];
					if (Stream.m_Stream >= 0)
						m_pSC->Voice_Unpause(Stream.m_Stream);
				}
			}
		}
	}

}

void CSC_DualStream::RefreshDualStream()
{
	if (!m_pSC)
		return;
	M_LOCK(m_pSC->GetInterfaceLock());
	{
		M_LOCK(m_DualStreamLock);

		if (!m_bPausedUpdate)
		{
			// when going from game to main menu, m_lStreams.Len() can be larger than m_lFadeSpeeds.Len() during the fade out duration
			int FadeSpeedLen = m_lFadeSpeeds.Len();
			for (int i = 0; i < m_lStreams.Len(); ++i)
			{
				CSC_DualStream::CStream &Stream = m_lStreams[i];
				fp32 ChangeSpeed;
				
				if(FadeSpeedLen < (i*2+1))
					ChangeSpeed = 0.25f;
				else
				{
					if(Stream.m_Volume <= Stream.m_VolumeTarget)
						ChangeSpeed = m_lFadeSpeeds[i*2]; //fade in
					else
						ChangeSpeed = m_lFadeSpeeds[i*2+1];
				}
				
				if (Stream.m_OldStream >= 0)
				{
					fp32 OldVolume = Stream.m_OldVolume * (1.0f - Stream.m_OldNewBlend) * m_DualStreamMasterVolume;
	//				M_TRACEALWAYS("OldVolume %f\n", OldVolume);
					m_pSC->Voice_SetVolume(Stream.m_OldStream, OldVolume);
					ModerateFloat(Stream.m_OldVolume, Stream.m_OldVolumeTarget, Stream.m_OldVolumePrim, ChangeSpeed);
				}
				if (Stream.m_Stream >= 0)
				{
					m_pSC->Voice_SetVolume(Stream.m_Stream, Stream.m_Volume * Stream.m_OldNewBlend * m_DualStreamMasterVolume);
					ModerateFloat(Stream.m_Volume, Stream.m_VolumeTarget, Stream.m_VolumePrim, ChangeSpeed);
				}
				ModerateFloat(Stream.m_OldNewBlend, Stream.m_OldNewBlendTarget, Stream.m_OldNewBlendPrim, ChangeSpeed);
			}

		}
	}
}

void CSC_DualStream::MultiStream_Play(const char * *_Files, int _nFiles, bool _FadeIn, bool _bResetVolumes, uint32 _LoopMask)
{
	if (!m_pSC)
		return;
	M_LOCK(m_pSC->GetInterfaceLock());
	{
		M_LOCK(m_DualStreamLock);

		// Don't have enough bitmasks for more streams atm
		M_ASSERT(_nFiles <= 32,"TOO MANY STREAMS");
		int iHighestRunning = 0;
		bool bChanged = false;
		for (int i = 0; i < m_lStreams.Len(); ++i)
		{
			CSC_DualStream::CStream &Stream = m_lStreams[i];
			if (Stream.m_Stream >= 0 || Stream.m_OldStream >= 0)
				iHighestRunning = i+1;

			if (i < _nFiles && Stream.m_LastStream.CompareNoCase(_Files[i]))
				bChanged = true;
		}

		if (!bChanged && m_lStreams.Len() == _nFiles)
			return;
		
		m_lStreams.SetLen(Max(iHighestRunning, _nFiles));
		
		for (int i = 0; i < m_lStreams.Len(); ++i)
		{
			CSC_DualStream::CStream &Stream = m_lStreams[i];

			if (i >= _nFiles || Stream.m_LastStream.CompareNoCase(_Files[i]))
			{
				if (Stream.m_OldStream >= 0)
					m_pSC->Voice_Destroy(Stream.m_OldStream);

				Stream.m_OldStream = Stream.m_Stream;
				Stream.m_OldVolume = Stream.m_Volume;
				Stream.m_OldVolumePrim = Stream.m_VolumePrim;
				Stream.m_OldVolumeTarget = Stream.m_VolumeTarget;
				if (_bResetVolumes)
				{
					Stream.m_VolumeTarget = 1.0;
					Stream.m_Volume = 0.0;
					Stream.m_VolumePrim = 0.0;
				}

				if ( i < _nFiles)
					Stream.m_LastStream = _Files[i];
				else
					Stream.m_LastStream.Clear();

				if (Stream.m_LastStream.Len())
				{
					CSC_SFXDesc *pDesc = m_pSC->GetSFXDesc(Stream.m_LastStream);
				
					if (pDesc)
					{
						CSC_VoiceCreateParams CreateParams;
						m_pSC->Voice_InitCreateParams(CreateParams, pDesc->GetCategory());
						CreateParams.m_Volume = 0.0f;
						CreateParams.m_hChannel = m_DualStreamChannels;
						uint32 Prio = pDesc->GetPriority();
						if (Prio)
							CreateParams.m_Priority = Prio;
						CreateParams.m_WaveID = pDesc->GetWaveId(0);
						CreateParams.m_bLoop = (_LoopMask & M_BitD(i)) != 0;
						CreateParams.m_Pitch *= pDesc->GetPitch() + Random*pDesc->GetPitchRandAmp();
						CreateParams.m_bStartPaused = true;

						Stream.m_Stream = m_pSC->Voice_Create(CreateParams);
						if (Stream.m_Stream < 0)
							ConOut(CStrF("Failed to play multi stream %s", _Files[i]));						
					}
					else
					{
						ConOut(CStrF("Failed to play multi stream %s", _Files[i]));						
						Stream.m_Stream = -1;
					}
				}
				else
					Stream.m_Stream = -1;

				if (_FadeIn)
				{
					Stream.m_OldNewBlend = 0;
					Stream.m_OldNewBlendTarget = 1;
				}
				else
				{
					Stream.m_OldNewBlend = 1;
					Stream.m_OldNewBlendTarget = 1;
				}
			}
		}

		m_bPausedUpdate = true;
	}

}

void CSC_DualStream::MultiStream_Pause()
{
	if (!m_pSC)
		return;
	M_LOCK(m_pSC->GetInterfaceLock());
	{
		M_LOCK(m_DualStreamLock);

		for (int i = 0; i < m_lStreams.Len(); ++i)
		{
			CSC_DualStream::CStream &Stream = m_lStreams[i];
			if (Stream.m_OldStream >= 0)
				m_pSC->Voice_Destroy(Stream.m_OldStream);
			if (Stream.m_Stream >= 0)
				m_pSC->Voice_Destroy(Stream.m_Stream);
		}
	}
}



void CSC_DualStream::MultiStream_Resume()
{
	if (!m_pSC)
		return;
	M_LOCK(m_DualStreamLock);

	TArray<CStr> lTemp;
	TArray<const char *> lTemp2;
	lTemp.SetLen(m_lStreams.Len());
	lTemp2.SetLen(m_lStreams.Len());
	for (int i = 0; i < m_lStreams.Len(); ++i)
	{
		CSC_DualStream::CStream &Stream = m_lStreams[i];
		lTemp[i] = Stream.m_LastStream;
		Stream.m_LastStream.Clear();
		lTemp2[i] = lTemp[i].Str();
	}

	MultiStream_Play(lTemp2.GetBasePtr(),lTemp2.Len(), 0, false);

}

void CSC_DualStream::MultiStream_Stop(bool _FadeOut)
{
	if (!m_pSC)
		return;
	M_LOCK(m_pSC->GetInterfaceLock());
	{
		M_LOCK(m_DualStreamLock);

		for (int i = 0; i < m_lStreams.Len(); ++i)
		{
			CSC_DualStream::CStream &Stream = m_lStreams[i];
			Stream.m_LastStream.Clear();

			// Destroy old streams
			if (Stream.m_OldStream >= 0)
				m_pSC->Voice_Destroy(Stream.m_OldStream);

			Stream.m_OldStream = Stream.m_Stream;
			Stream.m_Stream = -1;
			Stream.m_OldVolume = Stream.m_Volume;
			Stream.m_OldVolumePrim = Stream.m_VolumePrim;
			Stream.m_OldVolumeTarget = Stream.m_VolumeTarget;
			if (_FadeOut)
			{
				Stream.m_OldNewBlend = 0;
				Stream.m_OldNewBlendTarget = 1;
			}
			else
			{
				Stream.m_OldNewBlend = 1;
				Stream.m_OldNewBlendTarget = 1;
			}
		}
	}

}

void CSC_DualStream::MultiStream_Volumes(fp32 *_Volumes, int _nVolumes, bool _Fade, fp32 *_pFadeSpeeds)
{
	if (!m_pSC)
		return;
	M_LOCK(m_DualStreamLock);
	
	int nVolumes = Min(_nVolumes, m_lStreams.Len());
	m_lFadeSpeeds.SetLen(_nVolumes * 2); // one for up and one for down.

	int j = 0;
	for (int i = 0; i < nVolumes; ++i)
	{
		CSC_DualStream::CStream &Stream = m_lStreams[i];

		Stream.m_VolumeTarget = _Volumes[i];
		if (!_Fade)
			Stream.m_Volume = _Volumes[i];
		
		if(_pFadeSpeeds && _pFadeSpeeds[j] && _pFadeSpeeds[j+1])
		{
			m_lFadeSpeeds[j] = _pFadeSpeeds[j];
			m_lFadeSpeeds[j+1] = _pFadeSpeeds[j+1];	
		}
		else
		{
			m_lFadeSpeeds[j] = 0.25f;
			m_lFadeSpeeds[j+1] = 0.25f;
		}	
		j++;
		j++;
	}
}

void CSC_DualStream::MultiStream_Volume(fp32 _Volume)
{
	if (!m_pSC)
		return;
	M_LOCK(m_DualStreamLock);
	
	m_DualStreamMasterVolume = _Volume;
}


void CSC_DualStream::Con_Stream_Play(CStr _File1, CStr _File2, int _Fade)
{
	const char *lStreams[2];
	lStreams[0] = _File1.Str();
	lStreams[1] = _File2.Str();
	fp32 Volumes[2];
	Volumes[0] = 1.0f;
	Volumes[1] = 0.0f;
	MultiStream_Play(lStreams, 2, _Fade != 0, false);
	MultiStream_Volumes(Volumes, 2, _Fade != 0, NULL);
}

void CSC_DualStream::Con_Stream_Stop(int _Fade)
{
	MultiStream_Stop(_Fade != 0);
}

void CSC_DualStream::Con_Stream_Relation(fp32 _Relation, int _Fade)
{
	fp32 Volumes[2];
	Volumes[0] = 1.0f - _Relation;
	Volumes[1] = _Relation;
	MultiStream_Volumes(Volumes, 2, _Fade != 0, NULL);
}

void CSC_DualStream::Con_Stream_Volume(fp32 _Volume)
{
	MultiStream_Volume(_Volume);
}

void CSC_DualStream::Register(CScriptRegisterContext & _RegContext)
{	

	_RegContext.RegFunction("stream_play", this, &CSC_DualStream::Con_Stream_Play);
	_RegContext.RegFunction("stream_relation", this, &CSC_DualStream::Con_Stream_Relation);
	_RegContext.RegFunction("stream_stop", this, &CSC_DualStream::Con_Stream_Stop);
	_RegContext.RegFunction("stream_volume", this, &CSC_DualStream::Con_Stream_Volume);
}

