
#include "PCH.h"


#include "WObj_Emitter.h"

//-------------------------------------------------------------------
// Spell Emitter
//-------------------------------------------------------------------

void CWObject_Emitter::OnCreate()
{
	MAUTOSTRIP(CWObject_Emitter_OnCreate, MAUTOSTRIP_VOID);
	m_ClientFlags |= CWO_CLIENTFLAGS_NOUPDATE;
	
	m_Freq = 1;
	m_MinDuration = 0;
	m_Distortion = 0;
	m_iAnim0 = 1;
	m_Velocity = 0;
	m_lspEmiteeRegs.Clear();	
}

void CWObject_Emitter::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Emitter_OnEvalKey, MAUTOSTRIP_VOID);
	
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();
	int KeyValuei = KeyValue.Val_int();
	
	switch (_KeyHash)
	{
	case MHASH3('FREQ','UENC','Y'): // "FREQUENCY"
		{
			m_Freq = KeyValuef * m_pWServer->GetGameTickTime();
			break;
		}
	
	case MHASH2('VELO','CITY'): // "VELOCITY"
		{
			m_Velocity = KeyValuef;
			break;
		}
	
	case MHASH3('MIND','URAT','ION'): // "MINDURATION"
		{
			m_MinDuration = (int32)(KeyValuef * m_pWServer->GetGameTicksPerSecond());
			//Set creation game tick so that we always can emit immediately after creation
			m_CreationGameTick = m_MinDuration + 1;
			break;
		}
	
	case MHASH3('DIST','ORTI','ON'): // "DISTORTION"
		{
			m_Distortion = KeyValuef;
			break;
		}
	
	case MHASH2('WIDT','H'): // "WIDTH"
		{
			m_iAnim1 = KeyValuei;
			break;
		}
	
	case MHASH2('QUAN','TITY'): // "QUANTITY"
		{
			m_iAnim0 = KeyValuei;
			break;
		}

	default:
		{
			if (KeyName.CompareSubStr("SPAWN") == 0)
			{
				uint iSlot = atoi(KeyName.Str() + 5);
				m_lSpawn.SetMinLen(iSlot + 1);
				m_lSpawn[iSlot] = KeyValue;
				m_pWServer->GetMapData()->GetResourceIndex_Class(KeyValue);
			}
			else if(KeyName.CompareSubStr("EMITEEREG:") == 0)
			{
				CStr Name = KeyName;
				Name.GetStrSep(":");
				//		const char * n = Name.Str();//DEBUG
				spCRegistry spEmiteeReg = _pKey->Duplicate();
				spEmiteeReg->SetThisKey(Name, KeyValue);
				m_lspEmiteeRegs.Add(spEmiteeReg);
			}
			else
				CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

int CWObject_Emitter::Emit(fp32 _Velocity, int _iSender)
{
	MAUTOSTRIP(CWObject_Emitter_Emit, 0);
	int nEmissions = 0;
	fp32 Width = m_iAnim1;

	CMat4Dfp32 Mat = GetPositionMatrix();
	CVec3Dfp32 &Left = CVec3Dfp32::GetMatrixRow(Mat, 1);
	CVec3Dfp32 Begin = CVec3Dfp32::GetMatrixRow(Mat, 3) - Left * (Width * 0.5f);
	CVec3Dfp32 Add = Left * (Width / fp32(m_iAnim0));

	Begin.SetMatrixRow(Mat, 3);

	for(int i = 0; i < m_iAnim0; i++)
	{
		CMat4Dfp32 Pos;
		if(m_Distortion > 0)
		{
			CMat4Dfp32 RotMat;
			RotMat.SetXRotation(Random); // Angle;
			RotMat.RotY_x_M(m_Distortion * Random); //Radius
			RotMat.Multiply(Mat, Pos);
		}
		else
			Pos = Mat;

		int nSpawns = m_lSpawn.Len();
		if(nSpawns > 0)
		{
			int iSpawn = (int)(Random * nSpawns);
			iSpawn = Clamp(iSpawn, 0, nSpawns - 1);
			int iObj = m_pWServer->Object_Create(m_lSpawn[iSpawn], Pos, m_iObject);
			CWObject * pObj = m_pWServer->Object_Get(iObj);
			if(pObj == NULL)
			{
				ConOut(CStrF("CWObject_Emitter::Emit, Failed to Emit %s at %s", (char *)m_lSpawn[iSpawn], (char *)CVec3Dfp32::GetMatrixRow(Pos, 3).GetFilteredString()));
			}
			else
			{
				for (int i = 0; i < m_lspEmiteeRegs.Len(); i++)
				{
					//Let emitee evaluate registries. Note that this might not work for all kinds of 
					//keys since some stuff need to be handled when object is created.
					//This should really be done when object is created...FIX
					const CRegistry* pReg = m_lspEmiteeRegs[i];
					pObj->OnEvalKey(pReg->GetThisNameHash(), pReg);
				}
				nEmissions++;
				m_pWServer->Object_SetVelocity(iObj, CVec3Dfp32::GetMatrixRow(Pos, 0) * _Velocity);
			}
		}

		CVec3Dfp32::GetRow(Mat, 3) += Add;
	}

	m_CreationGameTick = 0;
	m_CreationGameTickFraction = 0.0f;
	return nEmissions;
}

aint CWObject_Emitter::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Emitter_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
	case OBJMSG_IMPULSE:
		Emit(m_Velocity, _Msg.m_iSender);
		return 1;

	case OBJMSG_EMITTER_EMIT:
		//Don't emit if we're on minduration hold
		if ((m_MinDuration == 0) || (m_CreationGameTick > m_MinDuration))
			return Emit(m_Velocity, _Msg.m_iSender);
		else
			return 0;

	case OBJMSG_EMITTER_EMITWITHVELOCITY:
		Emit(*(fp32 *)_Msg.m_Param0, _Msg.m_iSender);
		return 1;
		
	case OBJMSG_SPAWNER_CANSPAWN:
		return 1;

	case OBJMSG_SPAWNER_CANSPAWNNAMED:
		{
			//Targetname of spawn is either set by emiteereg or in template
			CStr OwnName = CStr("");

			//Check emiteeregs
			for (int i = 0; i < m_lspEmiteeRegs.Len(); i++)
			{
				if (m_lspEmiteeRegs[i]->GetThisName() == "TARGETNAME")
				{
					OwnName = m_lspEmiteeRegs[i]->GetThisValue();
					break;
				}
			}

			//Must we check template? (This is actually slightly incorrect since only one of all spawns names need to match
			if (OwnName == CStr(""))
			{
				//Create dummy objects
				for (int i = 0; i < m_lSpawn.Len(); i++)
				{
					int iDummy = m_pWServer->Object_Create(m_lSpawn[i], GetPositionMatrix(), m_iObject);
					CWObject * pDummy = m_pWServer->Object_Get(iDummy);
					if(pDummy != NULL)
					{
						//Get name and destroy dummy
						OwnName = pDummy->GetName();
						m_pWServer->Object_Destroy(iDummy);

						//Check if name matches
						if (OwnName.CompareNoCase((char *)_Msg.m_pData) == 0)
							return 1;
					}
				}
			}
			else
			{
				//Check if name matches
				if (OwnName.CompareNoCase((char *)_Msg.m_pData) == 0)
					return 1;
			}

			//No matching name
			return 0;
		}
	case OBJMSG_SPAWNER_ONLOADOWNED:
		{
			//Values set by emitee regs will usually not be saved (targetname, events etc) so we must 
			//give an owned object these regs again when loading.
			CWObject * pObj = m_pWServer->Object_Get(_Msg.m_iSender);
			if (pObj)
			{
				for (int i = 0; i < m_lspEmiteeRegs.Len(); i++)
				{
					//Let emitee evaluate registries. 
					const CRegistry* pReg = m_lspEmiteeRegs[i];
					pObj->OnEvalKey(pReg->GetThisNameHash(), pReg);
				}
			}
			return 1;
		}

	default :
		return CWObject_Model::OnMessage(_Msg);
	}
}

void CWObject_Emitter::OnRefresh()
{
	MAUTOSTRIP(CWObject_Emitter_OnRefresh, MAUTOSTRIP_VOID);
	if(m_MinDuration > 0 && m_CreationGameTick > m_MinDuration)
	{
		if(Random < m_Freq)
			Emit(m_Velocity, m_iObject);
	}
	m_CreationGameTick++;
}

void CWObject_Emitter::OnLoad(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Emitter_OnLoad, MAUTOSTRIP_VOID);
	CWObject::OnLoad(_pFile);
	_pFile->ReadLE(m_Freq);
	_pFile->ReadLE(m_MinDuration);

	//Spawn is only changed in OnEvalKey, so we don't need to save/load this. This should be true for minduration and frq as well, but I'm not sure
}

void CWObject_Emitter::OnSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Emitter_OnSave, MAUTOSTRIP_VOID);
	CWObject::OnSave(_pFile);
	_pFile->WriteLE(m_Freq);
	_pFile->WriteLE(m_MinDuration);

	//Spawn is only changed in OnEvalKey, so we don't need to save/load this. This should be true for minduration and frq as well, but I'm not sure
}

void CWObject_Emitter::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	int32 Temp;
	_pFile->ReadLE(Temp);
	m_CreationGameTick = Temp;
};

void CWObject_Emitter::OnDeltaSave(CCFile* _pFile)
{
	//Save duration since last emission (to enable minduration to work properly)
	int32 Temp = m_CreationGameTick;
	_pFile->WriteLE(Temp);
};

void CWObject_Emitter::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Emitter_OnClientRender, MAUTOSTRIP_VOID);
	if(_pObj->m_iModel[0] != 0)
	{
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
		if(pModel)
		{
			fp32 Width = _pObj->m_iAnim1;

			CMat4Dfp32 Mat = _pObj->GetPositionMatrix();
			CVec3Dfp32 &Left = CVec3Dfp32::GetMatrixRow(Mat, 1);
			CVec3Dfp32 Begin = CVec3Dfp32::GetMatrixRow(Mat, 3) - Left * (Width * 0.5f);
			CVec3Dfp32 Add = Left * (Width / fp32(_pObj->m_iAnim0));

			Begin.SetMatrixRow(Mat, 3);

			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
			
			for(int i = 0; i < _pObj->m_iAnim0; i++)
			{
				_pEngine->Render_AddModel(pModel, Mat, AnimState);
				CVec3Dfp32::GetRow(Mat, 3) += Add;
			}
		}
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Emitter, CWObject_Model, 0x0100);




/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
|	CWObject_Emitter_Suspendable
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CWObject_Emitter_Suspendable::OnCreate()
{
	m_SuspensionTime = 0;
	m_ActivationTick = -1;
	CWObject_Emitter::OnCreate();
};


void CWObject_Emitter_Suspendable::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	switch (_KeyHash)
	{
	case MHASH4('SUSP','ENSI','ONTI','ME'): // "SUSPENSIONTIME"
		{
			fp32 time = _pKey->GetThisValuef();
			if (time < 0)
				m_SuspensionTime = -1;
			else
				m_SuspensionTime = (int)(time * m_pWServer->GetGameTicksPerSecond());
			break;
		}
	case MHASH4('STAR','TSUS','PEND','ED'): // "STARTSUSPENDED"
		{
			if (_pKey->GetThisValuei() != 0)
			{
				// Start in suspended mode. Only really useful with suspensiontime -1. 
				// Should really be set in OnFinishedEvalKeys to gte right suspendiontime, but wtf...
				m_ActivationTick = 100;
			}
		}
	default:
		{
			CWObject_Emitter::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}


aint CWObject_Emitter_Suspendable::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_IMPULSE:
		{
			//Suspend on 0, resume on -1, let base class handle otherwise
			if (_Msg.m_Param0 == 0)
			{
				if (m_pWServer)
				{	
					if (m_SuspensionTime == -1)
					{
						// Suspend indefinitely (activation tick will get updated continuosly)
						m_ActivationTick = m_pWServer->GetGameTick() + 100;
					}
					else
					{
						m_ActivationTick = m_pWServer->GetGameTick() + m_SuspensionTime;
					}
				}
				return 1;
			}
			if (_Msg.m_Param0 == -1)
			{
				m_ActivationTick = -1;
				return 1;
			}
			else
			{
				return CWObject_Emitter::OnMessage(_Msg);
			}
		}
	default:
		return CWObject_Emitter::OnMessage(_Msg);
	}
};

int CWObject_Emitter_Suspendable::Emit(fp32 _Velocity, int _iSender)
{
	//Will not emit if suspended or pending minduration
	if (m_pWServer && 
		(m_pWServer->GetGameTick() > m_ActivationTick) &&
		((m_MinDuration == 0) || (m_CreationGameTick > m_MinDuration)))
		return CWObject_Emitter::Emit(_Velocity, _iSender);
	else 
		return 0;
}

void CWObject_Emitter_Suspendable::OnRefresh()
{
	//Should we suspend indefinitely?
	if (m_pWServer && 
		(m_pWServer->GetGameTick() <= m_ActivationTick) &&
		(m_SuspensionTime == -1))
	{
		m_ActivationTick = m_pWServer->GetGameTick() + 100;
	}

	CWObject_Emitter::OnRefresh();
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Emitter_Suspendable, CWObject_Emitter, 0x0100);




/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
|	CWObject_EmitterCoordinator (used for reinforcementcontrol)	
|__________________________________________________________________________________________________
\*************************************************************************************************/

//Perform emission, or fail if no more emissions can be performed right now
bool CWObject_EmitterCoordinator::PerformEmission()
{
	int nUntried = m_lEmitters.Len();
	if (nUntried > 0)
	{
		// Set up the list of unused indices into m_lEmitters
		TList_Linked<int> lUntried;
		for (int i = 0; i < nUntried; i++)
			lUntried.Add(i);

		int iRandom = (int)(Random * nUntried);
		CWObject_Message Emit(OBJMSG_EMITTER_EMIT);
		while (nUntried > 0)
		{
			//Try to emit
			if (m_pWServer->Message_SendToObject(Emit, m_lEmitters[lUntried[iRandom]]) > 0)
			{
				return true;
			}
			else
			{
				//Failed to emit
				lUntried.Del(iRandom);
				nUntried--;
				iRandom = (int)(Random * nUntried);
			}
		}
	}

	//Failed to emit
	return false;
};


void CWObject_EmitterCoordinator::OnCreate()
{
	m_lEmitterNamesFromKey.Clear();
	m_lEmitters.Clear();
	m_EmissionDelay = 0;
	m_lScheduledEmissions.Create(CScheduledEmission());
	m_nPendingEmissions = 0;
	m_bPaused = false;

	CWObject::OnCreate();
};


void CWObject_EmitterCoordinator::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	if (!_pKey)
		return;

	const CStr KeyName = _pKey->GetThisName();

	switch (_KeyHash)
	{
	case MHASH2('DELA','Y'): // "DELAY"
		{
			m_EmissionDelay = (int)(_pKey->GetThisValuef() * m_pWServer->GetGameTicksPerSecond());
			break;
		}
	default:
		{
			if (KeyName.CompareSubStr("EMITTER") == 0)
			{
				uint iSlot = atoi(KeyName.Str() + 7);
				m_lEmitterNamesFromKey.SetMinLen(iSlot + 1);
				m_lEmitterNamesFromKey[iSlot] = _pKey->GetThisValue();
			}
			else
				CWObject::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
};


void CWObject_EmitterCoordinator::OnSpawnWorld()
{
	if (!m_pWServer)
		return;

	//Get object indices of named emitters
	for (int i = 0; i < m_lEmitterNamesFromKey.Len(); i++)
	{
		//Find all objects with the given name
		const int16* piObj;
		int nObj;

		//Set up selection
		TSelection<CSelection::LARGE_BUFFER> Selection;

		//Add objects and shut down selection
		m_pWServer->Selection_AddTarget(Selection, m_lEmitterNamesFromKey[i]);
		nObj = m_pWServer->Selection_Get(Selection, &piObj);

		//Append objects to emitter index list
		m_lEmitters.Insertx(m_lEmitters.Len(), piObj, nObj); 
	}

	//Nuke names
	m_lEmitterNamesFromKey.Destroy();
};


aint CWObject_EmitterCoordinator::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_IMPULSE:
		{
			if (_Msg.m_Param0 == 0)
			{
				//Pause emissions
				m_bPaused = true;
			}
			else if (_Msg.m_Param0 == -1)
			{
				//Resume emissions
				m_bPaused = false;
			}
			else if (m_pWServer && (_Msg.m_Param0 > 0))
			{
				if (m_EmissionDelay > 0)
				{
					//Schedule new emission(s)
					m_lScheduledEmissions.Add(CScheduledEmission(m_pWServer->GetGameTick() + m_EmissionDelay, _Msg.m_Param0));
				}
				else
				{
					m_nPendingEmissions += _Msg.m_Param0;
				}
			};
			return 1;
		}

	case OBJMSG_EMITTERCOOORDINATOR_RESET:
		{
			//Clear all pending/scheduled emissions. Note that there might still be recently 
			//emitted agents that will cause new emissions when they die, so use with care.
            m_lScheduledEmissions.Clear();
			m_nPendingEmissions = 0;
		}
		return 1;

	case OBJMSG_EMITTERCOOORDINATOR_DELAY:
		{
			CStr SDelay((char *)_Msg.m_pData);
			m_EmissionDelay = (int)(SDelay.Val_fp64() * m_pWServer->GetGameTicksPerSecond());
		}
		return 1;

	default:
		return CWObject::OnMessage(_Msg);
	}
};


//Make scheduled emissions that are due pending emissions and perform as many 
//pending emissions as possible
void CWObject_EmitterCoordinator::OnRefresh()
{
	if (!m_pWServer)
		return;

	//Handle scheduled emissions
	int Tick = m_pWServer->GetGameTick();
	for (int i = 0; i < m_lScheduledEmissions.Length(); i++)
	{
		if (m_lScheduledEmissions.IsValid(i) &&
			(Tick > m_lScheduledEmissions.Get(i).m_Tick))
		{
			m_nPendingEmissions += m_lScheduledEmissions.Get(i).m_nEmissions;
			m_lScheduledEmissions.Remove(i);
		}
	}
	
	//Perform as many pending emissions as possible unless paused
	if (!m_bPaused)
	{
		while ((m_nPendingEmissions > 0) && PerformEmission())
		{
			m_nPendingEmissions--;
		}
	}

	CWObject::OnRefresh();
};


void CWObject_EmitterCoordinator::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	int8 NumScheduledEmissions = 0;
	_pFile->ReadLE(NumScheduledEmissions);

	int32 Tick;
	int8 nEmissions;
	for (int i = 0; i < NumScheduledEmissions; i++)
	{
		_pFile->ReadLE(Tick);
		_pFile->ReadLE(nEmissions);
		m_lScheduledEmissions.Add(CScheduledEmission(Tick, nEmissions));
	}

 	_pFile->ReadLE(m_nPendingEmissions);
	int8 Temp;
	_pFile->ReadLE(Temp);
	m_bPaused = (Temp != 0);
};

void CWObject_EmitterCoordinator::OnDeltaSave(CCFile* _pFile)
{
	int i;

	//Count number of scheduled emissions (GetNum should return the same, but this feels safer)
	int8 nScheduledEmissions = 0;
	for (i = 0; i < m_lScheduledEmissions.Length(); i++)
	{
		if (m_lScheduledEmissions.IsValid(i))
		{
			nScheduledEmissions++;
		}
	}
	_pFile->WriteLE(nScheduledEmissions);

	//Only save valid slots; the structure and order of the list is unimportant
	for (i = 0; i < m_lScheduledEmissions.Length(); i++)
	{
		if (m_lScheduledEmissions.IsValid(i))
		{
			_pFile->WriteLE(m_lScheduledEmissions.Get(i).m_Tick);
			_pFile->WriteLE(m_lScheduledEmissions.Get(i).m_nEmissions);
		}
	}
	_pFile->WriteLE(m_nPendingEmissions);
	_pFile->WriteLE((int8)m_bPaused);
};


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_EmitterCoordinator, CWObject, 0x0100);
