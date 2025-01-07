#ifndef _INC_WPhys
#define _INC_WPhys

#include "../../MOS.h"
#include "WPhysOBB.h"
#include "WPhysCapsule.h"
#include "WPhysCollision.h"

class CCollisionInfo;


//---------------------------------------------------------------------------
#define DIFF(diff,p,q) \
{ \
    diff[0] = p[0]-q[0]; \
    diff[1] = p[1]-q[1]; \
    diff[2] = p[2]-q[2]; \
}

//---------------------------------------------------------------------------
#define DOT(p,q) (p[0]*q[0]+p[1]*q[1]+p[2]*q[2])

//---------------------------------------------------------------------------
#define CROSS(cross,p,q) \
{ \
    cross[0] = p[1]*q[2]-p[2]*q[1]; \
    cross[1] = p[2]*q[0]-p[0]*q[2]; \
    cross[2] = p[0]*q[1]-p[1]*q[0]; \
}

//---------------------------------------------------------------------------
#define COMBO(combo,p,t,q) \
{ \
    combo[0] = p[0]+t*q[0]; \
    combo[1] = p[1]+t*q[1]; \
    combo[2] = p[2]+t*q[2]; \
}

//---------------------------------------------------------------------------
unsigned int TestIntersectionS (const CVec3Dfp32 tri0[3], const CVec3Dfp32 tri1[3]);

unsigned int TestIntersectionV (float dt, const CVec3Dfp32 tri0[3],
    const CVec3Dfp32& V0, const CVec3Dfp32 tri1[3], const CVec3Dfp32& V1);

unsigned int FindIntersectionV (float dt, const CVec3Dfp32 tri0[3],
    const CVec3Dfp32& V0, const CVec3Dfp32 tri1[3], const CVec3Dfp32& V1,
    float& T, CVec3Dfp32& P);

bool Phys_Intersect_OBB(const CPhysOBB& _Box, const CPhysOBB& _BoxStart, const CPhysOBB& _BoxDest, CCollisionInfo* _pCollisionInfo = NULL);

bool    Phys_Intersect_TriOBB(const CVec3Dfp32 tri[3], const CPhysOBB& _BoxStart, const CPhysOBB& _BoxDest, bool _bOrder, CCollisionInfo* _pCollisionInfo = NULL);
bool    Phys_Intersect_PolyOBB(const CVec3Dfp32* _pVertices, const uint32* _pVertIndices, const int nVertexCount, const CPlane3Dfp32& _PolyPlane, const CPhysOBB& _BoxStart, const CPhysOBB& _BoxDest, bool _bOrder, CCollisionInfo* _pCollisionInfo = NULL);

bool Phys_Intersect_TriSphere(const CVec3Dfp32 tri[3], const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, float _Radius, bool _bOrder, CCollisionInfo* _pCollisionInfo = NULL);

bool   Phys_Intersect_TriLine(const CVec3Dfp32 tri[3], const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, bool _bOrder, CCollisionInfo* _pCollisionInfo = NULL);

#endif // _INC_WPhys


