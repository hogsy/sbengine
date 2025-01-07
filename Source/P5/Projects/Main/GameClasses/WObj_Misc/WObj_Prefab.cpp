#include "PCH.h"

#ifndef M_DISABLE_CURRENTPROJECT

#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Hook.h"
#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Game.h"

class CWObject_Prefab : public CWObject_Engine_Path
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	CStr m_Prefab;
	CStr m_MainEntity;
	TArray<spCRegistry> m_lspKeys;

	enum
	{
		FLAGS_RELAYMESSAGES = 1,
	};

public:
	virtual void OnCreate()
	{
		MAUTOSTRIP(CWObject_TeleportController_OnCreate, MAUTOSTRIP_VOID);
		CWObject_Engine_Path::OnCreate();
	}

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
	{
		MAUTOSTRIP(CWObject_TeleportController_OnEvalKey, MAUTOSTRIP_VOID);

		CStr KeyName = _pKey->GetThisName();
		CStr KeyValue = _pKey->GetThisValue();

		switch (_KeyHash)
		{
		case MHASH2('PREF','AB'): // "PREFAB"
			{
				m_Prefab = KeyValue;
				break;
			}

		case MHASH3('MAIN','ENTI','TY'): // "MAINENTITY"
			{
				m_MainEntity = KeyValue;
				break;
			}


/*		
		case MHASH2('FLAG','S'): // "FLAGS"
			{
				static const char *FlagsTranslate[] =
				{
					"relaymessages", NULL
				};

				m_Flags = _pKey->GetThisValue().TranslateFlags(FlagsTranslate);
				break;
			}
*/

		default:
			{
				if(KeyName.CompareSubStr("MAINENTITY_") == 0)
				{
					// Store keys, and pipe them to main prefab entity
					spCRegistry spReg = _pKey->Duplicate();
					spReg->RenameThisKey(spReg->GetThisName().Copy(11, 1024));
					m_lspKeys.Add(spReg);
				}
				default:
					{
						CWObject_Engine_Path::OnEvalKey(_pKey);
						break;
					}
				}
				break;
			}
		}
	}

	virtual void OnDestroy()
	{
		MAUTOSTRIP(CWObject_TeleportController_OnDestroy, MAUTOSTRIP_VOID);
/*		// This code should be removed and Object_AddChild should be run in OnFinishEval keys instead
		// Doesn't seem to work with the cannon (engine/hook krick I guess)
		for(int i = 0; i < m_NumObj; i++)
			m_pWServer->Object_Destroy(m_iObject + i + 1);*/
	}
	
	virtual aint OnMessage(const CWObject_Message& _Msg)
	{
		MAUTOSTRIP(CWObject_TeleportController_OnMessage, 0);

		switch(_Msg.m_Msg)
		{
		case OBJSYSMSG_DESTROY:
		case OBJSYSMSG_GETDEBUGSTRING:
			return CWObject_Engine_Path::OnMessage(_Msg);

/*		case OBJMSG_IMPULSE:
		case OBJMSG_GAME_SPAWN:
			{
				// Fix this too
				if(m_Flags & FLAGS_RELAYMESSAGES)
				{
					for(int i = 0; i < m_NumObj; i++)
						m_pWServer->Message_SendToObject(_Msg, m_iObject + i + 1);
				}
			}*/

		case OBJMSG_IMPULSE:
		case OBJMSG_GAME_SPAWN:
			if(m_MainEntity != "")
			{
				int iObj = m_pWServer->Selection_GetSingleTarget(Selection);
				CWObject *pObj = m_pWServer->Object_Get(iObj);
				if(pObj)
					return pObj->OnMessage(_Msg);
			}
		}
		return CWObject_Engine_Path::OnMessage(_Msg);
	}

	virtual void OnFinishEvalKeys()
	{
		MAUTOSTRIP(CWObject_TeleportController_OnFinishEvalKeys, MAUTOSTRIP_VOID);
		MSCOPE(CWObject_Prefab::OnFinishEvalKeys, PREFAB);

		CWObject_Engine_Path::OnFinishEvalKeys();
		
		if(m_Prefab != "")
		{
			CMat4Dfp32 Mat = GetPositionMatrix();
			m_pWServer->World_PushSpawnTargetNamePrefix(CStrF("%i_", m_iObject));
			
			m_pWServer->World_SpawnObjectsFromWorld(m_Prefab, &Mat, true);
			if(m_MainEntity != "")
			{
				m_MainEntity = m_pWServer->World_MangleTargetName(m_MainEntity);
				int iObj = m_pWServer->Selection_GetSingleTarget(Selection);
				CWObject *pObj = m_pWServer->Object_Get(iObj);
				if(pObj)
				{
					for(int i = 0; i < m_lspKeys.Len(); i++)
					{
						const CRegistry* pReg = m_lspKeys[i];
						pObj->OnEvalKey(pReg->GetThisNameHash(), pReg);
					}
					
					m_pWServer->Object_AddChild(m_iObject, iObj);
				}
				m_lspKeys.Clear();
			}
			int NumObj = 0;
			while(true)
			{
				int iObj = NumObj + m_iObject + 1;
				CWObject *pObj = m_pWServer->Object_Get(iObj);
				if(!pObj)
					break;
				if(pObj->GetParent() == 0)
					m_pWServer->Object_AddChild(m_iObject, iObj);
				NumObj++;
			}

			m_pWServer->World_PopSpawnTargetNamePrefix();
		}
	}

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine)
	{
		MAUTOSTRIP(CWObject_TeleportController_OnClientRender, MAUTOSTRIP_VOID);
	}	
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Prefab, CWObject_Engine_Path, 0x0100);

#endif
