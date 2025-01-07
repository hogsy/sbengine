#include "PCH.h"

#include "WRPGCore.h"
#include "WRPGChar.h"

#include "WRPGDef.h"
#include "../WObj_Game/WObj_GameMessages.h"
#include "../../GameWorld/WServerMod.h"
#include "../WObj_Misc/WObj_Shell.h"

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object, CReferenceCount);

// blow: Fist
// pierce: Shanks etc
// tranquillizer: Sleep drug
// pistol: Soft, slowmoving bullet, shotpellets, kevlar should be "safe"
// rifle: High speed jacketed bullet
// neck: Neck breaking attempt
// fall: Fall damage (taken andgiven)
// blast: Damage from explosions
// ap: Armour Piercing such as depleted uranium, discarding sabot, HEAP etc
const char* CRPG_Object::ms_DamageTypeStr[] =
{
	"blow",
	"pierce",
	"tranquillizer",
	"pistol",
	"rifle",
	"darkness",
	"fall",
	"blast",
	"ap",
	"physics",
	"darkness_gun",
	"light",
	"flash",
	"blackhole",
	"_dmg14_no_crit",
	"blockable",
	NULL
};

const char* CRPG_Object::ms_AnimProperty[] =
{
	"barehands",
	"gun",
	"shotgun",
	"assaultrifle",
	"minigun",
	"knuckleduster",
	"shank",
	"club",
	"grenade",
	"tranqgun",
	NULL,
};

bool CRPG_Object::m_bPrecacheForPlayerUse = false;

// -------------------------------------------------------------------
//  CRPG_Object
// -------------------------------------------------------------------
//
// The fileformat is limited to:
//	32768 attributes
//	32768 strings
//	32768 child-items

void CRPG_Object::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_OnCreate, MAUTOSTRIP_VOID);
}
 
spCRPG_Object CRPG_Object::Duplicate() const
{
	MAUTOSTRIP(CRPG_Object_Duplicate, NULL);
	spCRPG_Object spObj = CreateObject(m_Name, m_pWServer);
	if (!spObj) Error("Duplicate", "Unable to duplicate object.");

	spObj->m_Name = m_Name;

	// Children
	{
		int nChildren = GetNumChildren();
		spObj->SetNumChildren(nChildren);
		for(int c = 0; c < nChildren; c++)
			spObj->m_lspItems[c] = m_lspItems[c]->Duplicate();
	}

	// Attr
	{
		int nAttribs = GetNumAttribs();
		spObj->SetNumAttribs(nAttribs);
		for(int a = 0; a < nAttribs; a++)
			spObj->Attrib(a) = m_lAttribs[a];
	}

	// Strings
	{
		int nStrings = GetNumStrings();
		spObj->SetNumStrings(nStrings);
		for(int s = 0; s < nStrings; s++)
			spObj->String(s)= m_lStrings[s];
	}

	spObj->OnDuplicate(this);

	return spObj;
}

spCRPG_Object CRPG_Object::CreateRuntimeClassObject(const char *_pName, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_CreateRuntimeClassObject, NULL);
	spCRPG_Object spItem = (CRPG_Object *)MRTC_GetObjectManager()->CreateObject(CFStrF("CRPG_Object_%s", _pName));
	if(spItem!=NULL)
	{
		spItem->m_Name = _pName;
		spItem->m_pWServer = _pWServer;
		spItem->OnCreate();
	}
	return spItem;
}

spCRegistry CRPG_Object::GetEvaledRegistry(const char *_pName, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_GetEvaledRegistry, NULL);
	MSCOPE(CRPG_Object::GetEvaledRegistry, RPG_OBJECT);

	//JK-FIX: Don't do it using TDynamicCast!
	CWServer_Mod *pServerMod = TDynamicCast<CWServer_Mod >(_pWServer);
	if(pServerMod)
	{
		CRegistry *pReg = pServerMod->GetEvaledRPGTemplate(_pName);
		if(pReg)
			return pReg;
	}
	
	//Not a runtime class. Looking for a rpg-template
	CStr Params = _pName;
	CStr Name = Params.GetStrSep(":");

	spCRegistry spTemplates = _pWServer->Registry_GetServer()->Find("RPGTEMPLATES_EVAL");
	if(spTemplates)
	{
		CRegistry *pReg = spTemplates->FindChild(Name);
		if (pReg)
		{
			if (Params != "")
			{
				spCRegistry spReg = pReg->Duplicate();
				while(Params != "")
				{
					CStr Val = Params.GetStrSep(":");
					CStr Key = Val.GetStrSep("=");
					spReg->AddKey(Key, Val);
				}

				spReg->SetThisKey(_pName, "");

				if(pServerMod)
					pServerMod->AddEvaledRPGTemplate(spReg);

				return spReg;
			}

			if(pServerMod)
				pServerMod->AddEvaledRPGTemplate(pReg);
			return pReg;
		}
	}
	
	spTemplates = _pWServer->Registry_GetServer()->Find("RPGTEMPLATES");
	if(spTemplates == NULL)
		return NULL;

	CRegistry_Dynamic Reg;
	Reg.AddKey("CLASSNAME", Name);
	while(Params != "")
	{
		CStr Val = Params.GetStrSep(":");
		CStr Key = Val.GetStrSep("=");
		Reg.AddKey(Key, Val);
	}
	
//	spTemplates->EnableAutoHashing(false);
	
	spCRegistry spReg = spTemplates->EvalTemplate_r(&Reg);
	if(!spReg)
		Error_static("CRPG_Object::CreateObject", "EvalTemplate_r");

	spReg->SetThisKey(_pName, "");

	if(pServerMod)
		pServerMod->AddEvaledRPGTemplate(spReg);

	return spReg;
}

spCRPG_Object CRPG_Object::CreateObject(const char *_pName, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_CreateObject, NULL);
	spCRPG_Object spItem = CreateRuntimeClassObject(_pName, _pWServer);
	if(spItem != NULL)
		return spItem;

	//Not a runtime class. Looking for a rpg-template
	spCRegistry spReg = GetEvaledRegistry(_pName, _pWServer);
	if(spReg)
	{
		CRegistry *pRPGClass = spReg->FindChild("RPGCLASS");
		if(!pRPGClass)
			return NULL;

		spItem = CreateRuntimeClassObject(pRPGClass->GetThisValue(), _pWServer);
		if(spItem!=NULL)
		{
			spItem->m_Name = _pName;
			int nKeys = spReg->GetNumChildren();
			for(int k = 0; k < nKeys; k++)
			{
				const CRegistry* pReg = spReg->GetChild(k);
				spItem->OnEvalKey(pReg->GetThisNameHash(), pReg);
			}
			
			spItem->OnFinishEvalKeys();
			return spItem;
		}
	}

	ConOutL(CStrF("§cf80WARNING: (CRPG_Object::CreateObject) Could not create item: %s", _pName));
	//If runtime-class can't be created a dummy object is created instead.
	spItem = MNew(CRPG_Object);
	if(!spItem)
		Error_static("CRPG_Object::CreateObject", "Out of memory");
	spItem->m_Name = _pName;
	spItem->m_pWServer = _pWServer;
	return spItem;
}

spCRPG_Object CRPG_Object::CreateObjectVirtual(const char *_pName, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_CreateObjectVirtual, NULL);
	// Same as CreateObject, but can be called from modules that can 
	// access an existing CRPG_Object and use it as a class factory.

	return CreateObject(_pName, _pWServer);
}


spCRPG_Object CRPG_Object::CreateObject(const CRegistry* _pKeys, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_CreateObject_2, NULL);
	spCRPG_Object spObj = CreateObject(_pKeys->GetThisValue(), _pWServer);
	if (spObj != NULL)
	{
		spObj->OnIncludeClass(_pKeys,_pWServer->GetMapData(),_pWServer);
		for(int i = 0; i < _pKeys->GetNumChildren(); i++)
		{
			const CRegistry* pReg = _pKeys->GetChild(i);
			spObj->OnEvalKey(pReg->GetThisNameHash(), pReg);
		}

		spObj->OnFinishEvalKeys();
	}
	return spObj;
}

spCRPG_Object CRPG_Object::CreateObject(CCFile* _pFile, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_CreateObject_3, NULL);
	CFStr name;
	name.Read(_pFile);
	spCRPG_Object spItem = CreateObject(name, _pWServer);

	int16 nAttribs;
	_pFile->ReadLE(nAttribs);
	spItem->m_lAttribs.SetLen(nAttribs);
	for(int a = 0; a < nAttribs; a++)
	{
		_pFile->ReadLE(spItem->Attrib(a).m_Value);
//		_pFile->ReadLE(spItem->Attrib(a).m_Current);
	}

	int16 nStrings;
	_pFile->ReadLE(nStrings);
	spItem->m_lStrings.SetLen(nStrings);
	for(int s = 0; s < nStrings; s++)
		spItem->m_lStrings[s].Read(_pFile);

	int16 nItems;
	_pFile->ReadLE(nItems);
	spItem->m_lspItems.SetLen(nItems);
	for(int i = 0; i < nItems; i++)
		spItem->m_lspItems[i] = CRPG_Object::CreateObject(_pFile, _pWServer);

	return spItem;
}

void CRPG_Object::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CRPG_Object_Write, MAUTOSTRIP_VOID);
	m_Name.Write(_pFile);

	int16 nAttribs = m_lAttribs.Len();
	_pFile->WriteLE(nAttribs);
	for(int a = 0; a < nAttribs; a++)
	{
		_pFile->WriteLE(Attrib(a).m_Value);
//		_pFile->WriteLE(Attrib(a).m_Current);
	}

	int16 nStrings = m_lStrings.Len();
	_pFile->WriteLE(nStrings);
	for(int s = 0; s < nStrings; s++)
		m_lStrings[s].Write(_pFile);

	int16 nItems = m_lspItems.Len();
	_pFile->WriteLE(nItems);
	for(int i = 0; i < nItems; i++)
		m_lspItems[i]->Write(_pFile);
}

bool CRPG_Object::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CRPG_Object_OnEvalKey, false);
	if (_pKey->GetThisName().CompareSubStr("RPG_CHILD") == 0)
	{
		spCRPG_Object spObj = CreateObject(_pKey, m_pWServer);
		if (spObj != NULL) AddChild(spObj);
	}
	else
		return false;

	return true;
}

bool CRPG_Object::Process(CRPG_Object *_pRoot, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Process, false);
	MSCOPE(CRPG_Object::Process, CHARACTER);

	if(PreProcess(_pRoot, _iObject))
		return true;

//	for(int a = 0; a < m_lAttribs.Len(); a++)
//		Attrib(a).m_Current = Attrib(a).m_Value;

/*	CRPG_Object *pRecurs = m_bProcessParent ? _pParent : this;
	for(int c = 0; c < GetNumChildren(); c++)
	{
		if(Child(c)->Process(pRecurs, _Mat, _iObject))
		{
			//Destroy object
			DelChild(c);
			c--;
		}
	}*/

	bool Ret = OnProcess(_pRoot, _iObject);
	return Ret;
}

CRPG_Object *CRPG_Object::GetChild(const char *_pName)
{
	MAUTOSTRIP(CRPG_Object_GetChild, NULL);
	int iObj = GetChildIndex(_pName);
	if(iObj == -1)
		return NULL;
	return GetChild(iObj);
}

int CRPG_Object::GetChildIndex(const char *_pName)
{
	MAUTOSTRIP(CRPG_Object_GetChildIndex, 0);
	for(int c = 0; c < m_lspItems.Len(); c++)
		if(GetChild(c)->m_Name.CompareNoCase(_pName) == 0)
			return c;

	return -1;
}

CRPG_Object *CRPG_Object::Find(const char *_pName)
{
	MAUTOSTRIP(CRPG_Object_Find, NULL);
	if(m_Name.CompareNoCase(_pName) == 0)
		return this;

	for(int c = 0; c < m_lspItems.Len(); c++)
	{
		CRPG_Object *pObj = m_lspItems[c]->Find(_pName);
		if(pObj)
			return pObj;
	}

	return NULL;
}

bool CRPG_Object::TraceRay(const CVec3Dfp32 &_Pos0, const CVec3Dfp32 &_Pos1, CCollisionInfo* _pCInfo, int _CollisionObjects, int _CollisionMediums, int _iExclude)
{
	MAUTOSTRIP(CRPG_Object_TraceRay, false);
	if (_CollisionObjects == 0)
		_CollisionObjects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;

	if (_CollisionMediums == 0)
		_CollisionMediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;

	bool bHit = m_pWServer->Phys_IntersectLine(_Pos0, _Pos1, OBJECT_FLAGS_PROJECTILE, _CollisionObjects, _CollisionMediums, _pCInfo, _iExclude);

	if((!bHit) || (!_pCInfo->m_bIsValid))
	{
//		if (true)
//			m_pWServer->Debug_RenderWire(_Pos0, _Pos1, 0xFF00FF00, 50.0f);

		return false;
	}
	else
	{
//		if (true)
//			m_pWServer->Debug_RenderWire(_Pos0, _pCInfo->m_Pos, 0xFFFF0000, 50.0f);

		return true;
	}
}

bool CRPG_Object::TraceRay(const CMat4Dfp32 &_Mat, float _Range, CCollisionInfo* _pCInfo, int _CollisionObjects, int _CollisionMediums, int _iExclude)
{
	MAUTOSTRIP(CRPG_Object_TraceRay_2, false);
	const CVec3Dfp32 &Source = CVec3Dfp32::GetMatrixRow(_Mat, 3);
	CVec3Dfp32 Dest = Source + CVec3Dfp32::GetMatrixRow(_Mat, 0) * _Range;
	return TraceRay(Source, Dest, _pCInfo, _CollisionObjects, _CollisionMediums, _iExclude);
}

int CRPG_Object::SendMsg(int _iObject, int _Msg, int _Param0, int _Param1, int _iSender)
{
	MAUTOSTRIP(CRPG_Object_SendMsg, 0);
	return m_pWServer->Message_SendToObject(CWObject_Message(_Msg, _Param0, _Param1, _iSender), _iObject);
}

int CRPG_Object::SendMsg(int _iObject, int _Msg, const CVec3Dfp32 &_v, int _Param0, int _iSender)
{
	MAUTOSTRIP(CRPG_Object_SendMsg_2, 0);
	return m_pWServer->Message_SendToObject(CWObject_Message(_Msg, _Param0, 0, _iSender, 0, _v), _iObject);
}

int CRPG_Object::SendMsg(int _iObject, int _Msg, int _Param0, int _Param1, int16 _iSender, int16 _Reason, const CVec3Dfp32& _VecParam0, const CVec3Dfp32& _VecParam1, void* _pData, int _DataSize)
{
	MAUTOSTRIP(CRPG_Object_SendMsg_3, 0);
	return m_pWServer->Message_SendToObject(CWObject_Message(_Msg, _Param0, _Param1, _iSender, _Reason, _VecParam0, _VecParam1, _pData, _DataSize), _iObject);
}

int CRPG_Object::GetRegValuei(const char *_Key, int _Default)
{
	MAUTOSTRIP(CRPG_Object_GetRegValuei, 0);
	return m_pWServer->Registry_GetServer()->GetValuei(_Key, _Default);
}

fp32 CRPG_Object::GetRegValuef(const char *_Key, fp32 _Default)
{
	MAUTOSTRIP(CRPG_Object_GetRegValuef, 0.0f);
	return m_pWServer->Registry_GetServer()->GetValuef(_Key, _Default);
}

char *CRPG_Object::GetRegValue(const char *_Key, char *_Default)
{
	MAUTOSTRIP(CRPG_Object_GetRegValue, NULL);
	return m_pWServer->Registry_GetServer()->GetValue(_Key, _Default);
}

void CRPG_Object::Dump(int _Level)
{
	MAUTOSTRIP(CRPG_Object_Dump, MAUTOSTRIP_VOID);
/*	CStr Indent = CStrF(' ', 4*_Level);
	ConOutL(Indent + m_Name);
	{
		for(int i = 0; i < GetNumAttribs(); i++)
			ConOutL(CStrF("%sAttr %d, Val %d, Cur %d", (char*) Indent, i, Attrib(i).m_Value, Attrib(i).m_Current));
	}
	{
		for(int i = 0; i < GetNumStrings(); i++)
			ConOutL(CStrF("%sStr %d, %s", (char*) Indent, i, (char*)String(i)));
	}

	{
		for(int i = 0; i < GetNumChildren(); i++)
		{
			ConOutL(CStrF("%sChild %d", (char*) Indent, i));
			GetChild(i)->Dump(_Level+1);
		}
	}*/
}

aint CRPG_Object::OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner)
{
	MAUTOSTRIP(CRPG_Object_OnMessage, 0);
	for(int c = 0; c < GetNumChildren(); c++)
	{
		int Ret = GetChild(c)->OnMessage(this, _Msg, _iOwner);
		switch(Ret)
		{
		case OBJMSG_RPGRET_CONTINUE : break;

		case OBJMSG_RPGRET_DELETE : 
			{
				DelChild(c);
				c--;
				break;
			}
		default :
			return Ret;
		}
	}
	return 0;
}

void CRPG_Object::IncludeRPGClass(const char *_pName, CMapData *_pMapData, CWorld_Server *_pWServer, bool _bPrecacheForPlayer, TArray<int32>* _plAnimTypesNeeded)
{
	MAUTOSTRIP(CRPG_Object_IncludeRPGClass, MAUTOSTRIP_VOID);
	CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(_pWServer);
	if(pServerMod)
	{
/*		int Flag = 1;
		bool bOldPrecache = m_bPrecacheForPlayerUse;
		if(_bPrecacheForPlayer || bOldPrecache)
		{
			m_bPrecacheForPlayerUse = true;			
			Flag = 2;
		}

		if(_plAnimTypesNeeded != NULL)
			Flag = 3;*/

		spCRegistry spReg = pServerMod->GetEvaledRPGTemplate(_pName);
//		if(spReg && spReg->GetThisValuei() >= Flag)
//		{
//			m_bPrecacheForPlayerUse = bOldPrecache;
//			return;
//		}

		bool bOldPrecache = m_bPrecacheForPlayerUse;
		m_bPrecacheForPlayerUse  = true;

		if(!spReg)
			spReg = GetEvaledRegistry(_pName, _pWServer);

		IncludeRPGEvaledRegistry(spReg, _pMapData, _pWServer, _plAnimTypesNeeded);
		m_bPrecacheForPlayerUse = bOldPrecache;
//		spReg->SetThisValuei(Flag);
	}


	CStr Params = _pName;
	CStr Name = Params.GetStrSep(":");
	CRegistry_Dynamic Reg;
	Reg.AddKey("CLASSNAME", Name);
	while(Params != "")
	{
		CStr Val = Params.GetStrSep(":");
		CStr Key = Val.GetStrSep("=");
		Reg.AddKey(Key, Val);
	}
	IncludeRPGRegistry(&Reg, _pMapData, _pWServer);

	return;
}

bool CRPG_Object::IncludeRPGEvaledRegistry(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer, TArray<int32>* _plAnimTypesNeeded)
{
	MAUTOSTRIP(CRPG_Object_IncludeRPGEvaledRegistry, false);
	CRegistry *pRPGClass = _pReg->FindChild("RPGCLASS");
	if(!pRPGClass)
	{
		if(_pReg->GetThisName() == "RPGOBJECT")
			pRPGClass = _pReg;
		else
			return false;
	}
		
	spCRPG_Object spItem = (CRPG_Object *)MRTC_GetObjectManager()->CreateObject("CRPG_Object_" + pRPGClass->GetThisValue());
	if(spItem != NULL)
	{
		spItem->OnIncludeClass(_pReg, _pMapData, _pWServer);
		// Ok, find animsets needed in the registry
		if (_plAnimTypesNeeded)
		{
			CRegistry* pAnim = _pReg->FindChild("ANIMTYPE");
			if (pAnim)
			{
				_plAnimTypesNeeded->Add(pAnim->GetThisValue().Val_int());
			}
			else
			{
				pAnim = _pReg->FindChild("ITEMTYPE");
				if (pAnim)
					_plAnimTypesNeeded->Add(pAnim->GetThisValue().Val_int());
			}
		}
	}
	
	for(int i = 0; i < _pReg->GetNumChildren(); i++)
		if(_pReg->GetName(i).CompareSubStr("RPG_CHILD") == 0)
		{
			IncludeRPGClass(_pReg->GetValue(i), _pMapData, _pWServer);
		}
	
	return true;
}

void CRPG_Object::IncludeRPGRegistry(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_IncludeRPGRegistry, MAUTOSTRIP_VOID);
	MSCOPE(CRPG_Object::IncludeRPGRegistry, RPG_OBJECT);

	CRegistry *pTemplates;
	pTemplates = _pMapData->m_spWData->GetGameReg()->Find("SERVER\\RPGTEMPLATES_EVAL");
	if(pTemplates)
	{
		CRegistry *pReg = pTemplates->FindChild(_pReg->GetValue("CLASSNAME", "-_-"));
		if (pReg)
		{
			IncludeRPGEvaledRegistry(pReg, _pMapData, _pWServer);
			return ;
		}
		else
		{
			IncludeRPGEvaledRegistry(_pReg, _pMapData, _pWServer);
			return ;
		}
	}
	{
		pTemplates = _pMapData->m_spWData->GetGameReg()->Find("SERVER\\RPGTEMPLATES");
	}
	if(pTemplates)
	{
//		pTemplates->EnableAutoHashing(false);

		spCRegistry spReg = pTemplates->EvalTemplate_r(_pReg);
		if(!spReg)
			return;

		IncludeRPGEvaledRegistry(spReg, _pMapData, _pWServer);
	}
}

void CRPG_Object::IncludeAnimFromKey(const CStr Key, const CRegistry *pReg, CMapData *_pMapData)
{
	MAUTOSTRIP(CRPG_Object_IncludeAnimFromKey, MAUTOSTRIP_VOID);
	if(pReg)
	{
		const CRegistry *pChild = pReg->FindChild(Key);
		if(pChild)
		{
			CFStr St = pChild->GetThisValue();
			if(St != "")
			{
				CFStr Anim = St.GetStrSep(":");
				if(Anim.CompareNoCase("animlist") != 0)
					_pMapData->GetResourceIndex_Anim(Anim);
			}
		}
	}
}

void CRPG_Object::IncludeShellTypeFromKey(const CFStr& _Key, const CRegistry* _pReg, CMapData* _pMapData)
{
	if(_pReg)
	{
		const CRegistry* pChild = _pReg->FindChild(_Key);
		CWO_ShellManager* pShellMgr = (CWO_ShellManager*)_pMapData->m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETSHELLMANAGER), _pMapData->m_pWServer->Game_GetObjectIndex());
		if(pShellMgr && pChild) pShellMgr->IncludeShellType(pChild->GetThisValuei(), _pMapData->m_pWServer);
	}
}

void CRPG_Object::IncludeModelFromKey(const CStr Key, const CRegistry *pReg, CMapData *_pMapData)
{
	MAUTOSTRIP(CRPG_Object_IncludeModelFromKey, MAUTOSTRIP_VOID);
	if(pReg)
	{
		const CRegistry *pChild = pReg->FindChild(Key);
		if(pChild)
			_pMapData->GetResourceIndex_Model(pChild->GetThisValue());
	}
}

void CRPG_Object::IncludeSoundFromKey(const CStr Key, const CRegistry *pReg, CMapData *_pMapData)
{
	MAUTOSTRIP(CRPG_Object_IncludeSoundFromKey, MAUTOSTRIP_VOID);
	if(pReg)
	{
		const CRegistry *pChild = pReg->FindChild(Key);
		if(pChild)
			_pMapData->GetResourceIndex_Sound(pChild->GetThisValue());
	}
}

void CRPG_Object::IncludeClassFromKey(const CStr Key, const CRegistry *pReg, CMapData *_pMapData)
{
	MAUTOSTRIP(CRPG_Object_IncludeClassFromKey, MAUTOSTRIP_VOID);
	if(pReg)
	{
		const CRegistry *pChild = pReg->FindChild(Key);
		if(pChild)
			_pMapData->GetResourceIndex_Class(pChild->GetThisValue());
	}
}

void CRPG_Object::IncludeRPGClassFromKey(const CStr Key, const CRegistry *pReg, CMapData *_pMapData, CWorld_Server *_pWServer, bool _bPrecacheForPlayer)
{
	MAUTOSTRIP(CRPG_Object_IncludeRPGClassFromKey, MAUTOSTRIP_VOID);
	if(pReg)
	{
		const CRegistry *pChild = pReg->FindChild(Key);
		if(pChild)
			IncludeRPGClass(pChild->GetThisValue(), _pMapData, _pWServer, _bPrecacheForPlayer);
	}
}

void CRPG_Object::IncludeSurfaceFromKey(const CStr Key, const CRegistry *pReg, CMapData *_pMapData)
{
	MAUTOSTRIP(CRPG_Object_IncludeSurfaceFromKey, MAUTOSTRIP_VOID);
	if(pReg)
	{
		const CRegistry *pChild = pReg->FindChild(Key);
		if(pChild)
			_pMapData->GetResourceIndex_Surface(pChild->GetThisValue());
	}
}
int32 CRPG_Object::GetItemAnimProperty(const CStr& _Str)
{
	CStr Str = _Str;
	return Str.TranslateFlags(ms_AnimProperty);
}

