#include "pch.h"
#include "../../GameWorld/WServerMod.h"

#if !defined(M_DISABLE_TODELETE) || 1
#include "WObj_TranquillizerDart.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_TranquillizerDart, CWObject_ProjectileCluster, 0x0100);

void CWObject_TranquillizerDart::OnCreate()
{
	CWObject_ProjectileCluster::OnCreate();
	m_TranquillizerEffect = 5;
};

void CWObject_TranquillizerDart::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	switch (_KeyHash)
	{
	case MHASH5('TRAN','QUIL','LIZE','R_EF','FECT'): // "TRANQUILLIZER_EFFECT"
		{
			m_TranquillizerEffect = _pKey->GetThisValuei();
			break;
		}
	default:
		{
			CWObject_ProjectileCluster::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
};

/*void CWObject_TranquillizerDart::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	CWObject_ProjectileCluster::OnIncludeTemplate(_pReg,_pMapData,_pWServer);

	if (_pReg)
	{
		CRegistry* pAnimReg = _pReg->Find("ANIMSETS_NEEDED");
		if (pAnimReg)
		{
			CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(_pWServer);
			if (pServerMod)
			{
				pServerMod->AddExtraAnimSet(pAnimReg->GetThisValue(), CWServer_Mod::SERVERMOD_EXTRANIMSET_TYPE_TRANQ);
			}
		}
	}
	
}*/

void CWObject_TranquillizerDart::OnImpact(int _iProjectile, const CVec3Dfp32& _Pos, uint16 _iObject, CCollisionInfo* _pCInfo)
{
	if (_iObject != 0)
	{
		CWObject_Message Msg(OBJMSG_CHAR_STUN, 0, m_TranquillizerEffect);
		m_pWServer->Message_SendToObject(Msg, _iObject);
	}

	CWObject_ProjectileCluster::OnImpact(_iProjectile, _Pos, _iObject, _pCInfo);
};
#endif
