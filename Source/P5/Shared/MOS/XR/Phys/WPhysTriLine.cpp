
#include "PCH.h"
#include "WPhys.h"
#include "../XRClass.h"
#include "MFloat.h"

bool Phys_Intersect_TriLine(const CVec3Dfp32 tri[3], const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, bool _bOrder, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(Phys_Intersect_TriLine, false);
	CPlane3Dfp32 PolyPlane;  
	float       pDist[2];  

	// The normalization is only needed so that we can fill in _CollisionInfo correctly
	// FIXME: Move normalization to when collision info is created and we actually know there's an intersection  /mh

	PolyPlane.n = (tri[0] - tri[1]) / (tri[2] - tri[1]);  
	PolyPlane.n.Normalize();
	PolyPlane.d = -tri[0] * PolyPlane.n;

	pDist[0] = PolyPlane.n * _v0 + PolyPlane.d;
	pDist[1] = PolyPlane.n * _v1 + PolyPlane.d;

	// If both distances have the same sign we cant have a collision
	if(pDist[0] * pDist[1] > 0)
		return false;

	bool bInvert;

	if(pDist[0] > 0)
		bInvert = false;
	else
		bInvert = true;

	CPlane3Dfp32 EdgePlanes[3];

	int i,j;

	for(i = 0; i < 3; i++)
	{
		j = (i + 1) % 3;

		EdgePlanes[i].n = (tri[i] - tri[j]) / PolyPlane.n;            
		EdgePlanes[i].d = -tri[i] * EdgePlanes[i].n;       
	}  

	float t = -pDist[0] / (pDist[1] - pDist[0]);

	CVec3Dfp32 Intersection = _v0 + (_v1 - _v0) * t;

	for(i = 0; i < 3; i++)
		if((Intersection * EdgePlanes[i].n + EdgePlanes[i].d) > 0.0f)
			break;

	if(i != 3)
		return false;

	if(!_pCollisionInfo)
		return true;

	_pCollisionInfo->m_bIsValid    = true;
	_pCollisionInfo->m_bIsCollision = true;
	if(!(_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_TIME))
		return true;

	if(bInvert)  
		PolyPlane.Inverse(); 

	_pCollisionInfo->m_Time        = t;
	_pCollisionInfo->m_LocalPos    = Intersection;    
	_pCollisionInfo->m_Velocity    = CVec3Dfp32(0.0f, 0.0f, 0.0f);
	_pCollisionInfo->m_RotVelocity = 0.0f;
	_pCollisionInfo->m_Plane       = PolyPlane;    


	return true;
}

