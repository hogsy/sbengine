
#ifndef COLLISIONINFO_H
#define COLLISIONINFO_H


class CXW_Surface;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CCollisionInfo
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	CXR_COLLISIONTYPE_PHYSICS				= 0,	// Typical physics-primitive collision
	CXR_COLLISIONTYPE_BOUND					= 1,	// If BOUND return false all other collisiontypes should also return false.
	CXR_COLLISIONTYPE_PROJECTILE			= 2,
	CXR_COLLISIONTYPE_FLAREOCCLUSION		= 3,	// Not yet used nor supported
#ifndef PLATFORM_CONSOLE
	CXR_COLLISIONTYPE_RAYTRACING			= 4,
#endif

	/*

	The behaviour of a collisiontype depend on the model:

	CXR_Model_BSP:
	BOUND				Collision against the visual & physical bounding box of the model
	All other collision types are on polygon-level

	CXR_Model_TriMesh:
	PHYSICS				Not supported
	BOUND				Collision against the visual & physical bounding box of the model
	PROJECTILE			Collision against collision solids
	FLAREOCCLUSION		Not supported
	RAYTRACING			Triangle-level collision

	CXR_PhysicsModel_Sphere/Box:
	All collision types are on primitive-level
	*/


	// Use SetReturnValues() to set the return fields in CCollisionInfo

	CXR_COLLISIONRETURNVALUE_TIME = 1,					// m_Time, m_iObject
	CXR_COLLISIONRETURNVALUE_POSITION = 2,				// m_Pos, m_Plane + TIME (means TIME return values are needed and will be calculated.)
	CXR_COLLISIONRETURNVALUE_PENETRATIONDEPTH = 4,		// m_Distance + TIME + POSITION
	CXR_COLLISIONRETURNVALUE_SURFACE = 8,				// m_pSurface, m_SurfaceType + POSITION
	CXR_COLLISIONRETURNVALUE_SURFACEVELOCITY = 16,		// m_Velocity, m_RotVelocity + POSITION
	CXR_COLLISIONRETURNVALUE_LOCALPOSITION = 32,		// m_LocalPos, m_LocalNodePos, m_RotVelocity + POSITION
};

class CCollisionInfo
{
public:
	void Clear();
	CCollisionInfo();

	void SetReturnValues(int _Mask);

	void CopyParams(const CCollisionInfo& _CInfo);			// Copy params from _CInfo to *this
	void CopyReturnValues(const CCollisionInfo& _CInfo);	// Copy return values from _CInfo to *this
	bool Improve(const CCollisionInfo& _CInfo);				// Copies _CInfo to *this if _CInfo collision is more recent than *this.
	bool IsImprovement(const CCollisionInfo& _CInfo) const;	// returns true if _CInfo's collision is more recent than *this's collision.
	bool IsImprovement(fp32 _Time, fp32 _Distance = 0.0f) const;	// returns true if a collision at _Time/_Distance is more recent than *this's collision.
	bool IsComplete() const;								// returns true if no more collision tests need to be done, ie. if it m_bIsCollision and !m_bIsValid or no return values are needed.

	CMat4Dfp32			m_LocalNodePos;				// (note: also reused by rigid body physics events)
	CPlane3Dfp32		m_Plane;
	uint32	m_bIsCollision : 1;			// True if the collision info represent a collision.
	uint32	m_bIsValid : 1;				// True if the collision time could be calculated. False if collision at start position. If true, m_bIsCollision is also true.
	uint32	m_bIsContact : 1;			// Not used for anything yet.
	uint32	m_CollisionType : 5;		// Not a return value
	uint32	m_ReturnValues : 8;			// You MUST use SetReturnValues to initialize this field properly. The default value is all fields.
	uint32	m_iObject : 16;
	uint8				m_IN1N2Flags;				// Intersect and Notify flags
	fp32				m_Time;						// -> interval [0,1]
	fp32				m_Distance;					// This should be named PenetrationDepth. It is the distance along the collision plane normal between two colliding objects. Since the objects are intersecting, the distance is negative.
	CVec3Dfp32			m_Pos;
	CVec3Dfp32			m_LocalPos;
	int					m_LocalNode;
	CVec3Dfp32			m_Velocity;
	fp32				m_RotVelocity;				// Rotation around m_Plane.n
	const CXW_Surface*	m_pSurface;
	int					m_SurfaceType;
	fp32				m_Friction;
};

#endif
