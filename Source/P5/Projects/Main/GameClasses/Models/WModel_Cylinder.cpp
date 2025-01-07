#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "CDynamicVB.h"
#include "CSurfaceKey.h"
#include "CSinRandTable.h"
#include "ModelsMisc.h"

//----------------------------------------------------------------------

const char* lpCYLINDER_FLAGS[] = { NULL };
enum
{
	CYLINDER_FLAGS_ = 0x0001,
};

//----------------------------------------------------------------------
// CXR_Model_Cylinder
//----------------------------------------------------------------------

class CXR_Model_Cylinder : public CXR_Model_Custom
{

	MRTC_DECLARE;

public:

private:

	//----------------------------------------------------------------------

	CStr				m_Keys;

	fp32					m_Time; // Total lifetime of model.
	fp32					m_Duration;
	int					m_Randseed, m_RandseedBase;

	CDynamicVB			m_DVB;
	CSurfaceKey			m_SK;

	CWireContainer*		m_pWC;

	int					m_iSurface;
	fp32					m_RenderPriorityBias;

	int					m_Flags;

	fp32					m_DurationFade;

	//----------------------------------------------------------------------

	/*
		Parameters:

			TimeScale
			TimeOffset

			Duration

			FadeInTime
			FadeOutTime

			BaseColor
			  
			BaseRadius
			Height
			FadeHeight

			ConeAngle
			
			Sides
			Segments
			
			TexMapType
			* Circular
			* Top




	*/

	//----------------------------------------------------------------------

	void GenerateQuad()
	{
		m_DVB.AddTriangle(0, 1, 2, 0);
		m_DVB.AddTriangle(2, 3, 0, 0);
	}	
	
	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_Cylinder_Render, MAUTOSTRIP_VOID);
		m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));

		m_Time = _pAnimState->m_AnimTime0;
		m_Duration = _pAnimState->m_AnimTime1;

		m_DurationFade = ::GetFade(m_Time, m_Duration, m_FadeTime, m_FadeTime);

		m_RandseedBase = _pAnimState->m_Anim0;
		m_Randseed = m_RandseedBase;

		CMat43fp32 InvWorldToCamera;
		_VMat.InverseOrthogonal(InvWorldToCamera);

		int NumVertices = 4;
		int NumTriangles = 2;

		m_SK.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface);

		if (!m_DVB.Create(this, m_pVBM, NumVertices, NumTriangles, DVBFLAGS_NORMALS | DVBFLAGS_TEXTURE | DVBFLAGS_COLORS))
			return;

		m_DVB.GetVB()->m_Priority += m_RenderPriorityBias;

		GenerateQuad();

		if (m_DVB.IsValid())
		{
			m_DVB.Render(_VMat);
			m_SK.Render(m_DVB.GetVB(), m_pVBM, m_pEngine);
		}
	}

	//----------------------------------------------------------------------

	void OnEvalKey(const CRegistry *_pReg)
	{
		MAUTOSTRIP(CXR_Model_Cylinder_OnEvalKey, MAUTOSTRIP_VOID);
		CStr Name = _pReg->GetThisName();
		CStr Value = _pReg->GetThisValue();
		int Valuei = _pReg->GetThisValue().Val_int();
		fp32 Valuef = _pReg->GetThisValue().Val_fp64();

		if (Name == "SU")
		{
			CFStr SurfaceName = Value.GetStrMSep(" #");
			m_iSurface = GetSurfaceID(SurfaceName);
			m_RenderPriorityBias = (fp32)Value.Val_fp64() * TransparencyPriorityBiasUnit;
		}

		else if (Name == "FLG")
			m_Flags = Value.TranslateFlags(lpCYLINDER_FLAGS);

		else
			CXR_Model_Custom::OnEvalKey(_pReg);
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *_keys)
	{
		MAUTOSTRIP(CXR_Model_Cylinder_OnCreate, MAUTOSTRIP_VOID);
		m_Keys = _keys;
		InitParameters();
		ParseKeys(_keys);
		PostprocessParameters();
	}

	//----------------------------------------------------------------------

	void InitParameters()
	{
		MAUTOSTRIP(CXR_Model_Cylinder_SetDefaultParameters, MAUTOSTRIP_VOID);
		m_iSurface = GetSurfaceID("");
		m_RenderPriorityBias = 0;

		m_Flags = 0;
	}

	//----------------------------------------------------------------------

	void PostprocessParameters()
	{
		MAUTOSTRIP(CXR_Model_Cylinder_PreprocessParameters, MAUTOSTRIP_VOID);
		m_RenderPriorityBias += TransparencyPriorityBaseBias;
	}

	//----------------------------------------------------------------------

};

//----------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Cylinder, CXR_Model_Custom);

//----------------------------------------------------------------------
