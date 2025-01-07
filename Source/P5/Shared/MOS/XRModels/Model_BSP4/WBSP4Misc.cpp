#include "PCH.h"

#include "WBSP4Model.h"
#include "WBSP4Def.h"
#include "MFloat.h"

#define FACECUT3_EPSILON 0.01f

extern int aShiftMulTab[];

// -------------------------------------------------------------------

void CXR_Model_BSP4::EnumFaces_Sphere_r(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, int _iNode) const
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_Sphere_r, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP4::EnumFaces_Sphere_r); //AR-SCOPE

	if (!_iNode) return;
	CBSP4_Node* pN = &m_pNodes[_iNode];

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;
		uint8* pFTag = _pEnumContext->m_pFTag;

		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_piFaces[iiFaces + f];
			uint16 iFaceIdx = iFace >> 3;
			uint8 iFaceMask = aShiftMulTab[iFace & 7];
			if (pFTag[iFaceIdx] & iFaceMask) continue;
			if(!pFTag[iFaceIdx]) _pEnumContext->m_pFUntag[_pEnumContext->m_nUntagEnum++]	= iFaceIdx;
			pFTag[iFaceIdx] |= iFaceMask;

			CBSP4_Face* pF = &m_pFaces[iFace];

			// Check face flag?
			if (_pEnumContext->m_EnumQuality & ENUM_FACEFLAGS && !(_pEnumContext->m_EnumFaceFlags & pF->m_Flags)) continue;

			// Check medium
			if (_pEnumContext->m_EnumQuality & ENUM_MEDIUMFLAGS && !(m_pMediums[pF->m_iBackMedium].m_MediumFlags & _pEnumContext->m_EnumMedium)) continue;

			int iPlane = pF->m_iPlane;
			fp32 Dist = m_pPlanes[iPlane].Distance(_pEnumContext->m_EnumSphere);
			if (Abs(Dist) < _pEnumContext->m_EnumSphereR)
			{
				if (_pEnumContext->m_nEnum < _pEnumContext->m_MaxEnum)
				{
					_pEnumContext->m_piEnum[_pEnumContext->m_nEnum] = iFace;
					_pEnumContext->m_nEnum++;
				}
				else
				{
					_pEnumContext->m_bEnumError = true;
					return;
				}
			}
		}
	}
	else if (pN->IsNode())
	{
		fp32 Dist = m_pPlanes[pN->m_iPlane].Distance(_pEnumContext->m_EnumSphere);
		if (Dist < _pEnumContext->m_EnumSphereR)
			EnumFaces_Sphere_r(_pPhysContext, _pEnumContext, pN->m_iNodeBack);
		if (Dist > -_pEnumContext->m_EnumSphereR)
			EnumFaces_Sphere_r(_pPhysContext, _pEnumContext, pN->m_iNodeFront);
	}
}

void CXR_Model_BSP4::EnumFaces_Box_r(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, int _iNode) const
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_Box_r, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	CBSP4_Node* pN = &m_pNodes[_iNode];

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;
		uint8* pFTag = _pEnumContext->m_pFTag;

		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_piFaces[iiFaces + f];
			uint16 iFaceIdx = iFace >> 3;
			uint8 iFaceMask = aShiftMulTab[iFace & 7];
			if (pFTag[iFaceIdx] & iFaceMask) continue;
			if(!pFTag[iFaceIdx]) _pEnumContext->m_pFUntag[_pEnumContext->m_nUntagEnum++]	= iFaceIdx;
			pFTag[iFaceIdx] |= iFaceMask;

			if (_pPhysContext->m_pWC) 
				__Phys_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xff100020);

			CBSP4_Face* pF = &m_pFaces[iFace];
			int iPlane = pF->m_iPlane;
			fp32 MinDist = m_pPlanes[iPlane].GetBoxMinDistance(_pEnumContext->m_EnumBox.m_Min, _pEnumContext->m_EnumBox.m_Max);
			if (MinDist > MODEL_BSP_EPSILON) continue;

			// Check face flag?
			if (_pEnumContext->m_EnumQuality & ENUM_FACEFLAGS && !(_pEnumContext->m_EnumFaceFlags & pF->m_Flags)) continue;

			// Check medium
			if (_pEnumContext->m_EnumQuality & ENUM_MEDIUMFLAGS && !(m_pMediums[pF->m_iBackMedium].m_MediumFlags & _pEnumContext->m_EnumMedium)) continue;

// Box can't be behind face.
//			fp32 MaxDist = m_pPlanes[iPlane].GetBoxMaxDistance(ms_EnumBox.m_Min, ms_EnumBox.m_Max);
//			if (MaxDist < -MODEL_BSP_EPSILON) continue;

			// Check face bounding box
			if (_pEnumContext->m_EnumQuality & ENUM_HQ)
			{
				int nv = pF->m_nVertices;
				int iiv = pF->m_iiVertices;

				for(int k = 0; k < 3; k++)
				{
					fp32 PMin = _FP32_MAX;
					fp32 PMax = -_FP32_MAX;
					for(int v = 0; v < nv; v++)
					{
						const CVec3Dfp32& V = m_pVertices[m_piVertices[iiv + v]];
						if (V.k[k] < PMin) PMin = V.k[k];
						if (V.k[k] > PMax) PMax = V.k[k];
					}

					if (_pEnumContext->m_EnumBox.m_Min[k] > PMax)
						goto Continue;
					if (_pEnumContext->m_EnumBox.m_Max[k] < PMin)
						goto Continue;
				}

#ifdef NEVER
				// Get bound-box for polygon.
				CBox3Dfp32 PBox;
				PBox.m_Min = _FP32_MAX;
				PBox.m_Max = -_FP32_MAX;
				for(int v = 0; v < nv; v++)
				{
					const CVec3Dfp32& V = m_pVertices[m_piVertices[iiv + v]];
					for(int k = 0; k < 3; k++)
					{
						if (V.k[k] < PBox.m_Min.k[k]) PBox.m_Min.k[k] = V.k[k];
						if (V.k[k] > PBox.m_Max.k[k]) PBox.m_Max.k[k] = V.k[k];
					}
				}
				if (!PBox.IsInside(_pEnumContext->m_EnumBox)) continue;
#endif
			}

			if (_pEnumContext->m_nEnum < _pEnumContext->m_MaxEnum)
			{
				_pEnumContext->m_piEnum[_pEnumContext->m_nEnum] = iFace;
				_pEnumContext->m_nEnum++;
//				ConOut(CStrF("    Added face %d  (%d)", iFace, ms_nEnum-1));
			}
			else
			{
				_pEnumContext->m_bEnumError = true;
				return;
			}
Continue: ;
		}
	}
	else if (pN->IsNode())
	{
		fp32 MinDist = m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_pEnumContext->m_EnumBox.m_Min, _pEnumContext->m_EnumBox.m_Max);
		fp32 MaxDist = m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_pEnumContext->m_EnumBox.m_Min, _pEnumContext->m_EnumBox.m_Max);

		if (MinDist < MODEL_BSP_EPSILON)
			EnumFaces_Box_r(_pPhysContext, _pEnumContext, pN->m_iNodeBack);
		if (MaxDist > -MODEL_BSP_EPSILON)
			EnumFaces_Box_r(_pPhysContext, _pEnumContext, pN->m_iNodeFront);
	}
}

void CXR_Model_BSP4::EnumFaces_All_r(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, int _iNode) const
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_All_r, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP4::EnumFaces_All_r); //AR-SCOPE

	if (!_iNode) return;
	CBSP4_Node* pN = &m_pNodes[_iNode];

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;
		uint8* pFTag = _pEnumContext->m_pFTag;

		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_piFaces[iiFaces + f];
			uint16 iFaceIdx = iFace >> 3;
			uint8 iFaceMask = aShiftMulTab[iFace & 7];
			if (pFTag[iFaceIdx] & iFaceMask) continue;
			if(!pFTag[iFaceIdx]) _pEnumContext->m_pFUntag[_pEnumContext->m_nUntagEnum++]	= iFaceIdx;
			pFTag[iFaceIdx] |= iFaceMask;

			if (_pEnumContext->m_nEnum < _pEnumContext->m_MaxEnum)
			{
				_pEnumContext->m_piEnum[_pEnumContext->m_nEnum] = iFace;
				_pEnumContext->m_nEnum++;
			}
			else
			{
				_pEnumContext->m_bEnumError = true;
				return;
			}
		}
	}
	else if (pN->IsNode())
	{
		EnumFaces_All_r(_pPhysContext, _pEnumContext, pN->m_iNodeBack);
		EnumFaces_All_r(_pPhysContext, _pEnumContext, pN->m_iNodeFront);
	}
}

int CXR_Model_BSP4::EnumFaces_Sphere(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, int _iNode) const
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_Sphere, 0);
	MSCOPESHORT(CXR_Model_BSP4::EnumFaces_Sphere);

	EnumFaces_Sphere_r(_pPhysContext, _pEnumContext, _iNode);
	return _pEnumContext->m_nEnum;
}

int CXR_Model_BSP4::EnumFaces_Box(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, int _iNode) const
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_Box, 0);
	MSCOPESHORT(CXR_Model_BSP4::EnumFaces_Box);

	EnumFaces_Box_r(_pPhysContext, _pEnumContext, _iNode);
	return _pEnumContext->m_nEnum;
}

int CXR_Model_BSP4::EnumFaces_All(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, int _iNode) const
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_All, 0);
	MSCOPESHORT(CXR_Model_BSP4::EnumFaces_All);

	EnumFaces_All_r(_pPhysContext, _pEnumContext, _iNode);
	return _pEnumContext->m_nEnum;
}


// -------------------------------------------------------------------
aint CXR_Model_BSP4::GetParam(int _Param)
{
	MAUTOSTRIP(CXR_Model_BSP_GetParam, 0);
	switch(_Param)
	{
	case MODEL_BSP_PARAM_GLOBALFLAGS :
		return GetGlobalEnable();

#ifndef PLATFORM_CONSOLE
	case MODEL_PARAM_NUMTRIANGLES :
		return m_lFaces.Len();		// Not quite right, but...

	case MODEL_PARAM_NUMVERTICES :
		return m_lVertices.Len();	// Not quite right either, but...
#endif

	case CXR_MODEL_PARAM_TIMEMODE:
		return CXR_MODEL_TIMEMODE_CONTROLLED;

	case CXR_MODEL_PARAM_ISSHADOWCASTER :
		{
			return 0;
		}

	default :
		return CXR_Model::GetParam(_Param);
	}
}

void CXR_Model_BSP4::SetParam(int _Param, aint _Value)
{
	MAUTOSTRIP(CXR_Model_BSP_SetParam, MAUTOSTRIP_VOID);
	switch(_Param)
	{
	case MODEL_BSP_PARAM_GLOBALFLAGS :
		GetGlobalEnable() = _Value;

	default :
		CXR_Model::SetParam(_Param, _Value);
	}
}

int CXR_Model_BSP4::GetParamfv(int _Param, fp32* _pRetValues)
{
	MAUTOSTRIP(CXR_Model_BSP_GetParamfv, 0);
	return CXR_Model::GetParamfv(_Param, _pRetValues);
}

void CXR_Model_BSP4::SetParamfv(int _Param, const fp32* _pValues)
{
	MAUTOSTRIP(CXR_Model_BSP_SetParamfv, MAUTOSTRIP_VOID);
	switch(_Param)
	{
	case CXR_MODEL_PARAM_TEXTUREPARAM :
		{
			for(int i = 0; i < m_lspSurfaces.Len(); i++)
				m_lspSurfaces[i]->SetTextureParam((int)_pValues[0], (int)_pValues[1]);
		}
		break;

	default :
		CXR_PhysicsModel::SetParamfv(_Param, _pValues);
	}
}

fp32 CXR_Model_BSP4::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_BSP_GetBound_Sphere, 0.0f);
	return m_BoundRadius;
}

void CXR_Model_BSP4::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_BSP_GetBound_Box, MAUTOSTRIP_VOID);
	_Box = m_BoundBox;
}

int CXR_Model_BSP4::NumFacesInTree(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_NumFacesInTree, 0);
	if (!_iNode) return 0;
	if (m_pNodes[_iNode].IsLeaf()) return m_pNodes[_iNode].m_nFaces;

	return NumFacesInTree(m_pNodes[_iNode].m_iNodeFront) + NumFacesInTree(m_pNodes[_iNode].m_iNodeBack);
}

// -------------------------------------------------------------------
int CXR_Model_BSP4::GetLeaf(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CXR_Model_BSP_GetLeaf, 0);
	int iNode = 1;
	while(m_pNodes[iNode].IsNode())
	{
		int iPlane = m_pNodes[iNode].m_iPlane;
		int VSide = m_pPlanes[iPlane].GetPlaneSide(_v);
		iNode = (VSide >= 0) ? m_pNodes[iNode].m_iNodeFront : m_pNodes[iNode].m_iNodeBack;
	}
	return iNode;
}


CStr CXR_Model_BSP4::GetInfo()
{
	MAUTOSTRIP(CXR_Model_BSP_GetInfo, CStr());
	return "No profiling available for BSP4.";
}

void CXR_Model_BSP4::InitializeListPtrs()
{
	MAUTOSTRIP(CXR_Model_BSP_InitializeListPtrs, MAUTOSTRIP_VOID);
	m_pPlanes = (m_lPlanes.Len()) ? &m_lPlanes[0] : NULL;
	m_pVertices = (m_lVertices.Len()) ? &m_lVertices[0] : NULL;
	m_piVertices = (m_liVertices.Len()) ? &m_liVertices[0] : NULL;
	m_pFaces = (m_lFaces.Len()) ? &m_lFaces[0] : NULL;
	m_piFaces = (m_liFaces.Len()) ? &m_liFaces[0] : NULL;
	m_pNodes = (m_lNodes.Len()) ? &m_lNodes[0] : NULL;
	m_pMediums	= (m_lMediums.Len())? &m_lMediums[0] : NULL;
}

