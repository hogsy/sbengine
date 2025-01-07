#ifndef _INC_WPhysOBB
#define _INC_WPhysOBB

#include "../../MOS.h"

class CPhysOBB
{
public:
    CVec3Dfp4 m_C;     // center
    CVec3Dfp4 m_A[3];  // right-handed orthonormal axes
    CVec3Dfp4 m_E;     // extents along axes (must be positive)

    // The eight vertices are
    //     C + s0*E[0]*A[0] + s1*E[1]*A[1] + s2*E[2]*A[2]
    // where |s0| = |s1| = |s2| = 1 (eight choices of
    // signs).



	CPhysOBB(){};

/*
	const CPhysOBB& operator = (const CPhysOBB &ToCopy)
	{
		memcpy(this, &ToCopy, sizeof(*this));

		return *this;
	}

	CPhysOBB(const CPhysOBB &ToCopy)
	{
		memcpy(this, &ToCopy, sizeof(*this));
	}
*/

	CPhysOBB(const CPhysOBB &ToCopy)
	{
		*this = ToCopy;
	}


	void SetDimensions(const CVec3Dfp4& _Extents)
	{
		m_E = _Extents;
	}

	void SetPosition(const CMat43fp4& _Pos)
	{
		m_C = CVec3Dfp4::GetMatrixRow(_Pos, 3);
		m_A[0] = CVec3Dfp4::GetMatrixRow(_Pos, 0);
		m_A[1] = CVec3Dfp4::GetMatrixRow(_Pos, 1);
		m_A[2] = CVec3Dfp4::GetMatrixRow(_Pos, 2);
	}

	void operator*= (const CMat43fp4& _Mat)
	{
		m_A[0].MultiplyMatrix3x3(_Mat);
		m_A[1].MultiplyMatrix3x3(_Mat);
		m_A[2].MultiplyMatrix3x3(_Mat);
		m_C *= _Mat;
	}

	float GetRadiusSquared() const;

	void GetVertices(CVec3Dfp4 vertex[8]) const;

	void Transform(const CMat43fp4& _Mat, CPhysOBB& _Dest) const;
	void GetBoundBox(CBox3Dfp4& _Box) const;

	void TransformToBoxSpace(const CVec3Dfp4& _v, CVec3Dfp4& _Dst) const;
	void TransformFromBoxSpace(const CVec3Dfp4& _v, CVec3Dfp4& _Dst) const;
	fp4 GetMinSqrDistance(const CVec3Dfp4& _v) const;
	fp4 GetMaxSqrDistance(const CVec3Dfp4& _v) const;

	fp4 MinDistance(const CPlane3Dfp4& _Plane) const;
	fp4 MaxDistance(const CPlane3Dfp4& _Plane) const;
#ifndef	CPU_SOFTWARE_FP8
	fp8 MinDistance(const CPlane3Dfp8& _Plane) const;
	fp8 MaxDistance(const CPlane3Dfp8& _Plane) const;
#endif
	bool LocalPointInBox(const CVec3Dfp4 _p0) const;
	bool IntersectLine(const CVec3Dfp4 _v0, const CVec3Dfp4 _v1, CVec3Dfp4& _HitPos) const;
	void GetPlaneFromLocalPoint(const CVec3Dfp4& _p0, CPlane3Dfp4& _Plane) const;
};

#define OBB_DOTPROD( a, b ) ( a[0] * b[0] + a[1] * b[1] + a[2] * b[2] )

/*
M_INLINE fp4 CPhysOBB::MinDistance(const CPlane3Dfp4& _Plane) const
{
	return OBB_DOTPROD(_Plane.n, m_C ) + _Plane.d
		  - M_Fabs(OBB_DOTPROD(_Plane.n, m_A[0])) * m_E[0]
		  - M_Fabs(OBB_DOTPROD(_Plane.n, m_A[1])) * m_E[1]
		  - M_Fabs(OBB_DOTPROD(_Plane.n, m_A[2])) * m_E[2];
}

M_INLINE fp4 CPhysOBB::MaxDistance(const CPlane3Dfp4& _Plane) const
{
	return OBB_DOTPROD(_Plane.n, m_C ) + _Plane.d
		  + M_Fabs(OBB_DOTPROD(_Plane.n, m_A[0])) * m_E[0]
		  + M_Fabs(OBB_DOTPROD(_Plane.n, m_A[1])) * m_E[1]
		  + M_Fabs(OBB_DOTPROD(_Plane.n, m_A[2])) * m_E[2];
}

#ifndef	CPU_SOFTWARE_FP8
M_INLINE fp8 CPhysOBB::MinDistance(const CPlane3Dfp8& _Plane) const
{
	MAUTOSTRIP(CPhysOBB_MinDistance_2, 0.0);
	return OBB_DOTPROD(_Plane.n, m_C) + _Plane.d
		  - M_Fabs(OBB_DOTPROD(_Plane.n, m_A[0])) * m_E[0]
		  - M_Fabs(OBB_DOTPROD(_Plane.n, m_A[1])) * m_E[1]
		  - M_Fabs(OBB_DOTPROD(_Plane.n, m_A[2])) * m_E[2];
}

M_INLINE fp8 CPhysOBB::MaxDistance(const CPlane3Dfp8& _Plane) const
{
	MAUTOSTRIP(CPhysOBB_MaxDistance_2, 0.0);
	return OBB_DOTPROD(_Plane.n, m_C) + _Plane.d
		  + M_Fabs(OBB_DOTPROD(_Plane.n, m_A[0])) * m_E[0]
		  + M_Fabs(OBB_DOTPROD(_Plane.n, m_A[1])) * m_E[1]
		  + M_Fabs(OBB_DOTPROD(_Plane.n, m_A[2])) * m_E[2];
}

#endif	// CPU_SOFTWARE_FP8
*/

#endif // _INC_WPhysOBB
