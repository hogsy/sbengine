
#include "PCH.h"

/*************************************************************
 *  File name   : XWTraceObj.cpp
 *
 *  Author      : Fredrik Larsson (fredrikl@o3games.com)
 *
 *  Date        : 2001-02-14
 *
 *  Description : 
 *
 *************************************************************/

#include "../../Classes/Tracer/XWTraceObj.h"

CXWC_TracerObject_TriMesh::CXWC_TracerObject_TriMesh(spCXR_Model_TriangleMesh _spModel, const CMat4Dfp32& _TracePos)
{
	m_spModel = _spModel;
	m_iNodeBound = 1;
	m_TracePos	= _TracePos;

	m_BoundRadius = m_spModel->GetBoundRadius();
	m_Bound =  m_spModel->GetBoundBox();
}



void CXWC_TracerObject_TriMesh::InitTraceBound(CXWC_Tracer* _pTracer, const CBox3Dfp32& _Box)
{
}


void CXWC_TracerObject_TriMesh::ReleaseTraceBound()
{
	m_iNodeBound = 1;
}



int CXWC_TracerObject_TriMesh::TraceLine_Bounded(const CVec3Dfp64& _p0, const CVec3Dfp64& _p1, CVec3Dfp32& _Color)
{
	CVec3Dfp32 HitPos;
	if (!m_Bound.IntersectLine(_p0.Getfp32(), _p1.Getfp32(), HitPos)) return true;

	CCollisionInfo CInfo;
	CInfo.m_CollisionType = CXR_COLLISIONTYPE_RAYTRACING;
	CVec3Dfp64 g_PhysTraceLineV0 = _p0;
	CVec3Dfp64 g_PhysTraceLineV1 = _p1;
	CXR_PhysicsContext PhysContext(m_TracePos);
	if(m_spModel->Phys_IntersectLine(&PhysContext, _p0.Getfp32(), _p1.Getfp32(), XW_MEDIUM_SOLID, &CInfo))
	{
		return false;
	}
	else
		return true;
//	m_TraceLineMedium |= m_spModel->m_LightTraceMedium;
//	return Res;
}

int CXWC_TracerObject_TriMesh::TraceLine_Bounded(const CVec3Dfp64& _p0, const CVec3Dfp64& _p1, int _MediumFlags, CCollisionInfo* _pCInfo)
{
	CVec3Dfp32 HitPos;
	if (!m_Bound.IntersectLine(_p0.Getfp32(), _p1.Getfp32(), HitPos)) return true;

	CCollisionInfo CInfo;
	if (!_pCInfo) _pCInfo = &CInfo;
	_pCInfo->m_CollisionType = CXR_COLLISIONTYPE_RAYTRACING;

	CVec3Dfp64 g_PhysTraceLineV0 = _p0;
	CVec3Dfp64 g_PhysTraceLineV1 = _p1;
	CXR_PhysicsContext PhysContext(m_TracePos);
	if(m_spModel->Phys_IntersectLine(&PhysContext, _p0.Getfp32(), _p1.Getfp32(), _MediumFlags, _pCInfo))
	{
		return false;
	}
	else
		return true;
//	m_TraceLineMedium |= m_spModel->m_LightTraceMedium;
//	return Res;
}