#ifndef WAGI_StateInstPacked_h
#define WAGI_StateInstPacked_h

//--------------------------------------------------------------------------------

//#include "WAGI.h"
#include "../../../XR/XRAnimGraph/AnimGraphDefs.h"

//--------------------------------------------------------------------------------
/*
#define BITS2MASK(Bits)				((1 << (Bits)) - 1)
#define SHARED32_BITS_TOKENID		6
#define SHARED32_BITS_ENTERACTION	((32 - SHARED32_BITS_TOKENID) >> 1) // 13
#define SHARED32_BITS_LEAVEACTION	((32 - SHARED32_BITS_TOKENID) >> 1) // 13
#define SHARED32_SHIFT_TOKENID		0
#define SHARED32_SHIFT_ENTERACTION	(SHARED32_SHIFT_TOKENID + SHARED32_BITS_TOKENID)
#define SHARED32_SHIFT_LEAVEACTION	(SHARED32_SHIFT_ENTERACTION + SHARED32_BITS_ENTERACTION)
#define SHARED32_MASK_TOKENID		(BITS2MASK(SHARED32_BITS_TOKENID) << SHARED32_SHIFT_TOKENID)
#define SHARED32_MASK_ENTERACTION	(BITS2MASK(SHARED32_BITS_ENTERACTION) << SHARED32_SHIFT_ENTERACTION)
#define SHARED32_MASK_LEAVEACTION	(BITS2MASK(SHARED32_BITS_LEAVEACTION) << SHARED32_SHIFT_LEAVEACTION)
*/
//--------------------------------------------------------------------------------

//CWAGI_StateInstance_Packed
class CWAGI_SIP
{
	private:

//		int8	m_TokenID;
		CMTime	m_EnqueueTime;
		CMTime	m_EnterTime;
		CMTime	m_LeaveTime;
		CMTime	m_BlendOutEndTime;
		fp32		m_Enter_AnimTimeOffset;
		int16	m_iEnterAction;
		int16	m_iLeaveAction;

/*
		uint32	m_Shared32; // m_TokenID, m_iEnterAction, m_iLeaveAction
		fp32		m_EnqueueTime;
		fp32		m_EnterTime;
		fp32		m_LeaveTime;
		fp32		m_BlendOutEndTime;
*/

	public:

		CWAGI_SIP();
		void Clear();

		int16 GetEnterActionIndex() const { return m_iEnterAction; }
		int16 GetLeaveActionIndex() const { return m_iLeaveAction; }
		void SetEnterActionIndex(int16 _iEnterAction) { m_iEnterAction = _iEnterAction; }
		void SetLeaveActionIndex(int16 _iLeaveAction) { m_iLeaveAction = _iLeaveAction; }

/*
		int8 GetTokenID() { return ((m_Shared32 & SHARED32_MASK_TOKENID) >> SHARED32_SHIFT_TOKENID); }
		int16 GetEnterActionIndex() { return ((m_Shared32 & SHARED32_MASK_ENTERACTION) >> SHARED32_SHIFT_ENTERACTION); }
		int16 GetLeaveActionIndex() { return ((m_Shared32 & SHARED32_MASK_LEAVEACTION) >> SHARED32_SHIFT_LEAVEACTION); }
		void SetTokenID(int8 _TokenID) { m_Shared32 = ((m_Shared32 & ~SHARED32_MASK_TOKENID) | ((_TokenID << SHARED32_SHIFT_TOKENID) & SHARED32_MASK_TOKENID)); }
		void SetEnterActionIndex(int16 _EnterActionIndex) { m_Shared32 = ((m_Shared32 & ~SHARED32_MASK_ENTERACTION) | ((_EnterActionIndex << SHARED32_SHIFT_ENTERACTION) & SHARED32_MASK_ENTERACTION)); }
		void SetLeaveActionIndex(int16 _LeaveActionIndex) { m_Shared32 = ((m_Shared32 & ~SHARED32_MASK_LEAVEACTION) | ((_LeaveActionIndex << SHARED32_SHIFT_LEAVEACTION) & SHARED32_MASK_LEAVEACTION)); }
*/

		CMTime GetEnqueueTime() const { return m_EnqueueTime; }
		CMTime GetEnterTime() const { return m_EnterTime; }
		CMTime GetLeaveTime() const { return m_LeaveTime; }
		CMTime GetBlendOutEndTime() const { return m_BlendOutEndTime; }
		fp32 GetAnimTimeOffset() const { return m_Enter_AnimTimeOffset; }
		void SetEnqueueTime(CMTime _EnqueueTime) { m_EnqueueTime = _EnqueueTime; }
		void SetEnterTime(CMTime _EnterTime) { m_EnterTime = _EnterTime; }
		void SetLeaveTime(CMTime _LeaveTime) { m_LeaveTime = _LeaveTime; }
		void SetBlendOutEndTime(CMTime _BlendOutEndTime) { m_BlendOutEndTime = _BlendOutEndTime; }
		void SetAnimTimeOffset(fp32 _Offset) { m_Enter_AnimTimeOffset = _Offset; }

		static bool SameIdentity(const CWAGI_SIP* _pSIPA, const CWAGI_SIP* _pSIPB);
		static bool Equal(const CWAGI_SIP* _pSIPA, const CWAGI_SIP* _pSIPB);
		bool IsDepricated(CMTime _GameTime) const;

};

//--------------------------------------------------------------------------------

#endif /* WAGI_StateInstPacked_h */