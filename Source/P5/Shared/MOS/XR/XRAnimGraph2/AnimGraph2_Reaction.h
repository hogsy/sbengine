#ifndef ANIMGRAPH2_REACTION_H
#define ANIMGRAPH2_REACTION_H

//--------------------------------------------------------------------------------

#include "AnimGraph2Defs.h"
#include "AnimGraph2_GraphBlock.h"

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

class CXRAG2_Reaction
{
public:
	enum
	{
		EIOWholeStruct = 0
	};

	// Impulse, what we react to
	CXRAG2_Impulse			m_Impulse;			// 4 byte
	CAG2EffectInstanceIndex	m_iBaseEffectInstance; // 2 Byte
	CAG2MoveTokenIndex		m_iBaseMoveToken;	// 2 byte
	CAG2MoveAnimgraphIndex  m_iBaseMoveAnimgraph; // 2 byte
	
	uint8					m_nEffectInstances;  // 1 Byte
	uint8					m_nMoveTokens;		// 1 byte
	uint8					m_nMoveAnimgraph;	// 1 byte
	


	CXRAG2_Reaction() { Clear(); }
	void Clear();

	M_AGINLINE CXRAG2_Impulse			GetImpulse() const { return m_Impulse; }
	M_AGINLINE CAG2EffectInstanceIndex	GetBaseEffectInstanceIndex() const { return m_iBaseEffectInstance; }
	M_AGINLINE CAG2MoveAnimgraphIndex GetBaseMoveAnimgraphIndex() const { return m_iBaseMoveAnimgraph; }
	M_AGINLINE CAG2MoveTokenIndex		GetBaseMoveTokenIndex() const { return m_iBaseMoveToken; }
	M_AGINLINE uint8					GetNumEffectInstances() const { return m_nEffectInstances; }
	
	M_AGINLINE uint8					GetNumMoveTokens() const { return m_nMoveTokens; }
	M_AGINLINE uint8					GetNumMoveAnimGraphs() const { return m_nMoveAnimgraph; }

	void 					Read(CCFile* _pFile, int _Ver);
	void 					Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void 					SwapLE();
#endif


		// Defaults
static	M_AGINLINE CAG2GraphBlockIndex		GetDefaultTargetGraphBlockIndex() { return AG2_GRAPHBLOCKINDEX_NULL; }
static	M_AGINLINE CAG2EffectInstanceIndex	GetDefaultBaseEffectInstanceIndex() { return AG2_EFFECTINSTANCEINDEX_NULL; }
static	M_AGINLINE CAG2MoveTokenIndex		GetDefaultMoveTokenIndex() { return AG2_MOVETOKENINDEX_NULL; }
static	M_AGINLINE CAG2MoveAnimgraphIndex	GetDefaultMoveAnimgraphIndex() { return AG2_MOVEANIMGRAPHINDEX_NULL; }
};


//--------------------------------------------------------------------------------

#endif /* AnimGraph_Action_h */
