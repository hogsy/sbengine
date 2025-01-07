/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_Object.cpp

	Author:			Anton Ragnarsson

	Copyright:		2004 Starbreeze Studios AB

	Contents:		CWObject_Object implementation

	Comments:

	History:
		041110:		Created File
		050112:		Added simple script support (OnDamage / OnBreak).
		050608:		Added basic support for destructables + temp physics.
		050614:		Added support for rigid body dynamics-engine.
		060110:		Added support for playing animations.

\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_Object.h"
#include "WObj_ScenePoint.h"
#include "../../../../Shared/MOS/Classes/GameWorld/Server/WServer_Core.h"
#include "../../GameWorld/WServerMod.h"


#define DBG_OUT DO_IF(0) M_TRACE

enum
{
	class_Object_Lamp =			MHASH6('CWOb','ject','_','Obje','ct_','Lamp'),
	e_NameHash_World =			MHASH2('$wor','ld'),
};


static uint32 s_iNoname = 0; // used to assign temp-names to unnamed objects

/////
template<class T> void WriteArrayHeader(const TArray<T>& _Array, CCFile* _pF) 
{ 
	_pF->WriteLE(uint32(_Array.Len() * 1087));
}
template<class T> void ReadArrayHeader(TArray<T>& _Array, CCFile* _pF) 
{ 
	uint32 nElems; 
	_pF->ReadLE(nElems); 
	M_ASSERT((nElems % 1087) == 0, "Bad data in stream! (old savegame?)");
	_Array.QuickSetLen(0);
	_Array.SetLen(nElems / 1087);
}

template<class T> void WriteArray(const TArray<T>& _Array, CCFile* _pF)
{
	TAP<const T> pElems = _Array;
	WriteArrayHeader(_Array, _pF);
	for (uint i = 0; i < pElems.Len(); i++)
		pElems[i].Write(_pF);
}
template<class T> void ReadArray(TArray<T>& _Array, CCFile* _pF)
{
	ReadArrayHeader(_Array, _pF);
	TAP<T> pElems = _Array;
	for (uint i = 0; i < pElems.Len(); i++)
		pElems[i].Read(_pF);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMessageContainer
|
| Handles multiple CWO_SimpleMessages
|   (TODO: use/extend the CWO_SimpleMessageContainer?)
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CMessageContainer::Add(uint _iSlot, CStr _Msg, CWObject& _Obj)
{
	m_lMessages.SetMinLen(_iSlot + 1);
	Elem& msg = m_lMessages[_iSlot];

	msg.m_nHitpoints = 0;
	if (_Msg.CompareSubStr("0x") != 0)
		msg.m_nHitpoints = _Msg.GetStrSep(";").Val_int();

	msg.Parse(_Msg, _Obj.m_pWServer);
}


void CMessageContainer::Precache(CWObject& _Obj)
{
	m_lMessages.OptimizeMemory();
	foreach (const Elem, Msg, m_lMessages)
		Msg.SendPrecache(_Obj.m_iObject, _Obj.m_pWServer);
}


void CMessageContainer::Send(uint _iSender, CWObject& _Obj, uint _nHitpoints)
{
	TAP<Elem> pMessages = m_lMessages;
	for (int i = 0; i < pMessages.Len(); i++)
	{
		Elem& Msg = pMessages[i];
		if (_nHitpoints && Msg.m_nHitpoints > 0)
		{
			// Message with defined hitpoints, reduce amount and send if all hitpoints are out
			Msg.m_nHitpoints -= _nHitpoints;
			if (Msg.m_nHitpoints <= 0)
			{
				Msg.SendMessage(_Obj.m_iObject, _iSender, _Obj.m_pWServer);
				Msg.m_nHitpoints = -32768; // mark as sent (TODO: delete?)
			}
		}
		else if (Msg.m_nHitpoints == 0)
		{
			// Regular message, just send it
			Msg.SendMessage(_Obj.m_iObject, _iSender, _Obj.m_pWServer);
		}
	}
}


CMessageContainer& CMessageContainer::operator=(const CMessageContainer& _x)
{
	m_lMessages.Clear();
	m_lMessages.Add( _x.m_lMessages );
	return *this;
}


void CMessageContainer::Read(CCFile* _pFile)
{
	uint8 nElem;
	_pFile->ReadLE(nElem);
	m_lMessages.SetLen(nElem);
	foreach (Elem, Msg, m_lMessages)
	{
		_pFile->ReadLE(Msg.m_nHitpoints);
		Msg.OnDeltaLoad(_pFile);
	}
}


void CMessageContainer::Write(CCFile* _pFile) const
{
	uint8 nElem = m_lMessages.Len();
	_pFile->WriteLE(nElem);
	foreach (const Elem, Msg, m_lMessages)
	{
		_pFile->WriteLE(Msg.m_nHitpoints);
		Msg.OnDeltaSave(_pFile);
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_SoundGroupManager
|
| TODO: Move somewhere else
|__________________________________________________________________________________________________
\*************************************************************************************************/
const static char* s_aSoundGroupParamNames[CWO_SoundGroupManager::NUM_PARAMS] = 
	{ "impact_threshold_soft", "impact_threshold_medium", "impact_threshold_hard", "impact_volume_offset", "impact_volume_scale" };


void CWO_SoundGroupManager::InitWorld(CWorld_Server& _Server)
{
	m_lSoundsGroups.Clear();

	m_aDefaultParams[PARAM_THRESHOLD_SOFT] =	0.0f;
	m_aDefaultParams[PARAM_THRESHOLD_MEDIUM] =	5.0f;
	m_aDefaultParams[PARAM_THRESHOLD_HARD] =	15.0f;
	m_aDefaultParams[PARAM_VOLUME_OFFSET] =		0.1f;
	m_aDefaultParams[PARAM_VOLUME_SCALE] =		0.2f;

	spCRegistry spReg = _Server.Registry_GetServer()->Find("SOUNDTWEAK");
	if (spReg)
	{
		for (uint i = 0; i < NUM_PARAMS; i++)
			m_aDefaultParams[i] = spReg->GetValuef(s_aSoundGroupParamNames[i], m_aDefaultParams[i]);
	}
}

void CWO_SoundGroupManager::CloseWorld()
{
	m_lSoundsGroups.Clear();
}

int CWO_SoundGroupManager::GetSoundGroup(const char* _pName, CWorld_Server& _Server)
{
	// search existing
	TAP<CSoundGroup> pGroups = m_lSoundsGroups;
	for (uint i = 0; i < (uint)pGroups.Len(); i++)
		if (pGroups[i].m_Name == _pName)
			return i;

	// look in server registry
	spCRegistry spReg = _Server.Registry_GetServer()->Find("SOUNDGROUPS");
	if (spReg)
	{
		const CRegistry* pSoundGroupReg = spReg->Find(_pName);
		if (pSoundGroupReg)
		{
			CMapData& MapData = *_Server.GetMapData();
			CSoundGroup tmp;
			tmp.m_Name = _pName;

			// read sound definitions
			const static char* apKeys[NUM_SOUNDS] = { "impact_soft", "impact_medium", "impact_hard", "impact_projectile", "slide", "break", "destroy" };
			for (uint i = 0; i < NUM_SOUNDS; i++)
				tmp.m_aiSounds[i] = MapData.GetResourceIndex_Sound( pSoundGroupReg->GetValue(apKeys[i]) );

			// read impact thresholds
			for (uint i = 0; i < NUM_PARAMS; i++)
			{
				fp32 Default = m_aDefaultParams[i];
				tmp.m_aParams[i] = pSoundGroupReg->GetValuef(s_aSoundGroupParamNames[i], Default);
			}

			int iRet = m_lSoundsGroups.Len();
			m_lSoundsGroups.Add(tmp);
			return iRet;
		}
	}
	else
		LogFile("WARNING: No soundgroups found in server registry!");

	// not found
	return -1;
}

int16 CWO_SoundGroupManager::GetSound(uint _iSoundGroup, uint _iSlot) const
{
	if (m_lSoundsGroups.ValidPos(_iSoundGroup))
	{
		const CSoundGroup& g = m_lSoundsGroups[_iSoundGroup];
		return g.m_aiSounds[_iSlot];
	}
	return -1;
}

bool CWO_SoundGroupManager::GetImpactSound(uint _iSoundGroup, fp32 _Impact, uint* _piSlot, int16* _piSound, fp32* _pVolume) const
{
	M_ASSERT(SOUND_IMPACT_HARD == PARAM_THRESHOLD_HARD, "enums not in sync!");

	if (m_lSoundsGroups.ValidPos(_iSoundGroup))
	{
		const CSoundGroup& g = m_lSoundsGroups[_iSoundGroup];
		for (int iSlot = PARAM_THRESHOLD_HARD; iSlot >= PARAM_THRESHOLD_SOFT; iSlot--)
			if (_Impact > g.m_aParams[iSlot])
			{
				*_piSlot = iSlot;
				*_piSound = g.m_aiSounds[iSlot];
				*_pVolume = Clamp01(g.m_aParams[PARAM_VOLUME_OFFSET] + (g.m_aParams[PARAM_VOLUME_SCALE] * _Impact));
				return true;
			}
	}
	return false;
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object::SPhysPrim
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_Object::SPhysPrim::SPhysPrim()
	: m_BoundBox(0, 0)
//	, m_Origin(0)
	, m_VisibilityMask(~0)
	, m_iPhysModel(0)
	, m_iPhysModel_Dynamics(0)
{
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object::SDamage
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_Object::SDamage::SDamage()
	: m_nHitPoints(0)
	, m_nMinDamage(0)
	, m_nMaxDamage(-1)
	, m_nMinDamage_Last(0)
{
}


void CWObject_Object::SDamage::Parse(CStr _Str)
{
	m_nHitPoints =      _Str.GetStrSep(",").Val_int();
	m_nMinDamage =      _Str.GetStrSep(",").Val_int();
	m_nMaxDamage =      _Str.GetStrSep(",").Val_int();
	m_nMinDamage_Last = _Str.GetStrSep(",").Val_int();

	if (m_nHitPoints < 0)
	{
		m_nMinDamage = 32767; // unbreakable
		m_nHitPoints = 32767; // not broken
	}
}


// Returns true if the object received damage
bool CWObject_Object::SDamage::GiveDamage(int16 _nDamage)
{
	if (_nDamage < m_nMinDamage)
		return false; // not enough damage

	if ((m_nHitPoints <= _nDamage) && (_nDamage < m_nMinDamage_Last))
		return false; // not enough damage for final blow

	if (m_nMaxDamage != -1)
		_nDamage = MinMT(_nDamage, m_nMaxDamage);
	m_nHitPoints -= _nDamage;
	return true;
}


bool CWObject_Object::SDamage::IsDestroyed() const 
{
	return (m_nHitPoints <= 0); 
}


void CWObject_Object::SDamage::Write(CCFile* _pF) const
{
	_pF->WriteLE(m_nHitPoints);
	_pF->WriteLE(m_nMinDamage);
	_pF->WriteLE(m_nMaxDamage);
	_pF->WriteLE(m_nMinDamage_Last);
}


void CWObject_Object::SDamage::Read(CCFile* _pF)
{
	_pF->ReadLE(m_nHitPoints);
	_pF->ReadLE(m_nMinDamage);
	_pF->ReadLE(m_nMaxDamage);
	_pF->ReadLE(m_nMinDamage_Last);
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object::SBreakableBase
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_Object::SBreakableBase& CWObject_Object::SBreakableBase::operator=(const SBreakableBase& _x)
{
	m_DamageInfo = _x.m_DamageInfo;
	m_Messages_OnDamage = _x.m_Messages_OnDamage;
	m_Messages_OnBreak = _x.m_Messages_OnBreak;
	return *this;
}


bool CWObject_Object::SBreakableBase::OnEvalKey(uint32 _KeyHash, const CRegistry& _Key, CWObject& _Obj)
{
	CStr KeyName = _Key.GetThisName();
	CStr KeyValue = _Key.GetThisValue();

	switch (_KeyHash)
	{
	case MHASH4('DAMA','GE_','PAR','AMS'): // "DAMAGE_PARAMS"
		m_DamageInfo.Parse(KeyValue);
		break;

	default:
		{
			if (KeyName.CompareSubStr("MESSAGE_ONDAMAGE") == 0)
			{
				int iSlot = atoi(KeyName.Str() + 16);
				m_Messages_OnDamage.Add(iSlot, KeyValue, _Obj);
			}
			else if (KeyName.CompareSubStr("MESSAGE_ONBREAK") == 0)
			{
				int iSlot = atoi(KeyName.Str() + 15);
				m_Messages_OnBreak.Add(iSlot, KeyValue, _Obj);
			}
			else
				return false;
		}
		break;
	}

	return true;
}


void CWObject_Object::SBreakableBase::OnSpawnWorld(CWObject& _Obj)
{
	m_Messages_OnDamage.Precache(_Obj);
	m_Messages_OnBreak.Precache(_Obj);
}


void CWObject_Object::SBreakableBase::Write(CCFile* _pF) const
{
	m_DamageInfo.Write(_pF);
	m_Messages_OnDamage.Write(_pF);
	m_Messages_OnBreak.Write(_pF);
}


void CWObject_Object::SBreakableBase::Read(CCFile* _pF)
{
	m_DamageInfo.Read(_pF);
	m_Messages_OnDamage.Read(_pF);
	m_Messages_OnBreak.Read(_pF);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object::SPhysBase
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_Object::SPhysBase& CWObject_Object::SPhysBase::operator=(const SPhysBase& _x)
{
	SBreakableBase::operator=(_x);

	m_Name = _x.m_Name;
	m_PhysPrim = _x.m_PhysPrim;
	return *this;
}


bool CWObject_Object::SPhysBase::OnEvalKey(uint32 _KeyHash, const CRegistry& _Key, CWObject& _Obj)
{
	if (SBreakableBase::OnEvalKey(_KeyHash, _Key, _Obj))
		return true;

	CStr KeyName = _Key.GetThisName();

	switch (_KeyHash)
	{
	case MHASH4('VISI','BILI','TYMA','SK'): // "VISIBILITYMASK"
		m_PhysPrim.m_VisibilityMask = _Key.GetThisValuei();
		break;


	default:
		{
			if (KeyName.CompareSubStr("BSPMODELINDEX") == 0)
			{
				//DBG_OUT("%s: %s\n", KeyName.Str(), KeyValue.Str());
				CFStr28 ModelName;
				ModelName.CaptureFormated("$WORLD:%d", _Key.GetThisValuei());
				CMapData& MapData = *_Obj.m_pWServer->GetMapData();
				int iModel = MapData.GetResourceIndex_BSPModel(ModelName);

				int iSlot = atoi(KeyName.Str() + 13);
				if (iSlot == 0)
				{
					m_PhysPrim.m_iPhysModel = iModel;
				}
				else if (iSlot == 1)
				{
					m_PhysPrim.m_iPhysModel_Dynamics = iModel;
				}
			}
			else
				return false;
		}
		break;
	}

	return true;
}


void CWObject_Object::SPhysBase::OnFinishEvalKeys(CWObject& _Obj)
{
	if (!m_PhysPrim.m_iPhysModel_Dynamics) // no low-detail physmodel, use the regular one (TODO: remove this support?)
		m_PhysPrim.m_iPhysModel_Dynamics = m_PhysPrim.m_iPhysModel;

	if (m_PhysPrim.m_iPhysModel)
	{
		CXR_Model* pM = _Obj.m_pWServer->GetMapData()->GetResource_Model(m_PhysPrim.m_iPhysModel);
		if (pM)
		{
			pM->GetBound_Box(m_PhysPrim.m_BoundBox, m_PhysPrim.m_VisibilityMask);
//			m_PhysPrim.m_BoundBox.m_Min += m_PhysPrim.m_Origin;
//			m_PhysPrim.m_BoundBox.m_Max += m_PhysPrim.m_Origin;
			DBG_OUT(" - part/trigger: '%s'  boundbox: %s\n", m_Name.DbgName().Str(), m_PhysPrim.m_BoundBox.GetString().Str());

#ifndef M_RTM
			CVec3Dfp32 Size = m_PhysPrim.m_BoundBox.m_Max - m_PhysPrim.m_BoundBox.m_Min;
			if ((M_Fabs(Size.k[0]) > CWO_PHYSICSPRIM_MAXDIM_XY)
			 || (M_Fabs(Size.k[1]) > CWO_PHYSICSPRIM_MAXDIM_XY)
			 || (M_Fabs(Size.k[2]) > CWO_PHYSICSPRIM_MAXDIM_Z))
			{
				LogFile(CStrF("WARNING: Boundbox too large!  object: '%s' (%d), Size: %s", _Obj.GetName(), _Obj.m_iObject, Size.GetString().Str()));
				M_TRACE("WARNING: Boundbox too large!  object: '%s' (%d), Size: %s\n", _Obj.GetName(), _Obj.m_iObject, Size.GetString().Str());
			}
#endif
		}
	}
}






#define Box_IsNull(b) ((b).m_Min.AlmostEqual(CVec3Dfp32(0.0f), 0.001f) && (b).m_Max.AlmostEqual(CVec3Dfp32(0.0f), 0.001f))

void CWObject_Object::SPhysBase::Merge(const SPhysBase& _Other)
{
//TODO: This assert won't work until we get the new solid-based physmodel..
//	M_ASSERT(m_PhysPrim.m_iPhysModel == 0 || _Other.m_PhysPrim.m_iPhysModel == m_PhysPrim.m_iPhysModel, "Can't merge different physmodels!");

	m_PhysPrim.m_iPhysModel = _Other.m_PhysPrim.m_iPhysModel;
	m_PhysPrim.m_iPhysModel_Dynamics = _Other.m_PhysPrim.m_iPhysModel_Dynamics;
	m_PhysPrim.m_VisibilityMask |= _Other.m_PhysPrim.m_VisibilityMask;

	if (Box_IsNull(m_PhysPrim.m_BoundBox))
		m_PhysPrim.m_BoundBox = _Other.m_PhysPrim.m_BoundBox;
	else
		m_PhysPrim.m_BoundBox.Expand( _Other.m_PhysPrim.m_BoundBox );
}



void CWObject_Object::SPhysBase::Write(CCFile* _pF) const
{
	SBreakableBase::Write(_pF);

	m_Name.Write(_pF);
	m_PhysPrim.m_BoundBox.Write(_pF);
	m_PhysPrim.m_Origin.Write(_pF);
	_pF->WriteLE(m_PhysPrim.m_VisibilityMask);
	_pF->WriteLE(m_PhysPrim.m_iPhysModel);
	_pF->WriteLE(m_PhysPrim.m_iPhysModel_Dynamics);
}


void CWObject_Object::SPhysBase::Read(CCFile* _pF)
{
	SBreakableBase::Read(_pF);

	m_Name.Read(_pF);
	m_PhysPrim.m_BoundBox.Read(_pF);
	m_PhysPrim.m_Origin.Read(_pF);
	_pF->ReadLE(m_PhysPrim.m_VisibilityMask);
	_pF->ReadLE(m_PhysPrim.m_iPhysModel);
	_pF->ReadLE(m_PhysPrim.m_iPhysModel_Dynamics);
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object::SObjectModel
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_Object::SObjectModel::SObjectModel()
{
	m_iModel[0] = 0;
	m_iModel[1] = 0;
	m_iModel[2] = 0;
	m_LifeTime = 20;
	m_Flags = 0;
	m_LocalPos.Unit();
}


bool CWObject_Object::SObjectModel::OnEvalKey(uint32 _KeyHash, const CRegistry& _Key, CWObject& _Obj)
{
	CMapData& MapData = *_Obj.m_pWServer->GetMapData();

	CStr KeyName = _Key.GetThisName();
	CStr KeyValue;

	if (KeyName.CompareSubStr("MODEL") == 0)
	{
		uint8 i = atoi(KeyName.Str()+5);
		if (i < 3)
		{
			KeyValue = _Key.GetThisValue();
			m_iModel[i] = 0;
			if (KeyValue.Len())
			{
				m_iModel[i] = MapData.GetResourceIndex_Model(KeyValue);
				if (KeyValue.Len() > 246)
					ConOutL(CStrF("WARNING: very long model name for object '%s'", _Obj.GetName()));
			}
		}
	}
	else switch (_KeyHash)
	{
	case MHASH2('TEMP','LATE'): // "TEMPLATE"
		{
			KeyValue = _Key.GetThisValue();
			m_iModel[0] = MapData.GetResourceIndex_Template(KeyValue);
		}
		break;

	case MHASH2('FLAG','S'): // "FLAGS"
		{
			static const char* FlagsTranslate[] = { "Impact_Reflect", "Impact_WorldUp", "Template", "Attached", NULL };
			KeyValue = _Key.GetThisValue();
			m_Flags = KeyValue.TranslateFlags(FlagsTranslate);
		}
		break;

	case MHASH2('LIFE','TIME'): // "LIFETIME"
		{
			fp32 Seconds = _Key.GetThisValuef();
			m_LifeTime = int(Seconds * _Obj.m_pWServer->GetGameTicksPerSecond());
		}
		break;

	case MHASH2('ANGL','ES'): // "ANGLES"
		{
			CVec3Dfp32 Move = m_LocalPos.GetRow(3);
			CVec3Dfp32 Rot;
			_Key.GetThisValueaf(3, Rot.k);  // Rot.ParseString(KeyValue);
			Rot *= (1.0f/360.0f);
			Rot.CreateMatrixFromAngles(0, m_LocalPos);
			m_LocalPos.GetRow(3) = Move;
		}
		break;

	case MHASH2('ORIG','IN'): // "ORIGIN"
		{
			_Key.GetThisValueaf(3, m_LocalPos.GetRow(3).k);
		}
		break;

	default: 
		return false;
	}
	return true;
}


void CWObject_Object::SObjectModel::Relocate(const CMat4Dfp32& _ParentInv)
{
	CMat4Dfp32 Curr = m_LocalPos;
	Curr.Multiply(_ParentInv, m_LocalPos);
}


void CWObject_Object::SObjectModel::Write(CCFile* _pF) const
{
	m_LocalPos.Write(_pF);
	m_Name.Write(_pF);
	_pF->WriteLE(m_iModel, 3);
	uint16 x = m_LifeTime | (uint16(m_Flags) << 12);
	_pF->WriteLE(x);
}


void CWObject_Object::SObjectModel::Read(CCFile* _pF)
{
	m_LocalPos.Read(_pF);
	m_Name.Read(_pF);
	_pF->ReadLE(m_iModel, 3);
	uint16 x;
	_pF->ReadLE(x);
	m_LifeTime = x & 0xfff;
	m_Flags = x >> 12;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object::SConstraint
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_Object::SConstraint& CWObject_Object::SConstraint::operator=(const SConstraint& _x)
{
	SObjectModel::operator=(_x);
	SBreakableBase::operator=(_x);
	m_iObj0 = _x.m_iObj0;
	m_iObj1 = _x.m_iObj1;
	m_Type = _x.m_Type;
	m_Flags = _x.m_Flags;
	m_iPhysConstraint = _x.m_iPhysConstraint;
	m_Length = _x.m_Length;
	m_MinAngle = _x.m_MinAngle;
	m_MaxAngle = _x.m_MaxAngle;
	m_Friction = _x.m_Friction;
	m_StartPoint = _x.m_StartPoint;
	return *this;
}


void CWObject_Object::SConstraint::SetDefault()
{
	const fp32 QNan = TNumericProperties<fp32>::QuietNaN();
	m_iObj0 = CONSTRAINT_TARGET_NONE;
	m_iObj1 = CONSTRAINT_TARGET_NONE;
	m_Type = CONSTRAINT_TYPE_RIGID;
	m_Flags = 0;
	m_iPhysConstraint = -1;
	m_Length = 5.0f;
	m_MinAngle = QNan;
	m_MaxAngle = 0.25f; // 90 degrees
	m_Friction = QNan;
	m_StartPoint = CVec3Dfp16(QNan);
}


void CWObject_Object::SConstraint::SetDefault2()
{
	m_iObj0 = CONSTRAINT_TARGET_NONE;
	m_iObj1 = CONSTRAINT_TARGET_NONE;
	m_Type = CONSTRAINT_TYPE_RIGID;
	m_Flags = CONSTRAINT_FLAGS_ISVERSION2;
	m_iPhysConstraint = -1;
	m_Length = 5.0f;
	m_MinAngle = 0.125f; // 45 degrees
	m_MaxAngle = 0.125f;
	m_Friction = 0.0f;
	m_StartPoint = CVec3Dfp16(0.0f);
}


bool CWObject_Object::SConstraint::OnEvalKey(uint32 _KeyHash, const CRegistry& _Key, CWObject& _Obj)
{
	if (SObjectModel::OnEvalKey(_KeyHash, _Key, _Obj))
		return true;

	if (SBreakableBase::OnEvalKey(_KeyHash, _Key, _Obj))
		return true;

	static const char* lTypes[] = { "rigid", "ball", "axis", NULL };
	CVec3Dfp32 Tmp;

	//CStr KeyName = _Key.GetThisName();
	switch (_KeyHash)
	{
	case MHASH1('TYPE'): // "TYPE"
		m_Type = _Key.GetThisValue().TranslateInt(lTypes);
		break;

	case MHASH2('OBJE','CT0'): // "OBJECT0"
		m_iObj0 = _Key.GetThisValue().StrHash();		// Temp store as hash
		break;

	case MHASH2('OBJE','CT1'): // "OBJECT1"
		m_iObj1 = _Key.GetThisValue().StrHash();		// Temp store as hash
		break;

	case MHASH3('AXIS','_LEN','GTH'): // "AXIS_LENGTH"
		m_Length = _Key.GetThisValuef();
		break;

	case MHASH2('MINA','NGLE'): // "MINANGLE"
		m_MinAngle = _Key.GetThisValuef() * (1.0f / 360.0f);
		break;

	case MHASH2('MAXA','NGLE'): // "MAXANGLE"
		m_MaxAngle = _Key.GetThisValuef() * (1.0f / 360.0f);
		break;

	case MHASH2('FRIC','TION'): // "FRICTION"
		m_Friction = _Key.GetThisValuef();
		break;

	case MHASH3('STAR','TPOI','NT'): // "STARTPOINT"
		_Key.GetThisValueaf(3, Tmp.k);
		m_StartPoint = Tmp.Get<fp16>();
		break;

	default:
		return false;
	}
	return true;
}


void CWObject_Object::SConstraint::Write(CCFile* _pF) const
{
	SBreakableBase::Write(_pF);
	SObjectModel::Write(_pF);
	_pF->WriteLE(m_iObj0);
	_pF->WriteLE(m_iObj1);
	_pF->WriteLE(m_Type);
	_pF->WriteLE(m_Flags);
	_pF->WriteLE(m_iPhysConstraint);
	m_Length.Write(_pF);
	m_MinAngle.Write(_pF);
	m_MaxAngle.Write(_pF);
	m_Friction.Write(_pF);
	m_StartPoint.Write(_pF);
}


void CWObject_Object::SConstraint::Read(CCFile* _pF)
{
	SBreakableBase::Read(_pF);
	SObjectModel::Read(_pF);
	_pF->ReadLE(m_iObj0);
	_pF->ReadLE(m_iObj1);
	_pF->ReadLE(m_Type);
	_pF->ReadLE(m_Flags);
	_pF->ReadLE(m_iPhysConstraint);
	m_Length.Read(_pF);
	m_MinAngle.Read(_pF);
	m_MaxAngle.Read(_pF);
	m_Friction.Read(_pF);
	m_StartPoint.Read(_pF);
}


int32 CWObject_Object::SConstraint::HashToIndex(uint32 _NameHash, const SPart& _Owner, CWObject& _Object)
{
	if (_NameHash == e_NameHash_World)
		return CONSTRAINT_TARGET_WORLD;

	int iPart = _Owner.FindPart(_NameHash);
	if (iPart >= 0)
		return iPart;

	if (_NameHash == _Object.GetNameHash())
		return 0; // this is a bit of a hack. if having an object with no sub-parts, the object's name is treated as part 0  (instead of treating it as an "external" object)

	int iObject = _Object.m_pWServer->Selection_GetSingleTarget(_NameHash);
	if (iObject >= 3) // Q: why 3?  A: because -1 == 'unknown' and -2 == 'world'  (ugly, i know)
		return -iObject;

	return CONSTRAINT_TARGET_NONE;
}


uint32 CWObject_Object::SConstraint::IndexToHash(int32 _Index, const SPart& _Owner, CWObject& _Object)
{
	if (_Index == CONSTRAINT_TARGET_NONE)
		return 0;

	else if (_Index == CONSTRAINT_TARGET_WORLD)
		return e_NameHash_World;

	else if (_Index >= 0)
	{
		M_ASSERTHANDLER(_Owner.m_lSubParts.Len() || _Index == 0, "non-zero index but no sub-parts!?", return CONSTRAINT_TARGET_NONE);
		if (_Owner.m_lSubParts.Len() > 0)
			return _Owner.m_lSubParts[_Index].m_Name;
		else
			return _Object.GetNameHash();
	}

	CWObject* pObj = _Object.m_pWServer->Object_Get(-_Index);
	return pObj ? pObj->GetNameHash() : 0;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object::STrigger
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_Object::STrigger::STrigger()
{
	m_PhysPrim.m_VisibilityMask = M_Bit(15);
	m_DamageTypes = ~0;
}


bool CWObject_Object::STrigger::OnEvalKey(uint32 _KeyHash, const CRegistry& _Key, CWObject& _Obj)
{
	if (SPhysBase::OnEvalKey(_KeyHash, _Key, _Obj))
		return true;

	switch (_KeyHash)
	{
	case MHASH3('DAMA','GETY','PES'): // "DAMAGETYPES"
		m_DamageTypes = (uint32)_Key.GetThisValue().TranslateFlags( CRPG_Object::ms_DamageTypeStr );
		return true;
	}
	return false;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object::SSoundInfo
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CWObject_Object::SSoundInfo::IsEmpty() const
{
	return (m_iSoundGroup < 0);
}

void CWObject_Object::SSoundInfo::Write(CCFile* _pF) const
{
	_pF->WriteLE(m_iSoundGroup);
	_pF->WriteLE(m_anLastPlayed, NUM_SOUNDS);
}

void CWObject_Object::SSoundInfo::Read(CCFile* _pF)
{
	_pF->ReadLE(m_iSoundGroup);
	_pF->ReadLE(m_anLastPlayed, NUM_SOUNDS);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object::SPart
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_Object::SPart::SPart()
	: SPhysBase()
	, m_Flags(FLAGS_GAMEPHYSICS | FLAGS_BREAKCONSTRAINTS | FLAGS_NAVIGATION)
	, m_RenderMask(~0)
	, m_Lifetime(0)
	, m_Mass(0.0f)
{
	m_DamageInfo.m_nHitPoints = 10000;

	m_iModel[0] = 0;
	m_iModel[1] = 0;
	m_iModel[2] = 0;

	m_Sounds.m_iSoundGroup = -1;
	for (uint i = 0; i < SSoundInfo::NUM_SOUNDS; i++)
		m_Sounds.m_anLastPlayed[i] = 0;
}


CWObject_Object::SPart& CWObject_Object::SPart::operator=(const CWObject_Object::SPart& _Part)
{
	SPhysBase::operator=(_Part);

	m_LocalPos = _Part.m_LocalPos;
	m_Flags = _Part.m_Flags;
	m_RenderMask = _Part.m_RenderMask;
	m_iModel[0] = _Part.m_iModel[0];
	m_iModel[1] = _Part.m_iModel[1];
	m_iModel[2] = _Part.m_iModel[2];
	m_Mass = _Part.m_Mass;

	m_lConstraints.Clear();		m_lConstraints.Add(_Part.m_lConstraints);
	m_lTriggers.Clear();		m_lTriggers.Add(_Part.m_lTriggers);
	m_lSubParts.Clear();		m_lSubParts.Add(_Part.m_lSubParts);
	m_lObjectModels.Clear();	m_lObjectModels.Add(_Part.m_lObjectModels);
	m_lDestroyModels.Clear();	m_lDestroyModels.Add(_Part.m_lDestroyModels);
	m_lImpactModels.Clear();	m_lImpactModels.Add(_Part.m_lImpactModels);
	m_lAttach_DemonArm.Clear();	m_lAttach_DemonArm.Add(_Part.m_lAttach_DemonArm);
	m_lAttach_Grab.Clear();		m_lAttach_Grab.Add(_Part.m_lAttach_Grab);
	m_liJoints.Clear();			m_liJoints.Add(_Part.m_liJoints);

	m_Sounds = _Part.m_Sounds;
	m_Messages_OnMove = _Part.m_Messages_OnMove;
	m_Messages_OnDemonArmGrab = _Part.m_Messages_OnDemonArmGrab;
	m_Messages_OnDemonArmRelease = _Part.m_Messages_OnDemonArmRelease;

	return *this;
}


void CWObject_Object::SPart::Clear()
{
	m_iModel[0] = 0;
	m_iModel[1] = 0;
	m_iModel[2] = 0;
	m_lConstraints.Clear();
	m_lTriggers.Clear();
	m_lSubParts.Clear();
	m_lObjectModels.Clear();
	m_lDestroyModels.Clear();
	m_lImpactModels.Clear();
	m_lAttach_DemonArm.Clear();
	m_lAttach_Grab.Clear();
	m_liJoints.Clear();
}


bool CWObject_Object::SPart::OnEvalKey(uint32 _KeyHash, const CRegistry& _Key, CWObject_Object& _Obj)
{
	if (SPhysBase::OnEvalKey(_KeyHash, _Key, _Obj))
		return true;

	CMapData& MapData = *_Obj.m_pWServer->GetMapData();
	CStr KeyName = _Key.GetThisName();
	CStr KeyValue = _Key.GetThisValue();

	if (KeyName.CompareSubStr("MODEL") == 0)
	{
		uint8 i = atoi(KeyName.Str()+5);
		if (i < 3)
		{
			m_iModel[i] = 0;
			if (KeyValue.Len())
			{
				m_iModel[i] = MapData.GetResourceIndex_Model(KeyValue);

				// See if this is a variation model  (TODO: make this less ugly...)
				if (_Obj.m_VariationName.IsEmpty())
				{
					CFStr Variation = KeyValue;
					Variation.GetStrSep(":");
					Variation = Variation.GetStrSep(";");
					if (!Variation.IsEmpty())
						_Obj.m_VariationName = ":" + Variation;
				}
			}
		}
	}
	else if (KeyName.CompareSubStr("IMPACT_MODEL") == 0)
	{
		m_lImpactModels.Add( CNameHash(KeyValue) );
	}
	else if (KeyName.CompareSubStr("DESTROY_MODEL") == 0)
	{
		m_lDestroyModels.Add( CNameHash(KeyValue) );
	}
	else if (KeyName.CompareSubStr("ATTACH_DEMONARM") == 0)
	{
		m_lAttach_DemonArm.Add( CVec3Dfp32().ParseString(KeyValue) );
	}
	else if (KeyName.CompareSubStr("ATTACH_GRAB") == 0)
	{
		m_lAttach_Grab.Add( CVec3Dfp32().ParseString(KeyValue) );
	}
	else if (KeyName.CompareSubStr("MESSAGE_ONMOVE") == 0)
	{
		int iSlot = atoi(KeyName.Str() + 14);
		m_Messages_OnMove.Add(iSlot, KeyValue, _Obj);
	}
	else if (KeyName.CompareSubStr("MESSAGE_ONDEMONARMGRAB") == 0)
	{
		int iSlot = atoi(KeyName.Str() + 22);
		m_Messages_OnDemonArmGrab.Add(iSlot, KeyValue, _Obj);
	}
	else if (KeyName.CompareSubStr("MESSAGE_ONDEMONARMRELEASE") == 0)
	{
		int iSlot = atoi(KeyName.Str() + 25);
		m_Messages_OnDemonArmRelease.Add(iSlot, KeyValue, _Obj);
	}
	else if (_KeyHash == MHASH2('SUBC','LASS')) //"SUBCLASS"
	{
		KeyName = KeyValue;
		KeyValue = _Key.GetValue("TARGETNAME");
		uint nKeys = _Key.GetNumChildren();
		uint32 KeyHash = KeyName.StrHash();

		if (KeyValue.Len() == 0)
		{ // noname objects is a pain. let's assign some random name...
			KeyValue.CaptureFormated("__noname_%d", s_iNoname++);
		}

		switch (KeyHash)
		{
		case MHASH2('OBJE','CT'): // "OBJECT"
			{
			//	M_TRACE("Adding subpart '%s'\n", KeyValue.Str()); 
				uint iPart = m_lSubParts.Len();
				m_lSubParts.SetLen(iPart + 1);
				SPart& p = m_lSubParts[iPart];

				p.m_Name = KeyValue;
				for (uint i = 0; i < nKeys; i++)
					p.OnEvalKey(_Key.GetName(i).StrHash(), *_Key.GetChild(i), _Obj);
			}
			break;

		case MHASH5('OBJE','CT_C','ONST','RAIN','T'): // "OBJECT_CONSTRAINT"
		case MHASH5('OBJE','CT_C','ONST','RAIN','T2'): // "OBJECT_CONSTRAINT2"
			{
			//	M_TRACE("Adding constraint '%s'\n", KeyValue.Str());
				uint iConstraint = m_lConstraints.Len();
				m_lConstraints.SetLen(iConstraint + 1);

				SConstraint& c = m_lConstraints[iConstraint];
				if (KeyHash == MHASH5('OBJE','CT_C','ONST','RAIN','T2'))
					c.SetDefault2();
				else
					c.SetDefault();

				c.m_Name = KeyValue;
				c.m_iObj0 = e_NameHash_World;
				c.m_iObj1 = e_NameHash_World;

				for (uint i = 0; i < nKeys; i++)
					c.OnEvalKey(_Key.GetName(i).StrHash(), *_Key.GetChild(i), _Obj);
			}
			break;

		case MHASH4('OBJE','CT_','TRIG','GER'): // "OBJECT_TRIGGER"
			{
				uint iTrigger = m_lTriggers.Len();
				m_lTriggers.SetLen(iTrigger + 1);
				STrigger& t = m_lTriggers[iTrigger];

				DBG_OUT("Adding trigger '%s'\n", KeyValue.Str());
				t.m_Name = KeyValue;
				for (uint i = 0; i < nKeys; i++)
					t.OnEvalKey(_Key.GetName(i).StrHash(), *_Key.GetChild(i), _Obj);
			}
			break;

		case MHASH4('OBJE','CT_','MOD','EL'): // "OBJECT_MODEL"
			{
				uint iSlot = m_lObjectModels.Len();
				m_lObjectModels.SetLen(iSlot + 1);
				SObjectModel& m = m_lObjectModels[iSlot];

			//	M_TRACE("Adding model '%s'\n", KeyValue.Str());
				m.m_Name = KeyValue;

				for (uint i = 0; i < nKeys; i++)
					m.OnEvalKey(_Key.GetName(i).StrHash(), *_Key.GetChild(i), _Obj);
			}
			break;

		default:
			ConOutL(CStrF("WARNING: Unknown object subclass '%s'", KeyName.Str()));
		}
	}
	else switch (_KeyHash)
	{
	case MHASH2('FLAG','S'): // "FLAGS"
		{
			static const char *FlagsTranslate[] =
			{
				"nophysics", "animphys", "waitspawn", "gamephysics", "autobreakconstraints", "spawnactive", "debris", "navigation", "trimeshcollision", "lamp", "modelshadow", "autoaim" , NULL 
			};
			m_Flags = KeyValue.TranslateFlags(FlagsTranslate);
		}
		break;

	case MHASH3('SOUN','DGRO','UP'): // "SOUNDGROUP"
		{
			CWObject_Message Msg(OBJMSG_GAME_GETSOUNDGROUPMANAGER);
			CWO_SoundGroupManager* pMan = (CWO_SoundGroupManager*)
					_Obj.m_pWServer->Message_SendToObject(Msg, _Obj.m_pWServer->Game_GetObjectIndex());
			if (pMan)
				m_Sounds.m_iSoundGroup = pMan->GetSoundGroup(KeyValue, *_Obj.m_pWServer);
		}
		break;

	case MHASH1('MASS'): // "MASS"
		m_Mass = _Key.GetThisValuef();
		break;

	case MHASH2('LIFE','TIME'): // "LIFETIME"
		m_Lifetime = (uint32)(KeyValue.Val_fp64() * _Obj.m_pWServer->GetGameTicksPerSecond());
		m_Flags |= FLAGS_SAVELIFETIME;
		break;

	case MHASH2('JOIN','T'): // "JOINT"
		{
			int iJoint = _Key.GetThisValue().GetStrSep(",").Val_int();
			m_liJoints.Add(iJoint);
		}
		break;

	default:
		return false;
	}

	m_Flags |= FLAGS_FROMTEMPLATE;
	return true;
}


void CWObject_Object::SPart::OnFinishEvalKeys(CWObject& _Obj)
{
	SPhysBase::OnFinishEvalKeys(_Obj);

	CMat4Dfp32 InvObjMat;
	_Obj.GetPositionMatrix().InverseOrthogonal(InvObjMat);

	if (m_PhysPrim.m_iPhysModel)
	{
		if (!m_iModel[0]) // no visual model, use bsp model for rendering
			m_iModel[0] = m_PhysPrim.m_iPhysModel;
	}
	else
	{
		int iPhysModel = m_iModel[0]; // no phys model, use visual model to get boundbox
		CXR_Model* pM = _Obj.m_pWServer->GetMapData()->GetResource_Model(iPhysModel);
		if (pM)
			pM->GetBound_Box(m_PhysPrim.m_BoundBox);
	}

	foreach (STrigger, t, m_lTriggers)
	{
		// If trigger have no physmodel, use the current one
		if (t.m_PhysPrim.m_iPhysModel == 0)
			t.m_PhysPrim.m_iPhysModel = m_PhysPrim.m_iPhysModel;

		if (t.m_PhysPrim.m_iPhysModel_Dynamics == 0)
			t.m_PhysPrim.m_iPhysModel_Dynamics = m_PhysPrim.m_iPhysModel_Dynamics;

		t.OnFinishEvalKeys(_Obj);
	}

	foreach (SPart, child, m_lSubParts)
	{
		// If subpart have no physmodel, use the current one
		if (child.m_PhysPrim.m_iPhysModel == 0)
			child.m_PhysPrim.m_iPhysModel = m_PhysPrim.m_iPhysModel;

		if (child.m_PhysPrim.m_iPhysModel_Dynamics == 0)
			child.m_PhysPrim.m_iPhysModel_Dynamics = m_PhysPrim.m_iPhysModel_Dynamics;

		// If subpart have no visual model, use the current one
		if (child.m_iModel[0] == 0)
			child.m_iModel[0] = m_iModel[0];

		// If subpart have no sounds defined, use current one
		if (child.m_Sounds.IsEmpty())
			child.m_Sounds = m_Sounds;

		child.OnFinishEvalKeys(_Obj);
	}

	if (m_Flags & FLAGS_FROMTEMPLATE)
	{
		// Set rendermask
		m_RenderMask = m_PhysPrim.m_VisibilityMask;

		foreach (SConstraint, c, m_lConstraints)
		{
			c.m_iObj0 = SConstraint::HashToIndex(c.m_iObj0, *this, _Obj);
			c.m_iObj1 = SConstraint::HashToIndex(c.m_iObj1, *this, _Obj);
			c.Relocate(InvObjMat);
		}

		foreach (SObjectModel, m, m_lObjectModels)
			m.Relocate(InvObjMat);

		foreach (CVec3Dfp32, v, m_lAttach_DemonArm)
			v *= InvObjMat;

		foreach (CVec3Dfp32, v, m_lAttach_Grab)
			v *= InvObjMat;

		foreach (SPart, child, m_lSubParts)
		{
			// Copy triggers from subparts. 
			m_lTriggers.Add( child.m_lTriggers );

			// Include subpart occlusion mask in this  (will not affect rendermask)
			m_PhysPrim.m_VisibilityMask |= child.m_PhysPrim.m_VisibilityMask;
		}

		m_Flags &= ~FLAGS_FROMTEMPLATE;
	}

	m_lConstraints.OptimizeMemory();
	m_lTriggers.OptimizeMemory();
	m_lSubParts.OptimizeMemory();
	m_lObjectModels.OptimizeMemory();
	m_lImpactModels.OptimizeMemory();
	m_lDestroyModels.OptimizeMemory();
	m_lAttach_DemonArm.OptimizeMemory();
	m_lAttach_Grab.OptimizeMemory();
	m_liJoints.OptimizeMemory();
}


void CWObject_Object::SPart::OnSpawnWorld(CWObject& _Obj)
{
	SPhysBase::OnSpawnWorld(_Obj);

	foreach (SConstraint, c, m_lConstraints)
		c.OnSpawnWorld(_Obj);

	foreach (STrigger, t, m_lTriggers)
		t.OnSpawnWorld(_Obj);

	foreach (SPart, p, m_lSubParts)
		p.OnSpawnWorld(_Obj);

	m_Messages_OnMove.Precache(_Obj);
	m_Messages_OnDemonArmGrab.Precache(_Obj);
	m_Messages_OnDemonArmRelease.Precache(_Obj);
}


void CWObject_Object::SPart::Write(CCFile* _pF) const
{
	SPhysBase::Write(_pF);

	m_LocalPos.Write(_pF);
	_pF->WriteLE(m_Flags);
	_pF->WriteLE(m_RenderMask);
	_pF->WriteLE(m_Lifetime);
	_pF->WriteLE(m_iModel, 3);
	m_Mass.Write(_pF);

	WriteArray(m_lConstraints, _pF);
	WriteArray(m_lTriggers, _pF);
	WriteArray(m_lObjectModels, _pF);
	WriteArray(m_lImpactModels, _pF);
	WriteArray(m_lDestroyModels, _pF);
	WriteArray(m_lAttach_DemonArm, _pF);
	WriteArray(m_lAttach_Grab, _pF);

	WriteArrayHeader(m_liJoints, _pF);
	_pF->WriteLE(m_liJoints.GetBasePtr(), m_liJoints.Len());

	m_Sounds.Write(_pF);
	m_Messages_OnMove.Write(_pF);
	m_Messages_OnDemonArmGrab.Write(_pF);
	m_Messages_OnDemonArmRelease.Write(_pF);

	// Write sub-parts
	WriteArrayHeader(m_lSubParts, _pF);
	foreach (const SPart, s, m_lSubParts)
		s.Write(_pF);
}


void CWObject_Object::SPart::Read(CCFile* _pF, CWObject_Object& _Obj)
{
	SPhysBase::Read(_pF);

	m_LocalPos.Read(_pF);
	_pF->ReadLE(m_Flags);
	_pF->ReadLE(m_RenderMask);
	_pF->ReadLE(m_Lifetime);
	_pF->ReadLE(m_iModel, 3);
	m_Mass.Read(_pF);

	// Delete all existing phys constraints before reading
	foreach (SConstraint, c, m_lConstraints)
		if (c.m_iPhysConstraint >= 0)
		{
			_Obj.DeletePhysConstraint(c.m_iPhysConstraint);
			c.m_iPhysConstraint = -1;
		}

	ReadArray(m_lConstraints, _pF);
	ReadArray(m_lTriggers, _pF);
	ReadArray(m_lObjectModels, _pF);
	ReadArray(m_lImpactModels, _pF);
	ReadArray(m_lDestroyModels, _pF);
	ReadArray(m_lAttach_DemonArm, _pF);
	ReadArray(m_lAttach_Grab, _pF);

	ReadArrayHeader(m_liJoints, _pF);
	_pF->ReadLE(m_liJoints.GetBasePtr(), m_liJoints.Len());

	m_Sounds.Read(_pF);
	m_Messages_OnMove.Read(_pF);
	m_Messages_OnDemonArmGrab.Read(_pF);
	m_Messages_OnDemonArmRelease.Read(_pF);

	// Read sub-parts
	ReadArrayHeader(m_lSubParts, _pF);
	foreach (SPart, s, m_lSubParts)
		s.Read(_pF, _Obj);
}


void CWObject_Object::SPart::Merge(const SPart& _Other)
{
	SPhysBase::Merge(_Other);

	if ((m_iModel[0] != _Other.m_iModel[0]) && (m_iModel[0] != 0))
		ConOutL(CStrF("ERROR: Can't merge different models! (%s, %s)", m_Name.DbgName().Str(), _Other.m_Name.DbgName().Str()));

	for (uint i = 0; i < 3; i++)
		if (!m_iModel[i])
			m_iModel[i] = _Other.m_iModel[i];

	if (m_Sounds.IsEmpty() && !_Other.m_Sounds.IsEmpty())
		m_Sounds = _Other.m_Sounds;

	m_RenderMask |= _Other.m_RenderMask;

	// Add attachpoints
	m_lAttach_DemonArm.Add(_Other.m_lAttach_DemonArm);
	m_lAttach_Grab.Add(_Other.m_lAttach_Grab);

	// Add triggers
	m_lTriggers.Add(_Other.m_lTriggers);

	// If some part is stationary, the whole object is stationary..
	if (!(_Other.m_Flags & FLAGS_GAMEPHYSICS))
		m_Flags &= ~FLAGS_GAMEPHYSICS;

	// If some part is 'lamp', the whole object is lamp
	if (_Other.m_Flags & FLAGS_LAMP)
		m_Flags = ((m_Flags | FLAGS_LAMP) & ~FLAGS_MODELSHADOW) | (_Other.m_Flags & FLAGS_MODELSHADOW);

	// Add mass
	m_Mass.Set(m_Mass.Getfp32() + _Other.m_Mass.Getfp32());

	// Use longest lifetime
	m_Lifetime = Max(m_Lifetime, _Other.m_Lifetime);
}


int CWObject_Object::SPart::FindConstraint(uint32 _NameHash) const
{
	foreach (const SConstraint, c, m_lConstraints)
		if (c.m_Name == _NameHash)
			return (&c - m_lConstraints.GetBasePtr());
	return -1;
}


int CWObject_Object::SPart::FindTrigger(uint32 _NameHash) const
{
	foreach (const STrigger, t, m_lTriggers)
		if (t.m_Name == _NameHash)
			return (&t - m_lTriggers.GetBasePtr());
	return -1;
}


int CWObject_Object::SPart::FindPart(uint32 _NameHash) const
{
	foreach (const SPart, p, m_lSubParts)
		if (p.m_Name == _NameHash)
			return (&p - m_lSubParts.GetBasePtr());
	return -1;
}


const CWObject_Object::SObjectModel* CWObject_Object::SPart::FindModel(uint32 _NameHash, const char* _pVariationName) const
{
	TAP<const SObjectModel> pModels = m_lObjectModels;

	// First, look for models with matching variation name
	if (_pVariationName)
	{
		uint32 Hash2 = StringToHash(_pVariationName, _NameHash);	// create a stringhash for "name:variation"
		for (uint i = 0; i < pModels.Len(); i++)
		{
			if (pModels[i].m_Name == Hash2)
				return &pModels[i];
		}
	}

	// Next, look for models with matching base name
	for (uint i = 0; i < pModels.Len(); i++)
	{
		if (pModels[i].m_Name == _NameHash)
			return &pModels[i];
	}
	return NULL;
}


void CWObject_Object::SPart::DeleteConstraint(int _iConstraint)
{
	int iLast = m_lConstraints.Len() - 1;
	m_lConstraints[_iConstraint] = m_lConstraints[iLast];
	m_lConstraints.Del(iLast);
}


void CWObject_Object::SPart::DeleteTrigger(int _iTrigger)
{
	int iLast = m_lTriggers.Len() - 1;
	m_lTriggers[_iTrigger] = m_lTriggers[iLast];
	m_lTriggers.Del(iLast);
}


void CWObject_Object::SPart::DeletePart(int _iPart, CWObject_Object& _Obj)
{
	int iLast = m_lSubParts.Len() - 1;
	m_lSubParts[_iPart] = m_lSubParts[iLast];
	m_lSubParts.Del(iLast);

	// Remove all constraints connected to this part
	int nConstraints = m_lConstraints.Len();
	for (int i = nConstraints - 1; i >= 0; i--)
	{
		SConstraint& c = m_lConstraints[i];
		if (c.m_iObj0 == _iPart || c.m_iObj1 == _iPart)
		{ // Remove constraint
//			OnBreak(c);						-- TODO: Should OnBreak be run on constraints that are automatically removed because of destroyed parts?
			if (c.m_iPhysConstraint >= 0)
				_Obj.DeletePhysConstraint(c.m_iPhysConstraint);
			m_lConstraints.Del(i);
		}
		else
		{ // Relocate constraint indices if needed
			if (c.m_iObj0 >= _iPart)
				c.m_iObj0--;

			if (c.m_iObj1 >= _iPart)
				c.m_iObj1--;
		}
	}
}




/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Object, parent, 0x0100);

void CWObject_Object::OnCreate()
{
	parent::OnCreate();

	m_iOwner = 0;
	m_iParentObject = 0;
	m_SpawnMask = safe_cast<CWorld_ServerCore>(m_pWServer)->m_CurSpawnMask & (SERVER_SPAWNFLAGS_GLOBAL_MASK | SERVER_SPAWNFLAGS_LAYERS_MASK | SERVER_SPAWNFLAGS_CUSTOM_MASK);

	m_PhysFailCount = 0;
	m_LifetimeEnd = 0;

	m_ImpactPos = 0;
	m_ImpactForce = 0;
	m_iImpactPart = -1;
	m_ImpactSoundTick = -2;

	m_iPhysEP = -1;
}


void CWObject_Object::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	parent::OnIncludeClass(_pWData, _pWServer);
	_pWData->GetResourceIndex_Class("Object_Lamp");
}


void CWObject_Object::OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer)
{
	CWO_Burnable::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
}


void CWObject_Object::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	CFStr KeyName = _pKey->GetThisName();
	CFStr KeyValue = _pKey->GetThisValue();
//	CMapData* pMapData = m_pWServer->GetMapData();

	if (m_Root.OnEvalKey(_KeyHash, *_pKey, *this))
		return; // ignore - already handled

	if (KeyName.CompareSubStr("BURN_", 0) == 0)
	{
		CWO_Object_ClientData* pCD = AllocClientData(this);
		if (pCD->m_Burnable.m_Value.OnEvalKey(m_pWServer, _KeyHash, _pKey))
		{
			m_Root.m_Flags |= FLAGS_BURNABLE;
			return;	// ignore - already handled
		}
	}

	switch (_KeyHash) // TODO: Remove, is obsolete
	{
	case MHASH4('PARE','NT_O','BJEC','T'): // "PARENT_OBJECT"
		{
			// This key is generated by Ogier, if the object is placed inside a 'node_destructable'
			m_iParentObject = KeyValue.Val_int();
			if (!m_iParentObject)
			{
				CStr ParentName = m_pWServer->World_MangleTargetName(KeyValue);
				m_iParentObject = ParentName.StrHash();
				m_Root.m_Flags |= FLAGS_PARENTNAMEHASH;
			}
			break;
		}
	default:
		{
	//		M_TRACE("Unknown key: '%s' = '%s'\n", KeyName.Str(), KeyValue.Str());
			parent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}


void CWObject_Object::AddPart(const SPart& _Data)
{
	uint iPart = m_Root.m_lSubParts.Len();
	m_Root.m_lSubParts.SetLen(iPart + 1);
	m_Root.m_lSubParts[iPart] = _Data;
}


void CWObject_Object::AddConstraint(const SConstraint& _Data)
{
	uint iConstraint = m_Root.m_lConstraints.Len();
	m_Root.m_lConstraints.SetLen(iConstraint + 1);
	SConstraint& Dest = m_Root.m_lConstraints[iConstraint];
	Dest = _Data;
	if (Dest.m_iObj0 != 0)
		Dest.m_iObj0 = SConstraint::HashToIndex(Dest.m_iObj0, m_Root, *this);
}


void CWObject_Object::RemoveConstraint(int _iPhysConstraint)
{
	TAP<SConstraint> pConstraints = m_Root.m_lConstraints;
	for (int i = 0; i < pConstraints.Len(); i++)
		if (pConstraints[i].m_iPhysConstraint == _iPhysConstraint)
		{
			m_Root.m_lConstraints.Del(i);
			return;
		}
}


// This function is used when spawning sub-objects from destructable objects
void CWObject_Object::OnInitInstance(const aint* _pParam, int _nParam)
{
	if (_nParam == 2)
	{
		// This is used for creating sub-parts
		const CWObject_Object* pBaseObj = reinterpret_cast<const CWObject_Object*>(_pParam[0]);
		const SPart* pInfo = reinterpret_cast<const SPart*>(_pParam[1]);

		m_Root = *pInfo;
		m_Root.m_Flags |= FLAGS_SAVEPARTS;
		m_Root.m_Flags &= ~(FLAGS_FROMTEMPLATE | FLAGS_BROKEN);

		if (pBaseObj)
		{
			iModel(0) = pBaseObj->m_iModel[0];
			if (pBaseObj->m_Root.m_Flags & FLAGS_LAMP)
			{
				if (!(pBaseObj->m_Root.m_Flags & FLAGS_MODELSHADOW))
					Data(DATA_LAMP_LIGHTINDEX) = pBaseObj->Data(DATA_LAMP_LIGHTINDEX);
			}

			// Copy velocity from parent
			m_pWServer->Object_SetVelocity(m_iObject, pBaseObj->GetVelocity());

			// Copy spawn mask from parent
			m_SpawnMask = pBaseObj->m_SpawnMask;
		}
	}
	else if (_nParam == 1)
	{
		// This is used when spawning effects 
		const SObjectModel* pInfo = reinterpret_cast<const SObjectModel*>(_pParam[0]);
		m_Root.m_iModel[0] = pInfo->m_iModel[0];
		m_Root.m_iModel[1] = pInfo->m_iModel[1];
		m_Root.m_iModel[2] = pInfo->m_iModel[2];
		m_Root.m_Lifetime = pInfo->m_LifeTime;
		m_Root.m_Flags &= ~FLAGS_GAMEPHYSICS;
		m_Root.m_Flags |= FLAGS_NOPHYSICS;
	}
}


void CWObject_Object::OnFinishEvalKeys()
{
	parent::OnFinishEvalKeys();
	m_Root.OnFinishEvalKeys(*this);

	if (m_Root.m_Flags & FLAGS_BURNABLE)
		GetClientData(this)->m_Burnable.m_Value.SetValidModel(m_pWServer);

	if (m_Root.m_Flags & FLAGS_PARENTNAMEHASH)
		m_iParentObject = m_pWServer->Selection_GetSingleTarget(m_iParentObject);

	if (m_iParentObject > 0)
	{
		// If m_iParentObject is set, just add our SPart-data to the parent's part-list, and then remove ourselves 
		// (destruction is done in OnSpawnWorld2, to allow hierarchies)
		CWObject* pObjBase = m_pWServer->Object_Get(m_iParentObject);
		if (!pObjBase) Error("OnFinishEvalKeys", "Ogier set us up the bomb (no parent object)!");

		CMat4Dfp32 ParentInv;
		pObjBase->GetPositionMatrix().InverseOrthogonal(ParentInv);
		GetPositionMatrix().Multiply(ParentInv, m_Root.m_LocalPos);

		//m_Root.m_Origin = GetPositionMatrix();//() - pObjBase->GetPosition();
		CWObject_Object* pObj = safe_cast<CWObject_Object>(pObjBase);
		m_Root.m_Name = m_pWServer->Object_GetName(m_iObject);
		pObj->AddPart(m_Root);
		return;
	}

	ObjectSpawn(!(m_Root.m_Flags & FLAGS_WAITSPAWN));
}


void CWObject_Object::OnSpawnWorld()
{
	parent::OnSpawnWorld();
	m_Root.OnSpawnWorld(*this);
}


void CWObject_Object::OnSpawnWorld2()
{
	// This is called after all 'Object' entities have been spawned,
	// so now it's safe to remove child objects
	if (m_iParentObject)
		m_pWServer->Object_Destroy(m_iObject);
}


void CWObject_Object::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Nuke old client data
	_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = NULL;

	// Allocate only on server (Used during creation, may be nuked in OnFinishEvalKey)
	if (_pWPhysState->IsServer())
		AllocClientData(_pObj);
}


CWO_Object_ClientData* CWObject_Object::AllocClientData(CWObject_CoreData* _pObj)
{
	if (_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] == NULL)
	{
		CWO_Object_ClientData* pCD = MNew(CWO_Object_ClientData);
		if (!pCD)
			Error_static("CWObject_Object", "Could not allocate client data!");

		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = pCD;
	}
	return GetClientData(_pObj);
}


// Get ClientData, or NULL if it does not exist
const CWO_Object_ClientData* CWObject_Object::GetClientData(const CWObject_CoreData* _pObj)
{
	return safe_cast<const CWO_Object_ClientData>((const CReferenceCount*)_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
}

// Get ClientData, or NULL if it does not exist
CWO_Object_ClientData* CWObject_Object::GetClientData(CWObject_CoreData* _pObj)
{
	return safe_cast<CWO_Object_ClientData>((CReferenceCount*)_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
}


void CWObject_Object::RefreshDirty()
{
	CWO_Object_ClientData* pCD = GetClientData(this);
	if (pCD)
		m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}


int CWObject_Object::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const
{
	uint8* pD = _pData;
	int Flags = 0;

	const CWO_Object_ClientData* pCD = GetClientData(this);
	uint AutoVarMask = _pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT;
	if (pCD && AutoVarMask)
		Flags |= CWO_CLIENTUPDATE_AUTOVAR;

	pD += parent::OnCreateClientUpdate(_iClient, _pClObjInfo, _pOld, _pData, Flags);
	if ((pD - _pData) == 0)
		return 0;

	if (pCD && AutoVarMask)
	{ // Handle AutoVars
		pCD->AutoVar_Pack(AutoVarMask, pD, m_pWServer->GetMapData(), 0);
	}

	return (pD - _pData);
}


int CWObject_Object::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	const uint8* pD = _pData;

	pD += parent::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);
	if (_pObj->m_iClass == 0 || (pD - _pData) == 0)
		return 0;

	if (_pObj->m_bAutoVarDirty)
	{ // Handle AutoVars
		CWO_Object_ClientData* pCD = AllocClientData(_pObj);

		CWO_Burnable& Burnable = pCD->m_Burnable.m_Value;
		bool bWasBurning = Burnable.IsBurning();
		if (pCD->AutoVar_Unpack(pD, _pWClient->GetMapData(), 0))
		{
			Burnable.OnClientUpdate(_pObj, _pWClient);
			if (!bWasBurning && Burnable.IsBurning())
				InitBurnable(_pObj, _pWClient);
		}
	}

	return (pD - _pData);
}


void CWObject_Object::InitBurnable(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	// Objected was set on fire, fetch boundings on client for rendering
	const CWO_PhysicsState& PhysState = _pObj->GetPhysState();
	if (PhysState.m_nPrim > 0)
	{
		const CWO_PhysicsPrim& PhysPrim = PhysState.m_Prim[0];
		CXR_Model* pM = _pWClient->GetMapData()->GetResource_Model(PhysPrim.m_iPhysModel);
		if (pM)
		{
			// fetch boundings
			CBox3Dfp32 PhysBoundBox[16];
			uint nBoundBoxes = 0;
			for (uint i = 0; i < 16; i++)
			{
				if (PhysPrim.m_PhysModelMask & M_BitD(i))
					pM->GetBound_Box(PhysBoundBox[nBoundBoxes++], PhysPrim.m_PhysModelMask);
			}

			// All bounding boxes fetched, store data
			CWO_Object_ClientData* pCD = GetClientData(_pObj);
			if (nBoundBoxes > 0)
			{
				pCD->m_lPhysBoundBox.SetLen(nBoundBoxes);
				memcpy(pCD->m_lPhysBoundBox.GetBasePtr(), PhysBoundBox, sizeof(CBox3Dfp32) * nBoundBoxes);
			}
			else
				pCD->m_lPhysBoundBox.Clear();
		}
	}
}



void CWObject_Object::OnRefresh()
{
	MSCOPESHORT(CWObject_Object::OnRefresh);
	parent::OnRefresh();

	bool bStopRefresh = true;

	// Time to shut down?
	if (m_LifetimeEnd)
	{
		bStopRefresh = false;
		if (m_LifetimeEnd < m_pWServer->GetGameTick())
		{
			// Destroy ourselves
			DBG_OUT("Object %d, lifetime end\n", m_iObject);
			m_LifetimeEnd = 0;
			m_pWServer->Object_Destroy(m_iObject);
			return;
		}
	}

	// Burning?
	if (m_Root.m_Flags & FLAGS_BURNABLE)
	{
		CWO_Object_ClientData* pCD = GetClientData(this);
		if (pCD->m_Burnable.m_Value.OnRefresh(this, m_pWServer))
			pCD->m_Burnable.MakeDirty();

		if (pCD->m_Burnable.m_Value.IsBurning())
			bStopRefresh = false;
	}

	// Need to retry phys init?
	if (m_Root.m_Flags & FLAGS_FAILEDSETPHYSICS)
	{
		ObjectSpawn(true);
		bStopRefresh = false;
	}

	// Need to do phys-related checks?
	if (m_Root.m_Flags & (FLAGS_GAMEPHYSICS | FLAGS_ISMOVING))
	{
		if (OnRefresh_GamePhysics())
			bStopRefresh = false;
	}

	// Update animation stuff
	if (m_iAnim0)
	{
		if (OnRefresh_Animation())
			bStopRefresh = false;
	}

	// Ok to turn off refresh?
	if (bStopRefresh)
	{
		//DBG_OUT("Object[%d: %s], deactivated\n", m_iObject, GetName());
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
	}

	RefreshDirty();
}

// Returns true if refresh is still needed
bool CWObject_Object::OnRefresh_GamePhysics()
{
	MSCOPESHORT(CWObject_Object::OnRefresh_GamePhysics);

	const CVec3Dfp32 MoveVel = GetPosition() - GetLastPosition();  // -- GetMoveVelocity() doesn't work if object is linked to a parent
	bool bWasMoving = (m_Root.m_Flags & FLAGS_ISMOVING) != 0;
	bool bIsMoving = (MoveVel.LengthSqr() > 0.05f) || !m_pWServer->Phys_IsStationary(m_iObject);
	bool bRet = false;

	if (bIsMoving)
	{
		m_Root.m_Flags |= FLAGS_ISMOVING;
		m_Root.m_Flags |= FLAGS_SAVEPOSITION;
		bRet = true;

		// This is a hack to make triggers react to physics-controlled objects.
		// TODO: trigger notification should be handled by physics engine / server.
		if (m_pWServer->Phys_GetCollisionPrecision() == 0)
		{
			TSelection<CSelection::SMALL_BUFFER> Sel1, Sel2;

			CWO_PhysicsState Phys = GetPhysState();
			const CBox3Dfp32& BBox = m_Root.m_PhysPrim.m_BoundBox;
			CVec3Dfp32 Offset; BBox.GetCenter(Offset);
			CVec3Dfp32 Size = (BBox.m_Max - BBox.m_Min) * 0.5f;
			Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, Size, Offset);
			Phys.m_nPrim = 1;
			Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_TRIGGER;	// Shouldn't collide with anything but triggers
			Phys.m_ObjectFlags = OBJECT_FLAGS_OBJECT;
			CMat4Dfp32 Pos = GetPositionMatrix();
			m_pWServer->Phys_IntersectWorld(NULL, Phys, Pos, Pos, m_iObject, NULL, m_IntersectNotifyFlags, &Sel1, &Sel2);

			const int16* pSel;
			int nSel = m_pWServer->Selection_Get(Sel1, &pSel);
			for (int iSelObj = 0; iSelObj < nSel; iSelObj++)
				m_pWServer->MessageQueue_SendToObject(CWObject_Message(OBJSYSMSG_NOTIFY_INTERSECTION, pSel[iSelObj]), m_iObject);

			nSel = m_pWServer->Selection_Get(Sel2, &pSel);
			for (int iSelObj = 0; iSelObj < nSel; iSelObj++)
				m_pWServer->MessageQueue_SendToObject(CWObject_Message(OBJSYSMSG_NOTIFY_INTERSECTION, m_iObject), pSel[iSelObj]);

			m_pWServer->MessageQueue_Flush();
		}

		if( CheckForOOB() )
		{
			if (!(m_Root.m_Flags & FLAGS_DEBRIS)) // don't spam about debris outside world...
				ConOutL(CStrF("§cf80WARNING: Removing object '%s' because it is outside the world..", GetName()));
		}
	}
	else
	{
		m_Root.m_Flags &= ~FLAGS_ISMOVING;
	}

	if (bIsMoving && !bWasMoving)
	{
		m_Root.m_Messages_OnMove.Send(m_iOwner, *this);
	}

	if (m_Root.m_Flags & FLAGS_UPDATECONSTRAINTS)
	{
		foreach (const SConstraint, c, m_Root.m_lConstraints)
		{
			if ((c.m_iPhysConstraint < 0) || (c.m_iObj0 >= CONSTRAINT_TARGET_WORLD) && (c.m_iObj1 >= CONSTRAINT_TARGET_WORLD))
				continue;

			if (c.m_Type != CONSTRAINT_TYPE_BALL)
				continue;

			int iExternalObj = (c.m_iObj0 < CONSTRAINT_TARGET_WORLD) ? -c.m_iObj0 : -c.m_iObj1;
			const CMat4Dfp32& ObjMat = m_pWServer->Object_GetPositionMatrix(iExternalObj);
			CVec3Dfp32 Pos = c.m_LocalPos.GetRow(3);
			Pos *= ObjMat;
			m_pWServer->Phys_UpdateBallConstraint(c.m_iPhysConstraint, Pos);
			bRet = true;
		}
	}
	return bRet;
}

bool CWObject_Object::OnRefresh_Animation()
{
	MSCOPESHORT(CWObject_Object::OnRefresh_Animation);
	M_ASSERT(m_iAnim0, "Don't run anim refresh when no anim is active!");
	CWO_Object_ClientData* pCD = GetClientData(this);
	M_ASSERT(pCD, "client data should be allocated when starting animation!");

	CMTime StartTime = pCD->m_Anim_StartTime;
	if (StartTime.IsInvalid() || GetParent() != 0)
	{ // animation has reached it's end. do nothing
		return false;
	}

	CXR_AnimLayer Tmp;
	GetAnimLayer(this, m_pWServer, m_pWServer->GetGameTime(), Tmp);
	CXR_Anim_SequenceData* pSeq = Tmp.m_spSequence;

	// Time to freeze animation?
	fp32 Duration = pSeq->GetDuration();
	if ((pSeq->GetLoopType() == ANIM_SEQ_LOOPTYPE_ONCE) && (Tmp.m_Time >= Duration))
	{
		DBG_OUT("freezing animation... (time: %.1f)\n", Tmp.m_Time);
		pCD->m_Anim_StartTime = CMTime::CreateInvalid();
		return false;
	}

	// Calculate movement
	vec128 Move;
	VecUnion dMove, CurrPos, WantedPos;
	CQuatfp32 Rot, Rot1, dRot, AnimBaseRot, CurrRot, WantedRot;
	pSeq->EvalTrack0(CMTime::CreateFromSeconds(Tmp.m_Time), Move, Rot);

	CurrPos.v128 = GetPosition_vec128();
	WantedPos.v128 = M_VMulMat4x3(M_VSetW1(Move), pCD->m_Anim_BaseMat);

	CurrRot.Create(GetPositionMatrix());
	AnimBaseRot.Create(pCD->m_Anim_BaseMat);
	WantedRot = Rot;
	WantedRot *= AnimBaseRot;

	{
#define DEBUG_COLOR_RED   0xff0000ff
#define DEBUG_COLOR_GREEN 0xff00ff00
		m_pWServer->Debug_RenderVertex(CurrPos.v3 + CVec3Dfp32(0,0,16), DEBUG_COLOR_RED, 10.0f, true);
		m_pWServer->Debug_RenderVertex(WantedPos.v3 + CVec3Dfp32(0,0,16), DEBUG_COLOR_GREEN, 10.0f, true);
	}

	if (m_Root.m_Flags & FLAGS_GAMEPHYSICS)
	{
		if (m_Root.m_Flags & FLAGS_ANIM_DELTAMOVE)
		{
			vec128 Move1;
			// Calculate movement by comparing animation data at 't' with 't+1'
			CMTime Time1 = CMTime::CreateFromSeconds(Tmp.m_Time + m_pWServer->GetGameTickTime());
			pSeq->EvalTrack0(Time1, Move1, Rot1);

			dMove.v128 = M_VSub(Move1, Move);
			dMove.v128 = M_VMulMat3x3(dMove.v128, pCD->m_Anim_BaseMat);
			dRot = Rot;
			dRot.Inverse();
			dRot *= Rot1;
		}
		else
		{
			// Calculate movement by comparing actualpos with wantedpos
			dMove.v128 = M_VSub(WantedPos.v128, CurrPos.v128);
			if (dMove.v3.LengthSqr() > Sqr(64.0f))
				dMove.v3.SetLength(64.0f); // safety precaution

			dRot = CurrRot;
			dRot.Inverse();
			dRot *= WantedRot;
		}

		// Set velocity
		CVelocityfp32 Vel;
		Vel.m_Move = dMove.v3;
		Vel.m_Rot.Create(dRot);
		m_pWServer->Object_SetVelocity(m_iObject, Vel);

		if (dMove.v3.LengthSqr() > 0.001f || M_Fabs(Vel.m_Rot.m_Angle) > 0.001f)
		{
			m_pWServer->Phys_SetStationary(m_iObject, false);
			m_Root.m_Flags |= FLAGS_ISMOVING;
		}
	}
	else
	{
		// Set position
		if ((CurrPos.v3.DistanceSqr(WantedPos.v3) > 0.001f) || (CurrRot.DotProd(WantedRot) < 0.9999f))
		{
			CMat4Dfp32 Mat;
			Mat.Create(WantedRot, M_VSetW1(WantedPos.v128));
			m_pWServer->Object_SetPosition_World(m_iObject, Mat);
			m_Root.m_Flags |= FLAGS_ISMOVING;
		}
	}

	{ // Check anim events
		uint Mask = ANIM_EVENT_MASK_SOUND;
		CMTime t0 = m_pWServer->GetGameTime() - StartTime;
		CMTime t1 = t0 + CMTime::CreateFromSeconds(m_pWServer->GetGameTickTime() - 0.0001f);
		int16 iKey = 0;
		const CXR_Anim_DataKey* pKey = pSeq->GetEvents(t0, t1, Mask, iKey);
		while (pKey != NULL)
		{
			fp32 EventTimeOffset = pKey->m_AbsTime - t0.GetTime();
			OnAnimEvent(*pKey, EventTimeOffset);
			pKey = pSeq->GetEvents(t0, t1, Mask, iKey);
		}
	}
	return true;
}


void CWObject_Object::OnAnimEvent(const CXR_Anim_DataKey& _Key, fp32 _EventTimeOffset)
{
	switch (_Key.m_Type)
	{
	case ANIM_EVENT_TYPE_SOUND:
		{
			int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(_Key.Data());
			if (iSound > 0)
			{
				uint8 iMaterial = 0;
				if(_Key.m_Param)
				{
					CVec3Dfp32 Start = GetPosition() + CVec3Dfp32(0, 0, 1);
					CVec3Dfp32 Stop = GetPosition() - CVec3Dfp32(0, 0, 10);
					CCollisionInfo CInfo;
					CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_SURFACE);
					if(m_pWServer->Phys_IntersectLine(Start, Stop, 0, OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_PHYSMODEL, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS | XW_MEDIUM_CAMERASOLID, &CInfo, m_iObject))
					{
						iMaterial = CInfo.m_SurfaceType;
						if(iMaterial == 0 && CInfo.m_pSurface && CInfo.m_pSurface->GetBaseFrame())
							iMaterial = CInfo.m_pSurface->GetBaseFrame()->m_MaterialType;
					}
				}

				CVec3Dfp32 Pos;
				GetAbsBoundBox()->GetCenter(Pos);
				m_pWServer->Sound_At(Pos, iSound, 0, iMaterial);
			}
		}
		break;

	case ANIM_EVENT_TYPE_SETSOUND:
		{
			CWObject_Message Msg(OBJSYSMSG_SETSOUND, _Key.m_Param);
			Msg.m_pData = const_cast<char*>( _Key.Data() );
			Msg.m_DataSize = _Key.DataSize();
			OnMessage(Msg);
		}
		break;
	}
}


void CWObject_Object::OnDestroy()
{
	DBG_OUT("CWObject_Object::OnDestroy, this = 0x%08X, iObject = %d, name = '%s'\n", this, m_iObject, GetName());
	DeleteAllPhysConstraints();
	//m_Root.Clear();
}


void CWObject_Object::UpdateRenderMask()
{
	m_Root.m_RenderMask = 0;
	foreach (SPart, child, m_Root.m_lSubParts)
		m_Root.m_RenderMask |= child.m_RenderMask;

	Data(DATA_OBJ_VISIBILITYMASK) = m_Root.m_RenderMask;		// visibilitymask for rendering models
}


void CWObject_Object::AddDelayedBreak(const CNameHash& _Name, uint _iSender)
{
	M_ASSERT(uint32(_Name) != 0, "Can't add noname object to destruction queue.");

	// Already in queue?
	TAP<SDelayedBreak> pExisting = m_lDelayedBreak;
	for (int i = 0; i < pExisting.Len(); i++)
	{
		if (pExisting[i].m_Name == _Name)
			return;
	}

	// No, so add it
	SDelayedBreak tmp = { _Name, _iSender };
	m_lDelayedBreak.Add(tmp);
}


void CWObject_Object::FlushDelayedBreak()
{
	// Don't delete stuff until breakage-handling is done
	if (m_Root.m_Flags & FLAGS_ISBREAKING)
		return;

	// We need to set this flag, since the OnBreak-calls below may cause even more breaks
	m_Root.m_Flags |= FLAGS_ISBREAKING;

	uint nElem;
	while ((nElem = m_lDelayedBreak.Len()))
	{
		uint32 NameHash;
		uint iSender;
		{
			const SDelayedBreak& e = m_lDelayedBreak[nElem - 1];
			NameHash = e.m_Name;
			iSender = e.m_iSender;

			m_lDelayedBreak[0] = m_lDelayedBreak[nElem - 1];
			m_lDelayedBreak.Del(nElem - 1);
		}

		int iConstraint = m_Root.FindConstraint(NameHash);
		int iTrigger = m_Root.FindTrigger(NameHash);
		int iPart = m_Root.FindPart(NameHash);

		if (iConstraint >= 0)
		{
			OnBreak(m_Root.m_lConstraints[iConstraint], iSender);
		}
		else if (iTrigger >= 0)
		{
			OnDestroy(m_Root.m_lTriggers[iTrigger], iSender);
		}
		else if (iPart >= 0)
		{
			OnDestroy(m_Root.m_lSubParts[iPart], iSender);
		}
	}

	// All done!
	m_Root.m_Flags &= ~FLAGS_ISBREAKING;
}


void CWObject_Object::DeletePhysConstraint(int _iPhysConstraint)
{
	uint16 iObj1, iObj2;
	m_pWServer->Phys_GetConnectedObjects(_iPhysConstraint, &iObj1, &iObj2);
	uint16 iObjOther = (iObj1 == m_iObject) ? iObj2 : iObj1;

	// Delete constraint from engine
	DBG_OUT("Object %d, RemoveConstraint %d (other obj: %d)\n", m_iObject, _iPhysConstraint, iObjOther);
	m_pWServer->Phys_RemoveConstraint(_iPhysConstraint);

	// Delete constraint from other object
	CWObject* pObjx = m_pWServer->Object_Get(iObjOther);
	CWObject_Object* pObjOther = (pObjx && (pObjx->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_OBJECT)) ? safe_cast<CWObject_Object>(pObjx) : NULL;
	if (pObjOther)
		pObjOther->RemoveConstraint(_iPhysConstraint);
}

void CWObject_Object::DeleteAllPhysConstraints()
{
	foreach (SConstraint, c, m_Root.m_lConstraints)
	{
		if (c.m_iPhysConstraint >= 0)
		{
			DeletePhysConstraint(c.m_iPhysConstraint);
			c.m_iPhysConstraint = -1;
		}
	}
}


bool CWObject_Object::CreatePhysState(const SPart& _Part, CWO_PhysicsState& _Result)
{
	const CBox3Dfp32& BBox = _Part.m_PhysPrim.m_BoundBox;

	//
	// Setup rigid body ("lowres") physprim
	//
	int iPhysModel = _Part.m_PhysPrim.m_iPhysModel_Dynamics;
	CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(iPhysModel);
//	if (!pModel)
//		return false; //TODO: Use trimesh bound if no physmodel?

	_Result = GetPhysState();
	_Result.m_ObjectFlags = int16(OBJECT_FLAGS_OBJECT);
	CXR_PhysicsModel* pPhysModel = pModel ?  pModel->Phys_GetInterface() : NULL;
	if (pPhysModel)
	{
		CVec3Dfp32 Offset = 0.0f;//m_Root.m_PhysPrim.m_Origin;
		_Result.m_nPrim = 1;
		_Result.m_Prim[0].Create(OBJECT_PRIMTYPE_PHYSMODEL, iPhysModel, CVec3Dfp32(0.0f), CVec3Dfp32(Offset), _Part.m_PhysPrim.m_VisibilityMask);
	}
	else
	{
		// Create phys from model's bounding box   TODO: remove this support?
		CVec3Dfp32 Size = (BBox.m_Max - BBox.m_Min) * 0.5f;
		if (Size.LengthSqr() < 0.0001f)
			return false; // null object
		// Make sure each component is atleast 1 (prim is uint16)
		Size.k[0] = Max(Size.k[0],1.0f);
		Size.k[1] = Max(Size.k[1],1.0f);
		Size.k[2] = Max(Size.k[2],1.0f);
		CVec3Dfp32 Offset;
		BBox.GetCenter(Offset);
		_Result.m_nPrim = 1;
		_Result.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, 0, Size, Offset);
	}

	//
	// Setup "highres" physprim (used for projectile collisions & mass calculations)
	//
	iPhysModel = _Part.m_PhysPrim.m_iPhysModel;
	pModel = m_pWServer->GetMapData()->GetResource_Model(iPhysModel);
	if (pModel)
	{
		pPhysModel = pModel->Phys_GetInterface();
		if (pPhysModel)
		{
			_Result.m_Prim[_Result.m_nPrim].Create(OBJECT_PRIMTYPE_PHYSMODEL, iPhysModel, CVec3Dfp32(0.0f), CVec3Dfp32(0.0f), _Part.m_PhysPrim.m_VisibilityMask);
			_Result.m_nPrim++;
		}
	}

	//
	// Setup trimesh physprim (used for projectile collisions)
	//
	if (_Part.m_Flags & FLAGS_TRIMESHCOLLISION)
	{
		pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
		if (pModel && (pPhysModel = pModel->Phys_GetInterface()))
		{
			_Result.m_Prim[_Result.m_nPrim].Create(OBJECT_PRIMTYPE_PHYSMODEL, m_iModel[0], CVec3Dfp32(0.0f), CVec3Dfp32(0.0f), _Part.m_PhysPrim.m_VisibilityMask);
			_Result.m_nPrim++;
		}
	}

	return true;
}

// Checks if any of the constraints is connected to $world, if so make the object immobile
// Returns true if the physmovement flag changed
bool CWObject_Object::CheckIfMobile(CWO_PhysicsState& _Phys)
{
	if (!(m_Root.m_Flags & FLAGS_GAMEPHYSICS))
		return false;

	uint16 OldFlags = _Phys.m_PhysFlags;
	_Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_PHYSMOVEMENT;
	foreach (const SConstraint, c, m_Root.m_lConstraints)
	{
		if (c.m_Type == CONSTRAINT_TYPE_RIGID && (c.m_iObj0 == CONSTRAINT_TARGET_WORLD || c.m_iObj1 == CONSTRAINT_TARGET_WORLD))
		{
			// this is a rigid constraint connected with '$world' - object should be unmovable
			_Phys.m_PhysFlags &= ~OBJECT_PHYSFLAGS_PHYSMOVEMENT;
			return (OldFlags & OBJECT_PHYSFLAGS_PHYSMOVEMENT) != 0;
		}
	}
	return (OldFlags & OBJECT_PHYSFLAGS_PHYSMOVEMENT) == 0;
}


void CWObject_Object::ObjectSpawn(bool _bSpawn)
{
	MSCOPESHORT(CWObject_Object::ObjectSpawn);
//	ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;

	DBG_OUT("[Object %d (%s)], ObjectSpawn: %d\n", m_iObject, GetName(), int(_bSpawn));

	if (_bSpawn)
	{
		InitStatic();  //TODO: move this call - not necessary each object spawn...

		// Copy some parameters that needs to be on client
		iModel(0) = m_Root.m_iModel[0];
		iModel(1) = m_Root.m_iModel[1];
		iModel(2) = m_Root.m_iModel[2];
		Data(DATA_OBJ_VISIBILITYMASK) = m_Root.m_RenderMask;

		// Remove invisible flag
		ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
		m_Root.m_Flags &= ~FLAGS_WAITSPAWN;
		if (!(m_Root.m_Flags & FLAGS_NOPHYSICS))
		{
			const CBox3Dfp32& BBox = m_Root.m_PhysPrim.m_BoundBox;

			CWO_PhysicsState Phys;
			bool bOK = CreatePhysState(m_Root, Phys);
			if (bOK)
			{
				if (m_Root.m_Flags & FLAGS_GAMEPHYSICS)
				{ // Dynamics setup...
					Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_PHYSICSCONTROLLED;
					Phys.m_ObjectFlags |= OBJECT_FLAGS_PICKUP; // This is so that characters can grab the object
				}

				CheckIfMobile(Phys);

				foreach (const SConstraint, c, m_Root.m_lConstraints)
				{
					if (c.m_Type != CONSTRAINT_TYPE_RIGID && c.m_iObj0 >= 0 && c.m_iObj1 >= 0)
					{
						// this is a constraint that keeps the object together.
						// if we get damage, we must split up and stop acting like one rigid object
						m_Root.m_Flags |= FLAGS_FRAGILE;
					}
				}

				Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_PUSHER | OBJECT_PHYSFLAGS_ROTATION;// | OBJECT_PHYSFLAGS_OFFSET
				if (m_Root.m_Flags & FLAGS_DEBRIS)
				{
					Phys.m_ObjectFlags |= OBJECT_FLAGS_PHYSMODEL;
					Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_PHYSMODEL;
				}
				else if (m_Root.m_Flags & FLAGS_ANIMPHYS)
				{
					Phys.m_ObjectFlags |= OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_ANIMPHYS;
					Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_PHYSMODEL;
				}
				else
				{
					Phys.m_ObjectFlags |= OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
					Phys.m_ObjectIntersectFlags |= OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_PHYSMODEL;	// Should use OBJECT_FLAGS_PLAYERPHYSMODEL but not can't use BSP4 for physics atm.
				}

				if (m_Root.m_Flags & FLAGS_NAVIGATION)
					Phys.m_ObjectFlags |= OBJECT_FLAGS_NAVIGATION;

				//
				// Include trigger visibility masks in projectile physprims (to be able to hit triggers)
				//
				if (Phys.m_nPrim > 1)
				{
					uint32 TriggerMask = 0;
					foreach (const STrigger, t, m_Root.m_lTriggers)
						TriggerMask |= t.m_PhysPrim.m_VisibilityMask;

					for (uint i = 1; i < Phys.m_nPrim; i++)
						Phys.m_Prim[i].m_PhysModelMask |= TriggerMask;
				}

				//
				// Done setting up phys state, let's use it!
				//
				if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
				{
					m_PhysFailCount++;
					if (m_PhysFailCount < 20)
						M_TRACE("[Object %d, '%s' (%s)] Unable to set model physics (position: %s, fail count: %d)\n", m_iObject, GetName(), GetTemplateName(), GetPosition().GetString().Str(), m_PhysFailCount);
					if (m_PhysFailCount < 5)
					{
						ConOutL(CStrF("§cf80WARNING: Unable to set model physics state: (name: %s, template: %s, position: %s", 
							GetName(), GetTemplateName(), GetPosition().GetString().Str() ));
					}

					if (m_PhysFailCount < 100)
					{
						m_Root.m_Flags |= FLAGS_FAILEDSETPHYSICS;
						ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
					}
					else
					{
						m_Root.m_Flags &= ~FLAGS_FAILEDSETPHYSICS;
					}
				}
				else
				{
					m_Root.m_Flags &= ~FLAGS_FAILEDSETPHYSICS;

					//
					// Init rigid body
					//
					fp32 Mass = m_Root.m_Mass.Getfp32();
					CWO_PhysicsState Phys = GetPhysState();
					M_ASSERT(Phys.m_nPrim, "!");

					// Always override mass for rigid bodies (for non-physics objects, only override if value is non-zero)
					if ((Mass > 0.0f) || (m_Root.m_Flags & FLAGS_GAMEPHYSICS))
						SetMass(Mass);

					if (m_Root.m_Flags & FLAGS_GAMEPHYSICS)
					{
						DBG_OUT("Object %d '%s', init rigid body\n", m_iObject, GetName());
						m_pWServer->Object_InitRigidBody(m_iObject, (m_Root.m_Flags & FLAGS_DEBRIS) != 0);

						// Init world constraints
						foreach (SConstraint, c, m_Root.m_lConstraints)
						{
							if (c.m_Type == CONSTRAINT_TYPE_RIGID)
								continue;

							if (c.m_iObj0 > CONSTRAINT_TARGET_WORLD && c.m_iObj1 > CONSTRAINT_TARGET_WORLD)
								continue;

							DBG_OUT(" [Object %d], Creating constraint...\n", m_iObject);
							SConstraint tmp = c;
							tmp.m_iObj0 = m_iObject;
							tmp.m_iObj1 = 0;
							tmp.Relocate( GetPositionMatrix() );
							c.m_iPhysConstraint = CWObject_Object_Constraint::Spawn(*m_pWServer, tmp);
							DBG_OUT("   - id: %d\n", c.m_iPhysConstraint);

							// Check if the constraint is connected to an external object
							if (c.m_iObj0 < CONSTRAINT_TARGET_WORLD || c.m_iObj1 < CONSTRAINT_TARGET_WORLD)
							{
								int iExternalObj = (c.m_iObj0 < CONSTRAINT_TARGET_WORLD) ? -c.m_iObj0 : -c.m_iObj1;
								if (iExternalObj != m_iObject)
								{
									m_Root.m_Flags |= (FLAGS_UPDATECONSTRAINTS | FLAGS_SPAWNACTIVE);
									const CMat4Dfp32& ObjMat = m_pWServer->Object_GetPositionMatrix(iExternalObj);
									CMat4Dfp32 InvObjMat;
									ObjMat.InverseOrthogonal(InvObjMat);
									c.Relocate(InvObjMat);
								}
							}
						}

						bool bStationary = !(m_Root.m_Flags & FLAGS_SPAWNACTIVE);
						m_pWServer->Phys_SetStationary(m_iObject, bStationary);


						// Temp: add default grab attach
						if ((m_Root.m_lAttach_Grab.Len() == 0) && (Mass < 40.0f))
						{
							CVec3Dfp32 p;
							BBox.GetCenter(p);
							m_Root.m_lAttach_Grab.Add(p);
						}
					}

					// Turn off refresh - but not for active physics objects (because we need interpolation in order to make physics look good)
					if (!(Phys.m_PhysFlags & OBJECT_PHYSFLAGS_PHYSMOVEMENT))
						ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;

					//
					// Auto-calc mass for sub parts?
					//
					if (Mass > 0.0f) // root has got manually set mass
					{
						fp32 TmpMass, TmpVolume, InvTotalVolume; 
						m_pWServer->Phys_GetPhysicalProperties(GetPhysState(), TmpMass, TmpVolume);
						InvTotalVolume = 1.0f / TmpVolume;

						foreach (SPart, p, m_Root.m_lSubParts)
						{
							CWO_PhysicsState TmpPhys;
							if ((p.m_Mass.IsZero()) && CreatePhysState(p, TmpPhys))  // child does not have manually set mass
							{
								m_pWServer->Phys_GetPhysicalProperties(TmpPhys, TmpMass, TmpVolume);
								TmpMass = (TmpVolume * InvTotalVolume) * Mass;
								p.m_Mass.Set(TmpMass);
							}
						}
					}
				}
			}
			else if (m_Root.m_iModel[0])
			{
				ConOutL(CStrF("§cf80WARNING: No physics model for object '%s'", GetName()));
			}
			UpdateVisibility();
		}

		if (m_Root.m_Lifetime)
		{
			ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
			m_LifetimeEnd = m_pWServer->GetGameTick() + m_Root.m_Lifetime;
		}
	}
	else
	{
		// Unspawn
		m_Root.m_Flags |= FLAGS_WAITSPAWN;
		ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;
		m_pWServer->Object_SetPhysics(m_iObject, CWO_PhysicsState());
		m_pWServer->Object_InitRigidBody(m_iObject, false);
		DeleteAllPhysConstraints();
		m_LifetimeEnd = 0;
	}

	UpdateVisibility();
}


aint CWObject_Object::OnMessage(const CWObject_Message& _Msg)
{
	if (m_Root.m_Flags & FLAGS_DESTROYED)
		return 0;

	MSCOPESHORT(CWObject_Object::OnMessage);
	switch (_Msg.m_Msg)
	{
	case OBJSYSMSG_SETPARENT:
		{
			// Indicate that we have parent
			m_Root.m_Flags |= FLAGS_HASPARENT;
			return parent::OnMessage(_Msg);
		}
	case OBJMSG_HOOK_GETRENDERMATRIX:
		{
			CMat4Dfp32& ResultMat = *(CMat4Dfp32*)_Msg.m_Param1;
			int iParentAttach = m_iParentAttach;
			if (GetParent() && iParentAttach >= 0)
			{
				CWObject_Message Msg(OBJMSG_HOOK_GETRENDERMATRIX, iParentAttach, aint(&ResultMat));
				if (m_pWServer->Message_SendToObject(Msg, GetParent()))
				{
					M_VMatMul(GetLocalPositionMatrix(), ResultMat, ResultMat);
					goto matrix_oksvmsg1;
				}
			}
			ResultMat = GetPositionMatrix();
matrix_oksvmsg1:
			return 1;
		}

	case OBJMSG_HOOK_GETCURRENTMATRIX:
		{
			CMat4Dfp32& ResultMat = *(CMat4Dfp32*)_Msg.m_Param0;
			int iParentAttach = m_iParentAttach;
			if (GetParent() && iParentAttach >= 0)
			{
				CWObject_Message Msg(OBJMSG_HOOK_GETCURRENTMATRIX, aint(&ResultMat), iParentAttach);
				if (m_pWServer->Message_SendToObject(Msg, GetParent()))
				{
					M_VMatMul(GetLocalPositionMatrix(), ResultMat, ResultMat);
					goto matrix_oksvmsg2;
				}
			}
			ResultMat = GetPositionMatrix();
matrix_oksvmsg2:
			return 1;
		}

	case OBJMSG_GAME_SPAWN:
		{
			if (_Msg.m_Param0 <= 0)
			{
				m_CreationGameTick = m_pWServer->GetGameTick();
				ObjectSpawn(true);
				return 1;
			}
			else
			{
				ObjectSpawn(false);
				return 0;
			}
		} 

	case OBJMSG_OBJECT_SETUP:
		{
			m_iPhysEP = _Msg.m_Param0;
			return 1;
		}

	case OBJMSG_DAMAGE:
		{
			if (!(m_Root.m_Flags & (FLAGS_WAITSPAWN | FLAGS_IGNORE_DAMAGE)))
			{
				const CWO_DamageMsg* pMsg = CWO_DamageMsg::GetSafe(_Msg);
				if (pMsg)
				{
					bool bWasBreaking = (m_Root.m_Flags & FLAGS_ISBREAKING) != 0;
					if (!bWasBreaking)
					{
						m_ImpactPos = pMsg->m_Position;
						m_ImpactForce = pMsg->m_Force;
					}

					m_Root.m_Flags |= FLAGS_ISBREAKING;
					uint nParts = m_Root.m_lSubParts.Len();

					if (pMsg->m_DamageType == DAMAGETYPE_BLACKHOLE)
					{
						// ugly shit: look if the object is connected to a lamp - if so, ignore blackhole damage
						TAP<SConstraint> pConstraints = m_Root.m_lConstraints;
						for (int i = 0; i < pConstraints.Len(); i++)
							if (pConstraints[i].m_iPhysConstraint >= 0)
							{
								uint16 iObj0, iObj1;
								m_pWServer->Phys_GetConnectedObjects(pConstraints[i].m_iPhysConstraint, &iObj0, &iObj1);
								if (iObj0 == m_iObject)
									iObj0 = iObj1;
								if (iObj0)
								{
									CWObject* pOther = m_pWServer->Object_Get(iObj0);
									if (pOther && pOther->IsClass(class_Object_Lamp))
										return 0;
								}
							}
					}

					OnDamage(*pMsg, _Msg.m_iSender);

					if (!bWasBreaking)
					{
						m_Root.m_Flags &= ~FLAGS_ISBREAKING;

						// FIXME: These checks are pretty ugly.
						bool bBreaks = (m_lDelayedBreak.Len() != 0) || m_Root.m_DamageInfo.IsDestroyed() || (nParts != m_Root.m_lSubParts.Len());
						FlushDelayedBreak();

						if (bBreaks || (m_Root.m_lSubParts.Len() > 1 && m_Root.m_lConstraints.Len() == 0) || (m_Root.m_Flags & FLAGS_FRAGILE))
							CheckSeparation();
					}
				}
			}
			return 0;
		}

	case OBJMSG_RADIALSHOCKWAVE:
		{
			const CWO_ShockwaveMsg* pMsg = CWO_ShockwaveMsg::GetSafe(_Msg);
			if (pMsg && !(m_Root.m_Flags & (FLAGS_BROKEN | FLAGS_ISBREAKING)))
			{
				DBG_OUT("Object %d (%s) - radial damage from %d\n", m_iObject, GetName(), _Msg.m_iSender);

				CVec3Dfp32 TracePos;
				GetAbsBoundBox()->GetCenter(TracePos);

				CVec3Dfp32 Vel;
				CWO_DamageMsg DmgMsg;
				const CBox3Dfp32* pPhysBox = GetAbsBoundBox();
				fp32 r = ((pPhysBox->m_Max.k[0] -  pPhysBox->m_Min.k[0]) + (pPhysBox->m_Max.k[1] -  pPhysBox->m_Min.k[1])) / 2.0f;
				fp32 h = pPhysBox->m_Max.k[2] -  pPhysBox->m_Min.k[2];
				if (pMsg->GetTracedDamage(m_iObject, TracePos, r, h, m_pWServer, DmgMsg))
				{
					// Give damage
					bool bWasBreaking = (m_Root.m_Flags & FLAGS_ISBREAKING) != 0;
					m_Root.m_Flags |= FLAGS_ISBREAKING;
					OnDamage(DmgMsg, _Msg.m_iSender, false);
					if (!bWasBreaking)
					{
						m_Root.m_Flags &= ~FLAGS_ISBREAKING;
						FlushDelayedBreak();
						CheckSeparation();
					}

					// Give force
					CWObject_Message Msg;
					Msg.m_Msg = OBJMSG_OBJECT_APPLYVELOCITY;
					Msg.m_iSender = _Msg.m_iSender;
					Msg.m_VecParam0 = DmgMsg.m_Position;
					Msg.m_VecParam1 = DmgMsg.m_Force;
					fp32 mass = GetMass();
					if (mass > pMsg->m_Mass)
					{
						fp32 diff = mass - pMsg->m_Mass;
						fp32 p = M_Pow(0.97f, diff);
						Msg.m_VecParam1 *= p;
						Msg.m_Param0 = pMsg->m_Mass;
					}

					OnMessage(Msg);
				}
			}

			return 0;
		}

	case OBJMSG_OBJECT_APPLYFORCE:
		{
			// Physics test
			const CVec3Dfp32 Position = _Msg.m_VecParam0;
			CVec3Dfp32 Force = _Msg.m_VecParam1 * 20.0f;  //(32.0f * m_pWServer->GetGameTickTime());	// [m/s]
			if (Force.LengthSqr() > 0.01f)
			{
				fp32 Mass = GetMass();
				if (Mass < 5.0f)
					Force *= Mass * (1.0f / 5.0f); // damp so that small objects don't get too much velocity

				DBG_OUT("OBJMSG_OBJECT_APPLYFORCE, Force = %s, Mass = %.2f\n", Force.GetString().Str(), Mass);
				m_pWServer->Phys_AddImpulse(m_iObject, Position, Force);
			}
			ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
			return 0;
		}

	case OBJMSG_OBJECT_APPLYVELOCITY:	
		{
			const CVec3Dfp32 Position = _Msg.m_VecParam0;
			CVec3Dfp32 Velocity = _Msg.m_VecParam1;
//			m_pWServer->Phys_AddImpulse(m_iObject, Position, Velocity, _Msg.m_Param0, 1.0f);
			m_pWServer->Phys_AddMassInvariantImpulse(m_iObject, Position, Velocity);
			ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
			return 0;
		}

	case OBJMSG_OBJECT_BREAK:
		{
			DBG_OUT("break: %s\n", _Msg.m_pData);

			bool bWasBreaking = (m_Root.m_Flags & FLAGS_ISBREAKING) != 0;
			m_Root.m_Flags |= FLAGS_ISBREAKING;
			m_Root.m_Flags |= FLAGS_SAVEPARTS;
		///
			CNameHash NameHash( (const char*)_Msg.m_pData );
			AddDelayedBreak(NameHash, _Msg.m_iSender);
		///
			if (!bWasBreaking)
			{
				m_Root.m_Flags &= ~FLAGS_ISBREAKING;
				FlushDelayedBreak();
				CheckSeparation();
			}
			return 0;
		}
		break;

	case OBJMSG_OBJECT_SPAWN:
		{
			CNameHash NameHash( (const char*)_Msg.m_pData );
			const SObjectModel* pObjectModel = m_Root.FindModel(NameHash, m_VariationName);
			if (pObjectModel)
			{
				// Found in root
				SpawnModel(*pObjectModel);
			}
			else
			{	// Look in children
				foreach (const SPart, child, m_Root.m_lSubParts)
				{
					pObjectModel = child.FindModel(NameHash, m_VariationName);
					if (pObjectModel)
					{
						SpawnModel(*pObjectModel);
						break;
					}
				}
			}
			return 0;
		}
		break;

	case OBJMSG_PUSH_RIGID:
		{
			// A force is applied to target object with direc
			if (_Msg.m_pData)
			{
				static const char* s_lOptions[] = { "SenderSpeed", NULL };

				CFStr Data = (const char*)_Msg.m_pData;
				CFStr DirObjStr = Data.GetStrSep(",");							// Direction of force is taken from this object
				fp32 Speed = Data.GetStrSep(",").Val_fp64();					// Speed (m/s)
				fp32 Elevation = Data.GetStrSep(",").Val_fp64();				// If != 0 this is the angle above horisontal floor
				fp32 Height = Data.GetStrSep(",").Val_fp64();					// Vertical offset from origin
				uint32 SubPartNameHash = Data.GetStrSep(",").StrHash();			// Sub part
				uint Options = Data.GetStrSep(",").TranslateFlags(s_lOptions);

				if (SubPartNameHash && (m_Root.m_Name != SubPartNameHash) && (m_Root.FindPart(SubPartNameHash) < 0))
					return 0; // ignore push if it was targetted for another sub-object

				int iObj = _Msg.m_iSender;
				if (DirObjStr.CompareNoCase("$ACTIVATOR") != 0)
				{
					int iDirObj = m_pWServer->Selection_GetSingleTarget(DirObjStr);
					if (iDirObj > 0)
						iObj = iDirObj;
				}
				CWObject* pPusher = m_pWServer->Object_Get(iObj);
				if (pPusher)
				{
					const CMat4Dfp32& PusherMat = pPusher->GetPositionMatrix();
					CVec3Dfp32 ForceDir = PusherMat.GetRow(0);
					CVec3Dfp32 ForcePos = PusherMat.GetRow(3);
					if (M_Fabs(Elevation) > 0.001f)
					{	// Ignore object pitch and use Elevation angle instead
						ForceDir.k[2] = 0.0f;		// Horizontal
						ForceDir.Normalize();
						ForceDir.k[2] = QSin(Elevation * (_PI2 / 360.0f));
					}
					ForceDir.Normalize();
					if (M_Fabs(Height) > 0.001f)
					{	// Note: Should we try to get hold of local up for a character here?
						ForcePos += PusherMat.GetRow(2) * Height;
					}
					if (Options & M_Bit(0))
					{ // Use sender's speed
						CWObject* pSender = m_pWServer->Object_Get(_Msg.m_iSender);
						if (!pSender) return 0;
						fp32 Velocity = pSender->GetLastPosition().Distance(pSender->GetPosition());  // [units/tick]
						Velocity *= m_pWServer->GetGameTicksPerSecond() * (1.0f / 32.0f);             // [m/s]
						Speed *= Velocity;   // (note: the 'Speed' parameter is used as velocity modifier when 'SenderSpeed' is set)
					}
					else
					{ // Use specified speed value
						if (M_Fabs(Speed) <= 0.001f)
							Speed = 10.0f; // Default speed is 10.0f
					}
#ifndef M_RTM
					m_pWServer->Debug_RenderVector(ForcePos, ForceDir * Speed * 0.1f, 0xffff0000, 5.0f);
					DBG_OUT("Object[%d, %s], OBJMSG_PUSH_RIGID: Pos = %s, Force = %s\n", m_iObject, GetName(), ForcePos.GetString().Str(), (ForceDir * Speed).GetString().Str());
#endif
					m_pWServer->Phys_AddMassInvariantImpulse(m_iObject, ForcePos, ForceDir * Speed);
					// m_pWServer->Phys_AddImpulse(m_iObject, ForcePos, ForceDir * Force, 1.0f, 1.0f);
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
				}
			}
			return 0;
		}
		break;

	case OBJMSG_OBJECT_FIND_DEMONARMATTACHPOINT:
		{
			const CVec3Dfp32& StartPos = _Msg.m_VecParam0;
		//	const CVec3Dfp32& SearchDir = _Msg.m_VecParam1;
			const CMat4Dfp32& ObjMat = GetPositionMatrix();

			int iBest = -1;
			fp32 Best = _FP32_MAX;
			TAP<const CVec3Dfp32> pPoints = m_Root.m_lAttach_DemonArm;
			for (uint i = 0; i < pPoints.Len(); i++)
			{
				CVec3Dfp32 point = pPoints[i];
				point *= ObjMat;

			/*	CVec3Dfp32 ToPoint = point - StartPos;
				fp32 DistanceToPoint = ToPoint.Length();
				fp32 ClosestDistanceToLine = 10000.0f;
				fp32 Proj = ToPoint * SearchDir;
				if (Proj > 3.0f)
					ClosestDistanceToLine = (StartPos + SearchDir * Proj - point).Length();
				ToPoint *= (1.0f / DistanceToPoint);

				fp32 Score = DistanceToPoint * 0.1f + ClosestDistanceToLine; //tweak me
				*/
				fp32 Score = point.DistanceSqr(StartPos);

				if (Score < Best)
				{
					iBest = i;
					Best = Score;
				}
			}

			if (iBest >= 0)
			{
			// 	return (aint)&m_Root.m_lAttachPoints[iBest];  -- disabled because of problems with prefabs / world space attach key positions
				static CVec3Dfp32 Center;
				m_Root.m_PhysPrim.m_BoundBox.GetCenter(Center);
				return (aint)&Center;
			}
			return (aint)NULL;
		}
		break;

	case OBJMSG_OBJECT_GET_GRABATTACHPOINTS:
		{
			const CVec3Dfp32** ppPoints = reinterpret_cast<const CVec3Dfp32**>(_Msg.m_pData);
			if (ppPoints)
				*ppPoints = m_Root.m_lAttach_Grab.GetBasePtr();
			return m_Root.m_lAttach_Grab.Len();
		}
		break;

	case OBJMSG_OBJECT_SETVISIBILITYMASK: // update render visibilitymask
		{
			CNameHash NameHash( (const char*)_Msg.m_pData );
			int iPart = m_Root.FindPart(NameHash);
			if (iPart >= 0)
			{
				SPart& child = m_Root.m_lSubParts[iPart];
				child.m_RenderMask = _Msg.m_Param0;
				UpdateRenderMask();
				m_Root.m_Flags |= FLAGS_SAVEPARTS;
			}
			else
			{
				ConOutL(CStrF("§cf80WARNING: Message SetVisibilityMask, Invalid sub-part: '%s'  (object: %s)", (const char*)_Msg.m_pData, GetName()));
			}
			return 0;
		}
		break;

	case OBJMSG_CHAR_LOCKTOPARENT:
		{
			// Lock ourselves to parent
			if (_Msg.m_pData && _Msg.m_Param0 == 0)
			{
				int iChar = m_pWServer->Selection_GetSingleTarget((char*)_Msg.m_pData);
				// Set exact position to that of our "parent" before making it our real parent
				m_pWServer->Object_SetPosition(m_iObject, m_pWServer->Object_GetPositionMatrix(iChar));
				// Make given character our parent
				m_pWServer->Object_AddChild(iChar, m_iObject);
			}
			return 1;
		}
		break;

	case OBJMSG_CHAR_PLAYANIM: //TODO: remove!
		{
			CWO_Object_ClientData* pCD = AllocClientData(this);
			CFStr OgierStr = (char *)_Msg.m_pData;
			CFStr Container = OgierStr.GetStrSep(":").UpperCase();
			int iAnim = m_pWServer->GetWorldData()->ResourceExistsPartial("ANM:" + Container);
			CWResource* pRes = m_pWServer->GetWorldData()->GetResource(iAnim);
			CXR_Anim_Base* pAnim = (pRes && (pRes->GetClass() == WRESOURCE_CLASS_XSA)) ? safe_cast<CWRes_Anim>(pRes)->GetAnim() : NULL;
			if (pAnim)
			{
				int iSeq = OgierStr.Val_int();
				CXR_Anim_SequenceData* pSeq = pAnim->GetSequence(iSeq);
				if (pSeq)
				{
					// Start animation!
					iAnim0() = iAnim;
					iAnim1() = iSeq;

					// More exact starttime
					if (_Msg.m_Reason == 'CT')
						pCD->m_Anim_StartTime = *((CMTime*)(void*)_Msg.m_Param1);
					else
						pCD->m_Anim_StartTime = m_pWServer->GetGameTime();

					pCD->m_Anim_BaseMat = GetPositionMatrix();
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
					RefreshDirty();
				}
			}
			return 0;
		}
		break;

	case OBJMSG_OBJECT_PLAYANIM:
		{
			CWO_Object_ClientData* pCD = AllocClientData(this);
			CFStr OgierStr = (char *)_Msg.m_pData;
			if (OgierStr.IsEmpty())
			{
				iAnim0() = 0;
				iAnim1() = 0;
				pCD->m_Anim_StartTime = CMTime::CreateInvalid();
				return 1;
			}

			CFStr Container = OgierStr.GetStrSep(":").UpperCase();
			int iAnim = m_pWServer->GetWorldData()->ResourceExistsPartial("ANM:" + Container);
			CWResource* pRes = m_pWServer->GetWorldData()->GetResource(iAnim);
			CXR_Anim_Base* pAnim = (pRes && (pRes->GetClass() == WRESOURCE_CLASS_XSA)) ? safe_cast<CWRes_Anim>(pRes)->GetAnim() : NULL;
			if (pAnim)
			{
				int iSeq = OgierStr.Val_int();
				CXR_Anim_SequenceData* pSeq = pAnim->GetSequence(iSeq);
				if (pSeq)
				{
					// Start animation!
					iAnim0() = iAnim;
					iAnim1() = iSeq;

					// More exact starttime
					if (_Msg.m_Reason == 'CT')
						pCD->m_Anim_StartTime = *((CMTime*)(void*)_Msg.m_Param1);
					else
						pCD->m_Anim_StartTime = m_pWServer->GetGameTime();

					m_Root.m_Flags &= ~FLAGS_ANIM_DELTAMOVE;
					if (_Msg.m_Param0 & 1)
						m_Root.m_Flags |= FLAGS_ANIM_DELTAMOVE;

					ClientFlags() &= ~(CWO_CLIENTFLAGS_NOREFRESH | CWO_CLIENTFLAGS_RENDEREXACT);
					if (_Msg.m_Param0 & 2)
						m_ClientFlags |= CWO_CLIENTFLAGS_RENDEREXACT;

					pCD->m_Anim_BaseMat = GetPositionMatrix();
					RefreshDirty();
					return 1;
				}
			}
			return 0;
		}
		break;

	case OBJMSG_OBJECT_SETDYNAMIC:
		{
			bool bPhysicsControlled = (_Msg.m_Param0 & 1) != 0;
			bool bIgnoreDamage = (_Msg.m_Param0 & 2) != 0;
			DBG_OUT("Object [%d, %s], SetDynamic: %d, IngoreDamage: %d\n", m_iObject, GetName(), bPhysicsControlled, bIgnoreDamage);
			if (bPhysicsControlled && !(m_Root.m_Flags & FLAGS_GAMEPHYSICS))
			{
				// Turn gamephysics on
				m_Root.m_Flags |= FLAGS_GAMEPHYSICS;

				if (!(GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_PHYSICSCONTROLLED))
				{
					// First time. do new ObjectSpawn()
					DBG_OUT("- (1) ObjectSpawn(true);\n");
					ObjectSpawn(true);
				}
				else
				{
					// Just activate the object
					m_pWServer->Object_SetPhysics_ObjectFlags(m_iObject, GetPhysState().m_ObjectFlags | OBJECT_PHYSFLAGS_PHYSMOVEMENT);
					m_pWServer->Object_InitRigidBody(m_iObject, (m_Root.m_Flags & FLAGS_DEBRIS) != 0);
					DBG_OUT("- (2) Object_InitRigidBody;\n");
				}
			}
			else if (!bPhysicsControlled && (m_Root.m_Flags & FLAGS_GAMEPHYSICS))
			{
				// Turn gamephysics off
				m_Root.m_Flags &= ~FLAGS_GAMEPHYSICS;
				m_pWServer->Object_SetPhysics_ObjectFlags(m_iObject, GetPhysState().m_ObjectFlags & ~OBJECT_PHYSFLAGS_PHYSMOVEMENT);
				m_pWServer->Object_InitRigidBody(m_iObject, (m_Root.m_Flags & FLAGS_DEBRIS) != 0);
				DBG_OUT("- (3) Object_InitRigidBody;\n");
			}

			m_Root.m_Flags &= ~FLAGS_IGNORE_DAMAGE;
			if (bIgnoreDamage)
				m_Root.m_Flags |= FLAGS_IGNORE_DAMAGE;

			return 1;
		}
		break;


	case OBJMSG_GETSPAWNMASK:
		{
			return m_SpawnMask;
		}
		break;

	case OBJMSG_OBJECT_SWITCH_AUTOAIM:
		{
			if (_Msg.m_Param0)
				m_Root.m_Flags |= FLAGS_AUTOAIM;
			else
				m_Root.m_Flags &= ~FLAGS_AUTOAIM;
			return 1;
		}
		break;

	case OBJMSG_OBJECT_DEMONARM_GRABBED:
		{
			m_Root.m_Messages_OnDemonArmGrab.Send(_Msg.m_Param1, *this); // Activator is assumed to be param1
			return 1;
		}
		break;

	case OBJMSG_OBJECT_DEMONARM_RELEASED:
		{
			m_Root.m_Messages_OnDemonArmRelease.Send(_Msg.m_Param1, *this); // Activator is assumed to be param1
			return 1;
		}
		break;

	case OBJMSG_PHYSICS_KILL:
		{
			// This is sent when the player desperately tries to spawn at place obstructed by objects
			ConOutL(CStrF("Object %s (index: %d, position: %s) was brutually destroyed from PHYSICS_KILL message (telefrag?)", GetName(), m_iObject, GetPosition().GetFilteredString().Str()));
			m_pWServer->Object_Destroy(m_iObject);
			return 1;
		}
		break;

	//
	// Precache-messages:
	//
	case OBJSYSMSG_PRECACHEMESSAGE:
		{
			CWObject_Message *pMsg = (CWObject_Message *)_Msg.m_pData;
			switch (pMsg->m_Msg)
			{
			case OBJMSG_CHAR_PLAYANIM:
			case OBJMSG_OBJECT_PLAYANIM:
				{
					CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
					if (!pServerMod)
						return 0;

					CFStr St = (char *)pMsg->m_pData;
					if (!St.IsEmpty())
					{
						CFStr Container = St.GetStrSep(":");
						int iSeq = St.Val_int();
						pServerMod->AddAnimContainerEntry(Container, iSeq);
					}
				}
				return 1;
			}
		}
		break;
	}

	return parent::OnMessage(_Msg);
}


void CWObject_Object::OnCollision(CWObject* _pOther, const CVec3Dfp32& _Pos, fp32 _Impact)
{
	ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
	//DBG_OUT("OnCollision! f = %f\n", _Impact);

	//
	// Play impact sound
	// 
	if (!m_Root.m_Sounds.IsEmpty())
	{
		aint x = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETSOUNDGROUPMANAGER), m_pWServer->Game_GetObjectIndex());
		CWO_SoundGroupManager* pMan = (CWO_SoundGroupManager*)x;
		if (pMan)
		{
			uint iSlot;
			int16 iSound;
			fp32 Volume;
			if (pMan->GetImpactSound(m_Root.m_Sounds.m_iSoundGroup, _Impact, &iSlot, &iSound, &Volume))
			{
				int nLastPlayed = m_Root.m_Sounds.m_anLastPlayed[iSlot];
				int nCurrTick = m_pWServer->GetGameTick();
				if (nLastPlayed == 0 || ((nCurrTick - nLastPlayed) > 10))
				{
					m_pWServer->Sound_At(_Pos, iSound, WCLIENT_ATTENUATION_3D, 0, Volume);
					m_Root.m_Sounds.m_anLastPlayed[iSlot] = nCurrTick;
				}
			}
		}
	}

	// Check if we're burning
	CWO_Object_ClientData* pCD = GetClientData(this);
	if (pCD && _pOther) // (if client data doesn't exist we can be sure that the object isn't burning)
	{
		if (pCD->m_Burnable.m_Value.IsBurning())
		{
			// Put other object on fire
			CWO_DamageMsg FireDmg(0, DAMAGETYPE_FIRE, &_Pos);
			FireDmg.Send(_pOther->m_iObject, m_iObject, m_pWServer);
		}
	}

	//
	// Give damage
	// 
	uint nDamage = uint(_Impact * 0.5f);   //TODO: tweak damage/impact factor
	if (nDamage > 5)
	{
		CWO_DamageMsg Dmg(nDamage, 0, &_Pos);
		Dmg.Send(m_iObject, _pOther ? _pOther->m_iObject : 0, m_pWServer);
	}

	if(m_iPhysEP != -1)
	{
		CWObject_Message Msg(OBJMSG_ENGINEPATH_CHECKFALLOFF);
		Msg.m_VecParam0.k[0] = _Impact;
		if(m_pWServer->Message_SendToObject(Msg, m_iPhysEP))
			m_iPhysEP = -1;
	}
}


bool CWObject_Object::GetAnimLayer(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, const CMTime& _GlobalTime, CXR_AnimLayer& _Dest)
{
	//TODO: Cache pointer to the anim sequence (perhaps in m_lspClientObj[2]?)
	CXR_Anim_SequenceData* pSeq;
	{
		int iAnim = _pObj->m_iAnim0;
		int iSeq = _pObj->m_iAnim1;

		CWResource* pAnimContainerResource = _pWPhysState->GetWorldData()->GetResource(iAnim);
		M_ASSERTHANDLER(pAnimContainerResource && pAnimContainerResource->GetClass() == WRESOURCE_CLASS_XSA, "no anim resource!", return false); // shouldn't get a valid iAnim0 for invalid resource
		CWRes_Anim* pAnimRes = (CWRes_Anim*)pAnimContainerResource;
		//pAnimRes->m_TouchTime = _pMapData->m_TouchTime; // Added by Anton

		CXR_Anim_Base* pAnimBase = pAnimRes->GetAnim();
		M_ASSERTHANDLER(pAnimBase, "no anim resource!", return false); // shouldn't get a valid iAnim0 for invalid resource

		pSeq = (CXR_Anim_SequenceData*)pAnimBase->GetSequence(iSeq);
		M_ASSERTHANDLER(pSeq, "no anim sequence!", return false);  // shouldn't get a valid iAnim1 for invalid sequence
	}

	CWO_Object_ClientData* pCD = GetClientData(_pObj);
	M_ASSERTHANDLER(pCD, "ERROR: No client data in Object! (client data is allocated when starting animation...)", return false);

	CMTime t, StartTime = pCD->m_Anim_StartTime;
	if (StartTime.IsInvalid())
		t = CMTime::CreateFromSeconds( pSeq->GetDuration() );
	else
		t = pSeq->GetLoopedTime(_GlobalTime - StartTime);

	_Dest.Create3(pSeq, t, 1.0f, 1.0f, 0);
	return true;
}


static MRTC_CriticalSection s_AnimLock; // TEMP 

CXR_SkeletonInstance* CWObject_Object::EvalAnim(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Skeleton*& _pSkel)
{
	M_LOCK(s_AnimLock);

	CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	_pSkel = pModel ? pModel->GetSkeleton() : NULL;

	if (!_pSkel)
		return NULL;

	if (!_pObj->m_iAnim0)
		return NULL;

	CXR_AnimLayer Layer;
	if (!GetAnimLayer(_pObj, _pWClient, _pWClient->GetRenderTime(), Layer))
		return NULL;

	CReferenceCount* p = _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_SKELINSTANCE];
	CXR_SkeletonInstance* pSkelInstance = safe_cast<CXR_SkeletonInstance>(p);
	if (!pSkelInstance)
	{
		pSkelInstance = MNew(CXR_SkeletonInstance);
		pSkelInstance->Create(_pSkel->m_lNodes.Len());
		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_SKELINSTANCE] = pSkelInstance;
	}

	fp32 IPTime = _pWClient->GetRenderTickFrac();
	CMat4Dfp32 MatIP;
	if (_pObj->GetParent() && _pObj->m_iParentAttach >= 0)
	{
		CWObject_Message Msg(OBJMSG_HOOK_GETCURRENTMATRIX, aint(&MatIP), _pObj->m_iParentAttach);
		if (_pWClient->ClientMessage_SendToObject(Msg, _pObj->GetParent()))
		{
			M_VMatMul(_pObj->GetLocalPositionMatrix(), MatIP, MatIP);
			goto matrix_ok;
		}
	}
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);
matrix_ok:
	_pSkel->EvalAnim(&Layer, 1, pSkelInstance, MatIP);
	return pSkelInstance;
}


void CWObject_Object::DoRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, CXR_AnimState& _Anim, uint32 _RenderFlags)
{
	MSCOPESHORT(CWObject_Object::DoRender);

	bint bAnything = 0;
	CXR_Model* lpModels[CWO_NUMMODELINDICES];
	for (uint i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
		lpModels[i] = pModel;
		bAnything |= aint(pModel);
	}

	if (!bAnything)
		return;

	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetRenderTickFrac();

	int iParentAttach = _pObj->m_iParentAttach;
	if (_pObj->GetParent() && iParentAttach >= 0)
	{
		CWObject_Message Msg(OBJMSG_HOOK_GETCURRENTMATRIX, aint(&MatIP), iParentAttach);
		if (_pWClient->ClientMessage_SendToObject(Msg, _pObj->GetParent()))
		{
			M_VMatMul(_pObj->GetLocalPositionMatrix(), MatIP, MatIP);
			goto matrix_ok;
		}
	} //else not ok
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);
matrix_ok:

	_Anim.m_Data[3] = ~(_pObj->m_Data[DATA_OBJ_MODELFLAGS]);						// Modelflags
	_Anim.m_SurfaceOcclusionMask = ~_pObj->m_Data[DATA_OBJ_VISIBILITYMASK];			// VisibilityMask;
	_Anim.m_SurfaceShadowOcclusionMask = ~_pObj->m_Data[DATA_OBJ_VISIBILITYMASK];	//   - " -
	_Anim.m_GUID = _pObj->m_iObject;

	const CWO_Object_ClientData* pCD = GetClientData(_pObj);
	M_ASSERTHANDLER(lpModels[0], "ERROR: Do not set models on model slot 1,2 without setting model slot 0!!", return);

	// Eval animation
	CXR_Skeleton* pSkel = lpModels[0] ? lpModels[0]->GetSkeleton() : NULL;
	if (pSkel && _pObj->m_iAnim0)
	{
		CXR_AnimLayer Layer;
		if (GetAnimLayer(_pObj, _pWClient, _pWClient->GetRenderTime(), Layer))
		{
			M_ASSERTHANDLER(pCD, "playing anim without clientdata?!", return);
			if (_pObj->m_ClientFlags & CWO_CLIENTFLAGS_RENDEREXACT)
			{
				// Calculate animated position
				VecUnion AnimPos;
				CQuatfp32 AnimRot;
				Layer.m_spSequence->EvalTrack0(CMTime::CreateFromSeconds(Layer.m_Time), AnimPos.v128, AnimRot);

				AnimRot.CreateMatrix(MatIP);
				MatIP.GetRow(3) = AnimPos.v3;
				M_VMatMul(MatIP, pCD->m_Anim_BaseMat, MatIP);
			}

			_Anim.m_pSkeletonInst = CXR_VBMHelper::Alloc_SkeletonInst(_pEngine->GetVBM(), pSkel->m_lNodes.Len(), pSkel->m_nUsedRotations, pSkel->m_nUsedMovements);
			if (!_Anim.m_pSkeletonInst)
				return;

			CMat4Dfp32 AnimPos = MatIP;
			pSkel->EvalAnim(&Layer, 1, _Anim.m_pSkeletonInst, AnimPos);
		}
	}

	// Use joint objects		TODO: combine this with animation support!
	if (pSkel && pCD && pCD->m_liJointObjects.Len())
	{
		M_ASSERTHANDLER(_Anim.m_pSkeletonInst == NULL, "No support for joint objects combined with animation", return);

		_Anim.m_pSkeletonInst = CXR_VBMHelper::Alloc_SkeletonInst(_pEngine->GetVBM(), pSkel->m_lNodes.Len(), pSkel->m_nUsedRotations, pSkel->m_nUsedMovements);
		if (!_Anim.m_pSkeletonInst)
			return;

		CMat4Dfp32* pBoneTransform = _Anim.m_pSkeletonInst->m_pBoneTransform;
		uint nBoneTransform = _Anim.m_pSkeletonInst->m_nBoneTransform;

		TAP<const uint32> pJointObjects = pCD->m_liJointObjects;
		for (uint j = 0; j < pJointObjects.Len(); j++)
		{
			uint iObj = (pJointObjects[j] >> 16) & 0xffff;
			uint iJoint = pJointObjects[j] & 0xffff;

			CWObject_CoreData* pJointObj = _pWClient->Object_GetCD(iObj);
			if (pJointObj && (iJoint < nBoneTransform))
				pBoneTransform[iJoint] = pJointObj->GetPositionMatrix();
		}
	}

	// Render burning effects
	if (pCD)
	{
		const CWO_Burnable& Burnable = pCD->m_Burnable.m_Value;
		const CBox3Dfp32* pBoxes = pCD->m_lPhysBoundBox.GetBasePtr();
		const uint32 nBoxes = pCD->m_lPhysBoundBox.Len();

		if (pSkel && _Anim.m_pSkeletonInst)
			Burnable.OnClientRender(_pObj, _pWClient, MatIP, _pEngine, pSkel, _Anim.m_pSkeletonInst, NULL, NULL);
		else
			Burnable.OnClientRender(_pObj, _pWClient, MatIP, _pEngine, NULL, NULL, pBoxes, &nBoxes);

		// Add scorching layer to object
		if (Burnable.IsBurned())
		{
			_Anim.m_lpSurfaces[0] = _pEngine->m_pSC->GetSurface("burnscorch");
			_RenderFlags |= CXR_MODEL_ONRENDERFLAGS_SURF0_ADD;
			_Anim.m_AnimTime1 = Burnable.GetBurnTime(_pWClient);
		}
	}

	// Render all models
	if (!(_pObj->m_ClientFlags & CWO_CLIENTFLAGS_NORENDER))
	{
		// CBox3Dfp32 VisBox;
		// _pObj->GetVisBoundBox(VisBox);
		// _pWClient->Debug_RenderOBB(MatIP, VisBox, 0xff00ff00, 0.0f, false);

		for (uint i = 0; i < CWO_NUMMODELINDICES; i++)
		{
			if (lpModels[i])
			{
				_Anim.m_pModelInstance = _pObj->m_lModelInstances[i];
				_pEngine->Render_AddModel(lpModels[i], MatIP, _Anim, XR_MODEL_STANDARD, _RenderFlags);
			}
		}
	}
}


void CWObject_Object::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _RenderMat)
{
	MAUTOSTRIP(CWObject_Object_OnClientRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Object::OnClientRender);

	CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
	AnimState.m_AnimTime1 = CMTime::CreateFromSeconds(0.0f);

	uint LightIndex = _pObj->m_Data[DATA_LAMP_LIGHTINDEX];
	if (LightIndex)
	{
		// Non-lamp object with lightindex - implies no shadow should be cast from object
		uint LightGUID = _pEngine->m_pSceneGraphInstance->SceneGraph_Light_GetGUID(LightIndex);
		AnimState.m_NoShadowLightGUID = LightGUID;
	}

	AnimState.m_Data[1] = 0; // usercolor (for lamps)

	DoRender(_pObj, _pWClient, _pEngine, AnimState);
//	_pWClient->Debug_RenderAABB(*_pObj->GetAbsBoundBox(), 0xffffffff, 0.06f, true);
}


bool CWObject_Object::Intersect(const SPhysBase& _Phys, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CCollisionInfo& _CollInfo) const
{
	CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(_Phys.m_PhysPrim.m_iPhysModel);
	if (!pModel) return false;
	CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();
	M_ASSERTHANDLER(pPhysModel, "Bad model used as physmodel!", return false);

	CXR_PhysicsContext PhysContext(GetPositionMatrix(), NULL);
	PhysContext.m_PhysGroupMaskThis = _Phys.m_PhysPrim.m_VisibilityMask;
	pPhysModel->Phys_Init(&PhysContext);

	int MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
	return pPhysModel->Phys_IntersectLine(&PhysContext, _p0, _p1, MediumFlags, &_CollInfo);
}


void CWObject_Object::OnDamage(const CWO_DamageMsg& _Damage, int _iSender, bool _bNoForce)
{
	DBG_OUT("CWObject_Object::OnDamage[iObj: %d, name: %s], Damage: %d, iSender: %d (%s)\n", m_iObject, GetName(), _Damage.m_Damage, _iSender, m_pWServer->Object_GetName(_iSender));

	m_Root.m_Flags |= FLAGS_SAVEPARTS;

	bool bWasBreaking = (m_Root.m_Flags & FLAGS_ISBREAKING) != 0;
	if (!bWasBreaking)
	{
		m_ImpactPos = _Damage.m_Position;
		m_ImpactForce = _Damage.m_Force;
	}

	// Check which sub-part or trigger was hit
	uint nParts = m_Root.m_lSubParts.Len();
	uint nTriggers = m_Root.m_lTriggers.Len();
	if ((nParts || nTriggers) && _Damage.m_pCInfo)
	{
		// Run OnDamage messages for main object aswell
		m_Root.m_Messages_OnDamage.Send(_iSender, *this, _Damage.m_Damage);

		CCollisionInfo CInfo, Tmp;
		CInfo.CopyParams(*_Damage.m_pCInfo);

		CVec3Dfp32 p0 = _Damage.m_Position;
		CVec3Dfp32 p1 = _Damage.m_Force;
		p1.SetLength(16.0f);
		p0 -= p1;
		p1 *= 2;
		p1 += p0;

		for (uint i = 0; i < nTriggers; i++)
		{
			STrigger& Trigger = m_Root.m_lTriggers[i];
			if (Trigger.m_DamageTypes & _Damage.m_DamageType)
			{
				Tmp.Clear();
				Tmp.CopyParams(CInfo);
				if (Intersect(Trigger, p0, p1, Tmp))
				{
					Tmp.m_iObject = i | 0x8000;
					CInfo.Improve(Tmp);
				}
			}
		}

		for (uint i = 0; i < nParts; i++)
		{
			Tmp.Clear();
			Tmp.CopyParams(CInfo);
			if (Intersect(m_Root.m_lSubParts[i], p0, p1, Tmp))
			{
				Tmp.m_iObject = i;
				CInfo.Improve(Tmp);
			}
		}

		if (CInfo.m_bIsValid)
		{
			CWO_DamageMsg Damage2 = _Damage;
			Damage2.m_pCInfo = &CInfo;

			if (CInfo.m_iObject & 0x8000)
			{
				int iTrigger = CInfo.m_iObject & 0xfff;
				STrigger* pTrigger = &m_Root.m_lTriggers[iTrigger];
//				uint32 NameHash = pTrigger->m_Name;

				DBG_OUT("Hit trigger %d (%s)\n", iTrigger, pTrigger->m_Name.DbgName().Str());
				OnDamage(*pTrigger, Damage2, _iSender);
			}
			else
			{
				int iPart = CInfo.m_iObject & 0xfff;
				SPart* pSubPart = &m_Root.m_lSubParts[iPart];
//				uint32 NameHash = pSubPart->m_Name;

				DBG_OUT("Hit sub-part %d (%s)\n", iPart, pSubPart->m_Name.DbgName().Str());
				m_iImpactPart = iPart;
				OnDamage(*pSubPart, Damage2, _iSender);
			}
		}
	}
	else
	{
		// Well, then we hit the root part
		OnDamage(m_Root, _Damage, _iSender);

		if (_Damage.m_pCInfo == NULL)
		{
			// Give some damage to all constraints
			foreach (SConstraint, c, m_Root.m_lConstraints)
				OnDamage(c, _Damage, _iSender);

			// Give damage to all subparts
			foreach (SPart, p, m_Root.m_lSubParts)
				OnDamage(p, _Damage, _iSender);

			// Give damage to all triggers
			foreach (STrigger, t, m_Root.m_lTriggers)
			{
				if (t.m_DamageTypes & _Damage.m_DamageType)
					OnDamage(t, _Damage, _iSender);
			}
		}
	}

	// Set object on fire during fire damage
	if (_Damage.m_DamageType & DAMAGETYPE_FIRE)
	{
		CWO_Object_ClientData* pCD = AllocClientData(this);
		CWO_Burnable& Burnable = pCD->m_Burnable.m_Value;
		if (!Burnable.IsBurning() && Burnable.SetBurning(true, m_pWServer))
		{
			ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
			pCD->m_Burnable.MakeDirty();
		}
	}

	// Give physics impact
	if (_bNoForce == false)
	{
		CWObject_Message Msg(OBJMSG_OBJECT_APPLYFORCE);
		Msg.m_VecParam0 = _Damage.m_Position;
		Msg.m_VecParam1 = _Damage.m_Force * 2.0f;
		OnMessage(Msg);
	}


	if (!bWasBreaking && (m_Root.m_Flags & FLAGS_ISBREAKING))
	{
		m_Root.m_Flags &= ~FLAGS_ISBREAKING;
		FlushDelayedBreak();
		CheckSeparation();
	}
}


bool CWObject_Object::OnDamage(SBreakableBase& _PhysObj, const CWO_DamageMsg& _Damage, int _iSender)
{
	if (_PhysObj.m_DamageInfo.GiveDamage(_Damage.m_Damage))
	{
		DBG_OUT("Sending OnDamage msgs 2\n");

		_PhysObj.m_Messages_OnDamage.Send(_iSender, *this, _Damage.m_Damage);
		return true;		
	}
	return false;
}


bool CWObject_Object::OnDamage(SPart& _Part, const CWO_DamageMsg& _Damage, int _iSender)
{
DBG_OUT("  OnDamage, Part:%s  (%d)\n", _Part.m_Name.DbgName().Str(), _Part.m_DamageInfo.m_nHitPoints);
	uint32 NameHash = _Part.m_Name;

	bool bRootObj = (&_Part == &m_Root);

	int iPart = m_Root.FindPart(NameHash);
	M_ASSERT(bRootObj || iPart >= 0, "OnDamage, part is not m_Root or child of m_Root!");

	if (_Damage.m_DamageType & (DAMAGETYPE_PISTOL | DAMAGETYPE_RIFLE) &&  (m_ImpactSoundTick < m_pWServer->GetGameTick() - 2))
	{
		m_ImpactSoundTick = m_pWServer->GetGameTick();
		// Play projectile impact sound
		const CVec3Dfp32& Pos = _Damage.m_Position;

		CWO_SoundGroupManager* pMan = (CWO_SoundGroupManager*)m_pWServer->Message_SendToObject(
				CWObject_Message(OBJMSG_GAME_GETSOUNDGROUPMANAGER), m_pWServer->Game_GetObjectIndex());
		if (pMan)
		{
			int iSound = pMan->GetSound(_Part.m_Sounds.m_iSoundGroup, CWO_SoundGroupManager::SOUND_IMPACT_PROJECTILE);
			if (iSound <= 0)
				iSound = pMan->GetSound(m_Root.m_Sounds.m_iSoundGroup, CWO_SoundGroupManager::SOUND_IMPACT_PROJECTILE);
			if (iSound > 0)
				m_pWServer->Sound_At(_Damage.m_Position, iSound, WCLIENT_ATTENUATION_3D, 0, 1.0f);
		}
	}

	// Give damage to constraints connected with this part
	if (_Part.m_Flags & FLAGS_BREAKCONSTRAINTS)
	{
		for (int i = 0; i < m_Root.m_lConstraints.Len(); i++)
		{
			SConstraint& c = m_Root.m_lConstraints[i];
			if (bRootObj || c.m_iObj0 == iPart || c.m_iObj1 == iPart)
			{ // Give damage to constraint                   -- TODO: Distribute damage amount over constraints? 
				OnDamage(c, _Damage, _iSender);
			}
		}
	}

	// Perform OnDamage stuff (if the subpart still exists)
	iPart = m_Root.FindPart(NameHash);
	SPart* pSubPart = bRootObj ? &m_Root : (iPart >= 0 ? &m_Root.m_lSubParts[iPart] : NULL);
	if (pSubPart)
	{
		if (OnDamage((SBreakableBase&)*pSubPart, _Damage, _iSender))
		{
			iPart = m_Root.FindPart(NameHash);
			pSubPart = bRootObj ? &m_Root : (iPart >= 0 ? &m_Root.m_lSubParts[iPart] : NULL);
			if (pSubPart)
			{
				if (_Damage.m_bPositionValid && _Damage.m_pCInfo)
				{
					// Spawn impact models
					foreach (const CNameHash, NameHash, pSubPart->m_lImpactModels)
					{
						const SObjectModel* pModel = _Part.FindModel(NameHash, m_VariationName);
						if (!pModel)
							pModel = m_Root.FindModel(NameHash, m_VariationName);

						if (pModel)
						{
							CMat4Dfp32 SpawnMat;
							GetImpactMatrix(*pModel, _Damage, SpawnMat);
							SpawnModel(*pModel, &SpawnMat);
						}
					}
				}

				// Check to see if object should be destroyed
				if (pSubPart->m_DamageInfo.IsDestroyed())
				{
					m_Root.m_Flags |= FLAGS_ISBREAKING;
					if (bRootObj)
						OnDestroy(*pSubPart, _iSender);
					else
						AddDelayedBreak(pSubPart->m_Name, _iSender);
				}
			}
			return true;
		}
	}
	return false;
}


bool CWObject_Object::OnDamage(STrigger& _Trigger, const CWO_DamageMsg& _Damage, int _iSender)
{
	uint32 NameHash = _Trigger.m_Name;

	if (OnDamage((SBreakableBase&)_Trigger, _Damage, _iSender))
	{
		int iTrigger = m_Root.FindTrigger(NameHash);
		if (iTrigger >= 0)
		{
			// Check to see if trigger should be destroyed
			STrigger* pTrigger = &m_Root.m_lTriggers[iTrigger];
			if (pTrigger->m_DamageInfo.IsDestroyed())
			{
				m_Root.m_Flags |= FLAGS_ISBREAKING;
				AddDelayedBreak(pTrigger->m_Name, _iSender);
			}
		}
		return true;
	}
	return false;
}



bool CWObject_Object::OnDamage(SConstraint& _Constraint, const CWO_DamageMsg& _Damage, int _iSender)
{
DBG_OUT("  OnDamage, Constraint: %s (%d, %d)\n", _Constraint.m_Name.DbgName().Str(), _Damage.m_Damage, _Constraint.m_DamageInfo.m_nHitPoints);
	uint32 NameHash = _Constraint.m_Name;

	if (OnDamage((SBreakableBase&)_Constraint, _Damage, _iSender))
	{
		int iConstraint = m_Root.FindConstraint(NameHash);
		if (iConstraint >= 0)
		{
			SConstraint* pConstraint = &m_Root.m_lConstraints[iConstraint];
			if (pConstraint->m_DamageInfo.IsDestroyed())
			{
				m_Root.m_Flags |= FLAGS_ISBREAKING;
			//	OnBreak(*pConstraint, _iSender);
				AddDelayedBreak(pConstraint->m_Name, _iSender);
			}
		}
		return true;
	}
	return false;
}


void CWObject_Object::OnDestroy(SBreakableBase& _PhysObj, int _iSender)
{
	_PhysObj.m_Messages_OnBreak.Send(_iSender, *this);
}


void CWObject_Object::OnDestroy(STrigger& _Trigger, int _iSender)
{
DBG_OUT("  OnDestroy, Trigger: %s\n", _Trigger.m_Name.DbgName().Str());
	uint32 NameHash = _Trigger.m_Name;

	// Run OnDestroy messages
	OnDestroy((SBreakableBase&)_Trigger, _iSender);

	// Remove trigger  (might already be removed because of OnDestroy-messages)
	int iTrigger = m_Root.FindTrigger(NameHash);
	if (iTrigger >= 0)
		m_Root.DeleteTrigger(iTrigger);
}


void CWObject_Object::OnDestroy(SPart& _Part, int _iSender)
{
DBG_OUT("  OnDestroy, Part: %s\n", _Part.m_Name.DbgName().Str());
	m_Root.m_Flags |= FLAGS_BROKEN;
	uint32 NameHash = _Part.m_Name;

	// Play 'break' sound
	CWO_SoundGroupManager* pMan = (CWO_SoundGroupManager*)m_pWServer->Message_SendToObject(
			CWObject_Message(OBJMSG_GAME_GETSOUNDGROUPMANAGER), m_pWServer->Game_GetObjectIndex());
	if (pMan)
	{
		int iSound = pMan->GetSound(_Part.m_Sounds.m_iSoundGroup, CWO_SoundGroupManager::SOUND_DESTROY);
		if (iSound <= 0)
			iSound = pMan->GetSound(m_Root.m_Sounds.m_iSoundGroup, CWO_SoundGroupManager::SOUND_DESTROY);
		if (iSound > 0)
		{
			DBG_OUT("  - playing sound '%d'\n", iSound);
			CVec3Dfp32 Pos;
			_Part.m_PhysPrim.m_BoundBox.GetCenter(Pos);
			Pos *= GetPositionMatrix();
			m_pWServer->Sound_At(Pos, iSound, WCLIENT_ATTENUATION_3D, 0, 1.0f);
		}
	}

	// Spawn 'Destroy model'
	for (uint i = 0; i < _Part.m_lDestroyModels.Len(); i++)
	{
		DBG_OUT("  - model: '%s'\n", _Part.m_lDestroyModels[i].DbgName().Str());
		const SObjectModel* pModel = _Part.FindModel(_Part.m_lDestroyModels[i], m_VariationName);
		if (pModel)
		{
			SpawnModel(*pModel);
		}
		else
		{
			// No model found in subpart, check for 'generic' model in root
			pModel = m_Root.FindModel(_Part.m_lDestroyModels[i], m_VariationName);
			if (pModel)
			{
				// Calculate spawnmat from part boundbox center
				CVec3Dfp32 Center;
				_Part.m_PhysPrim.m_BoundBox.GetCenter(Center);

				CMat4Dfp32 SpawnMat = GetPositionMatrix();
				SpawnMat.GetRow(3) = Center * SpawnMat;

				SpawnModel(*pModel, &SpawnMat);
			}
		}
	}

	// Run OnDestroy messages
	OnDestroy((SBreakableBase&)_Part, _iSender);

	// Remove part  (might already be removed because of OnDestroy-messages)
	int iPart = m_Root.FindPart(NameHash);
	if (iPart >= 0)
		m_Root.DeletePart(iPart, *this);
}


void CWObject_Object::OnBreak(SConstraint& _Constraint, int _iSender)
{
DBG_OUT("  OnBreak, Constraint: %s\n", _Constraint.m_Name.DbgName().Str());
	uint32 NameHash = _Constraint.m_Name;

	// Set 'breaking'-flag
	m_Root.m_Flags |= FLAGS_ISBREAKING;

	// Spawn break effect
	if (_Constraint.m_iModel[0])
		SpawnModel(_Constraint);

	// Remove constraint from physics engine
	if (_Constraint.m_iPhysConstraint >= 0)
	{
		DeletePhysConstraint(_Constraint.m_iPhysConstraint);
		_Constraint.m_iPhysConstraint = -1;
	}

	// Run OnBreak messages
	OnDestroy((SBreakableBase&)_Constraint, _iSender);

	// Delete constraint  (if not already gone)
	int iConstraint = m_Root.FindConstraint(NameHash);
	if (iConstraint >= 0)
		m_Root.DeleteConstraint(iConstraint);

	// We might have deleted all rigid constraints
	CWO_PhysicsState PhysState(GetPhysState());
	if (CheckIfMobile(PhysState))
	{
		m_pWServer->Object_SetPhysics(m_iObject, PhysState);
		m_pWServer->Object_InitRigidBody(m_iObject, (m_Root.m_Flags & FLAGS_DEBRIS) != 0);
	}
}



void CWObject_Object::OnDeltaSave(CCFile* _pFile)
{
	parent::OnDeltaSave(_pFile);

	CWO_Object_ClientData* pCD = GetClientData(this);
	if (pCD)
		m_Root.m_Flags |= FLAGS_HAS_CLIENTDATA;	//TODO: set this flag when allocating client data?

	// Make sure object doesn't spawn in stationary state
	if ((m_Root.m_Flags & FLAGS_GAMEPHYSICS) && !m_pWServer->Phys_IsStationary(m_iObject))
		m_Root.m_Flags |= FLAGS_SPAWNACTIVE;

	// Make sure objects with constraints are saved...
	foreach (const SConstraint, c, m_Root.m_lConstraints)
		if (c.m_iPhysConstraint >= 0)
		{
			m_Root.m_Flags |= FLAGS_SAVEPARTS;
			break;
		}

	// if we are parented, save position
	_pFile->WriteLE(m_Root.m_Flags);
	if (m_Root.m_Flags & (FLAGS_SAVEPOSITION|FLAGS_HASPARENT))
	{
		if (m_Root.m_Flags & FLAGS_HASPARENT)
		{
			int32 ParentGUID = m_pWServer->Object_GetGUID(m_iObjectParent);
			_pFile->WriteLE(ParentGUID);
			_pFile->WriteLE(int8(m_iParentAttach));
		}
		m_PhysAbsBoundBox.Write(_pFile);
		m_PhysVelocity.Write(_pFile);
		m_VisBoxMin.Write(_pFile);
		m_VisBoxMax.Write(_pFile);
		m_LocalPos.Write(_pFile);
		m_Pos.Write(_pFile);
		m_LastPos.Write(_pFile);
	}

	if (m_Root.m_Flags & FLAGS_SAVEPARTS)
		m_Root.Write(_pFile);

	if (m_Root.m_Flags & FLAGS_SAVELIFETIME)
		_pFile->WriteLE(m_LifetimeEnd);

	if (pCD)
		pCD->AutoVar_Write(_pFile, m_pWServer->GetMapData());
}


void CWObject_Object::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	parent::OnDeltaLoad(_pFile, _Flags);

	_pFile->ReadLE(m_Root.m_Flags);

	if (m_Root.m_Flags & (FLAGS_SAVEPOSITION | FLAGS_SAVEPARTS))
		DBG_OUT("CWObject_Object::OnDeltaLoad, loading extra data for object %d (%s)\n", m_iObject, GetName());

	if (m_Root.m_Flags & (FLAGS_SAVEPOSITION|FLAGS_HASPARENT))
	{
		if (m_Root.m_Flags & FLAGS_HASPARENT)
		{
			int32 iObjParentGUID;
			_pFile->ReadLE(iObjParentGUID);
			int8 Temp;
			_pFile->ReadLE(Temp);
			m_iParentAttach = Temp;
			// parent away!
			m_pWServer->Object_AddChild(m_pWServer->Object_GetIndex(iObjParentGUID), m_iObject);
		}
		m_PhysAbsBoundBox.Read(_pFile);
		m_PhysVelocity.Read(_pFile);
		m_VisBoxMin.Read(_pFile);
		m_VisBoxMax.Read(_pFile);
		m_LocalPos.Read(_pFile);
		m_Pos.Read(_pFile);
		m_LastPos.Read(_pFile);
	}

	if (m_Root.m_Flags & FLAGS_SAVEPARTS)
		m_Root.Read(_pFile, *this);

	for (uint8 i = 0; i < CWO_NUMMODELINDICES; i++)
		m_Root.m_iModel[i] = m_iModel[i];

	ObjectSpawn(!(m_Root.m_Flags & FLAGS_WAITSPAWN));

	if (m_Root.m_Flags & FLAGS_SAVELIFETIME)
		_pFile->ReadLE(m_LifetimeEnd);

	if (m_Root.m_Flags & FLAGS_HAS_CLIENTDATA)
	{
		CWO_Object_ClientData* pCD = AllocClientData(this);
		pCD->AutoVar_Read(_pFile, m_pWServer->GetMapData());
	}

	ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
}


void CWObject_Object::OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWClient, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	parent::OnClientLoad(_pObj, _pWClient, _pFile, _pWData, _Flags);

	uint8 Flags;
	_pFile->ReadLE(Flags);

	if (Flags & 1)
	{
		CWO_Object_ClientData* pCD = AllocClientData(_pObj);
		pCD->AutoVar_Read(_pFile, _pWData);
	}
}


void CWObject_Object::OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWClient, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	parent::OnClientSave(_pObj, _pWClient, _pFile, _pWData, _Flags);

	uint8 Flags = 0;
	CWO_Object_ClientData* pCD = GetClientData(_pObj);
	if (pCD)
		Flags |= 1;

	_pFile->WriteLE(Flags);

	if (pCD)
		pCD->AutoVar_Write(_pFile, _pWData);
}


//
// Override default behaviour:
//
//  - if collision type 'projectile' is set, try and use the dedicated projectile phys model
//
bool CWObject_Object::OnIntersectLine(CWO_OnIntersectLineContext& _Context, CCollisionInfo* _pCollisionInfo)
{
	CWObject_CoreData* pObj = _Context.m_pObj;

	const CWO_PhysicsState& PhysState = pObj->GetPhysState();
	if (!_pCollisionInfo || (_pCollisionInfo->m_CollisionType != CXR_COLLISIONTYPE_PROJECTILE) || (PhysState.m_nPrim < 2))
		return parent::OnIntersectLine(_Context, _pCollisionInfo);

	uint nPrim = PhysState.m_nPrim;
	for (uint i = 1; i < nPrim; i++)
	{
		const CWO_PhysicsPrim& PhysPrim = PhysState.m_Prim[i];
		M_ASSERTHANDLER(PhysPrim.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL, "Incorrect projectile phys prim!", return parent::OnIntersectLine(_Context, _pCollisionInfo));
		M_ASSERTHANDLER(!(PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_OFFSET), "Shouldn't have offset!", return parent::OnIntersectLine(_Context, _pCollisionInfo));

		if (!(((PhysState.m_ObjectFlags & PhysPrim.m_ObjectFlagsMask) & _Context.m_ObjectIntersectionFlags) ||
			(PhysState.m_ObjectIntersectFlags & _Context.m_ObjectFlags)))
			continue;

		CXR_Model* pModel = _Context.m_pPhysState->GetMapData()->GetResource_Model(PhysPrim.m_iPhysModel);
		if (!pModel) continue;
		CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();

		CXR_PhysicsContext PhysContext(pObj->GetPositionMatrix(), NULL);
		PhysContext.m_PhysGroupMaskThis = PhysPrim.m_PhysModelMask;
		pPhysModel->Phys_Init(&PhysContext);

		CCollisionInfo Coll;
		Coll.CopyParams(*_pCollisionInfo);
		if (pPhysModel->Phys_IntersectLine(&PhysContext, _Context.m_p0, _Context.m_p1, _Context.m_MediumFlags, &Coll))
		{
			Coll.m_iObject = pObj->m_iObject;
			_pCollisionInfo->Improve(Coll);
		}
	}
	return _pCollisionInfo->m_bIsCollision;
}


int CWObject_Object::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	switch (_Event)
	{
	case CWO_PHYSEVENT_DYNAMICS_COLLISION:
		{
			M_ASSERT(_pCollisionInfo, "invalid collision event");
			if (_pPhysState->IsServer())
			{
				CWObject_Object* pThis = safe_cast<CWObject_Object>(_pObj);
				CWObject* pOther = static_cast<CWObject*>(_pObjOther);
				pThis->OnCollision(pOther, _pCollisionInfo->m_Pos, _pCollisionInfo->m_Velocity.k[0]);
			}
		}
		return 0;
	}
	return parent::OnPhysicsEvent(_pObj, _pObjOther, _pPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);
}


aint CWObject_Object::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_HOOK_GETRENDERMATRIX:
		{
			CMat4Dfp32& ResultMat = *(CMat4Dfp32*)_Msg.m_Param1;
			fp32 IPTime = _pWClient->GetRenderTickFrac();
			int iParentAttach = _pObj->m_iParentAttach;
			if (_pObj->GetParent() && iParentAttach >= 0)
			{
				CWObject_Message Msg(OBJMSG_HOOK_GETRENDERMATRIX, iParentAttach, aint(&ResultMat));
				if (_pWClient->ClientMessage_SendToObject(Msg, _pObj->GetParent()))
				{
					M_VMatMul(_pObj->GetLocalPositionMatrix(), ResultMat, ResultMat);
					goto matrix_okclmsg1;
				}
			}
			Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), ResultMat, IPTime);
matrix_okclmsg1:
			return 1;
		}

	case OBJMSG_HOOK_GETCURRENTMATRIX:
		{
			CMat4Dfp32& ResultMat = *(CMat4Dfp32*)_Msg.m_Param0;
			CVec3Dfp32 LocalPos(0.0f, 0.0f, 0.0f);

			uint iNode = _Msg.m_Param1;
			if (iNode)
			{
				CXR_Skeleton* pSkel = NULL;
				CXR_SkeletonInstance* pSkelInst = EvalAnim(_pObj, _pWClient, pSkel);
				if (pSkel && (iNode < pSkel->m_lNodes.Len()))
				{
					LocalPos = pSkel->m_lNodes[iNode].m_LocalCenter;
					if (pSkelInst)
					{
						ResultMat = pSkelInst->m_pBoneTransform[iNode];
						ResultMat.GetRow(3) = LocalPos * ResultMat;
						return 1;
					}
				}
			}
			// no animated stuff, just return object matrix
			fp32 IPTime = _pWClient->GetRenderTickFrac();

			int iParentAttach = _pObj->m_iParentAttach;
			if (_pObj->GetParent() && iParentAttach >= 0)
			{
				CWObject_Message Msg(OBJMSG_HOOK_GETCURRENTMATRIX, aint(&ResultMat), iParentAttach);
				if (_pWClient->ClientMessage_SendToObject(Msg, _pObj->GetParent()))
				{
					M_VMatMul(_pObj->GetLocalPositionMatrix(), ResultMat, ResultMat);
					goto matrix_okclmsg2;
				}
			}
			Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), ResultMat, IPTime);
matrix_okclmsg2:
			ResultMat.GetRow(3) = LocalPos * ResultMat;
			return 1;
		}

	case OBJMSG_SPAWN_WALLMARK:
		{
			CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
			if (pModel)
			{
				CXR_WallmarkDesc* pWMD = (CXR_WallmarkDesc*)_Msg.m_Param0;

				const CMat4Dfp32* pMat = (const CMat4Dfp32*)_Msg.m_Param1;
				CMat4Dfp32 InvObjMat, LocalPos;
				_pObj->GetPositionMatrix().InverseOrthogonal(InvObjMat);
				pMat->Multiply(InvObjMat, LocalPos);

				_pWClient->Wallmark_Create(pModel, *pWMD, LocalPos, 2.0f, 0);
			}
		}
		return 0;

	default:
		return parent::OnClientMessage(_pObj, _pWClient, _Msg);
	}
}



bool CWObject_Object::ConnectConstraint(int _iOtherObj, SConstraint& _Constraint)
{
	foreach (SConstraint, c, m_Root.m_lConstraints)
	{
		if (c.m_iPhysConstraint < 0)
			continue;

		uint16 iObj0, iObj1;
		m_pWServer->Phys_GetConnectedObjects(c.m_iPhysConstraint, &iObj0, &iObj1);
		if (iObj0 == _iOtherObj)
			Swap(iObj0, iObj1);

		if ((_Constraint.m_Name == c.m_Name) && (iObj1 == _iOtherObj))
		{
			// found matching constraint!
			_Constraint.m_iPhysConstraint = c.m_iPhysConstraint;
			return true;
		}
	}
	return false;
}


struct SGroup
{
	TArray<uint8> m_liObj;
	int m_iSpawnedObj;

	SGroup() : m_iSpawnedObj(0) {}
	SGroup(uint8 iObj)
	{
		m_liObj.Add(iObj);
	}
	bool Find(int _iObj) const
	{
		foreach (const uint8, i, m_liObj)
			if (i == _iObj)
				return true;
		return false;
	}
	CStr Dump() const
	{
		CStr Result("(");
		foreach (const uint8, i, m_liObj)
			Result += CStrF("%d,", i);
		Result += ")";
		return Result;
	}
};

struct SGroupList
{
	TArray<SGroup> m_lGroups;

	int FindGroup(int _iObj) const
	{
		for (uint i = 0; i < m_lGroups.Len(); i++)
			if (m_lGroups[i].Find(_iObj))
				return i;
		return -1;
	}

	int Merge(uint iGroup1, uint iGroup2)
	{
		if (iGroup1 > iGroup2)
			Swap(iGroup1, iGroup2);

		TAP<uint8> piGroup2 = m_lGroups[iGroup2].m_liObj;
		for (uint i = 0; i < piGroup2.Len(); i++)
		{
			uint8 iObj = piGroup2[i];
			TAP<uint8> piGroup1 = m_lGroups[iGroup1].m_liObj;
			uint j;
			for (j = 0; j < piGroup1.Len(); j++)
				if (piGroup1[j] == iObj)
					break;
			if (j == piGroup1.Len())
				m_lGroups[iGroup1].m_liObj.Add(iObj);
		}

		m_lGroups.Del(iGroup2);
		return iGroup1;
	}
};

void CWObject_Object::CheckSeparation()
{
	// Don't split up into sub-objects until breakage-handling is done
	if (m_Root.m_Flags & FLAGS_ISBREAKING)
		return;

	MSCOPESHORT(CWObject_Object::CheckSeparation);
	SGroupList Groups;

	// Build list of grouped sub-objects
	uint nParts = m_Root.m_lSubParts.Len();
	for (uint iPart = 0; iPart < nParts; iPart++)
	{
		int iGroup = Groups.FindGroup(iPart);
		M_ASSERT(iGroup == -1, "Already in a group?!");

		// connected with an object in a group?
		foreach (SConstraint, c, m_Root.m_lConstraints)
		{
			if (c.m_Type != CONSTRAINT_TYPE_RIGID)
				continue;

			int iGroup2 = -1;
			if (c.m_iObj0 == iPart)
				iGroup2 = Groups.FindGroup(c.m_iObj1);
			else if (c.m_iObj1 == iPart)
				iGroup2 = Groups.FindGroup(c.m_iObj0);

			if (iGroup2 >= 0)
			{
				DBG_OUT("- found connection: iPart=%d, c = ('%s', Obj1=%d, Obj2=%d)\n", iPart, c.m_Name.DbgName().Str(), c.m_iObj0, c.m_iObj1);
				if (iGroup >= 0 && iGroup != iGroup2)
				{ // object belongs to iGroup and is to be connected with iGroup2, let's merge the groups
					DBG_OUT("Merging groups '%d' [%s] with group '%d' [%s]\n", iGroup, Groups.m_lGroups[iGroup].Dump().Str(), iGroup2, Groups.m_lGroups[iGroup2].Dump().Str());
					iGroup2 = Groups.Merge(iGroup, iGroup2);
					DBG_OUT(" - Result: [%s]  (%d groups total)\n",  Groups.m_lGroups[iGroup2].Dump().Str(), Groups.m_lGroups.Len());
				}
				if (iGroup == -1)
					Groups.m_lGroups[iGroup2].m_liObj.Add(iPart);
				DBG_OUT("Part %d (%s) added to group %d [%s]\n", iPart, m_Root.m_lSubParts[iPart].m_Name.DbgName().Str(), iGroup2, Groups.m_lGroups[iGroup2].Dump().Str());
				iGroup = iGroup2;
			}
		}

		// if not part of any group, create a new one
		if (iGroup < 0)
		{
			iGroup = Groups.m_lGroups.Add( SGroup(iPart) );
			DBG_OUT("Part %d (%s) added to new group (%d)\n", iPart, m_Root.m_lSubParts[iPart].m_Name.DbgName().Str(), iGroup);
		}
	}

	DBG_OUT("\nResulting groups:\n");
	for (int i = 0; i < Groups.m_lGroups.Len(); i++)
	{
		const SGroup& g = Groups.m_lGroups[i];
		DBG_OUT(" Group: %d\n", i);
		foreach (const uint8, iPart, g.m_liObj)
		{
			const SPart& p = m_Root.m_lSubParts[iPart];
			DBG_OUT("    - %d: '%s' (%08X, %d, %05X)\n", iPart, p.m_Name.DbgName().Str(), (uint32)p.m_Name, p.m_iModel[0], p.m_PhysPrim.m_VisibilityMask);
		}
	}
	DBG_OUT("---\n");

	if ((m_Root.m_Flags & FLAGS_BROKEN) || (Groups.m_lGroups.Len() > 1))
	{
		DBG_OUT("  CheckSeparation, splitting object:\n");

		fp32 TotalMass, TotalVolume; // Use for auto-setting physics 
		m_pWServer->Phys_GetPhysicalProperties(GetPhysState(), TotalMass, TotalVolume);

		// Play break sound
		CWO_SoundGroupManager* pMan = (CWO_SoundGroupManager*)m_pWServer->Message_SendToObject(
				CWObject_Message(OBJMSG_GAME_GETSOUNDGROUPMANAGER), m_pWServer->Game_GetObjectIndex());
		if (pMan)
		{
			int iSound = pMan->GetSound(m_Root.m_Sounds.m_iSoundGroup, CWO_SoundGroupManager::SOUND_BREAK);
			if (iSound > 0)
			{
				DBG_OUT("- playing break sound '%d'\n", iSound);
				CVec3Dfp32 Pos;
				GetAbsBoundBox()->GetCenter(Pos);
				m_pWServer->Sound_At(Pos, iSound, WCLIENT_ATTENUATION_3D, 0, 1.0f);
			}
		}

		// Remove physics from main object, to make room for spawning child objects
		m_pWServer->Object_SetPhysics(m_iObject, CWO_PhysicsState());

		TStaticArray<uint32, 50> liJointObjects;
		int iJointRootGroup = -1;

		// Create new objects
		TAP<SGroup> pGroups = Groups.m_lGroups;
		for (uint iGroup = 0; iGroup < pGroups.Len(); iGroup++)
		{
			SGroup& g = pGroups[iGroup];

			// Create new object
			DBG_OUT(" Group:\n");

			SPart TmpPart;
			TmpPart.m_PhysPrim.m_VisibilityMask = 0;
			TmpPart.m_RenderMask = 0;
			foreach (uint8, iPart, g.m_liObj)
			{
				const SPart& p = m_Root.m_lSubParts[iPart];
				DBG_OUT("    - %d: '%s' (%08X, %d, %05X)\n", iPart, p.m_Name.DbgName().Str(), (uint32)p.m_Name, p.m_iModel[0], p.m_PhysPrim.m_VisibilityMask);

				TmpPart.Merge(p);
				TmpPart.m_lSubParts.Add(p);

				foreach (const int16, iJoint, p.m_liJoints)
				{
					if (iJoint == 0)
						iJointRootGroup = iGroup;

					if (iJoint >= 0)
						liJointObjects.Add( (iGroup << 16) | iJoint );
				}
			}

			if (m_Root.m_Flags & FLAGS_TRIMESHCOLLISION)
				TmpPart.m_Flags |= FLAGS_TRIMESHCOLLISION;

			// Copy relevant constraints
			foreach (const SConstraint, c, m_Root.m_lConstraints)
			{
				int iGroup1 = Groups.FindGroup(c.m_iObj0);
				int iGroup2 = Groups.FindGroup(c.m_iObj1);

				SConstraint tmp = c;
				tmp.m_iPhysConstraint = -1;
				tmp.m_iObj0 = SConstraint::IndexToHash(c.m_iObj0, m_Root, *this);    // Convert from old index to stringhash
				tmp.m_iObj1 = SConstraint::IndexToHash(c.m_iObj1, m_Root, *this);
				tmp.m_iObj0 = SConstraint::HashToIndex(tmp.m_iObj0, TmpPart, *this); // Convert from stringhash to new index
				tmp.m_iObj1 = SConstraint::HashToIndex(tmp.m_iObj1, TmpPart, *this);

				if (tmp.m_iObj0 < 0 && tmp.m_iObj1 < 0)
					continue; // this constraint doesn't belong to us...

				if ((iGroup1 >= 0) && (iGroup2 >= 0) && (iGroup1 != iGroup2))
				{
					// this is an unspawned constraint between two sub-parts
					if (tmp.m_iObj0 >= 0)
						tmp.m_iPhysConstraint = iGroup2 | 0x8000; // temp store group index here!
					else if (tmp.m_iObj1 >= 0)
						tmp.m_iPhysConstraint = iGroup1 | 0x8000; // - " -
					else
					{	M_ASSERT(false, "constraint not connected to any subpart?!"); }
				}
				else if (c.m_iPhysConstraint >= 0)
				{
					// this is a real physconstraint that needs to be transferred to the new object
					tmp.m_iPhysConstraint = c.m_iPhysConstraint; 
				}

				DBG_OUT("    - adding constraint '%s' (%d <-> %d, %x)\n", tmp.m_Name.DbgName().Str(), tmp.m_iObj0, tmp.m_iObj1, tmp.m_iPhysConstraint);
				TmpPart.m_lConstraints.Add(tmp);
			}

			// Copy effect-models from root
			TmpPart.m_lObjectModels.Add(m_Root.m_lObjectModels);

			// Time to create new object!
			{
				aint Params[] = { (aint)this, (aint)&TmpPart }; // this is used by OnInitInstance()
				const CMat4Dfp32& Pos = GetPositionMatrix();

				const char* pClassName = "Object";
				if ((TmpPart.m_Flags & FLAGS_LAMP) && (m_iClass == m_pWServer->GetMapData()->GetResourceIndex_Class("Object_Lamp")))
					pClassName = "Object_Lamp";

				int iNewObj = m_pWServer->Object_Create(pClassName, Pos, m_iOwner, Params, 2);
				g.m_iSpawnedObj = iNewObj;
				DBG_OUT("  Spawned object: %d  (%s, %d, %05X, %d, %d)\n", iNewObj, pClassName, TmpPart.m_iModel[0], TmpPart.m_PhysPrim.m_VisibilityMask, TmpPart.m_PhysPrim.m_iPhysModel, TmpPart.m_PhysPrim.m_iPhysModel_Dynamics);

				m_pWServer->Object_SetName(iNewObj, GetName());
				m_pWServer->Phys_SetStationary(iNewObj, false);

				{
					fp32 ForceMul = 0.1f; // this is a workaround tweak, since the phys engine seems to give alot more force to the object on the impact given here than given when shooting at a second time
					if (m_iImpactPart >= 0 && g.Find(m_iImpactPart))
						ForceMul = 0.3f;

					DBG_OUT("  Giving force..\n");
					CWObject_Message Msg(OBJMSG_OBJECT_APPLYFORCE);
					Msg.m_VecParam0 = m_ImpactPos;
					Msg.m_VecParam1 = m_ImpactForce * ForceMul;	
					m_pWServer->Message_SendToObject(Msg, iNewObj);
				}
			}
		}

		// Setup joint object list
		if ((iJointRootGroup >= 0) && (liJointObjects.Len() > 0))
		{
			CWObject* pRootObject = m_pWServer->Object_Get( Groups.m_lGroups[iJointRootGroup].m_iSpawnedObj );
			if (pRootObject)
			{
				CWO_Object_ClientData* pCD = AllocClientData(pRootObject);

				uint nJointObjects = liJointObjects.Len();
				pCD->m_liJointObjects.SetLen( nJointObjects );
				for (uint i = 0; i < nJointObjects; i++)
				{
					uint16 iGroup = liJointObjects[i] >> 16;
					uint16 iJoint = liJointObjects[i] & 0xffff;

					uint16 iObj = Groups.m_lGroups[iGroup].m_iSpawnedObj;
					pCD->m_liJointObjects[i] = (iObj << 16) | iJoint;

					CWObject* pJointObj = m_pWServer->Object_Get(iObj);
					if (pJointObj && iJoint > 0)
						pJointObj->iModel(0) = 0; // clear rendering for joint-object
				}
				pCD->m_liJointObjects.MakeDirty();
			}
		}

		CWO_Object_ClientData* pCD = GetClientData(this);
		bool bIsBurning = (pCD && pCD->m_Burnable.m_Value.IsBurning());

		// Connect constraints between newly spawned objects
		foreach (SGroup, g, Groups.m_lGroups)
		{
			int iObj = g.m_iSpawnedObj;
			CWObject_Object* pObj = safe_cast<CWObject_Object>( m_pWServer->Object_Get(iObj) );
			M_ASSERTHANDLER(pObj, "created object is gone??", continue);
			if (!pObj) continue; //wtf?!

			// Copy burn data to new object
			if (bIsBurning)
			{
				CWO_Object_ClientData* pObjCD = AllocClientData(pObj);
				pObjCD->m_Burnable.m_Value.CopyBurnable(pCD->m_Burnable.m_Value);
				pObjCD->m_Burnable.MakeDirty();
			}

			foreach (SConstraint, c, pObj->m_Root.m_lConstraints)
			{
				if (c.m_iPhysConstraint == -1)
					continue; // not a phys constraint

				if (c.m_iObj0 <= CONSTRAINT_TARGET_WORLD || c.m_iObj1 <= CONSTRAINT_TARGET_WORLD)
					continue; // ignore world constraints

				if (c.m_iObj0 >= 0 && c.m_iObj1 >= 0)
					continue; // ignore internal constraints

				if (c.m_iPhysConstraint & 0x8000)
				{
					// this is a constraint between two subparts. the group index for the other object (to connect iObj with) one is temp-stored in m_iPhysConstraint
					int iGroup = c.m_iPhysConstraint & 0x7fff;
					c.m_iPhysConstraint = -1;

				//	if (c.m_iObj0 < 0 || c.m_iObj1 != CONSTRAINT_TARGET_NONE)
				//		continue; // only handle external constraints with obj0 == this

					// Convert group index to object index
					int iOtherObj = Groups.m_lGroups[iGroup].m_iSpawnedObj;

					// First ask the other object if a constraint has already been created
					CWObject_Object* pOtherObj = safe_cast<CWObject_Object>( m_pWServer->Object_Get(iOtherObj) );
					M_ASSERTHANDLER(pOtherObj, "created object is gone??", continue);
					if (!pOtherObj) continue; //wtf?!
					if (pOtherObj->ConnectConstraint(iObj, c))
					{
						DBG_OUT("   - Existing constraint '%s' connected between %d and %d (id: %d)\n", c.m_Name.DbgName().Str(), iObj, iOtherObj, c.m_iPhysConstraint);
					}
					else
					{
						// Couldn't find a matching constraint in other object, create it!
						DBG_OUT(" [Object %d], Creating constraint...\n", m_iObject);
						SConstraint tmp = c;
						if (c.m_Flags & CWObject_Object::CONSTRAINT_FLAGS_ISVERSION2)
						{
							tmp.m_iObj0 = (c.m_iObj0 >= 0) ? iObj : iOtherObj;
							tmp.m_iObj1 = (c.m_iObj0 >= 0) ? iOtherObj : iObj;
						}
						else
						{
							tmp.m_iObj0 = iObj;
							tmp.m_iObj1 = iOtherObj;
						}
						tmp.Relocate( pObj->GetPositionMatrix() );
						c.m_iPhysConstraint = CWObject_Object_Constraint::Spawn(*m_pWServer, tmp);
						DBG_OUT("   - id: %d\n", c.m_iPhysConstraint);
					}
				}
				else
				{
					// transfer existing inter-object-constraint from old object to newly created object
					DBG_OUT(" [Object %d], Moving constraint '%s' from %d to %d.\n", m_iObject, c.m_Name.DbgName().Str(), m_iObject, iObj);
					m_pWServer->Phys_UpdateConnectedObject(c.m_iPhysConstraint, m_iObject, iObj);

					// remove constraint so that the physconstraint is kept when this object gets deleted
					RemoveConstraint(c.m_iPhysConstraint);
				}
			}
		}

		// Notify owner about this object being split
		if (m_iOwner)
		{
			uint16 liNewObj[16];
			uint nNewObj = Groups.m_lGroups.Len();
			M_ASSERT(nNewObj <= 16, "need to increase buffer size");
			for (uint i = 0; i < nNewObj; i++)
				liNewObj[i] = Groups.m_lGroups[i].m_iSpawnedObj;

			CWObject_Message Msg(OBJMSG_OBJECT_ISBREAKING, nNewObj, aint(&liNewObj), m_iObject);
			Msg.m_VecParam0.k[0] = TotalVolume;
			m_pWServer->Message_SendToObject(Msg, m_iOwner);

			// Also notify scenepoint manager, so that dynamic scenepoints can find a replacement owner
			CWObject_ScenePointManager* pSPM = (CWObject_ScenePointManager*)
				m_pWServer->Game_GetObject()->OnMessage(CWObject_Message(OBJMSG_GAME_GETSCENEPOINTMANAGER));
			pSPM->OnMessage(Msg);
		}

		// Transfer child objects      TODO: use a more robust system than closest distance...
		for (int iChild = GetFirstChild(); iChild > 0; )
		{
			CWObject* pChild = m_pWServer->Object_Get(iChild);
			CVec3Dfp32 ChildPos = pChild->GetPosition();
			fp32 Best = _FP32_MAX;
			uint iBest = 0;
			foreach (SGroup, g, Groups.m_lGroups)
			{
				CWObject* pSpawned = m_pWServer->Object_Get(g.m_iSpawnedObj);
				if (pSpawned)
				{
					CVec3Dfp32 Pos;
					pSpawned->GetAbsBoundBox()->GetCenter(Pos);
					fp32 d = ChildPos.DistanceSqr(Pos);
					if (d < Best)
					{
						Best = d;
						iBest = g.m_iSpawnedObj;
					}
				}
			}
			if (iBest > 0)
			{
				DBG_OUT("- transferring child object (%d, %s) to new object %d\n", iChild, m_pWServer->Object_GetName(iChild), iBest);
				m_pWServer->Object_AddChild(iBest, iChild);
				iChild = GetFirstChild();
			}
			else
			{
				iChild = pChild->GetNextChild();
			}
		}

		// Remove this object
		DeleteAllPhysConstraints();
		m_Root.m_Flags |= FLAGS_DESTROYED;
		m_Root.m_Flags &= ~FLAGS_GAMEPHYSICS;
		{ // turn off physics while object is "dying"
			m_PhysState.m_PhysFlags &= ~OBJECT_PHYSFLAGS_PHYSMOVEMENT;
			m_pWServer->Object_InitRigidBody(m_iObject, (m_Root.m_Flags & FLAGS_DEBRIS) != 0);
		}
		if (bIsBurning)
		{
			// Setup some flags
			m_Root.m_Flags |= FLAGS_NOPHYSICS;
			m_Root.m_Flags &= ~(FLAGS_NAVIGATION | FLAGS_ANIMPHYS | FLAGS_TRIMESHCOLLISION | FLAGS_MODELSHADOW);

			// Give fire some time to fade out
			ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
			ClientFlags() |= CWO_CLIENTFLAGS_NORENDER;
			m_LifetimeEnd = m_pWServer->GetGameTick() + TruncToInt(m_pWServer->GetGameTicksPerSecond());
			pCD->m_Burnable.m_Value.BurnEndTick() = m_LifetimeEnd;
		}
		else if (Groups.m_lGroups.Len() >= 1)
		{
			m_pWServer->Object_Destroy(m_iObject);
		}
		else
		{
			ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
			m_LifetimeEnd = m_pWServer->GetGameTick() + int(0.2f * m_pWServer->GetGameTicksPerSecond());
		}
	}

	m_Root.m_Flags &= ~FLAGS_FRAGILE; // if fragile, we've done the separation test now...
	m_iImpactPart = -1;
}


void CWObject_Object::GetImpactMatrix(const SObjectModel& _Model, const CWO_DamageMsg& _Damage, CMat4Dfp32& _Dest) const
{
	// This is an impact effect. Ignore original model position
	CVec3Dfp32 Normal = _Damage.m_pCInfo->m_Plane.n;
	CVec3Dfp32 ImpactDir;

	_Dest.Unit();
	_Dest.GetRow(3) = _Damage.m_Position;

	if (_Damage.m_bForceValid && (_Model.m_Flags & MODELFLAGS_IMPACT_REFLECT))
	{
		ImpactDir = _Damage.m_Force;
		ImpactDir.Normalize();
		_Dest.GetRow(0) = ImpactDir - Normal * (2.0f * (Normal * ImpactDir));
	}
	else
	{
		ImpactDir = (M_Fabs(Normal.k[0]) > 0.8f) ? CVec3Dfp32(0,1,0) : CVec3Dfp32(1,0,0);
		_Dest.GetRow(0) = Normal;
	}

	if (_Model.m_Flags & MODELFLAGS_IMPACT_WORLDUP)
	{
		_Dest.RecreateMatrix(2, 0);
	}
	else
	{
		_Dest.GetRow(1) = ImpactDir;
		_Dest.RecreateMatrix(0, 1);
	}
}


void CWObject_Object::SpawnModel(const SObjectModel& _Model, const CMat4Dfp32* _pPosMat)
{
	CMat4Dfp32 PosMat;

	if (_pPosMat == NULL)
	{
		// Use the position set on the model
		_Model.m_LocalPos.Multiply(GetPositionMatrix(), PosMat);
		m_pWServer->Debug_RenderMatrix(PosMat, 3.0f);
		_pPosMat = &PosMat;
	}

	int iNewObj = 0;
	if (_Model.m_Flags & MODELFLAGS_TEMPLATE)
	{
		CWRes_Template* pTpl = TDynamicCast<CWRes_Template>(m_pWServer->GetMapData()->GetResource(_Model.m_iModel[0]));
		if (pTpl)
		{
			const CRegistry& Reg = *pTpl->m_lspObjectReg[0];
			iNewObj = m_pWServer->Object_Create(Reg, _pPosMat, m_iOwner);
		}
	}
	else
	{
		aint Params[] = { (aint)&_Model };
		iNewObj = m_pWServer->Object_Create("Object", *_pPosMat, m_iOwner, Params, 1);
	}

	// Attach object    (TODO: implement this without using parent?)
	if (iNewObj > 0 && (_Model.m_Flags & MODELFLAGS_ATTACHED))
		m_pWServer->Object_AddChild(m_iObject, iNewObj);
}





/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object_Constraint
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Object_Constraint, CWObject, 0x0102);
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Object_Constraint2, CWObject_Object_Constraint, 0x0100);

void CWObject_Object_Constraint::OnCreate()
{
	m_Data.SetDefault();
}

void CWObject_Object_Constraint2::OnCreate()
{
	m_Data.SetDefault2();
}


void CWObject_Object_Constraint::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	switch (_KeyHash)
	{
	case MHASH2('ANGL','ES'): // "ANGLES"
	case MHASH2('ORIG','IN'): // "ORIGIN"
		CWObject::OnEvalKey(_KeyHash, _pKey);  // safer to let game object handle this
		return;

	case MHASH2('OBJE','CT0'): // "OBJECT0"
	case MHASH2('OBJE','CT1'): // "OBJECT1"
		{
			CFStr KeyValue = _pKey->GetThisValue();
			CFStr ObjName = KeyValue.GetStrSep(":");
			KeyValue.Trim();

			if (_KeyHash == MHASH2('OBJE','CT0'))
			{
				m_Data.m_iObj0 = ObjName.StrHash();
				m_SubPart0 = KeyValue;
			}
			else // OBJECT1
			{
				m_Data.m_iObj1 = ObjName.StrHash();
				m_SubPart1 = KeyValue;
			}
		}
		return;
	}

	if (!m_Data.OnEvalKey(_KeyHash, *_pKey, *this))
		CWObject::OnEvalKey(_KeyHash, _pKey);
}


void CWObject_Object_Constraint::OnSpawnWorld()
{
	// Use game object's position
	m_Data.m_LocalPos = GetLocalPositionMatrix();

	m_Data.m_Name = GetName();
	if ((uint32)m_Data.m_Name == 0)
		m_Data.m_Name = CFStrF("__noname_%d", s_iNoname++);

	if (m_Data.m_Type == CWObject_Object::CONSTRAINT_TYPE_RIGID)
	{
		ConOutL(CStrF("WARNING: Constraint '%s' failed to spawn because of invalid type. Global constraints cannot be of type 'rigid'", GetName()));
		m_pWServer->Object_Destroy(m_iObject);
		return;
	}

	m_Data.m_iObj0 = m_pWServer->Selection_GetSingleTarget( m_Data.m_iObj0 );
	m_Data.m_iObj1 = m_pWServer->Selection_GetSingleTarget( m_Data.m_iObj1 );

	int iConstraint = Spawn(*m_pWServer, m_Data);
	if (iConstraint >= 0)
	{
		DBG_OUT("   - id: %d (global)\n", iConstraint);
		CWObject* pObj0x = m_pWServer->Object_Get(m_Data.m_iObj0);
		CWObject* pObj1x = m_pWServer->Object_Get(m_Data.m_iObj1);
		CWObject_Object* pObj0 = TDynamicCast<CWObject_Object>( pObj0x );
		CWObject_Object* pObj1 = TDynamicCast<CWObject_Object>( pObj1x );
		M_ASSERT(pObj0 || pObj1, "!");
		if (!pObj0)
		{
			Swap(pObj0, pObj1);
			Swap(m_SubPart0, m_SubPart1);
		}

		m_Data.m_iPhysConstraint = iConstraint;
		m_Data.m_iObj0 = m_SubPart0;
		m_Data.m_iObj1 = -1;
		pObj0->AddConstraint(m_Data);

		if (pObj1)
		{
			m_Data.m_iObj0 = m_SubPart1;
			pObj1->AddConstraint(m_Data);
		}

		if (pObj1x && !pObj1)
		{
			// Constraint is connected to a object that isn't a CWObject_Object.
			// - don't delete self, but instead set as child to the other object
			//   and then update the constraint position in OnRefresh()
			m_pWServer->Object_SetPosition(m_iObject, m_Data.m_LocalPos);
			m_pWServer->Object_AddChild(pObj1x->m_iObject, m_iObject);
			ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
			//ClientFlags() |= CWO_CLIENTFLAGS_NOROTINHERITANCE;
			return;
		}

		m_Data.OnSpawnWorld(*pObj0);
	}

	m_pWServer->Object_DestroyInstant(m_iObject);
}


void CWObject_Object_Constraint::OnRefresh()
{
	int iConstraint = m_Data.m_iPhysConstraint;
	M_ASSERT(iConstraint >= 0, "[Constraint] No constraint, yet active?!");

	// Check if constraint is still alive
	uint16 iObj0, iObj1;
	m_pWServer->Phys_GetConnectedObjects(iConstraint, &iObj0, &iObj1);
	if (!iObj0 && !iObj1)
	{
		m_pWServer->Object_DestroyInstant(m_iObject);
		return;
	}

	// Ok, constraint is still valid, so update position
	bool bMoving = (GetPosition().DistanceSqr(GetLastPosition()) > 0.01f);
	if (bMoving)
	{
		const CMat4Dfp32& PosMat = GetPositionMatrix();
		if (m_Data.m_Type == CWObject_Object::CONSTRAINT_TYPE_BALL)
		{
			m_pWServer->Phys_UpdateBallConstraint(iConstraint, PosMat.GetRow(3));
		}
		else
		{
			m_pWServer->Phys_UpdateAxisConstraint(iConstraint, PosMat, m_Data.m_Length.Getfp32());
		}
		if (iObj0)
			m_pWServer->Phys_SetStationary(iObj0, false);
		if (iObj1)
			m_pWServer->Phys_SetStationary(iObj1, false);
	}
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:	Used to create physics engine constraint

	Parameters:
	  	_Server:		Server instance
		_Constraint:	Constraint info. NOTES:
							- m_LocalPos is actually world pos
							- m_iObj0 & M_iObj1 should be object id's,
								not local part id's or namehashes..

	Returns:			ID of created constraint
\*____________________________________________________________________*/ 
int CWObject_Object_Constraint::Spawn(CWorld_Server& _Server, const SConstraint& _Constraint)
{
	int iObj0 = (_Constraint.m_iObj0 < CWObject_Object::CONSTRAINT_TARGET_WORLD) ? -_Constraint.m_iObj0 : _Constraint.m_iObj0;
	int iObj1 = (_Constraint.m_iObj1 < CWObject_Object::CONSTRAINT_TARGET_WORLD) ? -_Constraint.m_iObj1 : _Constraint.m_iObj1;
	CWObject_Object* pObj0 = TDynamicCast<CWObject_Object>( _Server.Object_Get(iObj0) );
	CWObject_Object* pObj1 = TDynamicCast<CWObject_Object>( _Server.Object_Get(iObj1) );

	if (pObj0 && !pObj0->m_pRigidBody2) pObj0 = NULL;
	if (pObj1 && !pObj1->m_pRigidBody2) pObj1 = NULL;

	if (!pObj0 && !pObj1)
	{
		ConOutL(CStrF("WARNING: Constraint '%s' failed to spawn because of invalid targets. (Obj1: %d, Obj2: %d)", _Constraint.m_Name.DbgName().Str(), iObj0, iObj1));
		return -1;
	}
	else if (!pObj0)
	{
		Swap(iObj0, iObj1);
		Swap(pObj0, pObj1);
	}

	if (_Constraint.m_Type == CWObject_Object::CONSTRAINT_TYPE_BALL)
	{
		const CMat4Dfp32& WorldMat = _Constraint.m_LocalPos;
		const CVec3Dfp32& WorldPos = _Constraint.m_LocalPos.GetRow(3);
//		CMat4Dfp32 InvMat;

//		pObj0->GetPositionMatrix().InverseOrthogonal(InvMat);
//		CVec3Dfp32 LocalPos0 = WorldPos;
//		LocalPos0 *= InvMat;
		fp32 AngleValue = _Constraint.m_MaxAngle.Getfp32();

		if (pObj1)
		{
		//	pObj1->GetPositionMatrix().InverseOrthogonal(InvMat);
		///	CVec3Dfp32 LocalPos1 = WorldPos;
		//	LocalPos1 *= InvMat;

			DBG_OUT(" - Spawning ball constraint ('%s') between %d and %d. Pos: %s\n", _Constraint.m_Name.DbgName().Str(), iObj0, iObj1, WorldPos.GetString().Str());
			return _Server.Phys_AddBallJointConstraint(iObj0, iObj1, WorldPos, AngleValue);
		}
		else
		{
			DBG_OUT(" - Spawning world ball constraint ('%s') for %d. Pos: %s\n", _Constraint.m_Name.DbgName().Str(), iObj0, WorldPos.GetString().Str());
			return _Server.Phys_AddBallJointToWorld(iObj0, WorldMat, AngleValue);
		}
	}
	else if (_Constraint.m_Type == CWObject_Object::CONSTRAINT_TYPE_AXIS)
	{
		const CMat4Dfp32& WorldMat = _Constraint.m_LocalPos;
		const CVec3Dfp32& WorldPos = _Constraint.m_LocalPos.GetRow(3);

//		const CVec3Dfp32& Axis = _Constraint.m_LocalPos.GetRow(2);
//		const CVec3Dfp32& Pos = _Constraint.m_LocalPos.GetRow(3);
		fp32 AxisLength = _Constraint.m_Length.Getfp32();
		fp32 AngleValue = _Constraint.m_MaxAngle.Getfp32();

		if (pObj1)
		{
			DBG_OUT(" - Spawning axis constraint ('%s') between %d and %d\n", _Constraint.m_Name.DbgName().Str(), iObj0, iObj1);
			if (_Constraint.m_Flags & CWObject_Object::CONSTRAINT_FLAGS_ISVERSION2)
			{
				fp32 MinAngle = _Constraint.m_MinAngle.Getfp32();
				fp32 MaxAngle = _Constraint.m_MaxAngle.Getfp32();
				return _Server.Phys_AddHingeJointConstraint2(iObj0, iObj1, WorldMat, AxisLength, MinAngle, MaxAngle);  // The interpretation of min/max is subject to confusion
			}
			else
			{
				return _Server.Phys_AddHingeJointConstraint(iObj0, iObj1, WorldMat, AxisLength, AngleValue);
			}
		}
		else
		{
			DBG_OUT(" - Spawning world axis constraint ('%s') for %d, pos: %s\n", _Constraint.m_Name.DbgName().Str(), iObj0, WorldMat.GetRow(3).GetString().Str() );

			if (_Constraint.m_Flags & CWObject_Object::CONSTRAINT_FLAGS_ISVERSION2)
			{
				fp32 MinAngle = _Constraint.m_MinAngle.Getfp32();
				fp32 MaxAngle = _Constraint.m_MaxAngle.Getfp32();
				return _Server.Phys_AddAxisConstraint2(iObj0, WorldMat, AxisLength, MinAngle, MaxAngle);
			}
			else 
			{
				return _Server.Phys_AddAxisConstraint(iObj0, WorldMat, AxisLength, AngleValue);
			}
		}
	}
	return -1; //wtf?!
}
