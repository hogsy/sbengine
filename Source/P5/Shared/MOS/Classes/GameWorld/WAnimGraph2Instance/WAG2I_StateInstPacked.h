#ifndef WAG2I_StateInstPacked_h
#define WAG2I_StateInstPacked_h

//--------------------------------------------------------------------------------

//#include "WAG2I.h"
#include "../../../XR/XRAnimGraph2/AnimGraph2Defs.h"

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

//CWAG2I_StateInstance_Packed
class CWAG2I_SIP
{
	private:

//		int8	m_TokenID;
		CMTime	m_EnqueueTime;
		CMTime	m_EnterTime;
		CMTime	m_LeaveTime;
		CMTime	m_BlendOutEndTime;
		fp32		m_Enter_AnimTimeOffset;
		//int16	m_iEnterAction;
		int16	m_iEnterMoveToken;
		//int16	m_iLeaveAction;
		int16	m_iLeaveMoveToken;
		CAG2AnimGraphID m_iAnimGraph;

/*
		uint32	m_Shared32; // m_TokenID, m_iEnterAction, m_iLeaveAction
		fp32		m_EnqueueTime;
		fp32		m_EnterTime;
		fp32		m_LeaveTime;
		fp32		m_BlendOutEndTime;
*/

	public:

		CWAG2I_SIP();
		void Clear();

		//int16 GetEnterActionIndex() const { return m_iEnterAction; }
		M_AGINLINE int16 GetEnterMoveTokenIndex() const { return m_iEnterMoveToken; }
		//int16 GetLeaveActionIndex() const { return m_iLeaveAction; }
		M_AGINLINE int16 GetLeaveMoveTokenIndex() const { return m_iLeaveMoveToken; }
		//void SetEnterActionIndex(int16 _iEnterAction) { m_iEnterAction = _iEnterAction; }
		M_AGINLINE void SetEnterMoveTokenIndex(int16 _iEnterMoveToken) { m_iEnterMoveToken = _iEnterMoveToken; }
		//void SetLeaveActionIndex(int16 _iLeaveAction) { m_iLeaveAction = _iLeaveAction; }
		M_AGINLINE void SetLeaveMoveTokenIndex(int16 _iLeaveMoveToken) { m_iLeaveMoveToken = _iLeaveMoveToken; }

		M_AGINLINE CAG2AnimGraphID GetAnimGraphIndex() const { return m_iAnimGraph;	}
		M_AGINLINE void SetAnimGraphIndex(CAG2AnimGraphID _iAnimGraph) { m_iAnimGraph = _iAnimGraph; }

/*
		int8 GetTokenID() { return ((m_Shared32 & SHARED32_MASK_TOKENID) >> SHARED32_SHIFT_TOKENID); }
		int16 GetEnterActionIndex() { return ((m_Shared32 & SHARED32_MASK_ENTERACTION) >> SHARED32_SHIFT_ENTERACTION); }
		int16 GetLeaveActionIndex() { return ((m_Shared32 & SHARED32_MASK_LEAVEACTION) >> SHARED32_SHIFT_LEAVEACTION); }
		void SetTokenID(int8 _TokenID) { m_Shared32 = ((m_Shared32 & ~SHARED32_MASK_TOKENID) | ((_TokenID << SHARED32_SHIFT_TOKENID) & SHARED32_MASK_TOKENID)); }
		void SetEnterActionIndex(int16 _EnterActionIndex) { m_Shared32 = ((m_Shared32 & ~SHARED32_MASK_ENTERACTION) | ((_EnterActionIndex << SHARED32_SHIFT_ENTERACTION) & SHARED32_MASK_ENTERACTION)); }
		void SetLeaveActionIndex(int16 _LeaveActionIndex) { m_Shared32 = ((m_Shared32 & ~SHARED32_MASK_LEAVEACTION) | ((_LeaveActionIndex << SHARED32_SHIFT_LEAVEACTION) & SHARED32_MASK_LEAVEACTION)); }
*/

		M_AGINLINE CMTime GetEnqueueTime() const { return m_EnqueueTime; }
		M_AGINLINE CMTime GetEnterTime() const { return m_EnterTime; }
		M_AGINLINE CMTime GetLeaveTime() const { return m_LeaveTime; }
		M_AGINLINE CMTime GetBlendOutEndTime() const { return m_BlendOutEndTime; }
		M_AGINLINE fp32 GetAnimTimeOffset() const { return m_Enter_AnimTimeOffset; }
		M_AGINLINE void SetEnqueueTime(CMTime _EnqueueTime) { m_EnqueueTime = _EnqueueTime; }
		M_AGINLINE void SetEnterTime(CMTime _EnterTime) { m_EnterTime = _EnterTime; }
		M_AGINLINE void SetLeaveTime(CMTime _LeaveTime) { m_LeaveTime = _LeaveTime; }
		M_AGINLINE void SetBlendOutEndTime(CMTime _BlendOutEndTime) { m_BlendOutEndTime = _BlendOutEndTime; }
		M_AGINLINE void SetAnimTimeOffset(fp32 _Offset) { m_Enter_AnimTimeOffset = _Offset; }

		static bool SameIdentity(const CWAG2I_SIP* _pSIPA, const CWAG2I_SIP* _pSIPB);
		static bool Equal(const CWAG2I_SIP* _pSIPA, const CWAG2I_SIP* _pSIPB);
		bool IsDepricated(CMTime _GameTime) const;

};

//--------------------------------------------------------------------------------

#endif /* WAG2I_StateInstPacked_h */
