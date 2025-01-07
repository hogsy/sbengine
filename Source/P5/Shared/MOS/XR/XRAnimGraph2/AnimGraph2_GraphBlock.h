#ifndef ANIMGRAPH2_GRAPHBLOCK_H
#define ANIMGRAPH2_GRAPHBLOCK_H


//--------------------------------------------------------------------------------

#include "AnimGraph2Defs.h"

//--------------------------------------------------------------------------------

enum
{
	// impulse types available
	XRAG2_IMPULSETYPE_UNDEFINED = -1,
	XRAG2_IMPULSETYPE_BEHAVIOR,
	XRAG2_IMPULSETYPE_WEAPON,
	XRAG2_IMPULSETYPE_CONTROLMODE,
	XRAG2_IMPULSETYPE_RESPONSE,

	XRAG2_IMPULSEVALUE_UNDEFINED = -1,

};

class CXRAG2;

class CXRAG2_Impulse
{
public:
	CAG2ImpulseType m_ImpulseType;
	CAG2ImpulseValue m_ImpulseValue;

	CXRAG2_Impulse(CAG2ImpulseType _Type, CAG2ImpulseValue _Value);
	CXRAG2_Impulse();

	bool operator==(const CXRAG2_Impulse& _Impulse)
	{
		return (m_ImpulseType == _Impulse.m_ImpulseType && m_ImpulseValue == _Impulse.m_ImpulseValue);
	}

	bool operator!=(const CXRAG2_Impulse& _Impulse)
	{
		return (m_ImpulseType != _Impulse.m_ImpulseType || m_ImpulseValue != _Impulse.m_ImpulseValue);
	}

	void Clear();

	bool Compare(const CXRAG2_Impulse& _Condition);

	// IO
	void Read(CCFile* _pDFile);
	void Write(CCFile* _pDFile);

#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		::SwapLE(m_ImpulseType);
		::SwapLE(m_ImpulseValue);
	}
#endif
};

// What impulses this block can react to, ex weaponswitching, controlmode switching, hurt responses
class CXRAG2_BlockReaction
{
protected:
	// IMPULSE -> REACTION MAP
public:
};

// Ok then, this contains what this block can react to (ex block for gun, unarmed, ladder, etc)
// And all states/actions/etc needed
// When we load from resource, put everything in the same global array? (so the memory overhead
// of having many graphblocks won't be so hard, like 100's of behaviors/dialogs)
// Adding block constants
class CXRAG2_GraphBlock
{
public:
	enum
	{
		EIOWholeStruct = 0
	};
public:
	CXRAG2_Impulse m_Condition;

	// Animgraph info for this block, the states should have the info for actions and stuff
	uint16 m_iStartMoveToken;
	//uint16 m_iStartState;
	uint16 m_iStateFullStart;
	uint16 m_StateFullLen;

	uint16 m_iSwitchStateStart;
	uint16 m_SwitchStateLen;

	uint16 m_iReactionFullStart;
	uint16 m_ReactionFullLen;
	
	uint16 m_iStateConstantStart;
	uint16 m_StateConstantLen;

public:
	CXRAG2_GraphBlock();

	void Clear()
	{
		m_iStartMoveToken = ~0;
		m_iStateFullStart = 0;
		m_StateFullLen = 0;
		m_iReactionFullStart = 0;
		m_iSwitchStateStart = 0;
		m_SwitchStateLen = 0;
		m_ReactionFullLen = 0;
		m_iStateConstantStart = 0;
		m_StateConstantLen = 0;
	}

	// IO
	void Read(CCFile* _pDFile, int _Ver);
	void Write(CCFile* _pDFile);

	M_AGINLINE CXRAG2_Impulse GetCondition() const { return m_Condition; }
	M_AGINLINE uint16 GetNumStates() const { return m_StateFullLen; }
	M_AGINLINE uint16 GetNumSwitchStates() const { return m_SwitchStateLen; }
	M_AGINLINE uint16 GetNumReactions() const { return m_ReactionFullLen; }
	M_AGINLINE uint8 GetNumStateConstants() const { return m_StateConstantLen; }
	M_AGINLINE uint16 GetBaseReactionIndex() const { return m_iReactionFullStart; }
	M_AGINLINE uint16 GetBaseStateIndex() const { return m_iStateFullStart; }
	M_AGINLINE uint16 GetBaseSwitchStateIndex() const { return m_iSwitchStateStart; }
	M_AGINLINE uint16 GetStartMoveTokenIndex() const { return m_iStartMoveToken; }
	M_AGINLINE uint16 GetBaseStateConstantIndex() const { return m_iStateConstantStart; }

#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		m_Condition.SwapLE();
		::SwapLE(m_iStartMoveToken);
		//::SwapLE(m_iStartState);
		::SwapLE(m_iStateFullStart);
		::SwapLE(m_StateFullLen);
		::SwapLE(m_iSwitchStateStart);
		::SwapLE(m_SwitchStateLen);
		::SwapLE(m_iReactionFullStart);
		::SwapLE(m_ReactionFullLen);
		::SwapLE(m_iStateConstantStart);
	}
#endif
};

#endif
