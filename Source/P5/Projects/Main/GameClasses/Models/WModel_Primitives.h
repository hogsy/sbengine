
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

class CXR_Model_PhysicsPrim : public CXR_Model_Custom
{
	MRTC_DECLARE;
	
public:
	CXR_Model_PhysicsPrim();
	virtual void OnCreate(const char* _pParam);
	void RenderPrimitive(CXR_Model_Custom_RenderParams* _pRenderParams, const CWO_PhysicsPrim &_Prim, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);
	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);
};

// -------------------------------------------------------------------
//  CXR_Model_Line
// -------------------------------------------------------------------
class CXR_Model_Line : public CXR_Model
{
	MRTC_DECLARE;
	
public:
	CXR_Model_Line();
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = NULL);
	
	// Render
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
	
	MACRO_OPERATOR_TPTR(CXR_Model_Line);
};
