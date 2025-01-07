#include "PCH.h"
#include "LipSync.h"

/*
This is only a temporary measure until we get the full version of FaceFX.
*/

#ifdef LIPSYNC_TALKBACK_AND_FACEFX

	using namespace NLipSync;

	class CAnalyser_TalkbackAndFaceFX : public CAnalyser
	{
	public:
		CAnalyser *m_pTalkbackAnalyser;
		CAnalyser *m_pFaceFXAnalyser;

		CAnalyser_TalkbackAndFaceFX()
		{
			m_pTalkbackAnalyser = GetTalkbackAnalyser();
			m_pFaceFXAnalyser = GetFaceFXAnalyser();

			M_ASSERT(m_pTalkbackAnalyser, "Could not create Talkback analyser");
			M_ASSERT(m_pFaceFXAnalyser, "Could not create FaceFX analyser");
		}

		~CAnalyser_TalkbackAndFaceFX() 
		{
		}

		CAnalysis *GetAnalysis(const char *_pFilename, int32 _FrameRate)
		{
			CAnalysis *pAnalysis = m_pFaceFXAnalyser->GetAnalysis(_pFilename, _FrameRate);
			if (!pAnalysis)
				pAnalysis = m_pTalkbackAnalyser->GetAnalysis(_pFilename, _FrameRate);

			return pAnalysis;
		}
	};


	CAnalyser_TalkbackAndFaceFX *g_pTalkbackAndFaceFXAnalyser = NULL;

	//
	// 
	//
	class CTalkbackAndFaceFXRemover
	{
	public:
		~CTalkbackAndFaceFXRemover()
		{
			if(g_pTalkbackAndFaceFXAnalyser)
			{
				delete g_pTalkbackAndFaceFXAnalyser;
				g_pTalkbackAndFaceFXAnalyser = NULL;
			}
		}
	};

	CTalkbackAndFaceFXRemover g_FaceFXRemover;

	//
	//
	//
	CAnalyser *NLipSync::GetAnalyser()
	{
		if(!g_pTalkbackAndFaceFXAnalyser)
			g_pTalkbackAndFaceFXAnalyser = DNew(CAnalyser_TalkbackAndFaceFX) CAnalyser_TalkbackAndFaceFX();
		return g_pTalkbackAndFaceFXAnalyser;
	}

#endif // LIPSYNC_TALKBACK_AND_FACEFX
