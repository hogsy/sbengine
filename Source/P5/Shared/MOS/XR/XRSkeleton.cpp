#include "PCH.h"

#include "XRSkeleton.h"
#include "XRClass.h"
#include "XRAnim.h"
#include "MFloat.h"


MRTC_IMPLEMENT_DYNAMIC(CXR_SkeletonInstance, CReferenceCount);
MRTC_IMPLEMENT_DYNAMIC(CXR_Skeleton, CReferenceCount);

#define INVALID_QUAT(Q) ((FloatIsInvalid(Q.k[0]) || FloatIsInvalid(Q.k[1]) || FloatIsInvalid(Q.k[2]) || FloatIsInvalid(Q.k[3])))
#define INVALID_V3(Q) ((FloatIsInvalid(Q.k[0]) || FloatIsInvalid(Q.k[1]) || FloatIsInvalid(Q.k[2])))


/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_SkeletonNode
|_____________________________________________________________________________________
\************************************************************************************/
CXR_SkeletonNode::CXR_SkeletonNode()
{
	MAUTOSTRIP(CXR_SkeletonNode_ctor, MAUTOSTRIP_VOID);
	m_LocalCenterv = M_VConst(0,0,0,1.0f);
	m_Flags = 0;
	m_iiNodeChildren = 0;
	m_nChildren = 0;
	m_iTrackMask = CXR_SKELETON_INVALIDTRACKMASK;
	m_iNodeParent = -1;
	m_RotationScale = 1.0f;
	m_MovementScale = 1.0f;
	m_iRotationSlot = 0;
	m_iMovementSlot = 0;
}

void CXR_SkeletonNode::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CXR_SkeletonNode_Read, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();
	int16 Version;
	pF->ReadLE(Version);
	switch(Version)
	{
	case 0x0100 :
		{
			m_LocalCenter.Read(pF);
			pF->ReadLE(m_Flags);
			pF->ReadLE(m_iiNodeChildren);
			int16 nChildren;
			pF->ReadLE(nChildren);
			m_nChildren = nChildren;
			pF->ReadLE(m_iNodeParent);
			pF->ReadLE(m_RotationScale);
			pF->ReadLE(m_MovementScale);
			pF->ReadLE(m_iRotationSlot);
			pF->ReadLE(m_iMovementSlot);
		}
		break;
	default :
		Error_static("Read", CStrF("Unsupported node version. (%.4x)", Version));
	}

	if (m_iRotationSlot == 1)
		m_iMovementSlot = 1;
}

void CXR_SkeletonNode::Write(CDataFile* _pDFile) const
{
	MAUTOSTRIP(CXR_SkeletonNode_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	CCFile* pF = _pDFile->GetFile();
	pF->WriteLE((int16)CXR_SKELETONNODE_VERSION);
	m_LocalCenter.Write(pF);
	pF->WriteLE(m_Flags);
	pF->WriteLE(m_iiNodeChildren);
	pF->WriteLE(uint16(m_nChildren));
	pF->WriteLE(m_iNodeParent);
	pF->WriteLE(m_RotationScale);
	pF->WriteLE(m_MovementScale);

	pF->WriteLE(m_iRotationSlot);
	pF->WriteLE(m_iMovementSlot);
#endif	// PLATFORM_CONSOLE
}



/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_SkeletonAttachPoint
|_____________________________________________________________________________________
\************************************************************************************/
CXR_SkeletonAttachPoint::CXR_SkeletonAttachPoint()
{
	MAUTOSTRIP(CXR_SkeletonAttachPoint_ctor, MAUTOSTRIP_VOID);
	m_iNode = 0;
	__m_Padding = 0;
	m_LocalPos.Unit();
}

void CXR_SkeletonAttachPoint::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CXR_SkeletonAttachPoint_Read, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();
	uint16 Version;
	pF->ReadLE(Version);
	switch(Version)
	{
	case 0x0100: 
		{
			pF->ReadLE(m_iNode);
			pF->ReadLE(__m_Padding);
			m_LocalPos.Unit3x3();
			m_LocalPos.GetRow(3).Read(pF);
			break;
		}

	case 0x0101:
		{
			pF->ReadLE(m_iNode);
			pF->ReadLE(__m_Padding);
			m_LocalPos.Read(pF);
			break;
		}

	default :
		Error_static("CXR_SkeletonAttachPoint::Read", CStrF("Invalid attachpoint version: %.4x", Version));
	}
}

void CXR_SkeletonAttachPoint::Write(CDataFile* _pDFile) const
{
	MAUTOSTRIP(CXR_SkeletonAttachPoint_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	CCFile* pF = _pDFile->GetFile();
	pF->WriteLE((int16)CXR_SKELETONATTACH_VERSION);
	pF->WriteLE(m_iNode);
	pF->WriteLE(__m_Padding);
	m_LocalPos.Write(pF);
#endif	// PLATFORM_CONSOLE
}

bool CXR_SkeletonAttachPoint::Parse(CStr _Str)
{
	CVec3Dfp32 Pos, EulerRot;
	Pos.ParseString(_Str.GetStrSep(";"));
	EulerRot.ParseString(_Str.GetStrSep(";"));
	EulerRot *= (1.0f / 360.0f);
	uint iRotTrack = _Str.GetIntSep(";");
	if (iRotTrack > 256)
		return false;

	m_iNode = iRotTrack;
	__m_Padding = 0;
	EulerRot.CreateMatrixFromAngles(0, m_LocalPos);
	m_LocalPos.GetRow(3) = Pos;
	return true;
}


/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_ClothBoneWeights
|_____________________________________________________________________________________
\************************************************************************************/

void CXR_ClothBoneWeights::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CXR_ClothBoneWeights_Read, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();
	uint16 Version;
	pF->ReadLE(Version);
	switch(Version)
	{
	case CXR_CLOTHBONEWEIGHTS_VERSION: 
		{
			TArray<int> BoneIndex;
			int16 nWeights;
			pF->ReadLE(nWeights);
			pF->ReadLE(m_iClothJoint);
			for (int16 i = 0; i < nWeights; i++)
			{
				int16 index;
				pF->ReadLE(index);
				BoneIndex.Add(index);
			}
			for (int16 i = 0; i < nWeights; i++)
			{
				fp32 w;
				pF->ReadLE(w);
				Add(w,BoneIndex[i]);
			}
			break;
		}
	default :
		Error_static("CXR_ClothBoneWeights::Read", CStrF("Invalid clothconstraint version: %.4x", Version));
	}
}

void CXR_ClothBoneWeights::Write(CDataFile* _pDFile) const
{
	MAUTOSTRIP(CXR_ClothBoneWeights_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	CCFile* pF = _pDFile->GetFile();
	pF->WriteLE((int16)CXR_CLOTHBONEWEIGHTS_VERSION);

	pF->WriteLE(m_BoneCount);
	pF->WriteLE(m_iClothJoint);

	for (int i = 0; i < m_BoneCount; i++)
	{
		pF->WriteLE(m_iBoneIndex[i]);
	}

	for (int i = 0; i < m_BoneCount; i++)
	{
		pF->WriteLE(m_Weights[i]);
	}

#endif	// PLATFORM_CONSOLE
}



/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Cloth
|_____________________________________________________________________________________
\************************************************************************************/
CXR_Cloth::CXR_Cloth() 
{
	m_GroupId = 0;
	m_Name = CStr();
	InitParams(m_lParams);
}

CXR_Cloth::CXR_Cloth(int _groupid) 
{
	m_GroupId = _groupid;
	m_Name = CStr();
	InitParams(m_lParams);
}

void CXR_Cloth::InitParams(fp32 *_pParams)
{
	_pParams[CLOTHPARAM_COMPRESSIONFACTOR] = 0.95f;
	_pParams[CLOTHPARAM_STRETCHFACTOR] = 0.95f;
	_pParams[CLOTHPARAM_SHEARCOMPRESSIONFACTOR] = 0.95f;
	_pParams[CLOTHPARAM_SHEARSTRETCHFACTOR] = 0.95f;
	_pParams[CLOTHPARAM_CLOTHDENSITY] = 1.0f;
	_pParams[CLOTHPARAM_DAMPFACTOR] = 1.0f;
	_pParams[CLOTHPARAM_NITERATIONSTEPS] = 8;
	_pParams[CLOTHPARAM_SIMULATIONFREQUENCY] = 250.0f;
	_pParams[CLOTHPARAM_IMPULSEFACTOR] = 1.0f/5.0f;
	_pParams[CLOTHPARAM_BLENDFACTOR] = 0.5f;
	_pParams[CLOTHPARAM_BLENDFALLOFFACTOR] = 1000.0f;
	_pParams[CLOTHPARAM_MINBLENDFACTOR] = 0.0;
}

void CXR_Cloth::Read(CDataFile* _pDFile, const CXR_Skeleton* _pSkel)
{
	MAUTOSTRIP(CXR_Cloth_Read, MAUTOSTRIP_VOID);

	CCFile* pF = _pDFile->GetFile();
	if (!_pDFile->GetSubDir())
	{
		LogFile("Old cloth file format, ignored");
		return;
	}

	_pDFile->PushPosition();
	_pDFile->GetNext("VERSION");
		int32 Version;
		pF->ReadLE(Version);
		if (Version != (CXR_CLOTH_VERSION + 2))
		{
			LogFile("Invalid cloth version, ignored");
			return;
		}
	_pDFile->PopPosition();


	_pDFile->PushPosition();
	if (_pDFile->GetNext("NAME"))
	{
		m_Name.Read(pF);
	}
	_pDFile->PopPosition();

	/*
	int32 Version = _pDFile->GetUserData();
	if (Version != (CXR_CLOTH_VERSION + 2))
	{
		LogFile("Invalid cloth version, ignored");
		return;
	}*/

	_pDFile->PushPosition();
		_pDFile->GetNext("CONSTRAINTS");
		m_lConstraints.SetLen(_pDFile->GetUserData());
		for (int i = 0; i < m_lConstraints.Len(); i++) 
			m_lConstraints[i].Read(_pDFile);
	_pDFile->PopPosition();


	_pDFile->PushPosition();
	_pDFile->GetNext("CLOTHBONEWEIGHTS");
	m_lClothBoneWeights.SetLen(_pDFile->GetUserData());
		for (int i = 0; i < m_lClothBoneWeights.Len(); i++) 
			m_lClothBoneWeights[i].Read(_pDFile);
	_pDFile->PopPosition();


	TBitArray<MAX_BONE_COUNT_IN_BITARRAY> FixedJointsBitArray;
	FixedJointsBitArray.Clear();

	_pDFile->PushPosition();
	_pDFile->GetNext("FIXEDJOINTS");
		m_FixedJoints.SetLen(_pDFile->GetUserData());
		for(int i = 0; i < m_FixedJoints.Len(); i++) 
		{
			int16 tmp;
			pF->ReadLE(tmp);
			m_FixedJoints[i] = tmp;
			FixedJointsBitArray.Set1(tmp);
		}
	_pDFile->PopPosition();

	_pDFile->PushPosition();
	if (_pDFile->GetNext("INSIDECAPSULEJOINTS"))
	{
		m_InsideCapsuleJoints.SetLen(Min(int32(64), _pDFile->GetUserData()));
		for(int i = 0; i < m_InsideCapsuleJoints.Len(); i++) 
		{
			int16 tmp;
			pF->ReadLE(tmp);
			m_InsideCapsuleJoints[i] = tmp;
		}
	}
	_pDFile->PopPosition();

	_pDFile->PushPosition();
	_pDFile->GetNext("JOINTS");
		m_lJoints.SetLen(_pDFile->GetUserData());
		for(int i = 0; i < m_lJoints.Len(); i++) 
		{
			int16 tmp;
			pF->ReadLE(tmp);
			m_lJoints[i] = tmp;
		}
	_pDFile->PopPosition();

	_pDFile->PushPosition();
	_pDFile->GetNext("COLLISIONSOLIDS");
		m_lCollisionSolids.SetLen(_pDFile->GetUserData());
		for(int i = 0; i < m_lCollisionSolids.Len(); i++) 
		{
			int16 tmp;
			pF->ReadLE(tmp);
			m_lCollisionSolids[i] = tmp;
		}
	_pDFile->PopPosition();

	_pDFile->PushPosition();
	_pDFile->GetNext("COLLISIONCAPSULES");
		m_lCollisionCapsules.SetLen(_pDFile->GetUserData());
		for(int i = 0; i < m_lCollisionCapsules.Len(); i++) 
		{
			int16 tmp;
			pF->ReadLE(tmp);
			m_lCollisionCapsules[i] = tmp;
		}
	_pDFile->PopPosition();

	_pDFile->PushPosition();
	_pDFile->GetNext("PARAMETERS");
	int UD = _pDFile->GetUserData();
	int ParamVersion = UD >> 16;
	int n = UD & 0xffff;
	if(ParamVersion == 1)
	{
		for(int i = 0; i < n; i++)
			pF->ReadLE(m_lParams[i]);
	}
	else if(ParamVersion == 0)
	{
		for(int i = 0; i < n; i++) 
		{
			CStr ParamName;
			ParamName.Read(pF);
			fp32 value;
			pF->ReadLE(value);

			if (ParamName == "COMPRESSIONFACTOR")
				m_lParams[CLOTHPARAM_COMPRESSIONFACTOR] = value;
			else if (ParamName == "STRETCHFACTOR")
				m_lParams[CLOTHPARAM_STRETCHFACTOR] = value;
			else if (ParamName == "SHEARCOMPRESSIONFACTOR")
				m_lParams[CLOTHPARAM_SHEARCOMPRESSIONFACTOR] = value;
			else if (ParamName == "SHEARSTRETCHFACTOR")
				m_lParams[CLOTHPARAM_SHEARSTRETCHFACTOR] = value;
			else if (ParamName == "CLOTHDENSITY")
				m_lParams[CLOTHPARAM_CLOTHDENSITY] = value;
			else if (ParamName == "DAMPFACTOR")
				m_lParams[CLOTHPARAM_DAMPFACTOR] = value;
			else if (ParamName == "NITERATIONSTEPS")
				m_lParams[CLOTHPARAM_NITERATIONSTEPS] = value;
			else if (ParamName == "SIMULATIONFREQUENCY")
				m_lParams[CLOTHPARAM_SIMULATIONFREQUENCY] = value;
			else if (ParamName == "IMPULSFACTOR")
				m_lParams[CLOTHPARAM_IMPULSEFACTOR] = value;
			else if (ParamName == "BLENDFACTOR")
				m_lParams[CLOTHPARAM_BLENDFACTOR] = value;
			else if (ParamName == "BLENDFALLOFFACTOR")
				m_lParams[CLOTHPARAM_BLENDFALLOFFACTOR] = value;
			else if (ParamName == "MINBLENDFACTOR")
				m_lParams[CLOTHPARAM_MINBLENDFACTOR] = value;

		}
	}
	_pDFile->PopPosition();

	_pDFile->GetParent();


	uint nSkelNodes = _pSkel->m_lNodes.Len();
	m_MaxID = -1;
	TAP<int> piJoints = m_lJoints;
	for (int i = 0; i < piJoints.Len(); i++)
	{
		int iJoint = piJoints[i];
		if (iJoint >= nSkelNodes)
		{
			ConOutL(CStrF("§cf00ERROR: Invalid cloth!  (Name: %s - joint %d out of range. skeleton has %d)", m_Name.Str(), iJoint, nSkelNodes));
			m_MaxID = -1;
			return;
		}
		m_MaxID = Max(m_MaxID, iJoint);
	}

	// Init constraints
	for (int i = 0; i < m_lConstraints.Len(); i++) 
	{
		CXR_ClothConstraint& Constraint = m_lConstraints[i];
		M_ASSERTHANDLER(Constraint.m_id1 <= m_MaxID, "Cloth error!", continue);
		M_ASSERTHANDLER(Constraint.m_id2 <= m_MaxID, "Cloth error!", continue);

		Constraint.m_bFixed1 = FixedJointsBitArray.Get(Constraint.m_id1) ? 1.0f : 0.0f;
		Constraint.m_bFixed2 = FixedJointsBitArray.Get(Constraint.m_id2) ? 1.0f : 0.0f;

		vec128 p1 = _pSkel->m_lNodes[Constraint.m_id1].m_LocalCenterv;
		vec128 p2 = _pSkel->m_lNodes[Constraint.m_id2].m_LocalCenterv;
		vec128 diff = M_VSub(p1, p2);
		Constraint.m_restlength = M_VGetX_Slow( M_VSqrt(M_VDp3(diff, diff)) );
	}
}


void CXR_Cloth::Write(CDataFile* _pDFile, const CXR_Skeleton* _pSkel) const
{
	MAUTOSTRIP(CXR_Cloth_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	uint	nSkelNodes = _pSkel->m_lNodes.Len();
	for (int i = 0; i < m_lJoints.Len(); i++)
	{
		if (m_lJoints[i] >= nSkelNodes)
		{
			CStr msg;
			msg.CaptureFormated("Invalid cloth!  (Name: %s - joint %d is out of range (%d). Skeleton has only %d bones.)", 
								m_Name.Str(), i, m_lJoints[i], nSkelNodes);
			Error_static("CXR_Cloth::Write", msg.Str());
			return;
		}
	}

	CCFile* pF = _pDFile->GetFile();

	_pDFile->BeginEntry("CLOTH");
	_pDFile->EndEntry(CXR_CLOTH_VERSION+2);
	_pDFile->BeginSubDir();

	_pDFile->BeginEntry("VERSION");
		pF->WriteLE((int32) (CXR_CLOTH_VERSION+2));
	_pDFile->EndEntry(0);

	_pDFile->BeginEntry("NAME");
		m_Name.Write(pF);
	_pDFile->EndEntry(0);

	_pDFile->BeginEntry("CONSTRAINTS");
		for(int i = 0; i < m_lConstraints.Len(); i++) m_lConstraints[i].Write(_pDFile);
	_pDFile->EndEntry(m_lConstraints.Len());

	_pDFile->BeginEntry("CLOTHBONEWEIGHTS");
		for(int i = 0; i < m_lClothBoneWeights.Len(); i++) m_lClothBoneWeights[i].Write(_pDFile);
	_pDFile->EndEntry(m_lClothBoneWeights.Len());

	_pDFile->BeginEntry("FIXEDJOINTS");
		for(int i = 0; i < m_FixedJoints.Len(); i++) pF->WriteLE((int16) m_FixedJoints[i]);
	_pDFile->EndEntry(m_FixedJoints.Len());

	_pDFile->BeginEntry("INSIDECAPSULEJOINTS");
	for(int i = 0; i < m_InsideCapsuleJoints.Len(); i++) pF->WriteLE((int16) m_InsideCapsuleJoints[i]);
	_pDFile->EndEntry(m_InsideCapsuleJoints.Len());

	_pDFile->BeginEntry("JOINTS");
		for(int i = 0; i < m_lJoints.Len(); i++) pF->WriteLE((int16) m_lJoints[i]);
	_pDFile->EndEntry(m_lJoints.Len());

	_pDFile->BeginEntry("COLLISIONSOLIDS");
		for(int i = 0; i < m_lCollisionSolids.Len(); i++) pF->WriteLE((int16) m_lCollisionSolids[i]);
	_pDFile->EndEntry(m_lCollisionSolids.Len());

	_pDFile->BeginEntry("COLLISIONCAPSULES");
		for(int i = 0; i < m_lCollisionCapsules.Len(); i++) pF->WriteLE((int16) m_lCollisionCapsules[i]);
	_pDFile->EndEntry(m_lCollisionCapsules.Len());

	int16 ParamVersion = 0x0001;
	_pDFile->BeginEntry("PARAMETERS");
	for(int i = 0; i < NUM_CLOTHPARAMS; i++)
		pF->WriteLE(m_lParams[i]);
	_pDFile->EndEntry(NUM_CLOTHPARAMS | (ParamVersion << 16));

	_pDFile->EndSubDir();

#endif
}

#ifndef M_RTM
bool CXR_Cloth::ParseParamaters(const CRegistry &_Reg)
{
	fp32 lParam[NUM_CLOTHPARAMS];
	memcpy(lParam, m_lParams, sizeof(lParam));
	
	InitParams(m_lParams);

	static const char *slParams[] =
	{
		"COMPRESSIONFACTOR", "STRETCHFACTOR", "SHEARCOMPRESSIONFACTOR", "SHEARSTRETCHFACTOR", 
		"CLOTHDENSITY", "DAMPFACTOR", "NITERATIONSTEPS", "SIMULATIONFREQUENCY", "IMPULSFACTOR", "BLENDFACTOR", "BLENDFALLOFFACTOR", "MINBLENDFACTOR", NULL
		// Note that ImpulseFactor is missspelled. Not changed due to BW-compability -JA
	};

	for(int i = 0; i < _Reg.GetNumChildren(); i++)
	{
		int iParam = _Reg.GetName(i).TranslateInt(slParams);
		if(iParam != -1)
			m_lParams[iParam] = _Reg.GetValuef(i);
	}

	if(memcmp(lParam, m_lParams, sizeof(m_lParams)) != 0)
		return true;
	else
		return false;
}
#endif


/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_ClothConstraint
|_____________________________________________________________________________________
\************************************************************************************/
/*
CXR_ClothConstraint::CXR_ClothConstraint()
{
	m_id1=-1;
	m_id2=-1;
	m_weight=-1;
}
*/

CXR_ClothConstraint::CXR_ClothConstraint(int32 _id1, int32 _id2, fp64 _weight, int _type) {
	m_id1= _id1;
	m_id2= _id2;
	m_weight= _weight;
	m_type= _type;
}

void CXR_ClothConstraint::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CXR_ClothConstraint_Read, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();
	uint16 Version;
	pF->ReadLE(Version);
	switch(Version)
	{
	case CXR_CLOTHCONSTRAINT_VERSION: 
		{
			int32 id1,id2,type;
			pF->ReadLE(id1);
			pF->ReadLE(id2);
			pF->ReadLE(type);
			m_id1=int16(id1);
			m_id2=int16(id2);
			m_type=int16(type);
			pF->ReadLE(m_weight);
			break;
		}
	default :
		Error_static("CXR_ClothConstraint::Read", CStrF("Invalid clothconstraint version: %.4x", Version));
	}
}

void CXR_ClothConstraint::Write(CDataFile* _pDFile) const
{
	MAUTOSTRIP(CXR_ClothConstraint_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	CCFile* pF = _pDFile->GetFile();
	pF->WriteLE((int16)CXR_CLOTHCONSTRAINT_VERSION);
	pF->WriteLE(int32(m_id1));
	pF->WriteLE(int32(m_id2));
	pF->WriteLE(int32(m_type));
	pF->WriteLE(m_weight);
#endif	// PLATFORM_CONSOLE
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_SkeletonInstance
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXR_SkeletonInstance::CXR_SkeletonInstance()
{
	MAUTOSTRIP(CXR_SkeletonInstance_ctor, MAUTOSTRIP_VOID);
	m_pBoneTransform = NULL;
	m_pBoneLocalPos	= 0;
	m_pBoneScale	= 0;
	m_pTracksRot	= 0;
	m_pTracksMove	= 0;
	m_nBoneTransform	= 0;
	m_nBoneLocalPos	= 0;
	m_nBoneScale	= 0;
	m_nTracksRot	= 0;
	m_nTracksMove	= 0;
	m_Flags			= 0;
	m_VpuTaskId		= InvalidVpuTask;
	m_MoveScale		= 1.0f;
#ifndef M_RTM
	m_pDebugSkel = NULL;
#endif
}


CXR_SkeletonInstance::~CXR_SkeletonInstance()
{
	if(!(m_Flags & 1))
	{
		delete [] m_pBoneTransform;
		delete [] m_pBoneLocalPos;
		delete [] m_pBoneScale;
		delete [] m_pTracksRot;
		delete [] m_pTracksMove;
	}
}


void CXR_SkeletonInstance::Create(uint _nNodes)
{
	MAUTOSTRIP(CXR_SkeletonInstance_Create, MAUTOSTRIP_VOID);
	if(_nNodes == m_nBoneTransform)
		return;

	if( !(m_Flags & 1) )
	{
		CMat4Dfp32* pPrevBoneLocalPos = m_pBoneLocalPos;
		CMat4Dfp32* pPrevBoneTransform = m_pBoneTransform;
		fp32* pPrevBoneScale = m_pBoneScale;

		m_pBoneLocalPos		= DNew(CMat4Dfp32 ) CMat4Dfp32 [_nNodes + 1];
		m_pBoneTransform	= DNew(CMat4Dfp32 ) CMat4Dfp32 [_nNodes + 1];	// Should actually only be 4 extra bytes here but to get correct constructor/destructor typ we allocate an extra instance
		m_pBoneScale		= DNew(fp32 ) fp32 [_nNodes + 1];

		M_ASSERT(m_pBoneLocalPos, "Out of memory");
		M_ASSERT(m_pBoneTransform, "Out of memory");
		M_ASSERT(m_pBoneScale, "Out of memory");

		if(m_nBoneTransform > 0)
		{
			memcpy(m_pBoneLocalPos, pPrevBoneLocalPos, MinMT(m_nBoneLocalPos, _nNodes) * sizeof(CMat4Dfp32));
			memcpy(m_pBoneTransform, pPrevBoneTransform, MinMT(m_nBoneTransform, _nNodes) * sizeof(CMat4Dfp32));
			memcpy(m_pBoneScale, pPrevBoneScale, MinMT(m_nBoneScale, _nNodes) * sizeof(fp32));

			delete [] pPrevBoneLocalPos;
			delete [] pPrevBoneTransform;
			delete [] pPrevBoneScale;
		}

		for(int i = m_nBoneTransform; i < _nNodes; i++)
		{
			m_pBoneLocalPos[i].Unit();
			m_pBoneTransform[i].Unit();
			// Scale difference 0 == no difference
			m_pBoneScale[i] = 0.0f;
		}

		m_nBoneTransform	= _nNodes;
		m_nBoneLocalPos	= _nNodes;
		m_nBoneScale	= _nNodes;
	}
}


void CXR_SkeletonInstance::CreateTracks(uint16 _nRot, uint16 _nMove)
{
	MAUTOSTRIP(CXR_SkeletonInstance_CreateTracks, MAUTOSTRIP_VOID);

	if(!(m_Flags & 1))
	{
		if(_nRot != m_nTracksRot)
		{
			delete [] m_pTracksRot;
			m_pTracksRot	= DNew(CQuatfp32 ) CQuatfp32 [_nRot];
			M_ASSERT(m_pTracksRot, "Out of memory");
			m_nTracksRot	= _nRot;
		}
		if(_nMove != m_nTracksMove)
		{
			delete [] m_pTracksMove;
			m_pTracksMove	= DNew(CVec4Dfp32 ) CVec4Dfp32 [_nMove];
			M_ASSERT(m_pTracksMove, "Out of memory");
			m_nTracksMove	= _nMove;
		}
#ifndef M_RTM
		m_pDebugSkel = NULL;
#endif
	}
	else
	{
#ifdef M_Profile
		if((_nRot != m_nTracksRot) || (_nMove != m_nTracksMove))
			M_BREAKPOINT;
#endif
	}
}


void CXR_SkeletonInstance::Duplicate(CXR_SkeletonInstance* _pDest) const
{
	_pDest->Create(m_nBoneLocalPos);
	memcpy(_pDest->m_pBoneLocalPos, m_pBoneLocalPos, sizeof(CMat4Dfp32) * m_nBoneLocalPos);
	memcpy(_pDest->m_pBoneTransform, m_pBoneTransform, sizeof(CMat4Dfp32) * m_nBoneTransform);
	memcpy(_pDest->m_pBoneScale, m_pBoneScale, sizeof(fp32) * m_nBoneScale);

	if((m_nTracksRot > 0) || (m_nTracksMove > 0))
	{
		_pDest->CreateTracks(m_nTracksRot, m_nTracksMove);
		if(m_nTracksRot > 0) memcpy(_pDest->m_pTracksRot, m_pTracksRot, sizeof(CQuatfp32) * m_nTracksRot);
		if(m_nTracksMove > 0) memcpy(_pDest->m_pTracksMove, m_pTracksMove, sizeof(CVec4Dfp32) * m_nTracksMove);
	}
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Sets given scale to all bones
						
	Parameters:		
		_Scale:			Scale difference factor
			
	Comments:		
\*____________________________________________________________________*/
void CXR_SkeletonInstance::ApplyScale(fp32 _Scale)
{
	MAUTOSTRIP(CXR_SkeletonInstance_ApplyScale, MAUTOSTRIP_VOID);
	// Apply the scale to all bones
	if (!m_pBoneScale)
		return;

	fp32* pBoneScale = m_pBoneScale;
	int nBones = m_nBoneScale;
	for (int i = 0; i < nBones; i++)
		pBoneScale[i] = _Scale;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Sets the given scale to the given set of bones
						
	Parameters:
		_Scale:			Scale difference factor
		_lBoneSet:		Set of bones to apply scale to
			
	Comments:		
\*____________________________________________________________________*/
void CXR_SkeletonInstance::ApplyScale(fp32 _Scale, const TArray<int>& _liBoneSet)
{
	MAUTOSTRIP(CXR_SkeletonInstance_ApplyScale_2, MAUTOSTRIP_VOID);
	// Apply the scale to the given set of bones (indices)
	TAP_RCD<const int> piBoneSet(_liBoneSet);
	
	fp32* pBoneScale = m_pBoneScale;
	int nBoneScale = m_nBoneScale;
	for (int i = 0; i < piBoneSet.Len(); i++)
	{
		int iIndex = piBoneSet[i];
		if ((iIndex < nBoneScale) && (iIndex >= 0))
			pBoneScale[iIndex] = _Scale;
	}
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Sets the given scale to the given set of bones
						
	Parameters:		
		_Scale:			Scale difference factor
		_pliBoneSet:	Set of bones to apply scale to
		_BonesInSet:	Number of bones in set
			
	Comments:		Make sure _BonesInSet matches the number of bones
					in the array
\*____________________________________________________________________*/
void CXR_SkeletonInstance::ApplyScale(fp32 _Scale, const int* _pliBoneSet, int _BonesInSet)
{
	MAUTOSTRIP(CXR_SkeletonInstance_ApplyScale_3, MAUTOSTRIP_VOID);
	if (!_pliBoneSet)
		return;
	
	// Apply the scale to the given set of bones (indices)
	for (int i = 0; i < _BonesInSet; i++)
	{
		int iIndex = _pliBoneSet[i];
		if ((iIndex < m_nBoneScale) && (iIndex >= 0))
			m_pBoneScale[iIndex] = _Scale;
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Blends one skelinstance with another
						
	Parameters:		
		_pInstance:		Instance to blend with 
		_pTarget:		Where to store the result
		_Blen:			BlendAmount
			
	Comments:		Make sure _BonesInSet matches the number of bones
					in the array
\*____________________________________________________________________*/
bool CXR_SkeletonInstance::BlendInstance(const CXR_SkeletonInstance* _pInstance, const CXR_SkeletonInstance* _pTarget, fp32 _Blend) const
{
	if (!_pInstance || !_pTarget)
		return false;

	int32 NumBones = Min(Min(_pInstance->GetNumBones(),GetNumBones()),_pTarget->GetNumBones());
	int32 NumOtherBones = _pInstance->GetNumBones();
	int32 NumTargetBones = _pInstance->GetNumBones();

	CQuatfp32 Q1,Q2,Q3;
	int32 i;
	for (i = 0; i < NumBones; i++)
	{
		// For each bone do a matrix blend....
		const CMat4Dfp32& Mat = m_pBoneLocalPos[i];
		const CMat4Dfp32& MatOther = _pInstance->m_pBoneLocalPos[i];
		CMat4Dfp32& MatTarget = _pTarget->m_pBoneLocalPos[i];
		
		Q1.Create(Mat);
		Q2.Create(MatOther);
		Q1.Lerp(Q2, _Blend, Q3);
		Q3.CreateMatrix(MatTarget);
		CVec3Dfp32::GetMatrixRow(Mat,3).Lerp(CVec3Dfp32::GetMatrixRow(MatOther,3),_Blend,CVec3Dfp32::GetMatrixRow(MatTarget,3));
	}

	// Blend any rots in original not in instance to unit (if any)
	NumBones = Min(NumTargetBones,(int32)GetNumBones());
	Q2.Unit();
	CVec3Dfp32 ZeroPos(0.0f,0.0f,0.0f);
	for (; i < NumBones; i++)
	{
		const CMat4Dfp32& Mat = m_pBoneLocalPos[i];
		CMat4Dfp32& MatTarget = _pTarget->m_pBoneLocalPos[i];
		Q1.Create(Mat);
		Q1.Lerp(Q2, _Blend, Q3);
		Q3.CreateMatrix(MatTarget);
		CVec3Dfp32::GetMatrixRow(Mat,3).Lerp(ZeroPos,_Blend,CVec3Dfp32::GetMatrixRow(MatTarget,3));
	}

	// Blend any rots in instance not in original from unit (if any)
	NumBones = Min(NumOtherBones,NumTargetBones);
	for (; i < NumBones; i++)
	{
		const CMat4Dfp32& Mat = _pInstance->m_pBoneLocalPos[i];
		CMat4Dfp32& MatTarget = _pTarget->m_pBoneLocalPos[i];
		Q1.Create(Mat);
		Q2.Lerp(Q1, _Blend, Q3);
		Q3.CreateMatrix(MatTarget);
		ZeroPos.Lerp(CVec3Dfp32::GetMatrixRow(Mat,3),_Blend,CVec3Dfp32::GetMatrixRow(MatTarget,3));
	}

	// Zero out any remaining transforms
	for (; i < NumTargetBones; i++)
		_pTarget->m_pBoneLocalPos[i].Unit();

	return true;
}


/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Skeleton
|_____________________________________________________________________________________
\************************************************************************************/
CXR_Skeleton::CXR_Skeleton()
{
	MAUTOSTRIP(CXR_Skeleton_ctor, MAUTOSTRIP_VOID);
	m_nUsedRotations = 2;
	m_nUsedMovements = 2;
}

//static int g_TotalTrackMask = 0;

void CXR_Skeleton::Init()
{
	MAUTOSTRIP(CXR_Skeleton_Init, MAUTOSTRIP_VOID);
	m_nUsedRotations = 2;
	m_nUsedMovements = 2;
	for(int i = 0; i < m_lNodes.Len(); i++)
	{
		CXR_SkeletonNode* pN = &m_lNodes[i];
		if (pN->m_iRotationSlot >= int(m_nUsedRotations)) m_nUsedRotations = pN->m_iRotationSlot+1;
		if (pN->m_iMovementSlot >= int(m_nUsedMovements)) m_nUsedMovements = pN->m_iMovementSlot+1;
	}

	if (m_nUsedRotations > CXR_SKELETON_MAXROTATIONS) Error("Init", "Too many rotation-slots used.");
	if (m_nUsedMovements > CXR_SKELETON_MAXMOVEMENTS) Error("Init", "Too many movement-slots used.");


//	g_TotalTrackMask += m_lNodes.Len();

//	M_TRACEALWAYS("CurrentTrackMasks %d, %d bytes (%d per)\n", g_TotalTrackMask, g_TotalTrackMask*sizeof(CXR_Anim_TrackMask), sizeof(CXR_Anim_TrackMask) );

	InitTrackMask();

/*	for (int i=0; i<m_lClothConstraints.Len(); i++)
	{
		CXR_ClothConstraint& clothcons= m_lClothConstraints[i];
		CVec3Dfp32 p1= m_lNodes[clothcons.m_id1].m_LocalCenter;
		CVec3Dfp32 p2= m_lNodes[clothcons.m_id2].m_LocalCenter;
		clothcons.m_restlength= p1.Distance(p2);
	}*/

}


void CXR_Skeleton::CreateTrackNodeMask_r(uint16 _iNode, CXR_Anim_TrackMask& _Mask, CXR_Anim_TrackMask& _NodeMask) const
{
	const CXR_SkeletonNode& Node = m_lNodes[_iNode];

	_NodeMask.m_TrackMaskRot.Enable(_iNode);
	_Mask.m_TrackMaskRot.Enable(_iNode);

	int iRot = Node.m_iRotationSlot;
	if (iRot >= 0)
		_Mask.m_TrackMaskRot.Enable(iRot);
	int iMove = Node.m_iMovementSlot;
	if (iMove >= 0)
		_Mask.m_TrackMaskMove.Enable(iMove);

	int nCh = Node.m_nChildren;
	if (!nCh)
		return;

	const uint16* piNodes = &m_liNodes[Node.m_iiNodeChildren];
	for(int i = 0; i < nCh; i++)
		CreateTrackNodeMask_r(piNodes[i], _Mask, _NodeMask);
}


void CXR_Skeleton::GetTrackMask(uint16 _iNode, CXR_Anim_TrackMask*& _pTrackMask, CXR_Anim_TrackMask*& _pNodeMask, CXR_Anim_TrackMask*& _pCoverageMask) const
{
	const CXR_SkeletonNode& Node = m_lNodes[_iNode];
	if (Node.m_iTrackMask == CXR_SKELETON_INVALIDTRACKMASK)
	{
		// Double if so we can avoid lock whenever necessary
		M_LOCK(m_Lock);
		if (Volatile(Node.m_iTrackMask) == CXR_SKELETON_INVALIDTRACKMASK)
		{
			int iTrackMask = m_lspNodeTrackMask.Len();
			m_lspNodeTrackMask.SetLen(m_lspNodeTrackMask.Len()+3);
			m_lspNodeTrackMask[iTrackMask] = MNew(CXR_Anim_TrackMask);
			m_lspNodeTrackMask[iTrackMask+1] = MNew(CXR_Anim_TrackMask);
			m_lspNodeTrackMask[iTrackMask+2] = MNew(CXR_Anim_TrackMask);
			CXR_Anim_TrackMask& TrackMask = *m_lspNodeTrackMask[iTrackMask];
			CXR_Anim_TrackMask& CoverageMask = *m_lspNodeTrackMask[iTrackMask+1];
			CXR_Anim_TrackMask& NodeMask = *m_lspNodeTrackMask[iTrackMask+2];
			TrackMask.Clear();
			NodeMask.Clear();
			CreateTrackNodeMask_r(_iNode, TrackMask, NodeMask);

			CoverageMask.Copy(TrackMask);

			do
			{
				_iNode = m_lNodes[_iNode].m_iNodeParent;
				const CXR_SkeletonNode& Node = m_lNodes[_iNode];
				int iRot = Node.m_iRotationSlot;
				if (iRot >= 0)
					TrackMask.m_TrackMaskRot.Enable(iRot);
				int iMove = Node.m_iMovementSlot;
				if (iMove >= 0)
					TrackMask.m_TrackMaskMove.Enable(iMove);
			}
			while(_iNode);

			// Setting this validates the first if, make sure all data exists before then
			Node.m_iTrackMask = iTrackMask / 3;
		}
	}

	uint iTrackMask = Node.m_iTrackMask * 3;
	_pTrackMask = m_lspNodeTrackMask[iTrackMask + 0];
	_pCoverageMask = m_lspNodeTrackMask[iTrackMask + 1];
	_pNodeMask = m_lspNodeTrackMask[iTrackMask + 2];
}


void CXR_Skeleton::InitTrackMask()
{
	m_TrackMask.Clear();
	m_NodeMask.Clear();

	if (m_lNodes.Len())
		CreateTrackNodeMask_r(0, m_TrackMask, m_NodeMask);

//	for(int iAttach = 0; iAttach < m_lAttachPoints.Len(); iAttach++)
//		m_TrackMask.m_TrackMaskRot.Enable(m_lAttachPoints[iAttach].m_iNode);


/*	int nNodes = m_lNodes.Len();
	const CXR_SkeletonNode* pNodes = m_lNodes.GetBasePtr();

	for(int i = 0; i < nNodes; i++)
	{
		int iRot = pNodes[i].m_iRotationSlot;
		if (iRot >= 0)
			m_TrackMask.m_TrackMaskRot.Enable(iRot);
		int iMove = pNodes[i].m_iMovementSlot;
		if (iMove >= 0)
			m_TrackMask.m_TrackMaskMove.Enable(iMove);

		m_NodeMask.m_TrackMaskRot.Enable(i);
	}*/
}


void CXR_Skeleton::InitNode(uint16 _iNode, CXR_SkeletonInstance* _pInst, const CMat4Dfp32* _pRot, const CVec3Dfp32* _pMove, uint16 _nRot, uint16 _nMove) const
{
	MAUTOSTRIP(CXR_Skeleton_InitNode, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Skeleton::InitNode);
	InitNode_i( _iNode, _pInst, _pRot, _pMove, _nRot, _nMove );
}


/*
void CXR_Skeleton::InitNode_r(int16 _iNode, CXR_SkeletonInstance* _pInst, const CMat4Dfp32* _pRot, const CVec3Dfp32* _pMove, int _nRot, int _nMove) const
{
	MAUTOSTRIP(CXR_Skeleton_InitNode_r, MAUTOSTRIP_VOID);
	if (_iNode >= m_lNodes.Len()) return;
	const CXR_SkeletonNode* pN = &m_lNodes[_iNode];
	
	CMat4Dfp32& Mat = _pInst->m_lBoneLocalPos[_iNode];
	if (_pRot) 
	{
		int iRot = pN->m_iRotationSlot;
		if ((iRot < 0) || (iRot >= _nRot))
		{
			Mat.Unit();
		}
		else
		{
			Mat = _pRot[iRot];
		}

		int iMove = pN->m_iMovementSlot;
		if ((iMove >= 0) && (iMove < _nMove))
		{
			CVec3Dfp32::GetMatrixRow(Mat, 3) = -_pMove[iMove];
		}
	}
	else
		Mat.Unit();

	int nCh = pN->m_nChildren;
	for(int i = 0; i < nCh; i++)
		InitNode_r(m_liNodes[pN->m_iiNodeChildren + i], _pInst, _pRot, _pMove, _nRot, _nMove);
}
*/

void CXR_Skeleton::InitNode_i(uint16 _iNode, CXR_SkeletonInstance* _pInst, const CMat4Dfp32* _pRot, const CVec3Dfp32* _pMove, uint16 _nRot, uint16 _nMove) const
{
	MAUTOSTRIP(CXR_Skeleton_InitNode_i, MAUTOSTRIP_VOID);
	
	const uint16* piNodes = m_liNodes.GetBasePtr();
	const uint16 NodeCount = m_lNodes.Len();
	const CXR_SkeletonNode* pNodes = m_lNodes.GetBasePtr();
	CMat4Dfp32* pBoneLocalPos = _pInst->m_pBoneLocalPos;
	uint16 aWorkingSet[256];
	int WorkingStack = 0;
	aWorkingSet[WorkingStack++]	= _iNode;

	if( _pRot )
	{
StartOf_InitNode_i_With_pRot:
		uint16 WorkingNode = aWorkingSet[--WorkingStack];
		if (WorkingNode >= NodeCount)
			goto Continue_With_pRot;

		{
			const CXR_SkeletonNode* pN = &pNodes[WorkingNode];
			
			CMat4Dfp32& Mat = pBoneLocalPos[WorkingNode];
			{
				uint16 iRot = pN->m_iRotationSlot;
				if (iRot >= _nRot)
				{
					Mat.Unit();
				}
				else
				{
					Mat = _pRot[iRot];
				}

				uint16 iMove = pN->m_iMovementSlot;
				if (iMove < _nMove)
				{
					CVec3Dfp32::GetMatrixRow(Mat, 3) = -_pMove[iMove];
				}
			}

			uint nCh = pN->m_nChildren;
			for(uint i = 0; i < nCh; i++ )
				aWorkingSet[WorkingStack++]	= piNodes[pN->m_iiNodeChildren + i];
		}
			
Continue_With_pRot:
		if( WorkingStack > 0 )
			goto StartOf_InitNode_i_With_pRot;
	}
	else
	{
StartOf_InitNode_i_No_pRot:
		uint16 WorkingNode = aWorkingSet[--WorkingStack];
		if (WorkingNode >= NodeCount)
			goto Continue_No_pRot;

		{
			pBoneLocalPos[WorkingNode].Unit();

			const CXR_SkeletonNode* pN = &pNodes[WorkingNode];
			uint nCh = pN->m_nChildren;
			for(uint i = 0; i < nCh; i++ )
				aWorkingSet[WorkingStack++]	= piNodes[pN->m_iiNodeChildren + i];
		}
			
Continue_No_pRot:
		if( WorkingStack > 0 )
			goto StartOf_InitNode_i_No_pRot;
	}
}


void CXR_Skeleton::EvalPosition(uint16 _iNode, const CMat4Dfp32* _pTransform, CXR_SkeletonInstance* _pInst) const
{
	MAUTOSTRIP(CXR_Skeleton_EvalPosition, MAUTOSTRIP_VOID);
	EvalPosition_r( _iNode, _pTransform, _pInst );
}


void CXR_Skeleton::EvalPosition_r(uint16 _iNode, const CMat4Dfp32* _pTransform, CXR_SkeletonInstance* _pInst) const
{
	MAUTOSTRIP(CXR_Skeleton_EvalPosition_r, MAUTOSTRIP_VOID);
	const CXR_SkeletonNode* pN = &m_lNodes[_iNode];
	const CMat4Dfp32& Mat = _pInst->m_pBoneTransform[_iNode];

	CVec3Dfp32 Pos = pN->m_LocalCenter;
	Pos *= Mat;
	Pos -= pN->m_LocalCenter;

	CVec3Dfp32 v = pN->m_LocalCenter;
	v *= *_pTransform;
	v -= pN->m_LocalCenter;
	CVec3Dfp32::GetMatrixRow(_pInst->m_pBoneTransform[_iNode], 3) = -v;

	int nCh = pN->m_nChildren;
	for(int i = 0; i < nCh; i++)
		EvalPosition_r(m_liNodes[pN->m_iiNodeChildren + i], &_pInst->m_pBoneTransform[_iNode], _pInst);
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:	Evaluates skeleton hierachy	
						
	Parameters:		
		_iNode:			Node to start at
		_pTransform:	Parent matrix for _iNode
		_pInst:			Skeleton instance to work on

	Comments:	
		Transforms BoneLocalPos -> BoneTransform
\*____________________________________________________________________*/ 
void CXR_Skeleton::EvalNode(uint16 _iNode, const CMat4Dfp32* _pTransform, CXR_SkeletonInstance* _pInst) const
{
	MAUTOSTRIP(CXR_Skeleton_EvalNode, MAUTOSTRIP_VOID);
	MSCOPESHORT( CXR_Skeleton::EvalNode );
	EvalNode_i( _iNode, _pTransform, _pInst );
}


CMat4Dfp32 CXR_Skeleton::EvalNode(uint16 _iNode, CXR_Anim_Keyframe *_pFrame)
{
	MAUTOSTRIP(CXR_Skeleton_EvalNode2, MAUTOSTRIP_VOID);

	CXR_SkeletonNode *pNode = &m_lNodes[_iNode];
	CMat4Dfp32 Res;
	Res.Unit();
	while(pNode)
	{
		CXR_SkeletonNode *pParent = NULL;
		if(pNode->m_iNodeParent >= 0)
		{
			pParent = &m_lNodes[pNode->m_iNodeParent];
			if(pParent == pNode)
				pParent = NULL;
		}
		CMat4Dfp32 Mat, Res2;
		if(pNode->m_iRotationSlot != -1 && pNode->m_iRotationSlot < _pFrame->m_lRotKeys.Len())
		{
#ifdef USE_QUATERNION16
			CQuatfp32 Q;
			_pFrame->m_lRotKeys[pNode->m_iRotationSlot].m_Rot.GetQuatfp32(Q);
			Q.CreateMatrix(Mat);
#else
			_pFrame->m_lRotKeys[pNode->m_iRotationSlot].m_Rot.CreateMatrix(Mat);
#endif
		}
		else
			Mat.Unit();
		if(pNode->m_iMovementSlot != -1 && pNode->m_iMovementSlot < _pFrame->m_lMoveKeys.Len())
			_pFrame->m_lMoveKeys[pNode->m_iMovementSlot].m_Move.SetRow(Mat, 3);
		else
			CVec3Dfp32(0).SetRow(Mat, 3);

		if(pParent)
			Mat.GetRow(3) += pNode->m_LocalCenter - pParent->m_LocalCenter;
		Res.Multiply(Mat, Res2);
		Res = Res2;
		pNode = pParent;
	}
	return Res;
}

/*
void CXR_Skeleton::EvalNode_r(int16 _iNode, const CMat4Dfp32* _pTransform, CXR_SkeletonInstance* _pInst) const
{
	MAUTOSTRIP(CXR_Skeleton_EvalNode_r, MAUTOSTRIP_VOID);
	if (!m_lNodes.ValidPos(_iNode)) return;
	const CXR_SkeletonNode* pN = &m_lNodes[_iNode];
	CMat4Dfp32 Mat = _pInst->m_lBoneLocalPos[_iNode];

	CVec3Dfp32 Pos = pN->m_LocalCenter;
	Pos *= Mat;
	Pos -= pN->m_LocalCenter;

	// Apply bone scale (if any)
	fp32 BoneScale = _pInst->m_lBoneScale[_iNode];
	if ((BoneScale != 0.0f) && (pN->m_iNodeParent != -1))
	{
		CVec3Dfp32 ScaleDifference;
		CVec3Dfp32 ParentPos = m_lNodes[pN->m_iNodeParent].m_LocalCenter;
		if (_iNode == 0)
			ParentPos -= CVec3Dfp32::GetMatrixRow(Mat, 3);

		CVec3Dfp32::GetMatrixRow(Mat, 3) = -Pos;
		ScaleDifference = (pN->m_LocalCenter * Mat - ParentPos) * BoneScale;

		CVec3Dfp32::GetMatrixRow(Mat, 3) -= ScaleDifference;
	}
	else
	{
		CVec3Dfp32::GetMatrixRow(Mat, 3) = -Pos;
	}

	Mat.Multiply(*_pTransform, _pInst->m_pBoneTransform[_iNode]);

	int nCh = pN->m_nChildren;
	for(int i = 0; i < nCh; i++)
		EvalNode_r(m_liNodes[pN->m_iiNodeChildren + i], &_pInst->m_pBoneTransform[_iNode], _pInst);
}
*/

void CXR_Skeleton::EvalNode_i(uint16 _iNode, const CMat4Dfp32* _pTransform, CXR_SkeletonInstance* _pInst) const
{
	MAUTOSTRIP(CXR_Skeleton_EvalNode_i, MAUTOSTRIP_VOID);

	const uint16 NodeCount = m_lNodes.Len();
	if (_iNode >= NodeCount)
		return;
	
	const uint16* piNodes = m_liNodes.GetBasePtr();
	const CXR_SkeletonNode* pNodes = m_lNodes.GetBasePtr();
	const fp32* pBoneScale = _pInst->m_pBoneScale;
	const CMat4Dfp32* pBoneLocalPos = _pInst->m_pBoneLocalPos;

	struct JobItem
	{
		uint16 m_iNode;
		const CMat4Dfp32* m_pWorkingTransform;
	};

	JobItem aWorkingSet[256];
	int WorkingStack = 0;
	aWorkingSet[WorkingStack].m_iNode = _iNode;
	aWorkingSet[WorkingStack].m_pWorkingTransform = _pTransform;
	WorkingStack++;
	
StartOf_EvalNode_i:
	{
		--WorkingStack;
		const uint16 WorkingNode = aWorkingSet[WorkingStack].m_iNode;
		
		const CMat4Dfp32* pWorkingTransform = aWorkingSet[WorkingStack].m_pWorkingTransform;
		const CXR_SkeletonNode* pN = &pNodes[WorkingNode];
		CMat4Dfp32 Mat = pBoneLocalPos[WorkingNode];

		CVec3Dfp32 Pos = pN->m_LocalCenter;
		Pos *= Mat;
		Pos -= pN->m_LocalCenter;

		// Apply bone scale (if any)
		fp32 BoneScale = pBoneScale[WorkingNode];
		if ((BoneScale != 0.0f) && (pN->m_iNodeParent != -1))
		{
			CVec3Dfp32 ScaleDifference;
			CVec3Dfp32 ParentPos = pNodes[pN->m_iNodeParent].m_LocalCenter;
			if (WorkingNode == 0)
				ParentPos -= CVec3Dfp32::GetMatrixRow(Mat, 3);

			CVec3Dfp32::GetMatrixRow(Mat, 3) = -Pos;
			ScaleDifference = (pN->m_LocalCenter * Mat - ParentPos) * BoneScale;

			CVec3Dfp32::GetMatrixRow(Mat, 3) -= ScaleDifference;
		}
		else
		{
			CVec3Dfp32::GetMatrixRow(Mat, 3) = -Pos;
		}

		Mat.Multiply(*pWorkingTransform, _pInst->m_pBoneTransform[WorkingNode]);
//		MatrixMul( Mat, *pWorkingTransform, _pInst->m_pBoneTransform[WorkingNode] );

		uint nCh = pN->m_nChildren;

		const CMat4Dfp32* pBoneTransformMat = &_pInst->m_pBoneTransform[WorkingNode];
		for(uint i = 0; i < nCh; i++)
		{
			uint16 iNode = piNodes[pN->m_iiNodeChildren + i];
			if( iNode < NodeCount )
			{
				aWorkingSet[WorkingStack].m_iNode            	= iNode;
				aWorkingSet[WorkingStack].m_pWorkingTransform	= pBoneTransformMat;
				WorkingStack++;
			}
		}
	}

	if( WorkingStack > 0 )
		goto StartOf_EvalNode_i;
}

void CXR_Skeleton::EvalNode_IK_Special(uint16 _iNode, const CMat4Dfp32* _pTransform, CXR_SkeletonInstance* _pInst, int _iEvalNumberOfChildren) const
{
	MAUTOSTRIP(CXR_EvalNode_IK_Special, MAUTOSTRIP_VOID);

	const uint16 NodeCount = m_lNodes.Len();
	if (_iNode >= NodeCount)
		return;

	const uint16* piNodes = m_liNodes.GetBasePtr();
	const CXR_SkeletonNode* pNodes = m_lNodes.GetBasePtr();
	const fp32* pBoneScale = _pInst->m_pBoneScale;
	const CMat4Dfp32* pBoneLocalPos = _pInst->m_pBoneLocalPos;

	struct JobItem
	{
		uint16 m_iNode;
		const CMat4Dfp32* m_pWorkingTransform;
	};

	JobItem aWorkingSet[256];
	int WorkingStack = 0;
	aWorkingSet[WorkingStack].m_iNode = _iNode;
	aWorkingSet[WorkingStack].m_pWorkingTransform = _pTransform;
	WorkingStack++;

StartOf_EvalNode_i:
	{
		--WorkingStack;
		const uint16 WorkingNode = aWorkingSet[WorkingStack].m_iNode;

		const CMat4Dfp32* pWorkingTransform = aWorkingSet[WorkingStack].m_pWorkingTransform;
		const CXR_SkeletonNode* pN = &pNodes[WorkingNode];
		CMat4Dfp32 Mat = pBoneLocalPos[WorkingNode];

		CVec3Dfp32 Pos = pN->m_LocalCenter;
		Pos *= Mat;
		Pos -= pN->m_LocalCenter;

		// Apply bone scale (if any)
		fp32 BoneScale = pBoneScale[WorkingNode];
		if ((BoneScale != 0.0f) && (pN->m_iNodeParent != -1))
		{
			CVec3Dfp32 ScaleDifference;
			CVec3Dfp32 ParentPos = pNodes[pN->m_iNodeParent].m_LocalCenter;
			if (WorkingNode == 0)
				ParentPos -= CVec3Dfp32::GetMatrixRow(Mat, 3);

			CVec3Dfp32::GetMatrixRow(Mat, 3) = -Pos;
			ScaleDifference = (pN->m_LocalCenter * Mat - ParentPos) * BoneScale;

			CVec3Dfp32::GetMatrixRow(Mat, 3) -= ScaleDifference;
		}
		else
		{
			Mat.k[3][0] = -Pos.k[0];
			Mat.k[3][1] = -Pos.k[1];
			Mat.k[3][2] = -Pos.k[2];
		}

		Mat.Multiply(*pWorkingTransform, _pInst->m_pBoneTransform[WorkingNode]);
		//		MatrixMul( Mat, *pWorkingTransform, _pInst->m_pBoneTransform[WorkingNode] );

		uint nCh = pN->m_nChildren;

		const CMat4Dfp32* pBoneTransformMat = &_pInst->m_pBoneTransform[WorkingNode];
		for(uint i = 0; i < nCh; i++)
		{
			uint16 iNode = piNodes[pN->m_iiNodeChildren + i];
			if( iNode < NodeCount )
			{
				aWorkingSet[WorkingStack].m_iNode            	= iNode;
				aWorkingSet[WorkingStack].m_pWorkingTransform	= pBoneTransformMat;
				WorkingStack++;
			}
		}
	}

	if( WorkingStack > 0 && _iEvalNumberOfChildren != 0)
	{
		_iEvalNumberOfChildren--;
		goto StartOf_EvalNode_i;
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:	Evaluates skeleton hierachy	
						
	Parameters:		
		_iNode:			Node to start at
		_Transform:		Initial base transform
		_pInst:			Destination skeleton instance
		_pRot:			Rotation tracks
		_pMove:			Movement tracks
		_nRot:			Number of rotation tracks
		_nMove:			Number of movement tracks

	Comments:	
		Transforms Rot & Move -> BoneLocalPos & BoneTransform
\*____________________________________________________________________*/ 
void CXR_Skeleton::InitEvalNode_i(uint16 _iNode, const CMat4Dfp32& _Transform, CXR_SkeletonInstance* _pInst, const CQuatfp32* _pRot, const CVec4Dfp32* _pMove, uint16 _nRot, uint16 _nMove) const
{
	MAUTOSTRIP(CXR_Skeleton_EvalNode_i, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Skeleton::InitEvalNode_i);
//	PIXBeginNamedEvent(D3DCOLOR_XRGB(255,0,0),"InitEvalNode_i");
	const uint16 NodeCount = m_lNodes.Len();
	if (_iNode >= NodeCount)
		return;
	
	const uint16* piNodes = m_liNodes.GetBasePtr();
	const CXR_SkeletonNode* pNodes = m_lNodes.GetBasePtr();
	const fp32* pBoneScale = _pInst->m_pBoneScale;
//	const CMat4Dfp32* pBoneLocalPos = _pInst->m_pBoneLocalPos;

	struct JobItem
	{
		uint16 m_iNode;
		const CMat4Dfp32* m_pWorkingTransform;
	};


	JobItem aWorkingSet[256];
	int WorkingStack = 0;
	aWorkingSet[WorkingStack].m_iNode = _iNode;
	aWorkingSet[WorkingStack].m_pWorkingTransform = &_Transform;
	WorkingStack++;
	
	while (WorkingStack)
	{
		--WorkingStack;
		const uint16 WorkingNode = aWorkingSet[WorkingStack].m_iNode;
		const vec128 WorkingNodev = M_VLdScalar_i32(WorkingNode);
		const CMat4Dfp32* pWorkingTransform = aWorkingSet[WorkingStack].m_pWorkingTransform;
		const CXR_SkeletonNode* pN = &pNodes[WorkingNode];
		CMat4Dfp32& MatLocal = _pInst->m_pBoneLocalPos[WorkingNode];

		{
			uint16 iRot = pN->m_iRotationSlot;
			if (iRot >= _nRot)
			{
				MatLocal.Unit();
			}
			else
			{
				/*if (INVALID_QUAT(_pRot[iRot]))
				{ M_ASSERT(0,"!"); int a = 1; };*/
				_pRot[iRot].CreateMatrix(MatLocal);
			}

			uint16 iMove = pN->m_iMovementSlot;
			if (iMove < _nMove)
			{
				/*if (INVALID_V3(_pMove[iMove]))
				{ M_ASSERT(0,"!"); int b = 1; };*/
				MatLocal.r[3]=M_VSetW1(M_VNeg(_pMove[iMove]));
			}
		}

		CMat4Dfp32 Mat = MatLocal;

		vec128 Posv = pN->m_LocalCenterv;
		Posv = M_VMulMat4x3(Posv,Mat);
		Posv = M_VSub(Posv,pN->m_LocalCenterv);

		// Apply bone scale (if any)
		fp32 BoneScale = pBoneScale[WorkingNode];
		if ((BoneScale != 0.0f) && (pN->m_iNodeParent != -1))
		{
			const vec128 BoneScalev=M_VLdScalar(BoneScale);
			const vec128 Tmpv = M_VSub(pNodes[pN->m_iNodeParent].m_LocalCenterv,Mat.r[3]);
			const vec128 ParentPosv=M_VSel(WorkingNodev,Tmpv,pNodes[pN->m_iNodeParent].m_LocalCenterv);
			Mat.r[3]=M_VNeg(Posv);
			const vec128 ScaleDifferencev = M_VMul(M_VSub(M_VMulMat4x3(pN->m_LocalCenterv,Mat), ParentPosv), BoneScalev);
			Mat.r[3]= M_VSub(Mat.r[3], ScaleDifferencev);
		}
		else
		{
			Mat.r[3]=M_VNeg(Posv);
		}

		Mat.r[0]=M_VSetW0(Mat.r[0]);
		Mat.r[1]=M_VSetW0(Mat.r[1]);
		Mat.r[2]=M_VSetW0(Mat.r[2]);
		Mat.r[3]=M_VSetW1(Mat.r[3]);


		Mat.Multiply(*pWorkingTransform, _pInst->m_pBoneTransform[WorkingNode]);
//		MatrixMul( Mat, *pWorkingTransform, _pInst->m_pBoneTransform[WorkingNode] );

		uint nCh = pN->m_nChildren;

		const CMat4Dfp32* pBoneTransformMat = &_pInst->m_pBoneTransform[WorkingNode];
		for(uint i = 0; i < nCh; i++)
		{
			uint16 iNode = piNodes[pN->m_iiNodeChildren + i];
			if( iNode < NodeCount )
			{
				aWorkingSet[WorkingStack].m_iNode            	= iNode;
				aWorkingSet[WorkingStack].m_pWorkingTransform	= pBoneTransformMat;
				WorkingStack++;
			}
		}
	}

//	PIXEndNamedEvent();
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:	Evaluates skeleton hierachy	
						
	Parameters:		
		_iNode:			Node to start at
		_Transform:		Initial base transform
		_SkelInst:		Destination skeleton instance

	Comments:	
		Transforms Rot & Move -> BoneLocalPos & BoneTransform
\*____________________________________________________________________*/ 
void CXR_Skeleton::InitEvalNode(uint16 _iNode, const CMat4Dfp32& _Transform, CXR_SkeletonInstance* _pInst) const
{
	const CQuatfp32* pRot = _pInst->m_pTracksRot;
	const CVec4Dfp32* pMove = _pInst->m_pTracksMove;
	uint16 nRot = _pInst->m_nTracksRot;
	uint16 nMove = _pInst->m_nTracksMove;

	InitEvalNode_i(_iNode, _Transform, _pInst, pRot, pMove, nRot, nMove);
}


void CXR_Skeleton::EvalNodeQuat(uint16 _iNode, const CQuatfp32* _pQSrc, CQuatfp32& _Dst) const
{
	MAUTOSTRIP(CXR_Skeleton_EvalNodeQuat, MAUTOSTRIP_VOID);
	// _Dst must be initialized prior to call.

	TAP_RCD<const CXR_SkeletonNode> pNodes(m_lNodes);

	while(_iNode)
	{
		const CXR_SkeletonNode* pN = &pNodes[_iNode];
		_iNode = pN->m_iNodeParent;
		_Dst.Multiply( _pQSrc[pN->m_iRotationSlot] );
	}
}


/*void CXR_Skeleton::BlendTracks(int16 _iNode, fp32 _Blend, const CQuatfp32* _pQSrc, CQuatfp32* _pQDst, const CVec3Dfp32* _pMSrc, CVec3Dfp32* _pMDst, uint8* _pQDone, uint8* _pMDone, uint16 _Flags) const
{
	MAUTOSTRIP(CXR_Skeleton_BlendTracks, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Skeleton::BlendTracks);
	BlendTracks_i( _iNode, _Blend, _pQSrc, _pQDst, _pMSrc, _pMDst, _pQDone, _pMDone, _Flags );
}
*/

/*
void CXR_Skeleton::BlendTracks_r(int16 _iNode, fp32 _Blend, const CQuatfp32* _pQSrc, CQuatfp32* _pQDst, const CVec3Dfp32* _pMSrc, CVec3Dfp32* _pMDst, uint8* _pQDone, uint8* _pMDone, uint16 _Flags) const
{
	MAUTOSTRIP(CXR_Skeleton_BlendTracks_r, MAUTOSTRIP_VOID);
	if (!m_lNodes.ValidPos(_iNode)) return;
	const CXR_SkeletonNode* pN = &m_lNodes[_iNode];
	int iRot = pN->m_iRotationSlot;
//ConOut(CStrF("(BlendTracks_r) %d, %f, iRot %d, Par %d, nCh %d", _iNode, _Blend, iRot, pN->m_nChildren, pN->m_iNodeParent));
	if (!_pQDone[iRot])
	{
		_pQDone[iRot] = 1;
		if (iRot >= 0)
		{
			if (_Flags & CXR_ANIMLAYER_ATTACHPLANE_Z)
			{
				CQuatfp32 QuatSrc, QuatDstInv, dSrcDst;
				QuatSrc.Unit();
				QuatDstInv.Unit();
				EvalNodeQuat(pN->m_iNodeParent, _pQSrc, QuatSrc);
				EvalNodeQuat(pN->m_iNodeParent, _pQDst, QuatDstInv);
				QuatDstInv.Inverse();
				QuatDstInv.Multiply(QuatSrc, dSrcDst);

				CMat4Dfp32 dSrcDstMat, dSrcDstMatInv;
				dSrcDst.CreateMatrix(dSrcDstMat);

				// Keep the z-rotation
//				CVec3Dfp32(0,0,1).SetRow(dSrcDstMat, 2);
//				dSrcDstMat.RecreateMatrix(2, 0);

				// Keep the z-rotation, but scale it down a bit, so it don't affect too much
				fp32 dAngleZ = CVec3Dfp32::AngleFromVector(dSrcDstMat.k[0][0], dSrcDstMat.k[0][1]);
				if(dAngleZ < 0)
					dAngleZ += 1;
				dAngleZ = M_Sin((dAngleZ - 0.5f)*_PI)*0.5f + 0.5f;
				dSrcDstMat.SetZRotation(dAngleZ);
				dSrcDstMat.UnitNot3x3();

				dSrcDstMat.InverseOrthogonal(dSrcDstMatInv);

				EvalNodeQuat(_iNode, _pQSrc, QuatSrc);
				CQuatfp32 dQuatZ, Quat, QuatDstRel;
				dQuatZ.Create(dSrcDstMatInv);
				QuatSrc.Multiply(dQuatZ, Quat);

				CQuatfp32 QuatDstParent;
				QuatDstParent.Unit();
				EvalNodeQuat(pN->m_iNodeParent, _pQDst, QuatDstParent);
				QuatDstParent.Inverse();

				Quat.Multiply(QuatDstParent, QuatDstRel);

				_pQDst[iRot].Interpolate(QuatDstRel, _pQDst[iRot], _Blend);
			}
			else if (_Flags & CXR_ANIMLAYER_ATTACHNOREL)
			{
				CQuatfp32 QuatSrc, QuatDstRel;
				QuatSrc.Unit();
				EvalNodeQuat(_iNode, _pQSrc, QuatSrc);

				CQuatfp32 QuatDstParent;
				QuatDstParent.Unit();
				EvalNodeQuat(pN->m_iNodeParent, _pQDst, QuatDstParent);
				QuatDstParent.Inverse();

				QuatSrc.Multiply(QuatDstParent, QuatDstRel);

				_pQDst[iRot].Interpolate(QuatDstRel, _pQDst[iRot], _Blend);
			}
			else
				_pQDst[iRot].Interpolate(_pQSrc[iRot], _pQDst[iRot], _Blend);
		}
	}

	int iMove = pN->m_iMovementSlot;
	// TBC..

	int nCh = pN->m_nChildren;
	for(int i = 0; i < nCh; i++)
		BlendTracks_r(m_liNodes[pN->m_iiNodeChildren + i], _Blend, _pQSrc, _pQDst, _pMSrc, _pMDst, _pQDone, _pMDone, _Flags & ((_Flags >> 1) | (_Flags & 0xaaaaaaaa)));
}
*/

void CXR_Skeleton::BlendTracks(uint16 _iNode, fp32 _Blend, const CQuatfp32* _pQSrc, CQuatfp32* _pQDst, const CVec4Dfp32* _pMSrc, CVec4Dfp32* _pMDst, uint8* _pQDone, uint8* _pMDone, uint16 _Flags, const CXR_Anim_TrackMask& _NodeMask) const
{
	MAUTOSTRIP(CXR_Skeleton_BlendTracks, MAUTOSTRIP_VOID);

	const uint16 nNodeCount = m_lNodes.Len();
	if (_iNode >= nNodeCount)
		return;

	if (_NodeMask.IsEnabledRot(_iNode) == 0 && !(_Flags & CXR_ANIMLAYER_IGNOREBASENODE))
		return;

	bool bNoBlend = _Blend > 0.999f;

	TAP_RCD<const uint16> piNodes(m_liNodes);
	TAP_RCD<const CXR_SkeletonNode> pNodes(m_lNodes);
	struct JobItem	// Cache friendly JobItem struct
	{
		uint16 m_iNode;
		uint16 m_Flags;
	};
#ifdef	PLATFORM_PS2
	JobItem* aWorkingSet = (JobItem*)ScratchPad_Alloc( sizeof( JobItem ) * ANIM_MAXROTTRACKS );
#else
	JobItem aWorkingSet[ANIM_MAXROTTRACKS];
#endif
	int WorkingStack = 0;
	// If we are ignoring the basenode, just push it's children instead
	if (_Flags & CXR_ANIMLAYER_IGNOREBASENODE)
	{
		const CXR_SkeletonNode* pN = &pNodes[_iNode];
		uint nCh = pN->m_nChildren;
		for(uint i = 0; i < nCh; i++)
		{
			uint16 iNode = piNodes[pN->m_iiNodeChildren + i];
			if( iNode < nNodeCount && _NodeMask.IsEnabledRot(iNode))
			{
				aWorkingSet[WorkingStack].m_iNode	= iNode;
				aWorkingSet[WorkingStack].m_Flags	= _Flags & ((_Flags >> 1) | (_Flags & 0xaaaaaaaa));
				WorkingStack++;
			}
		}
	}
	else
	{
		aWorkingSet[WorkingStack].m_iNode	= _iNode;
		aWorkingSet[WorkingStack].m_Flags	= _Flags;
		WorkingStack++;
	}

	while(WorkingStack > 0)
	{
		M_ASSERT(WorkingStack < ANIM_MAXROTTRACKS, "!");
		--WorkingStack;
		const uint16 WorkingNode = aWorkingSet[WorkingStack].m_iNode;

		const uint16 WorkingFlags = aWorkingSet[WorkingStack].m_Flags;
		const CXR_SkeletonNode* pN = &pNodes[WorkingNode];

		int16 iRot = pN->m_iRotationSlot;
	//ConOut(CStrF("(BlendTracks_i) %d, %f, iRot %d, Par %d, nCh %d", _iNode, _Blend, iRot, pN->m_nChildren, pN->m_iNodeParent));
		if (iRot >= 0 && !_pQDone[iRot])
		{
			_pQDone[iRot] = 1;
			if (iRot >= 0)
			{
				if (WorkingFlags & CXR_ANIMLAYER_ATTACHPLANE_Z)
				{
					CQuatfp32 QuatSrc, QuatDstInv, dSrcDst;
					QuatSrc.Unit();
					QuatDstInv.Unit();
					EvalNodeQuat(pN->m_iNodeParent, _pQSrc, QuatSrc);
					EvalNodeQuat(pN->m_iNodeParent, _pQDst, QuatDstInv);
					QuatDstInv.Inverse();
					QuatDstInv.Multiply(QuatSrc, dSrcDst);

					CMat4Dfp32 dSrcDstMat, dSrcDstMatInv;
					dSrcDst.CreateMatrix(dSrcDstMat);

					// Keep the z-rotation
	/*				CVec3Dfp32(0,0,1).SetRow(dSrcDstMat, 2);
					dSrcDstMat.RecreateMatrix(2, 0);*/

					// Keep the z-rotation, but scale it down a bit, so it don't affect too much
					fp32 dAngleZ = CVec3Dfp32::AngleFromVector(dSrcDstMat.k[0][0], dSrcDstMat.k[0][1]);
					if(dAngleZ < 0)
						dAngleZ += 1;
					dAngleZ = M_Sin((dAngleZ - 0.5f)*_PI)*0.5f + 0.5f;
					dSrcDstMat.SetZRotation(dAngleZ);
					dSrcDstMat.UnitNot3x3();

					dSrcDstMat.InverseOrthogonal(dSrcDstMatInv);

					EvalNodeQuat(_iNode, _pQSrc, QuatSrc);
					CQuatfp32 dQuatZ, Quat, QuatDstRel;
					dQuatZ.Create(dSrcDstMatInv);
					QuatSrc.Multiply(dQuatZ, Quat);

					CQuatfp32 QuatDstParent;
					QuatDstParent.Unit();
					EvalNodeQuat(pN->m_iNodeParent, _pQDst, QuatDstParent);
					QuatDstParent.Inverse();

					Quat.Multiply(QuatDstParent, QuatDstRel);

					_pQDst[iRot].Lerp(QuatDstRel, _Blend, _pQDst[iRot]);
				}
				else if (WorkingFlags & CXR_ANIMLAYER_ATTACHNOREL)
				{
					CQuatfp32 QuatSrc, QuatDstRel;
					QuatSrc.Unit();
					EvalNodeQuat(_iNode, _pQSrc, QuatSrc);

					CQuatfp32 QuatDstParent;
					QuatDstParent.Unit();
					EvalNodeQuat(pN->m_iNodeParent, _pQDst, QuatDstParent);
					M_ASSERT(!INVALID_QUAT(QuatDstParent), "!");
					QuatDstParent.Inverse();

					QuatSrc.Multiply(QuatDstParent, QuatDstRel);
					M_ASSERT(!INVALID_QUAT(QuatDstRel), "!");

					if (bNoBlend)
						_pQDst[iRot] = QuatDstRel;
					else
					{
						M_ASSERT(!INVALID_QUAT(_pQDst[iRot]), "!");
						_pQDst[iRot].Lerp(QuatDstRel, _Blend, _pQDst[iRot]);
					}
				}
				else if (WorkingFlags & CXR_ANIMLAYER_ADDITIVEBLEND)
				{
					CQuatfp32 QRes;
					_pQSrc[iRot].Multiply(_pQDst[iRot], QRes);
					_pQDst[iRot].Lerp(QRes, _Blend, _pQDst[iRot]);
				}
				else
				{
					M_ASSERT(!INVALID_QUAT(_pQSrc[iRot]), "!");
					if (bNoBlend)
						_pQDst[iRot] = _pQSrc[iRot];
					else
					{
						M_ASSERT(!INVALID_QUAT(_pQDst[iRot]), "!");
						_pQDst[iRot].Lerp(_pQSrc[iRot], _Blend, _pQDst[iRot]);
					}
				}
			}
		}

		int16 iMove = pN->m_iMovementSlot;
		if (iMove >= 0 && !_pMDone[iMove])
		{
			_pMDone[iMove] = 1;
			if (iMove >= 0)
			{
				M_ASSERT(!INVALID_V3(_pMSrc[iMove]), "!");

				if (bNoBlend)
				{
					if(WorkingFlags & CXR_ANIMLAYER_ADDITIVEBLEND)
					{
						M_ASSERT(!INVALID_V3(_pMDst[iMove]), "!");
						_pMDst[iMove] = _pMDst[iMove] + _pMSrc[iMove];
					}
					else
						_pMDst[iMove] = _pMSrc[iMove];
				}
				else
				{
					M_ASSERT(!INVALID_V3(_pMDst[iMove]), "!");

					if(WorkingFlags & CXR_ANIMLAYER_ADDITIVEBLEND)
						_pMDst[iMove] = _pMDst[iMove] + _pMSrc[iMove] * _Blend;
					else
						_pMDst[iMove] = _pMDst[iMove] + (_pMSrc[iMove] - _pMDst[iMove]) * _Blend;
				}
			}
			_pMDst[iMove]=M_VSetW1(_pMDst[iMove]);
		}

		{
			uint nCh = pN->m_nChildren;

			const uint16 JobFlags = WorkingFlags & ((WorkingFlags >> 1) | (WorkingFlags & 0xaaaaaaaa));;
			for(uint i = 0; i < nCh; i++)
			{
				uint16 iNode = piNodes[pN->m_iiNodeChildren + i];
				if( iNode < nNodeCount && _NodeMask.IsEnabledRot(iNode))
				{
					aWorkingSet[WorkingStack].m_iNode	= iNode;
					aWorkingSet[WorkingStack].m_Flags	= JobFlags;
					WorkingStack++;
				}
			}
		}
	}

#ifdef	PLATFORM_PS2
	ScratchPad_Free( sizeof( JobItem ) * 256 );
#endif
}

#define XR_ANIM_USELAYERTRACKMASKING

//#define INTERPOLATE_OPT_PS2	// UNTESTED CODE. I didn't find any cases to test it on. Perhaps in other projects
void CXR_Skeleton::EvalTracks(CXR_AnimLayer* _pLayer, uint _nLayers, CXR_SkeletonInstance* _pSkelInst, uint _Flags, const CXR_Anim_TrackMask* _pTrackMaskOr) const
{
	MAUTOSTRIP(CXR_Skeleton_EvalTracks, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Skeleton::EvalTracks);
	if (!_nLayers)
	{
		uint nRotations = m_nUsedRotations;
		uint nMovements = m_nUsedMovements;
		_pSkelInst->CreateTracks(nRotations, nMovements);
		CQuatfp32* pQ = _pSkelInst->m_pTracksRot;
		CVec4Dfp32* pM = _pSkelInst->m_pTracksMove;
		{ for(int i = 0; i < nRotations; i++) pQ[i].Unit(); }
		{ for(int i = 0; i < nMovements; i++) pM[i] = M_VConst(0,0,0,1.0f); }
	}
	else
	{
		uint8* pScratchPad = MRTC_ScratchPadManager::Get(
			sizeof(CQuatfp32)*CXR_SKELETON_MAXROTATIONS+ 
			sizeof(CVec3Dfp32)*CXR_SKELETON_MAXMOVEMENTS+ 
			CXR_SKELETON_MAXROTATIONS+CXR_SKELETON_MAXMOVEMENTS
			);

		CQuatfp32 *QRot = (CQuatfp32 *)pScratchPad; pScratchPad += sizeof(CQuatfp32)*CXR_SKELETON_MAXROTATIONS;
		CVec4Dfp32 *pMove = (CVec4Dfp32 *)pScratchPad; pScratchPad += sizeof(CVec4Dfp32)*CXR_SKELETON_MAXMOVEMENTS;
		uint8 *QDone = pScratchPad; pScratchPad += CXR_SKELETON_MAXROTATIONS;
		uint8 *MDone = pScratchPad; pScratchPad += CXR_SKELETON_MAXMOVEMENTS;

		uint nRotations = m_nUsedRotations;
		uint nMovements = m_nUsedMovements;
		_pSkelInst->CreateTracks(nRotations, nMovements);

		CQuatfp32* pQ = _pSkelInst->m_pTracksRot;
		CVec4Dfp32* pM = _pSkelInst->m_pTracksMove;

#ifdef XR_ANIM_USELAYERTRACKMASKING
		CXR_Anim_TrackMask lTrackMask[16];
		CXR_Anim_TrackMask lNodeMask[16];
		if (_nLayers > 16)
		{
			M_TRACE("WARNING: (CXR_Skeleton::EvalTracks) Layer count capped from %d to %d\n", _nLayers, 16);
			_nLayers = 16;
		}

		{
//			CXR_Skeleton* pThisNonConst = const_cast<CXR_Skeleton*>(this);

			CXR_Anim_TrackMask Coverage;
			CXR_Anim_TrackMask NodeCoverage;
			Coverage.Clear();
			NodeCoverage.Clear();

			const CXR_AnimLayer& TopLayer = _pLayer[_nLayers-1];
			if (TopLayer.m_iBlendBaseNode)
			{
				if (TopLayer.m_iBlendBaseNode >= m_lNodes.Len())
				{
					M_TRACE("WARNING: (CXR_Skeleton::EvalTracks) Top layer blend base node %d not in skeleton\n", TopLayer.m_iBlendBaseNode);
					return;
				}

				CXR_Anim_TrackMask* pTrack;
				CXR_Anim_TrackMask* pNode;
				CXR_Anim_TrackMask* pCoverage;
				GetTrackMask(TopLayer.m_iBlendBaseNode, pTrack, pNode, pCoverage);
				lTrackMask[_nLayers-1].Copy(*pTrack);
				lNodeMask[_nLayers-1].Copy(*pNode);
				if (TopLayer.m_Flags & CXR_ANIMLAYER_IGNOREBASENODE)
				{
					// Disable layer base node from trackmasks
					const CXR_SkeletonNode& Node = m_lNodes[TopLayer.m_iBlendBaseNode];
					int iRot = Node.m_iRotationSlot;
					int iMove = Node.m_iMovementSlot;
					if (iRot >= 0)
					{
						lTrackMask[_nLayers-1].m_TrackMaskRot.Disable(iRot);
						lNodeMask[_nLayers-1].m_TrackMaskRot.Disable(iRot);
					}
					if (iMove >= 0)
					{
						lTrackMask[_nLayers-1].m_TrackMaskMove.Disable(iMove);
						lNodeMask[_nLayers-1].m_TrackMaskMove.Disable(iMove);
					}
				}
//				const CXR_Anim_TrackMask* pTM = pThisNonConst->GetTrackMask(TopLayer.m_iBlendBaseNode);
//				lTrackMask[_nLayers-1] = pTM[0];
//				lNodeMask[_nLayers-1] = pTM[2];
				if (!(TopLayer.m_Flags & CXR_ANIMLAYER_ADDITIVEBLEND) && (TopLayer.m_Blend > 0.999f))
				{
//					Coverage = pTM[1];
//					NodeCoverage = pTM[2];
					Coverage.Copy(*pCoverage);
					NodeCoverage.Copy(lNodeMask[_nLayers-1]);
					if (TopLayer.m_Flags & CXR_ANIMLAYER_IGNOREBASENODE)
					{
						// Disable that node
						const CXR_SkeletonNode& Node = m_lNodes[TopLayer.m_iBlendBaseNode];
						int iRot = Node.m_iRotationSlot;
						if (iRot >= 0)
							Coverage.m_TrackMaskRot.Disable(iRot);
						int iMove = Node.m_iMovementSlot;
						if (iMove >= 0)
							Coverage.m_TrackMaskMove.Disable(iMove);
					}
				}
			}
			else
			{
				lTrackMask[_nLayers-1].Copy(m_TrackMask);
				lNodeMask[_nLayers-1].Copy(m_NodeMask);
				if (!(TopLayer.m_Flags & CXR_ANIMLAYER_ADDITIVEBLEND) && (TopLayer.m_Blend > 0.999f))
				{
					Coverage.Copy(m_TrackMask);
					NodeCoverage.Copy(m_NodeMask);
				}
			}

			if (_pTrackMaskOr)
				lTrackMask[_nLayers-1].Or(*_pTrackMaskOr);

			if ((lNodeMask[_nLayers-1].m_TrackMaskRot.m_lMask[0] & lTrackMask[_nLayers-1].m_TrackMaskRot.m_lMask[0]) != lNodeMask[_nLayers-1].m_TrackMaskRot.m_lMask[0])
			{ M_ASSERT(0,"!"); };

			Coverage.m_TrackMaskMove.m_lMask[0] &= ~(1+2);
			Coverage.m_TrackMaskRot.m_lMask[0] &= ~(1+2);

			for(int l = _nLayers-2; l >= 0; l--)
			{
				const CXR_AnimLayer& Layer = _pLayer[l];
				if (Layer.m_iBlendBaseNode)
				{
					if (Layer.m_iBlendBaseNode >= m_lNodes.Len())
					{
						M_TRACE("WARNING: (CXR_Skeleton::EvalTracks) Layer %d blend base node %d not in skeleton\n", l, Layer.m_iBlendBaseNode);
						return;
					}

					CXR_Anim_TrackMask* pTrack;
					CXR_Anim_TrackMask* pNode;
					CXR_Anim_TrackMask* pCoverage;
					GetTrackMask(Layer.m_iBlendBaseNode, pTrack, pNode, pCoverage);
					if (Layer.m_Flags & CXR_ANIMLAYER_IGNOREBASENODE)
					{
						// Disable that node
						const CXR_SkeletonNode& Node = m_lNodes[Layer.m_iBlendBaseNode];
						int iRot = Node.m_iRotationSlot;
						int iMove = Node.m_iMovementSlot;
						
						lTrackMask[l].Copy(*pTrack);
						lNodeMask[l].Copy(*pNode);
						if (iRot >= 0)
						{
							lTrackMask[l].m_TrackMaskRot.Disable(iRot);
							lNodeMask[l].m_TrackMaskRot.Disable(iRot);
						}
						if (iMove >= 0)
						{
							lTrackMask[l].m_TrackMaskMove.Disable(iMove);
							lNodeMask[l].m_TrackMaskMove.Disable(iMove);
						}
						lTrackMask[l].AndNot(Coverage);
						lNodeMask[l].AndNot(NodeCoverage);
					}
					else
					{
						lTrackMask[l].Copy(*pTrack);
						lTrackMask[l].AndNot(Coverage);
						lNodeMask[l].Copy(*pNode);
						lNodeMask[l].AndNot(NodeCoverage);
					}
					
//					const CXR_Anim_TrackMask* pTM = pThisNonConst->GetTrackMask(Layer.m_iBlendBaseNode);
//					lTrackMask[l] = pTM[0];
//					lTrackMask[l].AndNot(Coverage);
//					lNodeMask[l] = pTM[2];
//					lNodeMask[l].AndNot(NodeCoverage);

					if (!(Layer.m_Flags & CXR_ANIMLAYER_ADDITIVEBLEND) && (Layer.m_Blend > 0.999f))
					{
//						Coverage.Or(pTM[1]);
//						NodeCoverage.Or(pTM[2]);
						if (Layer.m_Flags & CXR_ANIMLAYER_IGNOREBASENODE)
						{
							// Disable that node
							CXR_Anim_TrackMask Temp,Temp2;
							Temp.Copy(*pCoverage);
							Temp2.Copy(*pNode);
							const CXR_SkeletonNode& Node = m_lNodes[Layer.m_iBlendBaseNode];
							int iRot = Node.m_iRotationSlot;
							if (iRot >= 0)
							{
								Temp.m_TrackMaskRot.Disable(iRot);
								Temp2.m_TrackMaskRot.Disable(iRot);
							}
							int iMove = Node.m_iMovementSlot;
							if (iMove >= 0)
							{
								Temp.m_TrackMaskMove.Disable(iMove);
								Temp2.m_TrackMaskMove.Disable(iMove);
							}
							Coverage.Or(Temp);
							NodeCoverage.Or(Temp2);
						}
						else
						{
							Coverage.Or(*pCoverage);
							NodeCoverage.Or(*pNode);
						}
					}
				}
				else
				{
					lTrackMask[l].Copy(m_TrackMask);
					lTrackMask[l].AndNot(Coverage);
					lNodeMask[l].Copy(m_NodeMask);
					lNodeMask[l].AndNot(NodeCoverage);

					if (!(Layer.m_Flags & CXR_ANIMLAYER_ADDITIVEBLEND) && (Layer.m_Blend > 0.999f))
					{
						Coverage.Or(m_TrackMask);
						NodeCoverage.Or(m_NodeMask);
					}
				}

				if (_pTrackMaskOr)
					lTrackMask[l].Or(*_pTrackMaskOr);

				if ((lNodeMask[l].m_TrackMaskRot.m_lMask[0] & lTrackMask[l].m_TrackMaskRot.m_lMask[0]) != lNodeMask[l].m_TrackMaskRot.m_lMask[0])
				{ M_ASSERT(0,"!"); };
//				Coverage.m_TrackMaskMove.m_lMask[0] &= ~1;
//				Coverage.m_TrackMaskRot.m_lMask[0] &= ~(1+2);
			}
		}
#endif

		bool bFirstLayer = true;
		for(uint l = 0; l < _nLayers; l++)
		{
			const CXR_AnimLayer& Layer = _pLayer[l];
			fp32 t = Layer.m_Time;
			const CXR_Anim_SequenceData *pSeq = Layer.m_spSequence;

#ifdef XR_ANIM_USELAYERTRACKMASKING
			const CXR_Anim_TrackMask& NodeMask = lNodeMask[l];
#if ANIM_MAXROTTRACKS==320
			uint32 CollapsedMask = NodeMask.m_TrackMaskRot.m_lMask[0] | NodeMask.m_TrackMaskRot.m_lMask[1] | NodeMask.m_TrackMaskRot.m_lMask[2] | NodeMask.m_TrackMaskRot.m_lMask[3] |
				                   NodeMask.m_TrackMaskRot.m_lMask[4] | NodeMask.m_TrackMaskRot.m_lMask[5] | NodeMask.m_TrackMaskRot.m_lMask[6] | NodeMask.m_TrackMaskRot.m_lMask[7] | 
								   NodeMask.m_TrackMaskRot.m_lMask[8] | NodeMask.m_TrackMaskRot.m_lMask[9];
			if (!CollapsedMask)
			{
//				M_TRACEALWAYS("Rejecting layer %d / %d, Skel %.8x\n", l, _nLayers, this);
				continue;
			}
#else
	#error "Hardcoded"
#endif
#endif

			if (Layer.m_Blend > 0.999f && !Layer.m_iBlendBaseNode)
			{
				if(pSeq != NULL)
				{
					CXR_Anim_TrackMask TrackMask;
					#ifdef XR_ANIM_USELAYERTRACKMASKING
						TrackMask.Copy(lTrackMask[l]);
					#else
						TrackMask.Copy(m_TrackMask);
					#endif
					while(true)
					{
						if(pSeq->m_pNextAnimSequenceLayer)
						{
							CXR_Anim_TrackMask TrackMask2;
							TrackMask2.Copy(TrackMask);
							TrackMask2.And(pSeq->m_TrackMask);
							pSeq->Eval(t, pQ, nRotations, &pM->v, nMovements, TrackMask2);
							TrackMask.AndNot(pSeq->m_TrackMask);
							pSeq = pSeq->m_pNextAnimSequenceLayer;
						}
						else
						{
							pSeq->Eval(t, pQ, nRotations, &pM->v, nMovements, TrackMask);
							break;
						}
					}
				}
				else
				{
					// Reset
					for(uint i = 0; i < nRotations; i++) pQ[i].Unit();
					for(uint i = 0; i < nMovements; i++) pM[i] = M_VConst(0,0,0,1.0f);
				}

				if(_Flags == 0)
				{
					pQ[0].Unit();
					pM[0] = M_VConst(0,0,0,1.0f);
				}
			}
			else
			{
				if(pSeq != NULL)
				{
					CXR_Anim_TrackMask TrackMask;
					#ifdef XR_ANIM_USELAYERTRACKMASKING
						TrackMask.Copy(lTrackMask[l]);
					#else
						TrackMask.Copy(m_TrackMask);
					#endif
					while(true)
					{
						if(pSeq->m_pNextAnimSequenceLayer)
						{
							CXR_Anim_TrackMask TrackMask2;
							TrackMask2.Copy(TrackMask);
							TrackMask2.And(pSeq->m_TrackMask);
							pSeq->Eval(t, QRot, nRotations, &pMove->v, nMovements, TrackMask2);
							TrackMask.AndNot(pSeq->m_TrackMask);
							pSeq = pSeq->m_pNextAnimSequenceLayer;
						}
						else
						{
							pSeq->Eval(t, QRot, nRotations, &pMove->v, nMovements, TrackMask);
							break;
						}
					}
				}
				else
				{
					// Reset
					for(uint i = 0; i < nRotations; i++) QRot[i].Unit();
					for(uint i = 0; i < nMovements; i++) pMove[i] = M_VConst(0,0,0,1.0f);
				}

				if(_Flags == 0)
				{
					QRot[0].Unit();
					pMove[0] = M_VConst(0,0,0,1.0f);
				}

				if (bFirstLayer && Layer.m_Blend <= 0.999f)
				{
					// If we have a blendfactor < 1 for the first layer, zero the previous dummy layer.
					{ for(uint i = 0; i < nRotations; i++) pQ[i].Unit(); }
					{ for(uint i = 0; i < nMovements; i++) pM[i] = M_VConst(0,0,0,1.0f); }
				}

//				if (Layer.m_iBlendBaseNode)
				{
					FillChar(QDone, nRotations, 0);
					FillChar(MDone, nMovements, 0);

#ifdef XR_ANIM_USELAYERTRACKMASKING
					BlendTracks(Layer.m_iBlendBaseNode, Layer.m_Blend, QRot, pQ, pMove, pM, QDone, MDone, Layer.m_Flags, lNodeMask[l]);
#else
					BlendTracks(Layer.m_iBlendBaseNode, Layer.m_Blend, QRot, pQ, pMove, pM, QDone, MDone, Layer.m_Flags, m_NodeMask);
#endif
					if (Layer.m_Flags & CXR_ANIMLAYER_USEMOVE)
					{
						for(uint m = 0; m < nMovements; m++)
							pM[m].Lerp(pMove[m], Layer.m_Blend, pM[m]);
					}
				}
/*				else
				{
					for(int q = 0; q < nRotations; q++)
						pQ[q].Interpolate(QRot[q], pQ[q], Layer.m_Blend);

					for(int m = 0; m < nMovements; m++)
						pM[m].Lerp(Move[m], Layer.m_Blend, pM[m]);
				}*/
			}

			bFirstLayer = false;
		}

		// Scale final movetracks
		vec128 MoveScalev = M_VLdScalar(_pSkelInst->m_MoveScale);
		for (uint iMove = 0; iMove < nMovements; iMove++)
			pM[iMove] = M_VSetW1(M_VMul(pM[iMove], MoveScalev));

#ifdef	PLATFORM_PS2
		ScratchPad_Free( CXR_SKELETON_MAXMOVEMENTS );
		ScratchPad_Free( CXR_SKELETON_MAXROTATIONS );
		ScratchPad_Free( sizeof(CVec3Dfp32)*CXR_SKELETON_MAXMOVEMENTS );
		ScratchPad_Free( sizeof(CQuatfp32)*CXR_SKELETON_MAXROTATIONS );
#endif
	}
}


void CXR_Skeleton::EvalAnim(CXR_AnimLayer* _pLayers, uint _nLayers, CXR_SkeletonInstance* _pSkelInst, CMat4Dfp32& _WMat, uint _Flags, const CXR_Anim_TrackMask* _pTrackMaskOr) const
{
	MAUTOSTRIP(CXR_Skeleton_EvalAnim, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Skeleton::EvalAnim);
	
	{
		int nNodes = m_lNodes.Len();
		if (_pSkelInst->m_nBoneLocalPos < nNodes ||
			_pSkelInst->m_nBoneTransform < nNodes)
			_pSkelInst->Create(nNodes);
	}

	bool bFullLayerFound = false;
	for(int i = _nLayers-1; i >= 0; i--)
	{
		if (!_pLayers[i].m_iBlendBaseNode && (_pLayers[i].m_Blend > 0.999f))
			bFullLayerFound = true;
		if (_pLayers[i].m_iBlendBaseNode && bFullLayerFound)
		{
			M_TRACEALWAYS("Partial Layer found before non-opaque full layer. Layer %d / %d, RotNmoves %d,%d\n", i, _nLayers, m_nUsedRotations, m_nUsedMovements);
		}
	}

	if (!bFullLayerFound)
	{
		M_TRACEALWAYS("Aborting EvalAnim: Full layer not found, nLayers %d, RotNmoves %d,%d\n", _nLayers, m_nUsedRotations, m_nUsedMovements);
		MemSetD(_pSkelInst->m_pBoneLocalPos, 0x7Fc00000, _pSkelInst->m_nBoneLocalPos * sizeof(CMat4Dfp32) >> 2);
		MemSetD(_pSkelInst->m_pBoneTransform, 0x7Fc00000, _pSkelInst->m_nBoneTransform * sizeof(CMat4Dfp32) >> 2);
		return;
	}

	if (INVALID_V3(CVec3Dfp32::GetRow(_WMat, 0)) || 
		INVALID_V3(CVec3Dfp32::GetRow(_WMat, 1)) || 
		INVALID_V3(CVec3Dfp32::GetRow(_WMat, 2)) ||
		INVALID_V3(CVec3Dfp32::GetRow(_WMat, 3)))
	{
		M_TRACEALWAYS("Aborting EvalAnim: QNaN _WMat sent, nLayers %d, RotNmoves %d,%d\n", _nLayers, m_nUsedRotations, m_nUsedMovements);
		return;
	}

	// Fill with QNaN
#ifndef M_RTM
	MemSetD(_pSkelInst->m_pBoneLocalPos, 0x7Fc00000, _pSkelInst->m_nBoneLocalPos * sizeof(CMat4Dfp32) >> 2);
	MemSetD(_pSkelInst->m_pBoneTransform, 0x7Fc00000, _pSkelInst->m_nBoneTransform * sizeof(CMat4Dfp32) >> 2);
#endif

	EvalTracks(_pLayers, _nLayers, _pSkelInst, _Flags, _pTrackMaskOr);

#ifndef M_RTM
	_pSkelInst->m_pDebugSkel = const_cast<CXR_Skeleton*>(this);
#endif

	// Transform
	CQuatfp32* pRot = _pSkelInst->m_pTracksRot;
	CVec4Dfp32* pMove = _pSkelInst->m_pTracksMove;

	if(_Flags & CXR_SKELETON_APPLYOBJECTPOSITION)
	{
		// Pos1 is not compensated with Pos0 (Used in Ogier)
		// Move Pos0 -> WMat (WMat is a return value)
		_WMat.Create(pRot[0], pMove[0]);
	}
	else if(_Flags & 2)
	{
		// Move this code to ApplyObjectPosition when safe...
		CMat4Dfp32 Mat, Mat2;
		Mat.Create(pRot[0],pMove[0]);
		Mat.Multiply(_WMat, Mat2);
		pRot[0].Create(Mat2);
		pMove[0] = CVec3Dfp32::GetRow(Mat2, 3);
		_WMat = Mat2;
	}
	else
	{
		// Move WMat -> Pos0
		pRot[0].Create(_WMat);
		pMove[0] = CVec3Dfp32::GetRow(_WMat, 3);
		
		CMat4Dfp32 M0, M1;
		M0.Create(pRot[1],pMove[1]);
		M0.Multiply(_WMat, M1);
		_WMat = M1;
	}

//	InitEvalNode_i(0, _pSkelInst, pRot, pMove, m_nUsedRotations, m_nUsedMovements);

/*	int nRotations = m_nUsedRotations;
	int nMovements = m_nUsedMovements;
	CMat4Dfp32 Rot[CXR_SKELETON_MAXROTATIONS];	// 6144 bytes
	{
//		MSCOPESHORT(CreateMatrix);
		for(int i = 0; i < nRotations; i++)
			_pSkelInst->m_lTracksRot[i].CreateMatrix(Rot[i]);
	}

//InitNode(0, _pSkelInst, Rot, pMove, nRotations, nMovements);
	
	{
		CMat4Dfp32 Unit; Unit.Unit();
//		EvalNode(0, &Unit, _pSkelInst);
	}*/
	CMat4Dfp32 BaseTransform;
	BaseTransform.Unit();
	InitEvalNode_i(0, BaseTransform, _pSkelInst, pRot, pMove, m_nUsedRotations, m_nUsedMovements);
}


const CXR_SkeletonAttachPoint* CXR_Skeleton::GetAttachPoint(int _iAttach)
{
	MAUTOSTRIP(CXR_Skeleton_GetAttachPoint, NULL);
//	if (!m_lAttachPoints.Len()) return NULL;		// This check is done by ValidPos aswell
	if (m_lAttachPoints.ValidPos(_iAttach))
		return &m_lAttachPoints[_iAttach];
	else 
		return NULL;
}

void CXR_Skeleton::CalculateLocalMatrices(CXR_SkeletonInstance* _pSkelInstance, const CMat4Dfp32& _BaseMat)
{
	// Calculates local matrices
	CMat4Dfp32 MatInv;
	_BaseMat.InverseOrthogonal(MatInv);
	MatInv.Multiply(_pSkelInstance->m_pBoneTransform[0],_pSkelInstance->m_pBoneLocalPos[0]);

	const CXR_SkeletonNode* pNodes = m_lNodes.GetBasePtr();
	const int NumNodes = Min(m_lNodes.Len(),(int)_pSkelInstance->m_nBoneTransform);
	const CXR_SkeletonNode* pNode;
	const CMat4Dfp32* pWorkingTransform;

	for (int iNode = 1; iNode < NumNodes; iNode++)
	{
		pNode = &pNodes[iNode];
		pWorkingTransform = &_pSkelInstance->m_pBoneTransform[pNode->m_iNodeParent];

		CMat4Dfp32 Local2;
		CMat4Dfp32 MatParentInv;
		pWorkingTransform->InverseOrthogonal(MatParentInv);
		_pSkelInstance->m_pBoneTransform[iNode].Multiply(MatParentInv,Local2);

		CVec3Dfp32 Res = -Local2.GetRow(3) + pNode->m_LocalCenter;
		for (int i = 0; i < 3; i++)
			Res.k[i] -= Local2.k[0][i] * pNode->m_LocalCenter.k[0] + Local2.k[1][i] * pNode->m_LocalCenter.k[1] + Local2.k[2][i] * pNode->m_LocalCenter.k[2];

		Res.SetMatrixRow(Local2,3);
		_pSkelInstance->m_pBoneLocalPos[iNode] = Local2;
	}
}

void CXR_Skeleton::CalculateLocalMatrices(CXR_SkeletonInstance* _pSkelInstance, const CMat4Dfp32& _BaseMat, uint16* _liNodes, int32 _nNodes)
{
	// Calculates local matrices
	CMat4Dfp32 MatInv;
	_BaseMat.InverseOrthogonal(MatInv);
	MatInv.Multiply(_pSkelInstance->m_pBoneTransform[0],_pSkelInstance->m_pBoneLocalPos[0]);

	const CXR_SkeletonNode* pNodes = m_lNodes.GetBasePtr();
	const int NumNodes = Min(m_lNodes.Len(),(int)_pSkelInstance->m_nBoneTransform);
	const CXR_SkeletonNode* pNode;
	const CMat4Dfp32* pWorkingTransform;

	for (int iLNode = 0; iLNode < _nNodes; iLNode++)
	{
		uint16 iNode = _liNodes[iLNode];
		if (iNode >= NumNodes)
			continue;
		pNode = &pNodes[iNode];
		pWorkingTransform = &_pSkelInstance->m_pBoneTransform[pNode->m_iNodeParent];

		CMat4Dfp32 Local2;
		CMat4Dfp32 MatParentInv;
		pWorkingTransform->InverseOrthogonal(MatParentInv);
		_pSkelInstance->m_pBoneTransform[iNode].Multiply(MatParentInv,Local2);

		CVec3Dfp32 Res = -Local2.GetRow(3) + pNode->m_LocalCenter;
		for (int i = 0; i < 3; i++)
			Res.k[i] -= Local2.k[0][i] * pNode->m_LocalCenter.k[0] + Local2.k[1][i] * pNode->m_LocalCenter.k[1] + Local2.k[2][i] * pNode->m_LocalCenter.k[2];

		Res.SetMatrixRow(Local2,3);
		_pSkelInstance->m_pBoneLocalPos[iNode] = Local2;
	}
}

void CXR_Skeleton::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CXR_Skeleton_Read, MAUTOSTRIP_VOID);
	// NODES
	_pDFile->PushPosition();
	{
		if (!_pDFile->GetNext("NODES"))
			Error("Read", "No NODES entry.");

		m_lNodes.SetLen(_pDFile->GetUserData());
		for(int i = 0; i < m_lNodes.Len(); i++)
			m_lNodes[i].Read(_pDFile);
	}
	_pDFile->PopPosition();

	// NODEINDICES
	_pDFile->PushPosition();
	{
		if (!_pDFile->GetNext("NODEINDICES"))
			Error("Read", "No NODEINDICES entry.");

		uint nNodes = _pDFile->GetUserData();
		m_liNodes.SetLen(nNodes);
		_pDFile->GetFile()->ReadLE(m_liNodes.GetBasePtr(), nNodes);
	}
	_pDFile->PopPosition();

	// ATTACHPOINTS
	_pDFile->PushPosition();
	if (_pDFile->GetNext("ATTACHPOINTS"))
	{
		uint nPoints = _pDFile->GetUserData();
		m_lAttachPoints.SetLen(nPoints);
		for (uint i = 0; i < nPoints; i++) 
			m_lAttachPoints[i].Read(_pDFile);
	}
	_pDFile->PopPosition();

	// CLOTH
	_pDFile->PushPosition();
	uint nCloth = 0;
	{
		_pDFile->PushPosition();
		while (_pDFile->GetNext("CLOTH"))
			nCloth++;
		m_lCloth.SetLen(nCloth);
		_pDFile->PopPosition();
	}
	for (uint iCloth = 0; iCloth < nCloth; iCloth++)
	{
		_pDFile->GetNext("CLOTH");
		CXR_Cloth& cloth = m_lCloth[iCloth];
		cloth.Read(_pDFile, this);

		// Compat
		if (cloth.m_lJoints.Len() > 0)
			cloth.m_GroupId = m_lCloth.Len();

		if (cloth.m_Name == "")
			cloth.m_Name = CStrF("%d", cloth.m_GroupId);

		//m_lClothConstraints.SetLen(_pDFile->GetUserData());
		//for(int i = 0; i < m_lClothConstraints.Len(); i++) m_lClothConstraints[i].Read(_pDFile);
	}
	_pDFile->PopPosition();
	

	Init();
}


void CXR_Skeleton::Write(CDataFile* _pDFile) const
{
	MAUTOSTRIP(CXR_Skeleton_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	// NODES
	_pDFile->BeginEntry("NODES");
	{
		for(int i = 0; i < m_lNodes.Len(); i++)
			m_lNodes[i].Write(_pDFile);
	}
	_pDFile->EndEntry(m_lNodes.Len());

	// NODEINDICES
	_pDFile->BeginEntry("NODEINDICES");
	{
		for(int i = 0; i < m_liNodes.Len(); i++) _pDFile->GetFile()->WriteLE(m_liNodes[i]);
	}
	_pDFile->EndEntry(m_liNodes.Len());

	// ATTACHPOINTS
	if (m_lAttachPoints.Len())
	{
		_pDFile->BeginEntry("ATTACHPOINTS");
		for(int i = 0; i < m_lAttachPoints.Len(); i++) m_lAttachPoints[i].Write(_pDFile);
		_pDFile->EndEntry(m_lAttachPoints.Len());
	}

	// CLOTH
	if (m_lCloth.Len())
	{
		for(int i = 0; i < m_lCloth.Len(); i++) {
			m_lCloth[i].Write(_pDFile,this);
		}

/*		_pDFile->BeginEntry("CLOTH");
		for(int i = 0; i < m_lClothConstraints.Len(); i++) m_lClothConstraints[i].Write(_pDFile);
		_pDFile->EndEntry(m_lClothConstraints.Len());
		*/
	}
	

#endif	// PLATFORM_CONSOLE
}
