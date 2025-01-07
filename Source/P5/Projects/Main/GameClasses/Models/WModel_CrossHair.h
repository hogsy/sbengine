#ifndef _INC_WModel_CrossHair
#define _INC_WModel_CrossHair

#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XREngineVar.h"

// -------------------------------------------------------------------
//  CXR_Model_Sprite
// -------------------------------------------------------------------
class CXR_Model_CrossHair : public CXR_Model
{
	MRTC_DECLARE;
protected:
	int m_iDefSurface;
	fp32 m_DefSize;

public:
	void Create(const char* _pParam);
	fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState);
	void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState);
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, const CMat43fp32& _VMat, int _Flags);
};

#endif