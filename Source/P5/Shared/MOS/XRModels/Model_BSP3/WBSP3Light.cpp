#include "PCH.h"

#include "MFloat.h"
#include "WBSP3Model.h"
#include "WBSP3Def.h"


// #define MODEL_BSP_NOATTRSHARING

// -------------------------------------------------------------------
//  Stuff in MImage.cpp
// -------------------------------------------------------------------

void PPA_Mul_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);
void PPA_MulAddSaturate_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

void PPA_Mul_RGBA32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);



class CBSP3_VBFacePrimList
{
	enum { e_MaxElem = 512 };

	uint32 m_liVBFacePrims[e_MaxElem];
	int    m_nElem;

public:
	CBSP3_VBFacePrimList()
	{
		Clear();
	}

	void Clear()
	{
		m_nElem = 0;
	}

	bool IsFull() const
	{
		return m_nElem >= e_MaxElem;
	}

	void Add(uint32 _iVBFacePrim)
	{
		if (!IsFull())
			m_liVBFacePrims[m_nElem++] = _iVBFacePrim;
	}

	int Len() const
	{
		return m_nElem;
	}

	uint32 Get(int _iElem) const
	{
		return m_liVBFacePrims[_iElem];
	}
};

static CBSP3_VBFacePrimList ms_VBFacePrimList; // 2 KB of static data


// -------------------------------------------------------------------
//  CXR_Model_BSP3
// -------------------------------------------------------------------
void CXR_Model_BSP3::GetLightVertices(int _iFace, CPixel32* _pLVerts, CVec4Dfp32* _pDynBumpDir)
{
	MAUTOSTRIP(CXR_Model_BSP_GetLightVertices, MAUTOSTRIP_VOID);
	CBSP3_Face* pFace = &m_pFaces[_iFace];
	int nv = pFace->m_nVertices;

//	int LightID = m_pLightVerticesInfo[pFace->m_iLightInfo].m_LightID;
	int nLVerts = pFace->m_nLightInfo;
//	int DynamicLightMask = m_lFaceLightMask[_iFace];

	// Add static light-vertices 'maps'.
	CXR_LightID* pL = NULL; //m_spTempWLS->m_lLightIDs.GetBasePtr();
	if (nLVerts)
	{
		int iLightInfo = pFace->m_iLightInfo;
		int iLVerts = m_pLightVerticesInfo[iLightInfo].m_iLightVertices;
		int LightID = m_pLightVerticesInfo[iLightInfo].m_LightID;
		if (!pL || pL[LightID].m_IntensityInt32 == 0xffffff)
			for(int i = 0; i < nv; i++) _pLVerts[i] = m_pLightVertices[iLVerts+i];
		else
			PPA_Mul_RGB32(pL[LightID].m_IntensityInt32, &m_pLightVertices[iLVerts], _pLVerts, nv);
	}
	for(int i = 1; i < nLVerts; i++)
	{
		int iLightInfo = pFace->m_iLightInfo + i;
		int iLVerts = m_pLightVerticesInfo[iLightInfo].m_iLightVertices;
		int LightID = m_pLightVerticesInfo[iLightInfo].m_LightID;
		PPA_MulAddSaturate_RGB32((pL) ? pL[LightID].m_IntensityInt32 : CPixel32( 0xffffff ), &m_pLightVertices[iLVerts], _pLVerts, nv);
	}
}

// -------------------------------------------------------------------
static const uint8* g_pDynLightPVS = NULL;

void CXR_Model_BSP3::Light_TagDynamics()
{
	MAUTOSTRIP(CXR_Model_BSP_Light_TagDynamics, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::Light_TagDynamics);

	int nDynamic = ms_RenderInfo.m_nLights;
	const CXR_Light* pL = ms_lRenderLight;
	for (int iL = 0; iL < nDynamic; iL++)
	{
		g_pDynLightPVS = SceneGraph_PVSLock(0, pL[iL].GetPosition());

		ms_VBFacePrimList.Clear();

		fp32 Range = pL[iL].m_Range;
		CVec3Dfp32 LPos = pL[iL].GetPosition();

		if (pL[iL].m_Type == CXR_LIGHTTYPE_SPOT)
		{ // Fix pos & range for spotlights.. (TODO: Do light frustum clipping for spotlights?)
			const CVec3Dfp32& LDir = CVec3Dfp32::GetRow(pL[iL].m_Pos, 0);
			Range *= 0.5f;
			LPos += LDir * Range;
		}

		Light_FlagTree_r(1, LPos, Range, 1 << iL);

		if (g_pDynLightPVS)
		{
			SceneGraph_PVSRelease(g_pDynLightPVS);
			g_pDynLightPVS = NULL;
		}

		Light_RenderDynamic(pL[iL]);
	}

	if (nDynamic > 32)
	{
		ConOut(CStrF("§cf80WARNING: Too many dynamic lights %d/%d", nDynamic, 16));
		nDynamic = 32;
	}
}


void CXR_Model_BSP3::Light_RenderDynamic(const CXR_Light& _Light)
{
	MSCOPESHORT(CXR_Model_BSP3::Light_RenderDynamic);
	//NOTE: This function is pretty ugly at the moment, mainly because of non-working
	//      support for multi-channel rendering in the PS2 renderer..
	//
	//
	
	const int nElem = ms_VBFacePrimList.Len();

	CXR_VertexBuffer* pVB = ms_pVBM->Alloc_VBAttrib();
	if(!pVB)
		return;

	CPlane3Dfp32* pTexGenAttr = (CPlane3Dfp32*)ms_pVBM->Alloc_fp32(4*4*2);
	if (!pTexGenAttr)
		return;
	
	CRC_Attributes*pA = pVB->m_pAttrib;

	pA->SetDefault();
	pA->Attrib_TexGenAttr((fp32*)pTexGenAttr);



	#define ATTR1 pA
	#define TEX1 0
	#define TEXATTR1(i) pTexGenAttr[i]
	#define ATTR2 pA
	#define TEX2 1
	#define TEXATTR2(i) pTexGenAttr[2+i]


	pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pA->Attrib_Enable(CRC_FLAGS_CULL);
	pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
#ifndef PLATFORM_PS2
	pA->Attrib_ZCompare(CRC_COMPARE_EQUAL);
#endif

	const CVec3Dfp32& LPos = _Light.GetPosition();

	int nStages = 0;
	const fp32 Scale = 0.5f * _Light.m_RangeInv;

	const bool bPoint = true;
	const bool bSpot = true;
	if (bPoint && _Light.m_Type == CXR_LIGHTTYPE_POINT)
	{
#ifdef PLATFORM_PS2
		ATTR1->m_nChannels = TEX2+1;
		ATTR1->Attrib_TexEnvMode(TEX1, CRC_PS2_TEXENVMODE_REPLACE | CRC_PS2_TEXENVMODE_COLORBUFFER | CRC_PS2_TEXENVMODE_ALPHAMASK | CRC_PS2_TEXENVMODE_STENCILMASK | CRC_PS2_TEXENVMODE);
		ATTR2->Attrib_TexEnvMode(TEX2, CRC_PS2_TEXENVMODE_MULADD_DESTALPHA | CRC_PS2_TEXENVMODE_MODULATE | CRC_PS2_TEXENVMODE_COLORBUFFER | CRC_PS2_TEXENVMODE_RGBMASK | CRC_PS2_TEXENVMODE);
#endif
		nStages = 2;

		// XY Range
		ATTR1->Attrib_TextureID(TEX1, ms_TextureID_AttenuationExp);
		ATTR1->Attrib_TexGen(TEX1, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
		TEXATTR1(0).CreateND(CVec3Dfp32(Scale, 0.0f, 0.0f), -Scale * LPos.k[0] + 0.5f);
		TEXATTR1(1).CreateND(CVec3Dfp32(0.0f, Scale, 0.0f), -Scale * LPos.k[1] + 0.5f);

		// Z Range
		ATTR2->Attrib_TextureID(TEX2, ms_TextureID_AttenuationExp);
		ATTR2->Attrib_TexGen(TEX2, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
		TEXATTR2(0).CreateND(CVec3Dfp32(0.0f, 0.0f, Scale), -Scale * LPos.k[2] + 0.5f);
		TEXATTR2(1).CreateND(CVec3Dfp32(0.0f, 0.0f, 0), 0.5f);
	}
	else if (bSpot && _Light.m_Type == CXR_LIGHTTYPE_SPOT)
	{
//		pA->Attrib_Enable(CRC_FLAGS_COLORWRITE);
//		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);

#ifdef PLATFORM_PS2
		ATTR1->m_nChannels = TEX2+1;
//		ATTR1->Attrib_TexEnvMode(TEX1, CRC_PS2_TEXENVMODE_ADD | CRC_PS2_TEXENVMODE_COLORBUFFER | CRC_PS2_TEXENVMODE_MODULATE | CRC_PS2_TEXENVMODE_RGBMASK | CRC_PS2_TEXENVMODE);
		ATTR1->Attrib_TexEnvMode(TEX1, CRC_PS2_TEXENVMODE_REPLACE | CRC_PS2_TEXENVMODE_COLORBUFFER | CRC_PS2_TEXENVMODE_ALPHAMASK | CRC_PS2_TEXENVMODE_STENCILMASK | CRC_PS2_TEXENVMODE);
		ATTR2->Attrib_TexEnvMode(TEX2, CRC_PS2_TEXENVMODE_MULADD_DESTALPHA | CRC_PS2_TEXENVMODE_MODULATE | CRC_PS2_TEXENVMODE_COLORBUFFER | CRC_PS2_TEXENVMODE_RGBMASK | CRC_PS2_TEXENVMODE);

		//ATTR1->Attrib_PolygonOffset(-10, -130); 
		ATTR2->Attrib_PolygonOffset(-10, -130); // Because of PS2 q-clipping, we need to offset the polys a bit..
#endif
		nStages = 2;

		const CVec3Dfp32& ProjX = CVec3Dfp32::GetRow(_Light.m_Pos, 0);
		CVec3Dfp32 ProjY; CVec3Dfp32::GetRow(_Light.m_Pos, 1).Scale(1.0f / _Light.m_SpotWidth, ProjY);
		CVec3Dfp32 ProjZ; CVec3Dfp32::GetRow(_Light.m_Pos, 2).Scale(1.0f / _Light.m_SpotHeight, ProjZ);

		// LDir Range
		ATTR1->Attrib_TextureID(TEX1, ms_TextureID_AttenuationExp);
		ATTR1->Attrib_TexGen(TEX1, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
		TEXATTR1(0).CreateND(ProjX*Scale, 0.5f - (ProjX * LPos)*Scale);
		TEXATTR1(1).CreateND(CVec3Dfp32(0.0f, 0.0f, 0.0f), 0.5f);

		// Project Texture
		ATTR2->Attrib_TextureID(TEX2, ms_TextureID_DefaultLens);
		ATTR2->Attrib_TexGen(TEX2, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W | CRC_TEXGENCOMP_Q);
		TEXATTR2(0).CreateND((ProjY+ProjX) * 0.5f, -((ProjY+ProjX) * LPos) * 0.5f);
		TEXATTR2(1).CreateND((ProjZ+ProjX) * 0.5f, -((ProjZ+ProjX) * LPos) * 0.5f);
		TEXATTR2(2).CreateND(CVec3Dfp32(0.0f, 0.0f, 0.0f), 1.0f);
		TEXATTR2(3).CreateND(ProjX, -(ProjX * LPos));
	}

#ifdef	PLATFORM_PS2
#define	LIGHT_SCALE 64.0f
#else
#define	LIGHT_SCALE 255.0f
#endif
	CVec4Dfp32 LightColor = _Light.GetIntensityv();
	CPixel32 VBColor(	(int)(LightColor[0] * LIGHT_SCALE),
						(int)(LightColor[1] * LIGHT_SCALE),
						(int)(LightColor[2] * LIGHT_SCALE),
						255 );

	for( int i = 0; i < nElem; i++ )
	{
		uint32 iVBFP = ms_VBFacePrimList.Get(i);
		const CBSP3_VBFacePrim* pVBFP = &m_lVBFacePrim[iVBFP];

		CXR_VBIDChain* pChain = ms_pVBM->Alloc_VBIDChain();
		if (!pChain)
			return;

		pChain->Render_VertexBuffer(pVBFP->m_VBID);

		if (pVB->m_pVBChain)
			pChain->m_pNextVB = pVB->GetVBIDChain();

		pVB->SetVBIDChain(pChain);
	}

	pVB->m_Priority = CXR_VBPRIORITY_DYNLIGHT + _Light.m_iLight*0.01f;
	pVB->m_Color = VBColor;
	pVB->Matrix_Set(ms_pVBMatrixM2V);
	
	ms_pVBM->AddVB(pVB);

}

void CXR_Model_BSP3::Light_UnTagDynamics()
{
	MAUTOSTRIP(CXR_Model_BSP_Light_UnTagDynamics, MAUTOSTRIP_VOID);
	// Init masklist
	{

		if (m_nTagFaces >= m_lLightElemTagList.Len() && m_lLightElemTagList.Len() != m_lFaces.Len())
			ConOut(CStrF("Reached maximum dynamically lit elements (%d)", m_nTagFaces));

		uint16* pFLM = &m_lFaceLightMask[0];
		uint32* pFTL = m_lLightElemTagList.GetBasePtr();
		for(int i = 0; i < m_nTagFaces; i++)
		{
			int Elem = pFTL[i];
			switch(Elem >> 24)
			{
			case 0 :
				{
					int iFace = pFTL[i] & 0xffffff;
					pFLM[iFace] = 0;
					break;
				}
			case 1 :
				{
/*
					int iSB = pFTL[i] & 0xffffff;
					CSplineBrush* pSB = m_lspSplineBrushes[iSB];
					pSB->m_DynLightMask = 0;
					for(int i = 0; i < pSB->m_lFaces.Len(); i++)
						pSB->m_lFaces[i].m_DynLightMask = 0;
*/
					break;
				}
			}
		}
		m_nTagFaces = 0;
	}
	
	{
		int nDyn = ms_RenderInfo.m_nLights;
		for(int i = 0; i < nDyn; i++)
		{
			ms_Render_lpAttribDynLight[i] = NULL;
			ms_Render_lpAttribDynLight_DestAlphaExpAttn[i] = NULL;
		}
	}
}

void CXR_Model_BSP3::Light_FlagSplineBrush(int _iSB, const CVec3Dfp32& _Pos, fp32 _Range, int _Mask)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_FlagSplineBrush, MAUTOSTRIP_VOID);
}

bool CXR_Model_BSP3::Light_IntersectSphere_CheckFace(int _iFace, const CVec3Dfp32& _Pos, fp32 _Radius)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_IntersectSphere_CheckFace, false);
	// NOTE: This function doesn't guarantee that there was a hit, only that there was NOT a hit.
	const CBSP3_Face* pF = &m_pFaces[_iFace];

	int iPlane = pF->m_iPlane;
	const CPlane3Dfp32* pP = &m_pPlanes[iPlane];
/*	fp32 d = pP->Distance(_Pos);
	if (d < -_Radius) return false;
	if (d > _Radius) return false;
*/
	CPlane3Dfp32 Edge_Plane;

	fp32 RSqr = Sqr(_Radius);

	int nv = pF->m_nPhysV;
	int iiv = pF->m_iiPhysV;

	int iv1 = m_piVertices[iiv + nv - 1];
	int iv0 = 0;
	for(int v = 0; v < nv; v++, iv1 = iv0)
	{
		iv0 = m_piVertices[iiv + v];
		CVec3Dfp32 ve, n;
		m_pVertices[iv1].Sub(m_pVertices[iv0], ve);
		ve.CrossProd(pP->n, Edge_Plane.n);
		Edge_Plane.d = -Edge_Plane.n*m_pVertices[iv0];

		fp32 d = Edge_Plane.Distance(_Pos);
		if (d > 0.0f)
		{
			fp32 NSqrLen = Edge_Plane.n.LengthSqr();
			if (Sqr(d) > RSqr*NSqrLen) return false;
		}

	}

	return true;
}

void CXR_Model_BSP3::Light_FlagTree_r(int _iNode, const CVec3Dfp32& _Pos, fp32 _Range, int _Mask)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_FlagTree_r, MAUTOSTRIP_VOID);
//	int bLMExists = (m_spLMTC != NULL);

	const CBSP3_Node* pN = &m_pNodes[_iNode];
	if (pN->IsStructureLeaf())
	{
		int iPL = pN->m_iPortalLeaf;
		if (!(m_pView->m_liLeafRPortals[iPL]))
			return;

		if (g_pDynLightPVS && !(g_pDynLightPVS[iPL >> 3] & (1 << (iPL & 7))))
			return;

		CBSP3_PortalLeafExt* pPL = &m_lPortalLeaves[pN->m_iPortalLeaf];

		int nVBFP = pPL->m_nVBFacePrims;
		if (nVBFP)
		{
			int iVBFPBase = pPL->m_iVBFacePrims;
			const CBSP3_VBFacePrim* pVBFP = &m_lVBFacePrim[iVBFPBase];
			const fp32 SqrRange = Sqr(_Range);
			for (int i = 0; i < nVBFP; i++)
			{
				fp32 Dist2 = pVBFP->m_BoundBox.GetMinSqrDistance(_Pos);
				if (Dist2 < SqrRange)
				{
					int iVBFP = iVBFPBase + i;
					ms_VBFacePrimList.Add(iVBFP);
				}
				pVBFP++;
			}
		}

		return;
	}

	if (pN->IsNode())
	{
		int iPlane = pN->m_iPlane;
		fp32 Dist = m_pPlanes[iPlane].Distance(_Pos);
		if (Dist > -_Range)
		{
			if (Dist < _Range)
			{
				if (Dist > 0.0f)
				{
					Light_FlagTree_r(pN->m_iNodeFront, _Pos, _Range, _Mask);
					if (ms_VBFacePrimList.IsFull()) return; //if (m_nTagFaces >= m_lLightElemTagList.Len()) return;
					Light_FlagTree_r(pN->m_iNodeBack, _Pos, _Range, _Mask);
				}
				else
				{
					Light_FlagTree_r(pN->m_iNodeBack, _Pos, _Range, _Mask);
					if (ms_VBFacePrimList.IsFull()) return; //if (m_nTagFaces >= m_lLightElemTagList.Len()) return;
					Light_FlagTree_r(pN->m_iNodeFront, _Pos, _Range, _Mask);
				}
			}
			else
			{
				Light_FlagTree_r(pN->m_iNodeFront, _Pos, _Range, _Mask);
			}
		}
		else
		{
			if (Dist < _Range) 
			{
				Light_FlagTree_r(pN->m_iNodeBack, _Pos, _Range, _Mask);
			}
		}
	}
}


#include "../../XR/XRShader.h"



#define SLLog(s)

#define MODEL_BSP3_MAX_LIT_FACES 2048

#define MODEL_BSP3_MAXDYNAMICLIGHTFACES 1024
#define MODEL_BSP3_MAX_LIT_EDGES 4096


void CBSP3_SpotLightClip::CalcSpotVerticesAndPlanes(const CXR_Light* _pL, CVec3Dfp32* _pV, CPlane3Dfp32* _pP)
{
	fp32 Width = _pL->m_SpotWidth * _pL->m_Range;
	fp32 Height = _pL->m_SpotHeight * _pL->m_Range;
	CVec3Dfp32 Fwd;
	_pL->GetDirection().Scale(_pL->m_Range, Fwd);

	_pL->GetPosition().Combine(CVec3Dfp32::GetRow(_pL->m_Pos, 1), Width, _pV[1]);
	_pV[1].Combine(CVec3Dfp32::GetRow(_pL->m_Pos, 2), Height, _pV[1]);
	_pV[1] += Fwd;

	_pL->GetPosition().Combine(CVec3Dfp32::GetRow(_pL->m_Pos, 1), -Width, _pV[2]);
	_pV[2].Combine(CVec3Dfp32::GetRow(_pL->m_Pos, 2), Height, _pV[2]);
	_pV[2] += Fwd;

	_pL->GetPosition().Combine(CVec3Dfp32::GetRow(_pL->m_Pos, 1), -Width, _pV[3]);
	_pV[3].Combine(CVec3Dfp32::GetRow(_pL->m_Pos, 2), -Height, _pV[3]);
	_pV[3] += Fwd;

	_pL->GetPosition().Combine(CVec3Dfp32::GetRow(_pL->m_Pos, 1), Width, _pV[4]);
	_pV[4].Combine(CVec3Dfp32::GetRow(_pL->m_Pos, 2), -Height, _pV[4]);
	_pV[4] += Fwd;

	_pV[0] = _pL->GetPosition();

	_pP[0].Create(_pV[1], _pV[0], _pV[2]);
	_pP[1].Create(_pV[2], _pV[0], _pV[3]);
	_pP[2].Create(_pV[3], _pV[0], _pV[4]);
	_pP[3].Create(_pV[4], _pV[0], _pV[1]);
	_pP[4].Create(_pV[1], _pV[2], _pV[3]);
}

void CBSP3_SpotLightClip::Create(const CXR_Light* _pL)
{
	CalcSpotVerticesAndPlanes(_pL, m_lV, m_lPlanes);
}

int CBSP3_SpotLightClip::IntersectBox(const CBox3Dfp32& _Box) const
{
	for(int p = 0; p < 5; p++)
		if (m_lPlanes[p].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) > 0.0f)
			return false;

	return true;
}

void CBSP3_SpotLightClip::GetBoundBox(CBox3Dfp32& _Box) const
{
	CVec3Dfp32::GetMinBoundBox(m_lV, _Box.m_Min, _Box.m_Max, 5);
}


void VBM_Get2DMatrix(CRC_Viewport* _pVB, CMat4Dfp32& _Mat);
void VBM_RenderRect(CRC_Viewport* _pVP, CXR_VBManager* _pVBM, const CRect2Duint16& _Rect, CPixel32 _Color, fp32 _Priority, CRC_Attributes* _pA);
void VBM_RenderRect_AlphaBlend(CRC_Viewport* _pVP, CXR_VBManager* _pVBM, const CRect2Duint16& _Rect, CPixel32 _Color, fp32 _Priority);

