#ifndef __OGRRENDER_H
#define __OGRRENDER_H

#ifdef PLATFORM_CONSOLE
#error "This file should not be included on a console"
#endif

#include "../../XR/XR.h"
#include "../../XR/XRShader.h"

enum
{
	OGIER_RENDER_CULL = 1,
	OGIER_RENDER_TEXTURES = 2,
	OGIER_RENDER_SHADE = 4,
	OGIER_RENDER_WIRE = 8,
	OGIER_RENDER_SKIPSELECTION = 16,
	OGIER_RENDER_3DVIEW = 32,
	OGIER_RENDER_SIMULATE = 64,
	OGIER_RENDER_SKIPSELECTIONCOLOR = 128,
	OGIER_RENDER_SKIPSOLIDS = 256,
	OGIER_RENDER_PHYSICS = 1 << 9,
	OGIER_RENDER_LIT = 1 << 10,
	OGIER_RENDER_PERPIXEL = 1 << 11,
	OGIER_RENDER_OVERBRIGHT = 1 << 12,
	OGIER_RENDER_LIGHTMAP = 1 << 13,
	OGIER_RENDER_IGNOREINVISIBLE = 1 << 14,

	OGIER_RENDER_ABSTRACT_SHIFT = 15,
	OGIER_RENDER_ABSTRACT_TYPE0 = 1 << OGIER_RENDER_ABSTRACT_SHIFT,
	OGIER_RENDER_ABSTRACT_TYPE1 = 1 << (OGIER_RENDER_ABSTRACT_SHIFT + 1),
	OGIER_RENDER_ABSTRACT_MASK = OGIER_RENDER_ABSTRACT_TYPE0 | OGIER_RENDER_ABSTRACT_TYPE1,

	OGIER_RENDER_NOTRANSFORM = 1 << 17,	//For creating HW buffers - ae
	OGIER_RENDER_NOCULL = 1 << 18,
	
	OGIER_MAPPINGTYPE_STMAPPED = 0,
	OGIER_MAPPINGTYPE_FIXEDSTMAPPED,
	OGIER_MAPPINGTYPE_BOXMAPPED,
	OGIER_MAPPINGTYPE_PLANEMAPPED,
};

class COgrRenderBuffer : public CXR_Model
{
public:
	class CPendingVB
	{
	public:
		class CXR_VertexBuffer *m_pVB;
		class CXW_Surface *m_pSurf;
		int m_Flags;
	};

	TArray<CPendingVB> m_lPendingVB;
	TArray<CPendingVB> m_lPendingTransVB;
	CXR_SurfaceContext *m_pSurfaceContext;
	
	CXW_SurfaceKeyFrame m_TmpSurfKeyFrame;

	void Create()
	{
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSurfaceContext, "SYSTEM.SURFACECONTEXT");
		m_pSurfaceContext = pSurfaceContext;
	}

	void Clear()
	{
		m_lPendingVB.QuickSetLen(0);
		m_lPendingTransVB.QuickSetLen(0);
	}

	void OgrAddVB(CRenderContext* _pRC, CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, bool _bTransparent,
				const CVec3Dfp32 &_BoundPos, const CVec3Dfp32 &_LocalPos, CMTime _AnimTime,
				CXW_Surface *_pSurf = NULL)
	{
		MSCOPE(COgrRenderBuffer::AddVB1, OGIER);
		if(_pSurf)
		{
			CXW_SurfaceKeyFrame* pSKF = _pSurf->GetFrame(0, _AnimTime, &m_TmpSurfKeyFrame);

			if(_bTransparent || _pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT)
			{
				fp32 Dist = (_BoundPos - _LocalPos).Length();
				_pVB->m_Priority = CXR_VBPRIORITY_MODEL_TRANSPARENT + (1.0f - Dist / _pRC->Viewport_Get()->GetBackPlane());
				OgrAddTransVB(_pVB, _pSurf);
			}
			else
			{
				_pVB->m_Priority = CXR_VBPRIORITY_MODEL_OPAQUE;
				OgrAddVB(_pVB, _pSurf);
			}
		}
		else
		{
			if(_bTransparent)
			{
				fp32 Dist = (_BoundPos - _LocalPos).Length();
				_pVB->m_Priority = CXR_VBPRIORITY_MODEL_TRANSPARENT + (1.0f - Dist / _pRC->Viewport_Get()->GetBackPlane());
				OgrAddTransVB(_pVB, NULL);
			}
			else
			{
				_pVB->m_Priority = CXR_VBPRIORITY_MODEL_OPAQUE;
				OgrAddVB(_pVB, NULL);
			}
		}
	}

	void OgrAddVB(CXR_VertexBuffer *_pVB, CXW_Surface *_pSurf)
	{
		MSCOPE(COgrRenderBuffer::AddVB2, OGIER);
		for(int i = 0; i < m_lPendingVB.Len(); i++)
		{
			CPendingVB *pPVB = &m_lPendingVB[i];
			if(_pSurf == pPVB->m_pSurf && !((_pVB->m_Flags ^ pPVB->m_pVB->m_Flags) & CXR_VBFLAGS_OGR_SELECTED))
			{
				_pVB->GetVBChain()->m_pNextVB = pPVB->m_pVB->GetVBChain();
				pPVB->m_pVB = _pVB;
				return;
			}
		}

		CPendingVB PVB;
		PVB.m_pVB = _pVB;
		PVB.m_pSurf = _pSurf;
		m_lPendingVB.Add(PVB);
	}

	void OgrAddTransVB(CXR_VertexBuffer *_pVB, CXW_Surface *_pSurf)
	{
		MSCOPE(COgrRenderBuffer::AddTransVB, OGIER);
		CPendingVB PVB;
		PVB.m_pVB = _pVB;
		PVB.m_pSurf = _pSurf;
		m_lPendingTransVB.Add(PVB);
	}
	
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
	{
		_Box.m_Min = -1000000000;
		_Box.m_Max = 1000000000;
	}

	static bool ShouldShade(CXW_Surface *_pSurf, int _iType)
	{
		int bShade = _iType & OGIER_RENDER_SHADE;
		int bPerPixel = _iType & OGIER_RENDER_PERPIXEL;
		if(!bShade && !bPerPixel)
			return false;
		if(!_pSurf)
			return true;
		CXW_SurfaceKeyFrame *pSKF = _pSurf->GetBaseFrame();
		if(!pSKF || pSKF->m_lTextures.Len() == 0)
			return true;
		int bLighting = _pSurf->m_Flags & XW_SURFFLAGS_LIGHTING;
		if(bShade)
			return bLighting != 0;
		CXW_SurfaceLayer *pLayer = &pSKF->m_lTextures[0];
		int bRS = pLayer->m_Type == XW_LAYERTYPE_RENDERSURFACE;
		if(bPerPixel && bLighting && bRS)
			return true;
		return false;
	}

	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Type = 0);
};

#endif
