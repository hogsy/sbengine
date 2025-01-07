#ifndef WPHYSCOLLISION_H
#define WPHYSCOLLISION_H

//#include "PCH.h"
//#include "../xrclass.h"
#include "../CollisionInfo.h"


//class CCollisionInfo;

#define REAL(x) (x)
#define dRecip(x) (1.0/(x))
#define dSqrt(x) sqrt(x)
#define dRecipSqrt(x) (1.0/sqrt(x))
#define dSin(x) sin(x)
#define dCos(x) cos(x)
#define dFabs(x) fabs(x)
#define dAtan2(y,x) atan2((y),(x))
#define dFMod(a,b) (fmod((a),(b)))
#define dCopySign(a,b) (copysign((a),(b)))

#define dInfinity _FP8_MAX

typedef double dReal;
typedef dReal dVector3[4];
typedef dReal dVector4[4];
typedef dReal dMatrix3[4*3];
typedef dReal dMatrix4[4*4];
typedef dReal dMatrix6[8*6];
typedef dReal dQuaternion[4];

#define dMULTIPLYOP0_331(A,op,B,C) \
	(A)[0] op dDOT((B),(C)); \
	(A)[1] op dDOT((B+4),(C)); \
	(A)[2] op dDOT((B+8),(C));
#define dMULTIPLYOP1_331(A,op,B,C) \
	(A)[0] op dDOT41((B),(C)); \
	(A)[1] op dDOT41((B+1),(C)); \
	(A)[2] op dDOT41((B+2),(C));
#define dMULTIPLYOP0_133(A,op,B,C) \
	(A)[0] op dDOT14((B),(C)); \
	(A)[1] op dDOT14((B),(C+1)); \
	(A)[2] op dDOT14((B),(C+2));
#define dMULTIPLYOP0_333(A,op,B,C) \
	(A)[0] op dDOT14((B),(C)); \
	(A)[1] op dDOT14((B),(C+1)); \
	(A)[2] op dDOT14((B),(C+2)); \
	(A)[4] op dDOT14((B+4),(C)); \
	(A)[5] op dDOT14((B+4),(C+1)); \
	(A)[6] op dDOT14((B+4),(C+2)); \
	(A)[8] op dDOT14((B+8),(C)); \
	(A)[9] op dDOT14((B+8),(C+1)); \
	(A)[10] op dDOT14((B+8),(C+2));
#define dMULTIPLYOP1_333(A,op,B,C) \
	(A)[0] op dDOT44((B),(C)); \
	(A)[1] op dDOT44((B),(C+1)); \
	(A)[2] op dDOT44((B),(C+2)); \
	(A)[4] op dDOT44((B+1),(C)); \
	(A)[5] op dDOT44((B+1),(C+1)); \
	(A)[6] op dDOT44((B+1),(C+2)); \
	(A)[8] op dDOT44((B+2),(C)); \
	(A)[9] op dDOT44((B+2),(C+1)); \
	(A)[10] op dDOT44((B+2),(C+2));
#define dMULTIPLYOP2_333(A,op,B,C) \
	(A)[0] op dDOT((B),(C)); \
	(A)[1] op dDOT((B),(C+4)); \
	(A)[2] op dDOT((B),(C+8)); \
	(A)[4] op dDOT((B+4),(C)); \
	(A)[5] op dDOT((B+4),(C+4)); \
	(A)[6] op dDOT((B+4),(C+8)); \
	(A)[8] op dDOT((B+8),(C)); \
	(A)[9] op dDOT((B+8),(C+4)); \
	(A)[10] op dDOT((B+8),(C+8));

#define PURE_INLINE M_INLINE

#define DECL template <class TA, class TB, class TC> PURE_INLINE void

	DECL dMULTIPLY0_331(TA *A, const TB *B, const TC *C) { dMULTIPLYOP0_331(A,=,B,C)}
	DECL dMULTIPLY1_331(TA *A, const TB *B, const TC *C) { dMULTIPLYOP1_331(A,=,B,C)}
	DECL dMULTIPLY0_133(TA *A, const TB *B, const TC *C) { dMULTIPLYOP0_133(A,=,B,C)}
	DECL dMULTIPLY0_333(TA *A, const TB *B, const TC *C) { dMULTIPLYOP0_333(A,=,B,C)}
	DECL dMULTIPLY1_333(TA *A, const TB *B, const TC *C) { dMULTIPLYOP1_333(A,=,B,C)}
	DECL dMULTIPLY2_333(TA *A, const TB *B, const TC *C) { dMULTIPLYOP2_333(A,=,B,C)}

	DECL dMULTIPLYADD0_331(TA *A, const TB *B, const TC *C) { dMULTIPLYOP0_331(A,+=,
		B,C) }
	DECL dMULTIPLYADD1_331(TA *A, const TB *B, const TC *C) { dMULTIPLYOP1_331(A,+=,
		B,C) }
	DECL dMULTIPLYADD0_133(TA *A, const TB *B, const TC *C) { dMULTIPLYOP0_133(A,+=,
		B,C) }
	DECL dMULTIPLYADD0_333(TA *A, const TB *B, const TC *C) { dMULTIPLYOP0_333(A,+=,
		B,C) }
	DECL dMULTIPLYADD1_333(TA *A, const TB *B, const TC *C) { dMULTIPLYOP1_333(A,+=,
		B,C) }
	DECL dMULTIPLYADD2_333(TA *A, const TB *B, const TC *C) { dMULTIPLYOP2_333(A,+=,
		B,C) }

#define dDOTpq(a,b,p,q) ((a)[0]*(b)[0] + (a)[p]*(b)[q] + (a)[2*(p)]*(b)[2*(q)])


	PURE_INLINE dReal dDOT   (const dReal *a, const dReal *b) { return dDOTpq(a,b,1,
		1); }
	PURE_INLINE dReal dDOT13 (const dReal *a, const dReal *b) { return dDOTpq(a,b,1,
		3); }
	PURE_INLINE dReal dDOT31 (const dReal *a, const dReal *b) { return dDOTpq(a,b,3,
		1); }
	PURE_INLINE dReal dDOT33 (const dReal *a, const dReal *b) { return dDOTpq(a,b,3,
		3); }
	PURE_INLINE dReal dDOT14 (const dReal *a, const dReal *b) { return dDOTpq(a,b,1,
		4); }
	PURE_INLINE dReal dDOT41 (const dReal *a, const dReal *b) { return dDOTpq(a,b,4,
		1); }
	PURE_INLINE dReal dDOT44 (const dReal *a, const dReal *b) { return dDOTpq(a,b,4,
		4); }


#define dCROSS(a,op,b,c) \
	(a)[0] op ((b)[1]*(c)[2] - (b)[2]*(c)[1]); \
	(a)[1] op ((b)[2]*(c)[0] - (b)[0]*(c)[2]); \
	(a)[2] op ((b)[0]*(c)[1] - (b)[1]*(c)[0]);
#define dCROSSpqr(a,op,b,c,p,q,r) \
	(a)[  0] op ((b)[  q]*(c)[2*r] - (b)[2*q]*(c)[  r]); \
	(a)[  p] op ((b)[2*q]*(c)[  0] - (b)[  0]*(c)[2*r]); \
	(a)[2*p] op ((b)[  0]*(c)[  r] - (b)[  q]*(c)[  0]);
#define dCROSS114(a,op,b,c) dCROSSpqr(a,op,b,c,1,1,4)
#define dCROSS141(a,op,b,c) dCROSSpqr(a,op,b,c,1,4,1)
#define dCROSS144(a,op,b,c) dCROSSpqr(a,op,b,c,1,4,4)
#define dCROSS411(a,op,b,c) dCROSSpqr(a,op,b,c,4,1,1)
#define dCROSS414(a,op,b,c) dCROSSpqr(a,op,b,c,4,1,4)
#define dCROSS441(a,op,b,c) dCROSSpqr(a,op,b,c,4,4,1)
#define dCROSS444(a,op,b,c) dCROSSpqr(a,op,b,c,4,4,4)

void dNormalize3 (dVector3 a);
void dNormalize4 (dVector4 a);


#define CONTACT(p,skip) ((dContactGeom*) (((char*)p) + (skip)))

	typedef struct dContactGeom {
		dVector3 pos;
		dVector3 normal;
		dReal depth;
		//	dGeomID g1,g2;
	} dContactGeom;

//M_INLINE
int dBoxBox (const dVector3 p1, const dMatrix3 R1,
			 const dVector3 side1, const dVector3 p2,
			 const dMatrix3 R2, const dVector3 side2,
			 dVector3 normal, dReal *depth, int *return_code,
			 int maxc, dContactGeom *contact, int skip);

int Phys_Collide_OBB(const CPhysOBB& _BoxA, 
					 const CPhysOBB& _BoxB, 
					 CCollisionInfo* _pCollisionInfo,
					 int _MaxCollisions);

int Phys_Collide_PolyOBB(const CVec3Dfp4* _pVertices, 
						 const uint32* _pVertIndices, 
						 const int nVertexCount,
						 const CPlane3Dfp4& _PolyPlane, 
						 const CPhysOBB& _Box, 
						 CCollisionInfo* _pCollisionInfo, 
						 int _nMaxCollisions);



class CCollisionFunctions
{
public:
	/*
	static int BoxBox(const CVec3Dfp4& _BoxPos1, CMat4Dfp4 _BoxRot1, const CVec3Dfp4& _BoxExtent1,
	const CVec3Dfp4& _BoxPos2, CMat4Dfp4 _BoxRot2, const CVec3Dfp4& _BoxExtent2,
	CVec3Dfp4& _Normal, fp4 &_Depth,
	CVec3Dfp4 *_CollisionPoints, int _MaxCollisions);
	*/

	static int BoxTriangle(const TOBB<fp8>& _Box,
						   const CVec3Dfp8 *_Vertices,
						   CCollisionInfo* _pCollisionInfo,
						   int _MaxCollisions);

	static int BoxPolygon(const TOBB<fp8>& _Box,
						  const CVec3Dfp8 *_pVertices,
						  const uint32 *_pVertIndices,
						  int _nVertices,
						  CCollisionInfo* _pCollisionInfo,
						  int _MaxCollisions);

	static int BoxPolygon(const TOBB<fp4>& _Box,
						  const CVec3Dfp4 *_pVertices,
						  const uint32 *_pVertIndices,
						  int _nVertices,
						  CCollisionInfo* _pCollisionInfo,
						  int _MaxCollisions);

protected:
	static bool BoxTriangle_TestNormal(fp8 _fp0, 
									   fp8 _fR, 
									   const CVec3Dfp8& _vNormal, 
									   int _iAxis,
									   CVec3Dfp8& _BestNormal,
									   int& _BestAxis,
									   fp8& _BestDepth);

	static bool BoxTriangle_TestFace(fp8 _fp0, fp8 _fp1, fp8 _fp2, 
									 fp8 _fR, fp8 _fD, 
									 CVec3Dfp8& _vNormal, 
									 int _iAxis,
									 CVec3Dfp8& _BestNormal,
									 int& _BestAxis,
									 fp8& _BestDepth);

	static bool BoxTriangle_TestEdge(fp8 _fp0, fp8 _fp1, 
									 fp8 _fR, fp8 _fD, 
									 const CVec3Dfp8& vNormal, 
									 int iAxis,
									 CVec3Dfp8& _BestNormal,
									 int& _BestAxis,
									 fp8& _BestDepth);

#if 1
	static bool BoxTriangle_AddCollision(const CVec3Dfp8& _Position, 
										 const CVec3Dfp8& _Normal,
										 fp8 _IntersectionDepth,
										 CCollisionInfo *_pCollisionInfo,
										 int _Index,
										 int _MaxCollisions)
	{
		if (_Index < _MaxCollisions)
		{
			_pCollisionInfo[_Index].m_bIsCollision = true;
			_pCollisionInfo[_Index].m_Pos = _Position.Getfp4();
			_pCollisionInfo[_Index].m_Plane.n = _Normal.Getfp4();
			_pCollisionInfo[_Index].m_Distance = _IntersectionDepth;
			return true;
		}
		return false;
	}
#endif

	static int BoxTriangle_Clip(const TOBB<fp8>& _Box,
								const CVec3Dfp8& v0, 
								const CVec3Dfp8& v1, 
								const CVec3Dfp8& v2,
								const CVec3Dfp8& _vE0, 
								const CVec3Dfp8& _vE1, 
								const CVec3Dfp8& _vE2, 
								const CVec3Dfp8& _vN,
								const CVec3Dfp8& _BestNormal,
								int _iBestAxis,
								fp8 _BestDepth,
								CCollisionInfo *_pCollisionInfo,
								int _MaxCollisions);


	static bool ClosestPointOnTwoLines(CVec3Dfp8 vPoint1, CVec3Dfp8 vLenVec1, 
									   CVec3Dfp8 vPoint2, CVec3Dfp8 vLenVec2, 
									   fp8 &fvalue1, fp8 &fvalue2) 
	{
		// calulate denominator
		CVec3Dfp8 vp = vPoint2 - vPoint1;
		//SUBTRACT(vPoint2,vPoint1,vp);
		fp8 fuaub  = vLenVec1 * vLenVec2;
		fp8 fq1    = vLenVec1 * vp;
		fp8 fq2    = -(vLenVec2 * vp);
		fp8 fd     = 1.0 - fuaub * fuaub;

		// if denominator is positive
		if (fd > 0.0f) {
			// calculate points of closest approach
			fd = 1.0f/fd;
			fvalue1 = (fq1 + fuaub*fq2)*fd;
			fvalue2 = (fuaub*fq1 + fq2)*fd;
			return true;
			// otherwise  
		} else {
			// lines are parallel
			fvalue1 = 0.0f;
			fvalue2 = 0.0f;
			return false;
		}
	}

	static void ClipPolyToPlane(CVec3Dfp8 *avArrayIn, int ctIn, 
								CVec3Dfp8 *avArrayOut, int &ctOut, 
								const CVec4Dfp8& plPlane )
	{
		// start with no output points
		ctOut = 0;

		int i0 = ctIn-1;

		// for each edge in input polygon
		for (int i1=0; i1<ctIn; i0=i1, i1++) {

#define POINTDISTANCE__(p,v) \
	( p.k[0]*v.k[0] + p.k[1]*v.k[1] + p.k[2]*v.k[2] + p.k[3] )

			// calculate distance of edge points to plane
			fp8 fDistance0 = POINTDISTANCE__( plPlane ,avArrayIn[i0] );
			fp8 fDistance1 = POINTDISTANCE__( plPlane ,avArrayIn[i1] );

#undef POINTDISTANCE__

			// if first point is in front of plane
			if( fDistance0 >= 0 ) {
				// emit point
				avArrayOut[ctOut] = avArrayIn[i0];
/*
				avArrayOut[ctOut][0] = avArrayIn[i0][0];
				avArrayOut[ctOut][1] = avArrayIn[i0][1];
				avArrayOut[ctOut][2] = avArrayIn[i0][2];
				*/
				ctOut++;
			}

			// if points are on different sides
			if( (fDistance0 > 0 && fDistance1 < 0) || ( fDistance0 < 0 && fDistance1 > 0) ) {

				// find intersection point of edge and plane
				CVec3Dfp8 vIntersectionPoint;
				vIntersectionPoint = avArrayIn[i0] - (avArrayIn[i0]-avArrayIn[i1])*fDistance0/(fDistance0-fDistance1);
/*
				vIntersectionPoint[0]= avArrayIn[i0][0] - (avArrayIn[i0][0]-avArrayIn[i1][0])*fDistance0/(fDistance0-fDistance1);
				vIntersectionPoint[1]= avArrayIn[i0][1] - (avArrayIn[i0][1]-avArrayIn[i1][1])*fDistance0/(fDistance0-fDistance1);
				vIntersectionPoint[2]= avArrayIn[i0][2] - (avArrayIn[i0][2]-avArrayIn[i1][2])*fDistance0/(fDistance0-fDistance1);
				*/

				// emit intersection point
				avArrayOut[ctOut] = vIntersectionPoint;

/*				avArrayOut[ctOut][0] = vIntersectionPoint[0];
				avArrayOut[ctOut][1] = vIntersectionPoint[1];
				avArrayOut[ctOut][2] = vIntersectionPoint[2];*/
				ctOut++;
			}
		}
	}

//	static int BoxTriangle_Clip();
};


#endif
