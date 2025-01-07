#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"
#include "../../../../Shared/MOS/XRModels/Model_Flare/WModel_Flare.h"

//----------------------------------------------------------------------
// CXR_Model_AttachFlare
//----------------------------------------------------------------------
/*
class CXR_Model_AttachFlare : public CXR_Model_Flare
{

	MRTC_DECLARE_DYNAMIC;

protected:

	int32		m_iSurface;
	int32		m_iAttachPoint;

public:

private:

	//----------------------------------------------------------------------
	
	virtual void Create(const char *_params)
	{
		CStr params(_params);

		if (_params != NULL)
		{
//			m_iSurface = GetSurfaceID(params.GetStrSep(","));
			m_iAttachPoint = params.GetIntSep(",");
		}
		else
		{
//			m_iSurface = GetSurfaceID("Effects_Flare01");
			m_iAttachPoint = -1;
		}
	}

	//----------------------------------------------------------------------

	virtual void Render(CXR_Engine* _pEngine, CRenderContext* _pRC, CXR_VBManager* _pVBM, 
						CXR_ViewClipInterface* _pViewClip, CXR_WorldLightState* _pWLS, 
						const CXR_AnimState* _pAnimState, 
						const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
	{
		CMat4Dfp32 AttachMat = _WMat;

		if (m_iAttachPoint != -1)
		{
			CVec3Dfp32 AttachOffset;

			// Get attach point offset.

			AttachOffset.AddMatrixRow(AttachMat, 3);
		}

		CXR_Model_Flare::Render(_pEngine, _pRC, _pVBM, _pViewClip, _pWLS, _pAnimState, AttachMat, _VMat, _Flags);

		if (!_pRender) return;
		if (!_pAnimState) return;
		if (_pEngine && !_pEngine->GetVar(XR_ENGINE_FLARES)) return;

		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
		CRC_Attributes* pA = _pVBM->Alloc_Attrib();
		if (!pVB || !pA) return;
		pVB->m_pAttrib = pA;
		pA->SetDefault();
		pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
		pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
		pA->Attrib_TextureID(0, GetTextureID(0));

		CXR_Particle2 Flare;
		Flare.m_Pos = CVec3Dfp32::GetMatrixRow(_WMat, 3);
		Flare.m_Size = fp32(_pAnimState->m_Anim0) * 2.0f;
		Flare.m_Color = _pAnimState->m_Colors[0];

		fp32 Attenuation = _pAnimState->m_Anim1;
		fp32 DepthOffset = _pAnimState->m_Colors[1];

		if (CXR_Util::Render_Flares(_pRender, _pVBM, pVB, _VMat, &Flare, 1, Attenuation, DepthOffset, m_nSample, false))
		{
			pVB->m_Priority = CXR_VBPRIORITY_FLARE;
			_pVBM->AddVB(pVB);
		}
	}
};

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_AttachFlare, CXR_Model_Flare);

//----------------------------------------------------------------------
*/