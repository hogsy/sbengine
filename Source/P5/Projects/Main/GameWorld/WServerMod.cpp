
#include "PCH.h"

#include "WServerMod.h"

#include "../../Shared/MOS/Classes/GameWorld/WDataRes_AnimGraph2.h"
#include "../exe/WGameContextMain.h"
#include "../GameClasses/WObj_Game/WObj_GameMod.h"
#include "../GameClasses/WObj_Char.h"
//#include "../GameClasses/WRPG/WRPGCore.h"

MRTC_IMPLEMENT_DYNAMIC(CWServer_Mod, CWorld_ServerCore);

CWServer_Mod::CWServer_Mod()
{
	MAUTOSTRIP(CWServer_Mod_ctor, MAUTOSTRIP_VOID);
	m_AutoTestRunning = false;
}

bool CWServer_Mod::World_DeleteSave(CStr _SaveName)
{
	MAUTOSTRIP(CWServer_Mod_World_DeleteSave, false);

	M_TRY
	{
#ifdef PLATFORM_XBOX1
#if defined(M_DEMO_XBOX)
	const char *pSaveDrv = "Z:\\Saves\\";
#else
	const char *pSaveDrv = "U:\\";
#endif
		return XDeleteSaveGame(pSaveDrv, _SaveName.Unicode().StrW()) == ERROR_SUCCESS;
#else
		MACRO_GetSystem;
		CStr Path = pSys->TranslateSavePath(m_spWData->ResolvePath("Save\\") + _SaveName + "\\");
		bool bRet = CDiskUtil::DelTree(Path);
		bRet &= CDiskUtil::RemoveDir(Path);
		return bRet;
#endif
	}
	M_CATCH(
	catch(CCException _Ex)
	{
		return false;
	}
	)
}

bool CWServer_Mod::World_SaveExist(CStr _SaveName)
{
	MAUTOSTRIP(CWServer_Mod_World_SaveExist, false);

	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	if(pGame)
	{
		CGameContextMod *pGameContextMod = TDynamicCast<CGameContextMod>(pGame);
		if(pGameContextMod)
		{
			for(int32 i = 0; i < pGameContextMod->m_lSaveFiles.Len(); i++)
				if(pGameContextMod->m_lSaveFiles[i] == _SaveName)
					return true;
		}
	}
	return false;
}

void CWServer_Mod::World_Save(CStr _SaveName, bool _bDotest)
{
	MAUTOSTRIP(CWServer_Mod_World_Save, MAUTOSTRIP_VOID);
#ifdef PLATFORM_PS3
	return;
#endif
	MSCOPE(CWServer_Mod::World_Save, WSERVERMOD);
	if(m_GameState.SaveGame(this, _SaveName, _bDotest))
	{
		CWObject_Message Msg(OBJSYSMSG_GAME_POSTWORLDSAVE);
		Msg.m_pData = (void *)_SaveName.Str();
		Message_SendToObject(Msg, Game_GetObjectIndex());
	}
}

void CWServer_Mod::World_Load(CStr _SaveName)
{
	++m_Loading;
	ConExecute("cg_clearmenus();cg_dowindowswitch()");
	MAUTOSTRIP(CWServer_Mod_World_Load, MAUTOSTRIP_VOID);
	MSCOPE(CWServer_Mod::World_Load, WSERVERMOD);
	m_GameState.LoadGame(this, _SaveName);

	CWObject_Message Msg(OBJSYSMSG_GAME_POSTWORLDLOAD);
	Msg.m_pData = (void *)_SaveName.Str();
	Message_SendToObject(Msg, Game_GetObjectIndex());
	--m_Loading;

	if (!m_Loading)
	{
		// after loading level, run OnFinishDeltaLoad() on all objects (regardless of whether a load was actually performed)
		DoOnFinishDeltaLoad();
	}
}

void CWServer_Mod::World_Close()
{
	MAUTOSTRIP(CWServer_Mod_World_Close, MAUTOSTRIP_VOID);
	CWorld_ServerCore::World_Close();

	m_AnimCollector.Destroy();
	m_lItemAnimTypesNeeded.Clear();
	//m_lItemAnimsetsNeeded.Destroy();
	//m_ExtraAnimSetTypes = 0;

	m_lspEvalRPGTemplates.Clear();
}

void CWServer_Mod::World_Change(const CFStr &_WorldName, int _Flags)
{
	++m_Loading;
	ConExecute("cg_clearmenus();cg_dowindowswitch()");
	if(_Flags & SERVER_CHANGEWORLD_MIGRATION)
	{
		m_GameState.ChangeLevel(this, _WorldName, _Flags & (~SERVER_CHANGEWORLD_MIGRATION));
		CWObject_Message Msg(OBJSYSMSG_GAME_POSTWORLDLOAD);
		Message_SendToObject(Msg,Game_GetObjectIndex());
	}
	else
	{
		CWorld_ServerCore::World_Change(_WorldName, _Flags);
		m_GameState.RegisterOrgObjects(this);

		if(m_AutoTestRunning)
		{
			Con_ExecuteTestRun();
			if(!m_AutoTestRunning)
			{
				ConOutL("Warning: Breaking out of AutoTestRun\n");
				M_TRACEALWAYS("Warning: Breaking out of AutoTestRun\n");
			}
		}
	}
	--m_Loading;

	if (!m_Loading)
	{
		// after loading level, run OnFinishDeltaLoad() on all objects (regardless of whether a load was actually performed)
		DoOnFinishDeltaLoad();
	}
}

void CWServer_Mod::DoOnFinishDeltaLoad()
{
	TSelection<CSelection::LARGE_BUFFER> Sel;
	Selection_AddAll(Sel);
	const int16* piSel = NULL;
	uint nSel = Selection_Get(Sel, &piSel);
	for (uint i = 0; i < nSel; i++)
	{
		CWObject* pObj = m_lspObjects[piSel[i]];
		if (pObj)
			pObj->OnFinishDeltaLoad();
	}
}


static const char* s_SpawnFlagsTranslate1[] = 
{ 
	"easy", "medium", "hard", "commentary", 
	"ny1", "ny2", "ny3", "global4", "global5", "global6", "global7", "global8",
	"layer1", "layer2", "layer3", "layer4", "layer5", "layer6", "layer7", "layer8",
	"custom1", "custom2", "custom3", "custom4", "custom5", "custom6", "custom7", "custom8", "custom9", "custom10", "custom11", "once",
    NULL 
};
static const char* s_SpawnFlagsTranslate2[] = { "$$$", "$$$", "$$$", "$$$", "ow1", "ow2", NULL };


uint32 CWServer_Mod::World_TranslateSpawnFlags(const char* _pFlagsStr) const
{
	uint32 Flags = CStrBase::TranslateFlags(_pFlagsStr, s_SpawnFlagsTranslate1);
	Flags |= CStrBase::TranslateFlags(_pFlagsStr, s_SpawnFlagsTranslate2);
	return Flags;
}

const char* CWServer_Mod::World_GetCurrSpawnFlagsStr(uint32 _IgnoreMask) const
{
	static CFStr s_Result; // yeah yeah
	s_Result.Clear();

	bool bIsOtherworld = (CStrBase::cupr(m_WorldName[0]) == 'O' && CStrBase::cupr(m_WorldName[1]) == 'W');
	uint32 SpawnMask = m_CurSpawnMask & ~_IgnoreMask;
	for (uint i = 0, b = 1; i < 32; i++, b+=b)
		if (SpawnMask & b)
		{
			if ((i == 4 || i == 5) && bIsOtherworld)
				s_Result += CFStrF("+%s", s_SpawnFlagsTranslate2[i]);
			else
				s_Result += CFStrF("+%s", s_SpawnFlagsTranslate1[i]);
		}

	if (s_Result.IsEmpty())
		return NULL;
	return s_Result.Str() + 1; // skip first "+"
}


CRegistry* CWServer_Mod::GetEvaledRPGTemplate(const char *_pName)
{
	MAUTOSTRIP(CWServer_Mod_GetEvaledRPGTemplate, NULL);
	for(int i = 0; i < m_lspEvalRPGTemplates.Len(); i++)
	{
		if(m_lspEvalRPGTemplates[i]->GetThisName().CompareNoCase(_pName) == 0)
			return m_lspEvalRPGTemplates[i];
	}
	return NULL;
}

void CWServer_Mod::AddEvaledRPGTemplate(spCRegistry _spReg)
{
	MAUTOSTRIP(CWServer_Mod_AddEvaledRPGTemplate, MAUTOSTRIP_VOID);

//	TPtr<CRegistryCompiled> spCReg = DNew(CRegistryCompiled) CRegistryCompiled;
//	if (!spCReg)
//		MemError("AddEvaledRPGTemplate");
//	spCReg->Compile(_spReg);

//	m_lspEvalRPGTemplates.Add(spCReg);
	m_lspEvalRPGTemplates.Add(_spReg);
}

void CWServer_Mod::AddAnimContainerEntry(CStr _Name, int32 _iAnimSeq, int32 _iAnim)
{
	m_AnimCollector.AddEntry(_Name, _iAnimSeq, _iAnim);
}

void CWServer_Mod::RegisterAnimGraph2(int32 _iAnimGraph2Res)
{
	if (_iAnimGraph2Res < 0)
		return;
	// Check for duplicate list entries
	int32 Len = m_liAnimGraph2.Len();
	for (int32 i = 0; i < Len; i++)
	{
		if (m_liAnimGraph2[i] == _iAnimGraph2Res)
			return;
	}

	m_liAnimGraph2.Add(_iAnimGraph2Res);
}

void CWServer_Mod::LoadAnimations()
{
	MSCOPE(CWServer_Mod::LoadAnimations, RES_ANIM);
	m_AnimCollector.LoadData(m_spMapData, m_spWData);
	
	// Animations loaded, set container resources to animlists 
	TThinArray<CXRAG2*> lpAnimGraph2;
	int32 AnimGraphLen = m_liAnimGraph2.Len();
	lpAnimGraph2.SetLen(AnimGraphLen);
	for (int32 i = 0; i < AnimGraphLen; i++)
	{
		CWRes_AnimGraph2* pAnimGraphRes = m_spMapData->GetResource_AnimGraph2(m_liAnimGraph2[i]);
			
		lpAnimGraph2[i] = pAnimGraphRes ? pAnimGraphRes->GetAnimGraph() : NULL;
		if (lpAnimGraph2[i])
			lpAnimGraph2[i]->ClearAnimSequenceCache();
	}

	// Triple loop, yay
	int32 ContainerLen = m_AnimCollector.m_Containers.Len();
	for (int32 i = 0; i < ContainerLen; i++)
	{
		for (int32 k = 0; k < AnimGraphLen; k++)
		{
			if (lpAnimGraph2[k])
				lpAnimGraph2[k]->SetAnimContainerResource(m_AnimCollector.m_Containers[i].m_ContainerName, m_AnimCollector.m_Containers[i].m_iContainerResource);
		}
	}
}

void CWServer_Mod::AddAnimTypeItem(int32 _Type)
{
	m_lItemAnimTypesNeeded.Add(_Type);
}

void CWServer_Mod::mpGetGameMode()
{
	CStr cmd;
	cmd = m_spGameReg->GetValue("DEFAULTGAMEMODE");

	CWObject_Message Msg(OBJMSG_GAME_SETGAMEMODE);
//	int Mode = 0;
	if(cmd == '1')
		Msg.m_Param0 = 1;
	else if(cmd == '2')
		Msg.m_Param0 = 2;
	else
		Msg.m_Param0 = 0;
	Message_SendToObject(Msg, Game_GetObjectIndex());
}



void CWServer_Mod::Con_DialogueInfo(CStr _Char)
{
	TSelection<CSelection::SMALL_BUFFER> Selection;
	Selection_AddTarget(Selection, _Char.Str());

	int16* pSel = NULL;
	int nSel = Selection_Get(Selection, (const int16**)&pSel);

	for(int i = 0; i < nSel; i++)
	{
		CWObject *pObj = Object_Get(pSel[i]);
		CWObject_Character *pChar = TDynamicCast<CWObject_Character>(pObj);
		if(pChar)
		{
			pChar->Char_DebugDialogueInfo();
		}
	}
	if(!nSel)
		ConOutL("Character not found!");
}

void CWServer_Mod::Con_ExecuteTestRun()
{
	// get what layer we are running right now
	CRegistry* pReg = Registry_GetLevelKeys("");
	CStr ScriptLayerName = pReg->GetThisName();
	
	if(ScriptLayerName == "LAYER")
		ScriptLayerName = pReg->GetThisValue();

	if(ScriptLayerName == "")
	{
		// ERROR!
		return;
	}
	
	// with the scriptlayer name, now read the TestRun.xmp file, and get all
	// the timed messages

	spCRegistry spRegistry;

	MACRO_GetSystem;
	CStr Params;
	CStr ContentPath = pSys->GetEnvironment()->GetValue("CONTENTPATH");
	if (ContentPath == "")
		ContentPath = pSys->m_ExePath + "Content\\";

	CStr RegisterFile = ContentPath + "Dev\\";
	RegisterFile += "TestRun";

	// Read register
	CStr FileName = RegisterFile.GetPath() + RegisterFile.GetFilenameNoExt() + ".xmp";
	if (CDiskUtil::FileExists(FileName))
	{
		spRegistry = REGISTRY_CREATE;
		if(spRegistry) 
			spRegistry->XRG_Read(FileName);
		else
		{
			ConOut("Error reading Registry.");
			return;
		}

	}
	else
	{
		ConOutL(CStrF("§cf00ERROR (CWServer_Mod::Con_ExecuteTestRun): file '%s' not found!", RegisterFile.Str()));
		M_TRACEALWAYS("ERROR (CWServer_Mod::Con_ExecuteTestRun): file '%s' not found!\n", RegisterFile.Str());
		return;
	}

	m_AutoTestRunning = false;
	bool bOkToBreak = false;
	CRegistry *pRegMain = spRegistry->Find("WORLDSPAWN");
	if(pRegMain)
	{
		int iNrOfChildren = pRegMain->GetNumChildren();
		for (int i = 0; i < iNrOfChildren; i++)
		{
			CRegistry *pEnginePath = pRegMain->GetChild(i);
			if(pEnginePath->GetThisName() == "ENGINE_PATH")
			{
				CRegistry *pTargetName;
				pTargetName = pEnginePath->Find("TARGETNAME");
				if(pTargetName->GetThisValue().UpperCase() == ScriptLayerName.UpperCase())
				{
					// setup regkeys and create the enginepath
					bOkToBreak = true;
					spCRegistry spKeys;
					spKeys = REGISTRY_CREATE;
					spKeys->CopyDir(pEnginePath);
					spKeys->SetThisName("");

					CRegistry *pAddReg = spKeys->Find("OGR_CLASSNAME");
					if(pAddReg)
					{
						pAddReg->SetThisName("CLASSNAME");
						pAddReg->SetThisValue("engine_path");
					}
					
					int iObj = Object_Create(*spKeys);
					CWObject *pObj = Object_Get(iObj);
					if(pObj)
					{
						pObj->m_bNoSave = true;
						m_AutoTestRunning = true;
					}
				}
			}
			if(bOkToBreak)
				break;
		}
	}
}

#ifndef M_RTM
void CWServer_Mod::Con_GlassWire(int _v)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys)
		pSys->GetEnvironment()->SetValuei("SV_GLASSWIRE", _v);
}
#endif

void CWServer_Mod::Register(CScriptRegisterContext & _RegContext)
{
	_RegContext.RegFunction("mp_getgamemode", this, &CWServer_Mod::mpGetGameMode);
	_RegContext.RegFunction("sv_dialogueinfo", this, &CWServer_Mod::Con_DialogueInfo);
	_RegContext.RegFunction("sv_executetestrun", this, &CWServer_Mod::Con_ExecuteTestRun);

#ifndef M_RTM
	_RegContext.RegFunction("sv_glasswire", this, &CWServer_Mod::Con_GlassWire);
#endif
	
	CWorld_ServerCore::Register(_RegContext);
}

/*void CWServer_Mod::AddItem(CStr _ItemAnimSetsNeeded, int32 _ItemAnimType)
{
	// Only add for itemtypes not already added
	if (m_ItemAnimTypes & _ItemAnimType)
		return;

	m_lItemAnimsetsNeeded.Add(_ItemAnimSetsNeeded);
	m_ItemAnimTypes |= _ItemAnimType;
}

void CWServer_Mod::AddExtraAnimSet(CStr _ItemAnimset, int32 _ExtraSetType)
{
	if (m_ExtraAnimSetTypes & _ExtraSetType)
		return;

	m_lItemAnimsetsNeeded.Add(_ItemAnimset);
	m_ExtraAnimSetTypes |= _ExtraSetType;
}
int32 CWServer_Mod::GetItemAnimTypes() const
{
	return m_ItemAnimTypes;
}
*/
const TArray<int32>& CWServer_Mod::GetItemAnimTypesNeeded() const
{
	return m_lItemAnimTypesNeeded;
}

void CAnimContainerCollector::AddEntry(CStr _Name, int32 _iAnimSeq, int32 _iAnim)
{
	// Bleh, go through list of container names
	CStr Upper = _Name.UpperCase();
	int32 Len = m_Containers.Len();
	for (int32 i = 0; i < Len; i++)
	{
		if (m_Containers[i].m_ContainerName == Upper)
		{
			m_Containers[i].m_SeqHandler.AddEntry(_iAnimSeq);
			if (_iAnim != -1)
				m_Containers[i].m_liAnims.Add(_iAnim);
			return;
		}
	}
	m_Containers.SetLen(Len+1);
	m_Containers[Len].m_ContainerName = Upper;
	m_Containers[Len].m_SeqHandler.AddEntry(_iAnimSeq);
	if (_iAnim != -1)
		m_Containers[Len].m_liAnims.Add(_iAnim);
}

void CAnimContainerCollector::LoadData(CMapData* _pMapData, CWorldData* _pWData)
{
	int32 Len = m_Containers.Len();
	for (int32 i = 0; i < Len; i++)
	{
		// if container is already loaded, ignore it
		if (m_Containers[i].m_liAnims.Len())
			continue;

		CStr ResName = m_Containers[i].m_ContainerName + ":" + m_Containers[i].m_SeqHandler.GetHexString();
		_pMapData->GetResourceIndex_Anim(m_Containers[i].m_ContainerName + ":" + m_Containers[i].m_SeqHandler.GetHexString());
		m_Containers[i].m_iContainerResource = _pWData->GetResourceIndex("ANM:" + ResName, WRESOURCE_CLASS_XSA, _pMapData);
		/*spCWResource spRef = _pWData->GetResourceRef(iAnimContainerResource);

		int32 AnimLen = m_Containers[i].m_liAnims.Len();
		for (int32 j = 0; j < AnimLen; j++)
		{
			int32 iAnim = m_Containers[i].m_liAnims[j];
			_pAnimList->SetContainer(iAnim, iAnimContainerResource, spRef);
		}*/
	}
}

void CAnimContainerCollector::CSequenceHandler::AddEntry(aint _Pos)
{
	if (_Pos < 0)
		return;

	aint Index = _Pos / 8;
	aint Shift = _Pos % 8;
	aint Len = m_BitArray.Len();
	if (Index >= Len)
	{
		m_BitArray.SetLen(Index+1);

		// Reset bitfields
		for (mint i = Len; i <= Index; i++)
		{
			m_BitArray[i] = 0;
		}
	}
	m_BitArray[Index] |= 1 << Shift;
}

char CAnimContainerCollector::CSequenceHandler::GetHex(uint8 _Val)
{
	switch(_Val)
	{
	case 15:
		return 'f';
	case 14:
		return 'e';
	case 13:
		return 'd';
	case 12:
		return 'c';
	case 11:
		return 'b';
	case 10:
		return 'a';
	case 9:
		return '9';
	case 8:
		return '8';
	case 7:
		return '7';
	case 6:
		return '6';
	case 5:
		return '5';
	case 4:
		return '4';
	case 3:
		return '3';
	case 2:
		return '2';
	case 1:
		return '1';
	case 0:
		return '0';
	}

	return '0';
}

CStr CAnimContainerCollector::CSequenceHandler::GetHexString()
{
	CStr Str;
	int32 Len = m_BitArray.Len();
	for (int32 i = Len-1; i >= 0; i--)
	{
		Str += CStr(GetHex(m_BitArray[i] >> 4));
		Str += CStr(GetHex(m_BitArray[i] & 0x0f));
	}
	return Str;
}
