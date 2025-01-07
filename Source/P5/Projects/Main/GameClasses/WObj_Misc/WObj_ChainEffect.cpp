/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_ChainEffect.cpp

	Author:			Patrik Willbo

	Copyright:		Copyright Starbreeze AB 2005

	History:		
		050525:		Created File
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_ChainEffect.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_SkeletonRope
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CXR_Rope::CRopeMass::Simulate(const fp32& _dt)
{
	// Check if mass is nailed down to a position or not
	if(m_Flags & CXR_ROPE_CONNECTION)
	{
		m_Pos += m_Vel * _dt;
		//m_Force = CVec3Dfp32(0);
	}
	else
	{
		m_Vel += (m_Force / m_Mass) * _dt;
		m_Pos += m_Vel * _dt;
	}
}

CXR_Rope::CXR_Rope()
	: m_Flags(0)
	, m_pIPPosition(NULL)
{
	m_lMasses.Clear();
	m_lSprings.Clear();
	m_LastUpdate.Reset();
	m_lIPPosition.Clear();
}

CXR_Rope::CXR_Rope(const uint16& _nMasses, const fp32& _Mass, const fp32& _SpringC, const fp32& _SpringL, const fp32& _SpringFC,
                   const fp32& _AirFC, const fp32& _GroundRC, const fp32& _GroundFC, const fp32& _GroundAC)
	: m_Flags(0)
	, m_pIPPosition(NULL)
	, m_SpringConstant(_SpringC)
	, m_SpringLength(_SpringL)
	, m_SpringFrictionConstant(_SpringFC)
	, m_AirFrictionConstant(_AirFC)
	, m_GroundRepulsionConstant(_GroundRC)
	, m_GroundFrictionConstant(_GroundFC)
	, m_GroundAbsorptionConstant(_GroundAC)
{
	m_lMasses.SetLen(_nMasses);
	m_lSprings.SetLen(_nMasses-1);
	m_lIPPosition.Clear();
	
	m_nMasses = _nMasses;
	m_nSprings = _nMasses-1;
	m_pMasses = m_lMasses.GetBasePtr();
	m_pSprings = m_lSprings.GetBasePtr();

	for(uint16 i = 0; i < m_nMasses; i++)
		m_pMasses[i].m_Mass = _Mass;

	for(uint16 i = 0; i < m_nSprings; i++)
	{
		m_pSprings[i].m_pMass1 = &m_pMasses[i];
		m_pSprings[i].m_pMass2 = &m_pMasses[i + 1];
		m_pSprings[i].m_SpringConstant = _SpringC;
		m_pSprings[i].m_SpringLength = _SpringL;
		m_pSprings[i].m_FrictionConstant = _SpringFC;
	}
}

void CXR_Rope::Initialize(const uint16& _nMasses, const fp32& _Mass, const fp32& _SpringC, const fp32& _SpringL, const fp32& _SpringFC,
						  const fp32& _AirFC, const fp32& _GroundRC, const fp32& _GroundFC, const fp32& _GroundAC)
{
	m_lMasses.SetLen(_nMasses);
	m_lSprings.SetLen(_nMasses-1);
	
	m_nMasses = _nMasses;
	m_nSprings = _nMasses -1;
	m_pMasses = m_lMasses.GetBasePtr();
	m_pSprings = m_lSprings.GetBasePtr();

	// Setup masses
	for(uint16 i = 0; i < m_nMasses; i++)
		m_pMasses[i].m_Mass = _Mass;

	for(uint16 i = 0; i < m_nSprings; i++)
	{
		m_pSprings[i].m_pMass1 = &m_pMasses[i];
		m_pSprings[i].m_pMass2 = &m_pMasses[i + 1];
		m_pSprings[i].m_SpringConstant = _SpringC;
		m_pSprings[i].m_SpringLength = _SpringL;
		m_pSprings[i].m_FrictionConstant = _SpringFC;
	}

	m_BaseMassWeight = _Mass;
	m_SpringConstant = _SpringC;
	m_SpringLength = _SpringL;
	m_SpringFrictionConstant = _SpringFC;
	m_AirFrictionConstant = _AirFC;
	m_GroundRepulsionConstant = _GroundRC;
	m_GroundFrictionConstant = _GroundFC;
	m_GroundAbsorptionConstant = _GroundAC;
	m_nRefMasses = _nMasses;
	m_nRefSprings = _nMasses - 1;
}

void CXR_Rope::Solve()
{
	for(int i = 0; i < m_nSprings; i++)
		m_pSprings[i].Solve();

	for(int i = 0; i < m_nMasses; i++)
	{
		CRopeMass& Mass = m_pMasses[i];

		Mass.ApplyForce( CVec3Dfp32(0,0,-313.92f) * Mass.m_Mass );	// Fix gravitational force!
		Mass.ApplyForce(-Mass.m_Vel * m_AirFrictionConstant);

		if(Mass.IsColliding())
		{
			CVec3Dfp32 v = Mass.m_Vel;

			// Kill gravitational force and apply ground fritcion
			v.k[2] = 0;
			Mass.ApplyForce(-v * m_GroundFrictionConstant);

			v = Mass.m_Vel;
			v.k[0] = v.k[1] = 0;
			
			// if( hit )
			Mass.ApplyForce(-v * m_GroundAbsorptionConstant);

			//CVec3Dfp32 Force = CVec3Dfp32(0, m_GroundRepulsion, 0) * (GroundHeight - pos.height);	// Fix me, repuls ground
			//Mass.ApplyForce(Force);
			Mass.ApplyForce(CVec3Dfp32(0, 0, m_GroundRepulsionConstant) *
							(((Mass.m_Pos - Mass.m_PosHistory).Normalize() * Mass.m_PenDepth)[2] - Mass.m_PosHistory[2])
							);
			
		}
	}
}

void CXR_Rope::Simulate(const fp32& _dt)
{
	// Run simulation

	// Iterate all the masses
	for(int i = 0; i < m_nMasses; i++)
		m_pMasses[i].Simulate(_dt);
}

// Needed ?? - Honestly don't remember :$
void CXR_Rope::StorePosition()
{
	for(int i = 0; i < m_nMasses; i++)
		m_pMasses[i].m_PosHistory = m_pMasses[i].m_Pos;
}

void CXR_Rope::Operate(const fp32& _dt, const uint32& _nIter, CWorld_PhysState* _pWPhysState)
{
	// Iterate and solve the system
	for(uint32 iIter = 0; iIter < _nIter; iIter++)
	{
		for(int i = 0; i < m_nMasses; i++)
			m_pMasses[i].m_Force = CVec3Dfp32(0);

		Solve();
		Simulate(_dt);
	}
}

void CXR_Rope::SetMassConnectionPosition(const uint32& _iMass, const CVec3Dfp32& _Position)
{
	m_pMasses[_iMass].m_Pos = _Position;
	m_pMasses[_iMass].m_Flags |= CXR_ROPE_CONNECTION;
}

void CXR_Rope::SetMassConnectionVelocity(const uint32& _iMass, const CVec3Dfp32& _Velocity)
{
	m_pMasses[_iMass].m_Vel = _Velocity;
	m_pMasses[_iMass].m_Flags |= CXR_ROPE_CONNECTION;
}

void CXR_Rope::ReleaseMassConnection(const uint32& _iMass)
{
	m_pMasses[_iMass].m_Flags &= ~CXR_ROPE_CONNECTION;
}

int CXR_Rope::MassHasConnection(const uint32& _iMass)
{
	return (m_pMasses[_iMass].m_Flags & CXR_ROPE_CONNECTION);
}

#ifndef M_RTM
void CXR_Rope::Debug_RenderRopeMasses(const uint32& _iObjLink, CWorld_Client* _pWClient)
{
	uint32 iMass = _iObjLink * m_nRefMasses;
	CVec3Dfp32 p0 = m_pMasses[iMass].m_Pos;
	//_pWClient->Debug_RenderVector(p0, m_pMasses[iMass].m_Vel, 0xff007f7f, 0.0f, false);
	//_pWClient->Debug_RenderVector(p0, m_pMasses[0].m_Force, 0xff7f007f, 0.0f, false);

	for(int i = 1; i < m_nRefMasses; i++)
	{
		CVec3Dfp32 p1 = m_pMasses[iMass + i].m_Pos;
		_pWClient->Debug_RenderWire(p0, p1, 0xff7f7f00,0.15f);

		//_pWClient->Debug_RenderVector(p1, m_pMasses[iMass + i].m_Vel, 0xff007f7f, 0.0f, false);
		//_pWClient->Debug_RenderVector(p1, m_pMasses[iMass + i].m_Force, 0xff7f007f, 0.0f, false);

		p0 = p1;
	}
}
#endif

void CXR_Rope::Create(const uint32& _iRope, const CXR_Rope_CreationParam& _CreateParam, const uint32& _Flags)
{
	m_Flags = _Flags;
	m_iRope = _iRope;
	m_AutoShrink = 0.0f;
	m_AutoShrinkElapse = 0;
	m_AutoShrinkRecip = -1;
	m_nTargetShrink = 0;
//	int32 MaxId = -1;

	m_nMasses = _CreateParam.m_nMasses;
	m_nSprings = m_nMasses - 1;
	
	// Initialize system
	Initialize(m_nMasses, _CreateParam.m_MassWeight, _CreateParam.m_SpringK, _CreateParam.m_SpringL, _CreateParam.m_SpringFricK,
		_CreateParam.m_AirFricK, _CreateParam.m_CollRepelK, _CreateParam.m_CollFricK, _CreateParam.m_CollAbsK);

	m_pMasses = m_lMasses.GetBasePtr();

	// FIXME : Setup masses properly!!
	for(int i = 0; i < m_nMasses; i++)
	{
		// Fix this setup ASAP!!
		m_pMasses[i].m_Pos = CVec3Dfp32(0) + CVec3Dfp32(-_CreateParam.m_SpringL * i,0,0); //CVec3Dfp32(0,0,-_CreateParam.m_SpringL * i);
		m_pMasses[i].m_Vel = CVec3Dfp32(0);
		m_pMasses[i].m_Force = CVec3Dfp32(0);
	}
	
	for(int i = 0; i < m_nSprings; i++)
		m_pSprings[i].m_SpringLength = _CreateParam.m_SpringL;

	m_Radius = _CreateParam.m_SpringL;
	m_RefAvgSpringLength = _CreateParam.m_SpringL;

	m_lMasses.SetGrow(m_nRefMasses);
	m_lSprings.SetGrow(m_nRefSprings);
}

void CXR_Rope::Expand(const uint32& _nRefLinks)
{
	// This sure is a nasty expand routine,,,

	CXR_Rope::CRopeMass InsMass = m_lMasses[0];
	InsMass.m_Flags = 0;//&= ~CXR_ROPE_CONNECTION;

	// Expand rope using n, references
	for(uint32 i = 0; i < _nRefLinks; i++)
	{
//		uint32 iMass = i * m_nRefMasses;
//		uint32 iSpring = i * m_nRefSprings;

		for(int j = 0; j < m_nRefMasses; j++)
		{
			CXR_Rope::CRopeMass& CurrMass = m_lMasses[j*2];//iMass + j];
			InsMass.m_Flags = CurrMass.m_Flags;
			CurrMass.m_Flags &= ~CXR_ROPE_CONNECTION;
			//m_lMasses[iMass + j].m_Flags &= ~CXR_ROPE_CONNECTION;	// Release possible connections

			m_lMasses.Insertx(j, &InsMass, 1);
		}

		for(int j = 0; j < m_nRefSprings+1; j++)
		{
			if(j == 0)
			{
				// Add a connective spring between new masses
				CXR_Rope::CRopeSpring Spring;
				Spring.m_pMass1 = &m_lMasses[j];//iMass + m_nRefMasses - 1];
				Spring.m_pMass2 = &m_lMasses[j+1];//iMass + m_nRefMasses];
				Spring.m_SpringConstant = m_SpringConstant;
				Spring.m_SpringLength = m_RefAvgSpringLength;
				Spring.m_FrictionConstant = m_SpringFrictionConstant;
				m_lSprings.Insertx(j, &Spring, 1);
			}
			else
			{
				CXR_Rope::CRopeSpring Spring = m_lSprings[0];
				Spring.m_pMass1 = &m_lMasses[j];
				Spring.m_pMass2 = &m_lMasses[j+1];
				m_lSprings.Insertx(j, &Spring, 1);
			}
		}
	}

	m_nMasses = m_lMasses.Len();
	m_nSprings = m_lSprings.Len();

	m_pMasses = m_lMasses.GetBasePtr();
	m_pSprings = m_lSprings.GetBasePtr();

	// Re link all our springs to match the masses
	ReLink();
}

void CXR_Rope::Shrink(const uint32& _nRefLinks)
{
	// Shrink chain
//	uint32 iNew = _nRefLinks * m_nRefMasses;
	
	if(((int32)(m_nMasses / m_nRefMasses) - (int32)_nRefLinks) < 1)
		return;

	m_lMasses.Delx(m_nMasses - (m_nRefMasses * _nRefLinks), m_nRefMasses * _nRefLinks);
	m_nMasses = m_lMasses.Len();
	m_pMasses = m_lMasses.GetBasePtr();

	m_lSprings.Delx(m_nSprings - ((m_nRefSprings + 1) * _nRefLinks), (m_nRefSprings + 1) * _nRefLinks);
	m_nSprings = m_lSprings.Len();
	m_pSprings = m_lSprings.GetBasePtr();

	ReLink();
}

void CXR_Rope::AutoShrink(const fp32& _AutoShrink)
{
	int32 nLinkObjs = (m_nMasses / m_nRefMasses);
	if(nLinkObjs > 1)
	{
		m_AutoShrink = _AutoShrink;
		m_nTargetShrink = (nLinkObjs - 1);
		m_AutoShrinkRecip = _AutoShrink / m_nTargetShrink;
		m_AutoShrinkElapse = 0;
	}
	else
	{
		m_AutoShrink = 0;
		m_nTargetShrink = 0;
		m_AutoShrinkRecip = -1;
		m_AutoShrinkElapse = 0;
	}
}

void CXR_Rope::ExpandTo(const fp32& _Length, const CVec3Dfp32& _Point)
{
	fp32 RefSpringLength = 0;
	fp32 SpringLength = 0;
	for(uint32 i = 0; i < m_nRefSprings; i++)
		RefSpringLength += m_pSprings[i].m_SpringLength;

	RefSpringLength = m_RefAvgSpringLength * m_nRefMasses;

	for(uint32 i = 0; i < m_nRefSprings-1; i++)
		SpringLength += m_pSprings[i].m_SpringLength;

	//if(SpringLength < _Length)
	{
		const CVec3Dfp32 Dir = (_Point - m_pMasses[0].m_Pos).Normalize();
		const fp32 Length = (_Point - (Dir * SpringLength)).Length();
		//const fp32 Length = (_Point - m_pMasses[m_nMasses-1].m_Pos).Length();

		//int32 iRefs = (int32)((Length / RefSpringLength) - (SpringLength / RefSpringLength));
		int32 iRefs = (int32)((Length / RefSpringLength)) + 1;
		iRefs = MaxMT(0, iRefs);

		// Expand chain if needed
		Expand(iRefs);

		// Send impulse to last link
		const CVec3Dfp32 Velocity = Dir * RefSpringLength;//(_Point - m_pMasses[0].m_Pos);
		Impulse(-2, Velocity * Velocity.Length());
	}
}

void CXR_Rope::Impulse(const int32& _iMass, const CVec3Dfp32& _Velocity)
{
	if(_iMass < 0)
	{
		if(_iMass == -1)
			m_pMasses[m_nMasses-1].m_Vel = _Velocity;
		else
		{
			for(int i = 0; i < m_nMasses; i++)
				m_pMasses[i].m_Vel = _Velocity;
		}
	}
	else
		m_pMasses[MinMT(m_nMasses-1, _iMass)].m_Vel = _Velocity;
}

void CXR_Rope::ReLink()
{
	int iMass = 0;
	for(uint32 i = 0; i < m_nSprings; i++)
	{
		m_pSprings[i].m_pMass1 = &m_pMasses[iMass++];
		m_pSprings[i].m_pMass2 = &m_pMasses[iMass];
	}
}

void CXR_Rope::UpdateCollision(CWorld_Client* _pWClient)
{
	// check collision against world, 
	// Bad implementation with line tracers. We should re-do this and do it right with a potential collision set
	/*
	TSelection<CSelection::LARGE_BUFFER> Selection;
	_pWClient->Selection_AddBoundSphere(Selection, OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL, m_pMasses[0].m_Pos, m_Radius);
	
	const int16* piSel;
	const int nSel = _pWClient->Selection_Get(Selection, &piSel);

	CCollisionInfo CInfo;
	CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
	CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_LOCALPOSITION);

	const int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
	const int32 ObjectFlags = OBJECT_FLAGS_WORLD;// | OBJECT_FLAGS_PHYSMODEL;
	const int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;

	for(int i = 0; i < m_nMasses; i++)
	{
		const CVec3Dfp32 Dir = (m_pMasses[i].m_Pos - m_pMasses[i].m_PosHistory).Normalize();
		const CVec3Dfp32 Radius = Dir * 2.0f;

		bool bHit = false;
		for(int j = 0; j < nSel; j++)
			bHit |= _pWClient->Object_IntersectLine(piSel[j], m_pMasses[i].m_PosHistory + Radius, m_pMasses[i].m_Pos + Radius, OwnFlags, ObjectFlags, MediumFlags, &CInfo);

		// Set penetration depth and flag mass for collision
		if(bHit)
		{
			m_pMasses[i].m_Pos = m_pMasses[i].m_PosHistory + (Dir * CInfo.m_Distance);
			m_pMasses[i].m_PenDepth = CInfo.m_Distance;
			m_pMasses[i].m_Flags |= CXR_ROPE_COLLISION;
		}
	}

	*/
}

CVec3Dfp32* CXR_Rope::GetIPPosition(const fp32& _IPTime, const uint32& _iRefObj, const bool& _bUpdate)
{
	if(_bUpdate)
	{
		// Make sure we have enough space for positions
		if(m_lIPPosition.Len() != m_nMasses)
		{
			m_lIPPosition.SetLen(m_nMasses);
			m_pIPPosition = m_lIPPosition.GetBasePtr();
		}

		const uint32 iStart = _iRefObj * m_nRefMasses;
		const uint32 iEnd = iStart + m_nRefMasses;
		for(uint32 i = iStart; i < iEnd; i++)
			m_pMasses[i].m_PosHistory.Lerp(m_pMasses[i].m_Pos, _IPTime, m_pIPPosition[i]);

		return m_pIPPosition;
	}
	else
		return m_lIPPosition.GetBasePtr();
}

void CXR_Rope::NormalizeVelocities()
{
	for(int i = 0; i < m_nMasses; i++)
		m_pMasses[i].m_Vel.Normalize();
}

void CXR_Rope::UpdateAutoShrink(const fp32& _GameTickTime)
{
	if(m_AutoShrinkRecip > 0)
	{
		m_AutoShrinkElapse += _GameTickTime;
		while(m_AutoShrinkElapse >= m_AutoShrinkRecip)
		{
			m_AutoShrinkElapse -= m_AutoShrinkRecip;
			if(m_nTargetShrink > 0)
			{
				Shrink(1);
				m_nTargetShrink--;
			}

			if(m_nTargetShrink <= 0)
			{
				m_AutoShrink = 0;
				m_nTargetShrink = 0;
				m_AutoShrinkRecip = 1;
				m_AutoShrinkElapse = 0;
			}
		}
	}
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_ChainEffect
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ChainEffect, CWObject_ChainEffect_Parent, 0x0100);

void CWO_ChainEffect_ClientData::Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	m_pObj = _pObj;
	m_pWPhysState = _pWPhysState;

	m_iOwner = 0;
	m_Flags = 0;
	m_Connect0 = 0;
	m_Connect1 = 0;
	m_BoneConnect0 = 0;
	m_BoneConnect1 = 0;
	m_Type = 0;
	m_Width = 0;
	m_SurfaceID = 0;

	m_nBaseMasses	= 10;

	m_MassWeight	= 0.05f;
	m_SpringK		= 10000.0f;
	m_SpringL		= 1.0f;
	m_SpringFricK	= 0.6f;
	m_AirFricK		= 0.01f;
	m_CollRepelK	= 100.0f;
	m_CollFricK		= 0.2f;
	m_CollAbsK		= 2.0f;
}

bool CWO_ChainEffect_ClientData::IsValid(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	if(m_pObj != _pObj || m_pWPhysState != _pWPhysState)
		return false;

	return true;
}

const CWO_ChainEffect_ClientData& CWObject_ChainEffect::GetClientData(const CWObject_CoreData* _pObj)
{
	const CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "No client data!");
	return *safe_cast<const CWO_ChainEffect_ClientData>(pData);
}

CWO_ChainEffect_ClientData& CWObject_ChainEffect::GetClientData(CWObject_CoreData* _pObj)
{
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "No client data!");
	return *safe_cast<CWO_ChainEffect_ClientData>(pData);
}

void CWObject_ChainEffect::OnCreate()
{
	//CWObject_ChainEffect_Parent::OnCreate();
	CWO_ChainEffect_ClientData& CD = GetClientData(this);
	CD.m_iOwner = m_iOwner;
}

void CWObject_ChainEffect::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	CWO_ChainEffect_ClientData& CD = GetClientData(this);

	// Dynamic or static chain/rope
	switch (_KeyHash)
	{
	case MHASH2('STAT','IC'): // "STATIC"
		{
			CD.m_Type = (KeyValue.Val_int()) ? CWO_CHAIN_STATIC : CWO_CHAIN_DYNAMIC;
			break;
		}
	
	// Determine if rope/chain is nailed down somewhere
	case MHASH2('CONN','ECT0'): // "CONNECT0"
		{
			CD.m_Connect0 = KeyValue.Val_int();
			break;
		}
	case MHASH2('CONN','ECT1'): // "CONNECT1"
		{
			CD.m_Connect1 = KeyValue.Val_int();
			break;
		}

	// Check if end points should be connected to any character bones
	case MHASH3('CONN','ECTB','ONE0'): // "CONNECTBONE0"
		{
			CD.m_BoneConnect0 = KeyValue.Val_int();
			break;
		}
	case MHASH3('CONN','ECTB','ONE1'): // "CONNECTBONE1"
		{
			CD.m_BoneConnect1 = KeyValue.Val_int();
			break;
		}

	// Creation parameters
	case MHASH4('NUMB','ASEM','ASSE','S'): // "NUMBASEMASSES"
		{
			CD.m_nBaseMasses = KeyValue.Val_int();
			break;
		}
	case MHASH3('MASS','WEIG','HT'): // "MASSWEIGHT"
		{
			CD.m_MassWeight = KeyValue.Val_fp64();
			break;
		}
	case MHASH3('SPRI','NGLE','NGTH'): // "SPRINGLENGTH"
		{
			CD.m_SpringL = KeyValue.Val_fp64();
			break;
		}
	case MHASH2('SPRI','NGK'): // "SPRINGK"
		{
			CD.m_SpringK = KeyValue.Val_fp64();
			break;
		}
	case MHASH4('SPRI','NGFR','ICTI','ONK'): // "SPRINGFRICTIONK"
		{
			CD.m_SpringFricK = KeyValue.Val_fp64();
			break;
		}
	case MHASH3('AIRF','RICT','IONK'): // "AIRFRICTIONK"
		{
			CD.m_AirFricK = KeyValue.Val_fp64();
			break;
		}
	case MHASH4('COLL','ISIO','NREP','ELK'): // "COLLISIONREPELK"
		{
			CD.m_CollRepelK = KeyValue.Val_fp64();
			break;
		}
	case MHASH5('COLL','ISIO','NFRI','CTIO','NK'): // "COLLISIONFRICTIONK"
		{
			CD.m_CollFricK = KeyValue.Val_fp64();
			break;
		}
	case MHASH5('COLL','ISIO','NABS','ORPT','IONK'): // "COLLISIONABSORPTIONK"
		{
			CD.m_CollAbsK = KeyValue.Val_fp64();
			break;
		}
	
	// Render parameters
	case MHASH4('REND','ERSU','RFAC','E'): // "RENDERSURFACE"
		{
			CD.m_SurfaceID = m_pWServer->GetMapData()->GetResource_SurfaceID(m_pWServer->GetMapData()->GetResourceIndex_Surface(KeyValue));
			break;
		}
	case MHASH3('REND','ERWI','DTH'): // "RENDERWIDTH"
		{
			CD.m_Width = KeyValue.Val_fp64();
			break;
		}

	default:
		{
			CWObject_ChainEffect_Parent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_ChainEffect::OnFinishEvalKeys()
{
	CWObject_ChainEffect_Parent::OnFinishEvalKeys();
	CWO_ChainEffect_ClientData& CD = GetClientData(this);
	m_DirtyMask |= CD.AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}

void CWObject_ChainEffect::OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer)
{
	CWObject_ChainEffect_Parent::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

	IncludeSurfaceFromKey("RENDERSURFACE", _pReg, _pMapData);
}

void CWObject_ChainEffect::OnIncludeClass(CMapData* _pWData, CWorld_Server* _pWServer)
{
	_pWData->GetResourceIndex_Model("Chain");
}

void CWObject_ChainEffect::OnRefresh()
{
	CWO_ChainEffect_ClientData& CD = GetClientData(this);

	// Update position if needed
	if((CD.m_BoneConnect0 || CD.m_BoneConnect1) && m_iOwner)
	{
		CWObject_CoreData* pCharObj = m_pWServer->Object_GetCD(m_iOwner);
		if (!pCharObj)
			return;
//		CWObject_Character* pCharChar = safe_cast<CWObject_Character>(pCharObj);
		
		CXR_Skeleton* pCharSkel;
		
		CVec3Dfp32 CharAnimPos;
		CXR_AnimState CharAnim;
		CXR_SkeletonInstance* pCharSkelInst;
		
		if(CWObject_Character::GetEvaluatedPhysSkeleton(pCharObj, m_pWServer, pCharSkelInst, pCharSkel, CharAnim))//, IPTime))
		{
			if(CD.m_BoneConnect0)
			{
				const uint32& RotTrack = CD.m_BoneConnect0;

				const CMat4Dfp32& Mat = pCharSkelInst->m_pBoneTransform[RotTrack];
				CVec3Dfp32 BonePos = pCharSkel->m_lNodes[RotTrack].m_LocalCenter;
				BonePos *= Mat;

				m_pWServer->Object_SetPosition(m_iObject, BonePos);
			}
		}
	}

	m_DirtyMask |= CD.AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}

int CWObject_ChainEffect::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const
{
	int Flags = CWO_CLIENTUPDATE_EXTRADATA;
	if(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
		Flags |= CWO_CLIENTUPDATE_AUTOVAR;

	uint8* pD = _pData;
	pD += CWObject_ChainEffect_Parent::OnCreateClientUpdate(_iClient, _pClObjInfo, _pOld, pD, Flags);

	const CWO_ChainEffect_ClientData& CD = GetClientData(this);
	CD.AutoVar_Pack(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT, pD, m_pWServer->GetMapData(), 0);

	return (pD - _pData);
}

void CWObject_ChainEffect::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	CWO_ChainEffect_ClientData* pCD = TDynamicCast<CWO_ChainEffect_ClientData>(pData);

	if (!pCD || !pCD->IsValid(_pObj, _pWPhysState))
	{
		CWO_ChainEffect_ClientData* pCD = MNew(CWO_ChainEffect_ClientData);
		if(!pCD)
			Error_static("CWObject_ChainEffect.:OnInitClientObjects", "Unable to create client data.");

		_pObj->m_lspClientObj[0] = pCD;
		pCD->Clear(_pObj, _pWPhysState);
	}
}

int CWObject_ChainEffect::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	const uint8 *pD = _pData;
	pD += CWObject_ChainEffect_Parent::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);

	if (_pObj->m_iClass == 0 || (pD - _pData) == 0)
		return (pD - _pData);

	CWO_ChainEffect_ClientData& CD = GetClientData(_pObj);
	if(_pObj->m_bAutoVarDirty)
		CD.AutoVar_Unpack(pD, _pWClient->GetMapData(), 0);

	if(!(CD.m_Rope.m_Flags & CXR_ROPE_VALID))
	{
		CXR_Rope& Rope = CD.m_Rope;

		CXR_Rope_CreationParam CreateParam;
		CreateParam.Set(CD.m_nBaseMasses, CD.m_MassWeight, CD.m_SpringK, CD.m_SpringL, CD.m_AirFricK, CD.m_SpringFricK, CD.m_CollRepelK, CD.m_CollFricK, CD.m_CollAbsK);
		
		//CreateParam.m_SpringK = 10000.0f;
		//CreateParam.m_SpringFricK = 0.8f;
		//CreateParam.m_AirFricK = 0.02f;
		Rope.Create(0, CreateParam, 0);
		Rope.StorePosition();
		Rope.m_LastUpdate = _pWClient->GetGameTime();//Time;
		Rope.m_Flags |= CXR_ROPE_VALID;
	}

	return(pD - _pData);
}

aint CWObject_ChainEffect::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
		case OBJMSG_CHAIN_EXPAND:
		{
			// Create a net message
			CNetMsg NetMsg(NETMSG_CHAIN_EXPAND);
			NetMsg.AddInt8((int8)_Msg.m_Param0);

			m_pWServer->NetMsg_SendToObject(NetMsg, m_iObject);
		}
		return 1;

		case OBJMSG_CHAIN_SHRINK:
		{
			// Create a net message and send it
			CNetMsg NetMsg(NETMSG_CHAIN_SHRINK);
			NetMsg.AddInt8((int8)_Msg.m_Param0);

			m_pWServer->NetMsg_SendToObject(NetMsg, m_iObject);
		}
		return 1;

		case OBJMSG_CHAIN_AUTOSHRINK:
		{
			const fp32 Time = (_Msg.m_Param0 > 0) ? *(fp32*)_Msg.m_Param0 : 1.0f;

			// Create a net message and send it
			CNetMsg NetMsg(NETMSG_CHAIN_AUTOSHRINK);
			NetMsg.Addfp32(Time);
			
			m_pWServer->NetMsg_SendToObject(NetMsg, m_iObject);
		}
		return 1;

		case OBJMSG_CHAIN_IMPULSE:
		{
			const CVec3Dfp32 Velocity = *(CVec3Dfp32*)_Msg.m_Param1;

			// Create a net message and send it
			CNetMsg NetMsg(NETMSG_CHAIN_IMPULSE);
			NetMsg.AddInt8((int8)_Msg.m_Param0);
			NetMsg.Addfp32(Velocity.k[0]);
			NetMsg.Addfp32(Velocity.k[1]);
			NetMsg.Addfp32(Velocity.k[2]);

			m_pWServer->NetMsg_SendToObject(NetMsg, m_iObject);
		}
		return 1;

		case OBJMSG_CHAIN_LENGTHTO:
		{
			const fp32 Length = (_Msg.m_Param0 > 0) ? *(fp32*)_Msg.m_Param0 : 0;
			const CVec3Dfp32 Point = (_Msg.m_Param1 > 0) ? *(CVec3Dfp32*)_Msg.m_Param1 : 0;

			// Create a net message
			CNetMsg NetMsg(NETMSG_CHAIN_LENGTHTO);
			NetMsg.Addfp32(Length);
			NetMsg.Addfp32(Point.k[0]);
			NetMsg.Addfp32(Point.k[1]);
			NetMsg.Addfp32(Point.k[2]);

			m_pWServer->NetMsg_SendToObject(NetMsg, m_iObject);
		}
		return 1;

		default:
			return CWObject_ChainEffect_Parent::OnMessage(_Msg);
	}
}

void CWObject_ChainEffect::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	CWO_ChainEffect_ClientData& CD = GetClientData(_pObj);

	switch(_Msg.m_MsgType)
	{
		case NETMSG_CHAIN_EXPAND:
		{
			if(!(CD.m_Rope.m_Flags & CXR_ROPE_VALID))
				break;

			// Get data
			int iPos = 0;
			int8 nRefObjects = _Msg.GetInt8(iPos);
			
			// Expand chain
			CD.m_Rope.Expand(nRefObjects);
		}
		break;

		case NETMSG_CHAIN_SHRINK:
		{
			if(!(CD.m_Rope.m_Flags & CXR_ROPE_VALID))
				break;

			// Get data
			int iPos = 0;
			int8 nRefObjects = _Msg.GetInt8(iPos);

			// Shrink chain
			CD.m_Rope.Shrink(nRefObjects);
		}
		break;

		case NETMSG_CHAIN_AUTOSHRINK:
		{
			if(!(CD.m_Rope.m_Flags & CXR_ROPE_VALID))
				break;

			// Get data
			int iPos = 0;
			fp32 ShrinkTime = _Msg.Getfp32(iPos);

			// Setup auto shrink chain function
			CD.m_Rope.AutoShrink(ShrinkTime);
		}
		break;

		case NETMSG_CHAIN_IMPULSE:
		{
			if(!(CD.m_Rope.m_Flags & CXR_ROPE_VALID))
				break;

			// Get data
			int iPos = 0;
			int8 iMass = _Msg.GetInt8(iPos);
			
			CVec3Dfp32 Velocity;
			Velocity.k[0] = _Msg.Getfp32(iPos);
			Velocity.k[1] = _Msg.Getfp32(iPos);
			Velocity.k[2] = _Msg.Getfp32(iPos);

			CD.m_Rope.Impulse(iMass, Velocity);
		}
		break;

		case NETMSG_CHAIN_LENGTHTO:
		{
			if(!(CD.m_Rope.m_Flags & CXR_ROPE_VALID))
				break;

			int iPos = 0;
			fp32 Length = _Msg.Getfp32(iPos);

			CVec3Dfp32 Point;
			Point.k[0] = _Msg.Getfp32(iPos);
			Point.k[1] = _Msg.Getfp32(iPos);
			Point.k[2] = _Msg.Getfp32(iPos);

			CD.m_Rope.ExpandTo(Length, Point);
		}
		break;

		default:
			break;
	}
}

void CWObject_ChainEffect::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_ChainEffect::OnClientRefresh);
	CWO_ChainEffect_ClientData& CD = GetClientData(_pObj);
	
	CXR_Rope& Rope = CD.m_Rope;

	CMTime Time = CMTime::CreateFromTicks(_pWClient->GetGameTick(), _pWClient->GetGameTickTime(), 0);
	
	// Make sure rope has been initialized, (done after first render call)
	if(Rope.m_Flags & CXR_ROPE_VALID)
	{
		// Shrink objects,
		Rope.UpdateAutoShrink(_pWClient->GetGameTickTime());

		// Nail down first and/or last
		if(CD.m_Connect0 || CD.m_Connect1)
		{
			if(CD.m_Connect0)
				Rope.SetMassConnectionPosition(Rope.GetFirstMass(), CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(), 3));
			if(CD.m_Connect1)
				Rope.SetMassConnectionPosition(Rope.GetLastMass(), CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(), 3));
		}

		// Nail down first and/or last bone to character
		else if((CD.m_BoneConnect0 || CD.m_BoneConnect1) && CD.m_iOwner)
		{
			CWObject_Client* pCharObj = _pWClient->Object_Get(CD.m_iOwner);
			if (!pCharObj)
				return;
			
			CXR_Skeleton* pCharSkel;
			
			CVec3Dfp32 CharAnimPos;
			CXR_AnimState CharAnim;
			CXR_SkeletonInstance* pCharSkelInst;
			
			if(CWObject_Character::GetEvaluatedPhysSkeleton(pCharObj, _pWClient, pCharSkelInst, pCharSkel, CharAnim))//, IPTime))
			{
				if(CD.m_BoneConnect0)
				{
					const uint32& RotTrack = CD.m_BoneConnect0;

					const CMat4Dfp32& Mat = pCharSkelInst->m_pBoneTransform[RotTrack];
					CVec3Dfp32 BonePos = pCharSkel->m_lNodes[RotTrack].m_LocalCenter;
					BonePos *= Mat;

					int iFirst = Rope.GetFirstMass();
					if(Rope.MassHasConnection(iFirst))
						Rope.SetMassConnectionVelocity(iFirst, (BonePos - Rope.GetMassPosition(iFirst)) / _pWClient->GetGameTickTime());
					else
					{
						Rope.SetMassConnectionPosition(iFirst, BonePos);
						Rope.SetMassConnectionVelocity(iFirst, CVec3Dfp32(0));
					}

					//_pWClient->Object_SetPosition(_pObj->m_iObject, BonePos);
				}

				if(CD.m_BoneConnect1)
				{
					const uint32& RotTrack = CD.m_BoneConnect1;
					
					const CMat4Dfp32& Mat = pCharSkelInst->m_pBoneTransform[RotTrack];
					CVec3Dfp32 BonePos = pCharSkel->m_lNodes[RotTrack].m_LocalCenter;
					BonePos *= Mat;

					int iLast = Rope.GetLastMass();
					if(Rope.MassHasConnection(iLast))
						Rope.SetMassConnectionVelocity(iLast, (BonePos - Rope.GetMassPosition(iLast)));
					else
					{
						Rope.SetMassConnectionPosition(iLast, BonePos);
						Rope.SetMassConnectionVelocity(iLast, CVec3Dfp32(0));
					}
				}
			}
		}

		// FIXME, make sure simulation can be run without connections so we can snap them from places
		if(!Rope.MassHasConnection(0))
		{			
			Rope.SetMassConnectionPosition(0, CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(), 3));
			Rope.SetMassConnectionVelocity(0, CVec3Dfp32(0));
		}

		// Store old position and run a simulation step
		Rope.StorePosition();

		const fp32 dt_max = 0.002f;
		fp32 dt = _pWClient->GetGameTickTime();
		Rope.m_LastUpdate = Time;

		const int nIter = (int)((dt / dt_max) + 1);
		if(nIter != 0)
			dt = dt / nIter;

		//for(int i = 0; i < nIter; i++)
			Rope.Operate(dt, nIter, _pWClient);

		//Rope.ConstrainSprings();
		//Rope.NormalizeVelocities();
	}
}

void CWObject_ChainEffect::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	CWO_ChainEffect_ClientData& CD = GetClientData(_pObj);

	CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pWClient->GetMapData()->GetResourceIndex_Model("Chain"));
    
	CXR_Rope& Rope = CD.m_Rope;

	// Interpolate position matrix
	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);
	
	// Update position if needed
	if((CD.m_BoneConnect0 || CD.m_BoneConnect1) && CD.m_iOwner)
	{
		CWObject_CoreData* pCharObj = _pWClient->Object_GetCD(CD.m_iOwner);
		if (!pCharObj)
			return;
		
		CXR_Skeleton* pCharSkel;
		
		CVec3Dfp32 CharAnimPos;
		CXR_AnimState CharAnim;
		CXR_SkeletonInstance* pCharSkelInst;
		
		CMat4Dfp32 CharMatIP;
		Interpolate2(pCharObj->GetLastPositionMatrix(), pCharObj->GetPositionMatrix(), CharMatIP, IPTime);

		if(CWObject_Character::GetEvaluatedPhysSkeleton(pCharObj, _pWClient, pCharSkelInst, pCharSkel, CharAnim, IPTime, &CharMatIP))
		{
			if(CD.m_BoneConnect0)
			{
				const uint32& RotTrack = CD.m_BoneConnect0;

				const CMat4Dfp32& Mat = pCharSkelInst->m_pBoneTransform[RotTrack];
				CVec3Dfp32 BonePos = pCharSkel->m_lNodes[RotTrack].m_LocalCenter;
				BonePos *= Mat;

				_pWClient->Object_SetPosition(_pObj->m_iObject, BonePos);
			}
		}
	}

	CMTime Time = CMTime::CreateFromTicks(_pWClient->GetGameTick(), _pWClient->GetGameTickTime(), IPTime);
	uint16 nRefObjs = (Rope.m_Flags & CXR_ROPE_VALID) ? Rope.GetNumRefObjs() : 1;

	{
		CXR_AnimState Anim = _pObj->GetDefaultAnimState(_pWClient);

		{
			// Initialize rope, DAMN, this is ugly, move it where where it should belong!
			/*
			if(!(Rope.m_Flags & CXR_ROPE_VALID))
			{
				CXR_Rope_CreationParam CreateParam;
				CreateParam.Set(CD.m_nBaseMasses, CD.m_MassWeight, CD.m_SpringK, CD.m_SpringL, CD.m_AirFricK, CD.m_SpringFricK, CD.m_CollRepelK, CD.m_CollFricK, CD.m_CollAbsK);
				
				//CreateParam.m_SpringK = 10000.0f;
				//CreateParam.m_SpringFricK = 0.8f;
				//CreateParam.m_AirFricK = 0.02f;
				Rope.Create(0, CreateParam, 0);
				Rope.StorePosition();
				Rope.m_LastUpdate = Time;
				Rope.m_Flags |= CXR_ROPE_VALID;
			}
			*/

			//Rope.Update();
		}

		// Render rope debug lines
		for(uint16 i = 0; i < nRefObjs; i++)
		{
			Rope.GetIPPosition(IPTime, i, true);
			#ifndef M_RTM
				Rope.Debug_RenderRopeMasses(i, _pWClient);
			#endif
		}
				
		// Add model for rendering
		if(pModel && (Rope.m_Flags & CXR_ROPE_VALID))
		{
			Anim.m_Data[0] = (aint)&Rope;
			Anim.m_Data[1] = (aint)&CD;
			_pEngine->Render_AddModel(pModel, MatIP, Anim);
			/* Fuck it!!
			if(i == 0)
			{
				_pEngine->Render_AddModel(pModel, MatIP, Anim);
				//_pEngine->Render_AddModel(pModel, MatIP, Anim);
			}
			else
			{
				_pEngine->Render_AddModel(pModel, SegmentPos, Anim);
				// Calc new positon for this model
				//_pEngine->Render_AddModel(pModel, SegmentPos, Anim);
			}
			*/
		}
	}
}

