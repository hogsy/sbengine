#include "PCH.h"
#include "WOBJ_Model_Moth.h"
#include "../WObj_Game/WObj_GameMessages.h"

//int CWObject_Model_Moth::ms_NumButterflies = 0;
bool CWObject_Model_Moth::ms_SinRandTableInitialized = false;
fp32 CWObject_Model_Moth::ms_SinRandTable[CWOBJECT_MODEL_MOTH_SINRAND_TABLESIZE];

class CXR_Model;


// THIS IS A WORK IN PROGRESS, IT WILL BE CLEANED UP WHEN FINALIZED!!!


CWObject_Model_Moth::CWObject_Model_Moth()
{
	// Ain't doing nothing right now...
	return;
}

void CWObject_Model_Moth::SetDefaultValues()
{
	m_LifeTime = 0.0f;

	m_OldPos = CVec3Dfp32(0.0f);
	m_VelocityVector = CVec3Dfp32(0.0f);

	m_LastPosNoise = CVec3Dfp32(0.0f);
	m_LastDirNoise = CVec3Dfp32(0.0f);
	m_LastDir = CVec3Dfp32(0.0f);

	m_Avoid = false;
	m_AvoidVector = CVec3Dfp32(0.0f);

	m_TraceDelay = m_iObject % 3;	// Make moths trace on different ticks.. maybe
	m_IntersectDelay = 5;
	m_AltitudeDelay = 4;
	m_AltitudeDelayValue = 4;

	m_MaxMovementScans = 3;
	m_TraceProximityRange = 64.0f;
	m_IntersectProximityRange = 96.0f;
	m_CloseAvoidFactor = 15.0f;
	m_DistantAvoidFactor = 10.0f;
	m_MinSpeed = 15.0f;
	m_MaxSpeed = 20.0f;
	m_InvMaxSpeed = 1.0f / 20.0f;
	m_MinAcceleration = 15.0f;
	m_MaxAcceleration = 20.0f;
	m_AccelerationSpeedDependency = 0.5f;
	m_FreespaceAcceleration = 10.0f;
	m_MinDescendSlope = 0.5f;
	m_DescendAcceleration = 0.5f;
	m_MaxSlope = 0.5f;
	m_FlattenAcceleration = 0.0f;
	m_FlattenFactor = 0.2f;

	m_LandPositionOffset = 1.0f;
	m_MaxLandingDistance = 25.0f;
	m_MaxLandingSlope = 1.0f;
	m_DepartPositionOffset = 4.0f;

	m_NoiseMovement = true;
	m_MovementNoiseTimeScale = 0.3f;
	m_PositionNoise = 3.0f;
	m_DirectionNoise = 4.0f;

	m_FilterDirection = true;
	m_DirectionFilterLeak = 0.3f;
	
	m_LimitFacingSlope = true;
	m_MaxFacingSlope = 0.3f;

	m_MinFlyingDuration = 5.0f;
	m_MaxFlyingDuration = 30.0f;
	m_MinLandedDuration = 5.0f;
	m_MaxLandedDuration = 15.0f;
	m_MinLandDuration = 0.5f;
	m_MaxLandDuration = 1.0f;
	m_MinDepartDuration = 0.5f;
	m_MaxDepartDuration = 1.0f;

	m_FlyingFlutterFreq = 1.0f;
	m_MorphFlutterFreq = 1.0f;
	m_LandedFlutterFreq = 0.2f;
	m_LandedWingSpreadDuration = 0.2f;

	m_CorrectAltitude = true;
	m_WantedAltitude = 70.0f;
	m_AltitudeAcceleration = 15.0f;

	m_ApplyFlutterAscention = true;
	m_FlutterAscention = 3.0f;

	m_ModelSize = 10.0f;

	m_TimeScale = 1.0f;

	m_WingStateTime = 0.0f;
	m_WingStateFreq = 0.0f;

	m_Randseed = 0;

	m_Options = 0;

	SetAIState(AIState_Flying);
}

void CWObject_Model_Moth::CreateSinRandTable()
{
	ms_SinRandTableInitialized = true;

	fp32 x, y, dy;

	for (int i = 0; i < CWOBJECT_MODEL_MOTH_SINRAND_TABLESIZE; i++) 
	{
		x = (fp32)i *(1.0f/(fp32)(CWOBJECT_MODEL_MOTH_SINRAND_TABLESIZE - 1));
		y = 0;

		dy = M_Sin((3.0f * _PI2) * (x + 0.0f));
		if (dy < -1.0f) dy = -1.0f;
		if (dy > +1.0f) dy = +1.0f;
		y += 0.5f * dy;

		dy = M_Sin((7.0f * _PI2) * (x + 0.3f));
		if (dy < -1.0f) dy = -1.0f;
		if (dy > +1.0f) dy = +1.0f;
		y += 0.25f * dy;

		dy = M_Sin((13.0f * _PI2) * (x + 0.7f));
		if (dy < -1.0f) dy = -1.0f;
		if (dy > +1.0f) dy = +1.0f;
		y += 0.125f * dy;

		dy = M_Sin((25.0f * _PI2) * (x + 0.13f));
		if (dy < -1.0f) dy = -1.0f;
		if (dy > +1.0f) dy = +1.0f;
		y += 0.0625f * dy;

		ms_SinRandTable[i] = 0.5f + 0.5f * (y *(1.0f/ 0.9375f));
	}
}

fp32 CWObject_Model_Moth::GetSinRandTable(fp32 _Time, fp32 _Timescale)
{
	fp32 y, y1, y2;
	int32 x, xi, xf;

	x = RoundToInt((fp32)(CWOBJECT_MODEL_MOTH_SINRAND_TABLESIZE - 1) * 255.0f * _Time * _Timescale);

	xi = (x >> 8) & (CWOBJECT_MODEL_MOTH_SINRAND_TABLESIZE - 1);
	xf = x & 0xFF;

	y1 = ms_SinRandTable[xi];
	y2 = ms_SinRandTable[(xi + 1) & (CWOBJECT_MODEL_MOTH_SINRAND_TABLESIZE - 1)];

	y = y1 + (y2 - y1) * (fp32)xf *(1.0f/255.0f);

	return y;
}

fp32 CWObject_Model_Moth::GetSinRandTableSlope(fp32 _Time, fp32 _Timescale)
{
	return (GetSinRandTable(_Time + 0.01f, _Timescale) - GetSinRandTable(_Time - 0.01f, _Timescale));
}

void CWObject_Model_Moth::SetAIState(CWObject_Model_Moth::AIState _State)
{
	m_AIState = _State;
	m_StateTime = 0.0f;
	m_TraceDelay = 0;
	m_IntersectDelay = 0;
	m_AltitudeDelay = 0;

	switch (m_AIState)
	{
		case AIState_Smashed:
		case AIState_Escape:
			break;
		case AIState_Flying:
//			ConOut("Flying");
			m_WantedStateTime = m_MinFlyingDuration + GetRand() * (m_MaxFlyingDuration - m_MinFlyingDuration);
			break;

		case AIState_Landed:
//			ConOut("Landed");
			m_WantedStateTime = m_MinLandedDuration + GetRand() * (m_MaxLandedDuration - m_MinLandedDuration);
			break;

		case AIState_Land:
//			ConOut("Landing");
			m_WantedStateTime = m_MinLandDuration + GetRand() * (m_MaxLandDuration - m_MinLandDuration);
			break;

		case AIState_Depart:
//			ConOut("Departing");
			m_WantedStateTime = m_MinDepartDuration + GetRand() * (m_MaxDepartDuration - m_MinDepartDuration);

			m_OldPos = m_LandPos + m_LandNormal * m_DepartPositionOffset;

			if (m_LandNormal.k[2] < 0.5f)
			{ // Vertical landing surface
				m_VelocityVector.k[0] = GetRand() - 0.5f;
				m_VelocityVector.k[1] = GetRand() - 0.5f;
				m_VelocityVector.k[2] = GetRand() - 0.5f;
				fp32 aligned = m_VelocityVector * m_LandNormal;
				m_VelocityVector -= m_LandNormal * aligned;
				m_VelocityVector.Normalize();
				m_VelocityVector += m_LandNormal;

				CVec3Dfp32 delta = (m_VelocityVector - m_LandNormal);
//				ConOut(CStrF("Vertical departure (normal vs velocity = %f)", delta.Length()));
			}
			else
			{ // Horizontal landing surface
//				ConOut("Horizontal departure");
			}

			m_VelocityVector.Normalize();

			if ((m_LimitFacingSlope) && (m_AIState == AIState_Flying))
			{
				if (m_VelocityVector.k[2] > m_MaxFacingSlope)
					m_VelocityVector.k[2] = m_MaxFacingSlope;
				if (m_VelocityVector.k[2] < -m_MaxFacingSlope)
					m_VelocityVector.k[2] = -m_MaxFacingSlope;

				// Scale x, y to get unit length. Can't just normalize (z may still be too dominant).
				fp32 temp = M_Sqrt((1.0f - m_VelocityVector.k[2] * m_VelocityVector.k[2]) / (m_VelocityVector.k[0] * m_VelocityVector.k[0] + m_VelocityVector.k[1] * m_VelocityVector.k[1]));
				m_VelocityVector.k[0] *= temp;
				m_VelocityVector.k[1] *= temp;
			}

			m_VelocityVector *= 0.5f;


			break;
	}
}

bool CWObject_Model_Moth::TraceRay(CVec3Dfp32 _RayOrigin, CVec3Dfp32 _RayDir, fp32 _RayLength, SurfaceInfo& _SurfaceInfo)
{
#ifndef	M_RTM
	bool bDebugRender = true;
#endif
	fp32 DebugRenderDuration = 20.0f;
	
	CVec3Dfp32 RayEnd = _RayOrigin + _RayDir * _RayLength;

	CCollisionInfo CInfo;
	CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
	int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
	int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
	int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
	int32 iExclude = m_iObject;

	bool bHit = m_pWServer->Phys_IntersectLine(_RayOrigin, RayEnd, OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude);
	if (bHit)
	{
#ifndef	M_RTM
		if (bDebugRender)
			m_pWServer->Debug_RenderWire(_RayOrigin, CInfo.m_Pos, 0xFF00FF00, DebugRenderDuration);
#endif
		if (CInfo.m_bIsValid)
		{
			_SurfaceInfo.m_Pos = _RayOrigin + _RayDir * _RayLength * CInfo.m_Time;
			_SurfaceInfo.m_Normal = CInfo.m_Plane.n;
			_SurfaceInfo.m_Distance = _RayLength * CInfo.m_Time;
			_SurfaceInfo.m_iObject = CInfo.m_iObject;
			_SurfaceInfo.m_Pos += _SurfaceInfo.m_Normal * m_LandPositionOffset;
			_SurfaceInfo.m_CanLand = false;
			// Check out what we have hit
			if (CInfo.m_pSurface)
			{
				if (!(CInfo.m_pSurface->m_Flags & XW_SURFFLAGS_INVISIBLE))
					_SurfaceInfo.m_CanLand = true;

			}
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
#ifndef	M_RTM
		if (bDebugRender) 
			m_pWServer->Debug_RenderWire(_RayOrigin, RayEnd, 0xFFFF0000, DebugRenderDuration);
#endif
		return false;
	}
}

bool CWObject_Model_Moth::CheckIntersection()
{
	// Check if a character is nearby when the moth has landed
	// if the character is closer than "proximityrange" the moth should 
	// depart away from the character?

	int iObject = m_iObject;
	TSelection<CSelection::LARGE_BUFFER> Selection;

	CWO_PhysicsState PhysState;
	PhysState.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(m_IntersectProximityRange), 0);
	PhysState.m_nPrim = 1;
	PhysState.m_ObjectFlags = 0; //OBJECT_FLAGS_CHARACTER;
	PhysState.m_MediumFlags = 0; //XW_MEDIUM_PLAYERSOLID;
	PhysState.m_PhysFlags = 0;
	PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_CHARACTER;
	PhysState.m_iExclude = iObject;
	
	m_pWServer->Selection_AddIntersection(Selection, GetPosition(), PhysState);

	const int16* pSelection = NULL;
	int nSelection = m_pWServer->Selection_Get(Selection, &pSelection);

	CWObject_CoreData* pObjectCD;
	CVec3Dfp32 pos, dir;

	bool intersected = (nSelection > 0);

	if (intersected)
	{
		CVec3Dfp32 avoidvector(0);

		for(int i = 0; i < nSelection; i++)
		{
			pObjectCD = m_pWServer->Object_GetCD(pSelection[i]);
			pos = pObjectCD->GetPosition();
			avoidvector += (m_LandPos - pos);
		}

		avoidvector.SetLength( m_MaxSpeed );

		m_VelocityVector = avoidvector;

		SetAIState(AIState_Depart);
	}

	uint32 color;
	if (intersected)
		color = 0x80800000;
	else
		color = 0x80008000;

//	m_pWPhysState->Debug_RenderAABB(CBox3Dfp32(m_LandPos - CVec3Dfp32(INTERSECT_PROXIMITYRANGE), m_LandPos + CVec3Dfp32(INTERSECT_PROXIMITYRANGE)), color, 1.0f);

	return intersected;
}

void CWObject_Model_Moth::SetTransformation(CVec3Dfp32 _Pos, CVec3Dfp32 _Dir, CVec3Dfp32 _Up)
{
	CVec3Dfp32 Left;

	// Calculate orthoginal left and up vectors.
	_Dir.CrossProd(_Up, Left);
	_Dir.CrossProd(Left, _Up);

	CMat4Dfp32 LocalToWorld;

	LocalToWorld.Unit();

	// These should not be nessesary, but lets try...
	_Dir.Normalize();
	Left.Normalize();
	_Up.Normalize();

	// Set transformation matrix using orthogonal axis vectors and position.
	_Dir.SetMatrixRow(LocalToWorld, 0);
	Left.SetMatrixRow(LocalToWorld, 1);

	// Compensates for ill rotated model.
//	(-dir).SetMatrixRow(LocalToWorld, 1);
//	(-left).SetMatrixRow(LocalToWorld, 0);

	_Up.SetMatrixRow(LocalToWorld, 2);
	_Pos.SetMatrixRow(LocalToWorld, 3);

//	m_pWPhysState->Debug_RenderMatrix(LocalToWorld, 1.0f);
	
	SetPosition(LocalToWorld);
}

/*void CWObject_Model_Moth::UpdateWingParams()
{
	SetUserData(m_WingStateTime, 0);
	SetUserData(m_WingStateFreq, 1);
	SetUserData(m_TimeScale, 2);
	SetUserData(m_AIState, 3);
	SetUserData(m_LandedWingSpreadDuration, 4);
}*/

fp32 CWObject_Model_Moth::GetWingAnimTime(fp32 _WingStateTime, fp32 _WingStateFreq, fp32 _TimeScale, 
										 CWObject_Model_Moth::AIState _AIState, fp32 _LandedWingSpreadDuration,
										 fp32 _Interpolated)
{
	fp32 time = _WingStateTime + _WingStateFreq * _Interpolated * _TimeScale;
	fp32 xi = TruncToInt(time);
	fp32 xf = time - xi;

	if (xf < 0.5f)
		xf = xf * 2.0f;
	else
		xf = (1.0f - xf) * 2.0f;

	if (_AIState == AIState_Landed)
	{
		xf *= 1.0f / _LandedWingSpreadDuration;
		if (xf > 1.0f)
			xf = 1.0f;
	}

	return xf;
}

void CWObject_Model_Moth::MorphLandedState(fp32 _Morph, CVec3Dfp32& _Pos, CVec3Dfp32& _Dir, CVec3Dfp32& _Up)
{
	if (_Morph == 0.0f)
		return;

	fp32 imorph = 1.0f - _Morph;

	if (m_LandNormal.k[2] > 0.5f)
	{ // Horizontal surface
		if (_Morph == 1.0f)
		{
			_Pos = m_LandPos;
			_Up = -m_LandNormal;
		}
		else
		{
			_Pos = _Pos * imorph + m_LandPos * _Morph;
			_Up = _Up * imorph - m_LandNormal * _Morph;
			_Up.Normalize();
		}

		CVec3Dfp32 Left;
		_Up.CrossProd(m_LandDir, Left);
		Left.CrossProd(_Up, _Dir);
		_Dir.Normalize();
	}
	else
	{ // Vertical surface
		if (_Morph == 1.0f)
		{
			_Pos = m_LandPos;
			_Dir = -_Up;
			_Up = -m_LandNormal;
		}
		else
		{
			_Pos = _Pos * imorph + m_LandPos * _Morph;
			_Dir = m_LandDir * imorph - _Up * _Morph;
			_Up = _Up * imorph - m_LandNormal * _Morph;

			_Dir.Normalize();
			_Up.Normalize();
		}
	}
}

void CWObject_Model_Moth::OnRefresh()
{
	MAUTOSTRIP(CWObject_Model_Moth_OnRefresh, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_Model_Moth::OnRefresh, MOTH);

	fp32 deltatime = m_pWServer->GetGameTickTime();
//	m_pWPhysState->Debug_RenderWire(m_SpawnPos, m_OldPos, 0x20808080);

	deltatime *= m_TimeScale;

	m_LifeTime += deltatime;
	m_StateTime += deltatime;

	CVec3Dfp32 pos, dir, up;
	fp32 Speed, invSpeed;

	pos = m_OldPos;

	invSpeed = m_VelocityVector.LengthInv();
	Speed = 1.0f / invSpeed;
	dir = m_VelocityVector * (1.0f * invSpeed);

	up = CVec3Dfp32(0, 0, -1);

	bool keepstate = (m_StateTime <= m_WantedStateTime);
	fp32 statefraction = (m_StateTime / m_WantedStateTime);
	fp32 morphstate = 0.0f;

//	ConOut(CStrF("(%d/%d) AIState = %i, StateTime = %f/%f, Velocity = %f", m_ButterflyIndex, ms_NumButterflies, m_AIState, m_StateTime, m_WantedStateTime, velocity));

	switch (m_AIState)
	{
		case AIState_Flying:
		{
			morphstate = 0.0f;
			m_WingStateFreq = m_FlyingFlutterFreq;

			if (m_TraceDelay > 0)
			{
				fp32 movementfraction = (fp32)(m_TraceDelay) / (fp32)(CWOBJECT_MODEL_MOTH_TRACEDELAY + 1);
				pos = m_OldPos + m_VelocityVector * (1.0f - movementfraction);
				m_TraceDelay--;
			}
			else
			{
//				ConOut("...");

				// Do new trace and see where we end up....

				m_TraceDelay = CWOBJECT_MODEL_MOTH_TRACEDELAY;

				m_OldPos += m_VelocityVector;
				pos = m_OldPos;

				CVec3Dfp32 OldVelocityVector = m_VelocityVector;

				CVec3Dfp32 acceleration;
				acceleration.k[0] = GetRand() - 0.5f;
				acceleration.k[1] = GetRand() - 0.5f;
				acceleration.k[2] = GetRand() - 0.5f;
				acceleration.Normalize();
				acceleration.k[1] *= 0.1f;
				acceleration *= m_MinAcceleration + (m_MaxAcceleration - m_MinAcceleration) * GetRand();

//				m_VelocityVector += acceleration;
//				m_VelocityVector += acceleration * (velocity / MAX_VELOCITY);
				m_VelocityVector += acceleration * ((Speed * m_InvMaxSpeed) * m_AccelerationSpeedDependency + (1.0f - m_AccelerationSpeedDependency));

				invSpeed = m_VelocityVector.LengthInv();
				Speed = 1.0f / invSpeed;
				dir = m_VelocityVector * (1.0f * invSpeed);

				// Only apply accelerations when not avoiding.
				if (!m_Avoid)
				{
					if (!keepstate && (dir.k[2] > -m_MinDescendSlope))
					{
						m_VelocityVector.k[2] -= m_DescendAcceleration;
//						ConOut("Descending");
					}
					else if (m_CorrectAltitude)
					{
						if (m_AltitudeDelay == 0)
						{
							m_AltitudeDelay = m_AltitudeDelayValue;

							SurfaceInfo surface;
							if (TraceRay(pos, up, m_WantedAltitude * 2.0f, surface))
							{
								if (surface.m_Distance < m_WantedAltitude)
								{
									m_VelocityVector.k[2] += m_AltitudeAcceleration;
//									ConOut("Adjusting altitude up");
								}
								else
								{
									m_VelocityVector.k[2] -= m_AltitudeAcceleration;
//									ConOut("Adjusting altitude down");
								}
							}
							else
							{
								m_VelocityVector.k[2] -= m_AltitudeAcceleration;
//								ConOut("Adjusting altitude down");
							}
						}
						else
						{
							m_AltitudeDelay--;
						}
					}

					// Don't allow speedup if flatten is needed.
					if (dir.k[2] > m_MaxSlope)
					{
						m_VelocityVector.k[2] *= (1.0f - m_FlattenFactor);
						m_VelocityVector.k[2] -= m_FlattenAcceleration;
//						ConOut("Flattening down");
					}
					else if (dir.k[2] < -m_MaxSlope)
					{
						m_VelocityVector.k[2] *= (1.0f - m_FlattenFactor);
						m_VelocityVector.k[2] += m_FlattenAcceleration;
//						ConOut("Flattening up");
					}
					else if (Speed < m_MinSpeed)
					{
						m_VelocityVector += dir * m_FreespaceAcceleration;
//						ConOut("Speeding up");
					}

				}
				else
				{
					/*
					ConOutL(CStrF("Velo = <%f, %f, %f>, Avoid = <%f, %f, %f>", 
								m_VelocityVector.k[0],
								m_VelocityVector.k[1],
								m_VelocityVector.k[2],
								m_AvoidVector.k[0],
								m_AvoidVector.k[1],
								m_AvoidVector.k[2]));
					*/

					m_VelocityVector += m_AvoidVector;
					m_AvoidVector = CVec3Dfp32(0.0f);
					m_Avoid = false;
				}
				
				invSpeed = m_VelocityVector.LengthInv();
				Speed = 1.0f / invSpeed;
				// Hmm, this should be morphed, very noticable when departing (ie 180 degree
				// turnaround in a couple of frames), should still be fast but not that fast...
				dir = m_VelocityVector * (1.0f * invSpeed);

				if (Speed > m_MaxSpeed)
					Speed = m_MaxSpeed;

				m_VelocityVector = dir * Speed;

				bool movement_secure = false;
				int movement_scans = 0;
				SurfaceInfo surface;

				while ((!movement_secure) && (movement_scans < m_MaxMovementScans))
				{
//					Debug_RenderVector(m_OldPos, m_VelocityVector * 1.0f, 0xFF000080, 5.0f, true);

					if (TraceRay(pos, dir, m_TraceProximityRange, surface))
					{
						if ((!keepstate) && (surface.m_CanLand) &&
							(surface.m_Normal.k[2] >= (1.0f - m_MaxLandingSlope)))
						{
							// Trying to land
							if (surface.m_Distance < m_MaxLandingDistance)
							{
								m_LandPos = surface.m_Pos;
								m_LandDir = dir;
								m_LandNormal = surface.m_Normal;
								SetAIState(AIState_Land);
							}
							else
							{
//								ConOut("Distant landsurface spotted...proceeding...");
							}
							movement_secure = true;
						}
						else
						{
							if (surface.m_Distance < Speed)
							{
								CVec3Dfp32 avoidvector = surface.m_Normal * m_CloseAvoidFactor * (Speed * m_InvMaxSpeed);
								m_VelocityVector = surface.m_Pos + avoidvector - pos;
								invSpeed = m_VelocityVector.LengthInv();
								dir = m_VelocityVector * (1.0f * invSpeed);

//								m_pWPhysState->Debug_RenderWire(surface.m_Pos, surface.m_Pos + avoidvector * 20.0f, 0xFF800000, 5.0f, true);
							}
							else
							{
								m_AvoidVector = surface.m_Normal * m_DistantAvoidFactor * (Speed * m_InvMaxSpeed) * (1.0f - surface.m_Distance / m_TraceProximityRange);
								m_Avoid = true;

								movement_secure = true;

//								m_pWPhysState->Debug_RenderWire(surface.m_Pos, surface.m_Pos + m_AvoidVector * 20.0f, 0xFF800080, 5.0f, true);
							}
						}
					}
					else
					{
						movement_secure = true;
					}

					movement_scans++;
				}

				if (movement_scans == m_MaxMovementScans)
				{
					m_VelocityVector = -OldVelocityVector;
//					ConOut("Reversing");
				}
			}
			break;
		}
		case AIState_Land:
		{
			morphstate = statefraction;
			m_WingStateFreq = m_MorphFlutterFreq;
			if (!keepstate)
				SetAIState(AIState_Landed);

			break;
		}

		case AIState_Depart:
		{
			morphstate = 1.0f - statefraction;
			m_WingStateFreq = m_MorphFlutterFreq;
			if (!keepstate)
				SetAIState(AIState_Flying);
		
			break;
		}

		case AIState_Landed:
		{
			morphstate = 1.0f;
			m_WingStateFreq = m_LandedFlutterFreq;
			// Check if a charater is nearby, if so depart (scared)
			if (!keepstate)
				SetAIState(AIState_Depart);
			else
			{
				// Check character intersection every 8th tick.  -mh
				int CheckIntervalTime = m_pWServer->GetGameTick() + m_iObject;	// This will make different moths do checks on different ticks..  most of the time
				if ((CheckIntervalTime & 7) == 0)
					CheckIntersection();
			}
			
			break;
		}
		case AIState_Smashed:
		case AIState_Escape:
			break;
	}

	m_WingStateTime += m_WingStateFreq * deltatime;

	CVec3Dfp32 pos_noise, dir_noise;

	if (m_ApplyFlutterAscention && (m_AIState == AIState_Flying))
	{
		pos.k[2] += m_FlutterAscention * (1.0f - GetWingAnimTime(0));
	}

	if (m_NoiseMovement)
	{
		CVec3Dfp32 left;
		CVec3Dfp32 up2;

		// Calculate orthoginal left and up vectors.
		dir.CrossProd(up, left);
		dir.CrossProd(left, up2);

		pos_noise = (up2 * 2.0f * (GetSinRandTable(m_LifeTime, m_MovementNoiseTimeScale) - 0.5f) + left * 2.0f * (GetSinRandTable(m_LifeTime + 0.5f, m_MovementNoiseTimeScale) - 0.5f));
		pos += pos_noise * m_PositionNoise;

		dir_noise = (up2 * GetSinRandTableSlope(m_LifeTime, m_MovementNoiseTimeScale) + left * GetSinRandTableSlope(m_LifeTime + 0.5f, m_MovementNoiseTimeScale));
		dir += dir_noise * m_DirectionNoise;
		dir.Normalize();
	}

	if (m_FilterDirection)
	{
		m_LastDir = m_LastDir * (1.0f - m_DirectionFilterLeak) + dir * m_DirectionFilterLeak;
		m_LastDir.Normalize();
		dir = m_LastDir;
	}

	MorphLandedState(morphstate, pos, dir, up);

	if ((m_LimitFacingSlope) && (m_AIState == AIState_Flying))
	{
		if (dir.k[2] > m_MaxFacingSlope)
			dir.k[2] = m_MaxFacingSlope;
		if (dir.k[2] < -m_MaxFacingSlope)
			dir.k[2] = -m_MaxFacingSlope;

		// Scale x, y to get unit length. Can't just normalize (z may still be too dominant).
		fp32 temp = M_Sqrt((1.0f - dir.k[2] * dir.k[2]) / (dir.k[0] * dir.k[0] + dir.k[1] * dir.k[1]));
		dir.k[0] *= temp;
		dir.k[1] *= temp;
	}

	SetTransformation(pos, dir, up);

	// Update wing parameters for the client
	//UpdateWingParams();

	// Set AI State to position 6 (think it's unused), so that the client is aware of what 
	// state we are in...
	SetUserData(m_AIState,6);

	// Call base class refresh
	CWObject_Ext_Model::OnRefresh();
}


void CWObject_Model_Moth::OnCreate()
{
	// Check if the rand table has been created
	if (!ms_SinRandTableInitialized)
		CreateSinRandTable();

	// Set default values
	SetDefaultValues();

	m_pWServer->Object_SetPhysics_ObjectFlags(m_iObject, GetPhysState().m_ObjectFlags | OBJECT_FLAGS_PICKUP);

	m_Randseed = m_iObject;

	CWObject_Ext_Model::OnCreate();
}

void CWObject_Model_Moth::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	int KeyValuei = KeyValue.Val_int();

	const fp32 _Valuef = _pKey->GetThisValuef();
	
	switch (_KeyHash)
	{
	case MHASH4('LAND','EDAN','IMAT','ION'): // "LANDEDANIMATION"
		{
			CWObject_Message Msg(OBJMSG_GAME_RESOLVEANIMHANDLE);
			Msg.m_pData = (void*)(KeyValue.Str());
			//m_Data[0] = m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
			SetUserData((int32)m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex()), 0);
		
			//ConOut(CStrF("Moth Animation ID: %d",m_Data[7]));
			break;
		}
	case MHASH3('ANIM','ATIO','N'): // "ANIMATION"
		{
			CWObject_Message Msg(OBJMSG_GAME_RESOLVEANIMHANDLE);
			Msg.m_pData = (void*)(KeyValue.Str());
			//m_Data[1] = m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
			SetUserData((int32)m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex()), 1);
		
			//ConOut(CStrF("Moth Animation ID: %d",m_Data[7]));
			break;
		}
	case MHASH3('TRAC','EDEL','AY'): // "TRACEDELAY"
		{
			m_TraceDelay = KeyValuei;
			break;
		}
	case MHASH4('INTE','RSEC','TDEL','AY'): // "INTERSECTDELAY"
		{
			m_IntersectDelay = KeyValuei;
			break;
		}
	case MHASH4('ALTI','TUDE','DELA','Y'): // "ALTITUDEDELAY"
		{
			m_AltitudeDelayValue = KeyValuei;
			break;
		}
	case MHASH5('MAX_','MOVE','MENT','_SCA','NS'): // "MAX_MOVEMENT_SCANS"
		{
			m_MaxMovementScans = KeyValuei;
			break;
		}
	case MHASH5('TRAC','E_PR','OXIM','ITYR','ANGE'): // "TRACE_PROXIMITYRANGE"
		{
			m_TraceProximityRange = _Valuef;
			break;
		}
	case MHASH6('INTE','RSEC','T_PR','OXIM','ITYR','ANGE'): // "INTERSECT_PROXIMITYRANGE"
		{
			m_IntersectProximityRange = _Valuef;
			break;
		}
	case MHASH5('CLOS','E_AV','OIDF','ACTO','R'): // "CLOSE_AVOIDFACTOR"
		{
			m_CloseAvoidFactor = _Valuef;
			break;
		}
	case MHASH5('DIST','ANT_','AVOI','DFAC','TOR'): // "DISTANT_AVOIDFACTOR"
		{
			m_DistantAvoidFactor = _Valuef;
			break;
		}
	case MHASH3('MIN_','SPEE','D'): // "MIN_SPEED"
		{
			m_MinSpeed = _Valuef;
			break;
		}
	case MHASH3('MAX_','SPEE','D'): // "MAX_SPEED"
		{
			m_MaxSpeed = _Valuef;
			m_InvMaxSpeed	= 1.0f / _Valuef;
			break;
		}
	case MHASH4('MIN_','ACCE','LERA','TION'): // "MIN_ACCELERATION"
		{
			m_MinAcceleration = _Valuef;
			break;
		}
	case MHASH4('MAX_','ACCE','LERA','TION'): // "MAX_ACCELERATION"
		{
			m_MaxAcceleration = _Valuef;
			break;
		}
	case MHASH8('ACCE','LERA','TION','_VEL','OCIT','Y_DE','PEND','ENCY'): // "ACCELERATION_VELOCITY_DEPENDENCY"
		{
			m_AccelerationSpeedDependency = _Valuef;
			break;
		}
	case MHASH6('FREE','SPAC','E_AC','CELE','RATI','ON'): // "FREESPACE_ACCELERATION"
		{
			m_FreespaceAcceleration = _Valuef;
			break;
		}
	case MHASH5('MIN_','DESC','END_','SLOP','E'): // "MIN_DESCEND_SLOPE"
		{
			m_MinDescendSlope = _Valuef;
			break;
		}
	case MHASH5('DESC','END_','ACCE','LERA','TION'): // "DESCEND_ACCELERATION"
		{
			m_DescendAcceleration = _Valuef;
			break;
		}
	case MHASH3('MAX_','SLOP','E'): // "MAX_SLOPE"
		{
			m_MaxSlope = _Valuef;
			break;
		}
	case MHASH5('FLAT','TEN_','ACCE','LERA','TION'): // "FLATTEN_ACCELERATION"
		{
			m_FlattenAcceleration = _Valuef;
			break;
		}
	case MHASH4('FLAT','TEN_','FACT','OR'): // "FLATTEN_FACTOR"
		{
			m_FlattenFactor = _Valuef;
			break;
		}
	case MHASH5('LAND','_POS','ITIO','N_OF','FSET'): // "LAND_POSITION_OFFSET"
		{
			m_LandPositionOffset = _Valuef;
			break;
		}
	case MHASH5('MAX_','LAND','ING_','DIST','ANCE'): // "MAX_LANDING_DISTANCE"
		{
			m_MaxLandingDistance = _Valuef;
			break;
		}
	case MHASH5('MAX_','LAND','ING_','SLOP','E'): // "MAX_LANDING_SLOPE"
		{
			m_MaxLandingSlope = _Valuef;
			break;
		}
	case MHASH6('DEPA','RT_P','OSIT','ION_','OFFS','ET'): // "DEPART_POSITION_OFFSET"
		{
			m_DepartPositionOffset = _Valuef;
			break;
		}
	case MHASH4('NOIS','E_MO','VEME','NT'): // "NOISE_MOVEMENT"
		{
			m_NoiseMovement = (KeyValue.CompareNoCase("TRUE") == 0);
			break;
		}
	case MHASH6('MOVE','MENT','_NOI','SE_T','IMES','CALE'): // "MOVEMENT_NOISE_TIMESCALE"
		{
			m_MovementNoiseTimeScale = _Valuef;
			break;
		}
	case MHASH4('POSI','TION','_NOI','SE'): // "POSITION_NOISE"
		{
			m_PositionNoise = _Valuef;
			break;
		}
	case MHASH4('DIRE','CTIO','N_NO','ISE'): // "DIRECTION_NOISE"
		{
			m_DirectionNoise = _Valuef;
			break;
		}
	case MHASH4('FILT','ER_D','IREC','TION'): // "FILTER_DIRECTION"
		{
			m_FilterDirection = (KeyValue.CompareNoCase("TRUE") == 0);
			break;
		}
	case MHASH6('DIRE','CTIO','N_FI','LTER','_LEA','K'): // "DIRECTION_FILTER_LEAK"
		{
			m_DirectionFilterLeak = _Valuef;
			break;
		}
	case MHASH5('LIMI','T_FA','CING','_SLO','PE'): // "LIMIT_FACING_SLOPE"
		{
			m_LimitFacingSlope = (KeyValue.CompareNoCase("TRUE") == 0);
			break;
		}
	case MHASH4('MAX_','FACI','NG_S','LOPE'): // "MAX_FACING_SLOPE"
		{
			m_MaxFacingSlope = _Valuef;
			break;
		}
	case MHASH5('MIN_','FLYI','NG_D','URAT','ION'): // "MIN_FLYING_DURATION"
		{
			m_MinFlyingDuration = _Valuef;
			break;
		}
	case MHASH5('MAX_','FLYI','NG_D','URAT','ION'): // "MAX_FLYING_DURATION"
		{
			m_MaxFlyingDuration = _Valuef;
			break;
		}
	case MHASH5('MIN_','LAND','ED_D','URAT','ION'): // "MIN_LANDED_DURATION"
		{
			m_MinLandedDuration = _Valuef;
			break;
		}
	case MHASH5('MAX_','LAND','ED_D','URAT','ION'): // "MAX_LANDED_DURATION"
		{
			m_MaxLandedDuration = _Valuef;
			break;
		}
	case MHASH5('MIN_','LAND','_DUR','ATIO','N'): // "MIN_LAND_DURATION"
		{
			m_MinLandDuration = _Valuef;
			break;
		}
	case MHASH5('MAX_','LAND','_DUR','ATIO','N'): // "MAX_LAND_DURATION"
		{
			m_MaxLandDuration = _Valuef;
			break;
		}
	case MHASH5('MIN_','DEPA','RT_D','URAT','ION'): // "MIN_DEPART_DURATION"
		{
			m_MinDepartDuration = _Valuef;
			break;
		}
	case MHASH5('MAX_','DEPA','RT_D','URAT','ION'): // "MAX_DEPART_DURATION"
		{
			m_MaxDepartDuration = _Valuef;
			break;
		}
	case MHASH5('FLYI','NG_F','LUTT','ERFR','EQ'): // "FLYING_FLUTTERFREQ"
		{
			m_FlyingFlutterFreq = _Valuef;
			break;
		}
	case MHASH5('MORP','H_FL','UTTE','RFRE','Q'): // "MORPH_FLUTTERFREQ"
		{
			m_MorphFlutterFreq = _Valuef;
			break;
		}
	case MHASH5('LAND','ED_F','LUTT','ERFR','EQ'): // "LANDED_FLUTTERFREQ"
		{
			m_LandedFlutterFreq = _Valuef;
			break;
		}
	case MHASH7('LAND','ED_W','INGS','PREA','D_DU','RATI','ON'): // "LANDED_WINGSPREAD_DURATION"
		{
			m_LandedWingSpreadDuration = _Valuef;
			break;
		}
	case MHASH4('CORR','ECT_','ALTI','TUDE'): // "CORRECT_ALTITUDE"
		{
			m_CorrectAltitude = (KeyValue.CompareNoCase("TRUE") == 0);
			break;
		}
	case MHASH4('WANT','ED_A','LTIT','UDE'): // "WANTED_ALTITUDE"
		{
			m_WantedAltitude = _Valuef;
			break;
		}
	case MHASH6('ALTI','TUDE','_ACC','ELER','ATIO','N'): // "ALTITUDE_ACCELERATION"
		{
			m_AltitudeAcceleration = _Valuef;
			break;
		}
	case MHASH6('APPL','Y_FL','UTTE','R_AS','CENT','ION'): // "APPLY_FLUTTER_ASCENTION"
		{
			m_ApplyFlutterAscention = (KeyValue.CompareNoCase("TRUE") == 0);
			break;
		}
	case MHASH5('FLUT','TER_','ASCE','NTIO','N'): // "FLUTTER_ASCENTION"
		{
			m_FlutterAscention = _Valuef;
			break;
		}
	case MHASH3('MODE','L_SI','ZE'): // "MODEL_SIZE"
		{
			m_ModelSize = _Valuef;
			break;
		}
	case MHASH3('TIME','SCAL','E'): // "TIMESCALE"
		{
			m_TimeScale = _Valuef;
			break;
		}
	case MHASH2('OPTI','ONS'): // "OPTIONS"
		{
			static const char* spFlags[] = { "NoShadow",NULL};
			m_Options = KeyValue.TranslateFlags(spFlags);
			break;
		}
	default:
		{
		// Do base class
			break;
		}
	}
		CWObject_Ext_Model::OnEvalKey(_KeyHash, _pKey);
}

void CWObject_Model_Moth::OnFinishEvalKeys()
{
	// Set "old" position
	m_OldPos = GetPosition();
	// Update visibility
	UpdateVisibility();

	// Do base class as well
	CWObject_Ext_Model::OnFinishEvalKeys();
}

void CWObject_Model_Moth::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	CWObject_Ext_Model::OnIncludeTemplate(_pReg,_pMapData,_pWServer);

	// Include moth pickup
	CRPG_Object::IncludeRPGClass("pickup_moth", _pMapData, _pWServer, true);	
}

aint CWObject_Model_Moth::OnMessage(const CWObject_Message& _Msg)
{
	// Take care of assorted messages, for example if a character picks
	// up the moth and puts it in the inventory
	switch (_Msg.m_Msg)
	{
	case OBJMSG_RPG_AVAILABLEPICKUP:
		{
			CFStr Pickup = "pickup_moth";
			TPtr<CRPG_Object_Item> spMoth = (CRPG_Object_Item*)(CRPG_Object*)CRPG_Object::CreateObject(Pickup, m_pWServer);
			if (spMoth)
			{
				CWObject_Message Msg(OBJMSG_RPG_AVAILABLEPICKUP, (spMoth->m_iItemType), aint((CRPG_Object_Item *)(CReferenceCount *)spMoth));
				
				Msg.m_pData = (char *)Pickup;
				Msg.m_DataSize = Pickup.GetMax();
				Msg.m_iSender = m_iObject;

				// destroy this object, if successful pickup
				if (m_pWServer->Message_SendToObject(Msg, _Msg.m_Param0))
					Destroy();
			}
		}
	case OBJMSG_RPG_ISPICKUP:
		{
			return true;
		}
	case OBJMSG_MOTH_SETSHADOW:
		{
			if (_Msg.m_Param0)
				m_Options |= MOTH_OPTION_NOSHADOW;
			else
				m_Options &= ~MOTH_OPTION_NOSHADOW;
			UpdateVisibility();
			return true;
		}
	case OBJMSG_MOTH_TOGGLESHADOW:
		{
			if (m_Options & MOTH_OPTION_NOSHADOW)
				m_Options &= ~MOTH_OPTION_NOSHADOW;
			else
				m_Options |= MOTH_OPTION_NOSHADOW;
			UpdateVisibility();
			return true;
		}
	default:
		break;
	}
	return CWObject_Ext_Model::OnMessage(_Msg);
}

void CWObject_Model_Moth::UpdateVisibility(int *_lpExtraModels, int _nExtraModels)
{
	// Mmmkay, do base first then remove cast shadow flag (if any)
	CWObject_Ext_Model::UpdateVisibility(_lpExtraModels,_nExtraModels);
	if(!(m_Options & MOTH_OPTION_NOSHADOW) && !(m_ClientFlags & CWO_CLIENTFLAGS_SHADOWCASTER))
		ClientFlags() |= CWO_CLIENTFLAGS_SHADOWCASTER;
	else if((m_Options & MOTH_OPTION_NOSHADOW) && (m_ClientFlags & CWO_CLIENTFLAGS_SHADOWCASTER))
		ClientFlags() &= ~CWO_CLIENTFLAGS_SHADOWCASTER;
}

aint CWObject_Model_Moth::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	if (_pObj == NULL)
	{
		ConOutL("CWObject_Model_Moth::OnClientMessage() - Invalid object pointer (_pObj == NULL).");
		return 0;
	}

	switch (_Msg.m_Msg)
	{
		case OBJMSG_SPELLS_GETANIMSTATE:
		{
			if (_Msg.m_pData == NULL)
			{
				ConOutL("CWObject_Model_Moth::OnClientMessage() - Invalid data pointer (_Msg.m_pData == NULL).");
				return 0;
			}

			CXR_AnimState* pAnimState = (CXR_AnimState*)(((void**)_Msg.m_pData)[0]);
			CMat4Dfp32* pMatrix = (CMat4Dfp32*)(((void**)_Msg.m_pData)[1]);
			CMTime *pAnimTime = (CMTime *)_Msg.m_Param1;
			int iModel = _Msg.m_Param0;

			if (pAnimState == NULL)
			{
				ConOutL(CStr("CWObject_Model_Moth::OnClientMessage() - Invalid data pointer (pAnimState == NULL)."));
				return 0;
			}

			if (pMatrix == NULL)
			{
				ConOutL(CStr("CWObject_Model_Moth::OnClientMessage() - Invalid data pointer (pMatrix == NULL)."));
				return 0;
			}

			if ((iModel < 0) || (iModel >= CWO_NUMMODELINDICES))
			{
				ConOutL(CStrF("CWObject_Model_Moth::OnClientMessage() - Invalid model index %d (iObj %d).", iModel, _pObj->m_iObject));
				return 0;
			}

			CWObject_Ext_Model::GetDefaultAnimState(_pObj, _pWClient, *pAnimState, *pAnimTime, iModel);

			// BoneAnimated Specifics...
			{
				AIState MothState = AIState_Landed;
				GetUserData(_pObj, MothState, 6);
				int32 iAnim = -1;
				if (MothState == AIState_Landed || MothState == AIState_Smashed)
					GetUserData(_pObj, iAnim, 0);
				else
					GetUserData(_pObj, iAnim, 1);

				CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[iModel]);
				if (pModel == NULL)
					return 0;

				CXR_Skeleton* pSkeleton = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);

				// Return success, for we have a model, but it has no skeleton, and is thus not skeleton animated.
				// Though the AnimState is properly default initialised for a regular non-skeleton-animated model.
				if (pSkeleton == NULL)
					return 1; 

				CReferenceCount* RefCount = (CReferenceCount*)_pObj->m_lspClientObj[iModel];
				pAnimState->m_pSkeletonInst = safe_cast<CXR_SkeletonInstance>(RefCount);
				if (pAnimState->m_pSkeletonInst == NULL)
				{
					// Something was already allocated, but it wasn't a SkeletonInstance.
					// I think this should be treated as an error. Let's at least return 0.
					if (RefCount != NULL)
						return 0;

					MRTC_SAFECREATEOBJECT_NOEX(spSI, "CXR_SkeletonInstance", CXR_SkeletonInstance);
					pAnimState->m_pSkeletonInst = spSI;

					if (pAnimState->m_pSkeletonInst == NULL)
						return 0;

					_pObj->m_lspClientObj[iModel] = pAnimState->m_pSkeletonInst;

				}
				
				pAnimState->m_pSkeletonInst->Create(pSkeleton->m_lNodes.Len());
				
				CWObject_Message Msg(OBJMSG_GAME_GETANIMFROMHANDLE, iAnim);
				CXR_Anim_SequenceData* pAnim = (CXR_Anim_SequenceData*)_pWClient->Phys_Message_SendToObject(Msg, _pWClient->Game_GetObjectIndex());
				if (pAnim == NULL)
					return 0;

				CXR_AnimLayer Layer;
				Layer.Create3(pAnim, pAnim->GetLoopedTime(pAnimState->m_AnimTime0), 1.0f, 1.0f, 0, 0);

				CMat4Dfp32 MatrixEval = *pMatrix;
				pSkeleton->EvalAnim(&Layer, 1, pAnimState->m_pSkeletonInst, MatrixEval, 0);
			}

			return 1;
		}
		break;
	}

	return CWObject_Ext_Model::OnClientMessage(_pObj, _pWClient, _Msg);
}

CVec3Dfp32 CWObject_Model_Moth::InterpolateVector(const CVec3Dfp32& _V1, const CVec3Dfp32& _V2, const fp32& _Scale)
{
	// Linearly interpolate between the two vectors
	return _V1 + (_V2 - _V1) * _Scale;
}

void CWObject_Model_Moth::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	CMTime AnimTime;
	fp32 TickFrac;
	GetAnimTime(_pObj, _pWClient, AnimTime, TickFrac);

	CMat4Dfp32 Matrix;
	if (!GetRenderMatrix(_pObj, _pWClient, Matrix, TickFrac))
		return;


	CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if (pModel)
	{
		CXR_AnimState AnimState;
		if (GetAnimState(_pObj, _pWClient, AnimState, Matrix, AnimTime, TickFrac, 0))
			_pEngine->Render_AddModel(pModel, Matrix, AnimState);
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Model_Moth, CWObject_Ext_Model, 0x0100);


/*// Get the position
CVec3Dfp32 TempPos;
TempPos = CVec3Dfp32::GetMatrixRow(GetPositionMatrix(),3);

// Move moth to next target
m_CurrentTimeTick += SERVER_TIMEPERFRAME;
// Check if we need to find a new target position
if (m_CurrentTimeTick >= m_TimeTicks)
{
	// Start position is previous target position
	m_StartPosition = m_TargetPosition;

	// Find new target position and move towards it
	CVec3Dfp32 direction(GetRand() - GetRand(), GetRand() - GetRand(), 0);
	direction.Normalize();
	m_TargetPosition = m_StartPosition + direction * m_LengthBetweenTargets;
	
	m_TimeTicks = 0.5f;
	m_CurrentTimeTick = 0.0f;
}

// Set new position
CVec3Dfp32 Result = InterpolateVector(m_StartPosition, m_TargetPosition, m_CurrentTimeTick / m_TimeTicks);
SetPosition(Result);
//TempPos + (m_TargetPosition - TempPos).Normalize() * SERVER_TIMEPERFRAME);
*/

/*fp32 AnimTime;//, TickFrac;
	//GetAnimTime(_pObj, _pWClient, AnimTime, TickFrac);
	AnimTime = GetWingAnimTime(_pWClient->GetInterpolateTime());

	CMat4Dfp32 Matrix;
	if (!GetRenderMatrix(_pObj, _pWClient, Matrix, TickFrac))
		return;

	CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);

	// Interpolate between server frames...
	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetInterpolateTime() / fp32(SERVER_TIMEPERFRAME);

	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);
	_pWClient->Object_SetPosition(_pObj->m_iObject, _pObj->GetPositionMatrix());

	if (pModel)
	{
		CXR_AnimState AnimState(AnimTime, 10, _pObj->m_iAnim1, &_pObj->m_lspClientObj[0]);

		_pEngine->Render_AddModel(pModel, MatIP, AnimState);
	}*/
