#ifndef AnimGraphCompiler_h
#define AnimGraphCompiler_h

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// !!!NOTE!!! TODO: Ta bort den här skiten i RTM. Behöver inte finnas.
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

#include "../AnimGraph.h"
#ifndef TARGET_XBOX
#include "../../../MCC/Mda.h"
#include "../../../MCC/Mrtc.h"
#endif

//--------------------------------------------------------------------------------

#define AGC_NODEACTION_BITS			18

#define AGC_NODEACTION_TYPEMASK		0xC000
#define AGC_NODEACTION_ACTIONMASK	0x0FFF
#define AGC_NODEACTION_NODEMASK		0x0FFF

#define AGC_NODEACTION_ENDPARSE		0x8000
#define AGC_NODEACTION_PARSENODE	0x4000
#define AGC_NODEACTION_FAILPARSE	(AGC_NODEACTION_ENDPARSE | ((-1) & AGC_NODEACTION_ACTIONMASK))

#define AGC_INHERITFLAGS_FLAGS0					0x0001
#define AGC_INHERITFLAGS_FLAGS1					0x0002
#define AGC_INHERITFLAGS_PRIORITY				0x0004
#define AGC_INHERITFLAGS_ANIMINDEX				0x0008
#define AGC_INHERITFLAGS_ANIMBASEJOINT			0x0010
#define AGC_INHERITFLAGS_ANIMTIMEOFFSETTYPE		0x0020
#define AGC_INHERITFLAGS_ANIMTIMEOFFSET			0x0040
#define AGC_INHERITFLAGS_ANIMTIMESCALE			0x0080
#define AGC_INHERITFLAGS_ANIMOPACITY			0x0100
#define AGC_INHERITFLAGS_ALL					-1

#define AGC_DISABLEDWARNING_MISSINGANIMATIONINDEX	0x01
#define AGC_DISABLEDWARNING_MISSINGACTIONS			0x02

//--------------------------------------------------------------------------------

const char* ms_lpAGCAnimTimeOffsetTypes[];

//--------------------------------------------------------------------------------

class CXRAGC_Function
{
	public:

		CFStr	m_Name;
		int32	m_ID;

		CXRAGC_Function()
		{
			m_ID = -1;
		}

		CXRAGC_Function(const char* _Name, int32 _ID)
		{
			m_Name = _Name;
			m_ID = _ID;
		}
};

typedef CXRAGC_Function CXRAGC_Property;
typedef CXRAGC_Function CXRAGC_Operator;
typedef CXRAGC_Function CXRAGC_Effect;

//--------------------------------------------------------------------------------

class CXRAGC_StateConstant
{
	public:

		CFStr	m_Name;
		int32	m_ID;
		fp32		m_Value;

		CXRAGC_StateConstant()
		{
			m_ID = -1;
			m_Value = 0;
		}

		CXRAGC_StateConstant(const char* _Name, int32 _ID, fp32 _Value = 0)
		{
			m_Name = _Name;
			m_ID = _ID;
			m_Value = _Value;
		}
};

//--------------------------------------------------------------------------------

class CXRAGC_Constant
{
	public:

		CFStr	m_Name;
		fp32		m_Value;

		CXRAGC_Constant()
		{
			m_Value = 0;
		}

		CXRAGC_Constant(const char* _Name, fp32 _Value)
		{
			m_Name = _Name;
			m_Value = _Value;
		}
};

//--------------------------------------------------------------------------------

class CXRAGC_StateFlag
{
	public:

		CFStr	m_Name;
		uint32	m_Value;
		
		CXRAGC_StateFlag()
		{
			m_Value = 0;
		}

		CXRAGC_StateFlag(const char* _Name, uint32 _Value)
		{
			m_Name = _Name;
			m_Value = _Value;
		}
};

//--------------------------------------------------------------------------------

class CXRAGC_StateAnim
{
	public:

		int16		m_iAnim;
		uint8		m_iBaseJoint;
//		uint8		m_TimeOffsetType;
		fp32			m_TimeOffset;
		fp32			m_TimeScale;
		fp32			m_Opacity;
		int8		m_iLoopControlAnimLayer;

		CXRAGC_StateAnim()
		{
			m_iAnim = -1;
			m_iBaseJoint = 0;
//			m_TimeOffsetType = 0;
			m_TimeOffset = 0;
			m_TimeScale = 1;
			m_Opacity = 1;
			m_iLoopControlAnimLayer = 0;
		}
};

//--------------------------------------------------------------------------------

class CXRAGC_Action
{
	public:

		CFStr			m_Name;
		bool			m_bExported;
		int8			m_TokenID;

		CFStr			m_TargetState;
		int16			m_iTargetState;

		fp32				m_AnimBlendDuration;
		fp32				m_AnimBlendDelay;
		fp32				m_AnimTimeOffset;
		int8			m_iAnimTimeOffsetType;

		int32			m_iBaseEffectInstance;
		int				m_nEffectInstances;

		TArray<int32>	m_lConditionMap;
		uint32			m_ConditionBits; // (Built during State::Process())

		void operator=(CXRAGC_Action& _Action)
		{
			CopyFrom(_Action);
		}

		// Needed to safely duplicate m_lConditionMap. Otherwise it'll be a pointer only copy.
		void CopyFrom(CXRAGC_Action& _Action)
		{
			m_Name = _Action.m_Name;
			m_bExported = _Action.m_bExported;
			m_TokenID = _Action.m_TokenID;

			m_TargetState = _Action.m_TargetState;
			m_iTargetState = _Action.m_iTargetState;

			m_AnimBlendDuration = _Action.m_AnimBlendDuration;
			m_AnimBlendDelay = _Action.m_AnimBlendDelay;
			m_AnimTimeOffset = _Action.m_AnimTimeOffset;
			m_iAnimTimeOffsetType = _Action.m_iAnimTimeOffsetType;

			m_iBaseEffectInstance = _Action.m_iBaseEffectInstance;
			m_nEffectInstances = _Action.m_nEffectInstances;

			_Action.m_lConditionMap.Duplicate(&m_lConditionMap);
			m_ConditionBits = _Action.m_ConditionBits;
		}

};

//--------------------------------------------------------------------------------

class CXRAGC_EffectInstance
{
public:

	CAGEffectID		m_ID;
	TArray<fp32>		m_lParams;

	CXRAGC_EffectInstance()
	{
		m_ID = AG_EFFECTID_NULL;
	}

	CXRAGC_EffectInstance(CAGEffectID _ID)
	{
		m_ID = _ID;
	}

	CXRAGC_EffectInstance(const CXRAGC_EffectInstance& _EffectInstance)
	{
		m_ID = _EffectInstance.m_ID;
		_EffectInstance.m_lParams.Duplicate(&m_lParams);
	}

};

//--------------------------------------------------------------------------------

class CXRAGC_ConditionNode
{
	public:

		CAGOperatorID	m_iOperator;
		CAGPropertyID	m_iProperty;
		TArray<fp32>		m_lPropertyParams;
		fp32				m_Constant;
		uint16			m_TrueAction;
		uint16			m_FalseAction;

	public:

		CXRAGC_ConditionNode()
		{
			m_iOperator = AG_OPERATOR_NULL;
			m_iProperty = AG_PROPERTY_NULL;
			m_Constant = 0;
			m_TrueAction = 0;
			m_FalseAction = 0;
		}

		CXRAGC_ConditionNode(const CXRAGC_ConditionNode& _CNode)
		{
			m_iOperator = _CNode.m_iOperator;
			m_iProperty = _CNode.m_iProperty;
			_CNode.m_lPropertyParams.Duplicate(&m_lPropertyParams);
			m_Constant = _CNode.m_Constant;
			m_TrueAction = _CNode.m_TrueAction;
			m_FalseAction = _CNode.m_FalseAction;
		}

		bool operator==(CXRAGC_ConditionNode& _Node)
		{
			if (m_lPropertyParams.Len() != _Node.m_lPropertyParams.Len())
				return false;

			for (int iParam = 0; iParam < m_lPropertyParams.Len(); iParam++)
				if (m_lPropertyParams[iParam] != _Node.m_lPropertyParams[iParam])
					return false;

			return ((m_iOperator == _Node.m_iOperator) &&
					(m_iProperty == _Node.m_iProperty) &&
					(m_Constant == _Node.m_Constant) &&
					(m_TrueAction == _Node.m_TrueAction) &&
					(m_FalseAction == _Node.m_FalseAction));
		}

};

//--------------------------------------------------------------------------------

class CXRAGC_Condition
{
	public:

		CAGPropertyID	m_iProperty;
		CAGOperatorID	m_iOperator;
		CStr			m_ConstantStr;
		fp32				m_ConstantValue;
		TArray<fp32>		m_lPropertyParams;

		uint8			m_Occurrences;
		int				m_iFirstAction;
		uint16			m_SortedIndex; // Includes SortedBit (Built during State::Process())

		bool operator==(CXRAGC_Condition& _Condition)
		{
			if (m_lPropertyParams.Len() != _Condition.m_lPropertyParams.Len())
				return false;

			for (int iParam = 0; iParam < m_lPropertyParams.Len(); iParam++)
				if (m_lPropertyParams[iParam] != _Condition.m_lPropertyParams[iParam])
					return false;

			return ((m_iProperty == _Condition.m_iProperty) && 
					(m_iOperator == _Condition.m_iOperator) &&
//					(m_ConstantValue == _Condition.m_ConstantValue)); // FIXME: Will reuse conditions better, but is unfortunately not available at that time?
					(m_ConstantStr == _Condition.m_ConstantStr));
		}

		CXRAGC_Condition()
		{
			m_iProperty = 0;
			m_iOperator = 0;
			m_ConstantValue = 0;
			m_Occurrences = 0;
			m_iFirstAction = AG_ACTIONINDEX_NULL;
			m_SortedIndex = 0;
		}

};

//--------------------------------------------------------------------------------

class CXRAGC_State
{
	public:

		CFStr								m_Name;
		uint32								m_Flags[2], m_FlagsMask[2];
		uint8								m_Priority;

		int									m_References;
		int									m_Index;

		TArray<CXRAGC_StateAnim>			m_lStateAnims;

		TArray<CXRAGC_Action>				m_lActions;
		TArray<CXRAGC_EffectInstance>		m_lEffectInstances;
		TArray<CXRAGC_ConditionNode>		m_lConditionNodes;
		TArray<CXRAGC_StateConstant>		m_lConstants;
		TArray<fp32>							m_CallbackParams;

		TArray<CFStr>						m_lSuperStates;
		uint32								m_InheritFlags;
		bool								m_bExpandedInheritance;
		bool								m_bPendingExpandInheritance;

		uint32								m_DisabledWarnings, m_EnabledWarnings, m_InheritDEWarnings;

		TArray<CXRAGC_Condition>			m_lConditions;

		class CXR_AnimGraphCompiler*		m_pCompiler;

		bool ExpandInheritance();
		bool Inherit(CXRAGC_State& _SuperState);

		bool ParseReg(CXR_AnimGraphCompiler* _pCompiler, const CRegistry* _pStateReg);
		bool ParseInheritReg(const CRegistry* _pInheritReg);
		bool ParseAnimationReg(const CRegistry* _pAnimationReg);
		bool ParseStateConstantsReg(const CRegistry* _pStateConstantsReg);
		bool ParseActionsReg(const CRegistry* _pActionsReg);
		bool ParseActionReg(const CRegistry* _pActionReg);
		bool ParseMoveTokenReg(const CRegistry* _pTargetStateReg, CStr _Location, int8& _TokenID, CStr& _TargetState, fp32& _AnimBlendDuration, fp32& _AnimBlendDelay, fp32& _AnimTimeOffset, int8& _iAnimTimeOffsetType);
		bool ParseActionEffectsReg(const CRegistry* _pEffectsReg, int& _iBaseEffect, int& _nEffects, CStr _Location);
		bool ParseConditionSetReg(const CRegistry* _pActionReg, CStr _Location, CStr _ActionName,
								  bool _bExported, int8 _TokenID, CStr _TargetState, int _iBaseEffect, int _nEffects, 
								  fp32 _AnimBlendDuration, fp32 _AnimBlendDelay, fp32 _AnimTimeOffset, int8 _iAnimTimeOffsetType);
		int ParseConditionReg(const CRegistry* _pConditionReg, int _iAction, CStr _Location);

		int32 AddAction(CXRAGC_Action& _Action);
		int32 AddCondition(CXRAGC_Condition& _Condition);

		bool Process();
		void BuildNodeTree();
		int BuildNodeTree_Recurse(int _Level, int _Offset, int _Width);
		void OptimizeNodeTree();
		bool ConvertNodeActions();
		bool ConvertNodeAction(uint16& _NodeAction);

};

//--------------------------------------------------------------------------------

class CXR_AnimGraphCompiler
{
	private:

		TArray<CXRAGC_Property>					m_lProperties;
		TArray<CXRAGC_Operator>					m_lOperators;
		TArray<CXRAGC_Effect>					m_lEffects;
		TArray<CXRAGC_StateConstant>			m_lStateConstants;
		TArray<CXRAGC_StateFlag>				m_lStateFlags;
		TArray<CXRAGC_Constant>					m_lConstants;
		TArray<CFStr>							m_lActionHashNames;

		CFStr									m_Name;
		CFStr									m_StartState;
		int16									m_iStartState;
		TArray<CXRAGC_State*>	m_lpStates;

		TArray<CStr>							m_lFilenames;

		CXR_AnimGraph*							m_pAnimGraph;

		friend class CXRAGC_State;
		friend class CXRAGC_ConstExprParser;
		friend class CXRAGC_FileNameScope;

	public:

		bool ParseReg(const CRegistry* _pRootReg, CXR_AnimGraph* _pAnimGraph, CStr _FileName, CStr& _DestFileName);

	private:

		int32 GetPropertyIndexByID(int32 _ID);
		int32 GetOperatorIndexByID(int32 _ID);
		int32 GetEffectIndexByID(int32 _ID);
		int32 GetStateConstantIndexByID(int32 _ID);

		int32 GetPropertyID(const char* _Name);
		int32 GetOperatorID(const char* _Name);
		int32 GetEffectID(const char* _Name);
		int32 GetStateConstantID(const char* _Name);
		bool GetConstantValue(const char* _Name, fp32& _Value);
		bool EvalConstantExpression(CStr _ExprStr, fp32& _ConstantValue, CXRAGC_State* _pState, CStr _Location);
		bool ResolveConstantValue(CStr _ConstantStr, fp32& _ConstantValue, CXRAGC_State* _pState, CStr _Location);
		bool GetStateFlag(const char* _Name, uint32& _Value);
		bool ParseStateFlags(CStr _FlagsStr, uint32& _Flags, uint32& _FlagsMask, CStr _Location);

		bool ParseParamsStr(CStr _ParamsStr, TArray<fp32>& _lParams, CStr _Location);

		void PushFileName(CStr _Filename);
		void PopFileName();
		CStr GetFileName();
		CStr GetFilePath();

		bool IncludeStatesFile(CStr _FileName);
		bool IncludeDeclarationsFile(CStr _FileName);
		bool ParseStatesReg(const CRegistry* _pStatesReg, bool _bIgnoreStartState);
		bool ParseDeclarationsReg(const CRegistry* _pDeclReg);
		bool ParsePropertiesReg(const CRegistry* _pPropReg);
		bool ParseOperatorsReg(const CRegistry* _pOpReg);
		bool ParseEffectsReg(const CRegistry* _pEffectReg);
		bool ParseStateConstantsReg(const CRegistry* _pStateConstantsReg);
		bool ParseConstantsReg(const CRegistry* _pConstantsReg);
		bool ParseStateFlagsReg(const CRegistry* _pStateFlagsReg);

		bool ParseGeneratedStateReg(const CRegistry* _pStateReg);

		int32 AddAnimationName(CStr _Name, CXR_AnimGraph* _pAnimGraph);
		bool Process();
		bool Convert();

		void DumpLog(CStr _LogFileName);
		void DumpLog_State(CCFile* _pFile, int _iState);
		void DumpLog_Node(CCFile* _pFile, int _iBaseAction, int _iBaseNode, int _iStatePropertyParamsBase, int _iNode, CStr _Indent, CStr _BranchSymbol);
		void DumpLog_NodeAction(CCFile* _pFile, int _iBaseAction, int _NodeAction, int _iBaseNode, int _iStatePropertyParamsBase, int _iNode, CStr _Indent, CStr _BranchSymbol);
		CStr DumpLog_GetNodeStr(int _iBaseAction, int _iBaseNode, int _iStatePropertyParamsBase, int _iNode);
		CStr DumpLog_GetConditionStr(int _PropertyID, CXRAG_ICallbackParams* _pIParams, int _OperatorID, fp32 _Constant);
		CStr DumpLog_GetNodeActionStr(int _iBaseAction, int _iBaseNode, int _NodeAction);

		void OnError(const char* _pMsg);
		void OnWarning(const char* _pMsg);
		void OnNote(const char* _pMsg);
		void OnMsg(const char* _pMsg);

};

//--------------------------------------------------------------------------------

#define FILENAMESCOPE(FileName)		CXRAGC_FileNameScope FileNameScope(this, FileName)

//--------------------------------------------------------------------------------

class CXRAGC_FileNameScope
{
	private:

		CXR_AnimGraphCompiler*	m_pCompiler;

	public:

		CXRAGC_FileNameScope(CXR_AnimGraphCompiler* _pCompiler, CStr _FileName)
		{
			m_pCompiler = _pCompiler;
			m_pCompiler->PushFileName(_FileName);
		}

		~CXRAGC_FileNameScope()
		{
			m_pCompiler->PopFileName();
		}
};

//--------------------------------------------------------------------------------

class CXRAGC_ConstExprParser
{
	private:

		CStr					m_Source;

		CStr					m_Location;
		CXR_AnimGraphCompiler*	m_pCompiler;
		CXRAGC_State*			m_pState;

	private:

		char NextChar();
		char ReadChar();
		void SkipWhitespace();
		bool IsDigit(char c);
		bool IsAlpha(char c);

		bool ParseConstant(fp32& _Result);
		bool ParseIdentifier(fp32& _Result);
		bool ParseParantheses(fp32& _Result);
		bool ParseTerm(fp32& _Result);
		bool ParseMulDivExpr(fp32& _Result, bool _bShiftResult);
		bool ParseAddSubExpr(fp32& _Result, bool _bShiftResult);

		void OnError(CStr _Msg);

	public:

		CXRAGC_ConstExprParser(CStr _Source, CXR_AnimGraphCompiler* _pCompiler, CXRAGC_State* _pState, CStr _Location)
		{
			m_Source = _Source;
			m_pCompiler = _pCompiler;
			m_pState = _pState;
			m_Location = _Location;
		}

		bool ParseExpr(fp32& _Result);
};

//--------------------------------------------------------------------------------

#endif /* AnimGraphCompiler_h */