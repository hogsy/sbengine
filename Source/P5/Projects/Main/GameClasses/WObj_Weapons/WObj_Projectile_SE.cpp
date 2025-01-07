#include "pch.h"

#include "WObj_Projectile_SE.h"
#include "../WRPG/WRPGInitParams.h"
///#define VERBOSE

#define MAXTRAVELDISTANCE 200.0f	// This is multiplied by 100
#define ALLOW_CRITICAL_HEADSHOT 0	// Don't allow severe head shots for the moment

static const int8 s_lMaterialToImpactGroup[][10] =
{
	{ IMPACTMODEL_METAL },		// Undefined

	{ IMPACTMODEL_ROCK,			// Rock
		IMPACTMODEL_CONCRETE,	//	Rock Concrete
		0,						//	Rock Dusty
		IMPACTMODEL_PLASTER,	//	Rock Plaster
	},

	{ IMPACTMODEL_WOOD,			// Wood
		0,						//	Wood Hollow
		0,						//	Wood Groaning
		IMPACTMODEL_CARDBOARD,	//	Wood Paper
	},

	{ IMPACTMODEL_WATER },		// Water

	{ IMPACTMODEL_PLASTIC,		// Plastic
		IMPACTMODEL_RUBBER,		//	Plastic Rubber
		0,						//	Plastic Electronics
	},

	{ IMPACTMODEL_FLESH,		// Human
		IMPACTMODEL_FLESH,		//  Human head
		IMPACTMODEL_DEADFLESH,	//  Human corpse
	},

	{ IMPACTMODEL_METAL_RICOCHET,	// Metal
		IMPACTMODEL_METAL,			// Metal plate
		IMPACTMODEL_METAL,			// Metal Hollow
		IMPACTMODEL_METAL,			// Metal Loose
		IMPACTMODEL_METAL,			// Metal grate
	},

	{ IMPACTMODEL_GROUND },		// Snow
	{ IMPACTMODEL_GROUND },		// Sand

	{ IMPACTMODEL_GROUND,		// Ground
		0,						//	Ground Hard
		IMPACTMODEL_ASPHALT,	//	Ground Asphalt
		0,						//	Ground Mud
	},

	{ IMPACTMODEL_CLOTH,		// Soft
		IMPACTMODEL_RUBBER,		// Soft Cloth
	},

	{ IMPACTMODEL_GLASS,		// Glass
		IMPACTMODEL_PORCELAIN,	//	Glass Porcelain
		0,						//	Glass Tile
	},
};
enum { e_NumMaterialCategories = sizeof(s_lMaterialToImpactGroup) / sizeof(s_lMaterialToImpactGroup[0]) };

static const char* s_LeightweightProjectilesFlags[] = { "impactmodel", "impactsound", "impactwallmark", "ricochetsound", "lrpsound", "damage", "impactlight", "usehitlist", NULL };

CPClu_PTCollision::CPClu_PTCollision()
{
	Clear();
}

CPClu_PTCollision::CPClu_PTCollision(CWObject_ProjectileCluster* _pCluster, int _iProjectile, bool _bHasHit)
{
	Clear();
	m_pCluster = _pCluster;
	m_iProjectile = _iProjectile;
	m_bHasHit = _bHasHit;
}

void CPClu_PTCollision::Clear()
{
	m_iProjectile = -1;
	m_Position = 0;
	m_Direction = 0;
	m_Distance = 0;
	m_iGroup = -1;
	m_iMaterial = 0;
	m_SurfaceName = "";
	m_pCInfo = NULL;
	m_Pos1 = 0;
	m_Pos2 = 0;
	m_pCluster = NULL;
	m_bHasHit = false;
	m_pSurface = NULL;
	m_pProjectile = NULL;
	m_bCollision = false;
}

bool CPClu_PTCollision::IsValidProjectile()
{
	return ((m_iProjectile >= 0) ? true : false);
}

bool CPClu_PTCollision::IsValidGroup()
{
	return ((m_iGroup >= 0) ? true : false);
}

bool CPClu_PTCollision::IsValidMaterial()
{
	return ((m_iMaterial >= 0) ? true : false);
}

bool CPClu_PTCollision::IsValidSurface()
{
	return ((m_SurfaceName != "") ? true : false);
}

bool CPClu_PTCollision::IsValidCollInfo()
{
	return ((m_pCInfo) ? true : false);
}

bool CPClu_PTCollision::IsValidCluster()
{
	return ((m_pCluster) ? true : false);
}

bool CPClu_PTCollision::IsCollidingImpact()
{
	return m_bHasHit;
}

bool& CPClu_PTCollision::IsValidCollision()
{
	return m_bCollision;
}

const CVec3Dfp32& CPClu_PTCollision::GetPosition()
{
	return m_Position;
}

const CVec3Dfp32& CPClu_PTCollision::GetDirection()
{
	return m_Direction;
}

void CPClu_PTCollision::SetupLine(const CVec3Dfp32& _Pos1, const CVec3Dfp32& _Pos2)
{
	m_Pos1 = _Pos1;
	m_Pos2 = _Pos2;
}

bool CPClu_PTCollision::IntersectBoundBox(const CBox3Dfp32& _WBoundBox, bool _Store)
{
	if(_WBoundBox.IntersectLine(m_Pos1, m_Pos2, m_Position))
	{
		if(_Store)
		{
			m_Direction = (m_Position - m_Pos1);
			m_Distance = m_Direction.Length();
			m_bCollision = true;
		}
		return true;
	}

	return false;
}

bool CPClu_PTCollision::IntersectOBB(const COBBfp32& _WOBoundBox, bool _Store)
{
	const CVec3Dfp32 Pos1 = m_Pos1 - _WOBoundBox.m_C;
	const CVec3Dfp32 Pos2 = m_Pos2 - _WOBoundBox.m_C;
	if(_WOBoundBox.LocalIntersectLine(Pos1, Pos2, m_Position))
	{
		if(_Store)
		{
			m_Direction = m_Position - Pos1;
			m_Distance = m_Direction.Length();
			m_Direction.Normalize();
			m_bCollision = true;
			m_Position += _WOBoundBox.m_C;
		}
		return true;
	}

	return false;
}

bool CPClu_PTCollision::IntersectPlane(const CPlane3Dfp32& _Plane)
{
	if(_Plane.IntersectLineSegment(m_Pos1, m_Pos2, m_Position))
	{
		m_Direction = (m_Position - m_Pos1);
		m_Distance = m_Direction.Length();
		m_bCollision = true;
		return true;
	}

	return false;
}

bool CPClu_PTCollision::IsInfrontOfCollision()
{
	if(m_bHasHit && m_pCInfo)
	{
		if(m_Distance < (m_pCInfo->m_Pos - m_Pos1).Length())
			return true;

		return false;
	}

	return true;
}

void CPClu_PTCollision::ResolveSurfaceMaterial(CWorld_Server* _pWServer)
{
	m_pSurface = _pWServer->GetMapData()->GetResource_Surface(
		_pWServer->GetMapData()->GetResourceIndex_Surface(m_SurfaceName));

	if(m_iMaterial == 0 && m_pSurface && m_pSurface->GetBaseFrame())
	{
		m_iMaterial = m_pSurface->GetBaseFrame()->m_MaterialType;
		ResolveSurfaceGroup();
	}
}

void CPClu_PTCollision::ResolveSurfaceGroup()
{
	uint8 iCategory = m_iMaterial / 10;
	uint8 iSubType =  m_iMaterial % 10;

	if (iCategory >= e_NumMaterialCategories)
		return;

	m_iGroup = s_lMaterialToImpactGroup[iCategory][iSubType];
	if (m_iGroup == 0)
		m_iGroup = s_lMaterialToImpactGroup[iCategory][0];
}


CWO_ModelCluster::CWO_ModelCluster()
	: m_iExtension(0)
{
}

CWO_ModelCluster::CWO_ModelCluster(const CWO_ModelCluster& _o)
{
	*this = _o;
}

CWO_ModelCluster& CWO_ModelCluster::operator=(const CWO_ModelCluster& _o)
{
	m_lModels = _o.m_lModels;
	m_iExtension = _o.m_iExtension;
	return *this;
}

void CWO_ModelCluster::AddModel(int16 _iModel)
{
	int CurLen = m_lModels.Len();
	m_lModels.SetLen(CurLen+1);
	m_lModels[CurLen] = _iModel;
}

/*void CWO_ModelCluster::Read(CCFile* _pFile)
{
	uint8 iModel;
	uint8 nModels;
	_pFile->ReadLE(nModels);
	m_lModels.SetLen(nModels);
	for(iModel=0; iModel<nModels; iModel++)
	{
		uint16 Model;
		_pFile->ReadLE(Model);
		m_lModels[iModel] = Model;
	}
}

void CWO_ModelCluster::Write(CCFile* _pFile)
{
	uint8 iModel;
	uint8 nModels = m_lModels.Len();
	_pFile->WriteLE(nModels);
	for(iModel=0; iModel<nModels; iModel++)
	{
		_pFile->WriteLE(m_lModels[iModel]);
	}
}*/


CWO_ProjectileInstance::CWO_ProjectileInstance()
{
	m_Flags = 0;
}

void CWO_ProjectileInstance::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
/*
	MACRO_READ(uint8, m_bAlive);
	MACRO_READ(int32, m_PendingDeath);
	MACRO_READ(fp32, m_ImpactTime);
	MACRO_READ(int32, m_RandSeed);
	MACRO_READ(fp64, m_TravelDistance);
	m_Position.Read(_pFile);
	m_Velocity.Read(_pFile);
	m_ProjectileModelCluster.Read(_pFile);
	m_ImpactModelCluster.Read(_pFile);
*/
/* RUNE LOAD/SAVE
	uint8 Temp8;
	_pFile->ReadLE(Temp8);
	m_bAlive = (Temp8!=0);
	_pFile->ReadLE(m_PendingDeath);
	m_Position.Read(_pFile);
	m_Velocity.Read(_pFile);
	_pFile->ReadLE(m_ImpactTime);
	m_ProjectileModelCluster.Read(_pFile);
	m_ImpactModelCluster.Read(_pFile);
	_pFile->ReadLE(m_RandSeed);
	_pFile->ReadLE(m_TravelDistance);
*/
}

void CWO_ProjectileInstance::OnDeltaSave(CCFile* _pFile)
{
/*
	MACRO_WRITE(uint8, Temp8);
	MACRO_WRITE(int32, m_PendingDeath);
	MACRO_WRITE(fp32, m_ImpactTime);
	MACRO_WRITE(int32, m_RandSeed);
	MACRO_WRITE(fp64, m_TravelDistance);
	m_Position.Write(_pFile);
	m_Velocity.Write(_pFile);
	m_ProjectileModelCluster.Write(_pFile);
	m_ImpactModelCluster.Write(_pFile);
*/
/* RUNE LOAD/SAVE
	uint8 Temp8 = m_bAlive;
	_pFile->WriteLE(Temp8);
	_pFile->WriteLE(m_PendingDeath);
	m_Position.Write(_pFile);
	m_Velocity.Write(_pFile);
	_pFile->WriteLE(m_ImpactTime);
	m_ProjectileModelCluster.Write(_pFile);
	m_ImpactModelCluster.Write(_pFile);
	_pFile->WriteLE(m_RandSeed);
	_pFile->WriteLE(m_TravelDistance);
*/
}

CWO_ProjectileInstance_Client::CWO_ProjectileInstance_Client()
{
	m_ImpactTimeFrac = 0.0f;
	m_ImpactState = IMPACTSTATE_IMPACTPENDING;
	m_ImpactTick = 0;
}

bool CWO_ProjectileInstance_Client::GetPositionMatrix(fp32 _IPTime, CMat4Dfp32 &_Mat, CMat4Dfp32 &_ImpactMat)
{
	_Mat = m_Position;

	bool bRes = false;
	if(m_ImpactState == IMPACTSTATE_IMPACTHASHAPPENED || (m_ImpactState == IMPACTSTATE_IMPACTTHISFRAME && _IPTime > m_ImpactTimeFrac))
	{
		bRes = true;
		_ImpactMat = m_ImpactPos;
	}
	else
		CVec3Dfp32::GetRow(_Mat, 3) += m_Velocity * _IPTime;

	return bRes;
}


CWO_ProjectileCluster_DamageRange::CWO_ProjectileCluster_DamageRange()
{
	m_RangeBegin = _FP32_MIN;
	m_RangeEnd = _FP32_MAX;
	m_Damage = 0;
	m_nAllowedHits = ~0;
	m_pNextRange = NULL;
}

bool CWO_ProjectileCluster_DamageRange::IsInRange(fp32 _Pos)
{
	if((m_RangeBegin == _FP32_MIN)
	&& (m_RangeEnd == _FP32_MAX))
		return true;

	if((m_RangeBegin == _FP32_MIN)
	&& (m_RangeEnd != _FP32_MAX))
		return (_Pos < m_RangeEnd);

	if((m_RangeBegin != _FP32_MIN)
	&& (m_RangeEnd == _FP32_MAX))
		return (_Pos >= m_RangeBegin);

	return ((_Pos >= m_RangeBegin) && (_Pos < m_RangeEnd));
}

bool CWO_ProjectileCluster_DamageRange::IsFallback()
{
	if((m_RangeBegin == _FP32_MIN)
	&& (m_RangeEnd == _FP32_MAX))
		return true;

	return false;
}




void CWObject_ProjectileCluster::OnCreate()
{
	CWObject_Damage::OnCreate();

	m_nProjectiles = 0;
	m_DistributionWidth = 0.0f;
	m_DistributionHeight = 0.0f;

	m_ProjectileVelocity = 32.0f * m_pWServer->GetGameTicksPerSecond();
	m_ProjectileVelocityVariation = 0.0f;
	m_ProjectileAcceleration = _FP32_MAX;
	m_ProjectileGravity = 0.0f;

	m_InitialProjectilePoolSize = -1;

	m_ImpactSound = 0;
	m_ImpactFleshSound = 0;
	m_RicochetSound = 0;
	m_LRPSound = 0;

	m_RandSeedBase = 0;
	m_bCanBeRemoved = false;

	m_ImpactSoundTick = 0;
	m_RenderTimeout = (int32)(2 * m_pWServer->GetGameTicksPerSecond());

	m_ImpactLightDefaultColor = 0;
	m_ImpactLightDefaultRange = 0;

	m_LeightweightProjectiles.m_nProjectiles = 0;
	m_LeightweightProjectiles.m_Flags = 0;

#ifdef VERBOSE
	ConOutL(CStrF("Creating 0x%08x", this));
#endif

	m_pFirstDamageRange = NULL;

	m_SpawnGroup = 0;

	//M_TRACEALWAYS(CStrF("Spawning CWObject_ProjectileCluster(%d)\n", m_iObject));
}

void CWObject_ProjectileCluster::OnDestroy()
{
	//M_TRACEALWAYS(CStrF("Destroying CWObject_ProjectileCluster(%d)\n", m_iObject));

#ifdef VERBOSE
	ConOutL(CStrF("Destroying 0x%08x", this));
#endif

	m_lProjectileInstance.Clear();

	CWO_ProjectileCluster_DamageRange* pRange = m_pFirstDamageRange;

	while(pRange)
	{
		CWO_ProjectileCluster_DamageRange* pKillMe = pRange;
		pRange = pRange->m_pNextRange;
		delete pKillMe;
	}

	m_pFirstDamageRange = NULL;

	CWObject_Ext_Model::OnDestroy();
}

void CWObject_ProjectileCluster::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	CWObject_Damage::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

	// Make sure all impact models gets precached
	if(_pReg)
	{
		int nKeys = _pReg->GetNumChildren();
		for(int iKey=0; iKey<nKeys; iKey++)
		{
			CRegistry *pKey = _pReg->GetChild(iKey);
			CStr KeyName = pKey->GetThisName();
			if(KeyName.Copy(0, 17) == "PCLU_IMPACTMODEL_")
			{
				CFStr ModelName = pKey->GetThisValue();
				_pMapData->GetResourceIndex_Model(ModelName);
			}
			else if(KeyName.Copy(0, 14) == "PCLU_WALLMARK_")
			{
				CFStr Wallmark = pKey->GetThisValue();
				if(Wallmark[0] == '$')
					Wallmark = Wallmark.Copy(1, 1024);
				while(Wallmark != "")
				{					
					_pMapData->GetResourceIndex_Surface(Wallmark.GetStrSep(","));
					Wallmark.GetStrSep(",");
				}
			}
		}
	}

	IncludeSoundFromKey("PCLU_IMPACTSOUND", _pReg, _pMapData);
	IncludeSoundFromKey("PCLU_IMPACTFLESHSOUND", _pReg, _pMapData);
	IncludeSoundFromKey("PCLU_RICOCHETSOUND", _pReg, _pMapData);
	IncludeSoundFromKey("PCLU_LRPSOUND", _pReg, _pMapData);
	IncludeModelFromKey("PCLU_PROJECTILEMODEL", _pReg, _pMapData);
	CRegistry* pReg = _pReg->Find("PCLU_BOUNCESPAWNOBJECT");
	if (pReg)
		_pMapData->GetResourceIndex_Template(pReg->GetThisValue());
	
	_pMapData->GetResourceIndex_Class("Glass");
//	int ijk = 0;
}

void CWObject_ProjectileCluster::EvalDamageRange(const CRegistry* _pReg)
{
	int nChilds = _pReg->GetNumChildren();
	int iChild;
	CWO_ProjectileCluster_DamageRange* pRange = DNew(CWO_ProjectileCluster_DamageRange) CWO_ProjectileCluster_DamageRange;

	for (iChild=0; iChild<nChilds; iChild++)
	{
		const CRegistry* pChildKey = _pReg->GetChild(iChild);

		if (pChildKey->GetThisName() == "DAMAGERANGE_DAMAGE")
			pRange->m_Damage = pChildKey->GetThisValuei();

		else if (pChildKey->GetThisName() == "DAMAGERANGE_BEGIN")
			pRange->m_RangeBegin = (pChildKey->GetThisValuef() * 32.0f) / 100.0f;

		else if (pChildKey->GetThisName() == "DAMAGERANGE_END")
			pRange->m_RangeEnd = (pChildKey->GetThisValuef() * 32.0f) / 100.0f;

		else if (pChildKey->GetThisName() == "DAMAGERANGE_HITS")
			pRange->m_nAllowedHits = (uint8)pChildKey->GetThisValuei();

		else
			ConOutL(CStr("Warning: Unknown damage-range key"));
	}

	// Link this new damage range into the list
	// If this is a fallback range, make sure it
	// is linked last in the list
	if (pRange->IsFallback())
	{
		// Make m_Damage valid
		m_Damage.m_Damage = pRange->m_Damage;

		if (!m_pFirstDamageRange)
		{
			// pRange is the first range added. Link it first and it will
			// keep being pushed to the end of the list per automatic
			m_pFirstDamageRange = pRange;
		}
		else
		{
			// Traverse to the end of the linked list and add pRange there
			CWO_ProjectileCluster_DamageRange* pTempRange = m_pFirstDamageRange;
			while(pTempRange->m_pNextRange) pTempRange = pTempRange->m_pNextRange;
			pTempRange->m_pNextRange = pRange;
		}

		return;
	}

	// This is a regular range
	// Just link it at the beginning of the list
	pRange->m_pNextRange = m_pFirstDamageRange;
	m_pFirstDamageRange = pRange;
}

int CWObject_ProjectileCluster::EvalMaterial(const char* _pStr)
{
	const char* lTypes[] = { 
		"$$$", "Metal", "Concrete", "Wood", "Glass", "Plastic", "Plaster", "Cloth", 
		"Flesh", "Deadflesh", "Cardboard", "Ground", "Rock", "Asphalt", "Rubber", "Porcelain","Metal_Ricochet","Water", NULL 
	};

	int iGroup = CStrBase::TranslateInt(_pStr, lTypes);
	if (iGroup < 0)
	{
		ConOutL(CStrF("WARNING: Unknown impact model '%s'", _pStr));
		return IMPACTMODEL_METAL;
	}

	return iGroup;
}

void CWObject_ProjectileCluster::EvalImpactModelKey(const CRegistry* _pKey)
{
	CFStr Name = _pKey->GetThisName().Copy(17, 1024);
	int iImpactModel = m_pWServer->GetMapData()->GetResourceIndex_Model(_pKey->GetThisValue());

	int iMaterial = EvalMaterial(Name);
	m_ImpactModelCluster[iMaterial].AddModel(iImpactModel);
}

void CWObject_ProjectileCluster::EvalWallmarkKey(const CRegistry* _pKey)
{
	CFStr Name = _pKey->GetThisName().Copy(14, 1024);
	int iMaterial = EvalMaterial(Name);

	CWO_ModelCluster::CWallmarkInfo Info[8];
	CFStr Val = _pKey->GetThisValue();
	bool bRot = false;
	if(Val[0] == '$')
	{
		bRot = true;
		Val = Val.Copy(1, Val.Len());
	}
	int nInfo = 0;
	int iExt = 0;
	while(Val != "")
	{
		// Do we have extensions?
		if(Val[0] == '#')
		{
			iExt = 1;
			Val = Val.Copy(1, Val.Len());
			Info[nInfo].m_Distance.k[0] = Val.GetStrSep(",").Val_fp64();
			Info[nInfo].m_Distance.k[1] = Val.GetStrSep(",").Val_fp64();
		}

		Info[nInfo].m_iSurface = m_pWServer->GetMapData()->GetResourceIndex_Surface(Val.GetStrSep(","));
		if(bRot)
			Info[nInfo].m_iSurface |= 0x8000;
		Info[nInfo++].m_Size = Val.GetStrSep(",").Val_int();
	}
	if(nInfo > 0)
	{
		m_ImpactModelCluster[iMaterial].m_iExtension = iExt;
		m_ImpactModelCluster[iMaterial].m_lWallmarks.SetLen(nInfo);
		for(int i = 0; i < nInfo; i++)
			m_ImpactModelCluster[iMaterial].m_lWallmarks[i] = Info[i];
	}
}

void CWObject_ProjectileCluster::AddProjectileModel(const CRegistry* _pKey)
{
	int iModel = m_pWServer->GetMapData()->GetResourceIndex_Model(_pKey->GetThisValue());
	int CurLen = m_lProjectileModelCluster.Len();
	m_lProjectileModelCluster.SetLen(CurLen + 1);
	m_lProjectileModelCluster[CurLen].AddModel(iModel);
}

void CWObject_ProjectileCluster::SetInitialPoolSize(const CRegistry* _pKey)
{
	m_InitialProjectilePoolSize = _pKey->GetThisValue().Val_int();
}

void CWObject_ProjectileCluster::SetRenderTimeout(const CRegistry* _pKey)
{
	m_RenderTimeout = (int)(_pKey->GetThisValuei() * m_pWServer->GetGameTicksPerSecond());
}

void CWObject_ProjectileCluster::GetRandomProjectileModel(CWO_ModelCluster& _ModelCluster)
{
	int MaxModels = m_lProjectileModelCluster.Len();
	if (MaxModels == 0)
	{
		// Do somehting error-like
		_ModelCluster.m_lModels.SetLen(0);
		return;
	}

	int iModelCluster = MRTC_RAND() % MaxModels;
	iModelCluster = 0;	// DEBUG test
	_ModelCluster = m_lProjectileModelCluster[iModelCluster];
}

void CWObject_ProjectileCluster::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();

	// First make sure this is a projectile cluster (PCLU) registry key
	if (KeyName.Copy(0, 5) != "PCLU_")
		CWObject_Damage::OnEvalKey(_KeyHash, _pKey);
	else switch (_KeyHash)
	{
	// Number of projectiles in this cluster
	case MHASH4('PCLU','_PRO','JECT','ILES'): // "PCLU_PROJECTILES"
		{
			m_nProjectiles = KeyValue.Val_int();
			break;
		}

	case MHASH7('PCLU','_LIG','HTWE','IGHT','_PRO','JECT','ILES'): // PCLU_LIGHTWEIGHT_PROJECTILES
		{
			int nChilds = _pKey->GetNumChildren();
			for(int i = 0; i < nChilds; i++)
			{
				const CRegistry* pChild = _pKey->GetChild(i);
				OnEvalKey(pChild->GetThisNameHash(), pChild);
			}
			break;
		}

	case MHASH5('PCLU','_LW_','PROJ','ECTI','LES'): // PCLU_LW_PROJECTILES
		{
			m_LeightweightProjectiles.m_nProjectiles = KeyValue.Val_int();
			break;
		}

	case MHASH4('PCLU','_LW_','FLAG','S'): // PCLU_LW_FLAGS
		{
			m_LeightweightProjectiles.m_Flags = KeyValue.TranslateFlags(s_LeightweightProjectilesFlags);
			break;
		}

	// Width of projectile distribution (think shotgun spread) measured in width units per 32 length units (one meter)
	case MHASH4('PCLU','_DIS','TWID','TH'): // "PCLU_DISTWIDTH"
		{
			m_DistributionWidth = KeyValuef;
			break;
		}

	// Height of projectile distribution in height units per 32 length units
	case MHASH4('PCLU','_DIS','THEI','GHT'): // "PCLU_DISTHEIGHT"
		{
			m_DistributionHeight = KeyValuef;
			break;
		}

	// If the projectiles aren't instant, what should be it's initial velocity. Velocity from registry is in meters per second, here it is recalculated to units per gametick.
	case MHASH6('PCLU','_PRO','JECT','ILEV','ELOC','ITY'): // "PCLU_PROJECTILEVELOCITY"
		{
			m_ProjectileVelocity = KeyValuef * (32.0f * m_pWServer->GetGameTickTime());
			break;
		}

	case MHASH6('PCLU','_VEL','OCIT','Y_VA','RIAT','ION'): // "PCLU_VELOCITY_VARIATION"
		{
			m_ProjectileVelocityVariation = KeyValuef * (32.0f * m_pWServer->GetGameTickTime());
			break;
		}

	// If the projectiles aren't instant, how should they accelerate
	case MHASH7('PCLU','_PRO','JECT','ILEA','CCEL','ERAT','ION'): // "PCLU_PROJECTILEACCELERATION"
		{
			m_ProjectileAcceleration = KeyValuef * (32.0f * m_pWServer->GetGameTickTime());
			break;
		}

	// If the projectiles aren't instant, how they should be affected by gravity
	case MHASH6('PCLU','_PRO','JECT','ILEG','RAVI','TY'): // "PCLU_PROJECTILEACCELERATION"
		{
			m_ProjectileGravity = KeyValuef * (32.0f * m_pWServer->GetGameTickTime());
			break;
		}

	// Damage range for this projectile
	case MHASH5('PCLU','_DAM','AGER','ANGE','_0'): // "PCLU_DAMAGERANGE_0"
	case MHASH5('PCLU','_DAM','AGER','ANGE','_1'): // "PCLU_DAMAGERANGE_1"
	case MHASH5('PCLU','_DAM','AGER','ANGE','_2'): // "PCLU_DAMAGERANGE_2"
	case MHASH5('PCLU','_DAM','AGER','ANGE','_3'): // "PCLU_DAMAGERANGE_3"
	case MHASH5('PCLU','_DAM','AGER','ANGE','_4'): // "PCLU_DAMAGERANGE_4"
	case MHASH5('PCLU','_DAM','AGER','ANGE','_5'): // "PCLU_DAMAGERANGE_5"
	case MHASH5('PCLU','_DAM','AGER','ANGE','_6'): // "PCLU_DAMAGERANGE_6"
	case MHASH5('PCLU','_DAM','AGER','ANGE','_7'): // "PCLU_DAMAGERANGE_7"
	case MHASH5('PCLU','_DAM','AGER','ANGE','_8'): // "PCLU_DAMAGERANGE_8"
	case MHASH5('PCLU','_DAM','AGER','ANGE','_9'): // "PCLU_DAMAGERANGE_9"
		{
			EvalDamageRange(_pKey);
			break;
		}

	// Add another projectilemodel of which to randomize from
	case MHASH5('PCLU','_PRO','JECT','ILEM','ODEL'): // "PCLU_PROJECTILEMODEL"
		{
			AddProjectileModel(_pKey);
			break;
		}

	// Impact sound descriptor to be played when a projectile hits a surface
	case MHASH4('PCLU','_IMP','ACTS','OUND'): // "PCLU_IMPACTSOUND"
		{
			m_ImpactSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	// Impact sound descriptor to be played when a projectile hits flesh
	case MHASH6('PCLU','_IMP','ACTF','LESH','SOUN','D'): // "PCLU_IMPACTFLESHSOUND"
		{
			m_ImpactFleshSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	// Ricochet sound descriptor to be played when a projectile hits a surface
	case MHASH5('PCLU','_RIC','OCHE','TSOU','ND'): // "PCLU_RICOCHETSOUND"
		{
			m_RicochetSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	case MHASH4('PCLU','_LRP','SOUN','D'): // "PCLU_LRPSOUND"
		{
			m_LRPSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	case MHASH6('PCLU','_BOU','NCES','PAWN','OBJE','CT'): // "PCLU_BOUNCESPAWNOBJECT"
		{
			m_BounceSpawnObject = KeyValue;
			break;
		}

	// Performance: Initial size of the projectile pool.
	case MHASH7('PCLU','_PER','F_IN','ITIA','LPOO','LSIZ','E'): // "PCLU_PERF_INITIALPOOLSIZE"
		{
			SetInitialPoolSize(_pKey);
			break;
		}

	// Performance: Increase or decrease rendertimeout. For example to allow longer rendering of impact models. Unit is seconds.
	case MHASH7('PCLU','_PER','F_IM','PACT','REND','ERTI','ME'): // "PCLU_PERF_IMPACTRENDERTIME"
		{
			SetRenderTimeout(_pKey);
			break;
		}

	case MHASH4('PCLU','_SPA','WNFL','ASH'): // "PCLU_SPAWNFLASH"
		{
			iAnim0() = 1;
			ClientFlags() |= CWO_CLIENTFLAGS_VISIBILITY;
			break;
		}

	case MHASH8('PCLU','_IMP','ACTL','IGHT','_DEF','AULT','COLO','R'): // "PCLU_IMPACTLIGHT_DEFAULTCOLOR"
		{
			CVec3Duint8 Color;
			Color.ParseString(KeyValue);
			m_ImpactLightDefaultColor = ((0xff << 24) | (Color.k[0] << 16) | (Color.k[1] << 8) | (Color.k[2]));
			break;
		}
	case MHASH8('PCLU','_IMP','ACTL','IGHT','_DEF','AULT','RANG','E'): // "PCLU_IMPACTLIGHT_DEFAULTRANGE"
		{
			m_ImpactLightDefaultRange = KeyValue.Val_int();
			break;
		}

	// Not a recognized projectile cluster registry key but it happened to have the PCLU_ prefix. Safety
	default:
		{
			// The models that should be spawned at the point of impact
			if (KeyName.Copy(0, 17) == "PCLU_IMPACTMODEL_")
				EvalImpactModelKey(_pKey);

		// Add another projectilemodel of which to randomize from
			else if (KeyName.Copy(0, 14) == "PCLU_WALLMARK_")
				EvalWallmarkKey(_pKey);
			else
				CWObject_Damage::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_ProjectileCluster::OnFinishEvalKeys()
{
	CWObject_Damage::OnFinishEvalKeys();

	m_lProjectileInstance.SetLen(m_nProjectiles+m_LeightweightProjectiles.m_nProjectiles);
}

void CWObject_ProjectileCluster::OnInitInstance(const aint* _pParam, int _nParam)
{
	CWObject_Damage::OnInitInstance(_pParam, _nParam);

	CRPG_InitParams* pInitParams = NULL;
	if (_nParam > 0)
		pInitParams = (CRPG_InitParams*)_pParam[0];

	m_bFirstOnRefresh = true;
}

void CWObject_ProjectileCluster::NetMsgSendModelCluster(int _iProjectile, const CWO_ModelCluster& _ModelCluster, int _Target)
{
	CNetMsg Msg(NETMSG_PCLU_PROJECTILEINSTANCE_SENDMODELCLUSTER);
	Msg.AddInt16(_iProjectile);
	Msg.AddInt8(_Target);		// Projectile (0) or impact (1) cluster

	int nModels = _ModelCluster.m_lModels.Len();
	Msg.AddInt8(nModels);

	int iModel;
	for (iModel=0; iModel<nModels; iModel++)
		Msg.AddInt16(_ModelCluster.m_lModels[iModel]);

	m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
}

void CWObject_ProjectileCluster::InitProjectileArray(int _StartSize)
{
	//
	m_lProjectileInstance.SetLen(_StartSize);

	// Allocate client data
	// Initialize all objects
	CNetMsg Msg_Init(NETMSG_PCLU_CREATECLIENTOBJECT);
	Msg_Init.AddInt8(_StartSize);
	Msg_Init.Addfp32(m_DistributionWidth);
	Msg_Init.Addfp32(m_DistributionHeight);
	Msg_Init.Addfp32(m_ProjectileVelocity);
	Msg_Init.Addfp32(m_ProjectileVelocityVariation);
	Msg_Init.AddInt8((int8)(m_RenderTimeout * m_pWServer->GetGameTickTime()));
	m_pWServer->NetMsg_SendToObject(Msg_Init, m_iObject);

	m_iFirstEmptyProjectile = 0;

#ifdef VERBOSE
	ConOutL(CStrF("InitProjectileArray: %i", m_iFirstEmptyProjectile));
#endif
}

int CWObject_ProjectileCluster::FindEmptyProjectileIndex()
{
	CWO_ProjectileInstance* pProjectile = m_lProjectileInstance.GetBasePtr();
	int ProjectileArraySize = m_lProjectileInstance.Len();
	int t = 0;

	if((m_iFirstEmptyProjectile > ProjectileArraySize)
	|| (m_iFirstEmptyProjectile < 0))
	{
#ifdef VERBOSE
		ConOutL(CStr("Hack apa"));
#endif
		m_iFirstEmptyProjectile = 0;	// Hack-apa
	}

	do
	{
#ifdef VERBOSE
		ConOut(CStrF("Looking for empty projectileinstance at %i", m_iFirstEmptyProjectile));
#endif
		if (!(pProjectile[m_iFirstEmptyProjectile].m_Flags & PROJECTILE_FLAGS_ALIVE))
			break;	// Found a spot here

		m_iFirstEmptyProjectile++;
		m_iFirstEmptyProjectile %= ProjectileArraySize;
		t++;
	} while(t<ProjectileArraySize);

	if (t == ProjectileArraySize)
	{
		// Didn't find an empty space.
		// Grow array
		int Add = (m_nProjectiles+m_LeightweightProjectiles.m_nProjectiles) * 2;
#ifdef VERBOSE
		ConOut(CStrF("Growing projectile array from %i to %i", ProjectileArraySize, ProjectileArraySize+Add));
#endif

		m_lProjectileInstance.SetLen(ProjectileArraySize + Add);
		m_iFirstEmptyProjectile = ProjectileArraySize;
		int i;
		for (i=0; i<Add; i++)
		{
			m_lProjectileInstance[ProjectileArraySize+i].m_Flags &= ~PROJECTILE_FLAGS_ALIVE;
		}

		CNetMsg Msg(NETMSG_PCLU_GROWPROJECTILEARRAY);
		Msg.AddInt16(Add);
		m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
	}

	// This instance isn't used
	return m_iFirstEmptyProjectile;
}

CWO_ProjectileInstance *CWObject_ProjectileCluster::SpawnProjectile(const CMat4Dfp32& _Mat, CWO_ProjectileInstance *_pRicochetFrom, const uint8 _Properties)
{
	int RandSeed;
	if(_pRicochetFrom)
		RandSeed = 0;
	else
		RandSeed = (m_RandSeedBase++ & 0x7fff);

	int SpawnTime = m_pWServer->GetGameTick();
	int iProjectile = FindEmptyProjectileIndex();
	if (iProjectile == -1)
		return NULL;

	CWO_ProjectileInstance* pProjectile = &m_lProjectileInstance[iProjectile];

	pProjectile->m_Properties = _Properties;
	pProjectile->m_Flags = PROJECTILE_FLAGS_ALIVE;
	if(_pRicochetFrom)
		pProjectile->m_Flags |= PROJECTILE_FLAGS_RICOCHET;
	pProjectile->m_RandSeed = RandSeed;
	pProjectile->m_RandSeedTime = SpawnTime;

	int iRandVel = SpawnTime + RandSeed;
    const fp32 ProjectileVelocity = (m_ProjectileVelocity + ((-1.0f + (2.0f * MFloat_GetRand(iRandVel))) * m_ProjectileVelocityVariation));

	pProjectile->m_PendingDeath = -1;
	pProjectile->m_TravelDistance = 0.0f;
	pProjectile->m_Position = _Mat;
	pProjectile->m_Velocity = CVec3Dfp32::GetRow(_Mat, 0) * ProjectileVelocity;
	if(m_DistributionWidth > 0 || m_DistributionHeight > 0)
	{
		pProjectile->m_Velocity += CVec3Dfp32::GetRow(_Mat, 1) * ProjectileVelocity * m_DistributionWidth * (MFloat_GetRand(pProjectile->m_RandSeed + 0) - 0.5f);
		pProjectile->m_Velocity += CVec3Dfp32::GetRow(_Mat, 2) * ProjectileVelocity * m_DistributionHeight * (MFloat_GetRand(pProjectile->m_RandSeed + 1) - 0.5f);
	}

	// Movement
/*	fp32 HitBoxWidth = ((MFloat_GetRand(pProjectile->m_RandSeed + 0)-0.5f) * m_DistributionWidth) * m_ProjectileVelocity;
	fp32 HitBoxDepth = ((MFloat_GetRand(pProjectile->m_RandSeed + 1)-0.5f) * m_DistributionWidth) * m_ProjectileVelocity;
	fp32 HitBoxHeight = ((MFloat_GetRand(pProjectile->m_RandSeed + 2)-0.5f) * m_DistributionHeight) * m_ProjectileVelocity;
	pProjectile->m_Velocity.k[0] += HitBoxWidth;
	pProjectile->m_Velocity.k[1] += HitBoxDepth;
	pProjectile->m_Velocity.k[2] += HitBoxHeight;*/

	// Spawn object on client tooooo
	CNetMsg Msg(NETMSG_PCLU_PROJECTILEINSTANCE_INIT);
	Msg.AddInt16(iProjectile);
	Msg.AddInt16(pProjectile->m_RandSeed);
	Msg.AddInt8(pProjectile->m_Properties);

	CQuatfp32 q; q.Create(_Mat);
	Msg.Addfp32(q.k[0]);
	Msg.Addfp32(q.k[1]);
	Msg.Addfp32(q.k[2]);
	Msg.Addfp32(q.k[3]);
	Msg.Addfp32(_Mat.k[3][0]);
	Msg.Addfp32(_Mat.k[3][1]);
	Msg.Addfp32(_Mat.k[3][2]);
	m_pWServer->NetMsg_SendToObject(Msg, m_iObject);

	if(m_lProjectileModelCluster.Len() > 0)
		NetMsgSendModelCluster(iProjectile, m_lProjectileModelCluster[MRTC_RAND() % m_lProjectileModelCluster.Len()], 0);
	
	return pProjectile;
}

void CWObject_ProjectileCluster::SpawnProjectileCluster(const CMat4Dfp32& _Mat)
{
	if (m_bFirstOnRefresh == true)
	{
		// First onrefresh hasn't been called yet so we cannot create projectiles just yet. Sod off!
		return;
	}

	uint16 SpawnGroup = m_SpawnGroup++;
	//M_TRACEALWAYS(CStrF("   Spawning ProjectileCluster group(%d) in object(%d)\n", SpawnGroup, m_iObject));

	m_lHitList.Clear();
	int i;
	bool bPlayLRP = true;
	for (i=0; i<m_nProjectiles; i++)
	{
		CWO_ProjectileInstance *pProjectile = SpawnProjectile(_Mat);
		if(pProjectile)
		{
			pProjectile->m_Properties = (uint8)~0;
			pProjectile->m_SpawnGroup = SpawnGroup;
			CWObject *pObj = m_pWServer->Object_Get(m_iOwner);
			if(!pObj || !(pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER) && m_LRPSound != 0)
			{
				CVec3Dfp32 Pos = CVec3Dfp32::GetRow(_Mat, 3);
				CVec3Dfp32 Vel = CVec3Dfp32::GetRow(_Mat, 0) * 32000.0f; // * pProjectile->m_Velocity;
				m_pWServer->Sound_At(Pos, m_LRPSound, WCLIENT_ATTENUATION_LRP, 0, 1.0f, -1, Vel);
			}

			bPlayLRP = false;
		}
	}

	// Spawn leightweight projectiles
	if(m_LeightweightProjectiles.m_nProjectiles > 0)
	{
		for(i = 0; i < m_LeightweightProjectiles.m_nProjectiles; i++)
		{
			CWO_ProjectileInstance* pProjectile = SpawnProjectile(_Mat,NULL,m_LeightweightProjectiles.m_Flags);
			if(pProjectile)
			{
				pProjectile->m_SpawnGroup = SpawnGroup;
				if(m_LeightweightProjectiles.m_Flags & PCLU_LW_LRPSOUND)
				{
					CWObject* pObj = m_pWServer->Object_Get(m_iOwner);
					if(!pObj || !(pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER) && m_LRPSound != 0)
					{
						CVec3Dfp32 Pos = _Mat.GetRow(3);
						CVec3Dfp32 Vel = _Mat.GetRow(0) * 32000.0f; // * pProjectile->m_Velocity;
						m_pWServer->Sound_At(Pos, m_LRPSound, WCLIENT_ATTENUATION_LRP, 0, 1.0f, -1, Vel);
					}
				}

				bPlayLRP = false;
			}
		}
	}
}

void CWObject_ProjectileCluster::TraceProjectile(int _iProjectile)
{
	CWO_ProjectileInstance* pProjectile = &m_lProjectileInstance[_iProjectile];

	CVec3Dfp32 Start = CVec3Dfp32::GetRow(pProjectile->m_Position, 3);
	CVec3Dfp32 Stop = Start + pProjectile->m_Velocity;
	CCollisionInfo CInfo;

	// Debug render where the projectile is at and where it's going
	CVec3Dfp32 Exp = Start + (pProjectile->m_Velocity * m_pWServer->GetGameTicksPerSecond());
	//m_pWServer->Debug_RenderWire(Start, Stop, 0xa0c0c0c0, 10.0f);
	//m_pWServer->Debug_RenderWire(Start, Exp, 0xff0000ff, 0.1f, false);

	CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
	CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_SURFACE | CXR_COLLISIONRETURNVALUE_LOCALPOSITION);

	int iRandVel = pProjectile->m_RandSeedTime + pProjectile->m_RandSeed;
	const fp32 ProjectileVelocity = (m_ProjectileVelocity + ((-1.0f + (2.0f * MFloat_GetRand(iRandVel))) * m_ProjectileVelocityVariation));
	if (TraceRay(_iProjectile, Start, Stop, &CInfo))
	{
		// Impact
		pProjectile->m_TravelDistance += (ProjectileVelocity * ( 1.0f / 100.0f )) * CInfo.m_Time;
		pProjectile->m_Position.GetRow(3) = CInfo.m_Pos;

//		CWObject* pHitObj = m_pWServer->Object_Get(CInfo.m_iObject);
//		if (pHitObj)
//		{
//			M_TRACEALWAYS(CStrF("      Projectile from group(%d) hit object(%d,%s)\n", pProjectile->m_SpawnGroup, pHitObj->m_iObject, pHitObj->GetName()));
//		}
//		else
//		{
//			M_TRACEALWAYS(CStrF("      Projectile from group(%d) hit object(%d,<Invalid Object>)\n", pProjectile->m_SpawnGroup, CInfo.m_iObject));
//		}
		
		bool bInvulnerable = false;
		//Check here if we hit a character that the projectiles should "bounce" off of
		if (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETFLAGS),CInfo.m_iObject) & PLAYER_FLAGS_PROJECTILEINVULNERABLE)
		{
			// Ok then, found invulnerable character, create object_object that has rb physics
			// that has same model as projectile?
			bInvulnerable = true;
			CMat4Dfp32 PosMat = GetPositionMatrix();
			CVec3Dfp32 Offset = CVec3Dfp32::GetMatrixRow(PosMat,3) - CInfo.m_Pos;
			Offset.Normalize();
			CVec3Dfp32::GetMatrixRow(PosMat,3) = CInfo.m_Pos + Offset * 20.0f;
			int32 iObject = m_pWServer->Object_Create(m_BounceSpawnObject,PosMat);
			m_pWServer->Object_AddVelocity(iObject,(Offset + CVec3Dfp32(0.0f,0.0f,1.0f)*5.0f));

		}
		if (CInfo.m_pSurface && CInfo.m_SurfaceType == 0 && CInfo.m_pSurface->GetBaseFrame())
			CInfo.m_SurfaceType = CInfo.m_pSurface->GetBaseFrame()->m_MaterialType;
		
		OnImpact(_iProjectile, CInfo.m_Pos, CInfo.m_iObject, &CInfo,bInvulnerable /*|| (CInfo.m_SurfaceType == 61)*/);
	}
	else
	{
		// Set new position
		pProjectile->m_Position.GetRow(3) = Stop;
		pProjectile->m_TravelDistance += ProjectileVelocity * ( 1.0f / 100.0f );
	}

	if (pProjectile->m_PendingDeath == -1)
	{
		// Make sure the projectile doesn't travel endlessly
		if (pProjectile->m_TravelDistance >= MAXTRAVELDISTANCE)
			pProjectile->m_PendingDeath = 0;	// Kill projectile
	}
}

void CWObject_ProjectileCluster::OnRefresh()
{
	if (m_bFirstOnRefresh)
	{
		int ProjectilePoolSize = m_nProjectiles;
		if (m_InitialProjectilePoolSize > ProjectilePoolSize)
			ProjectilePoolSize = m_InitialProjectilePoolSize;
		InitProjectileArray(ProjectilePoolSize);

		m_bFirstOnRefresh = false;

		SpawnProjectileCluster(GetPositionMatrix());
		m_pWServer->Object_SetDirty(m_iObject,-1);
	}

	bool bAnyoneAlive = false;
	int iProjectile;
	int nProjectiles = m_lProjectileInstance.Len();
	for (iProjectile=0; iProjectile<nProjectiles; iProjectile++)
	{
		CWO_ProjectileInstance* pProjectile = &m_lProjectileInstance[iProjectile];

		if (pProjectile->m_Flags & PROJECTILE_FLAGS_ALIVE)
		{
			bAnyoneAlive = true;

			if (pProjectile->m_PendingDeath == -1)
			{
				// This projectile is not pending death
				TraceProjectile(iProjectile);
				pProjectile->m_Velocity.k[2] -= m_ProjectileGravity;
			} else if (pProjectile->m_PendingDeath > 0)
			{
				// If this branch is followed we know that the projectile have
				// had an impact and is currently rendering its impact models
				pProjectile->m_PendingDeath--;
			} else {
				// At this branch we know that the projectile have had its
				// impact, the impact model have been rendered and it's
				// time to kill this projectile.
				pProjectile->m_Flags &= ~PROJECTILE_FLAGS_ALIVE;

				//M_TRACEALWAYS(CStrF("   Killing projectile in group (%d) in object (%d)\n", pProjectile->m_SpawnGroup, m_iObject));
			}
		}
	}

	if (!bAnyoneAlive)
	{
		// All projectiles are dead and this gameobject isn't usefull anymore
		if (m_bCanBeRemoved)
			Destroy();

		// Stop refreshing
		if (!(m_ClientFlags & CWO_CLIENTFLAGS_NOREFRESH))
			ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
	}

/*	if((m_PlayImpactSound != 0)
	&& (m_ImpactSoundDelay == 0))
	{
		int iImpact = m_ImpactSound;
		int iRicochet = m_RicochetSound;
		if (m_PlayImpactSound > 1)
		{
			iImpact = m_ImpactSoundMultiple;
			iRicochet = m_RicochetSoundMultiple;
		}

		m_AvarageImpactPos = m_AvarageImpactPos / fp32(m_PlayImpactSound);	// m_AvarageImpactPos contains the accumulated impact-position and m_PlayImpactSound is the number of impacts. This will give us the avarage impact position
		if(iImpact)
			m_pWServer->Sound_At(m_AvarageImpactPos, iImpact, 0, m_MaterialType);
		if (iRicochet)
			m_pWServer->Sound_At(m_AvarageImpactPos, iRicochet, 0, m_MaterialType);

		m_PlayImpactSound = 0;	// m_PlayImpactSound is increased for each projectile that impacts
		m_ImpactSoundDelay = 5;	// Ticks until next impact sound will be played
	}*/

	CWObject_Ext_Model::OnRefresh();
}

aint CWObject_ProjectileCluster::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
		case OBJMSG_SUMMON_SPAWNPROJECTILES:
			{
				m_Damage.m_DamageType |= _Msg.m_Param0;	// Damage delivery flags added
				const CMat4Dfp32* pMat = (CMat4Dfp32*)_Msg.m_pData;
				SpawnProjectileCluster(*pMat);
				if (m_ClientFlags & CWO_CLIENTFLAGS_NOREFRESH)
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
				return 1;
			}

		case OBJMSG_SUMMON_REMOVEPROJECTILES:
			{
				m_bCanBeRemoved = true;
				ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
				return 1;
			}
	}

	return CWObject_Damage::OnMessage(_Msg);
}

static int RandExclude(int _nRand, int _Exclude)
{
	if(_nRand <= 1)
		return 0;
	int Rand = MRTC_RAND() % (_nRand - 1);
	if(Rand >= _Exclude && _Exclude > 0)
		return Rand - 1;
	else
		return Rand;
};

void CWObject_ProjectileCluster::OnPassThroughImpact(CPClu_PTCollision& _PTCollision)
{
//	CWO_ProjectileInstance* pProjectile = &m_lProjectileInstance[_PTCollision.m_iProjectile];
//	const fp32 TravelDistance = pProjectile->m_TravelDistance + _PTCollision.m_Distance;
	
	// Send impact model-cluster to client
	_PTCollision.ResolveSurfaceMaterial(m_pWServer);
//	int iGroup = 0;
//	int iMaterial = 0;
//
//	if(_iGroup >= 0)
//		iGroup = _iGroup;
//	else
//	{
//		if(_pCInfo)
//		{
//			iMaterial = _pCInfo->m_SurfaceType;
//			if(iMaterial == 0 && _pCInfo->m_pSurface && _pCInfo->m_pSurface->GetBaseFrame())
//				iMaterial = _pCInfo->m_pSurface->GetBaseFrame()->m_MaterialType;
//			switch(iMaterial / 10)
//			{
//			case 0: iGroup = IMPACTMODEL_METAL; break;
//			case 1: iGroup = IMPACTMODEL_CONCRETE; break;
//			case 2: iGroup = IMPACTMODEL_WOOD; break;
//			case 3: iGroup = IMPACTMODEL_GROUND; break;
//			case 4: iGroup = IMPACTMODEL_PLASTIC; break;
//			case 5: iGroup = IMPACTMODEL_FLESH; break;
//			case 6: iGroup = IMPACTMODEL_METAL; break;
//			case 7: iGroup = IMPACTMODEL_CONCRETE; break;
//			case 8: iGroup = IMPACTMODEL_GROUND; break;
//			case 9: iGroup = IMPACTMODEL_GROUND; break;
//			case 10: iGroup = IMPACTMODEL_PLASTIC; break;
//			case 11: iGroup = IMPACTMODEL_GLASS; break;
//			}
//		}
//	}

//	if(_SurfaceName != "")
//	{
//		CXW_Surface* pSurface = m_pWServer->GetMapData()->GetResource_Surface(
//			m_pWServer->GetMapData()->GetResourceIndex_Surface(_SurfaceName));
//		if(pSurface && pSurface->GetBaseFrame())
//			iMaterial = pSurface->GetBaseFrame()->m_MaterialType;
//	}

	CWO_ModelCluster* pCluster = &m_ImpactModelCluster[_PTCollision.m_iGroup];
	NetMsgSendModelCluster(_PTCollision.m_iProjectile, *pCluster, 1);

	// Notify client
	CNetMsg Msg(NETMSG_PCLU_PROJECTILEINSTANCE_SETIMPACTTIME);
	Msg.AddInt16(_PTCollision.m_iProjectile);
	Msg.AddInt8(_PTCollision.m_iGroup);
	
	Msg.Addfp32(0.0f);	// TIME !!!
	Msg.Addfp32(_PTCollision.m_Direction[0]);
	Msg.Addfp32(_PTCollision.m_Direction[1]);
	Msg.Addfp32(_PTCollision.m_Direction[2]);
	
	Msg.Addfp32(_PTCollision.m_Position[0]);
	Msg.Addfp32(_PTCollision.m_Position[1]);
	Msg.Addfp32(_PTCollision.m_Position[2]);

	// No impact light
	Msg.AddInt32(0);
	Msg.AddInt16(0);

	// No wallmarks!
	Msg.AddInt8(0);
	//Msg.AddInt16(0);
	//Msg.AddInt8(0);

	// Send information to object
	m_pWServer->NetMsg_SendToObject(Msg, m_iObject);

	// Play impact sound to dude
	if(m_ImpactSoundTick < m_pWServer->GetGameTick() - 2)
 	{
 		m_ImpactSoundTick = m_pWServer->GetGameTick();

		if ((m_ImpactFleshSound != 0) && (_PTCollision.m_iGroup == IMPACTMODEL_FLESH))
			m_pWServer->Sound_At(_PTCollision.m_Position, m_ImpactFleshSound, WCLIENT_ATTENUATION_3D, _PTCollision.m_iMaterial);
		else if(m_ImpactSound)
			m_pWServer->Sound_At(_PTCollision.m_Position, m_ImpactSound, WCLIENT_ATTENUATION_3D, _PTCollision.m_iMaterial);

		if (m_RicochetSound)
			m_pWServer->Sound_At(_PTCollision.m_Position, m_RicochetSound, WCLIENT_ATTENUATION_3D, _PTCollision.m_iMaterial);
	}
}

bool CWObject_ProjectileCluster::IsValidHit(uint _iProjectile, uint& _nHeadHits)
{
	TAP_RCD<CWO_ProjectileInstance> lProjectileInstance = m_lProjectileInstance;

	uint16 SpawnGroup = lProjectileInstance[_iProjectile].m_SpawnGroup;
	int16 iObject = lProjectileInstance[_iProjectile].m_iObject;
	uint nHits = 0;
	_nHeadHits = 0;
	// Check against projectiles in same spawn group
	
	uint nProjectiles = lProjectileInstance.Len();
	for (uint iProjectile = 0; iProjectile < nProjectiles; iProjectile++)
	{
		// Check if this projectile accounts for a hit
		CWO_ProjectileInstance* pProjectile = &lProjectileInstance[iProjectile];
		if (_iProjectile == iProjectile || pProjectile->m_SpawnGroup != SpawnGroup ||
			pProjectile->m_PendingDeath < 0 || pProjectile->m_iObject != iObject || 
			!(pProjectile->m_Flags & PROJECTILE_FLAGS_ALIVE))
			continue;

		if (pProjectile->m_Flags & PROJECTILE_FLAGS_HEADSHOT)
			_nHeadHits++;

		nHits++;
	}

	// Check how many impacts is allowed at this range
	fp32 Range = lProjectileInstance[_iProjectile].m_TravelDistance;
	CWO_ProjectileCluster_DamageRange* pRange = m_pFirstDamageRange;
	while(pRange)
	{
		if (pRange->IsInRange(Range))
			break;
		pRange = pRange->m_pNextRange;
	}

	if (pRange && nHits < uint(pRange->m_nAllowedHits))
		return true;

	return false;
}

void CWObject_ProjectileCluster::OnImpact(int _iProjectile, const CVec3Dfp32& _Pos, uint16 _iObject, CCollisionInfo* _pCInfo, bool _bNoDamage)
{
	CWO_ProjectileInstance* pProjectile = &m_lProjectileInstance[_iProjectile];
	uint Properties = pProjectile->m_Properties;

	// Make sure this hit is valid first
	pProjectile->m_iObject = _iObject;
	uint nHeadHits = 0;
	if (!IsValidHit(_iProjectile, nHeadHits))
	{
		// Ignore damage and other spawning properties
		_bNoDamage = true;
		Properties = 0;

		//M_TRACEALWAYS("         - IGNORED!!\n");
	}
//	else
//	{
//		M_TRACEALWAYS("         - GOOD HIT!!\n");
//	}

	// Flag projectile as headshot if character was hit in head
#if ALLOW_CRITICAL_HEADSHOT
	CWObject_Message HeadMsg(OBJMSG_CHAR_GETHITLOCATION);
	HeadMsg.m_pData = (void*)_pCInfo;
	HeadMsg.m_DataSize = sizeof(CCollisionInfo);
	if (m_pWServer->Message_SendToObject(HeadMsg, _iObject) == CWObject_Character::SHitloc::HITLOC_HIGH)
	{
		pProjectile->m_Flags = pProjectile->m_Flags | PROJECTILE_FLAGS_HEADSHOT;

		if ((nHeadHits+1) >= 1)
		{
			HeadMsg.m_Msg = OBJMSG_CHAR_CRITICALHEADSHOT;
			m_pWServer->Message_SendToObject(HeadMsg, _iObject);
		}
	}
#endif

//	bool bRico = false;
	int DamageRet = DAMAGE_RETURN_NORMAL;
    if (!_bNoDamage && _iObject && _pCInfo && m_Damage.IsValid())
	{
		int iHit = m_lHitList.Find(_iObject);
		if((pProjectile->m_Properties & PCLU_LW_USEHITLIST && iHit == -1) || !(pProjectile->m_Properties & PCLU_LW_USEHITLIST))
		{
			if(iHit == -1)
				m_lHitList.Add(_iObject);

			CVec3Dfp32 Force = pProjectile->m_Velocity;
			if(pProjectile->m_Properties & PCLU_LW_DAMAGE)
			{
				m_Damage.m_Damage = ResolveDamageRange(pProjectile->m_TravelDistance);

				// Distribute damage to affected parties
				
				Force.k[0] /= 30.0f;
				Force.k[1] /= 30.0f;
				Force.k[2] /= 30.0f;

				// No criticals from shotguns and railguns
				if ((m_nProjectiles > 1)||(m_LeightweightProjectiles.m_nProjectiles > 1))
				{
					fp32 forceFraction = 1.0f / Max(m_nProjectiles,m_LeightweightProjectiles.m_nProjectiles);
					Force *= forceFraction;
					m_Damage.m_DamageType |= DAMAGEFLAG_NO_CRIT;
				}

				if (m_Shockwave.IsValid())
				{
					M_TRACEALWAYS("Sending shockwave!!\n");
					fp32 ShockwaveForce = (Force * m_Shockwave.m_Force).LengthSqr();
					CWO_ShockwaveMsg ShockwaveMsg(_pCInfo->m_Pos, m_Shockwave.m_ObjectRange, m_Shockwave.m_ObjectRange,
													ShockwaveForce, m_Shockwave.m_Damage.m_Damage, m_Shockwave.m_Damage.m_DamageType);
					ShockwaveMsg.Send(NULL, 0, m_iOwner, m_pWServer);
				}
			}
			else
			{
				m_Damage.m_Damage = 0;
				m_Damage.m_DamageType |= DAMAGEFLAG_NO_CRIT;
				Force.k[0] = Force.k[1] = Force.k[2] = 0.0f;
			}

			if (m_Damage.m_Damage > 0)
				DamageRet = m_Damage.SendExt(_iObject, m_iOwner, m_pWServer, _pCInfo, &Force, 0, 0);
		}
	}

	if (DamageRet == DAMAGE_RETURN_PASSTHROUGH)
		return;

	// Flag this projectile for death (after render timout passed)
	pProjectile->m_PendingDeath = m_RenderTimeout;

	// Fetch group
	int iGroup = GetGroup(_pCInfo);
	CWO_ModelCluster *pCluster = &m_ImpactModelCluster[iGroup];
	
	// Do we want to spawn an impact model ?
	if(Properties & PCLU_LW_IMPACTMODEL)
		NetMsgSendModelCluster(_iProjectile, *pCluster, 1);

	// Notify client
	NetMsgSendImpactTime(_iProjectile, iGroup, _pCInfo, _Pos, _iObject, Properties);

	// Do we want to spawn ricochet
	if (DamageRet == DAMAGE_RETURN_RICOCHET)
	{
		// Ricochet
		CMat4Dfp32 Mat;
		Mat.Unit();
		CVec3Dfp32 Fwd = (pProjectile->m_Velocity + CVec3Dfp32(Random, Random, Random) * 0.5).Normalize();
		CVec3Dfp32 Res, Project;
		Fwd.Project(_pCInfo->m_Plane.n, Project);
		(Fwd - Project + _pCInfo->m_Plane.n * 0.02f).SetMatrixRow(Mat, 0);
		_pCInfo->m_Plane.n.SetMatrixRow(Mat, 2);
		Mat.RecreateMatrix(0, 2);
		CVec3Dfp32::GetMatrixRow(Mat, 3) = _pCInfo->m_Pos + _pCInfo->m_Plane.n * 1;

		SpawnProjectile(Mat, pProjectile);	// Spawn ricochet

		// Array may have been reallocated
		pProjectile = &m_lProjectileInstance[_iProjectile];
	}

	int iMaterial = 0;
	if(_pCInfo)
	{
		iMaterial = _pCInfo->m_SurfaceType;
		if(iMaterial == 0 && _pCInfo->m_pSurface && _pCInfo->m_pSurface->GetBaseFrame())
			iMaterial = _pCInfo->m_pSurface->GetBaseFrame()->m_MaterialType;
	}

	// Play impact sound to dude
	if(m_ImpactSoundTick < m_pWServer->GetGameTick() - 2)
 	{
 		m_ImpactSoundTick = m_pWServer->GetGameTick();

		if(pProjectile->m_Properties & PCLU_LW_IMPACTSOUND)
		{
			if ((m_ImpactFleshSound != 0) && (iGroup == IMPACTMODEL_FLESH))
				m_pWServer->Sound_At(_Pos, m_ImpactFleshSound, WCLIENT_ATTENUATION_3D, iMaterial);
			else if(m_ImpactSound)
				m_pWServer->Sound_At(_Pos, m_ImpactSound, WCLIENT_ATTENUATION_3D, iMaterial);
		}

		if(pProjectile->m_Properties & PCLU_LW_RICOCHETSOUND)
		{
			if (m_RicochetSound)
				m_pWServer->Sound_At(_Pos, m_RicochetSound, WCLIENT_ATTENUATION_3D, iMaterial);
		}
	}

	// Add this position to the projectile bounding-box
	AddVisBoundBoxPos(_Pos);
/*
	// Impact on surface
	if (_pCInfo && _pCInfo->m_pSurface && _pCInfo->m_pSurface->GetBaseFrame())
	{
		int nProjectiles = 
		uint8 MaterialType = _pCInfo->m_pSurface->GetBaseFrame()->m_MaterialType;
		pProjectile->m_ImpactModelCluster = m_ImpactModelCluster[MaterialType];
		NetMsgSendModelCluster(iProjectile, pProjectile->m_ImpactModelCluster, 1);
	} else {
		pProjectile->m_ImpactModelCluster = m_ImpactModelCluster[IMPACTMODEL_DEFAULT];
		NetMsgSendModelCluster(iProjectile, pProjectile->m_ImpactModelCluster, 1);
	}
*/
}

uint CWObject_ProjectileCluster::GetGroup(CCollisionInfo* _pCInfo)
{
	if(_pCInfo)
	{
		int iMaterial = _pCInfo->m_SurfaceType;
		if(iMaterial == 0 && _pCInfo->m_pSurface && _pCInfo->m_pSurface->GetBaseFrame())
			iMaterial = _pCInfo->m_pSurface->GetBaseFrame()->m_MaterialType;

		uint8 iCategory = iMaterial / 10;
		uint8 iSubType =  iMaterial % 10;
		if (iCategory < e_NumMaterialCategories)
		{
			uint iGroup = s_lMaterialToImpactGroup[iCategory][iSubType];
			if (iGroup == 0)
				iGroup = s_lMaterialToImpactGroup[iCategory][0];
			
			return iGroup;
		}
	}

	return 0;
}

void CWObject_ProjectileCluster::NetMsgSendImpactTime(uint16 _iProjectile, uint8 _iGroup, const CCollisionInfo* _pCInfo, const CVec3Dfp32& _Pos, int16 _iObject, uint _Properties)
{
	CWO_ProjectileInstance* pProjectile = &m_lProjectileInstance[_iProjectile];

	CNetMsg Msg(NETMSG_PCLU_PROJECTILEINSTANCE_SETIMPACTTIME);
	Msg.AddInt16(_iProjectile);
	Msg.AddInt8(_iGroup);
	if (_pCInfo)
	{
		Msg.Addfp32(_pCInfo->m_Time);
		Msg.Addfp32(_pCInfo->m_Plane.n.k[0]);
		Msg.Addfp32(_pCInfo->m_Plane.n.k[1]);
		Msg.Addfp32(_pCInfo->m_Plane.n.k[2]);
	}
	else
	{
		Msg.Addfp32(0.0f);
		Msg.Addfp32(pProjectile->m_Velocity.k[0]);
		Msg.Addfp32(pProjectile->m_Velocity.k[1]);
		Msg.Addfp32(pProjectile->m_Velocity.k[2]);
	}

	Msg.Addfp32(_Pos.k[0]);
	Msg.Addfp32(_Pos.k[1]);
	Msg.Addfp32(_Pos.k[2]);

	Msg.AddInt32(m_ImpactLightDefaultColor);
	Msg.AddInt16(m_ImpactLightDefaultRange);

	Msg.AddInt16(_iObject);
	Msg.AddInt8(_pCInfo ? _pCInfo->m_LocalNode : 0);

	// Do we want to spawn a wallmark?
	int nWallmarks = (_Properties & PCLU_LW_IMPACTWALLMARK) ? m_ImpactModelCluster[_iGroup].m_lWallmarks.Len() : 0;
	if(nWallmarks > 0)
	{
		static int LastWallmark = -1;
		if(!m_ImpactModelCluster[_iGroup].m_iExtension)
		{
			// Message type
			Msg.AddInt8(1);

			// Data
			LastWallmark = RandExclude(nWallmarks, LastWallmark);
		}
		else
		{
			// Message type
			Msg.AddInt8(2);

			// Walk through wallmarks and determine which one to spawn by comparing distances
			const fp32 Distance = pProjectile->m_TravelDistance;
			CWO_ModelCluster::CWallmarkInfo* pWallmarks = m_ImpactModelCluster[_iGroup].m_lWallmarks.GetBasePtr();
			TThinArray<uint8> liWallmarks;
			liWallmarks.SetLen(nWallmarks);
			int niWallmarks = 0;
			for(int i = 0; i < nWallmarks; i++)
			{
				// Check if hitlocation is within range of impact
				if(pWallmarks[i].m_Distance.k[0] < 0 || pWallmarks[i].m_Distance.k[0] >= Distance)
				{
					if(pWallmarks[i].m_Distance.k[1] < 0 || pWallmarks[i].m_Distance.k[1] <= Distance)
						liWallmarks[niWallmarks++] = i;
				}
			}

			// If no distance tested wallmarks was found, randomize between all our wallmarks
			if(niWallmarks == 0)
				LastWallmark = RandExclude(nWallmarks, -1);
			else
			{
				// Otherwise we pick one of our chosen ones
				LastWallmark = liWallmarks[RandExclude(niWallmarks, LastWallmark)];
			}
		}

		// Add data
		Msg.AddInt16(m_ImpactModelCluster[_iGroup].m_lWallmarks[LastWallmark].m_iSurface);
		Msg.AddInt8(m_ImpactModelCluster[_iGroup].m_lWallmarks[LastWallmark].m_Size);
	}
	else
	{
		// Message type (invalid, or not wanted)
		Msg.AddInt8(0);
	}

	m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
}

void CWObject_ProjectileCluster::AddVisBoundBoxPos(const CVec3Dfp32& _Pos)
{
	// Add this position to the projectile bounding-box
	CVec3Dfp32 Min = GetAbsBoundBox()->m_Min;
	CVec3Dfp32 Max = GetAbsBoundBox()->m_Max;
	if (_Pos.k[0] < Min.k[0]) Min.k[0] = _Pos.k[0];
	if (_Pos.k[1] < Min.k[1]) Min.k[1] = _Pos.k[1];
	if (_Pos.k[2] < Min.k[2]) Min.k[2] = _Pos.k[2];
	if (_Pos.k[0] > Max.k[0]) Max.k[0] = _Pos.k[0];
	if (_Pos.k[1] > Max.k[1]) Max.k[1] = _Pos.k[1];
	if (_Pos.k[2] > Max.k[2]) Max.k[2] = _Pos.k[2];

	m_pWServer->Object_SetVisBox(m_iObject, Min, Max);
}

void CWObject_ProjectileCluster::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
/* RUNE LOAD/SAVE
	MAUTOSTRIP(CWObject_ProjectileCluster_OnDeltaLoad, MAUTOSTRIP_VOID);

	CWObject_Damage::OnDeltaLoad(_pFile);
*/

/*
	_pFile->ReadLE(m_RenderTimeout);
	int iProjectile;
	int nProjectiles = m_lProjectileInstance.Len();
	for (iProjectile=0; iProjectile<nProjectiles; iProjectile++)
		m_lProjectileInstance[iProjectile].Read(_pFile);
*/
/* RUNE LOAD/SAVE

	m_bFirstOnRefresh = false;

	// Transfer to client
*/
	Destroy();
}

void CWObject_ProjectileCluster::OnDeltaSave(CCFile* _pFile)
{
/* RUNE LOAD/SAVE
	MAUTOSTRIP(CWObject_ProjectileCluster_OnDeltaLoad, MAUTOSTRIP_VOID);
	CWObject_Damage::OnDeltaSave(_pFile);
*/

/*
	_pFile->WriteLE(m_RenderTimeout);
	int iProjectile;
	int nProjectiles = m_lProjectileInstance.Len();
	for (iProjectile=0; iProjectile<nProjectiles; iProjectile++)
		m_lProjectileInstance[iProjectile].Write(_pFile);
*/
}

bool CWObject_ProjectileCluster::TraceRay(const int& _iProjectile, CVec3Dfp32 _Pos1, CVec3Dfp32 _Pos2, CCollisionInfo* _pCInfo, int _CollisionObjects, bool _bExcludeOwner, bool _bDebug)
{
	int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
	int32 ObjectFlags = (_CollisionObjects != 0) ? _CollisionObjects: (OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL);
	int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
	int32 iExclude = _bExcludeOwner ? m_iOwner : 0;

	bool bCollision = m_pWServer->Phys_IntersectLine(_Pos1, _Pos2, OwnFlags, ObjectFlags, MediumFlags, _pCInfo, iExclude,true);

#ifndef M_RTM
	if (bCollision && _bDebug)
		m_pWServer->Debug_RenderWire(_Pos1, _pCInfo->m_Pos, 0xFF00FF00, 20.0f);
#endif

	return bCollision && _pCInfo->m_bIsValid;
}

int CWObject_ProjectileCluster::ResolveDamageRange(fp32 _Range)
{
	CWO_ProjectileCluster_DamageRange* pRange = m_pFirstDamageRange;
	while(pRange)
	{
		if (pRange->IsInRange(_Range))
			return pRange->m_Damage;

		pRange = pRange->m_pNextRange;
	}

	return m_Damage.m_Damage;
}

void CWObject_ProjectileCluster::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	switch(_Msg.m_MsgType)
	{
		case NETMSG_PCLU_CREATECLIENTOBJECT:	// This has to be called on an actual ProjectileCluster instance
			if (_pObj)
			{
				// Allocate data on client
				_pObj->m_lspClientObj[0] = NULL;
				_pObj->m_lspClientObj[0] = MNew(CWO_ProjectileCluster_Client);
				CWO_ProjectileCluster_Client* pCluCli = (CWO_ProjectileCluster_Client*)((CReferenceCount*)_pObj->m_lspClientObj[0]);
				if(!pCluCli)
					Error_static("OnClientNetMsg", "Out of memory");

				int MsgPos = 0;
				pCluCli->m_nProjectiles = _Msg.GetInt8(MsgPos);
				pCluCli->m_DistributionWidth = _Msg.Getfp32(MsgPos);
				pCluCli->m_DistributionHeight = _Msg.Getfp32(MsgPos);
				pCluCli->m_ProjectileVelocity = _Msg.Getfp32(MsgPos);
				pCluCli->m_ProjectileVelocityVariation = _Msg.Getfp32(MsgPos);
				pCluCli->m_RenderTimeout = (int)(_Msg.GetInt8(MsgPos) * _pWClient->GetGameTicksPerSecond());

				pCluCli->m_lProjectileInstance_Client.SetLen(pCluCli->m_nProjectiles);
/*
				// All projectiles should start at the player position (for now)
				int iProjectile;
				for (iProjectile=0; iProjectile<nProjectiles; iProjectile++)
				{
					CWO_ProjectileInstance_Client* pProjectile = &pCluCli->m_lProjectileInstance_Client[iProjectile];
					pProjectile->m_Position = _pObj->GetPositionMatrix();
				}
*/
			}
			break;

		case NETMSG_PCLU_GROWPROJECTILEARRAY:
			{
				int MsgPos = 0;
				int Add = _Msg.GetInt16(MsgPos);
				CWO_ProjectileCluster_Client* pCluCli = (CWO_ProjectileCluster_Client*)((CReferenceCount*)_pObj->m_lspClientObj[0]);
				if (pCluCli)
				{
					int CurLen = pCluCli->m_lProjectileInstance_Client.Len();
					pCluCli->m_lProjectileInstance_Client.SetLen(CurLen + Add);
				} else {
#ifdef VERBOSE
					ConOutL(CStr("NETMSG_PCLU_GROWPROJECTILEARRAY Error: No projectile cluster found"));
#endif
				}
/*
				int i;
				for (i=0; i<Add; i++)
					pCluCli->m_lProjectileInstance_Client[i].m_bAlive = false;
*/
			}
			break;

		case NETMSG_PCLU_PROJECTILEINSTANCE_INIT:
			if (_pObj && _pObj->m_lspClientObj[0])
			{
				int MsgPos = 0;
				int iProjectile;
				int RandSeed;
				uint8 Properties;
				CMat4Dfp32 Pos;

				// Extract data from message
				iProjectile = _Msg.GetInt16(MsgPos);
				RandSeed = _Msg.GetInt16(MsgPos);
				Properties = (uint8)_Msg.GetInt8(MsgPos);
				CQuatfp32 q;
				q.k[0] = _Msg.Getfp32(MsgPos);
				q.k[1] = _Msg.Getfp32(MsgPos);
				q.k[2] = _Msg.Getfp32(MsgPos);
				q.k[3] = _Msg.Getfp32(MsgPos);
				q.CreateMatrix(Pos);
				Pos.k[3][0] = _Msg.Getfp32(MsgPos);
				Pos.k[3][1] = _Msg.Getfp32(MsgPos);
				Pos.k[3][2] = _Msg.Getfp32(MsgPos);

				// Set data
				CWO_ProjectileCluster_Client* pCluCli = (CWO_ProjectileCluster_Client*)((CReferenceCount*)_pObj->m_lspClientObj[0]);
				if (pCluCli)
				{
					if(pCluCli->m_lProjectileInstance_Client.Len() <= iProjectile)
						break;
					CWO_ProjectileInstance_Client* pProjectile = &pCluCli->m_lProjectileInstance_Client[iProjectile];
					if (pProjectile)
					{
						pProjectile->m_RandSeed = RandSeed & 0x7fff;
						pProjectile->m_SpawnTick = _pWClient->GetGameTick();
						pProjectile->m_PreviousRenderTime = _pWClient->GetRenderTime();
						pProjectile->m_Position = Pos;
						pProjectile->m_SpawnPos = Pos;
						pProjectile->m_ImpactTimeFrac = 0.0f;
						pProjectile->m_ImpactState = IMPACTSTATE_IMPACTPENDING;
						pProjectile->m_RenderTicks = pCluCli->m_RenderTimeout;
						pProjectile->m_ImpactTick = 0;
						pProjectile->m_Properties = Properties;
						int iRandVel = pProjectile->m_RandSeed + pProjectile->m_SpawnTick;
						const fp32 ProjectileVelocity = (pCluCli->m_ProjectileVelocity + ((-1.0f + (2.0f * MFloat_GetRand(iRandVel))) * pCluCli->m_ProjectileVelocityVariation));
						fp32 HitBoxWidth = ((MFloat_GetRand(RandSeed++)-0.5f) * pCluCli->m_DistributionWidth) * ProjectileVelocity;
						fp32 HitBoxDepth = ((MFloat_GetRand(RandSeed++)-0.5f) * pCluCli->m_DistributionWidth) * ProjectileVelocity;
						fp32 HitBoxHeight = ((MFloat_GetRand(RandSeed++)-0.5f) * pCluCli->m_DistributionHeight) * ProjectileVelocity;

						pProjectile->m_Velocity = CVec3Dfp32::GetRow(Pos, 0) * ProjectileVelocity;
						pProjectile->m_Velocity.k[0] += HitBoxWidth;
						pProjectile->m_Velocity.k[1] += HitBoxDepth;
						pProjectile->m_Velocity.k[2] += HitBoxHeight;
					}
				} else {
#ifdef VERBOSE
					ConOutL(CStr("NETMSG_PCLU_PROJECTILEINSTANCE_INIT Error: No projectile cluster found"));
#endif
				}
			}
			break;

		case NETMSG_PCLU_PROJECTILEINSTANCE_SETIMPACTTIME:
			if (_pObj && _pObj->m_lspClientObj[0])
			{
				CWO_ProjectileCluster_Client* pCluCli = (CWO_ProjectileCluster_Client*)((CReferenceCount*)_pObj->m_lspClientObj[0]);

				if (pCluCli)
				{
					int MsgPos = 0;
					int iProjectile = _Msg.GetInt16(MsgPos);
					_Msg.GetInt8(MsgPos);	// int8 iGroup, unused
					if(pCluCli->m_lProjectileInstance_Client.Len() <= iProjectile)
						break;
					CWO_ProjectileInstance_Client* pProjectile = &pCluCli->m_lProjectileInstance_Client[iProjectile];

					pProjectile->m_ImpactTimeFrac = _Msg.Getfp32(MsgPos);

					pProjectile->m_ImpactNormal[0] = _Msg.Getfp32(MsgPos);
					pProjectile->m_ImpactNormal[1] = _Msg.Getfp32(MsgPos);
					pProjectile->m_ImpactNormal[2] = _Msg.Getfp32(MsgPos);

					pProjectile->m_ImpactPos = pProjectile->m_Position;
					pProjectile->m_ImpactPos.k[3][0] = _Msg.Getfp32(MsgPos);
					pProjectile->m_ImpactPos.k[3][1] = _Msg.Getfp32(MsgPos);
					pProjectile->m_ImpactPos.k[3][2] = _Msg.Getfp32(MsgPos);
					pProjectile->m_ImpactNormal.SetRow(pProjectile->m_ImpactPos, 0);
					pProjectile->m_ImpactPos.RecreateMatrix(0, 1);

					pProjectile->m_ImpactState = IMPACTSTATE_IMPACTTHISFRAME;
					pProjectile->m_ImpactTick = _pWClient->GetGameTick();

					pProjectile->m_ImpactLightColor = _Msg.GetInt32(MsgPos);
					pProjectile->m_ImpactLightRange = _Msg.GetInt16(MsgPos);

					int16 iObject = _Msg.GetInt16(MsgPos);
					uint8 iNode = _Msg.GetInt8(MsgPos);

					// New, handle diffrent type of wallmark data.
					int8 iWallmarkType = _Msg.GetInt8(MsgPos);
					int iWallmarkSurface = 0;
					int Size = 0;
					switch(iWallmarkType)
					{
						case 1:
							iWallmarkSurface = _Msg.GetInt16(MsgPos);
							Size = _Msg.GetInt8(MsgPos);
							break;

						case 2:
							iWallmarkSurface = _Msg.GetInt16(MsgPos);
							Size = _Msg.GetInt8(MsgPos);
							break;

						default:
							break;
					}

					//int iWallmarkSurface = _Msg.GetInt16(MsgPos);
					//int Size = _Msg.GetInt8(MsgPos);

					if(iWallmarkSurface)
					{
						CXR_WallmarkDesc WMD;
						WMD.m_SurfaceID = _pWClient->GetMapData()->GetResource_SurfaceID(iWallmarkSurface & 0x7fff);
						WMD.m_Size = Size;
						WMD.m_SpawnTime = _pWClient->GetRenderTime();
						WMD.m_GUID = iObject;
						WMD.m_iNode = iNode;

						CMat4Dfp32 Mat = pProjectile->m_ImpactPos;
						Swap(CVec3Dfp32::GetRow(Mat, 0), CVec3Dfp32::GetRow(Mat, 2));
						Swap(CVec3Dfp32::GetRow(Mat, 0), CVec3Dfp32::GetRow(Mat, 1));
						//CVec3Dfp32::GetRow(Mat, 0) = -CVec3Dfp32::GetRow(Mat, 0);
						if(iWallmarkSurface & 0x8000)
							Mat.RotZ_x_M(Random);

						CWObject_Message WallmarkMsg(OBJMSG_SPAWN_WALLMARK, (aint)&WMD, (aint)&Mat);
						_pWClient->ClientMessage_SendToObject(WallmarkMsg, iObject);
						//_pWClient->Debug_RenderMatrix(Mat, 20.0f, false);
					}
				} else {
#ifdef VERBOSE
					ConOutL(CStr("NETMSG_PCLU_PROJECTILEINSTANCE_SETIMPACTTIME Error: No projectile cluster found"));
#endif
				}
			}
			break;

		case NETMSG_PCLU_PROJECTILEINSTANCE_SENDMODELCLUSTER:
			if (_pObj && _pObj->m_lspClientObj[0])
			{
				int MsgPos = 0;
				int iProjectile;
				int Target;
				int nModels;

				// Extract data from message
				iProjectile = _Msg.GetInt16(MsgPos);
				Target = _Msg.GetInt8(MsgPos);
				nModels = _Msg.GetInt8(MsgPos);

				// Set data
				CWO_ProjectileCluster_Client* pCluCli = (CWO_ProjectileCluster_Client*)((CReferenceCount*)_pObj->m_lspClientObj[0]);
				if (pCluCli)
				{
					if(pCluCli->m_lProjectileInstance_Client.Len() <= iProjectile)
						break;
					CWO_ModelCluster* pModelCluster = NULL;
					switch(Target)
					{
						case 0:
							pModelCluster = &pCluCli->m_lProjectileInstance_Client[iProjectile].m_ProjectileModelCluster;
							break;

						case 1:
							pModelCluster = &pCluCli->m_lProjectileInstance_Client[iProjectile].m_ImpactModelCluster;
							break;
					}

					pModelCluster->m_lModels.SetLen(0);

					int iModel;
					for (iModel=0; iModel<nModels; iModel++)
						pModelCluster->AddModel(_Msg.GetInt16(MsgPos));
				} else {
#ifdef VERBOSE
					ConOutL(CStr("NETMSG_PCLU_PROJECTILEINSTANCE_SENDMODELCLUSTER Error: No projectile cluster found"));
#endif
				}
			}
			break;

/*		case NETMSG_PCLU_PROJECTILEINSTANCE_SETNEWVELOCITY:
			if (_pObj && _pObj->m_lspClientObj[0])
			{
				CWO_ProjectileCluster_Client* pCluCli = (CWO_ProjectileCluster_Client*)((CReferenceCount*)_pObj->m_lspClientObj[0]);

				if (pCluCli)
				{
					int MsgPos = 0;
					int iProjectile;
					CVec3Dfp32 Vel;

					// Extract data from message
					iProjectile = _Msg.GetInt16(MsgPos);
					Vel.k[0] = _Msg.Getfp32(MsgPos);
					Vel.k[1] = _Msg.Getfp32(MsgPos);
					Vel.k[2] = _Msg.Getfp32(MsgPos);

					// Set data
					if(pCluCli->m_lProjectileInstance_Client.Len() <= iProjectile)
						break;
					CWO_ProjectileInstance_Client* pProjectile = &pCluCli->m_lProjectileInstance_Client[iProjectile];
					pProjectile->m_Velocity = Vel;
				} else {
#ifdef VERBOSE
					ConOutL(CStr("NETMSG_PCLU_PROJECTILEINSTANCE_SETNEWVELOCITY Error: No projectile cluster found"));
#endif
				}
			}
			break;

		case NETMSG_PCLU_PROJECTILEINSTANCE_SETNEWDIRECTION:
			if (_pObj && _pObj->m_lspClientObj[0])
			{
				CWO_ProjectileCluster_Client* pCluCli = (CWO_ProjectileCluster_Client*)((CReferenceCount*)_pObj->m_lspClientObj[0]);

				if (pCluCli)
				{
					int MsgPos = 0;
					int iProjectile;
					CQuatfp32 Dir;

					// Extract data from message
					iProjectile = _Msg.GetInt16(MsgPos);
					Dir.k[0] = _Msg.Getfp32(MsgPos);
					Dir.k[1] = _Msg.Getfp32(MsgPos);
					Dir.k[2] = _Msg.Getfp32(MsgPos);
					Dir.k[3] = _Msg.Getfp32(MsgPos);

					// Set data
					if(pCluCli->m_lProjectileInstance_Client.Len() <= iProjectile)
						break;
					CWO_ProjectileInstance_Client* pProjectile = &pCluCli->m_lProjectileInstance_Client[iProjectile];
					Dir.CreateMatrix3x3(pProjectile->m_Position);
				} else {
#ifdef VERBOSE
					ConOutL(CStr("NETMSG_PCLU_PROJECTILEINSTANCE_SETNEWIDRECTION Error: No projectile cluster found"));
#endif
				}
			}
			break;*/
	}
}

void CWObject_ProjectileCluster::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT( CWObject_ProjectileCluster::OnClientRefresh );
	CWO_ProjectileCluster_Client* pCluCli = (CWO_ProjectileCluster_Client*)((CReferenceCount*)_pObj->m_lspClientObj[0]);
	if (pCluCli)
	{
//		int GameTick = _pWClient->GetGameTick();
		int iProjectile;
		int nProjectiles = pCluCli->m_lProjectileInstance_Client.Len();
		for (iProjectile=0; iProjectile<nProjectiles; iProjectile++)
		{
			CWO_ProjectileInstance_Client* pProjectile = &pCluCli->m_lProjectileInstance_Client[iProjectile];

			// Check for impact.
			if (pProjectile->m_ImpactState == IMPACTSTATE_IMPACTPENDING)
			{
//				if(pProjectile->m_SpawnTick != GameTick - 1)
					// Move projectile
					CVec3Dfp32::GetRow(pProjectile->m_Position, 3) += pProjectile->m_Velocity;

			}
			else if(pProjectile->m_ImpactState == IMPACTSTATE_IMPACTTHISFRAME)
			{
//				CVec3Dfp32::GetRow(pProjectile->m_Position, 3) += pProjectile->m_Velocity * pProjectile->m_ImpactTimeFrac;
				CVec3Dfp32::GetRow(pProjectile->m_Position, 3) = CVec3Dfp32::GetRow(pProjectile->m_ImpactPos, 3);
				pProjectile->m_ImpactState = IMPACTSTATE_IMPACTHASHAPPENED;
			}
			else
			{
				if (pProjectile->m_RenderTicks > 0)
					pProjectile->m_RenderTicks--;
			}
		}
	}
}

void CWObject_ProjectileCluster::OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, const CMat4Dfp32& _ParentMat)
{
	MSCOPESHORT(CWObject_ProjectileCluster::OnClientRenderVis);
	if(_pObj->m_iAnim0 == 1)
	{
		int NewestTick = 0;
		int NewestProjectile = -1;
		CWO_ProjectileCluster_Client* pCluCli = (CWO_ProjectileCluster_Client*)((CReferenceCount*)_pObj->m_lspClientObj[0]);
		if (pCluCli)
		{
			int nProjectiles = pCluCli->m_lProjectileInstance_Client.Len();
			for(int i = 0; i < nProjectiles; i++)
			{
				CWO_ProjectileInstance_Client* pProjectile = &pCluCli->m_lProjectileInstance_Client[i];
				if(pProjectile->m_ImpactTick > NewestTick)
				{
					NewestTick = pProjectile->m_ImpactTick;
					NewestProjectile = i;
				}
			}

			if(NewestProjectile != -1)
			{
				CWO_ProjectileInstance_Client* pProjectile = &pCluCli->m_lProjectileInstance_Client[NewestProjectile];
				if(pProjectile->m_RenderTicks > 0 && (pProjectile->m_ImpactState == IMPACTSTATE_IMPACTHASHAPPENED || pProjectile->m_ImpactState == IMPACTSTATE_IMPACTTHISFRAME))
				{
					CMat4Dfp32 PosMat, ImpactMat;
					bool bImpact = pProjectile->GetPositionMatrix(_pWClient->GetRenderTickFrac(), PosMat, ImpactMat);
					if(bImpact && pProjectile->m_Properties & PCLU_LW_IMPACTLIGHT)
					{
						fp32 Time = (fp32(_pWClient->GetGameTick() - pProjectile->m_ImpactTick) + _pWClient->GetRenderTickFrac() - pProjectile->m_ImpactTimeFrac) * _pWClient->GetGameTickTime();
						const fp32 Duration = 0.15f;
						if(Time < Duration)
						{
							const uint32& LightColor = pProjectile->m_ImpactLightColor;
							const CVec3Dfp32 Intensity((LightColor >> 16) & 0xff, (LightColor >> 8) & 0xff, (LightColor) & 0xff);
							ImpactMat.GetRow(3) += pProjectile->m_ImpactNormal * 12;
							CXR_Light Light(ImpactMat, Intensity * 0.005f * (1.0f - Time / Duration), pProjectile->m_ImpactLightRange, 0, CXR_LIGHTTYPE_POINT);
//							Light.m_LightGUID = MRTC_RAND() + 0x4000;	// wtf!?

							static int g_ProjectileGUID = 0;

							Light.m_LightGUID = 0x7000 + (g_ProjectileGUID & 0xff);
							g_ProjectileGUID++;

							Light.m_iLight = 0;
							Light.m_Flags = CXR_LIGHT_NOSHADOWS | CXR_LIGHT_NOSPECULAR;
							CXR_SceneGraphInstance* pSGI = _pWClient->World_GetSceneGraphInstance();
							if(pSGI)
								pSGI->SceneGraph_Light_LinkDynamic(Light);
						}
					}
				}
			}
		}
	}
}

void CWObject_ProjectileCluster::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MSCOPESHORT(CWObject_ProjectileCluster::OnClientRender);
	CWO_ProjectileCluster_Client* pCluCli = (CWO_ProjectileCluster_Client*)((CReferenceCount*)_pObj->m_lspClientObj[0]);
	if (pCluCli)
	{
		int nProjectiles = pCluCli->m_lProjectileInstance_Client.Len();
		for (int iProjectile=0; iProjectile<nProjectiles; iProjectile++)
		{
			CWO_ProjectileInstance_Client* pProjectile = &pCluCli->m_lProjectileInstance_Client[iProjectile];

			if (pProjectile->m_RenderTicks > 0)
			{
				// Get position interpolation time for projectile
//				fp32 PositionInterpolationTime = 0;

				CMat4Dfp32 PosMat, ImpactMat;
				bool bImpact = pProjectile->GetPositionMatrix(_pWClient->GetRenderTickFrac(), PosMat, ImpactMat);

				{
					CWO_ModelCluster* pModelCluster = &pProjectile->m_ProjectileModelCluster;

					int nModels = pModelCluster->m_lModels.Len();
					for (int iModel=0; iModel<nModels; iModel++)
					{
						// Add model to renderlist
						CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(pModelCluster->m_lModels[iModel]);
						if (pModel)
						{
							CXR_AnimState Anim;
							Anim.Clear();
							Anim.m_iObject = _pObj->m_iObject;
							Anim.m_AnimTime0 = _pWClient->GetRenderTime() - PHYSSTATE_TICKS_TO_TIME(pProjectile->m_SpawnTick, _pWClient);
							Anim.m_Anim0 = pProjectile->m_RandSeed & 0x7fff;
							Anim.m_AnimAttr0 = (CVec3Dfp32::GetRow(pProjectile->m_SpawnPos, 3) - CVec3Dfp32::GetRow(PosMat, 3)).LengthSqr();
							Anim.m_Data[0] = aint(&pProjectile->m_SpawnPos);
							Anim.m_Data[1] = bImpact;

							//CVec3Dfp32 Dir = pProjectile->m_Velocity;
							//Dir.Normalize();
							Anim.m_Data[2] = aint(&pProjectile->m_Velocity);

							_pEngine->Render_AddModel(pModel, PosMat, Anim);
						}
					}
				}

				// Impact? In that case, render impact model cluster instead of projectile model cluster
				if (bImpact && pProjectile->m_Properties & PCLU_LW_IMPACTMODEL)
				{
					CWO_ModelCluster *pModelCluster = &pProjectile->m_ImpactModelCluster;

					int nModels = pModelCluster->m_lModels.Len();
					for (int iModel=0; iModel<nModels; iModel++)
					{
						// Add model to renderlist
						CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(pModelCluster->m_lModels[iModel]);
						if (pModel)
						{
							CXR_AnimState Anim;
							Anim.Clear();
							Anim.m_iObject = _pObj->m_iObject;
							Anim.m_AnimTime0 = CMTime::CreateFromTicks(_pWClient->GetGameTick() - pProjectile->m_ImpactTick, _pWClient->GetGameTickTime(), _pWClient->GetRenderTickFrac() - pProjectile->m_ImpactTimeFrac);
							Anim.m_Anim0 = pProjectile->m_RandSeed & 0x7fff;
							Anim.m_AnimAttr0 = (CVec3Dfp32::GetRow(pProjectile->m_SpawnPos, 3) - CVec3Dfp32::GetRow(PosMat, 3)).LengthSqr();
							Anim.m_Data[0] = aint(&pProjectile->m_SpawnPos);
							Anim.m_Data[1] = bImpact;

							//CVec3Dfp32 Dir = pProjectile->m_Velocity;
							//Dir.Normalize();
							Anim.m_Data[2] = aint(&pProjectile->m_Velocity);

							_pEngine->Render_AddModel(pModel, ImpactMat, Anim);
						}
					}
				}
			}
		}
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ProjectileCluster, CWObject_Damage, 0x0100);



void CWObject_ProjectileCluster_ImpactSpawn::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();

	switch (_KeyHash)
	{
	case MHASH3('IMPA','CTSP','AWN'): // "IMPACTSPAWN"
		{
			m_ImpactSpawn = KeyValue;
			m_pWServer->GetMapData()->GetResourceIndex_Class(KeyValue); // Precache
			break;
		}
	default:
		{
			CWObject_ProjectileCluster::OnEvalKey(_KeyHash, _pKey);
		}
	}
}

void CWObject_ProjectileCluster_ImpactSpawn::OnImpact(int _iProjectile, const CVec3Dfp32& _Pos, uint16 _iObject, CCollisionInfo* _pCInfo, bool _bNoDamage)
{
	CWObject_ProjectileCluster::OnImpact(_iProjectile, _Pos, _iObject, _pCInfo, _bNoDamage);
	int32 iObject = m_pWServer->Object_Create(m_ImpactSpawn, _Pos);
}

void CWObject_ProjectileCluster_ImpactSpawn::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	CWObject_ProjectileCluster::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

	// Make sure impact spawn gets precached
	CRegistry* pReg = _pReg->Find("IMPACTSPAWN");
	if (pReg)
	{
		_pMapData->GetResourceIndex_Template(pReg->GetThisValue());
	}
}


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ProjectileCluster_ImpactSpawn, CWObject_ProjectileCluster, 0x0100);
