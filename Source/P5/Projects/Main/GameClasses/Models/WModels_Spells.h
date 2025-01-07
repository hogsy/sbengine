
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

//-------------------------------------------------------------------
// Blizzard
//-------------------------------------------------------------------
class CXR_Model_Blizzard : public CXR_Model_Custom, public CCritters
{
	MRTC_DECLARE;

	enum
	{
		MAXCRITTERS = 50,
	};

	int m_iTexture;
	int m_iIceTexture;

	virtual void OnCreate(const char* _pParam);
	virtual bool RenderIceRain(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat);
	virtual bool RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat);
};

// -------------------------------------------------------------------
//  CXR_Model_Rail
// -------------------------------------------------------------------
class CXR_Model_Rail : public CXR_Model_Custom
{
	MRTC_DECLARE;

	int m_iTexture;

	virtual void OnCreate(const char *);
	virtual bool RenderRings_Old(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat);
	virtual void OnPreRender(const CXR_AnimState *_pAnimState, const CMat43fp32 &_WMat);
	void RenderRings(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat);
	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, const CMat43fp32& _VMat);
};

// -------------------------------------------------------------------
//  CXR_Model_FlameTounge - How do you spell Tounge???
// -------------------------------------------------------------------
class CXR_Model_FlameTounge : public CXR_Model_Custom
{
	MRTC_DECLARE;

	int m_iTexture;

	virtual void OnCreate(const char *);
	virtual bool RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat);
};
;
// -------------------------------------------------------------------
//  CXR_Model_IceTounge - How do you spell Tounge???
// -------------------------------------------------------------------
class CXR_Model_IceTounge : public CXR_Model_Custom
{
	MRTC_DECLARE;

	int m_iTexture;

	virtual void OnCreate(const char *);
	virtual bool RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat);
};
;