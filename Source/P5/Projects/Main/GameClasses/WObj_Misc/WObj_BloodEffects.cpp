/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WObj_BloodEffects.cpp

Author:			

Copyright:		2006 Starbreeze Studios AB

Contents:		CWObject_BloodEffect
				CWObject_BloodSystem

Comments:

History:
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_BloodEffects.h"


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_BloodEffect, CWObject_Ext_Model, 0x1000);


const char* CWObject_BloodEffect::m_slBloodSystemFlagsTranslate[] = { "SPURT", "CLOUD", "ENTRANCE", "EXIT", "MODELS", "0x0020", "0x0040", "0x0080", "0x0100", "0x0200", "0x0400", "0x0800", "0x1000", "0x2000", "0x4000", "CINFO", NULL };


CDataPacker<0,0x0000FFFF,0>		CWObject_BloodEffect::m_SystemType;
CDataPacker<0,0x0000FFFF,16>	CWObject_BloodEffect::m_SpurtType;
CDataPacker<1,0x0000FFFF,0>		CWObject_BloodEffect::m_CloudType;
CDataPacker<1,0x00003FFF,16>	CWObject_BloodEffect::m_iOwnerData;
CDataPacker<2,0xFFFFFFFF,0>		CWObject_BloodEffect::m_Seed;
CDataPacker<3,0x0000FFFF,0>		CWObject_BloodEffect::m_iBloodModel;
CDataPacker<4,0xFFFFFFFF,0>		CWObject_BloodEffect::m_DecalScaleEntrance;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_BloodEffect_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWO_BloodEffect_ClientData::CWO_BloodEffect_ClientData()
	: m_pWClient(NULL)
	, m_pObj(NULL)
	, m_pBloodModel(NULL)
	, m_CreationTick(0)
	, m_nActivePoints(0)
	, m_nDecalPointsEntrance(0)
	, m_nDecalPointsExit(0)
	, m_nHelpPointsEntrance(0)
	, m_nHelpPointsExit(0)
{
	m_lDecalPoints.Clear();
	m_lHelpPoints.Clear();
	m_lDecalPoints.Clear();
}


CWO_BloodEffect_ClientData::~CWO_BloodEffect_ClientData()
{
}


void CWO_BloodEffect_ClientData::Clear(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, const int16 _iBloodModel)
{
	m_pWClient = _pWClient;
	m_pObj = _pObj;
	
	m_lDecalPoints.Clear();
	m_lHelpPoints.Clear();

	m_CreationTick = m_pWClient->GetGameTick();
	m_pBloodModel = (_iBloodModel) ? m_pWClient->GetMapData()->GetResource_Model(_iBloodModel) : NULL;
}


void CWO_BloodEffect_ClientData::OnClientUpdate(const uint8*& _pD, const uint16 _SystemFlags, const uint16 _SpurtFlags, const uint16 _CloudFlags)
{
	// Unpack exit hole
	m_ExitHole = 0.0f;
	if (_SystemFlags & OBJ_BLOODSYS_FLAGS_CINFO)
	{
		PTR_GETFP32(_pD, m_ExitHole.k[0]);
		PTR_GETFP32(_pD, m_ExitHole.k[1]);
		PTR_GETFP32(_pD, m_ExitHole.k[2]);
	}

	if (_SystemFlags & OBJ_BLOODSYS_FLAGS_SPURT)
	{
		PTR_GETFP32(_pD, m_Strength1);
		PTR_GETFP32(_pD, m_Strength2);
		PTR_GETFP32(_pD, m_Strength3);
		PTR_GETFP32(_pD, m_Strength4);

		// Unpack entrance points
		m_nDecalPointsEntrance = 0;
		m_nHelpPointsEntrance = 0;
		if (_SpurtFlags & OBJ_BLOODSYS_FLAGS_ENTRANCE)
		{
			PTR_GETUINT8(_pD, m_nDecalPointsEntrance);
			PTR_GETUINT8(_pD, m_nHelpPointsEntrance);
		}

		// Unpack exit points
		m_nDecalPointsExit = 0;
		m_nHelpPointsExit = 0;
		if (_SpurtFlags & OBJ_BLOODSYS_FLAGS_EXIT)
		{
			PTR_GETUINT8(_pD, m_nDecalPointsExit);
			PTR_GETUINT8(_pD, m_nHelpPointsExit);
		}
		
		// Unpack wall/floor marks
		m_lDecalData.SetLen(m_nDecalPointsEntrance + m_nDecalPointsExit);
		TAP_RCD<SDecalData> lDecalData = m_lDecalData;
		for (uint i = 0; i < lDecalData.Len(); i++)
		{
			SDecalData& DecalData = lDecalData[i];

			PTR_GETINT16(_pD, DecalData.m_WallSurfaceID);
			PTR_GETINT16(_pD, DecalData.m_FloorSurfaceID);
			PTR_GETUINT8(_pD, DecalData.m_WallSize);
			PTR_GETUINT8(_pD, DecalData.m_FloorSize);
		}
	}
}


void CWO_BloodEffect_ClientData::AllocSpurtPoints(const CMat4Dfp32& _PosMat, const CVec3Dfp32& _ExitPos, CRand_MersenneTwister* pRand)
{
	if (m_lDecalPoints.Len() == 0 && m_lHelpPoints.Len() == 0)
	{
		const CVec3Dfp32& HitPos = _PosMat.GetRow(3);
		const CVec3Dfp32& MaxUp = _PosMat.GetRow(2);
		const CVec3Dfp32  MinUp = -MaxUp;
		const CVec3Dfp32& MaxLeft = _PosMat.GetRow(1);
		const CVec3Dfp32  MinLeft = -MaxLeft;
		const CVec3Dfp32& HitDir = _PosMat.GetRow(0);
		CVec3Dfp32 NegHitDir = -HitDir;
		
		m_lDecalPoints.SetLen(m_nDecalPointsEntrance + m_nDecalPointsExit);
		m_lHelpPoints.SetLen(m_nHelpPointsEntrance + m_nHelpPointsExit);

		m_nActivePoints = m_lDecalPoints.Len() + m_lHelpPoints.Len();

		TAP_RCD<SSpurtPoint> lDecalPoints = m_lDecalPoints;
		TAP_RCD<SSpurtPoint> lHelpPoints = m_lHelpPoints;

		// Entrance strength
		static fp32 DebugStr1 = 64.0f;
		static fp32 DebugStr2 = 32.0f;

		// Exit strength
		static fp32 DebugStr3 = 256.0f;
		static fp32 DebugStr4 = 75.0f;

		static int sParamCtrl = 1;
		if (!sParamCtrl)
		{
			m_Strength1 = DebugStr1;
			m_Strength2 = DebugStr2;
			m_Strength3 = DebugStr3;
			m_Strength4 = DebugStr4;
		}

		const fp32 TickTime = m_pWClient->GetGameTickTime();
		const fp32 Strength1 = (m_Strength1 * TickTime);
		const fp32 Strength2 = (m_Strength2 * TickTime);
		const fp32 Strength3 = (m_Strength3 * TickTime);
		const fp32 Strength4 = (m_Strength4 * TickTime);
		int32 GameTick = m_pWClient->GetGameTick();

		CVec3Dfp32 LerpUp, LerpLeft, RandDir;

		// Initialize decal points
		for (uint i = 0; i < m_nDecalPointsEntrance; i++)
		{
			SSpurtPoint& DecalPoint = lDecalPoints[i];
			DecalPoint.m_Pos = HitPos;
			DecalPoint.m_Flags = SSpurtPoint::FLAGS_MOVEMENT;

			MaxUp.Lerp(MinUp, pRand->GenRand1Inclusive_fp32(), LerpUp);
			MaxLeft.Lerp(MinLeft, pRand->GenRand1Inclusive_fp32(), LerpLeft);

			DecalPoint.m_Vel =	(LerpUp * (pRand->GenRand1Inclusive_fp32() * Strength2)) + 
								(LerpLeft * (pRand->GenRand1Inclusive_fp32() * Strength2)) +
								(NegHitDir * (((pRand->GenRand1Inclusive_fp32() * 0.5f) + 0.5f) * Strength1));
		}

		for (uint i = m_nDecalPointsEntrance; i < lDecalPoints.Len(); i++)
		{
			SSpurtPoint& DecalPoint = lDecalPoints[i];
			DecalPoint.m_Pos = HitPos;
			DecalPoint.m_Flags = SSpurtPoint::FLAGS_MOVEMENT;

			MaxUp.Lerp(MinUp, pRand->GenRand1Inclusive_fp32(), LerpUp);
			MaxLeft.Lerp(MinLeft, pRand->GenRand1Inclusive_fp32(), LerpLeft);

			DecalPoint.m_Vel =	(LerpUp * (pRand->GenRand1Inclusive_fp32() * Strength4)) + 
								(LerpLeft * (pRand->GenRand1Inclusive_fp32() * Strength4)) +
								(HitDir * (((pRand->GenRand1Inclusive_fp32() * 0.5f) + 0.5f) * Strength3));
		}

		// Initialize help points
		for (uint i = 0; i < m_nHelpPointsEntrance; i++)
		{
			SSpurtPoint& HelpPoint = lHelpPoints[i];
			HelpPoint.m_Pos = HitPos;
			HelpPoint.m_Flags = SSpurtPoint::FLAGS_MOVEMENT;

			MaxUp.Lerp(MinUp, pRand->GenRand1Inclusive_fp32(), LerpUp);
			MaxLeft.Lerp(MinLeft, pRand->GenRand1Inclusive_fp32(), LerpLeft);

			HelpPoint.m_Vel =	(LerpUp * (pRand->GenRand1Inclusive_fp32() * Strength2)) + 
								(LerpLeft * (pRand->GenRand1Inclusive_fp32() * Strength2)) +
								(NegHitDir * (((pRand->GenRand1Inclusive_fp32() * 0.5f) + 0.5f) * Strength1));
		}

		for (uint i = m_nHelpPointsEntrance; i < lHelpPoints.Len(); i++)
		{
			SSpurtPoint& HelpPoint = lHelpPoints[i];
			HelpPoint.m_Pos = HitPos;
			HelpPoint.m_Flags = SSpurtPoint::FLAGS_MOVEMENT;

			MaxUp.Lerp(MinUp, pRand->GenRand1Inclusive_fp32(), LerpUp);
			MaxLeft.Lerp(MinLeft, pRand->GenRand1Inclusive_fp32(), LerpLeft);

			HelpPoint.m_Vel =	(LerpUp * (pRand->GenRand1Inclusive_fp32() * Strength4)) + 
								(LerpLeft * (pRand->GenRand1Inclusive_fp32() * Strength4)) +
								(HitDir * (((pRand->GenRand1Inclusive_fp32() * 0.5f) + 0.5f) * Strength3));
		}
	}
}


void CWO_BloodEffect_ClientData::RefreshSpurtPoints()
{
	TAP_RCD<SSpurtPoint> lDecalPoints = m_lDecalPoints;
	TAP_RCD<SSpurtPoint> lHelpPoints = m_lHelpPoints;
	TAP_RCD<SDecalData> lDecalData = m_lDecalData;

	const fp32 RefreshTime = CMTime::CreateFromTicks((m_pWClient->GetGameTick() - m_CreationTick), m_pWClient->GetGameTickTime()).GetTime();
	const fp32 RefreshTime01 = Clamp01(RefreshTime);

	static fp32 VelocityScale = 0.25f;

	const fp32 VelZ = (VelocityScale * 9.82f * 32.0f * m_pWClient->GetGameTickTime()) * RefreshTime01;
	m_nActivePoints = 0;

//	int16 ColorDec =  (255.0f / m_pWClient->GetGameTicksPerSecond()) * 2;

	int32 GameTick = m_pWClient->GetGameTick();
	fp32 GameTickPerSec = (6.0f / RefreshTime01) / m_pWClient->GetGameTicksPerSecond();

	for (uint i = 0; i < lDecalPoints.Len(); i++)
	{
		SSpurtPoint& DecalPoint = lDecalPoints[i];

		// Check collision with world
		if (DecalPoint.m_Flags & SSpurtPoint::FLAGS_MOVEMENT)
		{
			CVec3Dfp32 NewPos = DecalPoint.m_Pos + DecalPoint.m_Vel;
			
			int Objects = OBJECT_FLAGS_WORLD;
			int Mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;
			
			CCollisionInfo CInfo;
			CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
			CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_TIME | CXR_COLLISIONRETURNVALUE_POSITION);
			
			bool bHit = m_pWClient->Phys_IntersectLine(DecalPoint.m_Pos, NewPos, 0, Objects, Mediums, &CInfo, 0);
			if (bHit && CInfo.m_bIsValid)
			{
				const bool bWall = (M_Fabs(CInfo.m_Plane.n.k[2]) < 0.5f);
				const bool bRotate = !bWall;

				DecalPoint.m_Pos = NewPos;
				DecalPoint.m_Vel.k[0] = 1.0f;
				DecalPoint.m_Vel.k[1] = MaxMT(1.0f - (M_Fabs(DecalPoint.m_Vel.k[2]) / 16.0f), 0.0f);
				DecalPoint.m_Vel.k[2] -= VelZ;
				DecalPoint.m_Flags &= ~(SSpurtPoint::FLAGS_MOVEMENT);
				DecalPoint.m_Flags |= SSpurtPoint::FLAGS_COLLISION;

				// Create decals
				CMat4Dfp32 DecalMat;
				DecalMat.Unit();
				DecalMat.GetRow(0) = CInfo.m_Plane.n;
				DecalMat.GetRow(1) = (bWall) ? CVec3Dfp32(0,0,1) : CVec3Dfp32(0,1,0);
				DecalMat.RecreateMatrix(0, 1);

				Swap(DecalMat.GetRow(0), DecalMat.GetRow(2));
				DecalMat.GetRow(1) = -DecalMat.GetRow(1);
				DecalMat.GetRow(3) = CInfo.m_Pos;

				if (bRotate)
					DecalMat.RotZ_x_M(Random);

				CXR_WallmarkDesc WMD;
				WMD.m_SurfaceID = (bWall) ? lDecalData[i].m_WallSurfaceID : lDecalData[i].m_FloorSurfaceID;
				WMD.m_Size = (bWall) ? lDecalData[i].m_WallSize : lDecalData[i].m_FloorSize;
				WMD.m_SpawnTime = m_pWClient->GetRenderTime();
				m_pWClient->Wallmark_Create(WMD, DecalMat, 4, 0);
			}
			else
			{
				DecalPoint.m_Pos = NewPos;
				DecalPoint.m_Vel.k[2] -= VelZ;
			}
		}
		else if (DecalPoint.m_Flags & SSpurtPoint::FLAGS_COLLISION)
		{
			DecalPoint.m_Vel.k[0] = DecalPoint.m_Vel.k[1];
			DecalPoint.m_Vel.k[1] = MaxMT(DecalPoint.m_Vel.k[1] - (M_Fabs(DecalPoint.m_Vel.k[2]) / 16.0f), 0.0f);
			DecalPoint.m_Vel.k[2] -= VelZ;
		}
	}

	m_nActivePoints += lDecalPoints.Len();
	m_nActivePoints += lHelpPoints.Len();
	for (uint i = 0; i < lHelpPoints.Len(); i++)
	{
		SSpurtPoint& HelpPoint = lHelpPoints[i];
		HelpPoint.m_Pos += HelpPoint.m_Vel;
		HelpPoint.m_Vel.k[2] -= VelZ;
	}
}


CBloodEffectModelParam* CWO_BloodEffect_ClientData::CreateModelData(CXR_VBManager* _pVBM)
{
	uint32 Size1 = FXAlign16(sizeof(CBloodEffectModelParam));
	uint32 Size2 = FXAlign16(sizeof(CVec3Dfp32) * (m_nActivePoints + 2));
	uint32 Size3 = FXAlign16(sizeof(fp32) * (m_nActivePoints));
	uint32 TotalSize = FXAlign16(Size1 + Size2 + Size3);

	uint8* pVBData = (uint8*)_pVBM->Alloc(TotalSize);
	CBloodEffectModelParam* pModelParam = (CBloodEffectModelParam*)pVBData;

	if (pModelParam)
	{
		CVec3Dfp32* pPoints = (CVec3Dfp32*)(pVBData + Size1);
		fp32* pLength = (fp32*)(pVBData + (Size1 + Size2));

		TAP_RCD<SSpurtPoint> lDecalPoints = m_lDecalPoints;
		TAP_RCD<SSpurtPoint> lHelpPoints = m_lHelpPoints;

		const fp32 TickFrac = m_pWClient->GetRenderTickFrac();

		// Setup decal points
		uint iPoints = 0;
		pModelParam->m_pLength = pLength;
		pModelParam->m_pPointData = pPoints;
		pModelParam->m_nPoints[0] = m_nDecalPointsEntrance;
		pModelParam->m_nPoints[1] = m_nDecalPointsExit;
		pModelParam->m_nPoints[2] = m_nHelpPointsEntrance;
		pModelParam->m_nPoints[3] = m_nHelpPointsExit;
		for (uint i = 0; i < lDecalPoints.Len(); i++)
		{
			SSpurtPoint& SpurtPoint = lDecalPoints[i];
			if (SpurtPoint.m_Flags & SSpurtPoint::FLAGS_MOVEMENT)
			{
				SpurtPoint.m_Pos.Lerp(SpurtPoint.m_Pos + SpurtPoint.m_Vel, TickFrac, pPoints[iPoints]);
				pLength[iPoints++] = 1.0f;
			}
			else if (SpurtPoint.m_Flags & SSpurtPoint::FLAGS_COLLISION)
			{
//				// Set point, and store collision time some way
				pPoints[iPoints] = SpurtPoint.m_Pos;
				pLength[iPoints++] = CFXSysUtil::LerpMT(SpurtPoint.m_Vel.k[0], SpurtPoint.m_Vel.k[1], TickFrac);
			}
		}

		// Setup help points
		for (uint i = 0; i < lHelpPoints.Len(); i++)
		{
			SSpurtPoint& SpurtPoint = lHelpPoints[i];
			SpurtPoint.m_Pos.Lerp(SpurtPoint.m_Pos + SpurtPoint.m_Vel, TickFrac, pPoints[iPoints]);
			pLength[iPoints++] = 1.0f;
		}

		// Set Set positions
		pModelParam->m_pEntrance = &pPoints[m_nActivePoints];
		pModelParam->m_pExit = &pPoints[m_nActivePoints + 1];
	}

	return pModelParam;
}


#ifdef BLOODSYSTEM_DEBUG
void CWO_BloodEffect_ClientData::Debug_RenderSpurtPoints()
{
	if (!m_pWClient->Debug_GetWireContainer())
		return;

	TAP_RCD<SSpurtPoint> lDecalPoints = m_lDecalPoints;
	TAP_RCD<SSpurtPoint> lHelpPoints = m_lHelpPoints;

	const fp32 TickFrac = m_pWClient->GetRenderTickFrac();
	CVec3Dfp32 LerpPoint;

	const CPixel32 DecalColor(255, 0, 0);
	const CPixel32 HelpColor(0, 255, 0);
	const fp32 Duration = 0.0f;
	bool bFade = false;

	// Render debug information
	CBox3Dfp32 EntranceBound, ExitBound;
	for (uint i = 0; i < lDecalPoints.Len(); i++)
	{
		SSpurtPoint& DecalPoint = lDecalPoints[i];
		if (DecalPoint.m_Flags & SSpurtPoint::FLAGS_MOVEMENT)
			DecalPoint.m_Pos.Lerp(DecalPoint.m_Pos + DecalPoint.m_Vel, TickFrac, LerpPoint);
		else
			LerpPoint = DecalPoint.m_Pos;

		// Get bounding
		if (i < m_nDecalPointsEntrance)
		{
			if (i != 0)	EntranceBound.Expand(LerpPoint);
			else		EntranceBound = CBox3Dfp32(LerpPoint, LerpPoint);
		}
		else
		{
			if (i != m_nDecalPointsEntrance)	ExitBound.Expand(LerpPoint);
			else							ExitBound = CBox3Dfp32(LerpPoint, LerpPoint);
		}

		m_pWClient->Debug_RenderVertex(LerpPoint, DecalColor, Duration, bFade);
	}
	EntranceBound.Grow(2);
	ExitBound.Grow(2);
	if (m_nDecalPointsEntrance > 0) m_pWClient->Debug_RenderAABB(EntranceBound, CPixel32(255,255,0), 0.0f, false);
	if (m_nDecalPointsExit > 0) m_pWClient->Debug_RenderAABB(ExitBound, CPixel32(255,255,0), 0.0f, false);

	for (uint i = 0; i < lHelpPoints.Len(); i++)
	{
		SSpurtPoint& HelpPoint = lHelpPoints[i];
		HelpPoint.m_Pos.Lerp(HelpPoint.m_Pos + HelpPoint.m_Vel, TickFrac, LerpPoint);

		// Get bounding
		if (i < m_nHelpPointsEntrance)
		{
			if (i != 0)	EntranceBound.Expand(LerpPoint);
			else		EntranceBound = CBox3Dfp32(LerpPoint, LerpPoint);
		}
		else
		{
			if (i != m_nHelpPointsEntrance)	ExitBound.Expand(LerpPoint);
			else							ExitBound = CBox3Dfp32(LerpPoint, LerpPoint);
		}

		m_pWClient->Debug_RenderVertex(LerpPoint, HelpColor, Duration, bFade);
	}
	EntranceBound.Grow(2);
	ExitBound.Grow(2);
	if (m_nDecalPointsEntrance > 0) m_pWClient->Debug_RenderAABB(EntranceBound, CPixel32(0,255,255), 0.0f, false);
	if (m_nDecalPointsExit > 0) m_pWClient->Debug_RenderAABB(ExitBound, CPixel32(0,255,255), 0.0f, false);

	m_pWClient->Debug_RenderVector(m_pObj->GetPosition(), m_pObj->GetPositionMatrix().GetRow(0) * 8.0f, CPixel32(0,0,255), 0.0f, false);
}
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_BloodEffect
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CWObject_BloodEffect::OnCreate()
{
	parent::OnCreate();

	fp32 DefaultScale = 0.5f;

	m_nWallmarks = 0;
	m_nFloormarks = 0;

	SystemType(this, 0);
	CloudType(this, 0);
	SpurtType(this, OBJ_BLOODSYS_FLAGS_EXIT);
	iOwnerData(this, 0);
	iBloodModel(this, 0);
	DecalScaleEntrance(this, *(int32*)&DefaultScale);
	m_DecalPoints.m_MinEntrance = 1;
	m_DecalPoints.m_MaxEntrance = 1;
	m_DecalPoints.m_MinExit = 1;
	m_DecalPoints.m_MaxExit = 1;
	m_HelpPoints.m_MinEntrance = 1;
	m_HelpPoints.m_MaxEntrance = 1;
	m_HelpPoints.m_MinExit = 1;
	m_HelpPoints.m_MaxExit = 3;
	
	m_DirtyMask |= (OBJ_BLOODSYS_DIRTY_CREATE << CWO_DIRTYMASK_USERSHIFT);
	ClientFlags() |= (CWO_CLIENTFLAGS_NOREFRESH | CWO_CLIENTFLAGS_INVISIBLE);

	if (m_iOwner)
		m_bNoSave = true;
}


CWO_BloodEffect_ClientData*	CWObject_BloodEffect::AllocClientData(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, const int16 _iBloodModel)
{
	CWO_BloodEffect_ClientData* pCD = GetClientData(_pObj);
	if (!pCD)
	{
		pCD = MNew(CWO_BloodEffect_ClientData);
		if (!pCD)
			Error_static("CWObject_BloodEffect", "Could not allocate client data!");

		pCD->Clear(_pObj, _pWClient, _iBloodModel);
		_pObj->m_lspClientObj[BLOODSYSTEM_CLIENTDATA] = pCD;
	}

	return pCD;
}


CWO_BloodEffect_ClientData*	CWObject_BloodEffect::GetClientData(CWObject_CoreData* _pObj)
{
	return safe_cast<CWO_BloodEffect_ClientData>((CReferenceCount*)_pObj->m_lspClientObj[BLOODSYSTEM_CLIENTDATA]);
}


void CWObject_BloodEffect::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	switch(_KeyHash)
	{
	case MHASH2('WALL','MARK'):	// "WALLMARK"
		{
			CFStr St = _pKey->GetThisValue();
			m_nWallmarks = 0;
			while(St != "")
			{
				int iSurfaceRc = m_pWServer->GetMapData()->GetResourceIndex_Surface(St.GetStrSep(","));
				m_lWallmark[m_nWallmarks].m_SurfaceID = m_pWServer->GetMapData()->GetResource_SurfaceID(iSurfaceRc);
				m_lWallmark[m_nWallmarks++].m_Size = (int32)St.GetStrSep(",").Val_int();
				if (m_nWallmarks >= BLOODSYSTEM_WALLMARK_MAX)
					break;
			}
		}
		break;

	case MHASH3('FLOO','RMAR','K'): // "FLOORMARK"
		{
			CFStr St = _pKey->GetThisValue();
			m_nFloormarks = 0;
			while(St != "")
			{
				int iSurfaceRc = m_pWServer->GetMapData()->GetResourceIndex_Surface(St.GetStrSep(","));
				m_lFloormark[m_nFloormarks].m_SurfaceID = m_pWServer->GetMapData()->GetResource_SurfaceID(iSurfaceRc);
				m_lFloormark[m_nFloormarks++].m_Size = (int32)St.GetStrSep(",").Val_int();
				if (m_nFloormarks >= BLOODSYSTEM_FLOORMARK_MAX)
					break;
			}
		}
		break;

	case MHASH2('DURA','TION'): // "DURATION"
		{
			m_Duration = (fp32)_pKey->GetThisValue().Val_fp64();
			ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
		}
		break;

	case MHASH3('SPUR','T_TY','PE'): // "SPURT_TYPE"
		{
			SpurtType(this, _pKey->GetThisValue().TranslateFlags(m_slBloodSystemFlagsTranslate) & OBJ_BLOODSYS_SPURT_MASK);
		}
		break;

	case MHASH3('CLOU','D_TY','PE'): // "CLOUD_TYPE"
		{
			CloudType(this, _pKey->GetThisValue().TranslateFlags(m_slBloodSystemFlagsTranslate) & OBJ_BLOODSYS_CLOUD_MASK);
		}
		break;

	case MHASH3('SYST','EM_T','YPE'):	// "SYSTEM_TYPE"
		{
			SystemType(this, _pKey->GetThisValue().TranslateFlags(m_slBloodSystemFlagsTranslate) & OBJ_BLOODSYS_SYSTEM_MASK);
			if (SystemType(this) != 0)
				ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
		}
		break;

	case MHASH6('SPUR','T_EN','TRAN','CE_P','OINT','S'):	// "SPURT_ENTRANCE_POINTS"
		{
			CFStr St = _pKey->GetThisValue();
			uint8 MinEntrance1 = (uint8)St.GetStrSep(",").Val_int();
			uint8 MaxEntrance1 = (uint8)St.GetStrSep(",").Val_int();
			uint8 MinEntrance2 = (uint8)St.GetStrSep(",").Val_int();
			uint8 MaxEntrance2 = (uint8)St.GetStrSep(",").Val_int();

			m_DecalPoints.m_MinEntrance = MinEntrance1;
			m_DecalPoints.m_MaxEntrance = MaxEntrance1;
			m_HelpPoints.m_MinEntrance = MinEntrance2;
			m_HelpPoints.m_MaxEntrance = MaxEntrance2;
		}
		break;

	case MHASH5('SPUR','T_EX','IT_P','OINT','S'):	// "SPURT_EXIT_POINTS"
		{
			CFStr St = _pKey->GetThisValue();
			uint8 MinExit1 = (uint8)St.GetStrSep(",").Val_int();
			uint8 MaxExit1 = (uint8)St.GetStrSep(",").Val_int();
			uint8 MinExit2 = (uint8)St.GetStrSep(",").Val_int();
			uint8 MaxExit2 = (uint8)St.GetStrSep(",").Val_int();

			m_DecalPoints.m_MinExit = MinExit1;
			m_DecalPoints.m_MaxExit = MaxExit1;
			m_HelpPoints.m_MinExit = MinExit2;
			m_HelpPoints.m_MaxExit = MaxExit2;
		}
		break;

	case MHASH3('BLOO','DMOD','EL'):	// "BLOODMODEL"
		{
			CFStr BloodModel(CFStrF("BloodEffect:%s", _pKey->GetThisValue().GetStr()));
			iBloodModel(this, m_pWServer->GetMapData()->GetResourceIndex_Model(BloodModel));
			ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
		}
		break;

	case MHASH4('SPUR','T_ST','RENG','TH'):	// "SPURT_STRENGTH"
		{
			CFStr St = _pKey->GetThisValue();
			m_Strength1 = (fp32)St.GetStrSep(",").Val_fp64();
			m_Strength2 = (fp32)St.GetStrSep(",").Val_fp64();
			m_Strength3 = (fp32)St.GetStrSep(",").Val_fp64();
			m_Strength4 = (fp32)St.GetStrSep(",").Val_fp64();
		}
		break;

	case MHASH5('DECA','L_SC','ALE_','ENTR','ANCE'):	// "DECAL_SCALE_ENTRANCE"
		{
			fp32 Scale = (fp32)_pKey->GetThisValue().Val_fp64();
			DecalScaleEntrance(this, *(int32*)&Scale);
		}
		break;

	case MHASH2('MODE','L'):	// "MODEL"
	case MHASH2('MODE','L0'):	// "MODEL0"
	case MHASH2('MODE','L1'):	// "MODEL1"
	case MHASH2('MODE','L2'):	// "MODEL2"
		{
			//parent::OnEvalKey(_KeyHash, _pKey);
			ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
		}

	default:
		{
			parent::OnEvalKey(_KeyHash, _pKey);
		}
		break;
	}
}


void CWObject_BloodEffect::OnFinishEvalKeys()
{
}


void CWObject_BloodEffect::OnIncludeClass(CMapData* _pMapData, CWorld_Server* _pWServer)
{
	parent::OnIncludeClass(_pMapData, _pWServer);

	int ModelRc = _pMapData->GetResourceIndex_Model("BloodEffect");
	_pMapData->GetResource_Model(ModelRc);
}


void CWObject_BloodEffect::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	parent::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

	CFStr St = _pReg->GetValue("WALLMARK");
	while(St != "")
	{
		int iSurfaceRc = _pMapData->GetResourceIndex_Surface(St.GetStrSep(","));
		_pMapData->GetResource_SurfaceID(iSurfaceRc);
		_pMapData->GetResource_Surface(iSurfaceRc);
		St.GetStrSep(",");
	}
	
	St = _pReg->GetValue("FLOORMARK");
	while(St != "")
	{
		int iSurfaceRc = _pMapData->GetResourceIndex_Surface(St.GetStrSep(","));
		_pMapData->GetResource_SurfaceID(iSurfaceRc);
		_pMapData->GetResource_Surface(iSurfaceRc);
		St.GetStrSep(",");
	}

	{ // Get blood model
		CRegistry* pBloodModel = (_pReg) ? _pReg->FindChild("BLOODMODEL") : NULL;
		if(pBloodModel)
			_pMapData->GetResourceIndex_Model(CFStrF("BloodEffect:%s", pBloodModel->GetThisValue().GetStr()).GetStr());
	}
}


void CWObject_BloodEffect::OnInitInstance(const aint* _pParam, int _nParam)
{
	parent::OnInitInstance(_pParam, _nParam);

	Sound(m_iSoundSpawn);
	m_iSound[0] = m_iSoundLoop0;
	m_iSound[1] = m_iSoundLoop1;

	// Do we want to run as old effect ?
	if (SystemType(this) == 0)
	{
		SimpleEffect_OnInitInstance();
		return;
	}

	// Setup creation data
	const CMat4Dfp32& PosMat = GetPositionMatrix();
	const CVec3Dfp32& HitPos = PosMat.GetRow(3);
	const CVec3Dfp32& HitDir = PosMat.GetRow(0);

	bool bSpurtEntrance		= (SpurtType(this) & OBJ_BLOODSYS_FLAGS_ENTRANCE) != 0;
	bool bCloudEntrance		= (CloudType(this) & OBJ_BLOODSYS_FLAGS_ENTRANCE) != 0;
	bool bSystemEntrance	= bSpurtEntrance | bCloudEntrance;
	bool bSpurtExit			= (SpurtType(this) & OBJ_BLOODSYS_FLAGS_EXIT) != 0;
	bool bCloudExit			= (CloudType(this) & OBJ_BLOODSYS_FLAGS_EXIT) != 0;
	bool bSystemExit		= bSpurtExit | bCloudExit;

	Create_BloodData(HitPos, HitDir, SystemType(this), bSystemEntrance, bSystemExit);
	Create_BloodSpurt(HitDir, bSpurtEntrance, bSpurtExit);
	Create_BloodCloud(bCloudEntrance, bCloudExit);
}


void CWObject_BloodEffect::SimpleEffect_OnInitInstance()
{
	SimpleEffect_TraceWallmark();
}


int CWObject_BloodEffect::SimpleEffect_RandExclude(int _nRand, int _Exclude)
{
	if (_nRand <= 1)
		return 0;
	int Rand = MRTC_RAND() & (_nRand-1);
	if (Rand >= _Exclude && _Exclude > 0)
		return Rand - 1;
	else
		return Rand;
}


void CWObject_BloodEffect::SimpleEffect_TraceWallmark()
{
	/*
	int			m_Wallmark_iSurface[MAXWALLMARKS];
	uint8		m_Wallmark_Sizes[MAXWALLMARKS];
	int			m_Wallmark_nSurfaces;
	int			m_Floormark_iSurface[MAXFLOORMARKS];
	uint8		m_Floormark_Sizes[MAXFLOORMARKS];
	int			m_Floormark_nSurfaces;

	CVec3Dfp32	m_Wallmark_Pos;
	CVec3Dfp32	m_Wallmark_Normal;
	*/

	const CVec3Dfp32 Splash_Origin = CVec3Dfp32::GetMatrixRow(GetPositionMatrix(), 3);
	const CVec3Dfp32 Splash_Dir = CVec3Dfp32::GetMatrixRow(GetPositionMatrix(), 0);
	const fp32 Splash_Velocity = 300;
	const CVec3Dfp32 Splash_Gravity = CVec3Dfp32(0, 0, -2000);

	const int Splash_nMaxIterations = 10;
	const fp32 Splash_WantedApproximation = 5.0f;
	const fp32 Splash_MaxApproximationError = 0.1f;
	const fp32 Splash_TimeStepErrorCorrectionScale = 0.1f;

	const fp32 Wallmark_SpawnTime = 0;

	fp32 Wallmark_MaxDistanceSize = 0;
	fp32 Wallmark_MinDistanceSize = 0;
	fp32 Wallmark_ProjectionDepth = 40;
	fp32 Splash_SafeTracedTime = 0.0f;
	fp32 Splash_TimeStep = 1.0f;

	bool bWallmarkSpawned = false;

	CVec3Dfp32 P0, P1;
	while (!bWallmarkSpawned && (Splash_SafeTracedTime < 10.0f))
	{
		fp32 t0 = Splash_SafeTracedTime;
		fp32 dt = Splash_TimeStep;
		int nIterations = Splash_nMaxIterations;
		fp32 ApproximationError = 0.0f;
		do
		{
			dt = Max(dt - ApproximationError * Splash_TimeStepErrorCorrectionScale, 0.0f);
			fp32 t1 = t0 + dt;
			fp32 tm = (t0 + t1) * 0.5f;
			P0 = SimpleEffect_F(t0);
			P1 = SimpleEffect_F(t1);
			CVec3Dfp32 PM = SimpleEffect_F(tm);
			CVec3Dfp32 LM = (P0 + P1) * 0.5f;
			CVec3Dfp32 Approximation = (PM - LM) * 0.5f;
			ApproximationError = (Approximation.Length() - Splash_WantedApproximation);
			nIterations--;

			P0 += Approximation;
			P1 += Approximation;
		}
		while ((M_Fabs(ApproximationError) > Splash_MaxApproximationError) && (nIterations > 0));

		CCollisionInfo CInfo;
		CInfo.m_CollisionType = CXR_COLLISIONTYPE_BOUND;
		CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION);
		int32 OwnFlags = 0;
		int32 ObjectFlags = OBJECT_FLAGS_WORLD;// | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
		int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
		bool bHit = m_pWServer->Phys_IntersectLine(P0, P1, OwnFlags, ObjectFlags, MediumFlags, &CInfo, 0);
		if (bHit && CInfo.m_bIsValid)
		{
			CNetMsg Msg(FXBLOOD_NETMSG_OLDDECAL);
			Msg.Addfp32(CInfo.m_Pos[0]);
			Msg.Addfp32(CInfo.m_Pos[1]);
			Msg.Addfp32(CInfo.m_Pos[2]);
			Msg.Addfp32(CInfo.m_Plane.n[0]);
			Msg.Addfp32(CInfo.m_Plane.n[1]);
			Msg.Addfp32(CInfo.m_Plane.n[2]);
			bool bValid = true;
			if (m_nWallmarks > 0 && M_Fabs(CInfo.m_Plane.n.k[2]) < 0.5f)
			{
				static int LastWallmark = -1;
				LastWallmark = SimpleEffect_RandExclude(m_nWallmarks, LastWallmark);
				Msg.AddInt16(m_lWallmark[LastWallmark].m_SurfaceID);
				Msg.AddInt8(m_lWallmark[LastWallmark].m_Size);
			}
			else if (m_nFloormarks > 0)
			{
				static int LastFloormark = -1;
				LastFloormark = SimpleEffect_RandExclude(m_nFloormarks, LastFloormark);
				Msg.AddInt16(m_lFloormark[LastFloormark].m_SurfaceID);
				Msg.AddInt8(m_lFloormark[LastFloormark].m_Size);
			}
			else
				bValid = false;

			if (bValid)
			{
				m_pWServer->NetMsg_SendToClass(Msg, m_iClass);
				return;
			}
		}

		Splash_TimeStep = dt;
		Splash_SafeTracedTime = t0 + dt;
	}
}


void CWObject_BloodEffect::Create_BloodData(const CVec3Dfp32& _HitPos, const CVec3Dfp32& _HitDir, uint16 _SystemFlags, const bool _bEntrance, const bool _bExit)
{
	// Get game tick
	const int32 GameTick = m_pWServer->GetGameTick();
	
	// If exit wounds are specified, trace the exit
	CVec3Dfp32 ExitHole;
	if (_bExit && m_iOwner && m_iOwner != m_iObject)
	{
		CWObject* pOwner = m_pWServer->Object_Get(m_iOwner);
		if (!pOwner)
			return;

		CWO_OnIntersectLineContext IntersectLineCtx;
		IntersectLineCtx.m_pObj = pOwner;
		IntersectLineCtx.m_pPhysState = safe_cast<CWorld_PhysState>(m_pWServer);
		IntersectLineCtx.m_p0 = _HitPos + (_HitDir * 16.0f);
		IntersectLineCtx.m_p1 = _HitPos;
		IntersectLineCtx.m_ObjectFlags = (OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PROJECTILE);
		IntersectLineCtx.m_ObjectIntersectionFlags = 0;
		IntersectLineCtx.m_MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;

		CCollisionInfo CInfo;
		CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
		CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_TIME | CXR_COLLISIONRETURNVALUE_SURFACE |
							  CXR_COLLISIONRETURNVALUE_LOCALPOSITION | CXR_COLLISIONRETURNVALUE_POSITION);

		const bool bHit = pOwner->m_pRTC->m_pfnOnIntersectLine(IntersectLineCtx, &CInfo);
		if (bHit && CInfo.m_bIsValid)
		{
			ExitHole = CInfo.m_Pos;
			_SystemFlags |= OBJ_BLOODSYS_FLAGS_CINFO;
		}
	}

	// Set blood effect data
	Seed(this, (GameTick + TruncToInt(M_Fabs(_HitPos * _HitDir)) + 0xfab5) * m_iObject);
	iOwnerData(this, m_iOwner);
	SystemType(this, _SystemFlags);

	// Add exit hole position
	if (_SystemFlags & OBJ_BLOODSYS_FLAGS_CINFO)
		m_ExitHole = ExitHole;
}


void CWObject_BloodEffect::Create_BloodSpurt(const CVec3Dfp32& _HitDir, const bool _bEntrance, const bool _bExit)
{
	uint8 BloodSpurtFlags = 0;

	BloodSpurtFlags |= (_bEntrance) ? OBJ_BLOODSYS_FLAGS_ENTRANCE : 0;
	BloodSpurtFlags |= (_bExit) ? OBJ_BLOODSYS_FLAGS_EXIT : 0;

	// Setup blood spurt information
	SpurtType(this, BloodSpurtFlags);
}


void CWObject_BloodEffect::Create_BloodCloud(const bool _bEntrance, const bool _bExit)
{
	uint8 BloodCloudFlags = 0;

	BloodCloudFlags |= (_bEntrance) ? OBJ_BLOODSYS_FLAGS_ENTRANCE : 0;
	BloodCloudFlags |= (_bExit) ? OBJ_BLOODSYS_FLAGS_EXIT : 0;

	// Setup blood cloud information
	CloudType(this, BloodCloudFlags);
}


int CWObject_BloodEffect::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const
{
	int Flags = 0;
	uint8* pD = _pData;
	const uint16 UserDirtyMask = (_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT) & OBJ_BLOODSYS_DIRTY_MASK;

	pD += parent::OnCreateClientUpdate(_iClient, _pClObjInfo, _pOld, _pData, Flags);
	if ((pD - _pData) == 0)
		return 0;

	PTR_PUTUINT16(pD, UserDirtyMask);
	if (UserDirtyMask & OBJ_BLOODSYS_DIRTY_CREATE)
	{
		CRand_MersenneTwister* pRand = MRTC_GetRand();
		pRand->InitRand(Seed(const_cast<CWObject_BloodEffect*>(this)));

		uint8 nDecalPointsEntranceMod = MaxMT(1, m_DecalPoints.m_MaxEntrance - m_DecalPoints.m_MinEntrance);
		uint8 nDecalPointsExitMod = MaxMT(1, m_DecalPoints.m_MaxExit - m_DecalPoints.m_MinExit);
		uint8 nHelpPointsEntranceMod = MaxMT(1, m_HelpPoints.m_MaxEntrance - m_HelpPoints.m_MinEntrance);
		uint8 nHelpPointsExitMod = MaxMT(1, m_HelpPoints.m_MaxExit - m_HelpPoints.m_MinExit);

		const int32 SpurtFlags = SpurtType(const_cast<CWObject_BloodEffect*>(this));

		uint8 nDecalPointsEntrance	= (SpurtFlags & OBJ_BLOODSYS_FLAGS_ENTRANCE) ? m_DecalPoints.m_MinEntrance + (pRand->GenRand32() % nDecalPointsEntranceMod) : 0;
		uint8 nDecalPointsExit		= (SpurtFlags & OBJ_BLOODSYS_FLAGS_EXIT) ? m_DecalPoints.m_MinExit + (pRand->GenRand32() % nDecalPointsExitMod) : 0;
		uint8 nHelpPointsEntrance	= (nDecalPointsEntrance > 0) ? m_HelpPoints.m_MinEntrance + (pRand->GenRand32() % nHelpPointsEntranceMod) : 0;
		uint8 nHelpPointsExit		= (nDecalPointsExit > 0) ? m_HelpPoints.m_MinExit + (pRand->GenRand32() % nHelpPointsExitMod) : 0;
		uint16 nDecalPoints = nDecalPointsEntrance + nDecalPointsExit;

		// Pack exit hole
		const int16 SystemFlags = SystemType(const_cast<CWObject_BloodEffect*>(this));

		if (SystemFlags & OBJ_BLOODSYS_FLAGS_CINFO)
		{
			PTR_PUTFP32(pD, m_ExitHole.k[0]);
			PTR_PUTFP32(pD, m_ExitHole.k[1]);
			PTR_PUTFP32(pD, m_ExitHole.k[2]);
		}

		if (SystemFlags & OBJ_BLOODSYS_FLAGS_SPURT)
		{
			PTR_PUTFP32(pD, m_Strength1);
			PTR_PUTFP32(pD, m_Strength2);
			PTR_PUTFP32(pD, m_Strength3);
			PTR_PUTFP32(pD, m_Strength4);

			// Pack entrance points
			if (SpurtFlags & OBJ_BLOODSYS_FLAGS_ENTRANCE)
			{
				PTR_PUTUINT8(pD, nDecalPointsEntrance);
				PTR_PUTUINT8(pD, nHelpPointsEntrance);
			}

			// Pack exit point
			if (SpurtFlags & OBJ_BLOODSYS_FLAGS_EXIT)
			{
				PTR_PUTUINT8(pD, nDecalPointsExit);
				PTR_PUTUINT8(pD, nHelpPointsExit);
			}

			// Pack wall/floor marks
			static int iWallmark = -1;
			static int iFloormark = -1;
			const int32 iScale = DecalScaleEntrance(const_cast<CWObject_BloodEffect*>(this));
			const fp32 Scale = *(fp32*)&iScale;
			for (uint32 i = 0; i < nDecalPointsEntrance; i++)
			{
				iWallmark = SimpleEffect_RandExclude(m_nWallmarks, iWallmark);
				iFloormark = SimpleEffect_RandExclude(m_nFloormarks, iFloormark);

				const SBloodDecal& Wallmark = m_lWallmark[iWallmark];
				const SBloodDecal& Floormark = m_lFloormark[iFloormark];

				uint8 WallmarkSize = (uint8)Min(255, TruncToInt(fp32(Wallmark.m_Size) * Scale));
				uint8 FloormarkSize = (uint8)Min(255, TruncToInt(fp32(Floormark.m_Size) * Scale));

				PTR_PUTINT16(pD, Wallmark.m_SurfaceID);
				PTR_PUTINT16(pD, Floormark.m_SurfaceID);
				PTR_PUTUINT8(pD, Wallmark.m_Size);
				PTR_PUTUINT8(pD, Floormark.m_Size);
			}

			for (uint32 i = 0; i < nDecalPointsExit; i++)
			{
				iWallmark = SimpleEffect_RandExclude(m_nWallmarks, iWallmark);
				iFloormark = SimpleEffect_RandExclude(m_nFloormarks, iFloormark);

				const SBloodDecal& Wallmark = m_lWallmark[iWallmark];
				const SBloodDecal& Floormark = m_lFloormark[iFloormark];

				PTR_PUTINT16(pD, Wallmark.m_SurfaceID);
				PTR_PUTINT16(pD, Floormark.m_SurfaceID);
				PTR_PUTUINT8(pD, Wallmark.m_Size);
				PTR_PUTUINT8(pD, Floormark.m_Size);
			}
		}
	}

	return (pD - _pData);
}


int CWObject_BloodEffect::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	const int32 DataOwner = m_iOwnerData.Get(_pObj);

	const uint8* pD = _pData;
	pD += parent::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);
	
	uint16 UserDirtyMask = 0;
	PTR_GETUINT16(pD, UserDirtyMask);
	if (UserDirtyMask & OBJ_BLOODSYS_DIRTY_CREATE)
	{
		CWO_BloodEffect_ClientData* pCD = AllocClientData(_pObj, _pWClient, m_iBloodModel.Get(_pObj));
		if (pCD)
			pCD->OnClientUpdate(pD, m_SystemType.Get(_pObj), m_SpurtType.Get(_pObj), m_CloudType.Get(_pObj));

		// Get hit position and direction
		const CMat4Dfp32& PosMat = (_pObj->GetParent()) ? _pObj->GetPositionMatrix() : _pObj->GetLocalPositionMatrix();
		const CVec3Dfp32& HitPos = PosMat.GetRow(3);
		const CVec3Dfp32& HitUp = PosMat.GetRow(2);
		const CVec3Dfp32& HitDir = PosMat.GetRow(0);
		CVec3Dfp32 ExitHole;

		const uint8 SystemFlags = m_SystemType.Get(_pObj);
		const uint8 SpurtFlags = m_SpurtType.Get(_pObj);
		const uint8 CloudFlags = m_CloudType.Get(_pObj);

		//CWO_BloodEffect_ClientData* pCD = AllocClientData(_pObj, _pWClient);
		if (pCD)
		{
			int32 Seed = m_Seed.Get(_pObj);
			CRand_MersenneTwister* pRand = MRTC_GetRand();
			pRand->InitRand(Seed);
			
			// Allocate the points and set them up properly
			pCD->AllocSpurtPoints(PosMat, ExitHole, pRand);
		}
	}

	return (pD - _pData);
}


void CWObject_BloodEffect::OnRefresh()
{
	parent::OnRefresh();

	// Clear dirty mask
	m_DirtyMask &= ~(OBJ_BLOODSYS_DIRTY_MASK << CWO_DIRTYMASK_USERSHIFT);

	// Remove ourself when duration time runs out.
	fp32 RefreshTime = CMTime::CreateFromTicks((m_pWServer->GetGameTick() - m_CreationGameTick), m_pWServer->GetGameTickTime()).GetTime();
	if (RefreshTime > m_Duration)
		m_pWServer->Object_Destroy(m_iObject);
}


void CWObject_BloodEffect::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	// If no new system types defined, render as old effect
	if (m_SystemType.Get(_pObj) == 0)
	{
		parent::OnClientRender(_pObj, _pWClient, _pEngine, _ParentMat);
		return;
	}

	// Do we want to render simple models?
	if (m_SystemType.Get(_pObj) & OBJ_BLOODSYS_FLAGS_MODELS)
		parent::OnClientRender(_pObj, _pWClient, _pEngine, _ParentMat);

	CXR_VBManager* pVBM = _pEngine->m_pVBM;
	CWO_BloodEffect_ClientData* pCD = GetClientData(_pObj);
	if (pCD && pVBM)
	{
		// Create spurt points to be used during rendering
		CBloodEffectModelParam* pModelParam = pCD->CreateModelData(pVBM);

		// Create anim state
		CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
		AnimState.m_Data[0] = (aint)pModelParam;
		AnimState.m_Data[1] = m_Seed.Get(_pObj);

		const CMat4Dfp32& PosMat = _pObj->GetPositionMatrix();

		// Set model parameters
		*(pModelParam->m_pEntrance) = PosMat.GetRow(3);
		*(pModelParam->m_pExit) = pCD->m_ExitHole;
		
		uint16 CloudFlags = m_CloudType.Get(_pObj);
		pModelParam->m_bCloudEntrance = (CloudFlags & OBJ_BLOODSYS_FLAGS_ENTRANCE) ? 1 : 0;
		pModelParam->m_bCloudExit = (CloudFlags & OBJ_BLOODSYS_FLAGS_EXIT) ? 1 : 0;

		// Add blood model for rendering
		if (pCD->m_pBloodModel)
			_pEngine->Render_AddModel(pCD->m_pBloodModel, PosMat, AnimState);
	}

	#ifdef BLOODSYSTEM_DEBUG
	{
		CWO_BloodEffect_ClientData* pCD = GetClientData(_pObj);
		if (pCD)
			pCD->Debug_RenderSpurtPoints();
	}
	#endif
}


void CWObject_BloodEffect::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	parent::OnClientRefresh(_pObj, _pWClient);

	CWO_BloodEffect_ClientData* pCD = GetClientData(_pObj);
	if (pCD)
		pCD->RefreshSpurtPoints();
}


void CWObject_BloodEffect::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	switch(_Msg.m_MsgType)
	{
	case FXBLOOD_NETMSG_OLDDECAL:
		{
			int P = 0;
			CVec3Dfp32 Pos;
			CVec3Dfp32 Normal;
			Pos[0] = _Msg.Getfp32(P);
			Pos[1] = _Msg.Getfp32(P);
			Pos[2] = _Msg.Getfp32(P);
			Normal[0] = _Msg.Getfp32(P);
			Normal[1] = _Msg.Getfp32(P);
			Normal[2] = _Msg.Getfp32(P);

			CMat4Dfp32 Mat;
			Mat.Unit();
			Normal.SetRow(Mat, 0);
			bool bRotate = false;
			if (M_Fabs(Normal.k[2]) < 0.5f)
				CVec3Dfp32(0, 0, 1).SetRow(Mat, 1);
			else
			{
				CVec3Dfp32(0, 1, 0).SetRow(Mat, 1);
				bRotate = true;
			}
			Mat.RecreateMatrix(0, 1);
			
			Swap(CVec3Dfp32::GetRow(Mat, 0), CVec3Dfp32::GetRow(Mat, 2));
			CVec3Dfp32::GetRow(Mat, 1) = -CVec3Dfp32::GetRow(Mat, 1);

			Pos.SetRow(Mat, 3);
			if (bRotate)
				Mat.RotZ_x_M(Random);

			CXR_WallmarkDesc WMD;
			WMD.m_SurfaceID = _Msg.GetInt16(P);
			WMD.m_Size = _Msg.GetInt8(P);
			WMD.m_SpawnTime = _pWClient->GetRenderTime();
			_pWClient->Wallmark_Create(WMD, Mat, 4, 0);
			CVec3Dfp32::GetRow(Mat, 3) += CVec3Dfp32::GetRow(Mat, 2) * 10.0f;
		}
		return;
	}

	CWObject_Ext_Model::OnClientNetMsg(_pObj, _pWClient, _Msg);
}


/*
void CWObject_BloodEffect::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	switch(_Msg.m_MsgType)
	{
	case BLOODSYSTEM_NETMSG_CREATE:
		{
			int Pos = 0;
			int32 GameTick = _Msg.GetInt32(Pos);
			uint8 SysFlags = (uint8)_Msg.GetInt8(Pos);
			uint8 SpurtFlags = 0;
			uint8 CloudFlags = 0;
			
			CVec3Dfp32 HitPos, HitDir, ExitPos;
			HitPos.k[0] = _Msg.Getfp32(Pos);
			HitPos.k[1] = _Msg.Getfp32(Pos);
			HitPos.k[2] = _Msg.Getfp32(Pos);

			_pWClient->Debug_RenderVertex(HitPos, CPixel32(255, 0, 0), 10.0f, false);

			if (SysFlags & OBJ_BLOODSYS_FLAGS_CINFO)
			{
				ExitPos.k[0] = _Msg.Getfp32(Pos);
				ExitPos.k[1] = _Msg.Getfp32(Pos);
				ExitPos.k[2] = _Msg.Getfp32(Pos);

				_pWClient->Debug_RenderVertex(ExitPos, CPixel32(0, 255, 0), 10.0f, false);
			}

			if (SysFlags & OBJ_BLOODSYS_FLAGS_SPURT)
			{
				SpurtFlags = _Msg.GetInt8(Pos);
				if (SpurtFlags)
				{
					HitDir.k[0] = _Msg.Getfp32(Pos);
					HitDir.k[1] = _Msg.Getfp32(Pos);
					HitDir.k[2] = _Msg.Getfp32(Pos);

					_pWClient->Debug_RenderVector(HitPos, HitDir * 16.0f, CPixel32(255,0,0), 10.0f, false);
				}
			}

			if (SysFlags & OBJ_BLOODSYS_FLAGS_CLOUD)
			{
				CloudFlags = _Msg.GetInt8(Pos);
				if (CloudFlags)
				{
					CMat4Dfp32 SpherePos;
					SpherePos.Unit();
					SpherePos.GetRow(3) = HitPos;

					_pWClient->Debug_RenderSphere(SpherePos, 8.0f, CPixel32(0,255,0), 10.0f, false);
				}
			}
		}
		break;
	}
}


void CWObject_BloodEffect::TraceBloodDecal()
{
	
}
*/

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_BloodEffect
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_BloodEffect, CWObject_Ext_Model, 0x0100);

void CWObject_BloodEffect::OnCreate()
{
	CWObject_Ext_Model::OnCreate();
	m_Wallmark_nSurfaces = 0;
	m_Floormark_nSurfaces = 0;
//	ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
}

void CWObject_BloodEffect::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	switch (_KeyHash)
	{
	case MHASH2('WALL','MARK'): // "WALLMARK"
		{
			CFStr St = KeyValue;
			m_Wallmark_nSurfaces = 0;
			while(St != "")
			{
				m_Wallmark_iSurface[m_Wallmark_nSurfaces] = m_pWServer->GetMapData()->GetResourceIndex_Surface(St.GetStrSep(","));
				m_Wallmark_Sizes[m_Wallmark_nSurfaces++] = St.GetStrSep(",").Val_int();
				if (m_Wallmark_nSurfaces >= MAXWALLMARKS)
					break;
			}
			break;
		}
	case MHASH3('FLOO','RMAR','K'): // "FLOORMARK"
		{
			CFStr St = KeyValue;
			m_Floormark_nSurfaces = 0;
			while(St != "")
			{
				m_Floormark_iSurface[m_Floormark_nSurfaces] = m_pWServer->GetMapData()->GetResourceIndex_Surface(St.GetStrSep(","));
				m_Floormark_Sizes[m_Floormark_nSurfaces++] = St.GetStrSep(",").Val_int();
				if (m_Floormark_nSurfaces >= MAXFLOORMARKS)
					break;
			}
			break;
		}
	default:
		{
			CWObject_Ext_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_BloodEffect::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	CWObject_Ext_Model::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

	CFStr St = _pReg->GetValue("WALLMARK");
	while(St != "")
	{
		_pMapData->GetResourceIndex_Surface(St.GetStrSep(","));
		St.GetStrSep(",");
	}
	St = _pReg->GetValue("FLOORMARK");
	while(St != "")
	{
		_pMapData->GetResourceIndex_Surface(St.GetStrSep(","));
		St.GetStrSep(",");
	}
}

void CWObject_BloodEffect::OnInitInstance(const aint* _pParam, int _nParam)
{
	MAUTOSTRIP(CWObject_Ext_Model_OnInitInstance, MAUTOSTRIP_VOID);

	CWObject_Ext_Model::OnInitInstance(_pParam, _nParam);

	Sound(m_iSoundSpawn);
	m_iSound[0] = m_iSoundLoop0;
	m_iSound[1] = m_iSoundLoop1;

	{
		m_Splash_Origin = CVec3Dfp32::GetMatrixRow(GetPositionMatrix(), 3);
		m_Splash_Dir = CVec3Dfp32::GetMatrixRow(GetPositionMatrix(), 0);

		m_Splash_Velocity = 300;
		m_Splash_Gravity = CVec3Dfp32(0, 0, -2000);

		m_Splash_nMaxIterations = 10;
		m_Splash_WantedApproximation = 5.0f;
		m_Splash_MaxApproximationError = 0.1f;
		m_Splash_TimeStepErrorCorrectionScale = 0.1f;

		m_Wallmark_SpawnTime = 0;

		m_Wallmark_MaxDistanceSize = 0;
		m_Wallmark_MinDistanceSize = 0;
		m_Wallmark_ProjectionDepth = 40;
		//m_Wallmark_iSurface = 0;
		//m_Floormark_iSurface = 0;
	}

	TraceWallmark();

//	{
//		CWO_BloodEffect_ClientData* pBECD = ((CWO_BloodEffect_ClientData*)(&(m_Data[0])));
//		pBECD->m_Wallmark_SpawnTime = m_Wallmark_SpawnTime;
//
//		int iWallmark = MRTC_RAND() % m_Wallmark_nSurfaces;
//		pBECD->m_Wallmark_iSurface = m_Wallmark_iSurface[iWallmark];
//		pBECD->m_Wallmark_Size = m_Wallmark_Sizes[iWallmark];
//
//		int iFloormark = MRTC_RAND() % m_Floormark_nSurfaces;
//		pBECD->m_Floormark_iSurface = m_Floormark_iSurface[iFloormark];
//		pBECD->m_Floormark_Size = m_Floormark_Sizes[iFloormark];
//
//		pBECD->m_Wallmark_ProjectionDepth = m_Wallmark_ProjectionDepth;
//		pBECD->m_Wallmark_Pos = m_Wallmark_Pos;
//		pBECD->m_Wallmark_Normal = m_Wallmark_Normal;
//	}
}

static int RandExclude(int _nRand, int _Exclude)
{
	if (_nRand <= 1)
		return 0;
	int Rand = MRTC_RAND() % (_nRand - 1);
	if (Rand >= _Exclude && _Exclude > 0)
		return Rand - 1;
	else
		return Rand;
};

void CWObject_BloodEffect::TraceWallmark()
{

	m_Splash_SafeTracedTime = 0.0f;
	m_Splash_TimeStep = 1.0f;

	CVec3Dfp32 P0, P1;
	while ((m_Wallmark_SpawnTime == 0) && (m_Splash_SafeTracedTime < 10.0f))
	{
		fp32 t0 = m_Splash_SafeTracedTime;
		fp32 dt = m_Splash_TimeStep;
		int nIterations = m_Splash_nMaxIterations;
		fp32 ApproximationError = 0.0f;
		do 
		{
			dt = Max(dt - ApproximationError * m_Splash_TimeStepErrorCorrectionScale, 0.0f);
			fp32 t1 = t0 + dt;
			fp32 tm = (t0 + t1) * 0.5f;
			P0 = F(t0);
			P1 = F(t1);
			CVec3Dfp32 PM = F(tm);
			CVec3Dfp32 LM = (P0 + P1) * 0.5f;
			CVec3Dfp32 Approximation = (PM - LM) * 0.5f;
			ApproximationError = (Approximation.Length() - m_Splash_WantedApproximation);
			nIterations--;

			P0 += Approximation;
			P1 += Approximation;

			static int iColor = 0;
			fp32 DebugTime = 30.0f;
			bool bFade = false;
			switch(iColor % 3)
			{
			case 0:
				m_pWServer->Debug_RenderWire(P0, P1, 0xff7f7f00, DebugTime, bFade);
				m_pWServer->Debug_RenderVertex(P0, 0xff7f7f00, DebugTime, bFade);
				m_pWServer->Debug_RenderVertex(P1, 0xff7f7f00, DebugTime, bFade);
				break;

			case 1:
				m_pWServer->Debug_RenderWire(P0, P1, 0xff007f7f, DebugTime, bFade);
				m_pWServer->Debug_RenderVertex(P0, 0xff007f7f, DebugTime, bFade);
				m_pWServer->Debug_RenderVertex(P1, 0xff007f7f, DebugTime, bFade);
				break;

			case 2:
				m_pWServer->Debug_RenderWire(P0, P1, 0xff00007f, DebugTime, bFade);
				m_pWServer->Debug_RenderVertex(P0, 0xff7f007f, DebugTime, bFade);
				m_pWServer->Debug_RenderVertex(P1, 0xff7f007f, DebugTime, bFade);
				break;
			}
			iColor++;
		}
		while ((M_Fabs(ApproximationError) > m_Splash_MaxApproximationError) && (nIterations > 0));

		{
//			m_pWServer->Debug_RenderWire(P0, P1, 0xFFFFFFFF, 10.0f);

			CCollisionInfo CInfo;
			CInfo.m_CollisionType = CXR_COLLISIONTYPE_BOUND;
			CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION);
			int32 OwnFlags = 0;
			int32 ObjectFlags = OBJECT_FLAGS_WORLD;// | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
			int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
			bool bHit = m_pWServer->Phys_IntersectLine(P0, P1, OwnFlags, ObjectFlags, MediumFlags, &CInfo, 0);
			if (bHit && CInfo.m_bIsValid)
			{
				CNetMsg Msg(NETMSG_CREATEWALLMARK);
				Msg.Addfp32(CInfo.m_Pos[0]);
				Msg.Addfp32(CInfo.m_Pos[1]);
				Msg.Addfp32(CInfo.m_Pos[2]);
				Msg.Addfp32(CInfo.m_Plane.n[0]);
				Msg.Addfp32(CInfo.m_Plane.n[1]);
				Msg.Addfp32(CInfo.m_Plane.n[2]);
				bool bValid = true;
//				if (M_Fabs(CInfo.m_Plane.n * CVec3Dfp32(0, 0, 1.0f)) < 0.5f && m_Wallmark_nSurfaces > 0)
				if (m_Wallmark_nSurfaces > 0 && M_Fabs(CInfo.m_Plane.n.k[2]) < 0.5f)
				{
					static int LastWallmark = -1;
					LastWallmark = RandExclude(m_Wallmark_nSurfaces, LastWallmark);
//					ConOut(CStrF("Wallmark %i", iWallmark));
					Msg.AddInt16(m_Wallmark_iSurface[LastWallmark]);
					Msg.AddInt8(m_Wallmark_Sizes[LastWallmark]);
				}
				else if (m_Floormark_nSurfaces > 0)
				{
					static int LastFloormark = -1;
					LastFloormark = RandExclude(m_Floormark_nSurfaces, LastFloormark);
//					ConOut(CStrF("Floormark %i", iFloormark));
					Msg.AddInt16(m_Floormark_iSurface[LastFloormark]);
					Msg.AddInt8(m_Floormark_Sizes[LastFloormark]);
				}
				else
					bValid = false;
				
				if (bValid)
				{
					m_pWServer->NetMsg_SendToClass(Msg, m_iClass);
					//Destroy();
					return;
				}
			}
		}

		m_Splash_TimeStep = dt;
		m_Splash_SafeTracedTime = t0 + dt;
	}
}

void CWObject_BloodEffect::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	switch(_Msg.m_MsgType)
	{
	case NETMSG_CREATEWALLMARK:
		{
			int P = 0;
			CVec3Dfp32 Pos;
			CVec3Dfp32 Normal;
			Pos[0] = _Msg.Getfp32(P);
			Pos[1] = _Msg.Getfp32(P);
			Pos[2] = _Msg.Getfp32(P);
			Normal[0] = _Msg.Getfp32(P);
			Normal[1] = _Msg.Getfp32(P);
			Normal[2] = _Msg.Getfp32(P);

			CMat4Dfp32 Mat;
			Mat.Unit();
			Normal.SetRow(Mat, 0);
//			fp32 Dot = Abs(Normal * CVec3Dfp32(0, 0, 1));
			bool bRotate = false;
			if (M_Fabs(Normal.k[2]) < 0.5f)
				CVec3Dfp32(0, 0, 1).SetRow(Mat, 1);
			else
			{
				CVec3Dfp32(0, 1, 0).SetRow(Mat, 1);
				bRotate = true;
			}
			Mat.RecreateMatrix(0, 1);
			
			Swap(CVec3Dfp32::GetRow(Mat, 0), CVec3Dfp32::GetRow(Mat, 2));
			//Swap(CVec3Dfp32::GetRow(Mat, 0), CVec3Dfp32::GetRow(Mat, 1));
//			Swap(CVec3Dfp32::GetRow(Mat, 0), CVec3Dfp32::GetRow(Mat, 2));
			CVec3Dfp32::GetRow(Mat, 1) = -CVec3Dfp32::GetRow(Mat, 1);

			Pos.SetRow(Mat, 3);
			if (bRotate)
				Mat.RotZ_x_M(Random);

			CXR_WallmarkDesc WMD;
			WMD.m_SurfaceID = _pWClient->GetMapData()->GetResource_SurfaceID(_Msg.GetInt16(P));
			WMD.m_Size = _Msg.GetInt8(P);
			WMD.m_SpawnTime = _pWClient->GetRenderTime();
			_pWClient->Wallmark_Create(WMD, Mat, 4, 0);
			CVec3Dfp32::GetRow(Mat, 3) += CVec3Dfp32::GetRow(Mat, 2) * 10.0f;
			//_pWClient->Debug_RenderMatrix(Mat, 20.0f, true, 0xff7f0000, 0xff007f00, 0xff00007f);
		}
		return;
	}
	CWObject_Ext_Model::OnClientNetMsg(_pObj, _pWClient, _Msg);
}
*/


/*
void CWObject_BloodEffect::OnRefresh()
{
	fp32 GameTime = (m_pWServer->GetGameTick() * SERVER_TIMEPERFRAME);
	CVec3Dfp32 Pos = F(GameTime - m_Splash_SpawnTime);
	m_pWServer->Debug_RenderVertex(Pos, 0xFF801010, 1.0f);

	CWObject_Ext_Model::OnRefresh();
}

static void CreateTempWallmark(CWorld_Client* _pWClient, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Out, const CVec3Dfp32& _Up, fp32 _Size, fp32 _Depth, int _iSurface, fp32 AnimTime = 0, int _Flags = 0)
{
	MAUTOSTRIP(Wallmark_CreateWallmark, MAUTOSTRIP_VOID);
	CXR_WallmarkDesc WMD;
	WMD.m_Time = AnimTime;
	WMD.m_Size = _Size;

	WMD.m_SurfaceID = _iSurface;

	CMat4Dfp32 WMMat;
	_Out.SetRow(WMMat, 2);
	_Up.SetRow(WMMat, 0);
	WMMat.RecreateMatrix(2, 0);
	//	(CVec3Dfp32::GetMatrixRow(WMMat, 1)).SetMatrixRow(WMMat, 1);
	_Pos.SetMatrixRow(WMMat, 3);

	_pWClient->Wallmark_CreateTemp(WMD, WMMat, _Depth, _Flags);
}

void CWObject_BloodEffect::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	CWO_BloodEffect_ClientData* pBECD = ((CWO_BloodEffect_ClientData*)(&(_pObj->m_Data[0])));

	int iWallmark;
	int Size;
	if (Abs(Wallmark_Normal * CVec3Dfp32(0.0f, 0.0f, 1.0f) > 0.5f)
	{
		iWallmark = pBECD->m_Wallmark_iSurface;
		Size = pBECD->m_Wallmark_Size;
	}
	else
	{
		iWallmark = pBECD->m_Floormark_iSurface;
		Size = pBECD->m_Floor_Size;
	}

	CXR_WallmarkDesc WMD;
	WMD.m_SpawnTime = _pWClient->GetRenderTime();
	WMD.m_Size = _Size;

	CMat4Dfp32 Mat;
	_Out.SetRow(Mat, 2);
	_Up.SetRow(Mat, 0);
	WMMat.RecreateMatrix(2, 0);
	Swap(CVec3Dfp32::GetRow(Mat, 0), CVec3Dfp32::GetRow(Mat, 2));
	CVec3Dfp32::GetRow(Mat, 0) = -CVec3Dfp32::GetRow(Mat, 0);
	_pWClient->Wallmark_Create(WMD, Mat, 4, XR_WALLMARK_TEMPORARY);


	CVec3Dfp32 Wallmark_Up = CVec3Dfp32(0.0f, 0.0f, 1.0f);
	if ((Wallmark_Up * Wallmark_Normal) > 0.9f)
		Wallmark_Up = CVec3Dfp32(-1.0f + 2.0f * MFloat_GetRand(int(Wallmark_Pos[0] + Wallmark_SpawnTime)), -1.0f + 2.0f * MFloat_GetRand(int(Wallmark_Pos[1] + Wallmark_SpawnTime + 1.0f)), 0.0f);

	CVec3Dfp32 Wallmark_Left;
	Wallmark_Normal.CrossProd(Wallmark_Up, Wallmark_Left);
	Wallmark_Normal.CrossProd(Wallmark_Left, Wallmark_Up);

	CreateTempWallmark(_pWClient, Wallmark_Pos, Wallmark_Normal, Wallmark_Up,
					   pBECD->m_Wallmark_Size, 40,
					   pBECD->m_Wallmark_iSurface, _pWClient->GetRenderTime());

	CWObject_Ext_Model::OnClientRender(_pObj, _pWClient, _pEngine);
}
*/

