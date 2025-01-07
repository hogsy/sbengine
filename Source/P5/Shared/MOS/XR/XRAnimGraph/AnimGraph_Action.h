#ifndef AnimGraph_Action_h
#define AnimGraph_Action_h

//--------------------------------------------------------------------------------

#include "AnimGraphDefs.h"

//--------------------------------------------------------------------------------
/*
class CXRAG_MoveAction
{
	public:

		uint8		m_TokenID;
		uint16		m_iTargetState;

		fp32			m_AnimBlendDuration;
		fp32			m_AnimBlendDelay;
		fp32			m_AnimTimeOffset;

}
*/
//--------------------------------------------------------------------------------

class CXRAG_Action
{
	public:

		// Interface
virtual	CAGTokenID				GetTokenID() const { return GetDefaultTokenID(); }
virtual	CAGStateIndex			GetTargetStateIndex() const { return GetDefaultTargetStateIndex(); }
virtual	fp32						GetAnimBlendDuration() const { return GetDefaultAnimBlendDuration(); }
virtual	fp32						GetAnimBlendDelay() const { return GetDefaultAnimBlendDelay(); }
virtual	fp32						GetAnimTimeOffset() const { return GetDefaultAnimTimeOffset(); }
virtual	int8					GetAnimTimeOffsetType() const { return GetDefaultAnimTimeOffsetType(); }
virtual	CAGEffectInstanceIndex	GetBaseEffectInstanceIndex() const { return GetDefaultBaseEffectInstanceIndex(); }
virtual	uint8					GetNumEffectInstances() const { return GetDefaultNumEffectInstances(); }

		// Defaults
static	CAGTokenID				GetDefaultTokenID() { return AG_TOKENID_NULL; }
static	CAGStateIndex			GetDefaultTargetStateIndex() { return AG_STATEINDEX_NULL; }
static	fp32						GetDefaultAnimBlendDuration() { return 0; }
static	fp32						GetDefaultAnimBlendDelay() { return 0; }
static	fp32						GetDefaultAnimTimeOffset() { return 0; }
static	int8					GetDefaultAnimTimeOffsetType() { return AG_ANIMLAYER_TIMEOFFSETTYPE_INDEPENDENT; }
static	CAGEffectInstanceIndex	GetDefaultBaseEffectInstanceIndex() { return AG_EFFECTINSTANCEINDEX_NULL; }
static	uint8					GetDefaultNumEffectInstances() { return 0; }

static	bool IsSmall(CXRAG_Action* _pAction);
};

//--------------------------------------------------------------------------------

class CXRAG_Action_Small : public CXRAG_Action
{
	private:
		fp32				m_AnimBlendDuration;

		CAGStateIndex	m_iTargetState; // -1 == Terminate // 2 Byte
		CAGTokenID		m_TokenID; // 1 Byte


	public:
	enum
	{
		EIOWholeStruct = 0
	};

		CXRAG_Action_Small() { Clear(); }
		void Clear();

		CAGTokenID		GetTokenID() const { return m_TokenID; }
		CAGStateIndex	GetTargetStateIndex() const { return m_iTargetState; }
		fp32				GetAnimBlendDuration() const { return m_AnimBlendDuration; }
		
		void			Read(CCFile* _pFile, int _Ver);
		void			Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
		void			SwapLE();
#endif

};

//--------------------------------------------------------------------------------

class CXRAG_Action_Full : public CXRAG_Action
{
	private:
	public:
	enum
	{
		EIOWholeStruct = 0
	};

		fp32						m_AnimTimeOffset;
		fp32						m_AnimBlendDuration;
		fp32						m_AnimBlendDelay;

		CAGEffectInstanceIndex	m_iBaseEffectInstance; // 2 Byte
		CAGStateIndex			m_iTargetState;  // 2 Byte
		CAGTokenID				m_TokenID; // 1 Byte
		uint8					m_nEffectInstances;  // 1 Byte
		int8					m_iAnimTimeOffsetType;  // 1 Byte
	public:

		CXRAG_Action_Full() { Clear(); }
		void Clear();

		CAGTokenID				GetTokenID() const { return m_TokenID; }
		CAGStateIndex			GetTargetStateIndex() const { return m_iTargetState; }

		fp32						GetAnimBlendDuration() const { return m_AnimBlendDuration; }
		fp32						GetAnimBlendDelay() const { return m_AnimBlendDelay; }
		fp32						GetAnimTimeOffset() const { return m_AnimTimeOffset; }
		int8					GetAnimTimeOffsetType() const { return m_iAnimTimeOffsetType; }
		
		CAGEffectInstanceIndex	GetBaseEffectInstanceIndex() const { return m_iBaseEffectInstance; }
		uint8					GetNumEffectInstances() const { return m_nEffectInstances; }

		void 					Read(CCFile* _pFile, int _Ver);
		void 					Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
		void 					SwapLE();
#endif

};

//--------------------------------------------------------------------------------

#endif /* AnimGraph_Action_h */