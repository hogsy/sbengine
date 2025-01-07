#ifndef _CCONSTRAINTSYSTEM_
#define _CCONSTRAINTSYSTEM_

#include "../../../Shared/MOS/XR/XRSkeleton.h"
#include "WObj_Messages.h"

// Constraint system
// Anders Backman 2002-10-11
// Rewritten 2003-07-14
// 2004-02-03 Fixed missed frame, retain velocity Backman
// ================================================================================================
// Physics
// =======
// The physics of the system will use a modified Verlet integrator as used in Hitman but modified
// to allow variable timestep. We store the previous frames timestep to normalize steps when we
// integerate (or step). There will be no inertia tensors, torques etc as we will rely solely on
// cartesian velocity vectors and robust constraints.
// ===================
// Collision detection
// ===================
// Collision detection will be handled in the simplest possible manner. The colliding body should
// not need to do intra object colldet, we merely subject all the systems points that have the m_bCollision
// flag set to true.
// ==================
// Collision response
// ==================
// A bodypart tha hits will be moved out of the offending geometry and we may store the dislocation
// if we find that we want a more bouncy response. We will also brake any velocity tangential to
// the collision normal (proportional to depth) to simulate friction.
// ====
// Rest
// ====
// Each simulation round we measure if the body has stopped moving and flag this to avoid further
// calculations. Bodies at rest that get hit will reset the rest flags again.
// ===========
// Constraints
// ===========
// Inverse Kinematics with angle limits, relative masses (damping in some systems), various joint
// types will all be handled by a simple yet effective constraint system.
// A constraint consist of:
// A point fixed to one of the parts/bone of the body, the Fixed Point or FP
// A rule (larger than, equal or smaller than) a certain length, L
// A point fixed to another part/bone that it is constraining, the Constraint Point or CP
// There may be a need for another constraint type where the FP is replaced with a line segment FL
// We enforce the constraint by executing them one by one in the list, possibly several iterations.
//
//
//
//
// ================================================================================================
// Classes
// ================================================================================================
// =================
// CConstraintSystem
// =================
// Each body will have one instance of CConstraintSystem to hold all
// CConstraintPoints and CConstraintRules
// The passes gravity and impulses to the CConstraintHandle's as well as the enforcement
// of all constraints of the system.

// ================
// CConstraintPoint
// ================
// Class that implements the Verlet integrator.
// Litlle more than a container for current and previous pos and the last time interval

// ===============
// CConstraintRule
// ===============
// Implements various range derived rules
// ==================
// CConstraintAngleRule
// ==================
// Implements rules for angles between points as measured from a common point
// ==================
// CConstraintKneeRule
// ==================
// Implements knee and elbow joints

// Each macro #defined 'constant' begins with a lowercase k
// ===================================================================================================
// Standard errorcodes to use as return values
// ===================================================================================================
// All error codes are signed 32 bit integers, positive values indicate success
// zero indicate a failure that we should handle, negative are mortal failures
// Some calls may wish to return a count of some kind (searches etc), it is then OK to return the count
// as only negative results are real errors
// ===================================================================================================
// kSuccess: The call succeeded in whatever it was doing
// kFailure: The call failed in an OK way (example: CheckCollision() is OK even if no coll occur)
// kError: Generic error code, should only be used when the other are not appropriate
// kErrParams: One or more of the params was wrong
// kErrIndexOutOfRange: A supplied/internal index was out of range (this code has precedence over kErrParams)
// kErrMemory: Failed to allocate memory
// kErrFiles: failed to read/write/find a file
// kErrOS: The OS failed somehow (aside from kErrMemory or kErrFiles)
// kNotImplemented: Dummy errcode for methods with body not yet fully implemented
#define kSuccess				1
#define kFailure				0
#define kError					-1
#define kErrParams				-2
#define kErrIndexOutOfRange		-3
#define kErrMemory				-4
#define kErrFiles				-5
#define kErrOS					-6
#define kErrState				-7
#define kErrNull				-8
#define kErrNotImplemented		-666


#ifndef MY_ABS
#define MY_ABS(x) ((x) >= 0 ? (x) : (-(x)))
#endif

#ifndef MY_MIN
#define MY_MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MY_MAX
#define MY_MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

#define SAFE_DELETE(p) if (p != NULL) {delete p; p = NULL;}
#define ERR_CHECK(err) if (err < kFailure) {return(err);}
#define MEM_CHECK(ptr) if (ptr == NULL) {return(kErrMemory);}

// Some commonly used math and physics constants
// kPi: The quotient between circumference and diameter in euclidan space
// If computers had better floating point precision we might use:
// kPi = 3.14159265358979323846264338337950288419716939937510f if I recall correctly
#define kPi						3.141592654f
// kTol: Anything less is considred to be zero
#define kTol					0.0000000001f
// Some angle definitions in radians, degrees etc
#define k90DegRad				1.57079632679f
#define k180DegRad				3.14159265359f
#define k270DegRad				4.71238898038f
#define k360DegRad				6.28318530718f
// Multipliers to convert between radians and degrees
// (Much faster than the inline templates below)
#define kDeg2Rad				0.01745329222f	
#define kRad2Deg				57.29577951f
// Small value below which we sometimes treat as zero
#define kEpsilon				0.00001f


#define kConstraintDefaultStiffness			0.5f
#define kConstraintBoneStiffness			0.9f

class CWObject_Character;
class CRegistry;

class CConstraintSystemClient;
class CConstraintPoint;
class CConstraintRule;
class CConstraintSubPosRule;
class CConstraintAngleRule;
class CConstraintKneeRule;
class CConstraintAngleRule;

// #ifdef _DEBUG
#define _DRAW_SYSTEM 1
// #endif


class CConstraintSystem : public CReferenceCount
{
public:
	enum
	{
		CS_SKELETON = 0,
		CS_RIGIDBODY = 1,
		CS_GIBSYSTEM = 2,
	};
	enum {
		NOTREADY = 0,		// Not yet started up
		GETFRAME = 1,		// We get the first frame and apply it to the ragdoll with inertia
		READY = 4,			// We run entirely in ragdoll with collision detection
		STOPPED = 5,		// We've stopped, set m_State to READY again if dragged etc
	};

	enum
	{
		BODY_NONE = -1,
		BODY_ALL = 0,
		BODY_PART,
		HEAD_PART,
		R_ARM_PART,
		L_ARM_PART,
		R_LEG_PART,
		L_LEG_PART,

		BODYPARTS_NUM,
	};

	enum
	{
		ORIENTATION_BACK = 0,	// The default case
		ORIENTATION_BELLY = 1,
		ORIENTATION_LEFT = 2,
		ORIENTATION_RIGHT = 3,
		ORIENTATION_BENTOVER = 4,
		ORIENTATION_SITUP = 5,
	};

	CConstraintSystem();
	virtual ~CConstraintSystem();

	virtual void Init(int _iObj,CWObject_CoreData* _pClient,CWorld_PhysState* _pWPhysState,CWO_Character_ClientData* _pCD,CConstraintSystemClient* _pSystemClient,int _nTracks = 0);
	virtual void Setup(struct SConstraintSystemSettings* _pSettings);
	// These pack down and restore ram after ragdoll is stopped
	virtual void PackStopped();
	virtual void UnPackStopped();

	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);

	int16 SetRebound(fp32 _Rebound);
	int16 SetStopThreshold(fp32 _StopThreshold);
	int16 SetConstraintIterations(int _ConstraintIterations);
	int16 SetGravity(fp32 _Gravity);

	virtual void SetState(int _State);
	int16 GetState();
	bool IsStopped();
	// Returns the current orientation of the ragdoll
	int16 GetOrientation();

	virtual void Clear(bool _bAll = true);
	bool IsActive();
	bool IsInited();
	void SetOrgMat(const CMat4Dfp32& _OrgPos);
	void Activate(bool _Active);
	void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	int16 SetRootPos(const CVec3Dfp32& _Pos);
	int16 GetRootPos(CVec3Dfp32* _pReturnPos);

	// Adds a CConstraintPoint with the given params to the system
	// Returns its index or a negative errcode
	void SetPointRadius(fp32 _Radius);
	int16 AddPoint(const CVec3Dfp32& _Pos,fp32 _Mass,bool _Collision,int _BodyPart);
	int16 AddPoint(int16 _iP,const CVec3Dfp32& _Pos,fp32 _Mass,bool _Collision,int _BodyPart);
	// Multiplies the current speed by _Factor
	void ScaleSpeed(fp32 _Factor);
	// Sets the speed of all points to _Speed
	void SetSpeed(CVec3Dfp32 _Speed,fp32 _Random = 0.0f);
	// Adds _Speed to all points of the system
	void AddSpeed(CVec3Dfp32 _Speed,fp32 _Random = 0.0f);
	// Adds _Impulse (in kg units per second) to all points belonging to _BodyPart
	// NOTE kR_Leg and kL_Leg belongs to BODY_PART as does kR_Arm and kL_Arm
	int16 AddImpulse(int _BodyPart,const CVec3Dfp32& _Impulse);
	// Adds impulse and _Location to be applied later
	// _Pos is used to determine what body part was hit WHEN the impulse is applied
	void AddPendingImpulse(const CVec3Dfp32& _Pos,const CVec3Dfp32& _Impulse);
	// Applies any pending impulse, 100% to closest pos, 50% to second closest
	void ApplyPendingImpulse();
	// Forces the position of a particular bone
	// _bKeep detrmines wether the position will be kept or just for the next update 
	int16 SetPendingBonepos(int _iBone,const CVec3Dfp32& _Pos,bool _bKeep = false);
	// Add an object ID that will push all points away from its bbox
	void AddPushObject(int _iPusher);


	// Converts _Pos in world coordinates to the closest bodypart
	int GetBodypart(const CVec3Dfp32& _Pos);
	// The _SoundIndex will be played whenever some part of _BodyPart collides with something for the first
	// time.
	int16 AddBodypartSound(int _BodyPart,int _SoundIndex);
	void SetMaterialSound(int _Material, CWObject_CoreData *_pObj, CWorld_Server *_pWServer);
	// Plays _BodyPart sound if any, returns true if a sound should play and false if not
	virtual bool PlayBodypartSound(int _BodyPart,const CVec3Dfp32& _Pos,fp32 _Impact);

	// Adds a CBaseConstraintRule with the given params
	// _Rule can be one of: kConstraintRuleSmaller,kConstraintRuleEqual,kConstraintRuleLarger,
	// kConstraintRuleSmallerSoft,kConstraintRuleEqualSoft,kConstraintRuleLargerSoft.
	// Returns the index of the rule or a negative errcode
	int16 AddConstraint(int16 _iPA,int16 _iPB,int16 _Rule,fp32 _Stiffness = kConstraintDefaultStiffness);
	// Similar to AddConstraint but adds an actual bone
	int16 AddBone(int16 _iPA,int16 _iPB,bool _bForce = false);
	// Adds a child bone to rule _iRule
	int16 AddChildBone(int16 _iRule,int16 _iChildBone);
	void AddFixedClothChildBones(const int16& _curBone, const int16& _iMin, const int16& _iMax, CXR_Skeleton* _lpClothSkeleton[CWO_NUMMODELINDICES]);

	// Adds a constraint that will simply keep the point _iP0 at _Fract between _iPA and _iPB
	// Constraining _iP0 will move _iPA and _iPB in a way to conserve angular momentum around _iP0
	int16 AddBoneSubPos(int16 _iP0,int16 _iPA,int16 _iPB,fp32 _Fract);
	// Adds a constraint with a range that is the actual range multiplied by _RangeFactor
	// It is an error if kConstraintRuleEqual or kConstraintRuleEqualSoft is supplied as _Rule
	int16 AddConstraintRangeMultiplier(int16 _iPA,int16 _iPB,int16 _Rule,fp32 _Stiffness,fp32 _RangeFactor);
	// Adds a constraint with a range that is the actual range multiplied by _RangeFactor
	// It is an error if kConstraintRuleEqual or kConstraintRuleEqualSoft is supplied as _Rule
	int16 AddConstraintRangeValue(int16 _iPA,int16 _iPB,int16 _Rule,fp32 _Stiffness,fp32 _RangeValue);
	// Adds an angle constraint for the angle between _iP0->_iPA and _iP0->_iPB
	// _Rule can be one of: kConstraintRuleSmaller,kConstraintRuleEqual,kConstraintRuleLarger,
	// kConstraintRuleSmallerSoft,kConstraintRuleEqualSoft,kConstraintRuleLargerSoft.
	// Returns the index of the rule or an errcode
	int16 AddAngleConstraint(int16 _iP0,int16 _iPA,int16 _iPB,int16 _Rule,fp32 _Stiffness,fp32 _Angle);
	// Adds a rule that will keep the knee correct, relies on SetupCoordinateSystem correctly set up
	int16 AddKneeConstraint(int16 _iP0,int16 _iP1,int16 _iPA,int16 _iPB,bool _bLeft,fp32 _Stiffness = kConstraintDefaultStiffness);
	// Adds a rule that will keep the elbow correct, relies on SetupCoordinateSystem correctly set up
	int16 AddElbowConstraint(int16 _iP0,int16 _iP1,int16 _iPA,int16 _iPB,bool _bLeft,fp32 _Stiffness = kConstraintDefaultStiffness);
	// Adds a rule that will keep _iPB _Distance or more from the plane set up by _iP0->iP1 and _iP0->_iPA
	int16 AddBodyPlaneConstraint(int16 _iP0,int16 _iP1,int16 _iPA,int16 _iPB,bool _bLeft,fp32 _Distance,fp32 _Stiffness = kConstraintDefaultStiffness);
	int16 AddPlaneConstraint(int16 _iPt0,int16 _iPt1,int16 _iPt2,int16 _iPt3,int16 _i0,int16 _i1,int16 _i2,int16 _i3,fp32 _Distance,fp32 _Stiffness);
	int16 AddCapsuleConstraint(int16 _iPA,int16 _iPB,int16 _iPt0,int16 _iPt1,fp32 _Radius,fp32 _Stiffness);
	// Similar to AddCapsuleConstraint above but uses _iRadiusPt to determine radius 
	int16 AddCapsuleConstraintByPoint(int16 _iPA,int16 _iPB,int16 _iPt0,int16 _iPt1,int16 _iRadiusPt,fp32 _Stiffness);
	// Adds a rule that will keep int16 _iPA->_iPB pointing within _Angle of _Direction, relies on SetupCoordinateSystem correctly set up
	// _Direction can be any of FORWARD,BACKWARD,RIGHT,LEFT,UP,DOWN and _Angle should be 0 - 90 degrees
	int16 AddDirAngleConstraint(int16 _iPA,int16 _iPB,fp32 _Stiffness,int16 _Direction,fp32 _Angle);
	int16 AddDirAngleConstraint2(int16 _iPA,int16 _iPB,fp32 _Stiffness,int16 _iRef,int16 _iRef2,fp32 _Angle);

	// Changes the mass of point _iP to _Mass, updating all rules that apply to it
	int16 ModifyPointMass(int16 _iP,fp32 _Mass);

	// Sets up the default Starbreeze skeleton
	int16 SetupSkeleton(int _Type, CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CXR_Skeleton* _lpClothSkeleton[CWO_NUMMODELINDICES]);

	// Sets the first coordinate system the first time it's called and the second the second (did you get that?)
	// Returns an errcode if some of the supplied indices doesn't exist
	int16 SetupCoordinateSystem(int16 _iUp0,int16 _iUp1,int16 _iLeft0,int16 _iLeft1);

	// Updates m_SystemMat0 and m_SystemMat1
	virtual void UpdateCoordinateSystem();

	// Draws the coordinate system (if any) at _iOrigo0 and _iOrigo1,
	// Fwd is drawn in red, right is drawn in green and up is drawn in blue
	int16 DrawCoordinateSystem(int16 _iOrigo0,int16 _iOrigo1,fp32 _Length,fp32 _Duration,const CVec3Dfp32& _Offset);

	// Calls CalcForces,Move,Collision,Constrain,Apply for the system
	virtual int16 Animate(int32 _GameTick,CMat4Dfp32& _WMat);
	int16 Simulate();
	void GetBBox(CVec3Dfp32& _Min,CVec3Dfp32& _Max);	// A box containing all the points

	// Draw system as appropriate
	int16 Draw(fp32 _Duration,CVec3Dfp32 _Offset = CVec3Dfp32(0,0,0));
	int16 DrawPoints(fp32 _Duration,uint32 _Color = 0xffffffff);
	void EvalAnim(fp32 _Frac,CXR_AnimLayer* _pLayers, int _nLayers,CXR_Skeleton* _pSkel,CXR_SkeletonInstance* _pSkelInst, CMat4Dfp32& _WMat,int _Flags = 0);
	
	// Gets the skeletons default pose to be used for rot/mov diffing
	int16 GetDefaultPose(CXR_SkeletonInstance* _pSkelInstance = NULL);
	int16 PostSetupToLocal();

	// Retrieves the bone positions of the character
	// If _bVelocity == false the system will be teleported thus no velocity will change
	// If _bAnimatedPose == true we take the current pose, otherwise we take the default
	// _BlendFactor blends between pose (0.0) and ragdoll (1.0)
	// int16 BlendAnimState(bool _bVelocity,bool _bAnimatedPose,fp32 _BlendFactor);

	// Retrieves the bone positions of the character
	// If _bVelocity == false the system will be teleported thus no velocity will change
	virtual int16 GetAnimatedFrame(bool _bVelocity);

	// Draw the joints of the supplied skeleton
	void DebugRenderSkeletonJoints(const CMat4Dfp32 &_Mat,
									CXR_SkeletonInstance* _pSkelInstance,
									CXR_Skeleton* _pSkel);

	void CalcMatrix(bool _bAnimatedPose,
						int _iNode,
						const CMat4Dfp32 &_Mat,
						CXR_SkeletonInstance* _pSkelInstance,
						CXR_Skeleton* _pSkel,
						CMat4Dfp32& _lMat);

	virtual void CalcMatrices_r(bool _bAnimatedPose,
						int _iNode,
						const CMat4Dfp32 &_Mat,
						CXR_SkeletonInstance* _pSkelInstance,
						CXR_Skeleton* _pSkel,
						TArray<CMat4Dfp32> &_lMat);

	void DebugRenderSkeletonJoints(CXR_Skeleton* _pSkel,
		CXR_SkeletonInstance* _pSkelInstance,
		const CMat4Dfp32 &_Mat);

protected:
friend class CConstraintSystemClient;
friend class CConstraintRule;
friend class CConstraintSubPosRule;
friend class CConstraintAngleRule;
friend class CConstraintKneeRule;
friend class CConstraintBodyPlaneRule;
friend class CConstraintPlaneRule;
friend class CConstraintDirAngleRule;
friend class CConstraintDirAngleRule2;
friend class CConstraintBox;
friend class CConstraintCapsuleRule;

	// Gravity, wind etc
	int16 CalcForces();
	// Translate points
	int16 Move();
	// Prepares/Updates collision detection data
	// Returns true if the colldata was updated
	bool UpdateCollisionBox();
	// Returns the current dimensions of the system
	virtual void GetCollBBox(CVec3Dfp32& _Min,CVec3Dfp32& _Max);
	// Draws various coll det related vectors
	void DrawCollisionBox();

	int16 CheckCollision();
	int16 CheckCollisionCenter();
	int16 CheckCollisionRegular();
	int16 CheckCollisionPushObjects();

	// Get the center for the bbox used in colldet
	virtual int16 GetCollBoxPos(CMat4Dfp32* _pPosMat);
	// Collide points that belong to actual bones
	int16 CheckCollision2(bool _bCenter = false);
	// Apply constraint rules to the points
	int16 Constrain();

	// Determines if the system has stopped or not
	// If none of the bone points has moved more than _Diff the system is considered stopped
	bool CheckStop(fp32 _Diff);

	// Calculate quats/matrices from data
	virtual int16 Apply(CMat4Dfp32& _WMat);

	bool m_bInited;
	int16 m_State;					// What state the system is in (but you already guessed that didn't you?)
	int16 m_SystemType;
public:
	int16 m_IdleTicks;
	int								m_nTracks;	// Max nbr of tracks rot/move
	int m_iMaterial;

protected:
	bool		m_bFirstApply;
	bool		m_bFirstCollCheck;			
	int			m_ConstraintIterations;		// Number of constraint iterations
	fp32			m_Gravity;
	fp32			m_ReboundFactor;			// How much bounce after initial xxx frames
	fp32			m_StopThreshold;			// Move below which the system is considered stopped

	int m_LastFrameTick;			// Timestamp of last frame
	int m_AnimateCount;				// The nbr of times Animate has been called

	// Ptrs to the client objects
	// Really annoying having to hold ptrs to all these
	int									m_iObject;
	class CWObject_CoreData*			m_pClient;
	class CWorld_PhysState*				m_pWPhysState;
	class CWO_Character_ClientData*		m_pCD;
	class CConstraintSystemClient*		m_pSystemClient;
	
	// List of CConstraintPoint that make up the system
	TArray<CVec3Dfp32>					m_lDefaultPts;
	TArray<CMat4Dfp32>					m_lDefaultMats;

	// List of CConstraintPoint used by the rules and collision system
	TArray<CConstraintPoint*>			m_lpPts;
	fp32									m_PtRadius;

	// If m_liBodypartSound[n] != -1 it has a sound associated to it
	// When colliding with enough force a sound will be played if gametick >= m_lBodyPartSoundNextTick
	// m_lBodyPartSoundNextTick will get an added delay
	int32								m_liBodypartSoundWeak;	// The generic index for the weak impacts
	int32								m_lBodyPartSoundWeakNextTick;
	int32								m_liBodypartSound[BODYPARTS_NUM];
	int32								m_lBodyPartSoundNextTick[BODYPARTS_NUM];

	// List of rules that apply to the points
	TArray<CConstraintRule*>			m_lpRules;
	TArray<int32>						m_liPushObjects;

	CMat4Dfp32						m_OrgMat;
	int								m_iPendingBone;
	CVec3Dfp32						m_PendingBonePos;
	bool							m_bPendingBonePosKeep;	// Keep pos forever when true
	CVec3Dfp32						m_PendingImpulse;
	CVec3Dfp32						m_PendingImpulsePos;

	CVec3Dfp32						m_SystemVelocity;		// Average speed of system
	CVec3Dfp32						m_PrevSystemVelocity;	// Previous m_SystemVelocity
public:
	CVec3Dfp32						m_SystemAvgPos;			// Avg pos of system points

protected:
	bool							m_bCoordSysAvailable;	// True when indices below are valid
	CMat4Dfp32						m_SystemMat;
	int16							m_iSystemUp0;
	int16							m_iSystemUp1;
	int16							m_iSystemLeft0;
	int16							m_iSystemLeft1;

	bool		m_bCollReady;		// We must prepare collision data if m_bCollReady is false
	CVec3Dfp32	m_CollMinBox;		// These two make up the dimensions of the collision box
	CVec3Dfp32	m_CollMaxBox;
	CVec3Dfp32	m_MoveMinBox;
	CVec3Dfp32	m_MoveMaxBox;
	int32		m_iCollSelection;	// Index of collision selection
	// CPotColSet	m_PCS;				// PCS system
	struct SCollPoly
	{
		SCollPoly()
		{
		};
		SCollPoly(const CVec3Dfp32& _N,const CVec3Dfp32& _P0,const CVec3Dfp32& _P1,const CVec3Dfp32& _P2)
		{
			m_N = _N;
			m_Pt0 = _P0;
			m_Pt1 = _P1;
			m_Pt2 = _P2;
		};

		CVec3Dfp32	m_N;
		CVec3Dfp32	m_Pt0;
		CVec3Dfp32	m_Pt1;
		CVec3Dfp32	m_Pt2;
	};
	int								m_CurCollCount;	// Nbr of collisions from last call to CheckCollision()
	TArray<SCollPoly>				m_lCollPolys;
	TArray<SCollPoly>				m_lDynamicCollPolys;
	
	// SortByNormal used by UpdateCollisionBox() to sort polys by their normal 'up-ness'
	static int M_CDECL SortByNormalAndHeight(const void* _FirstElem,const void* _SecondElem);

	// Returns the quaternion that will rotate _v0 into _v1
	// It handles small differences very well but has problems when _v0 == -_v1
	static CQuatfp32 RotationArcQuaternion(const CVec3Dfp32& _v0,const CVec3Dfp32& _v1);
	static CQuatfp32 RotationArcQuaternion2(const CVec3Dfp32& _v0,const CVec3Dfp32& _v1);
	// Returns the CMat4Dfp32 that will rotate _v0 into _v1
	static CMat4Dfp32 RotationArcMatrix(const CVec3Dfp32& _v0,const CVec3Dfp32& _v1);
	static CMat4Dfp32 RotationArcMatrix2(const CVec3Dfp32& _v0,const CVec3Dfp32& _v1);

	// Measures the squared distance from _p to segment _M + t_ * _B, returning the param t.
	static inline fp32 SqrDistanceToSegment(const CVec3Dfp32& _P,const CVec3Dfp32& _B,const CVec3Dfp32& _M,fp32* _pT = NULL,CVec3Dfp32* _pDiff = NULL)
	{
		CVec3Dfp32 diff;
		fp32 t;

		diff = _P - _B;
		t = _M * diff;
		if (t > 0.0f)
		{
			fp32 dotMM = _M * _M;
			if (t < dotMM)
			{
				t = t / dotMM;
				diff = diff - _M * t;
			}
			else
			{
				t = 1.0f;
				diff = diff - _M;
			}
		}
		else
		{
			t = 0.0f;
		}

		if (_pDiff)
		{
			*_pDiff = diff;
		}
		if (_pT)
		{
			*_pT = t;
		}
		return(diff * diff);
	};

	inline bool SameSide( const CVec3Dfp32& _p1, const CVec3Dfp32& _p2, const CVec3Dfp32& _a, const CVec3Dfp32& _b)
	{
		CVec3Dfp32 cp1,cp2;
		(_b - _a).CrossProd(_p1 - _a,cp1);
		(_b - _a).CrossProd(_p2 - _a,cp2);
		if (cp1 * cp2 >= 0.0f)
		{
			return(true);
		}
		else
		{
			return(false);
		}
	};

	// Fast test for wether a point _p lies inside the triangle _a, _b, _c when _p is projected onto
	// the triangles plane.
	inline bool TriPointTest( const CVec3Dfp32& _p,
							 const CVec3Dfp32& _a,
							 const CVec3Dfp32& _b,
							 const CVec3Dfp32& _c)
	{
		if ((SameSide(_p,_a,_b,_c))&&(SameSide(_p,_b,_a,_c))&&(SameSide(_p,_c,_a,_b)))
		{
			return(true);
		}
		else
		{
			return(false);
		}
	};
	
	// Fast general ray triangle intersection test
	// _orig,_dir are the origin and direction of the ray
	// _p0,_p1,_p2 make up the triangle
	// t: Parameterized ray depth value if method returned true
	// u: Parameterized u value if method returned true
	// v: Parameterized v value if method returned true
	// Hit ocurred at (two alternative formulations)
	// _orig + _dir * t
	// _p0 + (_p1 - _p0) * u + (_p2 - _p0) * v
	bool inline TriRayTest(const CVec3Dfp32& _orig,
							const CVec3Dfp32& _dir,
							const CVec3Dfp32& _p0,
							const CVec3Dfp32& _p1,
							const CVec3Dfp32& _p2,
							fp32* t,fp32* u,fp32* v)
	{
		CVec3Dfp32 edge01,edge02,tvec,pvec,qvec;
		fp32 det,inv_det;

		// Calculate edge vectors from vert0
		edge02 = _p2 - _p0;
		edge01 = _p1 - _p0;

		// Begin calc determinant
		pvec = _dir / edge02;
		det = edge01 * pvec;
		
		// If determinant is near zero ray is parallell to triangle plane
		// Use fabs(det) if we don't want backface culling
		if (det < kEpsilon)
		{
			return(false);
		}

		// Calculate parameterized distance from vert0 to ray origin
		tvec = _orig - _p0;

		// Calc u and check its bounds
		*u = tvec * pvec;
		if ((*u < 0.0)||(*u > det))
		{
			return(false);
		}

		// Calc v param and check bounds
		qvec = tvec / edge01;
		*v = _dir * qvec;

		// Q: Why test against det instead of 1.0?
		// A: This way we may bail before doing the costly inv_det division
		if ((*v < 0.0)||((*u) + (*v) > det))
		{
			return(false);
		}

		// Return true here if we're not interested in the exact spot the ray hit
		
		// We now know that the ray hits the triangle, we scale the u,v,t params
		*t = edge02 * qvec;
		inv_det = 1.0 / det;
		*t *= inv_det;
		*u *= inv_det;
		*v *= inv_det;

		return(true);
	}
};




class CConstraintRigidObject : public CConstraintSystem
{
public:
	CConstraintRigidObject();
	virtual void Init(int _iObj,CWObject_CoreData* _pClient,CWorld_PhysState* _pWPhysState);
	// Uses the models bounding box to set up a rigid body system
	int16 SetupRigidSystem(CXR_Model *_pModel);

	CVec3Dfp32 m_CenterOffset;
	CXR_Model *m_pModel;

	bool m_bFirstImpactDone;  // Initially false, set to true after first impact
	int	m_iCollisionSoundNextTick;
	int m_iHardImpactSound;
	int m_iSoftImpactSound;

	virtual void StartCollecting(CWorld_PhysState* _pWPhysState, CXR_Model *_pModel, struct SConstraintSystemSettings* _pSettings);
	virtual void Setup(struct SConstraintSystemSettings* _pSettings,CXR_Model *_pModel);

	virtual void PackStopped();
	virtual void UnPackStopped();
	virtual void SetState(int _State);

	virtual void UpdateCoordinateSystem();
	virtual int16 GetCollBoxPos(CMat4Dfp32* _pPosMat);
	virtual void CalcMatrices(const CMat4Dfp32 &_Mat, TArray<CMat4Dfp32> &_lMat);
	virtual void CalcMatrices_r(bool _bAnimatedPose,
		int _iNode,
		const CMat4Dfp32 &_Mat,
		CXR_SkeletonInstance* _pSkelInstance,
		CXR_Skeleton* _pSkel,
		TArray<CMat4Dfp32> &_lMat);

	virtual int16 GetAnimatedFrame(bool _bVelocity);
	virtual int16 CollectFrame(bool _bVelocity, CMat4Dfp32& _WMat);

	virtual int16 Animate(int32 _GameTick,CMat4Dfp32& _WMat);
	// Plays the collision sound on this rigid body
	virtual bool PlayBodypartSound(int _BodyPart,const CVec3Dfp32& _Pos,fp32 _Impact);
	
	void GetPosition(CMat4Dfp32 &_Pos);

	void SetCollisionSound(int _iSound,bool _bStrong = true)
	{
		if (_bStrong)
		{
			m_iHardImpactSound = _iSound;
		}
		else
		{
			m_iSoftImpactSound = _iSound;
		}
	}

	void SetSingleCollisionSound(int _iSound)
	{
		m_iHardImpactSound = _iSound;
	}

protected:
	friend class CConstraintSystemClient;
	friend class CConstraintRule;
	friend class CConstraintSubPosRule;
	friend class CConstraintAngleRule;
	friend class CConstraintKneeRule;
	friend class CConstraintBodyPlaneRule;
	friend class CConstraintPlaneRule;
	friend class CConstraintDirAngleRule;
	friend class CConstraintDirAngleRule2;

};
typedef TPtr<CConstraintSystem> spCConstraintSystem;

// A box of 8 points and constraints that keep them sound
class CConstraintBox : public CReferenceCount
{
public:
	CConstraintBox();
	~CConstraintBox();
	int16 Init(CConstraintSystem* _pSystem);
	int16 Setup(CXR_Model *_pModel,CMat4Dfp32& _PosMat);
	int16 SetupCoordinateSystem(int16 _iUp0,int16 _iUp1,int16 _iLeft0,int16 _iLeft1);
	virtual int16 UpdateCoordinateSystem();
	// Set explosion parameters
	// _Origin: Origin of explosion
	// _Params[0]: Force
	// _Params[1]: Radius
	// _Params[2]: Randomness (between 0.0 and 1.0)
	int16 Explode(CVec3Dfp32 _Origin,CVec3Dfp32 _Params);

	CConstraintSystem*		m_pCS;				// System owning us
	CXR_Model*				m_pModel;

	int16					m_iiPts[8];			// Indices to our points
	int16					m_iSystemUp0;
	int16					m_iSystemUp1;
	int16					m_iSystemLeft0;
	int16					m_iSystemLeft1;
	CVec3Dfp32				m_CenterOffset;			// CG offest from m_iiPts[0] when unrotated
	CMat4Dfp32				m_SystemMat;			// Coordinate system (yes, pos and direction)

protected:

};

// A class for handling multiple simple boxes of flesh and goo flying around
class CConstraintGib: public CConstraintSystem
{
public:
	CConstraintGib();
	virtual ~CConstraintGib();

	virtual void Init(CWObject_Client* _pObj,CWorld_PhysState* _pWPhysState);
	virtual void Clear(bool _bAll = true);
	int16 GetCollBoxPos(CMat4Dfp32* _pPosMat);

	// Adds a box (an oriented model with 8 cornerpoints)
	int16 AddBox(CXR_Model* _pModel,CMat4Dfp32& _Pos);
	int16 GetBoxMatrix(int _iBox,CMat4Dfp32& _Pos);
	virtual int16 Apply(CMat4Dfp32& _WMat);
	int16 Animate(int32 _GameTick);
	// Set explosion parameters
	// _Origin: Origin of explosion
	// _Params[0]: Force
	// _Params[1]: Radius
	// _Params[2]: Randomness (between 0.0 and 1.0)
	int16 Explode(CVec3Dfp32 _Origin,CVec3Dfp32 _Params);

	TArray<CConstraintBox*>				m_lpBoxes;
};
typedef TPtr<CConstraintGib> spCConstraintGib;


#define kConstraintPointDefaultMass		1.0
#define kConstraintPointDefaultRadius	1.0
class CConstraintPoint
{
public:
	CConstraintPoint();

protected:
friend class CConstraintSystem;
friend class CConstraintBox;
friend class CConstraintRule;
friend class CConstraintSubPosRule;
friend class CConstraintAngleRule;
friend class CConstraintKneeRule;
friend class CConstraintBodyPlaneRule;
friend class CConstraintPlaneRule;
friend class CConstraintDirAngleRule;
friend class CConstraintDirAngleRule2;
friend class CConstraintCapsuleRule;
friend class CConstraintRigidObject;

	// SetPos magically moves the point to the new position with no added velocity
	void SetPos(const CVec3Dfp32& _pos);
	
	// GetPos returns the pos
	CVec3Dfp32 GetPos() const;

	// MovePos sets the point to a new pos with or without inertia
	void MovePos(const CVec3Dfp32& _pos,bool _bInertia = true);
	// Offsets the point by _Offset, with or without inertia
	void OffsetPos(const CVec3Dfp32& _Offset,bool _bInertia = true);

	// SetVel sets the velocity of the point without moving it
	void SetVel(const CVec3Dfp32& _vel);

	// Sets up the physical params
	// NOTE: _Mass <= 0 indicates infinite mass
	void Setup(fp32 _Mass,bool _bCollision,int _iBodypart);
	
	void OnDeltaLoad(CCFile* _pFile);
	void OnDeltaSave(CCFile* _pFile);

	// Change mass value, called by CConstraintSystem
	void ModifyMass(fp32 _Mass);

	// Updates the point
	void Move();

	// Adds _dv velocity to the point
	void ApplyVelocity(const CVec3Dfp32& _dv);
	
	// Adds the _di impulse (impulse is velocity * mass)
	void ApplyImpulse(const CVec3Dfp32& _di);

	bool					m_bCollision;	// Affected by collisions when true
	bool					m_bImpact;		// Affected by shots etc when true
	int16					m_iCollPoly;	// What poly index was collided (-1 means none), only valid current frame
	int16					m_iBonePt;		// Index to bone point if any (-1 indicates none)
	int16					m_iBodypart;	// WHat part if any does the pt belongs to
	CVec3Dfp32				m_Pos;
	CVec3Dfp32				m_PrevPos;		// This may not be last frames pos if subject to inertialess moves
	CVec3Dfp32				m_PrevFramePos;	// The actul world pos of the previous frame
	CVec3Dfp32				m_Vel;
	CVec3Dfp32				m_DeltaV;
	fp32						m_Mass;
	fp32						m_InvMass;
};


// Constants for the different rules we allow
// All rules with kConstraintRuleSoftMask set use the stiffness parameter
// All rules with kConstraintRuleRotMask set are parent rot rules
// Bit 0 tells us wether the rule is soft or not
// Bit 1 tells us wether the rule is equal or not
// Bit 2 tells us wether the rule is equal or not
// Bit 3 tells us wether the rule is equal or not
// NOTE: The bits are not totally consistent, it is possible to bitwise create a kConstraintRuleEqualSmallerLargerSoft
#define kConstraintRuleIgnore		0
#define kConstraintRuleEqual		2
#define kConstraintRuleEqualSoft	3
#define kConstraintRuleSmaller		4
#define kConstraintRuleSmallerSoft	5
#define kConstraintRuleLarger		8
#define kConstraintRuleLargerSoft	9

#define kConstraintRuleEqualMask	2
#define kConstraintRuleSmallerMask	4
#define kConstraintRuleLargerMask	8
#define kConstraintRuleSoftMask		1

// A rule that governs the length between point m_iPA and m_iPB
// The default length is taken from the value at setup so care must be taken that the initial
// values are good.
class CConstraintRule
{
public:
	CConstraintRule(CConstraintSystem* _pSystem);
	virtual ~CConstraintRule();

	int16 Setup(int16 _iPA,
				int16 _iPB,
				int16 _Rule,
				fp32 _Stiffness = kConstraintDefaultStiffness);
	
	// Adds _iChildBone to list of child bone indices
	// Child bones are moved and rotated realtive the parent bone, useful for skeleton bones that
	// doesn't exist in the CConstraintSystem ie facial bones, fingers etc
	// If _iChildBone was already added or was one of _iPA or _iPB inSetup kErrParams is returned
	// If the instance is not a bone (was not created by CConstraintSystem::AddBone()) kErrState is returned
	int16 AddChildBone(int16 _iChildBone);

	// Call after changing mass
	int16 SetRangeMultiplier(fp32 _RangeMultiplier);
	int16 SetRangeValue(fp32 _RangeValue);
	int16 ModifyMass();
	int16 ConserveMomentum(int16 _iPA,int16 _iPB,int16 _iPC,CVec3Dfp32 _FB,bool _bInertia = true);
	virtual int16 Constrain() const;
	virtual void Draw(fp32 _Duration,const CVec3Dfp32& _Offset);
	// Supply three points and the tentative move we want on _iPB and this method will
	// calculate forces on _iPB,iPC and apply all the forces.
	// The sum of all momenta (force*mass) will zero.
	// The sum of all rotational momenta will also be zero.
	CConstraintSystem*	m_pCS;

	int16				m_iPA;
	int16				m_iPB;
	int16				m_Rule;
	bool				m_bForce;
	
// #ifdef _DRAW_SYSTEM
	mutable bool	m_bDraw;		// Draw when true, 
	mutable CVec3Dfp32		m_DrawStart0;
	mutable CVec3Dfp32		m_DrawEnd0;
// #endif

	bool			m_bBone;		// When true the constraint represents an actual model bone
	TArray<int16>	m_liChildBones;	// List of indices to points that should be rotated with the bone
	fp32				m_MassFactorAB;	// dA = diff * m_MassFactorAB, dB its diff * (1.0 - m_MassFactorAB)
	fp32				m_Length;		// We keep both these values around as we don't want to divide
	fp32				m_InvLength;	// We keep both these values around as we don't want to divide
	fp32				m_Stiffness;	// From 0 to 1.0, ignored unless bit 0 of rule is set (ie soft rule)
};

// A rule that will keep m_iPt0 at _m_Fract between m_iPA and m_iPB
class CConstraintSubPosRule  : public CConstraintRule
{
public:
	CConstraintSubPosRule(CConstraintSystem* _pSystem);

	int16 Setup(int16 _iP0,
		int16 _iPA,
		int16 _iPB,
		fp32 _Stiffness,
		fp32 _Fract);

	virtual int16 Constrain() const;
	int16				m_iPt0;
	fp32					m_Fract;
};

// A rule that govern the angle between point m_iPt0->m_iPA and m_iPt0->m_iPB
// The rule assumes that the lengths of m_iPt0->m_iPA and m_iPt0->m_iPB are valid at setup
// Make shure to add CConstraintRule for these before this rule to ensure that they are
// in their proper place when this rule is evaluated
class CConstraintAngleRule : public CConstraintRule
{
public:
	CConstraintAngleRule(CConstraintSystem* _pSystem);

	int16 Setup(int16 _iP0,
				int16 _iPA,
				int16 _iPB,
				int16 _Rule,
				fp32 _Stiffness,
				fp32 _Angle);
	virtual int16 Constrain() const;
	int16				m_iPt0;
};

// Sets up a plane of the two vectors m_iPt0->m_iPt1 and m_iPt0->m_iPA
// If m_iPB above the plane then m_iPB and m_iPA will move to put them on the right side
// _bLeft should be true for left knees and right elbows and false for right knees and left elbows
class CConstraintKneeRule : public CConstraintRule
{
public:
	CConstraintKneeRule(CConstraintSystem* _pSystem);

	int16 Setup(int16 _iP0,
				int16 _iP1,
				int16 _iPA,
				int16 _iPB,
				bool _bLeft,
				fp32 _Stiffness = kConstraintDefaultStiffness);

	virtual int16 Constrain() const;

	virtual void Draw(fp32 _Duration,const CVec3Dfp32& _Offset);
	fp32					m_KneeFactor;	// Where along m_iPt0-m_iPB is the knee located
	bool				m_bLeft;
	int16				m_iPt0;
	int16				m_iPt1;

#ifdef _DRAW_SYSTEM
	mutable CVec3Dfp32		m_DrawStart1;
	mutable CVec3Dfp32		m_DrawEnd1;
#endif
};

// Sets up a plane of m_iPt0->m_iPt1 and m_iPt0->m_iPA
// If _iPB is below _Distance it will be moved _Stiffness of the distance
// _bLeft determine wether the plane should be reversed
class CConstraintBodyPlaneRule : public CConstraintRule
{
public:
	CConstraintBodyPlaneRule(CConstraintSystem* _pSystem);

	int16 Setup(int16 _iP0,
		int16 _iP1,
		int16 _iPA,
		int16 _iPB,
		bool _bLeft,
		fp32 _Distance,
		fp32 _Stiffness = kConstraintDefaultStiffness);

	virtual int16 Constrain() const;

	virtual void Draw(fp32 _Duration,const CVec3Dfp32& _Offset);

	fp32					m_Distance;
	bool				m_bLeft;
	int16				m_iPt0;
	int16				m_iPt1;
};

// Sets up a plane of _iP0->_iP1 and _iP0->_iP2 using _iP0 as point
// If _i0,_i1,_i2,_i3 lies below _Distance of the plane they will be pairwise moved as follows
// Each pointpair _iP0,_i0 etc will be separated according to mass to satisfy the constraint
class CConstraintPlaneRule : public CConstraintRule
{
public:
	CConstraintPlaneRule(CConstraintSystem* _pSystem);

	int16 Setup(int16 _iPt0,
		int16 _iPt1,
		int16 _iPt2,
		int16 _iPt3,
		int16 _i0,
		int16 _i1,
		int16 _i2,
		int16 _i3,
		fp32 _Distance,
		fp32 _Stiffness = kConstraintDefaultStiffness);


	virtual int16 ModifyMass();
	virtual int16 Constrain() const;

	virtual void Draw(fp32 _Duration,const CVec3Dfp32& _Offset);

	fp32 m_Distance;

	int16 m_iPt0;
	int16 m_iPt1;
	int16 m_iPt2;
	int16 m_iPt3;

	int16 m_i0;
	int16 m_i1;
	int16 m_i2;
	int16 m_i3;

	fp32 m_MassFactor0;
	fp32 m_MassFactor1;
	fp32 m_MassFactor2;
	fp32 m_MassFactor3;
};

// _iPtA and _iPtB make up a capsule (cylinder with rounded ends) of radius _Radius
// The points _iPt0 and _iPt1 will be constraint by this surface moving the points and _iPtA,_iPtB
class CConstraintCapsuleRule : public CConstraintRule
{
public:
	CConstraintCapsuleRule(CConstraintSystem* _pSystem);

	int16 Setup(int16 _iPA,int16 _iPB,int16 _iPt0,int16 _iPt1,
		fp32 _Radius,fp32 _Stiffness = kConstraintDefaultStiffness);

	virtual int16 ModifyMass();
	virtual int16 Constrain() const;

	virtual void Draw(fp32 _Duration,const CVec3Dfp32& _Offset);

	fp32 m_Radius;

	int16 m_iPt0;
	int16 m_iPt1;
};

// A rule that enforces iPA->iPB to be within an angle of left,right,up,down,back,front
class CConstraintDirAngleRule : public CConstraintRule
{
public:
	CConstraintDirAngleRule(CConstraintSystem* _pSystem);

	enum {
		FORWARD = 0,
		BACKWARD = 1,
		RIGHT = 2,
		LEFT = 3,
		UP = 4,
		DOWN = 5,
	};

	// Angle must be between 0 and 90 degrees, if you want more, use the reverse direction and use (180-angle)
	int16 Setup(int16 _iPA,
				int16 _iPB,
				fp32 _Stiffness,
				int _Direction,
				fp32 _Angle);

	virtual int16 Constrain() const;
	
	virtual void Draw(fp32 _Duration,const CVec3Dfp32& _Offset);

	fp32 m_Cos;
	fp32 m_Sin;
	int16 m_Dir;
};

// A rule that enforces iPA->iPB to be within an angle from _iPA and _iRef
// If _iRef2 is supplied a 'virtual' counter will be used at the midpoint between _iRef and _iRef
// actual displacement will then be split and applied to both _iRef and _iRef
class CConstraintDirAngleRule2 : public CConstraintRule
{
public:
	CConstraintDirAngleRule2(CConstraintSystem* _pSystem);

	// Angle must be between 0 and 90 degrees
	int16 Setup(int16 _iPA,
		int16 _iPB,
		fp32 _Stiffness,
		int16 _iRef,
		int16 _iRef2,
		fp32 _Angle);

	virtual int16 Constrain() const;

	virtual void Draw(fp32 _Duration,const CVec3Dfp32& _Offset);

	fp32 m_Cos;
	fp32 m_Sin;
	int16 m_iRef;
	int16 m_iRef2;	// If m__iRef2 > -1 means we use a 'virtual' counter midway between m_iRef and m_iRef2
};
#endif	// _CCONSTRAINTSYSTEM_
