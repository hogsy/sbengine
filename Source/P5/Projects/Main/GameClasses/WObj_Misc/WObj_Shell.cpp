/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_Shell.h

	Author:			Patrik Willbo

	Copyright:		Copyright Starbreeze AB 2005

	Contents:		CWObject_Shell
					

	History:		
		051006:		Created File, first pass mockup
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_Shell.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Shell, CWObject_Shell_Parent, 0x0100);


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Shell_SpawnData
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CWO_Shell_SpawnData::AddNetMsgData(CNetMsg& _Msg) const
{
	CVec3Dfp32 Pos = m_AttachMat.GetRow(3);
	CQuatfp32 q;
	q.Create(m_AttachMat);
	
	// Send quaternion and position
	_Msg.Addfp32(Pos.k[0]);
	_Msg.Addfp32(Pos.k[1]);
	_Msg.Addfp32(Pos.k[2]);
	_Msg.Addfp32(q.k[0]);
	_Msg.Addfp32(q.k[1]);
	_Msg.Addfp32(q.k[2]);
	_Msg.Addfp32(q.k[3]);
}


void CWO_Shell_SpawnData::GetNetMsgData(const CNetMsg& _Msg, int& _iPos) //, CWO_Shell_SpawnData _SpawnData)
{
	CQuatfp32 q;
	CVec3Dfp32 Pos;

	// Unpack quaternion and position
	Pos.k[0] = _Msg.Getfp32(_iPos);
	Pos.k[1] = _Msg.Getfp32(_iPos);
	Pos.k[2] = _Msg.Getfp32(_iPos);
	q.k[0] = _Msg.Getfp32(_iPos);
	q.k[1] = _Msg.Getfp32(_iPos);
	q.k[2] = _Msg.Getfp32(_iPos);
	q.k[3] = _Msg.Getfp32(_iPos);

	// Create matrix
	q.CreateMatrix3x3(m_AttachMat);
	m_AttachMat.GetRow(3) = Pos;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Shell_Type
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWO_Shell_Type::CWO_Shell_Type()
	: m_iModel(0)
	, m_pModel(NULL)
	, m_Direction(0)
	, m_Force(0)
	, m_iSound(0)
	, m_OffsetZ(0.0f)
{
}


CWO_Shell_Type::~CWO_Shell_Type()
{
}


void CWO_Shell_Type::OnEvalKey(CWorld_Server* _pWServer, const CRegistry* _pKey)
{
	CStr KeyName = _pKey->GetThisName();
	uint32 KeyHash = StringToHash(KeyName);
	CStr KeyValue = _pKey->GetThisValue();

	switch (KeyHash)
	{
	case MHASH2('MODE','L'): // "MODEL"
		{
			//Find the correct model
			m_iModel = _pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
			break;
		}
	case MHASH2('FORC','E'): // "FORCE"
		{
			// Get force parameters
			m_Force.ParseString(KeyValue);
			break;
		}
	case MHASH3('DIRE','CTIO','N'): // "DIRECTION"
		{
			// Get direciton behaviours
			m_Direction.ParseString(KeyValue);
			break;
		}
	case MHASH2('SOUN','D'): // "SOUND"
		{
			// Parse bounce sound index
			m_iSound = _pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH2('OFFS','ETZ'): // "OFFSETZ"
		{
			m_OffsetZ = (fp32)KeyValue.Val_fp64();
			break;
		}
	default:
		{
			break;
		}
	}
}


void CWO_Shell_Type::OnIncludeTemplate(const CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer)
{
	// Include template
	CWO_Shell_Type::IncludeModelFromKey("MODEL", _pReg, _pMapData);
	CWO_Shell_Type::IncludeSoundFromKey("SOUND", _pReg, _pMapData);
}


void CWO_Shell_Type::IncludeModelFromKey(const CStr _Key, const CRegistry* _pReg, CMapData* _pMapData)
{
	// Include model from key
	if(_pReg->FindChild(_Key))
	{
		int iModelIndex = _pMapData->GetResourceIndex_Model(_pReg->FindChild(_Key)->GetThisValue());
		if(iModelIndex)
			_pMapData->GetResource_Model(iModelIndex);
	}
}


void CWO_Shell_Type::IncludeSoundFromKey(const CStr _Key, const CRegistry* _pReg, CMapData* _pMapData)
{
	// Include sound from key
	if(_pReg->FindChild(_Key))
	{
		int iSoundIndex = _pMapData->GetResourceIndex_Sound(_pReg->FindChild(_Key)->GetThisValue());
	}
}


void CWO_Shell_Type::Pack(uint8*& _pD) const
{
	// Pack data
	TAutoVar_Pack(m_iModel, _pD);
	TAutoVar_Pack(m_Force, _pD);
	TAutoVar_Pack(m_Direction, _pD);
	TAutoVar_Pack(m_iSound, _pD);
	TAutoVar_Pack(m_OffsetZ, _pD);
}


void CWO_Shell_Type::Unpack(const uint8*& _pD)
{
	// Unpack data
	TAutoVar_Unpack(m_iModel, _pD);
	TAutoVar_Unpack(m_Force, _pD);
	TAutoVar_Unpack(m_Direction, _pD);
	TAutoVar_Unpack(m_iSound, _pD);
	TAutoVar_Unpack(m_OffsetZ, _pD);
}


void CWO_Shell_Type::Write(CCFile* _pFile, CMapData* _pMapData) const
{
	_pFile->WriteLE(m_iModel);
	m_Force.Write(_pFile);
	m_Direction.Write(_pFile);
	_pFile->WriteLE(m_iSound);
	_pFile->WriteLE(m_OffsetZ);
}


void CWO_Shell_Type::Read(CCFile* _pFile, CMapData* _pMapData)
{
	_pFile->ReadLE(m_iModel);
	m_Force.Read(_pFile);
	m_Direction.Read(_pFile);
	_pFile->ReadLE(m_iSound);
	_pFile->ReadLE(m_OffsetZ);

	m_pModel = _pMapData->GetResource_Model(m_iModel);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Shell_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWO_Shell_ClientData::CShellOwnerData::CShellOwnerData()
	: m_iOwner(0)
	, m_iFirst(-1)
	, m_nShells(0)
{
}


void CWO_Shell_ClientData::CShellOwnerData::OnRefresh(CWorld_Client* _pWClient, CBox3Dfp32& _BoundingBox, const fp32& _TickTime, CWO_Shell_ClientData::CShellData* _pShellData, CWO_Shell_Type* _pShellTypes)
{
	if(!m_iOwner)
		return;

	int32 iNextShell = -1;
	int32 iPrevShell = -1;
	int32 iShell = m_iFirst;

	// Update this objects shells
	const CVec3Dfp32 Gravity = CVec3Dfp32(0, 0, -314.24f * _TickTime);
	while(iShell >= 0)
	{
		CWO_Shell_ClientData::CShellData& ShellData = _pShellData[iShell];
		iNextShell = ShellData.m_iPrev;
		
		if(ShellData.m_Flags & SHELLDATA_FLAGS_FIRSTTICK)
			ShellData.m_Flags &= ~SHELLDATA_FLAGS_FIRSTTICK;
		else
		{
			_BoundingBox.Expand(ShellData.m_Position.GetRow(3));

			if(ShellData.m_Flags & SHELLDATA_FLAGS_MOVING)
			{
				ShellData.m_Position.M_x_RotZ(MFloat_GetRand(ShellData.m_Rand) * _TickTime);
				ShellData.m_Position.M_x_RotY(MFloat_GetRand(ShellData.m_Rand + 1) * _TickTime);
				ShellData.m_Position.M_x_RotX(MFloat_GetRand(ShellData.m_Rand + 2) * _TickTime);
				ShellData.m_Velocity += Gravity;
				CVec3Dfp32 NewPos = ShellData.m_Position.GetRow(3) + (ShellData.m_Velocity * _TickTime);
				bool bHitGround = false;

				if(NewPos.k[2] <= ShellData.m_CollisionZ)
				{
					if((ShellData.m_Flags & SHELLDATA_FLAGS_WORLDBOUNDING) == 0)
					{
						// Make sure we hit the ground
						CVec3Dfp32 EndPos = ShellData.m_Position.GetRow(3) + ((ShellData.m_Velocity * 5.0f) + (Gravity * 5.0f));
						int ObjectIntersectFlags = OBJECT_FLAGS_WORLD;
						int MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;
						CCollisionInfo CInfo;
						
						CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_TIME | CXR_COLLISIONRETURNVALUE_POSITION);
						bool bHit = _pWClient->Phys_IntersectLine(ShellData.m_Position.GetRow(3), EndPos, 0, ObjectIntersectFlags, MediumFlags, &CInfo, 0);
						if(bHit && CInfo.m_bIsValid)
						{
							if(M_Fabs(CInfo.m_Pos.k[2] - ShellData.m_CollisionZ) < 0.1f)
							{
								NewPos = CInfo.m_Pos;
								bHitGround = true;
							}

							ShellData.m_CollisionZ = CInfo.m_Pos.k[2];
						}
						else
						{
							CWObject_Client* pWorldSpawn = _pWClient->Object_Get(_pWClient->Object_GetWorldspawnIndex());
							if (pWorldSpawn)
							{
								CBox3Dfp32 WorldBound = *pWorldSpawn->GetAbsBoundBox();
								WorldBound.Grow(8.0f);
								ShellData.m_CollisionZ = WorldBound.m_Min.k[2];
								ShellData.m_Flags |= SHELLDATA_FLAGS_WORLDBOUNDING;
							}
						}
					}
					else
						bHitGround = true;
				}

				ShellData.m_Position.GetRow(3) = NewPos;

				if(bHitGround)
				{
					if(ShellData.m_Flags & SHELLDATA_FLAGS_WORLDBOUNDING)
					{
						ShellData.m_Life = -1.0f;
						ShellData.m_Flags &= ~SHELLDATA_FLAGS_WORLDBOUNDING;
					}
					else
					{
						//CBox3Dfp32 BBox(0,0);
						const CVec3Dfp32 ShellPosition = ShellData.m_Position.GetRow(3);
						//CXR_Model* pModel = _pShellTypes[ShellData.m_iShellType].m_pModel;
						//if(pModel)
						//	pModel->GetBound_Box(BBox);
						
						//const fp32 BoxHalfZ = ((BBox.m_Max.k[2] - BBox.m_Min.k[2]) * 0.5f);
						const fp32 BoxHalfZ = _pShellTypes[ShellData.m_iShellType].m_OffsetZ;
						ShellData.m_Position.Unit();
						ShellData.m_Position.RotZ_x_M(MFloat_GetRand(ShellData.m_Rand));
						ShellData.m_Position.GetRow(3) = CVec3Dfp32(ShellPosition.k[0], ShellPosition.k[1], ShellData.m_CollisionZ + BoxHalfZ);
					
						int32 iSound = _pShellTypes[ShellData.m_iShellType].m_iSound;
						if(iSound)
							_pWClient->Sound_At(WCLIENT_CHANNEL_SFX, ShellData.m_Position.GetRow(3), iSound, 0);
					}
					
					ShellData.m_Velocity = 0;
					ShellData.m_Flags &= ~SHELLDATA_FLAGS_MOVING;
				}
			}
			else
			{
				ShellData.m_Life -= _TickTime;
				if(ShellData.m_Life < 0)
				{
					if(iShell == m_iFirst)
					{
						ShellData.m_iPrev = iShell = SHELLDATA_PREV_FREE;
						m_iFirst = iNextShell;
					}
					else
					{
						ShellData.m_iPrev = SHELLDATA_PREV_FREE;
						_pShellData[iPrevShell].m_iPrev = iNextShell;
						iShell = iPrevShell;
					}

					m_nShells--;
				}
			}
		}

		iPrevShell = iShell;
		iShell = iNextShell;
	}
}


void CWO_Shell_ClientData::CShellOwnerData::OnRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, CWO_Shell_ClientData::CShellData* _pShellData, CWO_Shell_Type* _pShellTypes, const fp32& _TickTime, const fp32& _IPTime)
{
	// Make sure object has owner assigned
	if(!m_iOwner)
		return;

	// Render this owners shell types
	const CVec3Dfp32 Gravity = CVec3Dfp32(0, 0, -314.24f * _TickTime);
	
	uint32 nShells = 0;//m_nShells;
	int32 iShell = m_iFirst;
	int32 iShellModel = 0;
	int32 iNextShell = -1;
	uint32 iBone = 0;
	uint32 BoneTransformOffset = 0;

	CXR_SkeletonInstance* pSkelInst = (m_nShells > 0) ? CXR_VBMHelper::Alloc_SkeletonInst(_pEngine->GetVBM(), m_nShells, 0, 0) : NULL;
	if(!pSkelInst)
		return;

	while(nShells < m_nShells)
	{
		uint32 nCurrentShells = 0;
		CXR_Model* pModel = NULL;
		{
			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
			AnimState.m_pSkeletonInst = pSkelInst;
			
			while(iShell >= 0)
			{
				CWO_Shell_ClientData::CShellData& ShellData = _pShellData[iShell];
				
				if(ShellData.m_iModel == iShellModel || iShellModel == 0)
				{
					if(iShellModel == 0)
					{
						pModel = _pShellTypes[ShellData.m_iShellType].m_pModel;
						iShellModel = ShellData.m_iModel;
					}

					nCurrentShells++;

					CMat4Dfp32& IPMat = pSkelInst->m_pBoneTransform[iBone++];
					if(ShellData.m_Flags & SHELLDATA_FLAGS_MOVING)
					{
						CMat4Dfp32 DestMat = ShellData.m_Position;
						const uint16& Rand = ShellData.m_Rand;
						DestMat.M_x_RotZ((MFloat_GetRand(Rand) * _TickTime));
						DestMat.M_x_RotY((MFloat_GetRand(Rand+1) * _TickTime));
						DestMat.M_x_RotX((MFloat_GetRand(Rand+2) * _TickTime));
						DestMat.GetRow(3) += ((ShellData.m_Velocity + Gravity) * _TickTime);

						// Matrix lerp
						{
							CVec3Dfp32 IPPos;
							ShellData.m_Position.GetRow(3).Lerp(DestMat.GetRow(3), _IPTime, IPPos);

							CQuatfp32 q1, q2, q3;
							q1.Create(ShellData.m_Position);
							q2.Create(DestMat);
							q1.Lerp(q2, _IPTime, q3);

							q3.CreateMatrix(IPMat);
							IPMat.GetRow(3) = IPPos;
						}

						// Debug render velocity vectors
						#ifndef M_RTM
						{
							CVec3Dfp32 RenderVelocity = ShellData.m_Velocity;
							RenderVelocity.Normalize();
							_pWClient->Debug_RenderVector(ShellData.m_Position.GetRow(3), RenderVelocity * 4.0f);
						}
						#endif
					}
					else
					{
						IPMat.GetRow(0) = ShellData.m_Position.GetRow(0);
						IPMat.GetRow(1) = ShellData.m_Position.GetRow(1);
						IPMat.GetRow(2) = ShellData.m_Position.GetRow(2);
						IPMat.GetRow(3) = ShellData.m_Position.GetRow(3);
					}
				}
				else if(iNextShell < 0)
					iNextShell = iShell;

				// Fetch next shell to setup
				iShell = ShellData.m_iPrev;
			}

			// Opps...
			#ifndef M_RTM
			{
				if(nCurrentShells == 0 && iShell == -1 && iNextShell == -1)
				{
					M_TRACEALWAYS("CWO_Shell_ClientData::CShellOwnerData::OnRender: Invalid shell indicing. This should NEVER, EVER happen!\n");
					nCurrentShells = (m_nShells - nShells);
				}
			}
			#endif

			iShell = iNextShell;
			iNextShell = -1;
			iShellModel = 0;
			
			AnimState.m_Data[0] = BoneTransformOffset;
			AnimState.m_Data[1] = nCurrentShells;
			
			nShells += nCurrentShells;
			BoneTransformOffset += nCurrentShells;
			
			// Add model to engine
			if(pModel)
				_pEngine->Render_AddModel(pModel, _pObj->GetPositionMatrix(), AnimState, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_NOSHADOWS);
		}
	}
}


void CWO_Shell_ClientData::CShellData::Init(CWorld_Client* _pWClient, const CWO_Shell_Type& _ShellType, const CMat4Dfp32& _Position, const fp32& _CollisionZ, const int32& _iPrev)
{
	// Setup shell
	{
		const CVec3Dfp32& F = _ShellType.m_Force;
		const CVec3Dfp32& D = _ShellType.m_Direction;
		CVec3Dfp32& V = m_Velocity;
        uint16 iRandom = TruncToInt(Random * 65535.0f);

		// Setup data
		m_iModel = _ShellType.m_iModel;
		m_Position = _Position;
		m_Life = 6.0f + (MFloat_GetRand(iRandom+3) * 3.0f);
		m_Rand = iRandom+4;
		m_CollisionZ = _CollisionZ;
		m_iPrev = _iPrev;
		m_Flags = SHELLDATA_FLAGS_MOVING | SHELLDATA_FLAGS_FIRSTTICK;

		// Setup velocity vector
		V = CVec3Dfp32(D.k[0] + MFloat_GetRand(iRandom)*F.k[0], D.k[1] + MFloat_GetRand(iRandom+1)*F.k[1], D.k[2] + MFloat_GetRand(iRandom+2)*F.k[2]);
		V.MultiplyMatrix3x3(_Position);
	}
}


CWO_Shell_ClientData::CWO_Shell_ClientData(CWorld_PhysState* _pWPhysState)
	: m_nOwners(0)
	, m_MaxShells(0)
	, m_MaxOwners(0)
	, m_iShell(0)
	, m_iOldest(0)
	, m_pWPhysState(_pWPhysState)
{
	m_lShellTypes.Clear();
	m_lOwners.Clear();
	m_lSortedOwners.Clear();
	m_lShells.Clear();
}


void CWO_Shell_ClientData::AllocateData(const uint8& _MaxOwners, const uint16& _MaxShells)
{
	m_nOwners = 0;
	m_MaxShells = _MaxShells;
	m_MaxOwners = _MaxOwners;
	m_iShell = 0;
	m_iOldest = 0;
	m_lOwners.SetLen(_MaxOwners);
	m_lSortedOwners.SetLen(_MaxOwners);
	m_lShells.SetLen(_MaxShells);
}


int32 CWO_Shell_ClientData::FindOwnerContainer(const int32& _iOwner, const int32& _First, const int32& _Last, const bool _bInsert)
{
	CShellOwnerData** pShellOwners = m_lSortedOwners.GetBasePtr();

	if(m_nOwners > 0)
	{
		int32 First = _First;
		int32 Last = _Last;
		if(Last == -1)
			Last = m_nOwners-1;

		int32 i = 0;
		while(First <= Last)
		{
			i = First + ((Last - First) >> 1);
			if(pShellOwners[i]->m_iOwner == _iOwner)
				return i;
			if(_iOwner < pShellOwners[i]->m_iOwner)
				Last = i - 1;
			else
				First = i + 1;
		}
	}

	// Do we want to insert new object if not found ?
	if(_bInsert && m_nOwners + 1 < m_MaxOwners)
	{
		pShellOwners[m_nOwners] = &m_lOwners[m_nOwners];
		pShellOwners[m_nOwners]->m_iOwner = _iOwner;
		pShellOwners[m_nOwners]->m_iFirst = -1;
		pShellOwners[m_nOwners]->m_nShells = 0;
		
		QuickSortOwners_r(pShellOwners, ++m_nOwners);
		
		// Return wheter we found inserted object or not
		return FindOwnerContainer(_iOwner, 0, -1, false);
	}

	return -1;
}


void CWO_Shell_ClientData::UpdateOwnerContainers(uint16 _iShell)
{
	CShellOwnerData** pShellOwners = m_lSortedOwners.GetBasePtr();
	CShellData* pShells = m_lShells.GetBasePtr();

	for(uint16 i = 0; i < m_nOwners; i++)
	{
		uint32& nShells = pShellOwners[i]->m_nShells;
		int16& iFirst = pShellOwners[i]->m_iFirst;
		int16 iShell = iFirst;
		int16 iPrevShell = -1;
		int16 iNextShell = -1;

		while(iShell >= 0)
		{
			iNextShell = pShells[iShell].m_iPrev;

			if(iShell == _iShell)
			{
				if(nShells == 1)
				{
					nShells = 0;
					iFirst = 0;
				}
				else
				{
					// Fix link
					if(iShell == iFirst)
						iFirst = iNextShell;
					else
						pShells[iPrevShell].m_iPrev = iNextShell;
					
					nShells--;
				}
				return;
			}

			iPrevShell = iShell;
			iShell = iNextShell;
		}
	}
	
	// If we didn't find it, we shouldn't have to bother about it.
	//M_TRACEALWAYS("CWO_Shell_ClientData::UpdateOwnerContainers: Shell (%d) was indicated as in use, but was no where to be found!\n");
}


void CWO_Shell_ClientData::QuickSortOwners_r(CShellOwnerData** _pShellOwners, const int32& _Len)
{
	if(_Len == 2)
	{
		if(_pShellOwners[0]->m_iOwner > _pShellOwners[1]->m_iOwner)
			Swap(_pShellOwners[0], _pShellOwners[1]);

		return;
	}

	const uint32 iPivot = _Len / 2;
	const int32& PivotValue = _pShellOwners[iPivot]->m_iOwner;
	int32 iLeft = -1;
	int32 iRight = _Len;
	while(iLeft < iRight)
	{
		do
		{
			iLeft++;
		} while(iLeft <= iRight && (_pShellOwners[iLeft]->m_iOwner < PivotValue));

		do
		{
			iRight--;
		} while(iLeft <= iRight && (_pShellOwners[iRight]->m_iOwner > PivotValue));

		if(iLeft < iRight)
			Swap(_pShellOwners[iLeft], _pShellOwners[iRight]);
	}

	if(iLeft > 1)
		QuickSortOwners_r(_pShellOwners, iLeft);
	if(iRight < _Len - 2)
		QuickSortOwners_r(&_pShellOwners[iRight + 1], _Len - iRight - 1);
}


void CWO_Shell_ClientData::SpawnShell(CWorld_Client* _pWClient, const int8& _iShellType, const CWO_Shell_SpawnData& _SpawnData, const int32& _iOwner, const fp32& _CollisionZ)
{
	if(m_MaxShells <= 0)
		return;

	CShellData* pShells = m_lShells.GetBasePtr();

	if(m_iShell >= m_MaxShells)
		m_iShell = 0;

	const int32 iOwnerObject = FindOwnerContainer(_iOwner);
	if(iOwnerObject >= 0)
	{
		CShellOwnerData* pShellOwnerData = m_lSortedOwners[iOwnerObject];
		
		// Container is full, find owner and fix linking
		if(pShells[m_iShell].m_iPrev != SHELLDATA_PREV_FREE)
			UpdateOwnerContainers(m_iShell);

		pShells[m_iShell].m_iShellType = _iShellType;
		pShells[m_iShell].Init(_pWClient, m_lShellTypes[_iShellType], _SpawnData.m_AttachMat, _CollisionZ, pShellOwnerData->m_iFirst);

		pShellOwnerData->m_iFirst = m_iShell++;
		pShellOwnerData->m_nShells++;
	}
}


void CWO_Shell_ClientData::OnRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	MSCOPESHORT(CWO_Shell_ClientData::OnRender);

	CWO_Shell_Type* pShellTypes = m_lShellTypes.GetBasePtr();
	CShellData* pShellData = m_lShells.GetBasePtr();
	const fp32 IPTime = _pWClient->GetRenderTickFrac();
	const fp32 TickTime = _pWClient->GetGameTickTime();

	CShellOwnerData** pSortedOwners = m_lSortedOwners.GetBasePtr();
	for(int i = 0; i < m_nOwners; i++)
		pSortedOwners[i]->OnRender(_pObj, _pWClient, _pEngine, pShellData, pShellTypes, TickTime, IPTime);
}


void CWO_Shell_ClientData::OnRefresh(CWorld_Client* _pWClient, CBox3Dfp32& _BoundingBox)
{
	MSCOPESHORT(CWO_Shell_ClientData::OnRefresh);

	if(m_nOwners)
		_BoundingBox = CBox3Dfp32(_FP32_MAX, -_FP32_MAX);
	else
		_BoundingBox = CBox3Dfp32(0,0);

	CWObject_Client* pPlayerObj = _pWClient->Object_Get(_pWClient->Player_GetLocalObject());
	if(pPlayerObj)
		pPlayerObj->GetVisBoundBox(_BoundingBox);

    const fp32 TickTime = _pWClient->GetGameTickTime();
	CShellData* pShellData = m_lShells.GetBasePtr();
	CWO_Shell_Type* pShellTypes = m_lShellTypes.GetBasePtr();

	// Update sorted owner list
	CShellOwnerData** pSortedOwners = m_lSortedOwners.GetBasePtr();
	int nOwners = m_nOwners;
	int iIndex = 0;
	int iRemoved = 0;
	for(; iIndex < nOwners; iIndex++)
	{
		pSortedOwners[iIndex]->OnRefresh(_pWClient, _BoundingBox, TickTime, pShellData, pShellTypes);

		if(pSortedOwners[iIndex]->m_iOwner && !pSortedOwners[iIndex]->m_nShells)
		{
			m_nOwners--;
			iRemoved++;
		}
		else
			pSortedOwners[iIndex-iRemoved] = pSortedOwners[iIndex];
	}

	if(iRemoved && iRemoved != nOwners)
		QuickSortOwners_r(pSortedOwners, m_nOwners);

	_BoundingBox.Grow(8);
}


int CWO_Shell_ClientData::OnCreateClientUpdate(uint8* _pData) const
{
	uint8* pD = _pData;
	
	const CWO_Shell_Type* pShellTypes = m_lShellTypes.GetBasePtr();
	const int32 nShellTypes = m_lShellTypes.Len();
	int32 i;

	TAutoVar_Pack(nShellTypes, pD);
	for(i = 0; i < nShellTypes; i++)
		TAutoVar_Pack(pShellTypes[i], pD);

	return (pD - _pData);
}


int CWO_Shell_ClientData::OnClientUpdate(const uint8* _pData)
{
	const uint8* pD = _pData;
	int32 i;

	int32 nShellTypes = 0;
	TAutoVar_Unpack(nShellTypes, pD);

	m_lShellTypes.SetLen(nShellTypes);
	CWO_Shell_Type* pShellTypes = m_lShellTypes.GetBasePtr();
	for(i = 0; i < nShellTypes; i++)
	{
		TAutoVar_Unpack(pShellTypes[i], pD);
		pShellTypes[i].m_pModel = m_pWPhysState->GetMapData()->GetResource_Model(pShellTypes[i].m_iModel);
	}
	
	return (pD - _pData);
}


void CWO_Shell_ClientData::IncludeShellType(const int8& _iShellType, CWorld_Server* _pWServer)
{
	CWO_Shell_Type& Shell = m_lShellTypes.GetBasePtr()[_iShellType];
	CStr ModelName = _pWServer->GetMapData()->GetResourceName(Shell.m_iModel); ModelName.GetStrSep(":");
	int iModelIndex = _pWServer->GetMapData()->GetResourceIndex_Model(ModelName);
    _pWServer->GetMapData()->GetResource_Model(iModelIndex);
}


void CWO_Shell_ClientData::Write(CCFile* _pFile, CMapData* _pMapData)
{
	AutoVar_Write(_pFile, _pMapData);

	const uint8 nShellTypes = m_lShellTypes.Len();
	_pFile->WriteLE(nShellTypes);
	for(uint8 i = 0; i < nShellTypes; i++)
		TAutoVar_Write(m_lShellTypes[i], _pFile, _pMapData);
}


void CWO_Shell_ClientData::Read(CCFile* _pFile, CMapData* _pMapData)
{
	AutoVar_Read(_pFile, _pMapData);

	uint8 nShellTypes = 0;
	_pFile->ReadLE(nShellTypes);

	m_lShellTypes.SetLen(nShellTypes);
	for(uint i = 0; i < nShellTypes; i++)
		TAutoVar_Read(m_lShellTypes[i], _pFile, _pMapData);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Shell
|__________________________________________________________________________________________________
\*************************************************************************************************/
const CWO_Shell_ClientData& CWObject_Shell::GetClientData(const CWObject_CoreData* _pObj)
{
    const CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who deleted my client data?!");
	return *safe_cast<const CWO_Shell_ClientData>(pData);
}


CWO_Shell_ClientData& CWObject_Shell::GetClientData(CWObject_CoreData* _pObj)
{
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who deleted my client data?!");
	return *safe_cast<CWO_Shell_ClientData>(pData);
}


void CWObject_Shell::OnCreate()
{
	//ClientFlags() |= CWO_CLIENTFLAGS_LINKINFINITE;
}


void CWObject_Shell::OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer)
{
	int i = 0;
	while(_pReg->FindChild(CStrF("SHELL%i", i)))
	{
		const CRegistry* pShellReg = _pReg->FindChild(CStrF("SHELL%i", i));
		CWO_Shell_Type::OnIncludeTemplate(pShellReg, _pMapData, _pWServer);
		i++;
	}
}


void CWObject_Shell::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr ThisValue = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH3('MAXO','WNER','S'): // "MAXOWNERS"
		{
			m_nMaxOwners = ThisValue.Val_int();
			break;
		}
	case MHASH3('MAXM','AXSH','ELLS'): // "MAXMAXSHELLS"
		{
			m_nMaxShells = ThisValue.Val_int();
			break;
		}
	default:
		{
			if(KeyName.CompareSubStr("SHELL") == 0)
			{
				CWO_Shell_ClientData& CD = GetClientData(this);
				const int iShell = KeyName.CopyFrom(5).Val_int();

				// Get shell types, make sure we have enough space to hold types
				if(CD.m_lShellTypes.Len() <= iShell)
					CD.m_lShellTypes.SetLen(iShell+1);

				CWO_Shell_Type& ShellType = CD.m_lShellTypes.GetBasePtr()[iShell];

				int nChilds = _pKey->GetNumChildren();
				for(int i = 0; i < nChilds; i++)
				{
					const CRegistry* pChild = _pKey->GetChild(i);
					ShellType.OnEvalKey(m_pWServer, pChild);
				}
			}
			else
				CWObject_Shell_Parent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}


void CWObject_Shell::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	CWO_Shell_ClientData* pCD = TDynamicCast<CWO_Shell_ClientData>(pData);

	// Allocate clientdata
	if (!pCD)
	{
		pCD = MNew1(CWO_Shell_ClientData, _pWPhysState);
		if (!pCD)
			Error_static("CWO_Shell_ClientData", "Could not allocate client data!");

		_pObj->m_lspClientObj[0] = pCD;

		// Allocate data on client only!!
		if(_pWPhysState->IsClient())
			pCD->AllocateData(32, 256);
	}
}


void CWObject_Shell::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	MSCOPESHORT(CWObject_Shell::OnClientNetMsg);
	switch(_Msg.m_MsgType)
	{
		case CWO_SHELL_NETMSG_SPAWNSHELL:
		{
			MSCOPESHORT(CWO_SHELL_NETMSG_SPAWNSHELL);
			// Get message data
			int iPos = 0;
			CWO_Shell_SpawnData SpawnData;

			int32 iOwner = _Msg.GetInt32(iPos);
			int8 iShellType = _Msg.GetInt8(iPos);
			SpawnData.GetNetMsgData(_Msg, iPos);
			fp32 CollisionZ = _Msg.Getfp32(iPos);
			
			// Set shell position to whoever wanted a shell spawned
			CWObject_Client* pOwner = _pWClient->Object_Get(iOwner);
			if(pOwner)
				_pWClient->Object_SetPosition(_pObj->m_iObject, pOwner->GetPositionMatrix());

			// Setup new shell
			CWO_Shell_ClientData& CD = GetClientData(_pObj);
			CD.SpawnShell(_pWClient, iShellType, SpawnData, iOwner, CollisionZ);
		}
		break;

		default:
			break;
	}
}


void CWObject_Shell::AddNetMsgData(CNetMsg& _Msg, const int8& _ShellType)
{
	CWO_Shell_ClientData& CD = GetClientData(this);
	const CWO_Shell_Type& ShellType = CD.m_lShellTypes.GetBasePtr()[_ShellType];

	_Msg.Addfp32(ShellType.m_Force.k[0]);
	_Msg.Addfp32(ShellType.m_Force.k[1]);
	_Msg.Addfp32(ShellType.m_Force.k[2]);
	_Msg.Addfp32(ShellType.m_Direction.k[0]);
	_Msg.Addfp32(ShellType.m_Direction.k[1]);
	_Msg.Addfp32(ShellType.m_Direction.k[2]);
}


void CWObject_Shell::GetNetMsgData(const CNetMsg& _Msg, int& _iPos, CWO_Shell_Type& _ShellType)
{
	_ShellType.m_Force.k[0] = _Msg.Getfp32(_iPos);
	_ShellType.m_Force.k[1] = _Msg.Getfp32(_iPos);
	_ShellType.m_Force.k[2] = _Msg.Getfp32(_iPos);
	_ShellType.m_Direction.k[0] = _Msg.Getfp32(_iPos);
	_ShellType.m_Direction.k[1] = _Msg.Getfp32(_iPos);
	_ShellType.m_Direction.k[2] = _Msg.Getfp32(_iPos);
}


void CWObject_Shell::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	// Render client data
	CWO_Shell_ClientData& CD = GetClientData(_pObj);
	CD.OnRender(_pObj, _pWClient, _pEngine);
}


void CWObject_Shell::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_Shell::OnClientRefresh);
	CWObject_Shell_Parent::OnClientRefresh(_pObj, _pWClient);

	// Refresh system, by animating bullets.
	CBox3Dfp32 BoundingBox;
	CWO_Shell_ClientData& CD = GetClientData(_pObj);
	CD.OnRefresh(_pWClient, BoundingBox);
	
	// Update vis bounding box
	_pWClient->Object_SetVisBox(_pObj->m_iObject, BoundingBox.m_Min, BoundingBox.m_Max);
}


void CWObject_Shell::OnRefresh()
{
	MSCOPESHORT(CWObject_Shell::OnRefresh);
	CWObject_Shell_Parent::OnRefresh();

	//const int32& ClientFlags = ClientFlags();
	CWO_Shell_ClientData& CD = GetClientData(this);
	m_DirtyMask |= CD.AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}


int CWObject_Shell::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const
{
	int Flags = CWO_CLIENTUPDATE_EXTRADATA;
	if(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
		Flags = CWO_CLIENTUPDATE_AUTOVAR;
	
	uint8* pD = _pData;
	pD += CWObject_Shell_Parent::OnCreateClientUpdate(_iClient, _pClObjInfo, _pOld, _pData, Flags);
	if((pD - _pData) == 0)
		return (pD - _pData);

	const CWO_Shell_ClientData& CD = GetClientData(this);
	pD += CD.OnCreateClientUpdate(pD);
	CD.AutoVar_Pack(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT, pD, m_pWServer->GetMapData(), 0);

	return (pD - _pData);
}


int CWObject_Shell::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	CBox3Dfp32 BBox;
	_pObj->GetVisBoundBox(BBox);

	const uint8* pD = _pData;
	pD += CWObject_Shell_Parent::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);

	if(_pObj->m_iClass == 0 || (pD - _pData) == 0)
		return (pD - _pData);

	CWO_Shell_ClientData& CD = GetClientData(_pObj);
	pD += CD.OnClientUpdate(pD);
	if (_pObj->m_bAutoVarDirty)
		CD.AutoVar_Unpack(pD, _pWClient->GetMapData(), 0);

	_pWClient->Object_SetVisBox(_pObj->m_iObject, BBox.m_Min, BBox.m_Max);

	return (pD - _pData);
}

void CWObject_Shell::OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject::OnClientLoad(_pObj, _pWorld, _pFile, _pWData, _Flags);

	CWO_Shell_ClientData &CD = GetClientData(_pObj);
	CD.Read(_pFile, _pWData);
}

void CWObject_Shell::OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject::OnClientSave(_pObj, _pWorld, _pFile, _pWData, _Flags);

	CWO_Shell_ClientData &CD = GetClientData(_pObj);
	CD.Write(_pFile, _pWData);
}

void CWObject_Shell::IncludeShellType(const int8& _iShellType, CWorld_Server* _pWServer)
{
	CWO_Shell_ClientData& CD = GetClientData(this);
	CD.IncludeShellType(_iShellType, _pWServer);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_ShellManager
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWO_ShellManager::CWO_ShellManager()
	: m_pWServer(NULL)
	, m_pGameObject(NULL)
	, m_iContainerObject(0)
{
}


CWO_ShellManager::~CWO_ShellManager()
{
	if(m_iContainerObject)
		m_pWServer->Object_Destroy(m_iContainerObject);
}


void CWO_ShellManager::SpawnWorld(CWObject_Game* _pGameObject, CWorld_Server* _pWServer)
{
	m_pWServer = _pWServer;
	m_pGameObject = _pGameObject;

	// Create a container object
	m_iContainerObject = m_pWServer->Object_Create("ShellManager");
}


void CWO_ShellManager::CloseWorld()
{
	m_pWServer = NULL;
	m_pGameObject = NULL;
	m_iContainerObject = 0;
}


void CWO_ShellManager::SpawnShell(const int32& _iOwner, const int8& _ShellType, const CWO_Shell_SpawnData& _SpawnData)
{
	if(_ShellType < 0)
		return;

	CWObject_Shell* pShellObj = safe_cast<CWObject_Shell>(m_pWServer->Object_Get(m_iContainerObject));
	if(pShellObj)
	{
		 //Willbo: Nope this isn't a crash fix. Shouldn't be needed, removed during RTM. If it crashes here have a look in your template instead!
#ifndef M_RTM
		if (!CWObject_Shell::GetClientData(pShellObj).m_lShellTypes.ValidPos(_ShellType))
		{
			ConOutL(CStrF("Invalid shell type: %d", _ShellType));
			return;
		}
#endif

		CNetMsg NetMsg(CWO_SHELL_NETMSG_SPAWNSHELL);
		NetMsg.AddInt32(_iOwner);
		NetMsg.AddInt8(_ShellType);
		_SpawnData.AddNetMsgData(NetMsg);
		
		CWObject* pObject = m_pWServer->Object_Get(_iOwner);
		const fp32 CollisionZ = (pObject) ? pObject->GetPosition().k[2] : -100000.0f;
		NetMsg.Addfp32(CollisionZ);

		// ...And send it
		m_pWServer->NetMsg_SendToObject(NetMsg, m_iContainerObject);
	}
}


void CWO_ShellManager::IncludeClass(const char* _pClassName, CMapData* _pMapData)
{
	int iClass = _pMapData->GetResourceIndex_Class(_pClassName);
	if(iClass)
		_pMapData->GetResource_Class(iClass);
}

void CWO_ShellManager::IncludeTemplate(const char* _pTemplate, CMapData* _pMapData)
{
	_pMapData->GetResourceIndex_Template(_pTemplate);
}


void CWO_ShellManager::OnIncludeClass(CMapData* _pMapData, CWorld_Server *_pWServer)
{
	// Include shell object class
	IncludeClass("ShellManager", _pMapData);
	CWObject_Shell::OnIncludeClass(_pMapData, _pWServer);
}


void CWO_ShellManager::IncludeShellType(const int8& _iShellType, CWorld_Server* _pWServer)
{
	CWObject_Shell* pShellObj = safe_cast<CWObject_Shell>(_pWServer->Object_Get(m_iContainerObject));
	if (pShellObj)
		pShellObj->IncludeShellType(_iShellType, _pWServer);
}

