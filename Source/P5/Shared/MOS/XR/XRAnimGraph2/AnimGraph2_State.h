#ifndef ANIMGRAPH2_STATE_H
#define ANIMGRAPH2_STATE_H

//--------------------------------------------------------------------------------

#include "AnimGraph2Defs.h"

//--------------------------------------------------------------------------------

class CXRAG2_State
{
public:
	enum
	{
		EIOWholeStruct = 0
	};

	CAG2StateFlags			m_lFlags[2];

	CAG2NodeIndex			m_iBaseNode; // -1 means the state have no nodes, and is thus eternal (until forced). // 2 Byte
	CAG2ActionIndex			m_iBaseAction; // 2 Byte
	CAG2CallbackParamIndex	m_iBasePropertyParam; // 2 Byte
	CAG2StateConstantIndex	m_iBaseConstant; // 2 Byte
	CAG2AnimLayerIndex		m_iBaseAnimLayer; // 2 Byte

	uint8					m_nConstants;
	uint8					m_nAnimLayers;
	uint8					m_Priority;

	CXRAG2_State() { Clear(); }
	void Clear();

	CAG2StateFlags			GetFlags(uint8 _iFlag) const { return m_lFlags[_iFlag]; }

	CAG2NodeIndex			GetBaseNodeIndex() const { return m_iBaseNode; }
	CAG2ActionIndex			GetBaseActionIndex() const { return m_iBaseAction; }
	CAG2CallbackParamIndex	GetBasePropertyParamIndex() const { return m_iBasePropertyParam; }

	CAG2StateConstantIndex	GetBaseConstantIndex() const { return m_iBaseConstant; }
	uint8					GetNumConstants() const { return m_nConstants; }

	CAG2AnimLayerIndex		GetBaseAnimLayerIndex() const { return m_iBaseAnimLayer; }
	uint8					GetNumAnimLayers() const { return m_nAnimLayers; }

	uint8					GetPriority() const { return m_Priority; }

	void					Read(CCFile* _pFile, int _Ver);
	void					Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void					SwapLE();
#endif

	int8					GetLoopControlAnimLayerIndex() const { return GetDefaultLoopControlAnimLayerIndex(); }

			// Defaults
	static	CAG2StateFlags			GetDefaultFlags(uint8 _iFlag) { return 0; }

	static	CAG2NodeIndex			GetDefaultBaseNodeIndex() { return AG2_NODEINDEX_NULL; }
	static	CAG2ActionIndex			GetDefaultBaseActionIndex() { return AG2_ACTIONINDEX_NULL; }
	static	CAG2CallbackParamIndex	GetDefaultBasePropertyParamIndex() { return AG2_CALLBACKPARAMINDEX_NULL; }
	static	CAG2StateConstantIndex	GetDefaultBaseConstantIndex() { return AG2_STATECONSTANTINDEX_NULL; }
	static	uint8					GetDefaultNumConstants() { return 0; }

	static	CAG2AnimLayerIndex		GetDefaultBaseAnimLayerIndex() { return AG2_ANIMLAYERINDEX_NULL; }
	static	uint8					GetDefaultNumAnimLayers() { return 0; }
	static	uint8					GetDefaultPriority() { return 0; }
	static	int8					GetDefaultLoopControlAnimLayerIndex() { return 0; }
};

// increased number of operators
class CXRAG2_SwitchStateActionVal
{
public:
	enum
	{
		EIOWholeStruct = 0
	};
	
	// Depending on what type of property we have, the constants will be interpreted differently
	union
	{
		fp32		m_ConstantFloat;	// [ 32bit Float Constant ]
		int32	m_ConstantInt;		// [ 32bit Int Constant ]
		int32	m_ConstantBool;		// [ 32bit Bool Constant ] (bool won't work well on ps3/360 it seems...)
	};
	// Which movetoken to use
	CAG2MoveTokenIndex m_iMoveToken;

	void Clear();

	M_AGINLINE void	SetConstantFloat(fp32 _Constant) { m_ConstantFloat = _Constant; };
	M_AGINLINE void	SetConstantInt(int32 _Constant) { m_ConstantInt = _Constant; };
	M_AGINLINE void	SetConstantBool(bool _Constant) { m_ConstantBool = _Constant; };

	M_AGINLINE fp32		GetConstantFloat() const { return m_ConstantFloat; }
	M_AGINLINE int32	GetConstantInt() const { return m_ConstantInt; }
	M_AGINLINE int32	GetConstantBool() const { return m_ConstantBool; }

	M_AGINLINE CAG2MoveTokenIndex	GetMoveTokenIndex() const { return m_iMoveToken; }

public:

	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif
};

class CXRAG2_SwitchState
{
public:
	enum
	{
		EIOWholeStruct = 0
	};

	//CAG2ActionIndex			m_iBaseAction; // 2 Byte
	CAG2ActionValIndex		m_iBaseActionValIndex; // 2 byt2

	CAG2PropertyID			m_iProperty;	// 1 byte
	int8					m_PropertyType;	// 1 byte
	
	uint8					m_NumActionVal;	// 1 byte
	// Default action is last (m_iBaseAction + m_NumActionVal - 1)
	uint8					m_Priority;	// 1 byte


	CXRAG2_SwitchState() { Clear(); }
	void Clear();

	M_AGINLINE CAG2PropertyID			GetPropertyID() const { return m_iProperty; }
	M_AGINLINE int8						GetPropertyType() const { return m_PropertyType; }
	//M_AGINLINE CAG2NodeIndex			GetBaseActionValIndex() const { return m_iBaseActionValIndex; }
	M_AGINLINE CAG2ActionIndex			GetBaseActionValIndex() const { return m_iBaseActionValIndex; }
	M_AGINLINE CAG2ActionIndex			GetDefaultActionValIndex() const { return (m_iBaseActionValIndex + m_NumActionVal - 1); }
	M_AGINLINE uint8					GetNumActionVal() const { return m_NumActionVal; }

	uint8					GetPriority() const { return m_Priority; }

	void					Read(CCFile* _pFile, int _Ver);
	void					Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void					SwapLE();
#endif

	// Defaults
	static	CAG2StateFlags			GetDefaultFlags(uint8 _iFlag) { return 0; }

	static	CAG2ActionIndex			GetDefaultBaseActionIndex() { return AG2_ACTIONINDEX_NULL; }
	static	CAG2CallbackParamIndex	GetDefaultBasePropertyParamIndex() { return AG2_CALLBACKPARAMINDEX_NULL; }

	static	uint8					GetDefaultPriority() { return 0; }
};

//--------------------------------------------------------------------------------

#endif /* AnimGraph_State_h */
