

#include "PCH.h"

#ifdef PLATFORM_WIN_PC

#include "MSound_ASIO.h"


MRTC_IMPLEMENT_DYNAMIC(CSoundContext_ASIO, CSoundContext_Vorbis);

CSoundContext_ASIO::CSoundContext_ASIO()
{
	m_bInit = false;
	m_DebugStrTempDSound2 = NULL;

	MACRO_AddSubSystem(this);
}

CSoundContext_ASIO::~CSoundContext_ASIO()
{ 

	KillThreads();

	MACRO_RemoveSubSystem(this);

	DestroyAll();

	if (m_DebugStrTempDSound2)
	{
		for (int i = 0; i < mcs_MaxDebugSounds - 1; ++i)
			delete [] m_DebugStrTempDSound2[i];

		delete [] m_DebugStrTempDSound2;
	}

}

bint CSoundContext_ASIO::Destroy_Everything()
{
	bint bDone = CSuper::Destroy_Everything();

	return bDone;
}


void CSoundContext_ASIO::CreateAll()
{
	if (!m_bInit)
	{

		m_Thread_AudioRender.m_pThis = this;
		m_Thread_AudioRender.Thread_Destroy();

		MultiStream_Resume();

		m_Thread_AudioRender.Thread_Create(NULL, 16384, MRTC_THREAD_PRIO_TIMECRITICAL);

		m_bInit = true;
	}
}

void CSoundContext_ASIO::DestroyAll()
{
	m_Thread_AudioRender.Thread_Destroy();
	if (m_bInit)
	{
		m_bInit = false;
		MultiStream_Pause();
	}

}

void CSoundContext_ASIO::Thread_AudioRender()
{
}


void CSoundContext_ASIO::Create(CStr _Params)
{
	CreateAll();

	CSuper::Create(_Params);
}

void CSoundContext_ASIO::KillThreads()
{
	m_Thread_AudioRender.Thread_Destroy();
	while (!Destroy_Everything())
	{
		m_Mixer.StartNewFrame();

		Sleep(10);
	}

	CSuper::KillThreads();
}

void CSoundContext_ASIO::Platform_GetInfo(CPlatformInfo &_Info)
{
	_Info.m_nProssingThreads = 1;
	_Info.m_FrameLength = 256;
	_Info.m_SampleRate = 48000;
	_Info.m_nChannels = 8;
	
	switch (_Info.m_nChannels)
	{
	case 1:
		{
			_Info.m_Speakers[0].m_Position = CVec3Dfp32(-1.0f, 0.0f, 0.0f); // Front Center
		}
		break;
	case 2:
		{
			_Info.m_Speakers[0].m_Position = CVec3Dfp32(0.0f, -1.0f, 0.0f); // Surround Left
			_Info.m_Speakers[1].m_Position = CVec3Dfp32(0.0f, 1.0f, 0.0f); // Surround Right
		}
		break;
	case 4:
		{
			_Info.m_Speakers[0].m_Position = CVec3Dfp32(-1.0f, -1.0f, 0.0f); // Front Left
			_Info.m_Speakers[1].m_Position = CVec3Dfp32(-1.0f, 1.0f, 0.0f); // Front Right
			_Info.m_Speakers[2].m_Position = CVec3Dfp32(0.0f, -1.0f, 0.0f); // Surround Left
			_Info.m_Speakers[3].m_Position = CVec3Dfp32(0.0f, 1.0f, 0.0f); // Surround Right
		}
		break;
	case 5:
		{
			_Info.m_Speakers[0].m_Position = CVec3Dfp32(-1.0f, -1.0f, 0.0f); // Front Left
			_Info.m_Speakers[1].m_Position = CVec3Dfp32(-1.0f, 1.0f, 0.0f); // Front Right
			_Info.m_Speakers[2].m_Position = CVec3Dfp32(0.0f, -1.0f, 0.0f); // Surround Left
			_Info.m_Speakers[3].m_Position = CVec3Dfp32(0.0f, 1.0f, 0.0f); // Surround Right
			_Info.m_Speakers[4].m_Position = CVec3Dfp32(-1.0f, 0.0f, 0.0f); // Front Center
		}
		break;
	case 6:
		{
			_Info.m_Speakers[0].m_Position = CVec3Dfp32(-1.0f, -1.0f, 0.0f); // Front Left
			_Info.m_Speakers[1].m_Position = CVec3Dfp32(-1.0f, 1.0f, 0.0f); // Front Right
			_Info.m_Speakers[2].m_Position = CVec3Dfp32(0.0f, -1.0f, 0.0f); // Surround Left
			_Info.m_Speakers[3].m_Position = CVec3Dfp32(0.0f, 1.0f, 0.0f); // Surround Right
			_Info.m_Speakers[4].m_Position = CVec3Dfp32(-1.0f, 0.0f, 0.0f); // Front Center
			_Info.m_Speakers[5].m_Position = CVec3Dfp32(0.0f, 0.0f, 0.0f); // LFE
			_Info.m_Speakers[5].m_bLFE = true;
		}
		break;
	case 8:
		{
			_Info.m_Speakers[0].m_Position = CVec3Dfp32(-1.0f, -1.0f, 0.0f); // Front Left
			_Info.m_Speakers[1].m_Position = CVec3Dfp32(-1.0f, 1.0f, 0.0f); // Front Right
			_Info.m_Speakers[2].m_Position = CVec3Dfp32(0.0f, -1.0f, 0.0f); // Surround Left
			_Info.m_Speakers[3].m_Position = CVec3Dfp32(0.0f, 1.0f, 0.0f); // Surround Right
			_Info.m_Speakers[4].m_Position = CVec3Dfp32(-1.0f, 0.0f, 0.0f); // Front Center
			_Info.m_Speakers[5].m_Position = CVec3Dfp32(0.0f, 0.0f, 0.0f); // LFE
			_Info.m_Speakers[5].m_bLFE = true;
			_Info.m_Speakers[6].m_Position = CVec3Dfp32(1.0f, -0.5f, 0.0f); // Back Left
			_Info.m_Speakers[7].m_Position = CVec3Dfp32(1.0f, 0.5f, 0.0f); // Back Right
		}
		break;

	}

	////////////////////////
	// 4 Surround:
	////////////////////////
	//fL                fR//
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//sL        L       sR//
	////////////////////////

	////////////////////////
	// 5 Surround:
	////////////////////////
	//fL        fC      fR//
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//sL        L       sR//
	////////////////////////

	////////////////////////
	// 6 Surround:
	////////////////////////
	//fL       fC       fR//
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//sL        L       sR//
	//                    //
	//                    //
	//          rC        //
	////////////////////////

	////////////////////////
	// 7 Surround:
	////////////////////////
	//fL       fC       fR//
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//sL        L       sR//
	//                    //
	//                    //
	//     rL       rR    //
	////////////////////////


	
}

void CSoundContext_ASIO::Platform_Init(uint32 _MaxMixerVoices)
{
	CSuper::Platform_Init(_MaxMixerVoices);
}

void CSoundContext_ASIO::Refresh()
{

	CSuper::Refresh();
}

aint CSoundContext_ASIO::OnMessage(const CSS_Msg& _Msg)
{
	return CSubSystem::OnMessage(_Msg);
}

void CSoundContext_ASIO::OnRefresh(int _Context)
{
	Refresh();
	return CSubSystem::OnRefresh(_Context);
}

void CSoundContext_ASIO::OnBusy(int _Context)
{
	Refresh();
	return CSubSystem::OnBusy(_Context);
}

#endif // PLATFORM_WIN_PC
