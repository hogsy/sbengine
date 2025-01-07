#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "CSinRandTable.h"

//----------------------------------------------------------------------

const char* lpDYNLIGHT_FLAGS[] = { NULL };
enum
{
	DYNLIGHT_FLAGS_ = 0x0001,
};

//----------------------------------------------------------------------
// CXR_Model_DynLight
//----------------------------------------------------------------------

class CXR_Model_DynLight : public CXR_Model_Custom
{

	MRTC_DECLARE;

public:

private:

	//----------------------------------------------------------------------

	CStr				m_Keys;

	fp32					m_Time; // Total lifetime of model.
	fp32					m_Duration;
	int					m_Randseed, m_RandseedBase;


	int					m_Flags;

	fp32					m_DurationFade;

	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat)
	{
		//m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));

		m_Time = _pAnimState->m_AnimTime0;
		m_Duration = _pAnimState->m_AnimTime1;

		m_DurationFade = ::GetFade(m_Time, m_Duration, m_FadeTime, m_FadeTime);

		m_RandseedBase = _pAnimState->m_Anim0;
		m_Randseed = m_RandseedBase;

/*
	void AddPulseLight(const CXR_AnimState* _pAnimState, const CVec3Dfp32 &_Pos, const CVec3Dfp32 &_Col, const CVec3Dfp32 &_AddCol, fp32 _Radius, fp32 _AddRadius, fp32 _PulseTime);
	void AddDistortLight(const CXR_AnimState* _pAnimState, const CVec3Dfp32 &_Pos, const CVec3Dfp32 &_Col, const CVec3Dfp32 &_AddCol, fp32 _Radius, fp32 _AddRadius);
	bool ApplyDiffuseLight(const CVec3Dfp32 &_Pos, int _NumVertices, CVec3Dfp32* _VertexPos, CVec3Dfp32* _VertexNormal, CPixel32* _VertexLight, fp32 _Alpha = 1.0f, fp32 _Boost = 1.0f);

		AddDistortLight(_pAnimState, CVec3Dfp32::GetMatrixRow(pos, 3),
						CVec3Dfp32(0.5f, 0.5f, 0.5f), CVec3Dfp32(0.1f, 0.1f, 0.1f), SPARK_RADIUS, SPARK_RADIUS * 0.1f);
*/

/*
		CVec3Dfp32 Pos;
		CVec3Dfp32 Color;
		fp32 Range;

		m_pEngine->Render_Light_AddDynamic(Pos, Color, Range);
*/
	}

	//----------------------------------------------------------------------

	void OnEvalKey(const CRegistry *_pReg)
	{
		CStr Name = _pReg->GetThisName();
		CStr Value = _pReg->GetThisValue();
		int Valuei = _pReg->GetThisValue().Val_int();
		fp32 Valuef = _pReg->GetThisValue().Val_fp64();

		if (Name == "FLG")
			m_Flags = Value.TranslateFlags(lpDYNLIGHT_FLAGS);

		else
			CXR_Model_Custom::OnEvalKey(_pReg);
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *_keys)
	{
		m_Keys = _keys;
		SetDefaultParameters();
		ParseKeys(_keys);
		PreprocessParameters();
	}

	//----------------------------------------------------------------------

	void SetDefaultParameters()
	{
		m_Flags = 0;
	}

	//----------------------------------------------------------------------

	void PreprocessParameters()
	{
	}

	//----------------------------------------------------------------------

};

//----------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_DynLight, CXR_Model_Custom);

//----------------------------------------------------------------------
