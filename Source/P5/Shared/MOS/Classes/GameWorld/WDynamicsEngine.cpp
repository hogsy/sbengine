#define RIGHTMATORDER

/*
	Rolling and Spinning Friction
	http://www.cs.ubc.ca/~rbridson/docs/rbridson_phd.pdf
	s 53
 */

#ifndef DYNAMICS_USE_OPCODE
//#ifdef DYNAMICS_SKIP_OPCODE
#else
#define DNew(_Class) new
#endif

//#define CHECK_STATIONERY

#include "pch.h" 

//#define DYNAMICS_DISABLE_CONSTRAINTS
#define DYNAMICS_FREEZE_OBJECTS
#define DYNAMICS_CONSTRAINT_LINEAR_DAMPFACTOR (0.995)
#define DYNAMICS_CONSTRAINT_ANGULAR_DAMPFACTOR (0.99)
//#define DYNAMICS_CONSTRAINT_DAMPFACTOR (1.0)

#include <time.h>
#include "WDynamicsEngine.h"
#include "WDynamicsSupport.h"


//#pragma optimize( "", off )
//#pragma inline_depth(0)


#ifndef MAX
#define MAX(a,b) ((a > b) ? a : b)
#endif

#ifndef MIN
#define MIN(a,b) ((a < b) ? a : b)
#endif

#define IsValidFloat(f) ((f) == (f))
#define IsValidVec3(v) (IsValidFloat(v.k[0]) && IsValidFloat(v.k[1]) && IsValidFloat(v.k[2]))

//#define CHECK_VALID_VEC(v)  M_ASSERT(IsValidVec3(v), "Invalid vec!")
#define CHECK_VALID_VEC(v)

TArray<CContactInfo> gcollisioninfolist;
int gncollisions = 0;



	CVec3Dfp64 CInertia::Sphere (double mass, double r)
	{
		double x = (2.0 / 5.0) * mass * r * r;

		CVec3Dfp64 m;

		m.k[0]= x;
		m.k[1]= x;
		m.k[2]= x;
		return m;
	}

	/*
	TODO: Här är nog något fel!!!!
	eller inte längre...
	Är x och z vända???
	*/ 

	CVec3Dfp64 CInertia::Block(double mass, double a, double b, double c) 
	{
		double x = (1.0 / 12.0) * mass * (b*b + c*c);
		double y = (1.0 / 12.0) * mass * (a*a + c*c);
		double z = (1.0 / 12.0) * mass * (a*a + b*b);
		CVec3Dfp64 m;

		m.k[0]= x;
		m.k[1]= y;
		m.k[2]= z;

		return m;
	}
	// TODO: Hmm ta bort sen kanske...
	M_INLINE CVec3Dfp64 ElemMul(const CVec3Dfp64 &v1, const CVec3Dfp64& v2) {
		CVec3Dfp64 ret;
		ret.k[0]= v1.k[0] * v2.k[0];
		ret.k[1]= v1.k[1] * v2.k[1];
		ret.k[2]= v1.k[2] * v2.k[2];
		return ret;
	}

	M_INLINE CMat4Dfp64 MatMul(const CMat4Dfp64& m, const CVec3Dfp64& v) {
		CMat4Dfp64 ret;
		//		ret.Unit();
		ret.k[0][3]= 0;
		ret.k[1][3]= 0;
		ret.k[2][3]= 0;
		ret.k[3][3]= 1;

		ret.k[3][0]= 0;
		ret.k[3][1]= 0;
		ret.k[3][2]= 0;

		ret.k[0][0]= m.k[0][0]*v.k[0];
		ret.k[0][1]= m.k[0][1]*v.k[1];
		ret.k[0][2]= m.k[0][2]*v.k[2];

		ret.k[1][0]= m.k[1][0]*v.k[0];
		ret.k[1][1]= m.k[1][1]*v.k[1];
		ret.k[1][2]= m.k[1][2]*v.k[2];

		ret.k[2][0]= m.k[2][0]*v.k[0];
		ret.k[2][1]= m.k[2][1]*v.k[1];
		ret.k[2][2]= m.k[2][2]*v.k[2];


		// TODO: Ska [3][3] vara 1 (som den är nu)

		return ret;
	}

	M_INLINE CMat4Dfp64 MatMul(const CVec3Dfp64& v, const CMat4Dfp64& m) {
		CMat4Dfp64 ret;
		//ret.Unit();
		ret.k[0][3]= 0;
		ret.k[1][3]= 0;
		ret.k[2][3]= 0;
		ret.k[3][3]= 1;

		ret.k[3][0]= 0;
		ret.k[3][1]= 0;
		ret.k[3][2]= 0;


		ret.k[0][0]= m.k[0][0]*v.k[0];
		ret.k[0][1]= m.k[0][1]*v.k[0];
		ret.k[0][2]= m.k[0][2]*v.k[0];

		ret.k[1][0]= m.k[1][0]*v.k[1];
		ret.k[1][1]= m.k[1][1]*v.k[1];
		ret.k[1][2]= m.k[1][2]*v.k[1];

		ret.k[2][0]= m.k[2][0]*v.k[2];
		ret.k[2][1]= m.k[2][1]*v.k[2];
		ret.k[2][2]= m.k[2][2]*v.k[2];



		// TODO: Ska [3][3] vara 1 (som den är nu)
		return ret;
	}

	CContactInfo::CContactInfo() {
		m_isColliding= false;
		m_distance= numeric_limits<double>::quiet_NaN();
		m_relativevelocity= numeric_limits<double>::quiet_NaN();
		m_PointOfCollision= CVec3Dfp64(numeric_limits<double>::quiet_NaN(),
			numeric_limits<double>::quiet_NaN(),
			numeric_limits<double>::quiet_NaN());
		m_Normal= CVec3Dfp64(numeric_limits<double>::quiet_NaN(),
			numeric_limits<double>::quiet_NaN(),
			numeric_limits<double>::quiet_NaN());
		m_pRigidBody1= NULL;
		m_pRigidBody2= NULL;
		m_UserData1 = 0;
		m_UserData2 = 0;
	}

	/*
	CStr CContactInfo::GetString() {
	CStr ret;
	ret.CaptureFormated("g1=%X, g2=%X, relvel=%f, distance=%f",
	m_rigidbody1,m_rigidbody2,
	m_relativevelocity,m_distance);
	ret+=", p="+m_PointOfCollision.GetString();
	ret+=", normal="+m_Normal.GetString();
	ret+=", ra="+m_ra.GetString();
	ret+=", rb="+m_rb.GetString();
	return ret;
	}*/

	// TODO: Kan kanske skulle kunna uppdatera m_relatvevel. istället?
	// Eller räkna ut flera med tex altivec

	/*
	Är inte detta masscentrums relativa hastighet och inte kollisionspunkterna!?!?!?
	*/
	fp64 CContactInfo::GetRelativeVelocity() const {
#if 0
		return m_relativevelocity;
#else

#if 1
		CRigidBody *rb1 = m_pRigidBody1->GetRigidBody();
		CRigidBody *rb2 = m_pRigidBody2->GetRigidBody();
		CVec3Dfp64 v1,v2;
		rb1->GetAngularVelocity().CrossProd(m_ra,v1);
		v1+=rb1->GetVelocity();

		rb2->GetAngularVelocity().CrossProd(m_rb,v2);
		v2+=rb2->GetVelocity();

		return m_Normal*(v2-v1);
#else
		return m_Normal*(m_pRigidBody2->GetRigidBody()->GetVelocity() - m_pRigidBody1->GetRigidBody()->GetVelocity());
#endif
#endif
	}

	CRigidBody::CRigidBody(CWorld *world) 
	{
		Init(world);
	}

	void CRigidBody::Init(CWorld *_pWorld)
	{
		//		m_active= true;
		m_stationary= false;
		m_stationary_candidate_for= 0;
		m_stationary_movement= CVec3Dfp64(0,0,0);
		m_avg_velocity= CVec3Dfp64(0,0,0);
		m_avg_angularvelocity= CVec3Dfp64(0,0,0);
		//m_id= world->GetRigidBodyID();
		m_id = -1;
		m_world_= _pWorld;
		//m_mass= 1;
		m_userdata = NULL;

		m_processed= false;
		m_PartOfCompound = false;

		m_childObject = false;

		m_pstate = &m_state;
		m_psavedstate = &m_savedstate;

		m_FreezeCounter = 0;

		/*		m_collisiongridobjects[0].m_rigidbody= this;
		m_collisiongridobjects[1].m_rigidbody= this;
		m_collisiongridobjects[2].m_rigidbody= this;
		m_collisiongridobjects[3].m_rigidbody= this;
		m_collisiongridobjects[4].m_rigidbody= this;
		m_collisiongridobjects[5].m_rigidbody= this;
		m_collisiongridobjects[6].m_rigidbody= this;
		m_collisiongridobjects[7].m_rigidbody= this;
		*/

		MRIGIDBODY_BODYSTATE.m_active= true;
		//		m_world->m_bodystate[m_id].m_active= true;
		//m_dirtyworldinertiatensor= true;
		// TODO: Budget
		SetInertiaTensor(CInertia::Sphere(1,1));
		m_MassCenter = 0;

		// Set invalid box.
		m_BoundingBox.m_Min = CVec3Dfp64(_FP64_MAX);
		m_BoundingBox.m_Max = CVec3Dfp64(-_FP64_MAX);

		m_DynamicsFriction = 0.3;
		m_StaticFriction = 0.4;

//		m_DynamicsFriction = 0.7;
//		m_StaticFriction = 0.9;

		for (int i = 0; i < DYNAMICS_MAX_CONNECT_COUNT; i++)
		{
			m_pConnectedTo[i] = NULL;
		}

		m_ExternalForce = CVec3Dfp64(0.0f);
		m_ExternalTorque= CVec3Dfp64(0.0f);
	}


	void *CRigidBody::GetUserData() {
		return m_userdata;
	}

	void CRigidBody::SetUserData(void *userdata) {
		m_userdata = userdata;
	}

	/*
	int CRigidBody::GetRigidBodyType() const {
	M_ASSERT(m_GeometryType != -1,"Invalid rigidbody type!");
	return m_GeometryType;
	}
	*/
	/*
	void CRigidBody::GetTransform(CMat4Dfp64& transform) const {
	// TODO: Onödigt?!?!?!?
	MRIGIDBODY_BODYSTATE.m_orientation.Normalize();
	CMat4Dfp64 rot;
	MRIGIDBODY_BODYSTATE.m_orientation.CreateMatrix(rot);
	CMat4Dfp64 trans;
	trans.Unit();


	#ifdef RIGHTMATORDER
	CVec3Dfp64::GetRow(trans,3) = MRIGIDBODY_BODYSTATE.m_position;
	#else
	trans.k[0][3]= MRIGIDBODY_BODYSTATE.m_position.k[0];
	trans.k[1][3]= MRIGIDBODY_BODYSTATE.m_position.k[1];
	trans.k[2][3]= MRIGIDBODY_BODYSTATE.m_position.k[2];
	#endif

	#ifdef RIGHTMATORDER
	rot.Multiply(trans,transform);
	#else
	trans.Multiply(rot,transform);
	#endif
	}
	*/
	void CRigidBody::AddAngularMomentum(const CVec3Dfp64& dangularmomentum) {
		//if (dangularmomentum.Length() > 0.01f) 
		//	SetStationary(false);

		//		if (m_active && !m_stationary)
		if (IsActive())
			//m_state.m_delta_angularmomentum+= dangularmomentum;
			MRIGIDBODY_BODYSTATE.m_angularvelocity+= dangularmomentum * (1.0/MRIGIDBODY_BODYSTATE.m_mass);
		//m_world->m_bodystate[m_id].m_angularvelocity+= dangularmomentum * (1.0/m_world->m_bodystate[m_id].m_mass);
		//m_state.m_angularvelocity+= dangularmomentum * (1.0/m_state.m_mass);
		//		m_state.m_angularvelocity+= dangularmomentum * (1.0/m_state.m_mass);

		//		CMat4Dfp64 t;
		//		GetWorldInertiaTensorInvert(t);
		//Matrix3d t = getWorldIntertiaTensorInvert ();

		// TODO: Ska det göras här???
		//		m_state.m_angularvelocity=  m_state.m_angularmomentum * t;
		//		t.transform (angularVelocity);
	}

	void CRigidBody::AddMomentum(fp64 dmx, fp64 dmy, fp64 dmz) {
		AddMomentum(CVec3Dfp64(dmx,dmy,dmz));
	}

	void CRigidBody::AddMomentum(const CVec3Dfp64& dmomentum) {
		//if (m_active && !m_stationary)
		if (IsActive())
			MRIGIDBODY_BODYSTATE.m_velocity+= dmomentum * (1.0 / MRIGIDBODY_BODYSTATE.m_mass);
	}

	void CRigidBody::AddForce(const CVec3Dfp64& dforce) {
		if (IsActive())
			MRIGIDBODY_BODYSTATE.m_force+=dforce;
	}

	void CRigidBody::AddForce(fp64 dx, fp64 dy, fp64 dz) {
		AddForce(CVec3Dfp64(dx,dy,dz));
	}

	void CRigidBody::AddTorque(const CVec3Dfp64& dtorque) {
		if (IsActive())
			MRIGIDBODY_BODYSTATE.m_torque+= dtorque;
	}

	void CRigidBody::AddTorque(fp64 dx, fp64 dy, fp64 dz) {
		AddTorque(CVec3Dfp64(dx,dy,dz));
	}

	// TODO: Ska denna heta något annat?
	void CRigidBody::ClearForces() {
		MRIGIDBODY_BODYSTATE.m_force= CVec3Dfp64(0,0,0);
		MRIGIDBODY_BODYSTATE.m_torque= CVec3Dfp64(0,0,0);
	}

	void CRigidBody::Stop() {
		//		m_state.m_angularmomentum= CVec3Dfp64(0,0,0);
		//		m_state.m_momentum= CVec3Dfp64(0,0,0);
	}

	void CRigidBody::SetMass(fp64 mass) {
		MRIGIDBODY_BODYSTATE.m_mass= mass;
	}

	fp64 CRigidBody::GetMass() const {
		return MRIGIDBODY_BODYSTATE.m_mass;
	}

	bool CRigidBody::IsActive() const {
		return MRIGIDBODY_BODYSTATE.m_active;
	}

	bool CRigidBody::IsStationary() const {
		return m_stationary;
	}

	void CRigidBody::SetStationary(bool stationary) {
		//if (!stationary)
		//	m_stationary_candidate_for= 0;		

		if (!stationary)
		{
			m_FreezeCounter = 0;
		}

		if (m_stationary && !stationary) {
			//m_stationary_candidate_for= 0;
		}
		m_stationary= stationary;
		MRIGIDBODY_BODYSTATE.m_Stationary = stationary;

		/*		if (stationary) {
		MRIGIDBODY_BODYSTATE.m_velocity= CVec3Dfp64(0,0,0);
		MRIGIDBODY_BODYSTATE.m_angularvelocity= CVec3Dfp64(0,0,0);
		}*/

		if (!m_stationary && stationary) {
			MRIGIDBODY_BODYSTATE.m_velocity= CVec3Dfp64(0,0,0);
			MRIGIDBODY_BODYSTATE.m_angularvelocity= CVec3Dfp64(0,0,0);
		}
	}

#define STAT_VEL_CONST (0.01f)
#define STAT_ANG_VEL_CONST (0.01f)

	bool CRigidBody::StationaryCandidate() {
		fp64 velmag = GetVelocity().Length();
		fp64 angmag = GetAngularVelocity().Length();

		//fp64 velmag= (m_avg_velocity*(1.0/m_stationary_candidate_for)).Length();
		//fp64 angmag= (m_avg_angularvelocity*(1.0/m_stationary_candidate_for)).Length();

		bool isstat = velmag < STAT_VEL_CONST && angmag < STAT_ANG_VEL_CONST;

		if (!isstat) {
			//M_TRACEALWAYS("v=%f\n",velmag);
			//M_TRACEALWAYS("a=%f\n",angmag);
		}

		return isstat;
	}

	void CRigidBody::CheckStationary() {
		if (!IsActive()) return;

		if (!StationaryCandidate()) {
			m_stationary_candidate_for = 0;
			return;
		}
		else {
			if (m_stationary_candidate_for < 0.0001f) {
				m_avg_velocity = GetPosition();
			}
			m_stationary_candidate_for++;

			if (m_stationary_candidate_for > 100) {
				if ((GetPosition() - m_avg_velocity).Length() < 0.1) {
					SetStationary(true);
					SetVelocity(0,0,0);
					SetAngularVelocity(0,0,0);
				}
				else {
					m_stationary_candidate_for = 0;
				}
			}
		}
		/*
		m_stationary_candidate_for+= 1;
		m_avg_velocity+= GetVelocity();
		m_avg_angularvelocity+= GetAngularVelocity();

		if (m_stationary_candidate_for > 20) {			
		fp64 velmag= (m_avg_velocity*(1.0f/m_stationary_candidate_for)).Length();
		fp64 angmag= (m_avg_angularvelocity*(1.0f/m_stationary_candidate_for)).Length();

		if (velmag < STAT_VEL_CONST*0.01f*2 && angmag < STAT_ANG_VEL_CONST*0.01f*2) {
		SetStationary(true);
		}
		else if (velmag > STAT_VEL_CONST*0.1f*2 || angmag > STAT_ANG_VEL_CONST*0.1f*2) {
		//			else if (velmag > STAT_VEL_CONST*0.1f) {
		SetStationary(false);
		}
		m_stationary_candidate_for= 0;
		m_avg_velocity= CVec3Dfp64(0,0,0);
		m_avg_angularvelocity= CVec3Dfp64(0,0,0);
		}*/
	}

	void CRigidBody::SetActive(bool active) {
		MRIGIDBODY_BODYSTATE.m_active= active;
	}

	void CRigidBody::SetInertiaTensor(const CVec3Dfp64& tensor) {
		m_tensor= tensor;
		m_tensorInverted.k[0]= 1.0f / tensor.k[0];
		m_tensorInverted.k[1]= 1.0f / tensor.k[1];
		m_tensorInverted.k[2]= 1.0f / tensor.k[2];

		UpdateWorldInertiaTensorInvert();
	}

	void CRigidBody::UpdatePosition(fp64 dt) {
		//		if (m_active && !m_stationary || force)
		//		if (m_active && !m_stationary)
		if (IsActive())
			MRIGIDBODY_BODYSTATE.m_position+= (MRIGIDBODY_BODYSTATE.m_velocity)*dt;
	}

	void CRigidBody::UpdateRotation(fp64 dt) {
		//		if (m_active && !m_stationary || force) {
		//		if (m_active && !m_stationary) {
		if (IsActive()) {
			CQuatfp64 v;
			CVec3Dfp64 tmpangvel= GetAngularVelocity();
			v.k[0]= tmpangvel.k[0];
			v.k[1]= tmpangvel.k[1];
			v.k[2]= tmpangvel.k[2];
			v.k[3]= 0;

			CQuatfp64 drot= v*GetOrientation();
			// TODO: Fixa detta i TQuat?
			drot.k[0]*= 0.5*dt;
			drot.k[1]*= 0.5*dt;
			drot.k[2]*= 0.5*dt;
			drot.k[3]*= 0.5*dt;
			// TODO: Fixa detta i TQuat?
			// Duh, inte effektivt. Spelar ingen roll om det sker i CWorld iofs...
			MRIGIDBODY_BODYSTATE.m_orientation.k[0]+= drot.k[0];
			MRIGIDBODY_BODYSTATE.m_orientation.k[1]+= drot.k[1];
			MRIGIDBODY_BODYSTATE.m_orientation.k[2]+= drot.k[2];
			MRIGIDBODY_BODYSTATE.m_orientation.k[3]+= drot.k[3];
			MRIGIDBODY_BODYSTATE.m_orientation.Normalize();
		}
	}

	void CRigidBody::UpdateVelocity(fp64 dt) {
		//if (!m_active || m_stationary) return;

		if (IsActive()) {
			MRIGIDBODY_BODYSTATE.m_velocity+= MRIGIDBODY_BODYSTATE.m_force*(dt * (1.0/MRIGIDBODY_BODYSTATE.m_mass));
			CMat4Dfp64 t;
			GetWorldInertiaTensorInvert(t);
			MRIGIDBODY_BODYSTATE.m_angularvelocity+= (MRIGIDBODY_BODYSTATE.m_torque*dt)*t;
		}
	}

	void CRigidBody::UpdateState() {
		//		if (!m_active || m_stationary) return;
		if (!IsActive()) return;

		//		m_state.m_velocity= m_state.m_momentum;
		//		m_state.m_velocity*=1.0/m_state.m_mass;

		/*		CMat4Dfp64 t;
		GetWorldInertiaTensorInvert(t);
		m_state.m_angularvelocity=  m_state.m_angularmomentum * t;
		*/

		//cout << "m: " << m_state.m_momentum.GetString().Str() << endl;
		//cout << "am: " << m_state.m_angularmomentum.GetString().Str() << endl;

		/*
		if (m_state.m_momentum.Length() < 0.001f
		&& m_state.m_angularmomentum.Length() < 0.001f) {
		m_stationary= true;
		m_state.m_momentum= CVec3Dfp64(0,0,0);
		m_state.m_angularmomentum= CVec3Dfp64(0,0,0);
		}*/

	}

	void CRigidBody::SaveFullState() {
		MRIGIDBODY_SAVEDBODYSTATE.m_position= MRIGIDBODY_BODYSTATE.m_position;
		MRIGIDBODY_SAVEDBODYSTATE.m_orientation= MRIGIDBODY_BODYSTATE.m_orientation;
		MRIGIDBODY_SAVEDBODYSTATE.m_velocity= MRIGIDBODY_BODYSTATE.m_velocity;
		MRIGIDBODY_SAVEDBODYSTATE.m_angularvelocity= MRIGIDBODY_BODYSTATE.m_angularvelocity;
	}

	void CRigidBody::RestoreVelocityState() {
		MRIGIDBODY_BODYSTATE.m_velocity= MRIGIDBODY_SAVEDBODYSTATE.m_velocity;
		MRIGIDBODY_BODYSTATE.m_angularvelocity= MRIGIDBODY_SAVEDBODYSTATE.m_angularvelocity;
	}

	void CRigidBody::RestorePositionOrientationState() {
		MRIGIDBODY_BODYSTATE.m_position= MRIGIDBODY_SAVEDBODYSTATE.m_position;
		MRIGIDBODY_BODYSTATE.m_orientation= MRIGIDBODY_SAVEDBODYSTATE.m_orientation;
	}

	void CRigidBody::GetVelocityAt(const CVec3Dfp64& point, /* out */ CVec3Dfp64& vel ) const {
		GetAngularVelocity().CrossProd(point - GetPosition(), vel);
		vel+= GetVelocity();
	}

	CVec3Dfp64 CRigidBody::GetVelocityAt(const CVec3Dfp64& point) const {
		CVec3Dfp64 tmp;
		GetVelocityAt(point,tmp);
		return tmp;
	}

	const CMat4Dfp64 &CRigidBody::GetWorldInertiaTensorInvert() {
		return m_worldinertiatensor;
	}

	void CRigidBody::GetWorldInertiaTensorInvert(CMat4Dfp64& tensorinverted) {
		tensorinverted= m_worldinertiatensor;
	}

	void CRigidBody::UpdateWorldInertiaTensorInvert() {
		CMat4Dfp64 rot;
		rot= MRIGIDBODY_BODYSTATE.m_matrix_orientation;

		// TEEEEEEEEEEEEEEEMP
		//rot.Transpose3x3();

		CMat4Dfp64 tmp;
		tmp= MatMul(rot,m_tensorInverted);
		rot.Transpose();
		tmp.Multiply(rot,m_worldinertiatensor);

		m_pstate->m_WorldInertiaTensorInverted = m_worldinertiatensor; // TODO: Is to be removed after pending reorg.
	}

	// TODO: Flytta denna metod till rätt plats
	void CWorld::DetectCollisions2(int *startindex, TArray<CContactInfo>& collisioninfo, void *_pArgument1, void *_pArgument2)
	{
#ifndef DYNAMICS_DISABLE_SCOPES
		MSCOPESHORT(CWorld::DetectCollisions);
		M_ASSERT(m_pWorldCollider != NULL, "");
#endif

		CContactInfo *pCollisionInfo = collisioninfo.GetBasePtr();
		int CollisionInfoLen = collisioninfo.Len();
		int nCollisions = m_pWorldCollider->Collide(this, m_rigidbodylist, pCollisionInfo, CollisionInfoLen, _pArgument1, _pArgument2);

//		CRigidBodyState *pstate= m_bodystate.GetBasePtr();

		CVec3Dfp64 v1;
		CVec3Dfp64 v2;
		//		CRigidBodyState *rigid1state= &pstate[rigid1->m_id];
		//		CRigidBodyState *rigid2state= &pstate[rigid2->m_id];

		for (int k=0; k<nCollisions; k++) {
			CContactInfo &collisioninfo= pCollisionInfo[k];

			if (collisioninfo.m_pRigidBody1 == NULL)
			{
				collisioninfo.m_pRigidBody1 = &m_DummyRigidBody;
			}

			if (collisioninfo.m_pRigidBody2 == NULL)
			{
				collisioninfo.m_pRigidBody2 = &m_DummyRigidBody;
			}

			CRigidBody *rigidbody1 = collisioninfo.m_pRigidBody1;
			CRigidBody *rigidbody2 = collisioninfo.m_pRigidBody2;

			CRigidBodyState *rigid1state= rigidbody1->GetBodyState();
			CRigidBodyState *rigid2state= rigidbody2->GetBodyState();

//			CRigidBodyState *rigid1state= &pstate[rigidbody1->m_id];
//			CRigidBodyState *rigid2state= &pstate[rigidbody2->m_id];

			CVec3Dfp64 p= collisioninfo.m_PointOfCollision;

			/*			M_TRACEALWAYS("pos = %s\n", collisioninfo.m_PointOfCollision.GetString().Str());
			M_TRACEALWAYS("normal = %s\n", collisioninfo.m_Normal.GetString().Str());
			M_TRACEALWAYS("distance = %f\n",collisioninfo.m_distance);
			*/

			rigid1state->m_angularvelocity.CrossProd(p-rigid1state->m_position,v1);
			v1+= rigid1state->m_velocity;

			rigid2state->m_angularvelocity.CrossProd(p-rigid2state->m_position,v2);
			v2+= rigid2state->m_velocity;

			collisioninfo.m_relativevelocity= collisioninfo.m_Normal*(v2-v1);
			//collisioninfo.m_relativevelocity *= m_ScaleFactor*m_ScaleFactor*m_ScaleFactor*m_ScaleFactor;

			/*			collisioninfo.m_relativetangentvelocity= 
			(v2-v1) - collisioninfo.m_Normal * ((v2-v1)*collisioninfo.m_Normal);
			*/
			collisioninfo.m_ra= p - rigid1state->m_position;
			collisioninfo.m_rb= p - rigid2state->m_position;

			// TODO: Detta verkar inte funka så bra...

			collisioninfo.m_ra *= m_ScaleFactor;
			collisioninfo.m_rb *= m_ScaleFactor;
			collisioninfo.m_PointOfCollision *= m_ScaleFactor;

			collisioninfo.m_MaxAppliedImpulse = 0.0;

			/*
			Testar med att använda alla interfererande då en som är ogiltig
			för v kan vara giltig för v' och vice versa.
			*/
			//			if (collisioninfo.m_isColliding && collisioninfo.m_relativevelocity > 0) {
			if (collisioninfo.m_isColliding) 
			{
				//collisioninfo.m_pRigidBody1= rigidbody1;
				//collisioninfo.m_pRigidBody2= rigidbody2;
				/*
				TODO: Är det inte bättre att göra detta för alla på en gång...
				samt ovan
				*/
				//collisioninfo.m_collision_impulse_denominator= CalculateImpulseDenominator(collisioninfo);

//				if (rigidbody1 == NULL || rigidbody2 == NULL)
//				{
//					int foobar = 123;
//				}

				rigidbody1->AddCollisionInfo(*startindex);
				rigidbody2->AddCollisionInfo(*startindex);

				// DEBUGTEST
				//rigidbody1->AddCollisionInfo(rigidbody1->m_pRigidBody->m_id);
				//rigidbody2->AddCollisionInfo(rigidbody2->m_pRigidBody->m_id);

//				int foo= *startindex;
				//				collisioninfolist[*startindex]= collisioninfo;
				//				cout << *startindex << endl;
				(*startindex)++;

				//collisioninfolist.Add(collisioninfo);
				//				startindex++;
				//				HandleCollision(collisioninfo, 0);
			}
		}
		//(*startindex) += nCollisions;
	}

	CRigidBodyGroup::CRigidBodyGroup(const char *name) {
		m_name= name;
		m_internalCollisions= false;
	}

	void CRigidBodyGroup::AddRigidBody(CRigidBody *rigidbody) {
		m_rigidbodylist.Add(rigidbody);		
	}

	bool CRigidBodyGroup::IsInternalCollision() {
		return m_internalCollisions;
	}

	void CRigidBodyGroup::SetInternalCollision(bool internalcollision) {
		m_internalCollisions= internalcollision;
	}

	TArray<CRigidBody *>& CRigidBodyGroup::GetRigidBodyList() {
		return m_rigidbodylist;
	}

	CConstraint::CConstraint() {
		m_userdata = NULL;
		m_Type = -1;
	}

	void CConstraint::SetUserData(void *userdata) {
		m_userdata = userdata;
	}

	void *CConstraint::GetUserData() {
		return m_userdata;
	}

	void CConstraint::Render()
	{

	}

	CSpringConstraint::CSpringConstraint(CRigidBody *rigidbody1, CRigidBody *rigidbody2, 
		fp64 equilibriumlen, 
		fp64 k,
		fp64 damp) : CConstraint()
	{
		m_rigidbody1= rigidbody1;
		m_rigidbody2= rigidbody2;
		m_equilibrium= equilibriumlen;
		m_k= k;
		m_damp= damp;
	}

	int CSpringConstraint::Apply(fp64 dt) 
	{
		CRigidBody *rigid1= m_rigidbody1->GetRigidBody();
		CRigidBody *rigid2= m_rigidbody2->GetRigidBody();

		CVec3Dfp64 p1= rigid1->GetPosition();
		CVec3Dfp64 p2= rigid2->GetPosition();
		CVec3Dfp64 d= p2-p1;

#if 1
		// Gå sönder...
		if (d.Length() > 1.8) {
			//		if (d.Length() > 2.5) {
			m_k=0;
			m_damp= 0;
		}
#endif

		CVec3Dfp64 v1= rigid1->GetVelocity();
		CVec3Dfp64 v2= rigid2->GetVelocity();
		CVec3Dfp64 dv= v2-v1;

		//Dterm = (DotProduct(&deltaV,&deltaP) * spring->Kd) / dist; // Damping Term

		//		(d/d.Length())*(d.Length() - )


		fp64 F= m_k*(d.Length()-m_equilibrium);
		d.Normalize();
		//		fp64 damp= (dv*d)*m_damp/d.LengthSqr();
		fp64 damp= (dv*d)*m_damp;

		//cout << F << ", " << damp << endl;
		//cout << dv.GetString().Str() << endl;

		rigid1->AddMomentum(d*(dt*(F+damp)/rigid1->GetMass()));
		rigid2->AddMomentum(d*(-dt*(F+damp)/rigid2->GetMass()));
 
		return 0;
	}

	CConstantDistanceConstraint::CConstantDistanceConstraint(CRigidBody *rigidbody1, CRigidBody *rigidbody2, fp64 distance) {
		m_rigidbody1= rigidbody1;
		m_rigidbody2= rigidbody2;
		m_distance= distance;
	}

	int CConstantDistanceConstraint::Apply(fp64 dt) {
		/*
		CRigidBody *rigid1= m_rigidbody1->GetRigidBody();
		CRigidBody *rigid2= m_rigidbody2->GetRigidBody();

		CVec3Dfp64 p1= rigid1->GetPosition();
		CVec3Dfp64 p2= rigid2->GetPosition();
		CVec3Dfp64 pd= p2-p1;		
		fp64 diff= pd.Length() - m_distance;
		fp64 factor= 0.98f;
		pd.Normalize();
		pd*= 0.5f*diff*factor;

		int nactive= 0;
		if (rigid1->m_world->m_bodystate[rigid1->m_id].m_active)
		nactive++;

		if (rigid2->m_world->m_bodystate[rigid2->m_id].m_active)
		nactive++;

		if (nactive==0) 
		pd*= 0;
		else if (nactive==1)
		pd*= 2;

		p1+= pd;
		p2-= pd;

		if (rigid1->m_world->m_bodystate[rigid1->m_id].m_active)
		rigid1->SetPosition(p1);
		if (rigid2->m_world->m_bodystate[rigid2->m_id].m_active)
		rigid2->SetPosition(p2);
		*/
		return 0;
	}

	CBallJointConstraint::CBallJointConstraint(CRigidBody *rigidbody1, CRigidBody *rigidbody2)
	{
		m_rigidbody1 = rigidbody1;
		m_rigidbody2 = rigidbody2;
	}


//#define BALLCONSTRAINT_TRACE

	int CBallJointConstraint::Apply(fp64 dt)
	{
		CMat4Dfp64 rot1 = m_rigidbody1->GetOrientationMatrix();
		CMat4Dfp64 rot2 = m_rigidbody2->GetOrientationMatrix();

		rot1.Transpose3x3();
		rot2.Transpose3x3();

		CMat4Dfp64 I1inv, I2inv;
		m_rigidbody1->GetWorldInertiaTensorInvert(I1inv);
		m_rigidbody2->GetWorldInertiaTensorInvert(I2inv);


		fp64 m1 = m_rigidbody1->GetMass();
		fp64 m2 = m_rigidbody2->GetMass();
		fp64 m1inv = 1.0 / m1;
		fp64 m2inv = 1.0 / m2;

		TMatrix<fp64> V1(12,1);
		CVec3Dfp64 v1 = m_rigidbody1->GetVelocity();
		CVec3Dfp64 v2 = m_rigidbody2->GetVelocity();
		CVec3Dfp64 w1 = m_rigidbody1->GetAngularVelocity();
		CVec3Dfp64 w2 = m_rigidbody2->GetAngularVelocity();

		V1[0][0] = v1[0];
		V1[1][0] = v1[1];
		V1[2][0] = v1[2];

		V1[3][0] = w1[0];
		V1[4][0] = w1[1];
		V1[5][0] = w1[2];

 		V1[6][0] = v2[0];
		V1[7][0] = v2[1];
		V1[8][0] = v2[2];

		V1[9][0] = w2[0];
		V1[10][0] = w2[1];
		V1[11][0] = w2[2];

		TMatrix<fp64> Fext(12,1);

//		fp64 grav = -9.8;
		fp64 grav = 0;

		Fext[0][0] = 0;
		Fext[1][0] = grav;
		Fext[2][0] = 0;

		Fext[3][0] = 0;
		Fext[4][0] = grav;
		Fext[5][0] = 0;

		Fext[6][0] = 0;
		Fext[7][0] = grav;
		Fext[8][0] = 0;

		Fext[9][0] = 0;
		Fext[10][0] = grav;
		Fext[11][0] = 0;

		TMatrix<fp64> Jball(3,12);
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 12; j++)
			{
				Jball[i][j] = 0.0;
			}
		}

		// J1
		Jball[0][0] = 1.0;
		Jball[1][1] = 1.0;
		Jball[2][2] = 1.0;

		// J3
		Jball[0][0+6] = -1.0;
		Jball[1][1+6] = -1.0;
		Jball[2][2+6] = -1.0;

		CVec3Dfp64 J2vec = -((CVec3Dfp64(4,0,0)*rot1));
		CVec3Dfp64 J4vec = ((CVec3Dfp64(-4,0,0)*rot2));
//		CVec3Dfp64 J2vec = CVec3Dfp64(5,0,0);
//		CVec3Dfp64 J4vec = CVec3Dfp64(-5,0,0);

//TEST
		/*
		
		CVec3Dfp64 dist = (m_rigidbody2->GetPosition() - m_rigidbody1->GetPosition());
		dist = CVec3Dfp64(0,0,0);
		Jball[0][0] = -dist[0];
		Jball[1][1] = -dist[1];
		Jball[2][2] = -dist[2];

		Jball[0][0+6] = dist[0];
		Jball[1][1+6] = dist[1];
		Jball[2][2+6] = dist[2];


		Jball[0][0+3] = 1;
		Jball[1][1+3] = 1;
		Jball[2][2+3] = 1;

		Jball[0][0+9] = -1;
		Jball[1][1+9] = -1;
		Jball[2][2+9] = -1;

		J2vec = -((dist * rot1));
		J4vec = ((dist * rot2));
		*/

//TEST

#if 1

		Jball[0][0+3] = 0.0;
		Jball[1][0+3] = J2vec[2];
		Jball[2][0+3] = -J2vec[1];

		Jball[0][0+4] = -J2vec[2];
		Jball[1][0+4] = 0.0;
		Jball[2][0+4] = J2vec[0];

		Jball[0][0+5] = J2vec[1];
		Jball[1][0+5] = -J2vec[0];
		Jball[2][0+5] = 0.0;

		Jball[0][0+9] = 0.0;
		Jball[1][0+9] = J4vec[2];
		Jball[2][0+9] = -J4vec[1];

		Jball[0][0+10] = -J4vec[2];
		Jball[1][0+10] = 0.0;
		Jball[2][0+10] = J4vec[0];

		Jball[0][0+11] = J4vec[1];
		Jball[1][0+11] = -J4vec[0];
		Jball[2][0+11] = 0.0;

#endif

		CVec3Dfp64 tmp = m_rigidbody2->GetPosition() - m_rigidbody1->GetPosition();
		tmp.Normalize();
		CVec3Dfp64 t1(1,0,0);
		CVec3Dfp64 t2(0,1,0);

		tmp = CVec3Dfp64(0,0,1) * rot1;
		t1 = CVec3Dfp64(0,1,0) * rot1;
		t2 = CVec3Dfp64(1,0,0) * rot1;

		/*
		t1 = tmp / CVec3Dfp64(1,2,3);
		t1.Normalize();
		t2 = t1 / tmp;
		t2.Normalize();
		*/

		/*
		Jball[3][3] = t1[0];
		Jball[3][4] = t1[1];
		Jball[3][5] = t1[2];

		Jball[4][3] = t2[0];
		Jball[4][4] = t2[1];
		Jball[4][5] = t2[2];

		Jball[3][9] = -t1[0];
		Jball[3][10] = -t1[1];
		Jball[3][11] = -t1[2];

		Jball[4][9] = -t2[0];
		Jball[4][10] = -t2[1];
		Jball[4][11] = -t2[2];*/

		TMatrix<fp64> Jball_T;
		Jball.Transpose(Jball_T);


		TMatrix<fp64> Minv(12,12);
		Minv.SetIdentity();
		Minv[0][0] = m1inv;
		Minv[1][1] = m1inv;
		Minv[2][2] = m1inv;


		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				Minv[3+i][3+j] = I1inv.GetRow(i)[j];
			}
		}

		Minv[0+6][0+6] = m2inv;
		Minv[1+6][1+6] = m2inv;
		Minv[2+6][2+6] = m2inv;


		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				Minv[9+i][9+j] = I2inv.GetRow(i)[j];
			}
		}

		TMatrix<fp64> B, J_Minv_Jt;
		Minv.Mult(Jball_T, B);
		Jball.Mult(B, J_Minv_Jt);


		TMatrix<fp64> _JV1;
		Jball.Mult(V1, _JV1);
		_JV1 *= -1.0 / dt;

		TMatrix<fp64> Minv_Fext, tmp3;
		Minv.Mult(Fext,tmp3);
		Jball.Mult(tmp3, Minv_Fext);
		Minv_Fext *= -1;
		_JV1 += Minv_Fext;


		TVector<fp64> _JV1vec;
		_JV1.GetColumn(0, _JV1vec);

		TVector<fp64> lambda(3);
		lambda[0] = 2;
		lambda[1] = 3;
		lambda[2] = 4;
		//lambda[3] = 1;
		//lambda[4] = 2;

#ifdef BALLCONSTRAINT_TRACE
		J_Minv_Jt.Print();
		_JV1vec.Print();
#endif

		for (int i = 0; i < 50; i++)
		{
			GaussSiedel<fp64>::Iterate(J_Minv_Jt, lambda, _JV1vec);

//			int l1 = int(lambda[0]);
//			int l2 = int(lambda[1]);
//			int l3 = int(lambda[2]);

//			int breakme = 01;

		}

		lambda *= -1;
		//lambda.Print();

		TMatrix<fp64> tmp1, tmp2;
		Jball_T.Mult(lambda, tmp1);
		tmp1 += Fext;
		Minv.Mult(tmp1,tmp2);
		tmp2 *= dt;
		tmp2 -= V1;

#ifdef BALLCONSTRAINT_TRACE
		M_TRACEALWAYS("V1:\n");
		V1.Print();

		M_TRACEALWAYS("V2:\n");
		tmp2.Print();
#endif

			
		m_rigidbody1->SetVelocity(tmp2[0][0], tmp2[1][0], tmp2[2][0]);
		m_rigidbody1->SetAngularVelocity(tmp2[3][0], tmp2[4][0], tmp2[5][0]);
		m_rigidbody2->SetVelocity(tmp2[6][0], tmp2[7][0], tmp2[8][0]);
		m_rigidbody2->SetAngularVelocity(tmp2[9][0], tmp2[10][0], tmp2[11][0]);

//		CVec3Dfp64 v2 = m_rigidbody2->GetVelocity();
//		CVec3Dfp64 w2 = m_rigidbody2->GetAngularVelocity();


#if 0
		CVec3Dfp64 J1(1,1,1);
		CVec3Dfp64 J2(6,0,0);
		CVec3Dfp64 J3(-1,-1,-1);
		CVec3Dfp64 J4(-6,0,0);


		CMat4Dfp64 rot1 = m_rigidbody1->GetOrientationMatrix();
		CMat4Dfp64 rot2 = m_rigidbody2->GetOrientationMatrix();

		rot1.Transpose3x3();
		rot2.Transpose3x3();

		//rot1.Unit();
		//rot2.Unit();

		CVec3Dfp64 p1 = m_rigidbody1->GetPosition();
		CVec3Dfp64 p2 = m_rigidbody2->GetPosition();

		CVec3Dfp64 d = p2 - p1;
		fp64 foo = d.Length();
//		d = CVec3Dfp64(1,0,0);
		J1 = -d;
		J2 = -((CVec3Dfp64(2,0,0)*rot1));
		J3 = d;
		J4 = ((CVec3Dfp64(-2,0,0)*rot2));

		/*
		J1 = -d;
		J2 = -((CVec3Dfp64(2,0,0)*rot1) / d);
		J3 = d;
		J4 = ((CVec3Dfp64(-2,0,0)*rot2) / d);
*/

		fp64 m1 = m_rigidbody1->GetMass();
		fp64 m2 = m_rigidbody2->GetMass();
		fp64 m1inv = 1.0 / m1;
		fp64 m2inv = 1.0 / m2;
		CMat4Dfp64 I1inv, I2inv;

		m_rigidbody1->GetWorldInertiaTensorInvert(I1inv);
		m_rigidbody2->GetWorldInertiaTensorInvert(I2inv);

		CVec3Dfp64 v1 = m_rigidbody1->GetVelocity();
		CVec3Dfp64 v2 = m_rigidbody2->GetVelocity();
		CVec3Dfp64 w1 = m_rigidbody1->GetAngularVelocity();
		CVec3Dfp64 w2 = m_rigidbody2->GetAngularVelocity();

		//fp64 tmp1 = J2[0] * J2[0] + ( )

		fp64 J_Minv_JT = (J1 * J1) * m1inv + (J2 * I1inv) * J2 + (J3 * J3) * m2inv + (J4 * I2inv) * J4;
		fp64 J_V1 = J1 * v1 + J2 * w1 + J3 * v2 + J4 * w2;

		fp64 psi = 0.8;
		fp64 lambdanum = psi * (1.0/dt) - J_V1 / dt;
		fp64 lambda = lambdanum / J_Minv_JT;

		lambda *= -1.0;

//		CVec3Dfp64 v1prim = (J1 * (1.0/m1) + J2 * I1inv + J3 * (1.0/m2) + J4 * I2inv) * dt * lambda - v1;
//		CVec3Dfp64 v2prim = (J1 * (1.0/m1) + J2 * I1inv + J3 * (1.0/m2) + J4 * I2inv) * dt * lambda - v2;

		CVec3Dfp64 v1prim = (J1 * m1inv) * dt * lambda - v1;
		CVec3Dfp64 v2prim = (J3 * m2inv) * dt * lambda - v2;

		CVec3Dfp64 w1prim = (J2 * I1inv) * dt * lambda - w1;
		CVec3Dfp64 w2prim = (J4 * I2inv) * dt * lambda - w2;

		m_rigidbody1->SetVelocity(v1prim);
		m_rigidbody2->SetVelocity(v2prim);

		m_rigidbody1->SetAngularVelocity(w1prim);
		m_rigidbody2->SetAngularVelocity(w2prim);

		//m_rigidbody1->AddMomentum((J1 * lambda) * (1.0 / m1) );
		//m_rigidbody2->AddMomentum((J3 * lambda) * (1.0 / m2) );

		//m_rigidbody1->AddAngularMomentum((J2 * lambda) * (1.0 / m1) );
		//m_rigidbody2->AddAngularMomentum((J4 * lambda) * (1.0 / m1) );

#endif

		return 0;

	}


	void CBallJointConstraintSolver::PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{

	}

	void CBallJointConstraintSolver::Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
		CRigidBodyState *rb1state = _pConstraint->m_pRigidBodyState1;
		CRigidBodyState *rb2state = _pConstraint->m_pRigidBodyState2;
		CVec3Dfp64 tmp,tmp2;

		fp64 ScaleFactor = _pWorld->GetScaleFactor();

		CMat4Dfp64 T1, T2;
		rb1state->GetTransform(T1);
		rb2state->GetTransform(T2);
		T1.Transpose3x3();
		T2.Transpose3x3();

		CMat4Dfp64 R1 = T1;
		CMat4Dfp64 R2 = T2;

		R1.UnitNot3x3();
		R2.UnitNot3x3();

		CVec3Dfp64 p1 = _pConstraint->m_VectorParams[CBallJointConstraintSolver::POSITION1];
		CVec3Dfp64 p2 = _pConstraint->m_VectorParams[CBallJointConstraintSolver::POSITION2];
		p1 *= R1;
		p2 *= R2;

		CVec3Dfp64 wp1 = p1 + rb1state->m_position;
		CVec3Dfp64 wp2 = p2 + rb2state->m_position;
		CVec3Dfp64 wdiff = wp1 - wp2;

		const double len = 0.005;
		CVec3Dfp64 vbias(0);
		//		if (wdiff.Length() > len)
		if (fabs(wdiff.Length() - len) > 0.00001 )
		{
			CVec3Dfp64 wdiffn = wdiff;
			wdiffn.Normalize();
			vbias = wdiffn * ((wdiff.Length() - len) / _dt);
		}

		vbias *= ScaleFactor;

		CVec3Dfp64 vp1, vp2;
		rb1state->m_angularvelocity.CrossProd(p1 * ScaleFactor, tmp);
		vp1 = rb1state->m_velocity + tmp;

		rb2state->m_angularvelocity.CrossProd(p2 * ScaleFactor, tmp);
		vp2 = rb2state->m_velocity + tmp;
		
//		CVec3Dfp64 vp1 = rb1state->m_velocity + (rb1state->m_angularvelocity / (p1 * ScaleFactor));
//		CVec3Dfp64 vp2 = rb2state->m_velocity + (rb2state->m_angularvelocity / (p2 * ScaleFactor));

		p1 *= ScaleFactor;
		p2 *= ScaleFactor;

		//p1 *= (1.0 / ScaleFactor);
		//p2 *= (1.0 / ScaleFactor);

		CVec3Dfp64 vr = vp1 - vp2 + vbias;
	

		CVec3Dfp64 n = vr;
		n.Normalize();

		double B1 = 1.0 / rb1state->m_mass;
		double B2 = 1.0 / rb2state->m_mass;

		//p1 *= (1.0 / ScaleFactor);
		//p2 *= (1.0 / ScaleFactor);

		p1.CrossProd(n,tmp);
		tmp.MultiplyMatrix(rb1state->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(p1,tmp);
		double B3 = n*tmp;

		p2.CrossProd(n,tmp);
		tmp.MultiplyMatrix(rb2state->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(p2,tmp);
		double B4 = n*tmp;


		double denom =  B1 + B2 + B3 + B4;

		CVec3Dfp64 impulse = n * (-(vr.Length()) / denom);
		CHECK_VALID_VEC(impulse);
	
		//impulse *= 0.5;

		double rb1massinv = 1.0 / rb1state->m_mass;
		double rb2massinv = 1.0 / rb2state->m_mass;

		//CVec3Dfp64 ImpulseSave = impulse;
		//impulse = ImpulseSave * (2.0 * rb1state->m_mass / (rb1state->m_mass + rb2state->m_mass));

		if (rb1state->m_active)
		{
			rb1state->m_velocity+= impulse * (rb1massinv);
			p1.CrossProd(impulse,tmp);
			tmp.MultiplyMatrix(rb1state->m_WorldInertiaTensorInverted,tmp2);
			rb1state->m_angularvelocity+= tmp2;
		}

		impulse *= -1.0;
		//impulse = ImpulseSave * (-2.0 * rb2state->m_mass / (rb1state->m_mass + rb2state->m_mass));

		if (rb2state->m_active)
		{
			rb2state->m_velocity+= impulse * (rb2massinv);
			p2.CrossProd(impulse,tmp);
			tmp.MultiplyMatrix(rb2state->m_WorldInertiaTensorInverted,tmp2);
			rb2state->m_angularvelocity+= tmp2;
		}

		CRigidConstraint Constraint = *_pConstraint;
		Constraint.GetVectorParam(CMinDistanceConstraintSolver::LOCALPOSITION1) =  _pConstraint->GetVectorParam(CBallJointConstraintSolver::MAXDISTREFPOINT1);
		Constraint.GetVectorParam(CMinDistanceConstraintSolver::LOCALPOSITION2) =  _pConstraint->GetVectorParam(CBallJointConstraintSolver::MAXDISTREFPOINT2);
		Constraint.GetScalarParam(CMinDistanceConstraintSolver::MINDISTANCE) = _pConstraint->GetScalarParam(CBallJointConstraintSolver::MINDISTANCE);
		CMinDistanceConstraintSolver::Solve(_pWorld, &Constraint, _dt);

		CHECK_VALID_VEC(rb1state->m_velocity);
		CHECK_VALID_VEC(rb2state->m_velocity);
		CHECK_VALID_VEC(rb1state->m_position);
		CHECK_VALID_VEC(rb2state->m_position);
	}

	void CBallJointConstraintSolver::PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
		CRigidBodyState *rb1state = _pConstraint->m_pRigidBodyState1;
		CRigidBodyState *rb2state = _pConstraint->m_pRigidBodyState2;

		rb1state->m_angularvelocity *= DYNAMICS_CONSTRAINT_ANGULAR_DAMPFACTOR;
		rb1state->m_velocity *= DYNAMICS_CONSTRAINT_LINEAR_DAMPFACTOR;
		rb2state->m_angularvelocity *= DYNAMICS_CONSTRAINT_ANGULAR_DAMPFACTOR;
		rb2state->m_velocity *= DYNAMICS_CONSTRAINT_LINEAR_DAMPFACTOR;
	}

	void CBallJointToWorldSolver::PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{

	}

	void CBallJointToWorldSolver::Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
		CRigidConstraint Constraint = *_pConstraint;

		Constraint.m_VectorParams[CFixedPointConstraintSolver::LOCALPOSITION] = _pConstraint->m_VectorParams[CBallJointToWorldSolver::LOCALPOSITION];
		Constraint.m_VectorParams[CFixedPointConstraintSolver::WORLDPOSITION] = _pConstraint->m_VectorParams[CBallJointToWorldSolver::WORLDPOSITION];
		CFixedPointConstraintSolver::Solve(_pWorld, &Constraint, _dt);

		Constraint.m_VectorParams[CMinDistanceToPointConstraintSolver::WORLDPOSITION] = _pConstraint->m_VectorParams[CBallJointToWorldSolver::MAXDISTANCEPOINT];
		Constraint.m_ScalarParams[CMinDistanceToPointConstraintSolver::MINDISTANCE] = _pConstraint->m_ScalarParams[CBallJointToWorldSolver::MAXDISTANCE];
		CMinDistanceToPointConstraintSolver::Solve(_pWorld, &Constraint, _dt);

		CHECK_VALID_VEC(_pConstraint->m_pRigidBodyState1->m_velocity);
		CHECK_VALID_VEC(_pConstraint->m_pRigidBodyState2->m_velocity);
		CHECK_VALID_VEC(_pConstraint->m_pRigidBodyState1->m_position);
		CHECK_VALID_VEC(_pConstraint->m_pRigidBodyState2->m_position);
	}

	void CBallJointToWorldSolver::PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{

	}

	void CMaxDistanceConstraintSolver::PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
	}

	void CMaxDistanceConstraintSolver::Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
#if 0
		CRigidConstraint Constraint = *_pConstraint;

		CMat4Dfp64 T1, T2;
		_pConstraint->m_pRigidBody1->GetTransform(T1);
		T1.Transpose3x3();
		_pConstraint->m_pRigidBody2->GetTransform(T2);
		T2.Transpose3x3();

		CVec3Dfp64 wp1 = _pConstraint->m_VectorParams[CMaxDistanceConstraintSolver::LOCALPOSITION1] * T1;
		CVec3Dfp64 wp2 = _pConstraint->m_VectorParams[CMaxDistanceConstraintSolver::LOCALPOSITION2] * T2;

		Constraint.m_ScalarParams[CMaxDistanceToPointConstraintSolver::MAXDISTANCE] = _pConstraint->m_ScalarParams[CMaxDistanceConstraintSolver::MAXDISTANCE];

		Constraint.m_pRigidBody1 = _pConstraint->m_pRigidBody1;
		Constraint.m_VectorParams[CMaxDistanceToPointConstraintSolver::WORLDPOSITION] = wp1;
		CMaxDistanceToPointConstraintSolver::Solve(_pWorld, &Constraint, _dt);

		Constraint.m_pRigidBody1 = _pConstraint->m_pRigidBody2;
		Constraint.m_VectorParams[CMaxDistanceToPointConstraintSolver::WORLDPOSITION] = wp2;
		CMaxDistanceToPointConstraintSolver::Solve(_pWorld, &Constraint, _dt);

#endif

		CRigidBodyState *pState1 = _pConstraint->m_pRigidBodyState1;
		CRigidBodyState *pState2 = _pConstraint->m_pRigidBodyState2;
		fp64 ScaleFactor = _pWorld->GetScaleFactor();

		CMat4Dfp64 T1, T2;
		pState1->GetTransform(T1);
		T1.Transpose3x3();
		pState2->GetTransform(T2);
		T2.Transpose3x3();

		CVec3Dfp64 wp1 = _pConstraint->m_VectorParams[LOCALPOSITION1] * T1;
		CVec3Dfp64 wp2 = _pConstraint->m_VectorParams[LOCALPOSITION2] * T2;

		CVec3Dfp64 RelativePosition = wp1 - wp2;
		CVec3Dfp64 RelativeVelocity = pState1->m_velocity - pState2->m_velocity;
		CVec3Dfp64 ExtrapolatedRelativePosition = RelativePosition + RelativeVelocity * _dt;

		fp64 Distance = ExtrapolatedRelativePosition.Length();
		fp64 MaxDistance = _pConstraint->m_ScalarParams[MAXDISTANCE];

		RelativePosition *= ScaleFactor;
		CVec3Dfp64 NewVel(0);
		CVec3Dfp64 NewRelativePosition = ExtrapolatedRelativePosition;

		if (Distance > MaxDistance)
		{
			CVec3Dfp64 tmp = ExtrapolatedRelativePosition;
			tmp.Normalize();
//			NewRelativePosition -= tmp * (Distance - MaxDistance);
			NewRelativePosition *= MaxDistance / ExtrapolatedRelativePosition.Length();
		}
		else return;

		//		NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		//NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		CVec3Dfp64 NewRelativeVelocity = (NewRelativePosition - RelativePosition) * (1.0 / _dt);

		//		CVec3Dfp64 N = PointVelocity - NewVel;
//		CVec3Dfp64 N = NewVel - state->m_velocity;
		CVec3Dfp64 N = NewRelativeVelocity - RelativeVelocity;
		//		CVec3Dfp64 N = m_pRigidBody->GetVelocity() - NewVel;
		fp64 Vel = N.Length();
		N.Normalize();

		fp64 massinv = 1.0 / pState1->m_mass + 1.0 / pState2->m_mass;
		fp64 num = -Vel;


		/*	
		CVec3Dfp64 tmp,tmp2;
		CVec3Dfp64(0,0,0).CrossProd(N,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(CVec3Dfp64(0,0,0),tmp);
		*/
		//		fp64 denom = massinv +  N * tmp;

		fp64 denom = massinv;
		fp64 Impulse = num / denom;

		CVec3Dfp64 force = N * Impulse;

		//force *= -0.001;

		//force *= -0.01;
		force *= -1.0;

		//force *= 0.1;

		//		tmp2 = _pConstraint->m_VectorParams[LOCALPOSITION];
		//		tmp2 *= ScaleFactor;
		//		T.UnitNot3x3();
		//		tmp2 *= T;

		//		tmp2 = CVec3Dfp64(0,0,0);

		pState1->m_velocity+= force * (1.0 / pState1->m_mass);
		pState2->m_velocity-= force * (1.0 / pState2->m_mass);

		/*		tmp2.CrossProd(force,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		state->m_angularvelocity+= tmp2;
		*/

		//M_ASSERT(state->m_velocity.k[0] > -100000.0f && state->m_velocity.k[0] < 100000.0f, "Invalid velocity");

	}

	void CMaxDistanceConstraintSolver::PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
	}

	void CFixedPointConstraintSolver::PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{

	}

	void CFixedPointConstraintSolver::Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
		CRigidBodyState *state = _pConstraint->m_pRigidBodyState1;
		fp64 ScaleFactor = _pWorld->GetScaleFactor();

		CMat4Dfp64 T;
		state->GetTransform(T);
		T.Transpose3x3();

		CVec3Dfp64 FixedPointWorldPos = _pConstraint->m_VectorParams[LOCALPOSITION];
		FixedPointWorldPos *= T;

		CVec3Dfp64 ConstraintDir = FixedPointWorldPos - state->m_position;
		ConstraintDir *= ScaleFactor;
		CVec3Dfp64 PointVelocity = state->m_velocity + (state->m_angularvelocity / ConstraintDir);

		//		CVec3Dfp64 Diff = FixedPointWorldPos - m_WorldPos;
		CVec3Dfp64 Diff = _pConstraint->m_VectorParams[WORLDPOSITION] - FixedPointWorldPos;
		Diff *= ScaleFactor;
		CVec3Dfp64 NewVel(0);
		fp64 MaxDist = 0.005;
		//		if (Diff.Length() > MaxDist)
		if (Diff.Length() > 0.00001 && fabs(Diff.Length() - MaxDist) > 0.00001)

		{
			CVec3Dfp64 tmp = Diff;
			// TODO: SKA DET INTE VARA tmp.Normalize() här!!?!?!?!
			//Diff.Normalize();
			tmp.Normalize();
			NewVel = tmp * ((Diff.Length() - MaxDist) / _dt);
		}

		CVec3Dfp64 N = PointVelocity - NewVel;
		//		CVec3Dfp64 N = m_pRigidBody->GetVelocity() - NewVel;
		fp64 Vel = N.Length();
		N.Normalize();

		fp64 massinv = 1.0 / state->m_mass;
		fp64 num = -Vel;


		CVec3Dfp64 tmp,tmp2;
		ConstraintDir.CrossProd(N,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(ConstraintDir,tmp);

		fp64 denom = massinv +  N * tmp;
		fp64 Impulse = num / denom;

		CVec3Dfp64 force = N * Impulse;
		CHECK_VALID_VEC(force);

		//force *= 0.5;

		//force *= 0.1;

		tmp2 = _pConstraint->m_VectorParams[LOCALPOSITION];
		tmp2 *= ScaleFactor;
		T.UnitNot3x3();
		tmp2 *= T;

		state->m_velocity+= force * (massinv);
		tmp2.CrossProd(force,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		state->m_angularvelocity+= tmp2;

		CHECK_VALID_VEC(state->m_velocity);
		CHECK_VALID_VEC(state->m_position);
		CHECK_VALID_VEC(state->m_angularvelocity);
	}

	void CFixedPointConstraintSolver::PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
		CRigidBodyState *rb1state = _pConstraint->m_pRigidBodyState1;

		rb1state->m_angularvelocity *= DYNAMICS_CONSTRAINT_ANGULAR_DAMPFACTOR;
		rb1state->m_velocity *= DYNAMICS_CONSTRAINT_LINEAR_DAMPFACTOR;
	}

	void CAxisConstraintSolver::PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{

	}

	void CAxisConstraintSolver::Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
		CRigidConstraint Constraint = *_pConstraint;

		Constraint.m_VectorParams[CFixedPointConstraintSolver::LOCALPOSITION] = _pConstraint->m_VectorParams[CAxisConstraintSolver::LOCALPOSITION1];
		Constraint.m_VectorParams[CFixedPointConstraintSolver::WORLDPOSITION] = _pConstraint->m_VectorParams[CAxisConstraintSolver::WORLDPOSITION1];
		CFixedPointConstraintSolver::Solve(_pWorld, &Constraint, _dt);

		Constraint.m_VectorParams[CFixedPointConstraintSolver::LOCALPOSITION] = _pConstraint->m_VectorParams[CAxisConstraintSolver::LOCALPOSITION2];
		Constraint.m_VectorParams[CFixedPointConstraintSolver::WORLDPOSITION] = _pConstraint->m_VectorParams[CAxisConstraintSolver::WORLDPOSITION2];
		CFixedPointConstraintSolver::Solve(_pWorld, &Constraint, _dt);

		Constraint.m_VectorParams[CMinDistanceToPointConstraintSolver::WORLDPOSITION] = _pConstraint->m_VectorParams[CAxisConstraintSolver::MAXDISTANCEPOINT];
		Constraint.m_ScalarParams[CMinDistanceToPointConstraintSolver::MINDISTANCE] = _pConstraint->m_ScalarParams[CAxisConstraintSolver::MINDISTANCE];
		CMinDistanceToPointConstraintSolver::Solve(_pWorld, &Constraint, _dt);
	}

	void CAxisConstraintSolver::PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{

	}

	void CHingeConstraintSolver::PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{

	}

	void CHingeConstraintSolver::Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
		CRigidConstraint Constraint = *_pConstraint;

		Constraint.m_ScalarParams[CBallJointConstraintSolver::MINDISTANCE] = 0.0f;
		Constraint.m_VectorParams[CBallJointConstraintSolver::POSITION1] = _pConstraint->m_VectorParams[CHingeConstraintSolver::POSITION1a];
		Constraint.m_VectorParams[CBallJointConstraintSolver::POSITION2] = _pConstraint->m_VectorParams[CHingeConstraintSolver::POSITION1b];
		CBallJointConstraintSolver::Solve(_pWorld, &Constraint, _dt);

		Constraint.m_VectorParams[CBallJointConstraintSolver::POSITION1] = _pConstraint->m_VectorParams[CHingeConstraintSolver::POSITION2a];
		Constraint.m_VectorParams[CBallJointConstraintSolver::POSITION2] = _pConstraint->m_VectorParams[CHingeConstraintSolver::POSITION2b];
		CBallJointConstraintSolver::Solve(_pWorld, &Constraint, _dt);

		CMat4Dfp64 T, Tinv;
		_pConstraint->m_pRigidBody2->GetCorrectTransform(T);
		T.InverseOrthogonal(Tinv);

		CVec3Dfp64 HingeMid = (_pConstraint->GetVectorParam(CHingeConstraintSolver::POSITION2b) + _pConstraint->GetVectorParam(CHingeConstraintSolver::POSITION1b)) * 0.5;
		CVec3Dfp64 HingeAxis = _pConstraint->GetVectorParam(CHingeConstraintSolver::POSITION2b) - _pConstraint->GetVectorParam(CHingeConstraintSolver::POSITION1b);
		HingeAxis.Normalize();
		HingeAxis.MultiplyMatrix3x3(T);
		//HingeAxis *= T;
		HingeMid *= T;

		fp64 RelativeAngle = _pConstraint->GetScalarParam(CHingeConstraintSolver::RELATIVEANGLE);

		CAxisRotfp64 AxisRot(HingeAxis, RelativeAngle / (2.0 * 3.14159265));
		CMat4Dfp64 AxisRotMatrix;
		AxisRot.CreateMatrix(AxisRotMatrix);

		CVec3Dfp64 LocalPos2 = _pConstraint->GetVectorParam(CHingeConstraintSolver::MAXDISTREFPOINT2);
		LocalPos2 *= T;
		LocalPos2 -= HingeMid;
		LocalPos2 *= AxisRotMatrix;
		LocalPos2 += HingeMid;
		LocalPos2 *= Tinv;

		Constraint.GetVectorParam(CMinDistanceConstraintSolver::LOCALPOSITION2) =  LocalPos2;


		Constraint.GetVectorParam(CMinDistanceConstraintSolver::LOCALPOSITION1) =  _pConstraint->GetVectorParam(CHingeConstraintSolver::MAXDISTREFPOINT1);
		//Constraint.GetVectorParam(CMinDistanceConstraintSolver::LOCALPOSITION2) =  _pConstraint->GetVectorParam(CHingeConstraintSolver::MAXDISTREFPOINT2);
		Constraint.GetScalarParam(CMinDistanceConstraintSolver::MINDISTANCE) = _pConstraint->GetScalarParam(CHingeConstraintSolver::MINDISTANCE);
		CMinDistanceConstraintSolver::Solve(_pWorld, &Constraint, _dt);
	}

	void CHingeConstraintSolver::PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
		CRigidBodyState *rb1state = _pConstraint->m_pRigidBodyState1;
		CRigidBodyState *rb2state = _pConstraint->m_pRigidBodyState2;

		rb1state->m_angularvelocity *= DYNAMICS_CONSTRAINT_ANGULAR_DAMPFACTOR;
		rb1state->m_velocity *= DYNAMICS_CONSTRAINT_LINEAR_DAMPFACTOR;

		rb2state->m_angularvelocity *= DYNAMICS_CONSTRAINT_ANGULAR_DAMPFACTOR;
		rb2state->m_velocity *= DYNAMICS_CONSTRAINT_LINEAR_DAMPFACTOR;
	}

	void CMaxDistanceToPointConstraintSolver::PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{

	}

	/*
		TODO:

		Om en kropp tryckas undan och är inaktivt (pga depenetrationslösaren) kan det ligga långt
		utanför dess giltiga område. När kroppen sedan blir aktiv igen får den en orimligt stor impuls.
	 */

	void CMaxDistanceToPointConstraintSolver::Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
#if 0
		CRigidBodyState *state = _pConstraint->m_pRigidBodyState1;
		fp64 ScaleFactor = _pWorld->GetScaleFactor();

		CMat4Dfp64 T;
		state->GetTransform(T);
		T.Transpose3x3();

		//CVec3Dfp64 FixedPointWorldPos = _pConstraint->m_VectorParams[LOCALPOSITION];
		//FixedPointWorldPos *= T;
		CVec3Dfp64 WorldPos = _pConstraint->m_VectorParams[WORLDPOSITION];

//		CVec3Dfp64 ConstraintDir = FixedPointWorldPos - state->m_position;
//		ConstraintDir *= ScaleFactor;
//		CVec3Dfp64 PointVelocity = state->m_velocity + (state->m_angularvelocity / ConstraintDir);

		CVec3Dfp64 ExtrapolPos = state->m_position + state->m_velocity * _dt;

		//CVec3Dfp64 Diff = _pConstraint->m_VectorParams[WORLDPOSITION] - FixedPointWorldPos;
//		CVec3Dfp64 Diff = state->m_position - WorldPos;
		CVec3Dfp64 Diff = ExtrapolPos - WorldPos;
		fp64 MaxDist = _pConstraint->m_ScalarParams[MAXDISTANCE];

		Diff *= ScaleFactor;
		CVec3Dfp64 NewVel(0);
		//fp64 MaxDist = 0.005;
		//		if (Diff.Length() > MaxDist)
//		if (fabs(Diff.Length() - MaxDist) > 0.00001 )

		//CVec3Dfp64 NewPos = state->m_position;
		CVec3Dfp64 NewPos = ExtrapolPos;

		fp64 Distance = Diff.Length();
		if (Distance > MaxDist)
		{
			CVec3Dfp64 tmp = Diff;
			tmp.Normalize();
			//NewVel = tmp * ((Diff.Length() - MaxDist) / _dt);
			NewPos -= tmp * (Distance - MaxDist);
		}
		else return;

//		NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		NewVel = (NewPos - state->m_position) * (1.0 / _dt);

//		CVec3Dfp64 N = PointVelocity - NewVel;
		CVec3Dfp64 N = NewVel - state->m_velocity;
		//		CVec3Dfp64 N = m_pRigidBody->GetVelocity() - NewVel;
		fp64 Vel = N.Length();
		N.Normalize();

		fp64 massinv = 1.0 / state->m_mass;
		fp64 num = -Vel;


/*	
		CVec3Dfp64 tmp,tmp2;
		CVec3Dfp64(0,0,0).CrossProd(N,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(CVec3Dfp64(0,0,0),tmp);
		*/
//		fp64 denom = massinv +  N * tmp;

		fp64 denom = massinv;
		fp64 Impulse = num / denom;

		CVec3Dfp64 force = N * Impulse;

		//force *= -0.001;

		//force *= -0.01;
		force *= -1.0;

		//force *= 0.1;

//		tmp2 = _pConstraint->m_VectorParams[LOCALPOSITION];
//		tmp2 *= ScaleFactor;
//		T.UnitNot3x3();
//		tmp2 *= T;

//		tmp2 = CVec3Dfp64(0,0,0);

		state->m_velocity+= force * (massinv);
/*		tmp2.CrossProd(force,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		state->m_angularvelocity+= tmp2;
		*/
#endif


		CRigidBodyState *pState1 = _pConstraint->m_pRigidBodyState1;
		//CRigidBodyState *pState2 = _pConstraint->m_pRigidBodyState2;
		fp64 ScaleFactor = _pWorld->GetScaleFactor();

		CMat4Dfp64 T1; //, T2;
		pState1->GetTransform(T1);
		T1.Transpose3x3();
		//pState2->GetTransform(T2);
		//T2.Transpose3x3();

		CVec3Dfp64 wp1 = _pConstraint->m_VectorParams[WORLDPOSITION];
		CVec3Dfp64 wp2 = pState1->m_position;
		//CVec3Dfp64 wp1 = _pConstraint->m_VectorParams[LOCALPOSITION1] * T1;
		//CVec3Dfp64 wp2 = _pConstraint->m_VectorParams[LOCALPOSITION2] * T2;

		CVec3Dfp64 RelativePosition = wp1 - wp2;
//		CVec3Dfp64 RelativeVelocity = pState1->m_velocity - pState2->m_velocity;
		CVec3Dfp64 RelativeVelocity = -pState1->m_velocity;
		CVec3Dfp64 ExtrapolatedRelativePosition = RelativePosition + RelativeVelocity * _dt;

		fp64 Distance = ExtrapolatedRelativePosition.Length();
		fp64 MaxDistance = _pConstraint->m_ScalarParams[MAXDISTANCE];

		RelativePosition *= ScaleFactor;
		CVec3Dfp64 NewVel(0 );
		CVec3Dfp64 NewRelativePosition = ExtrapolatedRelativePosition;

		if (Distance > MaxDistance)
		{
			CVec3Dfp64 tmp = ExtrapolatedRelativePosition;
			tmp.Normalize();
			//			NewRelativePosition -= tmp * (Distance - MaxDistance);
			NewRelativePosition *= MaxDistance / ExtrapolatedRelativePosition.Length();
		}
		else return;

		//		NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		//NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		CVec3Dfp64 NewRelativeVelocity = (NewRelativePosition - RelativePosition) * (1.0 / _dt);

		//		CVec3Dfp64 N = PointVelocity - NewVel;
		//		CVec3Dfp64 N = NewVel - state->m_velocity;
		CVec3Dfp64 N = NewRelativeVelocity - RelativeVelocity;
		//		CVec3Dfp64 N = m_pRigidBody->GetVelocity() - NewVel;
		fp64 Vel = N.Length();
		N.Normalize();

//		fp64 massinv = 1.0 / pState1->m_mass + 1.0 / pState2->m_mass;
		fp64 massinv = 1.0 / pState1->m_mass;
		fp64 num = -Vel;


		/*	
		CVec3Dfp64 tmp,tmp2;
		CVec3Dfp64(0,0,0).CrossProd(N,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(CVec3Dfp64(0,0,0),tmp);
		*/
		//		fp64 denom = massinv +  N * tmp;

		fp64 denom = massinv;
		fp64 Impulse = num / denom;

		CVec3Dfp64 force = N * Impulse;

		//force *= -0.001;

		//force *= -0.01;
		force *= -1.0;

		//force *= 0.1;

		//		tmp2 = _pConstraint->m_VectorParams[LOCALPOSITION];
		//		tmp2 *= ScaleFactor;
		//		T.UnitNot3x3();
		//		tmp2 *= T;

		//		tmp2 = CVec3Dfp64(0,0,0);

		pState1->m_velocity+= force * (1.0 / pState1->m_mass);
		//pState2->m_velocity-= force * (1.0 / pState2->m_mass);

		/*		tmp2.CrossProd(force,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		state->m_angularvelocity+= tmp2;
		*/

		//M_ASSERT(state->m_velocity.k[0] > -100000.0f && state->m_velocity.k[0] < 100000.0f, "Invalid velocity");

	}

	void CMaxDistanceToPointConstraintSolver::PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{

	}

	void CMinDistanceToPointConstraintSolver::PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
	}

#if 0
	void CMinDistanceToPointConstraintSolver::Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
		CRigidBodyState *pState1 = _pConstraint->m_pRigidBodyState1;
		//CRigidBodyState *pState2 = _pConstraint->m_pRigidBodyState2;
		fp64 ScaleFactor = _pWorld->GetScaleFactor();

		CMat4Dfp64 T1, T2;
		pState1->GetTransform(T1);
		T1.Transpose3x3();
		//pState2->GetTransform(T2);
		//T2.Transpose3x3();

/*		CVec3Dfp64 p1 = _pConstraint->m_VectorParams[LOCALPOSITION1];
		CVec3Dfp64 p2 = _pConstraint->m_VectorParams[LOCALPOSITION2];
		p1.MultiplyMatrix3x3(T1);
		p2.MultiplyMatrix3x3(T2);
		p1 *= ScaleFactor;
		p2 *= ScaleFactor;
*/

		CVec3Dfp64 p1 = pState1->m_position;
		p1 *= ScaleFactor;

		CVec3Dfp64 wp1 = pState1->m_position;
		CVec3Dfp64 wp2 = _pConstraint->GetVectorParam(WORLDPOSITION);

//		CVec3Dfp64 wp1 = _pConstraint->m_VectorParams[LOCALPOSITION1] * T1;
//		CVec3Dfp64 wp2 = _pConstraint->m_VectorParams[LOCALPOSITION2] * T2;
		wp1 *= ScaleFactor;
		wp2 *= ScaleFactor;

		CVec3Dfp64 RelativePosition = wp1 - wp2;
		//RelativePosition *= ScaleFactor;

		CVec3Dfp64 tmp, tmp2;

		CVec3Dfp64 wv1, wv2;
		pState1->m_angularvelocity.CrossProd(p1, tmp);
		wv1 = pState1->m_velocity + tmp;

		//pState2->m_angularvelocity.CrossProd(p2, tmp);
		//wv2 = pState2->m_velocity + tmp;
		wv2 = CVec3Dfp64(0.0f);

		CVec3Dfp64 RelativeVelocity = wv1 - wv2;
		CVec3Dfp64 ExtrapolatedRelativePosition = RelativePosition + RelativeVelocity * _dt;

		fp64 Distance = ExtrapolatedRelativePosition.Length();
		fp64 MinDistance = _pConstraint->m_ScalarParams[MINDISTANCE];
		MinDistance *= ScaleFactor;

		CVec3Dfp64 NewVel(0);
		CVec3Dfp64 NewRelativePosition = ExtrapolatedRelativePosition;

		if (Distance < MinDistance)
		{
			CVec3Dfp64 tmp = ExtrapolatedRelativePosition;
			tmp.Normalize();
			//			NewRelativePosition -= tmp * (Distance - MinDistance);
			NewRelativePosition *= MinDistance / ExtrapolatedRelativePosition.Length();
		}
		else return;

		//		NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		//NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		CVec3Dfp64 NewRelativeVelocity = (NewRelativePosition - RelativePosition) * (1.0 / _dt);

		//		CVec3Dfp64 N = PointVelocity - NewVel;
		//		CVec3Dfp64 N = NewVel - state->m_velocity;
		CVec3Dfp64 N = NewRelativeVelocity - RelativeVelocity;
		//		CVec3Dfp64 N = m_pRigidBody->GetVelocity() - NewVel;
		fp64 Vel = N.Length();
		N.Normalize();

		fp64 massinv = 1.0 / pState1->m_mass; // + 1.0 / pState2->m_mass;
		fp64 num = -Vel;


		/*	
		CVec3Dfp64 tmp,tmp2;
		CVec3Dfp64(0,0,0).CrossProd(N,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(CVec3Dfp64(0,0,0),tmp);
		*/
		//		fp64 denom = massinv +  N * tmp;

		fp64 denom = massinv;
		fp64 Impulse = num / denom;

		CVec3Dfp64 force = N * Impulse;

		//force *= -0.001;

		//		force *= -0.01;
		force *= -1.0;
		//force *= -0.1;

		force *= 0.0;

		double B1 = 1.0 / pState1->m_mass;
		double B2 = 0.0f; //1.0 / pState2->m_mass;

		p1.CrossProd(N,tmp);
		tmp.MultiplyMatrix(pState1->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(p1,tmp);
		double B3 = N*tmp;

/*		p2.CrossProd(N,tmp);
		tmp.MultiplyMatrix(pState2->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(p2,tmp);
		double B4 = N*tmp;*/
		double B4 = 0.0;

		double denom2 =  B1 + B2 + B3 + B4;

		Vel *= 0.01;

		//		CVec3Dfp64 impulse = N * (-(vr.Length()) / denom);
		CVec3Dfp64 impulse = N * ((Vel) / denom2);

		impulse *= -1;


		fp64 rb1massinv = 1.0 / pState1->m_mass;
//		fp64 rb2massinv = 1.0 / pState2->m_mass;

		if (pState1->m_active)
		{
			pState1->m_velocity+= impulse * (rb1massinv);
			p1.CrossProd(impulse,tmp);
			tmp.MultiplyMatrix(pState1->m_WorldInertiaTensorInverted,tmp2);
			pState1->m_angularvelocity+= tmp2;
		}
/*
		impulse *= -1.0;

		if (pState2->m_active)
		{
			pState2->m_velocity+= impulse * (rb2massinv);
			p2.CrossProd(impulse,tmp);
			tmp.MultiplyMatrix(pState2->m_WorldInertiaTensorInverted,tmp2);
			pState2->m_angularvelocity+= tmp2;
		}
*/

		//force *= 0.1;

		//		tmp2 = _pConstraint->m_VectorParams[LOCALPOSITION];
		//		tmp2 *= ScaleFactor;
		//		T.UnitNot3x3();
		//		tmp2 *= T;

		//		tmp2 = CVec3Dfp64(0,0,0);

		//		pState1->m_velocity+= force * (1.0 / pState1->m_mass);
		//		pState2->m_velocity-= force * (1.0 / pState2->m_mass);

		/*		tmp2.CrossProd(force,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		state->m_angularvelocity+= tmp2;
		*/
	}

#else
	void CMinDistanceToPointConstraintSolver::Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{

		CRigidBodyState *pState1 = _pConstraint->m_pRigidBodyState1;
		//CRigidBodyState *pState2 = _pConstraint->m_pRigidBodyState2;
		fp64 ScaleFactor = _pWorld->GetScaleFactor();

		CMat4Dfp64 T1; //, T2;
		pState1->GetTransform(T1);
		T1.Transpose3x3();
		//pState2->GetTransform(T2);
		//T2.Transpose3x3();

		CVec3Dfp64 wp1 = _pConstraint->GetVectorParam(WORLDPOSITION);
		CVec3Dfp64 wp2 = pState1->m_position;
		//CVec3Dfp64 wp1 = _pConstraint->m_VectorParams[LOCALPOSITION1] * T1;
		//CVec3Dfp64 wp2 = _pConstraint->m_VectorParams[LOCALPOSITION2] * T2;

		CVec3Dfp64 RelativePosition = wp1 - wp2;
		RelativePosition *= ScaleFactor;

//		CVec3Dfp64 RelativeVelocity = pState1->m_velocity - pState2->m_velocity;
		CVec3Dfp64 RelativeVelocity = -pState1->m_velocity;
		CVec3Dfp64 ExtrapolatedRelativePosition = RelativePosition + RelativeVelocity * _dt;

		fp64 Distance = ExtrapolatedRelativePosition.Length();
		fp64 MaxDistance = _pConstraint->m_ScalarParams[MINDISTANCE];
		MaxDistance *= ScaleFactor;

		CVec3Dfp64 NewVel(0);
		CVec3Dfp64 NewRelativePosition = ExtrapolatedRelativePosition;

		if (Distance < MaxDistance)
		{
			CVec3Dfp64 tmp = ExtrapolatedRelativePosition;
			tmp.Normalize();
			//			NewRelativePosition -= tmp * (Distance - MaxDistance);
			NewRelativePosition *= MaxDistance / ExtrapolatedRelativePosition.Length();
		}
		else return;

		//		NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		//NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		CVec3Dfp64 NewRelativeVelocity = (NewRelativePosition - RelativePosition) * (1.0 / _dt);

		//		CVec3Dfp64 N = PointVelocity - NewVel;
		//		CVec3Dfp64 N = NewVel - state->m_velocity;
		CVec3Dfp64 N = NewRelativeVelocity - RelativeVelocity;
		//		CVec3Dfp64 N = m_pRigidBody->GetVelocity() - NewVel;
		fp64 Vel = N.Length();
		N.Normalize();

//		fp64 massinv = 1.0 / pState1->m_mass + 1.0 / pState2->m_mass;
		fp64 massinv = 1.0 / pState1->m_mass;
		fp64 num = -Vel;


		/*	
		CVec3Dfp64 tmp,tmp2;
		CVec3Dfp64(0,0,0).CrossProd(N,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(CVec3Dfp64(0,0,0),tmp);
		*/
		//		fp64 denom = massinv +  N * tmp;

		fp64 denom = massinv;
		fp64 Impulse = num / denom;

		CVec3Dfp64 force = N * Impulse;

		//force *= -0.001;

		//force *= -0.01;
		force *= -1.0;

		//force *= 0.1;

		//		tmp2 = _pConstraint->m_VectorParams[LOCALPOSITION];
		//		tmp2 *= ScaleFactor;
		//		T.UnitNot3x3();
		//		tmp2 *= T;

		//		tmp2 = CVec3Dfp64(0,0,0);

		//pState1->m_velocity+= force * (1.0 / pState1->m_mass);
		//pState2->m_velocity-= force * (1.0 / pState2->m_mass);

		pState1->m_velocity-= force * (1.0 / pState1->m_mass);

		/*		tmp2.CrossProd(force,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		state->m_angularvelocity+= tmp2;
		*/

	}

#endif
	
	void CMinDistanceToPointConstraintSolver::PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{

	}

	void CMinDistanceConstraintSolver::PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
	}

#if 1

	void CMinDistanceConstraintSolver::Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
		CRigidBodyState *pState1 = _pConstraint->m_pRigidBodyState1;
		CRigidBodyState *pState2 = _pConstraint->m_pRigidBodyState2;
		fp64 ScaleFactor = _pWorld->GetScaleFactor();

		CMat4Dfp64 T1, T2;
		pState1->GetTransform(T1);
		T1.Transpose3x3();
		pState2->GetTransform(T2);
		T2.Transpose3x3();

		CVec3Dfp64 p1 = _pConstraint->m_VectorParams[LOCALPOSITION1];
		CVec3Dfp64 p2 = _pConstraint->m_VectorParams[LOCALPOSITION2];
		p1.MultiplyMatrix3x3(T1);
		p2.MultiplyMatrix3x3(T2);
		p1 *= ScaleFactor;
		p2 *= ScaleFactor;

		CVec3Dfp64 wp1 = _pConstraint->m_VectorParams[LOCALPOSITION1] * T1;
		CVec3Dfp64 wp2 = _pConstraint->m_VectorParams[LOCALPOSITION2] * T2;
		wp1 *= ScaleFactor;
		wp2 *= ScaleFactor;

		CVec3Dfp64 RelativePosition = wp1 - wp2;
		//RelativePosition *= ScaleFactor;

		CVec3Dfp64 tmp, tmp2;

		CVec3Dfp64 wv1, wv2;
		pState1->m_angularvelocity.CrossProd(p1, tmp);
		wv1 = pState1->m_velocity + tmp;

		pState2->m_angularvelocity.CrossProd(p2, tmp);
		wv2 = pState2->m_velocity + tmp;

		CVec3Dfp64 RelativeVelocity = wv1 - wv2;
		CVec3Dfp64 ExtrapolatedRelativePosition = RelativePosition + RelativeVelocity * _dt;

		fp64 Distance = ExtrapolatedRelativePosition.Length();
		fp64 MinDistance = _pConstraint->m_ScalarParams[MINDISTANCE];
		MinDistance *= ScaleFactor;

		CVec3Dfp64 NewVel(0);
		CVec3Dfp64 NewRelativePosition = ExtrapolatedRelativePosition;

		if (Distance < MinDistance)
		{
			CVec3Dfp64 tmp = ExtrapolatedRelativePosition;
			tmp.Normalize();
			//			NewRelativePosition -= tmp * (Distance - MinDistance);
			NewRelativePosition *= MinDistance / ExtrapolatedRelativePosition.Length();
		}
		else return;

		//		NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		//NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		CVec3Dfp64 NewRelativeVelocity = (NewRelativePosition - RelativePosition) * (1.0 / _dt);

		//		CVec3Dfp64 N = PointVelocity - NewVel;
		//		CVec3Dfp64 N = NewVel - state->m_velocity;
		CVec3Dfp64 N = NewRelativeVelocity - RelativeVelocity;
		//		CVec3Dfp64 N = m_pRigidBody->GetVelocity() - NewVel;
		fp64 Vel = N.Length();
		N.Normalize();

		fp64 massinv = 1.0 / pState1->m_mass + 1.0 / pState2->m_mass;
		fp64 num = -Vel;


		/*	
		CVec3Dfp64 tmp,tmp2;
		CVec3Dfp64(0,0,0).CrossProd(N,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(CVec3Dfp64(0,0,0),tmp);
		*/
		//		fp64 denom = massinv +  N * tmp;

		fp64 denom = massinv;
		fp64 Impulse = num / denom;

		CVec3Dfp64 force = N * Impulse;

		//force *= -0.001;

		//		force *= -0.01;
		force *= -1.0;
		//force *= -0.1;

		double B1 = 1.0 / pState1->m_mass;
		double B2 = 1.0 / pState2->m_mass;

		p1.CrossProd(N,tmp);
		tmp.MultiplyMatrix(pState1->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(p1,tmp);
		double B3 = N*tmp;

		p2.CrossProd(N,tmp);
		tmp.MultiplyMatrix(pState2->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(p2,tmp);
		double B4 = N*tmp;

		double denom2 =  B1 + B2 + B3 + B4;

//		CVec3Dfp64 impulse = N * (-(vr.Length()) / denom);
		CVec3Dfp64 impulse = N * ((Vel) / denom2);

		fp64 rb1massinv = 1.0 / pState1->m_mass;
		fp64 rb2massinv = 1.0 / pState2->m_mass;

		if (pState1->m_active)
		{
			pState1->m_velocity+= impulse * (rb1massinv);
			p1.CrossProd(impulse,tmp);
			tmp.MultiplyMatrix(pState1->m_WorldInertiaTensorInverted,tmp2);
			pState1->m_angularvelocity+= tmp2;
		}

		impulse *= -1.0;

		if (pState2->m_active)
		{
			pState2->m_velocity+= impulse * (rb2massinv);
			p2.CrossProd(impulse,tmp);
			tmp.MultiplyMatrix(pState2->m_WorldInertiaTensorInverted,tmp2);
			pState2->m_angularvelocity+= tmp2;
		}


		//force *= 0.1;

		//		tmp2 = _pConstraint->m_VectorParams[LOCALPOSITION];
		//		tmp2 *= ScaleFactor;
		//		T.UnitNot3x3();
		//		tmp2 *= T;

		//		tmp2 = CVec3Dfp64(0,0,0);

//		pState1->m_velocity+= force * (1.0 / pState1->m_mass);
//		pState2->m_velocity-= force * (1.0 / pState2->m_mass);

		/*		tmp2.CrossProd(force,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		state->m_angularvelocity+= tmp2;
		*/
	}

#else

	// OLD
	void CMinDistanceConstraintSolver::Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
		CRigidBodyState *pState1 = _pConstraint->m_pRigidBodyState1;
		CRigidBodyState *pState2 = _pConstraint->m_pRigidBodyState2;
		fp64 ScaleFactor = _pWorld->GetScaleFactor();
		
		CMat4Dfp64 T1, T2;
		pState1->GetTransform(T1);
		T1.Transpose3x3();
		pState2->GetTransform(T2);
		T2.Transpose3x3();

		CVec3Dfp64 wp1 = _pConstraint->m_VectorParams[LOCALPOSITION1] * T1;
		CVec3Dfp64 wp2 = _pConstraint->m_VectorParams[LOCALPOSITION2] * T2;

		CVec3Dfp64 RelativePosition = wp1 - wp2;
		RelativePosition *= ScaleFactor;

		CVec3Dfp64 RelativeVelocity = pState1->m_velocity - pState2->m_velocity;
//		CVec3Dfp64 RelativeVelocity = _pConstraint->m_pRigidBody1->GetVelocityAt(wp1) - _pConstraint->m_pRigidBody2->GetVelocityAt(wp2);
		CVec3Dfp64 ExtrapolatedRelativePosition = RelativePosition + RelativeVelocity * _dt;

		fp64 Distance = ExtrapolatedRelativePosition.Length();
		fp64 MaxDistance = _pConstraint->m_ScalarParams[MINDISTANCE];
		MaxDistance *= ScaleFactor;

		CVec3Dfp64 NewVel(0);
		CVec3Dfp64 NewRelativePosition = ExtrapolatedRelativePosition;

		if (Distance < MaxDistance)
		{
			CVec3Dfp64 tmp = ExtrapolatedRelativePosition;
			tmp.Normalize();
			//			NewRelativePosition -= tmp * (Distance - MaxDistance);
			NewRelativePosition *= MaxDistance / ExtrapolatedRelativePosition.Length();
		}
		else return;

		//		NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		//NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		CVec3Dfp64 NewRelativeVelocity = (NewRelativePosition - RelativePosition) * (1.0 / _dt);

		//		CVec3Dfp64 N = PointVelocity - NewVel;
		//		CVec3Dfp64 N = NewVel - state->m_velocity;
		CVec3Dfp64 N = NewRelativeVelocity - RelativeVelocity;
		//		CVec3Dfp64 N = m_pRigidBody->GetVelocity() - NewVel;
		fp64 Vel = N.Length();
		N.Normalize();

		fp64 massinv = 1.0 / pState1->m_mass + 1.0 / pState2->m_mass;
		fp64 num = -Vel;


		/*	
		CVec3Dfp64 tmp,tmp2;
		CVec3Dfp64(0,0,0).CrossProd(N,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(CVec3Dfp64(0,0,0),tmp);
		*/
		//		fp64 denom = massinv +  N * tmp;

		fp64 denom = massinv;
		fp64 Impulse = num / denom;

		CVec3Dfp64 force = N * Impulse;

		//force *= -0.001;

//		force *= -0.01;
		force *= -1.0;

		//force *= 0.1;

		//		tmp2 = _pConstraint->m_VectorParams[LOCALPOSITION];
		//		tmp2 *= ScaleFactor;
		//		T.UnitNot3x3();
		//		tmp2 *= T;

		//		tmp2 = CVec3Dfp64(0,0,0);

		pState1->m_velocity+= force * (1.0 / pState1->m_mass);
		pState2->m_velocity-= force * (1.0 / pState2->m_mass);

		/*		tmp2.CrossProd(force,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		state->m_angularvelocity+= tmp2;
		*/
	}
#endif


#if 0
	void CMinDistanceConstraintSolver::Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{
		CRigidBodyState *pState1 = _pConstraint->m_pRigidBodyState1;
		CRigidBodyState *pState2 = _pConstraint->m_pRigidBodyState2;
		fp64 ScaleFactor = _pWorld->GetScaleFactor();

		CMat4Dfp64 T1, T2;
		pState1->GetTransform(T1);
		T1.Transpose3x3();
		pState2->GetTransform(T2);
		T2.Transpose3x3();

		CVec3Dfp64 wp1 = _pConstraint->m_VectorParams[LOCALPOSITION1] * T1;
		CVec3Dfp64 wp2 = _pConstraint->m_VectorParams[LOCALPOSITION2] * T2;

		CVec3Dfp64 RelativePosition = wp1 - wp2;
		CVec3Dfp64 RelativeVelocity = pState1->m_velocity - pState2->m_velocity;
		CVec3Dfp64 ExtrapolatedRelativePosition = RelativePosition + RelativeVelocity * _dt;

		//CVec3Dfp64 FixedPointWorldPos = _pConstraint->m_VectorParams[LOCALPOSITION];
		//FixedPointWorldPos *= T;
		//CVec3Dfp64 WorldPos = _pConstraint->m_VectorParams[WORLDPOSITION];

		//		CVec3Dfp64 ConstraintDir = FixedPointWorldPos - state->m_position;
		//		ConstraintDir *= ScaleFactor;
		//		CVec3Dfp64 PointVelocity = state->m_velocity + (state->m_angularvelocity / ConstraintDir);

		//CVec3Dfp64 ExtrapolPos = state->m_position + state->m_velocity * _dt;

		//CVec3Dfp64 Diff = _pConstraint->m_VectorParams[WORLDPOSITION] - FixedPointWorldPos;
		//		CVec3Dfp64 Diff = state->m_position - WorldPos;

//		CVec3Dfp64 Diff = ExtrapolPos - WorldPos;
		fp64 Distance = ExtrapolatedRelativePosition.Length();
		fp64 MinDist = _pConstraint->m_ScalarParams[MINDISTANCE];

		RelativePosition *= ScaleFactor;
		//Diff *= ScaleFactor;
		CVec3Dfp64 NewVel(0);
		//fp64 MaxDist = 0.005;
		//		if (Diff.Length() > MaxDist)
		//		if (fabs(Diff.Length() - MaxDist) > 0.00001 )

		//CVec3Dfp64 NewPos = state->m_position;
		//CVec3Dfp64 NewPos = ExtrapolPos;
		CVec3Dfp64 NewRelativePosition = ExtrapolatedRelativePosition;

		//fp64 Distance = Diff.Length();
		if (Distance < MinDist)
		{
			CVec3Dfp64 tmp = ExtrapolatedRelativePosition;
			tmp.Normalize();
			//NewVel = tmp * ((Diff.Length() - MaxDist) / _dt);
//			NewPos -= tmp * (Distance - MinDist);
			NewRelativePosition -= tmp * (Distance - MinDist);
		}
		else return;

		//		NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		//NewVel = (NewPos - state->m_position) * (1.0 / _dt);
		CVec3Dfp64 NewRelativeVelocity = (NewRelativePosition - RelativePosition) * (1.0 / _dt);

		//		CVec3Dfp64 N = PointVelocity - NewVel;
//		CVec3Dfp64 N = NewVel - state->m_velocity;
		CVec3Dfp64 N = NewRelativeVelocity - RelativeVelocity;
		//		CVec3Dfp64 N = m_pRigidBody->GetVelocity() - NewVel;
		fp64 Vel = N.Length();
		N.Normalize();

		fp64 massinv = 1.0 / pState1->m_mass + 1.0 / pState2->m_mass;
		fp64 num = -Vel;


		/*	
		CVec3Dfp64 tmp,tmp2;
		CVec3Dfp64(0,0,0).CrossProd(N,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		tmp2.CrossProd(CVec3Dfp64(0,0,0),tmp);
		*/
		//		fp64 denom = massinv +  N * tmp;

		fp64 denom = massinv;
		fp64 Impulse = num / denom;

		CVec3Dfp64 force = N * Impulse;

		//force *= -0.001;

		//force *= -0.01;
		force *= -1.0;

		//force *= 0.1;

		//		tmp2 = _pConstraint->m_VectorParams[LOCALPOSITION];
		//		tmp2 *= ScaleFactor;
		//		T.UnitNot3x3();
		//		tmp2 *= T;

		//		tmp2 = CVec3Dfp64(0,0,0);

		pState1->m_velocity+= force * (1.0 / pState1->m_mass);
		pState2->m_velocity-= force * (1.0 / pState2->m_mass);

		/*		tmp2.CrossProd(force,tmp);
		tmp.MultiplyMatrix(state->m_WorldInertiaTensorInverted,tmp2);
		state->m_angularvelocity+= tmp2;
		*/
	}

#endif

	void CMinDistanceConstraintSolver::PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt)
	{

	}

	/*
	CWorld
	*/

	CWorld::CWorld() {
		/*		CRigidBodyGroup *g= new CRigidBodyGroup("defaultgroup");
		g->SetInternalCollision(true);
		m_rigidbodygrouplist.Add(g);
		*/
		m_nobjects= 0;
		m_nextid= 0;

		// TODO: Hmm, hårdkodat...
		//		m_collisionspace= new CCollisionGrid(20,20,20,64,2);
		//		m_collisionspace= DNew CDummyCollisionSpace();

		m_Gravity = CVec3Dfp64(0,-9.81,0);

		m_ScaleFactor = 1.0;

		m_pWorldCollider = NULL;

		m_DummyRigidBody.SetMass(100000000);
		m_DummyRigidBody.SetInertiaTensor(CInertia::Sphere(100000000, 100));
		m_DummyRigidBody.SetActive(false);

		m_MinEventImpulse = 0.5;
		m_nMaxImpulseEvents = 20;
		m_nMaxTemporaryImpulseEvents = 100;

		m_CollectCollisionEvents = true;

		m_CollisionPrecision = Default;

		m_bSimulating = false;

		gcollisioninfolist.SetLen(0);
		gncollisions = 0;
	}

	void CWorld::SetWorldCollider(IWorldCollider *_pCollider)
	{
		m_pWorldCollider = _pCollider;
	}

	IWorldCollider *CWorld::GetWorldCollider()
	{
		return m_pWorldCollider;
	}

	CWorld::~CWorld() {
		//		delete m_collisionspace;
	}

	void CWorld::SetGravity(const CVec3Dfp64& gravity)
	{
		m_Gravity = gravity;
	}

	CVec3Dfp64 CWorld::GetGravity()
	{
		return m_Gravity;
	}


	void CWorld::AddMassInvariantImpulse(CRigidBody *_pRigidBody, const CVec3Dfp64& _ApplyAt, const CVec3Dfp64& _Force)
	{
		CRigidBodyState* rb1state = _pRigidBody->GetBodyState();
		if (rb1state->m_active) 
		{
			CVec3Dfp64 ra = _ApplyAt - _pRigidBody->GetPosition();
			ra *= m_ScaleFactor;
			CVec3Dfp64 tmp;
			ra.CrossProd(_Force,tmp);
			CMat4Dfp64 t;
			_pRigidBody->GetWorldInertiaTensorInvert(t);
			rb1state->m_velocity+= _Force;
			rb1state->m_angularvelocity+= tmp * t;
		}
	}

	void CWorld::AddMassInvariantImpulse(CRigidBody *_pRigidBody, const CVec3Dfp32& _ApplyAt, const CVec3Dfp32& _Force)
	{
		AddMassInvariantImpulse(_pRigidBody, _ApplyAt.Getfp64(), _Force.Getfp64());
	}

	void CWorld::AddImpulse(CRigidBody *_pRigidBody, 
							const CVec3Dfp64& _ApplyAt, 
							const CVec3Dfp64& _Force) 
	{
		CRigidBodyState *rb1state= _pRigidBody->GetBodyState();

		fp64 rb1mass= rb1state->m_mass;
		fp64 rb1massinv= 1.0/rb1mass;

		CVec3Dfp64 ra = _ApplyAt - _pRigidBody->GetPosition();
		ra *= m_ScaleFactor;

		CVec3Dfp64 tmp, tmp2;

		if (rb1state->m_active) 
		{
			ra.CrossProd(_Force,tmp);

			if (m_bSimulating)
			{
				rb1state->m_force += _Force;
				rb1state->m_torque += tmp;
			}
			else
			{
				_pRigidBody->m_ExternalForce += _Force;
				_pRigidBody->m_ExternalTorque += tmp;
			}
		}		
	}

	void CWorld::AddImpulse(CRigidBody *_pRigidBody, 
							const CVec3Dfp64& _ApplyAt, 
							const CVec3Dfp64& _Velocity, 
							fp64 _Mass,
							fp64 _Restitution)
	{
		CRigidBodyState *rb1state= _pRigidBody->GetBodyState();

		fp64 rb1mass= rb1state->m_mass;
		fp64 rb2mass= _Mass;
		fp64 rb1massinv= 1.0/rb1mass;
		fp64 rb2massinv= 1.0/rb2mass;

		CVec3Dfp64 Normal = _Velocity;
		Normal.Normalize();

		CVec3Dfp64 ra = _ApplyAt - _pRigidBody->GetPosition();
		ra *= m_ScaleFactor;

//		const CMat4Dfp64 &rb1worldinv= _pRigidBody->GetWorldInertiaTensorInvert();

		CVec3Dfp64 BodyVelocity;
		_pRigidBody->GetAngularVelocity().CrossProd(ra,BodyVelocity);
		BodyVelocity+=_pRigidBody->GetVelocity();

		fp64 RelativeVelocity = Normal*(_Velocity*m_ScaleFactor - BodyVelocity);

		double A = -(1.0 + _Restitution) * RelativeVelocity;
//		double B1 = rb1massinv;
//		double B2 = rb2massinv;

		CVec3Dfp64 tmp, tmp2;
		ra.CrossProd(Normal,tmp);
		tmp.MultiplyMatrix(_pRigidBody->GetWorldInertiaTensorInvert(),tmp2);
		tmp2.CrossProd(ra,tmp);
		double B3 = Normal*tmp; 

		double B =  rb1massinv + rb2massinv + B3 + 0.0;
		double impulse = A / (B);

		impulse*=-1;

		CVec3Dfp64 force= Normal*impulse;

		if (rb1state->m_active) 
		{
			if (m_bSimulating)
				rb1state->m_force += force;
			else
				_pRigidBody->m_ExternalForce += force;

//			rb1state->m_velocity+= force * (rb1massinv);
			ra.CrossProd(force,tmp);

			tmp.MultiplyMatrix(_pRigidBody->GetWorldInertiaTensorInvert(),tmp2);
//			rb1state->m_angularvelocity+= tmp2;

			if (m_bSimulating)
				rb1state->m_torque += tmp2;
			else
				_pRigidBody->m_ExternalTorque += tmp2;
		}		
	}

	void CWorld::AddImpulse(CRigidBody *_pRigidBody, 
							const CVec3Dfp32& _ApplyAt, 
							const CVec3Dfp32& _Velocity, 
							fp32 _Mass, 
							fp32 _Restitution)
	{
		AddImpulse(_pRigidBody, ConvertVector(_ApplyAt), ConvertVector(_Velocity), _Mass, _Restitution);
	}

	void CWorld::AddImpulse(CRigidBody *_pRigidBody, const CVec3Dfp32& _ApplyAt, const CVec3Dfp32& _Force)
	{
		AddImpulse(_pRigidBody, _ApplyAt.Getfp64(), _Force.Getfp64());
	}

	void CWorld::AddForce(CRigidBody *_pRigidBody, const CVec3Dfp64 _Force)
	{
		if (m_bSimulating)
			_pRigidBody->GetBodyState()->m_force += _Force;
		else
			_pRigidBody->m_ExternalForce += _Force;
	}

	void CWorld::AddForce(CRigidBody *_pRigidBody, const CVec3Dfp32 _Force)
	{
		AddForce(_pRigidBody, ConvertVector(_Force));
	}

	void CWorld::UpdateInternalStates() {
		m_nobjects= 0;
		//int id=0;
		int maxid=0;

		for (int i=0; i<m_rigidbodylist.Len(); i++)
		{
			m_nobjects++;
			maxid= MAX(maxid,m_rigidbodylist[i]->GetRigidBody()->m_id);
		}
#if 0
		M_TRACEALWAYS("m_nobjects = %d\n", m_nobjects);
		M_TRACEALWAYS("maxid = %d\n",maxid);
#endif


		//		m_testedcollisions.SetLen(m_nobjects*m_nobjects);

		// Detta gjorde att det craschade. Då objekt kan skapas
		// och få ett id utan att vara tillaga... 
		// Id-systemet måste fixas till...

		//		m_testedcollisions.SetLen((maxid+1)*(maxid+1));		
		//m_testedcollisions.SetLen((m_nextid+1)*(m_nextid+1));		

		// Ändrar igen.... efter det nya systemet
		m_testedcollisions.SetLen((maxid+1)*(maxid+1));		

#if 1
		// TODO: !!!!
		// Är inte detta buggit???
		// Antar man inte att .SetLen() inte skall förstöra de gamla värdena???

		for (int i=0; i<m_rigidbodylist.Len(); i++) 
		{
			CRigidBody *rb = m_rigidbodylist[i]->GetRigidBody();
			rb->m_pstate = &(m_bodystate.GetBasePtr()[rb->m_id]);
			rb->m_psavedstate = &(m_savedbodystate.GetBasePtr()[rb->m_id]);
		}
#endif
	}

	void CWorld::UpdateInternalStates2() {
		m_nobjects= 0;
		//int id=0;
		int maxid=0;

		int nBodies = m_rigidbodylist.Len();

		for (int i=0; i<nBodies; i++)
		{
			m_nobjects++;
			maxid= MAX(maxid,m_rigidbodylist[i]->GetRigidBody()->m_id);
		}
		m_testedcollisions.SetLen((maxid+1)*(maxid+1));		

		int id = 0;
		for (int i=0; i<nBodies; i++)
		{
			CRigidBody *pRB = m_rigidbodylist[i];
			pRB->m_id = id;
			pRB->m_state = m_bodystate[pRB->m_id];
			pRB->m_pstate = &(m_bodystate.GetBasePtr()[pRB->m_id]);
			pRB->m_psavedstate = &(m_savedbodystate.GetBasePtr()[pRB->m_id]);
			id++;
		}
	}

	void CWorld::AddRigidBody(CRigidBody *rigidbody) {

		//M_TRACE("CWorld::AddRigidBody\n");

		m_rigidbodylist.Add(rigidbody);
		m_savedbodystate.Add(CRigidBodyState());
		m_bodystate.Add(CRigidBodyState());
		rigidbody->GetRigidBody()->m_id = m_rigidbodylist.Len()-1;
		rigidbody->GetRigidBody()->m_world_= this;

		CRigidBody *rb = rigidbody->GetRigidBody();

		// TODO: Copy old state... duhhh
		m_bodystate[rb->m_id] = rb->m_state;
		m_savedbodystate[rb->m_id] = rb->m_state;

		UpdateInternalStates();

		// TODO: Duh, detta måste bort helt...
		m_nextid++;

		/*
		m_rigidbodylist[rigidbody->GetRigidBody()->m_id]= rigidbody;
		m_rigidbodygrouplist[0]->AddRigidBody(rigidbody);
		//rigidbody->GetRigidBody()->m_id= m_nextid++;
		rigidbody->GetRigidBody()->m_world_= this;


		UpdateInternalStates();
		CRigidBody *rb = rigidbody->GetRigidBody();

		// TODO: Copy old state... duhhh
		m_bodystate[rb->m_id] = rb->m_state;*/

		/*
		CRigidBody *rb = rigidbody->GetRigidBody();
		rb->m_pstate = &(m_bodystate.GetBasePtr()[rb->m_id]);
		rb->m_psavedstate = &(m_savedbodystate.GetBasePtr()[rb->m_id]);
		*/
		//m_rigidbodylist.Add(rigidbody);
	}

	void CWorld::RemoveRigidBody(CRigidBody* _pRigidBody)
	{
//		M_TRACE("CWorld::RemoveRigidBody, _pRigidBody = 0x%08X\n", _pRigidBody);

		uint i;

		// Remove all constraints connected to '_pRigidBody'
		TAP<CRigidConstraint> pConstraints = m_lRigidConstraints;
		for (i = 0; i < pConstraints.Len(); i++)
		{
			CRigidConstraint& c = pConstraints[i];
			if (c.m_Type >= 0 && (c.m_pRigidBody1 == _pRigidBody || c.m_pRigidBody2 == _pRigidBody))
			{
				RemoveConnection(c.m_pRigidBody1, c.m_pRigidBody2);
				c.m_Type = -1;
				c.m_pRigidBody1 = c.m_pRigidBody2 = NULL;
			}
		}

		int nBodies = m_rigidbodylist.Len();
		for (i=0; i<nBodies; i++) 
		{
			CRigidBody *pRigidBody = m_rigidbodylist[i];
			if (pRigidBody == _pRigidBody)
			{
				m_rigidbodylist.Del(i);
				m_bodystate.Del(i);
				UpdateInternalStates2();
				break;
			}
		}
		M_ASSERT(i < nBodies, "Can't find rigidbody");
		

		bool FoundToDelete = true;
		while (FoundToDelete && m_lJustUnfreezedObjects.Len() > 0)
		{
			TAP_RCD<CRigidBody*> pJustUnfreezed = m_lJustUnfreezedObjects;
			for (i = 0; i < pJustUnfreezed.Len(); i++)
			{
				if (pJustUnfreezed[i] == _pRigidBody)
				{
					m_lJustUnfreezedObjects.Del(i);				
					FoundToDelete = true;
					break;
				}
				FoundToDelete = false;
			}
		}
	}

	/*
		TODO: Vem ska egentligen äga objekten...!??!?!?!
	*/
	void CWorld::Clear()
	{
		M_TRACEALWAYS("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		M_TRACEALWAYS("CWorld::Clear\n");
//		int nBodies = m_rigidbodylist.Len();
//		for (int i = 0; i < nBodies; i++)
		{
//			M_TRACEALWAYS("%i\n",i);
//			CRigidBody *pRB = m_rigidbodylist[i];
			
			// TODO: Denna får det att crasha...
			//delete pRB;
		}

		m_rigidbodylist.SetLen(0);
		m_bodystate.SetLen(0);
		m_savedbodystate.SetLen(0);
		m_lRigidConstraints.SetLen(0);
		m_constraintlist.SetLen(0);
		m_lJustUnfreezedObjects.SetLen(0);
		m_nextid = 0;
		m_nobjects = 0;

		gcollisioninfolist.SetLen(0);
		gncollisions = 0;
	}

	void CWorld::AddRigidBodyGroup(CRigidBodyGroup *group) {
		M_ASSERT(0,"TODO");
		//		m_rigidbodygrouplist.Add(group);
		// TODO: Behövs detta?
		UpdateInternalStates();
	}

	/*
	TODO: Kontrollera att objekten som constrainten 
	innehåller verkligen finns i världen.
	*/
	void CWorld::AddConstraint(CConstraint *constraint) {
		m_constraintlist.Add(constraint);
	}

	int CWorld::AddBallJointConstraint(CRigidBody *_pRigidBody1, CRigidBody *_pRigidBody2, const CVec3Dfp32& _p1, const CVec3Dfp32& _p2, fp32 _MaxAngle)
	{
		return AddBallJointConstraint(_pRigidBody1, _pRigidBody2, _p1.Getfp64(), _p2.Getfp64(), _MaxAngle);
	}

	int CWorld::AddFixedPointConstraint(CRigidBody *_pRigidBody, const CVec3Dfp32& _LocalPosition, const CVec3Dfp32& _WorldPosition)
	{
		return AddFixedPointConstraint(_pRigidBody, _LocalPosition.Getfp64(), _WorldPosition.Getfp64());
	}

	int CWorld::AddAxisConstraint(CRigidBody *_pRigidBody, const CVec3Dfp32& _Axis, fp32 _AxisLength, const CVec3Dfp32& _Position, const CVec3Dfp32& _AngleAxis, fp32 _MaxAngle)
	{
		return AddAxisConstraint(_pRigidBody, _Axis.Getfp64(), _AxisLength, _Position.Getfp64(), _AngleAxis.Getfp64(), _MaxAngle);		
	}

	int CWorld::AddHingeConstraint(CRigidBody *_pRigidBody1, CRigidBody *_pRigidBody2, const CVec3Dfp32& _Axis, const CVec3Dfp32& _AngleAxis, fp32 _AxisLength, const CVec3Dfp32& _Position, fp32 _MaxAngle)
	{
		return AddHingeConstraint(_pRigidBody1, _pRigidBody2, _Axis.Getfp64(), _AngleAxis.Getfp64(), _AxisLength, _Position.Getfp64(), _MaxAngle);
	}

	int CWorld::AddBallJointToWorld(CRigidBody *_pRigidBody, const CVec3Dfp32& _LocalPosition, const CVec3Dfp32& _WorldPosition, CVec3Dfp32 _AngleAxis, fp32 _MaxAngle)
	{
		return AddBallJointToWorld(_pRigidBody, _LocalPosition.Getfp64(), _WorldPosition.Getfp64(), _AngleAxis.Getfp64(), _MaxAngle);
	}

	int CWorld::AddBallJointConstraint(CRigidBody *_pRigidBody1, CRigidBody *_pRigidBody2, const CVec3Dfp64& _p1, const CVec3Dfp64& _p2, fp64 _MaxAngle)
	{
		CVec3Dfp64 Dir = _pRigidBody1->GetPosition() - _pRigidBody2->GetPosition();
		Dir.Normalize();

/*
		CVec3Dfp64 Ref1 = _pRigidBody1->GetPosition();
		CVec3Dfp64 Ref2 = _pRigidBody2->GetPosition();
		*/

		CMat4Dfp64 T1;
		_pRigidBody1->GetTransform(T1);
		T1.Transpose3x3();

		CMat4Dfp64 T2;
		_pRigidBody2->GetTransform(T2);
		T2.Transpose3x3();

		CVec3Dfp64 Anchor = _p1 * T1;

		CVec3Dfp64 Dir1 = _pRigidBody1->GetPosition() - Anchor;
		Dir1.Normalize();

		CVec3Dfp64 Dir2 = _pRigidBody2->GetPosition() - Anchor;
		Dir2.Normalize();

//		CVec3Dfp64 Ref1 = Dir;
//		CVec3Dfp64 Ref2 = -Dir;
		CVec3Dfp64 Ref1 = Dir1;
		CVec3Dfp64 Ref2 = Dir2;

		CVec3Dfp64 Ref1World = Ref1 * T1;
		CVec3Dfp64 Ref2World = Ref2 * T2;


		CRigidConstraint Constraint;
		Constraint.m_Type = CRigidConstraint::BALLJOINTCONSTRAINT;
		Constraint.m_VectorParams[CBallJointConstraintSolver::POSITION1] = _p1;
		Constraint.m_VectorParams[CBallJointConstraintSolver::POSITION2] = _p2;

		// TODO: Detta är lite oklart om dessa behövs. I fall att man vill ha olika vinklar kanske...
		Constraint.m_VectorParams[CBallJointConstraintSolver::MAXDISTREFPOINT1] = Ref1;
		Constraint.m_VectorParams[CBallJointConstraintSolver::MAXDISTREFPOINT2] = Ref2;
		fp64 b = Ref1World.Distance(Anchor);
		fp64 c = Ref2World.Distance(Anchor);

		fp64 MinDistance = b * b + c * c - 2 * b * c * cos(_PI - _MaxAngle);
		MinDistance = M_Sqrt(MinDistance);
		Constraint.m_ScalarParams[CBallJointConstraintSolver::MINDISTANCE] = MinDistance;

		Constraint.m_pRigidBody1 = _pRigidBody1;
		Constraint.m_pRigidBody2 = _pRigidBody2;
		Constraint.m_pRigidBodyState1 = NULL;
		Constraint.m_pRigidBodyState2 = NULL;

		m_lRigidConstraints.Add(Constraint);

		AddConnection(_pRigidBody1, _pRigidBody2);
		AddConnection(_pRigidBody2, _pRigidBody1);

		return m_lRigidConstraints.Len() - 1;
	}

	int CWorld::AddFixedPointConstraint(CRigidBody *_pRigidBody, const CVec3Dfp64& _LocalPosition, const CVec3Dfp64& _WorldPosition)
	{
		CRigidConstraint Constraint;
		Constraint.m_Type = CRigidConstraint::FIXEDPOINTCONSTRAINT;
		Constraint.m_VectorParams[CFixedPointConstraintSolver::LOCALPOSITION] = _LocalPosition;
		Constraint.m_VectorParams[CFixedPointConstraintSolver::WORLDPOSITION] = _WorldPosition;
		Constraint.m_pRigidBody1 = _pRigidBody;
		Constraint.m_pRigidBody2 = &m_DummyRigidBody;
		Constraint.m_pRigidBodyState1 = NULL;
		Constraint.m_pRigidBodyState2 = NULL;
		m_lRigidConstraints.Add(Constraint);

		AddConnection(_pRigidBody, &m_DummyRigidBody);

		return m_lRigidConstraints.Len() - 1;
	}

	int CWorld::AddAxisConstraint(CRigidBody *_pRigidBody, const CVec3Dfp64& _Axis, fp64 _AxisLength, const CVec3Dfp64& _Position,  const CVec3Dfp64& _AngleAxis, fp64 _MaxAngle)
	{
		CMat4Dfp64 M, Minv,tmp;
		_pRigidBody->GetTransform(tmp);
		M = tmp;
		tmp.Transpose3x3();
		tmp.InverseOrthogonal(Minv);

		CVec3Dfp64 WorldPosition1 = _Position + _Axis * (_AxisLength / 2.0);
		CVec3Dfp64 WorldPosition2 = _Position - _Axis * (_AxisLength / 2.0);

		CVec3Dfp64 LocalPosition1 = WorldPosition1 * Minv;
		CVec3Dfp64 LocalPosition2 = WorldPosition2 * Minv;

		CVec3Dfp64 Anchor = _Position;

		CVec3Dfp64 Dir = -_AngleAxis;
		Dir.Normalize();

		fp64 AnchorToObjectDistance = _pRigidBody->GetPosition().Distance(Anchor);
		Dir *= AnchorToObjectDistance;


		CVec3Dfp64 RefWorld = _Position + Dir;

		fp64 b = RefWorld.Distance(Anchor);
		fp64 c = _pRigidBody->GetPosition().Distance(Anchor);

		fp64 MinDistance = b * b + c * c - 2 * b * c * cos(_PI - _MaxAngle);
		MinDistance = M_Sqrt(MinDistance);
  
		CRigidConstraint Constraint;
		Constraint.m_Type = CRigidConstraint::AXISCONSTRAINT;
		Constraint.m_VectorParams[CAxisConstraintSolver::LOCALPOSITION1] = LocalPosition1;
		Constraint.m_VectorParams[CAxisConstraintSolver::WORLDPOSITION1] = WorldPosition1;
		Constraint.m_VectorParams[CAxisConstraintSolver::LOCALPOSITION2] = LocalPosition2;
		Constraint.m_VectorParams[CAxisConstraintSolver::WORLDPOSITION2] = WorldPosition2;
		Constraint.m_VectorParams[CAxisConstraintSolver::MAXDISTANCEPOINT] = RefWorld;
		Constraint.m_ScalarParams[CAxisConstraintSolver::MINDISTANCE] = MinDistance;

		Constraint.m_pRigidBody1 = _pRigidBody;
		Constraint.m_pRigidBody2 = &m_DummyRigidBody;
		Constraint.m_pRigidBodyState1 = NULL;
		Constraint.m_pRigidBodyState2 = NULL;
		m_lRigidConstraints.Add(Constraint);

		AddConnection(_pRigidBody, &m_DummyRigidBody);

		return m_lRigidConstraints.Len() - 1;
	}


	static fp64 GetPlanarRotation(const CVec3Dfp64& _From, const CVec3Dfp64& _To, const CVec3Dfp64& _Axis)
	{
		CVec3Dfp64 Ref;
		_Axis.CrossProd(_From, Ref);
		fp64 u = _To * _From;
		fp64 v = _To * Ref;
		fp64 Angle = atan2f(v, u);
		return Angle;
	}

	int CWorld::AddHingeConstraint(CRigidBody *_pRigidBody1, CRigidBody *_pRigidBody2, const CVec3Dfp64& _Axis, const CVec3Dfp64& _AngleAxis, fp64 _AxisLength, const CVec3Dfp64& _Position, fp64 _MaxAngle)
	{
		CMat4Dfp64 M1, M2, M1inv,M2inv,tmp;
		_pRigidBody1->GetTransform(tmp);
		M1 = tmp;
		tmp.Transpose3x3();
		tmp.InverseOrthogonal(M1inv);

		_pRigidBody2->GetTransform(tmp);
		M2 = tmp;
		tmp.Transpose3x3();
		tmp.InverseOrthogonal(M2inv);

		CVec3Dfp64 Anchor = _Position;

		CVec3Dfp64 Dir1 = _pRigidBody1->GetPosition() - Anchor;
		Dir1.Normalize();

		CVec3Dfp64 Dir2 = _pRigidBody2->GetPosition() - Anchor;
		Dir2.Normalize();

//		CVec3Dfp64 Ref1 = Dir1 * 10.0;

		CVec3Dfp64 Ref1 = Dir1 * 1.0;
		CVec3Dfp64 Ref2 = Dir2 * 1.0;

		CVec3Dfp64 Ref1World = Ref1 * M1;
		CVec3Dfp64 Ref2World = Ref2 * M2;

		//fp64 RelativeAngle = acos(Dir2 * _AngleAxis);
		//fp64 RelativeAnchorCenterOfMassAngle = acos((-Dir1) * Dir2);
		//RelativeAngle -= RelativeAnchorCenterOfMassAngle;


		fp64 RelativeAngle =  GetPlanarRotation(_AngleAxis, Dir2, _Axis);
		fp64 RelativeAnchorCenterOfMassAngle = GetPlanarRotation(-Dir1, Dir2, _Axis);
		RelativeAngle -= RelativeAnchorCenterOfMassAngle;

//		CVec3Dfp64 Ref2World = (_AngleAxis * (_pRigidBody1->GetPosition().Distance(Anchor) + 10.0)) + Anchor;
//		CVec3Dfp64 Ref2 = Ref2World * M2inv;


		/*
		CVec3Dfp64 Ref1World = Anchor - _AngleAxis * 20.0;
		CVec3Dfp64 Ref2World = Anchor + _AngleAxis * 20.0;

		CVec3Dfp64 Ref1 = Ref1World * M1inv;
		CVec3Dfp64 Ref2 = Ref2World * M2inv;
		*/


//		CVec3Dfp64 WorldPosition1 = _Position + _Axis * 10.0;
//		CVec3Dfp64 WorldPosition2 = _Position - _Axis * 10.0;

		CVec3Dfp64 WorldPosition1 = _Position + _Axis * (_AxisLength / 2.0);
		CVec3Dfp64 WorldPosition2 = _Position - _Axis * (_AxisLength / 2.0);

		CVec3Dfp64 LocalPosition1a = WorldPosition1 * M1inv;
		CVec3Dfp64 LocalPosition2a = WorldPosition2 * M1inv;

		CVec3Dfp64 LocalPosition1b = WorldPosition1 * M2inv;
		CVec3Dfp64 LocalPosition2b = WorldPosition2 * M2inv;

		CRigidConstraint Constraint;
		Constraint.m_Type = CRigidConstraint::HINGECONSTRAINT;

		Constraint.m_ScalarParams[CHingeConstraintSolver::RELATIVEANGLE] = RelativeAngle;

		Constraint.m_VectorParams[CHingeConstraintSolver::POSITION1a] = LocalPosition1a;
		Constraint.m_VectorParams[CHingeConstraintSolver::POSITION2a] = LocalPosition2a;

		Constraint.m_VectorParams[CHingeConstraintSolver::POSITION1b] = LocalPosition1b;
		Constraint.m_VectorParams[CHingeConstraintSolver::POSITION2b] = LocalPosition2b;

		Constraint.m_VectorParams[CHingeConstraintSolver::MAXDISTREFPOINT1] = Ref1;
		Constraint.m_VectorParams[CHingeConstraintSolver::MAXDISTREFPOINT2] = Ref2;
		fp64 b = Ref1World.Distance(Anchor);
		fp64 c = Ref2World.Distance(Anchor);

		fp64 MinDistance = b * b + c * c - 2 * b * c * cos(_PI - _MaxAngle);
		MinDistance = M_Sqrt(MinDistance);
		Constraint.m_ScalarParams[CHingeConstraintSolver::MINDISTANCE] = MinDistance;

		Constraint.m_pRigidBody1 = _pRigidBody1;
		Constraint.m_pRigidBody2 = _pRigidBody2;
		Constraint.m_pRigidBodyState1 = NULL;
		Constraint.m_pRigidBodyState2 = NULL;
		m_lRigidConstraints.Add(Constraint);

		AddConnection(_pRigidBody1, _pRigidBody2);
		AddConnection(_pRigidBody2, _pRigidBody1);

		return m_lRigidConstraints.Len() - 1;
	}

	int CWorld::AddMaxDistanceToPointConstraint(CRigidBody *_pRigidBody, const CVec3Dfp64& _WorldPosition, fp64 _MaxDistance)
	{
		CRigidConstraint Constraint;
		Constraint.m_Type = CRigidConstraint::MAXDISTANCETOPOINTCONSTRAINT;
		Constraint.m_VectorParams[CMaxDistanceToPointConstraintSolver::WORLDPOSITION] = _WorldPosition;
		Constraint.m_ScalarParams[CMaxDistanceToPointConstraintSolver::MAXDISTANCE] = _MaxDistance;
		Constraint.m_pRigidBody1 = _pRigidBody;
		Constraint.m_pRigidBody2 = &m_DummyRigidBody;
		Constraint.m_pRigidBodyState1 = NULL;
		Constraint.m_pRigidBodyState2 = NULL;
		m_lRigidConstraints.Add(Constraint);

		AddConnection(_pRigidBody, &m_DummyRigidBody);

		return m_lRigidConstraints.Len() - 1;
	}

	int CWorld::AddMinDistanceToPointConstraint(CRigidBody *_pRigidBody, const CVec3Dfp64& _WorldPosition, fp64 _MinDistance)
	{
		CRigidConstraint Constraint;
		Constraint.m_Type = CRigidConstraint::MAXDISTANCETOPOINTCONSTRAINT;
		Constraint.m_VectorParams[CMinDistanceToPointConstraintSolver::WORLDPOSITION] = _WorldPosition;
		Constraint.m_ScalarParams[CMinDistanceToPointConstraintSolver::MINDISTANCE] = _MinDistance;
		Constraint.m_pRigidBody1 = _pRigidBody;
		Constraint.m_pRigidBody2 = &m_DummyRigidBody;
		Constraint.m_pRigidBodyState1 = NULL;
		Constraint.m_pRigidBodyState2 = NULL;
		m_lRigidConstraints.Add(Constraint);

		AddConnection(_pRigidBody, &m_DummyRigidBody);

		return m_lRigidConstraints.Len() - 1;
	}

	int CWorld::AddBallJointToWorld(CRigidBody *_pRigidBody, const CVec3Dfp64& _LocalPosition, const CVec3Dfp64& _WorldPosition, CVec3Dfp64 _AngleAxis, fp64 _MaxAngle)
	{
		//CVec3Dfp64 Dir = _pRigidBody->GetPosition() - _WorldPosition;
		//fp64 PointToCenterDist = Dir.Length();
		//Dir.Normalize();

		CMat4Dfp64 T;
		_pRigidBody->GetTransform(T);
		T.Transpose3x3();

		CVec3Dfp64 Dir = -_AngleAxis;
		Dir.Normalize();
		CVec3Dfp64 Anchor = _WorldPosition;

		CVec3Dfp64 RefWorld = _WorldPosition + Dir * 32.0f;
//		CVec3Dfp64 RefWorld = _pRigidBody->GetPosition() + Dir;
		//CVec3Dfp64 RefWorld = Ref * T;


		CRigidConstraint Constraint;
		fp64 b = RefWorld.Distance(Anchor);
		fp64 c = _pRigidBody->GetPosition().Distance(Anchor);

		fp64 MaxDistance = b * b + c * c - 2 * b * c * cos(_PI - _MaxAngle);
		MaxDistance = M_Sqrt(MaxDistance);

		Constraint.m_VectorParams[CBallJointToWorldSolver::MAXDISTANCEPOINT] = RefWorld;
		Constraint.m_ScalarParams[CBallJointToWorldSolver::MAXDISTANCE] = MaxDistance;

		/*
		CVec3Dfp64 MaxDistPoint = _pRigidBody->GetPosition() + Dir * PointToCenterDist;
//		CVec3Dfp64 RefPoint = _pRigidBody->GetPosition();
		CVec3Dfp64 RefPoint = _WorldPosition;
		RefPoint[0] += PointToCenterDist * sin(_MaxAngle);
		RefPoint[1] -= PointToCenterDist * cos(_MaxAngle);
		fp64 MaxDist = MaxDistPoint.Distance(RefPoint);
		*/

		Constraint.m_Type = CRigidConstraint::BALLJOINTTOWORLD;
		Constraint.m_VectorParams[CBallJointToWorldSolver::LOCALPOSITION] = _LocalPosition;
		Constraint.m_VectorParams[CBallJointToWorldSolver::WORLDPOSITION] = _WorldPosition;
		//Constraint.m_VectorParams[CBallJointToWorldSolver::MAXDISTANCEPOINT] = MaxDistPoint;
		//Constraint.m_ScalarParams[CBallJointToWorldSolver::MAXDISTANCE] = MaxDist;

		Constraint.m_pRigidBody1 = _pRigidBody;
		Constraint.m_pRigidBody2 = &m_DummyRigidBody;
		Constraint.m_pRigidBodyState1 = NULL;
		Constraint.m_pRigidBodyState2 = NULL;
		m_lRigidConstraints.Add(Constraint);

		AddConnection(_pRigidBody, &m_DummyRigidBody);

		return m_lRigidConstraints.Len() - 1;
	}

	void CWorld::UpdateBallConstraint(int _iConstraint, const CVec3Dfp64& _WorldPos)
	{
		CRigidConstraint& Constraint = m_lRigidConstraints[_iConstraint];
		M_ASSERT(Constraint.m_Type == CRigidConstraint::BALLJOINTTOWORLD, "Invalid constraint type!");

		Constraint.SetVectorParam(CBallJointToWorldSolver::WORLDPOSITION, _WorldPos);
	}

	void CWorld::UpdateAxisConstraint(int _iConstraint, const CMat4Dfp64& _WorldPos, fp64 _AxisLength)
	{
		CRigidConstraint& Constraint = m_lRigidConstraints[_iConstraint];
		M_ASSERT(Constraint.m_Type == CRigidConstraint::AXISCONSTRAINT, "Invalid constraint type!");

		CVec3Dfp64 Anchor = _WorldPos.GetRow(3);
		CVec3Dfp64 Axis = _WorldPos.GetRow(2) * (_AxisLength *  0.5f);
		Constraint.SetVectorParam(CAxisConstraintSolver::WORLDPOSITION1, Anchor + Axis);
		Constraint.SetVectorParam(CAxisConstraintSolver::WORLDPOSITION2, Anchor - Axis);

		fp64 AnchorToObjectDistance = Anchor.Distance( Constraint.m_pRigidBody1->GetPosition() );
		CVec3Dfp64 Dir = _WorldPos.GetRow(0) * -AnchorToObjectDistance;
		Constraint.SetVectorParam(CAxisConstraintSolver::MAXDISTANCEPOINT, Anchor + Dir);
	}

	int CWorld::AddMaxDistanceConstraint(CRigidBody *_pRigidBody1, CRigidBody *_pRigidBody2, const CVec3Dfp64& _LocalPosition1, const CVec3Dfp64& _LocalPosition2, fp64 _MaxDistance)
	{
		CRigidConstraint Constraint;
		Constraint.m_Type = CRigidConstraint::MAXDISTANCECONSTRAINT;
		Constraint.m_VectorParams[CMaxDistanceConstraintSolver::LOCALPOSITION1] = _LocalPosition1;
		Constraint.m_VectorParams[CMaxDistanceConstraintSolver::LOCALPOSITION2] = _LocalPosition2;
		Constraint.m_ScalarParams[CMaxDistanceConstraintSolver::MAXDISTANCE] = _MaxDistance;
		Constraint.m_pRigidBody1 = _pRigidBody1;
		Constraint.m_pRigidBody2 = _pRigidBody2;
		Constraint.m_pRigidBodyState1 = NULL;
		Constraint.m_pRigidBodyState2 = NULL;
		m_lRigidConstraints.Add(Constraint);

		AddConnection(_pRigidBody1, _pRigidBody2);
		AddConnection(_pRigidBody2, _pRigidBody1);

		return m_lRigidConstraints.Len() - 1;
	}

	int CWorld::AddMinDistanceConstraint(CRigidBody *_pRigidBody1, CRigidBody *_pRigidBody2, const CVec3Dfp64& _LocalPosition1, const CVec3Dfp64& _LocalPosition2, fp64 _MinDistance)
	{
		CRigidConstraint Constraint;
		Constraint.m_Type = CRigidConstraint::MINDISTANCECONSTRAINT;
		Constraint.m_VectorParams[CMinDistanceConstraintSolver::LOCALPOSITION1] = _LocalPosition1;
		Constraint.m_VectorParams[CMinDistanceConstraintSolver::LOCALPOSITION2] = _LocalPosition2;
		Constraint.m_ScalarParams[CMinDistanceConstraintSolver::MINDISTANCE] = _MinDistance;
		Constraint.m_pRigidBody1 = _pRigidBody1;
		Constraint.m_pRigidBody2 = _pRigidBody2;
		Constraint.m_pRigidBodyState1 = NULL;
		Constraint.m_pRigidBodyState2 = NULL;
		m_lRigidConstraints.Add(Constraint);

		AddConnection(_pRigidBody1, _pRigidBody2);
		AddConnection(_pRigidBody2, _pRigidBody1);

		return m_lRigidConstraints.Len() - 1;
	}

	void CWorld::RemoveRigidConstraint(int _iConstraint)
	{
		CRigidConstraint* pConstraint = &m_lRigidConstraints[_iConstraint];
		if (pConstraint->m_Type != -1)
		{
			RemoveConnection(pConstraint->m_pRigidBody1, pConstraint->m_pRigidBody2);

			pConstraint->m_Type = -1;
			pConstraint->m_pRigidBody1 = NULL;
			pConstraint->m_pRigidBody2 = NULL;
		}
	}

	void CWorld::GetConnectedObjects(int _iConstraint, CRigidBody **_pRigidBody1, CRigidBody **_pRigidBody2)
	{
		const CRigidConstraint& Constraint = m_lRigidConstraints[_iConstraint];
		if (Constraint.m_Type == -1)
		{
			*_pRigidBody1 = *_pRigidBody2 = NULL;
			return;
		}
		CRigidBody* pRB1 = Constraint.m_pRigidBody1;
		CRigidBody* pRB2 = Constraint.m_pRigidBody2;
		*_pRigidBody1 = (pRB1 == &m_DummyRigidBody) ? NULL : pRB1;
		*_pRigidBody2 = (pRB2 == &m_DummyRigidBody) ? NULL : pRB2;
	}

	// Used to move constraint from a destroyed object to a spawned sub-object
	//
	// TODO: need to compensate for different mass centras in old vs new object...
	// 
	void CWorld::UpdateConnectedObject(int _iConstraint, CRigidBody* _pOld, CRigidBody* _pNew)
	{
		CRigidConstraint& Constraint = m_lRigidConstraints[_iConstraint];
		M_ASSERT(Constraint.m_pRigidBody1 == _pOld || Constraint.m_pRigidBody2 == _pOld, "Constraint not connected to old object!");

		if (Constraint.m_pRigidBody1 == _pOld)
		{
			Constraint.m_pRigidBody1 = _pNew;
			RemoveConnection(Constraint.m_pRigidBody2, _pOld);
			AddConnection(Constraint.m_pRigidBody2, _pNew);
			AddConnection(_pNew, Constraint.m_pRigidBody2);
		}
		else
		{
			Constraint.m_pRigidBody2 = _pNew;
			RemoveConnection(Constraint.m_pRigidBody1, _pOld);
			AddConnection(Constraint.m_pRigidBody1, _pNew);
			AddConnection(_pNew, Constraint.m_pRigidBody1);
		}
	}

	fp64 CWorld::GetSystemMass(const CRigidBody *_pRigidBody)
	{
		DummySet<CRigidBody *, 16> Set;
		return DoGetSystemMass(_pRigidBody, Set) + _pRigidBody->m_pstate->m_mass;
	}

	fp64 CWorld::DoGetSystemMass(const CRigidBody *_pRigidBody, DummySet<CRigidBody *, 16>& _Set)
	{
		fp32 m = 0.0;
		for (int i = 0; i < DYNAMICS_MAX_CONNECT_COUNT; i++)
		{
			if (_pRigidBody->m_pConnectedTo[i] != NULL)
			{
				CRigidBody *pR = _pRigidBody->m_pConnectedTo[i];

				if (!_Set.Contains(pR) && pR != &m_DummyRigidBody)
				{
					m += pR->m_pstate->m_mass;

					if (_Set.Add(pR))
					{
						m += DoGetSystemMass(pR, _Set);
					}
				}				
			}
		}

		return m;
	}

	const TArray<CRigidBody *>& CWorld::GetRigidBodyList() const {
		return m_rigidbodylist;
	}

	TArray<CRigidBody *>& CWorld::GetRigidBodyList() {
		return m_rigidbodylist;
	}


	/*
	TODO: !!! Que??? Ska det inte vara en referens här?
	*/
	/*
	const TArray<CRigidBodyGroup *> CWorld::GetGroups() {
	return m_rigidbodygrouplist;
	}*/

	const TArray<CConstraint *>& CWorld::GetConstraints() {
		return m_constraintlist;
	}
	/*
	CVec3Dfp64 GetVelocityAtTest(CRigidBody& rb, const CVec3Dfp64& point ) {
	CVec3Dfp64 vel;
	CVec3Dfp64 velocity= rb.m_state.m_velocity*(1.0/rb.m_state.m_mass);
	velocity*=1.0/rb.m_state.m_mass;
	CMat4Dfp64 t;
	rb.GetWorldInertiaTensorInvert(t);
	CVec3Dfp64 angularvelocity=  (rb.m_state.m_angularvelocity * (1.0/rb.m_state.m_mass)) * t;

	angularvelocity.CrossProd(point - rb.m_state.m_position, vel);
	vel+= velocity;
	return vel;
	//		vel+= m_state.m_momentum/m_state.m_mass;
	}*/

	fp64 CWorld::CalculateImpulseDenominator(const CContactInfo& ci) {
		CRigidBody *rb1= ci.m_pRigidBody1->GetRigidBody();
		CRigidBody *rb2= ci.m_pRigidBody2->GetRigidBody();

		//		CRigidBodyState *pstate= m_bodystate.GetBasePtr();
		const CRigidBodyState *rb1state= ci.m_pRigidBody1->GetBodyState();
		const CRigidBodyState *rb2state= ci.m_pRigidBody2->GetBodyState();

		//		CRigidBodyState *rb1state= &pstate[rb1->m_id];
		//		CRigidBodyState *rb2state= &pstate[rb2->m_id];

		double B1 = 1.0 / rb1state->m_mass;
		double B2 = 1.0 / rb2state->m_mass;

		CVec3Dfp64 n = ci.m_Normal;
		CVec3Dfp64 tmp,tmp2;

		ci.m_ra.CrossProd(n,tmp);
		tmp.MultiplyMatrix(rb1->GetWorldInertiaTensorInvert(),tmp2);
		tmp2.CrossProd(ci.m_ra,tmp);
		double B3 = n*tmp;

		ci.m_rb.CrossProd(n,tmp);
		tmp.MultiplyMatrix(rb2->GetWorldInertiaTensorInvert(),tmp2);
		tmp2.CrossProd(ci.m_rb,tmp);
		double B4 = n*tmp;

		return B1 + B2 + B3 + B4;
	}

	void CWorld::CheckStationary(fp64 _dt)
	{
#ifndef DYNAMICS_DISABLE_SCOPES
		MSCOPESHORT(CWorld::CheckStationary);
#endif

		TArray<CRigidBody *>& BodyList = GetRigidBodyList();
		CRigidBody **pBodyList = BodyList.GetBasePtr();
		int nObjects = BodyList.Len();

		for (int i = 0 ;i < nObjects; i++)
		{
			CRigidBody *pRigidBody = pBodyList[i];

			if (!pRigidBody->IsActive()) continue;

			fp64 FreezeEnergyThreshold = GetFreezeEnergyThreshold(pRigidBody, _dt);
			fp64 KineticEnergy = GetKineticEnergy(pRigidBody);
			if (pRigidBody->IsStationary())
			{
				if (KineticEnergy > FreezeEnergyThreshold)
				{
					m_lJustUnfreezedObjects.Add(pRigidBody);

					pRigidBody->SetStationary(false);
					pRigidBody->m_FreezeCounter = 0;
				}
				else
				{
					pRigidBody->SetVelocity(CVec3Dfp64(0,0,0));
					pRigidBody->SetAngularVelocity(CVec3Dfp64(0,0,0));
				}
			}
			else
			{
				if (KineticEnergy < FreezeEnergyThreshold)
				{
					pRigidBody->m_FreezeCounter++;
					if (pRigidBody->m_FreezeCounter > 20)
					{
						pRigidBody->SetStationary(true);
					}
				}
				else 
				{
					pRigidBody->m_FreezeCounter = 0;
				}
			}
		}
	}

	void CWorld::HandleCollision(CContactInfo& ci, fp64 epsilon) 
	{
		MSCOPESHORT(CWorld::HandleCollision);

		CRigidBody *shape1= ci.m_pRigidBody1;
		CRigidBody *shape2= ci.m_pRigidBody2;
		CRigidBody *rb1= shape1->GetRigidBody();
		CRigidBody *rb2= shape2->GetRigidBody();
		/*		CRigidBodyState *pstate= m_bodystate.GetBasePtr();
		CRigidBodyState *rb1state= &pstate[rb1->m_id];
		CRigidBodyState *rb2state= &pstate[rb2->m_id];*/
		CRigidBodyState *rb1state= ci.m_pRigidBody1->GetBodyState();
		CRigidBodyState *rb2state= ci.m_pRigidBody2->GetBodyState();

		fp64 rb1mass= rb1state->m_mass;
		fp64 rb2mass= rb2state->m_mass;
		fp64 rb1massinv= 1.0/rb1mass;
		fp64 rb2massinv= 1.0/rb2mass;

		CVec3Dfp64 ra = ci.m_ra;
		CVec3Dfp64 rb = ci.m_rb;

		const CMat4Dfp64 &rb1worldinv= rb1->GetWorldInertiaTensorInvert();
		//		rb1->GetWorldInertiaTensorInvert(rb1worldinv);

		const CMat4Dfp64 &rb2worldinv= rb2->GetWorldInertiaTensorInvert();
		//		rb2->GetWorldInertiaTensorInvert(rb2worldinv);


		// Budget depenetration.
		// TODO: Borde ske "globalt", dvs på alla kollision
		// eftersom djupet på andra kollisioner påverkas av en ändring i
		// en enstaka

#if 0
		CVec3Dfp64 move = (ci.m_Normal * ci.m_distance * 0.5 * 0.02*5*4);
		//		CVec3Dfp64 move = (ci.m_Normal * ci.m_distance * 0.5);

		if (rb1->IsActive()) {
			rb1->SetPosition(rb1->GetPosition() + move);
		}

		if (rb2->IsActive()) {
			rb2->SetPosition(rb2->GetPosition() - move);
		}
#endif

		double A = -(1.0 + epsilon) * ci.GetRelativeVelocity();
		//double A = -(1.0 + epsilon) * ci.GetRelativeVelocity() * m_ScaleFactor;
		//		double A = -(1.0 + epsilon) * (ci.m_Normal*(rb2state->m_velocity - rb1state->m_velocity));		
		double B1 = rb1massinv;
		double B2 = rb2massinv;

		CVec3Dfp64 n = ci.m_Normal;
		CVec3Dfp64 tmp, tmp2;

		/*
		if (!rb1->IsActive())
		B3= 0.0;

		if (!rb2->IsActive())
		B4= 0.0;
		*/

		//double impulse= ci.m_impulse;
		double B= ci.m_collision_impulse_denominator;
		//		impulse= CalculateImpulse(ci,epsilon);
		double impulse = A / (B);


		/*
		TODO: !!!
		if (ci->insideout == false) {
		double d = ci->distance;
		if (impulse < 0)
		d *= -1;

		double mom = d / (dt);
		//impulse+= mom;
		}
		*/

		//impulse *= ci->scale;

		// TODO: Varför berättas det fel tecken?!??!?!
		impulse*=-1;

		ci.m_MaxAppliedImpulse = Max(ci.m_MaxAppliedImpulse, Abs(impulse));

		CVec3Dfp64 force= n*impulse;

		//if (!(rb1->IsStationary() && rb2->IsStationary())) {
		if (rb1state->m_active) {
//			ci.m_pRigidBody1->m_TotalAppliedImpulse += Abs(impulse);

			rb1state->m_velocity+= force * (rb1massinv);
			ra.CrossProd(force,tmp);
			//			ci.m_ra.CrossProd(force,tmp);
			//tmp = ci.m_ra / force;

			// TODO: !!!! Är inte detta fel, dvs ska det inte vara tensorn här!?!?!
			// Alternativt kan man byta till att använda rörelsemängdsmoment som 
			// tillståndsvariabel istället för rotationshastighet.
			// Samma nedan


#if 1
			tmp.MultiplyMatrix(rb1worldinv,tmp2);
			rb1state->m_angularvelocity+= tmp2;
#else
			rb1state->m_angularvelocity+= tmp * (rb1massinv);
#endif
		}

		force *= -1.0;
		if (rb2state->m_active) {
//			ci.m_pRigidBody2->m_TotalAppliedImpulse += Abs(impulse);

			rb2state->m_velocity+= force * (rb2massinv);
			//			ci.m_rb.CrossProd(force,tmp);
			rb.CrossProd(force,tmp);
			//			tmp = ci.m_rb / force;

#if 1
			tmp.MultiplyMatrix(rb2worldinv,tmp2);
			rb2state->m_angularvelocity+= tmp2;
#else
			rb2state->m_angularvelocity+= tmp * (rb2massinv);
#endif
		}


		/*
		OLD AND SLOW
		rb1->AddMomentum(force);
		tmp = ci.m_ra / force;
		rb1->AddAngularMomentum(tmp);
		force *= -1.0;
		rb2->AddMomentum(force);
		tmp = ci.m_rb / force;
		rb2->AddAngularMomentum(tmp);*/
		//}

		// Friktionstest


		//		CVec3Dfp64 v1= GetVelocityAtTest(*rb1,ci.m_PointOfCollision);
		//		CVec3Dfp64 v2= GetVelocityAtTest(*rb2,ci.m_PointOfCollision);

		rb1->UpdateState();
		rb2->UpdateState();

		//		CVec3Dfp64 v1= rb1->GetVelocityAt(ci.m_PointOfCollision);
		//		CVec3Dfp64 v2= rb2->GetVelocityAt(ci.m_PointOfCollision);
		CVec3Dfp64 v1;
		CVec3Dfp64 v2;

		rb1state->m_angularvelocity.CrossProd(ci.m_PointOfCollision - rb1state->m_position*m_ScaleFactor,v1);
		v1+= rb1state->m_velocity;

		rb2state->m_angularvelocity.CrossProd(ci.m_PointOfCollision - rb2state->m_position*m_ScaleFactor,v2);
		v2+= rb2state->m_velocity;


		CVec3Dfp64 nn= -ci.m_Normal;
		CVec3Dfp64 relativetangentvelocity= 
			(v2-v1) - (ci.m_Normal * ((v2-v1)*ci.m_Normal));

		// TODO: Tecken här???
		CVec3Dfp64 T= relativetangentvelocity;
		T.Normalize();
		double Af = relativetangentvelocity.Length();

		//		tmp = ci.m_ra/T;
		tmp = ra/T;
		//rb1->GetWorldInertiaTensorInvert(tmpmat);
		//tmp= MatMul(tmp, tmpmat);
		tmp.MultiplyMatrix(rb1worldinv,tmp2);
		//tmp= tmp * rb1worldinv;
		//		tmp2.CrossProd(ci.m_ra,tmp);
		tmp2.CrossProd(ra,tmp);
		//tmp = tmp / ci.m_ra;
		double B3f = T*tmp;

		//		tmp = ci.m_rb/T;
		tmp = rb/T;
		//rb2->GetWorldInertiaTensorInvert(tmpmat);
		//tmp= MatMul(tmp, tmpmat);
		tmp.MultiplyMatrix(rb2worldinv,tmp2);
		//tmp= tmp*rb2worldinv;
		//tmp = tmp / ci.m_rb;
		//		tmp2.CrossProd(ci.m_rb,tmp);
		tmp2.CrossProd(rb,tmp);
		double B4f = T*tmp;

		/*

		if (!rb1->IsActive())
		B3f= 0.0;

		if (!rb2->IsActive())
		B4f= 0.0;
		*/

		double restimpulse = Af / (B1 + B2 + B3f + B4f);
		//		restimpulse*=-1;

		{
			//			if (relativetangentvelocity.Length() > 0.0) {

			//cout << relativetangentvelocity.GetString().Str() << endl;
			//cout << restimpulse << endl;

			// 0.3 och 0.5 funkar bra

			// TODO: SKA ENENTLIGEN VARA MIN HÄR!
			const fp64 DynamicFriction = Min(ci.m_pRigidBody1->GetDynamicFriction(), ci.m_pRigidBody2->GetDynamicFriction());
			const fp64 StaticFriction = Min(ci.m_pRigidBody1->GetStaticFriction(), ci.m_pRigidBody2->GetStaticFriction());

			fp64 frictionimpulse= DynamicFriction *  impulse;
			fp64 staticfrictionimpulse= StaticFriction * impulse;
//			fp64 frictionimpulse= 0.3*impulse;
//			fp64 staticfrictionimpulse= 0.4*impulse;
			bool restimpulseapplied= false;

			/*	
			TODO: !!!!!
			Ska man inte jämföra absolutbelopp här???
			Kan inte "restimpulse" ha omvänt tecken till "staticfrictionimpulse"?
			*/

			fp64 impulsetoapply= 0.0;
			if (restimpulse < staticfrictionimpulse) {
				impulsetoapply= restimpulse;		
				restimpulseapplied= true;
			}
			else {
				impulsetoapply= frictionimpulse;				

				/*					if (frictionimpulse < 0.01)
				impulsetoapply= restimpulse*0.5;
				else
				impulsetoapply= frictionimpulse;				*/
			}

			/*				if (impulsetoapply < 0.001)
			impulsetoapply= 0;
			*/
			CVec3Dfp64 frictionforce= T*impulsetoapply;

			// Här berättas också fel tecken...
			//frictionforce *= -1.0;

#if 1
			//	if (!(rb1->IsStationary() && rb2->IsStationary())) {

			//				if (!(rb1->IsStationary() && restimpulseapplied)) {

			if (rb1state->m_active) {
				rb1state->m_velocity+= frictionforce * (rb1massinv);
				//					ci.m_ra.CrossProd(frictionforce,tmp);
				ra.CrossProd(frictionforce,tmp);
				//tmp = ci.m_ra / frictionforce;
#if 1
				tmp.MultiplyMatrix(rb1worldinv,tmp2);
				rb1state->m_angularvelocity+= tmp2;
#else
				rb1state->m_angularvelocity+= tmp * (rb1massinv);
#endif
			}

			frictionforce *= -1.0;
			if (rb2state->m_active) {
				rb2state->m_velocity+= frictionforce * (rb2massinv);
				//					ci.m_rb.CrossProd(frictionforce,tmp);
				rb.CrossProd(frictionforce,tmp);
				//tmp = ci.m_rb / frictionforce;
#if 1
				tmp.MultiplyMatrix(rb2worldinv,tmp2);
				rb2state->m_angularvelocity+= tmp2;
#else
				rb2state->m_angularvelocity+= tmp * (rb2massinv);
#endif
			}


			/*
			OLD AND SLOW
			rb1->AddMomentum(frictionforce);
			tmp = ci.m_ra / frictionforce;
			rb1->AddAngularMomentum(tmp);
			//				}

			frictionforce *= -1.0;

			//				if (!(rb2->IsStationary() && restimpulseapplied)) {
			rb2->AddMomentum(frictionforce);
			tmp = ci.m_rb / frictionforce;
			rb2->AddAngularMomentum(tmp);
			//				}
			//}
			*/
#endif

			//	}
		}

		//rb1->UpdateState();
		//rb2->UpdateState();

#if 0
		//rb1->CheckStationary();
		//rb2->CheckStationary();

		/*
		if (rb1->m_state.m_momentum.Length() > 0.1) {
		rb1->SetStationary(false);
		rb2->SetStationary(false);
		}

		if (rb2->m_state.m_momentum.Length() > 0.1) {
		rb1->SetStationary(false);
		rb2->SetStationary(false);
		}
		*/
		if (!rb1->IsStationary() && rb1->IsActive())
			rb2->SetStationary(false);

		if (!rb2->IsStationary() && rb2->IsActive())
			rb1->SetStationary(false);

		/*
		if (!rb1->IsStationary() && rb1->IsActive())
		//			if (impulse > 0.1)
		rb2->SetStationary(false);

		if (!rb2->IsStationary() && rb2->IsActive())
		//			if (impulse > 0.1)
		rb1->SetStationary(false);
		*/

#endif

	}

	class GeometryIterator {

		CRigidBody *next();
	};

	int CWorld::ProcessCollisions(TArray<CContactInfo>& collisioninfolist,
		int ncollisions,
		fp64 dt, 
		fp64 epsilon, 
		bool downorder) 
	{
		MSCOPESHORT(CWorld::ProcessCollisions);

		int n=0;
		CContactInfo *pcollisioninfolist = collisioninfolist.GetBasePtr();
		for (int i=0; i<ncollisions; i++) 
		{
			int index=i;
			if (downorder) {
				index= ncollisions-i-1;
			}
			// GetRelativeVelocity verkar returnerna masscentrums relativa hastighet
			// och inte kollisionspunkterna!!!
			if (pcollisioninfolist[index].GetRelativeVelocity() > 0) 
			{
				if (epsilon < -0.1)
				{
					const CContactInfo& CI = pcollisioninfolist[index];
					epsilon = Min(CI.m_pRigidBody1->m_CoefficientOfRestitution, CI.m_pRigidBody1->m_CoefficientOfRestitution);
				}

				HandleCollision(pcollisioninfolist[index],epsilon);
				n++;
			}
		}
		return n;
	}

		/*	int CWorld::GetRigidBodyID() {
		m_bodystate.Add(CRigidBodyState());
		m_savedbodystate.Add(CRigidBodyState());
		CRigidBody *tmp= NULL;
		m_rigidbodylist.Add(tmp);
		return m_nextid++;
		}
		*/
		void CWorld::UpdatePosition(fp64 dt) {
			int n= m_bodystate.Len();
			CRigidBodyState *pstate= m_bodystate.GetBasePtr();
			for (int i=0; i<n; i++) {
				CRigidBodyState *p= &pstate[i];
				if (p->m_active && !p->m_Stationary) {
					p->m_position.k[0]+= (p->m_velocity.k[0])*dt * m_ScaleFactorInv;
					p->m_position.k[1]+= (p->m_velocity.k[1])*dt * m_ScaleFactorInv;
					p->m_position.k[2]+= (p->m_velocity.k[2])*dt * m_ScaleFactorInv;
					//				p->m_position+= (p->m_velocity)*dt;
				}
			}
		}

		void CWorld::UpdatePositionSubdiv(fp64 _dt)
		{
			CBox3Dfp64 Box;

			TArray<CRigidBody *> lBodyList;
			lBodyList.SetLen(1);
			
			int n= m_bodystate.Len();
			CRigidBodyState *pstate= m_bodystate.GetBasePtr();
			for (int i=0; i<n; i++) 
			{
				CRigidBodyState *p= &pstate[i];
				CRigidBody *pRB = m_rigidbodylist[i];
				if (p->m_active && !p->m_Stationary) 
				{
					m_pWorldCollider->GetBoundingBox(pRB, Box, m_pColliderArgument1, m_pColliderArgument2);
					CVec3Dfp64 Extent = Box.m_Max - Box.m_Min;

					fp64 MinE = Min<fp64>(Min<fp64>(Extent[0], Extent[1]), Extent[2]);
					fp64 PosDiff = p->m_velocity.Length() * _dt * m_ScaleFactorInv;
					if (PosDiff < MinE * 0.5)
					{
						p->m_position.k[0]+= (p->m_velocity.k[0]) * _dt * m_ScaleFactorInv;
						p->m_position.k[1]+= (p->m_velocity.k[1]) * _dt * m_ScaleFactorInv;
						p->m_position.k[2]+= (p->m_velocity.k[2]) * _dt * m_ScaleFactorInv;
					}
					else
					{
						int nSteps = (int)(PosDiff / (MinE * 0.5));
						fp64 StepFactor = 1.0 / nSteps;
						for (int j = 0; j < nSteps; j++)
						{
							p->m_position.k[0]+= (p->m_velocity.k[0]) * _dt * m_ScaleFactorInv * StepFactor;
							p->m_position.k[1]+= (p->m_velocity.k[1]) * _dt * m_ScaleFactorInv * StepFactor;
							p->m_position.k[2]+= (p->m_velocity.k[2]) * _dt * m_ScaleFactorInv * StepFactor;

							lBodyList[0] = pRB;
							if (m_pWorldCollider->Collide(this, lBodyList, NULL, 0, m_pColliderArgument1, m_pColliderArgument2))
								break;
						}
					}
				}
			}
		}

		// TODO: Kolla att denna verkligen stämmer...

		M_INLINE void CreateMatrix(const CQuatfp64& q, CMat4Dfp64& m) {
			fp64 xs,ys,zs,wx,wy,wz,xx,xy,xz,yy,yz,zz;
			fp64 s =2.0;

			xs = q.k[0]*s;  ys = q.k[1]*s;  zs = q.k[2]*s;
			wx = q.k[3]*xs; wy = q.k[3]*ys; wz = q.k[3]*zs;
			xx = q.k[0]*xs; xy = q.k[0]*ys; xz = q.k[0]*zs;
			yy = q.k[1]*ys; yz = q.k[1]*zs; zz = q.k[2]*zs;

			m.k[0][0] = (1.0 - (yy+zz));
			m.k[0][1] = (xy-wz);
			m.k[0][2] = (xz+wy);

			m.k[1][0] = (xy+wz);
			m.k[1][1] = (1.0 - (xx+zz));
			m.k[1][2] = (yz-wx);

			m.k[2][0] = (xz-wy);
			m.k[2][1] = (yz+wx);
			m.k[2][2] = (1.0 - (xx+yy));

			m.k[0][3]= 0;
			m.k[1][3]= 0;
			m.k[2][3]= 0;
			//m.k[3][3]= 1;


			m.k[3][0]= 0;
			m.k[3][1]= 0;
			m.k[3][2]= 0;
			m.k[3][3]= 1;


		}

		void Multiply(const CMat4Dfp64& m1, const CMat4Dfp64& m2, CMat4Dfp64& DestMat) 
		{
			// 64 mul, 48 add, 16 write
			for (int row=0; row<4; row++)
				for (int kol=0; kol<4; kol++)
					DestMat.k[row][kol] = 
					(m1.k[row][0]*m2.k[0][kol]) + 
					(m1.k[row][1]*m2.k[1][kol]) + 
					(m1.k[row][2]*m2.k[2][kol]) + 
					(m1.k[row][3]*m2.k[3][kol]);
		}

		void CWorld::UpdateVelocity(fp64 dt) 
		{
			MSCOPESHORT(CWorld::UpdateVelocity);

			int n= m_bodystate.Len();
			CRigidBodyState *pstate= m_bodystate.GetBasePtr();
			CRigidBody **pgeom= m_rigidbodylist.GetBasePtr();
			CMat4Dfp64 t;
			CMat4Dfp64 rot;
			for (int i=0; i<n; i++) {
				CRigidBodyState *p= &pstate[i];

				if (p->m_active && !p->m_Stationary) 
				{
					p->m_velocity+= p->m_force*(dt * (1.0/p->m_mass));

		// **** SAFETY PRECAUTION **** //
					if (p->m_velocity.LengthSqr() > Sqr(35.0f))
						p->m_velocity.SetLength(35.0f); 
		// **** SAFETY PRECAUTION **** //

					if (p->m_torque.Length() > 0.001)
					{
						CVec3Dfp64 tmp;
						p->m_torque.MultiplyMatrix(pgeom[i]->GetWorldInertiaTensorInvert(),tmp);
 						tmp *= dt;
						p->m_angularvelocity += tmp;
					}

					rot= p->m_matrix_orientation;

					CMat4Dfp64 tmp; //= m_tensorInverted * rot;
					CRigidBody *foo= pgeom[i];
					if (foo == NULL) {
						// TODO: Budget!!! Saker och ting behandlas fel...
						// Alla behandlas trots att de inte är tillagde med AddGeom...
						continue;
					}

					CVec3Dfp64 foobar= pgeom[i]->m_tensorInverted;
					tmp= MatMul(rot,pgeom[i]->m_tensorInverted);
					rot.Transpose();

					// TODO: Hur ofta ger man objekt kraftmoment...
					Multiply(tmp,rot,t);
					//p->m_angularvelocity+= (p->m_torque*dt)*t;
				}
			}
		}

		void CWorld::UpdateRotation(fp64 dt) 
		{
			MSCOPESHORT(CWorld::UpdateRotation);

			int n= m_bodystate.Len();
			CRigidBodyState *pstate= m_bodystate.GetBasePtr();
			CRigidBody **pgeom= m_rigidbodylist.GetBasePtr();

			CQuatfp64 v;
			CVec3Dfp64 tmpangvel;
			CQuatfp64 drot;

			for (int i=0; i<n; i++) {
				CRigidBodyState *p= &pstate[i];

				if (p->m_active && !p->m_Stationary) {

					tmpangvel= p->m_angularvelocity;
					v.k[0]= tmpangvel.k[0];
					v.k[1]= tmpangvel.k[1];
					v.k[2]= tmpangvel.k[2];
					v.k[3]= 0;

					drot= v*p->m_orientation;
					// TODO: Fixa detta i TQuat?
					drot.k[0]*= 0.5*dt;
					drot.k[1]*= 0.5*dt;
					drot.k[2]*= 0.5*dt;
					drot.k[3]*= 0.5*dt;
					// TODO: Fixa detta i TQuat?
					p->m_orientation.k[0]+= drot.k[0];
					p->m_orientation.k[1]+= drot.k[1];
					p->m_orientation.k[2]+= drot.k[2];
					p->m_orientation.k[3]+= drot.k[3];
					p->m_orientation.Normalize();
					p->m_orientation.CreateMatrix(p->m_matrix_orientation);

					if (pgeom[i] != NULL) {
						pgeom[i]->UpdateWorldInertiaTensorInvert();
					}
				}
			}
		}

#if 0
		void CWorld::UpdateBoundingSpheres() {
			int n= m_rigidbodylist.Len();
			CRigidBody **prigidbodylist= m_rigidbodylist.GetBasePtr();
			CRigidBodyState *pstate= m_bodystate.GetBasePtr();

			for (int i=0; i<n; i++) {
				CRigidBody *pgeom= prigidbodylist[i];
				// TODO: Hmmm måste fixa detta med att denna kan vara NULL...
				if (pgeom) {
					/*
					if (pgeom->m_GeometryType == GEOM_BOX) {				
					pgeom->m_boundingsphere.m_center= pstate[pgeom->m_id].m_position;
					}
					else if (pgeom->m_GeometryType == GEOM_SPHERE) {
					CDynamicsSphere *sphere= (CDynamicsSphere *) pgeom;
					pgeom->m_boundingsphere= CBoundingSphere(pstate[pgeom->m_id].m_position,sphere->m_r);
					}
					else {
					pgeom->UpdateBoundingSphere();
					}*/

					pgeom->UpdateBoundingSphere();
				}
			}
		}
#endif

		void CWorld::ClearForces() {
			int n= m_bodystate.Len();
			CRigidBodyState *pstate= m_bodystate.GetBasePtr();
			for (int i=0; i<n; i++) 
			{
				pstate[i].m_force= CVec3Dfp64(0,0,0);
				pstate[i].m_torque= CVec3Dfp64(0,0,0);
			}
		}

		void CWorld::AddExternalForces()
		{
			TAP<CRigidBody *> tapRigidBodies = m_rigidbodylist;

			for (int i = 0; i < tapRigidBodies.Len(); i++)
			{
				CRigidBody *pRB = tapRigidBodies[i];
				CRigidBodyState *pState = pRB->m_pstate;

				pState->m_force += pRB->m_ExternalForce;
				pState->m_torque += pRB->m_ExternalTorque;
			}
		}

		void CWorld::ClearExternalForces()
		{
			TAP<CRigidBody *> tapRigidBodies = m_rigidbodylist;

			for (int i = 0; i < tapRigidBodies.Len(); i++)
			{
				CRigidBody *pRB = tapRigidBodies[i];

				pRB->m_ExternalForce = CVec3Dfp64(0.0f);
				pRB->m_ExternalTorque = CVec3Dfp64(0.0f);
			}
		}

		void CWorld::AddForces(fp64 dt) 
		{
			MSCOPESHORT(CWorld::AddForces);
			int n= m_bodystate.Len();
			CRigidBodyState *pstate= m_bodystate.GetBasePtr();
			for (int i=0; i<n; i++) 
			{
				pstate[i].m_force+= m_Gravity * pstate[i].m_mass;
			}
		}

		void CWorld::SaveFullState() {
			int n= m_bodystate.Len();
			CRigidBodyState *pstate= m_bodystate.GetBasePtr();
			CRigidBodyState *psavedstate= m_savedbodystate.GetBasePtr();
			for (int i=0; i<n; i++) {
				CRigidBodyState *p= &pstate[i];
				CRigidBodyState *ps= &psavedstate[i];
				ps->m_position= p->m_position;
				ps->m_orientation= p->m_orientation;
				ps->m_velocity= p->m_velocity;
				ps->m_angularvelocity= p->m_angularvelocity;
			}
		}

		void CWorld::RestoreVelocityState() {
			int n= m_bodystate.Len();
			CRigidBodyState *pstate= m_bodystate.GetBasePtr();
			CRigidBodyState *psavedstate= m_savedbodystate.GetBasePtr();
			for (int i=0; i<n; i++) {
				CRigidBodyState *p= &pstate[i];
				CRigidBodyState *ps= &psavedstate[i];
				p->m_velocity= ps->m_velocity;
				p->m_angularvelocity= ps->m_angularvelocity;
			}
		}

		void CWorld::RestorePositionOrientationState() {
			int n= m_bodystate.Len();
			CRigidBodyState *pstate= m_bodystate.GetBasePtr();
			CRigidBodyState *psavedstate= m_savedbodystate.GetBasePtr();
			for (int i=0; i<n; i++) {
				CRigidBodyState *p= &pstate[i];
				CRigidBodyState *ps= &psavedstate[i];
				p->m_position= ps->m_position;
				p->m_orientation= ps->m_orientation;
			}
		}


		const TArray<CCollisionEvent> CWorld::GetCollisionEvents()
		{
			return m_lCollisionEvents;
		}

                // TODO: O(n^2) 
		void CWorld::CollectCollisionEvents(const TArray<CContactInfo>& _lCollisionList, int _nCollisions)
		{
			fp64 MinEventImpulse = m_MinEventImpulse;
			fp64 MinCollectEventImpulse = MinEventImpulse / 5.0;
			int nMaxEvents = m_nMaxImpulseEvents;
			int nMaxTemporaryEvents = m_nMaxTemporaryImpulseEvents;
			int nTempEvents = 0;

			m_lCollisionEvents.SetLen(nMaxEvents);
			m_lTemporaryCollisionEvents.SetLen(nMaxTemporaryEvents);

			TAP_RCD<const CContactInfo> Collisions = _lCollisionList;
			TAP_RCD<CCollisionEvent> CollisionEvents = m_lCollisionEvents;
			TAP_RCD<CCollisionEvent> TempCollisionEvents = m_lTemporaryCollisionEvents;

			for (int i = 0; i < _nCollisions && nTempEvents < nMaxTemporaryEvents; i++)
			{
				CRigidBody* pBody1 = Collisions[i].m_pRigidBody1;
				CRigidBody* pBody2 = Collisions[i].m_pRigidBody2;

				if (pBody1 == &m_DummyRigidBody)
					pBody1 = NULL;

				if (pBody2 == &m_DummyRigidBody)
					pBody2 = NULL;

				fp64 MinMass = _FP64_MAX;
				if (pBody1 != NULL)
					MinMass = Min(MinMass, pBody1->GetMass());

				if (pBody2 != NULL)
					MinMass = Min(MinMass, pBody2->GetMass());

				if (Collisions[i].m_MaxAppliedImpulse / MinMass > MinCollectEventImpulse)
				{
					bool found = false;
					for (int j = 0; j < nTempEvents; j++)
					{
						CCollisionEvent& Event = TempCollisionEvents[j];
						if (Event.m_pRigidBodyA == pBody1 && Event.m_pRigidBodyB == pBody2 ||
						    Event.m_pRigidBodyA == pBody2 && Event.m_pRigidBodyB == pBody1)
						{
							Event.m_ImpulseTimesInvMass += Collisions[i].m_MaxAppliedImpulse / MinMass;
							found = true;
							break;
						}
					}

					if (!found)
					{
						TempCollisionEvents[nTempEvents].m_ImpulseTimesInvMass = Collisions[i].m_MaxAppliedImpulse / MinMass;
						TempCollisionEvents[nTempEvents].m_pRigidBodyA = pBody1;
						TempCollisionEvents[nTempEvents].m_pRigidBodyB = pBody2;
						TempCollisionEvents[nTempEvents].m_PointOfCollision = Collisions[i].m_PointOfCollision.Getfp32() * 32.0f;  // meters -> units
						TempCollisionEvents[nTempEvents].m_UserData1 = Collisions[i].m_UserData1;
						TempCollisionEvents[nTempEvents].m_UserData2 = Collisions[i].m_UserData2;
						nTempEvents++;
					}
				}
			}

			m_lTemporaryCollisionEvents.QuickSetLen(nTempEvents);

			int nEvents = m_nCollisionEvents;
//			fp32 SmallestImpulse = -1;
			for (int i = 0; i < nTempEvents; i++)
			{
				CCollisionEvent& Event = TempCollisionEvents[i];

				if (Event.m_ImpulseTimesInvMass > MinEventImpulse)
				{
					//M_TRACEALWAYS("ImpulseScaled: %f\n", Event.m_ImpulseTimesInvMass);

					if (nEvents < nMaxEvents)
					{
						CollisionEvents[nEvents] = Event;
						nEvents++;
					}
					else
					{
						for (int j = 0; j < nEvents; j++)
						{
							if (CollisionEvents[j].m_ImpulseTimesInvMass < Event.m_ImpulseTimesInvMass)
							{
								CollisionEvents[j] = Event;
								break;
							}
						}
					}
				}
			}

			m_nCollisionEvents = nEvents;
			m_lCollisionEvents.QuickSetLen(nEvents);
		}

#if 0
		CDynamicsCloth *gcloth= NULL;
#endif

#define N_SEPARATION_ITER (4)
#define SEPARATION_SPEED (0.1)
		// TODO: Stavas separate....
		// TODO: Borde man inte göra det "upp och ner"? (dvs fram bak, bak fram, osv)
		void CWorld::SeperateObjects(int ncollisions) 
		{
			MSCOPESHORT(CWorld::SeperateObjects);

			CContactInfo *pgcollisioninfolist= gcollisioninfolist.GetBasePtr();
			for (int k=0; k<N_SEPARATION_ITER; k++)
				for (int i=0; i<ncollisions; i++) {
					CContactInfo *ci;

#if 0
					if (k % 2 == 0)
						ci = &pgcollisioninfolist[i];
					else
						ci = &pgcollisioninfolist[ncollisions-i-1];
#else

					ci = &pgcollisioninfolist[i];
#endif

					CRigidBody *rb1= ci->m_pRigidBody1->GetRigidBody();
					CRigidBody *rb2= ci->m_pRigidBody2->GetRigidBody();

					M_ASSERT(rb1->m_childObject == false && rb2->m_childObject == false, "");

//					CRigidBodyState *pstate= m_bodystate.GetBasePtr();
//					CRigidBodyState *rb1state= &pstate[rb1->m_id];
//					CRigidBodyState *rb2state= &pstate[rb2->m_id];
					CRigidBodyState *rb1state= rb1->GetBodyState();
					CRigidBodyState *rb2state= rb2->GetBodyState();


					if (ci->m_distance < 0) continue;
					if (fabs(ci->m_distance) < 0.001) continue;

					if (ci->m_distance > 0.1) {
						if (ci->m_pRigidBody1->GetRigidBody()->m_id == 1 || 
							ci->m_pRigidBody2->GetRigidBody()->m_id == 1) {
								int foobar;
								foobar = 123;
							}
					}

					//			CVec3Dfp64 move = (ci->m_Normal * ci->m_distance * 0.5 * 0.02*5 );
					//				CVec3Dfp64 move = (ci->m_Normal * ci->m_distance * 0.5 / N_SEPARATION_ITER);
					fp64 movedist = ci->m_distance * 0.45 * SEPARATION_SPEED;
					CVec3Dfp64 move = (ci->m_Normal * movedist);

					// Hmmm vad händer med icke aktiva objekt?
					// Borde inte all förflyttning läggas på det ena?

					//ci->m_distance -= 2*movedist;
					ci->m_distance *= (1-2*0.45 * SEPARATION_SPEED);
					//ci->m_distance -= move * ci->m_Normal;


					if (!rb1state->m_active || !rb2state->m_active) {
						move*= 2;
					}

					if (rb1state->m_active) {
						rb1state->m_position+= move;
						rb1->m_stationary_movement+= move;
					}

					if (rb2state->m_active) {
						rb2state->m_position-= move;
						rb2->m_stationary_movement-= move;
					}

					// TODO: Milestonetest!
					//if (rb1 == &m_DummyRigidBody) continue;
					//if (rb2 == &m_DummyRigidBody) continue;

					/*
					if (rb1->IsActive()) {
					rb1->SetPosition(rb1->GetPosition() + move);
					rb1->m_stationary_movement+= move;
					}

					if (rb2->IsActive()) {
					rb2->SetPosition(rb2->GetPosition() - move);
					rb2->m_stationary_movement=- move;
					}*/
#if 1
#define MOVEPLUS ci2->m_distance+= move * ci2->m_Normal;
#define MOVEMINUS ci2->m_distance-= move * ci2->m_Normal;

					// TODO: Kan inte saker göras dubbelt här???
					int n=ci->m_pRigidBody1->m_nCollisions;
					//if (ci->m_pRigidBody1 != &m_DummyRigidBody)
					for (int j=0; j<n; j++) {
						CContactInfo *ci2= &pgcollisioninfolist[ci->m_pRigidBody1->m_iCollisions[j]];
						if (ci == ci2) continue;

						if (ci2->m_pRigidBody1 == ci->m_pRigidBody1 || ci2->m_pRigidBody1 == ci->m_pRigidBody2) {
							if (ci2->m_pRigidBody1 == ci->m_pRigidBody1)
								MOVEMINUS
								//ci2->m_distance-= movedist;
								//ci2->m_distance-= move * ci2->m_Normal;
							else
							MOVEPLUS
							//ci2->m_distance+= movedist;
							//ci2->m_distance+= move * ci2->m_Normal;
						}

						if (ci2->m_pRigidBody2 == ci->m_pRigidBody1 || ci2->m_pRigidBody2 == ci->m_pRigidBody2) {
							if (ci2->m_pRigidBody2 == ci->m_pRigidBody1)
								MOVEPLUS
								//ci2->m_distance+= movedist;
								//ci2->m_distance+= move * ci2->m_Normal;
							else
							MOVEMINUS
							//ci2->m_distance-= movedist;
							//ci2->m_distance-= move * ci2->m_Normal;
						}						
					}

					n=ci->m_pRigidBody2->m_nCollisions;
					//if (ci->m_pRigidBody2 != &m_DummyRigidBody)
					for (int j=0; j<n; j++) {
						CContactInfo *ci2= &pgcollisioninfolist[ci->m_pRigidBody2->m_iCollisions[j]];
						if (ci == ci2) continue;

						if (ci2->m_pRigidBody1 == ci->m_pRigidBody1 || ci2->m_pRigidBody1 == ci->m_pRigidBody2) {
							if (ci2->m_pRigidBody1 == ci->m_pRigidBody1)
								MOVEMINUS
								//ci2->m_distance-= movedist;
								//ci2->m_distance-= move * ci2->m_Normal;
							else
							MOVEPLUS
							//ci2->m_distance+= movedist;
							//ci2->m_distance+= move * ci2->m_Normal;
						}

						if (ci2->m_pRigidBody2 == ci->m_pRigidBody1 || ci2->m_pRigidBody2 == ci->m_pRigidBody2) {
							if (ci2->m_pRigidBody2 == ci->m_pRigidBody1)
								MOVEPLUS
								//ci2->m_distance+= movedist;
								//ci2->m_distance+= move * ci2->m_Normal;
							else
							MOVEMINUS
							//ci2->m_distance-= movedist;
							//ci2->m_distance-= move * ci2->m_Normal;
						}						
					}

					// OLD O(n^2)
#else
					for (int j=0; j<ncollisions; j++) {
						CContactInfo *ci2= &pgcollisioninfolist[j];

						if (ci == ci2) continue;

#if 1
						if (ci2->m_pRigidBody1 == ci->m_pRigidBody1 || ci2->m_pRigidBody1 == ci->m_pRigidBody2) {
							if (ci2->m_pRigidBody1 == ci->m_pRigidBody1)
								ci2->m_distance-= move * ci2->m_Normal;
							else
								ci2->m_distance+= move * ci2->m_Normal;
						}

						if (ci2->m_pRigidBody2 == ci->m_pRigidBody1 || ci2->m_pRigidBody2 == ci->m_pRigidBody2) {
							if (ci2->m_pRigidBody2 == ci->m_pRigidBody1)
								ci2->m_distance+= move * ci2->m_Normal;
							else
								ci2->m_distance-= move * ci2->m_Normal;

							//ci2->m_distance+= move * ci2->m_Normal;
						}
#endif

					}
#endif
				}
		}

		template<class T>
			void RandomizeVector(TArray<T>& vec, int n) {
				for (int i=0; i<100; i++) {
					int i1 = ((n-1)*MRTC_RAND())/RAND_MAX;
					int i2 = ((n-1)*MRTC_RAND())/RAND_MAX;

					T tmp = vec[i1];
					vec[i1] = vec[i2];
					vec[i2] = tmp;
				}

			}
			//#pragma optimize( "", off )
			//#pragma inline_depth(0)


			void CWorld::Simulate(fp64 dt, int nSteps, void *_pColliderArgument1, void *_pColliderArgument2,IDynamicsDebugRenderer *_pDebugRenderer)
			{
				m_pColliderArgument1 = _pColliderArgument1;
				m_pColliderArgument2 = _pColliderArgument2;

				m_nCollisionEvents = 0;
				m_bSimulating = true;
			
				for (int i = 0; i < nSteps; i++)
				{
					AddForces(dt);
					AddExternalForces();
					//bool Collect = i == 0 ? true : false;
					bool Collect = true;
					if (m_pWorldCollider)
						m_pWorldCollider->PreApplyExternalForces(_pColliderArgument1, _pColliderArgument2);

					Step(dt/nSteps, i, nSteps, _pColliderArgument1, _pColliderArgument2, _pDebugRenderer, Collect);
					ClearForces();
				}

				ClearExternalForces();

#ifdef DYNAMICS_FREEZE_OBJECTS
				CheckStationary(dt);
#endif

				m_bSimulating = false;
			}

			void CWorld::Step(fp64 dt, int _iStep, int _iSteps,
							  void *_pColliderArgument1, void *_pColliderArgument2,
							  IDynamicsDebugRenderer *_pDebugRenderer,
							  bool _CollectCollisionEvents)
			{
#ifndef DYNAMICS_DISABLE_SCOPES
				MSCOPESHORT(CWorld::Step);
#endif

				m_DummyRigidBody.m_nCollisions = 0;

#if 0
				for (int i=0; i<m_rigidbodylist.Len(); i++) {
					CRigidBody *rb = m_rigidbodylist[i]->GetRigidBody();
					m_bodystate[rb->m_id] = rb->m_state;		
				}
#endif

#if 0
				if (gcloth==NULL) {
					gcloth= DNew(CDynamicsCloth) CDynamicsCloth(this,7,7,20);
					//			gcloth= new CDynamicsCloth(this,7,7,6);
					AddRigidBody(gcloth);
				}
#endif

				int nintegrationsteps= 1;
				//CCollisionGrid collgrid(20,20,20,128,1);
				//		dynamicsstats.Clear();

				//TArray<CRigidBody *> rigidbodylist;
				//rigidbodylist.SetGrow(100);

				m_pWorldCollider->PreCollide(this);
				//UpdateBoundingSpheres();
				for (int i=0; i<m_rigidbodylist.Len(); i++) {

					//rigidbodylist.Add(grouplist[j]);
					// Gridtest
					//				collgrid.Insert(grouplist[j]);

#if 0
					if (!m_rigidbodylist[i]->m_PartOfCompound)
						m_collisionspace->Update(m_rigidbodylist[i]);
#endif

					/*	
					TODO: Lägg till en mera generell mekansim för att anropa
					en funktion varje tick...
					*/
					m_rigidbodylist[i]->m_nCollisions = 0;
					//m_rigidbodylist[i]->m_TotalAppliedImpulse = 0.0f;

					/*
					BROKEN...
					if (m_rigidbodylist[i]->GetRigidBodyType() == GEOM_COMPOUND) {
					CDynamicsCompound *compound = reinterpret_cast<CDynamicsCompound *>(m_rigidbodylist[i]);
					compound->UpdateChildrens();
					}*/

#ifdef CHECK_STATIONERY
					if (m_rigidbodylist[i]->GetRigidBody()->IsStationary()) {
						m_rigidbodylist[i]->GetRigidBody()->SetVelocity(0,0,0);
						m_rigidbodylist[i]->GetRigidBody()->SetAngularVelocity(0,0,0);
					}
#endif

#if 0
					if (i == 1)
						M_TRACEALWAYS("%s\n",m_rigidbodylist[i]->GetRigidBody()->GetOrientation().GetString().Str());
#endif 
				}

				//ClearForces();
				//AddForces(dt);
#define N_CONSTRAINT_ITERATIONS 10
#if 1
				for (int j=0; j<N_CONSTRAINT_ITERATIONS; j++) {
					for (int i=0; i<m_constraintlist.Len(); i++) {
						//int ncon= m_constraintlist.Len();
						CConstraint *constraint= m_constraintlist[i];
						constraint->Apply(dt);
						/*
						if (j % 2 == 0)
						m_constraintlist[i]->Apply(dt);
						else
						m_constraintlist[m_constraintlist.Len()-i-1]->Apply(dt);*/
					}
				}
#endif

				// RigidConstraints
//				int nRigidConstraints = m_lRigidConstraints.Len();

#ifndef DYNAMICS_DISABLE_CONSTRAINTS
				{
				MSCOPESHORT(CWorld::SolveConstraints);

				for (int j = 0; j < N_CONSTRAINT_ITERATIONS; j++) 
				{
					for (int i = 0; i < m_lRigidConstraints.Len(); i++) {
						CRigidConstraint *pConstraint= &m_lRigidConstraints[i];

						if (pConstraint->m_pRigidBody1)
							pConstraint->m_pRigidBodyState1 = pConstraint->m_pRigidBody1->m_pstate;
						if (pConstraint->m_pRigidBody2)
							pConstraint->m_pRigidBodyState2 = pConstraint->m_pRigidBody2->m_pstate;
						if (pConstraint->m_Type == CRigidConstraint::BALLJOINTCONSTRAINT)
						{
							if (!pConstraint->m_pRigidBody1->IsStationary() || !pConstraint->m_pRigidBody2->IsStationary())
								CBallJointConstraintSolver::Solve(this, pConstraint, dt);

							if (j == N_CONSTRAINT_ITERATIONS - 1)
								CBallJointConstraintSolver::PostSolve(this, pConstraint, dt);
						}
						else if (pConstraint->m_Type == CRigidConstraint::FIXEDPOINTCONSTRAINT)
						{							
							if (!pConstraint->m_pRigidBody1->IsStationary())
								CFixedPointConstraintSolver::Solve(this, pConstraint, dt);

							if (j == N_CONSTRAINT_ITERATIONS - 1)
								CFixedPointConstraintSolver::PostSolve(this, pConstraint, dt);								
						}
						else if (pConstraint->m_Type == CRigidConstraint::AXISCONSTRAINT)
						{
							if (!pConstraint->m_pRigidBody1->IsStationary())
								CAxisConstraintSolver::Solve(this, pConstraint, dt);

							if (j == N_CONSTRAINT_ITERATIONS - 1)
								CAxisConstraintSolver::PostSolve(this, pConstraint, dt);
						}
						else if (pConstraint->m_Type == CRigidConstraint::HINGECONSTRAINT)
						{
							if (!pConstraint->m_pRigidBody1->IsStationary() || !pConstraint->m_pRigidBody2->IsStationary())
								CHingeConstraintSolver::Solve(this, pConstraint, dt);

							if (j == N_CONSTRAINT_ITERATIONS - 1)
								CHingeConstraintSolver::PostSolve(this, pConstraint, dt);
						}
						else if (pConstraint->m_Type == CRigidConstraint::MAXDISTANCETOPOINTCONSTRAINT)
						{
							if (!pConstraint->m_pRigidBody1->IsStationary())
								CMaxDistanceToPointConstraintSolver::Solve(this, pConstraint, dt);

							if (j == N_CONSTRAINT_ITERATIONS - 1)
								CMaxDistanceToPointConstraintSolver::PostSolve(this, pConstraint, dt);
						}
						else if (pConstraint->m_Type == CRigidConstraint::BALLJOINTTOWORLD)
						{
							if (!pConstraint->m_pRigidBody1->IsStationary())
								CBallJointToWorldSolver::Solve(this, pConstraint, dt);

							if (j == N_CONSTRAINT_ITERATIONS - 1)
								CBallJointToWorldSolver::PostSolve(this, pConstraint, dt);
						}
						else if (pConstraint->m_Type == CRigidConstraint::MAXDISTANCECONSTRAINT)
						{
							CMaxDistanceConstraintSolver::Solve(this, pConstraint, dt);

							if (j == N_CONSTRAINT_ITERATIONS - 1)
								CMaxDistanceConstraintSolver::PostSolve(this, pConstraint, dt);
						}
						else if (pConstraint->m_Type == CRigidConstraint::MINDISTANCECONSTRAINT)
						{
							CMinDistanceConstraintSolver::Solve(this, pConstraint, dt);

							if (j == N_CONSTRAINT_ITERATIONS - 1)
								CMinDistanceConstraintSolver::PostSolve(this, pConstraint, dt);
						}
						else
						{
							//M_ASSERT(false, "Invalid constraint type!");
						}
					}
				}
				}
#endif
				
#if 0
				if (gcloth)
					gcloth->Integrate(dt);
#endif 

				SaveFullState();

				for (int j=0; j<nintegrationsteps; j++) {
					UpdateVelocity(dt/nintegrationsteps);
					UpdatePosition(dt/nintegrationsteps);
					UpdateRotation(dt/nintegrationsteps);
				}

				if (gcollisioninfolist.Len() < 1000) {
					gcollisioninfolist.SetLen(1000);
				}
				//collisioninfolist.SetGrow(200);
				int startindex=0;
				//gcollisioninfolist.SetLen(0);

				m_pWorldCollider->PreCollide(this);
				//		UpdateBoundingSpheres();
				//printf("------------------------\n");
				//DetectCollisions(engine,*m_collisionspace, &startindex, gcollisioninfolist);
				DetectCollisions2(&startindex, gcollisioninfolist, _pColliderArgument1, _pColliderArgument2);

				if (m_pWorldCollider)
				{			
					m_pWorldCollider->ApplyExternalForces(m_pColliderArgument1, m_pColliderArgument2);
				}

				int ncollisions= startindex;
				gncollisions = startindex;
				//		CalculateImpulseDenominator(gcollisioninfolist, ncollisions);

				RestoreVelocityState();
				CalculateImpulseDenominator(gcollisioninfolist, ncollisions);
				//		ClearForces();

#define N_ELASTIC_COLLISIONS 1
#define N_INELASTIC_COLLISIONS 8

				for (int i=0; i<N_ELASTIC_COLLISIONS; i++) 
				{
					//int ncoll= ProcessCollisions(gcollisioninfolist,ncollisions,dt,-1.0f, i % 2 == 0);
					int ncoll= ProcessCollisions(gcollisioninfolist,ncollisions,dt, 0.5, i % 2 == 0);
					if (ncoll==0) break;
				}

				RestorePositionOrientationState();

				if (m_pWorldCollider)
				{			
					m_pWorldCollider->PostApplyExternalForces(m_pColliderArgument1, m_pColliderArgument2);
				}

				for (int j=0; j<nintegrationsteps; j++) {
					UpdateVelocity(dt/nintegrationsteps);
				}
				//ClearForces();

#if 1
				for (int i=0; i<N_INELASTIC_COLLISIONS; i++) {
					/*			int ncoll= ProcessCollisions(gcollisioninfolist,
					ncollisions,
					dt,
					0-(N_INELASTIC_COLLISIONS-1-i)*0.05, 
					i % 2 ==0);
					*/
					int ncoll= ProcessCollisions(gcollisioninfolist,ncollisions,dt,0, i % 2 ==0);
					if (ncoll==0) break;
				}
#endif

				/*for (int i=0; i<rigidbodylist.Len(); i++) {
				rigidbodylist[i]->GetRigidBody()->CheckStationary();
				}*/

#if 1
				SeperateObjects(ncollisions);
#endif

#if 0
				for (int i=0; i<rigidbodylist.Len(); i++) {
					CRigidBody *geom= rigidbodylist[i];
					CRigidBody *rigid= geom->GetRigidBody();
					if (rigid->IsActive()) {

						/*
						CVec3Dfp64 pos,vel;
						rigid->GetPosition(pos);
						rigid->GetVelocity(vel);
						if (fabs(pos.k[0]) > 3 && pos.k[0]*vel.k[0] > 0) {
						vel.k[0]*=-1.0;
						}
						if (fabs(pos.k[2]) > 3 && pos.k[2]*vel.k[2] > 0) {
						vel.k[2]*=-1.0;
						}
						rigid->SetVelocity(vel);
						*/

						for (int j=0; j<nintegrationsteps; j++) {
							rigid->UpdateState();
							rigid->UpdatePosition(dt/nintegrationsteps);
							rigid->UpdateRotation(dt/nintegrationsteps);
						}

						//rigid->CheckStationary();

					}			
				}
#endif 

#if 0
				// Debugutskrift
				M_TRACEALWAYS("-----------\n");
				for (int i=0; i<m_rigidbodylist.Len(); i++) {
					M_TRACEALWAYS("%s,%f\n",m_rigidbodylist[i]->GetRigidBody()->GetVelocity().GetString().Str(),
						m_rigidbodylist[i]->GetRigidBody()->GetVelocity().Length());
					M_TRACEALWAYS("%s,%f\n",m_rigidbodylist[i]->GetRigidBody()->GetAngularVelocity().GetString().Str(),
						m_rigidbodylist[i]->GetRigidBody()->GetAngularVelocity().Length());
				}
#endif

				//M_TRACEALWAYS("-----------\n");
				// Check stationary
#ifdef CHECK_STATIONERY
				for (int i=0; i<m_rigidbodylist.Len(); i++) {
					//M_TRACEALWAYS("%s\n",grouplist[j]->GetRigidBody()->GetVelocity().GetString().Str());
					//M_TRACEALWAYS("%s\n",grouplist[j]->GetRigidBody()->GetAngularVelocity().GetString().Str());
					m_rigidbodylist[i]->GetRigidBody()->CheckStationary();
				}
#endif


				for (int j=0; j<nintegrationsteps; j++) {
					//			UpdateState();
					UpdatePosition(dt/nintegrationsteps);
					UpdateRotation(dt/nintegrationsteps);
				}

#ifdef DYNAMICS_FREEZE_OBJECTS
//#ifdef DYNAMICS_FREEZE_OBJECTS
//				CheckStationary(dt);
#endif

				// TODO: Måste man göra detta för varje tick...
				TArray<CRigidBody *> lUnfreezedObjects;
				if (m_pWorldCollider)
				{
					for (int i = 0; i<m_lJustUnfreezedObjects.Len(); i++)
					{
						CRigidBody *pRigidBody = m_lJustUnfreezedObjects[i];
						TArray<CRigidBody *> tmplist;
						CVec3Dfp64 p = pRigidBody->GetPosition();
						p[2] += 2.0;
						pRigidBody->SetPosition(p);
						tmplist.Add(pRigidBody);
						CContactInfo tmpcc[10];
						int OldPrec = m_CollisionPrecision;
						SetCollisionPrecision(1);
						int nColl = m_pWorldCollider->Collide(this, tmplist, tmpcc, 10, _pColliderArgument1, _pColliderArgument2);
						SetCollisionPrecision(OldPrec);
						M_ASSERT(nColl <= 10, "");

						p[2] -= 2.0;
						pRigidBody->SetPosition(p);

						for (int j = 0; j < nColl; j++)
						{
							//lUnfreezedObjects.Add(pRigidBody);

							// TODO: BUGG
							// Här adderas samma objekt flera gånger (flera kontaktytor)
							CContactInfo *pCI = &tmpcc[j];
							if (pCI->m_pRigidBody1 != NULL && pCI->m_pRigidBody1 != pRigidBody)
							{
								bool found = false;
								for (int k = 0; k < lUnfreezedObjects.Len(); k++)
								{
									if (lUnfreezedObjects[k] == pCI->m_pRigidBody1)
									{
										found = true;
										break;
									}
								}
								if (!found)
								{
									if (pCI->m_pRigidBody1->IsStationary())
									{
										lUnfreezedObjects.Add(pCI->m_pRigidBody1);
										pCI->m_pRigidBody1->SetStationary(false);
									}
								}
							}

							if (pCI->m_pRigidBody2 != NULL && pCI->m_pRigidBody2 != pRigidBody)
							{
								bool found = false;
								for (int k = 0; k < lUnfreezedObjects.Len(); k++)
								{
									if (lUnfreezedObjects[k] == pCI->m_pRigidBody2)
									{
										found = true;
										break;
									}
								}
								if (!found)
								{
									if (pCI->m_pRigidBody2->IsStationary())
									{
										lUnfreezedObjects.Add(pCI->m_pRigidBody2);
										pCI->m_pRigidBody2->SetStationary(false);
									}
								}
							}

							//pRigidBody->SetStationary(false);
						}
					}
				}
				m_lJustUnfreezedObjects.SetLen(0);
				for (int i = 0; i < lUnfreezedObjects.Len(); i++)
				{
					m_lJustUnfreezedObjects.Add(lUnfreezedObjects[i]);
				}

#if 0
				for (int j=0; j<N_CONSTRAINT_ITERATIONS; j++) {
					for (int i=0; i<m_constraintlist.Len(); i++) {
						if (j % 2 == 0)
							m_constraintlist[i]->Apply(dt);
						else
							m_constraintlist[m_constraintlist.Len()-i-1]->Apply(dt);
					}
				}
#endif

				//		stats= m_collisionspace->GetStats();

#if 0
				for (int i=0; i<m_rigidbodylist.Len(); i++) {
					CRigidBody *rb = m_rigidbodylist[i]->GetRigidBody();
					rb->m_state = m_bodystate[rb->m_id];
				}
#endif

				if (_CollectCollisionEvents && m_CollectCollisionEvents)
					CollectCollisionEvents(gcollisioninfolist, ncollisions);

				if (_pDebugRenderer != NULL)
				{
					for (int i = 0; i < ncollisions; i++)
					{
						_pDebugRenderer->Render(gcollisioninfolist[i]);
					}

					for (int i=0; i<m_rigidbodylist.Len(); i++) 
					{
						_pDebugRenderer->Render(m_rigidbodylist[i]);
					}

				}
				//ClearForces();

			}

			int nhit= 0;
			int nmiss= 0;

			void CWorld::CalculateImpulseDenominator(TArray<CContactInfo>& collisioninfolist, int ncollisions)
			{
				MSCOPESHORT(CWorld::CalculateImpulseDenominator);
//				CRigidBodyState *pstate= m_bodystate.GetBasePtr();
//				CContactInfo *pcolllist= collisioninfolist.GetBasePtr();

				for (int i=0; i<ncollisions; i++) 
				{
					CContactInfo &ci= collisioninfolist[i];

					CRigidBody *rb1= ci.m_pRigidBody1->GetRigidBody();
					CRigidBody *rb2= ci.m_pRigidBody2->GetRigidBody();

//					CRigidBodyState *rb1state= &pstate[rb1->m_id];
//					CRigidBodyState *rb2state= &pstate[rb2->m_id];
					CRigidBodyState *rb1state= rb1->GetBodyState();
					CRigidBodyState *rb2state= rb2->GetBodyState();

					double B1 = 1.0 / rb1state->m_mass;
					double B2 = 1.0 / rb2state->m_mass;

					CVec3Dfp64 n = ci.m_Normal;
					CVec3Dfp64 tmp,tmp2;

					CVec3Dfp64 ra = ci.m_ra; // * m_ScaleFactor;
					CVec3Dfp64 rb = ci.m_rb; // * m_ScaleFactor;

					//			ci.m_ra.CrossProd(n,tmp);
					ra.CrossProd(n,tmp);
					tmp.MultiplyMatrix(rb1->GetWorldInertiaTensorInvert(),tmp2);
					//			tmp2.CrossProd(ci.m_ra,tmp);
					tmp2.CrossProd(ra,tmp);
					double B3 = n*tmp; ///(m_ScaleFactor*m_ScaleFactor);

					//			(ci.m_rb*m_ScaleFactor).CrossProd(n,tmp);
					rb.CrossProd(n,tmp);
					tmp.MultiplyMatrix(rb2->GetWorldInertiaTensorInvert(),tmp2);
					//			tmp2.CrossProd(ci.m_rb,tmp);
					tmp2.CrossProd(rb,tmp);
					double B4 = n*tmp; ///(m_ScaleFactor*m_ScaleFactor);

					ci.m_collision_impulse_denominator=  B1 + B2 + B3 + B4;
				}
			}

