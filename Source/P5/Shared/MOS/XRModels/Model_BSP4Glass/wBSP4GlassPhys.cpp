#include "PCH.h"
#include "WBSP4Glass.h"

#include "../../Classes/Render/MWireContainer.h"


#if GLASS_OPTIMIZE_OFF
#pragma xrMsg("optimize off!")
#pragma optimize("", off)
#pragma inline_depth(0)
#endif


// -------------------------------------------------------------------
// Physics interface overrides
// -------------------------------------------------------------------
void CXR_Model_BSP4Glass::__PhysGlass_RenderFace(int _iFace, const CMat4Dfp32& _WMat, CWireContainer* _pWC, int _Col) const
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::__PhysGlass_RenderFace, XR_BSP4GLASS);

	const CBSP4_Face* pF = &m_pFaces[_iFace];

	int iLast = m_piVertices[pF->m_iiVertices + pF->m_nVertices - 1];
	CVec3Dfp32 p0 = m_pVertices[iLast] * _WMat;
	for(int i = 0; i < pF->m_nVertices; i++)
	{
		int iVertex = m_piVertices[pF->m_iiVertices + i];
		CVec3Dfp32 p1 = m_pVertices[iVertex] * _WMat;
		_pWC->RenderWire(p0, p1, _Col, 0.0f, false);
		p0	= p1;
	}

	const CVec3Dfp32 VertexOffset = -m_pPlanes[pF->m_iPlane].n * m_pMediums[pF->m_iBackMedium].m_Thickness;
	iLast = m_piVertices[pF->m_iiVertices + pF->m_nVertices - 1];
	p0 = (m_pVertices[iLast] + VertexOffset) * _WMat;
	for(int i = 0; i < pF->m_nVertices; i++)
	{
		// Oops...
		//int iVertex = m_piVertices[pF->m_iiVertices - (pF->m_nVertices - i) - 1];
		int iVertex = m_piVertices[pF->m_iiVertices + i];
		CVec3Dfp32 p1 = (m_pVertices[iVertex] + VertexOffset) * _WMat;
		_pWC->RenderWire(p0, p1, _Col, 0.0f, false);
		p0  = p1;
	}
}

void CXR_Model_BSP4Glass::Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _v0, fp32& _Radius)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::Phys_GetBound_Sphere, XR_BSP4GLASS);
	CXR_Model_BSP4::Phys_GetBound_Sphere(_Pos, _v0, _Radius);
}

void CXR_Model_BSP4Glass::Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _Box)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::Phys_GetBound_Box, XR_BSP4GLASS);
	CXR_Model_BSP4::Phys_GetBound_Box(_Pos, _Box);
}

void CXR_Model_BSP4Glass::Phys_GetBound_BoxInstance(const uint32& _iInstance, const CMat4Dfp32& _Pos, CBox3Dfp32& _Box)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::Phys_GetBound_BoxInstance, XR_BSP4GLASS);

	// Set box to current instance, and run bound box calculation using current bounding
	_Box = m_lGlassIndices[_iInstance].m_BoundingBox;
	
	CVec3Dfp32 E;
	CVec3Dfp32 E2;
	CVec3Dfp32 C;
	_Box.m_Max.Lerp(_Box.m_Min, 0.5f, C);
	_Box.m_Max.Sub(C, E);

	E2[0] = M_Fabs(E[0] * _Pos.k[0][0]) + M_Fabs(E[1] * _Pos.k[1][0]) + M_Fabs(E[2] * _Pos.k[2][0]);
	E2[1] = M_Fabs(E[0] * _Pos.k[0][1]) + M_Fabs(E[1] * _Pos.k[1][1]) + M_Fabs(E[2] * _Pos.k[2][1]);
	E2[2] = M_Fabs(E[0] * _Pos.k[0][2]) + M_Fabs(E[1] * _Pos.k[1][2]) + M_Fabs(E[2] * _Pos.k[2][2]);

	C *= _Pos;
	C.Add(E2, _Box.m_Max);
	C.Sub(E2, _Box.m_Min);
}

void CXR_Model_BSP4Glass::Phys_Init(CXR_PhysicsContext* _pPhysContext)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::Phys_Init, XR_BSP4GLASS);
	CXR_Model_BSP4::Phys_Init(_pPhysContext);
}


void CXR_Model_BSP4Glass::CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet *_pcs, const int _iObj, const int _MediumFlags )
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::CollectPCS, XR_BSP4GLASS);

	CBox3Dfp32 _Box; 	//oh this is just soo stupid!
	CBox3Dfp32 TransformedBox;
	CBoxInterleaved TransformedBoxInterleaved;
	CVec3Dfp32 TransformedCenter;

	_pcs->GetBox( _Box.m_Min.k ); // and this is a very ugly way to set the bloody template box

	// Transform PCS into BSP local space
	TransformedCenter = *(CVec3Dfp32*)_pcs->m_fCenter;
	TransformedCenter.MultiplyMatrix(_pPhysContext->m_WMatInv);
	_Box.Transform(_pPhysContext->m_WMatInv, TransformedBox);
	TransformedBoxInterleaved.Init(TransformedBox);

	TObjectPoolAllocator<CBSP4_EnumContext> EnumAlloc(m_spEnumContextPool);
	CBSP4_EnumContext* pEnumContext = EnumAlloc.GetObject();
	pEnumContext->Create(m_lFaces.Len(), ENUM_HQ | ENUM_FACEFLAGS | ENUM_MEDIUMFLAGS, _MediumFlags, XW_FACE_PHYSICAL);
	pEnumContext->SetupPCSEnum(_pcs, _iObj, _IN1N2Flags, TransformedCenter, &TransformedBox, &TransformedBoxInterleaved);

	__Glass_CollectPCS_i(_pPhysContext, pEnumContext, 1);
	pEnumContext->Untag();

#ifdef PCSCHECK_INSOLID
	//AR-TEMP-ADD: Get medium in middle of box
	CVec3Dfp32 Center;
	_Box.m_Min.Lerp(_Box.m_Max, 0.5f, Center);
	int Medium = CXR_Model_BSP4::Phys_GetMedium(_pPhysContext, Center);
	if (Medium & _MediumFlags)
		_pcs->m_bIsInSolid = true;
#endif
}

void CXR_Model_BSP4Glass::__Glass_CollectPCS_r(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, int _iNode) const
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::__Glass_CollectPCS_r, XR_BSP4GLASS);

	if (!_iNode)
	{ // empty tree
		return;
	}
	
	const CBSP4_Node* pN = &m_pNodes[_iNode];

//	const CMat4Dfp32 &T = _pPhysContext->m_WMat; // syntaktiskt socker!

	
	if (pN->IsLeaf())
	{
		__Glass_CollectPCS_IsLeaf(_pPhysContext, _pEnumContext, pN);
	}

	if (pN->IsNode())
	{
		const CPlane3Dfp32& W = m_pPlanes[pN->m_iPlane];
		const fp32 d = W.Distance(_pEnumContext->m_TransformedCenter);
		const fp32 R = _pEnumContext->m_pPCS->m_fRadius;

		if (d > R)
		{ // positive side of partition plane
			__Glass_CollectPCS_r(_pPhysContext, _pEnumContext, pN->m_iNodeFront);
			return;
		}	
		else if (d < -R)
		{ // negative side of partition plane
			__Glass_CollectPCS_r(_pPhysContext, _pEnumContext, pN->m_iNodeBack);
			return;
		}

		// bound-sphere is split by plane - better check with the box..
		fp32 MinDist, MaxDist;
		GetBoxMinMaxDistance(W, *_pEnumContext->m_pTransformedBoxInterleaved, MinDist, MaxDist);

		if (MinDist < MODEL_BSP_EPSILON)
			__Glass_CollectPCS_r(_pPhysContext, _pEnumContext, pN->m_iNodeBack);

		if (MaxDist > -MODEL_BSP_EPSILON)
			__Glass_CollectPCS_r(_pPhysContext, _pEnumContext, pN->m_iNodeFront);
	}
}

void CXR_Model_BSP4Glass::__Glass_CollectPCS_i(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, int _iNode) const
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::__Glass_CollectPCS_i, XR_BSP4GLASS);
	if (!_iNode)
	{ // empty tree
		return;
	}

//	const CMat4Dfp32 &T = _pPhysContext->m_WMat; // syntaktiskt socker!
	const CVec3Dfp32& C = _pEnumContext->m_TransformedCenter;
	const CBoxInterleaved& B = *_pEnumContext->m_pTransformedBoxInterleaved;
	const fp32 Radius = _pEnumContext->m_pPCS->m_fRadius;

	int aWorkingStack[256];
	int iWorkPos = 0;
	aWorkingStack[iWorkPos++]	= _iNode;
StartOf_CollectPCS_i:
	int iWorkNode = aWorkingStack[--iWorkPos];

StartOf_CollectPCS_i_NoAdd:
	const CBSP4_Node* pN = &m_pNodes[iWorkNode];

	if (pN->IsLeaf())
	{
		__Glass_CollectPCS_IsLeaf(_pPhysContext, _pEnumContext, pN);
	}
	else if (pN->IsNode())
	{
		const CPlane3Dfp32& W = m_pPlanes[pN->m_iPlane];
		const fp32 d = W.Distance(C);

		int iBack = pN->m_iNodeBack;
		int iFront = pN->m_iNodeFront;

		if (d > Radius)
		{ // positive side of partition plane
			if( iFront )
			{
				iWorkNode	= iFront;
				goto StartOf_CollectPCS_i_NoAdd;
			}
		}	
		else if (d < -Radius)
		{ // negative side of partition plane
			if( iBack )
			{
				iWorkNode	= iBack;
				goto StartOf_CollectPCS_i_NoAdd;
			}
		}
		else
		{
			// bound-sphere is split by plane - better check with the box..
			fp32 MinDist, MaxDist;
			GetBoxMinMaxDistance(W, B, MinDist, MaxDist);

			if (iBack && (MinDist < MODEL_BSP_EPSILON))
			{
				aWorkingStack[iWorkPos++]	= iBack;
			}

			if (iFront && (MaxDist > -MODEL_BSP_EPSILON))
			{
				iWorkNode	= iFront;
				goto StartOf_CollectPCS_i_NoAdd;
			}
		}
	}

	if( iWorkPos > 0 )
		goto StartOf_CollectPCS_i;
}

void CXR_Model_BSP4Glass::__Glass_CollectPCS_IsLeaf(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, const CBSP4_Node* _pNode) const
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::__Glass_CollectPCS_IsLeaf, XR_BSP4GLASS);
	int nFaces = _pNode->m_nFaces;
	int iiFaces = _pNode->m_iiFaces;

	const uint32* piFaces = &m_piFaces[iiFaces];

	for (int f = 0; f < nFaces; f++)
	{
		int iFace = piFaces[f];
		
		int iFaceByte = iFace >> 3;
		int iFaceMask = 1 << (iFace & 7);
		if (_pEnumContext->m_pFTag[iFaceByte] & iFaceMask) // check if already processed
			continue;
		if(!_pEnumContext->m_pFTag[iFaceByte]) _pEnumContext->m_pFUntag[_pEnumContext->m_nUntagEnum++] = iFaceByte;
			
		_pEnumContext->m_pFTag[iFaceByte] |= iFaceMask;	// tag face as processed

		const CBSP4_Face* const pF = &m_pFaces[iFace];

		if ((_pEnumContext->m_EnumQuality & ENUM_FACEFLAGS) && !(_pEnumContext->m_EnumFaceFlags & pF->m_Flags))
			continue;

		if ((_pEnumContext->m_EnumQuality & ENUM_MEDIUMFLAGS) && !(m_pMediums[pF->m_iBackMedium].m_MediumFlags & _pEnumContext->m_EnumMedium))
			continue;

		int iPlane = pF->m_iPlane;
		const CPlane3Dfp32& WOrig = m_pPlanes[iPlane];
		const CPlane3Dfp32 WOrigM = CPlane3Dfp32(-WOrig.n, m_pMediums[pF->m_iBackMedium].m_Thickness - WOrig.d);

		fp32 MinDist = WOrig.GetBoxMinDistance(_pEnumContext->m_pTransformedBox->m_Min, _pEnumContext->m_pTransformedBox->m_Max);
		fp32 MinDistM = WOrigM.GetBoxMinDistance(_pEnumContext->m_pTransformedBox->m_Min, _pEnumContext->m_pTransformedBox->m_Max);
		if (MinDist > MODEL_BSP_EPSILON && MinDistM > MODEL_BSP_EPSILON)
			continue;

		const int nv = pF->m_nVertices;
		const uint32* piVerts = m_piVertices + pF->m_iiVertices;

		const CVec3Dfp32 VertexOffset = (WOrigM.n * m_pMediums[pF->m_iBackMedium].m_Thickness);

		// check face bound min/max
		if(_pEnumContext->m_EnumQuality & ENUM_HQ)
		{
			for (int k = 0; k < 3; k++)
			{
				fp32 PMin(_FP32_MAX), PMax(-_FP32_MAX);
				for (int v = 0; v < nv; v++)
				{
					const CVec3Dfp32& V = m_pVertices[piVerts[v]];
					const CVec3Dfp32 VO = V + VertexOffset;
					PMin = Min(PMin, V.k[k]);
					PMin = Min(PMin, VO.k[k]);
					PMax = Max(PMax, V.k[k]);
					PMax = Max(PMax, VO.k[k]);
				}

				if (_pEnumContext->m_pTransformedBox->m_Min.k[k] > PMax) goto Continue;
				if (_pEnumContext->m_pTransformedBox->m_Max.k[k] < PMin) goto Continue;
			}
		}

		// Add faces
		{
			const CMat4Dfp32& T = _pPhysContext->m_WMat;
			CPlane3Dfp32 W = WOrig;
			CPlane3Dfp32 WM = WOrigM;
			W.Transform(T);
			WM.Transform(T);

			CVec3Dfp32 tri[3];
			tri[0] = m_pVertices[piVerts[0]] * T;
			tri[1] = m_pVertices[piVerts[1]] * T;

			for (int v = 2; v < nv; v++)
			{ // extract all triangles and add them to PCS
				tri[2] = m_pVertices[piVerts[v]] * T;

				if (_pEnumContext->m_pPCS->ValidFace((const float *)&W, (const float *)tri))
					_pEnumContext->m_pPCS->AddFace( _pEnumContext->m_bIN1N2Flags, _pEnumContext->m_iObj, (const float *)&W, (const float *)tri, m_lspSurfaces[pF->m_iSurface] );

				tri[1] = tri[2];
			}

			tri[0] = (m_pVertices[piVerts[0]] + VertexOffset) * T;
			tri[1] = (m_pVertices[piVerts[1]] + VertexOffset) * T;

			for(int v = 2; v < nv; v++)
			{
				tri[2] = (m_pVertices[piVerts[v]] + VertexOffset) * T;

				if(_pEnumContext->m_pPCS->ValidFace((const float*)&WM, (const float *)tri))
					_pEnumContext->m_pPCS->AddFace(_pEnumContext->m_bIN1N2Flags, _pEnumContext->m_iObj, (const float *)&W, (const float *)tri, m_lspSurfaces[pF->m_iSurface]);

				tri[1] = tri[2];
			}
		}

Continue:
		continue;
	}
}

int CXR_Model_BSP4Glass::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::Phys_GetMedium, XR_BSP4GLASS);
	return CXR_Model_BSP4::Phys_GetMedium(_pPhysContext, _v0);
}

void CXR_Model_BSP4Glass::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::Phys_GetMedium, XR_BSP4GLASS);
	CXR_Model_BSP4::Phys_GetMedium(_pPhysContext, _v0, _RetMedium);
}

int CXR_Model_BSP4Glass::Phys_GetCombinedMediumFlags(CXR_PhysicsContext* _pPhysContext, const CBox3Dfp32& _Box)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::Phys_GetCombinedMediumFlags, XR_BSP4GLASS);
	return CXR_Model_BSP4::Phys_GetCombinedMediumFlags(_pPhysContext, _Box);
}

bool CXR_Model_BSP4Glass::__PhysGlass_Intersect_PolyOBB(const bool& _bTrueFace, const fp32& _Thickness, const CVec3Dfp32* _pVertices, const uint32* _pVertIndices, const int _nVertexCount, const CPlane3Dfp32& _PolyPlane, const CPhysOBB& _BoxStart, const CPhysOBB& _BoxDest, bool _bOrder, CCollisionInfo* _pCollisionInfo)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::__PhysGlass_Intersect_PolyOBB, XR_BSP4GLASS);
	M_TRACEALWAYS("CXR_Model_BSP4Glass::__PhysGlass_Intersect_PolyOBB: Implement me if you want to use me you dumb fuck!!\n");
	return false;
}
