#include "PCH.h"
#include "XRVBMHelper.h"

template <int tSize>
int Align(int _Input)
{
	return (_Input + tSize - 1) & ~(tSize - 1);
}

CXR_SkeletonInstance* CXR_VBMHelper::Alloc_SkeletonInst(CXR_VBManager* _pVBM, int _nNodes, int _nRotTracks, int _nMoveTracks)
{
	int RamSize = Align<4>(sizeof(CXR_SkeletonInstance))
				+ Align<4>(sizeof(CMat43fp4_MP) * (_nNodes+1)) * 2
				+ Align<4>(sizeof(fp4) * (_nNodes+1))
				+ Align<4>(sizeof(CQuatfp4) * _nRotTracks)
				+ Align<4>(sizeof(CVec3Dfp4) * _nMoveTracks);
	uint8* pDataPtr = (uint8*)_pVBM->Alloc(RamSize);
	if(pDataPtr)
	{
		CXR_SkeletonInstance* pSkel = (CXR_SkeletonInstance*)pDataPtr; pDataPtr += Align<4>(sizeof(CXR_SkeletonInstance));
		pSkel = new (pSkel) CXR_SkeletonInstance;
		pSkel->m_pBoneLocalPos	= (CMat43fp4_MP*)pDataPtr; pDataPtr += Align<4>(sizeof(CMat43fp4_MP) * (_nNodes+1));
		pSkel->m_pBoneTransform = (CMat43fp4_MP*)pDataPtr; pDataPtr += Align<4>(sizeof(CMat43fp4_MP) * (_nNodes+1));
		pSkel->m_pBoneScale		= (fp4*)pDataPtr; pDataPtr += Align<4>(sizeof(fp4) * (_nNodes+1));

		pSkel->m_pTracksRot		= (CQuatfp4*)pDataPtr; pDataPtr += Align<4>(sizeof(CQuatfp4) * _nRotTracks);
		pSkel->m_pTracksMove	= (CVec3Dfp4*)pDataPtr; pDataPtr += Align<4>(sizeof(CVec3Dfp4) * _nMoveTracks);

		pSkel->m_nBoneLocalPos	= _nNodes;
		pSkel->m_nBoneTransform	= _nNodes;
		pSkel->m_nBoneScale		= _nNodes;
		pSkel->m_nTracksRot		= _nRotTracks;
		pSkel->m_nTracksMove	= _nMoveTracks;
		pSkel->m_Flags			= 1;

		for(int i = 0; i < _nNodes; i++)
		{
			pSkel->m_pBoneLocalPos[i].Unit();
			pSkel->m_pBoneTransform[i].Unit();
			pSkel->m_pBoneScale[i]	= 0;
		}
		return pSkel;
	}
	return NULL;
}
