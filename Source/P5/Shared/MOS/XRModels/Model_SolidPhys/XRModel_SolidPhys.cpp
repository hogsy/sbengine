
#include "PCH.h"
#include "XRModel_SolidPhys.h"


void CXR_Model_SolidPhys::Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _RetPos, fp32& _Radius)
{
}
void CXR_Model_SolidPhys::Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _RetBox)
{
}

void CXR_Model_SolidPhys::Phys_Init(CXR_PhysicsContext* _pPhysContext)
{
}

int CXR_Model_SolidPhys::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0)
{
	return 0;
}

void CXR_Model_SolidPhys::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium)
{
}

bool CXR_Model_SolidPhys::Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	return false;
}
bool CXR_Model_SolidPhys::Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	return false;
}
bool CXR_Model_SolidPhys::Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	return false;
}

