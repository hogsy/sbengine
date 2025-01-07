#include "PCH.h"
#include "WModel_Chain.h"
#include "../WObj_Misc/WObj_ChainEffect.h"

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Chain, CXR_Model_Custom);


void CXR_Model_Chain::Create(const char* _pParam)
{
	CXR_Model_Custom::Create(_pParam);
}

void CXR_Model_Chain::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	_Box = CBox3Dfp32(0,0);
	if(_pAnimState && _pAnimState->m_Data[0])
	{
		//const CXR_Rope* Rope = (const CXR_Rope*)&_pAnimState->m_Data[0];
		CXR_Rope& Rope = *(CXR_Rope*)_pAnimState->m_Data[0];
		//CVec3Dfp32* pMassPositions = pRope->GetIPPosition();
		//CVec3Dfp32* pMassPositions = Rope->GetIPPosition();//pMassPositions = Rope.GetAllMassPositions();
		CVec3Dfp32* pMassPositions = Rope.GetIPPosition();
		uint32 nMasses = Rope.GetNumMasses();

		if(nMasses > 0)
		{
			//_Box = CBox3Dfp32(pMassPositions[0], pMassPositions[1]);
			for(int i = 1; i < nMasses; i++)
				_Box.Expand(pMassPositions[i] - pMassPositions[0]);
		}
	}

	_Box.Grow(8);
}

fp32 CXR_Model_Chain::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	fp32 Radius = 16.0f;
	if(_pAnimState && _pAnimState->m_Data[0])
	{
		CXR_Rope& Rope = *(CXR_Rope*)_pAnimState->m_Data[0];
		CVec3Dfp32* pMassPositions = Rope.GetIPPosition();//Rope.GetAllMassPositions();
		uint32 nMasses = Rope.GetNumMasses();

		if(nMasses > 0)
		{
			CBox3Dfp32 Box = CBox3Dfp32(pMassPositions[0], pMassPositions[1]);

			for(int i = 1; i < nMasses; i++)
				Box.Expand(pMassPositions[i]);

			Radius += ((Box.m_Max - Box.m_Min).Length() * 0.5f);
		}
	}

	return Radius;
}

void CXR_Model_Chain::Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
{
	MSCOPESHORT(CXR_Model_Chain::Render);

	if(!_pAnimState || !_pAnimState->m_Data[0] || !_pAnimState->m_Data[1])
		return;

	TThinArray<CXR_BeamStrip> m_lBeamStrip;
	CXR_BeamStrip* pBeamStrip = NULL;
	CXR_Rope& Rope = *(CXR_Rope*)_pAnimState->m_Data[0];
	CWO_ChainEffect_ClientData& RopeCD = *(CWO_ChainEffect_ClientData*)_pAnimState->m_Data[1];
	//const fp32& RopeWidth = *(fp32*)&_pAnimState->m_Data[1];

//	CXR_VBManager* pVBM = _pRenderParams->m_pVBM;

	//uint16 nRefs = Rope.GetNumRefObjs();

    CVec3Dfp32* pMassPositions = Rope.GetIPPosition();//Rope.GetAllMassPositions();
	uint32 nMasses = Rope.GetNumMasses();

	m_lBeamStrip.SetLen(nMasses);
	pBeamStrip = m_lBeamStrip.GetBasePtr();

	// Create a beam strip
	if(nMasses > 1)
	{
		pBeamStrip[0].m_Color = CPixel32(255,255,255,255);
		pBeamStrip[0].m_Flags = 0;
		pBeamStrip[0].m_Pos = pMassPositions[0];
		pBeamStrip[0].m_TextureYOfs = 0;
		pBeamStrip[0].m_Width = RopeCD.m_Width;//RopeWidth;

		for(int i = 1; i < nMasses; i++)
		{
			pBeamStrip[i].m_Color = CPixel32(255,255,255,255);
			pBeamStrip[i].m_Flags = 0;
			pBeamStrip[i].m_Pos = pMassPositions[i];
			pBeamStrip[i].m_TextureYOfs = i;//SomeValue; //=)
			pBeamStrip[i].m_Width = RopeCD.m_Width;//RopeWidth;
		}
	}

	// Prepare beam strip ...
	CXR_VertexBuffer* pVB = _pRenderParams->m_pVBM->Alloc_VB();
	if(!pVB)
		return;

	m_SK.Create(GetSurfaceContext(), _pRenderParams->m_pEngine, _pAnimState, RopeCD.m_SurfaceID);
	
	CMat4Dfp32 Unit; Unit.Unit();
	if(CXR_Util::Render_BeamStrip2(_pRenderParams->m_pVBM, pVB, Unit, _VMat, pBeamStrip, nMasses))
	{
		// ... and render surface
		pVB->m_Priority = (m_SK.m_pSurface->m_Flags & XW_SURFFLAGS_TRANSPARENT) ? _pRenderParams->m_RenderInfo.m_BasePriority_Transparent :
																				 _pRenderParams->m_RenderInfo.m_BasePriority_Opaque;
		m_SK.Render(pVB, _pRenderParams->m_pVBM, _pRenderParams->m_pEngine);
	}
}

