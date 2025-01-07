
#ifndef _INC_XRThinSolid
#define _INC_XRThinSolid

#include "../XR.h"

#define XWSOLID_GEOMETRY_VERSION	0x0100
#define XWTHINSOLID_GEOMETRY_VERSION 0x0101
#define CSOLID_EPSILON 0.00001f

class CXR_ThinSolid //: public CReferenceCount
{
public:

// FIXME: DECLARE_OPERATOR_NEW should also have placement new. The class won't work with TThinArray otherwise.
/*	
	DECLARE_OPERATOR_NEW;
	*/

	TThinArray<CPlane3Dfp32> m_lPlanes;
	uint16 m_iNode;
	uint16 m_iSurface;

	CXR_ThinSolid();
	void operator=(const CXR_ThinSolid&);

	void ReadGeometry(CCFile* _pF);
	void WriteGeometry(CCFile* _pF);

#ifndef PLATFORM_CONSOLE
	void Create(class CSolid& _Solid);
#endif

	fp32 IntersectRay(const CVec3Dfp32& _Origin, const CVec3Dfp32& _Ray);
};

// typedef TPtr<CXR_ThinSolid> spCXR_ThinSolid;

#endif // _INC_XRThinSolid
