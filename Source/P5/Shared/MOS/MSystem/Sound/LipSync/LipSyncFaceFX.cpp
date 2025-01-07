//
// FaceFX LipSync Analyser
//

#include "PCH.h"
#include "LipSync.h"

//#ifdef LIPSYNC_FACEFX
#if defined(LIPSYNC_FACEFX) || defined(LIPSYNC_TALKBACK_AND_FACEFX)

	#include <FaceFX 1.61/FxSDK.h>
	#include <FaceFX 1.61/FxActor.h>
	#include <FaceFX 1.61/FxAnim.h>
	#include <FaceFX 1.61/FxAnimCurve.h>

	//#pragma comment(linker, "/DEFAULTLIB:S:\\Source\\P5\\SDK\\FaceFX_1.61\\lib\\win32\\FxSDK_RMT.lib")
	#pragma comment(lib, "..\\..\\..\\SDK\\FaceFX_1.61\\lib\\win32\\FxSDK_RMT.lib")

	// Includes

	using namespace NLipSync;
	using namespace OC3Ent::Face;

/*
	// This array is used to find the correct FaceFX anim channel for each speech target.
	// The order of the phonemes comes from Facial_Talkback.xan
	static const char *gc_FaceFXSpeechTargets[NUMSPEECHTARGETS] = {
		"Bump",
		"Cage",
		"Church",
		"Earth",
		"Eat",
		"Fave",
		"If",
		"New",
		"Oat",
		"Ox",
		"Roar",
		"Size",
		"Though",
		"Told",
		"Wet",
	};
*/
	// This array is used to find the correct FaceFX anim channel for each speech target.
	// The order of the SpeechTargets is the same as Talkback used.
	static const char *gc_FaceFXSpeechTargets[NUMSPEECHTARGETS] = {
		"Earth",
		"If",
		"Ox",
		"Oat",
		"Wet",
		"Size",
		"Church",
		"Fave",
		"Though",
		"Told",
		"Bump",
		"New",
		"Roar",
		"Cage",
	};

	class CAnalysis_FaceFX : public CAnalysis
	{
	public:
		CAnalysis_FaceFX(const FxAnim &_Anim, int _FrameRate) : m_Anim(_Anim), m_FrameRate(_FrameRate),
			m_SecondsPerFrame(1.0f / (float)_FrameRate)
		{
			float Len = m_Anim.GetDuration();

			float NumFrames = Len / m_SecondsPerFrame;

			int nAnimCurves = m_Anim.GetNumAnimCurves();

			// NOTE: Rounding the exact anim start DOWN to the nearest whole frame.
			m_FirstFrame = (int32)Floor(m_Anim.GetStartTime() / m_SecondsPerFrame);	
			// NOTE: Rounding the exact anim end UP to the nearest whole frame.
			m_LastFrame = m_FirstFrame + (int32)Ceil(m_Anim.GetDuration() / m_SecondsPerFrame);

			/*
			// NOTE: Rounding the exact anim start UP to the nearest whole frame.
			m_FirstFrame = (int32)Ceil(m_Anim.GetStartTime() / m_SecondsPerFrame);	
			// NOTE: Rounding the exact anim end DOWN to the nearest whole frame.
			m_LastFrame = m_FirstFrame + (int32)Floor(m_Anim.GetDuration() / m_SecondsPerFrame);
			*/

			// Find the correct anim curve for each speech target.
			for (int i = 0; i < NUMSPEECHTARGETS; i++)
			{
				m_AnimCurves[i] = NULL;
				for (int c = 0; c < nAnimCurves; c++) 
				{
					const FxAnimCurve &CurCurve = m_Anim.GetAnimCurve(c);
					if (CurCurve.GetName() == gc_FaceFXSpeechTargets[i]) 
					{
						m_AnimCurves[i] = &CurCurve;
						break;
					}
				}
				if (m_AnimCurves[i] == NULL) 
				{
					// TODO: Report error?
				}
			}
		}

		~CAnalysis_FaceFX()
		{
		}

		int32 GetFrameRate()
		{
			return m_FrameRate;
		}

		int32 GetFirstFrame() 
		{
			return m_FirstFrame;
		}

		int32 GetLastFrame() 
		{
			return m_LastFrame;
		}

		fp32 GetSpeechTargetValueAtFrame(int32 _Track, int32 _Frame) 
		{
			if (_Track >= 0 && _Track < NUMSPEECHTARGETS )
			{
				if (m_AnimCurves[_Track])
					return m_AnimCurves[_Track]->EvaluateAt(m_SecondsPerFrame * (float)(_Frame));
				else
					return 0.0f;
			}
			else
				return 0.0f;
		}

	protected:
		const FxAnim &m_Anim;
		const FxAnimCurve *m_AnimCurves[NUMSPEECHTARGETS];
		int m_FrameRate;
		float m_SecondsPerFrame;

		int32 m_FirstFrame;
		int32 m_LastFrame;
	};

	// Memory functions used by FaceFX
	void * gfFaceFXUserAllocate(FxSize _nBytes)
	{
		return DNew(char) char [_nBytes];
	}

	void *gfFaceFXUserAllocateDebug(FxSize _nBytes, const FxChar *system)
	{
		return DNew(char) char [_nBytes];
	}

	void gfFaceFXUserFree(void *_pPtr, FxSize _nBytes)
	{
		delete [](char *)_pPtr;
	}

	class CAnalyser_FaceFX : public CAnalyser
	{
	public:
		CAnalyser_FaceFX()
		{
			// Start up the FaceFX SDK using our memory routines.
			FxMemoryAllocationPolicy AllocPolicy(MAT_Custom, FxFalse, 
				gfFaceFXUserAllocate, gfFaceFXUserAllocateDebug, gfFaceFXUserFree);

			FxSDKStartup(AllocPolicy);
		}

		~CAnalyser_FaceFX() 
		{
			// Shut down the FaceFX SDK.
			FxSDKShutdown();
		}

		CAnalysis *GetAnalysis(const char *_pFilename, int32 _FrameRate)
		{
			CStr Filename = _pFilename;

			CStr FaceFXDataFilename = Filename.GetPath() + Filename.GetFilenameNoExt() + ".FXA";

			// Load the associated actor file.
			FxActor *pActor = new FxActor();
			FxBool loaded = FxLoadActorFromFile(*pActor, (const char *)FaceFXDataFilename, FxTrue);

			if (!loaded)
				return NULL;

			// Get the anim group called "Starbreeze"
			FxSize iSBAnimGroup;
			iSBAnimGroup = pActor->FindAnimGroup("Starbreeze");

			if (iSBAnimGroup == FxInvalidIndex)
				return NULL;

			FxAnimGroup &SBAnims = pActor->GetAnimGroup(iSBAnimGroup);

			// Find the correct animation.
			CStr AnimName = Filename.GetFilenameNoExt();
			FxSize iAnim = SBAnims.FindAnim((const char *)AnimName);

			if (iAnim == FxInvalidIndex)
				return NULL;

			const FxAnim &Anim = SBAnims.GetAnim(iAnim);

			return new CAnalysis_FaceFX(Anim, _FrameRate);
		}
	};


	CAnalyser_FaceFX *g_pFaceFXAnalyser = NULL;

	//
	// 
	//
	class CFaceFXRemover
	{
	public:
		~CFaceFXRemover()
		{
			if(g_pFaceFXAnalyser)
			{
				delete g_pFaceFXAnalyser;
				g_pFaceFXAnalyser = NULL;
			}
		}
	};

	CFaceFXRemover g_FaceFXRemover;

#endif // defined(LIPSYNC_FACEFX) || defined(LIPSYNC_TALKBACK_AND_FACEFX)


#ifdef LIPSYNC_FACEFX
	//
	//
	//
	CAnalyser *NLipSync::GetAnalyser()
	{
		if(!g_pFaceFXAnalyser)
			g_pFaceFXAnalyser = DNew(CAnalyser_FaceFX) CAnalyser_FaceFX();
		return g_pFaceFXAnalyser;
	}

#endif // LIPSYNC_FACEFX

#ifdef LIPSYNC_TALKBACK_AND_FACEFX
	//
	//
	//
	CAnalyser *NLipSync::GetFaceFXAnalyser()
	{
		if(!g_pFaceFXAnalyser)
			g_pFaceFXAnalyser = DNew(CAnalyser_FaceFX) CAnalyser_FaceFX();
		return g_pFaceFXAnalyser;
	}

#endif // LIPSYNC_FACEFX
