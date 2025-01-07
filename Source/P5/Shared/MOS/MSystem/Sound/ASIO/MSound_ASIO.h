


#ifndef DInc_MSound_ASIO_h
#define DInc_MSound_ASIO_h

#include "../Vorbis/MSound_Vorbis.h"

#include "../../../../SDK/ASIO/asiosys.h"
#include "../../../../SDK/ASIO/asio.h"
#include "../../../../SDK/ASIO/asiodrivers.h"

enum
{
	EPrimaryFormat_int16,
	EPrimaryFormat_int24,
	EPrimaryFormat_fp32,
};

// -------------------------------------------------------------------
class CSoundContext_ASIO : public CSoundContext_Vorbis, public CSubSystem
{
	typedef CSoundContext_Vorbis CSuper;

	MRTC_DECLARE;

private:

	class CAudioRenderThread : public MRTC_Thread
	{
	public:
		CSoundContext_ASIO *m_pThis;

		int Thread_Main()
		{
			// Make sure that we sleep only one ms
			timeBeginPeriod(1);
			MRTC_SystemInfo::Thread_SetName("DSound audio render thread");
			while (!Thread_IsTerminating())
			{
				m_pThis->Thread_AudioRender();
				Sleep(1);
			}
			timeEndPeriod(1);
			return 0;
		}
	};

	CAudioRenderThread m_Thread_AudioRender;

	const static int mcs_MaxDebugSounds = 384;
	char ** m_DebugStrTempDSound2;

	bint m_bInit;

	void Thread_AudioRender();

	void CreateAll();
	void DestroyAll();

	virtual void Create(CStr _Params);
	virtual void KillThreads();

	virtual void Platform_GetInfo(CPlatformInfo &_Info);
	virtual void Platform_Init(uint32 _MaxMixerVoices);

public:
	CSoundContext_ASIO();
	~CSoundContext_ASIO();

	bint Destroy_Everything();

	virtual void Refresh();

	// Overrides from CSubSystem
	virtual aint OnMessage(const CSS_Msg& _Msg);
	virtual void OnRefresh(int _Context);
	virtual void OnBusy(int _Context);
};

typedef TPtr<CSoundContext_ASIO> spCSoundContext_ASIO;

#endif // DInc_MSound_ASIO_h
