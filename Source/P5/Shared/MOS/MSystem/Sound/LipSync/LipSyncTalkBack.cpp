//
// talkback lipsync analyser
//
#include "PCH.h"
#include "LipSync.h"

#if defined(LIPSYNC_TALKBACK) || defined(LIPSYNC_TALKBACK_AND_FACEFX)
//#ifdef LIPSYNC_TALKBACK 

// includes
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRA_LEAN
#include <windows.h> // sue me

#include <TalkBack.h>

using namespace NLipSync;

#define TALKBACK_USEDLL

#ifdef TALKBACK_USEDLL
	
	//
	// This is a nasty hack. The Lib files for TalkBack doesn't work for VS 7.1 so the dll is compiled in 7.0
	//

	//
	// dll version
	//
	typedef TALKBACK_ERR(*TBSTARTUPLIBRARYPROC)(char const *iCoreDataDir);
	typedef TALKBACK_ERR(*TBSHUTDOWNLIBRARYPROC)();
	typedef TALKBACK_ERR(*TBGETANALYSISPROC)(TALKBACK_ANALYSIS **ioAnalysis, char const *iSoundFileName,
									char const *iSoundText, TALKBACK_ANALYSIS_SETTINGS *iSettings);
	typedef TALKBACK_ERR(*TBFREEANALYSISPROC)(TALKBACK_ANALYSIS **ioAnalysis);
	typedef TALKBACK_ERR(*TBGETFIRSTFRAMENUMPROC)(TALKBACK_ANALYSIS *iAnalysis, long *oResult);
	typedef TALKBACK_ERR(*TBGETLASTFRAMENUMPROC)(TALKBACK_ANALYSIS *iAnalysis, long *oResult);
	typedef TALKBACK_ERR(*TBGETSPEECHTARGETVALUEATFRAMEPROC)(TALKBACK_ANALYSIS *iAnalysis, long iTrackNum, long iFrame, double *oResult);

	TBSTARTUPLIBRARYPROC g_pfnStartupLibrary;
	TBSHUTDOWNLIBRARYPROC g_pfnShutdownLibrary;
	TBGETANALYSISPROC g_pfnGetAnalysis;
	TBFREEANALYSISPROC g_pfnFreeAnalysis;
	TBGETFIRSTFRAMENUMPROC g_pfnGetFirstFrameNum;
	TBGETLASTFRAMENUMPROC g_pfnGetLastFrameNum;
	TBGETSPEECHTARGETVALUEATFRAMEPROC g_pfnGetSpeechTargetValueAtFrame;

	class CAnalysis_TalkBack : public CAnalysis
	{
	public:
		TALKBACK_ANALYSIS *m_pAnalysis;
		int32 m_FrameRate;

		//
		//
		virtual ~CAnalysis_TalkBack()
		{
			g_pfnFreeAnalysis(&m_pAnalysis);
		}

		//
		//
		virtual int32 GetFrameRate()
		{
			return m_FrameRate;
		}

		//
		//
		virtual int32 GetFirstFrame()
		{
			int32 Frame;
			g_pfnGetFirstFrameNum(m_pAnalysis, &Frame);
			return Frame;
		}
			
		//
		//
		virtual int32 GetLastFrame()
		{
			int32 Frame;
			g_pfnGetLastFrameNum(m_pAnalysis, &Frame);
			return Frame;
		}

		//
		//
		virtual fp32 GetSpeechTargetValueAtFrame(int32 _Track, int32 _Frame)
		{
			fp64 Value;
			g_pfnGetSpeechTargetValueAtFrame(m_pAnalysis, _Track, _Frame, &Value);
			return (fp32)Value;
		}
	};

	//
	// dll version
	//
	class CAnalyser_TalkBack : public CAnalyser
	{
		HMODULE m_hDLL;
	public:
		CAnalyser_TalkBack()
		{
			m_hDLL = LoadLibrary("TalkbackDLL.dll");
			M_ASSERT(m_hDLL, "no DLL");

			//
			g_pfnStartupLibrary = (TBSTARTUPLIBRARYPROC)GetProcAddress(m_hDLL, "TBStartupLibrary");
			g_pfnShutdownLibrary = (TBSHUTDOWNLIBRARYPROC)GetProcAddress(m_hDLL, "TBShutdownLibrary");
			g_pfnGetAnalysis = (TBGETANALYSISPROC)GetProcAddress(m_hDLL, "TBGetAnalysis");
			g_pfnFreeAnalysis = (TBFREEANALYSISPROC)GetProcAddress(m_hDLL, "TBFreeAnalysis");
			g_pfnGetFirstFrameNum = (TBGETFIRSTFRAMENUMPROC)GetProcAddress(m_hDLL, "TBGetFirstFrameNum");
			g_pfnGetLastFrameNum = (TBGETLASTFRAMENUMPROC)GetProcAddress(m_hDLL, "TBGetLastFrameNum");
			g_pfnGetSpeechTargetValueAtFrame = (TBGETSPEECHTARGETVALUEATFRAMEPROC)GetProcAddress(m_hDLL, "TBGetSpeechTargetValueAtFrame");

			// time travel
			// Yet another hack. Set back the time to fool TalkBack. Waiting for licence.
			SYSTEMTIME StartTime, DemoTime;
			GetSystemTime(&StartTime);
			GetSystemTime(&DemoTime);
			DemoTime.wYear = 2003;
			DemoTime.wMonth = 2;
			DemoTime.wDay = 10;
			SetSystemTime(&DemoTime);

			// start lib
			g_pfnStartupLibrary("talkbackdata");

			// time travel
			SYSTEMTIME EndTime;
			GetSystemTime(&EndTime);
			EndTime.wYear = StartTime.wYear;
			EndTime.wDay = StartTime.wDay;
			EndTime.wMonth = StartTime.wMonth;
			SetSystemTime(&EndTime);
		}


		virtual ~CAnalyser_TalkBack()
		{
			if(g_pfnShutdownLibrary)
				g_pfnShutdownLibrary();
			FreeLibrary(m_hDLL);
		}

		virtual CAnalysis *GetAnalysis(const char *_pFilename, int32 _FrameRate)
		{
			M_ASSERT(_pFilename, "Bad move, punk!")

			M_TRACEALWAYS("LipSync: Analysing '%s'...", _pFilename);

			TALKBACK_ANALYSIS_SETTINGS AnalysisSettings =
			{
				TALKBACK_SETTINGS_SIZE,
				_FrameRate,
				TALKBACK_OPTIMIZE_FOR_FLIPBOOK_OFF,
				TALKBACK_RANDOM_SEED,
				TALKBACK_NO_CONFIG_FILE
			};

			TALKBACK_ANALYSIS *pTempAnalysis;
			TALKBACK_ERR TalkBackErr = g_pfnGetAnalysis(&pTempAnalysis, _pFilename, NULL, &AnalysisSettings);
			if(TalkBackErr != TALKBACK_NOERR)
			{
				M_TRACE("TalkBack error: %d\n", TalkBackErr);
				return NULL;
			}

			CAnalysis_TalkBack *pAnalysis = DNew(CAnalysis_TalkBack) CAnalysis_TalkBack();
			pAnalysis->m_FrameRate = _FrameRate;
			pAnalysis->m_pAnalysis = pTempAnalysis;
			M_TRACEALWAYS("Done\n");
			return pAnalysis;
		}
	};
#else
	//
	// non dll version
	//
/*
	class CAnalysis_TalkBack : public CAnalysis
	{
	public:
		TALKBACK_ANALYSIS *m_pAnalysis;

		//
		//
		virtual ~CAnalysis_TalkBack()
		{
			TalkBackFreeAnalysis(&m_pAnalysis);
		}

		//
		//
		virtual int32 GetFirstFrame()
		{
			int32 Frame;
			TalkBackGetFirstFrameNum(m_pAnalysis, &Frame);
			return Frame;
		}
			
		//
		//
		virtual int32 GetLastFrame()
		{
			int32 Frame;
			TalkBackGetLastFrameNum(m_pAnalysis, &Frame);
			return Frame;
		}

		//
		//
		virtual fp32 GetSpeechTargetValueAtFrame(int32 _Track, int32 _Frame)
		{
			fp64 Value;
			TalkBackGetSpeechTargetValueAtFrame(m_pAnalysis, _Track, _Frame, &Value);
			return (fp32)Value;
		}
	};

	class CAnalyser_TalkBack : public CAnalyser
	{
	public:
		CAnalyser_TalkBack()
		{
			// time travel
			SYSTEMTIME StartTime, DemoTime;
			GetSystemTime(&StartTime);
			GetSystemTime(&DemoTime);
			DemoTime.wMonth = 2;
			DemoTime.wDay = 10;
			SetSystemTime(&DemoTime);

			// start lib
			TalkBackStartupLibrary("./talkbackdata");

			// time travel
			SYSTEMTIME EndTime;
			GetSystemTime(&EndTime);
			EndTime.wDay = StartTime.wDay;
			EndTime.wMonth = StartTime.wMonth;
			SetSystemTime(&EndTime);
		}


		virtual ~CAnalyser_TalkBack()
		{
			TalkBackShutdownLibrary();
		}

		virtual CAnalysis *GetAnalysis(const char *_pFilename, int32 _FrameRate)
		{
			M_ASSERT(_pFilename, "Bad move, punk!")

			M_TRACEALWAYS("LipSync: Analysing '%s'...", _pFilename);

			TALKBACK_ANALYSIS_SETTINGS AnalysisSettings =
			{
				TALKBACK_SETTINGS_SIZE,
				_FrameRate,
				TALKBACK_OPTIMIZE_FOR_FLIPBOOK_OFF,
				TALKBACK_RANDOM_SEED,
				TALKBACK_NO_CONFIG_FILE
			};

			TALKBACK_ANALYSIS *pTempAnalysis;
			TALKBACK_ERR TalkBackErr = TalkBackGetAnalysis(&pTempAnalysis, _pFilename, NULL, &AnalysisSettings);
			if(TalkBackErr != TALKBACK_NOERR)
				return NULL;

			CAnalysis_TalkBack *pAnalysis = DNew(CAnalysis_TalkBack) CAnalysis_TalkBack();
			pAnalysis->m_pAnalysis = pTempAnalysis;
			M_TRACEALWAYS("Done\n");
			return pAnalysis;
		}
	};
	*/
#endif

CAnalyser_TalkBack *g_pTalkBackAnalyser = NULL;

//
// 
//
class CTalkBackRemover
{
public:
	~CTalkBackRemover()
	{
		if(g_pTalkBackAnalyser)
		{
			delete g_pTalkBackAnalyser;
			g_pTalkBackAnalyser = NULL;
		}
	}
};

CTalkBackRemover g_TalkBackRemover;

#endif // LIPSYNC_TALKBACK || LIPSYNC_TALKBACK_AND_FACEFX

#ifdef LIPSYNC_TALKBACK 

//
//
//
CAnalyser *NLipSync::GetAnalyser()
{
	if(!g_pTalkBackAnalyser)
		g_pTalkBackAnalyser = DNew(CAnalyser_TalkBack) CAnalyser_TalkBack();
	return g_pTalkBackAnalyser;
}

#endif // LIPSYNC_TALKBACK

#ifdef LIPSYNC_TALKBACK_AND_FACEFX

//
//
//
CAnalyser *NLipSync::GetTalkbackAnalyser()
{
	if(!g_pTalkBackAnalyser)
		g_pTalkBackAnalyser = DNew(CAnalyser_TalkBack) CAnalyser_TalkBack();
	return g_pTalkBackAnalyser;
}

#endif // LIPSYNC_TALKBACK

