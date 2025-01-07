/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_ChainEffect.h

	Author:			Patrik Willbo

	Copyright:		Copyright Starbreeze AB 2005

	Contents:		CWObject_ChainEffect
					CXR_SkeletonRope

	History:		
		050525:		Created File
\*____________________________________________________________________________________________*/
#ifndef __WObj_ChainEffect_h__
#define __WObj_ChainEffect_h__


enum
{
	CHAIN_STATE_EXPANDING	= M_Bit(0),
	CHAIN_STATE_SHRINKING	= M_Bit(1),
	CHAIN_STATE_ACTIVE		= M_Bit(2),

	CXR_ROPE_VALID			= M_Bit(0),
	CXR_ROPE_CONNECTION		= M_Bit(1),	// Tells our system this mass object has a fixed position
	CXR_ROPE_COLLISION		= M_Bit(2),	// Is this mass colliding or not
	
	CWO_CHAIN_STATIC		= M_Bit(0),
	CWO_CHAIN_DYNAMIC		= M_Bit(1),

	OBJMSG_CHAIN_EXPAND		= OBJMSGBASE_MISC_CHAIN,			// Magically increase the length of the chain/rope (expands from base)
	OBJMSG_CHAIN_SHRINK,									// Magically decrease the length if the chain/rope (shrinks at base)
	OBJMSG_CHAIN_IMPULSE,									// Sends an impulse somewhere on the chain
	OBJMSG_CHAIN_LENGTHTO,									// Magically increase the lenght of the chain/rope so it can reach a position
	OBJMSG_CHAIN_AUTOSHRINK,
	//OBJMSG_CHAIN_LINKTOPOS,									// Drags current chain towards goal

	NETMSG_CHAIN_EXPAND		= 0,
	NETMSG_CHAIN_SHRINK,
	NETMSG_CHAIN_IMPULSE,
	NETMSG_CHAIN_LENGTHTO,
	NETMSG_CHAIN_AUTOSHRINK,
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Rope
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Rope_CreationParam
{
public:
	CXR_Rope_CreationParam()
	{
		Set();
	}

	void Set(const uint32& _nBaseMasses = 10, const fp32& _MassWeight = 0.05f, const fp32& _SpringK = 10000.0f, const fp32& _SpringL = 1.0f, const fp32& _AirFK = 0.01f, const fp32& _SpringFK = 0.2f, const fp32& _CollRepelK = 100.0f, const fp32& _CollFricK = 0.2f, const fp32& _CollAbsK = 2.0f)
	{
		m_MassWeight	= _MassWeight;
		m_SpringK		= _SpringK;
		m_SpringL		= _SpringL;
		m_SpringFricK	= _SpringFK;
		m_AirFricK		= _AirFK;
		m_CollRepelK	= _CollRepelK;
		m_CollFricK		= _CollFricK;
		m_CollAbsK		= _CollAbsK;
		m_nMasses		= _nBaseMasses;
	}

	uint32	m_nMasses;
	fp32		m_MassWeight;
	fp32		m_SpringK;
	fp32		m_SpringL;
	fp32		m_SpringFricK;
	fp32		m_AirFricK;
	fp32		m_CollRepelK;
	fp32		m_CollFricK;
	fp32		m_CollAbsK;
};

class CXR_Rope
{
	class CRopeMass
	{
	public:
		fp32			m_Mass;
		fp32			m_PenDepth;
		CVec3Dfp32	m_PosHistory;
		CVec3Dfp32	m_Pos;
		CVec3Dfp32	m_Vel;
		CVec3Dfp32	m_Force;
		uint8		m_Flags;

		CRopeMass(const CRopeMass& _Mass) : m_Mass(_Mass.m_Mass), m_PenDepth(_Mass.m_PenDepth), m_PosHistory(_Mass.m_PosHistory), m_Pos(_Mass.m_Pos), m_Vel(_Mass.m_Vel), m_Force(_Mass.m_Force), m_Flags(_Mass.m_Flags) {}
		CRopeMass(const fp32& _Mass)	: m_Mass(_Mass), m_Flags(0) {}
		CRopeMass() : m_Flags(0) {}

		void operator = (const CRopeMass& _Mass)
		{
			m_Mass = _Mass.m_Mass;
			m_PenDepth = _Mass.m_PenDepth;
			m_PosHistory = _Mass.m_PosHistory;
			m_Pos = _Mass.m_Pos;
			m_Vel = _Mass.m_Vel;
			m_Force = _Mass.m_Force;
			m_Flags = _Mass.m_Flags;
		}

		bool IsColliding()							{ return (m_Flags & CXR_ROPE_COLLISION) ? true : false; }
		void ApplyForce(const CVec3Dfp32& _Force)	{ m_Force += _Force; }
		void ClearForce()							{ m_Force = CVec3Dfp32(0); }
		void Simulate(const fp32& _dt);
	};

	class CRopeSpring
	{
	public:
		CRopeMass*	m_pMass1;
		CRopeMass*	m_pMass2;
		fp32			m_SpringConstant;
		fp32			m_SpringLength;
		fp32			m_FrictionConstant;

		CRopeSpring() {}

		void operator = (const CRopeSpring& _Spring)
		{
			m_pMass1 = _Spring.m_pMass1;
			m_pMass2 = _Spring.m_pMass2;
			m_SpringConstant = _Spring.m_SpringConstant;
			m_SpringLength = _Spring.m_SpringLength;
			m_FrictionConstant = _Spring.m_FrictionConstant;
		}

		void Initialize(CRopeMass* _pMass1, CRopeMass* _pMass2, const fp32& _SpringC, const fp32& _SpringL, const fp32& _FrictionC)
		{
			m_pMass1 = _pMass1;
			m_pMass2 = _pMass2;

			m_SpringConstant = _SpringC;
			m_SpringLength = _SpringL;
			m_FrictionConstant = _FrictionC;
		}

		void Solve()
		{
			const CVec3Dfp32 SpringVec = m_pMass1->m_Pos - m_pMass2->m_Pos;
			const fp32 SpringLen = SpringVec.Length();

			CVec3Dfp32 Force = CVec3Dfp32(0);
			if(SpringLen != 0)
				Force += -(SpringVec / SpringLen) * (SpringLen - m_SpringLength) * m_SpringConstant;

			Force += -(m_pMass1->m_Vel - m_pMass2->m_Vel) * m_FrictionConstant;

			m_pMass1->ApplyForce( Force);
			m_pMass2->ApplyForce(-Force);
		}

		void ConstrainSpring()
		{
			CVec3Dfp32 SpringVec = m_pMass2->m_Pos - m_pMass1->m_Pos;
			const fp32 SpringLen = SpringVec.Length();
			
			if(SpringLen > m_SpringLength)
			{
				// Constrain the spring so it can't be flexed more than the length
				SpringVec.Normalize();
				m_pMass2->m_Pos = m_pMass1->m_Pos + (SpringVec * m_SpringLength);
			}
		}
	};

public:

	CXR_Rope();
	CXR_Rope(const uint16& _nMasses, const fp32& _Mass,							// Number of masses and weight of each one
		const fp32& _SpringC, const fp32& _SpringL, const fp32& _SpringFC,			// Stiffnes k / Length / Inner friction k
		const fp32& _AirFC,														// Air friction k
		const fp32& _GroundRC, const fp32& _GroundFC, const fp32& _GroundAC);		// Ground repulsion k / friction k / absorption k

	void Create(const uint32& _iRope, const CXR_Rope_CreationParam& _CreateParam, const uint32& _Flags);

	void Initialize(const uint16& _nMasses, const fp32& _Mass, const fp32& _SpringC, const fp32& _SpringL, const fp32& _SpringFC,
					const fp32& _AirFC, const fp32& _GroundRC, const fp32& _GroundFC, const fp32& _GroundAC);

	void Solve();
	void Simulate(const fp32& _dt);
	void Operate(const fp32& _dt, const uint32& _nIter, CWorld_PhysState* _pWPhysState);

	// Nailing down positions and reactivating solvers
	void SetMassConnectionPosition(const uint32& _iMass, const CVec3Dfp32& _Position);	// Nail down position
	void SetMassConnectionVelocity(const uint32& _iMass, const CVec3Dfp32& _Velocity);	// Moves a nailed down position
	int  GetLastMass() { return m_lMasses.Len()-1; }
	int  GetFirstMass() { return 0; }
	
	CVec3Dfp32 GetMassPosition(const int& _iMass) { return m_pMasses[_iMass].m_Pos; }

	void ReleaseMassConnection(const uint32& _iMass);									// Release mass and let the system animate it
	int  MassHasConnection(const uint32& _iMass);										// Returns true if fixed, otherwise false

	void StorePosition();
	void UpdateCollision(CWorld_Client* _pWClient);

	int GetNumMasses() { return m_lIPPosition.Len(); }

	CVec3Dfp32* GetIPPosition(const fp32& _IPTime, const uint32& _iRefObj, const bool& _bUpdate = true);
	CVec3Dfp32* GetIPPosition() { m_pIPPosition = m_lIPPosition.GetBasePtr(); return m_pIPPosition; }

	void NormalizeVelocities();

	#ifndef M_RTM
		// Debug drawing functions
		void Debug_RenderRopeMasses(const uint32& _iObjLink, CWorld_Client* _pWClient);
	#endif

	// Misc functions/utils
	uint16 GetNumRefObjs() { return (m_nMasses / m_nRefMasses); }
	void Expand(const uint32& _nRefLinks);
	void Shrink(const uint32& _nRefLinks);
	void AutoShrink(const fp32& _AutoShrink);

	void ExpandTo(const fp32& _Length, const CVec3Dfp32& _Point);
	void Impulse(const int32& _iMass, const CVec3Dfp32& _Velocity);

	void UpdateAutoShrink(const fp32& _GameTickTime);

	void ConstrainSprings()
	{
		for(int i = 0; i < m_nSprings; i++)
			m_pSprings[i].ConstrainSpring();
	}

	// Update time
	CMTime					m_LastUpdate;
	fp32						m_AutoShrink;
	fp32						m_AutoShrinkElapse;
	fp32						m_AutoShrinkRecip;
	uint32					m_nTargetShrink;

	uint32					m_iRope;
	uint32					m_Flags;

private:

	void ReLink();

	TArray<CRopeSpring>	m_lSprings;
	TArray<CRopeMass>		m_lMasses;
	TThinArray<CVec3Dfp32>		m_lIPPosition;
	
	CRopeSpring*			m_pSprings;
	CRopeMass*				m_pMasses;
	CVec3Dfp32*				m_pIPPosition;

	uint32					m_nSprings;
	uint32					m_nMasses;
	fp32						m_Radius;
	
	uint16					m_nRefMasses;	// Number of masses in reference
	uint16					m_nRefSprings;

	fp32						m_RefAvgSpringLength;

	// Rope physics constants
	fp32						m_SpringConstant;
	fp32						m_SpringLength;
	fp32						m_SpringFrictionConstant;
	fp32						m_AirFrictionConstant;
	fp32						m_GroundRepulsionConstant;
	fp32						m_GroundFrictionConstant;
	fp32						m_GroundAbsorptionConstant;

	fp32						m_BaseMassWeight;

	//CVec3Dfp32				m_LastLinkWPos;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_ChainEffect_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_ChainEffect_ClientData : public CReferenceCount, public CAutoVarContainer
{
	CWObject_CoreData* m_pObj;
	CWorld_PhysState* m_pWPhysState;
	AUTOVAR_SETCLASS(CWO_ChainEffect_ClientData, CAutoVarContainer);

public:
	CAUTOVAR_OP(CAutoVar_uint8, m_Flags,		DIRTYMASK_0_0);
	CAUTOVAR_OP(CAutoVar_int8,  m_Connect0,		DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_int8,  m_Connect1,		DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_int16, m_BoneConnect0,	DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_int16, m_BoneConnect1,	DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_int8,  m_Type,			DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_int32, m_iOwner,		DIRTYMASK_0_4);
	
	CAUTOVAR_OP(CAutoVar_fp32,	m_MassWeight,	DIRTYMASK_1_0);	// 'Particle' weight
	CAUTOVAR_OP(CAutoVar_fp32,	m_SpringK,		DIRTYMASK_1_0);	// Spring constant
	CAUTOVAR_OP(CAutoVar_fp32,	m_SpringL,		DIRTYMASK_1_0);	// Default spring length
	CAUTOVAR_OP(CAutoVar_fp32,	m_SpringFricK,	DIRTYMASK_1_0);	// Spring inner friction constant
	CAUTOVAR_OP(CAutoVar_fp32,	m_AirFricK,		DIRTYMASK_1_0);	// Air friction constant
	CAUTOVAR_OP(CAutoVar_fp32,	m_CollRepelK,	DIRTYMASK_1_0);	// Repel constant on collision
	CAUTOVAR_OP(CAutoVar_fp32,	m_CollFricK,	DIRTYMASK_1_0);	// Slide friction constant
	CAUTOVAR_OP(CAutoVar_fp32,	m_CollAbsK,		DIRTYMASK_1_0);	// Collision absorption constant
	CAUTOVAR_OP(CAutoVar_int32,	m_nBaseMasses,	DIRTYMASK_1_0);	// Number of masses without expansion

	CAUTOVAR_OP(CAutoVar_int32,	m_SurfaceID,	DIRTYMASK_1_1);	// Surface used during rendering
	CAUTOVAR_OP(CAutoVar_fp32,	m_Width,		DIRTYMASK_1_1);	// Width of beam strip during rendering

	AUTOVAR_PACK_BEGIN
	// Misc.
	AUTOVAR_PACK_VAR(m_Flags)
	AUTOVAR_PACK_VAR(m_Connect0)
	AUTOVAR_PACK_VAR(m_Connect1)
	AUTOVAR_PACK_VAR(m_BoneConnect0)
	AUTOVAR_PACK_VAR(m_BoneConnect1)
	AUTOVAR_PACK_VAR(m_Type)
	AUTOVAR_PACK_VAR(m_iOwner)

	// Creation properties
	AUTOVAR_PACK_VAR(m_MassWeight)
	AUTOVAR_PACK_VAR(m_SpringK)
	AUTOVAR_PACK_VAR(m_SpringL)
	AUTOVAR_PACK_VAR(m_SpringFricK)
	AUTOVAR_PACK_VAR(m_AirFricK)
	AUTOVAR_PACK_VAR(m_CollRepelK)
	AUTOVAR_PACK_VAR(m_CollFricK)
	AUTOVAR_PACK_VAR(m_CollAbsK)
	AUTOVAR_PACK_VAR(m_nBaseMasses)

	// Render properties
	AUTOVAR_PACK_VAR(m_SurfaceID)
	AUTOVAR_PACK_VAR(m_Width)
	AUTOVAR_PACK_END

	void Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	bool IsValid(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);

	CXR_Rope		m_Rope;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_ChainEffect
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define CWObject_ChainEffect_Parent CWObject_Model
class CWObject_ChainEffect : public CWObject_ChainEffect_Parent
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	static const CWO_ChainEffect_ClientData& GetClientData(const CWObject_CoreData* _pObj);
	static		 CWO_ChainEffect_ClientData& GetClientData(CWObject_CoreData* _pObj);

	virtual void OnCreate();
	virtual void OnRefresh();
	virtual int  OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const;
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();

	static void OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer);
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server* _pWServer);

	virtual aint OnMessage(const CWObject_Message& _Msg);
	//static int CreateFromObject(const CWObject& _Obj, const CRegistry& _Reg);

	//static void OnClientCreate(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static int  OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
};

#endif
