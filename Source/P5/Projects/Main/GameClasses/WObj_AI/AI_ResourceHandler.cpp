#include "PCH.h"
#include "AI_ResourceHandler.h"
#include "AI_Action/AI_Action.h"
#include "AI_ItemHandler.h"
#include "../WObj_Game/WObj_GameMod.h"

//CAI_ResourceHandler///////////////////////////////////////////////////////////////////////////////////
bool CAI_ResourceHandler::CNameList::operator==(const CNameList& _Val)
{
	if (m_lNames.Len() == _Val.m_lNames.Len())
	{
		for (int i = 0; i < m_lNames.Len(); i++)
		{	
			if (m_lNames[i] != _Val.m_lNames[i])
				return false;
		}
		return true;
	}
	else
	{
		return false;
	}
};


CAI_ResourceHandler::CAI_ResourceHandler()
{
	m_lActionDefinitions.Clear();
	m_lWeaponHandlerMap.Create(CNameList(), 10);

	m_pServer = NULL;

	// Setup ai globals
	ms_ServerTick = 0;
	ms_ServerTickFraction = 0.0f;
	ms_GameDifficulty = DIFFICULTYLEVEL_NORMAL;
	ms_Gamestyle = CWObject_GameP4::GAMESTYLE_SNEAK;
	ms_Tension = -1;

	// Passing NULL as server is no problem here as no users should exist when constructor is called
	if (!ms_PFResource_Grid.IsInitialized())
		ms_PFResource_Grid.Init(2, CAI_Resource_Pathfinding::GRID,NULL);
	if (!ms_PFResource_Graph.IsInitialized())
		ms_PFResource_Graph.Init(2, CAI_Resource_Pathfinding::GRAPH,NULL);
	
	// No initialization for ms_ActivityCounter needed

	if (!ms_FlashlightUsers.IsInitialized())
	{	// Two simultaneous flashlight users
		ms_FlashlightUsers.Init(2,1,1);
	}
	ms_bDebugRender = true;
	ms_iDebugRenderTarget = 0;
};

void CAI_ResourceHandler::Init(CWorld_Server* _pServer)
{
	m_pServer = _pServer;
	OnRefresh();
};

int CAI_ResourceHandler::GetAITick()
{
	return(ms_ServerTick);
};

CAI_ResourceHandler::~CAI_ResourceHandler()
{
	m_lActionDefinitions.Clear();
	m_lWeaponHandlerMap.Clear();
	ms_FlashlightUsers.Destroy();
};

//Handle registry keys
void CAI_ResourceHandler::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	if (!_pKey)
		return;
		
	const CStr KeyName = _pKey->GetThisName();

	switch (_KeyHash)
	{
	case MHASH3('AI_A','CTIO','N'): // "AI_ACTION"
		{
			//Add action definition
			int i = m_lActionDefinitions.Add(CAI_ActionDefinition());
			m_lActionDefinitions[i].Create(_pKey, this);
			break;
		}
	case MHASH4('AI_W','EAPO','NHAN','DLER'): // "AI_WEAPONHANDLER"
		{
			//Get type from ai type name
			int Type = CAI_WeaponHandler::GetWeaponType(_pKey->GetThisValue());
		
			if (Type != CAI_Weapon::UNKNOWN)
			{
				//Get action definition names from subkeys
				CNameList Names;
				for (int i = 0; i < _pKey->GetNumChildren(); i++)
				{
					if (_pKey->GetName(i) == "ACTION")
					{
						Names.m_lNames.Add(_pKey->GetValue(i));
					}
				}

				if (Names.m_lNames.Len() > 0)
					m_lWeaponHandlerMap.Add(Type, Names);
			}
			break;
		}
	default:
		{
			break;
		}
	}
};

//Get pointer to action definition for given name
CAI_ActionDefinition * CAI_ResourceHandler::GetActionDefinition(CStr _Str)
{
	for (int i = 0; i < m_lActionDefinitions.Len(); i++)
	{
		if (m_lActionDefinitions[i].MatchName(_Str))
		{
			return &(m_lActionDefinitions[i]);
		}
	}
	return NULL;
}; 


//Add all appropriate action dnames for the given weapon type to given list.
//Fail if there are no action definitions for that type
bool CAI_ResourceHandler::GetWeaponHandlerActions(int _WeaponType, TArray<CStr> * _plRes)
{
	CNameList Names = m_lWeaponHandlerMap.Get(_WeaponType);
	if (Names.m_lNames.Len())
	{
		_plRes->Insertx(_plRes->Len(), Names.m_lNames.GetBasePtr(), Names.m_lNames.Len());
		return true;
	}
	else
		return false;
};



//Set specific pause flags
void CAI_ResourceHandler::Pause(int _PauseFlags)
{
	m_PauseFlags |= _PauseFlags;
};


//Remove specific pause flags
void CAI_ResourceHandler::Unpause(int _PauseFlags)
{
	m_PauseFlags &= ~_PauseFlags;
};


//Clear all pause flags
void CAI_ResourceHandler::UnpauseAll()
{
	m_PauseFlags = 0;
};


//Check if AI is allowed to tale non-scripted actions
bool CAI_ResourceHandler::AllowActions()
{
	//Currently any pause flag means non-scripted actions should be paused
	return (m_PauseFlags == 0);
};

void CAI_ResourceHandler::OnRefresh()
{
	ms_ServerTickFraction += m_pServer->GetGameTickTime();
	if (ms_ServerTickFraction >= AI_TICK_DURATION)
	{
		ms_ServerTick++;
		ms_ServerTickFraction -= AI_TICK_DURATION;
	}
};

void CAI_ResourceHandler::CleanStatic()
{
	ms_ServerTick = 0;
	m_KBGlobals.CleanStatic();
	m_ActionGlobals.CleanStatic();

	ms_PFResource_Grid.Clean();
	ms_PFResource_Graph.Clean();
	ms_FlashlightUsers.ReleaseAll();
};

void CAI_ResourceHandler::OnDeltaLoad(CCFile* _pFile)
{
	int8 Temp8;
	_pFile->ReadLE(Temp8); m_PauseFlags = Temp8;

	//Dialogues and cutscenes should are currently not saved so that they 
	//continue after load, and will therefore never unpause AI. Thus we must 
	//explicitly reset some pause flags on loading.
    Unpause(PAUSE_DIALOGUE | PAUSE_CUTSCENE);
};

void CAI_ResourceHandler::OnDeltaSave(CCFile* _pFile)
{
	int8 Temp8;
	Temp8 = m_PauseFlags; _pFile->WriteLE(Temp8);
};



//CAI_ActionDefinition/////////////////////////////////////////////////////////////////////////////////

CAI_ActionDefinition::CAI_ActionDefinition()
{
	m_lKeys.Clear();
	m_lInts.Clear();
	m_lFloats.Clear();
	m_lVecs.Clear();
	m_lParamMap.Create(-1, 3);
	m_Name = "NoName";
	m_Type = CAI_Action::INVALID;
	m_pParent = NULL;
};

//Create action definition from registry key
void CAI_ActionDefinition::Create(const CRegistry* _pKey, CAI_ResourceHandler * _pAIRes)
{
	if (!_pKey || !_pAIRes)
		return;

	m_Name = _pKey->GetThisValue().UpperCase();
	//const char* n = m_Name.Str();//DEBUG
	for (int i = 0; i < _pKey->GetNumChildren(); i++)
	{ 
		const CRegistry* pReg = _pKey->GetChild(i);
		OnEvalKey(pReg->GetThisNameHash(), pReg, _pAIRes);	
	}
	OnFinishedEvalKeys();
};

//Handle registry keys
void CAI_ActionDefinition::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey, CAI_ResourceHandler * _pAIRes)
{
	if (!_pKey || !_pAIRes)
		return;

	const CStr KeyName = _pKey->GetThisName();
	CStr Val = _pKey->GetThisValue();

	//const char* n = Name.Str();//DEBUG
	//const char* v = Val.Str();//DEBUG

	switch (_KeyHash)
	{
	case MHASH3('CLAS','SNAM','E'): // "CLASSNAME"
		{
			//Find parent definition
			m_pParent = _pAIRes->GetActionDefinition(Val);
			if (!m_pParent)
			{
				//Base class, set type from classname
				m_Type = CAI_Action::StrToType(Val);
			}
			break;
		}
	default:
		{
			//Parameter, add to keys
			m_lKeys.Add(_pKey->Duplicate());
			break;
		}
	}
};


//Set parameter values properly and remove junk
void CAI_ActionDefinition::OnFinishedEvalKeys()
{
	//Inherit type from parent or get it from name if not previously set
//	const char* n = m_Name.Str();//DEBUG
	if (m_Type == CAI_Action::INVALID)
	{
		if (m_pParent)
		{
			m_Type = m_pParent->m_Type;
		}
		else
		{
			m_Type = CAI_Action::StrToType(m_Name);		
		}
	}

	//Set parameters from keys, if type has been set
	if (m_Type != CAI_Action::INVALID)
	{
		int Param;
		int Type;

		//Create dummy action object to get param info from
		spCAI_Action spDummy = CAI_Action::CreateAction(m_Type, NULL);

		if (spDummy)
		{
			for (int i = 0; i < m_lKeys.Len(); i++)
			{
				//Get param ID and type. Don't add duplicates of previous param
				if (((Param = spDummy->StrToParam(m_lKeys[i]->GetThisName(), &Type)) != CAI_Action::PARAM_INVALID) &&
					(m_lParamMap.Get(Param) == m_lParamMap.Null()))
				{
					int Index;
					switch (Type)
					{
					case CAI_Action::PARAMTYPE_PRIO:
						{
							Index = m_lInts.Add(spDummy->StrToPrio(m_lKeys[i]->GetThisValue()));
						}
						break;
					case CAI_Action::PARAMTYPE_FLAGS:
						{
							Index = m_lInts.Add(spDummy->StrToFlags(m_lKeys[i]->GetThisValue()));
						}
						break;
					case CAI_Action::PARAMTYPE_INT:
						{
							Index = m_lInts.Add(m_lKeys[i]->GetThisValuei());
						}
						break;
					case CAI_Action::PARAMTYPE_FLOAT:
						{
							Index = m_lFloats.Add(m_lKeys[i]->GetThisValuef());
						}
						break;
					case CAI_Action::PARAMTYPE_VEC:
						{
							CVec3Dfp32 Vec;
							Vec.ParseString(m_lKeys[i]->GetThisValue());
							Index = m_lVecs.Add(Vec);
						}
						break;
					case CAI_Action::PARAMTYPE_TARGET:
					case CAI_Action::PARAMTYPE_SPECIAL:
						{
							Index = m_lStrings.Add(m_lKeys[i]->GetThisValue());
						}
						break;
					default:
						Index = -1;
					}

					if (Index != -1)
					{
						//Create list index number and add to hash.
						int ListIndex = ((Index << INDEX_SHIFT) | (Type & LIST_MASK));
						m_lParamMap.Add(Param, ListIndex);
					}
				}
			}
		}

		//Copy any non-defined parameters from parent if we've got one
		if (m_pParent)
		{
			TArray<int> lParams;
			TArray<int> lTypes;
			int nParams = m_pParent->GetParamIDs(&lParams, &lTypes);
			for (int i = 0; i < nParams; i++)
			{
				//Don't overwrite params already defined from keys above
				if (m_lParamMap.Get(lParams[i]) == m_lParamMap.Null())
				{
					int Index = -1;
					switch (lTypes[i])
					{
					case CAI_Action::PARAMTYPE_PRIO:
					case CAI_Action::PARAMTYPE_FLAGS:
					case CAI_Action::PARAMTYPE_INT:
						{
							int Val;
							if (m_pParent->GetParam(lParams[i], Val))
								Index = m_lInts.Add(Val);
						}
						break;
					case CAI_Action::PARAMTYPE_FLOAT:
						{
							fp32 Val;
							if (m_pParent->GetParam(lParams[i], Val))
								Index = m_lFloats.Add(Val);
						}
						break;
					case CAI_Action::PARAMTYPE_VEC:
						{
							CVec3Dfp32 Val;
							if (m_pParent->GetParam(lParams[i], Val))
								Index = m_lVecs.Add(Val);
						}
						break;
					case CAI_Action::PARAMTYPE_TARGET:
					case CAI_Action::PARAMTYPE_SPECIAL:
						{
							CStr Val;
							if (m_pParent->GetParam(lParams[i], Val))
								Index = m_lStrings.Add(Val);
						}
						break;
					}

					if (Index != -1)
					{
						//Create list index number and add to hash.
						int ListIndex = ((Index << INDEX_SHIFT) | (lTypes[i] & LIST_MASK));
						m_lParamMap.Add(lParams[i], ListIndex);
					}
				}
			}

			//Remove parent pointer
			m_pParent = NULL;
		}
	}

	//Remove keys
	m_lKeys.Destroy();
};


//Check if name matches given string
bool CAI_ActionDefinition::MatchName(CStr _Str)
{
	return (m_Name == _Str.UpperCase()) != 0;
};	

//Type accessor
int CAI_ActionDefinition::GetType()
{
	return m_Type;
};

//Parameter accessors; set given value to value of given parameter if defined 
//or fail if not defined
bool CAI_ActionDefinition::GetParam(int _Param, int& _Val)
{
	int ListIndex = m_lParamMap.Get(_Param);
	if ((ListIndex != m_lParamMap.Null()) &&
	    (((ListIndex & LIST_MASK) == CAI_Action::PARAMTYPE_INT) ||
		 ((ListIndex & LIST_MASK) == CAI_Action::PARAMTYPE_FLAGS) ||
		 ((ListIndex & LIST_MASK) == CAI_Action::PARAMTYPE_PRIO)) &&
	 	m_lInts.ValidPos(ListIndex >> INDEX_SHIFT))
	{
		_Val = m_lInts[ListIndex >> INDEX_SHIFT];
		return true;
	}
	else
		return false;
};
bool CAI_ActionDefinition::GetParam(int _Param, fp32& _Val)
{
	int ListIndex = m_lParamMap.Get(_Param);
	if ((ListIndex != m_lParamMap.Null()) &&
		((ListIndex & LIST_MASK) == CAI_Action::PARAMTYPE_FLOAT) &&
		m_lFloats.ValidPos(ListIndex >> INDEX_SHIFT))
	{
		_Val = m_lFloats[ListIndex >> INDEX_SHIFT];
		return true;
	}
	else
		return false;
};
bool CAI_ActionDefinition::GetParam(int _Param, CVec3Dfp32& _Val)
{
	int ListIndex = m_lParamMap.Get(_Param);
	if ((ListIndex != m_lParamMap.Null()) &&
		((ListIndex & LIST_MASK) == CAI_Action::PARAMTYPE_VEC) &&
		m_lVecs.ValidPos(ListIndex >> INDEX_SHIFT))
	{
		_Val = m_lVecs[ListIndex >> INDEX_SHIFT];
		return true;
	}
	else
		return false;
};
bool CAI_ActionDefinition::GetParam(int _Param, CStr& _Val)
{
	int ListIndex = m_lParamMap.Get(_Param);
	if ((ListIndex != m_lParamMap.Null()) &&
		(((ListIndex & LIST_MASK) == CAI_Action::PARAMTYPE_TARGET) || ((ListIndex & LIST_MASK) == CAI_Action::PARAMTYPE_SPECIAL)) &&
		m_lStrings.ValidPos(ListIndex >> INDEX_SHIFT))
	{
		_Val = m_lStrings[ListIndex >> INDEX_SHIFT];
		return true;
	}
	else
		return false;
};


//Get all of the parameter IDs and corresponding types into given arrays. 
//Return the number of params.
int CAI_ActionDefinition::GetParamIDs(TArray<int>* _plParams, TArray<int>* _plTypes)
{
	int NumParams = m_lInts.Len() + m_lFloats.Len() + m_lVecs.Len() + m_lStrings.Len();

	if (!_plParams || !_plTypes)
		return NumParams;

	_plParams->SetLen(NumParams);
	_plTypes->SetLen(NumParams);

	//Since hash type doesn't support iteration we'll do this brute force. Won't actually be
	//that bad since param IDs should be pretty tightly packed.
	int iParam = 0;
	int nParamFound = 0;
	int ListIndex;
	while ((nParamFound < NumParams) && (iParam < CAI_Action::PARAM_MAX))	
	{
		ListIndex = m_lParamMap.Get(iParam);
		if (ListIndex != m_lParamMap.Null())
		{
			//Param found, add to lists
			(*_plParams)[nParamFound] = iParam;
			(*_plTypes)[nParamFound] = (ListIndex & LIST_MASK);
			nParamFound++;
		}

		iParam++;
	}

	if (nParamFound < NumParams)
	{
		//Shouldn't happen actually but I might fuck up or redesign later (or something)
		_plParams->SetLen(nParamFound);
		_plTypes->SetLen(nParamFound);
	}

	return nParamFound;
};



