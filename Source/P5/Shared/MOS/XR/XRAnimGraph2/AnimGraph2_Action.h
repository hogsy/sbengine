#ifndef ANIMGRAPH2_ACTION_H
#define ANIMGRAPH2_ACTION_H

//--------------------------------------------------------------------------------

#include "AnimGraph2Defs.h"

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

class CXRAG2_MoveToken
{
public:
	enum
	{
		EIOWholeStruct = 0
	};
	fp32						m_AnimTimeOffset;
	fp32						m_AnimBlendDuration;
	fp32						m_AnimBlendDelay;
	CAG2StateIndex			m_iTargetState;  // 2 Byte
	CAG2GraphBlockIndex		m_iTargetGraphBlock;  // 2 Byte
	CAG2TokenID				m_TokenID; // 1 Byte 
	// Switchstate or normal state
	int8					m_TargetStateType;  // 1 Byte
	
	CXRAG2_MoveToken()
	{
		Clear();
	}
	void Clear();
	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

	M_AGINLINE CAG2TokenID		GetTokenID() const { return m_TokenID; }
	M_AGINLINE CAG2StateIndex		GetTargetStateIndex() const { return m_iTargetState; }
	M_AGINLINE CAG2GraphBlockIndex GetTargetGraphBlockIndex() const { return m_iTargetGraphBlock; }

	M_AGINLINE fp32		GetAnimBlendDuration() const { return m_AnimBlendDuration; }
	M_AGINLINE fp32		GetAnimBlendDelay() const { return m_AnimBlendDelay; }
	M_AGINLINE fp32		GetAnimTimeOffset() const { return m_AnimTimeOffset; }
	M_AGINLINE int8		GetTargetStateType() const { return m_TargetStateType; }

	static	M_AGINLINE CAG2TokenID			GetDefaultTokenID() { return AG2_TOKENID_NULL; }
	static	M_AGINLINE CAG2StateIndex			GetDefaultTargetStateIndex() { return AG2_STATEINDEX_NULL; }
	static	M_AGINLINE CAG2GraphBlockIndex	GetDefaultTargetGraphBlockIndex() { return AG2_GRAPHBLOCKINDEX_NULL; }
	static	M_AGINLINE fp32					GetDefaultAnimBlendDuration() { return 0; }
	static	M_AGINLINE fp32					GetDefaultAnimBlendDelay() { return 0; }
	static	M_AGINLINE fp32					GetDefaultAnimTimeOffset() { return 0; }
};

//--------------------------------------------------------------------------------

class CXRAG2_MoveAnimGraph
{
public:
	enum
	{
		EIOWholeStruct = 0
	};
	// Hrm?
	fp32						m_AnimBlendDuration;
	fp32						m_AnimBlendDelay;
	// Hashed value of animgraphname
	CAG2AnigraphNameHash	m_AnimGraphName;
	// 
	CAG2TokenID				m_TokenID; // 1 Byte

	CXRAG2_MoveAnimGraph()
	{
		Clear();
	}
	void Clear();
	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

	M_AGINLINE CAG2TokenID	GetTokenID() const { return m_TokenID; }
	M_AGINLINE CAG2AnigraphNameHash GetAnimGraphName() const { return m_AnimGraphName; }

	M_AGINLINE fp32		GetAnimBlendDuration() const { return m_AnimBlendDuration; }
	M_AGINLINE fp32		GetAnimBlendDelay() const { return m_AnimBlendDelay; }

	static	M_AGINLINE CAG2TokenID			GetDefaultTokenID() { return AG2_TOKENID_NULL; }
	static	M_AGINLINE CAG2AnigraphNameHash	GetDefaultAnimGraphName() { return AG2_ANIMGRAPHNAMEHASH_NULL; }
	static	M_AGINLINE fp32					GetDefaultAnimBlendDuration() { return 0; }
	static	M_AGINLINE fp32					GetDefaultAnimBlendDelay() { return 0; }
};

//--------------------------------------------------------------------------------

class CXRAG2_Action
{
public:
	enum
	{
		EIOWholeStruct = 0
	};

	CAG2EffectInstanceIndex	m_iBaseEffectInstance; // 2 Byte
	CAG2MoveTokenIndex		m_iBaseMoveToken;	// 2 byte
	uint8					m_nMoveTokens;		// 1 byte
	uint8					m_nEffectInstances;  // 1 Byte
	

	CXRAG2_Action() { Clear(); }
	void Clear();

	// Have to be changed...

	M_AGINLINE CAG2EffectInstanceIndex	GetBaseEffectInstanceIndex() const { return m_iBaseEffectInstance; }
	M_AGINLINE uint8					GetNumEffectInstances() const { return m_nEffectInstances; }
	M_AGINLINE CAG2MoveTokenIndex		GetBaseMoveTokenIndex() const { return m_iBaseMoveToken; }
	M_AGINLINE uint8					GetNumMoveTokens() const { return m_nMoveTokens; }

	void 					Read(CCFile* _pFile, int _Ver);
	void 					Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void 					SwapLE();
#endif


		// Defaults
static	M_AGINLINE CAG2EffectInstanceIndex	GetDefaultBaseEffectInstanceIndex() { return AG2_EFFECTINSTANCEINDEX_NULL; }
static	M_AGINLINE CAG2MoveTokenIndex		GetDefaultMoveTokenIndex() { return AG2_MOVETOKENINDEX_NULL; }
};


//--------------------------------------------------------------------------------

#endif /* AnimGraph_Action_h */
