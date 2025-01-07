#ifndef ANIMGRAPH2COMPILER_H
#define ANIMGRAPH2COMPILER_H

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// !!!NOTE!!! TODO: Ta bort den här skiten i RTM. Behöver inte finnas.
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

#include "../AnimGraph2.h"
#ifndef TARGET_XBOX
#include "../../../MCC/Mda.h"
#include "../../../MCC/Mrtc.h"
#endif

//--------------------------------------------------------------------------------

#define AG2C_NODEACTION_BITS			18

#define AG2C_NODEACTION_TYPEMASK		0xC000
#define AG2C_NODEACTION_ACTIONMASK	0x0FFF
#define AG2C_NODEACTION_NODEMASK		0x0FFF

#define AG2C_NODEACTION_ENDPARSE		0x8000
#define AG2C_NODEACTION_PARSENODE	0x4000
#define AG2C_NODEACTION_FAILPARSE	(AG2C_NODEACTION_ENDPARSE | ((-1) & AG2C_NODEACTION_ACTIONMASK))

#define AG2C_INHERITFLAGS_FLAGS0					0x0001
#define AG2C_INHERITFLAGS_FLAGS1					0x0002
#define AG2C_INHERITFLAGS_PRIORITY				0x0004
#define AG2C_INHERITFLAGS_ANIMINDEX				0x0008
#define AG2C_INHERITFLAGS_ANIMBASEJOINT			0x0010
#define AG2C_INHERITFLAGS_ANIMTIMEOFFSETTYPE		0x0020
#define AG2C_INHERITFLAGS_ANIMTIMEOFFSET			0x0040
#define AG2C_INHERITFLAGS_ANIMTIMESCALE			0x0080
#define AG2C_INHERITFLAGS_ANIMOPACITY			0x0100
#define AG2C_INHERITFLAGS_ANIMFLAGS			0x0200

#define AG2C_INHERITFLAGS_ALL					0xffffffff

#define AG2C_DISABLEDWARNING_MISSINGANIMATIONINDEX	0x01
#define AG2C_DISABLEDWARNING_MISSINGACTIONS			0x02

//--------------------------------------------------------------------------------

extern const char* ms_lpAG2CAnimTimeOffsetTypes[];

//--------------------------------------------------------------------------------

class CXRAG2C_Function
{
	public:

		CFStr	m_Name;
		int32	m_ID;
		bool	m_bReferenced;

		CXRAG2C_Function()
		{
			m_ID = -1;
			m_bReferenced = 0;
		}

		CXRAG2C_Function(const char* _Name, int32 _ID)
		{
			m_Name = _Name;
			m_ID = _ID;
		}
};

typedef CXRAG2C_Function CXRAG2C_PropertyFloat;
typedef CXRAG2C_Function CXRAG2C_PropertyCondition;
typedef CXRAG2C_Function CXRAG2C_PropertyFunction;
typedef CXRAG2C_Function CXRAG2C_PropertyInt;
typedef CXRAG2C_Function CXRAG2C_PropertyBool;
typedef CXRAG2C_Function CXRAG2C_Operator;
typedef CXRAG2C_Function CXRAG2C_Effect;

//--------------------------------------------------------------------------------

class CXRAG2C_StateConstant
{
	public:

		CFStr	m_Name;
		int32	m_ID;
		fp32		m_Value;

		CXRAG2C_StateConstant()
		{
			m_ID = -1;
			m_Value = 0;
		}

		CXRAG2C_StateConstant(const char* _Name, int32 _ID, fp32 _Value = 0)
		{
			m_Name = _Name;
			m_ID = _ID;
			m_Value = _Value;
		}
};

//--------------------------------------------------------------------------------

class CXRAG2C_Constant_Float
{
	public:

		CFStr	m_Name;
		fp32		m_Value;

		CXRAG2C_Constant_Float()
		{
			m_Value = 0;
		}

		CXRAG2C_Constant_Float(const char* _Name, fp32 _Value)
		{
			m_Name = _Name;
			m_Value = _Value;
		}
};

class CXRAG2C_Constant_Int
{
public:

	CFStr	m_Name;
	int		m_Value;

	CXRAG2C_Constant_Int()
	{
		m_Value = 0;
	}

	CXRAG2C_Constant_Int(const char* _Name, int _Value)
	{
		m_Name = _Name;
		m_Value = _Value;
	}
};

class CXRAG2C_Constant_Bool
{
public:

	CFStr	m_Name;
	bool	m_Value;

	CXRAG2C_Constant_Bool()
	{
		m_Value = false;
	}

	CXRAG2C_Constant_Bool(const char* _Name, bool _Value)
	{
		m_Name = _Name;
		m_Value = _Value;
	}
};

//--------------------------------------------------------------------------------

class CXRAG2C_ImpulseConstant
{
public:

	CFStr	m_Name;
	int32	m_Value;

	CXRAG2C_ImpulseConstant()
	{
		m_Value = 0;
	}

	CXRAG2C_ImpulseConstant(const char* _Name, fp32 _Value)
	{
		m_Name = _Name;
		m_Value = (int32)_Value;
	}
};

//--------------------------------------------------------------------------------

class CXRAG2C_StateFlag
{
	public:

		CFStr	m_Name;
		uint32	m_Value;
		
		CXRAG2C_StateFlag()
		{
			m_Value = 0;
		}

		CXRAG2C_StateFlag(const char* _Name, uint32 _Value)
		{
			m_Name = _Name;
			m_Value = _Value;
		}
};

class CXRAG2C_AnimFlag
{
public:

	CFStr	m_Name;
	uint32	m_Value;

	CXRAG2C_AnimFlag()
	{
		m_Value = 0;
	}

	CXRAG2C_AnimFlag(const char* _Name, uint32 _Value)
	{
		m_Name = _Name;
		m_Value = _Value;
	}
};

//--------------------------------------------------------------------------------

class CXRAG2C_StateAnim
{
	public:
		CStr		m_Name;
		int16		m_iAnim;
		uint32		m_AnimFlags;
		uint8		m_iBaseJoint;
//		uint8		m_TimeOffsetType;
		fp32			m_TimeOffset;
		fp32			m_TimeScale;
		fp32			m_Opacity;
		uint8		m_iTimeControlProperty;
		uint8		m_iMergeOperator;

		CXRAG2C_StateAnim()
		{
			m_iAnim = -1;
			m_AnimFlags = 0;
			m_iBaseJoint = 0;
//			m_TimeOffsetType = 0;
			m_TimeOffset = 0;
			m_TimeScale = 1;
			m_Opacity = 1;
			m_iTimeControlProperty = 0;
			m_iMergeOperator = 0;
		}
};

class CXRAG2C_Animation
{
public:
	CStr m_Name;	// Name of the animation
	CStr m_Value;	// animation container and index
	// Offset from base
	int m_iOffset;
	int m_iContainerName;
	int16 m_iAnimSeq;
	// Unreferenced animations will be dropped when processing
	int m_nReferences;
	CXRAG2C_Animation()
	{
		m_nReferences = 0;
		m_iOffset = 0;
		m_iContainerName = -1;
		m_iAnimSeq = -1;
	}
	CXRAG2C_Animation(const CStr& _Name, const CStr& _Value)
	{
		m_iOffset = 0;
		m_Name = _Name;
		m_Value = _Value;
		m_nReferences = 0;
		m_iContainerName = -1;
		m_iAnimSeq = -1;
	}

	void operator=(CXRAG2C_Animation& _Anim)
	{
		CopyFrom(_Anim);
	}

	void CopyFrom(const CXRAG2C_Animation& _Anim)
	{
		m_Name = _Anim.m_Name;
		m_Value = _Anim.m_Value;
		m_iOffset = _Anim.m_iOffset;
		m_iContainerName = _Anim.m_iContainerName;
		m_iAnimSeq = _Anim.m_iAnimSeq;
		m_nReferences = _Anim.m_nReferences;
	}
};


class CXRAG2C_AnimContainerNames
{
public:
	TThinArray<CStr> m_lNames;

	void Clear()
	{
		m_lNames.Clear();
	}

	int32 GetNameIndex(CStr _ContainerName)
	{
		int32 Len = m_lNames.Len();
		CStr Upper = _ContainerName.UpperCase();
		for (int32 i = 0; i < Len; i++)
		{
			if (m_lNames[i] == Upper)
				return i;
		}
		int32 Index = m_lNames.Len();
		m_lNames.SetLen(Index+1);
		m_lNames[Index] = Upper;

		return Index;
	}

	CStr GetContainerName(int32 _iContainerName) const
	{
		if (m_lNames.ValidPos(_iContainerName))
			return m_lNames[_iContainerName];
		return CStr("");
	}
};

//--------------------------------------------------------------------------------
class CXRAG2C_MoveToken
{
public:
	
	CFStr			m_TargetState;
	CFStr			m_TargetGraphBlock;
	fp32				m_AnimBlendDuration;
	fp32				m_AnimBlendDelay;
	fp32				m_AnimTimeOffset;

	int16			m_iTargetState;
	int16			m_iTargetGraphBlock;
	int8			m_TokenID;
	int8			m_TargetStateType;
	

	CXRAG2C_MoveToken()
	{
		Clear();
	}

	void Clear()
	{
		m_TokenID = AG2_TOKENID_NULL;
		m_TargetState.Clear();
		m_TargetGraphBlock.Clear();
		m_iTargetState = AG2_STATEINDEX_NULL;
		m_iTargetGraphBlock = AG2_GRAPHBLOCKINDEX_NULL;

		m_AnimBlendDuration = 0.0f;
		m_AnimBlendDelay = 0.0f;
		m_AnimTimeOffset = 0.0f;
		m_TargetStateType = 0;
	}

	void operator=(CXRAG2C_MoveToken& _MoveToken)
	{
		CopyFrom(_MoveToken);
	}

	void CopyFrom(const CXRAG2C_MoveToken& _Token)
	{
		m_TokenID = _Token.m_TokenID;
		m_iTargetState = _Token.m_iTargetState;
		m_TargetState = _Token.m_TargetState;
		m_TargetGraphBlock = _Token.m_TargetGraphBlock;
		m_iTargetGraphBlock = _Token.m_iTargetGraphBlock;

		m_AnimBlendDuration = _Token.m_AnimBlendDuration;
		m_AnimBlendDelay = _Token.m_AnimBlendDelay;
		m_AnimTimeOffset = _Token.m_AnimTimeOffset;
		m_TargetStateType = _Token.m_TargetStateType;
	}
};

class CXRAG2C_MoveAnimGraph
{
public:

	CFStr			m_TargetAnimGraph;
	fp32				m_AnimBlendDuration;
	fp32				m_AnimBlendDelay;

	int32			m_TargetAnimGraphHash;
	int8			m_TokenID;


	CXRAG2C_MoveAnimGraph()
	{
		Clear();
	}

	void Clear()
	{
		m_TargetAnimGraph.Clear();
		m_AnimBlendDuration = 0.0f;
		m_AnimBlendDelay = 0.0f;

		m_TokenID = -1;
	}

	void operator=(CXRAG2C_MoveAnimGraph& _MoveAG)
	{
		CopyFrom(_MoveAG);
	}

	void CopyFrom(const CXRAG2C_MoveAnimGraph& _MoveAG)
	{
		m_TokenID = _MoveAG.m_TokenID;
		m_TargetAnimGraphHash = _MoveAG.m_TargetAnimGraphHash;
		m_TargetAnimGraph = _MoveAG.m_TargetAnimGraph;
	
		m_AnimBlendDuration = _MoveAG.m_AnimBlendDuration;
		m_AnimBlendDelay = _MoveAG.m_AnimBlendDelay;
	}
};

class CXRAG2C_SwitchStateActionval : public CReferenceCount
{
public:
	CStr			m_ConstantStr;
	fp32				m_ConstantValueFloat;
	int				m_ConstantValueInt;
	bool			m_ConstantValueBool;
	int				m_PropertyType;

	// If other is less than this return -> -1, if it's equal -> 0, otherwise 1
	int Compare(const CXRAG2C_SwitchStateActionval& _Other)const
	{
		switch (m_PropertyType)
		{
		case AG2_PROPERTYTYPE_FLOAT:
			{
				return (int)(_Other.m_ConstantValueFloat - m_ConstantValueFloat);
			}
		case AG2_PROPERTYTYPE_FUNCTION:
		case AG2_PROPERTYTYPE_INT:
			{
				return (_Other.m_ConstantValueInt - m_ConstantValueInt);
			}
		default:
			{
				// Shouldn't get here!
				return 0;
			}
		}
	}
	CXRAG2C_SwitchStateActionval()
	{
		Clear();
	}
	void Clear()
	{
		m_ConstantValueFloat = 0.0f;
		m_ConstantValueInt = 0;
		m_ConstantValueBool = false;
		m_PropertyType = -1;
	}

	CXRAG2C_SwitchStateActionval(const CXRAG2C_SwitchStateActionval& _ActionVal)
	{
		CopyFrom(_ActionVal);
	}

	void operator=(const CXRAG2C_SwitchStateActionval& _ActionVal)
	{
		CopyFrom(_ActionVal);
	}

	// Needed to safely duplicate m_lConditionMap. Otherwise it'll be a pointer only copy.
	void CopyFrom(const CXRAG2C_SwitchStateActionval& _ActionVal)
	{
		m_ConstantStr = _ActionVal.m_ConstantStr;
		m_ConstantValueFloat = _ActionVal.m_ConstantValueFloat;
		m_ConstantValueInt = _ActionVal.m_ConstantValueInt;
		m_ConstantValueBool = _ActionVal.m_ConstantValueBool;
		m_PropertyType = _ActionVal.m_PropertyType;
	}
};

typedef TPtr<CXRAG2C_SwitchStateActionval> spCXRAG2C_SwitchStateActionval;

class CXRAG2C_Action
{
	public:

		CFStr			m_Name;
		bool			m_bExported;

		// Multiple movetokens / action (maybe not as useful as it could have been with
		// the addition of graphblocks and reactions, but I'm sure it can be used)
		TArray<CXRAG2C_MoveToken> m_lMoveTokens;
		int16			m_iMoveTokenStart;
		int8			m_nMoveTokens;

		/*int8			m_TokenID;
		CFStr			m_TargetState;
		int16			m_iTargetState;

		fp32				m_AnimBlendDuration;
		fp32				m_AnimBlendDelay;
		fp32				m_AnimTimeOffset;
		int8			m_iAnimTimeOffsetType;*/

		int				m_iBaseEffectInstance;
		int				m_nEffectInstances;

		TArray<int32>	m_lConditionMap;
		uint32			m_ConditionBits; // (Built during State::Process())

		// For switchstates
		CXRAG2C_SwitchStateActionval m_ActionVal;

		CXRAG2C_Action(const CXRAG2C_Action& _Action)
		{
			CopyFrom(_Action);
		}

		CXRAG2C_Action()
		{
			Clear();
		}

		void Clear()
		{
			m_Name.Clear();
			m_bExported = false;

			m_lMoveTokens.Clear();
			m_iMoveTokenStart = 0;
			m_nMoveTokens = 0;

			m_iBaseEffectInstance = 0;
			m_nEffectInstances = 0;

			m_lConditionMap.Clear();
			m_ConditionBits = 0;
		}

		void operator=(const CXRAG2C_Action& _Action)
		{
			CopyFrom(_Action);
		}

		// Needed to safely duplicate m_lConditionMap. Otherwise it'll be a pointer only copy.
		void CopyFrom(const CXRAG2C_Action& _Action)
		{
			m_Name = _Action.m_Name;
			m_bExported = _Action.m_bExported;
			m_lMoveTokens.SetLen(_Action.m_lMoveTokens.Len());
			for (int32 i = 0; i < _Action.m_lMoveTokens.Len(); i++)
				m_lMoveTokens[i].CopyFrom(_Action.m_lMoveTokens[i]);

			m_iMoveTokenStart = _Action.m_iMoveTokenStart;
			m_nMoveTokens = _Action.m_nMoveTokens;
			/*m_TokenID = _Action.m_TokenID;

			m_TargetState = _Action.m_TargetState;
			m_iTargetState = _Action.m_iTargetState;

			m_AnimBlendDuration = _Action.m_AnimBlendDuration;
			m_AnimBlendDelay = _Action.m_AnimBlendDelay;
			m_AnimTimeOffset = _Action.m_AnimTimeOffset;
			m_iAnimTimeOffsetType = _Action.m_iAnimTimeOffsetType;*/

			m_iBaseEffectInstance = _Action.m_iBaseEffectInstance;
			m_nEffectInstances = _Action.m_nEffectInstances;

			_Action.m_lConditionMap.Duplicate(&m_lConditionMap);
			m_ConditionBits = _Action.m_ConditionBits;
			m_ActionVal = _Action.m_ActionVal;
		}

};

//--------------------------------------------------------------------------------

class CXRAG2C_EffectInstance
{
public:

	CAG2EffectID		m_ID;
	TArray<int>		m_lParams;

	CXRAG2C_EffectInstance()
	{
		m_ID = AG2_EFFECTID_NULL;
	}

	CXRAG2C_EffectInstance(CAG2EffectID _ID)
	{
		m_ID = _ID;
	}

	CXRAG2C_EffectInstance(const CXRAG2C_EffectInstance& _EffectInstance)
	{
		m_ID = _EffectInstance.m_ID;
		_EffectInstance.m_lParams.Duplicate(&m_lParams);
	}

};

//--------------------------------------------------------------------------------

class CXRAG2C_ConditionNode
{
	public:

		CAG2OperatorID	m_iOperator;
		CAG2PropertyID	m_iProperty;
		int				m_PropertyType;
		fp32				m_ConstantFloat;
		int				m_ConstantInt;
		bool			m_ConstantBool;
		uint16			m_TrueAction;
		uint16			m_FalseAction;

	public:

		CXRAG2C_ConditionNode()
		{
			m_iOperator = AG2_OPERATOR_NULL;
			m_iProperty = AG2_PROPERTY_NULL;
			m_PropertyType = AG2_PROPERTYTYPE_UNDEFINED;
			m_ConstantFloat = 0;
			m_ConstantInt = 0;
			m_ConstantBool = 0;
			m_TrueAction = 0;
			m_FalseAction = 0;
		}

		CXRAG2C_ConditionNode(const CXRAG2C_ConditionNode& _CNode)
		{
			m_iOperator = _CNode.m_iOperator;
			m_iProperty = _CNode.m_iProperty;
			m_PropertyType = _CNode.m_PropertyType;
			m_ConstantFloat = _CNode.m_ConstantFloat;
			m_ConstantInt = _CNode.m_ConstantInt;
			m_ConstantBool = _CNode.m_ConstantBool;
			m_TrueAction = _CNode.m_TrueAction;
			m_FalseAction = _CNode.m_FalseAction;
		}

		bool operator==(CXRAG2C_ConditionNode& _Node)
		{
			return ((m_iOperator == _Node.m_iOperator) &&
					(m_iProperty == _Node.m_iProperty) &&
					(m_PropertyType == _Node.m_PropertyType) &&
					(m_ConstantFloat == _Node.m_ConstantFloat) &&
					(m_ConstantInt == _Node.m_ConstantInt) &&
					(m_ConstantBool == _Node.m_ConstantBool) &&
					(m_TrueAction == _Node.m_TrueAction) &&
					(m_FalseAction == _Node.m_FalseAction));
		}

};

//--------------------------------------------------------------------------------

class CXRAG2C_Condition
{
	public:

		CAG2PropertyID	m_iProperty;
		CAG2OperatorID	m_iOperator;
		CStr			m_ConstantStr;
		fp32				m_ConstantValueFloat;
		int				m_ConstantValueInt;
		bool			m_ConstantValueBool;
		int				m_PropertyType;

		uint8			m_Occurrences;
		int				m_iFirstAction;
		uint16			m_SortedIndex; // Includes SortedBit (Built during State::Process())

		bool operator==(CXRAG2C_Condition& _Condition)
		{
			return ((m_iProperty == _Condition.m_iProperty) && 
					(m_iOperator == _Condition.m_iOperator) &&
					(m_PropertyType == _Condition.m_PropertyType) &&
//					(m_ConstantValue == _Condition.m_ConstantValue)); // FIXME: Will reuse conditions better, but is unfortunately not available at that time?
					(m_ConstantStr == _Condition.m_ConstantStr));
		}

		CXRAG2C_Condition()
		{
			m_iProperty = 0;
			m_iOperator = 0;
			m_PropertyType = AG2_PROPERTYTYPE_UNDEFINED;
			m_ConstantValueFloat = 0;
			m_ConstantValueInt = 0;
			m_ConstantValueBool = 0;
			m_Occurrences = 0;
			m_iFirstAction = AG2_ACTIONINDEX_NULL;
			m_SortedIndex = 0;
		}

};

//--------------------------------------------------------------------------------
class CXRAG2C_State_Base : public CReferenceCount
{
public:
	CFStr								m_Name;
	uint8								m_Priority;
	int									m_References;
	int									m_Index;

	TArray<CXRAG2C_EffectInstance>		m_lEffectInstances;

	class CXR_AnimGraph2Compiler*		m_pAGCompiler;
	class CXRAG2C_GraphBlock*			m_pGraphBlock;

	bool ParseMoveTokenReg(const CRegistry* _pTargetStateReg, CStr _Location, CXRAG2C_MoveToken& _MoveToken);
	bool ParseActionEffectsReg(const CRegistry* _pEffectsReg, int& _iBaseEffect, int& _nEffects, CStr _Location);
};

class CXRAG2C_State : public CXRAG2C_State_Base
{
public:
	uint32								m_Flags[2], m_FlagsMask[2];
	

	TArray<CXRAG2C_StateAnim>			m_lStateAnims;

	TArray<CXRAG2C_Action>				m_lActions;
	TArray<CXRAG2C_ConditionNode>		m_lConditionNodes;
	TArray<CXRAG2C_StateConstant>		m_lConstants;

	TArray<CFStr>						m_lSuperStates;
	uint32								m_InheritFlags;
	bool								m_bExpandedInheritance;
	bool								m_bPendingExpandInheritance;

	uint32								m_DisabledWarnings, m_EnabledWarnings, m_InheritDEWarnings;

	TArray<CXRAG2C_Condition>			m_lConditions;

	bool ExpandInheritance();
	bool Inherit(CXRAG2C_State& _SuperState);

	bool ParseReg(CXRAG2C_GraphBlock* _pGraphBlock, const CRegistry* _pStateReg);
	bool ParseInheritReg(const CRegistry* _pInheritReg);
	bool ParseAnimationReg(const CRegistry* _pAnimationReg);
	bool ParseStateConstantsReg(const CRegistry* _pStateConstantsReg);
	virtual bool ParseActionsReg(const CRegistry* _pActionsReg);
	bool ParseActionReg(const CRegistry* _pActionReg);
	bool ParseConditionSetReg(const CRegistry* _pActionReg, CStr _Location, CStr _ActionName, CXRAG2C_Action& _Action
								/*bool _bExported, int8 _TokenID, CStr _TargetState, int _iBaseEffect, int _nEffects, 
								fp32 _AnimBlendDuration, fp32 _AnimBlendDelay, fp32 _AnimTimeOffset, int8 _iAnimTimeOffsetType*/);
	int ParseConditionReg(const CRegistry* _pConditionReg, int _iAction, CStr _Location);

	int32 AddAction(CXRAG2C_Action& _Action);
	int32 AddCondition(CXRAG2C_Condition& _Condition);

	bool ProcessPass1();
	bool ProcessPass2();
	void BuildNodeTree();
	int BuildNodeTree_Recurse(int _Level, int _Offset, int _Width);
	void OptimizeNodeTree();
	bool ConvertNodeActions();
	bool ConvertNodeAction(uint16& _NodeAction);
};

typedef TPtr<CXRAG2C_State> spCXRAG2C_State;

// New type of state the switchstate, looks at one property and checks it's action
// values against that
class CXRAG2C_SwitchState : public CXRAG2C_State_Base
{
public:

	CFStr								m_Name;
	uint8								m_Priority;

	int									m_References;
	int									m_Index;

	TArray<CXRAG2C_Action>				m_lActions;
	CXRAG2C_Action						m_DefaultAction;
	TArray<CXRAG2C_EffectInstance>		m_lEffectInstances;
	TArray<CXRAG2C_SwitchStateActionval>		m_lActionVals;

	TArray<fp32>							m_CallbackParams;

	// Should only be one condition, just keep property and propertytype
	int32	m_iProperty;
	int32	m_PropertyType;

	bool ParseReg(CXRAG2C_GraphBlock* _pGraphBlock, const CRegistry* _pStateReg);
	bool ParseActionsReg(const CRegistry* _pActionsReg);
	bool ParseActionReg(const CRegistry* _pActionReg, CXRAG2C_Action& _Action);
	bool ParseSwitchStateActionValReg(const CRegistry* _pActionReg, CXRAG2C_SwitchStateActionval& _ActionVal);

	// Only parse the only condition there should 
	bool ParsePropertyReg(const CRegistry* _pConditionReg, CStr _Location);

	bool ProcessPass1();
	bool ProcessPass2();

	// REPLACE ME!!!
	bool ConvertNodeActions();
	bool ConvertNodeAction(uint16& _NodeAction);

};

typedef TPtr<CXRAG2C_SwitchState> spCXRAG2C_SwitchState;

class CXRAG2C_Impulse
{
public:
	CAG2ImpulseType m_ImpulseType;
	CAG2ImpulseType m_ImpulseValue;

	CXRAG2C_Impulse()
	{
		Clear();
	}
	void Clear()
	{
		m_ImpulseType = AG2_IMPULSETYPE_UNDEFINED;
		m_ImpulseValue = AG2_IMPULSEVALUE_UNDEFINED;
	}
	
	bool ParseReg(const CRegistry* _pImpulseReg);
};

// The graphblock contains the states, the animgraph contains the graphblocks

class CXRAG2C_Common
{
public:
	TArray<CStr>							m_lFilenames;

	void PushFileName(CStr _Filename);
	void PopFileName();
	CStr GetFileName();
	CStr GetFilePath();
	static CStr ResolvePath(CStr _Path);

	void OnError(const char* _pMsg);
	void OnWarning(const char* _pMsg);
	void OnNote(const char* _pMsg);
	void OnMsg(const char* _pMsg);
};

// Action like thingy, actions where the conditions are impulses
class CXRAG2C_Reaction
{
public:

	CFStr			m_Name;
	bool			m_bExported;
	//int8			m_TokenID;

	TArray<CXRAG2C_MoveToken> m_lMoveTokens;
	TArray<CXRAG2C_MoveAnimGraph> m_lMoveAnimGraphs;
	int16			m_iMoveTokenStart;
	int16			m_iMoveAnimGraphStart;
	int8			m_nMoveTokens;
	int8			m_nMoveAnimGraphs;

	// If not defined use start state in target graphblock
	/*CFStr			m_TargetState;
	int16			m_iTargetState;
	fp32				m_AnimBlendDuration;
	fp32				m_AnimBlendDelay;
	fp32				m_AnimTimeOffset;
	int8			m_iAnimTimeOffsetType;*/

	// Always set this, if not defined use current graphblock (like movetoken not adding token id)
	/*CFStr			m_TargetGraphBlock;
	int16			m_iTargetGraphBlock;*/

	
	int				m_iBaseEffectInstance;
	int				m_nEffectInstances;

	CXRAG2C_Impulse	m_Condition;

	// Skip conditions for now...
	/*TArray<int32>	m_lConditionMap;
	uint32			m_ConditionBits; // (Built during State::Process())*/

	void operator=(CXRAG2C_Reaction& _Reaction)
	{
		CopyFrom(_Reaction);
	}

	// Needed to safely duplicate m_lConditionMap. Otherwise it'll be a pointer only copy.
	void CopyFrom(CXRAG2C_Reaction& _Reaction)
	{
		m_Name = _Reaction.m_Name;
		m_bExported = _Reaction.m_bExported;
		m_Condition = _Reaction.m_Condition;

		m_iMoveTokenStart = _Reaction.m_iMoveTokenStart;
		m_iMoveAnimGraphStart = _Reaction.m_iMoveAnimGraphStart;
		m_iBaseEffectInstance = _Reaction.m_iBaseEffectInstance;
		m_nEffectInstances = _Reaction.m_nEffectInstances;
		m_nMoveAnimGraphs = _Reaction.m_nMoveAnimGraphs;

		m_lMoveTokens.SetLen(_Reaction.m_lMoveTokens.Len());
		for (int32 i = 0; i < _Reaction.m_lMoveTokens.Len(); i++)
			m_lMoveTokens[i].CopyFrom(_Reaction.m_lMoveTokens[i]);

		m_lMoveAnimGraphs.SetLen(_Reaction.m_lMoveAnimGraphs.Len());
		for (int32 i = 0; i < _Reaction.m_lMoveAnimGraphs.Len(); i++)
			m_lMoveAnimGraphs[i].CopyFrom(_Reaction.m_lMoveAnimGraphs[i]);

	}
};

class CXRAG2C_Reactions : public CReferenceCount
{
public:

	CFStr								m_Name;
	int									m_Index;

	TArray<CXRAG2C_Reaction>			m_lReactions;
	TArray<CXRAG2C_EffectInstance>		m_lEffectInstances;
	TArray<CXRAG2C_ConditionNode>		m_lConditionNodes;
	//TArray<CXRAG2C_StateConstant>		m_lConstants;
	TArray<fp32>							m_CallbackParams;

	TArray<CFStr>						m_lSuperReactions;
	uint32								m_InheritFlags;
	bool								m_bExpandedInheritance;
	bool								m_bPendingExpandInheritance;

	//TArray<CXRAG2C_Condition>			m_lConditions;

	class CXR_AnimGraph2Compiler*		m_pAGCompiler;
	class CXRAG2C_GraphBlock*			m_pGraphBlock;

	bool ExpandInheritance();
	bool Inherit(CXRAG2C_Reactions& _SuperReaction);

	// External reactions
	bool IncludeReactionsFile(CStr _FileName);

	bool ParseReg(CXRAG2C_GraphBlock* _pGraphBlock, const CRegistry* _pReactionsReg);
	bool ParseInheritReg(const CRegistry* _pInheritReg);
	bool ParseReactionsReg(const CRegistry* _pActionsReg);
	bool ParseReactionReg(const CRegistry* _pActionReg);
	bool ParseMoveTokenReg(const CRegistry* _pTargetStateReg, CStr _Location, CXRAG2C_MoveToken& _MoveToken);//int8& _TokenID, CFStr& _TargetState, CFStr& _TargetGraphBlock, fp32& _AnimBlendDuration, fp32& _AnimBlendDelay, fp32& _AnimTimeOffset, int8& _iAnimTimeOffsetType);
	bool ParseMoveAnimGraphReg(const CRegistry* _pMoveAG, CStr _Location, CXRAG2C_MoveAnimGraph& _MoveAG);
	bool ParseActionEffectsReg(const CRegistry* _pEffectsReg, int& _iBaseEffect, int& _nEffects, CStr _Location);
	/*bool ParseConditionSetReg(const CRegistry* _pActionReg, CStr _Location, CStr _ActionName,
		bool _bExported, int8 _TokenID, CStr _TargetState, CStr _TargetGraphBlock, int _iBaseEffect, int _nEffects, 
		fp32 _AnimBlendDuration, fp32 _AnimBlendDelay, fp32 _AnimTimeOffset, int8 _iAnimTimeOffsetType);
	int ParseConditionReg(const CRegistry* _pConditionReg, int _iAction, CStr _Location);*/

	int32 AddReaction(CXRAG2C_Reaction& _Action);
	//int32 AddCondition(CXRAG2C_Condition& _Condition);

	bool Process();
	/*void BuildNodeTree();
	int BuildNodeTree_Recurse(int _Level, int _Offset, int _Width);
	void OptimizeNodeTree();
	bool ConvertNodeActions();
	bool ConvertNodeAction(uint16& _NodeAction);*/
};

class CXRAG2C_GraphBlock : public CReferenceCount, public CXRAG2C_Common
{
protected:
	class CXR_AnimGraph2Compiler*	m_pCompiler;
	CXRAG2C_Impulse		m_Condition;
	CXRAG2C_MoveToken   m_StartMoveToken;
	CFStr				m_Name;
	/*CFStr				m_StartState;
	int16				m_iStartState;*/
	int32				m_BlockID;
	TArray<spCXRAG2C_State>	m_lspStates;
	TArray<spCXRAG2C_SwitchState> m_lspSwitchStates;
	// Added graphblock constants
	TArray<CXRAG2C_StateConstant>		m_lConstants;
	// State like structure
	CXRAG2C_Reactions	m_Reactions;
	friend class CXRAG2C_State;
	friend class CXRAG2C_State_Base;
	friend class CXRAG2C_SwitchState;
	friend class CXR_AnimGraph2Compiler;
	friend class CXRAG2C_Reactions;
public:

	bool PreProcess();
	bool ProcessPass1();
	bool ProcessPostPass1();
	bool ProcessPrePass2();
	bool ProcessPass2();

	bool ParseReg(CXR_AnimGraph2Compiler* _pCompiler, const CRegistry* _pStateReg);
	bool ParseGraphBlockConstantsReg(const CRegistry* _pStateConstantsReg);

	bool AcquireTargetState(CXRAG2C_MoveToken& _MoveToken);

	bool IncludeStatesFile(CStr _FileName);
	bool ParseStatesReg(const CRegistry* _pStatesReg);
	bool ParseSwitchStatesReg(const CRegistry* _pStatesReg);
	bool ParseGeneratedStateReg(const CRegistry* _pStateReg);
};

typedef TPtr<CXRAG2C_GraphBlock> spCXRAG2C_GraphBlock;

//--------------------------------------------------------------------------------

class CXR_AnimGraph2Compiler : public CXRAG2C_Common
{
	private:

		TArray<CXRAG2C_PropertyCondition>		m_lPropertiesCondition;
		TArray<CXRAG2C_PropertyFunction>		m_lPropertiesFunction;
		TArray<CXRAG2C_PropertyFloat>			m_lPropertiesFloat;
		TArray<CXRAG2C_PropertyInt>				m_lPropertiesInt;
		TArray<CXRAG2C_PropertyBool>			m_lPropertiesBool;
		TArray<CXRAG2C_Operator>				m_lOperators;
		TArray<CXRAG2C_Effect>					m_lEffects;
		TArray<CXRAG2C_StateConstant>			m_lStateConstants;
		TArray<CXRAG2C_StateFlag>				m_lStateFlags;
		TArray<CXRAG2C_AnimFlag>				m_lAnimFlags;
		TArray<CXRAG2C_Animation>				m_lAnimations;
		CXRAG2C_AnimContainerNames				m_ContainerNames;
		
		TArray<CXRAG2C_Constant_Float>			m_lFloatConstants;
		TArray<CXRAG2C_Constant_Int>			m_lIntConstants;
		TArray<CXRAG2C_Constant_Bool>			m_lBoolConstants;

		TArray<CXRAG2C_ImpulseConstant>			m_lImpulseValues;
		TArray<CXRAG2C_ImpulseConstant>			m_lImpulseTypes;
		TArray<CXRAG2C_MoveToken>				m_lMoveTokens;
		TArray<CFStr>							m_lActionHashNames;

		CFStr									m_Name;
		CFStr									m_StartBlock;
		int16									m_iStartBlock;
		int32									m_MaxPropertyIDCondition;
		int32									m_MaxPropertyIDFloat;
		int32									m_MaxPropertyIDInt;
		int32									m_MaxPropertyIDBool;
		//TArray<CXRAG2C_State*>	m_lpStates;
		TArray<spCXRAG2C_GraphBlock>	m_lspGraphBlocks;

		CXR_AnimGraph2*							m_pAnimGraph;

		friend class CXRAG2C_GraphBlock;
		friend class CXRAG2C_State;
		friend class CXRAG2C_SwitchState;
		friend class CXRAG2C_State_Base;
		friend class CXRAG2C_Reactions;
		friend class CXRAG2C_ConstExprParserFloat;
		friend class CXRAG2C_ConstExprParserInt;
		friend class CXRAG2C_ConstExprParserBool;
		friend class CXRAG2C_FileNameScope;

	public:

		bool ParseReg(const CRegistry* _pRootReg, CXR_AnimGraph2* _pAnimGraph, CStr _FileName, CStr& _DestFileName);

	private:

		int32 GetPropertyConditionIndexByID(int32 _ID);
		int32 GetPropertyFunctionIndexByID(int32 _ID);
		int32 GetPropertyFloatIndexByID(int32 _ID);
		int32 GetPropertyIntIndexByID(int32 _ID);
		int32 GetPropertyBoolIndexByID(int32 _ID);
		int32 GetOperatorIndexByID(int32 _ID);
		int32 GetEffectIndexByID(int32 _ID);
		int32 GetStateConstantIndexByID(int32 _ID);

		int32 GetPropertyConditionID(const char* _Name);
		int32 GetPropertyFunctionID(const char* _Name);
		int32 GetPropertyFloatID(const char* _Name);
		int32 GetPropertyIntID(const char* _Name);
		int32 GetPropertyBoolID(const char* _Name);
		int32 GetAnimationIndex(const char* _Name);
		bool GetPropertyID(const char* _Name, int32& _ID, int32& _PropertyType);
		int32 GetOperatorID(const char* _Name);
		int32 GetEffectID(const char* _Name);
		int32 GetStateConstantID(const char* _Name);
		int32 GetGraphBlockIndex(const char* _Name);
		bool GetImpulseValue(const char* _Name, int16& _Value);
		bool GetImpulseType(const char* _Name, int16& _Value);
		bool ResolveImpulseValue(CStr _ConstantStr, int16& _ConstantValue, CStr _Location);
		bool ResolveImpulseType(CStr _ConstantStr, int16& _ConstantValue, CStr _Location);
		
		bool GetConstantValueFloat(const char* _Name, fp32& _Value);
		bool GetConstantValueInt(const char* _Name, int& _Value);
		bool GetConstantValueBool(const char* _Name, bool& _Value);
		bool EvalConstantExpressionFloat(CStr _ExprStr, fp32& _ConstantValue, CXRAG2C_State* _pState, CStr _Location);
		bool EvalConstantExpressionInt(CStr _ExprStr, int& _ConstantValue, CXRAG2C_State* _pState, CStr _Location);
		bool EvalConstantExpressionBool(CStr _ExprStr, bool& _ConstantValue, CXRAG2C_State* _pState, CStr _Location);
		bool EvalConstantExpressionByType(CStr _ExprStr, int _Type, fp32& _ConstantValueFloat, int& _ConstantValueInt, bool& _ConstantValueBool, CXRAG2C_State* _pState, CStr _Location);
		bool ResolveConstantValueFloat(CStr _ConstantStr, fp32& _ConstantValue, CXRAG2C_State* _pState, CStr _Location);
		bool ResolveConstantValueInt(CStr _ConstantStr, int& _ConstantValue, CXRAG2C_State* _pState, CStr _Location);
		bool ResolveConstantValueBool(CStr _ConstantStr, bool& _ConstantValue, CXRAG2C_State* _pState, CStr _Location);

		bool GetAnimIndex(const CStr& _Name, int16& _iAnim, CXRAG2C_State* _pState, CStr _Location);

		
		bool GetStateFlag(const char* _Name, uint32& _Value);
		bool ParseStateFlags(CStr _FlagsStr, uint32& _Flags, uint32& _FlagsMask, CStr _Location);
		bool GetAnimFlag(const char* _Name, uint32& _Value);
		bool ParseAnimFlags(CStr _FlagsStr, uint32& _Flags, uint32& _FlagsMask, CStr _Location);

		bool ParseAnimationsReg(const CRegistry* _pAnimReg);

		bool ParseParamsStr(CStr _ParamsStr, TArray<int>& _lParams, CStr _Location);

		bool IncludeGraphBlockFile(CStr _FileName);
		bool IncludeDeclarationsFile(CStr _FileName);

		bool ParseGraphBlockReg(const CRegistry* _pBlockReg);
		bool ParsePropertiesConditionReg(const CRegistry* _pPropReg);
		bool ParsePropertiesFunctionReg(const CRegistry* _pPropReg);
		bool ParsePropertiesFloatReg(const CRegistry* _pPropReg);
		bool ParsePropertiesIntReg(const CRegistry* _pPropReg);
		bool ParsePropertiesBoolReg(const CRegistry* _pPropReg);
		bool ParseDeclarationsReg(const CRegistry* _pDeclReg);
		bool ParseGraphBlocksReg(const CRegistry* _pDeclReg);
		bool ParseOperatorsReg(const CRegistry* _pOpReg);
		bool ParseEffectsReg(const CRegistry* _pEffectReg);
		bool ParseStateConstantsReg(const CRegistry* _pStateConstantsReg);
		bool ParseFloatConstantsReg(const CRegistry* _pConstantsReg);
		bool ParseIntConstantsReg(const CRegistry* _pConstantsReg);
		bool ParseBoolConstantsReg(const CRegistry* _pConstantsReg);
		bool ParseStateFlagsReg(const CRegistry* _pStateFlagsReg);
		bool ParseAnimFlagsReg(const CRegistry* _pStateFlagsReg);
		bool ParseImpulseValueReg(const CRegistry* _pImpulseReg);
		bool ParseImpulseTypeReg(const CRegistry* _pImpulseReg);

		int32 AddAnimationName(CStr _Name, CXR_AnimGraph2* _pAnimGraph);
		bool Process();
		bool Convert();

		void DumpLog(CStr _LogFileName);
		void DumpLog_GraphBlock(CCFile* _pFile, int _iBlock);
		void DumpLog_State(CCFile* _pFile, int _iBlock, int _iState);
		void DumpLog_Node(CCFile* _pFile, int _iBaseAction, int _iBaseNode, int _iStatePropertyParamsBase, int _iNode, CStr _Indent, CStr _BranchSymbol);
		void DumpLog_NodeAction(CCFile* _pFile, int _iBaseAction, int _NodeAction, int _iBaseNode, int _iStatePropertyParamsBase, int _iNode, CStr _Indent, CStr _BranchSymbol);
		CStr DumpLog_GetNodeStr(int _iBaseAction, int _iBaseNode, int _iStatePropertyParamsBase, int _iNode);
		CStr DumpLog_GetConditionStrFloat(int _PropertyID, int _OperatorID, fp32 _Constant);
		CStr DumpLog_GetConditionStrInt(int _PropertyID, int _OperatorID, int _Constant);
		CStr DumpLog_GetConditionStrBool(int _PropertyID, int _OperatorID, int _Constant);
		CStr DumpLog_GetNodeActionStr(int _iBaseAction, int _iBaseNode, int _NodeAction);

};

//--------------------------------------------------------------------------------

#define FILENAMESCOPE2(FileName)		CXRAG2C_FileNameScope FileNameScope(this, FileName)
#define FILENAMESCOPE2GB(FileName)		CXRAG2C_FileNameScope FileNameScope(m_pCompiler, FileName)

//--------------------------------------------------------------------------------

class CXRAG2C_FileNameScope
{
	private:

		CXR_AnimGraph2Compiler*	m_pCompiler;

	public:

		CXRAG2C_FileNameScope(CXR_AnimGraph2Compiler* _pCompiler, CStr _FileName)
		{
			m_pCompiler = _pCompiler;
			m_pCompiler->PushFileName(_FileName);
		}

		~CXRAG2C_FileNameScope()
		{
			m_pCompiler->PopFileName();
		}
};

//--------------------------------------------------------------------------------
class CXRAG2C_ConstExprParserBase
{
protected:
	char NextChar();
	char ReadChar();
	void SkipWhitespace();
	bool IsDigit(char c);
	bool IsAlpha(char c);
	void OnError(CStr _Msg);

	CStr					m_Source;

	CStr					m_Location;
	CXR_AnimGraph2Compiler*	m_pCompiler;
	CXRAG2C_State*			m_pState;

};

class CXRAG2C_ConstExprParserFloat : public CXRAG2C_ConstExprParserBase
{
	
	private:
		bool ParseConstant(fp32& _Result);
		bool ParseIdentifier(fp32& _Result);
		bool ParseParantheses(fp32& _Result);
		bool ParseTerm(fp32& _Result);
		bool ParseMulDivExpr(fp32& _Result, bool _bShiftResult);
		bool ParseAddSubExpr(fp32& _Result, bool _bShiftResult);

	public:

		CXRAG2C_ConstExprParserFloat(CStr _Source, CXR_AnimGraph2Compiler* _pCompiler, CXRAG2C_State* _pState, CStr _Location)
		{
			m_Source = _Source;
			m_pCompiler = _pCompiler;
			m_pState = _pState;
			m_Location = _Location;
		}

		bool ParseExpr(fp32& _Result);
};

class CXRAG2C_ConstExprParserInt : public CXRAG2C_ConstExprParserBase
{
private:

	bool ParseConstant(int& _Result);
	bool ParseIdentifier(int& _Result);
	bool ParseParantheses(int& _Result);
	bool ParseTerm(int& _Result);
	bool ParseMulDivExpr(int& _Result, bool _bShiftResult);
	bool ParseAddSubExpr(int& _Result, bool _bShiftResult);

public:

	CXRAG2C_ConstExprParserInt(CStr _Source, CXR_AnimGraph2Compiler* _pCompiler, CXRAG2C_State* _pState, CStr _Location)
	{
		m_Source = _Source;
		m_pCompiler = _pCompiler;
		m_pState = _pState;
		m_Location = _Location;
	}

	bool ParseExpr(int& _Result);
};

class CXRAG2C_ConstExprParserBool : public CXRAG2C_ConstExprParserBase
{
private:

	bool ParseConstant(bool& _Result);
	bool ParseIdentifier(bool& _Result);
	bool ParseParantheses(bool& _Result);
	bool ParseTerm(bool& _Result);

public:

	CXRAG2C_ConstExprParserBool(CStr _Source, CXR_AnimGraph2Compiler* _pCompiler, CXRAG2C_State* _pState, CStr _Location)
	{
		m_Source = _Source;
		m_pCompiler = _pCompiler;
		m_pState = _pState;
		m_Location = _Location;
	}

	bool ParseExpr(bool& _Result);
};

//--------------------------------------------------------------------------------

#endif /* AnimGraphCompiler_h */
