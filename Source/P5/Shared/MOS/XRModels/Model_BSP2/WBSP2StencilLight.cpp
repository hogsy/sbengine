#include "PCH.h"

#include "WBSP2Model.h"
#include "MFloat.h"

#include "../../XR/XRShader.h"

#define MODEL_BSP2_MAX_LIT_FACES 2048

#define MODEL_BSP2_MAXDYNAMICLIGHTFACES 1024
#define MODEL_BSP2_MAX_LIT_EDGES 4096

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP2_StencilLight
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP2_StencilLight::CBSP2_StencilLight()
{
	m_LightGUID = -1;
	m_ShadowCastRange = 0.0f;
	m_iTriVolumeCap = 0;
	m_nTriVolumeCap = 0;
	m_bVBValid = false;
	m_VBID = 0;
	m_FramesUnchanged = 0;
}

CBSP2_StencilLight::~CBSP2_StencilLight()
{
	if (m_VBID)
	{
		m_pVBCtx->FreeID(m_VBID);
		m_VBID = NULL;
	}
}

bool CBSP2_StencilLight::LightChanged(const CXR_Light& _Light) const
{
	return 
		(_Light.m_LightGUID != m_LightGUID) || 
		(_Light.m_TransformedPos.DistanceSqr(m_Light.m_TransformedPos) > 0.0001f);
}

void CBSP2_StencilLight::Create()
{
	m_VBID = m_pVBCtx->AllocID(m_iVBC, 0);
}

void CBSP2_StencilLight::Clear()
{
	m_spSurfCluster = NULL;
	m_lV.Clear();
	m_liPrimTri.Clear();
}

int CBSP2_StencilLight::CalcOcclusionLevel(const CVec3Dfp32& _P) const
{
	return 0;

	const uint16* piPrim = &m_liPrimTri[m_iTriVolumeCap];
	const CVec3Dfp32* pV = m_lV.GetBasePtr();
	const CVec3Dfp32* pVOrg = m_lVOrg.GetBasePtr();

	int nOcclusions = 0;

	for(int t = 0; t < m_nTriVolumeCap; t++)
	{
		int iv0 = *piPrim++;
		int iv1 = *piPrim++;
		int iv2 = *piPrim++;

		// Calc normal
		CVec3Dfp32 E0,E1,E2,N;
		pV[iv1].Sub(pV[iv0], E0);
		pV[iv2].Sub(pV[iv1], E1);
		pV[iv0].Sub(pV[iv2], E2);
		E0.CrossProd(E1, N);

		// Check cap plane
		{
			CVec3Dfp32 V;
			_P.Sub(pV[iv0], V);
			if (V * N < 0.0f) continue;
		}

		// Plane E0
		const CVec3Dfp32& LPos = m_Light.m_TransformedPos;
		CVec3Dfp32 LV;
		_P.Sub(LPos, LV);
		{
			CVec3Dfp32 EN;
			(pV[iv0] - LPos).CrossProd(E0, EN);
			if (LV * EN > 0.0f) continue;
		}
		{
			CVec3Dfp32 EN;
			(pV[iv1] - LPos).CrossProd(E1, EN);
			if (LV * EN > 0.0f) continue;
		}
		{
			CVec3Dfp32 EN;
			(pV[iv2] - LPos).CrossProd(E2, EN);
			if (LV * EN > 0.0f) continue;
		}

/*		CVec3Dfp32 LV0,LV1,LV2;
		LPos.Sub(pVOrg[iv0 + m_VOrgIndexOffset], LV0);
		LPos.Sub(pVOrg[iv1 + m_VOrgIndexOffset], LV1);
		LPos.Sub(pVOrg[iv2 + m_VOrgIndexOffset], LV2);
*/
		{
			CVec3Dfp32 E0,E1,N;
			pVOrg[iv1 + m_VOrgIndexOffset].Sub(pVOrg[iv0 + m_VOrgIndexOffset], E0);
			pVOrg[iv2 + m_VOrgIndexOffset].Sub(pVOrg[iv0 + m_VOrgIndexOffset], E1);
			E0.CrossProd(E1, N);

			// Check cap plane
			CVec3Dfp32 V;
			_P.Sub(pVOrg[iv0 + m_VOrgIndexOffset], V);
			if (V * N > 0.0f) continue;
		}

		nOcclusions++;
	}

	return nOcclusions;
}

void CBSP2_StencilLight::MakeDirty()
{
	if (m_VBID)
		m_pVBCtx->VB_MakeDirty(m_VBID);
}

int CBSP2_StencilLight::GetNumLocal()
{
	return 1;
}

int CBSP2_StencilLight::GetID(int _iLocal)
{
	return m_VBID;
}

void CBSP2_StencilLight::Get(int _iLocal, CRC_VertexBuffer& _VB, int _Flags)
{
	CRC_VertexBuffer VB;
	VB.m_pV = m_lV.GetBasePtr();
	VB.m_nV = m_lV.Len();
	VB.m_piPrim = m_liPrimTri.GetBasePtr();
	VB.m_nPrim = m_liPrimTri.Len() / 3;
	VB.m_PrimType = CRC_RIP_TRIANGLES;

	_VB = VB;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP2::StencilLight_xxxxxx
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP2_StencilLight* CXR_Model_BSP2::StencilLight_Get(int _LightGUID)
{
	// FIXME: Do some hash stuff.

	if (_LightGUID >= m_lspStencilLights.Len())
		m_lspStencilLights.SetLen(_LightGUID+1);

	if (!m_lspStencilLights[_LightGUID])
	{
		m_lspStencilLights[_LightGUID] = DNew(CBSP2_StencilLight) CBSP2_StencilLight;
		if (!m_lspStencilLights[_LightGUID])
			MemError("StencilLight_Get");

		m_lspStencilLights[_LightGUID]->Create();
	}

	return m_lspStencilLights[_LightGUID];

/*	if (m_lStencilLights.ValidPos(_LightGUID))
		return &m_lStencilLights[_LightGUID];
	else
		return NULL;*/
}

#define SLLog(s)

void CXR_Model_BSP2::StencilLight_CreateCluster(CXR_Light& _Light, CBSP2_StencilLight_SurfaceCluster& _Cluster, uint32* _piFaces, int _nFaces, int _iSurface)
{
	_Cluster.m_liFaces.SetLen(_nFaces);
	_Cluster.m_iSurface = _iSurface;
	memcpy(_Cluster.m_liFaces.GetBasePtr(), _piFaces, sizeof(uint32) * _nFaces);

	int nV;
	int nPrim;
	RenderGetPrimCount(_piFaces, _nFaces, nV, nPrim);
#ifdef STENCILLIGHT_THINARRAY
	_Cluster.m_lV.SetLen(nV);
	_Cluster.m_lN.SetLen(nV);
	_Cluster.m_lTangU.SetLen(nV);
	_Cluster.m_lTangV.SetLen(nV);
	_Cluster.m_lTV0.SetLen(nV);
	_Cluster.m_lTV1.SetLen(nV);
	_Cluster.m_liPrim.SetLen(nPrim);
#else
	_Cluster.m_lV.QuickSetLen(nV);
	_Cluster.m_lN.QuickSetLen(nV);
	_Cluster.m_lTangU.QuickSetLen(nV);
	_Cluster.m_lTangV.QuickSetLen(nV);
	_Cluster.m_lTV0.QuickSetLen(nV);
	_Cluster.m_lTV1.QuickSetLen(nV);
	_Cluster.m_liPrim.QuickSetLen(nPrim);
#endif

	Tesselate(_piFaces, _nFaces, nV, _Cluster.m_lV.GetBasePtr(), _Cluster.m_lTV1.GetBasePtr(), NULL, NULL, _Cluster.m_lN.GetBasePtr(), NULL, _Cluster.m_lTangU.GetBasePtr(), _Cluster.m_lTangV.GetBasePtr());
	int nP = TesselatePrim(_piFaces, _nFaces, _Cluster.m_liPrim.GetBasePtr());

	if (nP != _Cluster.m_liPrim.Len())
		Error("StencilLight_CreateCluster", "Internal error.");

	// Transform lightvector into tangent space.
	{
		const CVec3Dfp32* pV = _Cluster.m_lV.GetBasePtr();
		const CVec3Dfp32* pN = _Cluster.m_lN.GetBasePtr();
		const CVec3Dfp32* pTangU = _Cluster.m_lTangU.GetBasePtr();
		const CVec3Dfp32* pTangV = _Cluster.m_lTangV.GetBasePtr();
		CVec3Dfp32* pTV0 = _Cluster.m_lTV0.GetBasePtr();

		for(int v = 0; v < nV; v++)
		{
			CVec3Dfp32 lv;
			_Light.m_TransformedPos.Sub(pV[v], lv);

			const CVec3Dfp32& N = pN[v];
			const CVec3Dfp32& TgUMap = pTangU[v];
			const CVec3Dfp32& TgVMap = pTangV[v];

			CVec3Dfp32 TgV, TgU;
			TgUMap.CrossProd(N, TgV);
			N.CrossProd(TgVMap, TgU);
			TgU.Normalize();
			TgV.Normalize();
			TgU *= Sign(TgU * TgUMap);
			TgV *= Sign(TgV * TgVMap);

			fp32 x = lv[0]*TgU[0] + lv[1]*TgU[1] + lv[2]*TgU[2];
			fp32 y = lv[0]*TgV[0] + lv[1]*TgV[1] + lv[2]*TgV[2];
			fp32 z = lv[0]*N[0] + lv[1]*N[1] + lv[2]*N[2];

			pTV0[v] = CVec3Dfp32(x,y,z);	
		}
	}
}


void CXR_Model_BSP2::StencilLight_Create(CXR_Light& _Light)
{
	if (_Light.m_Type != CXR_LIGHTTYPE_POINT) return;

	CBSP2_StencilLight* pSL = StencilLight_Get(_Light.m_LightGUID);
	if (!pSL) return;
	if (!pSL->LightChanged(_Light))
	{
		if (pSL->m_FramesUnchanged < 0x7fffffff)
			pSL->m_FramesUnchanged++;
		if (pSL->m_FramesUnchanged > 4)
			pSL->m_bVBValid = true;
		return;
	}

SLLog(CStrF("(CXR_Model_BSP2::StencilLight_Create) GUID %d, %s, %s", _Light.m_LightGUID, (char*)_Light.m_Pos.GetString(), (char*)_Light.m_TransformedPos.GetString() ));

	pSL->m_LightGUID = _Light.m_LightGUID;
	pSL->m_Light = _Light;

	uint32 lFaces[MODEL_BSP2_MAX_LIT_FACES];
	uint32 lFaces2[MODEL_BSP2_MAX_LIT_FACES];
//	uint32 lFacesLit[MODEL_BSP2_MAX_LIT_FACES];

	CBox3Dfp32 Box;
	Box.m_Min = _Light.m_TransformedPos;
	Box.m_Max = _Light.m_TransformedPos;
	Box.m_Min -= _Light.m_Range;
	Box.m_Max += _Light.m_Range;

	EnumFaces_Untag();

	int nFaces = EnumFaces_Box(1, lFaces, MODEL_BSP2_MAX_LIT_FACES, ENUM_HQ | ENUM_FACEFLAGS, 0, XW_FACE_VISIBLE, Box);
	if (!nFaces)
	{
		pSL->Clear();
		return;
	}

	memcpy(lFaces2, lFaces, nFaces*sizeof(uint32));
	int nFaces2 = nFaces;

	// Cull faces that are not to recieve any light.
/*	int nFacesLit = 0;
	{
		for(int i = 0; i < nFaces; i++)
		{
			int iFace = lFaces[i];
			CBSP2_Face& pF = m_pFaces[iFace];
			if (m_lspSurfaces[pF.m_iSurface]->m_Flags & XW_SURFFLAGS_LIGHTING)
				lFacesLit[nFacesLit++] = iFace;
		}
	}*/

//	if (nFaces >= MODEL_BSP2_MAX_LIT_FACES)
//		Error("StencilLight_Create", "Too many faces.");

SLLog(CStr("(CXR_Model_BSP2::StencilLight_Create) Progress 1"));

	// Init edge-tag
	if (!m_lEdgeTag.Len())
	{
		m_lEdgeTag.SetLen((m_lEdges.Len() + 7) >> 3);
		FillChar(m_lEdgeTag.GetBasePtr(), m_lEdgeTag.ListSize(), 0);
	}
	uint8* pET = m_lEdgeTag.GetBasePtr();
//FillChar(pET, m_lEdgeTag.ListSize(), 0);

	// Get all unique edges from front-facing faces.
	uint32 liEdges[MODEL_BSP2_MAX_LIT_EDGES];
	int nEdges = 0;

	EnumFaces_Untag();
	uint8* pFTag = m_lFaceTagList.GetBasePtr();
//FillChar(pFTag, m_lFaceTagList.ListSize(), 0);

	uint32* piFUntag = m_lFaceUntagList.GetBasePtr();
	if (m_lFaceTagList.Len()*8 < m_lFaces.Len() ||
		m_lFaceUntagList.Len() < m_lFaces.Len())
		Error("StencilLight_Create", "Tag array not initialized.");

	int nFaceTri = 0;
	int nFaceVert = 0;
	bool bError = false;
	{
SLLog(CStr("(CXR_Model_BSP2::StencilLight_Create) Progress 2"));
		int nFacesNew = 0;
		for(int f = 0; f < nFaces; f++)
		{
			int iFace = lFaces[f];
			CBSP2_Face* pF = &m_lFaces[iFace];

			// Cull faces backfacing the lightsource.
			if (m_lPlanes[pF->m_iPlane].Distance(_Light.m_TransformedPos) > 0.0f) continue;
			if (pF->m_iiEdges == -1) continue;

			lFaces[nFacesNew++] = iFace;
			nFaceTri += pF->m_nVertices-2;
			nFaceVert += pF->m_nVertices;

			// Tag face
			if (!pFTag[iFace >> 3])
				piFUntag[ms_nEnumUntag++] = iFace;
			pFTag[iFace >> 3] |= 1 << (iFace & 7);

			for(int e = 0; e < pF->m_nVertices; e++)
			{
				int iE = m_liEdges[pF->m_iiEdges + e];
				if (pET[iE >> 3] & (1 << (iE & 7))) continue;
				pET[iE >> 3] |= 1 << (iE & 7);

				if (nEdges >= MODEL_BSP2_MAX_LIT_EDGES)
				{
					ConOut(CStrF("§cf80WARNING: (CXR_Model_BSP2::StencilLight_Create) Too many edges. (Face %d/%d)", f, nFaces));
					bError = true;
					break;
				}
//					Error("StencilLight_Create", "Too many edges.");

				liEdges[nEdges++] = iE;
			}

			if (bError) break;
		}

		nFaces = nFacesNew;
	}
//#ifdef NEVER
SLLog(CStr("(CXR_Model_BSP2::StencilLight_Create) Progress 3"));

	// Clear edge tags
	{
		for(int e = 0; e < nEdges; e++)
			pET[liEdges[e] >> 3] = 0;
	}

	// Remove all edges that have both it's faces front-facing.
	{
		int nEdgesCulled= 0;
		for(int e = 0; e < nEdges; e++)
		{
			int iE = liEdges[e];
			pET[iE >> 3] = 0;

			CBSP_EdgeFaces& EF = m_lEdgeFaces[iE];
			int Face0 = EF.IsValid(0) && (pFTag[EF.GetFace(0) >> 3] & (1 << (EF.GetFace(0) & 7)));
			int Face1 = EF.IsValid(1) && (pFTag[EF.GetFace(1) >> 3] & (1 << (EF.GetFace(1) & 7)));

			Face0 = (Face0) ? 1 : 0;
			Face1 = (Face1) ? 1 : 0;

// This should never happen, and that seems to work so we can skip the test.
//			if (!(Face0 || Face1)) Error("StencilLight_Create", "Internal error.");

			if (Face0 ^ Face1)
				liEdges[nEdgesCulled++] = iE;
		}

		nEdges = nEdgesCulled;
	}

SLLog(CStr("(CXR_Model_BSP2::StencilLight_Create) Progress 4"));
	// Create geometry
	CVec3Dfp32 LightPos = _Light.m_TransformedPos;
	pSL->m_ShadowCastRange = _Light.m_Range * 1.25f;

	int nPrim = 3 * (nFaceTri*2 + nEdges*2);
	int nV = nEdges*4 + nFaceVert*2;

#ifdef STENCILLIGHT_THINARRAY
	pSL->m_lV.SetLen(nV);
	pSL->m_liPrimTri.SetLen(nPrim);
//	pSL->m_lVOrg.SetLen(nFaceVert);
#else
	pSL->m_lV.QuickSetLen(nV);
	pSL->m_liPrimTri.QuickSetLen(nPrim);
//	pSL->m_lVOrg.QuickSetLen(nFaceVert);
#endif
	CVec3Dfp32* pV = pSL->m_lV.GetBasePtr();
//	CVec3Dfp32* pVOrg = pSL->m_lVOrg.GetBasePtr();
	uint16* piPrimTri = pSL->m_liPrimTri.GetBasePtr();
	int nP = 0;

	{
		for(int e = 0; e < nEdges; e++)
		{
			int iE = liEdges[e];
			CBSP_Edge& E = m_lEdges[iE];
			CBSP_EdgeFaces& EF = m_lEdgeFaces[iE];

			int iv0 = E.m_iV[0];
			int iv1 = E.m_iV[1];
			int Face0 = (pFTag[EF.GetFace(0) >> 3] & (1 << (EF.GetFace(0) & 7)));
			if (Face0) Swap(iv0, iv1);

			pV[e*4] = m_lVertices[iv0];
			pV[e*4+1] = m_lVertices[iv1];

/* Render face with wireframe.
CVec3Dfp32 N(0);
if (EF.IsValid(0)) N += m_lPlanes[m_lFaces[EF.GetFace(0)].m_iPlane].n;
if (EF.IsValid(1)) N += m_lPlanes[m_lFaces[EF.GetFace(1)].m_iPlane].n;
m_pCurrentRC->Render_Wire(pV[e*4] + (N*1.0f), pV[e*4+1] + N*1.0f, 0xff008000);
*/

			CVec3Dfp32 dV0, dV1;
			pV[e*4].Sub(LightPos, dV0);
			pV[e*4+1].Sub(LightPos, dV1);
			LightPos.Combine(dV0, pSL->m_ShadowCastRange / dV0.Length(), pV[e*4 + 2]);
			LightPos.Combine(dV1, pSL->m_ShadowCastRange / dV1.Length(), pV[e*4 + 3]);

			piPrimTri[nP++] = e*4;
			piPrimTri[nP++] = e*4+1;
			piPrimTri[nP++] = e*4+3;

			piPrimTri[nP++] = e*4+0;
			piPrimTri[nP++] = e*4+3;
			piPrimTri[nP++] = e*4+2;
		}
SLLog(CStr("(CXR_Model_BSP2::StencilLight_Create) Progress 5"));

		pSL->m_iTriVolumeCap = nP;
		pSL->m_nTriVolumeCap = 0;

//		CVec3Dfp32* pOrgV = pSL->m_lVOrg.GetBasePtr();

		int iV = nEdges*4;
//		int iVOrg = 0;

//		pSL->m_VOrgIndexOffset = iVOrg-iV;

		for(int f = 0; f < nFaces; f++)
		{
			int iFace = lFaces[f];
			CBSP2_Face* pF = &m_lFaces[iFace];
			uint32* piV = &m_liVertices[pF->m_iiVertices];

	CVec3Dfp32 N = m_lPlanes[pF->m_iPlane].n;

			int nv = pF->m_nVertices;
			for(int v = 0; v < nv; v++)
			{
				const CVec3Dfp32& V = m_lVertices[piV[v]];

// m_pCurrentRC->Render_Wire(V + (N*2.0f), m_lVertices[piV[(v+1) % nv]] + N*2.0f, 0xff008000);

//				pVOrg[iVOrg + v] = V;
				pV[iV + v + nv] = V;

				CVec3Dfp32 dV0;
				V.Sub(LightPos, dV0);
				LightPos.Combine(dV0, pSL->m_ShadowCastRange / dV0.Length(), pV[iV + v]);

				if (v > 1)
				{
					piPrimTri[nP++] = iV;
					piPrimTri[nP++] = iV + v;
					piPrimTri[nP++] = iV + v-1;

					piPrimTri[nP++] = iV + nv;
					piPrimTri[nP++] = iV + nv + v-1;
					piPrimTri[nP++] = iV + nv + v;
					pSL->m_nTriVolumeCap++;
				}
			}
			iV += nv*2;
//			iVOrg += nv;
		}

SLLog(CStr("(CXR_Model_BSP2::StencilLight_Create) Progress 6"));
		// Check that we didnt write more or less than we planned for in the beginning.
		if (iV != pSL->m_lV.Len())
			Error("StencilLight_Create", "Internal error. (1)");
//		if (iVOrg != pSL->m_lVOrg.Len())
//			Error("StencilLight_Create", "Internal error. (2)");
		if (nP != pSL->m_liPrimTri.Len())
			Error("StencilLight_Create", "Internal error. (3)");
	}
//#endif
	// Clear face tags
	EnumFaces_Untag();

SLLog(CStr("(CXR_Model_BSP2::StencilLight_Create) Progress 7"));
	
SLLog(CStrF("(CXR_Model_BSP2::StencilLight_Create) End GUID %d", _Light.m_LightGUID));

	// Create clusters, destroys lFaces2
	{
		uint32 lSurfFaces[MODEL_BSP2_MAX_LIT_FACES];
		CBSP2_Face* pFaces = m_lFaces.GetBasePtr();

		if (!pSL->m_spSurfCluster)
			pSL->m_spSurfCluster = DNew(CBSP2_StencilLight_SurfaceCluster) CBSP2_StencilLight_SurfaceCluster;
		if (!pSL->m_spSurfCluster) MemError("StencilLight_Create");

		CBSP2_StencilLight_SurfaceCluster* pCluster = pSL->m_spSurfCluster;

// ConOut(CStrF("(CXR_Model_BSP2::StencilLight_Create) FACES %d", nFaces2));

		while(nFaces2)
		{
			int nSurfFaces = 0;
			int nFacesLeft = 0;

			int iiFace = 0;
			int iSurface = pFaces[lFaces2[iiFace]].m_iSurface;
			lSurfFaces[nSurfFaces++] = lFaces2[iiFace];
			iiFace++;
			while(iiFace < nFaces2)
			{
				if (iSurface == pFaces[lFaces2[iiFace]].m_iSurface)
				{
					lSurfFaces[nSurfFaces++] = lFaces2[iiFace];
				}
				else
				{
					lFaces2[nFacesLeft++] = lFaces2[iiFace];
				}

				iiFace++;
			}

			nFaces2 = nFacesLeft;

			StencilLight_CreateCluster(_Light, *pCluster, lSurfFaces, nSurfFaces, iSurface);

/*ConOut(CStrF("         Faces %d, Surf %d", nSurfFaces, iSurface));
for(int i = 0; i < nSurfFaces; i++)
	ConOut(CStrF("                 Face %d, Surf %d", lSurfFaces[i], pFaces[lSurfFaces[i]].m_iSurface));*/

			if (nFaces2)
			{
				if (!pCluster->m_spNext)
					pCluster->m_spNext = DNew(CBSP2_StencilLight_SurfaceCluster) CBSP2_StencilLight_SurfaceCluster;
				if (!pCluster->m_spNext) MemError("StencilLight_Create");

				pCluster = pCluster->m_spNext;
			}
		}

		// Invalidate all remaining clusters
		while(pCluster->m_spNext != NULL)
		{
			pCluster = pCluster->m_spNext;
			pCluster->m_iSurface = -1;
		}
	}

	pSL->MakeDirty();
	pSL->m_bVBValid = false;
}

void CXR_Model_BSP2::StencilLight_Create(CXR_WorldLightState& _WLS)
{
	for(CXR_Light* pL = _WLS.GetFirst(); pL; pL = pL->m_pNext)
		StencilLight_Create(*pL);
}


void CXR_Model_BSP2::StencilLight_Render(CXR_Light& _Light)
{
	CBSP2_StencilLight* pSL = StencilLight_Get(_Light.m_LightGUID);
	if (!pSL) return;

	// Render stencil shadowvolumes from this light
	if (pSL->m_liPrimTri.Len() && pSL->m_lV.Len() && (CXR_Model_BSP::ms_Enable & 512))
	{
//		int Occlusion = pSL->CalcOcclusionLevel(m_pView->m_CurLocalVP);
//		ConOut(CStrF("Occlusions %d (LightID %d)", Occlusion, _Light.m_LightGUID));
//		m_pCurrentEngine->SetVar(XR_ENGINE_UNIFIED_ADDLIGHTOCCLUSION, (_Light.m_LightGUID & 0xffff) + (Occlusion << 16));


		CXR_VertexBuffer* pVBFront = m_pVBM->Alloc_VB(CXR_VB_ATTRIB);
		CXR_VertexBuffer* pVBBack = m_pVBM->Alloc_VB(CXR_VB_ATTRIB);
		CRC_Attributes* pAF = pVBFront->m_pAttrib;
		CRC_Attributes* pAB = pVBBack->m_pAttrib;

		pAF->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_COLORWRITE | CRC_FLAGS_CULLCCW);
		pAF->Attrib_Enable(CRC_FLAGS_STENCIL | CRC_FLAGS_CULL);
		pAF->Attrib_ZCompare(CRC_COMPARE_GREATER);
		pAF->Attrib_StencilFunction(CRC_COMPARE_ALWAYS, 1, 255);
		pAF->Attrib_StencilOp(CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_DEC);
		pAF->Attrib_StencilWriteMask(255);

		pAB->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_COLORWRITE);
		pAB->Attrib_Enable(CRC_FLAGS_STENCIL | CRC_FLAGS_CULL | CRC_FLAGS_CULLCCW);
		pAB->Attrib_ZCompare(CRC_COMPARE_GREATER);
		pAB->Attrib_StencilFunction(CRC_COMPARE_ALWAYS, 1, 255);
		pAB->Attrib_StencilOp(CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_INC);
		pAB->Attrib_StencilWriteMask(255);

		pVBFront->m_Priority = _Light.m_LightGUID + 0.1f;
		pVBBack->m_Priority = _Light.m_LightGUID + 0.2f;

		if (pSL->m_bVBValid && pSL->m_VBID && (CXR_Model_BSP::ms_Enable & 1024))
		{
			pVBFront->Render_VertexBuffer(pSL->m_VBID);
			pVBBack->Render_VertexBuffer(pSL->m_VBID);
//ConOut(CStrF("VB Light %d, Tris %d", _Light.m_LightGUID, pSL->m_liPrimTri.Len()/3*2));
		}
		else
		{
			pVBFront->Geometry_VertexArray(pSL->m_lV.GetBasePtr(), pSL->m_lV.Len());
			pVBBack->Geometry_VertexArray(pSL->m_lV.GetBasePtr(), pSL->m_lV.Len());
			pVBFront->Render_IndexedTriangles(pSL->m_liPrimTri.GetBasePtr(), pSL->m_liPrimTri.Len() / 3);
			pVBBack->Render_IndexedTriangles(pSL->m_liPrimTri.GetBasePtr(), pSL->m_liPrimTri.Len() / 3);
//ConOut(CStrF("Immediate Light %d, Tris %d", _Light.m_LightGUID, pSL->m_liPrimTri.Len()/3*2));
		}


		pVBFront->Matrix_Set(m_pVBMatrixM2V);
		pVBBack->Matrix_Set(m_pVBMatrixM2V);

		m_pVBM->AddVB(pVBFront);
		m_pVBM->AddVB(pVBBack);
	}

	CXR_Shader Shader;
	Shader.Create(m_pCurrentEngine);

	CBSP2_StencilLight_SurfaceCluster* pCluster = pSL->m_spSurfCluster;
	while(pCluster)
	{
//ConOut(CStrF("(CXR_Model_BSP2::StencilLight_Render) Cluster surface %d", pCluster->m_iSurface));
		if (pCluster->m_iSurface < 0) break;

		CXR_VertexBuffer VB;
		VB.Geometry_VertexArray(pCluster->m_lV.GetBasePtr(), pCluster->m_lV.Len());
		VB.Geometry_TVertexArray(pCluster->m_lTV0.GetBasePtr(), 0);
		VB.Geometry_TVertexArray(pCluster->m_lTV1.GetBasePtr(), 1);
/*		VB.Geometry_NormalArray(pCluster->m_lN.GetBasePtr());
		VB.Geometry_TVertexArray(pCluster->m_lTangU.GetBasePtr(), 2);
		VB.Geometry_TVertexArray(pCluster->m_lTangV.GetBasePtr(), 3);*/
		VB.Render_IndexedPrimitives(pCluster->m_liPrim.GetBasePtr(), pCluster->m_liPrim.Len());
		VB.Matrix_Set(m_pVBMatrixM2V);

		Shader.RenderShading(_Light, &VB, m_lspSurfaces[pCluster->m_iSurface]);

		pCluster = pCluster->m_spNext;
	}
}

void CXR_Model_BSP2::StencilLight_Render(CXR_WorldLightState& _WLS)
{
	EnumFaces_Untag();

	_WLS.InitLinks();
	StencilLight_Create(_WLS);

	for(CXR_Light* pL = _WLS.GetFirst(); pL; pL = pL->m_pNext)
		StencilLight_Render(*pL);

	EnumFaces_Untag();
}

