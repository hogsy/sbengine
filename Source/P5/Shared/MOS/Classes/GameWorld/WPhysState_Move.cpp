#include "PCH.h"
#include "WPhysState.h"
#include "../../Classes/GameWorld/WPhysState_Hash.h"
#include "../../XR/Phys/WPhysPCS.h"
#include "../../XR/Phys/WPhysPCS_Enable.h"


#ifdef _DEBUG
	#include "MFloat.h"
	#define DEBUG_CHECK_ROW(v, r)\
		M_ASSERT(!FloatIsInvalid((v).k[r][0]) && \
		         !FloatIsInvalid((v).k[r][1]) && \
		         !FloatIsInvalid((v).k[r][2]) && \
		         !FloatIsInvalid((v).k[r][3]) && \
		         (M_Fabs((v).k[r][0]) + M_Fabs((v).k[r][1]) + M_Fabs((v).k[r][2]) + M_Fabs((v).k[r][3]) < 1000000.0f), "Invalid vector!");
	#define DEBUG_CHECK_MATRIX(m)\
		DEBUG_CHECK_ROW(m, 0)\
		DEBUG_CHECK_ROW(m, 1)\
		DEBUG_CHECK_ROW(m, 2)\
		DEBUG_CHECK_ROW(m, 3)
	#define DEBUG_CHECK_VECTOR(v) \
		M_ASSERT(!FloatIsInvalid((v).k[0]) && \
		         !FloatIsInvalid((v).k[1]) && \
		         !FloatIsInvalid((v).k[2]) && \
		         (M_Fabs((v).k[0]) + M_Fabs((v).k[1]) + M_Fabs((v).k[2]) < 1000000.0f), "Invalid vector!");
	#define DEBUG_CHECK_AXISANGLE(q) \
		M_ASSERT(!FloatIsInvalid((q).m_Axis.k[0]) && \
		         !FloatIsInvalid((q).m_Axis.k[1]) && \
		         !FloatIsInvalid((q).m_Axis.k[2]) && \
		         !FloatIsInvalid((q).m_Angle) && \
		         (M_Fabs((q).m_Axis.k[0]) + M_Fabs((q).m_Axis.k[0]) + M_Fabs((q).m_Axis.k[0]) + M_Fabs((q).m_Angle) <= 1000000.0f), "Invalid axis-angle!");
#else
	#define DEBUG_CHECK_ROW(v, r)
	#define DEBUG_CHECK_MATRIX(m)
	#define DEBUG_CHECK_VECTOR(v)
	#define DEBUG_CHECK_AXISANGLE(q)
#endif

//#pragma optimize("", off)
//#pragma inline_depth(0)


// -------------------------------------------------------------------
// #define MOVEPHYS_STEUPDEBUG
// #define PHYS_DEBUG

#define MOVEPHYS_MAXMOVE 5

/*
	static void BreakPoint()
	{
		M_BREAKPOINT;
	}

	if (StartPos.Distance(pObj->GetPosition()) > 128.0f) BreakPoint();
*/

#ifdef	SERVER_STATS
#define MOVEPHYS_RETURN(Result)				\
{											\
	m_MovePhysRecurse--;					\
	TStop(Cycles);							\
	m_TFunc_MovePhysical += Cycles;			\
	Phys_MessageQueue_Flush();				\
	return (Result);						\
}
#else
#define MOVEPHYS_RETURN(Result)				\
{											\
	m_MovePhysRecurse--;					\
	Phys_MessageQueue_Flush();				\
	DEBUG_GS_BGCOLOR( 0x000000 );			\
	return (Result);						\
}
#endif

#define MOVEPHYS_LERPMAT(Src, Dst, Res, Frac) { Res = Dst; CVec3Dfp32::GetRow(Src, 3).Lerp(CVec3Dfp32::GetRow(Dst, 3), Frac, CVec3Dfp32::GetRow(Res, 3) ); };

#define MOVEPHYS_OPTIONALROTATE(bDoIt, Frac) \
if (bDoIt)	\
{	\
	DEBUG_CHECK_MATRIX(pObj->m_LocalPos);\
	CMat4Dfp32 Tmp, Tmp2;	\
	pObj->GetVelocityMatrix(Frac, Tmp);	\
	DEBUG_CHECK_MATRIX(Tmp);\
	pObj->m_LocalPos.Multiply3x3(Tmp, Tmp2);	\
	DEBUG_CHECK_MATRIX(Tmp2);\
	Tmp2.UnitNot3x3();	\
	CVec3Dfp32::GetRow(Tmp2, 3) = CVec3Dfp32::GetRow(pObj->m_LocalPos, 3);	\
	pObj->m_LocalPos = Tmp2;	\
	Phys_InsertPosition(_iObj, pObj);	\
}

//#define PhysOut(s) ConOut(s)
#define PhysOut(s) {}

// -------------------------------------------------------------------


static void ClampLow( CVec3Dfp32& _Value )
{
	if( M_Fabs(_Value.k[0]) < 0.000001f )
		_Value.k[0]	= 0.0f;
	if( M_Fabs(_Value.k[1]) < 0.000001f )
		_Value.k[1]	= 0.0f;
	if( M_Fabs(_Value.k[2]) < 0.000001f )
		_Value.k[2]	= 0.0f;
}

fp32 CWorld_PhysState::Object_ShortDeltaMove( CPotColSet *_pcs, int _iObj, const CVec3Dfp32& _dPos, int _nSlide)
{
	MAUTOSTRIP(CWorld_PhysState_Object_ShortDeltaMove_PCS, 0.0f);
	MSCOPESHORT(CWorld_PhysState::Object_ShortDeltaMove_PCS);
	// NOTE:	Moves object with no subdivision. Caller is responsible for not calling with invalid _dPos.
	//			Return value is how far along _dPos we managed to move. 
	//			(1.0f == Mission accomplished. 0 == Could not move at all.)

	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj) return 0;

	CCollisionInfo CInfo;

	int nMove = 0;
	int nSlidePlanes = 0;
	CPlane3Dfp32 SlidePlanes[16];

	CVec3Dfp32 Vel = _dPos;
	fp32 Time = 0.0f;

	for(; (Time < 1.0f) && (nMove < _nSlide+1); nMove++)
	{
//		ConOut(CStrF("Short: Begin %d, T %f, Vel %s", nMove, Time, (char*)Vel.GetString()));

		fp32 dTime = 1.0f - Time;
		if (dTime < 0.001f) break;

		CMat4Dfp32 DestPos(pObj->GetLocalPositionMatrix());
		CVec3Dfp32::GetRow(DestPos, 3) += Vel;

		CInfo.m_bIsCollision = false;
		CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_TIME | CXR_COLLISIONRETURNVALUE_POSITION);

		if (Phys_MovePosition(_pcs, _iObj, pObj->GetLocalPositionMatrix(), DestPos, &CInfo))
		{
//			ConOutD("Short: success 1");
			return 1.0f;
		}
//		ConOutD("Short: fail 1");

		if (CInfo.m_bIsValid)
		{
//			ConOutD("Short: CInfo valid");

			CInfo.m_Time = Max(0.0f, CInfo.m_Time - 0.01f / dTime);

			// Move to impact position
			int bImpactPos = (CInfo.m_Time < 0.0001f);
			if (!bImpactPos)
			{
				CMat4Dfp32 TestPos;
				MOVEPHYS_LERPMAT(pObj->GetLocalPositionMatrix(), DestPos, TestPos, CInfo.m_Time);
				if (Phys_MovePosition( _pcs, _iObj, pObj->GetLocalPositionMatrix(), TestPos, NULL ))
					bImpactPos = true;
			}

			// Fail?, Try to trace to impact position
			if (!bImpactPos)
			{
				CVec3Dfp32 dPos = CVec3Dfp32::GetRow(DestPos, 3) - pObj->GetPosition();
				fp32 T = Object_TraceDeltaMove( _pcs, _iObj, dPos);
				if (T > 0.0f)
				{
					bImpactPos = true;
					CInfo.m_Time = T;
				}
				else
				{
					// Fail?, Try move out of the impact plane and then try move again.
					CMat4Dfp32 TestPos(pObj->GetLocalPositionMatrix());
					CVec3Dfp32::GetRow(TestPos, 3) += CInfo.m_Plane.n;
					Phys_MovePosition( _pcs, _iObj, pObj->GetLocalPositionMatrix(), TestPos, NULL);
					continue;
				}

//				ConOutD(CStrF("Short: TraceDeltaMove => T %f, Move %d, nSlide %d", T, nMove, nSlidePlanes));
			}

			if (bImpactPos)
			{
//				ConOutD(CStrF("Short: success 2, %f, Plane %s", CInfo.m_Time, (char*)CInfo.m_Plane.n.GetString() ));

				Time += CInfo.m_Time * dTime;
//return Time;

				// Add the latest collision plane to the sliding planes.
				SlidePlanes[nSlidePlanes++] = CInfo.m_Plane;

				// Calculate new velocity from the sliding planes.
				switch(nSlidePlanes)
				{
				case 1 :
					{
						// One plane, project velocity onto the plane.
OnePlane:
						CVec3Dfp32 VNProj;
						Vel.Project(SlidePlanes[0].n, VNProj);
						Vel.Sub(VNProj, Vel);

//						ConOutD(CStrF("Short: Success 2, dT %f, %s", CInfo.m_Time, (char*)Vel.GetString()) );
					}
					break;

				case 2 :
					{
//TwoPlanes:
//						ConOutD(CStrF("Short: NPlanes %d", nSlidePlanes));
//						Object_ShortDeltaMove(_iSel, _iObj, SlidePlanes[1].n);

						// Two planes, project velocity onto the vector (SlideN[0] crossprod SlideN[1])
						CVec3Dfp32 Proj;
						SlidePlanes[0].n.CrossProd(SlidePlanes[1].n, Proj);

						if (Proj.LengthSqr() < 0.001f)
						{
							// Planes are (close to) parallell, fallback to OnePlane method, remove the first plane.
							SlidePlanes[0] = SlidePlanes[1];
							nSlidePlanes--;
							goto OnePlane;
						}
						else
						{
							CVec3Dfp32 VProj;
							Vel.Project(Proj, VProj);
							Vel = VProj;
						}
					}
					break;

				default :
					return Time;
				}

				// New velocity calculation done, move again..
				continue;
			}
//			ConOutD("Short: fail 2");
		}
		else
			ConOutD("Short: CInfo false");

		break;
	}

	return Time;
}




fp32 CWorld_PhysState::Object_ShortDeltaMove(const CSelection& _Selection, int _iObj, const CVec3Dfp32& _dPos, int _nSlide)
{
	MAUTOSTRIP(CWorld_PhysState_Object_ShortDeltaMove_NoPCS, 0.0f);
	MSCOPESHORT(CWorld_PhysState::Object_ShortDeltaMove_NoPCS);
	// NOTE:	Moves object with no subdivision. Caller is responsible for not calling with invalid _dPos.
	//			Return value is how far along _dPos we managed to move. 
	//			(1.0f == Mission accomplished. 0 == Could not move at all.)

	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj) return 0;

	CCollisionInfo CInfo;

	int nMove = 0;
	int nSlidePlanes = 0;
	CPlane3Dfp32 SlidePlanes[16];

	CVec3Dfp32 Vel = _dPos;
	fp32 Time = 0.0f;

	for(; (Time < 1.0f) && (nMove < _nSlide+1); nMove++)
	{
//		ConOut(CStrF("Short: Begin %d, T %f, Vel %s", nMove, Time, (char*)Vel.GetString()));

		fp32 dTime = 1.0f - Time;
		if (dTime < 0.001f) break;

		CMat4Dfp32 DestPos(pObj->GetLocalPositionMatrix());
		CVec3Dfp32::GetRow(DestPos, 3) += Vel;

		CInfo.m_bIsCollision = false;
		CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_TIME | CXR_COLLISIONRETURNVALUE_POSITION);

		if (Phys_MovePosition(&_Selection, _iObj, pObj->GetLocalPositionMatrix(), DestPos, &CInfo))
		{
//			ConOutD("Short: success 1");
			return 1.0f;
		}
//		ConOutD("Short: fail 1");

		if (CInfo.m_bIsValid)
		{
//			ConOutD("Short: CInfo valid");

			CInfo.m_Time = Max(0.0f, CInfo.m_Time - 0.01f / dTime);

			// Move to impact position
			int bImpactPos = (CInfo.m_Time < 0.0001f);
			if (!bImpactPos)
			{
				CMat4Dfp32 TestPos;
				MOVEPHYS_LERPMAT(pObj->GetLocalPositionMatrix(), DestPos, TestPos, CInfo.m_Time);
				if (Phys_MovePosition(&_Selection, _iObj, pObj->GetLocalPositionMatrix(), TestPos, NULL))
					bImpactPos = true;
			}

			// Fail?, Try to trace to impact position
			if (!bImpactPos)
			{
				CVec3Dfp32 dPos = CVec3Dfp32::GetRow(DestPos, 3) - pObj->GetPosition();
				fp32 T = Object_TraceDeltaMove(_Selection, _iObj, dPos);
				if (T > 0.0f)
				{
					bImpactPos = true;
					CInfo.m_Time = T;
				}
				else
				{
					// Fail?, Try move out of the impact plane and then try move again.
					CMat4Dfp32 TestPos(pObj->GetLocalPositionMatrix());
					CVec3Dfp32::GetRow(TestPos, 3) += CInfo.m_Plane.n;
					Phys_MovePosition(&_Selection, _iObj, pObj->GetLocalPositionMatrix(), TestPos, NULL);
					continue;
				}

//				ConOutD(CStrF("Short: TraceDeltaMove => T %f, Move %d, nSlide %d", T, nMove, nSlidePlanes));
			}

			if (bImpactPos)
			{
//				ConOutD(CStrF("Short: success 2, %f, Plane %s", CInfo.m_Time, (char*)CInfo.m_Plane.n.GetString() ));

				Time += CInfo.m_Time * dTime;
//return Time;

				// Add the latest collision plane to the sliding planes.
				SlidePlanes[nSlidePlanes++] = CInfo.m_Plane;

				// Calculate new velocity from the sliding planes.
				switch(nSlidePlanes)
				{
				case 1 :
					{
						// One plane, project velocity onto the plane.
OnePlane:
						CVec3Dfp32 VNProj;
						Vel.Project(SlidePlanes[0].n, VNProj);
						Vel.Sub(VNProj, Vel);

//						ConOutD(CStrF("Short: Success 2, dT %f, %s", CInfo.m_Time, (char*)Vel.GetString()) );
					}
					break;

				case 2 :
					{
//TwoPlanes:
//						ConOutD(CStrF("Short: NPlanes %d", nSlidePlanes));
//						Object_ShortDeltaMove(_iSel, _iObj, SlidePlanes[1].n);

						// Two planes, project velocity onto the vector (SlideN[0] crossprod SlideN[1])
						CVec3Dfp32 Proj;
						SlidePlanes[0].n.CrossProd(SlidePlanes[1].n, Proj);

						if (Proj.LengthSqr() < 0.001f)
						{
							// Planes are (close to) parallell, fallback to OnePlane method, remove the first plane.
							SlidePlanes[0] = SlidePlanes[1];
							nSlidePlanes--;
							goto OnePlane;
						}
						else
						{
							CVec3Dfp32 VProj;
							Vel.Project(Proj, VProj);
							Vel = VProj;
						}
					}
					break;

				default :
					return Time;
				}

				// New velocity calculation done, move again..
				continue;
			}
//			ConOutD("Short: fail 2");
		}
		else
			ConOutD("Short: CInfo false");

		break;
	}

	return Time;
}


// -------------------------------------------------------------------
fp32 CWorld_PhysState::Object_TraceDeltaMove( CPotColSet *_pcs, int _iObj, const CVec3Dfp32& _dPos )
{
	MAUTOSTRIP(CWorld_PhysState_Object_TraceDeltaMove, 0.0f);
	// NOTE:	Moves object with no subdivision. Caller is responsible for not calling with invalid _dPos.
	//			Finds impact position with recursive subdivision
	//			Return value is how far along _dPos we managed to move. 
	//			Cannot be 1.0
	//			0 == Could not move at all.)

	return 0.0f;

	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj) return 0;

	fp32 TValid = 0.0f;
	CVec3Dfp32 PosValid = pObj->GetPosition();

	fp32 TStep = 0.25f;
	fp32 T = 0.5f;

	fp32 TMin = 1.0f / 256;

	CMat4Dfp32 Pos(pObj->GetPositionMatrix());
	while(TStep > TMin)
	{
		pObj->GetPosition().Combine(_dPos, T, CVec3Dfp32::GetRow(Pos, 3));
		if (!Phys_IntersectWorld( _pcs, pObj->GetPhysState(), Pos, Pos, _iObj))
		{
			PosValid = CVec3Dfp32::GetRow(Pos, 3);
			TValid = T;
			T += TStep;
		}
		else
			T -= TStep;

		TStep *= 0.5f;
	}

	Phys_InsertPosition(_iObj, pObj);
	return TValid;
}


// -------------------------------------------------------------------
fp32 CWorld_PhysState::Object_TraceDeltaMove(const CSelection& _Selection, int _iObj, const CVec3Dfp32& _dPos)
{
	MAUTOSTRIP(CWorld_PhysState_Object_TraceDeltaMove, 0.0f);
	// NOTE:	Moves object with no subdivision. Caller is responsible for not calling with invalid _dPos.
	//			Finds impact position with recursive subdivision
	//			Return value is how far along _dPos we managed to move. 
	//			Cannot be 1.0
	//			0 == Could not move at all.)
return 0.0f;

	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj) return 0;

	fp32 TValid = 0.0f;
	CVec3Dfp32 PosValid = pObj->GetPosition();

	fp32 TStep = 0.25f;
	fp32 T = 0.5f;

	fp32 TMin = 1.0f / 256;

	CMat4Dfp32 Pos(pObj->GetPositionMatrix());
	while(TStep > TMin)
	{
		pObj->GetPosition().Combine(_dPos, T, CVec3Dfp32::GetRow(Pos, 3));
		if (!Phys_IntersectWorld(&_Selection, pObj->GetPhysState(), Pos, Pos, _iObj))
		{
			PosValid = CVec3Dfp32::GetRow(Pos, 3);
			TValid = T;
			T += TStep;
		}
		else
			T -= TStep;

		TStep *= 0.5f;
	}

	Phys_InsertPosition(_iObj, pObj);
	return TValid;
}
#if 0
static void ProjectOnPlane(const CVec3Dfp32& _v, const CVec3Dfp32& _n, CVec3Dfp32& _Res)
{
	MAUTOSTRIP(ProjectOnPlane, MAUTOSTRIP_VOID);
	CVec3Dfp32 VNProj;
	_v.Project(_n, VNProj);
	_v.Sub(VNProj, _Res);
}
#endif

static CVec3Dfp32 ProjectOnPlane(const CVec3Dfp32& _v, const CVec3Dfp32& _n)
{
	MAUTOSTRIP(ProjectOnPlane_2, CVec3Dfp32());
	CVec3Dfp32 VNProj, VNew;
	_v.Project(_n, VNProj);
	_v.Sub(VNProj, VNew);
	return VNew;
}




void CWorld_PhysState::GetMovementBounds( float *_BoxMinMax, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest )
{
	Error("Phys_SetPosition", "Internal error.");
}

void CWorld_PhysState::GetMovementBounds( float *_BoxMinMax, CWObject_CoreData* _pObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest )
{
	// WOrigin and WDest -> defines matrices
	
	CMat4Dfp32 WOrigin = _Origin;
	CMat4Dfp32 WDest = _Dest;

	M_ASSERT( _pObj, "GetMovementBounds - NULL Object.");

	if (_pObj->m_iObjectParent)
	{
		const CWObject_CoreData* pParent = Object_GetCD(_pObj->m_iObjectParent);

		if (pParent)
		{
			if (_pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOROTINHERITANCE)
			{
				CVec3Dfp32::GetRow(WDest, 3) += pParent->GetPosition();
				CVec3Dfp32::GetRow(WOrigin, 3) += pParent->GetPosition();
			}
			else
			{
				_Origin.Multiply(pParent->GetPositionMatrix(), WOrigin);
				_Dest.Multiply(pParent->GetPositionMatrix(), WDest);
			}
		}
	}

	// now bound both WOrigin and WDest bounds, into [x,y,z,time] bound
	
	if (!(_pObj->m_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION))
	{
		WOrigin.Unit3x3();
		WDest.Unit3x3();
	}

	CBox3Dfp32 BoxSrc;
	CWO_EnumParams_Box Params;

	Phys_GetMinMaxBox(_pObj->m_PhysState, WOrigin, BoxSrc);
	Phys_GetMinMaxBox(_pObj->m_PhysState, WDest, Params.m_Box);
	Params.m_Box.Expand(BoxSrc);

	_BoxMinMax[0] = Params.m_Box.m_Min[0];
	_BoxMinMax[1] = Params.m_Box.m_Min[1];
	_BoxMinMax[2] = Params.m_Box.m_Min[2];
	_BoxMinMax[3] = Params.m_Box.m_Max[0];
	_BoxMinMax[4] = Params.m_Box.m_Max[1];
	_BoxMinMax[5] = Params.m_Box.m_Max[2];
}


void CWorld_PhysState::CalcDestPos( CMat4Dfp32 *_pDestPos, CWObject_CoreData *pObj, fp32 _dTime, CVec3Dfp32 *_pAccel )
{
	DEBUG_CHECK_MATRIX(pObj->GetLocalPositionMatrix());
	if ( pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION )
	{
		CMat4Dfp32 Vel;
		pObj->GetVelocityMatrix(_dTime, Vel);
		DEBUG_CHECK_MATRIX(Vel);
		pObj->GetLocalPositionMatrix().Multiply3x3(Vel, *_pDestPos);
		_pDestPos->UnitNot3x3();
		_pDestPos->RecreateMatrix(0, 2);
		pObj->GetLocalPosition().Add(CVec3Dfp32::GetRow(Vel, 3), CVec3Dfp32::GetRow(*_pDestPos, 3));
	}
	else
	{
		*_pDestPos = pObj->GetLocalPositionMatrix();
		CVec3Dfp32::GetRow(*_pDestPos, 3).Combine(pObj->GetMoveVelocity(), _dTime, CVec3Dfp32::GetRow(*_pDestPos, 3));
	}

	CVec3Dfp32::GetRow(*_pDestPos, 3).Combine(*_pAccel, _dTime*_dTime / 2.0f, CVec3Dfp32::GetRow(*_pDestPos, 3));
}



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			v2.0 of MovePhysical.
						
	Parameters:			
		_iSel:			An optional selection which is the set to use for collision. 
						Pass -1 for the entire world. (Standard usage)
		_iObj:			Object to move.
		_dTime:			The time frame to move.
		_pMPInfo:		Extra input and feed back structure. (NULL for standard usage)
						
	Returns:			False if object was destroyed. 
						(As a result of object feed back. Object could have, for example, 
						been moved into a disintegration trigger and been removed.)
						
	Comments:			

\*____________________________________________________________________*/
bool CWorld_PhysState::Object_MovePhysical(const CSelection* _pSelection, int _iObj, fp32 _dTime, CMovePhysInfo* _pMPInfo)
{
	MAUTOSTRIP(CWorld_PhysState_Object_MovePhysical, false);
	MSCOPE(Object_MovePhysical, WORLD_PHYSSTATE);
	
	// -------------------------------------------------------------------
	// NOTE:
	//		Selection (_iSel) may be invalid
	//		_dTime is in SERVER_TIMEPERFRAME units. i.e, _dTime == 1 <=> SERVER_TIMEPERFRAME

	if (m_MovePhysRecurse > SERVER_MAXPHYSRECURSE) return false;

	m_MovePhysRecurse++;

#ifdef	SERVER_STATS
	CMTime Cycles; TStart(Cycles);
	m_nFunc_MovePhysical++;
#endif	// SERVER_STATS

	// Make _pMPInfo always valid so we don't have to check the pointer everywhere.
	CMovePhysInfo ProxyMovePhysInfo;
	if (!_pMPInfo)
		_pMPInfo = &ProxyMovePhysInfo;


	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	//	CVec3Dfp32 StartPos = pObj->GetPosition();


	
	if (!pObj) 
		MOVEPHYS_RETURN(true);

	MRTC_CRuntimeClass_WObject* pRTC = pObj->m_pRTC;

	// Rotation used?
	int bPhysRotate = pObj->GetPhysState().m_PhysFlags & (OBJECT_PHYSFLAGS_ROTATION || OBJECT_PHYSFLAGS_APPLYROTVEL);

	// Get subdivision length
	fp32 SubDiv = pObj->GetPhysState().GetMoveSubdivisionSize();


#ifdef USE_PCS
 #define PCSHACK_ARG (CWObject_CoreData*)&pcs
	// -------------------------------------------------------------------
	// First of all, create a  potential collision set. 
	// Slightly extended bounds are used here, since different subsets will
	// be created from it. (step up, step down etc)

	CPotColSet pcs;	

	DEBUG_GS_BGCOLOR( 0x0000ff );

	// potential collision set
	{
		float fBoxMinMax[6];																// bounds of movement
		CMat4Dfp32 DestPos;																	// destination of movement

		CVec3Dfp32 TmpAccel(0.0f, 0.0f, -1.0f);
		CalcDestPos( &DestPos, pObj, _dTime, &TmpAccel);
		GetMovementBounds( fBoxMinMax, _iObj, pObj->GetLocalPositionMatrix(), DestPos );	// extract [x,y,z,t] bounds

		const fp32 StepSize = pObj->m_PhysAttrib.m_StepSize;
		#define PCS_SAFEBORDER 8.0f
		fBoxMinMax[0] -= PCS_SAFEBORDER; fBoxMinMax[1] -= PCS_SAFEBORDER; fBoxMinMax[2] -= (PCS_SAFEBORDER+StepSize); // säkerhetsområde +-4 enheter
		fBoxMinMax[3] += PCS_SAFEBORDER; fBoxMinMax[4] += PCS_SAFEBORDER; fBoxMinMax[5] += (PCS_SAFEBORDER+StepSize);

		pcs.SetBox( fBoxMinMax );
		Selection_GetArray( &pcs, _iSel, pObj->m_PhysState, _iObj, pObj->GetLocalPositionMatrix(), DestPos );
	 #ifndef M_RTM
		pcs.DebugRender(this);
	 #endif
	}
	// -------------------------------------------------------------------
	DEBUG_GS_BGCOLOR( 0x00ff00 );
#else
# define PCSHACK_ARG NULL
#endif


	CMat4Dfp32 AccelMat;
	CVec3Dfp32& Accel = CVec3Dfp32::GetRow(AccelMat, 3);
	if (_pMPInfo->m_Flags & SERVER_MOVEPHYSINFOFLAGS_ACCELLERATION)
		AccelMat = _pMPInfo->m_Accelleration;
	else
	{
		DEBUG_GS_BGCOLOR( 0xffffff );
		// MUPPJOKKO - IMPLEMENT ME! Det här anropet är toklångsamt!
		pRTC->m_pfnOnPhysicsEvent(pObj, PCSHACK_ARG, this, CWO_PHYSEVENT_GETACCELERATION, 0, _dTime, &AccelMat, NULL);
		DEBUG_GS_BGCOLOR( 0x000000 );
		DEBUG_CHECK_VECTOR(Accel);
	}




// CVER_ARG -> Collision Version Argument

#ifdef USE_PCS
#define CVER_ARG &pcs
#define CVER_ARG_P &pcs
#else
#define CVER_ARG _pSelection
#define CVER_ARG_P (*_pSelection)
#endif


	// FIXME: Mondelore wrote this scope. MH should verify it...
/*	{
		const CVec3Dfp32& Velo = pObj->GetMoveVelocity();
		const CVec3Dfp32& Axx = CVec3Dfp32::GetRow(AccelMat, 3);

		// Simply move "non-intersecting" objects to their destination position (update velo += axx).
		bool bGhost = (pObj->GetPhysState().m_ObjectIntersectFlags == 0); //(pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_GHOST)
		if (bGhost)
		{
			CVec3Dfp32 NewPosition = pObj->GetPosition() + (Velo * _dTime) + (Axx * (Sqr(_dTime) * (1.0f/ 2.0f)));
			if (Object_SetPosition(pObj->m_iObject, NewPosition))
			{
				pObj->m_PhysVelocity.m_Move.Combine(Accel, _dTime, pObj->m_PhysVelocity.m_Move);
				MOVEPHYS_RETURN(true);
			}
		}

		// Simply move "teleporting" objects to their destination position (update velo += axx).
		fp32 MoveLen = (Velo.Length() * _dTime) + (Axx.Length() * Sqr(_dTime) * (1.0f/ 2.0f));
		if (MoveLen > (SubDiv * MOVEPHYS_MAXMOVE))
		{
			CVec3Dfp32 NewPosition = pObj->GetPosition() + (Velo * _dTime) + (Axx * (Sqr(_dTime) * (1.0f/ 2.0f)));
			if (Object_SetPosition(pObj->m_iObject, NewPosition))
			{
				ConOut(CStrF("(iObj %d) Movement %3.3f too big for subdivision. Object was teleported.", pObj->m_iObject, MoveLen));
				pObj->m_PhysVelocity.m_Move.Combine(Accel, _dTime, pObj->m_PhysVelocity.m_Move);
				MOVEPHYS_RETURN(true);
			}
		}
	}*/

	// -------------------------------------------------------------------
	//  First, attemp move with no subdivision and no step-up.
	{
		// make sure to re-update pcs if object steps out of the set!
	
		const fp32 LenVel = pObj->GetMoveVelocity().Length();
		const fp32 LenAcc = Accel.Length();
		const fp32 MoveLen = (LenVel * _dTime) + (LenAcc * Sqr(_dTime) * (1.0f / 2.0f));
		if (MoveLen < SubDiv)
		{
//			PhysOut(CStrF("QuickMove, Pos %s", pObj->GetPosition().GetString().Str()));

			DEBUG_CHECK_VECTOR(Accel);
			CMat4Dfp32 DestPos;
			CalcDestPos( &DestPos, pObj, _dTime, &Accel );
			DEBUG_CHECK_MATRIX(DestPos);

			CCollisionInfo CInfo;

			if( Phys_MovePosition( CVER_ARG, _iObj, pObj->GetLocalPositionMatrix(), DestPos, &CInfo ))
			{
				//The problem with floats is that PhysVelocity will never reach 0 it'll just become very small
				pObj->m_PhysVelocity.m_Move.Combine(Accel, _dTime, pObj->m_PhysVelocity.m_Move);
				ClampLow(pObj->m_PhysVelocity.m_Move);

#ifdef	SERVER_STATS
				m_nFunc_MovePhysicalQuick++; 
#endif	// SERVER_STATS
//				PhysOut(CStrF("QuickMove: Success 1, Pos %s", pObj->GetPosition().GetString().Str()));
				MOVEPHYS_OPTIONALROTATE(!bPhysRotate, _dTime);
				MOVEPHYS_RETURN(true);
			}
//			PhysOut(CStrF("QuickMove: Fail 1, fallback to 'Slow', Pos %s", pObj->GetPosition().GetString().Str()));
		}
	}

	// -------------------------------------------------------------------
	// Step up variables.
	CVec3Dfp32 StepUp_PrevPosDebug = pObj->GetPosition();
	CMat4Dfp32 StepUp_PrevPos;
	CVec3Dfp32 StepUp_dMove;
	fp32 StepUp = 0;
	bool bStepUpEnable = pObj->m_PhysAttrib.m_StepSize > 0.0f;
	bool bStepUpActive = false;
//	int nStepUpPlanes = 0;

	// -------------------------------------------------------------------
	// NOTE:
	//		_dTime is in SERVER_TIMEPERFRAME units. i.e, _dTime == 1 <=> SERVER_TIMEPERFRAME
	//		dTime is in SERVER_TIMEPERFRAME units and represents the time elapse of one round in the for loop.

	fp32 Time = 0.0f;
	int nMove = 0;

	int nSlidePlanes = 0;
	CPlane3Dfp32 SlidePlanes[16];

	CVec3Dfp32 StolenVelocity(0);

	// -------------------------------------------------------------------
	//  Move until we've moved the specified period of time. (_dTime)	
	// -------------------------------------------------------------------
	for(; (Time < _dTime) && (nMove < MOVEPHYS_MAXMOVE); nMove++)
	{
		PhysOut(CStrF("Slow: Begin %d, T %f, Pos %s", nMove, Time, pObj->GetPosition().GetString().Str()));

		// -------------------------------------------------------------------
		// Calculate the delta time from the distance we're allowed to move in one step
		fp32 dTime = _dTime - Time;

		fp32 LenVel = pObj->GetMoveVelocity().Length();
		fp32 LenAcc = Accel.Length();
		fp32 MoveLen = (LenVel * dTime) + (LenAcc * Sqr(dTime) * ( 1.0f / 2.0f ));
		if (MoveLen < 0.001f)
		{
			PhysOut(CStrF("Slow: MoveLen too slow %f, dTime %f, abort", MoveLen, dTime));
			break;
		}

		// Check if we need to subdivide movement, and if so, calculate the length of this step.
		if (MoveLen > SubDiv)
		{
			if (LenAcc < 0.001f)
			{
				// No acceleration, simple P1 solution
				dTime = dTime * SubDiv / MoveLen;
			}
			else
			{
				// Acceleration != 0, solve 2:nd degree polynom
				// ConOut(CStrF("Need subdivide: Len %f/%f, dTime %f, Vel %f, Acc %f", MoveLen, SubDiv, dTime, LenVel, LenAcc));
				fp32 ts0, ts1;
				if (SolveP2(LenAcc * ( 1.0f / 2.0f ), LenVel, -SubDiv, ts0, ts1))
					dTime = ts0;
				else
				{
					ConOut(CStrF("Slow: No time solution. %f, dTime %f, abort", MoveLen, dTime));
					break;
				}
				// ConOut(CStrF("        Solve %f, %f", ts0, ts1));
				dTime = ts0;
			}
		}

		// -------------------------------------------------------------------
		// Calc destination pos for this delta-move (TODO: Calculate fractional rotation)
		CMat4Dfp32 DestPos;

		CalcDestPos( &DestPos, pObj, dTime, &Accel );

		// Try move
		CCollisionInfo CInfo;


		if (Phys_MovePosition( CVER_ARG, _iObj, pObj->GetLocalPositionMatrix(), DestPos, &CInfo ))
		{
			// Successful
			// - Accelerate
			// - Update time with the accomplished motion.
			pObj->m_PhysVelocity.m_Move.Combine(Accel, dTime, pObj->m_PhysVelocity.m_Move);
			ClampLow(pObj->m_PhysVelocity.m_Move);
			Time += dTime;
			PhysOut(CStrF("Slow: Success 1, Pos %s", pObj->GetPosition().GetString().Str()));
			continue;
		}


/*		PhysOut(CStrF("Slow: Fail 1, Pos %s, Vel %s, Acc %s, Dst %s", pObj->GetPosition().GetString().Str(),
			pObj->GetMoveVelocity().GetString().Str(), Accel.GetString().Str(), DestPos.GetString().Str() ));
*/



		// -------------------------------------------------------------------
		// We've collided, do we have a valid collision info?
		if (CInfo.m_bIsValid)
		{
			// Yes, try to move to impact position specified in CInfo.m_Time
			// If the time to impact is insignificant, don't move, just pretent we did.
			PhysOut(CStrF("Slow: CInfo valid, Pos %s", pObj->GetPosition().GetString().Str()));

			// Validate plane in collision info.
			#ifdef PHYS_DEBUG
				if (CInfo.m_Plane.n.LengthSqr() < Sqr(0.99f) || CInfo.m_Plane.n.LengthSqr() > Sqr(1.01f))
					ConOutL(CStr("(MovePhysical) Invalid CInfo plane: %s" + CInfo.m_Plane.GetString()));
			#endif

			CInfo.m_Time = Max(0.0f, CInfo.m_Time - 0.01f / dTime);

			// -------------------------------------------------------------------
			// Are we at impact position?
			int bImpactPos = (CInfo.m_Time < 0.0001f);

			// -------------------------------------------------------------------
			// Move to impact position, unless we are already there.
			if (!bImpactPos)
			{
				CMat4Dfp32 TestPos;
				MOVEPHYS_LERPMAT(pObj->GetLocalPositionMatrix(), DestPos, TestPos, CInfo.m_Time);
//				CVec3Dfp32::GetRow(TestPos, 3).Combine(CInfo.m_Plane.n, 0.001f, CVec3Dfp32::GetRow(TestPos, 3));


				// TESTCODE - REMOVE ME!

				bImpactPos = Phys_MovePosition( CVER_ARG, _iObj, pObj->GetLocalPositionMatrix(), TestPos, NULL );

				// Fail?, Try to trace to impact position
				if (!bImpactPos)
				{
					CVec3Dfp32 dPos = CVec3Dfp32::GetRow(DestPos, 3) - pObj->GetPosition();
					fp32 T = Object_TraceDeltaMove( CVER_ARG_P, _iObj, dPos);
					if (T > 0.0f)
					{
						bImpactPos = true;
						CInfo.m_Time = T;
					}
					else
					{
						// Fail?, Try move out of the impact plane and then try move again.
						// This thing causes wall-climbing on some rare occations.

//						Object_ShortDeltaMove( CVER_ARG, _iObj, CInfo.m_Plane.n*(-CInfo.m_Distance + 0.001f ));
						CInfo.m_Time = 0;
						bImpactPos = true;

//						PhysOut(CStrF("Slow: Moving out of plane %s, Pos %s", CInfo.m_Plane.n.GetString().Str(), pObj->GetPosition().GetString().Str()));
//						continue;
					}
				}
			}

			// -------------------------------------------------------------------
			// Did we succeed moving to impact pos?
			if (bImpactPos)
			{
				bStepUpActive = false;

				// Successful
				// - Accelerate
				// - Update time with the accomplished motion.
				pObj->m_PhysVelocity.m_Move.Combine(Accel, dTime * CInfo.m_Time, pObj->m_PhysVelocity.m_Move);
				ClampLow(pObj->m_PhysVelocity.m_Move);
				Time += dTime * CInfo.m_Time;

				// If we managed to move, throw away all earlier sliding planes.
				if (dTime * CInfo.m_Time > 0.01f) nSlidePlanes = 0;

				// -------------------------------------------------------------------
				// Impact feed back to object.
				// -------------------------------------------------------------------
				{
					// Call OnPhysicsEvent on object collided to.
					int Ret = SERVER_PHYS_DEFAULTHANDLER;

					if (!(_pMPInfo->m_Flags & SERVER_MOVEPHYSINFOFLAGS_NOEXTERNALEFFECTS))
					{
						CWObject_CoreData* pObjOther = Object_GetCD(CInfo.m_iObject);
						MRTC_CRuntimeClass_WObject* pRTCOther = (pObjOther) ? pObjOther->m_pRTC : NULL;
						if (pObjOther && pRTCOther)
						{
							int Ret = pRTCOther->m_pfnOnPhysicsEvent(pObjOther, pObj, this, CWO_PHYSEVENT_IMPACTOTHER, Time, dTime, NULL, &CInfo);
							pObj = Object_GetCD(_iObj);
							if (!pObj)
							{
								MOVEPHYS_RETURN(false);
							}
							if (Ret == SERVER_PHYS_ABORT)
							{
								MOVEPHYS_OPTIONALROTATE(!bPhysRotate, Time);
								MOVEPHYS_RETURN(true);
							}
						}
					}

					bool bHandled = Ret == SERVER_PHYS_HANDLED;

					if (Ret == SERVER_PHYS_HANDLED)
					{
						// Call OnPhysicsEvent(CWO_PHYSEVENT_IMPACTHANDLEDOTHER) since Impact was handled by other object
						Ret = pRTC->m_pfnOnPhysicsEvent(pObj, NULL, this, CWO_PHYSEVENT_IMPACTHANDLEDOTHER, Time, dTime, NULL, &CInfo);
						pObj = Object_GetCD(_iObj);
						if (!pObj)
						{
							MOVEPHYS_RETURN(false);
						}
						if (Ret == SERVER_PHYS_ABORT)
						{
							MOVEPHYS_OPTIONALROTATE(!bPhysRotate, Time);
							MOVEPHYS_RETURN(true);
						}
						if (Ret == SERVER_PHYS_HANDLED)
							bHandled = true;
					}
					else
					{
						// Call OnPhysicsEvent(CWO_PHYSEVENT_IMPACT)
						Ret = pRTC->m_pfnOnPhysicsEvent(pObj, NULL, this, CWO_PHYSEVENT_IMPACT, Time, dTime, &AccelMat, &CInfo);
						pObj = Object_GetCD(_iObj);
						if (!pObj)
						{
							MOVEPHYS_RETURN(false);
						}
						if (Ret == SERVER_PHYS_ABORT)
						{
							MOVEPHYS_OPTIONALROTATE(!bPhysRotate, Time);
							MOVEPHYS_RETURN(true);
						}
						if (Ret == SERVER_PHYS_HANDLED)
							bHandled = true;

						// Send preintersection message
						{
							int Ret = Phys_Message_SendToObject(CWObject_Message(OBJSYSMSG_PHYSICS_PREINTERSECTION, CInfo.m_iObject, 0, 0, 0, 0, 0, &CInfo, sizeof(CInfo)), _iObj);

							// Make sure the object wasn't zapped.
							if (!Object_GetCD(_iObj))
							{
								MOVEPHYS_RETURN(false);
							}
							if (Ret == SERVER_PHYS_ABORT)
							{
								MOVEPHYS_OPTIONALROTATE(!bPhysRotate, Time);
								MOVEPHYS_RETURN(true);
							}
							if (Ret == SERVER_PHYS_HANDLED)
								bHandled = true;
						}
					}

					// If intersection was handled by either object or other object, proceed with next move.
					if (bHandled)
						continue;
				}

				// -------------------------------------------------------------------
				// COLLISION RESPONSE: Step up
				if (bStepUpEnable && 
					M_Fabs(CInfo.m_Plane.n[2]) < 0.001f &&
					StepUp == 0.0f)
					{
						// -------------------------------------------------------------------
						// Check impact plane to se if it's vertical, and perform step-up (for stairs) if it is.

						CVec3Dfp32 VFwd;
						pObj->GetMoveVelocity().Combine(Accel, 0.5f, VFwd);
						VFwd[2] = 0;
						if (VFwd.LengthSqr() > Sqr(0.01f))
						{
							StepUp_PrevPos = pObj->GetPositionMatrix();

							VFwd.Normalize();
							// CVec3Dfp32(0,0, pObj->m_PhysAttrib.m_StepSize).Combine(VFwd, -0.05f * 0,StepUp_dMove);
							StepUp_dMove = CVec3Dfp32(0,0, pObj->m_PhysAttrib.m_StepSize);
							StepUp = pObj->m_PhysAttrib.m_StepSize * Object_ShortDeltaMove( CVER_ARG_P, _iObj, StepUp_dMove, 0);

							// ConOut(CStrF("    Stepped up %f, %s", StepUp, pObj->GetPosition().GetString().Str()));

							if (Object_ShortDeltaMove( CVER_ARG_P, _iObj, VFwd*0.1f, 0) < 0.99f &&
								(nSlidePlanes < 1 || 
								  !(M_Fabs(SlidePlanes[0].n[2]) < 0.001f) ||
								  !(M_Fabs(CInfo.m_Plane.n[2]) < 0.001f) ||
								  (Object_ShortDeltaMove( CVER_ARG_P, _iObj, ProjectOnPlane(VFwd, SlidePlanes[0].n), 0) < 0.99f &&
								   Object_ShortDeltaMove( CVER_ARG_P, _iObj, ProjectOnPlane(VFwd, CInfo.m_Plane.n[2]), 0) < 0.99f
								 )
								)
							   )

							{
								// ConOut(CStr("    Cancelled step up"));
								StepUp_dMove = 0;
								StepUp = 0.0f;
								
//								Phys_SetPosition(_iSel, _iObj, StepUp_PrevPos, NULL);
								DEBUG_CHECK_MATRIX(StepUp_PrevPos);
								pObj->m_LocalPos = StepUp_PrevPos; // StepUp_PrevPos should still be safe, duh!
								Phys_InsertPosition(_iObj, pObj);							

								bStepUpActive = false;

							}
							else
							{
								// ConOut(CStr("    Approved"));
								pObj->m_PhysVelocity.m_Move[2] = Max(0.0f, pObj->m_PhysVelocity.m_Move[2]);
								ClampLow(pObj->m_PhysVelocity.m_Move);
								AccelMat.k[3][2] = Max(0.0f, AccelMat.k[3][2]);
								bStepUpActive = true;
								nSlidePlanes = 0;
								continue;
							}
						}
					}

				// -------------------------------------------------------------------
				// COLLISION RESPONSE: Bounce
				if (pObj->m_PhysAttrib.m_Elasticy)
				{
					// * Takes into account the velocity of the other object.
					// * Assumes the other object is unaffected by the bounce. (ie, infinite mass object)

					CWObject_CoreData* pObjOther = Object_GetCD(CInfo.m_iObject);
					CVec3Dfp32 VelocityOther = (pObjOther) ? pObjOther->GetMoveVelocity() : 0;
					fp32 ProjVelocity = -((pObj->GetMoveVelocity() - VelocityOther) * CInfo.m_Plane.n);

					// Only bounce when projected velocity is significant, else just let sliding handle the collision.
					if (ProjVelocity > 0.001f)
					{
						fp32 NewProjVelocity = -ProjVelocity * pObj->m_PhysAttrib.m_Elasticy;
						CVec3Dfp32 NewVelocity;
						pObj->GetMoveVelocity().Combine(CInfo.m_Plane.n, ProjVelocity - NewProjVelocity, NewVelocity);
						NewVelocity += VelocityOther;
						Object_SetVelocity(pObj->m_iObject, NewVelocity);

						// Call OnPhysicsEvent
//						LogFile("Warning, a fp32-pointer is passed as CMat4Dfp32-pointer");
						int Ret = pRTC->m_pfnOnPhysicsEvent(pObj, NULL, this, CWO_PHYSEVENT_BOUNCE, Time, dTime, (CMat4Dfp32*)&ProjVelocity, &CInfo);
						pObj = Object_GetCD(_iObj);
						if (!pObj)
						{
							MOVEPHYS_RETURN(false);
						}
						if (Ret == SERVER_PHYS_ABORT)
						{
							MOVEPHYS_OPTIONALROTATE(!bPhysRotate, Time);
							MOVEPHYS_RETURN(true);
						}

						continue;
					}
				}

				// -------------------------------------------------------------------
				// COLLISION RESPONSE: Push
				if (!(_pMPInfo->m_Flags & SERVER_MOVEPHYSINFOFLAGS_NOEXTERNALEFFECTS) &&
					(pObj->m_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_PUSHER))
				{
					// * Only handles translation (i.e, no pushing due to rotation)
					// * Cheap in other ways.

					CWObject_CoreData* pObjOther = Object_GetCD(CInfo.m_iObject);
					if (pObjOther && pObjOther->m_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_PUSHABLE)
					{
						CCollisionInfo CInfoPush;
						CInfoPush = CInfo;
						int nPush = 0;
DoPushLoop:
						nPush++;
						CVec3Dfp32 MoveVelocity;
						CVec3Dfp32 MoveAccel;
//						fp32 MoveVelocityLenSqr = pObj->GetMoveVelocity().LengthSqr();

		/*				if (MoveVelocityLenSqr > Sqr(0.001f))
							pObj->GetMoveVelocity().Scale(-(pObj->GetMoveVelocity() * CInfoPush.m_Plane.n) / M_Sqrt(MoveVelocityLenSqr), MoveVelocity);
						else
							MoveVelocity = 0;*/

						CInfoPush.m_Plane.n.Scale(CInfoPush.m_Plane.n * pObj->GetMoveVelocity(), MoveVelocity);

						Accel.Scale(-Accel * CInfoPush.m_Plane.n, MoveAccel);

						CVelocityfp32 OldVel = pObjOther->m_PhysVelocity;


						// Call OnPhysicsEvent(CWO_PHYSEVENT_PREPUSHED) on OTHER object
						pObj = Object_GetCD(_iObj);
						if (!pObj)
						{
							MOVEPHYS_RETURN(false);
						}

//						LogFile("Warning, a CVec3Dfp32-pointer is passed as CMat4Dfp32-pointer");
						int Ret = pObj->m_pRTC->m_pfnOnPhysicsEvent(pObj, pObjOther, this, CWO_PHYSEVENT_PREPUSHED, Time, dTime, (CMat4Dfp32*)&MoveVelocity, &CInfoPush);

						if (Ret == SERVER_PHYS_ABORT)
						{
							MOVEPHYS_OPTIONALROTATE(!bPhysRotate, Time);
							MOVEPHYS_RETURN(true);
						}

						// If other object is still alive proceed with pushing it.
						pObjOther = Object_GetCD(CInfoPush.m_iObject);
						if (pObjOther)
						{
							Object_SetVelocity(CInfoPush.m_iObject, MoveVelocity);

							CMovePhysInfo MoveInfo(SERVER_MOVEPHYSINFOFLAGS_ACCELLERATION);
							MoveInfo.m_Accelleration.Unit();
	//						MoveAccel.SetRow(MoveInfo.m_Accelleration, 3);
							Object_MovePhysical(CInfoPush.m_iObject, _dTime - Time, &MoveInfo);

							Object_SetVelocity(CInfoPush.m_iObject, OldVel);
						}

						bool bTriedBlockRemoved = false;
					BlockRemoved:
						CCollisionInfo CInfoPostPush;
						if (!Phys_MovePosition( CVER_ARG, _iObj, pObj->GetPositionMatrix(), DestPos, &CInfoPostPush))
						{
							// Check if we're colliding the object we just pushed, if not, start from the beginning.
							if (CInfoPostPush.m_iObject != CInfo.m_iObject)
								continue;
							if (nPush < 2 && CInfoPostPush.m_bIsValid && (CInfoPostPush.m_Plane.n.DistanceSqr(CInfoPush.m_Plane.n) > 0.001f))
							{
								CInfoPush = CInfoPostPush;
								goto DoPushLoop;
							}

							// Call OnPhysicsEvent(CWO_PHYSEVENT_BLOCKING)
							int Ret = pRTC->m_pfnOnPhysicsEvent(pObj, pObjOther, this, CWO_PHYSEVENT_BLOCKING, Time, dTime, NULL, &CInfo);
							pObj = Object_GetCD(_iObj);
							if (!pObj)
							{
								MOVEPHYS_RETURN(false);
							}
							if (Ret == SERVER_PHYS_ABORT)
							{
								MOVEPHYS_OPTIONALROTATE(!bPhysRotate, Time);
								MOVEPHYS_RETURN(true);
							}
							if(!bTriedBlockRemoved)
							{
								bTriedBlockRemoved = true;
								// Try on more time
								goto BlockRemoved;
							}
						}
						else
						{
							// Successful
							// - Accelerate
							// - Update time with the accomplished motion.

							fp32 dTimePush = dTime * (1.0f - CInfo.m_Time);

							pObj->m_PhysVelocity.m_Move.Combine(Accel, dTimePush, pObj->m_PhysVelocity.m_Move);
							ClampLow(pObj->m_PhysVelocity.m_Move);
							Time += dTimePush;

							// Since we managed to move, throw away all earlier sliding planes.
							nSlidePlanes = 0;

							continue;
						}
					}
				}

				// -------------------------------------------------------------------
				// COLLISION RESPONSE: Slide against surface.

				if (!(pObj->m_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_SLIDEABLE))
				{
					Object_SetVelocity(_iObj, CVec3Dfp32(0));
					Accel= 0;
				}
				else
				{
					// Add the latest collision plane to the sliding planes.
					SlidePlanes[nSlidePlanes++] = CInfo.m_Plane;

#ifdef USE_PCS					
					// Put PCS through plane equation sieve
					pcs.PlaneSieve( CInfo.m_Plane.n[0], CInfo.m_Plane.n[1], CInfo.m_Plane.n[2], CInfo.m_Plane.d );
#endif

					// Calculate new velocity from the sliding planes.
					switch(nSlidePlanes)
					{
					case 1 :
						{
							// -------------------------------------------------------------------
							// One plane, project velocity onto the plane.
	OnePlane:
							CVec3Dfp32 ANProj, VNew;

							const CVec3Dfp32& Velo = pObj->GetMoveVelocity();
							fp32 s = Velo * SlidePlanes[0].n;
							s = Max(-s, 0.1f);
							Velo.Combine(SlidePlanes[0].n, s, VNew);

							CWObject_CoreData* pObjOther = Object_GetCD(CInfo.m_iObject);
							if (pObjOther && pObjOther->GetMoveVelocity().LengthSqr() > 0.001f)
							{
								CVec3Dfp32 VDiff, VDiffProj;
								VNew.Sub(pObj->GetMoveVelocity(), VDiff);
								fp32 Proj = -(VDiff * pObjOther->GetMoveVelocity());
								fp32 OtherMoveSqr = pObjOther->GetMoveVelocity().LengthSqr();
								if (Proj > OtherMoveSqr)
								{
									StolenVelocity.Combine(CInfo.m_Plane.n, pObjOther->GetMoveVelocity()*CInfo.m_Plane.n, StolenVelocity);
//									StolenVelocity = pObjOther->GetMoveVelocity();
								}
								else if (Proj > 0.0f)
								{
									Proj *= 1.0f / OtherMoveSqr;
//									VDiff.Project(pObjOther->GetMoveVelocity(), VDiffProj);

									CVec3Dfp32 MoveProj;
									pObjOther->GetMoveVelocity().Scale(Proj, MoveProj);

									CVec3Dfp32 Stolen;
									CInfo.m_Plane.n.Scale(CInfo.m_Plane.n * MoveProj, Stolen);
									StolenVelocity += Stolen;
//									StolenVelocity += (pObjOther->GetMoveVelocity() * Proj);
								}
							}
							Object_SetVelocity(_iObj, VNew);
							Accel.Project(SlidePlanes[0].n, ANProj);
							Accel.Sub(ANProj, Accel);

/*				ConOutL(CStrF("Slow: Slide 1, dT %f, VNew %s, VOld %s, Pos %s, Acc %s, OAcc %s", 
					CInfo.m_Time, (char*)VNew.GetString(), VOld.GetString().Str(), pObj->GetPosition().GetString().Str(),
					Accel.GetString().Str(), OldAcc.GetString().Str() ));*/
						}
						break;

					case 2 :
						{
	TwoPlanes:
							// -------------------------------------------------------------------
							// Two planes, project velocity onto the vector (SlideN[0] crossprod SlideN[1])
							CVec3Dfp32 Proj;
							SlidePlanes[0].n.CrossProd(SlidePlanes[1].n, Proj);

							if (Proj.LengthSqr() < 0.0001f)
							{
								// Planes are (close to) parallell, fallback to OnePlane method, remove the first plane.
								SlidePlanes[0] = SlidePlanes[1];
								nSlidePlanes--;
								goto OnePlane;
							}
							else
							{
								// ConOut(CStrF("Slow: 2 NPlanes, Proj axis %s", Proj.GetString().Str() ));
								Proj.Normalize();
								CVec3Dfp32 VNew, AProj, VDiff;
								pObj->GetMoveVelocity().Project(Proj, VNew);

								CWObject_CoreData* pObjOther = Object_GetCD(CInfo.m_iObject);
								if (pObjOther && pObjOther->GetMoveVelocity().LengthSqr() > 0.001f)
								{
									CVec3Dfp32 VDiffProj;
									VNew.Sub(pObj->GetMoveVelocity(), VDiff);
									fp32 Proj = -(VDiff * pObjOther->GetMoveVelocity());
									fp32 OtherMoveSqr = pObjOther->GetMoveVelocity().LengthSqr();
									if (Proj > OtherMoveSqr)
										StolenVelocity = pObjOther->GetMoveVelocity();
									else if (Proj > 0.0f)
									{
										Proj *= 1.0f / OtherMoveSqr;
	//									VDiff.Project(pObjOther->GetMoveVelocity(), VDiffProj);
										StolenVelocity += (pObjOther->GetMoveVelocity() * Proj);
									}
								}


								Object_SetVelocity(_iObj, VNew);

								Accel.Project(Proj, AProj);
								Accel = AProj;
							}

							PhysOut(CStrF("Slow: Slide 2, dT %f, %s, Pos %s", CInfo.m_Time, (char*)pObj->GetMoveVelocity().GetString(), pObj->GetPosition().GetString().Str() ));
						}
						break;

					case 3 :
						{
							// -------------------------------------------------------------------
							// Three planes.
							CVec3Dfp32 Proj;
							SlidePlanes[2].n.CrossProd(SlidePlanes[0].n, Proj);

							if (Proj.LengthSqr() < 0.0001f)
							{
								// Plane 2 and 0 are close to parallell, use plane 2 and 1.
								SlidePlanes[0] = SlidePlanes[1];
								SlidePlanes[1] = SlidePlanes[2];
								nSlidePlanes--;
								goto TwoPlanes;
							}
							else
							{
								// Plane 2 and 1 are close to parallell, use plane 2 and 0.
								SlidePlanes[1] = SlidePlanes[2];
								nSlidePlanes--;
								goto TwoPlanes;
							}
						}
						break;

					case 4 :
						{
							// -------------------------------------------------------------------
							PhysOut(CStrF("Slow: NPlanes %d, N %s", nSlidePlanes, SlidePlanes[3].n.GetString().Str() ));

							Object_ShortDeltaMove( CVER_ARG_P, _iObj, SlidePlanes[3].n);
							nSlidePlanes = 0;
						}
						break;

					default :
						{
							// -------------------------------------------------------------------
							// ConOut(CStrF("Slow: NPlanes %d, unsupported. Abort.", nSlidePlanes));
							PhysOut(CStrF("Slow: Internal error, NPlanes %d, Pos %s", nSlidePlanes, pObj->GetPosition().GetString().Str()));
							goto AbortMove;
						}
					}
				}

				// New velocity calculation done, move again..
				continue;

			} // End of collision response.

//			{
//				int b = 1;
//			}

			// Unable to move to impact position.
			PhysOut(CStrF("Slow: Fail 2, Move %d, N %s, nSlide %d, Pos %s", nMove, CInfo.m_Plane.n.GetString().Str(), nSlidePlanes, pObj->GetPosition().GetString().Str()));
		}
		else
			// No collision info, abort...
			PhysOut(CStrF("Slow: CInfo false, %s -> %s", pObj->GetPosition().GetString().Str(), CVec3Dfp32::GetRow(DestPos, 3).GetString().Str()));

//		{
//			int a = 1;
//		}

		// -------------------------------------------------------------------
AbortMove:
		// Could not move at all, mayday, mayday, abort, abort!!
		// Kill velocity.
		Object_SetVelocity(_iObj, CVec3Dfp32(0));
		break;

	} // End of move loop.

	// -------------------------------------------------------------------
	// Step down?
	if (bStepUpEnable && StepUp)
	{
		CVec3Dfp32 Pos = pObj->GetPosition();

		fp32 t = Object_ShortDeltaMove( CVER_ARG_P, _iObj, -StepUp_dMove, 0);	// nSlides must be 0. When sliding is enabled the player can be teleportet back a few units for some reason.
		Pos -= pObj->GetPosition();
		fp32 StepUp_dMoveLenSqr = StepUp_dMove.LengthSqr();
		if (StepUp_dMoveLenSqr > 0.0001f)
			t = (Pos * StepUp_dMove) / StepUp_dMoveLenSqr;
		else
			t = 0;

		if (t < 0.99f)
		{
			_pMPInfo->m_StepUpResult = StepUp * (1.0f - t);

/*			ConOut(CStrF("Step  Up %f, Down %f, dMove %s, StartPos %s --> %s, (%s --> %s)", StepUp, t*StepUp, 
				StepUp_dMove.GetString().Str(),
				StepUp_PrevPosDebug.GetString().Str(),
				Pos.GetString().Str(),
				CVec3Dfp32::GetRow(StepUp_PrevPos, 3).GetString().Str(),
				pObj->GetPosition().GetString().Str()));
*/
		}
	}

	pObj->m_PhysVelocity.m_Move += StolenVelocity;
	ClampLow(pObj->m_PhysVelocity.m_Move);

	// -------------------------------------------------------------------
	MOVEPHYS_OPTIONALROTATE(!bPhysRotate, Time);
	MOVEPHYS_RETURN(true);
}


bool CWorld_PhysState::Object_MovePhysical(int _iObj, fp32 _dTime, CMovePhysInfo* _pMPInfo)
{
	MAUTOSTRIP(CWorld_PhysState_Object_MovePhysical_3, false);
	return Object_MovePhysical((CSelection *) NULL, _iObj, _dTime, _pMPInfo);
}

bool CWorld_PhysState::Object_MoveTo(int _iObj, const CMat4Dfp32& _NewPos, CMovePhysInfo* _pMPInfo)
{
	MAUTOSTRIP(CWorld_PhysState_Object_MoveTo, false);
	MSCOPESHORT(CWorld_PhysState::Object_MoveTo);
//	CMat4Dfp32 PosInv, _Vel;

	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj) return true;

	CVec3Dfp32 dMove;
	CVec3Dfp32::GetMatrixRow(_NewPos, 3).Sub(pObj->GetLocalPosition(), dMove);

	CQuatfp32 Quat, QuatNew, dQuat;
	Quat.Create(pObj->GetLocalPositionMatrix());
//	pObj->GetRotVelocity().CreateQuaternion(Quat);
	Quat.Inverse();
	QuatNew.Create(_NewPos);
	Quat.Multiply(QuatNew, dQuat);

#ifdef PLATFORM_DOLPHIN
	dQuat.k[3] = Ceil1( -Ceil1(-dQuat.k[3]) );
#endif

#ifdef	PLATFORM_PS2
	// The FPU unit in the PS2 sucks! this can give an "invalid" quaternion sometimes (w slightly larger than 1.0f)
	dQuat.Normalize();
#endif	// PLATFORM_PS2

//	pObj->GetPositionMatrix().InverseOrthogonal(PosInv);
//	PosInv.Multiply(_NewPos, _Vel);
//	_NewPos.Multiply(PosInv, _Vel);
	pObj->m_PhysVelocity.m_Move = dMove;
	ClampLow(pObj->m_PhysVelocity.m_Move);
	pObj->m_PhysVelocity.m_Rot.Create(dQuat);
	DEBUG_CHECK_VECTOR(pObj->m_PhysVelocity.m_Move);
	DEBUG_CHECK_AXISANGLE(pObj->m_PhysVelocity.m_Rot);

//	LogFile("MoveTo: Vel " + pObj->m_PhysVelocity.GetString());
//	pObj->m_PhysVelocity = CMat4Dfp32::UnitMatrix;

//	return true;
	return Object_MovePhysical(_iObj, 1.0f, _pMPInfo);
}

bool CWorld_PhysState::Object_MoveTo_World(int _iObj, const CMat4Dfp32& _NewPos, CMovePhysInfo* _pMPInfo)
{
	MAUTOSTRIP(CWorld_PhysState_Object_MoveTo_World, false);
	MSCOPESHORT(CWorld_PhysState::Object_MoveTo_World);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj)
	{
		Error("Object_SetPosition", "NULL Object.");
		return false;
	}
	
	if (pObj->GetParent())
	{
		CWObject_CoreData* pParent = Object_GetCD(pObj->GetParent());
		if (!pParent)
			return false;
		
		CMat4Dfp32 Local;
		CMat4Dfp32 ParentInv;
		pParent->GetPositionMatrix().InverseOrthogonal(ParentInv);
		_NewPos.Multiply(ParentInv, Local);
		return Object_MoveTo(_iObj, Local, _pMPInfo);
	}
	else
		return Object_MoveTo(_iObj, _NewPos, _pMPInfo);
}
