#ifndef _INC_WPhysPCS
#define _INC_WPhysPCS


#if 0//def PLATFORM_PS2
# include "eeregs.h" 	// debug purpose only!
# define DEBUG_GS_BGCOLOR(x) DPUT_GS_BGCOLOR(x) // debug purpose only!
#else
# define DEBUG_GS_BGCOLOR(x)
#endif


#define PCSCHECK_INSOLID



#include "../../MOS.h"

//-------------------------------------------------------------------------------------------
//
// Physics Potential Collision Set		Created and Added by Joacim Jonsson 03/02/26 (yymmdd)
//
// Dont fucking touch the structs and variables unless you _really_ know what you
// are doing as there is probably a ton of PS2 specific assembler accessing them.
//
// Class was created in order to optimize physics/collision engine, it is using a lot
// of types such as float and int, but this is not considered to violate SBZ code standard
// since it is on _purpose_ to ensure platform _native_ types.
//
//-------------------------------------------------------------------------------------------

#define MAX_PCS_FACES 256															// max number of faces a set can contain
#define MAX_PCS_POINTS 8															// max number of points a set can contain
#define MAX_PCS_SPHERES 8															// max number of spheres a set can contain
#define MAX_PCS_BOXES 8																// max number of axis alignes boxes


#define PCS_SIGMA 0.001f																// faces within this range of a Box is Considered to be touching!

class CXR_PhysicsModel;


// Class Potential Collision Set

enum
{
	PCS_DO_INTERSECT	= 1,
	PCS_DO_NOTIFY1 		= 2,
	PCS_DO_NOTIFY2		= 4,
};


class CPotColSet
{
private:

	static inline float PlanePointDistance(const float *w, const float *x)
	{
		return x[0] * w[0] + x[1] * w[1] + x[2] * w[2] + w[3];
	}


	static inline float PlanePointDistance(const float *w, const float x, const float y, const float z)
	{
		return x * w[0] + y * w[1] + z * w[2] + w[3];
	}


	int IntersectFaceBox(const float *_fFace, const CMat43fp4 *_T0, const CMat43fp4 *_T1, const float *_BoxMinMax, CCollisionInfo *_pCollInfo);
	int IntersectBoxBox(const float *_Box, const CMat43fp4 *_T0, const CMat43fp4 *_T1, const float *_BoxMinMax, CCollisionInfo *_pCollInfo);


public:
#ifdef PCSCHECK_INSOLID
	bool  m_bIsInSolid;
	int   m_iSolidObj;
	uint8 m_SolidFlags;
#endif

	// bounds of set.
	float m_BoxMinMax[6];															// "upper left" followed bu "lower right" of set bounding box
	float m_fCenter[3];																// center of the sphere approximating BoxMinMax
	float m_fRadius;																// radius of the sphere approximating BoxMinMax

	// number of each primitives in set.
	int	m_nFaces; 																	// number of faces in set
	int m_nBoxes;																	// number of boxes in set
	int m_nPoints;																	// number of points in set
	int m_nSpheres;																	// number of spheres in set

	// all primitives within set. (plane equations are associated with faces)
	float m_fPlaneEqs[MAX_PCS_FACES][4];											// [a,b,c,d] plane Equations
	float m_fFaces[MAX_PCS_FACES][9];												// {[x,y,z]}^3 faces
	float m_fBoxes[MAX_PCS_BOXES][6];												// [xmin,ymin,zmin, xmax,ymax,zmax ]
	float m_fPoints[MAX_PCS_POINTS][3];												// [x,y,z]
	float m_fSpheres[MAX_PCS_SPHERES][4];											// [x,y,z,r]


	// flags and ObjIndexes could perhaps be ommited of notify system is separated!

	// Flags included to save multiple lookups in PhysStates
	uint8 m_FaceFlags[MAX_PCS_FACES];												// see enum
	uint8 m_BoxFlags[MAX_PCS_FACES];												// see enum
	uint8 m_PointFlags[MAX_PCS_POINTS];
	uint8 m_SphereFlags[MAX_PCS_SPHERES];

	const class CXW_Surface *m_pSurface[MAX_PCS_FACES];

	// ObjIndexes needed to send messages etc when a collision occurs
	int m_iObjFaces[MAX_PCS_FACES];													// ObjIndex associated with face
	int m_iObjBoxes[MAX_PCS_FACES];													// ObjIndex associated with face
	int m_iObjPoints[MAX_PCS_POINTS];
	int m_iObjSpheres[MAX_PCS_SPHERES];

	static class CWorld_PhysState *ms_pCurrPhysState;
	
	CPotColSet() { Clear(); }		// constructor
	CPotColSet(CWorld_PhysState *_pPS) { Clear(); ms_pCurrPhysState = _pPS; }		// constructor

	void Clear();		// clear a set

	void SetBox(const float *_BoxMinMax);
	void SetBox(const CBox3Dfp4* _pBox);
	void SetBox(const CVec3Dfp4* _pMin, const CVec3Dfp4* _pMax);
	void GetBox(float *_BoxMinMax) const;

	void CollectSet(const float *_BoxMinMax, CXR_PhysicsModel *_pObj, int _nObj);

	void SubSet(const float *_BoxMinMax, CPotColSet *_pcs);

	bool CollideClosestFace(const float *_BoxMinMax);

	int CollidePoint(const float *_u, const float *_v, CCollisionInfo *_pCollisionInfo, int _nMaxCollInfo);
	int CollideSphere(const float *_u, const float *_v, const float _radius, CCollisionInfo *_pCollisionInfo, int _nMaxCollInfo);
	int CollideBox(const CMat43fp4 *_T0, const CMat43fp4 *_T1, const float *_BoxMinMax, CCollisionInfo* _pCollInfo, int _nMaxCollInfo);

	void AddFace(const uint8 _iFlags, const int _iObj, const float *_fPlaneEq, const float *_fFace, const class CXW_Surface *_pSurface);
	void AddPoint(const uint8 _iFlags, const int _iObj, const float *_fPoint);
	void AddBox(const uint8 _iFlags, const int _iObj, const float *_fMinMax);
	void AddSphere(const uint8 _iFlags, const int _iObj, const float *_fPos, const float radius);

	int ValidBound(const float *_BoxMinMax) const;
	int ValidBox(const float *_BoxMinMax) const;
	int ValidFace(const float *_fPlaneEq, const float *_fFace) const;
	int ValidSphere(const float *_fSphere) const;
	int ValidPoint(const float *_fPoint) const;
	int ValidPlane(const float *_fPlaneEq) const;
	
	void PlaneSieve(const float a, const float b, const float c, const float d);
	
	void DebugRender(class CWorld_PhysState* _pPhysState);
};



#endif // _INC_WPhysPCS




