#include "PCH.h"

#include "WBSPModel.h"
#include "WBSPDef.h"
#include "MFloat.h"
#include "../../XR/XREngineVar.h"
#include "../../Classes/BitString/MBitString.h"

//#define MODEL_BSP_EXACTPORTALCULL
//#define MODEL_BSP_EXACTRELAXED

#ifdef PLATFORM_WIN_PC
	#define MODEL_BSP_NUMRPORTALS	192
#else
#define MODEL_BSP_NUMRPORTALS	128
#endif

#define MACRO_ISMIRRORED(Mat)	\
	((CVec3Dfp32::GetMatrixRow(Mat, 0) / CVec3Dfp32::GetMatrixRow(Mat, 1)) * CVec3Dfp32::GetMatrixRow(Mat, 2) < 0.0f)

#define fMin(a, b) ((((a)+(b)) - fp32(M_Fabs((a)-(b)))) * 0.5f)
#define fMax(a, b) ((((a)+(b)) + fp32(M_Fabs((a)-(b)))) * 0.5f)
#define fMin2(a, b) (((a)+(b)) - fp32(M_Fabs((a)-(b))))
#define fMax2(a, b) (((a)+(b)) + fp32(M_Fabs((a)-(b))))

// -------------------------------------------------------------------
//#define PORTAL_AND_DEBUG

int CXR_Model_BSP::Portal_And(const CRC_ClipVolume* _pPortal, const CBSP_PortalExt* _pP, bool _bFrontThis, CRC_ClipVolume* _pPDest)
{
	MAUTOSTRIP(CXR_Model_BSP_Portal_And, 0);
	MIncProfile(ms_nPortal0);
	// ANDs _pPortal and _pP, puts the result in _DestP
	
	fp32 Dist = m_pPlanes[_pP->m_iPlane].Distance(_pPortal->m_POV);
	if (!_bFrontThis) Dist = -Dist;
	if (Dist < -0.1f) return 0;

	int nv = _pP->m_nVertices;
	int iiv = _pP->m_iiVertices;
	if (nv < 3)
	{
		ConOut(CStrF("§cf80WARNING: Fucked up portal!  %d verts.", nv));
		return 0;
	}

	int nPlanes = _pPortal->m_nPlanes;
	CVec3Dfp32 POV = _pPortal->m_POV;

//ms_TimePortal0 -= GetCPUClock();

	if (Dist < 1.0f)
	{
		*_pPDest = *_pPortal;
#ifdef PORTAL_AND_DEBUG
		ConOut("Nu!");
#endif
		return 1;
	}
#ifdef PORTAL_AND_DEBUG
	ConOut("Inte nu!");
#endif

//ms_TimePortal0 -= GetCPUClock();
//	_pPDest->GetVertices(&m_pVertices[m_piVertices[iiv]], nv, POV, _bFrontThis);
	{
		_pPDest->m_nPlanes = nv;
		_pPDest->m_POV = POV;
		if (_bFrontThis)
			for(int v = 0; v < nv; v++)
				_pPDest->m_Vertices[v] = m_pVertices[m_piVertices[iiv + nv - v - 1]];
		else
			for(int v = 0; v < nv; v++)
				_pPDest->m_Vertices[v] = m_pVertices[m_piVertices[iiv + v]];
	}

	// Clip portal to backplane
	{
		int bClip;
		_pPDest->m_nPlanes = BSPModel_CutFace3(&_pPDest->m_Vertices[0], _pPDest->m_nPlanes, &m_pView->m_LocalBackPlane, 1, bClip);
		if (!_pPDest->m_nPlanes) return 0;

		if (ms_Vis_pSceneFog && ms_Vis_pSceneFog->VertexFogEnable())
			_pPDest->m_nPlanes = BSPModel_CutFace3(&_pPDest->m_Vertices[0], _pPDest->m_nPlanes, &ms_Vis_pSceneFog->m_VtxFog_EndPlane, 1, bClip);
	}

#ifdef MODEL_BSP_PORTALBOUNDBOX

	{
		CPlane3Dfp32 ClipPlanes[16];

		const CVec3Dfp32& VMin = _pP->m_BoxMin;
		const CVec3Dfp32& VMax = _pP->m_BoxMax;
/*		CVec3Dfp32 Center;
		Center.k[0] = (VMin.k[0] + VMax.k[0]) * 0.5f;
		Center.k[1] = (VMin.k[1] + VMax.k[1]) * 0.5f;
		Center.k[2] = (VMin.k[2] + VMax.k[2]) * 0.5f;
*/
		int nClip = 0;
		for(int ip = 0; ip < nPlanes; ip++)
		{
			const CPlane3Dfp32* pP = &_pPortal->m_Planes[ip];
//			if (pP->Distance(Center) > 0.0f)
			{
/*				fp32 x0 = VMin.k[0]*pP->n.k[0];
				fp32 x1 = VMax.k[0]*pP->n.k[0];
				fp32 min = fMin2(x0, x1);
				fp32 max = fMax2(x0, x1);
				fp32 y0 = VMin.k[1]*pP->n.k[1];
				fp32 y1 = VMax.k[1]*pP->n.k[1];
				min += fMin2(y0, y1);
				max += fMax2(y0, y1);
				fp32 z0 = VMin.k[2]*pP->n.k[2];
				fp32 z1 = VMax.k[2]*pP->n.k[2];
				min += fMin2(z0, z1);
				max += fMax2(z0, z1);
				if (min > pP->d*-2.0f)
				{
					_pPDest->m_nPlanes = 0;
					return 0;
				}

				if (max > pP->d*-2.0f)
					ClipPlanes[nClip++] = *pP;
*/
				fp32 xMin, xMax, yMin, yMax, zMin, zMax;
				if (pP->n.k[0] > 0.0f) 
					{ xMin = VMin.k[0]*pP->n.k[0]; xMax = VMax.k[0]*pP->n.k[0]; }
				else
					{ xMin = VMax.k[0]*pP->n.k[0]; xMax = VMin.k[0]*pP->n.k[0]; }

				if (pP->n.k[1] > 0.0f) 
					{ yMin = VMin.k[1]*pP->n.k[1]; yMax = VMax.k[1]*pP->n.k[1]; }
				else
					{ yMin = VMax.k[1]*pP->n.k[1]; yMax = VMin.k[1]*pP->n.k[1]; }

				if (pP->n.k[2] > 0.0f) 
					{ zMin = VMin.k[2]*pP->n.k[2]; zMax = VMax.k[2]*pP->n.k[2]; }
				else
					{ zMin = VMax.k[2]*pP->n.k[2]; zMax = VMin.k[2]*pP->n.k[2]; }

				if (xMin + yMin + zMin + pP->d > 0.0f)
				{
					_pPDest->m_nPlanes = 0;
	//				ms_TimePortal0 += GetCPUClock(); //ConOut("Easy skip.");
					return 0;
				}

				if (xMax + yMax + zMax + pP->d > 0.0f)
					ClipPlanes[nClip++] = *pP;
			}
		}

		if (!nClip)
		{
#ifndef MODEL_BSP_CFV3
			_pPDest->CreateFromVertices();
#else
			_pPDest->CreateFromVertices3(m_pView->m_LocalBackPlane);
#endif
//			ConOut("No Clip.");
//			ms_TimePortal0 += GetCPUClock();
			return 1;
		}
		MAddProfile(ms_nPortal2,nPlanes - nClip);

		int bClip;
		_pPDest->m_nPlanes = BSPModel_CutFace3(&_pPDest->m_Vertices[0], _pPDest->m_nPlanes, &ClipPlanes[0], nClip, bClip);
//		_pPDest->m_nPlanes = BSPModel_CutFace2(&_pPDest->m_Vertices[0], _pPDest->m_nPlanes, &ClipPlanes[0], nClip, true);
		if (_pPDest->m_nPlanes) 
#ifndef MODEL_BSP_CFV3
			_pPDest->CreateFromVertices();
#else
			_pPDest->CreateFromVertices3(m_pView->m_LocalBackPlane);
#endif

//		ms_TimePortal0 += GetCPUClock();
		return bClip ^ 1;
	}
#else
	int bClip;
	_pPDest->m_nPlanes = BSPModel_CutFace3(&_pPDest->m_Vertices[0], _pPDest->m_nPlanes, &_pPortal->m_Planes[0], nPlanes, bClip);
//	_pPDest->m_nPlanes = BSPModel_CutFace2(&_pPDest->m_Vertices[0], _pPDest->m_nPlanes, &_pPortal->m_Planes[0], nPlanes, true);
	if (!_pPDest->m_nPlanes)
	{
//ms_TimePortal0 += GetCPUClock();
		return 0;
	}
#ifndef MODEL_BSP_CFV3
	_pPDest->CreateFromVertices();
#else
	_pPDest->CreateFromVertices3(m_pView->m_LocalBackPlane);
#endif
#endif
//RenderRPortal(_pPDest, 0xff4040ff);
//ms_TimePortal0 += GetCPUClock();


#ifdef PORTAL_AND_DEBUG
	for(v = 0; v < _pPDest->m_nPlanes; v++)
	{
		for (int p = 0; p < _pPortal->m_nPlanes; p++)
			if (_pPortal->m_Planes[p].GetPlaneSide_Epsilon(_pPDest->m_Vertices[v], 0.1f) > 0)
			{
				ConOut(CStrF("P-And: Point %d outside plane %d", v, p));
			}
	}

#endif

	return 1;
}


#define PORTAL_OR_DEBUG

void CXR_Model_BSP::Portal_Or(CRC_ClipVolume* _pP1, const CRC_ClipVolume* _pP2)
{
	MAUTOSTRIP(CXR_Model_BSP_Portal_Or, MAUTOSTRIP_VOID);
	if (!_pP1->m_nPlanes)
	{
		// Portal 1 is void, just copy.
//		_pP1->Copy(*_pP2);
		*_pP1 = *_pP2;
		return;
	}
	else if (!_pP2->m_nPlanes)
	{
		// Portal 2 is void, do nada.
		return;
	}
	else
	{
#ifdef PORTAL_OR_DEBUG
//			ConOut("Or-ing..");
#endif
//		m_nPortal3++;

		int nv1 = _pP1->m_nPlanes;
		int nv2 = _pP2->m_nPlanes;

		CVec2Dfp32 VMin(_FP32_MAX);
		CVec2Dfp32 VMax(-_FP32_MAX);
		const CMat4Dfp32* pM = &m_pView->m_CurVMat;

		bool bErr = false;
		{
			const CVec3Dfp32* pV = &_pP1->m_Vertices[0];
			for(int v = 0; v < nv1; v++)
			{
				fp32 vx = pV[v].k[0];
				fp32 vy = pV[v].k[1];
				fp32 vz = pV[v].k[2];
				fp32 z = pM->k[0][2]*vx + pM->k[1][2]*vy + pM->k[2][2]*vz + pM->k[3][2];
				if (z < 0.1f) { bErr = true; break; }
				fp32 zinv = m_pView->m_ViewBackPlane / z;
				fp32 x = (pM->k[0][0]*vx + pM->k[1][0]*vy + pM->k[2][0]*vz + pM->k[3][0]) * zinv;
				if (x < VMin.k[0]) VMin.k[0] = x;
				if (x > VMax.k[0]) VMax.k[0] = x;
				fp32 y = (pM->k[0][1]*vx + pM->k[1][1]*vy + pM->k[2][1]*vz + pM->k[3][1]) * zinv;
				if (y < VMin.k[1]) VMin.k[1] = y;
				if (y > VMax.k[1]) VMax.k[1] = y;
			}
		}

		if (!bErr)
		{
			const CVec3Dfp32* pV = &_pP2->m_Vertices[0];
			for(int v = 0; v < nv2; v++)
			{
				fp32 vx = pV[v].k[0];
				fp32 vy = pV[v].k[1];
				fp32 vz = pV[v].k[2];
				fp32 z = pM->k[0][2]*vx + pM->k[1][2]*vy + pM->k[2][2]*vz + pM->k[3][2];
				if (z < 0.1f) { bErr = true; break; }
				fp32 zinv = m_pView->m_ViewBackPlane / z;
				fp32 x = (pM->k[0][0]*vx + pM->k[1][0]*vy + pM->k[2][0]*vz + pM->k[3][0]) * zinv;
				if (x < VMin.k[0]) VMin.k[0] = x;
				if (x > VMax.k[0]) VMax.k[0] = x;
				fp32 y = (pM->k[0][1]*vx + pM->k[1][1]*vy + pM->k[2][1]*vz + pM->k[3][1]) * zinv;
				if (y < VMin.k[1]) VMin.k[1] = y;
				if (y > VMax.k[1]) VMax.k[1] = y;
			}
		}

		if (bErr)
		{
			ConOut("P-Or: Using view min/max");

			VMin = ms_PortalOrMin;
			VMax = ms_PortalOrMax;
			VMin *= m_pView->m_ViewBackPlane;
			VMax *= m_pView->m_ViewBackPlane;
		}

		CVec3Dfp32 VVerts[4];
		if (MACRO_ISMIRRORED(m_pView->m_CurVMatInv))
		{
			VVerts[0] = CVec3Dfp32(VMin.k[0], VMin.k[1], m_pView->m_ViewBackPlane);
			VVerts[1] = CVec3Dfp32(VMin.k[0], VMax.k[1], m_pView->m_ViewBackPlane);
			VVerts[2] = CVec3Dfp32(VMax.k[0], VMax.k[1], m_pView->m_ViewBackPlane);
			VVerts[3] = CVec3Dfp32(VMax.k[0], VMin.k[1], m_pView->m_ViewBackPlane);
		}
		else
		{
			VVerts[3] = CVec3Dfp32(VMin.k[0], VMin.k[1], m_pView->m_ViewBackPlane);
			VVerts[2] = CVec3Dfp32(VMin.k[0], VMax.k[1], m_pView->m_ViewBackPlane);
			VVerts[1] = CVec3Dfp32(VMax.k[0], VMax.k[1], m_pView->m_ViewBackPlane);
			VVerts[0] = CVec3Dfp32(VMax.k[0], VMin.k[1], m_pView->m_ViewBackPlane);
		}

//		ConOut(CStrF("Min (%f, %f),  Max (%f, %f)", VMin.k[0], VMin.k[1], VMax.k[0], VMax.k[1]));
		CVec3Dfp32::MultiplyMatrix(&VVerts[0], &_pP1->m_Vertices[0], m_pView->m_CurVMatInv, 4);
		_pP1->m_nPlanes = 4;
		_pP1->CreateFromVertices2();
		
/*		for(v = 0; v < 4; v++)
			ConOut(_pP1->m_Vertices[v].GetString());*/

//	RenderRPortal(_pP1, 0xffff40);
	}
}


static int g_nPotentialPVSRejects = 0;

void CXR_Model_BSP::Portal_AddNode(int _iNode, int _iClipRPortal)
{
	MAUTOSTRIP(CXR_Model_BSP_Portal_AddNode, MAUTOSTRIP_VOID);
	CBSP_Node* pNode = &(m_pNodes[_iNode]);
	if (!(pNode->m_Flags & XW_NODE_STRUCTURE) ||
		(!(pNode->m_Flags & XW_NODE_ENABLE))) return;

	if (m_pView->m_nVisLeaves >= m_pView->m_MaxVisLeaves) return;

	if (pNode->m_Flags & XW_NODE_PORTAL)
	{
		// This is a portal leaf
		int iPL = pNode->m_iPortalLeaf;
		if ((ms_Enable & MODEL_BSP_ENABLE_PVSCULL) && !m_pView->InPVS(iPL))
		{
//			ConOut(CStrF("PL %d rejected with PVS.", iPL));
			return;
		}

		{
//if (!NumFacesInTree(_iNode)) return;
			CBSP_PortalLeafExt* pPL = &m_pPortalLeaves[pNode->m_iPortalLeaf];

			int np = pPL->m_nPortals;
			int iip = pPL->m_iiPortals;

#ifndef MODEL_BSP_EXACTPORTALCULL
			bool bCanDelete = false;
			int iRPortal = m_pView->m_liLeafRPortals[pNode->m_iPortalLeaf];
			if (!iRPortal)
			{
				if (m_pView->m_nRPortals >= m_pView->m_MaxRPortals)
				{
					ConOut("§cf80WARNING: Insufficient portal storage.");
					return;
				}
				iRPortal = m_pView->m_nRPortals;
				m_pView->m_liLeafRPortals[pNode->m_iPortalLeaf] = iRPortal;
				bCanDelete = true;
				m_pView->m_pRPortals[iRPortal].Init(m_pView->m_CurLocalVP);
				m_pView->m_piRPortalNext[iRPortal] = 0;
				m_pView->m_nRPortals++;
			}
#endif

//			if (m_iView > 0)
//				RenderRPortal(&m_pView->m_pRPortals[_iClipRPortal], 0xffffffff);

			bool bFrontThis = true;
			int iNodeNgbr = 0;
//			bool bAllFull = true;
			for(int p = 0; p < np; p++)
			{
				int ip = m_piPortals[iip + p];
				CBSP_PortalExt* pP = &m_pPortals[ip];

//#ifndef MODEL_BSP_ALLPORTALSOPEN
#ifdef MODEL_BSP_ALLUSERPORTALSCLOSED
				if (pP->m_PortalID) continue;
#endif
				if (pP->m_PortalID && !m_pView->PortalOpen(pP->m_PortalID)) continue;
//#endif

				if (pP->m_iNodeFront == _iNode)
				{	bFrontThis = true;	iNodeNgbr = pP->m_iNodeBack; }
				else if (pP->m_iNodeBack == _iNode)
				{	bFrontThis = false;	iNodeNgbr = pP->m_iNodeFront; }
				else
#ifdef MODEL_BSP_PARANOIA
					Error("Portal_AddNode", "Invalid portal linking.");
#else
					continue;
#endif

				int iRPNgbr = m_pView->m_liLeafRPortals[m_pNodes[iNodeNgbr].m_iPortalLeaf];
//ConOut(CStrF("PL %d, iNodeNgbr %d, iRPNgbr %d", iPL, iNodeNgbr, iRPNgbr));
#ifdef MODEL_BSP_EXACTPORTALCULL
#ifdef MODEL_BSP_EXACTRELAXED
				int iRPNew = m_pView->m_nRPortals;
				m_pView->m_pRPortals[iRPNew].Init(m_pView->m_CurLocalVP);
#endif

				int nClipVol = 0;
				while(iRPNgbr)
				{
					CRC_ClipVolume* pPortal = &m_pView->m_pRPortals[iRPNgbr];
#ifdef MODEL_BSP_EXACTRELAXED
					CRC_ClipVolume DestPortal;
#else
					int iRPNew = m_pView->m_nRPortals;
					CRC_ClipVolume& DestPortal = m_pView->m_pRPortals[iRPNew];
#endif

					if (m_pView->m_nRPortals >= m_pView->m_MaxRPortals)
					{
						ConOut("§cf80WARNING: Insufficient portal storage.");
						return;
					}

					DestPortal.Init(m_pView->m_CurLocalVP);
					ms_TimePortalAnd -= GetCPUClock();
					Portal_And(pPortal, &m_pPortals[ip], !bFrontThis, &DestPortal);
					T_Stop(ms_TimePortalAnd);

#ifdef MODEL_BSP_EXACTRELAXED
					if (DestPortal.m_nPlanes >= 3)
					{
						Portal_Or(&m_pView->m_pRPortals[iRPNew], &DestPortal);
						nClipVol++;
					}
#else
					if (DestPortal.m_nPlanes >= 3)
					{
						int iRPLast = m_pView->m_liLeafRPortals[iPL];
						m_pView->m_piRPortalNext[iRPNew] = iRPLast;
						m_pView->m_liLeafRPortals[iPL] = iRPNew;
						m_pView->m_nRPortals++;
						nClipVol++;
	//				ConOut(CStrF("PL %d, Added RP %d after %d, nRP %d", iPL, iRPNew, iRPLast, m_pView->m_nRPortals));
					}
#endif

					iRPNgbr = m_pView->m_piRPortalNext[iRPNgbr];
				}

#ifdef MODEL_BSP_EXACTRELAXED
				if (m_pView->m_pRPortals[iRPNew].m_nPlanes >= 3)
				{
					int iRPLast = m_pView->m_liLeafRPortals[iPL];
					m_pView->m_piRPortalNext[iRPNew] = iRPLast;
					m_pView->m_liLeafRPortals[iPL] = iRPNew;
					m_pView->m_nRPortals++;
//				ConOut(CStrF("PL %d, Added RP %d after %d, nRP %d", iPL, iRPNew, iRPLast, m_pView->m_nRPortals));
				}
#endif

//				if (nClipVol > 4) ConOut(CStrF("PL %d, nRP %d", iPL, nClipVol));
#else
				if (iRPNgbr != 0)
				{
					CRC_ClipVolume* pPortal = &m_pView->m_pRPortals[iRPNgbr];
					CRC_ClipVolume DestPortal;

					DestPortal.Init(m_pView->m_CurLocalVP);
					{
						TMeasureProfile(ms_TimePortalAnd);
						Portal_And(pPortal, &m_pPortals[ip], !bFrontThis, &DestPortal);
					}

					{
						TMeasureProfile(ms_TimePortalOr);
						if (DestPortal.m_nPlanes >= 3)
							Portal_Or(&m_pView->m_pRPortals[iRPortal], &DestPortal);
					}
				}
#endif

			} // end portal loop

#ifdef MODEL_BSP_EXACTPORTALCULL
			int iRPFirst = m_pView->m_liLeafRPortals[iPL];
			if (iRPFirst)
#else
			if (m_pView->m_pRPortals[iRPortal].m_nPlanes >= 3)
#endif
			{
				MIncProfile(ms_nPortal3);
				for(int p = 0; p < np; p++)
				{
					int ip = m_piPortals[iip + p];
					CBSP_PortalExt* pP = &m_pPortals[ip];
//#ifndef MODEL_BSP_ALLPORTALSOPEN
#ifdef MODEL_BSP_ALLUSERPORTALSCLOSED
					if (pP->m_PortalID) continue;
#endif
					if (pP->m_PortalID && !m_pView->PortalOpen(pP->m_PortalID)) continue;

					EnableTreeFromNode((pP->m_iNodeFront == _iNode) ? pP->m_iNodeBack : pP->m_iNodeFront);
				}

//#ifndef MODEL_BSP_EXACTPORTALCULL
				_iClipRPortal = m_pView->m_liLeafRPortals[iPL];
//#endif
			}
			else
			{
#ifndef MODEL_BSP_EXACTPORTALCULL
				if (bCanDelete)
				{
					m_pView->m_liLeafRPortals[iPL] = 0;
					m_pView->m_nRPortals--;
				}
#endif
				return;
			}
		}
//		RenderNodePortals(_iNode);

		// Add portal-leaf to list
		{
			if (!m_pView->InPVS(iPL))
			{
				g_nPotentialPVSRejects++;
				//ConOut(CStrF("PL %d rejected with PVS.", iPL));
				// return;
			}
			
			m_lPortalLeaves[iPL].m_DynLightMask = 0;
			m_pView->m_liVisPortalLeaves[m_pView->m_nVisLeaves++] = iPL;
//			m_pView->m_liLeafRPortals[pNode->m_iPortalLeaf] = _iClipRPortal;
		}
		return;
	}

	if (pNode->IsLeaf())
	{
//		ConOut("Ett vadå?");
		// This is a leaf!
//		m_pView->m_pCurVisLeaves[m_pView->m_nCurVisLeaves++] = _iNode;
//		m_pView->m_liLeafRPortals[pNode->m_iPortalLeaf] = _iClipRPortal;
	}
	else
	{
//ms_TimePortal3 -= GetCPUClock();
		// What sides of the plane can we see?

//#ifdef PORTAL_NO_BSP_CULL
		if (!(pNode->m_Flags & XW_NODE_STRUCTURE))
		{
			int iPlane = m_pNodes[_iNode].m_iPlane;
			CPlane3Dfp32* pP = &m_pPlanes[iPlane];
			fp32 d = pP->Distance(m_pView->m_CurLocalVP);

			int iFront = m_pNodes[_iNode].m_iNodeFront;
			int iBack = m_pNodes[_iNode].m_iNodeBack;
			if (d >= 0.0f)
			{
				if (iFront) Portal_AddNode(iFront, _iClipRPortal);
				if (iBack) Portal_AddNode(iBack, _iClipRPortal);
			}
			else
			{
				if (iBack) Portal_AddNode(iBack, _iClipRPortal);
				if (iFront) Portal_AddNode(iFront, _iClipRPortal);
			}
		}
		else
		{
//#else // PORTAL_NO_BSP_CULL
			int iPlane = m_pNodes[_iNode].m_iPlane;
			CPlane3Dfp32* pP = &m_pPlanes[iPlane];
			fp32 d = pP->Distance(m_pView->m_CurLocalVP);
			if (d >= 0.0f)
			{
				int iFront = m_pNodes[_iNode].m_iNodeFront;
				if (iFront) Portal_AddNode(iFront, _iClipRPortal);

				int iBack = m_pNodes[_iNode].m_iNodeBack;
				if (iBack)
				{
					CRC_ClipVolume* pRP = &m_pView->m_pRPortals[_iClipRPortal];
					for(int v = 0; v < pRP->m_nPlanes; v++)
						if (pP->Distance(pRP->m_Vertices[v]) < 0.0f)
						{
							Portal_AddNode(iBack, _iClipRPortal);
							return;
						}
				}
			}
			else
			{
				int iBack = m_pNodes[_iNode].m_iNodeBack;
				if (iBack) Portal_AddNode(iBack, _iClipRPortal);

				int iFront = m_pNodes[_iNode].m_iNodeFront;
				if (iFront)
				{
					CRC_ClipVolume* pRP = &m_pView->m_pRPortals[_iClipRPortal];
					for(int v = 0; v < pRP->m_nPlanes; v++)
						if (pP->Distance(pRP->m_Vertices[v]) > 0.0f)
						{
							Portal_AddNode(iFront, _iClipRPortal);
							return;
						}
				}
			}
		}
//#endif // PORTAL_NO_BSP_CULL
	}
}

void CXR_Model_BSP::Portal_OpenClipFront_r(CBSP_Node* pNode, const CVec3Dfp32* _pVPortal, int _nVPortal)
{
	MAUTOSTRIP(CXR_Model_BSP_Portal_OpenClipFront_r, MAUTOSTRIP_VOID);
//LogFile(CStrF("ClipFront %d", pNode->m_iNodeFront));
	CVec3Dfp32 Portal[32];
	memcpy(Portal, _pVPortal, _nVPortal*sizeof(CVec3Dfp32));

	int bClip;
	CPlane3Dfp32 Plane(m_lPlanes[pNode->m_iPlane]);
	Plane.Inverse();
	int Side = Plane.GetArrayPlaneSideMask_Epsilon(_pVPortal, _nVPortal, 0.01f);
	if (Side == 4)
	{
		Portal_Open_r(pNode->m_iNodeFront, Portal, _nVPortal);
	}
	else
	{
		_nVPortal = BSPModel_CutFace3(Portal, _nVPortal, &Plane, 1, bClip);
		if (_nVPortal > 2) Portal_Open_r(pNode->m_iNodeFront, Portal, _nVPortal);
	}
}

void CXR_Model_BSP::Portal_OpenClipBack_r(CBSP_Node* pNode, const CVec3Dfp32* _pVPortal, int _nVPortal)
{
	MAUTOSTRIP(CXR_Model_BSP_Portal_OpenClipBack_r, MAUTOSTRIP_VOID);
//LogFile(CStrF("ClipBack %d", pNode->m_iNodeBack));
	CVec3Dfp32 Portal[32];
	memcpy(Portal, _pVPortal, _nVPortal*sizeof(CVec3Dfp32));

	int bClip;
	int Side = m_lPlanes[pNode->m_iPlane].GetArrayPlaneSideMask_Epsilon(_pVPortal, _nVPortal, 0.01f);
	if (Side == 4)
	{
		Portal_Open_r(pNode->m_iNodeBack, Portal, _nVPortal);
	}
	else
	{
		_nVPortal = BSPModel_CutFace3(Portal, _nVPortal, &m_lPlanes[pNode->m_iPlane], 1, bClip);
		if (_nVPortal > 2) Portal_Open_r(pNode->m_iNodeBack, Portal, _nVPortal);
	}
}

void CXR_Model_BSP::Portal_Open_r(int _iNode, const CVec3Dfp32* _pVPortal, int _nVPortal)
{
	MAUTOSTRIP(CXR_Model_BSP_Portal_Open_r, MAUTOSTRIP_VOID);
	CBSP_Node* pNode = &(m_pNodes[_iNode]);
//LogFile(CStrF("Open %d, F %d, B %d", _iNode, pNode->m_iNodeFront, pNode->m_iNodeBack));
	if (pNode->m_Flags & XW_NODE_PORTAL)
	{
//LogFile(CStrF("   Assign %d", m_pView->m_nRPortals));
		if (m_pView->m_nRPortals >= m_pView->m_MaxRPortals)
		{
//LogFile("   Full");
			ConOut("Full");
			return;
		}

		int iPL = pNode->m_iPortalLeaf;
/*		if (!m_pView->m_pPVS)
		{
			m_pView->m_pPVS = SceneGraph_PVSLock(0, iPL);
		}*/

		CRC_ClipVolume* pRP = &m_pView->m_pRPortals[m_pView->m_nRPortals];
		m_pView->m_liLeafRPortals[iPL] = m_pView->m_nRPortals;
		if (_nVPortal > 16)
		{
			ConOut(CStrF("Portal %d", _nVPortal));
			_nVPortal = 16;
		}

/*		if (ms_pCurrentRC)
		{
			ms_pCurrentRC->Attrib_Push();
			ms_pCurrentRC->Matrix_Push();
			ms_pCurrentRC->Matrix_Set(m_pView->m_CurVMat);
			ms_pCurrentRC->Attrib_Enable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
			ms_pCurrentRC->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CLIP);
			ms_pCurrentRC->Render_WireLoop(_pVPortal, g_IndexRamp32, _nVPortal, 0xff80ff80);
			
			ms_pCurrentRC->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			ms_pCurrentRC->Attrib_TextureID(0,0);
//			ms_pCurrentRC->Render_IndexedPolygon(_nVPortal, _pVPortal, (CVec2Dfp32*) _pVPortal, m_IndexRamp, m_IndexRamp, 0x3f7fff7f);
			ms_pCurrentRC->Matrix_Pop();
			ms_pCurrentRC->Attrib_Pop();
			{
				CStr s;
				for(int i = 0; i < pRP->m_nPlanes; i++)
					s += pRP->m_Vertices[i].GetString();
				ConOut(CStrF("RPortal %d, Node %d, ", m_pView->m_nRPortals, _iNode) + s);
			}
		}*/


		*pRP = m_pView->m_CurClipVolume;
//		pRP->GetVertices(_pVPortal, _nVPortal, m_pView->m_CurLocalVP, false);
//		pRP->CreateFromVertices3(m_pView->m_LocalBackPlane);
		m_pView->m_nRPortals++;
		EnableTreeFromNode(_iNode);


		// Markera alla grannars träd.
		{
			CBSP_PortalLeafExt* pPL = &m_pPortalLeaves[pNode->m_iPortalLeaf];
			int np = pPL->m_nPortals;
			int iip = pPL->m_iiPortals;

			for(int p = 0; p < np; p++)
			{
				int ip = m_piPortals[iip + p];
				CBSP_PortalExt* pP = &m_pPortals[ip];
//#ifndef MODEL_BSP_ALLPORTALSOPEN
#ifdef MODEL_BSP_ALLUSERPORTALSCLOSED
				if (pP->m_PortalID) continue;
#endif
				if (pP->m_PortalID && !m_pView->PortalOpen(pP->m_PortalID)) continue;
//#endif
				EnableTreeFromNode((pP->m_iNodeFront == _iNode) ? pP->m_iNodeBack : pP->m_iNodeFront);
			}
		}

	}
	else if (pNode->IsNode())
	{
		if (!(pNode->m_Flags & XW_NODE_STRUCTURE)) return;

//		const CPlane3Dfp32* pP = &m_lPlanes[pNode->m_iPlane];

/*		int SideMask = 0;
		for(int v = 0; v < _nVPortal; v++)
		{
			SideMask |= pP->GetPlaneSideMask_Epsilon(_pVPortal[v], 0.0001f);
			if ((SideMask & 3) == 3)
			{
				if (pNode->m_iNodeFront)
					Portal_OpenClipFront_r(pNode, _pVPortal, _nVPortal);
				if (pNode->m_iNodeBack)
					Portal_OpenClipBack_r(pNode, _pVPortal, _nVPortal);
				return;
			}
		}

		if ((SideMask & 3 == 1) && pNode->m_iNodeFront)
			Portal_Open_r(pNode->m_iNodeFront, _pVPortal, _nVPortal);
		if ((SideMask & 3 == 2) && pNode->m_iNodeBack)
			Portal_Open_r(pNode->m_iNodeBack, _pVPortal, _nVPortal);*/

		if (pNode->m_iNodeFront)
			Portal_OpenClipFront_r(pNode, _pVPortal, _nVPortal);
		if (pNode->m_iNodeBack)
			Portal_OpenClipBack_r(pNode, _pVPortal, _nVPortal);
	}
}


int CXR_Model_BSP::Portal_GetFirstOpen_r(int _iNode, const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CXR_Model_BSP_Portal_GetFirstOpen_r, 0);
	CBSP_Node* pN = &m_lNodes[_iNode];
	if (pN->m_Flags & XW_NODE_PORTAL)
	{

		return _iNode;
	}
	else if (pN->IsNode())
	{
		if (pN->IsLeaf()) 
			Error("Portal_GetFirstOpen", "*krick*!");
		
	}
	return 0;
}


// -------------------------------------------------------------------
//  Visibility
// -------------------------------------------------------------------

CVec3Dfp32			CXR_Model_BSP::ms_VisVMin = 0;
CVec3Dfp32			CXR_Model_BSP::ms_VisVMax = 0;
CRC_ClipVolume*		CXR_Model_BSP::ms_pVisClipVolume = NULL;
CXR_RenderInfo*		CXR_Model_BSP::ms_pVisRenderInfo = NULL;
int					CXR_Model_BSP::ms_iClosestRPortal = 0;
static fp32			g_HighestPrior = 0;
CXR_FogState*		CXR_Model_BSP::ms_Vis_pSceneFog;

void CXR_Model_BSP::View_Reset(int _iView)
{
	MAUTOSTRIP(CXR_Model_BSP_View_Reset, MAUTOSTRIP_VOID);
	if (!m_lspViews.ValidPos(_iView))
		Error("View_Reset", "Invalid view index.");
	m_pView = m_lspViews[m_iView];

	// Set portals to open by default
	FillChar(m_pView->m_lPortalIDStates.GetBasePtr(), m_pView->m_lPortalIDStates.ListSize(), 1);
}

void CXR_Model_BSP::View_SetState(int _iView, int _State, int _Value)
{
	MAUTOSTRIP(CXR_Model_BSP_View_SetState, MAUTOSTRIP_VOID);
	if (!m_lspViews.ValidPos(_iView))
		Error("View_SetState", "Invalid view index.");
	m_pView = m_lspViews[m_iView];

	if (_State == XR_VIEWCLIPSTATE_PORTAL)
	{
		// 16 LSB = PortalID
		// 16 MSB = Portal state

		int PortalID = _Value & 0xffff;
		int State = _Value >> 16;

		int Shift = PortalID & 7;
		int Pos = PortalID >> 3;
		if (!m_pView->m_lPortalIDStates.ValidPos(Pos)) return;

		if (State)
			m_pView->m_lPortalIDStates[Pos] |= (1 << Shift);
		else
			m_pView->m_lPortalIDStates[Pos] &= ~(1 << Shift);
	}
}

#define DEFINE_THIS_IF_Z_PORTAL_CULLING_CAN_BE_USED

void CXR_Model_BSP::View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_ViewClipInterface* _pViewClip,
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_View_Init, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP::View_Init, XR_BSPMODEL);

	if ((_iView < 0) || (_iView >= m_lspViews.Len()))
		Error("View_Init", "Invalid view index.");
	m_iView = _iView;
	m_pView = m_lspViews[m_iView];
	
	m_Vis_pEngine = _pEngine;
	ms_Vis_pSceneFog = _pEngine->GetFogState();

#ifdef M_Profile
	ms_Time.Reset();
	ms_TimeRenderLeafList.Reset();
	ms_TimeRenderFaceQueue.Reset();
#endif

	_WMat.Multiply(_VMat, m_pView->m_CurVMat);

	// Clear
	{
		int nPL = m_lPortalLeaves.Len();
		m_pView->m_liVisPortalLeaves.SetLen(nPL);
		m_pView->m_MaxVisLeaves = m_pView->m_liVisPortalLeaves.Len();
		m_pView->m_nVisLeaves = 0;
//		m_pView->m_lPVS.SetLen(SceneGraph_PVSGetLen());

		// Initialize space for RPortals.
		m_pView->m_MaxRPortals = MODEL_BSP_NUMRPORTALS;
		m_pView->m_lRPortals.SetLen(m_pView->m_MaxRPortals);
		m_pView->m_pRPortals = &m_pView->m_lRPortals[0];
		m_pView->m_nRPortals = 0;

		m_CurrentView++;
		m_pView->m_CurrentView = m_CurrentView;

//		m_nPortalFaces = 0;
//		m_liPortalFaces.SetLen(CXR_MAXPORTALFACES);
	}


	CMat4Dfp32 WMatInv;
	_WMat.InverseOrthogonal(WMatInv);
	m_pView->m_CurVMat.InverseOrthogonal(m_pView->m_CurVMatInv);

	// Get bound-sphere, get the CRC_ClipView
	{
		CRC_Viewport* pVP = _pRender->Viewport_Get();
		
		fp32 FogCullOffset = _pEngine->GetVarf(XR_ENGINE_FOGCULLOFFSET);
		m_pView->m_ViewBackPlane = pVP->GetBackPlane();
#ifdef DEFINE_THIS_IF_Z_PORTAL_CULLING_CAN_BE_USED
		if (ms_Vis_pSceneFog && ms_Vis_pSceneFog->DepthFogEnable() /*&& m_pSceneFog->m_DepthFogDensity >= 1.0f*/)
			m_pView->m_ViewBackPlane = Min(m_pView->m_ViewBackPlane, ms_Vis_pSceneFog->m_DepthFogEnd + FogCullOffset);
#endif

		CRC_ClipVolume Clip; 
		if (_pViewClip)
		{
			CRC_ClipVolume Clip2; 
			fp32 BoundR = GetBound_Sphere();				// Local
			if (!_pViewClip->View_GetClip_Sphere(CVec3Dfp32::GetMatrixRow(_WMat, 3), BoundR, 0, 0, &Clip2, NULL)) return;

			Clip.CopyAndTransform(Clip2, _VMat);
			m_pView->m_CurClipVolume.CopyAndTransform(Clip2, WMatInv);
		}
		else
		{
			pVP->GetClipVolume(Clip);
			m_pView->m_CurClipVolume.CopyAndTransform(Clip, m_pView->m_CurVMatInv);
		}
		m_pView->m_CurLocalVP = CVec3Dfp32::GetRow(m_pView->m_CurVMatInv, 3);

		// Clip i V-space
		// m_CurClipVolume i L-Space

		CVec2Dfp32 VVert2D[16];
		for(int v = 0; v < Clip.m_nPlanes; v++)
		{
			VVert2D[v].k[0] = Clip.m_Vertices[v].k[0] / Clip.m_Vertices[v].k[2];
			VVert2D[v].k[1] = Clip.m_Vertices[v].k[1] / Clip.m_Vertices[v].k[2];
		}
		CVec2Dfp32::GetMinBoundRect(&VVert2D[1], ms_PortalOrMin, ms_PortalOrMax, Clip.m_nPlanes);

		m_pView->m_BackPlaneInv = 1.0f / m_pView->m_ViewBackPlane;

		// Create back-plane in model-space.
		m_pView->m_LocalBackPlane.n = CVec3Dfp32(0,0,1);
		m_pView->m_LocalBackPlane.d = -m_pView->m_ViewBackPlane;
		m_pView->m_LocalBackPlane.Transform(m_pView->m_CurVMatInv);

		// Create front-plane in model-space.
		m_pView->m_LocalFrontPlane.n = CVec3Dfp32(0,0,1);
		m_pView->m_LocalFrontPlane.d = -pVP->GetFrontPlane();
		m_pView->m_LocalFrontPlane.Transform(m_pView->m_CurVMatInv);
	}
	
	{
		TMeasureResetProfile(ms_Time);

		// Initialize view vertex and mask lists.
	//	m_lVVertMask.SetLen(m_lVertices.Len());
	//	m_lVVertices.SetLen(m_lVertices.Len());
	//	FillChar(&m_lVVertMask[0], m_lVertices.Len(), 0);

		if (m_lPortalLeaves.Len())
		{
			// Initialize nodes RPortal look-up table.
			if (m_pView->m_liLeafRPortals.Len() != m_lPortalLeaves.Len())
				m_pView->m_liLeafRPortals.SetLen(m_lPortalLeaves.Len());
			FillChar(&m_pView->m_liLeafRPortals[0], m_pView->m_liLeafRPortals.ListSize(), 0);

			// Initialize space for RPortals.
			m_pView->m_MaxRPortals = MODEL_BSP_NUMRPORTALS;
			m_pView->m_lRPortals.SetLen(m_pView->m_MaxRPortals);
			m_pView->m_liRPortalNext.SetLen(m_pView->m_MaxRPortals);
			m_pView->m_pRPortals = m_pView->m_lRPortals.GetBasePtr();
			m_pView->m_piRPortalNext = m_pView->m_liRPortalNext.GetBasePtr();
			m_pView->m_nRPortals = 0;
		}
		else
		{
			m_pView->m_MaxRPortals = 2;
			m_pView->m_lRPortals.SetLen(m_pView->m_MaxRPortals);
			m_pView->m_liRPortalNext.SetLen(m_pView->m_MaxRPortals);
			m_pView->m_pRPortals = m_pView->m_lRPortals.GetBasePtr();
			m_pView->m_piRPortalNext = m_pView->m_liRPortalNext.GetBasePtr();
			m_pView->m_nRPortals = 0;
		}

		InitializeListPtrs();

		// --------- Rendering is OK from here ------------------
#ifdef M_Profile
		ms_TimePortalAnd.Reset();
		ms_TimePortalOr.Reset();
		ms_TimePortal0.Reset();
		ms_TimePortal1.Reset();
		ms_TimePortal2.Reset();
		ms_TimePortal3.Reset();
		ms_nPortal0 = 0;
		ms_nPortal1 = 0;
		ms_nPortal2 = 0;
		ms_nPortal3 = 0;
#endif

		if (m_pPortals)
		{
			int iNodePortal = GetPortalLeaf(m_pView->m_CurLocalVP);
			if (iNodePortal == -1)
			{
		//		Error("Render", "No portal leaf!");
				ConOut("No portal leaf!");
	//			_pRender->Attrib_Pop();
				return;
			}

			{
				m_pView->m_nRPortals = 1;
				m_pView->m_pRPortals[m_pView->m_nRPortals] = m_pView->m_CurClipVolume;
				m_pView->m_piRPortalNext[m_pView->m_nRPortals] = 0;
				m_pView->m_nRPortals = 2;
				m_pView->m_liLeafRPortals[m_pNodes[iNodePortal].m_iPortalLeaf] = 1;
			}

			//		if (!m_pView->m_lPVS.GetBasePtr()) Error("Render", "No PVS.");
			//		CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[m_lNodes[iNodePortal].m_iPortalLeaf];
			//		int PVSLen = CBS_Uncompress(&m_lPVS[pPL->m_iPVS], pPL->m_PVSBitLen, m_pView->m_lPVS.GetBasePtr());
			//ConOut(CStrF("PL %d, iPVS %d, PVSLen %d", m_lNodes[iNodePortal].m_iPortalLeaf, pPL->m_iPVS, pPL->m_PVSBitLen));
			//		uint8* pPVS = m_pView->m_lPVS.GetBasePtr();
			g_nPotentialPVSRejects = 0;

			{
				TMeasureResetProfile(ms_TimePortals);
		#ifndef MODEL_BSP_DISABLE_PVS
				m_pView->m_pPVS = SceneGraph_PVSLock(0, m_lNodes[iNodePortal].m_iPortalLeaf);
		#endif
					DisableTree(1);
					EnableTreeFromNode(iNodePortal);
					Portal_AddNode(1, 1);
					DisableTree(1);
		#ifndef MODEL_BSP_DISABLE_PVS
				SceneGraph_PVSRelease(m_pView->m_pPVS);
		#endif
				m_pView->m_pPVS = NULL;
			}
		}

		// Add plane for vertex-fog clipping
		bool bVtxFog = _pEngine && _pEngine->GetFogState()->VertexFogEnable();

	//	if (bVtxFog || bDepthFog)
		{
			fp32 FogCullOffset = _pEngine->GetVarf(XR_ENGINE_FOGCULLOFFSET);
			fp32 ViewBackPlane = m_pView->m_ViewBackPlane;
			if (ms_Vis_pSceneFog && ms_Vis_pSceneFog->DepthFogEnable() /*&& m_pSceneFog->m_DepthFogDensity >= 1.0f*/)
				ViewBackPlane = Min(m_pView->m_ViewBackPlane, ms_Vis_pSceneFog->m_DepthFogEnd + FogCullOffset);

			// Create back-plane in model-space.
			CPlane3Dfp32 LocalBackPlane; 
			LocalBackPlane.n = CVec3Dfp32(0,0,1);
			LocalBackPlane.d = -ViewBackPlane;
			LocalBackPlane.Transform(m_pView->m_CurVMatInv);

			CXR_FogState* pSceneFog = _pEngine->GetFogState();
			for(int i = 1; i < m_pView->m_nRPortals; i++)
			{
				CRC_ClipVolume* pClip = &m_pView->m_pRPortals[i];
				if (bVtxFog)
					if (pClip->m_nPlanes < CRC_CLIPVOLUME_MAXPLANES)
						pClip->m_Planes[pClip->m_nPlanes++] = pSceneFog->m_VtxFog_EndPlane;

				if (pClip->m_nPlanes < CRC_CLIPVOLUME_MAXPLANES)
					pClip->m_Planes[pClip->m_nPlanes++] = LocalBackPlane;
			}
		}

		// Add plane for portal plane clipping
		if (_pEngine && _pEngine->GetVCDepth())
		{
			CPlane3Dfp32 Plane = _pEngine->GetVC()->m_FrontPlaneW;
			for(int i = 1; i < m_pView->m_nRPortals; i++)
			{
				CRC_ClipVolume* pClip = &m_pView->m_pRPortals[i];
				if (pClip->m_nPlanes < CRC_CLIPVOLUME_MAXPLANES)
					pClip->m_Planes[pClip->m_nPlanes++] = Plane;
			}
		}


	#ifdef MODEL_BSP_WRITEPORTALTIMING
		if (m_pPortals && !(ms_Enable & MODEL_BSP_ENABLE_NOPORTALSTATS))
		{
				int iNodePortal = GetPortalLeaf(m_pView->m_CurLocalVP);
				int iPL = m_lNodes[iNodePortal].m_iPortalLeaf;
			ConOut(CStrF("iPL %d, iNode %d, nPL %d, nPotPVSCull %d, nRP %d, ", iPL, iNodePortal, m_pView->m_nVisLeaves, g_nPotentialPVSRejects, m_pView->m_nRPortals) + 
				TString("P ", ms_TimePortals) + 
				TString(", And", ms_TimePortalAnd) + 
				TString(", Or", ms_TimePortalOr) +
				TString(", (0 nAnd)", ms_TimePortal0) + CStrF(" %d, ", ms_nPortal0) +
				TString(", (1)", ms_TimePortal1) + CStrF(" %d, ", ms_nPortal1) +
				TString(", (2 clpsv)", ms_TimePortal2) + CStrF(" %d, ", ms_nPortal2) +
				TString(", (3 nOpen)", ms_TimePortal3) + CStrF(" %d, ", ms_nPortal3)
				);
		}
	#endif
	}
}

void CXR_Model_BSP::View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CVec3Dfp32* _pVPortal, int _nVPortal,
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_View_Init_2, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP::View_Init, XR_BSPMODEL);

	if ((_iView < 0) || (_iView >= m_lspViews.Len()))
		Error("View_Init", "Invalid view index.");
	m_iView = _iView;
	m_pView = m_lspViews[m_iView];

	m_Vis_pEngine = _pEngine;
	ms_Vis_pSceneFog = _pEngine->GetFogState();

#ifdef M_Profile
	ms_Time.Reset();
	ms_TimeRenderLeafList.Reset();
	ms_TimeRenderFaceQueue.Reset();
#endif

	_WMat.Multiply(_VMat, m_pView->m_CurVMat);

	// Clear
	{
		int nPL = m_lPortalLeaves.Len();
		m_pView->m_liVisPortalLeaves.SetLen(nPL);
		m_pView->m_MaxVisLeaves = m_pView->m_liVisPortalLeaves.Len();
		m_pView->m_nVisLeaves = 0;
//		m_pView->m_lPVS.SetLen(SceneGraph_PVSGetLen());

		// Initialize space for RPortals.
		m_pView->m_MaxRPortals = MODEL_BSP_NUMRPORTALS;
		m_pView->m_lRPortals.SetLen(m_pView->m_MaxRPortals);
		m_pView->m_liRPortalNext.SetLen(m_pView->m_MaxRPortals);
		m_pView->m_pRPortals = m_pView->m_lRPortals.GetBasePtr();
		m_pView->m_piRPortalNext = m_pView->m_liRPortalNext.GetBasePtr();
		m_pView->m_nRPortals = 0;
//		m_liPortalFaces.SetLen(CXR_MAXPORTALFACES);
//		m_nPortalFaces = 0;

		m_CurrentView++;
		m_pView->m_CurrentView = m_CurrentView;
	}


	CMat4Dfp32 WMatInv;
	_WMat.InverseOrthogonal(WMatInv);
	m_pView->m_CurVMat.InverseOrthogonal(m_pView->m_CurVMatInv);

	// Get bound-sphere, get the CRC_ClipView
	{
		CRC_Viewport* pVP = _pRender->Viewport_Get();
		
		m_pView->m_ViewBackPlane = pVP->GetBackPlane();
#ifdef DEFINE_THIS_IF_Z_PORTAL_CULLING_CAN_BE_USED
		if (ms_Vis_pSceneFog && ms_Vis_pSceneFog->DepthFogEnable() /*&& m_pSceneFog->m_DepthFogDensity >= 1.0f*/)
			m_pView->m_ViewBackPlane = Min(m_pView->m_ViewBackPlane, ms_Vis_pSceneFog->m_DepthFogEnd);
#endif

		m_pView->m_CurLocalVP = CVec3Dfp32::GetRow(m_pView->m_CurVMatInv, 3);
		CRC_ClipVolume Clip; 
		if (_pVPortal)
		{
			m_pView->m_CurClipVolume.GetVertices(_pVPortal, _nVPortal, m_pView->m_CurLocalVP, false);
			Clip.CopyAndTransform(m_pView->m_CurClipVolume, _VMat);
		}
		else
		{
			pVP->GetClipVolume(Clip);
			m_pView->m_CurClipVolume.CopyAndTransform(Clip, m_pView->m_CurVMatInv);
		}

		// Clip i V-space
		// m_CurClipVolume i L-Space

		CVec2Dfp32 VVert2D[16];
		for(int v = 0; v < Clip.m_nPlanes; v++)
		{
			VVert2D[v].k[0] = Clip.m_Vertices[v].k[0] / Clip.m_Vertices[v].k[2];
			VVert2D[v].k[1] = Clip.m_Vertices[v].k[1] / Clip.m_Vertices[v].k[2];
		}
		CVec2Dfp32::GetMinBoundRect(&VVert2D[1], ms_PortalOrMin, ms_PortalOrMax, Clip.m_nPlanes);

		m_pView->m_BackPlaneInv = 1.0f / m_pView->m_ViewBackPlane;

		// Create back-plane in model-space.
		m_pView->m_LocalBackPlane.n = CVec3Dfp32(0,0,1);
		m_pView->m_LocalBackPlane.d = -m_pView->m_ViewBackPlane;
		m_pView->m_LocalBackPlane.Transform(m_pView->m_CurVMatInv);

		// Create front-plane in model-space.
		m_pView->m_LocalFrontPlane.n = CVec3Dfp32(0,0,1);
		m_pView->m_LocalFrontPlane.d = -pVP->GetFrontPlane();
		m_pView->m_LocalFrontPlane.Transform(m_pView->m_CurVMatInv);
	}

	// Get bound-sphere, get the CRC_ClipView
/*	{
		CRC_Viewport* pVP = _pRender->Viewport_Get();
		m_pView->m_ViewBackPlane = pVP->GetBackPlane();
		m_pView->m_CurLocalVP = CVec3Dfp32::GetRow(m_pView->m_CurVMatInv, 3);

		CRC_ClipVolume Clip; 
		if (_pVPortal)
		{
			m_pView->m_CurClipVolume.GetVertices(_pVPortal, _nVPortal, m_pView->m_CurLocalVP, false);
			Clip.CopyAndTransform(m_pView->m_CurClipVolume, _VMat);
		}
		else
		{
			pVP->GetClipVolume(Clip);
			m_pView->m_CurClipVolume.CopyAndTransform(Clip, m_pView->m_CurVMatInv);
		}

		// Clip i V-space
		// m_CurClipVolume i L-Space

		CVec2Dfp32 VVert2D[16];
		for(int v = 0; v < Clip.m_nPlanes; v++)
		{
			VVert2D[v].k[0] = Clip.m_Vertices[v].k[0] / Clip.m_Vertices[v].k[2];
			VVert2D[v].k[1] = Clip.m_Vertices[v].k[1] / Clip.m_Vertices[v].k[2];
		}
		CVec2Dfp32::GetMinBoundRect(&VVert2D[1], ms_PortalOrMin, ms_PortalOrMax, Clip.m_nPlanes);

		// Create back-plane in model-space.
		m_pView->m_LocalBackPlane.n = CVec3Dfp32(0,0,1);
		m_pView->m_LocalBackPlane.d = -pVP->GetBackPlane();
		m_pView->m_LocalBackPlane.Transform(m_pView->m_CurVMatInv);

		// Create front-plane in model-space.
		m_pView->m_LocalFrontPlane.n = CVec3Dfp32(0,0,1);
		m_pView->m_LocalFrontPlane.d = -pVP->GetFrontPlane();
		m_pView->m_LocalFrontPlane.Transform(m_pView->m_CurVMatInv);

		m_pView->m_CurClipVolume.CreateFromVertices3(m_pView->m_LocalBackPlane);
	}*/

	// Get clip mask
	{
//		fp32 BoundR = GetBound_Sphere();
//		int ViewMask = _pRender->Viewport_Get()->SphereInView(CVec3Dfp32::GetMatrixRow(m_pView->m_CurVMat, 3), BoundR);

		// FIXME:
		// If *this doesn't have a pViewClip, it's probably the world. If it's not rendered
		// there will be bugs when objects try to use it's view-interface. 
		// This prevents it from exiting.
		//if (!_pViewClip) 
		//	if (!ViewMask) return;
	}
	
	{
		TMeasureResetProfile(ms_Time);

		// Initialize view vertex and mask lists.
	//	m_lVVertMask.SetLen(m_lVertices.Len());
	//	m_lVVertices.SetLen(m_lVertices.Len());
	//	FillChar(&m_lVVertMask[0], m_lVertices.Len(), 0);

		if (m_lPortalLeaves.Len())
		{
			// Initialize nodes RPortal look-up table.
			if (m_pView->m_liLeafRPortals.Len() != m_lPortalLeaves.Len())
			{
				m_pView->m_liLeafRPortals.SetLen(m_lPortalLeaves.Len());
	//			m_pView->m_lnLeafRPortals.SetLen(m_lPortalLeaves.Len());
			}
			FillChar(&m_pView->m_liLeafRPortals[0], m_pView->m_liLeafRPortals.ListSize(), 0);
	//		FillChar(&m_pView->m_lnLeafRPortals[0], m_pView->m_lnLeafRPortals.ListSize(), 0);

			// Initialize space for RPortals.
			m_pView->m_MaxRPortals = MODEL_BSP_NUMRPORTALS;
			m_pView->m_lRPortals.SetLen(m_pView->m_MaxRPortals);
			m_pView->m_liRPortalNext.SetLen(m_pView->m_MaxRPortals);
			m_pView->m_pRPortals = m_pView->m_lRPortals.GetBasePtr();
			m_pView->m_piRPortalNext = m_pView->m_liRPortalNext.GetBasePtr();
			m_pView->m_nRPortals = 0;
		}
		else
		{
			m_pView->m_MaxRPortals = 2;
			m_pView->m_lRPortals.SetLen(m_pView->m_MaxRPortals);
			m_pView->m_liRPortalNext.SetLen(m_pView->m_MaxRPortals);
			m_pView->m_pRPortals = m_pView->m_lRPortals.GetBasePtr();
			m_pView->m_piRPortalNext = m_pView->m_liRPortalNext.GetBasePtr();
			m_pView->m_nRPortals = 0;
		}

		InitializeListPtrs();

		// --------- Rendering is OK from here ------------------
#ifdef M_Profile
		ms_TimePortalAnd.Reset();
		ms_TimePortalOr.Reset();
		ms_TimePortal0.Reset();
		ms_TimePortal1.Reset();
		ms_TimePortal2.Reset();
		ms_TimePortal3.Reset();
		ms_nPortal0 = 0;
		ms_nPortal1 = 0;
		ms_nPortal2 = 0;
		ms_nPortal3 = 0;
#endif

		if (m_pPortals)
		{
	//		int iNodePortal = GetPortalLeaf(m_pView->m_CurLocalVP, );
			CVec3Dfp32 Mid(0);
			for(int v = 0; v < _nVPortal; v++)
				Mid += _pVPortal[v];
			Mid *= 1.0f/fp32(_nVPortal);
	//		ConOut(Mid.GetString());
	//		ConOut(CStrF("%d, ", _nVPortal) + _pVPortal[0].GetString());

			{
				TMeasureResetProfile(ms_TimePortals);

				DisableTree(1);
				m_pView->m_nRPortals = 1;

		//		int iNodePortal = Portal_GetFirstOpen(m_pView->m_CurLocalVP);

//				int iNodePortal = GetPortalLeaf(Mid);
				m_pView->m_nRPortals = 1;


				m_pView->m_pPVS = NULL;
				Portal_Open_r(1, _pVPortal, _nVPortal);

				if (m_pView->m_nRPortals > 1)
				{
		//				EnableTreeFromNode(iNodePortal);
					Portal_AddNode(1, 1);
				}
				SceneGraph_PVSRelease(m_pView->m_pPVS);
		//		ConOut(CStrF("nRPortals %d", m_pView->m_nRPortals));
				DisableTree(1);


				// Add plane for vertex-fog clipping
				if (_pEngine && _pEngine->GetFogState()->VertexFogEnable())
				{
					CXR_FogState* pSceneFog = _pEngine->GetFogState();
					for(int i = 1; i < m_pView->m_nRPortals; i++)
					{
						CRC_ClipVolume* pClip = &m_pView->m_pRPortals[i];
						if (pClip->m_nPlanes < CRC_CLIPVOLUME_MAXPLANES)
							pClip->m_Planes[pClip->m_nPlanes++] = pSceneFog->m_VtxFog_EndPlane;
					}
				}

				// Add plane for portal plane clipping
				if (_pEngine && _pEngine->GetVCDepth())
				{
					CPlane3Dfp32 Plane = _pEngine->GetVC()->m_FrontPlaneW;
					for(int i = 1; i < m_pView->m_nRPortals; i++)
					{
						CRC_ClipVolume* pClip = &m_pView->m_pRPortals[i];
						if (pClip->m_nPlanes < CRC_CLIPVOLUME_MAXPLANES)
							pClip->m_Planes[pClip->m_nPlanes++] = Plane;
					}
				}
			}
		}
	}
}


void CXR_Model_BSP::View_SetCurrent(int _iView, CXR_SceneGraphInstance* _pSceneGraphInstance)
{
	MAUTOSTRIP(CXR_Model_BSP_View_SetCurrent, MAUTOSTRIP_VOID);
	m_iView = _iView;
	m_pView = m_lspViews[m_iView];
//	m_pView->m_pSceneGraphInstance = _pSceneGraphInstance;
}

bool CXR_Model_BSP::View_GetClip_Sphere_r(int _iNode, const CVec3Dfp32& _v0, fp32 _Radius)
{
	MAUTOSTRIP(CXR_Model_BSP_View_GetClip_Sphere_r, false);
	if (!_iNode) return false;
	CBSP_Node* pN = &m_pNodes[_iNode];
	if (pN->m_Flags & XW_NODE_PORTAL)
	{
		int iPL = pN->m_iPortalLeaf;
		int iRP = m_pView->m_liLeafRPortals[iPL];
		if (iRP != 0)
		{
			if (m_pView->m_pRPortals[iRP].m_nPlanes < 3) return false;
			CRC_ClipVolume* pClip = &m_pView->m_pRPortals[iRP];

			int p;
			for (p = 0; p < pClip->m_nPlanes; p++)
				if (pClip->m_Planes[p].Distance(_v0) > _Radius) break;
			if (p != pClip->m_nPlanes) return false;

			if (ms_pVisClipVolume)
			{
				Portal_Or(ms_pVisClipVolume, pClip);
			}

			const CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[iPL];

			if (ms_pVisRenderInfo)
				ms_pVisRenderInfo->m_MediumFlags |= m_lMediums[pPL->m_iMedium].m_MediumFlags;

			if (ms_pVisRenderInfo)
//				if (iRP < ms_iClosestRPortal)
				if (pPL->m_BasePriority > g_HighestPrior)
			{
				g_HighestPrior = pPL->m_BasePriority;

				ms_iClosestRPortal = iRP;

				// Set light-volume
				if (pPL->m_iLightVolume)
					ms_pVisRenderInfo->m_pLightVolume = &m_lLightVolumeInfo[pPL->m_iLightVolume];

				// NHF?
				ms_pVisRenderInfo->m_Flags = 0;
//				if (ms_Enable & MODEL_BSP_ENABLE_FOG)
				if (ms_Vis_pSceneFog && ms_Vis_pSceneFog->NHFEnable())
					if (m_lMediums[pPL->m_iMedium].m_MediumFlags & XW_MEDIUM_FOG)
						ms_pVisRenderInfo->m_Flags |= CXR_RENDERINFO_NHF;

				if (ms_Vis_pSceneFog && ms_Vis_pSceneFog->DepthFogEnable())
					ms_pVisRenderInfo->m_Flags |= CXR_RENDERINFO_DEPTHFOG;

				if (ms_Vis_pSceneFog && ms_Vis_pSceneFog->VertexFogEnable())
					ms_pVisRenderInfo->m_Flags |= CXR_RENDERINFO_VERTEXFOG;

				// Set rendering proirities.
				ms_pVisRenderInfo->m_BasePriority_Opaque = pPL->m_BasePriority + CXR_VBPRIORITY_MODEL_OPAQUE;
				ms_pVisRenderInfo->m_BasePriority_Transparent = pPL->m_BasePriority + CXR_VBPRIORITY_MODEL_TRANSPARENT;
				fp32 Dist = m_pView->m_LocalFrontPlane.Distance(_v0);
				fp32 dPrior = 1.0f - Dist*m_pView->m_BackPlaneInv;
				ms_pVisRenderInfo->m_BasePriority_Transparent += dPrior;
			}

			return true;
		}
		return false;
	}
	else if (pN->IsNode())
	{
		fp32 Dist = m_pPlanes[pN->m_iPlane].Distance(_v0);
		bool bVis = false;
		if (Dist > -_Radius)
			bVis = View_GetClip_Sphere_r(pN->m_iNodeFront, _v0, _Radius);

		if (Dist < _Radius) 
			if (ms_pVisRenderInfo || ms_pVisClipVolume || !bVis)
				bVis |= View_GetClip_Sphere_r(pN->m_iNodeBack, _v0, _Radius);

		return bVis;
	}
	return false;
}

bool CXR_Model_BSP::View_GetClip_Sphere(CVec3Dfp32 _v0, fp32 _Radius, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_View_GetClip_Sphere, false);
//	GetClipFromPortalLeaf_Point(_v0, _pClipVolume);

// URGENTFIXME,  Not true: Sphere <=> Point

/*	int iNode = GetPortalLeaf(_v0);
	int iRP = m_pView->m_liLeafRPortals[m_pNodes[iNode].m_iPortalLeaf];
	if (iRP != 0)
	{
		if (m_pView->m_pRPortals[iRP].m_nPlanes <= 0) return false;
		(*_pClipVolume) = m_pView->m_pRPortals[m_pView->m_liLeafRPortals[m_pNodes[iNode].m_iPortalLeaf]];
		return true;
	}
	return false;
*/
//LogFile(CStrF("(CXR_Model_BSP::View_GetClip_Sphere) this %.8x, vtbl %.8x", this, *(uint32*)this));
	ms_Vis_pSceneFog = m_Vis_pEngine->GetFogState();

	m_pView = m_lspViews[m_iView];
	m_pNodes = m_lNodes.GetBasePtr();
	m_pPlanes = m_lPlanes.GetBasePtr();

	ms_pVisClipVolume = _pClipVolume;
	ms_pVisRenderInfo = _pRenderInfo;
	ms_iClosestRPortal = 0x7fffffff;
	g_HighestPrior = -10000000;

	if (_pRenderInfo) _pRenderInfo->Clear(_pRenderInfo->m_pCurrentEngine);

	if (_pClipVolume)
	{
		_pClipVolume->Init(m_pView->m_CurLocalVP);
		View_GetClip_Sphere_r(1, _v0, _Radius);
		if (_pRenderInfo && _pClipVolume->m_nPlanes >= 3)
		{
			int iPL = GetStructurePortalLeaf(_v0);
			if (iPL > 0)
			{
				const CBSP_PortalLeaf* pPL = &m_lPortalLeaves[iPL];
				if (pPL->m_iLightVolume)
					ms_pVisRenderInfo->m_pLightVolume = &m_lLightVolumeInfo[pPL->m_iLightVolume];
			}
		}
		return (_pClipVolume->m_nPlanes >= 3);
	}
	else
	{
		if (View_GetClip_Sphere_r(1, _v0, _Radius))
		{
			if (_pRenderInfo)
			{
				int iPL = GetStructurePortalLeaf(_v0);
				if (iPL > 0)
				{
					const CBSP_PortalLeaf* pPL = &m_lPortalLeaves[iPL];
					if (pPL->m_iLightVolume)
						ms_pVisRenderInfo->m_pLightVolume = &m_lLightVolumeInfo[pPL->m_iLightVolume];
				}
			}
			return true;
		}
		else
			return false;
	}
}

bool CXR_Model_BSP::View_GetClip_Box_r(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_View_GetClip_Box_r, false);
	MSCOPESHORT(CXR_Model_BSP::View_GetClip_Box_r);
	if (!_iNode) return false;
	CBSP_Node* pN = &m_pNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		int iRP = m_pView->m_liLeafRPortals[pN->m_iPortalLeaf];
		if (iRP != 0)
		{
			if (m_pView->m_pRPortals[iRP].m_nPlanes < 3) return false;
			CRC_ClipVolume* pClip = &m_pView->m_pRPortals[iRP];

			int p;
			for (p = 0; p < pClip->m_nPlanes; p++)
				if (pClip->m_Planes[p].GetBoxMinDistance(ms_VisVMin, ms_VisVMax) > MODEL_BSP_EPSILON) break;
			if (p != pClip->m_nPlanes) return false;

//	if (_pClipVolume->m_nPlanes) ConOut("Vis-OR");
			if (ms_pVisClipVolume)
				Portal_Or(ms_pVisClipVolume, pClip);

			const CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[pN->m_iPortalLeaf];

			// NHF?
			if (ms_pVisRenderInfo)
			{
				CXR_MediumDesc* pMedium = &m_lMediums[pPL->m_iMedium];
				ms_pVisRenderInfo->m_MediumFlags |= pMedium->m_MediumFlags;

//				if (ms_Enable & MODEL_BSP_ENABLE_FOG)
				if (ms_Vis_pSceneFog && ms_Vis_pSceneFog->NHFEnable())
					if (pMedium->m_MediumFlags & XW_MEDIUM_FOG)
						ms_pVisRenderInfo->m_Flags |= CXR_RENDERINFO_NHF;

				if (ms_Vis_pSceneFog && ms_Vis_pSceneFog->DepthFogEnable())
					ms_pVisRenderInfo->m_Flags |= CXR_RENDERINFO_DEPTHFOG;

				if (ms_Vis_pSceneFog && ms_Vis_pSceneFog->VertexFogEnable())
					ms_pVisRenderInfo->m_Flags |= CXR_RENDERINFO_VERTEXFOG;

				if (pPL->m_BasePriority > g_HighestPrior)
//				if (iRP < ms_iClosestRPortal)
				{
					g_HighestPrior = pPL->m_BasePriority;
					ms_iClosestRPortal = iRP;

					// Set light-volume
					if (pPL->m_iLightVolume)
						ms_pVisRenderInfo->m_pLightVolume = &m_lLightVolumeInfo[pPL->m_iLightVolume];

					// Set rendering priorities.
					ms_pVisRenderInfo->m_BasePriority_Opaque = pPL->m_BasePriority + CXR_VBPRIORITY_MODEL_OPAQUE;
					ms_pVisRenderInfo->m_BasePriority_Transparent = pPL->m_BasePriority + CXR_VBPRIORITY_MODEL_TRANSPARENT;
					CVec3Dfp32 Pos;
					ms_VisVMax.Lerp(ms_VisVMin, 0.5f, Pos);
					fp32 Dist = m_pView->m_LocalFrontPlane.Distance(Pos);
					fp32 dPrior = 1.0f - Dist*m_pView->m_BackPlaneInv;
					if (dPrior < -0.25f)
						dPrior = -0.25f;
					else if (dPrior > 1.25f)
						dPrior = 1.25f;
/*				if (dPrior < -0.5f)
				{
					ConOutL(CStrF("dPrior %f, Dist %f, %f, %s - %s", dPrior, Dist, Dist*m_pView->m_BackPlaneInv, ms_VisVMin.GetString().Str(), ms_VisVMax.GetString().Str()));
				}*/
					ms_pVisRenderInfo->m_BasePriority_Transparent += dPrior;
				}
			}

			return true;
		}
		return false;
	}

	if (pN->IsNode())
	{
#ifdef MODEL_BSP_USEPLANETYPES
		int Type = pN->m_Flags & XW_NODE_PLANETYPEANDSIGN;
		if (!Type)
		{
#endif
			int iBack = pN->m_iNodeBack;
			if (!iBack)
			{
				int iFront = pN->m_iNodeFront;
				if (m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(ms_VisVMin, ms_VisVMax) > -MODEL_BSP_EPSILON)
					return View_GetClip_Box_r(iFront);
				return false;
			}
			else
			{
				int iFront = pN->m_iNodeFront;
				if (!iFront)
				{
					if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(ms_VisVMin, ms_VisVMax) < MODEL_BSP_EPSILON)
						return View_GetClip_Box_r(iBack);
					return false;
				}
				else
				{
					bool bVis = false;
					if (m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(ms_VisVMin, ms_VisVMax) > -MODEL_BSP_EPSILON)
						bVis = View_GetClip_Box_r(iFront);

					if (!(ms_pVisClipVolume || ms_pVisRenderInfo) && bVis) return true;
					if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(ms_VisVMin, ms_VisVMax) < MODEL_BSP_EPSILON)
						bVis |= View_GetClip_Box_r(iBack);

					return bVis;
//					int Side = m_pPlanes[pN->m_iPlane].GetBoxPlaneSideMask(ms_VisVMin, ms_VisVMax);
//					if (Side & 5) View_GetClip_Box_r(pN->m_iNodeFront);
//					if (Side & 6) View_GetClip_Box_r(pN->m_iNodeBack);
				}
			}
#ifdef MODEL_BSP_USEPLANETYPES
		}
		else
		{
			CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
			int k = ((Type & XW_NODE_PLANETYPEAND) >> XW_NODE_PLANETYPESHIFT)-1;

			int iBack = pN->m_iNodeBack;
			int iFront = pN->m_iNodeFront;
			fp32 d = pP->d;
			if (Type & XW_NODE_PLANENEG)
			{
				bool bVis = false;
				if (iFront && (ms_VisVMin.k[k] < d))
					bVis = View_GetClip_Box_r(iFront);
				if (!ms_pVisClipVolume && bVis) return true;
				if (iBack && (ms_VisVMax.k[k] > d))
					bVis |= View_GetClip_Box_r(iBack);
				return bVis;
			}
			else
			{
				bool bVis = false;
				if (iFront && (ms_VisVMax.k[k] + d > 0.0f))
					bVis = View_GetClip_Box_r(iFront);
				if (!(ms_pVisClipVolume || ms_pVisRenderInfo) && bVis) return true;
				if (iBack && (ms_VisVMin.k[k] + d < 0.0f))
					bVis |= View_GetClip_Box_r(iBack);
				return bVis;
			}
		}
#endif
/*		int Side = m_pPlanes[pN->m_iPlane].GetBoxPlaneSideMask(ms_VisVMin, ms_VisVMax);
		if (Side & 1) View_GetClip_Box_r(pN->m_iNodeFront, ms_VisVMin, ms_VisVMax, _pClipVolume);
		if (Side & 2) View_GetClip_Box_r(pN->m_iNodeBack, ms_VisVMin, ms_VisVMax, _pClipVolume);*/
	}
	return false;
}

bool CXR_Model_BSP::View_GetClip_Box(CVec3Dfp32 _VMin, CVec3Dfp32 _VMax, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_View_GetClip_Box, false);
	MSCOPESHORT(CXR_Model_BSP::View_GetClip_Box);
	ms_Vis_pSceneFog = m_Vis_pEngine->GetFogState();

	m_pNodes = m_lNodes.GetBasePtr();
	m_pPlanes = m_lPlanes.GetBasePtr();

	ms_VisVMin = _VMin;
	ms_VisVMax = _VMax;
	ms_pVisClipVolume = _pClipVolume;
	ms_pVisRenderInfo = _pRenderInfo;
	ms_iClosestRPortal = 0x7fffffff;
	g_HighestPrior = -10000000;

	if (_pRenderInfo) _pRenderInfo->Clear(_pRenderInfo->m_pCurrentEngine);

	if (_pClipVolume)
	{
		_pClipVolume->Init(m_pView->m_CurLocalVP);
		View_GetClip_Box_r(1);

		if (_pRenderInfo && _pClipVolume->m_nPlanes >= 3)
		{
			if (m_spLightOcttree != NULL)
			{
				ms_pVisRenderInfo->m_pLightVolume = m_spLightOcttree;
			}
			else
			{
				CVec3Dfp32 Center;
				_VMin.Lerp(_VMax, 0.5f, Center);
				int iPL = GetStructurePortalLeaf(Center);
				if (iPL > 0)
				{
					const CBSP_PortalLeaf* pPL = &m_lPortalLeaves[iPL];
					if (pPL->m_iLightVolume)
						ms_pVisRenderInfo->m_pLightVolume = &m_lLightVolumeInfo[pPL->m_iLightVolume];
				}
			}
		}
		return (_pClipVolume->m_nPlanes >= 3);
	}
	else
		if (View_GetClip_Box_r(1))
		{
			if (_pRenderInfo)
			{
				if (m_spLightOcttree != NULL)
				{
					ms_pVisRenderInfo->m_pLightVolume = m_spLightOcttree;
				}
				else
				{
					CVec3Dfp32 Center;
					_VMin.Lerp(_VMax, 0.5f, Center);
					int iPL = GetStructurePortalLeaf(Center);
					if (iPL > 0)
					{
						const CBSP_PortalLeaf* pPL = &m_lPortalLeaves[iPL];
						if (pPL->m_iLightVolume)
							ms_pVisRenderInfo->m_pLightVolume = &m_lLightVolumeInfo[pPL->m_iLightVolume];
					}
				}
			}
			return true;
		}
		else
			return false;
}

/*void CXR_Model_BSP::View_SetPortalState(int _iPortal, int _State)
{
	MAUTOSTRIP(CXR_Model_BSP_View_SetPortalState, NULL);
	if ((_iPortal < 0) || (_iPortal >= XW_MAXUSERPORTALS)) return;

	m_PortalIDState[_iPortal] = _State;

}*/

