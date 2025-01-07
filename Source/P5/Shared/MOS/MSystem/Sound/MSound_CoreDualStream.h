
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030606:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef _INC_MSOUND_CORE_DUAL_STREAM
#define _INC_MSOUND_CORE_DUAL_STREAM

#include "MSound.h"

// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CSC_DualStream : public CConsoleClient
{

private:
	MRTC_CriticalSection m_DualStreamLock;
	CSoundContext *m_pSC;

	class CDualStreamThread : public MRTC_Thread
	{
	public:
		CSC_DualStream *m_pSCC;
		void Create (CSC_DualStream *_pSCC)
		{
			m_pSCC = _pSCC;
		}
		const char* Thread_GetName() const
		{
			return "MSound Dual stream";
		}

		int Thread_Main()
		{
			while(!Thread_IsTerminating())
			{
//				M_TRY
//				{
					m_pSCC->RefreshDualStream();
/*				}
				M_CATCH(
				catch (CCExceptionFile)
				{
					CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
				}
				)
#ifdef M_SUPPORTSTATUSCORRUPT
					M_CATCH(
				catch (CCException)
				{
					CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
				}
				)
#endif*/
				Thread_Sleep(0.050f);
			}
			return 0;
		}
	};

	CDualStreamThread m_DualStreamThread;

	// Dual steam stuff
	int m_DualStreamChannels;
	
	fp32 m_DualStreamMasterVolume;

	class CStream
	{
	public:
		CStream()
		{
			m_OldStream = -1;
			m_Stream = -1;
			m_OldVolume = 0;
			m_OldVolumePrim = 0;
			m_OldVolumeTarget = 0;
			m_Volume = 0;
			m_VolumePrim = 0;
			m_VolumeTarget = 0;
			m_OldNewBlend = 1;
			m_OldNewBlendPrim = 0;
			m_OldNewBlendTarget = 0;
		}

		CStr m_LastStream;

		int m_OldStream;
		fp32 m_OldVolume;
		fp32 m_OldVolumePrim;
		fp32 m_OldVolumeTarget;

		int m_Stream;
		fp32 m_Volume;
		fp32 m_VolumePrim;
		fp32 m_VolumeTarget;

		fp32 m_OldNewBlend;
		fp32 m_OldNewBlendPrim;
		fp32 m_OldNewBlendTarget;
	};

	TArray<CStream> m_lStreams;
	TArray<fp32> m_lFadeSpeeds;

	int m_bPausedUpdate;
	int m_bAddedToConsole;

	public:

	DECLARE_OPERATOR_NEW

	CSC_DualStream();
	~CSC_DualStream();

	void KillThreads();

	void Refresh();
	void RefreshDualStream();

	void Create(CSoundContext *_pSC);

	void MultiStream_Play(const char * *_Files, int _nFiles, bool _FadeIn, bool _bResetVolumes, uint32 _LoopMask = 0xffffffff);
	void MultiStream_Stop(bool _FadeOut);
	void MultiStream_Volumes(fp32 *_Volumes, int _nVolumes, bool _Fade, fp32 *_pFadeSpeeds);
	void MultiStream_Volume(fp32 _Volume);
	void MultiStream_Pause();
	void MultiStream_Resume();

	// Console stuff

	void Con_Stream_Play(CStr _File1, CStr _File2, int _Fade);
	void Con_Stream_Stop(int _Fade);
	void Con_Stream_Relation(fp32 _Relation, int _Fade);
	void Con_Stream_Volume(fp32 _Volume);

	void Register(CScriptRegisterContext &_RegContext);

};

#endif // _INC_MSOUND_CORE_DUAL_STREAM
