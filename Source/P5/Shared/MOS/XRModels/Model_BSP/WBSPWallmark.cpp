#include "PCH.h"

#include "WBSPModel.h"
#include "../../XR/XREngineVar.h"
#include "MFloat.h"
#include "../../Classes/Render/MWireContainer.h"

CBSP_WallmarkContext::CBSP_WallmarkContext()
{
	MAUTOSTRIP(CBSP_WallmarkContext_ctor, MAUTOSTRIP_VOID);
	m_nV = 0;
	m_pSC = NULL;
}

void CBSP_WallmarkContext::Create(CXR_Model_BSP* _pModel, int _nV)
{
	MAUTOSTRIP(CBSP_WallmarkContext_Create, MAUTOSTRIP_VOID);
	MSCOPE(CBSP_WallmarkContext::Create, XR_BSPMODEL);

	m_lV.SetLen(_nV);
	m_lN.SetLen(_nV);
	m_lTV.SetLen(_nV);
	m_lTV2.SetLen(_nV);
	m_lCol.SetLen(_nV);
	m_nV = _nV;
	m_iVTail = 0;
	m_iVHead = 0;
	typedef TQueue<CBSP_Wallmark> TQueue_CBSP_Wallmark;
	m_spqWM = MNew1(TQueue_CBSP_Wallmark, _nV / 4);
	if (!m_spqWM) MemError("Create");

	m_spLink = MNew(CBSP_LinkContext);
	if (!m_spLink) MemError("Create");

	m_spLink->Create(_pModel, _nV / 4, _nV / 3);

	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC) Error("Wallmark_Render", "No surface-context available.");
	m_pSC = pSC;
}

void CBSP_WallmarkContext::Clear()
{
	MAUTOSTRIP(CBSP_WallmarkContext_Clear, MAUTOSTRIP_VOID);
	m_iVHead = 0;
	m_iVTail = 0;
	if (!m_spqWM) return;
	while(!m_spqWM->Empty())
	{
		m_spLink->Remove(m_spqWM->GetTailIndex());
		m_spqWM->Get();
	}
}

void CBSP_WallmarkContext::FreeWallmark()
{
	MAUTOSTRIP(CBSP_WallmarkContext_FreeWallmark, MAUTOSTRIP_VOID);
	if (!m_spqWM) return;
	int iTail = m_spqWM->GetTailIndex();
	CBSP_Wallmark WM = m_spqWM->Get();
	CBSP_Wallmark* pWM = m_spqWM->GetTail();
	m_spLink->Remove(iTail);

	if (!pWM)
	{
		m_iVTail = 0;
		m_iVHead = 0;
		return;
	}
	int iVLast = pWM->m_iV;
	while(m_iVTail != iVLast)
	{
		m_iVTail++;
		if (m_iVTail >= m_nV) m_iVTail = 0;
	}
}

int CBSP_WallmarkContext::AddWallmark(const CBSP_Wallmark& _WM)
{
	MAUTOSTRIP(CBSP_WallmarkContext_AddWallmark, 0);
	if (!m_spqWM) return 0;
	if (!m_spqWM->MaxPut()) FreeWallmark();

	int iHead = m_spqWM->GetHeadIndex();
	m_spqWM->Put(_WM);
	return iHead;
}

int CBSP_WallmarkContext::MaxPut() const
{
	MAUTOSTRIP(CBSP_WallmarkContext_MaxPut, 0);
	if (m_iVHead == m_iVTail)
		return m_nV-1;
	else
		return Max(0, ((2*m_nV - (m_iVHead - m_iVTail)) % m_nV) -1);
}


int CBSP_WallmarkContext::AddVertices(int _nV)
{
	MAUTOSTRIP(CBSP_WallmarkContext_AddVertices, 0);
	if ((m_iVHead + _nV > m_nV))
	{
		while(m_iVTail > m_iVHead)
			FreeWallmark();

		if (m_iVTail == 0) FreeWallmark();
		m_iVHead = 0;
	}

	while(MaxPut() < _nV+1) FreeWallmark();

	int iv = m_iVHead;
	m_iVHead += _nV;
	if (m_iVHead >= m_nV) m_iVHead = 0;
//ConOutL(CStrF("AddVerts %d, %d, %d, %d", iv, _nV, m_iVHead, m_iVTail));

	if (iv + _nV > m_nV)
		Error("AddVertices", CStrF("Internal error _nV %d, iV %d, nV %d, H %d, T %d", _nV, iv, m_nV, m_iVHead, m_iVTail));

	return iv;
}


void CXR_Model_BSP::Wallmark_Render(CBSP_WallmarkContext* _pWMC, const CBSP_Wallmark* _pWM, CMTime _Time)
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_Render, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP::Wallmark_Render);

	if(_pWM->m_Flags & XR_WALLMARK_DESTROYED)
		return;

	CXW_Surface* pSurf = _pWMC->m_pSC->GetSurface(_pWM->m_SurfaceID)->GetSurface(ms_SurfOptions, ms_SurfCaps);

	int nv = _pWM->m_nV;
	
	CXR_VertexBuffer* pVB = ms_pVBM->Alloc_VB(CXR_VB_ATTRIB);
	if (!pVB) return;
	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
	
	int nPrim = nv+2;
	uint16* pPrim = ms_pVBM->Alloc_Int16(nPrim);
	if (!pPrim) return;

	int iP = 0;
	pPrim[iP++] = CRC_RIP_TRIFAN + ((nv+2) << 8);
	pPrim[iP++] = nv;
	for(int v = 0; v < nv; v++) pPrim[iP++] = nv-1-v;

//		pVB->Matrix_Set(m_pVBMatrixM2V);
	if (!pVB->AllocVBChain(ms_pVBM, false))
		return;
	pVB->Geometry_VertexArray(&_pWMC->m_lV[_pWM->m_iV], _pWM->m_nV, true);
	pVB->Geometry_Normal(&_pWMC->m_lN[_pWM->m_iV]);
	pVB->Geometry_TVertexArray(&_pWMC->m_lTV[_pWM->m_iV], 0);
	pVB->Geometry_TVertexArray(&_pWMC->m_lTV2[_pWM->m_iV], 1);
	pVB->Geometry_ColorArray(&_pWMC->m_lCol[_pWM->m_iV]);
	pVB->Render_IndexedPrimitives(pPrim, nPrim);

	pVB->m_Priority = CXR_VBPRIORITY_WALLMARK;

	CMTime AnimTime = _Time - _pWM->m_SpawnTime;
	CXW_SurfaceKeyFrame* pSurfKey = pSurf->GetFrame(0, AnimTime, &m_TmpSurfKeyFrame);

	CXR_RenderSurfExtParam Params;
	for(int i = 0; i < XR_WALLMARK_MAXTEXTUREPARAMS; i++)
		Params.m_TextureID[i] = _pWM->m_SurfaceParam_TextureID[i];

	int Flags = /*RENDERSURFACE_VERTEXFOG | */ RENDERSURFACE_MATRIXSTATIC_M2V;
	CXR_Util::Render_Surface(Flags, AnimTime, pSurf, pSurfKey, ms_pCurrentEngine, ms_pVBM, (CMat4Dfp32*) NULL, (CMat4Dfp32*) NULL, ms_pVBMatrixM2V, pVB, 
		ms_BasePriority_Opaque /*ms_RenderInfo.m_BasePriority_Opaque*/+CXR_VBPRIORITY_WALLMARK, 0.0001f, &Params);
}


void CXR_Model_BSP::Wallmark_Render(CBSP_WallmarkContext* _pWMC, int _iPL)
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_Render_2, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP::Wallmark_Render, XR_BSPMODEL);

	if (!_pWMC->m_spqWM) return;
	if (!ms_pCurrentEngine->GetVar(XR_ENGINE_WALLMARKS)) return;
	CMTime RenderTime = ms_pCurrentEngine->GetEngineTime();

	if (_iPL < 0)
	{
		CBSP_Wallmark* pWM = _pWMC->m_spqWM->GetTail();
		while(pWM)
		{
			Wallmark_Render(_pWMC, pWM, RenderTime);
			pWM = _pWMC->m_spqWM->GetNext(pWM);
		}
	}
	else
	{
		uint16 liWM[1024];
		int nWM = _pWMC->m_spLink->EnumIDs(_iPL, liWM, 1024, NULL, 0);
		for(int i = 0; i < nWM; i++)
			Wallmark_Render(_pWMC, &_pWMC->m_spqWM->GetElem(liWM[i]), RenderTime);
	}
}

CXR_WallmarkInterface* CXR_Model_BSP::Wallmark_GetInterface()
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_GetInterface, NULL);
	if (m_lPortalLeaves.Len())
		return this;
	else
		return NULL;
}

int CXR_Model_BSP::Wallmark_CreateContext(const CXR_WallmarkContextCreateInfo& _CreateInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_CreateContext, 0);
	if (m_spWMC || m_spWMCTemp)
		return -1;

	m_spWMC = MNew(CBSP_WallmarkContext);
	m_spWMC->Create(this, _CreateInfo.m_CapacityDynamic);

	m_spWMCTemp = MNew(CBSP_WallmarkContext);
	m_spWMCTemp->Create(this, _CreateInfo.m_CapacityTemp);

	m_spWMCStatic = MNew(CBSP_WallmarkContext);
	m_spWMCStatic->Create(this, _CreateInfo.m_CapacityStatic);

	return 0;
}

void CXR_Model_BSP::Wallmark_DestroyContext(int _hContext)
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_DestroyContext, MAUTOSTRIP_VOID);
	if (_hContext != 0)
		return;

	m_spWMC = NULL;
	m_spWMCTemp = NULL;
	m_spWMCStatic = NULL;
}

#define XR_WALLMARK_FADECOLOR 2
#define MODEL_BSP_WALLMARKDISTANCE 0.0f
#define XR_WALLMARK_PERPENDICULAR_TREHOLD 0.1f

bool CXR_Model_BSP::Wallmark_Create(CBSP_WallmarkContext* _pWMC, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _iPL, const CVec3Dfp32* _pV, int _nV, int _GUID)
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_Create, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP::Wallmark_Create);

	if (_nV < 3) return false;

	const CVec3Dfp32& U = CVec3Dfp32::GetMatrixRow(_Origin, 0);
	const CVec3Dfp32& V = CVec3Dfp32::GetMatrixRow(_Origin, 1);
	const CVec3Dfp32& N = CVec3Dfp32::GetMatrixRow(_Origin, 2);
	const CVec3Dfp32& O = CVec3Dfp32::GetMatrixRow(_Origin, 3);

	CPlane3Dfp32 Plane;
	Plane.Create(_pV[0], _pV[2], _pV[1]);
	fp32 NDot = N * Plane.n;
	if(!(_Flags & XR_WALLMARK_ALLOW_PERPENDICULAR))
	{	
		if (NDot < XR_WALLMARK_PERPENDICULAR_TREHOLD) return false;
	}
	else
	{
		if (Abs(NDot) < 0.0001f)
			NDot = 0.0001f; 
	}
	fp32 d = Plane.Distance(O);
	if (M_Fabs(d) > _Tolerance) return false;

	CPlane3Dfp32 Planes[18];
	CVec3Dfp32 Verts[32];
	CVec2Dfp32 TVerts[32];
	CVec2Dfp32 TVerts2[32];
	CPixel32 Col[32];

	// Create edge planes:
	int nP = 0;
	{
		for(int p = 0; p < _nV; p++)
		{
			int iv0 = p;
			int iv1 = (p) ? (p-1) : _nV-1;
			Planes[nP++].Create(_pV[iv0], _pV[iv1], _pV[iv1] + Plane.n);
		}

		Planes[nP++].CreateNV(N, O + (N*_Tolerance));
		Planes[nP++].CreateNV(-N, O + (N*(-_Tolerance)));
	}

	CVec3Dfp32 OProj, UProj, VProj;
	O.Combine(N, -d/NDot, OProj);
	OProj += Plane.n * MODEL_BSP_WALLMARKDISTANCE;
	U.Combine(N, -(U*Plane.n) / NDot, UProj);
	V.Combine(N, -(V*Plane.n) / NDot, VProj);

	fp32 Size = _WM.m_Size;
	Verts[0] = OProj;
	OProj.Combine(UProj, -Size*0.5f, Verts[0]);
	Verts[0].Combine(VProj, -Size*0.5f, Verts[0]);
	Verts[0].Combine(UProj, Size, Verts[1]);
	Verts[1].Combine(VProj, Size, Verts[2]);
	Verts[2].Combine(UProj, -Size, Verts[3]);

	TVerts[0] = CVec2Dfp32(1,0);
	TVerts[1] = CVec2Dfp32(0,0);
	TVerts[2] = CVec2Dfp32(0,1);
	TVerts[3] = CVec2Dfp32(1,1);

	int nV = 0;
	{
		MSCOPE(CutFence, XR_BSPMODEL);
		nV = CutFence(Verts, 4, Planes, nP, true, TVerts);
	}
	if (nV <= 2)
		return false;

int iV = _pWMC->AddVertices(nV);

	fp32 TolerenceRecp = 1.0f / _Tolerance;
	for(int v = 0; v < nV; v++)
	{
		// Ugly texgen, do we still need this?
		TVerts2[v] = CVec2Dfp32(Verts[v][0] / 128.0f, Verts[v][1] / 128.0f);

		if (_WM.m_Flags & XR_WALLMARK_NOFADE)
		{
			Col[v] = 0xffffffff;
		}
		else
		{
			fp32 d = M_Fabs(N*(Verts[v] - O)) * TolerenceRecp;
			d = 1.0f - Clamp01(d);
			d *= 1.0f - Sqr(Sqr(1.0f - NDot));
			int Fade = RoundToInt(d*255.0f);
			if (_WM.m_Flags & XR_WALLMARK_FADECOLOR)
				Col[v] = 0xff000000 + (Fade << 16) + (Fade << 8) + Fade;
			else
				Col[v] = (Fade << 24) + 0xffffff;
		}
	}

	memcpy(&_pWMC->m_lV[iV], Verts, sizeof(CVec3Dfp32)*nV);
	memcpy(&_pWMC->m_lTV[iV], TVerts, sizeof(CVec2Dfp32)*nV);
	memcpy(&_pWMC->m_lTV2[iV], TVerts2, sizeof(CVec2Dfp32)*nV);
	memcpy(&_pWMC->m_lCol[iV], Col, sizeof(CPixel32)*nV);
	{ for(int v = 0; v < nV; v++) _pWMC->m_lN[iV+v] = Plane.n; };

	CBSP_Wallmark WM;
	WM.CXR_WallmarkDesc::operator=(_WM);
	WM.m_iV = iV;
	WM.m_nV = nV;
	WM.m_GUID = _GUID;

//ConOut(CStrF("    iPL %d, WM Added: ", _iPL) + N.GetString());
	int iWM = _pWMC->AddWallmark(WM);
	_pWMC->m_spLink->Insert(iWM, _iPL);

	
/*		CWireContainer* pWC = NULL;
	MACRO_GetRegisterObject(CWireContainer, pWCTmp, "GAMECONTEXT.CLIENT.WIRECONTAINER");
	pWC = pWCTmp;
	if (pWC)
	{
		for(int i = 0; i < nV; i++)
			pWC->RenderWire(Verts[i], Verts[(i+1) % nV], 0xff703f00, 0.05f);
	}*/

	return true;
}

int CXR_Model_BSP::Wallmark_Create(CBSP_WallmarkContext* _pWMC, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material)
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_Create_2, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP::Wallmark_Create);

	if (!_pWMC->m_spqWM) return 0;

	const int MaxFaces = 1024;
	uint32 liFaces[MaxFaces];
	uint16 liFacePL[MaxFaces];

//	const CVec3Dfp32& U = CVec3Dfp32::GetMatrixRow(_Origin, 0);
//	const CVec3Dfp32& V = CVec3Dfp32::GetMatrixRow(_Origin, 1);
	const CVec3Dfp32& N = CVec3Dfp32::GetMatrixRow(_Origin, 2);
	const CVec3Dfp32& O = CVec3Dfp32::GetMatrixRow(_Origin, 3);

	fp32 SH = _WM.m_Size * 0.5f;
	CBox3Dfp32 BoundLocal;
	BoundLocal.m_Min = CVec3Dfp32(-SH, -SH, -_Tolerance);
	BoundLocal.m_Max = CVec3Dfp32(SH, SH, _Tolerance);
	CBox3Dfp32 Bound;
	BoundLocal.Transform(_Origin, Bound);
/*	Bound.m_Min = O;
	Bound.m_Max = O;
	Bound.m_Min -= CVec3Dfp32(_Tolerance*_SQRT2);
	Bound.m_Max += CVec3Dfp32(_Tolerance*_SQRT2);*/


//	fp32 Size = _WM.m_Size;

//	int nFaces = EnumFaces_Sphere(1, liFaces, MaxFaces, ENUM_HQ | ENUM_FACEFLAGS, 0, XW_FACE_VISIBLE, O, Size*0.5f*_SQRT2, liFacePL);
	int nFaces = 0;
	CXR_PhysicsContext PhysContext;
	nFaces = EnumFaces_Box(&PhysContext, 1, liFaces, MaxFaces, ENUM_HQ | ENUM_FACEFLAGS, 0, XW_FACE_VISIBLE, Bound, liFacePL);
	EnumFaces_Untag(&PhysContext);

//ConOut(CStrF("WMFaces %d, Pos %s", nFaces, (char*)O.GetString() ));

	static int GUID = 1;
	int NewGUID = GUID + 1;
	bool bRes = false;
	{
		MSCOPE(Faces, XR_BSPMODEL);
		for(int i = 0; i < nFaces; i++)
		{
			const CBSP_Face* pF = &m_pFaces[liFaces[i]];
			CXW_Surface* pS = m_lspSurfaces[pF->m_iSurface];
			if (pS->m_Flags & XW_SURFFLAGS_NOWALLMARK)
				continue;

			if(_Material != 0 && pS->GetBaseFrame()->m_MaterialType != _Material)
				continue;

			const CPlane3Dfp32* pP = &m_pPlanes[pF->m_iPlane];
	//ConOut(CStr("    WM Face test: " + N.GetString() +  pP->n.GetString()));
			fp32 NDot = N * pP->n;
			if(!(_Flags & XR_WALLMARK_ALLOW_PERPENDICULAR))
			{	
				if (NDot < XR_WALLMARK_PERPENDICULAR_TREHOLD) continue;
			}
			else
			{
				if (Abs(NDot) < 0.0001f)
					NDot = 0.0001f; 
			}

			fp32 d = pP->Distance(O);
			if (Abs(d) > _Tolerance) continue;

			CVec3Dfp32 Verts[32];
			int iiV = pF->m_iiPhysV;
			const uint32* piV = &m_piVertices[iiV];
			for(int p = 0; p < pF->m_nPhysV; p++)
				Verts[p] = m_pVertices[piV[p]];

			bRes |= Wallmark_Create(_pWMC, _WM, _Origin, _Tolerance, _Flags, liFacePL[i], Verts, pF->m_nPhysV, NewGUID);
		}
	}
	
	{
		MSCOPE(SB, XR_BSPMODEL);
		uint32 ST[CSB_TESS_PHYSICS*CSB_TESS_PHYSICS];

		if (m_spSBLink!=NULL)
		{
			uint16 SBBuffer[1024];
			int nSB = 0;
			{
				MSCOPE(m_spSBLink->EnumBox, XR_BSPMODEL);
				nSB = m_spSBLink->EnumBox(Bound, SBBuffer, 1024, NULL, 0);
			}
			for(int i = 0; i < nSB; i++)
			{
				int iSB = SBBuffer[i];
				CSplineBrush* pSB = m_lspSplineBrushes[iSB];
				bool bIsInside = pSB->m_BoundBox.IsInside(Bound);
				if (!bIsInside) continue;

				for(int iFace = 0; iFace < pSB->m_lFaces.Len(); iFace++)
				{
					CSplineBrush_Face* pF = &pSB->m_lFaces[iFace];
					CXW_Surface* pS = m_lspSurfaces[pF->m_iSurface];
					if (pS->m_Flags & XW_SURFFLAGS_NOWALLMARK) continue;

					if (!pF->m_BoundBox.IsInside(Bound)) continue;

					int nS = 1 << pF->m_PhysTessShiftS;
					int nT = 1 << pF->m_PhysTessShiftT;
					fp32 sStep = 1.0f / fp32(nS);
					fp32 tStep = 1.0f / fp32(nT);

		//	LogFile(CStrF("iSB %d, iF %d, nS %d, nT %d", iSB, iFace, nS, nT));

					{
						MSCOPE(TessSpline, XR_BSPMODEL);
						int nST = pSB->EnumBoundNodes(iFace, Bound, ST);
			//if (nST) ConOut(CStrF("nST %d, iSB %d, iF %d", nST, iSB, iFace));
						for(int iST = 0; iST < nST; iST++)
						{
							fp32 fS = fp32(ST[iST] & 0x7fff) * sStep;
							fp32 fT = fp32(ST[iST] >> 16) * tStep;
							CVec3Dfp32 Verts[4];
							pSB->EvalXYZ(iFace, fS, fT, Verts[3]);
							pSB->EvalXYZ(iFace, fS + sStep, fT, Verts[2]);
							pSB->EvalXYZ(iFace, fS + sStep, fT + tStep, Verts[1]);
							pSB->EvalXYZ(iFace, fS, fT + tStep, Verts[0]);

							int iPL = GetStructurePortalLeaf((Verts[0] + Verts[1] + Verts[2] + Verts[3]) * 0.25f + N);
							if (iPL >= 0)
							{
								bRes |= Wallmark_Create(_pWMC, _WM, _Origin, _Tolerance, _Flags, iPL, &Verts[1], 3, NewGUID);
								Verts[2] = Verts[3];
								bRes |= Wallmark_Create(_pWMC, _WM, _Origin, _Tolerance, _Flags, iPL, &Verts[0], 3, NewGUID);
							}
						}
					}
				}
			}
		}
	}

	if(bRes)
		GUID = NewGUID;
	return bRes;
}

int CXR_Model_BSP::Wallmark_Create(int _hContext, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material)
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_Create_3, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP::Wallmark_Create, XR_BSPMODEL);

	if (!m_spWMC)
		return false;

/*	if (CVec3Dfp32::GetRow(_Origin,3).LengthSqr() < Sqr(200.0f))
	{
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
		if (pSC)
		{	
			CXW_Surface* pS = pSC->GetSurface(_WM.m_SurfaceID);
			ConOutL(CStrF("Wallmark close to origo, Pos %s, Tolerance %f, %s", CVec3Dfp32::GetRow(_Origin,3).GetString().Str(), _Tolerance, pS->m_Name.Str() ));
		}
	}*/

	if(_Flags & XR_WALLMARK_NEVERDESTROY)
		return Wallmark_Create(m_spWMCStatic, _WM, _Origin, _Tolerance, _Flags, _Material);
	else if(_Flags & XR_WALLMARK_TEMPORARY)
		return Wallmark_Create(m_spWMCTemp, _WM, _Origin, _Tolerance, _Flags, _Material);
	else
		return Wallmark_Create(m_spWMC, _WM, _Origin, _Tolerance, _Flags, _Material);
}

bool CXR_Model_BSP::Wallmark_Destroy(int _GUID)
{
	CBSP_WallmarkContext *pContexts[3] = { m_spWMCTemp, m_spWMC, m_spWMCStatic };

	bool bRes = false;
	for(int i = 0; i < 3; i++)
	{
		CBSP_Wallmark* pWM = pContexts[i]->m_spqWM->GetTail();
		while(pWM)
		{
			if(pWM->m_GUID == _GUID)
			{
				// Remove wallmark
				pWM->m_Flags |= XR_WALLMARK_DESTROYED;
				bRes = true;
			}
			pWM = pContexts[i]->m_spqWM->GetNext(pWM);
		}
	}
	return bRes;
}
