#include "PCH.h"

#include "MFloat.h"
#include "WBSPModel.h"
#include "WBSPDef.h"


// #define MODEL_BSP_NOATTRSHARING

// -------------------------------------------------------------------
//  Stuff in MImage.cpp
// -------------------------------------------------------------------

void PPA_Mul_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);
void PPA_MulAddSaturate_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

void PPA_Mul_RGBA32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

// -------------------------------------------------------------------
//  CXR_Model_BSP
// -------------------------------------------------------------------
int CXR_Model_BSP::Light_TraceLine(int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_TraceLine, 0);
	if (_iNode == 0) return FALSE;
	CBSP_Node* pN = &m_lNodes[_iNode];
	int iPlane = pN->m_iPlane;
	if (!iPlane) return TRUE;

	int iFront = pN->m_iNodeFront;
	int iBack = pN->m_iNodeBack;

	CPlane3Dfp32* pP = &m_pPlanes[iPlane];
	int Side1, Side2;

	fp32 d1 = pP->Distance(_p0);
	fp32 d2 = pP->Distance(_p1);

	// Mask1
	if (d1 < -0.01f)
		Side1 = 2;
	else if (d1 > 0.01f)
		Side1 = 1;
	else
		Side1 = 4;

	// Mask2
	if (d2 < -0.01f)
		Side2 = 2;
	else if (d2 > 0.01f)
		Side2 = 1;
	else
		Side2 = 4;

	if ((Side1 | Side2) == 4)
		return Light_TraceLine(iFront, _p0, _p1) || Light_TraceLine(iBack, _p0, _p1);


	if (((Side1 | Side2) & 3) == 3)
	{
		if (!(iFront || iBack)) return FALSE;

		fp32 s = d2 - d1;
		if (s == 0.0f) return FALSE;
		fp32 vscale = -d1/s;

		CVec3Dfp32 v, CutPoint;
		_p1.Sub(_p0, v);
		_p0.Combine(v, vscale, CutPoint);
//		CVec3Dfp32 v(_p1.k[0] - _p0.k[0], _p1.k[1] - _p0.k[1], _p1.k[2] - _p0.k[2]);
//		CVec3Dfp32 CutPoint(_p0.k[0] + v.k[0]*vscale, _p0.k[1] + v.k[1]*vscale, _p0.k[2] + v.k[2]*vscale);

		if (Side1 & 2)
		{
			if (Light_TraceLine(iBack, _p0, CutPoint))
				return Light_TraceLine(iFront, CutPoint, _p1);
			else
				return FALSE;
		}
		else
		{
			if (Light_TraceLine(iFront, _p0, CutPoint))
				return Light_TraceLine(iBack, CutPoint, _p1);
			else
				return FALSE;
		}
	}
	else
		if ((Side1 & 1) || (Side2 & 1))
			return Light_TraceLine(iFront, _p0, _p1);
		else
			return Light_TraceLine(iBack, _p0, _p1);
}

// -------------------------------------------------------------------
int CXR_Model_BSP::Light_TraceLine_Bounded(int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_TraceLine_Bounded, 0);
	if (_iNode == 0) return FALSE;
	CBSP_Node* pN = &m_lNodes[_iNode];
	int iPlane = pN->m_iPlane;
	if (!iPlane)
	{
/*		int Medium = m_lMediums[pN->m_iMedium].m_MediumFlags;
		if ( ((Medium & XW_MEDIUM_TYPEMASK) > XW_MEDIUM_SOLID) && 
			 ((Medium & XW_MEDIUM_TYPEMASK) < XW_MEDIUM_AIR))
		{
			fp32 l = Length3((_p1.k[0] - _p0.k[0]), (_p1.k[1] - _p0.k[1]), (_p1.k[2] - _p0.k[2]));
			fp32 w = Min(1.0f, 16.0f / Max(1.0f, l));
			fp32 wi = 1.0f - w;
		}
*/		return TRUE;
	}

	int iFront = pN->m_Bound.m_iBoundNodeFront;
	int iBack = pN->m_Bound.m_iBoundNodeBack;

	CPlane3Dfp32* pP = &m_pPlanes[iPlane];
	int Side1, Side2;

	fp32 d1 = pP->Distance(_p0);
	fp32 d2 = pP->Distance(_p1);

	// Mask1
	if (d1 < -0.01f)
		Side1 = 2;
	else if (d1 > 0.01f)
		Side1 = 1;
	else
		Side1 = 4;

	// Mask2
	if (d2 < -0.01f)
		Side2 = 2;
	else if (d2 > 0.01f)
		Side2 = 1;
	else
		Side2 = 4;

	if ((Side1 | Side2) == 4)
		return Light_TraceLine_Bounded(iFront, _p0, _p1) || Light_TraceLine_Bounded(iBack, _p0, _p1);


	if (((Side1 | Side2) & 3) == 3)
	{
		if (!(iFront || iBack)) return FALSE;

		fp32 s = d2 - d1;
		if (s == 0.0f) return FALSE;
		fp32 vscale = -d1/s;

		CVec3Dfp32 v, CutPoint;
		_p1.Sub(_p0, v);
		_p0.Combine(v, vscale, CutPoint);
//		CVec3Dfp32 v(_p1.k[0] - _p0.k[0], _p1.k[1] - _p0.k[1], _p1.k[2] - _p0.k[2]);
//		CVec3Dfp32 CutPoint(_p0.k[0] + v.k[0]*vscale, _p0.k[1] + v.k[1]*vscale, _p0.k[2] + v.k[2]*vscale);

		if (Side1 & 2)
		{
			if (Light_TraceLine_Bounded(iBack, _p0, CutPoint))
				return Light_TraceLine_Bounded(iFront, CutPoint, _p1);
			else
				return FALSE;
		}
		else
		{
			if (Light_TraceLine_Bounded(iFront, _p0, CutPoint))
				return Light_TraceLine_Bounded(iBack, CutPoint, _p1);
			else
				return FALSE;
		}
	}
	else
		if ((Side1 & 1) || (Side2 & 1))
			return Light_TraceLine_Bounded(iFront, _p0, _p1);
		else
			return Light_TraceLine_Bounded(iBack, _p0, _p1);
}

// -------------------------------------------------------------------
void CXR_Model_BSP::GetLightVertices(int _iFace, CPixel32* _pLVerts, CVec4Dfp32* _pDynBumpDir)
{
	MAUTOSTRIP(CXR_Model_BSP_GetLightVertices, MAUTOSTRIP_VOID);
	CBSP_Face* pFace = &m_pFaces[_iFace];
	int nv = pFace->m_nVertices;

//	int LightID = m_pLightVerticesInfo[pFace->m_iLightInfo].m_LightID;
	int nLVerts = pFace->m_nLightInfo;
//	int DynamicLightMask = m_lFaceLightMask[_iFace];

	// Add static light-vertices 'maps'.
	CXR_LightID* pL = m_spTempWLS->m_lLightIDs.GetBasePtr();
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
void CXR_Model_BSP::Light_TagDynamics()
{
	MAUTOSTRIP(CXR_Model_BSP_Light_TagDynamics, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP::Light_TagDynamics);

	int nDynamic = m_spTempWLS->m_nDynamic;

	CXR_Light* pL = m_spTempWLS->m_lDynamic.GetBasePtr();
	int iL;
	for(iL = 0; iL < nDynamic; iL++)
	{
		Light_FlagTree(1, pL[iL].GetPosition(), pL[iL].m_Range, 1 << iL);

		if (!m_spSBLink)
		{
			for(int iSB = 0; iSB < m_lspSplineBrushes.Len(); iSB++)
				Light_FlagSplineBrush(iSB, pL[iL].GetPosition(), pL[iL].m_Range, 1 << iL);
		}
	}

	if (nDynamic > 32)
	{
		ConOut(CStrF("§cf80WARNING: Too many dynamic lights %d/%d", nDynamic, 16));
		nDynamic = 32;
	}
}

void CXR_Model_BSP::Light_UnTagDynamics()
{
	MAUTOSTRIP(CXR_Model_BSP_Light_UnTagDynamics, MAUTOSTRIP_VOID);
	// Init masklist
	{

		if (m_nTagFaces >= m_lLightElemTagList.Len() && m_lLightElemTagList.Len() != m_lFaces.Len())
			ConOut(CStrF("Reached maximum dynamically lit elements (%d)", m_nTagFaces));

		uint32* pFLM = &m_lFaceLightMask[0];
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
					int iSB = pFTL[i] & 0xffffff;
					CSplineBrush* pSB = m_lspSplineBrushes[iSB];
					pSB->m_DynLightMask = 0;
					for(int i = 0; i < pSB->m_lFaces.Len(); i++)
						pSB->m_lFaces[i].m_DynLightMask = 0;
					break;
				}
			}
		}
		m_nTagFaces = 0;
	}
	
	{
		int nDyn = m_spTempWLS->m_nDynamic;
		for(int i = 0; i < nDyn; i++)
		{
			ms_Render_lpAttribDynLight[i] = NULL;
			ms_Render_lpAttribDynLight_DestAlphaExpAttn[i] = NULL;
		}
	}
}

void CXR_Model_BSP::Light_FlagSplineBrush(int _iSB, const CVec3Dfp32& _Pos, fp32 _Range, int _Mask)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_FlagSplineBrush, MAUTOSTRIP_VOID);
	CSplineBrush* pSB = m_lspSplineBrushes[_iSB];
	if (pSB->m_DynLightMask & _Mask) return;
	if (m_nTagFaces >= m_lLightElemTagList.Len()) return;

	if (pSB->m_BoundBox.GetMinSqrDistance(_Pos) < Sqr(_Range))
	{
		int nLit = 0;
		for(int f = 0; f < pSB->m_lFaces.Len(); f++)
			if (pSB->m_lFaces[f].m_BoundBox.GetMinSqrDistance(_Pos) < Sqr(_Range))
			{
				if (m_lspSurfaces[pSB->m_lFaces[f].m_iSurface]->m_Flags & XW_SURFFLAGS_NODYNAMICLIGHT)
					continue;

				if (ms_bCurrentDynLightCullBackFace && !pSB->Face_IsVisible(f, _Pos))
					continue;

				pSB->m_lFaces[f].m_DynLightMask |= _Mask;
				nLit++;
			}

		if(nLit)
		{
			pSB->m_DynLightMask |= _Mask;
			m_lLightElemTagList[m_nTagFaces++] = _iSB + (1 << 24);
		}
	}
}

bool CXR_Model_BSP::Light_IntersectSphere_CheckFace(int _iFace, const CVec3Dfp32& _Pos, fp32 _Radius)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_IntersectSphere_CheckFace, false);
	// NOTE: This function doesn't guarantee that there was a hit, only that there was NOT a hit.
	const CBSP_Face* pF = &m_pFaces[_iFace];

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

void CXR_Model_BSP::Light_TagSplineBrushes(int _iPL, const CVec3Dfp32& _Pos, fp32 _Range, int _Mask)
{
	uint16 liSB[512];
	int nSB = m_spSBLink->EnumIDs(_iPL, liSB, 512, NULL, 0);

	for(int iiSB = 0; iiSB < nSB; iiSB++)
		Light_FlagSplineBrush(liSB[iiSB], _Pos, _Range, _Mask);
}

void CXR_Model_BSP::Light_FlagTree(int _iNode, const CVec3Dfp32& _Pos, fp32 _Range, int _Mask)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_FlagTree, MAUTOSTRIP_VOID);
//	ConOut("Light_FlagTree");
//	return;
//	int bLMExists = (m_spLMTC != NULL);

	const CBSP_Node* pN = &m_pNodes[_iNode];
	if (pN->IsStructureLeaf())
	{
		if (!(m_pView->m_liLeafRPortals[pN->m_iPortalLeaf])) return;

		// Check if this PL has a fog-box and invalidate it.
		CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[pN->m_iPortalLeaf];
		if (pPL->m_iFogBox)
		{
			m_lspFogBoxes[pPL->m_iFogBox]->m_Flags &= ~1;
			m_lspFogBoxes[pPL->m_iFogBox]->m_UpdateCount = 2;
		}

		// Tag splinebrushes
		if (m_spSBLink != NULL)
			Light_TagSplineBrushes(pN->m_iPortalLeaf, _Pos, _Range, _Mask);

		pPL->m_DynLightMask = ~0;
	}

	if (m_pNodes[_iNode].IsLeaf())
	{
		int nFaces = m_pNodes[_iNode].m_nFaces;
		int iiFaces = m_pNodes[_iNode].m_iiFaces;

//	ConOut(CStrF("Light_FlagTree, Leaf  nFaces %d, iiFaces %d", nFaces, iiFaces));
		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_piFaces[iiFaces + f];
			CBSP_Face* pF = &m_pFaces[iFace];
			int Flags = pF->m_Flags;
			if (!(Flags & (XW_FACE_LIGHTVERTICES | XW_FACE_LIGHTMAP)))
				continue;

			int iSurf = pF->m_iSurface;
			if (m_lspSurfaces[iSurf]->m_Flags & XW_SURFFLAGS_NODYNAMICLIGHT)
				continue;

			fp32 Dist = m_pPlanes[pF->m_iPlane].Distance(_Pos);
			if (M_Fabs(Dist) > _Range)
				continue;

			if (ms_bCurrentDynLightCullBackFace && (Dist < 0.0f))
				continue;

			if (Light_IntersectSphere_CheckFace(iFace, _Pos, _Range))
			{
				if (m_nTagFaces >= m_lLightElemTagList.Len()) break;

				// If this is the first tag, add it to list and make the LM dirty.
				if (!m_lFaceLightMask[iFace])
				{
					m_lLightElemTagList[m_nTagFaces++] = iFace;

		/*			if ((Flags & XW_FACE_LIGHTMAP) && (bLMExists))
					{
						int TxtID = m_spLMTC->GetFaceLMCTextureID(iFace);
						m_pCurrentRC->Texture_GetTC()->MakeDirty(TxtID);
					}*/
				}
				m_lFaceLightMask[iFace] |= _Mask;
			}
		}
	}
	else
	{
		int iPlane = m_pNodes[_iNode].m_iPlane;
		fp32 Dist = m_pPlanes[iPlane].Distance(_Pos);
		if (Dist > -_Range)
		{
			if (Dist < _Range)
			{
				if (Dist > 0.0f)
				{
					Light_FlagTree(m_pNodes[_iNode].m_iNodeFront, _Pos, _Range, _Mask);
					if (m_nTagFaces >= m_lLightElemTagList.Len()) return;
					Light_FlagTree(m_pNodes[_iNode].m_iNodeBack, _Pos, _Range, _Mask);
				}
				else
				{
					Light_FlagTree(m_pNodes[_iNode].m_iNodeBack, _Pos, _Range, _Mask);
					if (m_nTagFaces >= m_lLightElemTagList.Len()) return;
					Light_FlagTree(m_pNodes[_iNode].m_iNodeFront, _Pos, _Range, _Mask);
				}
			}
			else
				Light_FlagTree(m_pNodes[_iNode].m_iNodeFront, _Pos, _Range, _Mask);
		}
		else 
			if (Dist < _Range) Light_FlagTree(m_pNodes[_iNode].m_iNodeBack, _Pos, _Range, _Mask);
	}
}

void CXR_Model_BSP::Light_ProjectDynamic(int _iFace, CXR_Light* _pL, CXR_VertexBuffer* _pVB, int _iV)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_ProjectDynamic, MAUTOSTRIP_VOID);
	const CBSP_Face* pFace = &m_pFaces[_iFace];
	const CXR_PlaneMapping* pMapping = &m_lMappings[pFace->m_iMapping];
	const CPlane3Dfp32* pPlane = &m_pPlanes[pFace->m_iPlane];

	const CVec3Dfp32& Pos = _pL->GetPosition();
	fp32 Dist = pPlane->Distance(Pos);
	CVec3Dfp32 PosPlane;
	Pos.Combine(pPlane->n, -Dist, PosPlane);

	const CVec3Dfp32& UVec = pMapping->m_U;
	const CVec3Dfp32& VVec = pMapping->m_V;

	fp32 Range = _pL->m_Range;
	fp32 PhongMapScale = _pL->m_RangeInv * 0.5f;
//		if (Dist < MODEL_BSP_LIGHTATTENUATIONMIN) PhongMapScale *= (Dist * 1.0f/MODEL_BSP_LIGHTATTENUATIONMIN);
	fp32 Intens = 255.0f * Max(0.0f, (Range - M_Fabs(Dist)) * _pL->m_RangeInv);	// 0..1
	PhongMapScale /= M_Cos((1.0f - Intens) * _PI * 0.5f * 0.5f);
	uint32 Color;
	M_VSt_V4f32_Pixel32(M_VMul(M_VLdScalar(Intens), _pL->GetIntensityv().v), &Color);
//	int Color = _pL->m_IntensityInt32*Intens;
	Color |= 0xff000000;

	CVec2Dfp32 Scale(PhongMapScale, PhongMapScale);
	CVec3Dfp32 UxN, VxN;
	UVec.CrossProd(pPlane->n, UxN);
	VVec.CrossProd(pPlane->n, VxN);
	fp32 UInvOrtLen = M_InvSqrt(UxN.LengthSqr());
	fp32 VInvOrtLen = M_InvSqrt(VxN.LengthSqr());
	Scale.k[0] *= UInvOrtLen;
	Scale.k[1] *= VInvOrtLen;


//	CVec2Dfp32 TVerts[MODEL_BSP_MAXFACEVERTICES];

	fp32 uProjLight = UVec * PosPlane;
	fp32 vProjLight = VVec * PosPlane;
	CVec2Dfp32* pTV = &((CVec2Dfp32*) _pVB->GetVBChain()->m_pTV[0])[_iV];
	CPixel32* pC = &_pVB->GetVBChain()->m_pCol[_iV];
	for(int v = 0; v < pFace->m_nVertices; v++)
	{
		int iv = m_piVertices[pFace->m_iiVertices + v];
		fp32 uProj = UVec * m_pVertices[iv];
		fp32 vProj = VVec * m_pVertices[iv];
		fp32 s = (uProj - uProjLight) * Scale[0];
		fp32 t = (vProj - vProjLight) * Scale[1];
		pTV[v][0] = s + 0.5f;
		pTV[v][1] = t + 0.5f;
		pC[v] = Color;
	}
}

bool CXR_Model_BSP::Light_ProjectDynamic(int _nFaces, const uint32* _piFaces, CXR_Light* _pLight, CXR_VertexBuffer* _pVB)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_ProjectDynamic_2, false);
	if (_nFaces > MODEL_BSP_MAXDYNAMICLIGHTFACES) return false;
	uint32 liFaceVert[MODEL_BSP_MAXDYNAMICLIGHTFACES];

	CRC_Attributes* pA = ms_pVBM->Alloc_Attrib();
	if (!pA) return false;
	pA->SetDefault();
	pA->Attrib_Enable(CRC_FLAGS_CULL);
	_pVB->m_pAttrib = pA;

	if (!RenderTesselate(_piFaces, _nFaces, 0, ms_pVBM, _pVB, liFaceVert)) return false;

	CVec2Dfp32* pTV = ms_pVBM->Alloc_V2(_pVB->GetVBChain()->m_nV);
	CPixel32* pCol = ms_pVBM->Alloc_CPixel32(_pVB->GetVBChain()->m_nV);
	if (!pTV || !pCol) return false;

	_pVB->Geometry_TVertexArray(pTV, 0);
	_pVB->Geometry_ColorArray(pCol);

	for(int i = 0; i < _nFaces; i++)
		Light_ProjectDynamic(_piFaces[i], _pLight, _pVB, liFaceVert[i]);
	return true;
}

void CXR_Model_BSP::Light_RenderDynamic(int _iFace, int _iDynLight)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_RenderDynamic, MAUTOSTRIP_VOID);
	// Soooooooo legacy..
#ifdef NEVER
	// Blending and texture must be setup before calling.

	int FaceLightMask = m_lFaceLightMask[_iFace];
	if (FaceLightMask & (1 << _iDynLight))
	{
		if (!m_pCurrentWLS) return;
		CBSP_Face* pFace = &m_lFaces[_iFace];

		int iPlane = pFace->m_iPlane;
		int iMapping = pFace->m_iMapping;
		int iBump = 0;

		// Hacked in rotation of lightmap, to be moved to CXR_Light structure.
//		CMTime t = CMTime::GetCPU();
		CMTime t;
		t.Snapshot();
		fp32 sinv, cosv;
		QSinCos(t.GetModulus(1.0f), sinv, cosv);

//		fp32 sinv = 0.0f; fp32 cosv = 1.0f;

		int iDynamic = _iDynLight;
		CXR_Light* pL = &m_pCurrentWLS->m_lDynamic[iDynamic];
		CVec3Dfp32 Pos = pL->m_TransformedPos;
		fp32 Dist = m_lPlanes[iPlane].Distance(Pos);
		CVec3Dfp32 PosPlane = Pos - (m_lPlanes[iPlane].n * Dist);

		// FIXME:
		// Much of these calculations can be removed if we prestore the length of the mapping projection vectors.
		// but since I don't feel like wrecking any fileformats today, I'll stick with this slower code.

		CVec3Dfp32 UVec = m_lMappings[iMapping].m_U;
		CVec3Dfp32 VVec = m_lMappings[iMapping].m_V;
		fp32 ULen = M_Sqrt(UVec.LengthSqr());
		UVec *= 1.0f / ULen;
		fp32 VLen = M_Sqrt(VVec.LengthSqr());
		VVec *= 1.0f / VLen;

		fp32 Range = pL->m_Range;
		fp32 PhongMapScale = pL->m_RangeInv * 0.5f;
//		if (Dist < MODEL_BSP_LIGHTATTENUATIONMIN) PhongMapScale *= (Dist * 1.0f/MODEL_BSP_LIGHTATTENUATIONMIN);
		fp32 Intens = Max(0.0f, (Range - Abs(Dist)) * pL->m_RangeInv);	// 0..1
		PhongMapScale /= M_Cos((1.0f - Intens) * _PI * 0.5f * 0.5f);
		int Color = pL->m_IntensityInt32*Intens;
		Color |= 0xff000000;

		CVec2Dfp32 Scale(PhongMapScale, PhongMapScale);
		fp32 UOrtLen = (UVec / m_lPlanes[iPlane].n).Length();
		fp32 VOrtLen = (VVec / m_lPlanes[iPlane].n).Length();
		Scale.k[0] /= UOrtLen;
		Scale.k[1] /= VOrtLen;


		CVec2Dfp32 TVerts[MODEL_BSP_MAXFACEVERTICES];

		fp32 uProjLight = UVec * PosPlane;
		fp32 vProjLight = VVec * PosPlane;
		for(int v = 0; v < pFace->m_nVertices; v++)
		{
			int iv = m_piVertices[pFace->m_iiVertices + v];
			fp32 uProj = UVec * m_pVertices[iv];
			fp32 vProj = VVec * m_pVertices[iv];
			fp32 s = (uProj - uProjLight) * Scale[0];
			fp32 t = (vProj - vProjLight) * Scale[1];
			TVerts[v][0] = cosv*s - sinv*t + 0.5f;
			TVerts[v][1] = sinv*s + cosv*t + 0.5f;
		}

		m_pCurrentRC->Render_Polygon(pFace->m_nVertices, m_pVertices, TVerts, 
			&m_piVertices[pFace->m_iiVertices], g_IndexRamp32, Color);
	}
#endif
}


void CXR_Model_BSP::Light_ProjectDynamic(CXR_Light* _pLight, const CVec3Dfp32& _UVec, const CVec3Dfp32& _VVec, const CPlane3Dfp32& _Plane, 
										 int _nV, const CVec3Dfp32* _pV, CVec2Dfp32* _pDestTV, CPixel32* _pDestCol)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_ProjectDynamic, MAUTOSTRIP_VOID);

	fp32 Range = _pLight->m_Range;
	fp32 LDist = _Plane.Distance(_pLight->GetPosition());
	fp32 uProjLight = _UVec*_pLight->GetPosition();
	fp32 vProjLight = _VVec*_pLight->GetPosition();

	fp32 Scale = _pLight->m_RangeInv * 0.5f;

	CPixel32 I32(_pLight->GetIntensityv());

	for(int v = 0; v < _nV; v++)
	{
		fp32 VDist = _Plane.Distance(_pV[v]);
		fp32 RelDist = M_Fabs(LDist-VDist);
		fp32 Intens = Clamp01((Range - RelDist) * _pLight->m_RangeInv);
		int Color = I32*Intens;
		Color |= 0xff000000;
//		_pDestCol[v] = 0xff808080;
		_pDestCol[v] = Color;
		fp32 uProj = _UVec * _pV[v];
		fp32 vProj = _VVec * _pV[v];
		fp32 s = (uProj - uProjLight) * Scale;
		fp32 t = (vProj - vProjLight) * Scale;
		_pDestTV[v][0] = s + 0.5f;
		_pDestTV[v][1] = t + 0.5f;
	}
}



/*
static bool SetTexGenUV(CRC_Attributes* _pA, CXR_VBManager* _pVBM, int _iTxtChannel, const CPlane3Dfp32& _U, const CPlane3Dfp32& _V)
{
	MAUTOSTRIP(SetTexGenUV, false);
	fp32* pTexGen = _pVBM->Alloc_fp32(8);
	if (!pTexGen) return false;

	pTexGen[0] = _U.n[0];		pTexGen[1] = _U.n[1];		pTexGen[2] = _U.n[2];		pTexGen[3] = _U.d;
	pTexGen[4] = _V.n[0];		pTexGen[5] = _V.n[1];		pTexGen[6] = _V.n[2];		pTexGen[7] = _V.d;

	_pA->Attrib_TexGenU(_iTxtChannel, CRC_TEXGENMODE_LINEAR, pTexGen);
	_pA->Attrib_TexGenV(_iTxtChannel, CRC_TEXGENMODE_LINEAR, pTexGen+4);
	return true;
}

static bool SetTexGenUVW(CRC_Attributes* _pA, CXR_VBManager* _pVBM, int _iTxtChannel, const CPlane3Dfp32& _U, const CPlane3Dfp32& _V, const CPlane3Dfp32& _W)
{
	MAUTOSTRIP(SetTexGenUVW, false);
	fp32* pTexGen = _pVBM->Alloc_fp32(12);
	if (!pTexGen) return false;

	pTexGen[0] = _U.n[0];		pTexGen[1] = _U.n[1];		pTexGen[2] = _U.n[2];		pTexGen[3] = _U.d;
	pTexGen[4] = _V.n[0];		pTexGen[5] = _V.n[1];		pTexGen[6] = _V.n[2];		pTexGen[7] = _V.d;
	pTexGen[8] = _W.n[0];		pTexGen[9] = _W.n[1];		pTexGen[10] = _W.n[2];		pTexGen[11] = _W.d;

	_pA->Attrib_TexGenU(_iTxtChannel, CRC_TEXGENMODE_LINEAR, pTexGen);
	_pA->Attrib_TexGenV(_iTxtChannel, CRC_TEXGENMODE_LINEAR, pTexGen+4);
	_pA->Attrib_TexGenW(_iTxtChannel, CRC_TEXGENMODE_LINEAR, pTexGen+8);
	return true;
}


#include "../../MSystem/Raster/MRender_NVidia.h"

void CXR_Model_BSP::Light_SetAttenuation(CRC_Attributes* _pA, CXR_Light* _pL, int _iTxtChnBase) const
{
	MAUTOSTRIP(CXR_Model_BSP_Light_SetAttenuation, MAUTOSTRIP_VOID);
	// Attenuation setup
	_pA->Attrib_TextureID(_iTxtChnBase+0, ms_TextureID_AttenuationExp);
	_pA->Attrib_TextureID(_iTxtChnBase+1, ms_TextureID_AttenuationExp);

	fp32 Range = _pL->m_Range;
	fp32 Offset = 0.5f;

	const CVec3Dfp32& Pos = _pL->GetPosition();

	// TexCoord2 generation
	::SetTexGenUV(_pA, ms_pVBM, _iTxtChnBase,
		CPlane3Dfp32(CVec3Dfp32(0.5f / Range,0,0), Offset - 0.5f* Pos[0] / Range),
		CPlane3Dfp32(CVec3Dfp32(0, 0.5f / Range,0), Offset - 0.5f* Pos[1] / Range));

	// TexCoord3 generation
	::SetTexGenUV(_pA, ms_pVBM, _iTxtChnBase+1,
		CPlane3Dfp32(CVec3Dfp32(0, 0, 0.5f / Range), Offset - 0.5f* Pos[2] / Range),
		CPlane3Dfp32(CVec3Dfp32(0, 0, 0), Offset));
}
*/

CRC_Attributes* CXR_Model_BSP::Light_GetAttributes_NV20_ExpAttn_NdotL(int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf)
{
	return NULL;
#ifdef NEVER

	MAUTOSTRIP(CXR_Model_BSP_Light_GetAttributes_NV20_ExpAttn_NdotL, NULL);
#if defined( PLATFORM_PS2 ) || defined( PLATFORM_DOLPHIN )
	return NULL;
#else
	CRC_Attributes* pA = ms_Render_lpAttribDynLight[_iDynamic];
#ifndef MODEL_BSP_NOATTRSHARING
	if (pA)
		return pA;
#endif

	pA = ms_pVBM->Alloc_Attrib();
	if (!pA)
		return pA;

	pA->SetDefault();

	Light_SetAttenuation(pA, _pL, 2);
	pA->Attrib_Enable(CRC_FLAGS_CULL);

	// Alloc reg-combiners attribute
#ifdef SUPPORT_FRAGMENTPROGRAM
	if (ms_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20)
	{
		CRC_ExtAttributes_FragmentProgram20* pComb = (CRC_ExtAttributes_FragmentProgram20*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
		if (!pComb) 
			return false;

		pComb->Clear();

		pComb->m_pProgramName = "WBSPLight_ExpAttn_NdotL";

		pA->Attrib_TextureID(0, ms_TextureID_Normalize);
		pA->Attrib_TextureID(1, ms_TextureID_Normalize);

		// TexCoord0 generation
		const CVec3Dfp32& Pos = _pL->GetPosition();

		// TexCoord0 generation
		::SetTexGenUVW(pA, ms_pVBM, 0,
			CPlane3Dfp32(CVec3Dfp32(1, 0, 0), -Pos[0]),
			CPlane3Dfp32(CVec3Dfp32(0, 1, 0), -Pos[1]),
			CPlane3Dfp32(CVec3Dfp32(0, 0, 1), -Pos[2]));

		// TexCoord1 generation
		pA->Attrib_TexGen(1, CRC_TEXGENMODE_NORMALMAP);

		// -------------------------------------------------------------------
		// Exponential attenuation

		pA->m_pExtAttrib = pComb;
		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);

		ms_Render_lpAttribDynLight[_iDynamic] = pA;
		return pA;
	}
	else if (ms_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM11)
	{
		CRC_ExtAttributes_FragmentProgram11* pComb = (CRC_ExtAttributes_FragmentProgram11*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram11));
		if (!pComb) 
			return false;

		pComb->Clear();

		pComb->m_pProgramName = "WBSPLight_ExpAttn_NdotL";

		pA->Attrib_TextureID(0, ms_TextureID_Normalize);
		pA->Attrib_TextureID(1, ms_TextureID_Normalize);

		// TexCoord0 generation
		const CVec3Dfp32& Pos = _pL->GetPosition();

		// TexCoord0 generation
		::SetTexGenUVW(pA, ms_pVBM, 0,
			CPlane3Dfp32(CVec3Dfp32(1, 0, 0), -Pos[0]),
			CPlane3Dfp32(CVec3Dfp32(0, 1, 0), -Pos[1]),
			CPlane3Dfp32(CVec3Dfp32(0, 0, 1), -Pos[2]));

		// TexCoord1 generation
		pA->Attrib_TexGen(1, CRC_TEXGENMODE_NORMALMAP);

		// -------------------------------------------------------------------
		// Exponential attenuation

		pA->m_pExtAttrib = pComb;
		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);

		ms_Render_lpAttribDynLight[_iDynamic] = pA;
		return pA;
	}
	else
#endif
	{

	CRC_ExtAttributes_NV10* pComb = (CRC_ExtAttributes_NV10*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_NV10));
	if (!pComb) return false;

	pComb->Clear();
	pA->Attrib_TextureID(0, ms_TextureID_Normalize);
	pA->Attrib_TextureID(1, ms_TextureID_Normalize);

	const CVec3Dfp32& Pos = _pL->GetPosition();

	// TexCoord0 generation
	::SetTexGenUVW(pA, ms_pVBM, 0,
		CPlane3Dfp32(CVec3Dfp32(1, 0, 0), -Pos[0]),
		CPlane3Dfp32(CVec3Dfp32(0, 1, 0), -Pos[1]),
		CPlane3Dfp32(CVec3Dfp32(0, 0, 1), -Pos[2]));

	// TexCoord1 generation
	pA->Attrib_TexGen(1, CRC_TEXGENMODE_NORMALMAP);

	// -------------------------------------------------------------------
	// Exponential attenuation

	pComb->Clear(0);
	pComb->SetInputRGB(0, NV_INPUT_TEXTURE0|NV_MAPPING_EXPNORMAL, NV_INPUT_TEXTURE1|NV_MAPPING_EXPNEGATE);
	pComb->SetInputAlpha(0, NV_INPUT_TEXTURE2, NV_INPUT_TEXTURE3);
	pComb->SetOutputRGB(0, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
	pComb->SetOutputAlpha(0, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_TWO, true, false, false);

	pComb->SetFinal(NV_INPUT_EF, NV_INPUT_PRIMARY, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_SPARE0, NV_INPUT_SPARE0|NV_COMP_ALPHA, NV_INPUT_SPARE0|NV_COMP_ALPHA);

	pComb->SetNumCombiners(1);
	pComb->Fixup();
	pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);

	pA->m_pExtAttrib = pComb;

	ms_Render_lpAttribDynLight[_iDynamic] = pA;
	return pA;
}
#endif
#endif
}

CRC_Attributes* CXR_Model_BSP::Light_GetAttributes_NV20_ExpAttn_NdotL_DestAlpha(int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_GetAttributes_NV20_ExpAttn_NdotL_DestAlpha, NULL);
	M_ASSERT(0, "Not implemented.");
	return NULL;

/*	CRC_Attributes* pA = ms_Render_lpAttribDynLight[_iDynamic];
	if (pA)
		return pA;

	pA = _pVBM->Alloc_Attrib();
	if (!pA)
		return pA;

	pA->SetDefault();

	Light_SetAttenuation(pA, _pL);
	pA->Attrib_Enable(CRC_FLAGS_CULL);

	return pA;*/
}

CRC_Attributes* CXR_Model_BSP::Light_GetAttributes_NV20_ExpAttn_NMapdotL(int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf, int _TextureIDBump)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_GetAttributes_NV20_ExpAttn_NMapdotL, NULL);

	return NULL;
#ifdef NEVER

#if defined( PLATFORM_PS2 ) || defined( PLATFORM_DOLPHIN )
	return NULL;
#else
	CRC_Attributes* pA = ms_pVBM->Alloc_Attrib();
	if (!pA)
		return pA;

	pA->SetDefault();

	Light_SetAttenuation(pA, _pL, 2);
	pA->Attrib_Enable(CRC_FLAGS_CULL);

	// Alloc reg-combiners attribute
#ifdef SUPPORT_FRAGMENTPROGRAM
	if (ms_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20)
	{
		CRC_ExtAttributes_FragmentProgram20* pComb = (CRC_ExtAttributes_FragmentProgram20*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
		if (!pComb) 
			return false;

		pComb->Clear();
		pComb->m_pProgramName = "WBSPLight_ExpAttn_NMapdotL";

		CVec3Dfp32* pLightVec = ms_pVBM->Alloc_V3(1);
		if (!pLightVec)
			return false;
		*pLightVec = _pL->GetPosition();

		pA->Attrib_TextureID(0, _TextureIDBump);
		pA->Attrib_TextureID(1, ms_TextureID_Normalize);
		pA->Attrib_TexGenU(1, CRC_TEXGENMODE_TSLV, (fp32*)pLightVec);

		pComb->m_Color0 = _pL->m_IntensityInt32;
		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		pA->m_pExtAttrib = pComb;

		return pA;
	}
	else if (ms_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM11)
	{
		CRC_ExtAttributes_FragmentProgram11* pComb = (CRC_ExtAttributes_FragmentProgram11*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram11));
		if (!pComb) 
			return false;

		pComb->Clear();
		pComb->m_pProgramName = "WBSPLight_ExpAttn_NMapdotL";

		CVec3Dfp32* pLightVec = ms_pVBM->Alloc_V3(1);
		if (!pLightVec)
			return false;
		*pLightVec = _pL->GetPosition();

		pA->Attrib_TextureID(0, _TextureIDBump);
		pA->Attrib_TextureID(1, ms_TextureID_Normalize);
		pA->Attrib_TexGenU(1, CRC_TEXGENMODE_TSLV, (fp32*)pLightVec);

		pComb->m_Color0 = _pL->m_IntensityInt32;
		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		pA->m_pExtAttrib = pComb;

		return pA;
	}
	else
#endif
	{

		CRC_ExtAttributes_NV10* pComb = (CRC_ExtAttributes_NV10*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_NV10));
		if (!pComb) return false;

		pComb->Clear();

		CVec3Dfp32* pLightVec = ms_pVBM->Alloc_V3(1);
		if (!pLightVec)
			return false;
		*pLightVec = _pL->GetPosition();

		pA->Attrib_TextureID(0, _TextureIDBump);
		pA->Attrib_TextureID(1, ms_TextureID_Normalize);
		pA->Attrib_TexGenU(1, CRC_TEXGENMODE_TSLV, (fp32*)pLightVec);

		pComb->Clear(0);
		pComb->SetInputRGB(0, NV_INPUT_TEXTURE1|NV_MAPPING_HALFBIASNORMAL, NV_INPUT_TEXTURE0|NV_MAPPING_EXPNORMAL);
		pComb->SetInputAlpha(0, NV_INPUT_TEXTURE2, NV_INPUT_TEXTURE3);
		pComb->SetOutputRGB(0, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_TWO, true, false, false);
		pComb->SetOutputAlpha(0, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);

		pComb->SetFinal(NV_INPUT_SPARE0, NV_INPUT_EF, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_SPARE0|NV_COMP_ALPHA, NV_INPUT_COLOR0, NV_INPUT_ZERO|NV_COMP_ALPHA);

		pComb->SetConst0(_pL->m_IntensityInt32);
		pComb->SetNumCombiners(1);
		pComb->Fixup();

		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		pA->m_pExtAttrib = pComb;
		return pA;
	}
#endif
#endif
}

CRC_Attributes* CXR_Model_BSP::Light_GetAttributes_NV20_ExpAttn_NMapdotH(int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf, int _TextureIDBump)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_GetAttributes_NV20_ExpAttn_NMapdotH, NULL);

	return NULL;
#ifdef NEVER

#if defined( PLATFORM_PS2 ) || defined( PLATFORM_DOLPHIN )
	return NULL;
#else
	CRC_Attributes* pA = ms_pVBM->Alloc_Attrib();
	if (!pA)
		return pA;

	pA->SetDefault();

	Light_SetAttenuation(pA, _pL, 2);
	pA->Attrib_Enable(CRC_FLAGS_CULL | CRC_FLAGS_ALPHAWRITE);

#ifdef SUPPORT_FRAGMENTPROGRAM
	if (ms_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20)
	{
		// Alloc reg-combiners attribute
		CRC_ExtAttributes_FragmentProgram20* pComb = (CRC_ExtAttributes_FragmentProgram20*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
		if (!pComb) 
			return false;

		pComb->Clear();
		pComb->m_pProgramName = "WBSPLight_ExpAttn_NMapdotH";


		CVec4Dfp32* pLightVec = ms_pVBM->Alloc_V4(2);
		if (!pLightVec)
			return false;
		const CVec3Dfp32& Pos = _pL->GetPosition();
		pLightVec[0][0] = Pos[0];
		pLightVec[0][1] = Pos[1];
		pLightVec[0][2] = Pos[2];
		pLightVec[0][3] = 1;
		pLightVec[1][0] = m_pView->m_CurLocalVP[0];
		pLightVec[1][1] = m_pView->m_CurLocalVP[1];
		pLightVec[1][2] = m_pView->m_CurLocalVP[2];
		pLightVec[1][3] = 1;

		pA->Attrib_TextureID(0, _TextureIDBump);
		pA->Attrib_TextureID(1, ms_TextureID_Normalize);
		pA->Attrib_TexGenU(1, CRC_TEXGENMODE_TSREFLECTION, (fp32*)pLightVec);
		pComb->m_Color0 = _pL->m_IntensityInt32 * 0.5f;
		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		pA->m_pExtAttrib = pComb;

		return pA;
	}
	else if (ms_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM11)
	{
		// Alloc reg-combiners attribute
		CRC_ExtAttributes_FragmentProgram11* pComb = (CRC_ExtAttributes_FragmentProgram11*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram11));
		if (!pComb) 
			return false;

		pComb->Clear();
		pComb->m_pProgramName = "WBSPLight_ExpAttn_NMapdotH";


		CVec4Dfp32* pLightVec = ms_pVBM->Alloc_V4(2);
		if (!pLightVec)
			return false;
		const CVec3Dfp32& Pos = _pL->GetPosition();
		pLightVec[0][0] = Pos[0];
		pLightVec[0][1] = Pos[1];
		pLightVec[0][2] = Pos[2];
		pLightVec[0][3] = 1;
		pLightVec[1][0] = m_pView->m_CurLocalVP[0];
		pLightVec[1][1] = m_pView->m_CurLocalVP[1];
		pLightVec[1][2] = m_pView->m_CurLocalVP[2];
		pLightVec[1][3] = 1;

		pA->Attrib_TextureID(0, _TextureIDBump);
		pA->Attrib_TextureID(1, ms_TextureID_Normalize);
		pA->Attrib_TexGenU(1, CRC_TEXGENMODE_TSREFLECTION, (fp32*)pLightVec);
		pComb->m_Color0 = _pL->m_IntensityInt32 * 0.5f;
		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		pA->m_pExtAttrib = pComb;

		return pA;
	}
	else
#endif
	{
		// Alloc reg-combiners attribute
		CRC_ExtAttributes_NV20* pComb = (CRC_ExtAttributes_NV20*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_NV20));
		if (!pComb) return false;

		pComb->Clear();

		CVec4Dfp32* pLightVec = ms_pVBM->Alloc_V4(2);
		if (!pLightVec)
			return false;
		const CVec3Dfp32& Pos = _pL->GetPosition();
		pLightVec[0][0] = Pos[0];
		pLightVec[0][1] = Pos[1];
		pLightVec[0][2] = Pos[2];
		pLightVec[0][3] = 1;
		pLightVec[1][0] = m_pView->m_CurLocalVP[0];
		pLightVec[1][1] = m_pView->m_CurLocalVP[1];
		pLightVec[1][2] = m_pView->m_CurLocalVP[2];
		pLightVec[1][3] = 1;

		pA->Attrib_TextureID(0, _TextureIDBump);
		pA->Attrib_TextureID(1, ms_TextureID_Normalize);
		pA->Attrib_TexGenU(1, CRC_TEXGENMODE_TSREFLECTION, (fp32*)pLightVec);

		pComb->Clear(0);
		pComb->SetInputRGB(0, NV_INPUT_TEXTURE1|NV_MAPPING_HALFBIASNORMAL, NV_INPUT_TEXTURE0|NV_MAPPING_EXPNORMAL);
		pComb->SetOutputRGB(0, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_TWO, true, false, false);

		pComb->Clear(1);
		pComb->SetInputRGB(1, NV_OUTPUT_SPARE0, NV_OUTPUT_SPARE0);
		pComb->SetOutputRGB(1, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
		pComb->Clear(2);
		pComb->SetInputRGB(2, NV_OUTPUT_SPARE0, NV_OUTPUT_SPARE0);
		pComb->SetOutputRGB(2, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
		pComb->Clear(3);
		pComb->SetInputRGB(3, NV_OUTPUT_SPARE0, NV_OUTPUT_SPARE0);
		pComb->SetOutputRGB(3, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
		pComb->Clear(4);
		pComb->SetInputRGB(4, NV_OUTPUT_SPARE0, NV_OUTPUT_SPARE0);
		pComb->SetOutputRGB(4, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
		pComb->Clear(5);
		pComb->SetInputRGB(5, NV_OUTPUT_SPARE0, NV_OUTPUT_SPARE0);
		pComb->SetOutputRGB(5, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
		pComb->Clear(6);
		pComb->SetInputRGB(6, NV_OUTPUT_SPARE0, NV_OUTPUT_SPARE0);
		pComb->SetOutputRGB(6, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);

		pComb->SetInputAlpha(0, NV_INPUT_TEXTURE2, NV_INPUT_TEXTURE3);
		pComb->SetOutputAlpha(0, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);

		pComb->SetFinal(NV_INPUT_SPARE0, NV_INPUT_EF, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_SPARE0|NV_COMP_ALPHA, NV_INPUT_COLOR0, NV_INPUT_SPARE0|NV_COMP_ALPHA);

		pComb->SetConst0(_pL->m_IntensityInt32 * 0.5f);
		pComb->SetNumCombiners(7);
		pComb->Fixup();

		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		pA->m_pExtAttrib = pComb;
		return pA;
	}
#endif
#endif
}


CRC_Attributes* CXR_Model_BSP::Light_GetAttributes_NV20_ExpAttn_NMapHSpaceCubeMap(int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf, int _TextureIDBump, int _TextureIDSpecFunc)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_GetAttributes_NV20_ExpAttn_NMapHSpaceCubeMap, NULL);
	return NULL;
#ifdef NEVER
#if defined( PLATFORM_PS2 ) || defined( PLATFORM_DOLPHIN )
	return NULL;
#else
	CRC_Attributes* pA = ms_pVBM->Alloc_Attrib();
	if (!pA)
		return pA;

	pA->SetDefault();

//	Light_SetAttenuation(pA, _pL);
	pA->Attrib_Enable(CRC_FLAGS_CULL);

#ifdef SUPPORT_FRAGMENTPROGRAM
	if (ms_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20)
	{
		// Alloc reg-combiners attribute
		CRC_ExtAttributes_FragmentProgram20* pComb = (CRC_ExtAttributes_FragmentProgram20*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
		if (!pComb) 
			return false;

		pComb->Clear();
		pComb->m_pProgramName = "WBSPLight_ExpAttn_NMapHSpaceCubeMap";

		CVec4Dfp32* pLightVec = ms_pVBM->Alloc_V4(2);
		if (!pLightVec)
			return false;

		const CVec3Dfp32& Pos = _pL->GetPosition();
		pLightVec[0][0] = Pos[0];
		pLightVec[0][1] = Pos[1];
		pLightVec[0][2] = Pos[2];
		pLightVec[0][3] = 1;
		pLightVec[1][0] = m_pView->m_CurLocalVP[0];
		pLightVec[1][1] = m_pView->m_CurLocalVP[1];
		pLightVec[1][2] = m_pView->m_CurLocalVP[2];
		pLightVec[1][3] = 1;

		pA->Attrib_TextureID(0, _TextureIDBump);
		pA->Attrib_TextureID(1, _TextureIDBump);
		pA->Attrib_TextureID(2, _TextureIDBump);
		pA->Attrib_TextureID(3, _TextureIDSpecFunc);
		pA->Attrib_TexGenU(1, CRC_TEXGENMODE_TSREFLECTION, (fp32*)pLightVec);
		pA->Attrib_TexGenU(2, CRC_TEXGENMODE_VOID, NULL);
		pA->Attrib_TexGenU(3, CRC_TEXGENMODE_VOID, NULL);

		pComb->m_Color0 = _pL->m_IntensityInt32 * 0.5f;
		pComb->m_Color1 = 0;

		pA->Attrib_Enable(CRC_FLAGS_BLEND);
		pA->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
		pA->Attrib_DestBlend(CRC_BLEND_ONE);
		pA->m_pExtAttrib = pComb;
		return pA;
	}
	else if (ms_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM11)
	{
		// Alloc reg-combiners attribute
		CRC_ExtAttributes_FragmentProgram11* pComb = (CRC_ExtAttributes_FragmentProgram11*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram11));
		if (!pComb) 
			return false;

		pComb->Clear();
		pComb->m_pProgramName = "WBSPLight_ExpAttn_NMapHSpaceCubeMap";

		CVec4Dfp32* pLightVec = ms_pVBM->Alloc_V4(2);
		if (!pLightVec)
			return false;

		const CVec3Dfp32& Pos = _pL->GetPosition();
		pLightVec[0][0] = Pos[0];
		pLightVec[0][1] = Pos[1];
		pLightVec[0][2] = Pos[2];
		pLightVec[0][3] = 1;
		pLightVec[1][0] = m_pView->m_CurLocalVP[0];
		pLightVec[1][1] = m_pView->m_CurLocalVP[1];
		pLightVec[1][2] = m_pView->m_CurLocalVP[2];
		pLightVec[1][3] = 1;

		pA->Attrib_TextureID(0, _TextureIDBump);
		pA->Attrib_TextureID(1, _TextureIDBump);
		pA->Attrib_TextureID(2, _TextureIDBump);
		pA->Attrib_TextureID(3, _TextureIDSpecFunc);
		pA->Attrib_TexGenU(1, CRC_TEXGENMODE_TSREFLECTION, (fp32*)pLightVec);
		pA->Attrib_TexGenU(2, CRC_TEXGENMODE_VOID, NULL);
		pA->Attrib_TexGenU(3, CRC_TEXGENMODE_VOID, NULL);

		pComb->m_Color0 = _pL->m_IntensityInt32 * 0.5f;
		pComb->m_Color1 = 0;

		pA->Attrib_Enable(CRC_FLAGS_BLEND);
		pA->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
		pA->Attrib_DestBlend(CRC_BLEND_ONE);
		pA->m_pExtAttrib = pComb;
		return pA;
	}
	else
#endif
	{
	// Alloc reg-combiners attribute
	CRC_ExtAttributes_NV20* pComb = (CRC_ExtAttributes_NV20*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_NV20));
	if (!pComb) return false;

	pComb->Clear();
	pComb->Clear(0);

	CVec4Dfp32* pLightVec = ms_pVBM->Alloc_V4(2);
	if (!pLightVec)
		return false;
	const CVec3Dfp32& Pos = _pL->GetPosition();
	pLightVec[0][0] = Pos[0];
	pLightVec[0][1] = Pos[1];
	pLightVec[0][2] = Pos[2];
	pLightVec[0][3] = 1;
	pLightVec[1][0] = m_pView->m_CurLocalVP[0];
	pLightVec[1][1] = m_pView->m_CurLocalVP[1];
	pLightVec[1][2] = m_pView->m_CurLocalVP[2];
	pLightVec[1][3] = 1;

	pA->Attrib_TextureID(0, _TextureIDBump);
	pA->Attrib_TextureID(1, _TextureIDBump);
	pA->Attrib_TextureID(2, _TextureIDBump);
	pA->Attrib_TextureID(3, _TextureIDSpecFunc);
	pA->Attrib_TexGenU(1, CRC_TEXGENMODE_TSREFLECTION, (fp32*)pLightVec);
	pA->Attrib_TexGenU(2, CRC_TEXGENMODE_VOID, NULL);
	pA->Attrib_TexGenU(3, CRC_TEXGENMODE_VOID, NULL);


	pComb->SetFinal(NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_EF, NV_INPUT_TEXTURE3, NV_INPUT_COLOR0, NV_INPUT_ZERO|NV_COMP_ALPHA);
	pComb->SetNumCombiners(1);
	pComb->SetConst0(_pL->m_IntensityInt32 * 0.5f);
	
	// Set texture shader
	pComb->SetTexureOps(NV_TS_TEXTURE_2D, NV_TS_DOT_PRODUCT_NV, NV_TS_DOT_PRODUCT_NV, NV_TS_DOT_PRODUCT_TEXTURE_CUBE_MAP_NV);
	pComb->Fixup();

	pA->Attrib_Enable(CRC_FLAGS_BLEND);
	pA->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
	pA->Attrib_DestBlend(CRC_BLEND_ONE);
	pA->m_pExtAttrib = pComb;
	return pA;
}
#endif
#endif
}


CRC_Attributes* CXR_Model_BSP::Light_GetAttributes_NV20_DestAlphaExpAttn(int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf, int _TextureIDMask)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_GetAttributes_NV20_DestAlphaExpAttn, NULL);
	return NULL;
#ifdef NEVER

#if defined( PLATFORM_PS2 ) || defined( PLATFORM_DOLPHIN )
	return NULL;
#else
	CRC_Attributes* pA = ms_Render_lpAttribDynLight_DestAlphaExpAttn[_iDynamic];
#ifndef MODEL_BSP_NOATTRSHARING
//	if (pA)
//		return pA;
#endif

	pA = ms_pVBM->Alloc_Attrib();
	if (!pA)
		return pA;

	pA->SetDefault();

	Light_SetAttenuation(pA, _pL, 1);

	pA->Attrib_TextureID(0, _TextureIDMask);
	pA->Attrib_TexCoordSet(0, 0);

#ifdef SUPPORT_FRAGMENTPROGRAM
	if (ms_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20)
	{
		// Alloc reg-combiners attribute
		CRC_ExtAttributes_FragmentProgram20* pComb = (CRC_ExtAttributes_FragmentProgram20*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
		if (!pComb) 
			return false;

		pComb->Clear();
		pComb->m_pProgramName = "WBSPLight_DestAlphaExpAttn";

		pA->Attrib_Enable(CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_CULL);
		pA->Attrib_Disable(CRC_FLAGS_COLORWRITE);
		pA->m_pExtAttrib = pComb;

		ms_Render_lpAttribDynLight_DestAlphaExpAttn[_iDynamic] = pA;
		return pA;
	}
	else if (ms_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM11)
	{
		// Alloc reg-combiners attribute
		CRC_ExtAttributes_FragmentProgram11* pComb = (CRC_ExtAttributes_FragmentProgram11*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram11));
		if (!pComb) 
			return false;

		pComb->Clear();
		pComb->m_pProgramName = "WBSPLight_DestAlphaExpAttn";

		pA->Attrib_Enable(CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_CULL);
		pA->Attrib_Disable(CRC_FLAGS_COLORWRITE);
		pA->m_pExtAttrib = pComb;

		ms_Render_lpAttribDynLight_DestAlphaExpAttn[_iDynamic] = pA;
		return pA;
	}
	else
#endif
	{
		// Alloc reg-combiners attribute
		CRC_ExtAttributes_NV10* pComb = (CRC_ExtAttributes_NV10*) ms_pVBM->Alloc(sizeof(CRC_ExtAttributes_NV10));
		if (!pComb) return false;

		pComb->Clear();

		pComb->Clear(0);
		pComb->SetInputAlpha(0, NV_INPUT_TEXTURE1, NV_INPUT_TEXTURE2);
		pComb->SetOutputAlpha(0, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);

		pComb->Clear(1);
		pComb->SetInputAlpha(1, NV_OUTPUT_SPARE0|NV_COMP_ALPHA, NV_INPUT_TEXTURE0);
		pComb->SetOutputAlpha(1, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);

		pComb->SetFinal(NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_SPARE0|NV_COMP_ALPHA);

		pComb->SetNumCombiners(2);
		pComb->SetConst0(_pL->m_IntensityInt32 * 0.5f);
		pComb->Fixup();

		pA->Attrib_Enable(CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_CULL);
		pA->Attrib_Disable(CRC_FLAGS_COLORWRITE);
		pA->m_pExtAttrib = pComb;

		ms_Render_lpAttribDynLight_DestAlphaExpAttn[_iDynamic] = pA;
		return pA;
	}
#endif
#endif
}


bool CXR_Model_BSP::Light_ProjectDynamic_NV20(CXR_VertexBuffer* _pVB, int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_ProjectDynamic_NV20, false);
	// This is for NV20+ really..
#if defined( PLATFORM_PS2 ) || defined( PLATFORM_DOLPHIN )
	return NULL;
#else
	// Get bump-map
	CXW_SurfaceKeyFrame* pSurfKey = _pSurf->GetBaseFrame();
//	CXW_SurfaceLayer* pLayerBump = _pSurf->GetBaseFrame()->GetBumpMap();
	int TextureIDBump = 0;
	int TextureIDSpec = 0;
	int SpecularExp = 1;
//	CPixel32 SpecularColor;

	{
		CXW_SurfaceLayer* pLayers = pSurfKey->m_lTextures.GetBasePtr();
		int nLayers = pSurfKey->m_lTextures.Len();
		
		int i = 0;
		while(i < nLayers)
		{
			if (pLayers[i].m_Type == XW_LAYERTYPE_NORMAL)
			{
				TextureIDBump = pLayers[i].m_TextureID;
				i++;
				break;
			}
			i++;
		}
		while(i < nLayers)
		{
			if (pLayers[i].m_Type == XW_LAYERTYPE_SPECULAR)
			{
				TextureIDSpec = pLayers[i].m_TextureID;
				SpecularExp = pLayers[i].m_AlphaRef;
//				SpecularColor = pLayers[i].m_HDRColor.GetPixel32();
				i++;
				break;
			}
			i++;
		}
	}

	if (TextureIDBump)
	{
		CRC_Attributes* pA = Light_GetAttributes_NV20_ExpAttn_NMapdotL(_iDynamic, _pL, _pSurf, TextureIDBump);
		if (!pA)
			return false;
		_pVB->m_pAttrib = pA;


		if (true)
		{
			fp32 LightPriorOffs = fp32(_iDynamic)*0.01f;


			// Render attenuation into destination alpha
			CXR_VertexBuffer* pVBAttn = ms_pVBM->Alloc_VB(CXR_VB_ATTRIB);
			if (!pVBAttn)
				return false;

			pVBAttn->m_pTransform = _pVB->m_pTransform;
			pVBAttn->CopyVBChain(_pVB);

			CRC_Attributes* pAAttn = Light_GetAttributes_NV20_DestAlphaExpAttn(_iDynamic, _pL, _pSurf, TextureIDSpec);
			if (!pAAttn)
				return false;
			pVBAttn->m_pAttrib = pAAttn;
			pVBAttn->m_Priority = CXR_VBPRIORITY_POSTDYNLIGHT + LightPriorOffs;

			ms_pVBM->AddVB(pVBAttn);

			// Render specular lighting. (FB.col += Spec * FB.alpha)
			CXR_VertexBuffer* pVBSpec = ms_pVBM->Alloc_VB(CXR_VB_ATTRIB);
			if (!pVBSpec)
				return false;

			pVBSpec->m_pTransform = _pVB->m_pTransform;
			pVBSpec->CopyVBChain(_pVB);

			int iSpecFunc = 0;
			{
				int Exp = 1;
				while(iSpecFunc < 7 && Exp < SpecularExp)
				{
					Exp += Exp;
					iSpecFunc++;
				}
			}

			int TextureIDSpecFunc = ms_TextureID_Specular[iSpecFunc];

			CRC_Attributes* pA = Light_GetAttributes_NV20_ExpAttn_NMapHSpaceCubeMap(_iDynamic, _pL, _pSurf, TextureIDBump, TextureIDSpecFunc);
//			CRC_Attributes* pA = Light_GetAttributes_NV20_ExpAttn_NMapdotH(_iDynamic, _pL, _pSurf, TextureIDBump);
			if (!pA)
				return false;
			pVBSpec->m_pAttrib = pA;
			pVBSpec->m_Priority = CXR_VBPRIORITY_POSTDYNLIGHT+0.001f + LightPriorOffs;

			ms_pVBM->AddVB(pVBSpec);
		}
	}
	else
	{
		CRC_Attributes* pA = Light_GetAttributes_NV20_ExpAttn_NdotL(_iDynamic, _pL, _pSurf);
		if (!pA)
			return false;
		_pVB->m_pAttrib = pA;
		M_VSt_V4f32_Pixel32(M_VMul(M_VScalar(255.0f), _pL->GetIntensityv().v), &_pVB->m_Color);
	}

	return true;

#ifdef NEVER
	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if (!pA) return false;
	pA->SetDefault();
	pA->Attrib_Enable(CRC_FLAGS_CULL);
	_pVB->m_pAttrib = pA;

	// Attenuation setup
	{
		pA->Attrib_TextureID(2, ms_TextureID_AttenuationExp);
		pA->Attrib_TextureID(3, ms_TextureID_AttenuationExp);

		fp32 Range = _pL->m_Range;
		fp32 Offset = 0.5f;

		// TexCoord2 generation
		::SetTexGenUV(_pVB, _pVBM, 2,
			CPlane3Dfp32(CVec3Dfp32(0.5f / Range,0,0), Offset - 0.5f* _pL->m_TransformedPos[0] / Range),
			CPlane3Dfp32(CVec3Dfp32(0, 0.5f / Range,0), Offset - 0.5f* _pL->m_TransformedPos[1] / Range));

		// TexCoord3 generation
		::SetTexGenUV(_pVB, _pVBM, 3,
			CPlane3Dfp32(CVec3Dfp32(0, 0, 0.5f / Range), Offset - 0.5f* _pL->m_TransformedPos[2] / Range),
			CPlane3Dfp32(CVec3Dfp32(0, 0, 0), Offset));
	}

	// Alloc reg-combiners attribute
	CRC_ExtAttributes_NV10* pComb = (CRC_ExtAttributes_NV10*) _pVBM->Alloc(sizeof(CRC_ExtAttributes_NV10));
	if (!pComb) return false;

	pComb->Clear();
	CNV_RegCombiners_Combiner& C0 = pComb->m_Combiners[0];;
	CNV_RegCombiners_FinalCombiner& FC = pComb->m_FinalCombiner;
	C0.Clear();

//	if (!TextureIDBump)
//		TextureIDBump = g_TextureID_TestBump;

	// Fulhack 1b:
//	bool bDestAlpha = (_pL->m_Range <= 128.0f);
	bool bDestAlpha = false;

//	int bSpecular = TextureIDBump;
	int bSpecular = 0;

	if (TextureIDBump)
	{
		CVec3Dfp32* pLightVec = _pVBM->Alloc_V3(1);
		if (!pLightVec)
			return false;
		*pLightVec = _pL->m_TransformedPos;

		pA->Attrib_TextureID(0, TextureIDBump);
		pA->Attrib_TextureID(1, g_TextureID_Normalize);
		pA->Attrib_TexGenU(1, CRC_TEXGENMODE_TSLV, (fp32*)pLightVec);

		pComb->m_NumCombiners = 1;

		C0.m_InputRGB[0].Set(NV_INPUT_TEXTURE1, NV_MAPPING_HALFBIASNORMAL, 0);
		C0.m_InputRGB[1].Set(NV_INPUT_TEXTURE0, NV_MAPPING_EXPNORMAL, 0);
		C0.m_OutputRGB.Set(NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_TWO, 0, true, false, false);

		C0.m_InputAlpha[0].Set(NV_INPUT_TEXTURE2, 0, 0);
		C0.m_InputAlpha[1].Set(NV_INPUT_TEXTURE3, 0, 0);
		C0.m_OutputAlpha.Set(NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, 0, 0, false, false, false);

		FC.m_InputRGB[0].Set(NV_INPUT_SPARE0, 0, 0);
		FC.m_InputRGB[1].Set(NV_INPUT_EF, 0, 0);
		FC.m_InputRGB[2].Set(NV_INPUT_ZERO, 0, 0);
		FC.m_InputRGB[3].Set(NV_INPUT_ZERO, 0, 0);
		FC.m_InputRGB[4].Set(NV_INPUT_SPARE0, 0, NV_COMP_ALPHA);
		FC.m_InputRGB[5].Set(NV_INPUT_COLOR0, 0, 0);

	//	FC.m_InputRGB[0].Set(NV_INPUT_TEXTURE0, 0, 0);
	//	FC.m_InputRGB[1].Set(NV_INPUT_COLOR1, 0, 0);

		FC.m_InputAlpha.Set(NV_INPUT_ZERO, 0, NV_COMP_ALPHA);

		pComb->m_Color0 = _pL->m_IntensityInt32;

		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		pA->m_pExtAttrib = pComb;
	}
	else
	{
		pA->Attrib_TextureID(0, g_TextureID_Normalize);
		pA->Attrib_TextureID(1, g_TextureID_Normalize);

		// TexCoord0 generation
		::SetTexGenUVW(_pVB, _pVBM, 0,
			CPlane3Dfp32(CVec3Dfp32(1, 0, 0), -_pL->m_TransformedPos[0]),
			CPlane3Dfp32(CVec3Dfp32(0, 1, 0), -_pL->m_TransformedPos[1]),
			CPlane3Dfp32(CVec3Dfp32(0, 0, 1), -_pL->m_TransformedPos[2]));

		// TexCoord1 generation
		pA->Attrib_TexGen(1, CRC_TEXGENMODE_NORMALMAP);

		// -------------------------------------------------------------------
		// Exponential attenuation

		C0.Clear();

		if (bDestAlpha)
		{
			CNV_RegCombiners_Combiner& C1 = pComb->m_Combiners[1];
			C1.Clear();

			C0.m_InputAlpha[0].Set(NV_INPUT_TEXTURE2, 0, 0);
			C0.m_InputAlpha[1].Set(NV_INPUT_TEXTURE3, 0, 0);
			C0.m_InputRGB[0].Set(NV_INPUT_TEXTURE0, NV_MAPPING_EXPNORMAL, 0);
			C0.m_InputRGB[1].Set(NV_INPUT_TEXTURE1, NV_MAPPING_EXPNEGATE, 0);
			C0.m_OutputAlpha.Set(NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, 0, 0, false, false, false);
			C0.m_OutputRGB.Set(NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_TWO, 0, true, false, false);

			C1.m_InputAlpha[0].Set(NV_OUTPUT_SPARE0, 0, 0);
			C1.m_InputAlpha[1].Set(NV_OUTPUT_SPARE0, 0, NV_COMP_ALPHA);
			C1.m_OutputAlpha.Set(NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, 0, 0, false, false, false);

			FC.m_InputRGB[0].Set(NV_INPUT_ZERO, 0, 0);
			FC.m_InputRGB[1].Set(NV_INPUT_ZERO, 0, 0);
			FC.m_InputRGB[2].Set(NV_INPUT_ZERO, 0, 0);
			FC.m_InputRGB[3].Set(NV_INPUT_ZERO, 0, 0);
			FC.m_InputRGB[4].Set(NV_INPUT_ZERO, 0, 0);
			FC.m_InputRGB[5].Set(NV_INPUT_ZERO, 0, 0);
			FC.m_InputAlpha.Set(NV_INPUT_SPARE0, 0, NV_COMP_ALPHA);
			pComb->m_NumCombiners = 2;
			pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
			pA->Attrib_Disable(CRC_FLAGS_COLORWRITE);
		}
		else
		{
			C0.m_InputAlpha[0].Set(NV_INPUT_TEXTURE2, 0, 0);
			C0.m_InputAlpha[1].Set(NV_INPUT_TEXTURE3, 0, 0);
			C0.m_InputRGB[0].Set(NV_INPUT_TEXTURE0, NV_MAPPING_EXPNORMAL, 0);
			C0.m_InputRGB[1].Set(NV_INPUT_TEXTURE1, NV_MAPPING_EXPNEGATE, 0);
			C0.m_OutputAlpha.Set(NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, 0, 0, false, false, false);
			C0.m_OutputRGB.Set(NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_TWO, 0, true, false, false);

			FC.m_InputRGB[0].Set(NV_INPUT_EF, 0, 0);
		//	FC.m_InputRGB[0].Set(NV_INPUT_SPARE0, 0, 0);
			FC.m_InputRGB[1].Set(NV_INPUT_PRIMARY, 0, 0);
		//	FC.m_InputRGB[1].Set(NV_INPUT_SPARE0, 0, NV_COMP_ALPHA);
			FC.m_InputRGB[2].Set(NV_INPUT_ZERO, 0, 0);
			FC.m_InputRGB[3].Set(NV_INPUT_ZERO, 0, 0);
			FC.m_InputRGB[4].Set(NV_INPUT_SPARE0, 0, 0);
			FC.m_InputRGB[5].Set(NV_INPUT_SPARE0, 0, NV_COMP_ALPHA);
			FC.m_InputAlpha.Set(NV_INPUT_SPARE0, 0, NV_COMP_ALPHA);
			pComb->m_NumCombiners = 1;
			pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		}

//		pComb->m_Color0 = _pL->m_IntensityInt32;
		_pVB->m_Color = _pL->m_IntensityInt32;

		pA->m_pExtAttrib = pComb;
	}

	if (bDestAlpha)
	{
		CXR_VertexBuffer* pVBLens = _pVBM->Alloc_VB(CXR_VB_ATTRIB);
		if (!pVBLens)
			return false;

		pVBLens->m_pTransform[0] = _pVB->m_pTransform[0];
		pVBLens->CRC_VertexBuffer::operator=(*_pVB);
		if (!pVBLens->SetVBChain(ms_pVBM, true)
			return false;
		pVBLens->Geometry_VertexBuffer(_pVB->m_VBID, _pVB->AllUsed());
		pVBLens->m_Priority = _pVB->m_Priority+0.001f;
			
		CRC_Attributes* pALens = pVBLens->m_pAttrib;
		pALens->SetDefault();
		pALens->Attrib_Enable(CRC_FLAGS_CULL);
		pALens->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
		pALens->Attrib_DestBlend(CRC_BLEND_ONE);
		pALens->Attrib_Enable(CRC_FLAGS_BLEND);

		for(int iTxt = 0; iTxt < 2; iTxt++)
		{
			CMTime TCurrent;
			TCurrent.Snapshot();
			CMTime Time = (TCurrent + CMTime::CreateFromSeconds(iTxt*36));

			::SetTexGenUVW(pVBLens, _pVBM, iTxt,
				CPlane3Dfp32(CVec3Dfp32(1, 0, 0), -_pL->m_TransformedPos[0]),
				CPlane3Dfp32(CVec3Dfp32(0, 1, 0), -_pL->m_TransformedPos[1]),
				CPlane3Dfp32(CVec3Dfp32(0, 0, 1), -_pL->m_TransformedPos[2]));

			CQuatfp32 Q;
			Q.k[0] = M_Sin(Time.GetTimeModulusScaled((1.0f + iTxt*0.5f)*0.25f, _PI2));
			Q.k[1] = M_Cos(Time.GetTimeModulusScaled((0.7f + iTxt*0.2f)*0.25f, _PI2));
			Q.k[2] = M_Sin(Time.GetTimeModulusScaled((1.3f - iTxt*0.2f)*0.25f, _PI2));
			Q.k[3] = 1.0f;
			Q.Normalize();

			CMat4Dfp32* pTV0Mat = _pVBM->Alloc_M4();
			if (!pTV0Mat)
				return false;
			Q.CreateMatrix(*pTV0Mat);

			pVBLens->Matrix_Set(pTV0Mat, CRC_MATRIX_TEXTURE0 + iTxt);
			pALens->Attrib_TextureID(iTxt, g_TextureID_CubeNoise);
		}

		// Alloc reg-combiners attribute
		CRC_ExtAttributes_NV10* pComb = (CRC_ExtAttributes_NV10*) _pVBM->Alloc(sizeof(CRC_ExtAttributes_NV10));
		if (!pComb) return false;

		pComb->Clear();
		CNV_RegCombiners_Combiner& C0 = pComb->m_Combiners[0];;
		CNV_RegCombiners_FinalCombiner& FC = pComb->m_FinalCombiner;

		C0.Clear();
		C0.m_InputRGB[0].Set(NV_INPUT_TEXTURE0, 0, 0);
		C0.m_InputRGB[1].Set(NV_INPUT_PRIMARY, 0, 0);
		C0.m_InputRGB[2].Set(NV_INPUT_TEXTURE1, 0, 0);
		C0.m_InputRGB[3].Set(NV_INPUT_PRIMARY, 0, 0);
//		C0.m_InputRGB[2].Set(NV_INPUT_ZERO, 0, 0);
//		C0.m_InputRGB[3].Set(NV_INPUT_ZERO, 0, 0);
		C0.m_OutputRGB.Set(NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, 0, false, false, false);

		FC.m_InputRGB[0].Set(NV_INPUT_SPARE0, 0, 0);
		FC.m_InputRGB[1].Set(NV_INPUT_ZERO, NV_MAPPING_INVERT, 0);
		FC.m_InputRGB[2].Set(NV_INPUT_ZERO, 0, 0);
		FC.m_InputRGB[3].Set(NV_INPUT_ZERO, 0, 0);
		FC.m_InputRGB[4].Set(NV_INPUT_ZERO, 0, 0);
		FC.m_InputRGB[5].Set(NV_INPUT_ZERO, 0, 0);
		FC.m_InputAlpha.Set(NV_INPUT_ZERO, NV_MAPPING_INVERT, NV_COMP_ALPHA);
		pComb->m_NumCombiners = 1;

//		pComb->m_Color0 = _pL->m_IntensityInt32;
		_pVB->m_Color = _pL->m_IntensityInt32;
		pALens->m_pExtAttrib = pComb;

		_pVBM->AddVB(pVBLens);
	}


	return true;
#endif

#endif
}

bool CXR_Model_BSP::Light_ProjectDynamic_MultiTexture(CXR_VertexBuffer* _pVB, int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_ProjectDynamic_MultiTexture, false);

#ifdef SUPERDUPERLJUS
	int CapsFlags = ms_pCurrentEngine->m_RenderCaps_Flags;
	if (CapsFlags & CRC_CAPS_FLAGS_CUBEMAP &&
		CapsFlags & CRC_CAPS_FLAGS_TEXENVMODE_DOT3 &&
		ms_pCurrentEngine->m_RenderCaps_nMultiTexture >= 2)
	{
/*		pA->Attrib_TextureID(0, ms_TextureID_Normalize);
		pA->Attrib_TextureID(1, ms_TextureID_Normalize);

		// TexCoord0 generation
		::SetTexGenUVW(pA, ms_pVBM, 0,
			CPlane3Dfp32(CVec3Dfp32(-1, 0, 0), _pL->m_TransformedPos[0]),
			CPlane3Dfp32(CVec3Dfp32(0, -1, 0), _pL->m_TransformedPos[1]),
			CPlane3Dfp32(CVec3Dfp32(0, 0, -1), _pL->m_TransformedPos[2]));

		// TexCoord1 generation
		pA->Attrib_TexGen(1, CRC_TEXGENMODE_NORMALMAP);

		pA->Attrib_TexEnvMode(0, CRC_TEXENVMODE_REPLACE);
		pA->Attrib_TexEnvMode(1, CRC_TEXENVMODE_DOT3);
		
		// Transform NORMALMAP back to model space
		CMat4Dfp32* pM = ms_pVBM->Alloc_M4();
		if (!pM)
			return false;

		*pM = m_pView->m_CurVMatInv;
		CVec3Dfp32::GetRow(*pM, 3) = 0;

		_pVB->Matrix_Set(pM, CRC_MATRIX_TEXTURE1);
		

		// Attenuation generation
		Light_SetAttenuation(pA, _pL, 2);
*/

	#ifndef MODEL_BSP_NOATTRSHARING
		CRC_Attributes* pADot3 = ms_Render_lpAttribDynLight[_iDynamic];
		CRC_Attributes* pAAttn = ms_Render_lpAttribDynLight_DestAlphaExpAttn[_iDynamic];

		if (pADot3 && pAAttn)
		{
			_pVB->m_pAttrib = pAAttn;
			_pVB->m_Color = _pL->m_IntensityInt32;

			// Render Dot3 into destination alpha
			CXR_VertexBuffer* pVBDot3 = ms_pVBM->Alloc_VB();
			if (!pVBDot3)
				return false;

			pVBDot3->m_pAttrib = pADot3;

			pVBDot3->m_pTransform[0] = _pVB->m_pTransform[0];
			pVBDot3->CRC_VertexBuffer::operator=(*_pVB);
			if (!pVBDot3->SetVBChain(ms_pVBM, true)
				return false;
			pVBDot3->Geometry_VertexBuffer(_pVB->m_VBID, _pVB->AllUsed());
			pVBDot3->m_Priority = _pVB->m_Priority-0.001f;
			ms_pVBM->AddVB(pVBDot3);

			return true;
		}
	#endif

		{
			fp32 LightPriorOffs = fp32(_iDynamic)*0.01f;


			// Render dot3 into destination alpha
			CXR_VertexBuffer* pVBDot3 = ms_pVBM->Alloc_VB(CXR_VB_ATTRIB);
			if (!pVBDot3)
				return false;

			CRC_Attributes* pADot3 = pVBDot3->m_pAttrib;

			pVBDot3->m_pTransform[0] = _pVB->m_pTransform[0];
			pVBDot3->CRC_VertexBuffer::operator=(*_pVB);
			if (!pVBDot3->SetVBChain(ms_pVBM, true)
				return false;
			pVBDot3->Geometry_VertexBuffer(_pVB->m_VBID, _pVB->AllUsed());
			pVBDot3->m_Priority = _pVB->m_Priority-0.001f;

			pADot3->Attrib_TextureID(0, ms_TextureID_Normalize);
			pADot3->Attrib_TextureID(1, ms_TextureID_Normalize);

			// TexCoord0 generation
			::SetTexGenUVW(pADot3, ms_pVBM, 0,
				CPlane3Dfp32(CVec3Dfp32(-1, 0, 0), _pL->m_TransformedPos[0]),
				CPlane3Dfp32(CVec3Dfp32(0, -1, 0), _pL->m_TransformedPos[1]),
				CPlane3Dfp32(CVec3Dfp32(0, 0, -1), _pL->m_TransformedPos[2]));

			// TexCoord1 generation
			pADot3->Attrib_TexGen(1, CRC_TEXGENMODE_NORMALMAP);

			pADot3->Attrib_TexEnvMode(0, CRC_TEXENVMODE_MULTIPLY);
			pADot3->Attrib_TexEnvMode(1, CRC_TEXENVMODE_DOT3);
			pADot3->Attrib_Disable(/*CRC_FLAGS_COLORWRITE |*/ CRC_FLAGS_ZWRITE);
			
			// Transform NORMALMAP back to model space
			CMat4Dfp32* pM = ms_pVBM->Alloc_M4();
			if (!pM)
				return false;

			*pM = m_pView->m_CurVMatInv;
			CVec3Dfp32::GetRow(*pM, 3) = 0;

			_pVB->Matrix_Set(pM, CRC_MATRIX_TEXTURE1);

			ms_pVBM->AddVB(pVBDot3);
	#ifndef MODEL_BSP_NOATTRSHARING
			ms_Render_lpAttribDynLight[_iDynamic] = pADot3;
	#endif
		}

		{
			CRC_Attributes* pA = ms_pVBM->Alloc_Attrib();
			if (!pA)
				return false;
			pA->SetDefault();

//			Light_SetAttenuation(pA, _pL, 0);
			CPlane3Dfp32* pTexGenAttr = (CPlane3Dfp32*) m_pVBM->Alloc_fp32(4 * 4);
			if (!pTexGenAttr)
				return;
			const CVec3Dfp32& Pos= _pL->GetPosition();
			fp32 Range = _pL->m_Range;
			fp32 Offset = 0.5f;
			pTexGenAttr[0].Create(CVec3Dfp32(0.5f / Range,0,0), Offset - 0.5f* Pos[0] / Range);
			pTexGenAttr[1].Create(CVec3Dfp32(0, 0.5f / Range,0), Offset - 0.5f* Pos[1] / Range);
			pTexGenAttr[2].Create(CVec3Dfp32(0, 0, 0.5f / Range), Offset - 0.5f* Pos[2] / Range);
			pTexGenAttr[3].Create(CVec3Dfp32(0, 0, 0), Offset);
			pA->Attrib_TexGen(0, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
			pA->Attrib_TexGen(1, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
			pA->Attrib_TexGenAttr(pTexGenAttr);
			pA->Attrib_TextureID(0, ms_TextureID_AttenuationExp);
			pA->Attrib_TextureID(1, ms_TextureID_AttenuationExp);

			pA->Attrib_Enable(CRC_FLAGS_CULL | CRC_FLAGS_BLEND);
			pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
			pA->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
			pA->Attrib_DestBlend(CRC_BLEND_ONE);

			_pVB->m_pAttrib = pA;
			_pVB->m_Color = _pL->m_IntensityInt32;
	#ifndef MODEL_BSP_NOATTRSHARING
			ms_Render_lpAttribDynLight_DestAlphaExpAttn[_iDynamic] = pA;
	#endif
		}
	}
	else
#endif
	{
		CRC_Attributes* pA = ms_Render_lpAttribDynLight[_iDynamic];
	#ifndef MODEL_BSP_NOATTRSHARING
		if (pA)
		{
			_pVB->m_pAttrib = pA;
//			_pVB->m_Color = _pL->m_IntensityInt32;
			M_VSt_V4f32_Pixel32(M_VMul(M_VScalar(255.0f), _pL->GetIntensityv().v), &_pVB->m_Color);
			return true;
		}
	#endif

		pA = ms_pVBM->Alloc_Attrib();
		if (!pA)
			return false;
		pA->SetDefault();

//		Light_SetAttenuation(pA, _pL, 0);
		CPlane3Dfp32* pTexGenAttr = (CPlane3Dfp32*) ms_pVBM->Alloc_fp32(4 * 4);
		if (!pTexGenAttr)
			return false;
		const CVec3Dfp32& Pos= _pL->GetPosition();
		fp32 Range = _pL->m_Range;
		fp32 Offset = 0.5f;
		pTexGenAttr[0].CreateNV(CVec3Dfp32(0.5f / Range,0,0), Offset - 0.5f* Pos[0] / Range);
		pTexGenAttr[1].CreateNV(CVec3Dfp32(0, 0.5f / Range,0), Offset - 0.5f* Pos[1] / Range);
		pTexGenAttr[2].CreateNV(CVec3Dfp32(0, 0, 0.5f / Range), Offset - 0.5f* Pos[2] / Range);
		pTexGenAttr[3].CreateNV(CVec3Dfp32(0, 0, 0), Offset);
		pA->Attrib_TexGen(0, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
		pA->Attrib_TexGen(1, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
		pA->Attrib_TexGenAttr((fp32*)pTexGenAttr);
		pA->Attrib_TextureID(0, ms_TextureID_AttenuationExp);
		pA->Attrib_TextureID(1, ms_TextureID_AttenuationExp);

		M_VSt_V4f32_Pixel32(M_VMul(M_VScalar(255.0f), _pL->GetIntensityv().v), &_pVB->m_Color);
//		_pVB->m_Color = _pL->m_IntensityInt32;
		pA->Attrib_Enable(CRC_FLAGS_CULL);
		pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);

		_pVB->m_pAttrib = pA;
	#ifndef MODEL_BSP_NOATTRSHARING
		ms_Render_lpAttribDynLight[_iDynamic] = pA;
	#endif
	}

	return true;
}
