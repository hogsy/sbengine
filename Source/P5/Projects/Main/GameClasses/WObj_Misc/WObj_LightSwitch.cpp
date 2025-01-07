#include "PCH.h"
#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_System.h"
#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Lights.h"
#include "../WNameHash.h"


class CWObject_LampSwitch : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

private:
	enum { MaxLamps = 4 };
	CNameHash m_lLamps[MaxLamps];
	uint m_nLamps;

	bool  m_bState;
	int16 m_iModel_On;
	int16 m_iModel_Off;

public:
	CWObject_LampSwitch()
		: m_nLamps(0)
		, m_bState(false) // Default: Off
		, m_iModel_On(0)
		, m_iModel_Off(0)
	{ }

	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server*)
	{
		IncludeModelFromKey("MODEL_OFF", _pReg, _pMapData);
	}

	virtual void OnCreate()
	{
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;

		CWO_PhysicsState Phys = GetPhysState();
		Phys.m_ObjectFlags |= OBJECT_FLAGS_PICKUP;
		m_pWServer->Object_SetPhysics(m_iObject, Phys);
	}

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		CStr KeyName = _pKey->GetThisName();
		CStr KeyValue = _pKey->GetThisValue();

		switch (_KeyHash)
		{
		case MHASH3('INIT','IALS','TATE'): // "INITIALSTATE"
			{
				m_bState = (_pKey->GetThisValuei() != 0);
				break;
			}
		case MHASH3('MODE','L_OF','F'): // "MODEL_OFF"
			{
				m_iModel_Off = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
				break;
			}
		default:
			{
				if (KeyName.Find("TARGET_LAMP") != -1)
				{
					if (m_nLamps < MaxLamps)
						m_lLamps[m_nLamps++] = KeyValue;
				}
				else
					CWObject_Model::OnEvalKey(_KeyHash, _pKey);
				break;
			}
		}
	}

	virtual void OnSpawnWorld2()
	{
		m_iModel_On = iModel(0);
		SetState(m_bState);
	}

	void SetState(bool _bState)
	{
		int16 Model = _bState ? m_iModel_On : m_iModel_Off;
		if (Model && Model != iModel(0))
		{
			iModel(0) = Model;
			m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_USER);
		}

		uint MsgParam = _bState ? CWObject_Light::MSG_ON : CWObject_Light::MSG_OFF;
		CWObject_Message Msg(OBJMSG_LIGHT_IMPULSE, MsgParam);
		for (uint i = 0; i < m_nLamps; i++)
			m_pWServer->Message_SendToTarget(Msg, m_lLamps[i]);

		m_bState = _bState;
	}

	virtual aint OnMessage(const CWObject_Message& _Msg)
	{
		switch (_Msg.m_Msg)
		{
		case OBJMSG_USE: 
			SetState(!m_bState); // toggle state
			return 1;
		}
		return CWObject_Model::OnMessage(_Msg);
	}

	void OnDeltaSave(CCFile* _pFile)
	{
		CWObject_Model::OnDeltaSave(_pFile);
		_pFile->WriteLE((uint8&)m_bState);
	}

	void OnDeltaLoad(CCFile* _pFile, int _Flags)
	{
		CWObject_Model::OnDeltaLoad(_pFile, _Flags);
		_pFile->ReadLE((uint8&)m_bState);

		SetState(m_bState);
	}
};
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_LampSwitch, CWObject_Model, 0x0100);

