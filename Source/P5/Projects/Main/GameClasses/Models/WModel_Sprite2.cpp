#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "CDynamicVB.h"
#include "CSurfaceKey.h"
#include "CSinRandTable.h"
#include "ModelsMisc.h"

//----------------------------------------------------------------------

const char* lpSPRITE_FLAGS[] = { NULL };
enum
{
	SPRITE_FLAGS_ = 0x0001,
};

//----------------------------------------------------------------------
// CXR_Model_Sprite2
//----------------------------------------------------------------------

class CXR_Model_Sprite2 : public CXR_Model_Custom
{

	MRTC_DECLARE;

public:

private:

	//----------------------------------------------------------------------

	CStr				m_Keys;

	class CRenderParams
	{
	public:
		fp32					m_Time; // Total lifetime of model.
		fp32					m_Duration;
		fp32					m_DurationFade;
		int					m_Randseed, m_RandseedBase;

		CDynamicVB			m_DVB;
		CSurfaceKey			m_SK;

		CVec3Dfp32			m_CameraFwd;
	};

//	CSinRandTable1024x8	m_SinRandTable;

//	CWireContainer*		m_pWC;

	int					m_iSurface;
	fp32					m_RenderPriorityBias;

	int					m_Flags;

	//----------------------------------------------------------------------

	/*
		Parameters:

			XSize
			YSize
			XSides
			YSides

			VertexPosTurbulence (hur unified som fluctar)
			VertexPosTurbulenceFluct (fluctuation amplitude)
			VertexPosTurbulenceFluctSpeed

			TexTurbulence
			TexTurbulenceFluct
			TexTurbulenceFluctSpeed

			OcclusionFadeTime (requires history)

			Rotation (constant delta, i.e. rot/time, when RotationFluctSpeed is zero, else it's the fluctuation amplitude)
			RotationFluctSpeed

			Flags
				DontFaceCamera
				FadeOccluded



			SEPARATE!
				GRID (grid with lots of fluct features, for magic portals and such)
				SPRITE (facing or not facing camera)
				FLARE (with occlusion tracing and fading, requires history)
		


	*/

	//----------------------------------------------------------------------

	void GenerateQuad(CRenderParams* M_RESTRICT _pRP)
	{
		_pRP->m_DVB.AddTriangle(0, 1, 2, 0);
		_pRP->m_DVB.AddTriangle(2, 3, 0, 0);
	}	
	
	//----------------------------------------------------------------------

	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_Sprite2_Render, MAUTOSTRIP_VOID);
//		m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));

		CRenderParams RP;

		RP.m_Time = _pAnimState->m_AnimTime0.GetTime(); // CMTIMEFIX
		RP.m_Duration = _pAnimState->m_AnimTime1.GetTime(); // CMTIMEFIX

		fp32 FadeTime = _pRenderParams->GetFadeTime();
		RP.m_DurationFade = ::GetFade(RP.m_Time, RP.m_Duration, FadeTime, FadeTime);

		RP.m_RandseedBase = _pAnimState->m_Anim0;
		RP.m_Randseed = RP.m_RandseedBase;

		CMat4Dfp32 InvWorldToCamera;
		_VMat.InverseOrthogonal(InvWorldToCamera);
		RP.m_CameraFwd = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 2);

		int NumVertices = 4;
		int NumTriangles = 2;

		RP.m_SK.Create(GetSurfaceContext(), _pRenderParams->m_pEngine, _pAnimState, m_iSurface);

		if (!RP.m_DVB.Create(_pRenderParams, this, _pRenderParams->m_pVBM, NumVertices, NumTriangles, DVBFLAGS_NORMALS | DVBFLAGS_TEXTURE | DVBFLAGS_COLORS))
			return;

		CXR_VertexBuffer* pVB = RP.m_DVB.GetVB();
		pVB->m_Priority += m_RenderPriorityBias;

		GenerateQuad(&RP);

		if (RP.m_DVB.IsValid())
		{
			RP.m_DVB.Render(_VMat);
			RP.m_SK.Render(pVB, _pRenderParams->m_pVBM, _pRenderParams->m_pEngine);
		}
	}

	//----------------------------------------------------------------------

	void OnEvalKey(const CRegistry *_pReg)
	{
		MAUTOSTRIP(CXR_Model_Sprite2_OnEvalKey, MAUTOSTRIP_VOID);
		CStr Name = _pReg->GetThisName();
		CStr Value = _pReg->GetThisValue();
//		int Valuei = _pReg->GetThisValue().Val_int();
//		fp32 Valuef = _pReg->GetThisValue().Val_fp64();

		if (Name == "SU")
		{
			CFStr SurfaceName = Value.GetStrMSep(" #");
			m_iSurface = GetSurfaceID(SurfaceName);
			m_RenderPriorityBias = (fp32)Value.Val_fp64() * TransparencyPriorityBiasUnit;
		}

		else if (Name == "FLG")
			m_Flags = Value.TranslateFlags(lpSPRITE_FLAGS);

		else
			CXR_Model_Custom::OnEvalKey(_pReg);
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *_keys)
	{
		MAUTOSTRIP(CXR_Model_Sprite2_OnCreate, MAUTOSTRIP_VOID);
		m_Keys = _keys;
		SetDefaultParameters();
		ParseKeys(_keys);
		PreprocessParameters();
	}

	//----------------------------------------------------------------------

	void SetDefaultParameters()
	{
		MAUTOSTRIP(CXR_Model_Sprite2_SetDefaultParameters, MAUTOSTRIP_VOID);
		m_iSurface = GetSurfaceID("");
		m_RenderPriorityBias = 0;

		m_Flags = 0;
	}

	//----------------------------------------------------------------------

	void PreprocessParameters()
	{
		MAUTOSTRIP(CXR_Model_Sprite2_PreprocessParameters, MAUTOSTRIP_VOID);
		m_RenderPriorityBias += TransparencyPriorityBaseBias;
	}

	//----------------------------------------------------------------------

};

//----------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Sprite2, CXR_Model_Custom);

//----------------------------------------------------------------------
