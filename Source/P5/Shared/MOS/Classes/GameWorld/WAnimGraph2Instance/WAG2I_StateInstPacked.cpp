#include "PCH.h"

//--------------------------------------------------------------------------------

#include "WAG2I_StateInstPacked.h"
#include "WAG2I.h"

//--------------------------------------------------------------------------------

CWAG2I_SIP::CWAG2I_SIP()
{
	Clear();
}

//--------------------------------------------------------------------------------

void CWAG2I_SIP::Clear()
{
//	SetTokenID(AG2_TOKENID_NULL);
	//SetEnterActionIndex(AG2_ACTIONINDEX_NULL);
	SetEnterMoveTokenIndex(AG2_MOVETOKENINDEX_NULL);
	//SetLeaveActionIndex(AG2_ACTIONINDEX_NULL);
	SetLeaveMoveTokenIndex(AG2_MOVETOKENINDEX_NULL);
	m_EnqueueTime = AG2I_UNDEFINEDTIME;
	m_EnterTime = AG2I_UNDEFINEDTIME;
	m_LeaveTime = AG2I_UNDEFINEDTIME;
	m_BlendOutEndTime = AG2I_UNDEFINEDTIME;
	m_Enter_AnimTimeOffset = AG2I_UNDEFINEDTIMEFP32;
}

//--------------------------------------------------------------------------------

bool CWAG2I_SIP::SameIdentity(const CWAG2I_SIP* _pSIPA, const CWAG2I_SIP* _pSIPB)
{
	return ((_pSIPA->GetEnqueueTime().Compare(_pSIPB->GetEnqueueTime()) == 0) && 
			(_pSIPA->GetEnterMoveTokenIndex() == _pSIPB->GetEnterMoveTokenIndex()));
}

//--------------------------------------------------------------------------------

bool CWAG2I_SIP::Equal(const CWAG2I_SIP* _pSIPA, const CWAG2I_SIP* _pSIPB)
{
	return ((_pSIPA->GetEnqueueTime().Compare(_pSIPB->GetEnqueueTime()) == 0) && 
			(_pSIPA->GetEnterMoveTokenIndex() == _pSIPB->GetEnterMoveTokenIndex()) &&
			(_pSIPA->GetEnterTime().Compare(_pSIPB->GetEnterTime()) == 0) &&
			(_pSIPA->GetLeaveMoveTokenIndex() == _pSIPB->GetLeaveMoveTokenIndex()) &&
			(_pSIPA->GetLeaveTime().Compare(_pSIPB->GetLeaveTime()) == 0) && 
			(_pSIPA->GetAnimTimeOffset() == _pSIPB->GetAnimTimeOffset()));
}

//--------------------------------------------------------------------------------

bool CWAG2I_SIP::IsDepricated(CMTime _GameTime) const
{
	return ((GetBlendOutEndTime().Compare(AG2I_UNDEFINEDTIME) != 0) && 
			(GetBlendOutEndTime().Compare(_GameTime) < 0));
}

//--------------------------------------------------------------------------------
