// Ragdoll implemented code

#include "PCH.h"
#include "../WObj_Char.h"
#include "WObj_CharRagdoll.h"

#include "MCC.h"

#include "../WObj_Game/WObj_GameCore.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PhysCluster.h"

#include "../../../../Shared/MOS/Classes/GameWorld/WDynamicsEngine/WDynamicsEngine2.h"


/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯| 
	CWPhys_RagdollClientData								  
|____________________________________________________________________________________|
\************************************************************************************/


//Initialize 
void CWPhys_RagdollClientData::Init(int _nTracks)
{
	m_lRot.SetLen(_nTracks);
	m_lMove.SetLen(_nTracks);
	m_nTracks = _nTracks;
	m_State = 0;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Function:	 Copy from another CD object	

    Parameters:
            _From:	Source object
\*____________________________________________________________________*/ 
void CWPhys_RagdollClientData::CopyFrom(const CWPhys_RagdollClientData &_From)
{
	m_State = _From.m_State;
	m_nTracks = _From.m_nTracks;
	m_lRot.QuickSetLen(0);
	m_lRot.Add(_From.m_lRot);
	m_lMove.QuickSetLen(0);
	m_lMove.Add(_From.m_lMove);
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Function:	Pack contents of CD into stream	

    Parameters:
            _pD:		Target stream
			_pMapData:	Unused (needed for template)
\*____________________________________________________________________*/ 
void CWPhys_RagdollClientData::Pack(uint8 *&_pD, class CMapData* _pMapData) const
{
	TAutoVar_Pack(m_State, _pD);
	TAutoVar_Pack(m_nTracks, _pD);

	uint16 nRot = m_lRot.Len();
	uint16 nMove = m_lMove.Len();
	TAutoVar_Pack(nRot, _pD);
	TAutoVar_Pack(nMove, _pD);

	const CQuatfp32* pRot = m_lRot.GetBasePtr();
	const CVec3Dfp32* pMove = m_lMove.GetBasePtr();

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
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Function:	Unpack contents from stream to CD

    Parameters:
            _pD:		Source stream
			_pMapData:	Unused (needed for template)
\*____________________________________________________________________*/ 
void CWPhys_RagdollClientData::Unpack(const uint8 *&_pD, class CMapData* _pMapData)
{
	TAutoVar_Unpack(m_State, _pD);
	TAutoVar_Unpack(m_nTracks, _pD);

	uint16 nRot, nMove;
	TAutoVar_Unpack(nRot, _pD);
	m_lRot.SetLen(nRot);
	TAutoVar_Unpack(nMove, _pD);
	m_lMove.SetLen(nMove);

	CQuatfp32* pRot = m_lRot.GetBasePtr();
	CVec3Dfp32* pMove = m_lMove.GetBasePtr();

	for(int iRot = 0; iRot < nRot; iRot++)
	{
		CXR_Anim_Quatint16 Q16;
		TAutoVar_Unpack(Q16.k[0], _pD);
		TAutoVar_Unpack(Q16.k[1], _pD);
		TAutoVar_Unpack(Q16.k[2], _pD);
		TAutoVar_Unpack(Q16.k[3], _pD);
		Q16.GetQuatfp32(pRot[iRot]);
	}

	if (nMove)
	{
		CBox3Dfp32 Box;
		TAutoVar_Unpack(Box.m_Min, _pD);
		TAutoVar_Unpack(Box.m_Max, _pD);

		CVec3Dfp32 BoxSize;
		Box.m_Max.Sub(Box.m_Min, BoxSize);
		BoxSize[0] *= 1.0f / 2047.0f;
		BoxSize[1] *= 1.0f / 2047.0f;
		BoxSize[2] *= 1.0f / 1023.0f;

		for(int iMove = 0; iMove < nMove; iMove++)
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
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Function:	Copy CD to skeleton instance

    Parameters:
            _pSkel:			Skeleton
			_pSkelinstance:	Skeleton instance
\*____________________________________________________________________*/ 
CVec3Dfp32 CWPhys_RagdollClientData::EvalAnim(CXR_Skeleton * _pSkel,CXR_SkeletonInstance * _pSkelInst,const CWPhys_RagdollClientMetaData *_pMD) const
{
	uint32 nC = Min<uint16>(_pMD->m_liBoneId.Len(),_pSkelInst->m_nBoneTransform);
	TAP<const CQuatfp32> pRot = m_lRot;
	TAP<const CVec3Dfp32> pMov = m_lMove;
	const uint8 * piB = _pMD->m_liBoneId.GetBasePtr();
	for(uint i = 0;i < nC;i++)
	{
		CQuatfp32 Rot = pRot[piB[i]];
		CVec3Dfp32 Mov = pMov[piB[i]];
		Rot.CreateMatrix(_pSkelInst->m_pBoneTransform[i]);
		Mov.SetRow(_pSkelInst->m_pBoneTransform[i],3);
	}

	CMat4Dfp32 MatJunk;
	_pSkelInst->m_pBoneTransform[0] = _pMD->m_OrgMat;
	_pSkelInst->m_pBoneTransform[1].Multiply(_pMD->m_OrgMat,MatJunk);

	uint nRotations = MaxMT(2, _pSkel->m_nUsedRotations);
	uint nMovements = MaxMT(2, _pSkel->m_nUsedMovements);
	_pSkelInst->CreateTracks(nRotations, nMovements);

	return MatJunk.GetRow(3);
}


/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯| 
	CWPhys_RagdollClientMetaData								  
|____________________________________________________________________________________|
\************************************************************************************/


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Function:	 Copy from another CD object	

    Parameters:
            _From:	Source object
\*____________________________________________________________________*/ 
void CWPhys_RagdollClientMetaData::CopyFrom(const CWPhys_RagdollClientMetaData &_From)
{
	m_OrgMat = _From.m_OrgMat;

	m_liBoneId.QuickSetLen(0);
	m_liBoneId.Add(_From.m_liBoneId);
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Function:	Pack contents of CD into stream	

    Parameters:
            _pD:		Target stream
			_pMapData:	Unused (needed for template)
\*____________________________________________________________________*/ 
void CWPhys_RagdollClientMetaData::Pack(uint8 *&_pD, class CMapData* _pMapData) const
{
	TAutoVar_Pack(m_OrgMat, _pD);

	uint16 nId = m_liBoneId.Len();
	TAutoVar_Pack(nId, _pD);

	const uint8* piB = m_liBoneId.GetBasePtr();
	for(int i = 0; i < nId; i++)
		TAutoVar_Pack( piB[i],_pD );
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Function:	Unpack contents from stream to CD

    Parameters:
            _pD:		Source stream
			_pMapData:	Unused (needed for template)
\*____________________________________________________________________*/ 
void CWPhys_RagdollClientMetaData::Unpack(const uint8 *&_pD, class CMapData* _pMapData)
{
	TAutoVar_Unpack(m_OrgMat, _pD);

	uint16 nId;
	TAutoVar_Unpack(nId, _pD);
	m_liBoneId.SetLen(nId);

	uint8* piB = m_liBoneId.GetBasePtr();
	for(int i = 0; i < nId; i++)
		TAutoVar_Unpack( piB[i],_pD );
}


/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯| 
	CWObject_Character								  
|____________________________________________________________________________________|
\************************************************************************************/


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Function:	Activate ragdoll state

    Parameters:
            _pCD:	Client data

	Returns:	True on success
\*____________________________________________________________________*/ 
bool CWObject_Character::Char_ActivateRagdoll(CWO_Character_ClientData *_pCD,bool _bCopyState)
{
	if( m_pPhysCluster )
	{
		M_TRACEALWAYS(CStrF("%s: Attempted to create ragdoll when already present",GetName()));
		return false;
	}

	MACRO_GetSystemEnvironment(pEnv);
	if (pEnv->GetValuei("DISABLE_RAGDOLL", 0) != 0)
		return false;

// -------------------------------------------------------------------
// Get skeleton instance
	CXR_AnimState Anim;
	CXR_Skeleton* pSkel;
	CXR_SkeletonInstance* pSkelInstance;					
	if( !GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, Anim) )
	{
		M_TRACEALWAYS(CStrF("%s: Failed to get skeleton instance",GetName()));
		return false;
	}

	m_pPhysCluster = DNew(CWPhys_Cluster) CWPhys_Cluster;
	uint nObj = pSkelInstance->m_nBoneTransform;
	CXR_SkeletonNode*	pNodes = pSkel->m_lNodes.GetBasePtr();

	m_OrgMat = GetPositionMatrix();


// -------------------------------------------------------------------
// Get real skeleton and set up matrix index list (aligned)
	CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
	if(!pModel)
	{
		M_TRACEALWAYS(CStrF("%s: Failed to acquire model for ragdoll",GetName()));
		delete m_pPhysCluster; m_pPhysCluster = NULL;
		return false;
	}
	CXR_Skeleton* pRealSkel = pModel->GetSkeleton();

	uint32 nIndex = pRealSkel->m_lNodes.Len();
	_pCD->m_RagdollClientMetaData.m_liBoneId.SetLen((nIndex + 3) & 0xFFFC);
	TAP<uint8> liMatrix = _pCD->m_RagdollClientMetaData.m_liBoneId;
	FillChar(liMatrix.GetBasePtr(),liMatrix.ListSize(),0xFF);


// -------------------------------------------------------------------
// Request that the game object create the ragdoll - get default if not found
// Tag the physcluster to not update object position first frame
	if( !CWObject_GameCore::CreateRagdoll((CWObject_GameCore*)m_pWServer->Game_GetObject(),
		m_pWServer, m_pPhysCluster, liMatrix.GetBasePtr(), m_iObject, m_lRagDollOffset, m_RagdollHash) )
	{
		LogFile(CStrF("ERROR: Object %s has invalid ragdoll, reverting to default",GetName()));
		if(! CWObject_GameCore::CreateRagdoll((CWObject_GameCore*)m_pWServer->Game_GetObject(),
				m_pWServer, m_pPhysCluster, liMatrix.GetBasePtr(), m_iObject, m_lRagDollOffset, MHASH2('DEFA','ULT')) )
		{
			delete m_pPhysCluster; m_pPhysCluster = NULL;
			return false;
		}
	}
	m_pPhysCluster->m_Flags |= PHYSCLUSTER_SKIPFIRSTTRANSFORM;


// -------------------------------------------------------------------
// Use real skeleton to fill matrix index list,
// Force metadata update
	{
		liMatrix[0] = 0;

		uint16	liStack[256];
		uint16	iStackPos = 1;
		liStack[0] = 0;
		while( iStackPos )
		{
			iStackPos--;
			CXR_SkeletonNode * pN = &pRealSkel->m_lNodes[liStack[iStackPos]];

			uint8 iThis = liMatrix[liStack[iStackPos]];
			for(uint i = 0;i < pN->m_nChildren;i++)
			{
				uint16 iNew = pRealSkel->m_liNodes[pN->m_iiNodeChildren+i];
				CXR_SkeletonNode *pNN = &pRealSkel->m_lNodes[iNew];

				if( liMatrix[iNew] == 0xFF ) liMatrix[iNew] = iThis;
				if( pNN->m_nChildren )
				{
					liStack[iStackPos++] = iNew;

#ifndef M_RTM
					if( iStackPos > 256 )
					{
						LogFile(CStrF("ERROR: Stack overflow when creating ragdoll for object %s",GetName()));
					}
#endif
				}
			}
		}

		//Clear 'unset' data
		for(uint i = 0;i < liMatrix.Len();i++)
			if( liMatrix[i] == 0xFF ) liMatrix[i] = 0;
	}

	_pCD->m_RagdollClientMetaData.MakeDirty();

// -------------------------------------------------------------------
// Move to current, add impulse
	if( _bCopyState )
	{
		TAP<CWPhys_ClusterObject> pPCO = m_pPhysCluster->m_lObjects;
		for(uint i = 0;i < pPCO.Len();i++)
		{
			// Yes, using PhysPrim.m_ObjectFlagsMask for Bone ID is FUGLY.
			// It isn't used beyond this point, however, and it is *never* used for anything else
			CXR_SkeletonNode * pN = &pSkel->m_lNodes[pPCO[i].m_PhysPrim.m_ObjectFlagsMask];

			//ObjectFlagsMask is only used temporarily so we reset it
			pPCO[i].m_PhysPrim.m_ObjectFlagsMask = 0xFFFF;
			if( pN->m_iRotationSlot < 0 ) continue;

			CMat4Dfp32 Mtx;
			pPCO[i].m_Transform.Multiply(pSkelInstance->m_pBoneTransform[pN->m_iRotationSlot],Mtx);
			pPCO[i].m_Transform = Mtx;

			m_pWServer->Phys_AddImpulse(pPCO[i].m_pRB,pPCO[i].m_Transform.GetRow(3),
				_pCD->m_AnimGraph2.m_PhysImpulse);
		}
	}

// -------------------------------------------------------------------
// Initialize client data, clear input and we're done
	_pCD->m_RagdollClientData.Init(m_pPhysCluster->m_lObjects.Len());
	_pCD->m_ExtraFlags = _pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_RAGDOLLACTIVE;

	_pCD->m_AnimGraph2.m_PhysImpulse = 0.0f;

	m_RagdollIdle = 0;

	return true;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Function:	Copy ragdoll information to client data

    Parameters:
            _pCD:	Client data
\*____________________________________________________________________*/ 
void CWObject_Character::Char_RagdollToCD(CWO_Character_ClientData *_pCD) const
{
	TAP<const CWPhys_ClusterObject> pPCO = m_pPhysCluster->m_lObjects;

	if( pPCO[0].m_pRB->IsStationary() )
	{
		_pCD->m_RagdollClientData.m_State = 3;
		return;
	}

// -------------------------------------------------------------------
// Since it isn't a physobject, there is no other Out-of-bounds- check
	if( CheckForOOB() )
	{
		ConOutL(CStrF("§cf80WARNING: Removing ragdoll for character '%s' because it is outside the world..", GetName()));
		return;
	}

// -------------------------------------------------------------------
// Create move data
	TAP<const CMat4Dfp32> pInv = m_lRagDollOffset;
	TAP<CQuatfp32> pQ = _pCD->m_RagdollClientData.m_lRot;
	TAP<CVec3Dfp32> pV = _pCD->m_RagdollClientData.m_lMove;
	for(uint i = 0;i < m_pPhysCluster->m_lObjects.Len();i++)
	{
		const CMat4Dfp32 &Trans = pPCO[i].m_Transform;
		const CMat4Dfp32 &InvMat = pInv[i];
		CMat4Dfp32 Mtx;

		InvMat.Multiply( Trans,Mtx );

		pQ[i].Create(Mtx);
		pV[i] = Mtx.GetRow(3);
	}


// -------------------------------------------------------------------
// Physstate and boundingbox
	CBox3Dfp32 BB;
	BB = m_pPhysCluster->GetBoundingBox();

	CWO_PhysicsState Phys;
	Phys.m_MediumFlags |= XW_MEDIUM_PLAYERSOLID | XW_MEDIUM_DYNAMICSSOLID;
	Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1,(BB.m_Max - BB.m_Min) * 0.5f,(BB.m_Max + BB.m_Min) * 0.5f);
	Phys.m_nPrim = 1;
	Phys.m_ObjectFlags &= OBJECT_FLAGS_WORLDTELEPORT | OBJECT_FLAGS_PLAYER;
	Phys.m_ObjectFlags |= OBJECT_FLAGS_PICKUP | OBJECT_FLAGS_TRIGGER | OBJECT_FLAGS_PHYSMODEL;
	Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_PROJECTILE | OBJECT_FLAGS_PHYSMODEL;
	Phys.m_PhysFlags = OBJECT_PHYSFLAGS_SLIDEABLE | OBJECT_PHYSFLAGS_PHYSMOVEMENT | OBJECT_PHYSFLAGS_OFFSET;
	// Make sure object matrix isn't turned upside down and stuff
	CMat4Dfp32 Pos = m_pPhysCluster->m_lObjects[0].m_Transform;
	_pCD->m_AnimGraph2.GetUpVector().SetMatrixRow(Pos,2);
	Pos.RecreateMatrix(2,0);
	// if we can't set physics, atleast set rotation
	if (!m_pWServer->Object_SetPhysics(m_iObject,Phys,Pos))
		m_pWServer->Object_SetRotation(m_iObject,Pos);


// -------------------------------------------------------------------
	// Finalize ClientData
	_pCD->m_RagdollClientData.MakeDirty();
	if( _pCD->m_RagdollClientData.m_State != 2 )
	{
		// Noooooo (might be in a ragdoll state but still not dead...)
		if (Char_GetPhysType(this) == PLAYER_PHYS_DEAD)
			_pCD->m_AnimGraph2.GetAG2I()->DisableAll();
		_pCD->m_RagdollClientData.m_State = 2;
	}
	_pCD->m_RagdollClientData.m_State = 1;
	_pCD->m_Phys_Flags = _pCD->m_Phys_Flags | 
		(PLAYER_PHYSFLAGS_NOGRAVITY | PLAYER_PHYSFLAGS_NOCHARACTERCOLL | PLAYER_PHYSFLAGS_NOWORLDCOLL);
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Function:	Write ragdoll deltadata

    Parameters:
		_pFile:	File pointer
\*____________________________________________________________________*/ 
void CWObject_Character::Char_RagdollDeltaWrite(CCFile * _pFile)
{
	if( !m_pPhysCluster )
	{
		M_ASSERT(false,CStrF("%s Tried to save ragdoll without having created one!",GetName()).Str());
	}

//These are just-in-case- values
	_pFile->WriteLE(m_RagdollHash);

	TAP<CWPhys_ClusterObject> pPCO = m_pPhysCluster->m_lObjects;
	uint32 nPCO = pPCO.Len();
	_pFile->WriteLE(nPCO);

// This would most likely be everything needed since the rest of it 
// is more or less arbitrary anyway
	for(uint i = 0;i < nPCO;i++)
	{
		_pFile->WriteLE( pPCO[i].m_Transform.k[0], sizeof(fp32) * 4 );
		_pFile->WriteLE( pPCO[i].m_Transform.k[1], sizeof(fp32) * 4 );
		_pFile->WriteLE( pPCO[i].m_Transform.k[2], sizeof(fp32) * 4 );
		_pFile->WriteLE( pPCO[i].m_Transform.k[3], sizeof(fp32) * 4 );
	}
}



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Function:	Read ragdoll deltadata

    Parameters:
		_pFile:	File pointer
\*____________________________________________________________________*/ 
void CWObject_Character::Char_RagdollDeltaRead(CCFile * _pFile,CWO_Character_ClientData* _pCD)
{
	CleanPhysCluster();

//Use this to be on the safe side
	_pFile->ReadLE(m_RagdollHash);
	Char_ActivateRagdoll(_pCD,false);

//Error-check
	uint32 nTf;
	TAP<CWPhys_ClusterObject> pPCO = m_pPhysCluster->m_lObjects;
	_pFile->ReadLE(nTf);
	M_ASSERT(nTf == pPCO.Len(),
		CStrF("%s - ragdoll deltaload failed, mismatch between ragdoll type & stored data",GetName()).Str());

//Set matrices
	for(uint i = 0;i < nTf;i++)
	{
		_pFile->ReadLE( pPCO[i].m_Transform.k[0], sizeof(fp32) * 4 );
		_pFile->ReadLE( pPCO[i].m_Transform.k[1], sizeof(fp32) * 4 );
		_pFile->ReadLE( pPCO[i].m_Transform.k[2], sizeof(fp32) * 4 );
		_pFile->ReadLE( pPCO[i].m_Transform.k[3], sizeof(fp32) * 4 );
	}
}
