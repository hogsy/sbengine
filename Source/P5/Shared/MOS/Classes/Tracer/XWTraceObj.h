
#ifndef __INC_XWTraceObj
#define __INC_XWTraceObj

#include "XWTrace.h"
#include "../../XWC/XWBSP.h"
#include "../../XRModels/Model_TriMesh/WTriMesh.h"

class CXWC_TracerObject_BSPModel : public CXWC_TracerObject
{
protected:
	spCXR_BSP_Model m_spModel;
	int m_iNodeBound;

	TArray<uint16> m_liSplineBrushes;
	int m_nSplineBrushes;

	mutable spCSolidCache m_spSolidCache;

public:
	CXWC_TracerObject_BSPModel(spCXR_BSP_Model _spModel);
//	virtual bool InitTraceBound_Polyhedron(const CVec3Dfp64* _pV, int _nVertices, const CPlane3Dfp64* _pP, int _nPlanes);
	virtual void InitTraceBound(CXWC_Tracer* _pTracer, const CBox3Dfp32& _Box);
	virtual void ReleaseTraceBound();
	virtual int TraceLine_Bounded(const CVec3Dfp64& _p0, const CVec3Dfp64& _p1, CVec3Dfp32& _Color);
	virtual int TraceLine_Bounded(const CVec3Dfp64& _p0, const CVec3Dfp64& _p1, int _MediumFlags, CCollisionInfo* _pCInfo);

	virtual int GetMedium(const CBox3Dfp64& _Box, CXR_MediumDesc& _Medium) const;
};


class CXWC_TracerObject_NavBSPModel : public CXWC_TracerObject_BSPModel
{
public:
	CXWC_TracerObject_NavBSPModel(spCXR_BSP_Model _spModel);

	virtual int GetMedium(const CBox3Dfp64& _Box, CXR_MediumDesc& _Medium) const;
};


class CXWC_TracerObject_TriMesh : public CXWC_TracerObject
{
	spCXR_Model_TriangleMesh m_spModel;
	int m_iNodeBound;
	CMat4Dfp32	m_TracePos;
	
//	TArray<uint16> m_liSplineBrushes;
//	int m_nSplineBrushes;
	
public:
	CXWC_TracerObject_TriMesh(spCXR_Model_TriangleMesh _spModel, const CMat4Dfp32& _TracePos);
	virtual void InitTraceBound(CXWC_Tracer* _pTracer, const CBox3Dfp32& _Box);
	virtual void ReleaseTraceBound();
	virtual int TraceLine_Bounded(const CVec3Dfp64& _p0, const CVec3Dfp64& _p1, CVec3Dfp32& _Color);
	virtual int TraceLine_Bounded(const CVec3Dfp64& _p0, const CVec3Dfp64& _p1, int _MediumFlags, CCollisionInfo* _pCInfo);
};
#endif // __INC_XWTraceObj