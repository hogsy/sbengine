#ifndef _INC_WModel_Flare
#define _INC_WModel_Flare

#include "../../XR/XR.h"
#include "../../XR/XREngineVar.h"
// -------------------------------------------------------------------
//  CXR_Model_Flare
// -------------------------------------------------------------------
class CXR_Model_Flare : public CXR_Model
{
	MRTC_DECLARE;

	TArray<int> m_lFlareTxtID;
	CVec2Dfp32 m_TVerts[4];
	uint16 m_iPrim[7];
	int m_nSample;
/*	fp32 m_DefSize;
	fp32 m_Fade;*/

public:
	CXR_Model_Flare();
	virtual void Create(const char* _pParam);
	int GetTextureID(int _iFlare);

	static void VBPreRender(CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext,CXR_VBMScope* _pScope,int _Flags);


	// Bounding volumes in model-space
	virtual int GetRenderPass(const CXR_AnimState* _pAnimState) { return 2; };
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = NULL);

	// Render
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
	virtual void OnPrecache(CXR_Engine* _pEngine, int _iVariation);

	MACRO_OPERATOR_TPTR(CXR_Model_Flare);
};

// -------------------------------------------------------------------
//  CXR_Model_Sprite
// -------------------------------------------------------------------
class CXR_Model_Sprite : public CXR_Model
{
	MRTC_DECLARE;

protected:
	CXW_SurfaceKeyFrame m_TmpSurfKey;
	int m_iDefSurface;
	fp32 m_DefSize;

public:
	CXR_Model_Sprite();
	void Create(const char* _pParam);
	fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState);
	void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState);
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
};


// -------------------------------------------------------------------
//  CXR_Model_SphereSprite
// -------------------------------------------------------------------
class CXR_Model_SphereSprite : public CXR_Model_Sprite
{
	MRTC_DECLARE;
protected:
public:
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
};


// -------------------------------------------------------------------
//  CXR_Model_ConcaveSprite
// -------------------------------------------------------------------
class CXR_Model_ConcaveSprite : public CXR_Model_Sprite
{
	MRTC_DECLARE;
protected:
public:
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
};

// -------------------------------------------------------------------
//  CXR_Model_SpotLightVolume
// -------------------------------------------------------------------
class CXR_Model_SpotLightVolume : public CXR_Model
{
	MRTC_DECLARE;
protected:
	CXW_SurfaceKeyFrame m_TmpSurfKey;
	int m_iDefSurface;
	int m_iFlareTexture;
	fp32 m_DefSize;

public:
	void Create(const char* _pParam);
	virtual void OnPrecache(CXR_Engine* _pEngine, int _iVariation);
	fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState);
	void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState);
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
};


#endif
