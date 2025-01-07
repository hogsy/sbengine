/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character class for Non-Player Characters.

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_CharNPC implementation

	Comments:

	History:		
		050308:		Created file
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_CharNPC.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_CharNPC, CWObject_Character, 0x0100);


const CWO_CharNPC_ClientData& CWObject_CharNPC::GetClientData(const CWObject_CoreData* _pObj)
{
	M_ASSERT(_pObj, "[CWObject_CharNPC] Bad this-pointer!");
	const CWO_CharNPC_ClientData* pCD = safe_cast<const CWO_CharNPC_ClientData>((const CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	M_ASSERT(pCD, "[CWObject_CharNPC] No clientdata?!");
	return *pCD;
};


CWO_CharNPC_ClientData& CWObject_CharNPC::GetClientData(CWObject_CoreData* _pObj)
{
	M_ASSERT(_pObj, "[CWObject_CharNPC] Bad this-pointer!");
	CWO_CharNPC_ClientData* pCD = safe_cast<CWO_CharNPC_ClientData>((CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	M_ASSERT(pCD, "[CWObject_CharNPC] No clientdata?!");
	return *pCD;
};


void CWObject_CharNPC::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA];
	if (!TDynamicCast<CWO_CharNPC_ClientData>(pData))
	{
		CWO_CharNPC_ClientData* pCD = MNew(CWO_CharNPC_ClientData);
		if (!pCD)
			Error_static("CWObject_CharNPC", "Could not allocate client data!");

		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = pCD;
		pCD->Clear(_pObj, _pWPhysState);
	}

	if (!InitClientObjects(_pObj, _pWPhysState))
		Error_static("CWObject_CharNPC", "InitClientObjects failed");
}


void CWObject_CharNPC::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	CWObject_Character::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

	IncludeClassFromKey("GHOST_GLOW", _pReg, _pMapData);
}


void CWObject_CharNPC::AI_SetEyeLookDir(const CVec3Dfp32& _EyeLook, const int16 _iObj)
{
	CWO_CharNPC_ClientData& CD = GetClientData();
	CD.SetEyeLook(_EyeLook,_iObj);
};


void CWObject_CharNPC::OnDestroy()
{
	CWObject_Character::OnDestroy();
	CWO_CharNPC_ClientData& CD = GetClientData();

	if(CD.m_iGhostGlow)
		m_pWServer->Object_Destroy(CD.m_iGhostGlow);
}


void CWObject_CharNPC::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	switch(_KeyHash)
	{
	case MHASH3('GHOS','T_GL','OW'):
		{
			CWO_CharNPC_ClientData& CD = GetClientData();
			CD.m_iGhostGlow = m_pWServer->Object_Create(_pKey->GetThisValue(), GetPositionMatrix(), m_iObject);
		}
		return;

	default:
		CWObject_Character::OnEvalKey(_KeyHash, _pKey);
		return;
	}
}

