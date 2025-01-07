#include "PCH.h"
#include "WObj_Spawner_P6.h"
#include "../WObj_Char/WObj_CharNPC_P6.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Spawner_P6, CWObject, 0x0100);

CWObject_Spawner_P6::CWObject_Spawner_P6()
{
	m_MinSpawnTime = 10;
	m_RandSpawnTime = 10;
	m_LastCheck = 0;
	m_TimeLeft = 0;
}

CWObject_Spawner_P6::~CWObject_Spawner_P6()
{
}

void CWObject_Spawner_P6::OnSpawnWorld2()
{
	CWObject::OnSpawnWorld2();
	m_LastCheck = m_pWServer->GetGameTick();
	TSelection<CSelection::SMALL_BUFFER> Selection;
	m_pWServer->Selection_AddTarget(Selection, m_SpawnObjectName);
	const int16 *pSel;
	int nSel = m_pWServer->Selection_Get(Selection, &pSel);
	if(nSel)
	{
		CWObject *pObj = m_pWServer->Object_Get(pSel[0]);
		m_SpawnObjectTemplate = pObj->GetTemplateName();
	}
}

void CWObject_Spawner_P6::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();

	switch(_KeyHash) 
	{
	case MHASH2('ENEM','Y'):
	    {
			m_SpawnObjectName = _pKey->GetThisValue();
	    }
		break;
	case MHASH4('SPAW','N_MI','N_TI','ME'):
	    {
			m_MinSpawnTime = _pKey->GetThisValue().Val_int();
	    }
		break;
	case MHASH4('SPAW','N_RA','ND_T','IME'):
	    {
			m_RandSpawnTime = _pKey->GetThisValue().Val_int();
	    }
		break;
	default:
		CWObject::OnEvalKey(_KeyHash, _pKey);
		break;
	}
}

void CWObject_Spawner_P6::OnRefresh()
{
	if(!m_SpawnObjectName.Len())
		return;

	CWObject::OnRefresh();
	if(m_TimeLeft)
	{
		m_TimeLeft--;
		if(m_TimeLeft == 0)
		{
			if(m_pWServer->Object_Create(m_SpawnObjectTemplate.Str(), GetPositionMatrix()) == -1)
			{
				m_TimeLeft += 10;
			}
		}
	}
	else if(m_LastCheck + 200 < m_pWServer->GetGameTick())
	{
		m_LastCheck = m_pWServer->GetGameTick();
		TSelection<CSelection::SMALL_BUFFER> Selection;
		m_pWServer->Selection_AddTarget(Selection, m_SpawnObjectName.Str());
		const int16 *pSel;
		int nSel = m_pWServer->Selection_Get(Selection, &pSel);

		bool bAllDead = true;
		for(int i = 0; i < nSel; i++)
		{
			CWObject *pObj = m_pWServer->Object_Get(pSel[i]);
			CWObject_CharNPC_P6 *pChar = TDynamicCast<CWObject_CharNPC_P6>(pObj);
			if(pChar && CWObject_CharNPC_P6::Char_GetPhysType(pChar) != PLAYER_PHYS_DEAD)
			{
				CVec3Dfp32 vec = GetPosition() - pChar->GetPosition();
				fp32 dist = vec.Length();
				if(dist < 750.0f)
				{
					bAllDead = false;
					break;
				}
			}
		}

		if(bAllDead)
		{
			m_TimeLeft = TruncToInt((m_MinSpawnTime + MRTC_RAND() % m_RandSpawnTime) * m_pWServer->GetGameTicksPerSecond());
		}
	}
}






