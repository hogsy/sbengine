#ifndef AnimGraph_State_h
#define AnimGraph_State_h

//--------------------------------------------------------------------------------

#include "AnimGraphDefs.h"

//--------------------------------------------------------------------------------

class CXRAG_State
{
	public:

		// Interface
virtual	CAGStateFlags			GetFlags(uint8 _iFlag) const { return GetDefaultFlags(_iFlag); }

virtual	CAGNodeIndex			GetBaseNodeIndex() const { return GetDefaultBaseNodeIndex(); }
virtual	CAGActionIndex			GetBaseActionIndex() const { return GetDefaultBaseActionIndex(); }
virtual	CAGCallbackParamIndex	GetBasePropertyParamIndex() const { return GetDefaultBasePropertyParamIndex(); }
virtual	CAGStateConstantIndex	GetBaseConstantIndex() const { return GetDefaultBaseConstantIndex(); }
virtual	uint8					GetNumConstants() const { return GetDefaultNumConstants(); }

virtual	CAGAnimLayerIndex		GetBaseAnimLayerIndex() const { return GetDefaultBaseAnimLayerIndex(); }
virtual	uint8					GetNumAnimLayers() const { return GetDefaultNumAnimLayers(); }
virtual	uint8					GetPriority() const { return GetDefaultPriority(); }
virtual	int8					GetLoopControlAnimLayerIndex() const { return GetDefaultLoopControlAnimLayerIndex(); }

virtual	CAGPropertyID			GetAQPCProperty() const { return GetDefaultAQPCProperty(); }
virtual	CAGOperatorID			GetAQPCOperator() const { return GetDefaultAQPCOperator(); }
virtual	fp32						GetAQPCConstant() const { return GetDefaultAQPCConstant(); }

		// Defaults
static	CAGStateFlags			GetDefaultFlags(uint8 _iFlag) { return 0; }

static	CAGNodeIndex			GetDefaultBaseNodeIndex() { return AG_NODEINDEX_NULL; }
static	CAGActionIndex			GetDefaultBaseActionIndex() { return AG_ACTIONINDEX_NULL; }
static	CAGCallbackParamIndex	GetDefaultBasePropertyParamIndex() { return AG_CALLBACKPARAMINDEX_NULL; }
static	CAGStateConstantIndex	GetDefaultBaseConstantIndex() { return AG_STATECONSTANTINDEX_NULL; }
static	uint8					GetDefaultNumConstants() { return 0; }

static	CAGAnimLayerIndex		GetDefaultBaseAnimLayerIndex() { return AG_ANIMLAYERINDEX_NULL; }
static	uint8					GetDefaultNumAnimLayers() { return 0; }
static	uint8					GetDefaultPriority() { return 0; }
static	int8					GetDefaultLoopControlAnimLayerIndex() { return 0; }

static	CAGPropertyID			GetDefaultAQPCProperty() { return AG_PROPERTY_NULL; }
static	CAGOperatorID			GetDefaultAQPCOperator() { return AG_OPERATOR_NULL; }
static	fp32						GetDefaultAQPCConstant() { return 0; }

	public:

		static bool IsSmall(CXRAG_State* _pState)
		{
			return ((_pState->GetAQPCProperty() == CXRAG_State::GetDefaultAQPCProperty()) &&
				(_pState->GetAQPCOperator() == CXRAG_State::GetDefaultAQPCOperator()) &&
				(_pState->GetAQPCConstant() == CXRAG_State::GetDefaultAQPCConstant()));
		}
};

//--------------------------------------------------------------------------------

class CXRAG_State_Small : public CXRAG_State
{
	public:
	enum
	{
		EIOWholeStruct = 0
	};

		CAGStateFlags			m_lFlags[2];

		CAGNodeIndex			m_iBaseNode; // 2
		CAGActionIndex			m_iBaseAction; // 2
		CAGCallbackParamIndex	m_iBasePropertyParam; // 2
		CAGStateConstantIndex	m_iBaseConstant; // 2
		CAGAnimLayerIndex		m_iBaseAnimLayer; // 2

		uint8					m_nConstants;
		uint8					m_nAnimLayers;
		uint8					m_Priority;

	public:

		CXRAG_State_Small() { Clear(); }
		void Clear();

		CAGStateFlags			GetFlags(uint8 _iFlag) const { return m_lFlags[_iFlag]; }

		CAGNodeIndex			GetBaseNodeIndex() const { return m_iBaseNode; }
		CAGActionIndex			GetBaseActionIndex() const { return m_iBaseAction; }
		CAGCallbackParamIndex	GetBasePropertyParamIndex() const { return m_iBasePropertyParam; }

		CAGStateConstantIndex	GetBaseConstantIndex() const { return m_iBaseConstant; }
		uint8					GetNumConstants() const { return m_nConstants; }

		CAGAnimLayerIndex		GetBaseAnimLayerIndex() const { return m_iBaseAnimLayer; }
		uint8					GetNumAnimLayers() const { return m_nAnimLayers; }

		uint8					GetPriority() const { return m_Priority; }

		void					Read(CCFile* _pFile, int _Ver);
		void					Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
		void					SwapLE();
#endif

};

//--------------------------------------------------------------------------------

class CXRAG_State_Full : public CXRAG_State
{
	public:
	enum
	{
		EIOWholeStruct = 0
	};

		CAGStateFlags			m_lFlags[2];

		fp32						m_AnimQueue_ProceedCondition_Constant;

		CAGNodeIndex			m_iBaseNode; // -1 means the state have no nodes, and is thus eternal (until forced). // 2 Byte
		CAGActionIndex			m_iBaseAction; // 2 Byte
		CAGCallbackParamIndex	m_iBasePropertyParam; // 2 Byte
		CAGStateConstantIndex	m_iBaseConstant; // 2 Byte
		CAGAnimLayerIndex		m_iBaseAnimLayer; // 2 Byte

		CAGPropertyID			m_AnimQueue_ProceedCondition_iProperty; // 1 Byte
		CAGOperatorID			m_AnimQueue_ProceedCondition_iOperator; // 1 Byte
		uint8					m_nConstants;
		uint8					m_nAnimLayers;
		uint8					m_Priority;

	public:

		CXRAG_State_Full() { Clear(); }
		void Clear();

		CAGStateFlags			GetFlags(uint8 _iFlag) const { return m_lFlags[_iFlag]; }

		CAGNodeIndex			GetBaseNodeIndex() const { return m_iBaseNode; }
		CAGActionIndex			GetBaseActionIndex() const { return m_iBaseAction; }
		CAGCallbackParamIndex	GetBasePropertyParamIndex() const { return m_iBasePropertyParam; }

		CAGStateConstantIndex	GetBaseConstantIndex() const { return m_iBaseConstant; }
		uint8					GetNumConstants() const { return m_nConstants; }

		CAGAnimLayerIndex		GetBaseAnimLayerIndex() const { return m_iBaseAnimLayer; }
		uint8					GetNumAnimLayers() const { return m_nAnimLayers; }

		uint8					GetPriority() const { return m_Priority; }

		CAGPropertyID			GetAQPCProperty() const { return m_AnimQueue_ProceedCondition_iProperty; }
		CAGOperatorID			GetAQPCOperator() const { return m_AnimQueue_ProceedCondition_iOperator; }
		fp32						GetAQPCConstant() const { return m_AnimQueue_ProceedCondition_Constant; }

		void					Read(CCFile* _pFile, int _Ver);
		void					Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
		void					SwapLE();
#endif

};

//--------------------------------------------------------------------------------

#endif /* AnimGraph_State_h */