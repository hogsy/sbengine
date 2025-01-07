#include "PCH.h"

#include "MFloat.h"
#include "WBSP4Model.h"
#include "WBSP4Def.h"
#include "../../Classes/BitString/MBitString.h"

#include "../../XR/XREngineVar.h"
#include "../../XR/XRVBContext.h"


// -------------------------------------------------------------------

// -------------------------------------------------------------------
//  CXR_Model_BSP4
// -------------------------------------------------------------------
int CXR_Model_BSP4::ms_Enable = -1 - MODEL_BSP_ENABLE_SPLINEWIRE - MODEL_BSP_ENABLE_FULLBRIGHT /* - MODEL_BSP_ENABLE_MINTESSELATION*/;
//spCXR_WorldLightState CXR_Model_BSP4::m_spTempWLS;

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_BSP4, CXR_PhysicsModel);

// -------------------------------------------------------------------
CXR_Model_BSP4::CXR_Model_BSP4()
{
	MAUTOSTRIP(CXR_Model_BSP_ctor, MAUTOSTRIP_VOID);
}

CXR_Model_BSP4::~CXR_Model_BSP4()
{
	MAUTOSTRIP(CXR_Model_BSP_dtor, MAUTOSTRIP_VOID);
}

void CXR_Model_BSP4::PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_PreRender, MAUTOSTRIP_VOID);
}

// -------------------------------------------------------------------
void CXR_Model_BSP4::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS_asdf,
			const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_OnRender, MAUTOSTRIP_VOID);
}

// -------------------------------------------------------------------
void CXR_Model_BSP4::OnPrecache(CXR_Engine* _pEngine, int _iVariation)
{
	MAUTOSTRIP(CXR_Model_BSP_OnPrecache, MAUTOSTRIP_VOID);
}

// -------------------------------------------------------------------

