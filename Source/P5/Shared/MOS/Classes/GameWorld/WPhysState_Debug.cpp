#include "PCH.h"

#include "WPhysState.h"


void CWorld_PhysState::Debug_RenderWire(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWorld_PhysState_Debug_RenderWire, MAUTOSTRIP_VOID);
	CWireContainer* pWC = Debug_GetWireContainer();
	if (pWC) pWC->RenderWire(_p0, _p1, _Color, _Duration, _bFade);
}

void CWorld_PhysState::Debug_RenderVertex(const CVec3Dfp32& _p, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWorld_PhysState_Debug_RenderVertex, MAUTOSTRIP_VOID);
	CWireContainer* pWC = Debug_GetWireContainer();
	if (pWC) pWC->RenderVertex(_p, _Color, _Duration, _bFade);
}

void CWorld_PhysState::Debug_RenderVector(const CVec3Dfp32& _p, const CVec3Dfp32& _v, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWorld_PhysState_Debug_RenderVector, MAUTOSTRIP_VOID);
	CWireContainer* pWC = Debug_GetWireContainer();
	if (pWC) pWC->RenderVector(_p, _v, _Color, _Duration, _bFade);
}

void CWorld_PhysState::Debug_RenderMatrix(const CMat4Dfp32& _Mat, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWorld_PhysState_Debug_RenderMatrix, MAUTOSTRIP_VOID);
	CWireContainer* pWC = Debug_GetWireContainer();
	if (pWC) pWC->RenderMatrix(_Mat, _Color, _Duration, _bFade);
}

void CWorld_PhysState::Debug_RenderMatrix(const CMat4Dfp32& _Mat, fp32 _Duration, bool _bFade, CPixel32 _ColorX, CPixel32 _ColorY, CPixel32 _ColorZ)
{
	MAUTOSTRIP(CWorld_PhysState_Debug_RenderMatrix_2, MAUTOSTRIP_VOID);
	CWireContainer* pWC = Debug_GetWireContainer();
	if (pWC) pWC->RenderMatrix(_Mat, _Duration, _bFade, _ColorX, _ColorY, _ColorZ);
}

void CWorld_PhysState::Debug_RenderQuaternion(const CVec3Dfp32& _p, const CQuatfp32& _Q, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWorld_PhysState_Debug_RenderQuaternion, MAUTOSTRIP_VOID);
	CWireContainer* pWC = Debug_GetWireContainer();
	if (pWC) pWC->RenderQuaternion(_p, _Q, _Color, _Duration, _bFade);
}

void CWorld_PhysState::Debug_RenderAxisRot(const CVec3Dfp32& _p, const CAxisRotfp32& _Q, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWorld_PhysState_Debug_RenderAxisRot, MAUTOSTRIP_VOID);
	CWireContainer* pWC = Debug_GetWireContainer();
	if (pWC) pWC->RenderAxisRot(_p, _Q, _Color, _Duration, _bFade);
}


void CWorld_PhysState::Debug_RenderAABB(const CVec3Dfp32& _Min, const CVec3Dfp32& _Max, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWorld_PhysState_Debug_RenderAABB, MAUTOSTRIP_VOID);
	CWireContainer* pWC = Debug_GetWireContainer();
	if (pWC) pWC->RenderAABB(_Min, _Max, _Color, _Duration, _bFade);
}

void CWorld_PhysState::Debug_RenderAABB(const CBox3Dfp32& _Box, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWorld_PhysState_Debug_RenderAABB_2, MAUTOSTRIP_VOID);
	CWireContainer* pWC = Debug_GetWireContainer();
	if (pWC) pWC->RenderAABB(_Box, _Color, _Duration, _bFade);
}


void CWorld_PhysState::Debug_RenderOBB(const CMat4Dfp32& _Pos, const CVec3Dfp32& _Extents, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWorld_PhysState_Debug_RenderOBB, MAUTOSTRIP_VOID);
	CWireContainer* pWC = Debug_GetWireContainer();
	if (pWC) pWC->RenderOBB(_Pos, _Extents, _Color, _Duration, _bFade);
}

void CWorld_PhysState::Debug_RenderOBB(const CMat4Dfp32& _Pos, const CBox3Dfp32& _Box, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	CWireContainer* pWC = Debug_GetWireContainer();
	if (pWC) 
	{
		CVec3Dfp32 c, e = (_Box.m_Max - _Box.m_Min) * 0.5f;
		_Box.GetCenter(c);
		CMat4Dfp32 TmpPos = _Pos;
		TmpPos.GetRow(3) = c * TmpPos;
		pWC->RenderOBB(TmpPos, e, _Color, _Duration, _bFade);
	}
}


void CWorld_PhysState::Debug_RenderSphere(const CMat4Dfp32& _Pos, fp32 _Radius, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWorld_PhysState_Debug_RenderSphere, MAUTOSTRIP_VOID);
	CWireContainer* pWC = Debug_GetWireContainer();
	if (pWC) pWC->RenderSphere(_Pos, _Radius, _Color, _Duration, _bFade);
}

void CWorld_PhysState::Debug_RenderSphere(const CVec3Dfp32& _Pos, fp32 _Radius, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWorld_PhysState_Debug_RenderSphere, MAUTOSTRIP_VOID);
	CWireContainer* pWC = Debug_GetWireContainer();
	if (pWC) 
	{
		CMat4Dfp32 Tmp;
		Tmp.CreateTranslation(_Pos);
		pWC->RenderSphere(Tmp, _Radius, _Color, _Duration, _bFade);
	}
}


void CWorld_PhysState::Debug_RenderSkeleton(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInst, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	MAUTOSTRIP(CWorld_PhysState_Debug_RenderSkeleton, MAUTOSTRIP_VOID);
	CWireContainer* pWC = Debug_GetWireContainer();
	if (!pWC) return;

	if (!_pSkel || !_pSkelInst) return;

	int nBones = _pSkelInst->GetNumBones();
	const CMat4Dfp32 *pBoneTransform = _pSkelInst->GetBoneTransforms();

	for(int iNode = 0; iNode < nBones; iNode++)
	{
		const CXR_SkeletonNode &Node = _pSkel->m_lNodes[iNode];

		if( Node.m_iNodeParent < 0 )
			continue;
		if ((Node.m_iRotationSlot == -1) && (Node.m_iMovementSlot == -1) && (Node.m_iNodeParent == 0))
			continue;

		const CXR_SkeletonNode &NodeParent = _pSkel->m_lNodes[Node.m_iNodeParent];

		CVec3Dfp32 Pos = Node.m_LocalCenter; 
		CVec3Dfp32 PosParent = NodeParent.m_LocalCenter;

		Pos *= pBoneTransform[iNode];
		PosParent *= pBoneTransform[Node.m_iNodeParent];

//		Debug_RenderVector(PosParent, Pos - PosParent, _Color, _Duration, _bFade);
		Debug_RenderWire(PosParent, Pos, _Color, _Duration, _bFade);
	}
}

void CWorld_PhysState::Debug_RenderText(const CVec3Dfp32& _Pos, const char* _pText, CPixel32 _Color, fp32 _Duration, bool _bFade)
{
	CDebugRenderContainer* pWC = Debug_GetWireContainer();
	if (pWC && _pText)
		pWC->RenderText(_Pos, _pText, _Color, _Duration, _bFade);
}

