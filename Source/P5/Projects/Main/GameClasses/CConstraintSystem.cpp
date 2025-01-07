
#include "PCH.h"
#include "CConstraintSystemClient.h"
#include "CConstraintSystem.h"

#undef	_DRAW_SYSTEM

#define kConstraintPointVelocityDamping			0.9999f
#define kConstraintDefaultGravity				15.0f
#define kConstraintDefaultImpactForceFactor		5.0f
#define kConstraintDefaultStopThreshold			0.2f
#define kConstraintDefaultIterations			5
#define kConstraintNoReboundFrames				5
#define kConstraintDefaultBlendFrames			1			
#define kConstraintDefaultReboundFactor			0.5f
#define kConstraintBodypartSoundInterval		20
#define kConstraintWeakImpactSoundThreshold		2.0f
#define kConstraintFirstImpactSoundThreshold	4.0f
#define kConstraintImpactSoundThreshold			6.0f

SConstraintSystemSettings::SConstraintSystemSettings()
{
	m_SkeletonType = RAGDOLL_DEFAULT;
	m_ConstraintIterations = kConstraintDefaultIterations;
	m_BlendFrames = kConstraintDefaultBlendFrames;
	m_Rebound = kConstraintDefaultReboundFactor;
	m_StopThreshold = kConstraintDefaultStopThreshold;
	m_pSkelInstance = NULL;
	m_pSkeleton = NULL;
};

// =====================================================================================================
// CConstraintSystem
// =====================================================================================================

#define MACRO_WRITE(_type, _data) {_type TempApa = _data; _pFile->WriteLE(TempApa);}
#define MACRO_READ(_type, _var) {_type TempApa; _pFile->ReadLE(TempApa); _var = TempApa; }

CQuatfp32 CConstraintSystem::RotationArcQuaternion(const CVec3Dfp32& _v0,const CVec3Dfp32& _v1)
{
	CVec3Dfp32 v0,v1;
	CQuatfp32 Q;
	CVec3Dfp32 C;
	fp32 D,S;
	v0 = _v0;
	v1 = _v1;
	v0.Normalize();	// Needed?
	v1.Normalize();	// Needed?
	v0.CrossProd(v1,C);
	D = v0 * v1;

	// What if D gets too close to -1?
	S = M_Sqrt(1.0f+D) * 2.0f;
	Q.k[0] = C.k[0] / S;
	Q.k[1] = C.k[1] / S;
	Q.k[2] = C.k[2] / S;
	Q.k[3] = S / 2.0f;
	// Q.Normalize();

	// Testing Q
	CMat4Dfp32 Mat;
	Q.CreateMatrix(Mat);
//	CVec3Dfp32 p =  _v0 * Mat;
//	fp32 delta = (_v0 - _v1).Length();
//	fp32 diff = (p - _v1).Length();

	return(Q);
};

CQuatfp32 CConstraintSystem::RotationArcQuaternion2(const CVec3Dfp32& _v0,const CVec3Dfp32& _v1)
{
	CQuatfp32 Q;
	CVec3Dfp32 n1,n2,axis;
	fp32 d,qw;
	
	n1 = _v0;
	n2 = _v1;
	n1.Normalize();
	n2.Normalize();
	d = n1 * n2;
	n1.CrossProd(n2,axis);
	qw = M_Sqrt(_v0.LengthSqr() * _v1.LengthSqr()) + d;
	if (qw < 0.0001f)
	{	// _v0, _v1 are aproximately 180 degrees apart
		Q.k[0] = -_v0[2];
		Q.k[1] = _v0[1];
		Q.k[2] = _v0[0];
		Q.k[3] = 0.0f;
	}
	else
	{
		Q.k[0] = axis[0];
		Q.k[1] = axis[1];
		Q.k[2] = axis[2];
		Q.k[3] = qw;
	}
	Q.Normalize();

	// Testing Q
	CMat4Dfp32 Mat;
	Q.CreateMatrix(Mat);
//	CVec3Dfp32 p =  _v0 * Mat;
//	fp32 delta = (_v0 - _v1).Length();
//	fp32 diff = (p - _v1).Length();

	return(Q);
};

CMat4Dfp32 CConstraintSystem::RotationArcMatrix(const CVec3Dfp32& _v0,const CVec3Dfp32& _v1)
{
	CVec3Dfp32 v0,v1;
	v0 = _v0;
	v1 = _v1;
	CMat4Dfp32 InvMat,Tmp,Tmp2,Mat;
	Tmp.Unit();
	CVec3Dfp32 Up = CVec3Dfp32(0,0,1);
	CVec3Dfp32 Left = CVec3Dfp32(0,1,0);
	if (M_Fabs(Up * v0) < M_Fabs(Left * v0))
	{
		v0.SetRow(Tmp,0);
		Up.SetRow(Tmp,2);
		Tmp.RecreateMatrix<0,2>();
		Tmp.Inverse3x3(InvMat);
		InvMat.UnitNot3x3();
	}
	else
	{
		v0.SetRow(Tmp,0);
		Left.SetRow(Tmp,1);
		Tmp.RecreateMatrix<0,1>();
		Tmp.Inverse3x3(InvMat);
		InvMat.UnitNot3x3();
	}

	Tmp.Unit();
	if (M_Fabs(Up * v1) < M_Fabs(Left * v1))
	{
		v1.SetRow(Tmp,0);
		Up.SetRow(Tmp,2);
		Tmp.RecreateMatrix<0,2>();
		Tmp.UnitNot3x3();
	}
	else
	{
		v1.SetRow(Tmp,0);
		Left.SetRow(Tmp,1);
		Tmp.RecreateMatrix<0,1>();
		Tmp.UnitNot3x3();
	}
	InvMat.Multiply(Tmp,Mat);

//	CVec3Dfp32 p = v0 * Mat;
//	fp32 delta = (_v0 - _v1).Length();
//	fp32 diff = (p - _v1).Length();

	return(Mat);
};

CMat4Dfp32 CConstraintSystem::RotationArcMatrix2(const CVec3Dfp32& _v0,const CVec3Dfp32& _v1)
{
	CVec3Dfp32 v0,v1,n0,n1,c;
	fp32 d,a;
	CMat4Dfp32 Mat;
	v0 = _v0;
	v1 = _v1;
	n0 = v0.Normalize();
	n1 = v1.Normalize();
	n1.CrossProd(n0,c);
	d = n0 * n1;
	Mat.Unit();
	if (M_Fabs(d) <= 0.9999f)
	{	// At least some angle between the vectors
		a = M_ACos(d) / _PI2;
		c.Normalize();
		c.CreateAxisRotateMatrix(a,Mat);
		Mat.RecreateMatrix<2,1>();
	}
	else
	{
		// ***
//		bool wtf = true;
		// ***
	}

//	CVec3Dfp32 p = v0 * Mat;
//	fp32 delta = (v0 - v1).Length();
//	fp32 diff = (p - v1).Length();

	return(Mat);
};

CConstraintSystem::CConstraintSystem()
{
	m_SystemType = CS_SKELETON;

	m_bInited = false;
	m_bFirstApply = true;
	m_bFirstCollCheck = true;
	m_pWPhysState = NULL;
	m_pClient = NULL;
	m_pCD = NULL;
	m_IdleTicks = 0x7fff;
	Clear();
	m_PtRadius = 5.0f;	// Very small, very bad radius
	m_SystemMat.Unit();
}

int16 CConstraintSystem::SetRebound(fp32 _Rebound)
{
	if ((_Rebound >= 0.0f)&&(_Rebound <= 1.0f))
	{
		m_ReboundFactor = _Rebound;
		return(kSuccess);
	}
	else
	{
		return(kErrParams);
	}
}

int16 CConstraintSystem::SetStopThreshold(fp32 _StopThreshold)
{
	if (_StopThreshold > 0.0f)
	{
		m_StopThreshold = _StopThreshold;
		return(kSuccess);
	}
	else
	{
		return(kErrParams);
	}
}

int16 CConstraintSystem::SetConstraintIterations(int _ConstraintIterations)
{
	if (_ConstraintIterations > 1)
	{
		m_ConstraintIterations = _ConstraintIterations;
		return(kSuccess);
	}
	else
	{
		return(kErrParams);
	}
}

int16 CConstraintSystem::SetGravity(fp32 _Gravity)
{
	if (_Gravity >= 0.0f)
	{
		m_Gravity = _Gravity;
		return(kSuccess);
	}
	else
	{
		return(kErrParams);
	}
}

void CConstraintSystem::SetState(int _State)
{
	if (_State != m_State)
	{
		if (_State == STOPPED)
		{
			m_liPushObjects.Clear();
			m_bFirstCollCheck = true;
			m_State = _State;
			if (m_pSystemClient)
			{
				m_pSystemClient->m_State = _State;
			}
			// Object should not be affected by physics
			m_pCD->m_Phys_Flags = m_pCD->m_Phys_Flags | 
				(PLAYER_PHYSFLAGS_NOGRAVITY | PLAYER_PHYSFLAGS_NOCHARACTERCOLL | PLAYER_PHYSFLAGS_NOWORLDCOLL);
			PackStopped();
		}
		else if (m_State == STOPPED)
		{
			m_bFirstCollCheck = true;
			UnPackStopped();
			m_State = _State;
			if (m_pSystemClient)
			{
				m_pSystemClient->m_State = _State;
			}
		}
		else
		{
			m_State = _State;
			if (m_pSystemClient)
			{
				m_pSystemClient->m_State = _State;
			}
		}
	}
};

int16 CConstraintSystem::GetState()
{
	if (m_State >= STOPPED)
	{
		return(STOPPED);
	}
	else
	{
		return m_State;
	}
};

bool CConstraintSystem::IsStopped()
{
	return (m_State >= STOPPED);
};

void CConstraintSystem::Setup(SConstraintSystemSettings* _pSettings)
{
	SetConstraintIterations(_pSettings->m_ConstraintIterations);
	SetStopThreshold(_pSettings->m_StopThreshold);
	SetRebound(_pSettings->m_Rebound);
	SetupSkeleton(_pSettings->m_SkeletonType, _pSettings->m_pSkeleton, _pSettings->m_pSkelInstance, _pSettings->m_lpClothSkeleton);
};

void CConstraintSystem::Init(int _iObj, CWObject_CoreData* _pClient, CWorld_PhysState* _pWPhysState,
							 CWO_Character_ClientData* _pCD, CConstraintSystemClient* _pSystemClient, int _nTracks)
{
	if (m_bInited)
		return;

	m_SystemType = CS_SKELETON;
	m_bInited = true;
	m_bFirstApply = true;
	m_iObject = _iObj;
	m_pClient = _pClient;
	m_pWPhysState = _pWPhysState;
	m_pCD = _pCD;
	m_pSystemClient = _pSystemClient;
 
	if (_nTracks == 0)
	{
		// Setup the past and present rota and move tracks to interpolate between
		CXR_Model* pModel = m_pWPhysState->GetMapData()->GetResource_Model(m_pClient->m_iModel[0]);
		if (pModel)
		{
			CXR_Skeleton* pSkel = pModel->GetPhysSkeleton();
			if (pSkel)
				m_nTracks = Max( pSkel->m_nUsedRotations, pSkel->m_nUsedMovements );
		}
	}
	else
	{
		m_nTracks = _nTracks;
	}

	m_pSystemClient->Init(m_nTracks);
}

void CConstraintSystem::PackStopped()
{
	if (m_State != STOPPED)
	{
		return;
	}
	m_lDefaultPts.Clear();
	m_lDefaultMats.Clear();
};

void CConstraintSystem::UnPackStopped()
{
	if (m_State == STOPPED)
	{
		GetDefaultPose();
	}
};

void CConstraintSystem::OnDeltaLoad(CCFile* _pFile)
{
	MACRO_READ(int16, m_State);
	MACRO_READ(int16, m_IdleTicks);

	// Read m_lpPts
	uint8 N;
	MACRO_READ(uint8,N);
	for (int i = 0; i < N; i++)
	{
		uint16 Temp16;
		MACRO_READ(uint16,Temp16);
		if ((Temp16 < m_lpPts.Len())&&(m_lpPts[Temp16]))
		{
			m_lpPts[Temp16]->OnDeltaLoad(_pFile);
		}
	}
	// Invalidate any collision box
	m_CollMinBox = CVec3Dfp32(_FP32_MAX);
	m_CollMaxBox = CVec3Dfp32(-_FP32_MAX);
	m_MoveMinBox = CVec3Dfp32(_FP32_MAX);
	m_MoveMaxBox = -CVec3Dfp32(_FP32_MAX);
	m_iCollSelection = -1;
	m_lCollPolys.Clear();
	m_lDynamicCollPolys.Clear();

	m_AnimateCount = 0;
};

void CConstraintSystem::OnDeltaSave(CCFile* _pFile)
{
	MACRO_WRITE(int16, m_State);
	MACRO_WRITE(int16, m_IdleTicks);

	// Save m_lpPts
	uint8 N = 0;
	for (int16 i = 0; i < m_lpPts.Len(); i++)
	{
		if (!m_lpPts[i]) {continue;}
		N++;
	}

	// Save nbr of used m_lpPts
	MACRO_WRITE(uint8, N);
	// Save each index and corr. CConstraintPt
	for (int16 i = 0; i < m_lpPts.Len(); i++)
	{
		if (!m_lpPts[i]) {continue;}
		MACRO_WRITE(int16,i);
		m_lpPts[i]->OnDeltaSave(_pFile);
	}
};

// Clear the contents of the system (for reuse)
void CConstraintSystem::Clear(bool _bAll)
{
	if (_bAll)
	{
		m_State = NOTREADY;
		m_IdleTicks = 0x7fff;
		m_pSystemClient = NULL;
	}
	else
	{
		m_IdleTicks = 0;
	}

	m_LastFrameTick = 0;

	for (int i = 0; i < m_lpRules.Len(); i++)
	{
		CConstraintRule* cur = m_lpRules[i];
		// No point in checking if cur == NULL, delete on NULL is a NOP
		delete cur;
	}
	m_lpRules.Clear();

	for (int i = 0; i < m_lpPts.Len(); i++)
	{
		CConstraintPoint* cur = m_lpPts[i];
		// No point in checking if cur == NULL, delete on NULL is a NOP
		delete cur;
	}
	m_lpPts.Clear();

	if (_bAll)
	{
		for (int i = 0; i < BODYPARTS_NUM; i++)
		{
			m_liBodypartSound[i] = -1;	// No sound
			m_lBodyPartSoundNextTick[i] = 0;
		}
		m_liBodypartSoundWeak = -1;
		m_lBodyPartSoundWeakNextTick = -1;

		m_ConstraintIterations = kConstraintDefaultIterations;
	}

	CVec3Dfp32 VZero( 0.0f );
	CVec3Dfp32 VMax(_FP32_MAX);
	
	if (_bAll)
	{
		m_OrgMat.Unit();
		m_nTracks = 0;
	}

	m_SystemVelocity = VZero;
	m_PrevSystemVelocity = VZero;
	m_SystemAvgPos = VZero;

	m_iPendingBone = -1;
	m_PendingBonePos = VMax;
	m_bPendingBonePosKeep = false;
	m_PendingImpulse = VZero;
	m_PendingImpulsePos = VMax;
	
	m_bCoordSysAvailable = false;

	m_iSystemUp0 = -1;
	m_iSystemUp1 = -1;
	m_iSystemLeft0 = -1;
	m_iSystemLeft1 = -1;

	if (_bAll)
	{
		if(m_pSystemClient)
			m_pSystemClient->Clear();
		m_StopThreshold = kConstraintDefaultStopThreshold;
		m_ReboundFactor = kConstraintDefaultReboundFactor;
		m_Gravity = kConstraintDefaultGravity;
	}

	m_bFirstCollCheck = true;
	m_CurCollCount = 0;
	m_bCollReady = false;
	m_CollMinBox = VMax;
	m_CollMaxBox = -VMax;
	m_MoveMinBox = VMax;
	m_MoveMaxBox = -VMax;
	m_iCollSelection = -1;

	m_liPushObjects.Clear();
}

// As all CConstraintPoint, CConstraintRule etc were created by me, I must also get rid of them
CConstraintSystem::~CConstraintSystem()
{
	CConstraintSystem::Clear();
};

void CConstraintSystem::SetOrgMat(const CMat4Dfp32& _OrgPos)
{
	m_OrgMat =_OrgPos;
};

void CConstraintSystem::Activate(bool _Active)
{
	if (_Active)
	{
		if (m_pCD)
		{	// We handle the physics from hereon
			m_pCD->m_Phys_Flags = m_pCD->m_Phys_Flags | 
				(PLAYER_PHYSFLAGS_NOGRAVITY | PLAYER_PHYSFLAGS_NOCHARACTERCOLL | PLAYER_PHYSFLAGS_NOWORLDCOLL);
		}

		if ((GetState() == NOTREADY)&&(m_pSystemClient))
		{	
			m_pSystemClient->m_OrgMat = m_OrgMat;
		}
		m_LastFrameTick = 0;
		m_AnimateCount = 0;
		SetState(GETFRAME);
	}
	else
	{
		SetState(NOTREADY);
	}
}

bool CConstraintSystem::IsActive()
{
	if (GetState() == NOTREADY)
	{
		return(false);
	}
	else
	{
		return(true);
	}
}

bool CConstraintSystem::IsInited()
{
	return(m_bInited);
}

// Set up a skeleton according to the keys
void CConstraintSystem::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
}

int16 CConstraintSystem::SetRootPos(const CVec3Dfp32& _Pos)
{
	if (m_lpPts.Len() > 0)
	{
		m_lpPts[0]->SetPos(_Pos);
		return(kSuccess);
	}
	else
	{
		return(kFailure);
	}
	return(kFailure);
}

int16 CConstraintSystem::GetRootPos(CVec3Dfp32* _pReturnPos)
{
	if (m_lpPts.Len() > 0)
	{
		*_pReturnPos = m_lpPts[0]->m_Pos;
		return(kSuccess);
	}
	else
	{
		return(kFailure);
	}
}

int16 CConstraintSystem::SetupCoordinateSystem(int16 _iUp0,int16 _iUp1,int16 _iLeft0,int16 _iLeft1)
{
	if ((_iUp0 == _iUp1)||
		(_iUp0 < 0)||(_iUp0 >= m_lpPts.Len())||
		(_iUp1 < 0)||(_iUp1 >= m_lpPts.Len())||
		(_iLeft0 == _iLeft1)||
		(_iLeft0 < 0)||(_iLeft0 >= m_lpPts.Len())||
		(_iLeft1 < 0)||(_iLeft1 >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}

	m_iSystemUp0 = _iUp0;
	m_iSystemUp1 = _iUp1;
	m_iSystemLeft0 = _iLeft0;
	m_iSystemLeft1 = _iLeft1;
	m_bCoordSysAvailable = true;
	UpdateCoordinateSystem();
	
	return(kSuccess);
}

void CConstraintSystem::UpdateCoordinateSystem()
{
	MSCOPESHORT(CConstraintSystem::UpdateCoordinateSystem);

	if (m_bCoordSysAvailable)
	{
		CVec3Dfp32 Up = m_lpPts[m_iSystemUp1]->m_Pos - m_lpPts[m_iSystemUp0]->m_Pos;
		CVec3Dfp32 Left =  m_lpPts[m_iSystemLeft1]->m_Pos - m_lpPts[m_iSystemLeft0]->m_Pos;
		Left.SetRow(m_SystemMat,1);
		Up.SetRow(m_SystemMat,2);
		m_SystemMat.RecreateMatrix<2,1>();
		m_lpPts[m_iSystemUp0]->m_Pos.SetRow(m_SystemMat,3);
	}
}

void CConstraintSystem::SetPointRadius(fp32 _Radius)
{
	m_PtRadius = _Radius;
};

// Adds a CConstraintPoint with the given params to the system
// Returns its index or a negative errcode
int16 CConstraintSystem::AddPoint(const CVec3Dfp32& _Pos,fp32 _Mass,bool _Collision,int _BodyPart)
{
	CConstraintPoint* pPt = DNew(CConstraintPoint) CConstraintPoint();
	if (!pPt) {return(kErrMemory);}

	pPt->Setup(_Mass,_Collision,_BodyPart);
	pPt->SetPos(_Pos);
	m_lpPts.Add(pPt);
	return(m_lpPts.Len()-1);
}

// Adds a CConstraintPoint at a given index with the given params to the system
int16 CConstraintSystem::AddPoint(int16 _iP,const CVec3Dfp32& _Pos,fp32 _Mass,bool _Collision,int _BodyPart)
{
	if (_iP < 0) {return(kErrParams);}

	while(_iP >= m_lpPts.Len())
	{
		m_lpPts.Add((CConstraintPoint*)NULL);
	}

	if (m_lpPts[_iP] == NULL)
	{
		CConstraintPoint* pPt = DNew(CConstraintPoint) CConstraintPoint();
		if (!pPt) {return(kErrMemory);}
		m_lpPts[_iP] = pPt;
		m_lpPts[_iP]->Setup(_Mass,_Collision,_BodyPart);
		pPt->SetPos(_Pos);
		return(kSuccess);
	}
	else
	{
		return(kErrParams);
	}
}

int CConstraintSystem::GetBodypart(const CVec3Dfp32& _Pos)
{
	fp32 BestSqr = 1000000.0;
	// We initially set BestPart to BODY_ALL
	// Thus when no body part is found aside from BODY_NONE we will apply
	// return the entire body (_Pos is closest top the entire body)
	int BestPart = BODY_ALL;

	for (int i = 0; i < m_lpPts.Len(); i++)
	{
		if (m_lpPts[i] != NULL)
		{
			CConstraintPoint* pPt = m_lpPts[i];
			if ((pPt->m_iBodypart != BODY_NONE)&&(pPt->m_iBodypart != BODY_ALL))
			{
				fp32 CurSqr = _Pos.DistanceSqr(pPt->GetPos());
				if (CurSqr < BestSqr)
				{
					BestSqr = CurSqr;
					BestPart = pPt->m_iBodypart;
				}
			}
		}
	}

	return(BestPart);
}

void CConstraintSystem::ScaleSpeed(fp32 _Factor)
{
	for (int i = 0; i < m_lpPts.Len(); i++)
	{
		if (m_lpPts[i] != NULL)
		{
			CConstraintPoint* pPt = m_lpPts[i];
			if (pPt)
			{
				CVec3Dfp32 Speed = pPt->m_Pos - pPt->m_PrevPos;
				pPt->m_PrevPos = pPt->m_Pos - (Speed * _Factor);
			}
		}
	}
};

void CConstraintSystem::SetSpeed(CVec3Dfp32 _Speed,fp32 _Random)
{
	for (int i = 0; i < m_lpPts.Len(); i++)
	{
		if (m_lpPts[i] != NULL)
		{
			CConstraintPoint* pPt = m_lpPts[i];
			if (pPt)
			{
				CVec3Dfp32 Force = _Speed * (1.0f + _Random * (Random - Random));
				pPt->m_PrevPos = pPt->m_Pos - _Speed;
			}
		}
	}
};

void CConstraintSystem::AddSpeed(CVec3Dfp32 _Speed,fp32 _Random)
{
	for (int i = 0; i < m_lpPts.Len(); i++)
	{
		if (m_lpPts[i] != NULL)
		{
			CConstraintPoint* pPt = m_lpPts[i];
			if (pPt)
			{
				CVec3Dfp32 Force = _Speed * (1.0f + _Random * (Random - Random));
				pPt->m_Pos += Force;
			}
		}
	}
};


void CConstraintSystem::AddPendingImpulse(const CVec3Dfp32& _Pos,const CVec3Dfp32& _Impulse)
{
	m_PendingImpulse += _Impulse * kConstraintDefaultImpactForceFactor;
	m_PendingImpulsePos = _Pos;

	// Restart the ragdoll if new impulse was added
	if (IsStopped())
	{
		m_AnimateCount = 1;
		SetState(READY);
	}
}

void CConstraintSystem::AddPushObject(int _iPusher)
{
	// Avoid duplicates
	for (int i = 0; i < m_liPushObjects.Len(); i++)
	{
		if (_iPusher == m_liPushObjects[i])
		{
			return;
		}
	}
	m_liPushObjects.Add(_iPusher);
	// Restart the ragdoll if new impulse was added
	if (IsStopped())
	{
		m_AnimateCount = 1;
		SetState(READY);
	}
};

// Applies any pending impulse, 100% to the closest point, 50% to the second closest
void CConstraintSystem::ApplyPendingImpulse()
{
	if (m_PendingImpulse == CVec3Dfp32(0.0f))
	{
		return;
	}

	if (m_PendingImpulsePos == CVec3Dfp32(_FP32_MAX))
	{
		AddSpeed(m_PendingImpulse);
		m_PendingImpulsePos = CVec3Dfp32(_FP32_MAX);
		m_PendingImpulse = CVec3Dfp32(0.0f);
		return;
	}

	fp32 BestSqr = 1000000.0;
	int iFirst = -1;
	int iSecond = -1;
	for (int i = 0; i < m_lpPts.Len(); i++)
	{
		if (m_lpPts[i] != NULL)
		{
			CConstraintPoint* pPt = m_lpPts[i];
			if ((pPt)&&(pPt->m_bCollision)&&(pPt->m_bImpact))
			{
				fp32 CurSqr = m_PendingImpulsePos.DistanceSqr(pPt->GetPos());
				if (CurSqr < BestSqr)
				{
					BestSqr = CurSqr;
					iSecond = iFirst;
					iFirst = i;
				}
			}
		}
	}

	CVec3Dfp32 V;
//	bool bPartial = false;
	if (iFirst != -1)
	{	// Add m_PendingImpulse to iFirst
		V = m_PendingImpulse * m_lpPts[iFirst]->m_InvMass;
		m_lpPts[iFirst]->ApplyImpulse(V);
	}
	if (iSecond != -1)
	{	// Add m_PendingImpulse * 0.5f to iSecond
		V = m_PendingImpulse * m_lpPts[iSecond]->m_InvMass * 0.5f;
		m_lpPts[iSecond]->ApplyImpulse(V);
	}

	m_PendingImpulsePos = CVec3Dfp32(_FP32_MAX);
	m_PendingImpulse = CVec3Dfp32(0.0f);
}

int16 CConstraintSystem::SetPendingBonepos(int _iBone,const CVec3Dfp32& _Pos, bool _bKeep)
{
	if (_iBone >= m_lpPts.Len())
	{
		return(kErrIndexOutOfRange);
	}
	if ((_iBone >= 0)&&(m_lpPts[_iBone] == NULL))
	{
		return(kErrParams);
	}

	if ((_iBone != -1)&&(IsStopped())&&((_iBone != m_iPendingBone)||(_Pos != m_PendingBonePos)))
	{
		SetState(READY);
	}

	if (_iBone > -1)
	{
		m_iPendingBone = _iBone;
		m_PendingBonePos = _Pos;
		m_bPendingBonePosKeep = _bKeep;
		m_AnimateCount = 0;
	}
	else
	{
		m_iPendingBone = -1;
		m_PendingBonePos = CVec3Dfp32(_FP32_MAX);
		m_bPendingBonePosKeep = false;
	}

	return(kSuccess);
}

int16 CConstraintSystem::AddImpulse(int _BodyPart,const CVec3Dfp32& _Impulse)
{
	MSCOPESHORT(CConstraintSystem::AddImpulse);
	if (_BodyPart == BODY_NONE)
	{
		return(kFailure);
	}
	if ((_BodyPart < 0)||(_BodyPart >= BODYPARTS_NUM))
	{
		return(kErrParams);
	}

	int16 rErr = kFailure;
	for (int i = 0; i < m_lpPts.Len(); i++)
	{
		if (m_lpPts[i] != NULL)
		{
			CConstraintPoint* pPt = m_lpPts[i];
			if (_BodyPart == BODY_ALL)
			{	// All none BODY_NONE will do
				if (pPt->m_iBodypart != BODY_NONE)
				{
					pPt->ApplyImpulse(_Impulse);
					rErr = kSuccess;
				}
			}
			else
			{	// Only exact match will do
				if (pPt->m_iBodypart == _BodyPart)
				{
					pPt->ApplyImpulse(_Impulse);
					rErr = kSuccess;
				}
			}
		}
	}

	if ((rErr == kSuccess)&&(IsStopped()))
	{	// Restart the ragdoll if new impulse was added
		m_AnimateCount = 1;
		SetState(READY);
	}

	return(rErr);
}

int16 CConstraintSystem::AddBodypartSound(int _BodyPart,int _SoundIndex)
{
	if (_BodyPart == BODY_NONE)
	{	// We treat this index as a generic weak part sound
		m_liBodypartSoundWeak = _SoundIndex;
		return(kSuccess);
	}
	if ((_BodyPart < 0)||(_BodyPart >= BODYPARTS_NUM))
	{
		return(kErrParams);
	}
	m_liBodypartSound[_BodyPart]  = _SoundIndex;

	return(kSuccess);
}

void CConstraintSystem::SetMaterialSound(int _Material, CWObject_CoreData *_pObj, CWorld_Server *_pWServer)
{
	if(!_pObj)
		return;
	CWRes_Dialogue* pDialogue = CWObject_RPG::GetDialogueResource(_pObj, _pWServer);

	if (!pDialogue)
	{
		ConOut(CStrF("§cf80WARNING: Unable to set material sound. %i", _pObj->m_iObject));
		return;
	}

	int iSoundWeak = pDialogue->GetSoundIndex_Hash(StringToHash("fol_bodyfall_weak"), _pWServer->GetMapData());
	int iSoundAll = pDialogue->GetSoundIndex_Hash(StringToHash("fol_bodyfall_all"), _pWServer->GetMapData());
	int iSoundBody = pDialogue->GetSoundIndex_Hash(StringToHash("fol_bodyfall_body"), _pWServer->GetMapData());
	int iSoundHead = pDialogue->GetSoundIndex_Hash(StringToHash("fol_bodyfall_head"), _pWServer->GetMapData());
	int iSoundRArm = pDialogue->GetSoundIndex_Hash(StringToHash("fol_bodyfall_right_arm"), _pWServer->GetMapData());
	int iSoundLArm = pDialogue->GetSoundIndex_Hash(StringToHash("fol_bodyfall_left_arm"), _pWServer->GetMapData());
	int iSoundRLeg = pDialogue->GetSoundIndex_Hash(StringToHash("fol_bodyfall_right_leg"), _pWServer->GetMapData());
	int iSoundLLeg = pDialogue->GetSoundIndex_Hash(StringToHash("fol_bodyfall_left_leg"), _pWServer->GetMapData());

	AddBodypartSound(BODY_NONE, iSoundWeak);
	AddBodypartSound(BODY_ALL, iSoundAll);
	AddBodypartSound(BODY_PART, iSoundBody);
	AddBodypartSound(HEAD_PART, iSoundHead);
	AddBodypartSound(R_ARM_PART, iSoundRArm);
	AddBodypartSound(L_ARM_PART, iSoundLArm);
	AddBodypartSound(R_LEG_PART, iSoundRLeg);
	AddBodypartSound(L_LEG_PART, iSoundLLeg);
	m_iMaterial = _Material;
}

bool CConstraintSystem::PlayBodypartSound(int _BodyPart,const CVec3Dfp32& _Pos,fp32 _Impact)
{
	if ((_BodyPart < BODY_NONE)||(_BodyPart >= BODYPARTS_NUM))
	{
		return(kErrParams);
	}

	if (_Impact < kConstraintWeakImpactSoundThreshold)
	{
		return(kFailure);
	}

	if ((_Impact < kConstraintFirstImpactSoundThreshold)&&(_Impact < kConstraintImpactSoundThreshold))
	{
		_BodyPart = BODY_NONE;
	}

	if (m_SystemType == CS_GIBSYSTEM)
	{	// Override and write specific for the gib client only case
		return(kFailure);
	}

	int iDialogue = -1;
	int NextTick = 0;

	// No heavy sounds while dragging
	if ((_BodyPart != BODY_NONE)&&(m_iPendingBone == -1))
	{
		if (m_lBodyPartSoundNextTick[_BodyPart] == 0)
		{	// First impact
			if (_Impact < kConstraintFirstImpactSoundThreshold)
			{
				return(kFailure);
			}
		}
		else
		{	// Second+ impact
			if (_Impact < kConstraintImpactSoundThreshold)
			{
				return(kFailure);
			}
		}
		iDialogue = m_liBodypartSound[_BodyPart];
		NextTick = m_lBodyPartSoundNextTick[_BodyPart];
	}
	else
	{	// Weak impact
		if (_Impact < kConstraintWeakImpactSoundThreshold)
		{
			return(kFailure);
		}
		iDialogue = m_liBodypartSoundWeak;
		NextTick = m_lBodyPartSoundWeakNextTick;
	}

	CWorld_Server* pWServer = safe_cast<CWorld_Server>(m_pWPhysState);
	if ((iDialogue > -1)&&(pWServer->GetGameTick() > NextTick))
	{
/*		CWRes_Dialogue* pDialogue = CWObject_RPG::GetDialogueResource(m_pClient,pWServer);
		if(!pDialogue || pDialogue->GetNumItems() <= iDialogue)
		{
			return(kFailure);
		}*/
		//int iSound = pDialogue->GetSoundIndex(iDialogue,pWServer->GetMapData());
		if(iDialogue > 0)
		{
			pWServer->Sound_On(m_iObject, iDialogue, WCLIENT_ATTENUATION_3D, m_iMaterial);
		}
		if (_BodyPart != BODY_NONE)
		{
			m_lBodyPartSoundNextTick[_BodyPart] = (int32)(pWServer->GetGameTick() + (0.5f+Random) * kConstraintBodypartSoundInterval);
		}
		else
		{
			m_lBodyPartSoundWeakNextTick = (int32)(pWServer->GetGameTick() + (0.5f+Random) * kConstraintBodypartSoundInterval);
		}
		return(kSuccess);
	}
	else
	{
		return(kFailure);
	}
}

// Adds a CBaseConstraintRule with the given params
// _Rule can be one of: kConstraintRuleSmaller,kConstraintRuleEqual,kConstraintRuleLarger,
// kConstraintRuleSmallerSoft,kConstraintRuleEqualSoft,kConstraintRuleLargerSoft.
// Returns the index of the rule or an errcode
int16 CConstraintSystem::AddConstraint(int16 _iPA,int16 _iPB,int16 _Rule,fp32 _Stiffness)
{
	if ((_iPA < 0)||(_iPA >= m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}
	if ((m_lpPts[_iPA] == NULL)||(m_lpPts[_iPB] == NULL))
	{
		return(kErrNull);
	}

	CConstraintRule* pRule = DNew(CConstraintRule) CConstraintRule(this);
	if (!pRule) {return(kErrMemory);}

	int16 rErr = pRule->Setup(_iPA,_iPB,_Rule,_Stiffness);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}

	m_lpRules.Add(pRule);

	return(m_lpRules.Len()-1);
}

int16 CConstraintSystem::AddBone(int16 _iPA,int16 _iPB,bool _bForce)
{
	if ((_iPA < 0)||(_iPA >= m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}
	if ((m_lpPts[_iPA] == NULL)||(m_lpPts[_iPB] == NULL))
	{
		return(kErrNull);
	}

	CConstraintRule* pRule = DNew(CConstraintRule) CConstraintRule(this);
	if (!pRule) {return(kErrMemory);}

	int16 rErr = pRule->Setup(_iPA,_iPB,kConstraintRuleEqual,kConstraintBoneStiffness);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}
	pRule->m_bForce = _bForce;
	m_lpPts[_iPB]->m_bImpact = false;

	pRule->m_bBone = true;
	m_lpPts[_iPA]->m_iBonePt = _iPB;
	m_lpRules.Add(pRule);

	return(m_lpRules.Len()-1);
}

int16 CConstraintSystem::AddChildBone(int16 _iRule,int16 _iChildBone)
{
	int16 rErr = kSuccess;

	if ((_iRule < 0)||(_iRule >= m_lpRules.Len()))
	{
		return(kErrIndexOutOfRange);
	}

	m_pSystemClient->GrowTracks(_iChildBone);

	rErr = m_lpRules[_iRule]->AddChildBone(_iChildBone);
	return(rErr);
}


void CConstraintSystem::AddFixedClothChildBones(const int16& _curBone, const int16& _iMin, const int16& _iMax, CXR_Skeleton* _lpClothSkeleton[CWO_NUMMODELINDICES])
{
	for(int iSkel = 0; iSkel < CWO_NUMMODELINDICES; iSkel++)
	{
		if(!_lpClothSkeleton[iSkel])
			continue;

		const CXR_SkeletonNode* pNodes = _lpClothSkeleton[iSkel]->m_lNodes.GetBasePtr();
		for(uint iCloth = 0; iCloth < _lpClothSkeleton[iSkel]->m_lCloth.Len(); iCloth++)
		{
			const int* pFixedJoints = _lpClothSkeleton[iSkel]->m_lCloth[iCloth].m_FixedJoints.GetBasePtr();
			const int  nFixedJoints = _lpClothSkeleton[iSkel]->m_lCloth[iCloth].m_FixedJoints.Len();

			for(uint iJoint = 0; iJoint < nFixedJoints; iJoint++)
			{
				const int16 iNodeParent = pNodes[pFixedJoints[iJoint]].m_iNodeParent;
				if(iNodeParent >= _iMin && iNodeParent <= _iMax)
					AddChildBone(_curBone, (int16)pFixedJoints[iJoint]);
			}
		}
	}
	
	//if(!_pSkel)
	//	return;

	//const CXR_SkeletonNode* pNodes = _pSkel->m_lNodes.GetBasePtr();
	//for(uint i = 0; i < _pSkel->m_lCloth.Len(); i++)
	//{
	//	const int* pFixedJoints = _pSkel->m_lCloth[i].m_FixedJoints.GetBasePtr();
	//	const int  nFixedJoints = _pSkel->m_lCloth[i].m_FixedJoints.Len();

	//	for(uint j = 0; j < nFixedJoints; j++)
	//	{
	//		const int16 iNodeParent = pNodes[pFixedJoints[j]].m_iNodeParent;
 //           if(iNodeParent >= _iMin && iNodeParent <= _iMax)
	//			AddChildBone(_curBone, (int16)pFixedJoints[j]);
	//	}
	//}
}


// Adds a constraint that will simply keep the point _iP0 at _Fract between _iPA and _iPB
// Constraining _iP0 will move _iPA and _iPB in a way to conserve angular momentum around _iP0
int16 CConstraintSystem::AddBoneSubPos(int16 _iP0,int16 _iPA,int16 _iPB,fp32 _Fract)
{
	if ((_iPA < 0)||(_iPA >= m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}
	if ((m_lpPts[_iPA] == NULL)||(m_lpPts[_iPB] == NULL))
	{
		return(kErrNull);
	}

	CConstraintSubPosRule* pRule = DNew(CConstraintSubPosRule) CConstraintSubPosRule(this);
	if (!pRule) {return(kErrMemory);}

	int16 rErr = pRule->Setup(_iP0,_iPA,_iPB,1.0f,_Fract);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}
	pRule->m_bBone = false;
	m_lpRules.Add(pRule);

	return(m_lpRules.Len()-1);
}

// Adds a constraint with a range that is the actual range multiplied by _RangeMultiplier
// It is an error if kConstraintRuleEqual or kConstraintRuleEqualSoft is supplied as _Rule
int16 CConstraintSystem::AddConstraintRangeMultiplier(int16 _iPA,int16 _iPB,int16 _Rule,fp32 _Stiffness,fp32 _RangeMultiplier)
{
	if ((_iPA < 0)||(_iPA >= m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}
	if ((m_lpPts[_iPA] == NULL)||(m_lpPts[_iPB] == NULL))
	{
		return(kErrNull);
	}
	CConstraintRule* pRule = DNew(CConstraintRule) CConstraintRule(this);
	if (!pRule) {return(kErrMemory);}

	int16 rErr = pRule->Setup(_iPA,_iPB,_Rule,_Stiffness);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}
	rErr = pRule->SetRangeMultiplier(_RangeMultiplier);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}

	m_lpRules.Add(pRule);

	return(m_lpRules.Len()-1);
}

// Adds a CBaseConstraintRule with the given params
// _Rule can be one of: kConstraintRuleSmaller,kConstraintRuleEqual,kConstraintRuleLarger,
// kConstraintRuleSmallerSoft,kConstraintRuleEqualSoft,kConstraintRuleLargerSoft.
// Returns the index of the rule or an errcode
int16 CConstraintSystem::AddConstraintRangeValue(int16 _iPA,int16 _iPB,int16 _Rule,fp32 _Stiffness,fp32 _RangeValue)
{
	if ((_iPA < 0)||(_iPA >= m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}
	if ((m_lpPts[_iPA] == NULL)||(m_lpPts[_iPB] == NULL))
	{
		return(kErrNull);
	}

	CConstraintRule* pRule = DNew(CConstraintRule) CConstraintRule(this);
	if (!pRule) {return(kErrMemory);}

	int16 rErr = pRule->Setup(_iPA,_iPB,_Rule,_Stiffness);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}
	rErr = pRule->SetRangeValue(_RangeValue);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}

	m_lpRules.Add(pRule);

	return(m_lpRules.Len()-1);
}

// Adds an angle constraint for the angle between _iP0->_iPA and _iP0->_iPB
// _Rule can be one of: kConstraintRuleSmaller,kConstraintRuleEqual,kConstraintRuleLarger,
// kConstraintRuleSmallerSoft,kConstraintRuleEqualSoft,kConstraintRuleLargerSoft.
// Returns the index of the rule or an errcode
int16 CConstraintSystem::AddAngleConstraint(int16 _iP0,int16 _iPA,int16 _iPB,int16 _Rule,fp32 _Stiffness,fp32 _Angle)
{
	if ((_iP0 < 0)||(_iP0 >= m_lpPts.Len())||
		(_iPA < 0)||(_iPA >= m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}
	if ((m_lpPts[_iP0] == NULL)||(m_lpPts[_iPA] == NULL)||(m_lpPts[_iPB] == NULL))
	{
		return(kErrNull);
	}

	CConstraintAngleRule* pRule = DNew(CConstraintAngleRule) CConstraintAngleRule(this);
	if (!pRule) {return(kErrMemory);}

	int16 rErr = pRule->Setup(_iP0,_iPA,_iPB,_Rule,_Stiffness,_Angle);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}

	m_lpRules.Add(pRule);

	return(m_lpRules.Len()-1);
}

// Adds a rule that will keep the knee correct, relies on SetupCoordinateSystem correctly set up
int16 CConstraintSystem::AddKneeConstraint(int16 _iP0,int16 _iP1,int16 _iPA,int16 _iPB,bool _bLeft,fp32 _Stiffness)
{
	if ((_iP0 < 0)||(_iP0 >= m_lpPts.Len())||
		(_iP1 < 0)||(_iP1 >= m_lpPts.Len())||
		(_iPA < 0)||(_iPA >= m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}
	if ((_iP0 == _iP1)||(_iP0 == _iPA)||(_iP0 == _iPB)||(_iP1 == _iPA)||(_iP1 == _iPB)||(_iPA == _iPB))
	{
		return(kErrParams);
	}
	if ((m_lpPts[_iP0] == NULL)||(m_lpPts[_iP1] == NULL)||(m_lpPts[_iPA] == NULL)||(m_lpPts[_iPB] == NULL))
	{
		return(kErrNull);
	}

	CConstraintKneeRule* pRule = DNew(CConstraintKneeRule) CConstraintKneeRule(this);
	if (!pRule) {return(kErrMemory);}

	int16 rErr = pRule->Setup(_iP0,_iP1,_iPA,_iPB,_bLeft,_Stiffness);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}

	m_lpRules.Add(pRule);

	return(m_lpRules.Len()-1);
}

// Adds a rule that will keep the elbow correct, relies on SetupCoordinateSystem correctly set up
int16 CConstraintSystem::AddElbowConstraint(int16 _iP0,int16 _iP1,int16 _iPA,int16 _iPB,bool _bLeft,fp32 _Stiffness)
{
	int16 rErr = kSuccess;
	if (_bLeft == true)
	{
		rErr = AddKneeConstraint(_iP0,_iP1,_iPA,_iPB,false,_Stiffness);
	}
	else
	{
		rErr = AddKneeConstraint(_iP0,_iP1,_iPA,_iPB,true,_Stiffness);
	}
	return(rErr);
}

// Adds a rule that will keep the knee correct, relies on SetupCoordinateSystem correctly set up
int16 CConstraintSystem::AddBodyPlaneConstraint(int16 _iP0,int16 _iP1,int16 _iPA,int16 _iPB,bool _bLeft,fp32 _Distance,fp32 _Stiffness)
{
	if ((_iP0 < 0)||(_iP0 >= m_lpPts.Len())||
		(_iP1 < 0)||(_iP1 >= m_lpPts.Len())||
		(_iPA < 0)||(_iPA >= m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}
	if ((_iP0 == _iP1)||(_iPA == _iPB))
	{
		return(kErrParams);
	}
	if ((m_lpPts[_iP0] == NULL)||(m_lpPts[_iP1] == NULL)||(m_lpPts[_iPA] == NULL)||(m_lpPts[_iPB] == NULL))
	{
		return(kErrNull);
	}

	CConstraintBodyPlaneRule* pRule = DNew(CConstraintBodyPlaneRule) CConstraintBodyPlaneRule(this);
	if (!pRule) {return(kErrMemory);}

	int16 rErr = pRule->Setup(_iP0,_iP1,_iPA,_iPB,_bLeft,_Distance,_Stiffness);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}

	m_lpRules.Add(pRule);

	return(m_lpRules.Len()-1);
}

int16 CConstraintSystem::AddPlaneConstraint(int16 _iPt0,int16 _iPt1,int16 _iPt2,int16 _iPt3,int16 _i0,int16 _i1,int16 _i2,int16 _i3,fp32 _Distance,fp32 _Stiffness)
{
	if ((_iPt0 < 0)||(_iPt0 >= m_lpPts.Len())||
		(_iPt1 < 0)||(_iPt1 >= m_lpPts.Len())||
		(_iPt2 < 0)||(_iPt2 >= m_lpPts.Len())||
		(_iPt3 < 0)||(_iPt3 >= m_lpPts.Len())||
		(_i0 < 0)||(_i0 >= m_lpPts.Len())||
		(_i1 < 0)||(_i1 >= m_lpPts.Len())||
		(_i2 < 0)||(_i2 >= m_lpPts.Len())||
		(_i3 < 0)||(_i3 >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}

	CConstraintPlaneRule* pRule = DNew(CConstraintPlaneRule) CConstraintPlaneRule(this);
	if (!pRule) {return(kErrMemory);}

	int16 rErr = pRule->Setup(_iPt0,_iPt1,_iPt2,_iPt3,_i0,_i1,_i2,_i3,_Distance,_Stiffness);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}

	m_lpRules.Add(pRule);

	return(m_lpRules.Len()-1);
};


int16 CConstraintSystem::AddCapsuleConstraint(int16 _iPA,int16 _iPB,int16 _iPt0,int16 _iPt1,fp32 _Radius,fp32 _Stiffness)
{
	// NOTE: We do NOT check the _iPt1 index for < 0 as the caller can supply -1 if he/she only
	// wants one point to be constrained.
	if ((_iPA < 0)||(_iPA >= m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_lpPts.Len())||
		(_iPt0 < 0)||(_iPt0 >= m_lpPts.Len())||
		(_iPt1 >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}

	CConstraintCapsuleRule* pRule = DNew(CConstraintCapsuleRule) CConstraintCapsuleRule(this);
	if (!pRule) {return(kErrMemory);}

	int16 rErr = pRule->Setup(_iPA,_iPB,_iPt0,_iPt1,_Radius,_Stiffness);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}

	m_lpRules.Add(pRule);

	return(m_lpRules.Len()-1);
};

// Similar to AddCapsuleConstraint above but uses _iRadiusPt to determine radius
int16 CConstraintSystem::AddCapsuleConstraintByPoint(int16 _iPA,int16 _iPB,int16 _iPt0,int16 _iPt1,int16 _iRadiusPt,fp32 _Stiffness)
{
	// NOTE: We do NOT check the _iPt1 index for < 0 as the caller can supply -1 if he/she only
	// wants one point to be constrained.
	if ((_iPA < 0)||(_iPA >= m_lpPts.Len())||(m_lpPts[_iPA] == NULL)||
		(_iPB < 0)||(_iPB >= m_lpPts.Len())||(m_lpPts[_iPB] == NULL)||
		(_iRadiusPt < 0)||(_iRadiusPt >= m_lpPts.Len())||(m_lpPts[_iRadiusPt] == NULL)||
		(_iPt0 < 0)||(_iPt0 >= m_lpPts.Len())||
		(_iPt1 >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}

	CConstraintCapsuleRule* pRule = DNew(CConstraintCapsuleRule) CConstraintCapsuleRule(this);
	if (!pRule) {return(kErrMemory);}

	fp32 RadiusSquare = SqrDistanceToSegment(m_lpPts[_iRadiusPt]->m_Pos,m_lpPts[_iPA]->m_Pos,m_lpPts[_iPB]->m_Pos);
	int16 rErr = pRule->Setup(_iPA,_iPB,_iPt0,_iPt1,M_Sqrt(RadiusSquare),_Stiffness);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}

	m_lpRules.Add(pRule);

	return(m_lpRules.Len()-1);
};

// Adds a rule that will keep int16 _iPA->_iPB pointing within _Angle of _Direction, relies on SetupCoordinateSystem correctly set up
// _Direction can be any of FORWARD,BACKWARD,RIGHT,LEFT,UP,DOWN and _Angle should be 0 - 90 degrees
int16 CConstraintSystem::AddDirAngleConstraint(int16 _iPA,int16 _iPB,fp32 _Stiffness,int16 _Direction,fp32 _Angle)
{
	if ((_iPA == _iPB)||
		(_iPA < 0)||(_iPA >= m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}
	if ((m_lpPts[_iPA] == NULL)||(m_lpPts[_iPB] == NULL))
	{
		return(kErrNull);
	}

	CConstraintDirAngleRule* pRule = DNew(CConstraintDirAngleRule) CConstraintDirAngleRule(this);
	if (!pRule) {return(kErrMemory);}

	int16 rErr = pRule->Setup(_iPA,_iPB,_Stiffness,_Direction,_Angle);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}

	m_lpRules.Add(pRule);

	return(m_lpRules.Len()-1);
}

int16 CConstraintSystem::AddDirAngleConstraint2(int16 _iPA,int16 _iPB,fp32 _Stiffness,int16 _iRef,int16 _iRef2,fp32 _Angle)
{
	if ((_iPA == _iPB)||
		(_iPA < 0)||(_iPA >= m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}
	if ((m_lpPts[_iPA] == NULL)||(m_lpPts[_iPB] == NULL))
	{
		return(kErrNull);
	}

	CConstraintDirAngleRule2* pRule = new CConstraintDirAngleRule2(this);
	if (!pRule) {return(kErrMemory);}

	int16 rErr = pRule->Setup(_iPA,_iPB,_Stiffness,_iRef,_iRef2,_Angle);
	if( rErr < kFailure )
	{
		delete pRule;
		return rErr;
	}

	m_lpRules.Add(pRule);

	return(m_lpRules.Len()-1);
}

int16 CConstraintSystem::ModifyPointMass(int16 _iP,fp32 _Mass)
{
	if ((_iP < 0)||(_iP >= m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}

	m_lpPts[_iP]->ModifyMass(_Mass);
	int32 N = m_lpRules.Len();
	for (int i = 0; i < N; i++)
	{
		m_lpRules[i]->ModifyMass();
	}

	return(kSuccess);
}

#define UNIFORM_MASS
#ifdef UNIFORM_MASS
// All bodyparts equal mass
	#define kHeadMass				1
	#define kNeckMass				1
	#define kChestMass				1
	#define kSpineMass				1
	#define kPelvisMass				1
	#define kCollarboneMass			1
	#define kShoulderMass			1
	#define kElbowMass				1
	#define kWristMass				1
	#define kHandMass				1
	#define kHipMass				1
	#define kLegMass				1
	#define kKneeMass				1
	#define kFootMass				1
#else
	#define kHeadMass				3
	#define kNeckMass				1
	#define kChestMass				?
	#define kCGMass					?
	#define kPelvisMass				?
	#define kCollarboneMass			?
	#define kShoulderMass			15
	#define kElbowMass				3
	#define kWristMass				2
	#define kHandMass				?
	#define kHipMass				15
	#define kKneeMass				10
	#define kFootMass				3
#endif

#define kV0							0
#define kV1							1
#define kV2							2
#define kV3							3
#define kV4							4
#define kV5							5
#define kV6							6
#define kV7							7

#define kRoot						0
#define kPelvis						1
#define kSpine						8
#define kTorso						9
#define kSpine2						15
#define kR_Leg						2
#define kR_Knee						3
#define kR_Foot						4
#define kL_Leg						5
#define kL_Knee						6
#define kL_Foot						7
#define kR_Shoulder					12
#define kR_Arm						13
#define kR_Elbow					14
#define kR_Hand						16
#define kL_Shoulder					17
#define kL_Arm						18
#define kL_Elbow					19
#define kL_Hand						21
#define kNeck						10
#define kHead						11
#define kAttachPoint				22
#define kAttachPoint2				23
#define kNose						24

#define kDwellerRoot						0
#define kDwellerPelvis						1
#define kDwellerSpine						2
#define kDwellerTorso						4
#define kDwellerR_Leg						5
#define kDwellerR_Knee						6
#define kDwellerR_Foot						7
#define kDwellerL_Leg						9
#define kDwellerL_Knee						10
#define kDwellerL_Foot						11
#define kDwellerR_Shoulder					13
#define kDwellerR_Arm						14
#define kDwellerR_Elbow						15
#define kDwellerR_Hand						17
#define kDwellerL_Shoulder					18
#define kDwellerL_Arm						19
#define kDwellerL_Elbow						20
#define kDwellerL_Hand						22
#define kDwellerNeck						23
#define kDwellerHead						24
#define kDwellerNose						25

// Sets up the default Starbreeze skeleton





int16 CConstraintSystem::SetupSkeleton(int _Type, CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CXR_Skeleton* _lpClothSkeleton[CWO_NUMMODELINDICES])
{
	GetDefaultPose(_pSkelInstance);

	switch(_Type)
	{
		case SConstraintSystemSettings::RAGDOLL_DEFAULT:	// Default Sbz skeleton
			{
				SetPointRadius(5.0f);

				AddPoint(kRoot,m_lDefaultPts[kRoot],0.25f,false,BODY_NONE);				// Root
				AddPoint(kPelvis,m_lDefaultPts[kPelvis],1.0f,true,BODY_PART);			// Pelvis
				AddPoint(kSpine,m_lDefaultPts[kSpine],1.0f,true,BODY_PART);			// Spine
				AddPoint(kTorso,m_lDefaultPts[kTorso],1.0f,false,BODY_PART);			// (Torso?)
				AddPoint(kL_Leg,m_lDefaultPts[kL_Leg],1.0f,true,BODY_PART);			// L_Leg (r_leg in jointconversion.xrg!)
				AddPoint(kL_Knee,m_lDefaultPts[kL_Knee],1.0f,true,L_LEG_PART);			// L_Knee
				AddPoint(kL_Foot,m_lDefaultPts[kL_Foot],1.0f,true,BODY_NONE);			// L_Foot
				AddPoint(kR_Leg,m_lDefaultPts[kR_Leg],1.0f,true,BODY_PART);			// R_Leg (l_leg in jointconversion.xrg!)
				AddPoint(kR_Knee,m_lDefaultPts[kR_Knee],1.0f,true,R_LEG_PART);			// R_Knee
				AddPoint(kR_Foot,m_lDefaultPts[kR_Foot],1.0f,true,BODY_NONE);			// R_Foot

				AddPoint(kL_Arm,m_lDefaultPts[kL_Arm],1.0f,true,BODY_PART);			// L_Arm
				AddPoint(kL_Elbow,m_lDefaultPts[kL_Elbow],1.0f,true,L_ARM_PART);		// L_Elbow
				AddPoint(kL_Hand,m_lDefaultPts[kL_Hand],1.0f,true,BODY_NONE);			// L_Hand
				AddPoint(kR_Arm,m_lDefaultPts[kR_Arm],1.0f,true,BODY_PART);			// R_Arm
				AddPoint(kR_Elbow,m_lDefaultPts[kR_Elbow],1.0f,true,R_ARM_PART);		// R_Elbow
				AddPoint(kR_Hand,m_lDefaultPts[kR_Hand],1.0f,true,BODY_NONE);			// R_Hand
				AddPoint(kNeck,m_lDefaultPts[kNeck],1.0f,true,BODY_PART);				// Neck
				AddPoint(kHead,m_lDefaultPts[kHead],1.0f,true,HEAD_PART);				// Head
				AddPoint(kNose,m_lDefaultPts[kHead]+CVec3Dfp32(3,0,0),1.0f,false,HEAD_PART);

				SetupCoordinateSystem(kPelvis,kNeck,kR_Arm,kL_Arm);
				
/*				AddBodypartSound(HEAD_PART,53);
				AddBodypartSound(BODY_PART,55);
				AddBodypartSound(L_ARM_PART,52);
				AddBodypartSound(R_ARM_PART,52);
				AddBodypartSound(L_LEG_PART,54);
				AddBodypartSound(R_LEG_PART,54);
				AddBodypartSound(BODY_NONE,60);	// Low impact*/
				int16 curBone;

				// Final head constraint
				AddBone(kNeck,kHead,true);

				// Pelvis
				AddConstraint(kL_Leg,kSpine,kConstraintRuleEqual);							// L_Leg to Spine IK
				AddConstraint(kR_Leg,kSpine,kConstraintRuleEqual);							// R_Leg to Spine IK
				AddConstraint(kL_Leg,kR_Leg,kConstraintRuleEqual);							// R_Leg to L_Leg IK
				curBone = AddBone(kSpine,kNeck);											// Spine to Neck IK
				AddChildBone(curBone,kPelvis);
				AddChildBone(curBone,kTorso);
				AddChildBone(curBone,kSpine2);
				AddChildBone(curBone,kR_Shoulder);
				AddChildBone(curBone,kL_Shoulder);
				AddBoneSubPos(kPelvis,kL_Leg,kR_Leg,0.5f);									// Pelvis bone halfway beteen legs

				// Cloth pelvis
				AddFixedClothChildBones(curBone, kNeck, kNeck, _lpClothSkeleton);
				AddFixedClothChildBones(curBone, kPelvis, kPelvis, _lpClothSkeleton);
				AddFixedClothChildBones(curBone, kTorso, kTorso, _lpClothSkeleton);
				AddFixedClothChildBones(curBone, kSpine2, kSpine2, _lpClothSkeleton);
				AddFixedClothChildBones(curBone, kR_Shoulder, kR_Shoulder, _lpClothSkeleton);
				AddFixedClothChildBones(curBone, kL_Shoulder, kL_Shoulder, _lpClothSkeleton);
				AddFixedClothChildBones(curBone, kL_Leg, kL_Leg, _lpClothSkeleton);
				AddFixedClothChildBones(curBone, kR_Leg, kR_Leg, _lpClothSkeleton);
				
				// Legs
				curBone = AddBone(kL_Leg,kL_Knee);
				AddFixedClothChildBones(curBone, kL_Knee, kL_Knee, _lpClothSkeleton);
				curBone = AddBone(kL_Knee,kL_Foot);
				AddChildBone(curBone,69);
				AddFixedClothChildBones(curBone, kL_Foot, kL_Foot, _lpClothSkeleton);
				AddFixedClothChildBones(curBone, 69, 69, _lpClothSkeleton);
				curBone = AddBone(kR_Leg,kR_Knee);											// L_Leg to L_Knee IK
				AddFixedClothChildBones(curBone, kR_Knee, kR_Knee, _lpClothSkeleton);
				curBone = AddBone(kR_Knee,kR_Foot);											// L_Knee to L_Foot IK
				AddChildBone(curBone,68);
				AddFixedClothChildBones(curBone, kR_Foot, kR_Foot, _lpClothSkeleton);
				AddFixedClothChildBones(curBone, 68, 68, _lpClothSkeleton);

				// Torso
				AddConstraint(kL_Leg,kL_Arm,kConstraintRuleEqual);							// L_Leg to L_Arm IK
				AddConstraint(kL_Leg,kR_Arm,kConstraintRuleEqual);							// Stiffen
				AddConstraint(kL_Arm,kNeck,kConstraintRuleEqual);							// L_Arm to Neck IK
				AddConstraint(kR_Leg,kR_Arm,kConstraintRuleEqual);							// R_Leg to R_Arm IK
				AddConstraint(kR_Leg,kL_Arm,kConstraintRuleEqual);							// Stiffen
				AddConstraint(kR_Arm,kNeck,kConstraintRuleEqual);							// L_Arm to Neck IK

				AddConstraint(kL_Leg,kNeck,kConstraintRuleEqual);							// L_Leg to Neck IK
				AddConstraint(kR_Leg,kNeck,kConstraintRuleEqual);							// R_Leg to Neck IK
				AddConstraint(kL_Arm,kR_Arm,kConstraintRuleEqual);							// L_Arm to R_Arm IK

				// Head
				curBone = AddBone(kNeck,kHead);
				AddChildBone(curBone,20);
				AddChildBone(curBone,24);
				for (int i = 25; i <= 37; i++)
					AddChildBone(curBone, i);
				for (int i = 70; i <= 92; i++)
					AddChildBone(curBone, i);
				AddConstraint(kHead,kNose,kConstraintRuleEqual);
				AddConstraint(kNeck,kNose,kConstraintRuleEqual);

				AddDirAngleConstraint(kNeck,kHead,kConstraintBoneStiffness,CConstraintDirAngleRule::UP,45);
				AddDirAngleConstraint(kHead,kNose,kConstraintBoneStiffness,CConstraintDirAngleRule::FORWARD,45);

				// Add Cloth head bones
				//AddFixedClothBones(kNeck, _pSkel);
				AddFixedClothChildBones(curBone, kHead, kHead, _lpClothSkeleton);
				AddFixedClothChildBones(curBone, 20, 20, _lpClothSkeleton);
				AddFixedClothChildBones(curBone, 24, 37, _lpClothSkeleton);
				AddFixedClothChildBones(curBone, 70, 90, _lpClothSkeleton);


				// Arms
				curBone = AddBone(kL_Arm,kL_Elbow);
				AddFixedClothChildBones(curBone, kL_Elbow, kL_Elbow, _lpClothSkeleton);
                curBone = AddBone(kL_Elbow,kL_Hand);
				AddChildBone(curBone,kAttachPoint2);
				AddChildBone(curBone,53);
				AddChildBone(curBone,54);
				AddChildBone(curBone,55);
				AddChildBone(curBone,56);
				AddChildBone(curBone,57);
				AddChildBone(curBone,58);
				AddChildBone(curBone,59);
				AddChildBone(curBone,60);
				AddChildBone(curBone,61);
				AddChildBone(curBone,62);
				AddChildBone(curBone,63);
				AddChildBone(curBone,64);
				AddChildBone(curBone,65);
				AddChildBone(curBone,66);
				AddChildBone(curBone,67);
				AddFixedClothChildBones(curBone, kL_Hand, kL_Hand, _lpClothSkeleton);
				AddFixedClothChildBones(curBone, 53, 67, _lpClothSkeleton);
				curBone = AddBone(kR_Arm,kR_Elbow);
				AddFixedClothChildBones(curBone, kR_Elbow, kR_Elbow, _lpClothSkeleton);
				curBone = AddBone(kR_Elbow,kR_Hand);
				AddChildBone(curBone,kAttachPoint);
				AddChildBone(curBone,38);
				AddChildBone(curBone,39);
				AddChildBone(curBone,40);
				AddChildBone(curBone,41);
				AddChildBone(curBone,42);
				AddChildBone(curBone,43);
				AddChildBone(curBone,44);
				AddChildBone(curBone,45);
				AddChildBone(curBone,46);
				AddChildBone(curBone,47);
				AddChildBone(curBone,48);
				AddChildBone(curBone,49);
				AddChildBone(curBone,50);
				AddChildBone(curBone,51);
				AddChildBone(curBone,52);
				AddFixedClothChildBones(curBone, kR_Hand, kR_Hand, _lpClothSkeleton);
				AddFixedClothChildBones(curBone, 38, 52, _lpClothSkeleton);

				// Keep left knees at left part of body and vice versa
				AddBodyPlaneConstraint(kL_Leg,kR_Leg,kR_Knee,kR_Foot,false,2.5f,0.1f);
				AddBodyPlaneConstraint(kR_Leg,kL_Leg,kL_Knee,kL_Foot,false,2.5f,0.1f);

				// Keep elbows, hands away from body
				AddConstraintRangeMultiplier(kL_Elbow,kR_Arm,kConstraintRuleLargerSoft,0.1f,0.9f);
				AddConstraintRangeMultiplier(kR_Elbow,kL_Arm,kConstraintRuleLargerSoft,0.1f,0.9f);
				AddConstraintRangeMultiplier(kL_Hand,kR_Arm,kConstraintRuleLargerSoft,0.1f,0.9f);
				AddConstraintRangeMultiplier(kR_Hand,kL_Arm,kConstraintRuleLargerSoft,0.1f,0.9f);

				// Keep knees, feet away from body (NOTE Sensitive values here!)
				AddConstraintRangeMultiplier(kNeck,kL_Knee,kConstraintRuleLargerSoft,0.1f,0.8f);
				AddConstraintRangeMultiplier(kNeck,kR_Knee,kConstraintRuleLargerSoft,0.1f,0.8f);
				AddConstraintRangeMultiplier(kNeck,kL_Foot,kConstraintRuleLargerSoft,0.1f,0.75f);
				AddConstraintRangeMultiplier(kNeck,kR_Foot,kConstraintRuleLargerSoft,0.1f,0.75f);

				// Keep legs and knees together somewhat
				AddConstraintRangeMultiplier(kL_Knee,kR_Knee,kConstraintRuleSmallerSoft,0.25f,3.0f);
				AddConstraintRangeMultiplier(kL_Foot,kR_Foot,kConstraintRuleSmallerSoft,0.25f,5.0f);
				
				// Keep knees from bending too much
				AddAngleConstraint(kL_Leg,kL_Knee,kL_Foot,kConstraintRuleLargerSoft,0.5f,30.0f);
				AddAngleConstraint(kR_Leg,kR_Knee,kR_Foot,kConstraintRuleLargerSoft,0.5f,30.0f);

				// Knee and elbow joints
				AddKneeConstraint(kR_Leg,kL_Leg,kL_Knee,kL_Foot,true,kConstraintBoneStiffness);
				AddKneeConstraint(kL_Leg,kR_Leg,kR_Knee,kR_Foot,false,kConstraintBoneStiffness);

				AddElbowConstraint(kR_Arm,kL_Arm,kL_Elbow,kL_Hand,true,kConstraintBoneStiffness);
				AddElbowConstraint(kL_Arm,kR_Arm,kR_Elbow,kR_Hand,false,kConstraintBoneStiffness);


				// Dummy yummy
				AddConstraint(kRoot,kPelvis,kConstraintRuleSmallerSoft);
			}
			break;

		case SConstraintSystemSettings::RAGDOLL_NEW:
			{
				SetPointRadius(5.0f);

				AddPoint(kRoot,m_lDefaultPts[kRoot],1.0f,false,BODY_NONE);				// Root
				AddPoint(kPelvis,m_lDefaultPts[kPelvis],1.0f,true,BODY_PART);			// Pelvis
				AddPoint(kSpine,m_lDefaultPts[kSpine],1.0f,true,BODY_PART);			// Spine
				AddPoint(kTorso,m_lDefaultPts[kTorso],1.0f,false,BODY_PART);			// (Torso?)
				AddPoint(kL_Leg,m_lDefaultPts[kL_Leg],1.0f,true,BODY_PART);			// L_Leg (r_leg in jointconversion.xrg!)
				AddPoint(kL_Knee,m_lDefaultPts[kL_Knee],1.0f,true,L_LEG_PART);			// L_Knee
				AddPoint(kL_Foot,m_lDefaultPts[kL_Foot],1.0f,true,BODY_NONE);			// L_Foot
				AddPoint(kR_Leg,m_lDefaultPts[kR_Leg],1.0f,true,BODY_PART);			// R_Leg (l_leg in jointconversion.xrg!)
				AddPoint(kR_Knee,m_lDefaultPts[kR_Knee],1.0f,true,R_LEG_PART);			// R_Knee
				AddPoint(kR_Foot,m_lDefaultPts[kR_Foot],1.0f,true,BODY_NONE);			// R_Foot

				AddPoint(kL_Arm,m_lDefaultPts[kL_Arm],1.0f,true,BODY_PART);			// L_Arm
				AddPoint(kL_Elbow,m_lDefaultPts[kL_Elbow],1.0f,true,L_ARM_PART);		// L_Elbow
				AddPoint(kL_Hand,m_lDefaultPts[kL_Hand],1.0f,true,BODY_NONE);			// L_Hand
				AddPoint(kR_Arm,m_lDefaultPts[kR_Arm],1.0f,true,BODY_PART);			// R_Arm
				AddPoint(kR_Elbow,m_lDefaultPts[kR_Elbow],1.0f,true,R_ARM_PART);		// R_Elbow
				AddPoint(kR_Hand,m_lDefaultPts[kR_Hand],1.0f,true,BODY_NONE);			// R_Hand
				AddPoint(kNeck,m_lDefaultPts[kNeck],1.0f,true,BODY_PART);				// Neck
				AddPoint(kHead,m_lDefaultPts[kHead],1.0f,true,HEAD_PART);				// Head
				AddPoint(kNose,m_lDefaultPts[kHead]+CVec3Dfp32(3,0,0),1.0f,false,HEAD_PART);

				SetupCoordinateSystem(kPelvis,kNeck,kR_Arm,kL_Arm);

				AddBodypartSound(HEAD_PART,53);
				AddBodypartSound(BODY_PART,55);
				AddBodypartSound(L_ARM_PART,52);
				AddBodypartSound(R_ARM_PART,52);
				AddBodypartSound(L_LEG_PART,54);
				AddBodypartSound(R_LEG_PART,54);
				AddBodypartSound(BODY_NONE,60);	// Low impact
				int16 curBone;

				// Head goes here!
				// Head
				curBone = AddBone(kNeck,kHead);
				//AddChildBone(curBone,24);
				AddChildBone(curBone,33);
				AddChildBone(curBone,15);
				AddChildBone(curBone,27);
				AddChildBone(curBone,25);
				AddChildBone(curBone,32);
				AddChildBone(curBone,20);
				AddChildBone(curBone,28);
				AddChildBone(curBone,26);
				AddChildBone(curBone,36);
				AddChildBone(curBone,37);
				AddChildBone(curBone,29);
				AddChildBone(curBone,30);
				AddChildBone(curBone,31);
				AddChildBone(curBone,35);
				AddChildBone(curBone,34);
				AddConstraint(kHead,kNose,kConstraintRuleEqual);
				AddConstraint(kNeck,kNose,kConstraintRuleEqual);

				// AddDirAngleConstraint2(kNeck,kHead,kConstraintBoneStiffness,kL_Arm,kR_Arm,45);
				AddDirAngleConstraint(kNeck,kHead,kConstraintBoneStiffness,CConstraintDirAngleRule::UP,5);
				AddDirAngleConstraint(kHead,kNose,kConstraintBoneStiffness,CConstraintDirAngleRule::FORWARD,5);

				// Pelvis
				AddConstraint(kL_Leg,kSpine,kConstraintRuleEqual);							// L_Leg to Spine IK
				AddConstraint(kR_Leg,kSpine,kConstraintRuleEqual);							// R_Leg to Spine IK
				AddConstraint(kL_Leg,kR_Leg,kConstraintRuleEqual);							// R_Leg to L_Leg IK
				curBone = AddBone(kSpine,kNeck);											// Spine to Neck IK
				AddChildBone(curBone,kPelvis);
				AddChildBone(curBone,kTorso);
				AddChildBone(curBone,kR_Shoulder);
				AddChildBone(curBone,kL_Shoulder);
				AddBoneSubPos(kPelvis,kL_Leg,kR_Leg,0.5f);									// Pelvis bone halfway beteen legs

				// Dummy yummy
				AddConstraint(kRoot,kPelvis,kConstraintRuleEqual);

				// Legs
				AddBone(kL_Leg,kL_Knee);
				curBone = AddBone(kL_Knee,kL_Foot);
				AddChildBone(curBone,69);
				AddBone(kR_Leg,kR_Knee);													// L_Leg to L_Knee IK
				curBone = AddBone(kR_Knee,kR_Foot);											// L_Knee to L_Foot IK
				AddChildBone(curBone,68);

				// Torso
				AddConstraint(kL_Leg,kL_Arm,kConstraintRuleEqual);							// L_Leg to L_Arm IK
				AddConstraint(kL_Leg,kR_Arm,kConstraintRuleEqual);							// Stiffen
				AddConstraint(kL_Arm,kNeck,kConstraintRuleEqual);							// L_Arm to Neck IK
				AddConstraint(kR_Leg,kR_Arm,kConstraintRuleEqual);							// R_Leg to R_Arm IK
				AddConstraint(kR_Leg,kL_Arm,kConstraintRuleEqual);							// Stiffen
				AddConstraint(kR_Arm,kNeck,kConstraintRuleEqual);							// L_Arm to Neck IK

				AddConstraint(kL_Leg,kNeck,kConstraintRuleEqual);							// L_Leg to Neck IK
				AddConstraint(kR_Leg,kNeck,kConstraintRuleEqual);							// R_Leg to Neck IK
				AddConstraint(kL_Arm,kR_Arm,kConstraintRuleEqual);							// L_Arm to R_Arm IK

				// Arms
				AddBone(kL_Arm,kL_Elbow);
				curBone = AddBone(kL_Elbow,kL_Hand);
				AddChildBone(curBone,kAttachPoint2);
				AddChildBone(curBone,53);
				AddChildBone(curBone,54);
				AddChildBone(curBone,55);
				AddChildBone(curBone,56);
				AddChildBone(curBone,57);
				AddChildBone(curBone,58);
				AddChildBone(curBone,59);
				AddChildBone(curBone,60);
				AddChildBone(curBone,61);
				AddChildBone(curBone,62);
				AddChildBone(curBone,63);
				AddChildBone(curBone,64);
				AddChildBone(curBone,65);
				AddChildBone(curBone,66);
				AddChildBone(curBone,67);
				AddBone(kR_Arm,kR_Elbow);
				curBone = AddBone(kR_Elbow,kR_Hand);
				AddChildBone(curBone,kAttachPoint);
				AddChildBone(curBone,38);
				AddChildBone(curBone,39);
				AddChildBone(curBone,40);
				AddChildBone(curBone,41);
				AddChildBone(curBone,42);
				AddChildBone(curBone,43);
				AddChildBone(curBone,44);
				AddChildBone(curBone,45);
				AddChildBone(curBone,46);
				AddChildBone(curBone,47);
				AddChildBone(curBone,48);
				AddChildBone(curBone,49);
				AddChildBone(curBone,50);
				AddChildBone(curBone,51);
				AddChildBone(curBone,52);

				AddBodyPlaneConstraint(kL_Leg,kR_Leg,kR_Knee,kR_Foot,false,2.5f,0.5f);
				AddBodyPlaneConstraint(kR_Leg,kL_Leg,kL_Knee,kL_Foot,false,2.5f,0.5f);

				// Cloth bones
				for(uint i = 0; i < _pSkel->m_lCloth.Len(); i++)
				{
					int* pFixedJoints = _pSkel->m_lCloth[i].m_FixedJoints.GetBasePtr();
					int nFixedJoints = _pSkel->m_lCloth[i].m_FixedJoints.Len();

					for(uint j = 0; j < nFixedJoints; i++)
					{
						curBone = _pSkel->m_lNodes[pFixedJoints[j]].m_iNodeParent;
						AddChildBone(curBone, (int16)pFixedJoints[j]);
					}
				}

				// Keep elbows, hands away from body
				AddConstraintRangeMultiplier(kL_Elbow,kR_Arm,kConstraintRuleLargerSoft,0.25f,0.9f);
				AddConstraintRangeMultiplier(kR_Elbow,kL_Arm,kConstraintRuleLargerSoft,0.25f,0.9f);
				AddConstraintRangeMultiplier(kL_Hand,kR_Arm,kConstraintRuleLargerSoft,0.25f,0.9f);
				AddConstraintRangeMultiplier(kR_Hand,kL_Arm,kConstraintRuleLargerSoft,0.25f,0.9f);

				// Keep knees, feet away from body (NOTE Sensitive values here!)
				AddConstraintRangeMultiplier(kNeck,kL_Knee,kConstraintRuleLargerSoft,0.5f,0.8f);
				AddConstraintRangeMultiplier(kNeck,kR_Knee,kConstraintRuleLargerSoft,0.5f,0.8f);
				AddConstraintRangeMultiplier(kNeck,kL_Foot,kConstraintRuleLargerSoft,0.5f,0.75f);
				AddConstraintRangeMultiplier(kNeck,kR_Foot,kConstraintRuleLargerSoft,0.5f,0.75f);

				// Keep legs and knees toghether somewhat
				AddConstraintRangeMultiplier(kL_Knee,kR_Knee,kConstraintRuleSmallerSoft,0.25f,3.0f);
				AddConstraintRangeMultiplier(kL_Foot,kR_Foot,kConstraintRuleSmallerSoft,0.25f,5.0f);

				// Keep knees from bending too much
				AddAngleConstraint(kL_Leg,kL_Knee,kL_Foot,kConstraintRuleLargerSoft,0.5f,30.0f);
				AddAngleConstraint(kR_Leg,kR_Knee,kR_Foot,kConstraintRuleLargerSoft,0.5f,30.0f);

				// Knee and elbow joints
				AddKneeConstraint(kR_Leg,kL_Leg,kL_Knee,kL_Foot,true,kConstraintBoneStiffness);
				AddKneeConstraint(kL_Leg,kR_Leg,kR_Knee,kR_Foot,false,kConstraintBoneStiffness);

				AddElbowConstraint(kR_Arm,kL_Arm,kL_Elbow,kL_Hand,true,kConstraintBoneStiffness);
				AddElbowConstraint(kL_Arm,kR_Arm,kR_Elbow,kR_Hand,false,kConstraintBoneStiffness);
			}
			break;

		default:
			{
			}
			break;
	}

	PostSetupToLocal();
	Constrain();
	Apply(m_OrgMat);

	return(kSuccess);
}


// Returns one of:
//enum
//{
//	ORIENTATION_BACK = 0,	// The default case
//	ORIENTATION_BELLY = 1,
//	ORIENTATION_LEFT = 2,
//	ORIENTATION_RIGHT = 3,
//	ORIENTATION_BENTOVER = 4,
//	ORIENTATION_SITUP = 5,
//};
int16 CConstraintSystem::GetOrientation()
{
	if ((m_SystemType != CS_SKELETON)||(!m_bCoordSysAvailable))
	{
		return(ORIENTATION_BACK);
	}

	// Use m_SystemMat for the basic orientations
	const CVec3Dfp32 Up(0.0f,0.0f,1.0f);
	CVec3Dfp32 Belly = m_SystemMat.GetRow(0);
	CVec3Dfp32 Head = m_SystemMat.GetRow(1);
	CVec3Dfp32 Left = m_SystemMat.GetRow(2);
	CVec3Dfp32 Pos = m_SystemMat.GetRow(3);
	if (Up * Left >= 0.5f)
	{
		return(ORIENTATION_LEFT);
	}
	if (Up * Left <= -0.5f)
	{
		return(ORIENTATION_RIGHT);
	}
	// Bent?
	// If at least one knee is substantially above the bodyplane we are bent
	CVec3Dfp32 lKneePos = m_lpPts[kL_Knee]->GetPos();
	CVec3Dfp32 rKneePos = m_lpPts[kR_Knee]->GetPos();
	if (((Belly * ((lKneePos - Pos).Normalize())) >= 0.25f)||
		((Belly * ((rKneePos - Pos).Normalize())) >= 0.25f))
	{	// Bent!
		if (Up * Belly <= 0.0f)
		{	// Sitting
			return(ORIENTATION_BENTOVER);
		}
		if (Up * Head >= 0.25f)
		{
			return(ORIENTATION_SITUP);
		}
	}

	// Back or Belly?
	if (Up * Belly >= 0.0f)
	{	// Sitting
		return(ORIENTATION_BACK);
	}
	else
	{	// Bent over
		return(ORIENTATION_BELLY);
	}
};

// Draws the coordinate system (if any) at _iOrigo
// Fwd is drawn in red, right is drawn in green and up is drawn in blue
int16 CConstraintSystem::DrawCoordinateSystem(int16 _iOrigo0,int16 _iOrigo1,fp32 _Length,fp32 _Duration,const CVec3Dfp32& _Offset)
{
	int16 rErr = kSuccess;

#ifdef _DRAW_SYSTEM
	rErr = kFailure;
	CVec3Dfp32 Origo;
	if ((m_bCoordSysAvailable)&&(_iOrigo0 != -1)&&(_iOrigo0 >= 0)&&(_iOrigo0 < m_lpPts.Len())&&(m_lpPts[_iOrigo0] != NULL))
	{
		Origo = m_lpPts[_iOrigo0]->m_Pos + _Offset;
		
		m_pWPhysState->Debug_RenderWire(Origo,Origo + CVec3Dfp32::GetRow(m_SystemMat,0) * _Length,kColorRed,_Duration,false);
		m_pWPhysState->Debug_RenderWire(Origo,Origo + CVec3Dfp32::GetRow(m_SystemMat,1) * _Length,kColorGreen,_Duration,false);
		m_pWPhysState->Debug_RenderWire(Origo,Origo + CVec3Dfp32::GetRow(m_SystemMat,2) * _Length,kColorBlue,_Duration,false);
		
		CVec3Dfp32 velocityVector = m_SystemVelocity * _Length;
		CVec3Dfp32 accelerationVector = (m_SystemVelocity - m_PrevSystemVelocity) * _Length;
		m_pWPhysState->Debug_RenderWire(Origo,Origo + velocityVector * _Length,kColorBlue,_Duration,false);
		m_pWPhysState->Debug_RenderWire(Origo + velocityVector * _Length,Origo + (velocityVector-accelerationVector) * _Length,kColorRed,_Duration,false);
		rErr = kSuccess;

		// *** Draw spine a looong time
		// _Duration = 30.0f;
		// m_pWPhysState->Debug_RenderWire(m_lpPts[kPelvis]->m_PrevPos+_Offset,m_lpPts[kPelvis]->m_Pos+_Offset,kColorRed,_Duration,true);
		// ***
	}
#endif

	return(rErr);
}
int16 CConstraintSystem::Animate(int32 _GameTick,CMat4Dfp32& _WMat)
{
	MSCOPESHORT( CConstraintSystem::Animate );

	int32 rErr = kSuccess;
	if (!m_pClient)
	{
		return(kFailure);
	}
	if (_GameTick <= m_LastFrameTick)
	{
		return(kFailure);
	}

	switch(m_State)
	{
	case NOTREADY:
		{
			rErr = kFailure;
			m_LastFrameTick = _GameTick;
			if (m_IdleTicks < 0x7fff)
				m_IdleTicks++;
		}
		break;

	case GETFRAME:
		{
			GetAnimatedFrame(true);
			m_CollMinBox = CVec3Dfp32(_FP32_MAX);
			m_CollMaxBox = CVec3Dfp32(-_FP32_MAX);
			m_MoveMinBox = CVec3Dfp32(_FP32_MAX);
			m_MoveMaxBox = -CVec3Dfp32(_FP32_MAX);
			m_iCollSelection = -1;
			m_lCollPolys.Clear();
			m_lDynamicCollPolys.Clear();
			Simulate();
			m_LastFrameTick = _GameTick;
			SetState(READY);
			rErr = kSuccess;
			m_IdleTicks = 0;
		}
		break;

	case READY:
		{
			Simulate();
			ERR_CHECK(rErr);

			// We should increase the threshold after a certain time
			int NSeconds = (int)(m_AnimateCount * 0.05f);
			// Note! No threshold
			if (CheckStop(m_StopThreshold * NSeconds))
			{
				SetState(STOPPED);
#ifndef M_RTM
				int16 Orientation = GetOrientation();
				switch(Orientation)
				{
				case ORIENTATION_BACK:
					ConOut(CStr("Dead on his back"));
					break;
				case ORIENTATION_BELLY:
					ConOut(CStr("Dead on his belly"));
					break;
				case ORIENTATION_LEFT:
					ConOut(CStr("Dead on his left"));
					break;
				case ORIENTATION_RIGHT:
					ConOut(CStr("Dead on his right"));
					break;
				case ORIENTATION_BENTOVER:
					ConOut(CStr("Dead bent over"));
					break;
				case ORIENTATION_SITUP:
					ConOut(CStr("Dead sits"));
					break;
				}	
#endif
			}

			m_LastFrameTick = _GameTick;
			m_AnimateCount++;
			rErr = kSuccess;
			m_IdleTicks = 0;
		}
		break;

	case STOPPED:
		{
			_WMat = m_SystemMat;
	//		CVec3Dfp32 SysPos = m_pWPhysState->Object_GetPosition(m_iObject);
	//		CVec3Dfp32 DollPos = m_lpPts[kPelvis]->GetPos();

	//		m_lpPts[kPelvis]->GetPos().SetMatrixRow(_WMat,3);
	//		m_pWPhysState->Object_SetPosition(m_iObject,_WMat);
			m_LastFrameTick = _GameTick;
			if (m_iPendingBone > -1)
			{	// Restart ragdoll
				m_AnimateCount = 1;
				SetState(READY);
				m_IdleTicks = 0;
			}
			rErr = kSuccess;
			if (m_IdleTicks < 0x7fff)
				m_IdleTicks++;
		}
		break;
	}

	if (!IsStopped())
	{
		Draw(0.05f,CVec3Dfp32(0,0,16));
	}

	m_pSystemClient->m_State = m_State;

	return(rErr);
}

// Gravity, wind etc
int16 CConstraintSystem::CalcForces()
{
	MSCOPESHORT(CConstraintSystem::CalcForces);
	int32 rErr = kSuccess;

	// London bridge is falling down
	CVec3Dfp32 gravity = CVec3Dfp32(0,0,-1.0f) * (m_Gravity * m_pWPhysState->GetGameTickTime());
	// CVec3Dfp32 gravity = CVec3Dfp32(0,0,-0.01f);	// Moon base alpha

	int32 N = m_lpPts.Len();
	for (int i = 0; i < N; i++)
	{
		if (m_lpPts[i] != NULL)
		{
			m_lpPts[i]->ApplyVelocity(gravity);
		}
	}

	rErr = CheckCollisionPushObjects();
	ERR_CHECK(rErr);

	if ((m_State >= READY)&&(m_PendingImpulse != CVec3Dfp32(0.0f)))
	{
		if(m_lpPts.Len() > kNeck)
		{
			m_pWPhysState->Debug_RenderWire(m_lpPts[kNeck]->GetPos(),m_lpPts[kNeck]->GetPos()+m_PendingImpulse*10.0f,kColorRed,10.0,true);
			m_pWPhysState->Debug_RenderVertex(m_lpPts[kNeck]->GetPos(),kColorRed,10.0,true);
		}
		ApplyPendingImpulse();
	}

	return(rErr);
}

// Translate points
int16 CConstraintSystem::Move()
{
	MSCOPESHORT(CConstraintSystem::Move);
	m_MoveMinBox = CVec3Dfp32(_FP32_MAX);
	m_MoveMaxBox = -CVec3Dfp32(_FP32_MAX);
	m_PrevSystemVelocity = m_SystemVelocity;
	m_SystemVelocity = CVec3Dfp32(0.0f,0.0f,0.0f);
	m_SystemAvgPos = m_SystemVelocity;
	int32 N = m_lpPts.Len();
	int Count = 0;
	for (int32 i = 0; i < N; i++)
	{
		if (m_lpPts[i] != NULL)
		{	// Save the previous frame position
			m_lpPts[i]->m_PrevFramePos = m_lpPts[i]->m_Pos;

			m_lpPts[i]->Move();
			m_SystemVelocity += m_lpPts[i]->m_Pos - m_lpPts[i]->m_PrevFramePos;
			m_SystemAvgPos += m_lpPts[i]->m_Pos;
			
			if (m_lpPts[i]->m_bCollision)
			{
				m_MoveMinBox[0] = Min(m_MoveMinBox[0], Min(m_lpPts[i]->m_Pos[0], m_lpPts[i]->m_PrevFramePos[0]));
				m_MoveMinBox[1] = Min(m_MoveMinBox[1], Min(m_lpPts[i]->m_Pos[1], m_lpPts[i]->m_PrevFramePos[1]));
				m_MoveMinBox[2] = Min(m_MoveMinBox[2], Min(m_lpPts[i]->m_Pos[2], m_lpPts[i]->m_PrevFramePos[2]));
				m_MoveMaxBox[0] = Max(m_MoveMaxBox[0], Max(m_lpPts[i]->m_Pos[0], m_lpPts[i]->m_PrevFramePos[0]));
				m_MoveMaxBox[1] = Max(m_MoveMaxBox[1], Max(m_lpPts[i]->m_Pos[1], m_lpPts[i]->m_PrevFramePos[1]));
				m_MoveMaxBox[2] = Max(m_MoveMaxBox[2], Max(m_lpPts[i]->m_Pos[2], m_lpPts[i]->m_PrevFramePos[2]));
			}

			Count++;
		}
	}
	if (Count > 0)
	{	// We want the average velocity
		m_SystemVelocity = m_SystemVelocity / Count;
		if ((m_SystemType == CS_SKELETON)&&(m_lpPts[kPelvis]))
		{
			m_SystemAvgPos = m_lpPts[kPelvis]->m_Pos;
		}
		else
		{
			m_SystemAvgPos = m_SystemAvgPos / Count;
		}
	}

	return(kSuccess);
}

int CConstraintSystem::SortByNormalAndHeight(const void* _FirstElem,const void* _SecondElem)
{
	CVec3Dfp32 Up = CVec3Dfp32(0.0f,0.0f,1.0f);
	SCollPoly* pFirst = (SCollPoly*)_FirstElem;
	SCollPoly* pSecond = (SCollPoly*)_SecondElem;
	
	fp32 Dot1 = pFirst->m_N * Up;
	fp32 Dot2 = pSecond->m_N * Up;
	if (Dot1 == Dot2)
	{	// Both normals are equally up
		if (Dot1 >= 0.0f)
		{	// Both normals are upwards
			if (pFirst->m_Pt0[2] >= pSecond->m_Pt0[2])
			{
				return(-1);
			}
			else
			{
				return(1);
			}
		}
		else
		{	// Both normals are downwards
			if (pFirst->m_Pt0[2] < pSecond->m_Pt0[2])
			{
				return(-1);
			}
			else
			{
				return(1);
			}
		}
	}
	else if (Dot1 > Dot2)
	{
		return(-1);
	}
	else
	{
		return(1);
	}
}

int16 CConstraintSystem::GetCollBoxPos(CMat4Dfp32* _pPosMat)
{
	if (m_lpPts[kPelvis] != NULL)
	{
		if (m_bCoordSysAvailable)
		{
			*_pPosMat = m_SystemMat;
			(*_pPosMat).GetRow(2) = CVec3Dfp32(0.0f,0.0f,1.0f);
			(*_pPosMat).RecreateMatrix<2,1>();
		}
		m_lpPts[kPelvis]->GetPos().SetRow(*_pPosMat, 3);
		return(kSuccess);
	}
	else
	{
		return(kErrNull);
	}
};

// NOTE: We might want to optimise here by calculating once per frame
void CConstraintSystem::GetBBox(CVec3Dfp32& _Min,CVec3Dfp32& _Max)
{
	int N = m_lpPts.Len();
	CConstraintPoint** lpPts = m_lpPts.GetBasePtr();

	CVec3Dfp32 BBoxMin = CVec3Dfp32(_FP32_MAX);
	CVec3Dfp32 BBoxMax = CVec3Dfp32(-_FP32_MAX);
	for (int32 i = 0; i < N; i++)
	{
		const CConstraintPoint*pPt = lpPts[i];
		if ((pPt != NULL)&&(pPt->m_bCollision))
		{
			const CVec3Dfp32& Pos = pPt->m_Pos;

			// m_CollMinBox
			CVec3Dfp32 MinPos = Pos - CVec3Dfp32(m_PtRadius);
			BBoxMin.k[0] = Min(BBoxMin.k[0], MinPos.k[0]);
			BBoxMin.k[1] = Min(BBoxMin.k[1], MinPos.k[1]);
			BBoxMin.k[2] = Min(BBoxMin.k[2], MinPos.k[2]);

			// m_CollMaxBox
			CVec3Dfp32 MaxPos = Pos + CVec3Dfp32(m_PtRadius);
			BBoxMax.k[0] = Max(BBoxMax.k[0], MaxPos.k[0]);
			BBoxMax.k[1] = Max(BBoxMax.k[1], MaxPos.k[1]);
			BBoxMax.k[2] = Max(BBoxMax.k[2], MaxPos.k[2]);
		}
	}

	_Min = BBoxMin;
	_Max = BBoxMax;
};

void CConstraintSystem::GetCollBBox(CVec3Dfp32& _Min,CVec3Dfp32& _Max)
{
	/*
	if ((m_MoveMaxBox[0] >= m_MoveMinBox[0])&&
		((m_MoveMaxBox[1] >= m_MoveMinBox[1]))&&
		((m_MoveMaxBox[2] >= m_MoveMinBox[2])))
	{
		CVec3Dfp32 Fat(m_PtRadius + 16);	// Radius plus some fat
		_Min = m_MoveMinBox - Fat;
		_Max = m_MoveMaxBox + Fat;
		return;
	}
	*/

	int N = m_lpPts.Len();
	CConstraintPoint** lpPts = m_lpPts.GetBasePtr();

	CVec3Dfp32 BBoxMin = CVec3Dfp32(_FP32_MAX);
	CVec3Dfp32 BBoxMax = CVec3Dfp32(-_FP32_MAX);
	for (int32 i = 0; i < N; i++)
	{
		const CConstraintPoint*pPt = lpPts[i];
		if ((pPt != NULL)&&(pPt->m_bCollision))
		{
			const CVec3Dfp32& Pos = pPt->m_Pos;

			// m_CollMinBox
			CVec3Dfp32 MinPos = Pos - CVec3Dfp32(m_PtRadius);
			BBoxMin.k[0] = Min(BBoxMin.k[0], MinPos.k[0]);
			BBoxMin.k[1] = Min(BBoxMin.k[1], MinPos.k[1]);
			BBoxMin.k[2] = Min(BBoxMin.k[2], MinPos.k[2]);

			// m_CollMaxBox
			CVec3Dfp32 MaxPos = Pos + CVec3Dfp32(m_PtRadius);
			BBoxMax.k[0] = Max(BBoxMax.k[0], MaxPos.k[0]);
			BBoxMax.k[1] = Max(BBoxMax.k[1], MaxPos.k[1]);
			BBoxMax.k[2] = Max(BBoxMax.k[2], MaxPos.k[2]);

			const CVec3Dfp32& PrevPos = pPt->m_PrevFramePos;
			// m_CollMinBox
			MinPos = PrevPos - CVec3Dfp32(m_PtRadius);
			BBoxMin.k[0] = Min(BBoxMin.k[0], MinPos.k[0]);
			BBoxMin.k[1] = Min(BBoxMin.k[1], MinPos.k[1]);
			BBoxMin.k[2] = Min(BBoxMin.k[2], MinPos.k[2]);

			// m_CollMaxBox
			MaxPos = PrevPos + CVec3Dfp32(m_PtRadius);
			BBoxMax.k[0] = Max(BBoxMax.k[0], MaxPos.k[0]);
			BBoxMax.k[1] = Max(BBoxMax.k[1], MaxPos.k[1]);
			BBoxMax.k[2] = Max(BBoxMax.k[2], MaxPos.k[2]);
		}
	}

	_Min = BBoxMin;
	_Max = BBoxMax;
};

// Checks if any of the m_lpPts lie outside m_CollMinBox,m_CollMaxBox
// If at least one does lie outside it will recalculate the box and get new polys
// This is a CPU intensive method that will run at startup and hopefully rarely 
bool CConstraintSystem::UpdateCollisionBox()
{
	MSCOPESHORT( CConstraintSystem::UpdateCollisionBox );
	if (!m_pWPhysState)
	{
		m_bCollReady = false;
		return(false);
	}

	// Bounding box
	int32 N = m_lpPts.Len();
	if (N == 0)
	{
		return(false);
	}

//	CConstraintPoint** lpPts = m_lpPts.GetBasePtr();

	CVec3Dfp32 BBoxMin = CVec3Dfp32(_FP32_MAX);
	CVec3Dfp32 BBoxMax = CVec3Dfp32(-_FP32_MAX);

	GetCollBBox(BBoxMin,BBoxMax);

	// Update bbox
	// We should update the bbox as well using BBoxMin,BBoxMax
	// Dead
	CVec3Dfp32 BBoxDim = BBoxMax - BBoxMin;
	CVec3Dfp32 BBoxCenter;
	BBoxDim *= 0.5f;
	BBoxCenter = BBoxMin + BBoxDim;
	CMat4Dfp32 ObjPosMat;
	ObjPosMat.Unit();
	GetCollBoxPos(&ObjPosMat);
	
	CVec3Dfp32 BoxOfs = BBoxCenter - CVec3Dfp32::GetRow(ObjPosMat, 3);

	CWO_PhysicsState Phys;
	Phys.m_MediumFlags |= XW_MEDIUM_PLAYERSOLID | XW_MEDIUM_DYNAMICSSOLID;
	Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, BBoxDim, BoxOfs);
	Phys.m_nPrim = 1;
	uint32 ObjectFlags = 0;
	if (m_SystemType == CS_SKELETON)
	{
		ObjectFlags &= OBJECT_FLAGS_WORLDTELEPORT | OBJECT_FLAGS_PLAYER;
	}
	else
	{
		ObjectFlags &= OBJECT_FLAGS_WORLDTELEPORT | OBJECT_FLAGS_PLAYER;
	}

	// Make the character "pickupable" and sensitive to corpse trigger
	ObjectFlags |= OBJECT_FLAGS_PICKUP | OBJECT_FLAGS_TRIGGER;
	Phys.m_ObjectFlags = ObjectFlags;
	if ((m_pClient)&&(m_pClient->m_ClientFlags & PLAYER_CLIENTFLAGS_GHOST))
	{
		Phys.m_ObjectIntersectFlags = 0;
	}
	else
	{
		Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_PROJECTILE;
	}
	Phys.m_PhysFlags = OBJECT_PHYSFLAGS_SLIDEABLE | OBJECT_PHYSFLAGS_PHYSMOVEMENT | OBJECT_PHYSFLAGS_OFFSET | OBJECT_FLAGS_ANIMPHYS;

	if ((m_pClient)&&(m_SystemType != CS_GIBSYSTEM))
	{
		if (!m_pWPhysState->Object_SetPhysics_DoNotify(m_pClient->m_iObject, Phys, ObjPosMat))
		{
			ConOut(CStrF("§cf80WARNING: Unable to set contraint system collision box. %s", BBoxDim.GetString().Str()));
		}
	}

	bool bUpdateNeeded = false;	// Set to true when at least one point lies outide of m_CollMinBox,m_CollMaxBox
	if ((BBoxMin[0] < m_CollMinBox[0])||
		(BBoxMin[1] < m_CollMinBox[1])||
		(BBoxMin[2] < m_CollMinBox[2])||
		(BBoxMax[0] > m_CollMaxBox[0])||
		(BBoxMax[1] > m_CollMaxBox[1])||
		(BBoxMax[2] > m_CollMaxBox[2]))
	{
		m_CollMinBox = BBoxMin;
		m_CollMaxBox = BBoxMax;

		CVec3Dfp32 BoxDim = m_CollMaxBox - m_CollMinBox;
		fp32 MaxXY = 0.25f * Max(BoxDim[0],BoxDim[1]);
		m_CollMinBox[0] = m_CollMinBox[0] - MaxXY;
		m_CollMinBox[1] = m_CollMinBox[1] - MaxXY;
		m_CollMinBox[2] = m_CollMinBox[2] - MaxXY - 0.02f;	// +0.02f: This is just the kind of thing I DONT LIKE!
		m_CollMaxBox[0] = m_CollMaxBox[0] + MaxXY;
		m_CollMaxBox[1] = m_CollMaxBox[1] + MaxXY;
		m_CollMaxBox[2] = m_CollMaxBox[2] + 16;	// Add 0.5 meters above for the rare cases of upwards motion
		
		CMat4Dfp32 Dummy;		// Destination of movement (why a matrix?)
		CPotColSet PCS;

		PCS.SetBox( &m_CollMinBox, &m_CollMaxBox );
		// CWO_PhysicsState PhysState = m_pClient->GetPhysState();
		CWO_PhysicsState PhysState = Phys;

		PhysState.m_MediumFlags |= XW_MEDIUM_PLAYERSOLID | XW_MEDIUM_DYNAMICSSOLID;

//		PhysState.m_ObjectIntersectFlags |= OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
		PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_PLAYERPHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_ANIMPHYS;

//		int iWorld = m_pWPhysState->Object_GetWorldspawnIndex();
		TSelection<CSelection::LARGE_BUFFER> Selection;
		{
			m_pWPhysState->Selection_AddBoundBox(Selection, PhysState.m_ObjectIntersectFlags, m_CollMinBox, m_CollMaxBox);
//			m_pWPhysState->Selection_RemoveOnIndex(Selection, iWorld);
			m_pWPhysState->Selection_GetArray(&PCS,&Selection,PhysState,m_iObject,Dummy,Dummy);
		}

		/*
		// TODO: Use this (and take out the commented line above) to separate the world from entities
		// Get the world now and then using m_lCollPolysbut
		// Get entities every frame using m_lDynamicCollPolys
		TSelection<CSelection::LARGE_BUFFER> Selection;
		{
			m_pWPhysState->Selection_AddObject(Selection, iWorld);
			m_pWPhysState->Selection_GetArray(&PCSWorld,iSel,PhysState,0,Dummy,Dummy);
		}
		*/

		// ***
		if (PCS.m_nFaces == 0)
		{
			// ConOut(CStr("Ragdoll: No coll polygons from PCS"));
		}
		else
		{
			m_lCollPolys.Clear();
		}

		// Process the planes and faces
		for (int i = 0; i < PCS.m_nFaces; i++)
		{
			SCollPoly newPoly = SCollPoly(
									CVec3Dfp32(PCS.m_fPlaneEqs[i][0],PCS.m_fPlaneEqs[i][1],PCS.m_fPlaneEqs[i][2]),
									CVec3Dfp32(PCS.m_fFaces[i][0],PCS.m_fFaces[i][1],PCS.m_fFaces[i][2]),
									CVec3Dfp32(PCS.m_fFaces[i][3],PCS.m_fFaces[i][4],PCS.m_fFaces[i][5]),
									CVec3Dfp32(PCS.m_fFaces[i][6],PCS.m_fFaces[i][7],PCS.m_fFaces[i][8]) );
			m_lCollPolys.Add(newPoly);
		}
		SCollPoly* pPolys = m_lCollPolys.GetBasePtr();
		qsort(pPolys,m_lCollPolys.Len(),sizeof(SCollPoly),&SortByNormalAndHeight);
		PCS.Clear();

		// ***
#ifdef _DRAW_SYSTEM	
		DrawCollisionBox();
#endif
		// ***

		bUpdateNeeded = true;
	}

	m_bCollReady = true;
	return(bUpdateNeeded);
}

void CConstraintSystem::DrawCollisionBox()
{
#ifdef _DRAW_SYSTEM
	if (!m_pWPhysState)
	{
		return;
	}

	// ***
	// Draw the corners of physics bbox
	CVec3Dfp32 Box = m_CollMaxBox - m_CollMinBox;
	m_pWPhysState->Debug_RenderVertex(m_CollMinBox,0xff7f7f7f,2.0f);
	m_pWPhysState->Debug_RenderVertex(m_CollMinBox+CVec3Dfp32(Box[0],0,0),0xff7f7f7f,2.0f);
	m_pWPhysState->Debug_RenderVertex(m_CollMinBox+CVec3Dfp32(0,Box[1],0),0xff7f7f7f,2.0f);
	m_pWPhysState->Debug_RenderVertex(m_CollMinBox+CVec3Dfp32(0,0,Box[2]),0xff7f7f7f,2.0f);

	m_pWPhysState->Debug_RenderVertex(m_CollMaxBox,0xff7f7f7f,2.0f);
	m_pWPhysState->Debug_RenderVertex(m_CollMaxBox-CVec3Dfp32(Box[0],0,0),0xff7f7f7f,2.0f);
	m_pWPhysState->Debug_RenderVertex(m_CollMaxBox-CVec3Dfp32(0,Box[1],0),0xff7f7f7f,2.0f);
	m_pWPhysState->Debug_RenderVertex(m_CollMaxBox-CVec3Dfp32(0,0,Box[2]),0xff7f7f7f,2.0f);

	// Draw PCS data
	for (int i = 0; i < m_lCollPolys.Len(); i++)
	{
		CVec3Dfp32 Off = m_lCollPolys[i].m_N * 10;
		// Sides
		m_pWPhysState->Debug_RenderWire(m_lCollPolys[i].m_Pt0+Off,m_lCollPolys[i].m_Pt1+Off,0xff7f0000,5.0f);
		m_pWPhysState->Debug_RenderWire(m_lCollPolys[i].m_Pt1+Off,m_lCollPolys[i].m_Pt2+Off,0xff7f0000,5.0f);
		m_pWPhysState->Debug_RenderWire(m_lCollPolys[i].m_Pt2+Off,m_lCollPolys[i].m_Pt0+Off,0xff7f0000,5.0f);
		// Normal
		CVec3Dfp32 Center = (m_lCollPolys[i].m_Pt0 + m_lCollPolys[i].m_Pt1 + m_lCollPolys[i].m_Pt2) * 0.33333f;
		m_pWPhysState->Debug_RenderWire(Center+Off,Center+Off+m_lCollPolys[i].m_N*10,0xff007f00,1.0f);
	}
	// ***
#endif
}

int16 CConstraintSystem::CheckCollision()
{
	int16 rErr;
	
	// *** Quick hack to avoid ragdoll falling through world
	if ((m_bFirstCollCheck)&&(m_SystemType != CS_SKELETON))
	{
		rErr = CheckCollisionCenter();
	}
	else
	{
		rErr = CheckCollisionRegular();
	}

	m_bFirstCollCheck = false;
	UpdateCoordinateSystem();

	return(rErr);
}
// Collide Points		
int16 CConstraintSystem::CheckCollisionRegular()
{
	MSCOPESHORT(CConstraintSystem::CheckCollision);

	if (!m_pWPhysState)
	{
		return(kFailure);
	}

	UpdateCollisionBox();

	int32 NPts = m_lpPts.Len();
	int32 NPolys = m_lCollPolys.Len();

	// NOTE By looping all points vs each poly we decrease the likelihood that the ragdoll will blow apart when
	// started inside an object.
	fp32 rebound = m_ReboundFactor;

	m_CurCollCount = 0;
	int16 rValue = kFailure;
	for (int32 j = 0; j < NPolys; j++)
	{
		const CVec3Dfp32& n = m_lCollPolys[j].m_N;
		const CVec3Dfp32& p0 = m_lCollPolys[j].m_Pt0;
		const CVec3Dfp32& p1 = m_lCollPolys[j].m_Pt1;
		const CVec3Dfp32& p2 = m_lCollPolys[j].m_Pt2;
		const fp32 r = m_PtRadius;
		for (int32 i = 0; i < NPts; i++)
		{
			CConstraintPoint* pPt = m_lpPts[i];
			if ((pPt != NULL)&&(pPt->m_bCollision))
			{
				pPt->m_iCollPoly = -1;
				
				// Check pPt (point i) against poly j
				const CVec3Dfp32& p = pPt->m_Pos;
				const CVec3Dfp32& prevP = pPt->m_PrevFramePos;
				
				const CVec3Dfp32& v = p - prevP;
				if (v * n > 0.0f) {continue;}
				// Note that we allow some slop for prevDepth in that we check without pPt->m_Radius
				fp32 prevDepth = (prevP - p0) * n;
				fp32 depth = (p - p0) * n - r;
				if ((depth < 0.0f)&&(prevDepth >= 0.0f))
				{	
					// m_lpPts[i] penetrate the plane (n p), but is it inside the triangle?
					CVec3Dfp32 testpoint = p - n*r;
					if (TriPointTest(testpoint,p0,p1,p2))
					{
						if (pPt->m_iBodypart != BODY_NONE)
						{
							PlayBodypartSound(pPt->m_iBodypart,pPt->m_Pos,M_Fabs(depth));
						}
						pPt->m_iCollPoly = j;
						CVec3Dfp32 delta,bounce;
						// delta is the distance we must move m_lpPts[i] to get it out of the plane
						delta = -n * depth;
						bounce = delta * rebound;
						pPt->m_Pos += delta;
						pPt->m_PrevPos = pPt->m_Pos - bounce;
						// We are moving the pt out of the poly along its normal, due to floating point inaccuracy we sometimes
						// may happen to get outside of the polybeam and I guess the assert is only here to signal that this can occur (irregularly)
						// There may something far more fishy going on if this happens often as it coulw well be that the poly normal
						// Isn't really pointing straight out of the poly (Oh my god, smoothed normals!) 
						// M_ASSERT(TriPointTest(pPt->m_Pos,p0,p1,p2), "!");

#ifdef _DRAW_SYSTEM
						m_pWPhysState->Debug_RenderWire(p,p+delta,kColorBlue,1.0f,true);
#endif
						m_CurCollCount++;
						rValue = kSuccess;
					}
					else
					{	// ***
						if (TriPointTest(p,p0,p1,p2))
						{
							ConOut(CStr("BadMojo!"));
						}
						// ***
					}
				}
			}
		}

		// Experimental midpoint collision detect
		// When neither the current point nor its bone point collides with poly j
		// we check wether a point midway between pt and bone pt collides. If it does
		// we will move pt and bone pt an equal amount.
		for (int32 i = 0; i < NPts; i++)
		{
			CConstraintPoint* pPt = m_lpPts[i];
			if ((pPt != NULL)&&(pPt->m_bCollision)&&(pPt->m_iCollPoly != j)&&(pPt->m_iBonePt != -1))
			{
				CConstraintPoint* pPt2 = m_lpPts[pPt->m_iBonePt];
				if ((pPt2 == NULL)||(!pPt2->m_bCollision)||(pPt2->m_iCollPoly == j)) {continue;}

				// We use the current midpoint between pPt and pPt2
				const CVec3Dfp32& p = (pPt2->m_Pos + pPt->m_Pos) * 0.5f;
				const CVec3Dfp32& prevP = (pPt2->m_PrevFramePos + pPt->m_PrevFramePos) * 0.5f;
				const CVec3Dfp32& v = p - prevP;
				// We are still checking the j:th poly so we don't have to recalc n,p0,p1,p2
				if (v * n > 0.0f) {continue;}
				// Note that we allow some slop for prevDepth in that we check without pPt->m_Radius
				fp32 prevDepth = (prevP - p0) * n;
				fp32 depth = (p - p0) * n - r;
				if ((depth < 0.0f)&&(prevDepth >= 0.0f))
				{	
					// m_lpPts[i] penetrate the plane (n p), but is it inside the triangle?
					CVec3Dfp32 testpoint = p - n*r;
					if (TriPointTest(testpoint,p0,p1,p2))
					{
						if (pPt->m_iBodypart != BODY_NONE)
						{
							PlayBodypartSound(pPt->m_iBodypart,pPt->m_Pos,M_Fabs(depth));
						}
						if (pPt->m_iCollPoly == -1)
						{
							pPt->m_iCollPoly = j;
						}
						if (pPt2->m_iCollPoly == -1)
						{
							pPt2->m_iCollPoly = j;
						}
						CVec3Dfp32 delta,bounce;
						// delta is the distance we must move m_lpPts[i] to get it out of the plane
						delta = -n * depth;
						bounce = delta * rebound;
						pPt->m_Pos += delta;
						pPt2->m_Pos += delta;
						pPt->m_PrevPos = pPt->m_Pos - bounce;
						pPt2->m_PrevPos = pPt2->m_Pos - bounce;

						// BACKMANFLOATSCALARKRICK
						CVec3Dfp32 friction = (v - CVec3Dfp32(v * n));
						// pPt->m_Pos -= friction;

#ifdef _DRAW_SYSTEM
						m_pWPhysState->Debug_RenderWire(p,p+delta,kColorBlue,1.0f,true);
#endif
						m_CurCollCount++;
						rValue = kSuccess;
					}
					else
					{	// ***
						if (TriPointTest(p,p0,p1,p2))
						{
							ConOut(CStr("BadMojo!"));
						}
						// ***
					}
				}
			}
		}
	}

	return(rValue);
}

int16 CConstraintSystem::CheckCollisionCenter()
{
	MSCOPESHORT(CConstraintSystem::CheckCollision);

	if (!m_pWPhysState)
	{
		return(kFailure);
	}

	UpdateCollisionBox();

	int32 NPts = m_lpPts.Len();
	int32 NPolys = m_lCollPolys.Len();

	// NOTE By looping all points vs each poly we decrease the likelihood that the ragdoll will blow apart when
	// started inside an object.
	fp32 rebound = m_ReboundFactor;

	m_CurCollCount = 0;
	int16 rValue = kFailure;
	for (int32 j = 0; j < NPolys; j++)
	{
		const CVec3Dfp32& n = m_lCollPolys[j].m_N;
		const CVec3Dfp32& p0 = m_lCollPolys[j].m_Pt0;
		const CVec3Dfp32& p1 = m_lCollPolys[j].m_Pt1;
		const CVec3Dfp32& p2 = m_lCollPolys[j].m_Pt2;
		const fp32 r = m_PtRadius;
		for (int32 i = 0; i < NPts; i++)
		{
			CConstraintPoint* pPt = m_lpPts[i];
			if ((pPt != NULL)&&(pPt->m_bCollision))
			{
				pPt->m_iCollPoly = -1;

				// Check pPt (point i) against poly j
				const CVec3Dfp32& p = pPt->m_Pos;
				const CVec3Dfp32& v = p - m_SystemAvgPos;
				if (v * n > 0.0f) {continue;}
				// Note that we allow some slop for prevDepth in that we check without pPt->m_Radius
				fp32 depth = (p - p0) * n - r;
				if (depth < 0.0f)
				{	
					// m_lpPts[i] penetrate the plane (n p), but is it inside the triangle?
					CVec3Dfp32 testpoint = p - n*r;
					if (TriPointTest(testpoint,p0,p1,p2))
					{
						if (pPt->m_iBodypart != BODY_NONE)
						{
							PlayBodypartSound(pPt->m_iBodypart,pPt->m_Pos,M_Fabs(depth));
						}
						pPt->m_iCollPoly = j;
						CVec3Dfp32 delta,bounce;
						// delta is the distance we must move m_lpPts[i] to get it out of the plane
						delta = -n * depth;
						bounce = delta * rebound;
						pPt->m_Pos += delta;

						M_ASSERT(TriPointTest(pPt->m_Pos,p0,p1,p2), "!");

#ifdef _DRAW_SYSTEM
						m_pWPhysState->Debug_RenderWire(p,p+delta,kColorBlue,1.0f,true);
#endif
						m_CurCollCount++;
						rValue = kSuccess;
					}
					else
					{	// ***
						if (TriPointTest(p,p0,p1,p2))
						{
							ConOut(CStr("BadMojo!"));
						}
						// ***
					}
				}
			}
		}

		// Experimental midpoint collision detect
		// When neither the current point nor its bone point collides with poly j
		// we check wether a point midway between pt and bone pt collides. If it does
		// we will move pt and bone pt an equal amount.
		for (int32 i = 0; i < NPts; i++)
		{
			CConstraintPoint* pPt = m_lpPts[i];
			if ((pPt != NULL)&&(pPt->m_bCollision)&&(pPt->m_iCollPoly != j)&&(pPt->m_iBonePt != -1))
			{
				CConstraintPoint* pPt2 = m_lpPts[pPt->m_iBonePt];
				if ((pPt2 == NULL)||(!pPt2->m_bCollision)||(pPt2->m_iCollPoly == j)) {continue;}

				// We use the current midpoint between pPt and pPt2
				const CVec3Dfp32& p = (pPt2->m_Pos + pPt->m_Pos) * 0.5f;
				const CVec3Dfp32& v = p - m_SystemAvgPos;
				// We are still checking the j:th poly so we don't have to recalc n,p0,p1,p2
				if (v * n > 0.0f) {continue;}
				fp32 depth = (p - p0) * n - r;
				if (depth < 0.0f)
				{	
					// m_lpPts[i] penetrate the plane (n p), but is it inside the triangle?
					CVec3Dfp32 testpoint = p - n*r;
					if (TriPointTest(testpoint,p0,p1,p2))
					{
						if (pPt->m_iBodypart != BODY_NONE)
						{
							PlayBodypartSound(pPt->m_iBodypart,pPt->m_Pos,M_Fabs(depth));
						}
						if (pPt->m_iCollPoly == -1)
						{
							pPt->m_iCollPoly = j;
						}
						if (pPt2->m_iCollPoly == -1)
						{
							pPt2->m_iCollPoly = j;
						}
						CVec3Dfp32 delta,bounce;
						// delta is the distance we must move m_lpPts[i] to get it out of the plane
						delta = -n * depth;
						bounce = delta * rebound;
						pPt->m_Pos += delta;
						pPt2->m_Pos += delta;

#ifdef _DRAW_SYSTEM
						m_pWPhysState->Debug_RenderWire(p,p+delta,kColorBlue,1.0f,true);
#endif
						m_CurCollCount++;
						rValue = kSuccess;
					}
					else
					{	// ***
						if (TriPointTest(p,p0,p1,p2))
						{
							ConOut(CStr("BadMojo!"));
						}
						// ***
					}
				}
			}
		}
	}

	return(rValue);
}

int16 CConstraintSystem::CheckCollisionPushObjects()
{
	int16 rErr = kFailure;

	for (int i = 0; i < m_liPushObjects.Len(); i++)
	{
		CVec3Dfp32 PushVector(0.0f);
		fp32 BestPush = 0.0f;

		int iObj = m_liPushObjects[i];
		CWorld_Server* pWServer = safe_cast<CWorld_Server>(m_pWPhysState);
		CWObject* pObj = pWServer->Object_Get(iObj);
		if (pObj)
		{
			const CBox3Dfp32* pBBox = pObj->GetAbsBoundBox();
			for (int j = 0; j < m_lpPts.Len(); j++)
			{
				if (m_lpPts[j] == NULL) {continue;}
				CVec3Dfp32 Pos = m_lpPts[j]->m_Pos;
				if ((Pos[2] <= pBBox->m_Min[2])||(Pos[2] >= pBBox->m_Max[2])) {continue;}
				// NOTE: This sorry mess need some optimizing
				CVec3Dfp32 Center =  (pBBox->m_Max + pBBox->m_Min) * 0.5f;
				CVec3Dfp32 Dir = Pos - Center;
				Dir[2] = 0.0f;
				fp32 RadiusSqr = (Sqr(pBBox->m_Max[0]-Center[0]) + Sqr(pBBox->m_Max[1]-Center[1])) * 2.0f;
				fp32 DirSqr = Dir.LengthSqr();
				if (DirSqr >= RadiusSqr) {continue;}
				Dir.Normalize();
				fp32 Push = (M_Sqrt(RadiusSqr) - M_Sqrt(DirSqr)) + 8.0f;
				m_lpPts[j]->m_Pos += Dir * Push;
				if (Push > BestPush)
				{
					BestPush = Push;
					PushVector += Dir * Push;
				}
				m_CurCollCount++;
				m_AnimateCount = 1;
				rErr = kSuccess;
			}
		}
	}

	return(rErr);
};

// Iterate constraints	
int16 CConstraintSystem::Constrain()
{
	MSCOPESHORT(CConstraintSystem::Constrain);
	int NIterations = m_ConstraintIterations;
	if (m_iPendingBone > -1)
	{
		NIterations *= 2;
	}

	UpdateCoordinateSystem();
	for (int32 iterations = 0; iterations < NIterations; iterations++)
	{	
		int32 N = m_lpRules.Len();

		// The we go through all the rules
		for (int32 i = N-1; i >= 0; i--)
		{
			const CConstraintRule* pRule = m_lpRules[i];
			if (pRule != NULL)
			{
				if ((m_iPendingBone > -1)&&(pRule->m_bBone)&&((pRule->m_iPA == m_iPendingBone)||(pRule->m_iPB == m_iPendingBone)))
				{
					if (pRule->m_Rule & kConstraintRuleSoftMask)
					{
						continue;
					}
					/*
					if (pRule->m_iPA == m_iPendingBone)
					{
						CVec3Dfp32 Delta = (m_PendingBonePos - m_lpPts[pRule->m_iPA]->m_Pos) * 0.5f;
						m_lpPts[pRule->m_iPA]->SetPos(m_PendingBonePos);
						pRule->Constrain();
					}
					else
					{
						m_lpPts[pRule->m_iPB]->SetPos(m_PendingBonePos);
						pRule->Constrain();
					}
					*/
					pRule->Constrain();
				}
				else
				{
					pRule->Constrain();
				}
			}
		}
		UpdateCoordinateSystem();

		// We constrain the dragged point
		if ((m_iPendingBone > -1)&&(m_iPendingBone < N))
		{
			if (m_lpPts[m_iPendingBone] != NULL)
			{
				m_lpPts[m_iPendingBone]->SetPos(m_PendingBonePos);
			}
		}
	}

	if (!m_bPendingBonePosKeep)
	{
		m_iPendingBone = -1;
		m_PendingBonePos = CVec3Dfp32(_FP32_MAX);
	}

	return(kSuccess);
}

bool CConstraintSystem::CheckStop(fp32 _Diff)
{
	if ((m_SystemType == CS_SKELETON)&&((m_iPendingBone != -1)||(m_CurCollCount < 2)))
	{
		m_AnimateCount = 0;
		return(false);
	}

	int32 N = m_lpPts.Len();
	const fp32 ThresholdSqr = Sqr(_Diff);
	for (int32 i = 0; i < N; i++)
	{
		if ((m_lpPts[i] != NULL)&&(m_lpPts[i]->m_bCollision))
		{
			fp32 diffSqr = m_lpPts[i]->m_Pos.DistanceSqr(m_lpPts[i]->m_PrevFramePos);
			if (diffSqr > ThresholdSqr)
			{
				return(false);
			}
		}
	}

	m_liPushObjects.Clear();

	return(true);
}

// Apply the new positions to m_Root and its children
int16 CConstraintSystem::Apply(CMat4Dfp32& _WMat)
{
	MSCOPESHORT(CConstraintSystem::Apply);
	if (!m_pSystemClient || m_pSystemClient->m_nTracks <= 0)
	{
		return(kErrState);
	}

	CXR_Model* pModel = m_pWPhysState->GetMapData()->GetResource_Model(m_pClient->m_iModel[0]);
	if (!pModel)
	{
		return(kErrNull);
	}
	CXR_Skeleton* pSkel = pModel->GetPhysSkeleton();
//	CXR_SkeletonInstance* pSkelInstance = CWObject_Character::GetClientSkelInstance(m_pClient);
	if ((!pSkel)/*||(!pSkelInstance)*/)
	{
		return(kErrNull);
	}

	// Copy frame 1 to frame 0 and evaluate new values for frame 1
	CVec3Dfp32 VZero = CVec3Dfp32(0.0f,0.0f,0.0f);
	// TArray is ref-counted.. use that to remove memcopy
	{
		TArray<CQuatfp32> lRotTemp;
		lRotTemp = m_pSystemClient->m_lRot0;
		m_pSystemClient->m_lRot0 = m_pSystemClient->m_lRot1;
		m_pSystemClient->m_lRot1 = lRotTemp;

		TArray<CVec3Dfp32> lMoveTemp;
		lMoveTemp = m_pSystemClient->m_lMove0;
		m_pSystemClient->m_lMove0 = m_pSystemClient->m_lMove1;
		m_pSystemClient->m_lMove1 = lMoveTemp;
	}

	TAP_RCD<CQuatfp32> pRot0 = m_pSystemClient->m_lRot0;
	TAP_RCD<CQuatfp32> pRot1 = m_pSystemClient->m_lRot1;
	TAP_RCD<CVec3Dfp32> pMove0 = m_pSystemClient->m_lMove0;
	TAP_RCD<CVec3Dfp32> pMove1 = m_pSystemClient->m_lMove1;
	int nTracks = m_pSystemClient->m_nTracks;

	for (int i = 0; i < nTracks; i++)
	{
		pRot1[i].Unit();
		pMove1[i] = VZero;
	}


	// Return the org pos, we  don't want to be pushed
	_WMat = m_OrgMat;

	// An alternative and perhaps better way of adding the skeleton is by modifying the
	// m_pBoneTransform array as that is in modelspace.
	const CVec3Dfp32& BaseDefault = CVec3Dfp32::GetRow(m_lDefaultMats[kRoot],3);
	const CVec3Dfp32 PelvisCur = m_lpPts[kPelvis]->GetPos();

	CMat4Dfp32 MBodySystem,MBodySystemInv;

	MBodySystem = m_SystemMat;
	MBodySystem.UnitNot3x3();
	MBodySystemInv.Unit();
	MBodySystem.Inverse3x3(MBodySystemInv);

	pRot1[kRoot].Create(m_OrgMat);
	pMove1[kRoot] = CVec3Dfp32::GetRow(m_OrgMat,3);

	TAP_RCD<CConstraintRule*> ppRules = m_lpRules;
	for (int iRule = 0; iRule < ppRules.Len(); iRule++)
	{
		CConstraintRule* pRule = ppRules[iRule];
		if ((pRule == NULL) || (!pRule->m_bBone)) 
			continue;

		int iStart = pRule->m_iPA;
		int iEnd = pRule->m_iPB;

		// Current bone rotated back into the default pose system
		CVec3Dfp32 BoneStart1 = ((m_lpPts[iStart]->GetPos() - PelvisCur)*MBodySystemInv) + PelvisCur;
		CVec3Dfp32 BoneEnd1 = ((m_lpPts[iEnd]->GetPos() - PelvisCur)*MBodySystemInv) + PelvisCur;

		// Default bone rotated back into the default pose system (not really ;))
		const CVec3Dfp32& BoneStart0 = m_lDefaultPts[iStart];
		const CVec3Dfp32& BoneEnd0 = m_lDefaultPts[iEnd];

		CMat4Dfp32 matRotation,matTemp,matResult;
		matRotation.Unit();
		const CVec3Dfp32 v0 = BoneEnd0 - BoneStart0;
		const CVec3Dfp32 v1 = BoneEnd1 - BoneStart1;
		BoneStart1 = m_lpPts[iStart]->GetPos();
		matRotation = RotationArcMatrix2(v0,v1);
		matTemp = matRotation;
		matTemp.Multiply(MBodySystem,matRotation);
		CMat4Dfp32 matInvTranslation;
		matInvTranslation.Unit();
		(BaseDefault - BoneStart0).SetMatrixRow(matInvTranslation,3);
		CMat4Dfp32 matTranslation;
		matTranslation.Unit();
		(BoneStart1 - BaseDefault).SetMatrixRow(matTranslation,3);
		matResult.Unit();
		// We change the origin of matRotation to 
		BaseDefault.SetRow(matRotation,3);
		matInvTranslation.Multiply(matRotation, matTemp);
		matTemp.Multiply(matTranslation,matResult);

		CQuatfp32 qResult;
		qResult.Create(matResult);

		pRot1[iStart] = qResult;
		pMove1[iStart] = matResult.GetRow(3);
		pRot1[iEnd] = qResult;
		pMove1[iEnd] = matResult.GetRow(3);

		TAP_RCD<int16> piChildBones = pRule->m_liChildBones;
		for (int iChild = 0; iChild < piChildBones.Len(); iChild++)
		{
			int iBone = piChildBones[iChild];
			M_ASSERT(iBone < nTracks, "invalid childbone. should be handled by GrowTracks!");
			pRot1[iBone] = qResult;
			pMove1[iBone] = matResult.GetRow(3);
		}
	}

	if (m_bFirstApply)
	{
		for (int i = 0; i < nTracks; i++)
		{
			pRot0[i] = pRot1[i];
			pMove0[i] = pMove1[i]; 
		}
		m_bFirstApply = false;
	}

	return(kSuccess);
}

int16 CConstraintSystem::Simulate()
{
	MSCOPESHORT(CConstraintSystem::Simulate);
	int rErr = kSuccess;

	rErr = CalcForces();
	ERR_CHECK(rErr);

	rErr = Move();
	ERR_CHECK(rErr);

	rErr = Constrain();
	ERR_CHECK(rErr);

	rErr = CheckCollision();
	ERR_CHECK(rErr);
	UpdateCoordinateSystem();

	rErr = Apply(m_OrgMat);
	ERR_CHECK(rErr);

	return(rErr);
}

void CConstraintSystem::DebugRenderSkeletonJoints(CXR_Skeleton* _pSkel,
												  CXR_SkeletonInstance* _pSkelInstance,
												  const CMat4Dfp32 &_Mat)
{
	// KEEP
	if (!m_pWPhysState)
	{
		return;
	}

	TArray<CMat4Dfp32> lMat;
	lMat.SetLen(_pSkel->m_lNodes.Len());
	CMat4Dfp32 Mat = _Mat;
	Mat.Unit();
	CalcMatrices_r(true,0,Mat,_pSkelInstance,_pSkel,lMat);
	int i;

	for (i = 0; i < lMat.Len(); i++)
	{	// Compare the positions of the ragdoll with those from CalcMatrices_r
		CXR_SkeletonNode* pN = &_pSkel->m_lNodes[i];
		int iRot = pN->m_iRotationSlot;
		if ((iRot < m_lpPts.Len())&&(m_lpPts[iRot] != NULL))
		{
			CVec3Dfp32 SystemPos = m_lpPts[iRot]->GetPos();
			const CVec3Dfp32& BonePos = CVec3Dfp32::GetRow(lMat[i],3);
			CVec3Dfp32 Diff = SystemPos - BonePos;
			fp32 DiffLength = Diff.Length();
//			CVec3Dfp32::GetRow(lMat[i],3);
			// Apply the Diff back to
			if (DiffLength > 1)
			{
				m_pWPhysState->Debug_RenderVertex(BonePos+CVec3Dfp32(0,0,50),kColorDkGreen,0.05f);
				m_pWPhysState->Debug_RenderWire(SystemPos+CVec3Dfp32(0,0,50),BonePos+CVec3Dfp32(0,0,50),kColorGreen,0.05f,true);
			}
		}
	}
};

// Draw m_Root and children
int16 CConstraintSystem::Draw(fp32 _Duration,CVec3Dfp32 _Offset)
{
#ifdef _DRAW_SYSTEM
	if (!m_pWPhysState)
	{
		return(kFailure);
	}

	int32 N = m_lpRules.Len();
	for (int32 i = 0; i < N; i++)
	{
		if (m_lpRules[i] == NULL) {continue;}
		m_lpRules[i]->Draw(_Duration,_Offset);
	}
	DrawCoordinateSystem(kPelvis,kSpine,5,_Duration,_Offset);

	// Draw white vectors from prev frame to current
	for (int i = 0; i < m_lpPts.Len(); i++)
	{
		if (m_lpPts[i] == NULL) {continue;}
		if (m_lpPts[i]->m_bCollision)
		{
			CVec3Dfp32 Pos = m_lpPts[i]->m_Pos;
			CVec3Dfp32 PrevPos = m_lpPts[i]->m_PrevFramePos;
			m_pWPhysState->Debug_RenderVertex(Pos+_Offset,0xffffff,_Duration,true);
			if (Pos.DistanceSqr(PrevPos) > Sqr(2.0f))
			{
				m_pWPhysState->Debug_RenderWire(PrevPos+_Offset,Pos+_Offset,0xffffff,_Duration,true);
			}
		}
	}
#endif
	return(kSuccess);
}

// Draw scenepoints
int16 CConstraintSystem::DrawPoints(fp32 _Duration,uint32 _Color)
{
#ifdef _DRAW_SYSTEM
	CVec3Dfp32 Offset = CVec3Dfp32(0,0,0);

	if (!m_pWPhysState)
	{
		return(kFailure);
	}
	// Draw white vectors from prev frame to current
	for (int i = 0; i < m_lpPts.Len(); i++)
	{
		if (m_lpPts[i] == NULL) {continue;}
		if (m_lpPts[i]->m_bCollision)
		{
			CVec3Dfp32 Pos = m_lpPts[i]->m_Pos;
			CVec3Dfp32 PrevPos = m_lpPts[i]->m_PrevFramePos;
			m_pWPhysState->Debug_RenderVertex(Pos+Offset,_Color,_Duration,true);
			if (Pos.DistanceSqr(PrevPos) > Sqr(2.0f))
			{
				m_pWPhysState->Debug_RenderWire(PrevPos+Offset,Pos+Offset,_Color,_Duration,true);
			}
		}
	}
#endif
	return(kSuccess);
}

void CConstraintSystem::EvalAnim(fp32 _Frac,CXR_AnimLayer* _pLayers, int _nLayers,CXR_Skeleton* _pSkel,CXR_SkeletonInstance* _pSkelInst, CMat4Dfp32& _WMat,int _Flags)
{
	m_pSystemClient->EvalAnim(_Frac,_pLayers,_nLayers,_pSkel,_pSkelInst,_WMat,_Flags);
}

int16 CConstraintSystem::GetDefaultPose(CXR_SkeletonInstance* _pSkelInstance)
{
	if (!m_pWPhysState)
	{
		return(kFailure);
	}

	CXR_Model* pModel = m_pWPhysState->GetMapData()->GetResource_Model(m_pClient->m_iModel[0]);
	if (!pModel)
		return kFailure;
	CXR_Skeleton* pSkel = pModel->GetPhysSkeleton();
	CXR_SkeletonInstance* pSkelInstance = _pSkelInstance;
	if (!pSkelInstance)
	{
		CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(m_pClient);
		if(!pCD)
			return kFailure;
		pSkelInstance = CWObject_Character::GetClientSkelInstance(pCD, 1);
	}
	
	if ((!pSkel)||(!pSkelInstance))
		return (kFailure);
	if (m_lDefaultPts.Len() != pSkel->m_lNodes.Len())
	{
		m_lDefaultPts.Clear();
		m_lDefaultPts.SetLen(pSkel->m_lNodes.Len());
	}
	if (m_lDefaultMats.Len() != pSkel->m_lNodes.Len())
	{
		m_lDefaultMats.Clear();
		m_lDefaultMats.SetLen(pSkel->m_lNodes.Len());
	}

	CMat4Dfp32 Mat;
	Mat.Unit();
	CMat4Dfp32 BaseMat;
	BaseMat.Unit();
	
	if (!m_OrgMat.AlmostUnit4x3(kEpsilon))
	{
		BaseMat = m_OrgMat;
	}
	else
	{
		CalcMatrix(true,0,Mat,pSkelInstance,pSkel,BaseMat);
	}
	BaseMat.Unit3x3();
	
	CalcMatrices_r(false,0,BaseMat,pSkelInstance,pSkel,m_lDefaultMats);
	for (int i = 0; i < m_lDefaultMats.Len(); i++)
	{
		m_lDefaultPts[i] = CVec3Dfp32::GetRow(m_lDefaultMats[i],3);
	}

	// We assume the constraint would accept the default pose so we just calculate the system
	// and use it to build the default matrix
	UpdateCoordinateSystem();

	return(kSuccess);
}

int16 CConstraintSystem::PostSetupToLocal()
{
	if (!m_pClient)
	{
		return(kFailure);
	}
	GetAnimatedFrame(false);

	return(kSuccess);
};

// Retrieves the bone positions of the character
// If _bVelocity == false the system will be teleported thus no velocity will change
int16 CConstraintSystem::GetAnimatedFrame(bool _bVelocity)
{
	if (!m_pWPhysState)
	{
		return(kFailure);
	}

	CXR_Model* pModel = m_pWPhysState->GetMapData()->GetResource_Model(m_pClient->m_iModel[0]);
	if (!pModel)
		return kFailure;
	CXR_Skeleton* pSkel = pModel->GetPhysSkeleton();
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(m_pClient);
	if(!pCD)
		return kFailure;
	CXR_SkeletonInstance* pSkelInstance = CWObject_Character::GetClientSkelInstance(pCD, 1);
	pSkelInstance->CreateTracks(pSkel->m_nUsedRotations, pSkel->m_nUsedMovements);

	TArray<CMat4Dfp32> lMat;
	lMat.SetLen(pSkel->m_lNodes.Len());

	CMat4Dfp32 Mat;
	Mat.Unit();
	CalcMatrices_r(true,0,Mat,pSkelInstance,pSkel,lMat);

	CVec3Dfp32 Offset = CVec3Dfp32(0,0,0);
	for (int i = 0; i < m_lpPts.Len(); i++)
	{
		if ((m_lpPts[i] != NULL)&&(m_lpPts[i]->m_bCollision))
		{
			CVec3Dfp32 Pos = CVec3Dfp32::GetRow(lMat[i],3);
			if (_bVelocity)
			{
				m_lpPts[i]->MovePos(Pos,true);
			}
			else
			{
				m_lpPts[i]->SetPos(Pos);
			}
		}
	}

	// We assume the constraint would accept the default pose so we just calculate the system
	// and use it to build the default matrix
	UpdateCoordinateSystem();

	return(kSuccess);
}
/*
// Retrieves the bone positions of the character
// If _bVelocity == false the system will be teleported thus no velocity will change
// If _bAnimatedPose == true we take the current pose, otherwise we take the default
// _BlendFactor blends between pose (0.0) and ragdoll (1.0)
int16 CConstraintSystem::BlendAnimState(bool _bVelocity,bool _bAnimatedPose,fp32 _BlendFactor)
{
	int16 rErr = kFailure;
	if (!m_pWPhysState)
	{
		return(kFailure);
	}

	if (_BlendFactor >= 1.0f)
	{	// No blending is needed when we want 100% ragdoll
		rErr = Simulate();
		ERR_CHECK(rErr);
		return(kSuccess);
	}

	// Retrieve the skeleton and the skeleton instance
	// Create a list of matrices
	// Fill the list through a recursive call of CalcMatrices_r
	// Set the existing m_lPts with values from the list of matrices
	CXR_Model* pModel = m_pWPhysState->GetMapData()->GetResource_Model(m_pClient->m_iModel[0]);
	CXR_Skeleton* pSkel = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
	CXR_SkeletonInstance* pSkelInstance = CWObject_Character::GetClientSkelInstance(m_pClient);

	CMat4Dfp32 Mat;
	Mat.Unit();
	TArray<CMat4Dfp32> lMat;
	lMat.SetLen(pSkel->m_lNodes.Len());

	CalcMatrices_r(_bAnimatedPose,0,Mat,pSkelInstance,pSkel,lMat);

	if (_BlendFactor <= 0.0f)
	{	// Just take the animation values
		for (int i = 0; i < m_lpPts.Len(); i++)
		{
			if (m_lpPts[i] != NULL)
			{
				const CVec3Dfp32& AnimPos = CVec3Dfp32::GetRow(lMat[i],3);
				if (_bVelocity)
				{
					m_lpPts[i]->MovePos(AnimPos);
				}
				else
				{
					m_lpPts[i]->SetPos(AnimPos);
				}
			}
		}
		return(kSuccess);
	}

	// _BlendFactor is higher than 0.0f and lower than 1.0f
	for (int i = 0; i < m_lpPts.Len(); i++)
	{
		if (m_lpPts[i] != NULL)
		{
			const CVec3Dfp32& AnimPos = CVec3Dfp32::GetRow(lMat[i],3);
			const CVec3Dfp32& RagdollPos = m_lpPts[i]->GetPos();
			CVec3Dfp32 Pos = (AnimPos * (1.0f - _BlendFactor)) + (RagdollPos * _BlendFactor);
			if (_bVelocity)
			{	
				m_lpPts[i]->MovePos(Pos);
			}
			else
			{
				m_lpPts[i]->SetPos(Pos);
			}
		}
	}

	rErr = Simulate();
	ERR_CHECK(rErr);

	return(kSuccess);
}
*/

void CConstraintSystem::CalcMatrix(bool _bAnimatedPose,
									   int _iNode,
										const CMat4Dfp32 &_Mat,
										CXR_SkeletonInstance* _pSkelInstance,
										CXR_Skeleton* _pSkel,CMat4Dfp32& _ReturnMat)
{
	if(!_pSkel || _pSkel->m_lNodes.Len() <= _iNode)
		return;

	CXR_SkeletonNode* pN = &_pSkel->m_lNodes[_iNode];
	
	CMat4Dfp32 Mat;
	int iRot = pN->m_iRotationSlot;
	if ((_bAnimatedPose)&&((iRot >= 0) && (iRot < _pSkelInstance->m_nTracksRot)))
		_pSkelInstance->m_pTracksRot[iRot].CreateMatrix(Mat);
	else
		Mat.Unit();
	
	int iMove = pN->m_iMovementSlot;
	if ((_bAnimatedPose)&&((iMove >= 0) && (iMove < _pSkelInstance->m_nTracksMove)))
		Mat.r[3]=M_VSetW1(M_VNeg(_pSkelInstance->m_pTracksMove[iMove]));

	CVec3Dfp32 Pos = pN->m_LocalCenter;
	Pos *= Mat;
	Pos -= pN->m_LocalCenter;
	CVec3Dfp32::GetMatrixRow(Mat,3) = -Pos;

	CMat4Dfp32 Tmp;
	Mat.Multiply(_Mat,Tmp);

	Pos = pN->m_LocalCenter;
	Pos *= Tmp;

	_ReturnMat = Tmp;
	Pos.SetRow(_ReturnMat,3);
}

void CConstraintSystem::CalcMatrices_r(bool _bAnimatedPose,
									   int _iNode,
										const CMat4Dfp32 &_Mat,
										CXR_SkeletonInstance* _pSkelInstance,
										CXR_Skeleton* _pSkel,TArray<CMat4Dfp32> &_lMat)
{
	if(!_pSkel || !_pSkelInstance || _pSkel->m_lNodes.Len() <= _iNode)
		return;

	CXR_SkeletonNode* pN = &_pSkel->m_lNodes[_iNode];
	
	CMat4Dfp32 Mat;
	int iRot = pN->m_iRotationSlot;
	if ((_bAnimatedPose)&&((iRot >= 0) && (iRot < _pSkelInstance->m_nTracksRot)))
		_pSkelInstance->m_pTracksRot[iRot].CreateMatrix(Mat);
	else
		Mat.Unit();
	
	int iMove = pN->m_iMovementSlot;
	if ((_bAnimatedPose)&&((iMove >= 0) && (iMove < _pSkelInstance->m_nTracksMove)))
		Mat.r[3]=M_VSetW1(M_VNeg(_pSkelInstance->m_pTracksMove[iMove]));

	CVec3Dfp32 Pos = pN->m_LocalCenter;
	Pos *= Mat;
	Pos -= pN->m_LocalCenter;
	CVec3Dfp32::GetMatrixRow(Mat,3) = -Pos;

	CMat4Dfp32 Tmp;
	Mat.Multiply(_Mat,Tmp);

	Pos = pN->m_LocalCenter;
	Pos *= Tmp;

	_lMat[_iNode] = Tmp;
	Pos.SetRow(_lMat[_iNode], 3);

	int nCh = pN->m_nChildren;
	for(int i = 0; i < nCh; i++)
	{
		CalcMatrices_r(_bAnimatedPose,_pSkel->m_liNodes[pN->m_iiNodeChildren + i],Tmp,_pSkelInstance,_pSkel,_lMat);
	}
}





// =====================================================================================================
// CConstraintRigidObject
// =====================================================================================================

#define RIGID_RADIUS 2.0f
CConstraintRigidObject::CConstraintRigidObject()
{
	m_SystemType = CS_RIGIDBODY;
	m_bFirstApply = true;
	m_iObject = 0;
	m_pClient = NULL;
	m_pWPhysState = NULL;
	m_pCD = NULL;
	m_pSystemClient = NULL;
	m_iHardImpactSound = -1;
	m_iSoftImpactSound = -1;
	m_iCollisionSoundNextTick = 0;
};

void CConstraintRigidObject::Init(int _iObj,CWObject_CoreData* _pClient,CWorld_PhysState* _pWPhysState)
{
	if (m_bInited) {return;}

	m_SystemType = CS_RIGIDBODY;
	m_bInited = true;
	m_bFirstApply = true;
	m_iObject = _iObj;
	m_pClient = _pClient;
	m_pWPhysState = _pWPhysState;
	m_pCD = NULL; // Truly seems unecessary
	m_pSystemClient = NULL;
	// *** Really? ***
	m_iHardImpactSound = -1;
	m_iSoftImpactSound = -1;
	m_iCollisionSoundNextTick = 0;
}

void CConstraintRigidObject::StartCollecting(CWorld_PhysState* _pWPhysState, CXR_Model *_pModel, SConstraintSystemSettings* _pSettings)
{
	m_bFirstApply = true;
	m_iObject = 0;
	m_pClient = NULL;
	m_pWPhysState = _pWPhysState;
	m_pCD = NULL; // Truly seems unecessary
	m_pSystemClient = NULL;
	Setup(_pSettings, _pModel);
	// *** Really? ***
	m_iHardImpactSound = -1;
	m_iSoftImpactSound = -1;
	m_iCollisionSoundNextTick = 0;
}

void CConstraintRigidObject::PackStopped()
{
	if (m_State != STOPPED)
	{
		return;
	}
	m_lDefaultMats.Clear();
};

void CConstraintRigidObject::UnPackStopped()
{
	if (m_State == STOPPED)
	{
		// Nothing yet
	}
};

void CConstraintRigidObject::SetState(int _State)
{
	/*
	if ((_State == STOPPED)&&(m_State != STOPPED))
	{	// Do a NOTICE_PLAYER_AT thingy here
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWPhysState->Selection_AddClass(Selection,"character");
		const int16* pSel = NULL;
		int nSel = m_pWPhysState->Selection_Get(Selection, &pSel);
		// Iterate through 'em all (this shure will generate a spike :)
		for (int k = 0; k < nSel; k++)
		{
			int iCur = pSel[k];
			CWObject* pObj = m_pWPhysState->Object_Get(iCur);
			if (pObj)
			{
				CWObject_Message Msg = CWObject_Message(OBJMSG_IMPULSE,IMPULSE_NOTICE_PLAYER_AT,m_iObject,m_iObject);
				Msg.m_pData = "$activator";
				pObj->OnMessage(Msg);
			}
		}
	}
	*/
	CConstraintSystem::SetState(_State);
};

void CConstraintRigidObject::Setup(SConstraintSystemSettings* _pSettings,CXR_Model *_pModel)
{
	SetConstraintIterations(_pSettings->m_ConstraintIterations);
	SetStopThreshold(_pSettings->m_StopThreshold);
	SetRebound(_pSettings->m_Rebound);
	SetupRigidSystem(_pModel);
};

static CVec3Dfp32 gs_lPointOffsets[] =
{
	CVec3Dfp32(RIGID_RADIUS,RIGID_RADIUS,RIGID_RADIUS), // INVOLVED IN COORDINATE SYSTEM
		CVec3Dfp32(-RIGID_RADIUS,RIGID_RADIUS,RIGID_RADIUS),// INVOLVED IN COORDINATE SYSTEM
		CVec3Dfp32(RIGID_RADIUS,-RIGID_RADIUS,RIGID_RADIUS),
		CVec3Dfp32(-RIGID_RADIUS,-RIGID_RADIUS,RIGID_RADIUS),
		CVec3Dfp32(RIGID_RADIUS,RIGID_RADIUS,-RIGID_RADIUS),// INVOLVED IN COORDINATE SYSTEM
		CVec3Dfp32(-RIGID_RADIUS,RIGID_RADIUS,-RIGID_RADIUS),
		CVec3Dfp32(RIGID_RADIUS,-RIGID_RADIUS,-RIGID_RADIUS),
		CVec3Dfp32(-RIGID_RADIUS,-RIGID_RADIUS,-RIGID_RADIUS),
/*			CVec3Dfp32(-2,0,0),
			CVec3Dfp32(2,0,0),
			CVec3Dfp32(0),
			CVec3Dfp32(2,0,0),
			CVec3Dfp32(0),
			CVec3Dfp32(0),
			CVec3Dfp32(-2,0,0),*/
};

int16 CConstraintRigidObject::SetupRigidSystem(CXR_Model *_pModel)
{
	if (!_pModel)
		return kFailure;

	m_pModel = _pModel;

	SetPointRadius(RIGID_RADIUS);
	
	CBox3Dfp32 BBox;
	m_pModel->GetBound_Box(BBox); 

	m_lDefaultPts.Clear();
	m_lDefaultPts.SetLen(8);
	BBox.GetVertices(m_lDefaultPts.GetBasePtr());

	for(int i= 0; i < 8; ++i)
	{
		CVec3Dfp32 Pos = m_lDefaultPts[i] * m_OrgMat;
		AddPoint(i,Pos,1.0f,true,BODY_PART);
	}

	m_pWPhysState->Debug_RenderVertex(m_lpPts[kV0]->m_Pos,kColorWhite,30.0f,false);
	m_pWPhysState->Debug_RenderVertex(m_lpPts[kV1]->m_Pos,kColorRed,30.0f,false);
	m_pWPhysState->Debug_RenderVertex(m_lpPts[kV2]->m_Pos,kColorGreen,30.0f,false);
	m_pWPhysState->Debug_RenderVertex(m_lpPts[kV3]->m_Pos,kColorBlue,30.0f,false);

	m_pWPhysState->Debug_RenderVertex(m_lpPts[kV4]->m_Pos,kColorWhite,30.0f,false);
	m_pWPhysState->Debug_RenderVertex(m_lpPts[kV5]->m_Pos,kColorRed,30.0f,false);
	m_pWPhysState->Debug_RenderVertex(m_lpPts[kV6]->m_Pos,kColorGreen,30.0f,false);
	m_pWPhysState->Debug_RenderVertex(m_lpPts[kV7]->m_Pos,kColorBlue,30.0f,false);

	//Side 1
	AddBone(kV0, kV2);
	AddBone(kV0, kV4);
	AddBone(kV0, kV6);
	AddBone(kV2, kV4);

	AddBone(kV2, kV6);
	AddBone(kV4, kV6);

	//Side 2
	AddBone(kV0, kV1);
	AddBone(kV0, kV4);
	AddBone(kV0, kV5);
	AddBone(kV1, kV4);

	AddBone(kV1, kV5);
	AddBone(kV4, kV5);

	//Side 3
	AddBone(kV1, kV3);
	AddBone(kV1, kV5);
	AddBone(kV1, kV7);
	AddBone(kV3, kV5);

	AddBone(kV3, kV7);
	AddBone(kV5, kV7);

	//Side 4
	AddBone(kV2, kV3);
	AddBone(kV2, kV6);
	AddBone(kV2, kV7);
	AddBone(kV3, kV6);

	AddBone(kV3, kV7);
	AddBone(kV6, kV7);

	//Side 5
	AddBone(kV0, kV1);
	AddBone(kV0, kV2);
	AddBone(kV0, kV3);
	AddBone(kV1, kV2);
	AddBone(kV1, kV3);
	AddBone(kV2, kV3);

	//Side 6
	AddBone(kV4, kV5);
	AddBone(kV4, kV6);
	AddBone(kV4, kV7);
	AddBone(kV5, kV6);

	AddBone(kV5, kV7);
	AddBone(kV6, kV7);

	//Crossings
	AddBone(kV0, kV7);
	AddBone(kV1, kV6);
	AddBone(kV2, kV5);
	AddBone(kV3, kV4);

	// We add them last so they'll be executed first before the box fixing stuff
	// Plane constraints (keeps system from turning inside out)
	/*
	AddPlaneConstraint(4,6,5,7,0,2,1,3,RIGID_RADIUS,kConstraintBoneStiffness);
	AddPlaneConstraint(3,2,1,0,7,6,5,4,RIGID_RADIUS,kConstraintBoneStiffness);
	AddPlaneConstraint(7,3,5,1,6,2,4,0,RIGID_RADIUS,kConstraintBoneStiffness);
	*/

	// Is this right?
	SetupCoordinateSystem(kV0,kV4,kV0,kV2);
	UpdateCoordinateSystem();
	Constrain();
	
	m_CenterOffset = -m_lDefaultPts[0];

	return(kSuccess);
}

void CConstraintRigidObject::UpdateCoordinateSystem()
{
	CConstraintSystem::UpdateCoordinateSystem();
	CMat4Dfp32 Mat = m_SystemMat;
	CVec3Dfp32(0.0f).SetMatrixRow(Mat,3);
	CVec3Dfp32 Origo = m_CenterOffset;
	Origo *= Mat;
	Origo += CVec3Dfp32::GetRow(m_SystemMat,3);
	Origo.SetRow(m_SystemMat,3);
};

int16 CConstraintRigidObject::GetCollBoxPos(CMat4Dfp32* _pPosMat)
{
	*_pPosMat = m_SystemMat;
	return(kSuccess);
};

void CConstraintRigidObject::GetPosition(CMat4Dfp32 &_Pos)
{
	m_pWPhysState->Debug_RenderMatrix(m_SystemMat,5.0f,false,0xff7f0000, 0xff007f00,0xff00007f);
	_Pos = m_SystemMat;
	return;
}

bool CConstraintRigidObject::PlayBodypartSound(int _BodyPart,const CVec3Dfp32& _Pos,fp32 _Impact)
{
	if ((m_iHardImpactSound < 0)&&(m_iSoftImpactSound))
		return(kFailure);

	CWorld_Server* pWServer = safe_cast<CWorld_Server>(m_pWPhysState);
	if (pWServer->GetGameTick() > m_iCollisionSoundNextTick)
	{
		int iSound = -1;
		if (m_iHardImpactSound != -1)
		{	// First impact has NOT been played
			// Play strong or weak depending on _Impact and reset strong
			if (_Impact >= kConstraintFirstImpactSoundThreshold)
			{
				iSound = m_iHardImpactSound;
			}
			else
			{
				iSound = m_iSoftImpactSound;
			}
			m_iHardImpactSound = -1;
		}
		else
		{	// First impact has been played
			if (_Impact >= kConstraintWeakImpactSoundThreshold)
			{
				iSound = m_iSoftImpactSound;
			}
		}
		
		if (iSound != -1)
		{
			pWServer->Sound_On(m_iObject, iSound, WCLIENT_ATTENUATION_3D);
			m_iCollisionSoundNextTick = pWServer->GetGameTick() + kConstraintBodypartSoundInterval;
		}
		return(kSuccess);
	}
	else
	{
		return(kFailure);
	}
}

// Retrieves the bone positions of the character
// If _bVelocity == false the system will be teleported thus no velocity will change
int16 CConstraintRigidObject::GetAnimatedFrame(bool _bVelocity)
{
	if (!m_pWPhysState)
	{
		return(kFailure);
	}

	TArray<CMat4Dfp32> lMat;
	lMat.SetLen(8);

	CMat4Dfp32 Mat;
	Mat.Unit();
	CalcMatrices_r(true,0,Mat,NULL,NULL,lMat);

	CVec3Dfp32 Offset = CVec3Dfp32(0,0,0);
	for (int i = 0; i < m_lpPts.Len(); i++)
	{
		if ((m_lpPts[i] != NULL)&&(m_lpPts[i]->m_bCollision))
		{
			CVec3Dfp32 Pos = CVec3Dfp32::GetRow(lMat[i],3);
			if (_bVelocity)
			{
				m_pWPhysState->Debug_RenderVertex(Pos+Offset,kColorRed,1,true);
				m_lpPts[i]->MovePos(Pos);
				Pos = m_lpPts[i]->m_Pos;
				CVec3Dfp32 PrevPos = m_lpPts[i]->m_PrevFramePos;
				m_pWPhysState->Debug_RenderWire(Pos+Offset,PrevPos+Offset,kColorRed,1,false);
			}
			else
			{
				m_pWPhysState->Debug_RenderVertex(Pos+Offset,kColorRed,10,true);
				m_lpPts[i]->SetPos(Pos);
			}
		}
	}

	// We assume the constraint would accept the default pose so we just calculate the system
	// and use it to build the default matrix
	UpdateCoordinateSystem();

	return(kSuccess);
}


// Retrieves the bone positions of the character
// If _bVelocity == false the system will be teleported thus no velocity will change
int16 CConstraintRigidObject::CollectFrame(bool _bVelocity, CMat4Dfp32& _WMat)
{
	if (!m_pWPhysState)
	{
		return(kFailure);
	}
	TArray<CMat4Dfp32> lMat;
	lMat.SetLen(8);
	CalcMatrices(_WMat, lMat);

	CVec3Dfp32 Offset = CVec3Dfp32(0,0,0);
	for (int i = 0; i < m_lpPts.Len(); i++)
	{
		if ((m_lpPts[i] != NULL)&&(m_lpPts[i]->m_bCollision))
		{
			CVec3Dfp32 Pos = CVec3Dfp32::GetRow(lMat[i],3);
			if (_bVelocity)
			{
				m_pWPhysState->Debug_RenderVertex(Pos+Offset,kColorRed,1,true);
				m_lpPts[i]->MovePos(Pos);
				Pos = m_lpPts[i]->m_Pos;
				CVec3Dfp32 PrevPos = m_lpPts[i]->m_PrevFramePos;
				m_pWPhysState->Debug_RenderWire(Pos+Offset,PrevPos+Offset,kColorRed,1,false);
			}
			else
			{
				m_pWPhysState->Debug_RenderVertex(Pos+Offset,kColorRed,10,true);
				m_lpPts[i]->SetPos(Pos);
			}
		}
	}

	// We assume the constraint would accept the default pose so we just calculate the system
	// and use it to build the default matrix
	UpdateCoordinateSystem();

	return(kSuccess);
}


void CConstraintRigidObject::CalcMatrices(const CMat4Dfp32 &_Mat, TArray<CMat4Dfp32> &_lMat)
{
	if (!m_pModel)
		return;
	CMat4Dfp32 Mat = _Mat;

	CBox3Dfp32 BBox;
	m_pModel->GetBound_Box(BBox); 

	CVec3Dfp32 BBVerts[8];
	BBox.GetVertices(BBVerts);
	/*	CVec3Dfp32 Center;
	BBox.GetCenter(Center);

	CVec3Dfp32::GetMatrixRow(Mat, 3) += Center;*/
	for(int i = 0; i < 8; ++i)
	{
		CVec3Dfp32 AttPos = (BBVerts[i]) * Mat;
		_lMat[i] = Mat;
		AttPos.SetRow(_lMat[i], 3);

	}
}

void CConstraintRigidObject::CalcMatrices_r(bool _bAnimatedPose,
									   int _iNode,
									   const CMat4Dfp32 &_Mat,
									   CXR_SkeletonInstance* _pSkelInstance,
									   CXR_Skeleton* _pSkel,TArray<CMat4Dfp32> &_lMat)
{
	if (!m_pModel)
		return;
	CMat4Dfp32 Mat = m_pClient->GetPositionMatrix();

	CBox3Dfp32 BBox;
	m_pModel->GetBound_Box(BBox); 

	CVec3Dfp32 BBVerts[8];
	BBox.GetVertices(BBVerts);


/*	CVec3Dfp32 Center;
	BBox.GetCenter(Center);

	CVec3Dfp32::GetMatrixRow(Mat, 3) += Center;*/

	for(int i = 0; i < 8; ++i)
	{
		CVec3Dfp32 AttPos = (BBVerts[i] + gs_lPointOffsets[i]) * Mat;
		_lMat[i] = Mat;
		AttPos.SetRow(_lMat[i], 3);
	}
}

int16 CConstraintRigidObject::Animate(int32 _GameTick,CMat4Dfp32& _WMat)
{
	MSCOPESHORT( CConstraintSystem::Animate );

	int32 rErr = kSuccess;

	if (_GameTick <= m_LastFrameTick)
	{
		return(kFailure);
	}

	if (!m_bInited)
	{
		CollectFrame(true,_WMat);
		m_LastFrameTick = _GameTick;
		return(kFailure);
	}

	switch(m_State)
	{
	case NOTREADY:
		{
			rErr = kFailure;
			m_LastFrameTick = _GameTick;
		}
		break;

	case GETFRAME:
		{
			m_CollMinBox = CVec3Dfp32(_FP32_MAX);
			m_CollMaxBox = CVec3Dfp32(-_FP32_MAX);
			m_MoveMinBox = CVec3Dfp32(_FP32_MAX);
			m_MoveMaxBox = -CVec3Dfp32(_FP32_MAX);
			m_iCollSelection = -1;
			m_lCollPolys.Clear();
			m_lDynamicCollPolys.Clear();
			Simulate();
			ERR_CHECK(rErr);
			m_State = READY;
			m_LastFrameTick = _GameTick;
			m_AnimateCount++;
			rErr = kSuccess;
		}
		break;

	case READY:
		{
			Simulate();
			ERR_CHECK(rErr);

			// We should increase the threshold after a certain time
			int NSeconds = (int)(m_AnimateCount * 0.05f);
			if (CheckStop(m_StopThreshold * NSeconds))
			{
				m_State = STOPPED;
			}

			m_LastFrameTick = _GameTick;
			m_AnimateCount++;
			rErr = kSuccess;
		}
		break;

	case STOPPED:
		{
			m_LastFrameTick = _GameTick;
			rErr = kSuccess;
		}
		break;
	}

	if (m_State != STOPPED)
	{
		DrawPoints(5.0f,kColorYellow);
	}

	return(rErr);
}

// =====================================================================================================
// CConstraintGib
// =====================================================================================================
CConstraintGib::CConstraintGib()
{
	CConstraintGib::Clear(true);
};

CConstraintGib::~CConstraintGib()
{
	// Kill the boxes before any rules and points
	// (not neccessary, just prudent as the boxes rely on the points and rules being there)
	for (int i = 0; i < m_lpBoxes.Len(); i++)
	{
		CConstraintBox* cur = m_lpBoxes[i];
		// No point in checking if cur == NULL, delete on NULL is a NOP
		delete cur;
	}
	m_lpBoxes.Clear();
};

void CConstraintGib::Init(CWObject_Client* _pObj,CWorld_PhysState* _pWPhysState)
{
	m_SystemType = CS_GIBSYSTEM;
	m_bInited = true;
	m_pClient = _pObj;
	m_pWPhysState = _pWPhysState;
	m_LastFrameTick = 0;
};

void CConstraintGib::Clear(bool _bAll)
{
	// Kill the boxes before any rules and points
	// (not neccessary, just prudent as the boxes rely on the points and rules being there)
	for (int i = 0; i < m_lpBoxes.Len(); i++)
	{
		CConstraintBox* cur = m_lpBoxes[i];
		// No point in checking if cur == NULL, delete on NULL is a NOP
		delete cur;
	}
	m_lpBoxes.Clear();

	CConstraintSystem::Clear(_bAll);
};

int16 CConstraintGib::GetCollBoxPos(CMat4Dfp32* _pPosMat)
{
	CVec3Dfp32 CGPos = CVec3Dfp32(0.0f);
	fp32 Count = 0.0f;
	for (int i = 0; i < m_lpBoxes.Len(); i++)
	{
		if (m_lpBoxes[i] == NULL) {continue;}
		CGPos += CVec3Dfp32::GetRow(m_lpBoxes[i]->m_SystemMat,3);
		Count += 1.0f;
	}
	if (Count > 0.0f)
	{
		CGPos *= 1.0f / Count;
		return(kSuccess);
	}
	else
	{
		return(kFailure);
	}
};

int16 CConstraintGib::AddBox(CXR_Model* _pModel,CMat4Dfp32& _Pos)
{
	int16 rErr = kSuccess;
	CConstraintBox* pBox = MNew(CConstraintBox);
	if (!pBox) {return(kErrMemory);}
	pBox->Init(this);
	rErr  = pBox->Setup(_pModel,_Pos);
	if( rErr < kFailure )
	{
		delete pBox;
		return rErr;
	}
	m_lpBoxes.Add(pBox);
	return(m_lpBoxes.Len()-1);
};

int16 CConstraintGib::GetBoxMatrix(int _iBox,CMat4Dfp32& _Pos)
{
	if ((_iBox < 0)||(_iBox >= m_lpBoxes.Len()))
	{
		return(kErrIndexOutOfRange);
	}
	if (m_lpBoxes[_iBox] == NULL)
	{
		return(kErrNull);
	}
	_Pos = m_lpBoxes[_iBox]->m_SystemMat;

	return(kSuccess);
};

// Calls CalcForces,Move,Collision,Constrain,Apply for the system§
int16 CConstraintGib::Animate(int32 _GameTick)
{
	if (_GameTick <= m_LastFrameTick)
	{
		return(kFailure);
	}

	Simulate();
	m_LastFrameTick = _GameTick;

	for (int i = 0; i < m_lpBoxes.Len(); i++)
	{
		if (m_lpBoxes[i] != NULL)
		{
			m_lpBoxes[i]->UpdateCoordinateSystem();
		}
	}

	DrawPoints(5.0f,kColorYellow);
	return(kSuccess);
};

int16 CConstraintGib::Apply(CMat4Dfp32& _WMat)
{
	int16 rErr = kSuccess;

	return(rErr);
};

int16 CConstraintGib::Explode(CVec3Dfp32 _Origin,CVec3Dfp32 _Params)
{
	for (int i = 0; i < m_lpBoxes.Len(); i++)
	{
		if (m_lpBoxes[i] != NULL)
		{
			m_lpBoxes[i]->Explode(_Origin,_Params);
		}
	}
	
	return(kSuccess);
};

// =====================================================================================================
// CConstraintSystemClient
// =====================================================================================================
CConstraintSystemClient::CConstraintSystemClient()
{
	m_State = NOTREADY;
	Clear();
}

CConstraintSystemClient::~CConstraintSystemClient()
{
	Clear();
}

void CConstraintSystemClient::Clear()
{
	m_State = NOTREADY;
	m_OrgMat.Unit();
	m_nTracks = 0;

	m_UnPackCount = 0;
	m_lRot0.Clear();
	m_lMove0.Clear();
	m_lRot1.Clear();
	m_lMove1.Clear();
}

bool CConstraintSystemClient::GrowTracks(int _nTracks)
{
	if (_nTracks >= m_nTracks)
	{
		CVec3Dfp32 VZero(0.0f);
	
		m_nTracks = _nTracks+1;
		m_lRot0.SetLen(m_nTracks);
		m_lRot1.SetLen(m_nTracks);
		m_lMove0.SetLen(m_nTracks);
		m_lMove1.SetLen(m_nTracks);
		for (int i = 0; i < m_nTracks; i++)
		{
			m_lRot0[i].Unit();
			m_lRot1[i].Unit();
			m_lMove0[i] = VZero;
			m_lMove1[i] = VZero;
		}
		return(true);
	}
	else
	{
		return(false);
	}

}

void CConstraintSystemClient::Init(int _nTracks)
{
	Clear();
	m_nTracks = _nTracks;
	CVec3Dfp32 VZero(0.0f);

	m_lRot0.SetLen(m_nTracks);
	m_lRot1.SetLen(m_nTracks);
	m_lMove0.SetLen(m_nTracks);
	m_lMove1.SetLen(m_nTracks);
	for (int i = 0; i < m_nTracks; i++)
	{
		m_lRot0[i].Unit();
		m_lRot1[i].Unit();
		m_lMove0[i] = VZero;
		m_lMove1[i] = VZero;
	}
}

void CConstraintSystemClient::EvalAnim(fp32 _Frac,CXR_AnimLayer* _pLayers, int _nLayers,CXR_Skeleton* _pSkel,CXR_SkeletonInstance* _pSkelInst, CMat4Dfp32& _WMat,int _Flags, fp32 _OverrideRagdollHeight)
{
	CMat4Dfp32 Mat;

	if ((m_State == CConstraintSystem::NOTREADY)||
		(m_State == CConstraintSystem::GETFRAME))
	{
		_pSkel->EvalAnim(_pLayers,_nLayers,_pSkelInst,_WMat,_Flags);
		return;
	}

	// Return the org pos, we  dn't want to be pushed
	_WMat = m_OrgMat;

	CQuatfp32 Rot;
	CVec3Dfp32 Move;

	// m_lBoneTransform.Len() is sometimes less than m_nTracks
	// when a skeleton with less bones ise used
	int Count = MinMT(m_nTracks, _pSkelInst->m_nBoneTransform);
	// if (m_State != CConstraintSystemClient::STOPPED)

	if (m_UnPackCount >= 2)
	{
		for (int i = 0; i < Count; i++)
		{
			/*
			if ((i != kHead)&&(i != kNeck))
			{
				Rot = m_lRot1[i];
				Move = m_lMove1[i];
				CQuatfp32::Lerp(m_lRot0[i],m_lRot1[i],Rot,_Frac);
				CVec3Dfp32::Lerp(m_lMove0[i],m_lMove1[i],Move,_Frac);
			}
			else
			{
				Rot = m_lRot0[i];
				Move = m_lMove0[i];

			}
			*/
			m_lRot0[i].Lerp(m_lRot1[i],_Frac,Rot);
			m_lMove0[i].Lerp(m_lMove1[i],_Frac,Move);
			Rot.CreateMatrix(_pSkelInst->m_pBoneTransform[i]);
			Move.k[2] -= _OverrideRagdollHeight;
			Move.SetRow(_pSkelInst->m_pBoneTransform[i],3);
		}
	}
	else	// Only one data packet received yet
	{
		for (int i = 0; i < Count; i++)
		{
			Rot = m_lRot1[i];
			Move = m_lMove1[i];
			Rot.CreateMatrix(_pSkelInst->m_pBoneTransform[i]);
			Move.SetRow(_pSkelInst->m_pBoneTransform[i],3);
		}
	}

	uint nRotations = MaxMT(2, _pSkel->m_nUsedRotations);
	uint nMovements = MaxMT(2, _pSkel->m_nUsedMovements);
	_pSkelInst->CreateTracks(nRotations, nMovements);

	// Transform
	CQuatfp32* pRot = _pSkelInst->m_pTracksRot;
	CVec4Dfp32* pMove = _pSkelInst->m_pTracksMove;

	{	// Flags == 0
		// Move WMat -> Pos0
		pRot[0].Create(_WMat);
		pMove[0] = M_VSetW1(_WMat.r[3]);

		CMat4Dfp32 M0, M1;
		M0.Create(pRot[1],pMove[1]);
		M0.Multiply(_WMat, M1);
		_WMat = M1;
	}
}

bool CConstraintSystemClient::IsValid()
{
	return m_State > CConstraintSystem::GETFRAME;
}

void CConstraintSystemClient::CopyFrom(const CConstraintSystemClient& _From)
{
	m_OrgMat = _From.m_OrgMat;
	m_State = _From.m_State;
	m_nTracks = _From.m_nTracks;
	m_lRot0.QuickSetLen(0);
	m_lRot0.Add(_From.m_lRot0);
	m_lMove0.QuickSetLen(0);
	m_lMove0.Add(_From.m_lMove0);
	m_lRot1.QuickSetLen(0);
	m_lRot1.Add(_From.m_lRot1);
	m_lMove1.QuickSetLen(0);
	m_lMove1.Add(_From.m_lMove1);
}

// Full precision?
// #define QUICKANDDIRTYREPLICATION

void CConstraintSystemClient::Pack(uint8 *&_pD, CMapData* _pMapData) const
{
#ifdef QUICKANDDIRTYREPLICATION
	TAutoVar_Pack(mint(this), _pD);
#else
	TAutoVar_Pack(m_OrgMat, _pD);
	TAutoVar_Pack(m_State, _pD);
	TAutoVar_Pack(m_nTracks, _pD);

	uint16 nRot = m_lRot1.Len();
	uint16 nMove = m_lMove1.Len();
	TAutoVar_Pack(nRot, _pD);
	TAutoVar_Pack(nMove, _pD);

	const CQuatfp32* pRot = m_lRot1.GetBasePtr();
	const CVec3Dfp32* pMove = m_lMove1.GetBasePtr();

	for(int iRot = 0; iRot < nRot; iRot++)
	{
		CXR_Anim_Quatint16 Q16(pRot[iRot]);
		TAutoVar_Pack(Q16.k[0], _pD);
		TAutoVar_Pack(Q16.k[1], _pD);
		TAutoVar_Pack(Q16.k[2], _pD);
		TAutoVar_Pack(Q16.k[3], _pD);
	}

	if(nMove)
	{
		CBox3Dfp32 Box;
		CVec3Dfp32::GetMinBoundBox(pMove, Box.m_Min, Box.m_Max, nMove);
		Box.m_Min -= 1.0f;
		Box.m_Max += 1.0f;

		TAutoVar_Pack(Box.m_Min, _pD);
		TAutoVar_Pack(Box.m_Max, _pD);

		CVec3Dfp32 BoxSizeInv;
		BoxSizeInv[0] = 2047.0f / (Box.m_Max[0] - Box.m_Min[0]);
		BoxSizeInv[1] = 2047.0f / (Box.m_Max[1] - Box.m_Min[1]);
		BoxSizeInv[2] = 1023.0f / (Box.m_Max[2] - Box.m_Min[2]);

		for(int iMove = 0; iMove < nMove; iMove++)
		{
			const CVec3Dfp32& Move = pMove[iMove];
			unsigned int x = RoundToInt((Move[0] - Box.m_Min[0]) * BoxSizeInv[0]);
			unsigned int y = RoundToInt((Move[1] - Box.m_Min[1]) * BoxSizeInv[1]);
			unsigned int z = RoundToInt((Move[2] - Box.m_Min[2]) * BoxSizeInv[2]);

			uint32 coord = x + (y << 11) + (z << 22);
			TAutoVar_Pack(coord, _pD);
		}
	}

#endif
/*	CConstraintSystemClient& This = *(CConstraintSystemClient*)_pThis;

	PTR_PUTDATA(_pD, &This.m_OrgMat, sizeof(This.m_OrgMat));
	PTR_PUTDATA(_pD, &This.m_State, sizeof(This.m_State));
	PTR_PUTDATA(_pD, &This.m_nTracks, sizeof(This.m_nTracks));

	PTR_PUTINT32(_pD, This.m_lRot0.Len());
	PTR_PUTINT32(_pD, This.m_lMove0.Len());
	PTR_PUTINT32(_pD, This.m_lRot1.Len());
	PTR_PUTINT32(_pD, This.m_lMove1.Len());

	PTR_PUTDATA(_pD, This.m_lRot0.GetBasePtr(), sizeof(CQuatfp32) * This.m_lRot0.Len());
	PTR_PUTDATA(_pD, This.m_lMove0.GetBasePtr(), sizeof(CVec3Dfp32) * This.m_lMove0.Len());
	PTR_PUTDATA(_pD, This.m_lRot1.GetBasePtr(), sizeof(CQuatfp32) * This.m_lRot1.Len());
	PTR_PUTDATA(_pD, This.m_lMove1.GetBasePtr(), sizeof(CVec3Dfp32) * This.m_lMove1.Len());*/
}

void CConstraintSystemClient::Unpack(const uint8 *&_pD, CMapData* _pMapData)
{
	// Direct access. Won't work with demos or network play.
	// Implementing correct replication should only be to 
	// pack the Rot1 and Move1 lists from the server and
	// unpack them here. Would be very bandwidth consuming though. - JA

	// Update: That didn't work at all. Maximum size of networkbuffer
	// was 2048 bytes. Direct copy of the data took about 4k of mem.
#ifdef QUICKANDDIRTYREPLICATION
	mint Server;
	PTR_GETMINT(_pD, Server);
	CConstraintSystemClient* pServerObj = (CConstraintSystemClient *)Server;
	CopyFrom(pServerObj);
	m_UnPackCount++;
#else
	m_UnPackCount++;

	TAutoVar_Unpack(m_OrgMat, _pD);
	TAutoVar_Unpack(m_State, _pD);
	TAutoVar_Unpack(m_nTracks, _pD);

	uint16 nRot0, nMove0;
	TAutoVar_Unpack(nRot0, _pD);
	m_lRot0.SetLen(nRot0);
	m_lRot1.SetLen(nRot0);
	TAutoVar_Unpack(nMove0, _pD);
	m_lMove0.SetLen(nMove0);
	m_lMove1.SetLen(nMove0);

	memcpy(m_lMove0.GetBasePtr(), m_lMove1.GetBasePtr(), m_lMove0.ListSize());
	memcpy(m_lRot0.GetBasePtr(), m_lRot1.GetBasePtr(), m_lRot0.ListSize());

	CQuatfp32* pRot = m_lRot1.GetBasePtr();
	CVec3Dfp32* pMove = m_lMove1.GetBasePtr();

	for(int iRot = 0; iRot < nRot0; iRot++)
	{
		CXR_Anim_Quatint16 Q16;
		TAutoVar_Unpack(Q16.k[0], _pD);
		TAutoVar_Unpack(Q16.k[1], _pD);
		TAutoVar_Unpack(Q16.k[2], _pD);
		TAutoVar_Unpack(Q16.k[3], _pD);
		Q16.GetQuatfp32(pRot[iRot]);
	}

	if (nMove0)
	{
		CBox3Dfp32 Box;
		TAutoVar_Unpack(Box.m_Min, _pD);
		TAutoVar_Unpack(Box.m_Max, _pD);

		CVec3Dfp32 BoxSize;
		Box.m_Max.Sub(Box.m_Min, BoxSize);
		BoxSize[0] *= 1.0f / 2047.0f;
		BoxSize[1] *= 1.0f / 2047.0f;
		BoxSize[2] *= 1.0f / 1023.0f;

		for(int iMove = 0; iMove < nMove0; iMove++)
		{
			uint32 coord;
			TAutoVar_Unpack(coord, _pD);

			unsigned x = coord & 2047;
			unsigned y = (coord >> 11) & 2047;
			unsigned z = (coord >> 22) & 1023;

			pMove[iMove][0] = fp32(x) * BoxSize[0] + Box.m_Min[0];
			pMove[iMove][1] = fp32(y) * BoxSize[1] + Box.m_Min[1];
			pMove[iMove][2] = fp32(z) * BoxSize[2] + Box.m_Min[2];
		}
	}
#endif

/*	CConstraintSystemClient& This = *(CConstraintSystemClient*)_pThis;

	PTR_GETDATA(_pD, &This.m_OrgMat, sizeof(This.m_OrgMat));
	PTR_GETDATA(_pD, &This.m_State, sizeof(This.m_State));
	PTR_GETDATA(_pD, &This.m_nTracks, sizeof(This.m_nTracks));

	int nRot0, nMove0, nRot1, nMove1;
	PTR_GETINT32(_pD, nRot0);
	This.m_lRot0.SetLen(nRot0);
	PTR_GETINT32(_pD, nMove0);
	This.m_lMove0.SetLen(nMove0);
	PTR_GETINT32(_pD, nRot1);
	This.m_lRot1.SetLen(nRot1);
	PTR_GETINT32(_pD, nMove1);
	This.m_lMove1.SetLen(nMove1);

	PTR_GETDATA(_pD, This.m_lRot0.GetBasePtr(), sizeof(CQuatfp32) * This.m_lRot0.Len());
	PTR_GETDATA(_pD, This.m_lMove0.GetBasePtr(), sizeof(CVec3Dfp32) * This.m_lMove0.Len());
	PTR_GETDATA(_pD, This.m_lRot1.GetBasePtr(), sizeof(CQuatfp32) * This.m_lRot1.Len());
	PTR_GETDATA(_pD, This.m_lMove1.GetBasePtr(), sizeof(CVec3Dfp32) * This.m_lMove1.Len());*/
}

// =====================================================================================================
// CConstraintPoint
// =====================================================================================================
CConstraintPoint::CConstraintPoint()
{
	CVec3Dfp32 VZero(0,0,0);
	m_Pos = VZero;
	m_PrevPos = m_Pos;
	m_PrevFramePos = m_Pos;
	m_iBonePt = -1;
	m_iBodypart = -1;
	m_Vel = VZero;
	m_DeltaV = VZero;
	m_bCollision = true;
	m_bImpact = true;
	m_iCollPoly = -1;
	m_Mass = 1.0f;
	m_InvMass = 1.0f;
}

// Sets up the physical params
// NOTE: _Mass <= 0 indicates infinite mass
void CConstraintPoint::Setup(fp32 _Mass,bool _bCollision,int _iBodypart)
{
	ModifyMass(_Mass);
	m_bCollision = _bCollision;
	m_iCollPoly = -1;
	if ((_iBodypart >= CConstraintSystem::BODY_NONE)&&(_iBodypart < CConstraintSystem::BODYPARTS_NUM))
	{
		m_iBodypart = _iBodypart;
	}
}

void CConstraintPoint::OnDeltaLoad(CCFile* _pFile)
{
	CVec3Dfp32 VTemp;
	MACRO_READ(fp32,VTemp[0]);
	MACRO_READ(fp32,VTemp[1]);
	MACRO_READ(fp32,VTemp[2]);
	SetPos(VTemp);
};

void CConstraintPoint::OnDeltaSave(CCFile* _pFile)
{
	CVec3Dfp32 VTemp = m_Pos;
	MACRO_WRITE(fp32,VTemp[0]);
	MACRO_WRITE(fp32,VTemp[1]);
	MACRO_WRITE(fp32,VTemp[2]);
};

void CConstraintPoint::ModifyMass(fp32 _Mass)
{
	if (_Mass > 0.0f)
	{
		m_Mass = _Mass;
		m_InvMass = 1.0f / _Mass;
	}
	else
	{
		m_Mass = 1000000.0f;
		m_InvMass = 0.0f;
	}
}

void CConstraintPoint::SetPos(const CVec3Dfp32& _pos)
{
	m_Pos = _pos;
	m_PrevPos = _pos;
	m_PrevFramePos = _pos;
	m_DeltaV = CVec3Dfp32(0,0,0);
	m_Vel = CVec3Dfp32(0,0,0);
}

CVec3Dfp32 CConstraintPoint::GetPos() const
{
	return(m_Pos);
}

void CConstraintPoint::OffsetPos(const CVec3Dfp32& _Offset,bool _bInertia)
{
	if (!_bInertia)
	{
		m_PrevPos += _Offset;
	}
	m_Pos += _Offset;
}

void CConstraintPoint::MovePos(const CVec3Dfp32& _pos,bool _bInertia)
{
	if (!_bInertia)
	{
		m_PrevPos += _pos - m_Pos;
	}
	m_Pos = _pos;
}

void CConstraintPoint::SetVel(const CVec3Dfp32& _vel)
{
	m_PrevPos = m_Pos - _vel;
	m_Vel = _vel;
	m_DeltaV = CVec3Dfp32(0,0,0);
}


void CConstraintPoint::Move()
{
	m_Vel = ((m_Pos - m_PrevPos) * kConstraintPointVelocityDamping) + m_DeltaV;
	m_PrevPos = m_Pos;
	m_Pos += m_Vel;
	m_DeltaV = CVec3Dfp32(0,0,0);
}

void CConstraintPoint::ApplyVelocity(const CVec3Dfp32& _dv)
{
	m_DeltaV += _dv;
}

void CConstraintPoint::ApplyImpulse(const CVec3Dfp32& _di)
{
	m_DeltaV += _di * m_InvMass;
}

// =====================================================================================================
// CConstraintBox
// =====================================================================================================
CConstraintBox::CConstraintBox()
{
	m_pCS = NULL;				// System owning us
	m_pModel = NULL;

	for (int i = 0; i < 8; i++)
	{
		m_iiPts[i] = -1;
	}

	m_iSystemUp0 = -1;
	m_iSystemUp1 = -1;
	m_iSystemLeft0 = -1;
	m_iSystemLeft1 = -1;

	m_SystemMat.Unit();
};


CConstraintBox::~CConstraintBox()
{

};

int16 CConstraintBox::Init(CConstraintSystem* _pSystem)
{
	m_pCS = _pSystem;
	return(kSuccess);
};

int16 CConstraintBox::Setup(CXR_Model* _pModel,CMat4Dfp32& _PosMat)
{
	if ((!m_pCS)||(!_pModel))
	{
		return kFailure;
	}

	m_pModel = _pModel;

	CBox3Dfp32 BBox;
	m_pModel->GetBound_Box(BBox); 

	CVec3Dfp32 BBVerts[8];
	BBox.GetVertices(BBVerts);

	for(int i= 0; i < 8; ++i)
	{
		CVec3Dfp32 Pos = BBVerts[i] * _PosMat;
		m_iiPts[i] = m_pCS->AddPoint(Pos,1.0f,true,CConstraintSystem::BODY_PART);
	}
	m_CenterOffset = -BBVerts[0];

	//Side 1
	m_pCS->AddBone(m_iiPts[0],m_iiPts[2]);
	m_pCS->AddBone(m_iiPts[0],m_iiPts[4]);
	m_pCS->AddBone(m_iiPts[0],m_iiPts[6]);
	m_pCS->AddBone(m_iiPts[2],m_iiPts[4]);

	m_pCS->AddBone(m_iiPts[2],m_iiPts[6]);
	m_pCS->AddBone(m_iiPts[4],m_iiPts[6]);

	//Side 2
	m_pCS->AddBone(m_iiPts[0],m_iiPts[1]);
	m_pCS->AddBone(m_iiPts[0],m_iiPts[4]);
	m_pCS->AddBone(m_iiPts[0],m_iiPts[5]);
	m_pCS->AddBone(m_iiPts[1],m_iiPts[4]);

	m_pCS->AddBone(m_iiPts[1],m_iiPts[5]);
	m_pCS->AddBone(m_iiPts[4],m_iiPts[5]);

	//Side 3
	m_pCS->AddBone(m_iiPts[1],m_iiPts[3]);
	m_pCS->AddBone(m_iiPts[1],m_iiPts[5]);
	m_pCS->AddBone(m_iiPts[1],m_iiPts[7]);
	m_pCS->AddBone(m_iiPts[3],m_iiPts[5]);

	m_pCS->AddBone(m_iiPts[3],m_iiPts[7]);
	m_pCS->AddBone(m_iiPts[5],m_iiPts[7]);

	//Side 4
	m_pCS->AddBone(m_iiPts[2],m_iiPts[3]);
	m_pCS->AddBone(m_iiPts[2],m_iiPts[6]);
	m_pCS->AddBone(m_iiPts[2],m_iiPts[7]);
	m_pCS->AddBone(m_iiPts[3],m_iiPts[6]);

	m_pCS->AddBone(m_iiPts[3],m_iiPts[7]);
	m_pCS->AddBone(m_iiPts[6],m_iiPts[7]);

	//Side 5
	m_pCS->AddBone(m_iiPts[0],m_iiPts[1]);
	m_pCS->AddBone(m_iiPts[0],m_iiPts[2]);
	m_pCS->AddBone(m_iiPts[0],m_iiPts[3]);
	m_pCS->AddBone(m_iiPts[1],m_iiPts[2]);
	m_pCS->AddBone(m_iiPts[1],m_iiPts[3]);
	m_pCS->AddBone(m_iiPts[2],m_iiPts[3]);

	//Side 6
	m_pCS->AddBone(m_iiPts[4],m_iiPts[5]);
	m_pCS->AddBone(m_iiPts[4],m_iiPts[6]);
	m_pCS->AddBone(m_iiPts[4],m_iiPts[7]);
	m_pCS->AddBone(m_iiPts[5],m_iiPts[6]);

	m_pCS->AddBone(m_iiPts[5],m_iiPts[7]);
	m_pCS->AddBone(m_iiPts[6],m_iiPts[7]);

	//Crossings
	m_pCS->AddBone(m_iiPts[0],m_iiPts[7]);
	m_pCS->AddBone(m_iiPts[1],m_iiPts[6]);
	m_pCS->AddBone(m_iiPts[2],m_iiPts[5]);
	m_pCS->AddBone(m_iiPts[3],m_iiPts[4]);

	SetupCoordinateSystem(m_iiPts[0],m_iiPts[4],m_iiPts[0],m_iiPts[1]);
	//Constrain(SERVER_TIMEPERFRAME);

	return(kSuccess);
};

int16 CConstraintBox::SetupCoordinateSystem(int16 _iUp0,int16 _iUp1,int16 _iLeft0,int16 _iLeft1)
{
	m_iSystemUp0 = _iUp0;
	m_iSystemUp1 = _iUp1;
	m_iSystemLeft0 = _iLeft0;
	m_iSystemLeft1 = _iLeft1;

	return(kSuccess);
};

int16 CConstraintBox::UpdateCoordinateSystem()
{
	if ((m_pCS)&&(m_iSystemUp0 >= 0)&&(m_iSystemUp1 >= 0)&&(m_iSystemLeft0 >= 0)&&(m_iSystemLeft1 >= 0))
	{
		CVec3Dfp32 Up = m_pCS->m_lpPts[m_iSystemUp1]->m_Pos - m_pCS->m_lpPts[m_iSystemUp0]->m_Pos;
		CVec3Dfp32 Left =  m_pCS->m_lpPts[m_iSystemLeft1]->m_Pos - m_pCS->m_lpPts[m_iSystemLeft0]->m_Pos;
		Left.SetRow(m_SystemMat,1);
		Up.SetRow(m_SystemMat,2);
		m_SystemMat.RecreateMatrix<2,1>();
		CVec3Dfp32 Pos = CVec3Dfp32(0,0,0);
		for (int i = 0; i < 8; i++)
		{
			Pos += m_pCS->m_lpPts[m_iiPts[i]]->m_Pos;
		}
		// Divide by 8 to get average position
		Pos *= 0.125f;
		Pos.SetRow(m_SystemMat,3);
		return(kSuccess);
	}
	return(kFailure);
};

int16 CConstraintBox::Explode(CVec3Dfp32 _Origin,CVec3Dfp32 _Params)
{
	if (_Params[0] <= 0.0f)
	{
		return(kFailure);
	}

	for (int i = 0; i < 8; i++)
	{
		CVec3Dfp32 Pos = m_pCS->m_lpPts[m_iiPts[i]]->m_Pos;
		fp32 Range = Pos.Distance(_Origin);
		if (Range >= _Params[1]) {continue;}
		if (Range < 0.1f) {continue;}
		fp32 Blast = _Params[0] * (1.0f - Range / _Params[1]) * (1.0f + _Params[2] * (Random - Random));
		CVec3Dfp32 Dir = (Pos - _Origin).Normalize();						
		m_pCS->m_lpPts[m_iiPts[i]]->MovePos(Pos + Dir * Blast,true);
	}

	return(kSuccess);
};

// =====================================================================================================
// CConstraintRule
// =====================================================================================================
// Some handy dandy macros
#define RULE_INVMASS_A	(m_pCS->m_lpPts[m_iPA]->m_InvMass)
#define RULE_INVMASS_B	(m_pCS->m_lpPts[m_iPB]->m_InvMass)
CConstraintRule::CConstraintRule(CConstraintSystem* _pSystem)
{
	m_pCS = _pSystem;

#ifdef _DRAW_SYSTEM
	m_bDraw = false;
#endif
	m_bBone = false;
	m_iPA = -1;
	m_iPB = -1;
	m_Rule = kConstraintRuleIgnore;
	m_bForce = false;
	m_MassFactorAB = 0.5f;
	m_InvLength = 1.0f;
	m_Stiffness = 1.0f;
}

CConstraintRule::~CConstraintRule()
{
	// Nothing here yet
	m_liChildBones.Clear();
}

int16 CConstraintRule::Setup(int16 _iPA,
			int16 _iPB,
			int16 _Rule,
			fp32 _Stiffness)
{
	if (!m_pCS) {return(kErrState);}
	if (_iPA == _iPB) {return(kErrParams);}
	if ((_iPA < 0)||(_iPA >= m_pCS->m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_pCS->m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}
	if ((_Stiffness < 0.0f)||(_Stiffness > 1.0f))
	{
		return(kErrParams);
	}
	
	if ((m_pCS->m_lpPts[_iPA]->m_InvMass < kEpsilon)&&(m_pCS->m_lpPts[_iPB]->m_InvMass < kEpsilon))
	{
		return(kErrParams);
	}

	m_Rule = _Rule;
	m_iPA = _iPA;
	m_iPB = _iPB;

	int16 rErr = ModifyMass();
	ERR_CHECK(rErr);

	CVec3Dfp32 diff = m_pCS->m_lpPts[m_iPB]->m_Pos - m_pCS->m_lpPts[m_iPA]->m_Pos;
	fp32 length = diff.Length();
	if (length >= kEpsilon)
	{
		m_Length = length;
		m_InvLength = 1.0f / m_Length;
	}
	else
	{
		return(kErrParams);
	}

	return(kSuccess);
}

int16 CConstraintRule::AddChildBone(int16 _iChildBone)
{
	if (!m_bBone)
	{
		return(kErrState);
	}

	// We check that _iChildBone is unique
	if ((_iChildBone == m_iPA)||
		(_iChildBone == m_iPB))
	{
		return(kErrParams);
	}

	for (int i = 0; i < m_liChildBones.Len(); i++)
	{
		if (m_liChildBones[i] == _iChildBone)
		{
			return(kErrParams);
		}
	}

	m_liChildBones.Add(_iChildBone);

	return(kSuccess);
}

int16 CConstraintRule::SetRangeMultiplier(fp32 _RangeMultiplier)
{
	if (_RangeMultiplier * m_Length <= kEpsilon)
	{
		return(kErrParams);
	}

	m_Length = m_Length * _RangeMultiplier;
	m_InvLength = 1.0f / m_Length;

	return(kSuccess);
}

int16 CConstraintRule::SetRangeValue(fp32 _RangeValue)
{
if (_RangeValue <= kEpsilon)
	{
		return(kErrParams);
	}

	m_Length = _RangeValue;
	m_InvLength = 1.0f / m_Length;

	return(kSuccess);
}

int16 CConstraintRule::ModifyMass()
{
	if ((RULE_INVMASS_A < kEpsilon)&&(RULE_INVMASS_B < kEpsilon))
	{
		return(kErrParams);
	}

	if (RULE_INVMASS_A >= RULE_INVMASS_B)
	{
		m_MassFactorAB = 1 / (RULE_INVMASS_B / RULE_INVMASS_A + 1.0f);
	}
	else
	{
		m_MassFactorAB = 1.0f - 1 / (RULE_INVMASS_A / RULE_INVMASS_B + 1.0f);
	}

	return(kSuccess);
}

int16 CConstraintRule::ConserveMomentum(int16 _iPA,int16 _iPB,int16 _iPC,CVec3Dfp32 _FB,bool _bInertia)
{
	MSCOPESHORT( CConstraintRule::ConserveMomentum );
	
	// *** Note: Maybe we should remove thse in the release build.
	if ((_iPA < 0)||(_iPA >= m_pCS->m_lpPts.Len())) {return(kErrIndexOutOfRange);}
	if (m_pCS->m_lpPts[_iPA] == NULL) {return(kErrParams);}
	if ((_iPB < 0)||(_iPB >= m_pCS->m_lpPts.Len())) {return(kErrIndexOutOfRange);}
	if (m_pCS->m_lpPts[_iPB] == NULL) {return(kErrParams);}
	if ((_iPC < 0)||(_iPC >= m_pCS->m_lpPts.Len())) {return(kErrIndexOutOfRange);}
	if (m_pCS->m_lpPts[_iPC] == NULL) {return(kErrParams);}
	
//	CVec3Dfp32 AB = m_pCS->m_lpPts[_iPB]->m_Pos - m_pCS->m_lpPts[_iPA]->m_Pos;
//	CVec3Dfp32 AC = m_pCS->m_lpPts[_iPC]->m_Pos - m_pCS->m_lpPts[_iPA]->m_Pos;
//	fp32 FBLen = _FB.Length();				// Force magnitude
//	CVec3Dfp32 FBn = _FB / FBLen;			// Force direction vector
//	CVec3Dfp32 ABr = AB - (AB * FBn);		// Rotational arm AB perpendicular to _FB
//	CVec3Dfp32 ACr = AC - (AC * FBn);		// Rotational arm AC perpendicular to _FB
//	fp32 ABrLen = ABr.Length();
//	fp32 ACrLen = ACr.Length();
//	fp32 ABTorque = ABrLen * FBLen;
//	if (ABrLen < kEpsilon)
//	{	//
//	}


	return(kSuccess);
};

int16 CConstraintRule::Constrain() const
{
	MSCOPESHORT( CConstraintRule::Constrain );
	CVec3Dfp32 separation;
	fp32 diff,realDiff;
	CVec3Dfp32* pPosA = &(m_pCS->m_lpPts[m_iPA]->m_Pos);
	CVec3Dfp32* pPosB = &(m_pCS->m_lpPts[m_iPB]->m_Pos);

#ifdef _DRAW_SYSTEM
	m_bDraw = false;
#endif

	separation = (*pPosA) - (*pPosB);
	// Early out before expensive normalization
	// NOTE: We could store separation.LengthSqr() to avoid doing it again in separation.Length() below
	const fp32 LengthSqr = separation.LengthSqr();
	if (m_Rule & kConstraintRuleSmallerMask)
	{	// Rule is smaller
		if (LengthSqr < Sqr(m_Length))
		{
			return(kSuccess);
		}
	}
	else if (m_Rule & kConstraintRuleLargerMask)
	{	// Rule is larger
		if (LengthSqr > Sqr(m_Length))
		{
			return(kSuccess);
		}
	}

	realDiff = M_Sqrt(LengthSqr) * m_InvLength;
	diff = realDiff;
	if (realDiff > 1.5f)
	{	// Not too big constraint steps
		diff = 1.5f;
	}
	else if (realDiff < kEpsilon)
	{	// Not very nice but it keeps the points apart
		separation[0] = Random;
		separation[1] = Random;
		separation[2] = Random;
		diff = separation.Length() * m_InvLength;
	}

	switch(m_Rule)
	{
	case kConstraintRuleLarger:
		if (M_Fabs(diff) < 1.0f)
		{
			(*pPosA) -= separation * (diff - 1.0f) * m_MassFactorAB;
			(*pPosB) += separation * (diff - 1.0f) * (1.0f - m_MassFactorAB);
#ifdef _DRAW_SYSTEM
			m_bDraw = true;
			m_DrawStart0 = (*pPosA);
			m_DrawEnd0 = (*pPosB);
#endif
		}
		break;

	case kConstraintRuleEqual:
		if (M_Fabs(diff) != 1.0f)
		{
			(*pPosA) -= separation * (diff - 1.0f) * m_MassFactorAB;
			(*pPosB) += separation * (diff - 1.0f) * (1.0f - m_MassFactorAB);
#ifdef _DRAW_SYSTEM
			m_bDraw = true;
			m_DrawStart0 = (*pPosA);
			m_DrawEnd0 = (*pPosB);
#endif
		}
		break;

	case kConstraintRuleSmaller:
		if (M_Fabs(diff) > 1.0f)
		{
			(*pPosA) -= separation * (diff - 1.0f) * m_MassFactorAB;
			(*pPosB) += separation * (diff - 1.0f) * (1.0f - m_MassFactorAB);
#ifdef _DRAW_SYSTEM
			m_bDraw = true;
			m_DrawStart0 = (*pPosA);
			m_DrawEnd0 = (*pPosB);
#endif
		}
		break;

	case kConstraintRuleLargerSoft:
		if (M_Fabs(diff) < 1.0f)
		{
			(*pPosA) -= separation * (diff - 1.0f) * m_MassFactorAB;
			(*pPosB) += separation * (diff - 1.0f) * (1.0f - m_MassFactorAB);
#ifdef _DRAW_SYSTEM
			m_bDraw = true;
			m_DrawStart0 = (*pPosA);
			m_DrawEnd0 = (*pPosB);
#endif
		}
		break;

	case kConstraintRuleEqualSoft:
		if (M_Fabs(diff) != 1.0f)
		{
			(*pPosA) -= separation * (diff - 1.0f) * m_MassFactorAB;
			(*pPosB) += separation * (diff - 1.0f) * (1.0f - m_MassFactorAB);
#ifdef _DRAW_SYSTEM
			m_bDraw = true;
			m_DrawStart0 = (*pPosA);
			m_DrawEnd0 = (*pPosB);
#endif
		}
		break;

	case kConstraintRuleSmallerSoft:
		if (M_Fabs(diff) > 1.0f)
		{
			(*pPosA) -= separation * (diff - 1.0f) * m_MassFactorAB;
			(*pPosB) += separation * (diff - 1.0f) * (1.0f - m_MassFactorAB);
#ifdef _DRAW_SYSTEM
			m_bDraw = true;
			m_DrawStart0 = (*pPosA);
			m_DrawEnd0 = (*pPosB);
#endif
		}
		break;

	case kConstraintRuleIgnore:
		break;

	default:
		break;
	}

	if (m_bForce)
	{	// We move pPosB to strictly enforce the rule.
		separation = (*pPosB) - (*pPosA);
		separation.Normalize();
		*pPosB = (*pPosA) + separation * m_Length;
	}

	return(kSuccess);
}

void CConstraintRule::Draw(fp32 _Duration,const CVec3Dfp32& _Offset)
{
#ifdef _DRAW_SYSTEM
	if ((!m_bDraw)||(!m_pCS))
	{
		return;
	}

	uint32 color;
	if (m_bBone)
	{
		color = kColorWhite;
	}
	else if (m_Rule == kConstraintRuleEqual)
	{
		color = kColorGreen;
	}
	else
	{
		color = kColorRed;
	}

	if ((m_pCS->m_State == CConstraintSystem::STOPPED)&&(m_Rule == kConstraintRuleEqual))
	{
		color = kColorDkPurple;
	}

	m_pCS->m_pWPhysState->Debug_RenderWire(m_DrawStart0+_Offset,m_DrawEnd0+_Offset,color,_Duration,true);

#endif	// End of #ifdef _DRAW_SYSTEM
}

// =============================================================================================
// CConstraintSubPosRule
// =============================================================================================
CConstraintSubPosRule::CConstraintSubPosRule(CConstraintSystem* _pSystem)
: CConstraintRule(_pSystem)
{
	m_iPt0 = -1;
	m_Fract = 0.5f;	// Default is midway between m_iPB and m_iPC
}

int16 CConstraintSubPosRule::Setup(int16 _iP0,
		int16 _iPA,
		int16 _iPB,
		fp32 _Stiffness,
		fp32 _Fract)
{
	if (!m_pCS) {return(kErrState);}
	if (_iPA == _iPB) {return(kErrParams);}
	if ((_iP0 == _iPA)||(_iP0 == _iPB)) {return(kErrParams);}
	if ((_iP0 < 0)||(_iP0 >= m_pCS->m_lpPts.Len())||
		(_iPA < 0)||(_iPA >= m_pCS->m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_pCS->m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}

	if ((_Fract <= 0.0f)||(_Fract >= 1.0f))
	{
		return(kErrParams);
	}

	if ((_Stiffness < 0.0f)||(_Stiffness > 1.0f))
	{
		return(kErrParams);
	}

	if ((m_pCS->m_lpPts[_iPA]->m_InvMass < kEpsilon)&&(m_pCS->m_lpPts[_iPB]->m_InvMass < kEpsilon))
	{
		return(kErrParams);
	}
	m_Rule = kConstraintRuleEqual;	// Not really needed
	m_iPt0 = _iP0;
	m_iPA = _iPA;
	m_iPB = _iPB;
	m_Fract = _Fract;	// Fraction from m_iPA to m_iPB

	// *** Dunno, what to do here actually ***
	int16 rErr = ModifyMass();
	ERR_CHECK(rErr);

	return(kSuccess);
}

int16 CConstraintSubPosRule::Constrain() const
{
	MSCOPESHORT( CConstraintSubPosRule::Constrain );
	if (!m_pCS->m_bCoordSysAvailable) {return(kErrState);}

	// Very simple:
	// We just put m_iPt0 at m_Fract between m_iPA and m_iPB
	// Is the system coordinates valid?

#ifdef _DRAW_SYSTEM
	m_bDraw = false;
#endif
	CVec3Dfp32 vAB = m_pCS->m_lpPts[m_iPB]->m_Pos - m_pCS->m_lpPts[m_iPA]->m_Pos;
	m_pCS->m_lpPts[m_iPt0]->m_Pos = m_pCS->m_lpPts[m_iPA]->m_Pos + vAB * m_Fract;

	return(kSuccess);
}

// =============================================================================================
// CConstraintAngleRule
// =============================================================================================
CConstraintAngleRule::CConstraintAngleRule(CConstraintSystem* _pSystem)
: CConstraintRule(_pSystem)
{
	m_iPt0 = -1;
}


int16 CConstraintAngleRule::Setup(int16 _iP0,
				int16 _iPA,
				int16 _iPB,
				int16 _Rule,
				fp32 _Stiffness,
				fp32 _Angle)
{
	if ((_iP0 == _iPA)||(_iP0 == _iPB)) {return(kErrParams);}
	int16 rErr = CConstraintRule::Setup(_iPA,_iPB,_Rule,_Stiffness);
	ERR_CHECK(rErr);

	if ((_iP0 < 0)||(_iP0 >= m_pCS->m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}

	// NOTE: We do not care what the mass of _iP0 is
	// The rule will only adjust _iPA and _iPB so the mass of _iP0 is unimportant
	m_iPt0 = _iP0;

	fp32 lengthOA = (m_pCS->m_lpPts[m_iPA]->m_Pos - m_pCS->m_lpPts[m_iPt0]->m_Pos).Length();
	if (lengthOA < kEpsilon)
	{
		return(kErrParams);
	}
	fp32 lengthOB = (m_pCS->m_lpPts[m_iPB]->m_Pos - m_pCS->m_lpPts[m_iPt0]->m_Pos).Length();
	if (lengthOB < kEpsilon)
	{
		return(kErrParams);
	}

	fp32 angleRadians = _Angle * kDeg2Rad;
	m_InvLength = M_InvSqrt(Sqr(lengthOA)+Sqr(lengthOB) - 2*lengthOA*lengthOB*M_Cos(angleRadians));
	m_Length = 1.0f / m_InvLength;
		
	return(kSuccess);
}

int16 CConstraintAngleRule::Constrain() const
{
	MSCOPESHORT( CConstraintAngleRule::Constrain );
	int16 rErr = CConstraintRule::Constrain();
	ERR_CHECK(rErr);

	return(kSuccess);
}

// =============================================================================================
// CConstraintKneeRule
// =============================================================================================
// A rule that checks wether _iP0->_iPA cross _iPA->_iPB points to the right or left and moves _iPA to adhere
// This move is done without velocity to avoid introducing rotation velocity (ugly, but it works)
CConstraintKneeRule::CConstraintKneeRule(CConstraintSystem* _pSystem)
: CConstraintRule(_pSystem)
{
	m_bLeft = true;
	m_iPt0 = -1;
	m_iPt1 = -1;
}


int16 CConstraintKneeRule::Setup(int16 _iP0,
								 int16 _iP1,
								 int16 _iPA,
								 int16 _iPB,
								 bool _bLeft,
								 fp32 _Stiffness)
{
	if ((_iP0 < 0)||(_iP0 >= m_pCS->m_lpPts.Len())||
		(_iP1 < 0)||(_iP1 >= m_pCS->m_lpPts.Len())||
		(_iPA < 0)||(_iPA >= m_pCS->m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_pCS->m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}
	if ((_iP0 == _iP1)||(_iP0 == _iPA)||(_iP0 == _iPB)||(_iP1 == _iPA)||(_iP1 == _iPB)||(_iPA == _iPB))
	{
		return(kErrParams);
	}
	if ((m_pCS->m_lpPts[_iP0] == NULL)||
		(m_pCS->m_lpPts[_iP1] == NULL)||
		(m_pCS->m_lpPts[_iPA] == NULL)||
		(m_pCS->m_lpPts[_iPB] == NULL))
	{
		return(kErrNull);
	}

	if ((_Stiffness < 0)||(_Stiffness > 1.0f))
	{
		return(kErrParams);
	}

	m_Stiffness = _Stiffness;

	m_bLeft = _bLeft;
	m_iPt0 = _iP0;
	m_iPt1 = _iP1;
	m_iPA = _iPA;
	m_iPB = _iPB;

	int16 rErr = ModifyMass();
	ERR_CHECK(rErr);

	return(kSuccess);
}


int16 CConstraintKneeRule::Constrain() const
{	
	MSCOPESHORT( CConstraintKneeRule::Constrain );
#ifdef _DRAW_SYSTEM
	m_bDraw = false;
#endif
	const CVec3Dfp32& LHip = m_pCS->m_lpPts[m_iPt0]->m_Pos;
	const CVec3Dfp32& RHip = m_pCS->m_lpPts[m_iPt1]->m_Pos;
	const CVec3Dfp32& RKnee = m_pCS->m_lpPts[m_iPA]->m_Pos;
	const CVec3Dfp32& RFoot = m_pCS->m_lpPts[m_iPB]->m_Pos;
	const CVec3Dfp32 Hip = RHip-LHip;
	const CVec3Dfp32 RLeg = RKnee-RHip;
	const CVec3Dfp32 RShin = RFoot-RKnee;
	
	CVec3Dfp32 N;
	fp32 height;

	// Hip and leg create a plane with N as it forwards pointing normal
	// If the foor lies above this plane it will be pushed down
	if (M_Fabs(RLeg * Hip) >= 0.1f)
	{
		Hip.CrossProd(RLeg,N);
		N.Normalize();
		if (m_bLeft)
		{
			N *= -1.0f;
		}
		height = N * (RFoot - RHip);
		if (height > 0.0f)
		{
	#ifdef _DRAW_SYSTEM
			m_bDraw = true;
			m_DrawStart0 = m_pCS->m_lpPts[m_iPt1]->m_Pos;
			m_DrawEnd0 = m_pCS->m_lpPts[m_iPB]->m_Pos;
	#endif
			CVec3Dfp32 Move = N * height;
			CVec3Dfp32 move = Move * m_Stiffness;
			m_pCS->m_lpPts[m_iPt1]->m_Pos -= move * 0.25f;	// RHip down
			m_pCS->m_lpPts[m_iPA]->m_Pos += move * 0.5f;	// RKnee up
			m_pCS->m_lpPts[m_iPB]->m_Pos -= move * 0.25f;	// RFoot down
		}
	}

	// We create a plane using Fwd and RLeg (or Up and RLeg if they are too parallell).
	// Then we measure how much to either side RFoot lies use this offset to straighten the leg
	// Straighten by moving RFoot 25%, RKNee -50%, and RHip 25% (to minimise rot momentum generation)
	if (true)
	{
		const CVec3Dfp32& Fwd = CVec3Dfp32::GetMatrixRow(m_pCS->m_SystemMat,0);
		const CVec3Dfp32& Up = CVec3Dfp32::GetMatrixRow(m_pCS->m_SystemMat,2);
		RLeg.CrossProd(Fwd,N);
		if (M_Fabs(N.LengthSqr()) < 0.1f)
		{
			RLeg.CrossProd(Up,N);
		}
		N.Normalize();
		height = N * (RFoot - RHip);
		CVec3Dfp32 Move = N * height;
		CVec3Dfp32 move = Move * m_Stiffness;
		m_pCS->m_lpPts[m_iPt1]->m_Pos -= move * 0.25f;	// RHip
		m_pCS->m_lpPts[m_iPA]->m_Pos += move * 0.5f;	// RKnee	
		m_pCS->m_lpPts[m_iPB]->m_Pos -= move * 0.25f;	// RFoot
#ifdef _DRAW_SYSTEM
		// m_DrawEnd0 = m_pCS->m_lpPts[m_iPA]->m_Pos;
		// m_DrawEnd1 = m_pCS->m_lpPts[m_iPB]->m_Pos;
#endif
	}

	return(kSuccess);
}

void CConstraintKneeRule::Draw(fp32 _Duration,const CVec3Dfp32& _Offset)
{
#ifdef _DRAW_SYSTEM
	if (m_bDraw)
	{
		uint32 color = kColorRed;
		m_pCS->m_pWPhysState->Debug_RenderWire(m_DrawStart0+_Offset,m_DrawEnd0+_Offset,color,_Duration,true);
		// m_pCS->m_pWPhysState->Debug_RenderWire(m_DrawStart1+_Offset,m_DrawEnd1+_Offset,color,_Duration,true);
	}
#endif
}

// =============================================================================================
// CConstraintBodyPlaneRule
// =============================================================================================
// Sets up a plane of m_iPt0->m_iPt1 and m_iPt0->m_iPA
// If _iPB is below _Distance it will be moved _Stiffness of the distance
// _bLeft determine wether the plane should be reversed
CConstraintBodyPlaneRule::CConstraintBodyPlaneRule(CConstraintSystem* _pSystem)
: CConstraintRule(_pSystem)
{
	m_Distance = 0.0f;
	m_bLeft = true;
	m_iPt0 = -1;
	m_iPt1 = -1;
};

int16 CConstraintBodyPlaneRule::Setup(int16 _iP0,
		int16 _iP1,
		int16 _iPA,
		int16 _iPB,
		bool _bLeft,
		fp32 _Distance,
		fp32 _Stiffness)
{
	if ((_iP0 < 0)||(_iP0 >= m_pCS->m_lpPts.Len())||
		(_iP1 < 0)||(_iP1 >= m_pCS->m_lpPts.Len())||
		(_iPA < 0)||(_iPA >= m_pCS->m_lpPts.Len())||
		(_iPB < 0)||(_iPB >= m_pCS->m_lpPts.Len()))
	{
		return(kErrIndexOutOfRange);
	}
	if ((_iP0 == _iP1)||(_iPA == _iPB))
	{
		return(kErrParams);
	}
	if ((m_pCS->m_lpPts[_iP0] == NULL)||
		(m_pCS->m_lpPts[_iP1] == NULL)||
		(m_pCS->m_lpPts[_iPA] == NULL)||
		(m_pCS->m_lpPts[_iPB] == NULL))
	{
		return(kErrNull);
	}

	if ((_Stiffness < 0)||(_Stiffness > 1.0f))
	{
		return(kErrParams);
	}

	m_Distance = _Distance;
	m_Stiffness = _Stiffness;

	m_bLeft = _bLeft;
	m_iPt0 = _iP0;
	m_iPt1 = _iP1;
	m_iPA = _iPA;
	m_iPB = _iPB;

	int16 rErr = ModifyMass();
	ERR_CHECK(rErr);

	return(kSuccess);
};

int16 CConstraintBodyPlaneRule::Constrain() const
{
	MSCOPESHORT( CConstraintBodyPlaneRule::Constrain );
#ifdef _DRAW_SYSTEM
	m_bDraw = false;
#endif
	const CVec3Dfp32& v0 = m_pCS->m_lpPts[m_iPt0]->m_Pos;	// Pelvis
	const CVec3Dfp32& v1 = m_pCS->m_lpPts[m_iPt1]->m_Pos;	// Leg (hipjoint)
	const CVec3Dfp32& vA = m_pCS->m_lpPts[m_iPA]->m_Pos;		// Knee
	const CVec3Dfp32& vB = m_pCS->m_lpPts[m_iPB]->m_Pos;		// Foot
	const CVec3Dfp32 v01 = v1-v0;
	const CVec3Dfp32 v0A = vA-v0;

	CVec3Dfp32 N = v01;
	N.Normalize();
	if (m_bLeft)
	{
		N *= -1.0;
	}

	fp32 height = N * (vA - v0);
	if (height < m_Distance)
	{
#ifdef _DRAW_SYSTEM
		m_bDraw = true;
		m_DrawStart0 = m_pCS->m_lpPts[m_iPA]->m_Pos;
#endif
		CVec3Dfp32 Move = N * (m_Distance - height);
		CVec3Dfp32 move = Move * m_Stiffness;
		m_pCS->m_lpPts[m_iPA]->m_Pos += move * m_MassFactorAB;
		m_pCS->m_lpPts[m_iPt0]->m_Pos -= move * (1.0f - m_MassFactorAB);
#ifdef _DRAW_SYSTEM
		m_DrawEnd0 = m_pCS->m_lpPts[m_iPA]->m_Pos;
#endif
	}

	height = N * (vB - v0);
	if (height < m_Distance)
	{
		CVec3Dfp32 Move = N * (m_Distance - height);
		CVec3Dfp32 move = Move * m_Stiffness;
		m_pCS->m_lpPts[m_iPB]->m_Pos += move * m_MassFactorAB;
		m_pCS->m_lpPts[m_iPt0]->m_Pos -= move * (1.0f - m_MassFactorAB);
	}

	return(kSuccess);
};

void CConstraintBodyPlaneRule::Draw(fp32 _Duration,const CVec3Dfp32& _Offset)
{
#ifdef _DRAW_SYSTEM
	if (m_bDraw)
	{
		uint32 color = kColorGreen;
		m_pCS->m_pWPhysState->Debug_RenderWire(m_DrawStart0+_Offset,m_DrawEnd0+_Offset,color,1.0f,true);
	}
#endif
};

// =============================================================================================
// CConstraintPlaneRule
// =============================================================================================
CConstraintPlaneRule::CConstraintPlaneRule(CConstraintSystem* _pSystem)
: CConstraintRule(_pSystem)
{
	m_Distance = 0.0f;

	m_iPt0 = -1;
	m_iPt1 = -1;
	m_iPt2 = -1;
	m_iPt3 = -1;
	m_i0 = -1;
	m_i1 = -1;
	m_i2 = -1;
	m_i3 = -1;

	m_MassFactor0 = 0.5f;
	m_MassFactor1 = 0.5f;
	m_MassFactor2 = 0.5f;
	m_MassFactor3 = 0.5f;
};

int16 CConstraintPlaneRule::Setup(int16 _iPt0,
			int16 _iPt1,
			int16 _iPt2,
			int16 _iPt3,
			int16 _i0,
			int16 _i1,
			int16 _i2,
			int16 _i3,
			fp32 _Distance,
			fp32 _Stiffness)
{
	if ((_iPt0 == _i0)||(_iPt1 == _i1)||(_iPt2 == _i2)||(_iPt3 == _i3)||
		(_iPt0 == _iPt1)||(_iPt1 == _iPt2))
	{
		return(kErrParams);
	}

	m_Distance = _Distance;
	m_Stiffness = _Stiffness;

	m_iPt0 = _iPt0;
	m_iPt1 = _iPt1;
	m_iPt2 = _iPt2;
	m_iPt3 = _iPt3;
	m_i0 = _i0;
	m_i1 = _i1;
	m_i2 = _i2;
	m_i3 = _i3;

	/*
	int16 rErr = ModifyMass();
	ERR_CHECK(rErr);
	*/

	return(kSuccess);
};

int16 CConstraintPlaneRule::ModifyMass()
{
	fp32 invMassA,invMassB;

	invMassA = (m_pCS->m_lpPts[m_iPt0]->m_InvMass);
	invMassB = (m_pCS->m_lpPts[m_i0]->m_InvMass);
	if ((invMassA < kEpsilon)&&(invMassB < kEpsilon))
	{
		return(kErrParams);
	}
	if (invMassA >= invMassB)
	{
		m_MassFactor0 = 1 / (invMassB / invMassA + 1.0f);
	}
	else
	{
		m_MassFactor0 = 1.0f - 1 / (invMassA / invMassB + 1.0f);
	}

	invMassA = (m_pCS->m_lpPts[m_iPt1]->m_InvMass);
	invMassB = (m_pCS->m_lpPts[m_i1]->m_InvMass);
	if ((invMassA < kEpsilon)&&(invMassB < kEpsilon))
	{
		return(kErrParams);
	}
	if (invMassA >= invMassB)
	{
		m_MassFactor1 = 1 / (invMassB / invMassA + 1.0f);
	}
	else
	{
		m_MassFactor1 = 1.0f - 1 / (invMassA / invMassB + 1.0f);
	}

	invMassA = (m_pCS->m_lpPts[m_iPt2]->m_InvMass);
	invMassB = (m_pCS->m_lpPts[m_i2]->m_InvMass);
	if ((invMassA < kEpsilon)&&(invMassB < kEpsilon))
	{
		return(kErrParams);
	}
	if (invMassA >= invMassB)
	{
		m_MassFactor2 = 1 / (invMassB / invMassA + 1.0f);
	}
	else
	{
		m_MassFactor2 = 1.0f - 1 / (invMassA / invMassB + 1.0f);
	}

	invMassA = (m_pCS->m_lpPts[m_iPt3]->m_InvMass);
	invMassB = (m_pCS->m_lpPts[m_i3]->m_InvMass);
	if ((invMassA < kEpsilon)&&(invMassB < kEpsilon))
	{
		return(kErrParams);
	}
	if (invMassA >= invMassB)
	{
		m_MassFactor3 = 1 / (invMassB / invMassA + 1.0f);
	}
	else
	{
		m_MassFactor3 = 1.0f - 1 / (invMassA / invMassB + 1.0f);
	}

	return(kSuccess);
};

int16 CConstraintPlaneRule::Constrain() const
{
	MSCOPESHORT(CConstraintPlaneRule::Constrain);

#ifdef _DRAW_SYSTEM
	m_bDraw = false;
#endif
	const CVec3Dfp32& vPt0 = m_pCS->m_lpPts[m_iPt0]->m_Pos;
	const CVec3Dfp32& vPt1 = m_pCS->m_lpPts[m_iPt1]->m_Pos;
	const CVec3Dfp32& vPt2 = m_pCS->m_lpPts[m_iPt2]->m_Pos;
//	const CVec3Dfp32& vPt3 = m_pCS->m_lpPts[m_iPt3]->m_Pos;
	const CVec3Dfp32 vPt01 = vPt1-vPt0;
	const CVec3Dfp32 vPt02 = vPt2-vPt0;

	const CVec3Dfp32& v0 = m_pCS->m_lpPts[m_i0]->m_Pos;
	const CVec3Dfp32& v1 = m_pCS->m_lpPts[m_i1]->m_Pos;
	const CVec3Dfp32& v2 = m_pCS->m_lpPts[m_i2]->m_Pos;
	const CVec3Dfp32& v3 = m_pCS->m_lpPts[m_i3]->m_Pos;

	CVec3Dfp32 N;
	vPt01.CrossProd(vPt02,N);
	N.Normalize();

	CVec3Dfp32 P,V,Move;
	fp32 depth;
	depth = N * (v0 - vPt0);
	if (depth < m_Distance)
	{
		P = m_pCS->m_lpPts[m_iPt0]->m_Pos;
		V = m_pCS->m_lpPts[m_i0]->m_Pos;

		Move = (depth - m_Distance) * m_MassFactor0 * m_Stiffness;
		m_pCS->m_lpPts[m_iPt0]->m_Pos += Move;
		m_pCS->m_lpPts[m_iPt0]->m_PrevPos += Move;		// No inertia
		Move = (depth - m_Distance) * (1.0f - m_MassFactor0) * m_Stiffness;
		m_pCS->m_lpPts[m_i0]->m_Pos -= Move;
		m_pCS->m_lpPts[m_i0]->m_PrevPos -= Move;		// No inertia

		m_pCS->m_pWPhysState->Debug_RenderWire(P,m_pCS->m_lpPts[m_iPt0]->m_Pos,kColorRed,1.0f,true);
		m_pCS->m_pWPhysState->Debug_RenderWire(V,m_pCS->m_lpPts[m_i0]->m_Pos,kColorRed,1.0f,true);
	}

	depth = N * (v1 - vPt0);
	if (depth < m_Distance)
	{
		P = m_pCS->m_lpPts[m_iPt1]->m_Pos;
		V = m_pCS->m_lpPts[m_i1]->m_Pos;

		Move = (depth - m_Distance) * m_MassFactor1 * m_Stiffness;
		m_pCS->m_lpPts[m_iPt1]->m_Pos += Move;
		m_pCS->m_lpPts[m_iPt1]->m_PrevPos += Move;		// No inertia
		Move = (depth - m_Distance) * (1.0f - m_MassFactor1) * m_Stiffness;
		m_pCS->m_lpPts[m_i1]->m_Pos -= Move;
		m_pCS->m_lpPts[m_i1]->m_PrevPos -= Move;		// No inertia

		m_pCS->m_pWPhysState->Debug_RenderWire(P,m_pCS->m_lpPts[m_iPt1]->m_Pos,kColorRed,1.0f,true);
		m_pCS->m_pWPhysState->Debug_RenderWire(V,m_pCS->m_lpPts[m_i1]->m_Pos,kColorRed,1.0f,true);
	}

	depth = N * (v2 - vPt0);
	if (depth < m_Distance)
	{
		P = m_pCS->m_lpPts[m_iPt2]->m_Pos;
		V = m_pCS->m_lpPts[m_i2]->m_Pos;

		Move = (depth - m_Distance) * m_MassFactor2 * m_Stiffness;
		m_pCS->m_lpPts[m_iPt2]->m_Pos += Move;
		m_pCS->m_lpPts[m_iPt2]->m_PrevPos += Move;		// No inertia
		Move = (depth - m_Distance) * (1.0f - m_MassFactor2) * m_Stiffness;
		m_pCS->m_lpPts[m_i2]->m_Pos -= Move;
		m_pCS->m_lpPts[m_i2]->m_PrevPos -= Move;		// No inertia

		m_pCS->m_pWPhysState->Debug_RenderWire(P,m_pCS->m_lpPts[m_iPt2]->m_Pos,kColorRed,1.0f,true);
		m_pCS->m_pWPhysState->Debug_RenderWire(V,m_pCS->m_lpPts[m_i2]->m_Pos,kColorRed,1.0f,true);
	}

	depth = N * (v3 - vPt0);
	if (depth < m_Distance)
	{
		P = m_pCS->m_lpPts[m_iPt3]->m_Pos;
		V = m_pCS->m_lpPts[m_i3]->m_Pos;

		Move = (depth - m_Distance) * m_MassFactor3 * m_Stiffness;
		m_pCS->m_lpPts[m_iPt3]->m_Pos += Move;
		m_pCS->m_lpPts[m_iPt3]->m_PrevPos += Move;		// No inertia
		Move = (depth - m_Distance) * (1.0f - m_MassFactor3) * m_Stiffness;
		m_pCS->m_lpPts[m_i3]->m_Pos -= Move;
		m_pCS->m_lpPts[m_i3]->m_PrevPos -= Move;		// No inertia

		m_pCS->m_pWPhysState->Debug_RenderWire(P,m_pCS->m_lpPts[m_iPt3]->m_Pos,kColorRed,1.0f,true);
		m_pCS->m_pWPhysState->Debug_RenderWire(V,m_pCS->m_lpPts[m_i3]->m_Pos,kColorRed,1.0f,true);
	}

	return(kSuccess);
};

void CConstraintPlaneRule::Draw(fp32 _Duration,const CVec3Dfp32& _Offset)
{
};


// =============================================================================================
// CConstraintDirAngleRule
// =============================================================================================
// A rule that enforces iPA->iPB to be within an angle of left,right,up,down,back,front
CConstraintDirAngleRule::CConstraintDirAngleRule(CConstraintSystem* _pSystem)
: CConstraintRule(_pSystem)
{
	m_Dir = FORWARD;
	m_Cos = 1.0f;
	m_Sin = 0.0f;
}

// Angle must be between 0 and 90 degrees, if you want more, use the reverse direction and supply (180-angle)
int16 CConstraintDirAngleRule::Setup(int16 _iPA,
										int16 _iPB,
										fp32 _Stiffness,
										int _Direction,
										fp32 _Angle)
{
	int16 rErr = CConstraintRule::Setup(_iPA,_iPB,kConstraintRuleSmallerSoft,_Stiffness);
	if (_iPA == _iPB) {return(kErrParams);}

	if ((_Direction != FORWARD)&&
		(_Direction != BACKWARD)&&
		(_Direction != RIGHT)&&
		(_Direction != LEFT)&&
		(_Direction != UP)&&
		(_Direction != DOWN))
	{
		return(kErrParams);
	}
	
	if ((_Angle < 0.0f)||(_Angle > 90.0f))
	{
		return(kErrParams);
	}

	m_Dir = _Direction;
	m_Cos = M_Cos(_Angle * kDeg2Rad);
	m_Sin = M_Sin(_Angle * kDeg2Rad);

	rErr = ModifyMass();
	ERR_CHECK(rErr);

	return(kSuccess);
}


int16 CConstraintDirAngleRule::Constrain() const
{
	MSCOPESHORT( CConstraintDirAngleRule::Constrain );
	// Is the system coordinates valid?
	if (!m_pCS->m_bCoordSysAvailable) {return(kErrState);}

	fp32 Dot = 0;
	CVec3Dfp32 offset;	// offset is the move needed to align perfect with 0 angle
	const CVec3Dfp32& AB = m_pCS->m_lpPts[m_iPB]->m_Pos - m_pCS->m_lpPts[m_iPA]->m_Pos;
	const CVec3Dfp32& Fwd = CVec3Dfp32::GetRow(m_pCS->m_SystemMat,0);
	const CVec3Dfp32& Left = CVec3Dfp32::GetRow(m_pCS->m_SystemMat,1);
	const CVec3Dfp32& Up = CVec3Dfp32::GetRow(m_pCS->m_SystemMat,2);
	
	switch(m_Dir)
	{
	case FORWARD:
		Dot = AB * Fwd * m_InvLength;
		offset = Fwd * m_Length - AB;
		break;

	case BACKWARD:
		Dot = -AB * Fwd * m_InvLength;
		offset = -Fwd * m_Length - AB;
		break;

	case RIGHT:
		Dot = -AB * Left * m_InvLength;
		offset = -Left * m_Length - AB;
		break;

	case LEFT:
		Dot = AB * Left * m_InvLength;
		offset = Left * m_Length - AB;
		break;

	case UP:
		Dot = AB * Up * m_InvLength;
		offset = Up * m_Length - AB;
		break;

	case DOWN:
		Dot = -AB * Up * m_InvLength;
		offset = -Up * m_Length - AB;
		break;
	}

#ifdef _DRAW_SYSTEM
	m_bDraw = false;
#endif
	if (Dot < m_Cos)
	{
		fp32 factor = m_Cos - Dot;
		if (factor > 1.0f) {factor = 1.0f;}
#ifdef _DRAW_SYSTEM
		m_bDraw = true;
		m_DrawStart0 = m_pCS->m_lpPts[m_iPA]->m_Pos;
#endif
		CVec3Dfp32 moveB = offset * factor * m_Stiffness;
		// We assume m_iPA->m_iPB already has an IK constraint so we try to keep the distance right
		CVec3Dfp32 A = m_pCS->m_lpPts[m_iPA]->m_Pos;
		CVec3Dfp32 B = m_pCS->m_lpPts[m_iPB]->m_Pos + moveB;
		CVec3Dfp32 AB = (B-A).Normalize();
		B = A + (AB * m_Length);
		moveB = B - m_pCS->m_lpPts[m_iPB]->m_Pos;
		m_pCS->m_lpPts[m_iPB]->m_Pos += moveB;
		m_pCS->m_lpPts[m_iPA]->m_Pos -= moveB;
		//m_pCS->m_lpPts[m_iPB]->m_PrevPos += moveB;	// No inertia
		//m_pCS->m_lpPts[m_iPA]->m_PrevPos -= moveB;	// No inertia
#ifdef _DRAW_SYSTEM
		m_DrawEnd0 = m_pCS->m_lpPts[m_iPA]->m_Pos;
#endif
	}

	return(kSuccess);
}

void CConstraintDirAngleRule::Draw(fp32 _Duration,const CVec3Dfp32& _Offset)
{
#ifdef _DRAW_SYSTEM
	if (m_bDraw)
	{
		uint32 color = kColorRed;
		m_pCS->m_pWPhysState->Debug_RenderWire(m_DrawStart0+_Offset,m_DrawEnd0+_Offset,color,_Duration,true);
	}
#endif
}

// =============================================================================================
// CConstraintDirAngleRule2
// =============================================================================================
// A rule that enforces iPA->iPB to be within an angle from _iPA and _iRef
// If _iRef2 is supplied a 'virtual' counter will be used at the midpoint between _iRef and _iRef
// actual displacement will then be split and applied to both _iRef and _iRef
CConstraintDirAngleRule2::CConstraintDirAngleRule2(CConstraintSystem* _pSystem)
: CConstraintRule(_pSystem)
{
	m_Cos = 1.0f;
	m_Sin = 0.0f;
	m_iRef = -1;
	m_iRef2= -1;	// If m__iRef2 > -1 means we use a 'virtual' counter midway between m_iRef and m__iRef2
}

int16 CConstraintDirAngleRule2::Setup(int16 _iPA,
			int16 _iPB,
			fp32 _Stiffness,
			int16 _iRef,
			int16 _iRef2,
			fp32 _Angle)
{
	int16 rErr = CConstraintRule::Setup(_iPA,_iPB,kConstraintRuleSmallerSoft,_Stiffness);
	ERR_CHECK(rErr);

	if (_iPA == _iPB) {return(kErrParams);}
	if (_iRef == _iRef2) {return(kErrParams);}
	if ((_Angle < 0.0f)||(_Angle > 90.0f))
	{
		return(kErrParams);
	}

	m_Cos = M_Cos(_Angle * kDeg2Rad);
	m_Sin = M_Sin(_Angle * kDeg2Rad);

	m_iRef = _iRef;
	m_iRef2 = _iRef2;

	m_Stiffness = _Stiffness;
	
	return(kSuccess);
};

int16 CConstraintDirAngleRule2::Constrain() const
{
	int16 rErr = kSuccess;

	CVec3Dfp32 A = m_pCS->m_lpPts[m_iPA]->m_Pos;
	CVec3Dfp32 B = m_pCS->m_lpPts[m_iPB]->m_Pos;
	CVec3Dfp32 Cur = (B-A).Normalize();
	CVec3Dfp32 Ref,Nor;
	if (m_iRef2 == -1)
	{	// m_iRef is real
		Ref = m_pCS->m_lpPts[m_iRef]->m_Pos;
	}
	else
	{	// Use virtual reference
		Ref = (m_pCS->m_lpPts[m_iRef]->m_Pos + m_pCS->m_lpPts[m_iRef2]->m_Pos) * 0.5f;
	}
	Nor = (A-Ref).Normalize();
	fp32 Dot = Cur * Nor;
	if (Dot < m_Cos)
	{	// Construct the position of B that would give us Cur * Nor == m_Cos
		CVec3Dfp32 Move = A + (Nor * m_Cos + (Cur - Nor) * m_Sin) - B;
#ifdef _DRAW_SYSTEM
		m_bDraw = true;
		m_DrawStart0 = Nor + A;
		m_DrawEnd0 = B;
#endif
		if (m_iRef2 == -1)
		{	// m_iRef is real
			m_pCS->m_lpPts[m_iPB]->m_Pos += Move * m_Stiffness;
		}
		else
		{	// Use virtual reference
			m_pCS->m_lpPts[m_iPB]->m_Pos += Move * m_Stiffness;
		}
	}

	return(rErr);
};

void CConstraintDirAngleRule2::Draw(fp32 _Duration,const CVec3Dfp32& _Offset)
{
#ifdef _DRAW_SYSTEM
	if ((!m_bDraw)||(!m_pCS))
	{
		return;
	}

	m_pCS->m_pWPhysState->Debug_RenderWire(m_DrawStart0+_Offset,m_DrawEnd0+_Offset,kColorRed,_Duration,true);
#endif
};

// =============================================================================================
// CConstraintCapsuleRule
// =============================================================================================

CConstraintCapsuleRule::CConstraintCapsuleRule(CConstraintSystem* _pSystem)
: CConstraintRule(_pSystem)
{
	m_Radius = 0.0f;

	m_iPt0 = -1;
	m_iPt1 = -1;
};

int16 CConstraintCapsuleRule::Setup(int16 _iPA,int16 _iPB,int16 _iPt0,int16 _iPt1,
			fp32 _Radius,fp32 _Stiffness)
{
	int16 rErr = CConstraintRule::Setup(_iPA,_iPB,kConstraintRuleLarger,_Stiffness);
	ERR_CHECK(rErr);
	if (_iPA == _iPB) {return(kErrParams);}
	if ((_iPt0 < 0)||
		(_Radius <= 0))
	{
		return(kErrParams);
	}
	m_iPt0 = _iPt0;
	m_iPt1 = _iPt1;
	m_Radius = _Radius;

	return(kSuccess);
};

int16 CConstraintCapsuleRule::ModifyMass()
{
	return(kErrNotImplemented);
};

// Measures the distance between 
int16 CConstraintCapsuleRule::Constrain() const
{
	CVec3Dfp32 M,B;
	fp32 t,rangeSqr,range;
	CVec3Dfp32 p,diff,dir,move;

	if ((m_iPt0 != -1)&&(m_pCS->m_lpPts[m_iPt0]))
	{
		B = m_pCS->m_lpPts[m_iPA]->m_Pos;
		M = (m_pCS->m_lpPts[m_iPB]->m_Pos - B).Normalize();

		p = m_pCS->m_lpPts[m_iPt0]->m_Pos;
		rangeSqr = CConstraintSystem::SqrDistanceToSegment(p,B,M,&t,&diff);
		if (rangeSqr < Sqr(m_Radius))
		{	// Constrain m_iPt0 to outside of capsule
			dir = diff;
			dir.Normalize();
			range = M_Sqrt(rangeSqr);
			move = dir * (m_Radius - range) * m_Stiffness;
			// Now, divide move among p0,A and B
			// p0 gets 50%, and A and by gets (1-t)*50% and t*50% respectively
			m_pCS->m_lpPts[m_iPt0]->m_Pos += move * 0.5f;
			m_pCS->m_lpPts[m_iPA]->m_Pos -= move * 0.5f * (1.0 - t);
			m_pCS->m_lpPts[m_iPB]->m_Pos -= move * 0.5f * t;
			// *** TEST ***
			B = m_pCS->m_lpPts[m_iPA]->m_Pos;
			M = (m_pCS->m_lpPts[m_iPB]->m_Pos - M).Normalize();

			p = m_pCS->m_lpPts[m_iPt0]->m_Pos;
			rangeSqr = CConstraintSystem::SqrDistanceToSegment(p,B,M,&t,&diff);
//			if (rangeSqr < Sqr(m_Radius))
//			{
//				fp32 delta = M_Sqrt(rangeSqr) - m_Radius;
//				bool wtf = true;
//			}
			// ***
		}
	}


	if ((m_iPt1 != -1)&&(m_pCS->m_lpPts[m_iPt1]))
	{
		M = m_pCS->m_lpPts[m_iPA]->m_Pos;
		B = (m_pCS->m_lpPts[m_iPB]->m_Pos - M).Normalize();

		p = m_pCS->m_lpPts[m_iPt1]->m_Pos;
		rangeSqr = CConstraintSystem::SqrDistanceToSegment(p,B,M,&t,&diff);
		if (rangeSqr < Sqr(m_Radius))
		{	// Constrain m_iPt0 to outside of capsule
			dir = diff;
			dir.Normalize();
			range = M_Sqrt(rangeSqr);
			move = dir * (m_Radius - range) * m_Stiffness;
			// Now, divide move among p1,A and B
			// p1 gets 50%, and A and by gets (1-t)*50% and t*50% respectively
			m_pCS->m_lpPts[m_iPt1]->m_Pos += move * 0.5f;
			m_pCS->m_lpPts[m_iPA]->m_Pos -= move * 0.5f * (1.0 - t);
			m_pCS->m_lpPts[m_iPB]->m_Pos -= move * 0.5f * t;

			// *** TEST ***
			M = m_pCS->m_lpPts[m_iPA]->m_Pos;
			B = m_pCS->m_lpPts[m_iPB]->m_Pos;

			p = m_pCS->m_lpPts[m_iPt1]->m_Pos;
			rangeSqr = CConstraintSystem::SqrDistanceToSegment(p,B,M,&t,&diff);
//			if (rangeSqr < Sqr(m_Radius))
//			{
//				fp32 delta = M_Sqrt(rangeSqr) - m_Radius;
//				bool wtf = true;
//			}
			// ***
		}
	}

	return(kErrNotImplemented);
};

void CConstraintCapsuleRule::Draw(fp32 _Duration,const CVec3Dfp32& _Offset)
{
};
