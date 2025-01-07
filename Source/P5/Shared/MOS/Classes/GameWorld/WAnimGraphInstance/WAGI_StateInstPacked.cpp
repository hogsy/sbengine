#include "PCH.h"

//--------------------------------------------------------------------------------

#include "WAGI_StateInstPacked.h"
#include "WAGI.h"

//--------------------------------------------------------------------------------

/*
	OnCreateClientUpdate()
	{
		Get ClientMirror AGI Packed
		Create Server AGI Packed
		Create Packed Diff
		Send Diff
	}
	
	OnClientUpdate()
	{
		Receive Diff
		Unpack and add Diff to AGI
		Create Client AGI Packed
	}

	Server/Client Diff Actions
		Add Token
			Add Token and initial StateInstance

		Move Token
			Update StateInstance (prev, setup blendout)
			Add StateInstance (new, setup blendin)

		Terminate Token
			Update StateInstance (prev, setup blendout)

		Proceed Token StateInstanceQueue
			Update StateInstance (prev, initiate blendout)
			Update StateInstance (next, initiate blendin)

*/

//--------------------------------------------------------------------------------

CWAGI_SIP::CWAGI_SIP()
{
	Clear();
}

//--------------------------------------------------------------------------------

void CWAGI_SIP::Clear()
{
//	SetTokenID(AG_TOKENID_NULL);
	SetEnterActionIndex(AG_ACTIONINDEX_NULL);
	SetLeaveActionIndex(AG_ACTIONINDEX_NULL);
	m_EnqueueTime = AGI_UNDEFINEDTIME;
	m_EnterTime = AGI_UNDEFINEDTIME;
	m_LeaveTime = AGI_UNDEFINEDTIME;
	m_BlendOutEndTime = AGI_UNDEFINEDTIME;
	m_Enter_AnimTimeOffset = AGI_UNDEFINEDTIMEFP32;
}

//--------------------------------------------------------------------------------

bool CWAGI_SIP::SameIdentity(const CWAGI_SIP* _pSIPA, const CWAGI_SIP* _pSIPB)
{
	return ( _pSIPA->GetEnqueueTime().AlmostEqual(_pSIPB->GetEnqueueTime()) && 
			(_pSIPA->GetEnterActionIndex() == _pSIPB->GetEnterActionIndex()));
}

//--------------------------------------------------------------------------------

bool CWAGI_SIP::Equal(const CWAGI_SIP* _pSIPA, const CWAGI_SIP* _pSIPB)
{
	return ( _pSIPA->GetEnqueueTime().AlmostEqual(_pSIPB->GetEnqueueTime()) && 
			(_pSIPA->GetEnterActionIndex() == _pSIPB->GetEnterActionIndex()) &&
			 _pSIPA->GetEnterTime().AlmostEqual(_pSIPB->GetEnterTime()) &&
			(_pSIPA->GetLeaveActionIndex() == _pSIPB->GetLeaveActionIndex()) &&
			 _pSIPA->GetLeaveTime().AlmostEqual(_pSIPB->GetLeaveTime()) && 
			(_pSIPA->GetAnimTimeOffset() == _pSIPB->GetAnimTimeOffset()));
}

//--------------------------------------------------------------------------------

bool CWAGI_SIP::IsDepricated(CMTime _GameTime) const
{
	return ((GetBlendOutEndTime().AlmostEqual(AGI_UNDEFINEDTIME) == false) && 
			(GetBlendOutEndTime().Compare(_GameTime) < 0));
}

//--------------------------------------------------------------------------------
