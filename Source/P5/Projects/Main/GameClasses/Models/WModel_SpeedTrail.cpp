#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"
#include "../../../../Shared/MOS/Classes/Render/MWireContainer.h"

#include "CDynamicVB.h"
#include "CModelHistory.h"
#include "CSurfaceKey.h"

//----------------------------------------------------------------------

#define MAXNUMBEAMSTRIPS				(40)

#define HISTORY_LENGTH					(20)
#define HISTORY_ENTRY_DELAY				(0.05f) // Time before a new entry is actually shifted on the history queue.

//----------------------------------------------------------------------

enum
{
	FLAGS_ALIGNFADE		= 0x01,
	FLAGS_TAILFADE		= 0x02,
	FLAGS_SPEEDFADE		= 0x04,
	FLAGS_TEXFIT		= 0x08,
	FLAGS_TEXLOCK		= 0x10,
};

//----------------------------------------------------------------------

#define LERP(a,b,t) ((a) + ((b) - (a))*t)
#define FRAC(a) ((a) - M_Floor(a))

#define getRand()		(MFloat_GetRand(m_Randseed++))
#define getSign(x)		(Sign(x))
#define getAbs(x)		(((x) < 0) ? -(x) : (x))

//----------------------------------------------------------------------

#define HISTORY_LENGTH					(20)
#define MAXTIMEJUMP						(0.1f)

//----------------------------------------------------------------------

typedef CModelHistory CMHistory;

//----------------------------------------------------------------------
// CXR_Model_SpeedTrail
//----------------------------------------------------------------------

class CXR_Model_SpeedTrail : public CXR_Model_Custom
{

	MRTC_DECLARE;

private:

	//----------------------------------------------------------------------

	CXR_BeamStrip*		m_BeamStrips;
	int32				m_NumBeamStrips, m_MaxNumBeamStrips;

	CXR_VertexBuffer*	m_pVB;
	CSurfaceKey			m_SK;

	fp32					m_Time;
	fp32					m_Duration;

	fp32					m_DurationFade;
	fp32					m_FadeInTime, m_FadeOutTime;

	fp32					m_TrailDuration;

	int					m_iSurface;
	fp32					m_MaxTrailDuration;
	fp32					m_Width;
	int32				m_Flags;


	fp32					m_HistoryEntryDelay;

	CMHistory*			m_pHistory;

	CBox3Dfp32			m_BBox;
	bool				m_BBox_bIsValid;

	CWireContainer*		m_pWC;

	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------

	CMHistory* AllocHistory()
	{
		MAUTOSTRIP(CXR_Model_SpeedTrail_AllocHistory, NULL);
		return (DNew(CMHistory) CMHistory(m_HistoryEntryDelay, 1.0f));
	}

	//----------------------------------------------------------------------

	TPtr<CXR_ModelInstance> CreateModelInstance()
	{
		return AllocHistory();
	}

	//----------------------------------------------------------------------

	// Get history from clientdata.
	CMHistory*
	GetHistory(const CXR_AnimState* pAnimState)
	{
		MAUTOSTRIP(CXR_Model_SpeedTrail_GetHistory, NULL);
		if (pAnimState == NULL)
			return NULL;

		if (pAnimState->m_spModelInstance == NULL)
			return NULL;

		CMHistory* pHistory = (CMHistory*)safe_cast<const CMHistory>((const CXR_ModelInstance*)pAnimState->m_spModelInstance);

		return pHistory;
	}

	//----------------------------------------------------------------------

#ifndef M_RTM

	CMHistory*
	ForceGetHistory(const CXR_AnimState* pAnimState)
	{
		MAUTOSTRIP(CXR_Model_SpeedTrail_ForceGetHistory, NULL);
		if (pAnimState == NULL)
			return NULL;

		if (pAnimState->m_pspClientData == NULL)
			return NULL;

		// Check if the clientdata is used for something else...
		CReferenceCount* RefCount = (CReferenceCount*)(*pAnimState->m_pspClientData);
		if ((RefCount != NULL) && (TDynamicCast<CMHistory>(RefCount) == NULL))
			return NULL;

		// Correct type, use it...
		CMHistory* pHistory = (CMHistory*)RefCount;

		// No history was allocated, so allocate it now...
		if (pHistory == NULL) {
			*pAnimState->m_pspClientData = AllocHistory();
			if (*pAnimState->m_pspClientData == NULL) return NULL;
			pHistory = (CMHistory*)(CReferenceCount*)(*pAnimState->m_pspClientData);
		}

		return pHistory;
	}

#endif

	//----------------------------------------------------------------------

	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
	{
		MAUTOSTRIP(CXR_Model_SpeedTrail_GetBound_Box, MAUTOSTRIP_VOID);
		m_pHistory = GetHistory(_pAnimState);
		if (m_pHistory != NULL)
		{
			_Box = m_pHistory->GetStoredBoundBox();
		}
		else
		{
			_Box.m_Min = -m_Width;
			_Box.m_Max = m_Width;
		}
	}

	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------

	void OnEvalKey(const CRegistry *_pReg)
	{
		MAUTOSTRIP(CXR_Model_SpeedTrail_OnEvalKey, MAUTOSTRIP_VOID);
		if ((_pReg->GetThisName() == "SURFACE") || (_pReg->GetThisName() == "0"))
			m_iSurface = GetSurfaceID(_pReg->GetThisValue());
		else if ((_pReg->GetThisName() == "DURATION") || (_pReg->GetThisName() == "1"))
			m_MaxTrailDuration = _pReg->GetThisValue().Val_fp64();
		else if ((_pReg->GetThisName() == "WIDTH") || (_pReg->GetThisName() == "2"))
			m_Width = _pReg->GetThisValue().Val_fp64();
		else if ((_pReg->GetThisName() == "FLAGS") || (_pReg->GetThisName() == "3"))
		{
			static const char* SpeedTrailTranslateFlags[] = {"ALIGNFADE", "TAILFADE", "SPEEDFADE", "TEXFIT", "TEXLOCK", NULL};
			m_Flags = _pReg->GetThisValue().TranslateFlags(SpeedTrailTranslateFlags);
		}
		else
			CXR_Model_Custom::OnEvalKey(_pReg);
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *_params)
	{
		MAUTOSTRIP(CXR_Model_SpeedTrail_OnCreate, MAUTOSTRIP_VOID);
		m_iSurface = GetSurfaceID("FuseSparks");

		SetDefaultParameters();
		ParseKeys(_params);
		PostProcessParameters();
	}

	//----------------------------------------------------------------------

	void SetDefaultParameters()
	{
		MAUTOSTRIP(CXR_Model_SpeedTrail_SetDefaultParameters, MAUTOSTRIP_VOID);
		m_MaxTrailDuration = 2.0f;
		m_Width = 5.0f;
		m_Flags = 0;
		m_FadeInTime = 0;
		m_FadeOutTime = 0;
	}

	//----------------------------------------------------------------------

	void PostProcessParameters()
	{
		MAUTOSTRIP(CXR_Model_SpeedTrail_PostProcessParameters, MAUTOSTRIP_VOID);
		m_HistoryEntryDelay = 1.2f * m_MaxTrailDuration / (HISTORY_LENGTH - 2);
	}

	//----------------------------------------------------------------------

	void
	addBeamStrip(CXR_BeamStrip& beamstrip)
	{
		MAUTOSTRIP(CXR_Model_SpeedTrail_addBeamStrip, MAUTOSTRIP_VOID);
		if (m_NumBeamStrips >= m_MaxNumBeamStrips)
			return;

		m_BeamStrips[m_NumBeamStrips] = beamstrip;
		m_NumBeamStrips++;
	}

	//----------------------------------------------------------------------

	void
	generateTrail()
	{
		MAUTOSTRIP(CXR_Model_SpeedTrail_generateTrail, MAUTOSTRIP_VOID);
		CXR_BeamStrip beamstrip;

		fp32 Alpha0, Alpha1;

		if (m_Flags & FLAGS_TAILFADE)
		{
			Alpha0 = 0.0f;
			Alpha1 = 1.0f;
		}
		else
		{
			Alpha0 = 1.0f;
			Alpha1 = 0.0f;
		}
		
		for (int iBeamStrip = 0; iBeamStrip < MAXNUMBEAMSTRIPS; iBeamStrip++)
		{
			fp32 TailFraction = (fp32)iBeamStrip / (fp32)MAXNUMBEAMSTRIPS;

			fp32 Time = m_Time - TailFraction * Min(m_Time, m_MaxTrailDuration);

			fp32 Alpha = Alpha0 + Alpha1 * (1.0f - TailFraction);
			if ((iBeamStrip == 0) || (iBeamStrip == (MAXNUMBEAMSTRIPS - 1)))
				Alpha = 0.0f;

			fp32 Speed;
			int Flags;
			CMat43fp32 Matrix;
			m_pHistory->GetInterpolatedMatrix(Time, Matrix, Speed, Flags);
			beamstrip.m_Pos = CVec3Dfp32::GetMatrixRow(Matrix, 3);

			if ((m_Flags & FLAGS_SPEEDFADE) && (Speed < 10.0f))
			{
				Alpha *= Speed / 10.0f;
			}

			beamstrip.m_Flags = (iBeamStrip == 0) ? CXR_BEAMFLAGS_BEGINCHAIN : 0;
			beamstrip.m_Color = 0x00FFFFFF | (int32(Alpha * 255.0f) << 24);
			beamstrip.m_Width = m_Width;
			
			if (m_Flags & FLAGS_TEXFIT)
				beamstrip.m_TextureYOfs = 0*TailFraction + Time / m_MaxTrailDuration;
			else
				beamstrip.m_TextureYOfs = 0*(fp32)iBeamStrip + Time;

			// BoundBox
			{
				int iFirstExpander = 0;

				if (!m_BBox_bIsValid)
				{
					m_BBox_bIsValid = true;
					m_BBox.m_Min = beamstrip.m_Pos;
					m_BBox.m_Max = beamstrip.m_Pos;
					iFirstExpander = 1;
				}

				if (beamstrip.m_Pos.k[0] < m_BBox.m_Min.k[0]) m_BBox.m_Min.k[0] = beamstrip.m_Pos.k[0];
				if (beamstrip.m_Pos.k[1] < m_BBox.m_Min.k[1]) m_BBox.m_Min.k[1] = beamstrip.m_Pos.k[1];
				if (beamstrip.m_Pos.k[2] < m_BBox.m_Min.k[2]) m_BBox.m_Min.k[2] = beamstrip.m_Pos.k[2];
				if (beamstrip.m_Pos.k[0] > m_BBox.m_Max.k[0]) m_BBox.m_Max.k[0] = beamstrip.m_Pos.k[0];
				if (beamstrip.m_Pos.k[1] > m_BBox.m_Max.k[1]) m_BBox.m_Max.k[1] = beamstrip.m_Pos.k[1];
				if (beamstrip.m_Pos.k[2] > m_BBox.m_Max.k[2]) m_BBox.m_Max.k[2] = beamstrip.m_Pos.k[2];
			}

			addBeamStrip(beamstrip);
		}
	}
	
	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_SpeedTrail_Render, MAUTOSTRIP_VOID);
		CXR_BeamStrip	Beamstrips[CXR_Util::MAXBEAMS];
		int32			numBeamstrips = CXR_Util::MAXBEAMS;

		m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));

		m_Time = _pAnimState->m_AnimTime0;
		m_Duration = _pAnimState->m_AnimTime1;

		m_DurationFade = ::GetFade(m_Time, m_Duration, m_FadeInTime, m_FadeOutTime);
		
		m_BeamStrips = Beamstrips;
		m_MaxNumBeamStrips = numBeamstrips;
		m_NumBeamStrips = 0;

		m_pHistory = GetHistory(_pAnimState);

#ifndef M_RTM
		if (m_pHistory == NULL)
			m_pHistory = ForceGetHistory(_pAnimState);
#endif

		if (m_pHistory != NULL)
		{
			CMat43fp32 Matrix = _WMat;
			int Flags = _pAnimState->m_Colors[3];
			m_pHistory->AddEntry(Matrix, m_Time, Flags);
		}

		m_pVB = AllocVB();
		if (m_pVB == NULL)
			return;

		m_SK.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface);

		m_BBox_bIsValid = false;

		generateTrail();

		if (m_BBox_bIsValid)
		{
			m_BBox.m_Min -= m_Width;
			m_BBox.m_Max += m_Width;
			CMat43fp32 WorldToLocal;
			_WMat.InverseOrthogonal(WorldToLocal);
			CBox3Dfp32 BBLocal;
			m_BBox.Transform(WorldToLocal, BBLocal);
			m_pHistory->StoreBoundBox(BBLocal);
		}

		CMat43fp32 Unit; Unit.Unit();

		if (CXR_Util::Render_BeamStrip2(m_pVBM, m_pVB, 
										Unit, _VMat, 
										m_BeamStrips, m_NumBeamStrips,
										((m_Flags & FLAGS_ALIGNFADE) ? CXR_BEAMFLAGS_EDGEFADE : 0)))
		{
			m_SK.Render(m_pVB, m_pVBM, m_pEngine);
		}
	}

};

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_SpeedTrail, CXR_Model_Custom);

//----------------------------------------------------------------------
