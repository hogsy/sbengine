/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character class for actual player characters.

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_CharPlayer implementation

	Comments:

	History:		
		050308:		Created file
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_CharPlayer.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_CharPlayer, CWObject_Character, 0x0100);


const CWO_CharPlayer_ClientData& CWObject_CharPlayer::GetClientData(const CWObject_CoreData* _pObj)
{
	M_ASSERT(_pObj, "[CWO_Character_ClientData] Bad this-pointer!");
	const CWO_CharPlayer_ClientData* pCD = safe_cast<const CWO_CharPlayer_ClientData>((const CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	M_ASSERT(pCD, "[CWObject_CharPlayer] No clientdata?!");
	return *pCD;
};

CWO_CharPlayer_ClientData& CWObject_CharPlayer::GetClientData(CWObject_CoreData* _pObj)
{
	M_ASSERT(_pObj, "[CWO_Character_ClientData] Bad this-pointer!");
	CWO_CharPlayer_ClientData* pCD = safe_cast<CWO_CharPlayer_ClientData>((CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	M_ASSERT(pCD, "[CWObject_CharPlayer] No clientdata?!");
	return *pCD;
};


void CWObject_CharPlayer::OnInitInstance(const aint* _pParam, int _nParam)
{
	MAUTOSTRIP(CWObject_CharPlayer_OnInitInstance, MAUTOSTRIP_VOID);
	CWO_CharPlayer_ClientData& CD = GetClientData();

	if (_pParam && _nParam > 0)
		CD.m_iPlayer = (int16) _pParam[0];

	m_Player.m_bSpecialClass = false;
	if(_pParam && _nParam > 1)
	{
		if(_pParam[1] & INFOPLAYERSTART_FLAGS_STARTCROUCHED)
			m_InitialPhysMode = PLAYER_PHYS_CROUCH;
		if(_pParam[1] & INFOPLAYERSTART_FLAGS_NIGHTVISION)
			CD.m_bNightvisionEnabled = true;
		if(_pParam[1] & INFOPLAYERSTART_FLAGS_OGR_NOCLIP)
			m_InitialPhysMode = PLAYER_PHYS_NOCLIP;
		if(_pParam[1] & INFOPLAYERSTART_FLAGS_OGR_SPECIALCLASS)
			m_Player.m_bSpecialClass = true;
	}
}


void CWObject_CharPlayer::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA];
	if (!TDynamicCast<CWO_CharPlayer_ClientData>(pData))
	{
		CWO_CharPlayer_ClientData* pCD = MNew(CWO_CharPlayer_ClientData);
		if (!pCD)
			Error_static("CWObject_CharPlayer", "Could not allocate client data!");

		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = pCD;
		pCD->Clear(_pObj, _pWPhysState);
	}

	if (!InitClientObjects(_pObj, _pWPhysState))
		Error_static("CWObject_CharPlayer", "InitClientObjects failed");
}


void CWObject_CharPlayer::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_CharPlayer::OnClientRefresh);
	parent::OnClientRefresh(_pObj, _pWClient);
	GetClientData(_pObj).OnRefresh();
}


void CWObject_CharPlayer::OnCreate()
{
	parent::OnCreate();
}


void CWObject_CharPlayer::OnRefresh()
{
	parent::OnRefresh();
	CWO_CharPlayer_ClientData& CD = GetClientData();

	// Update darkness visibility
	fp32 NewVal = Clamp01(m_spAI->GetLightIntensity(0));
	m_AverageLightIntensity = LERP(NewVal, m_AverageLightIntensity, 0.75f);	// keep 75% of old value
	if (CD.m_DarknessVisibility && m_AverageLightIntensity > 0.25f)
	{
		CD.m_DarknessVisibility = 0;
	}
	else if (!CD.m_DarknessVisibility && m_AverageLightIntensity < 0.15f)
	{
		CD.m_DarknessVisibility = 255;
	}

	// Run clientdata's OnRefresh
	CD.OnRefresh();

}
