//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MDA.h"
#include "MFile.h"
#include "AnimGraph2.h"

//--------------------------------------------------------------------------------

#define IndentBlank		("   ")
#define IndentBranch	(" |-")
#define IndentBranched	(" | ")
#define IndentBranchEnd	(" `-")

#ifndef M_RTM

//--------------------------------------------------------------------------------

static CStr FlagsStr(uint32 _Flags)
{
	CStr Result;
	for (int iBit = 0; iBit < 32; iBit++)
	{
		if (((iBit & 7) == 0) && (iBit > 0))
			Result = " " + Result;
		Result = CStrF("%s", ((_Flags & (1 << iBit)) != 0) ? "x" : ".") + Result;
	}
	return Result;
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG2::LogDump(CStr _LogFileName, uint32 _DumpFlags) const
{
#ifndef PLATFORM_CONSOLE
	try
	{
		CStr Indent = IndentBlank;

		CCFile File;
		File.Open(_LogFileName, CFILE_WRITE);

		File.Writeln(CStrF("AnimGraph %s", m_Name.Str()));
		File.Writeln(CStr("{"));
		File.Writeln(CStrF("%snGraphBlocks = %d", Indent.Str(), GetNumGraphBlocks()));
		File.Writeln(CStrF("%snStates = %d", Indent.Str(), GetNumStates()));
		File.Writeln(CStrF("%snActions = %d", Indent.Str(), GetNumActions()));
		File.Writeln(CStrF("%snMoveTokens = %d", Indent.Str(), GetNumMoveTokens()));
		File.Writeln(CStrF("%snMoveAnimGraphs = %d", Indent.Str(), GetNumMoveAnimGraphs()));
		File.Writeln(CStrF("%snAnimLayers = %d", Indent.Str(), GetNumAnimLayers()));
		File.Writeln(CStrF("%snAnimNames = %d", Indent.Str(), GetNumAnimNames()));
		File.Writeln(CStrF("%snNodes = %d", Indent.Str(), GetNumNodes()));
		File.Writeln(CStrF("%snEffectInstances = %d", Indent.Str(), GetNumEffectInstances()));
		File.Writeln(CStrF("%snCallbackParams = %d", Indent.Str(), GetNumCallbackParams()));
		File.Writeln(CStrF("%snStateConstants = %d", Indent.Str(), GetNumStateConstants()));
		int16 iStartBlock = GetMoveToken(0)->GetTargetGraphBlockIndex();
		File.Writeln(CStrF("%sStartBlock: %d <%s>", Indent.Str(), iStartBlock, GetExportedGraphBlockName(iStartBlock).Str()));
		File.Writeln(CStr("}"));
		File.Writeln(CStr(""));

		if (_DumpFlags & AG2_DUMPFLAGS_STATES)
			LogDump_GraphBlock(&File, _DumpFlags, "");

		if (_DumpFlags & AG2_DUMPFLAGS_ACTIONS)
		{
			LogDump_Actions(&File, _DumpFlags, "");
			LogDump_SwitchStateActionVals(&File, _DumpFlags, "");
		}

		if (_DumpFlags & AG2_DUMPFLAGS_ACTIONS)
			LogDump_MoveTokens(&File, _DumpFlags, "");

		if (_DumpFlags & AG2_DUMPFLAGS_ACTIONS)
			LogDump_MoveAnimGraphs(&File, _DumpFlags, "");

		if (_DumpFlags & AG2_DUMPFLAGS_ACTIONS)
			LogDump_Reactions(&File, _DumpFlags, "");

		if (_DumpFlags & AG2_DUMPFLAGS_CONDITIONNODES)
			LogDump_ConditionNodes(&File, _DumpFlags, "");

		if (_DumpFlags & AG2_DUMPFLAGS_ANIMLAYERS)
			LogDump_AnimLayers(&File, _DumpFlags, "");

		if (_DumpFlags & AG2_DUMPFLAGS_ANIMLAYERS)
			LogDump_AnimNames(&File, _DumpFlags, "");

		if (_DumpFlags & AG2_DUMPFLAGS_EFFECTINSTANCES)
			LogDump_EffectInstances(&File, _DumpFlags, "");

		if (_DumpFlags & AG2_DUMPFLAGS_CALLBACKPARAMS)
			LogDump_CallbackParams(&File, _DumpFlags, "");

		if (_DumpFlags & AG2_DUMPFLAGS_EXPORTEDACTIONS)
			LogDump_ExportedActions(&File, _DumpFlags, "");

		if (_DumpFlags & AG2_DUMPFLAGS_EXPORTEDSTATES)
		{
			LogDump_ExportedStates(&File, _DumpFlags, "");
			LogDump_ExportedSwitchStates(&File, _DumpFlags, "");
		}

		if (_DumpFlags & AG2_DUMPFLAGS_EXPORTEDPROPERTIES)
		{
			LogDump_ExportedPropertiesCondition(&File, _DumpFlags, "");
			LogDump_ExportedPropertiesFunction(&File, _DumpFlags, "");
			LogDump_ExportedPropertiesFloat(&File, _DumpFlags, "");
			LogDump_ExportedPropertiesInt(&File, _DumpFlags, "");
			LogDump_ExportedPropertiesBool(&File, _DumpFlags, "");
		}

		if (_DumpFlags & AG2_DUMPFLAGS_EXPORTEDOPERATORS)
			LogDump_ExportedOperators(&File, _DumpFlags, "");

		if (_DumpFlags & AG2_DUMPFLAGS_EXPORTEDEFFECTS)
			LogDump_ExportedEffects(&File, _DumpFlags, "");

		if (_DumpFlags & AG2_DUMPFLAGS_EXPORTEDSTATECONSTANTS)
			LogDump_ExportedStateConstants(&File, _DumpFlags, "");

		if (_DumpFlags & AG2_DUMPFLAGS_EXPORTEDIMPULSETYPES)
		{
			LogDump_ExportedImpulseTypes(&File, _DumpFlags, "");
			LogDump_ExportedImpulseValues(&File, _DumpFlags, "");
		}
	}
	catch (...)
	{
		ConOutL(CStrF("ERROR: Error while dumping AnimGraph '%s' to '%s'.", m_Name.Str(), _LogFileName.Str()));
	}
#endif
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
void CXRAG2::LogDump_GraphBlock(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_GraphBlock, MAUTOSTRIP_VOID );
	_pFile->Writeln(CStrF("%sGraphBlocks:", _Indent.Str()));
	_pFile->Writeln(CStrF("%s{", _Indent.Str()));

	CStr Indent = _Indent + IndentBlank;
	CStr IndentState = Indent + IndentBlank;

	int nBlocks = GetNumGraphBlocks();
	for (int iBlock = 0; iBlock < nBlocks; iBlock++)
	{
		const CXRAG2_GraphBlock* pBlock = GetGraphBlock(iBlock);
		_pFile->Writeln(CStrF("%sGraphBlock(%d): %s", Indent.Str(), iBlock,m_lExportedGraphBlockNames[iBlock].Str()));
		_pFile->Writeln(CStrF("%s{", Indent.Str()));
		_pFile->Writeln(CStrF("%sNumStates: %d", IndentState.Str(), pBlock->GetNumStates()));
		_pFile->Writeln(CStrF("%sStartMoveToken: %d", IndentState.Str(), pBlock->GetStartMoveTokenIndex()));
		const CXRAG2_MoveToken* pMoveToken = GetMoveToken(pBlock->GetStartMoveTokenIndex());
		_pFile->Writeln(CStrF("%sStartState: %d <%s>", IndentState.Str(), pMoveToken->GetTargetStateIndex(), pMoveToken->GetTargetStateType() ? GetExportedSwitchStateName(pMoveToken->GetTargetStateIndex()).Str() : GetExportedStateName(pMoveToken->GetTargetStateIndex()).Str()));
		{
			CStr IndentCondition = IndentState + IndentBlank;
			_pFile->Writeln(CStrF("%sConditions:", IndentState.Str()));
			_pFile->Writeln(CStrF("%s{", IndentState.Str()));
			_pFile->Writeln(CStrF("%sImpulseType: %d", IndentCondition.Str(), pBlock->GetCondition().m_ImpulseType));
			_pFile->Writeln(CStrF("%sImpulseValue: %d", IndentCondition.Str(), pBlock->GetCondition().m_ImpulseValue));
			_pFile->Writeln(CStrF("%s}", IndentState.Str()));
		}

		if (_DumpFlags & AG2_DUMPFLAGS_STATE_STATECONSTANTS)
		{
			if (GetNumStateConstants() > 0)
			{
				_pFile->Writeln(CStrF("%sGraphBlockConstants:", Indent.Str()));
				CStr Indent2 = Indent + IndentBlank;
				for (int16 jStateConstant = 0; jStateConstant < pBlock->GetNumStateConstants(); jStateConstant++)
				{
					int16 iStateConstant = pBlock->GetBaseStateConstantIndex() + jStateConstant;
					const CXRAG2_StateConstant* pStateConstant = GetStateConstant(iStateConstant);
					CStr StateConstantName = GetExportedStateConstantNameFromID(pStateConstant->m_ID);
					if (StateConstantName == "")
						_pFile->Writeln(CStrF("%s[C%03d]  <ID %d, Value %3.3f>", Indent2.Str(), iStateConstant, pStateConstant->m_ID, pStateConstant->m_Value));
					else
						_pFile->Writeln(CStrF("%s[C%03d](ID %d)  %s = %3.3f", Indent2.Str(), iStateConstant, pStateConstant->m_ID, StateConstantName.Str(), pStateConstant->m_Value));
				}
			}
			else
			{
				_pFile->Writeln(CStrF("%sStateConstants: (empty)", Indent.Str()));
			}
			_pFile->Writeln("");
		}
		LogDump_Structure_Reactions(_pFile, _DumpFlags, IndentState,pBlock);
		_pFile->Writeln("");

		LogDump_Structure(_pFile, _DumpFlags, IndentState, pBlock);
		_pFile->Writeln(CStrF("%s}", Indent.Str()));
	}

	//_pFile->Writeln(CStrF("%s} /* States */", _Indent.Str()));
	_pFile->Writeln(CStrF("%s}", _Indent.Str()));
	_pFile->Writeln("");
}

void CXRAG2::LogDump_Structure_Reactions(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, const CXRAG2_GraphBlock* _pBlock) const
{
	MAUTOSTRIP( CXRAG2_LogDump_Structure, MAUTOSTRIP_VOID );
	_pFile->Writeln(CStrF("%sReactions:", _Indent.Str()));
	_pFile->Writeln(CStrF("%s{", _Indent.Str()));

	CStr Indent = _Indent + IndentBlank;

	int nReactions = _pBlock->GetNumReactions();
	for (int iReaction = _pBlock->GetBaseReactionIndex(); iReaction < _pBlock->GetBaseReactionIndex() + nReactions; iReaction++)
	{
		const CXRAG2_Reaction* pReaction = GetReaction(iReaction);
		//_pFile->Writeln(CStrF("%sI'm a reaction, short and stout}", Indent.Str()));
		CStr ImpulseStr = CStrF("ImpulseType: %d ImpulseValue: %d ", pReaction->m_Impulse.m_ImpulseType, pReaction->m_Impulse.m_ImpulseValue);
		int32 nMoveTokens = pReaction->GetNumMoveTokens();
		int32 nMoveAnimgraphs = pReaction->GetNumMoveAnimGraphs();
//		int32 nEffects = pReaction->GetNumEffectInstances();
		CStr ReactionStr;
		for (int32 i = 0; i < nMoveTokens; i++)
		{
			int32 iMoveToken = i + pReaction->GetBaseMoveTokenIndex();
			const CXRAG2_MoveToken* pMoveToken = GetMoveToken(iMoveToken);

			CStr TokenIDStr = ((pMoveToken->GetTokenID() != AG2_TOKENID_NULL) ? CStrF("%d", pMoveToken->GetTokenID()) : CStr("CUR"));
			int16 iTargetState = pMoveToken->GetTargetStateIndex();
			CStr TargetStateStr;
			if (iTargetState == AG2_STATEINDEX_NULL)
				TargetStateStr = " (no move)";
			else if (iTargetState == AG2_STATEINDEX_TERMINATE)
				TargetStateStr = " -> TERMINATE";
			else if (iTargetState == AG2_STATEINDEX_STARTAG)
				TargetStateStr = " -> STARTAG";
			else
			{
				CStr TargetStateName;
				if (!pMoveToken->GetTargetStateType())
					TargetStateName = CStrF("State%d (%s(%f))", iTargetState, GetExportedStateName(iTargetState).Str(),pMoveToken->GetAnimBlendDuration());
				else
					TargetStateName = CStrF("SwitchState%d (%s(%f))", iTargetState, GetExportedSwitchStateName(iTargetState).Str(),pMoveToken->GetAnimBlendDuration());

				//TargetStateStr = GetExportedStateName(iTargetState).Str();
				TargetStateStr = CStrF(" -> State%d (%s)", iTargetState, TargetStateName.Str());
			}

			int16 iTargetGraphBlock = pMoveToken->GetTargetGraphBlockIndex();
			CStr TargetGraphBlockStr;
			if (iTargetGraphBlock == AG2_GRAPHBLOCKINDEX_NULL)
				TargetGraphBlockStr = " (no move)";
			else if (iTargetState == AG2_STATEINDEX_TERMINATE)
				TargetGraphBlockStr = " -> TERMINATE";
			else if (iTargetState == AG2_STATEINDEX_STARTAG)
				TargetGraphBlockStr = " -> STARTAG";
			else
			{
				TargetGraphBlockStr = GetExportedGraphBlockName(iTargetGraphBlock).Str();
				TargetGraphBlockStr = CStrF(" -> GraphBlock%d (%s)", iTargetGraphBlock, TargetGraphBlockStr.Str());
			}

			ReactionStr += CStrF("Token%s%s%s, ABDuration %3.3f ", TokenIDStr.Str(), TargetGraphBlockStr.Str(), TargetStateStr.Str(), pMoveToken->GetAnimBlendDuration());
		}
		for (int32 i = 0; i < nMoveAnimgraphs; i++)
		{
			int32 iMoveAnimGraph = pReaction->GetBaseMoveAnimgraphIndex() + i;
			const CXRAG2_MoveAnimGraph* pMoveAnimGraph = GetMoveAnimGraph(iMoveAnimGraph);
			CStr TokenIDStr = ((pMoveAnimGraph->GetTokenID() != AG2_TOKENID_NULL) ? CStrF("%d", pMoveAnimGraph->GetTokenID()) : CStr("CUR"));

			CStr TargetAnimGraphStr  = GetExportedAnimGraphName(i).Str();

			ReactionStr += CStrF("MoveAnimGraph[M%03d] Token%s -> %s, ABDuration %3.3f ", i,TokenIDStr.Str(), TargetAnimGraphStr.Str(), pMoveAnimGraph->GetAnimBlendDuration());
		}

		if (!nMoveTokens && !nMoveAnimgraphs)
			ReactionStr = CStr("<EMPTY>");
		for (int32 i = 0; i < pReaction->GetNumEffectInstances(); i++)
		{
			int32 iEffect = pReaction->GetBaseEffectInstanceIndex() + i;
			const CXRAG2_EffectInstance* pEffect = GetEffectInstance(iEffect);
			const CXRAG2_ICallbackParams& Params = GetICallbackParams(pEffect->m_iParams,pEffect->m_nParams);
			CStr ParamStr;
			for (int32 j = 0; j < Params.GetNumParams(); j++)
				ParamStr += CStrF("%d",Params.GetParam(i));
			CStr EffectName = GetExportedEffectNameFromID(pEffect->m_ID);
			ReactionStr += CStrF("Effect[E%03d] %s(%s)",pEffect->m_ID,EffectName.Str(),ParamStr.Str());
		}
		_pFile->Writeln(CStrF("%sReaction[A%03d] %s = <%s>", Indent.Str(), iReaction, ImpulseStr.Str(), ReactionStr.Str()));
	}

	//_pFile->Writeln(CStrF("%s} /* States */", _Indent.Str()));
	_pFile->Writeln(CStrF("%s}", _Indent.Str()));
}

void CXRAG2::LogDump_Structure(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, const CXRAG2_GraphBlock* _pBlock) const
{
	MAUTOSTRIP( CXRAG2_LogDump_Structure, MAUTOSTRIP_VOID );
	_pFile->Writeln(CStrF("%sStates:", _Indent.Str()));
	_pFile->Writeln(CStrF("%s{", _Indent.Str()));

	CStr Indent = _Indent + IndentBlank;
	
	int nStates = _pBlock->GetNumStates();
	for (int iState = 0; iState < nStates; iState++)
	{
		int iAGState = _pBlock->GetBaseStateIndex() + iState;
		const CXRAG2_State* pState = GetState(iAGState);
		LogDump_Structure_State(_pFile, _DumpFlags, Indent, iAGState, pState);

		if (iState < (nStates - 1))
		{
			_pFile->Writeln("");
			_pFile->Writeln(CStrF("%s- - - - - - - - - - - - - - - - - - - -", Indent.Str()));
			_pFile->Writeln("");
		}
	}

	//_pFile->Writeln(CStrF("%s} /* States */", _Indent.Str()));
	_pFile->Writeln(CStrF("%s}", _Indent.Str()));

	////////////////

	_pFile->Writeln(CStrF("%sSwitchStates:", _Indent.Str()));
	_pFile->Writeln(CStrF("%s{", _Indent.Str()));

	nStates = _pBlock->GetNumSwitchStates();
	for (int iState = 0; iState < nStates; iState++)
	{
		int iAGState = _pBlock->GetBaseSwitchStateIndex() + iState;
		const CXRAG2_SwitchState* pState = GetSwitchState(iAGState);
		LogDump_Structure_SwitchState(_pFile, _DumpFlags, Indent, iAGState, pState);

		if (iState < (nStates - 1))
		{
			_pFile->Writeln("");
			_pFile->Writeln(CStrF("%s- - - - - - - - - - - - - - - - - - - -", Indent.Str()));
			_pFile->Writeln("");
		}
	}

	//_pFile->Writeln(CStrF("%s} /* States */", _Indent.Str()));
	_pFile->Writeln(CStrF("%s}", _Indent.Str()));
}

//--------------------------------------------------------------------------------

void CXRAG2::LogDump_Structure_State(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, int16 _iState, const CXRAG2_State* _pState) const
{
	MAUTOSTRIP( CXRAG2_LogDump_Structure_State, MAUTOSTRIP_VOID );
	CStr StateName = GetExportedStateName(_iState);
	if (StateName != "")
		_pFile->Writeln(CStrF("%sState%d (%s)", _Indent.Str(), _iState, StateName.Str()));
	else
		_pFile->Writeln(CStrF("%sState%d", _Indent.Str(), _iState));

	_pFile->Writeln(CStrF("%s{", _Indent.Str()));
	CStr Indent = _Indent + IndentBlank;

	if (_DumpFlags & AG2_DUMPFLAGS_STATE_PROPERTIES)
	{
		_pFile->Writeln(CStrF("%sFlags[0] = 0x%08X <%s>", Indent.Str(), _pState->GetFlags(0), FlagsStr(_pState->GetFlags(0)).Str()));
		_pFile->Writeln(CStrF("%sFlags[1] = 0x%08X <%s>", Indent.Str(), _pState->GetFlags(1), FlagsStr(_pState->GetFlags(1)).Str()));
		_pFile->Writeln("");
	}

	if (_DumpFlags & AG2_DUMPFLAGS_STATE_PROPERTIES)
	{
		_pFile->Writeln(CStrF("%siBaseConstant = %d", Indent.Str(), _pState->GetBaseConstantIndex()));
		_pFile->Writeln(CStrF("%snBaseConstants = %d", Indent.Str(), _pState->GetNumConstants()));
		_pFile->Writeln("");
	}

	if (_DumpFlags & AG2_DUMPFLAGS_STATE_STATECONSTANTS)
	{
		if (_pState->GetNumConstants() > 0)
		{
			_pFile->Writeln(CStrF("%sStateConstants:", Indent.Str()));
			CStr Indent2 = Indent + IndentBlank;
			for (int16 jStateConstant = 0; jStateConstant < _pState->GetNumConstants(); jStateConstant++)
			{
				int16 iStateConstant = _pState->GetBaseConstantIndex() + jStateConstant;
				const CXRAG2_StateConstant* pStateConstant = GetStateConstant(iStateConstant);
				CStr StateConstantName = GetExportedStateConstantNameFromID(pStateConstant->m_ID);
				if (StateConstantName == "")
					_pFile->Writeln(CStrF("%s[C%03d]  <ID %d, Value %3.3f>", Indent2.Str(), iStateConstant, pStateConstant->m_ID, pStateConstant->m_Value));
				else
					_pFile->Writeln(CStrF("%s[C%03d](ID %d)  %s = %3.3f", Indent2.Str(), iStateConstant, pStateConstant->m_ID, StateConstantName.Str(), pStateConstant->m_Value));
			}
		}
		else
		{
			_pFile->Writeln(CStrF("%sStateConstants: (empty)", Indent.Str()));
		}
		_pFile->Writeln("");
	}

	if (_DumpFlags & AG2_DUMPFLAGS_STATE_PROPERTIES)
	{
		_pFile->Writeln(CStrF("%sPriority = %d", Indent.Str(), _pState->GetPriority()));
		_pFile->Writeln(CStrF("%snAnimLayers = %d", Indent.Str(), _pState->GetNumAnimLayers()));
		_pFile->Writeln(CStrF("%siBaseAnimLayer = %d", Indent.Str(), _pState->GetBaseAnimLayerIndex()));
		_pFile->Writeln(CStrF("%siLoopControlAnimLayer = %d", Indent.Str(), _pState->GetLoopControlAnimLayerIndex()));
		_pFile->Writeln("");
	}

	if (_DumpFlags & AG2_DUMPFLAGS_STATE_ANIMLAYERS)
	{
		if (_pState->GetNumAnimLayers() > 0)
		{
			_pFile->Writeln(CStrF("%sAnimLayers:", Indent.Str()));
			CStr Indent2 = Indent + IndentBlank;
			for (int16 jAnimLayer = 0; jAnimLayer < _pState->GetNumAnimLayers(); jAnimLayer++)
			{
				int16 iAnimLayer = _pState->GetBaseAnimLayerIndex() + jAnimLayer;
				const CXRAG2_AnimLayer* pAnimLayer = GetAnimLayer(iAnimLayer);
				CStr AnimLayerStr = CStrF("iAnim %d, iBaseJoint %d, TimeScale %3.3f TimeOffset %3.3f Flags: 0x%.8X", pAnimLayer->GetAnimIndex(), pAnimLayer->GetBaseJointIndex(), pAnimLayer->GetTimeScale(), pAnimLayer->GetTimeOffset(),pAnimLayer->GetAnimFlags());
				_pFile->Writeln(CStrF("%s[L%03d]  <%s>", Indent2.Str(), iAnimLayer, AnimLayerStr.Str()));
			}
		}
		else
		{
			_pFile->Writeln(CStrF("%sAnimLayers: (empty)", Indent.Str()));
		}
		_pFile->Writeln("");
	}

	if (_DumpFlags & AG2_DUMPFLAGS_STATE_PROPERTIES)
	{
		_pFile->Writeln(CStrF("%siBaseNode = %d", Indent.Str(), _pState->GetBaseNodeIndex()));
		_pFile->Writeln(CStrF("%siBaseAction = %d", Indent.Str(), _pState->GetBaseActionIndex()));
		_pFile->Writeln(CStrF("%siBasePropertyParam = %d", Indent.Str(), _pState->GetBasePropertyParamIndex()));
		_pFile->Writeln("");
	}

	if (_DumpFlags & AG2_DUMPFLAGS_STATE_NODETREES)
	{
		int16 iNode = _pState->GetBaseNodeIndex();
		if (iNode != AG2_NODEINDEX_NULL)
		{
			_pFile->Writeln(CStrF("%sNodeTree:", Indent.Str()));
			LogDump_Structure_Node(_pFile, _DumpFlags, Indent + IndentBlank, "", _pState, iNode);
		}
		else
		{
			_pFile->Writeln(CStrF("%sNodeTree: (empty)", Indent.Str()));
		}
		_pFile->Writeln("");
	}

	//_pFile->Writeln(CStrF("%s} /* State%d */", _Indent.Str(), _iState));
	_pFile->Writeln(CStrF("%s}", _Indent.Str()));
}

void CXRAG2::LogDump_Structure_SwitchState(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, int16 _iState, const CXRAG2_SwitchState* _pState) const
{
	MAUTOSTRIP( CXRAG2_LogDump_Structure_State, MAUTOSTRIP_VOID );
	CStr StateName = GetExportedSwitchStateName(_iState);
	if (StateName != "")
		_pFile->Writeln(CStrF("%sSwitchState%d (%s)", _Indent.Str(), _iState, StateName.Str()));
	else
		_pFile->Writeln(CStrF("%sSwitchState%d", _Indent.Str(), _iState));

	_pFile->Writeln(CStrF("%s{", _Indent.Str()));
	CStr Indent = _Indent + IndentBlank;

	if (_DumpFlags & AG2_DUMPFLAGS_STATE_PROPERTIES)
	{
		CStr PropertyName;
		int32 iProperty = _pState->GetPropertyID();
		switch (_pState->GetPropertyType())
		{
		case AG2_PROPERTYTYPE_FLOAT:
			{
				PropertyName = GetExportedPropertyFloatNameFromID(iProperty);
				break;
			}
		case AG2_PROPERTYTYPE_INT:
			{
				PropertyName = GetExportedPropertyIntNameFromID(iProperty);
				break;
			}
		case AG2_PROPERTYTYPE_BOOL:
			{
				PropertyName = GetExportedPropertyBoolNameFromID(iProperty);
				break;
			}
		case AG2_PROPERTYTYPE_CONDITION:
			{
				PropertyName = GetExportedPropertyConditionNameFromID(iProperty);
				break;
			}
		case AG2_PROPERTYTYPE_FUNCTION:
			{
				PropertyName = GetExportedPropertyFunctionNameFromID(iProperty);
				break;
			}
		default:
			PropertyName = CStr("ERROR: PROPERTYTYPE!!!");
		}

		_pFile->Writeln(CStrF("%sPriority = %d", Indent.Str(), _pState->GetPriority()));
		_pFile->Writeln(CStrF("%sProperty: %s", Indent.Str(), PropertyName.GetStr()));
		_pFile->Writeln(CStrF("%siBaseActionVal = %d", Indent.Str(), _pState->GetBaseActionValIndex()));
		_pFile->Writeln(CStrF("%sNumActionVals = %d", Indent.Str(), _pState->GetNumActionVal()));
		_pFile->Writeln("");
	}

	if (_DumpFlags & AG2_DUMPFLAGS_STATE_NODETREES)
	{
		int32 NumActionVals = _pState->GetNumActionVal();
		int32 iBaseActionVal = _pState->GetBaseActionValIndex();
		for (int32 i = 0; i < NumActionVals; i++)
		{
			CStr PropertyVal;
			const CXRAG2_SwitchStateActionVal* pVal = GetSwitchStateActionVal(iBaseActionVal + i);

			if (i < (NumActionVals -1))
			{
				switch (_pState->GetPropertyType())
				{
				case AG2_PROPERTYTYPE_FLOAT:
					{
						PropertyVal = CStrF("%f",pVal->GetConstantFloat());
						break;
					}
				case AG2_PROPERTYTYPE_INT:
					{
						PropertyVal = CStrF("%d",pVal->GetConstantInt());
						break;
					}
				case AG2_PROPERTYTYPE_BOOL:
					{
						PropertyVal = CStrF("%d",pVal->GetConstantBool());
						break;
					}
				case AG2_PROPERTYTYPE_CONDITION:
					{
						PropertyVal = CStrF("%f",pVal->GetConstantFloat());
						break;
					}
				case AG2_PROPERTYTYPE_FUNCTION:
					{
						PropertyVal = CStrF("%d",pVal->GetConstantInt());
						break;
					}
				default:
					PropertyVal = CStr("ERROR: PROPERTYTYPE!!!");
				}
			}
			else
			{
				PropertyVal = "Default";
			}
	
			int32 iBaseMoveToken = pVal->GetMoveTokenIndex();
			CStr MoveTokenStr = CStrF(" [IAV%03d]",iBaseActionVal + i);
			for (int32 i = 0; i < 1; i++)
			{
				const CXRAG2_MoveToken* pMoveToken = GetMoveToken(iBaseMoveToken + i);
				CAG2TokenID TokenID = pMoveToken->GetTokenID();
				CAG2StateIndex iTargetState = pMoveToken->GetTargetStateIndex();
				CAG2GraphBlockIndex iTargetGraphBlock = pMoveToken->GetTargetGraphBlockIndex();
				if (iTargetGraphBlock != AG2_GRAPHBLOCKINDEX_NULL || iTargetState != AG2_STATEINDEX_NULL)
				{
					CStr TokenName;
					if (TokenID == AG2_TOKENID_NULL)
						TokenName = "TokenCUR -> ";
					else
						TokenName = CStrF("Token%d -> ", TokenID);
					MoveTokenStr += TokenName;
				}
				if (iTargetGraphBlock != AG2_GRAPHBLOCKINDEX_NULL)
				{
					CStr TargetGBName;
					MoveTokenStr += CStrF("GB%d (%s)", iTargetGraphBlock, GetExportedGraphBlockName(iTargetGraphBlock).Str());

				}
				if (iTargetState != AG2_STATEINDEX_NULL)
				{
					CStr TargetStateName;
					if (iTargetState == AG2_STATEINDEX_TERMINATE)
						TargetStateName = "TERMINATE";
					else if (iTargetState == AG2_STATEINDEX_STARTAG)
						TargetStateName = "STARTAG";
					else
					{
						if (!pMoveToken->GetTargetStateType())
							TargetStateName = CStrF("State%d (%s(%f))", iTargetState, GetExportedStateName(iTargetState).Str(),pMoveToken->GetAnimBlendDuration());
						else
							TargetStateName = CStrF("SwitchState%d (%s(%f))", iTargetState, GetExportedSwitchStateName(iTargetState).Str(),pMoveToken->GetAnimBlendDuration());
					}

					MoveTokenStr += CStrF(" %s", (char*)TargetStateName);
				}
			}
			PropertyVal += MoveTokenStr;
			_pFile->Writeln(CStrF("%sVal: %s",Indent.GetStr(),PropertyVal.GetStr()));
		}

		_pFile->Writeln("");
	}

	//_pFile->Writeln(CStrF("%s} /* State%d */", _Indent.Str(), _iState));
	_pFile->Writeln(CStrF("%s}", _Indent.Str()));
}

//--------------------------------------------------------------------------------

void CXRAG2::LogDump_Structure_Node(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, CStr _BranchStr, const CXRAG2_State* _pState, int16 _iNode) const
{
	MAUTOSTRIP( CXRAG2_LogDump_Structure_Node, MAUTOSTRIP_VOID );
	const CXRAG2_ConditionNodeV2* pNode = GetNode(_iNode);

	CStr ConditionStr;

	CStr PropertyName;

	CStr OperatorName = GetExportedOperatorNameFromID(pNode->GetOperator());
	if (OperatorName == "")
		CStrF("O%d", pNode->GetOperator());

	switch (pNode->GetPropertyType())
	{
	case AG2_PROPERTYTYPE_CONDITION:
		PropertyName = GetExportedPropertyConditionNameFromID(pNode->GetProperty());
		if (PropertyName == "")
			PropertyName = CStrF("P%d(T:%d)", pNode->GetProperty(),pNode->GetPropertyType());
		ConditionStr = CStrF("C:%s %s %3.3f", PropertyName.Str(), OperatorName.Str(), pNode->GetConstantFloat());
		break;
	case AG2_PROPERTYTYPE_FUNCTION:
		PropertyName = GetExportedPropertyFunctionNameFromID(pNode->GetProperty());
		if (PropertyName == "")
			PropertyName = CStrF("P%d(T:%d)", pNode->GetProperty(),pNode->GetPropertyType());
		ConditionStr = CStrF("P:%s %s %d", PropertyName.Str(), OperatorName.Str(), pNode->GetConstantInt());
		break;
	case AG2_PROPERTYTYPE_FLOAT:
		PropertyName = GetExportedPropertyFloatNameFromID(pNode->GetProperty());
		if (PropertyName == "")
			PropertyName = CStrF("P%d(T:%d)", pNode->GetProperty(),pNode->GetPropertyType());
		ConditionStr = CStrF("F:%s %s %3.3f", PropertyName.Str(), OperatorName.Str(), pNode->GetConstantFloat());
		break;
	case AG2_PROPERTYTYPE_INT:
		PropertyName = GetExportedPropertyIntNameFromID(pNode->GetProperty());
		if (PropertyName == "")
			PropertyName = CStrF("P%d(T:%d)", pNode->GetProperty(),pNode->GetPropertyType());
		ConditionStr = CStrF("I:%s %s %d", PropertyName.Str(), OperatorName.Str(), pNode->GetConstantInt());		
		break;
	case AG2_PROPERTYTYPE_BOOL:
		PropertyName = GetExportedPropertyBoolNameFromID(pNode->GetProperty());
		if (PropertyName == "")
			PropertyName = CStrF("P%d(T:%d)", pNode->GetProperty(),pNode->GetPropertyType());
		ConditionStr = CStrF("B:%s %s %d", PropertyName.Str(), OperatorName.Str(), pNode->GetConstantBool());
		break;
	default:
		break;
	}

	CStr Indent = _Indent + _BranchStr;
	_pFile->Writeln(CStrF("%s[N%03d]  (%s)", Indent.Str(), _iNode, ConditionStr.Str()));

	int16 TrueAction = pNode->GetTrueAction();
	int16 FalseAction = pNode->GetFalseAction();

	Indent = _Indent;
	if (_BranchStr != "")
	{
		if (_BranchStr == IndentBranch)
			Indent += IndentBranched;
		else
			Indent += IndentBlank;
	}

	LogDump_Structure_NodeAction(_pFile, _DumpFlags, Indent, IndentBranch, _pState, TrueAction);
	LogDump_Structure_NodeAction(_pFile, _DumpFlags, Indent, IndentBranchEnd, _pState, FalseAction);
}

//--------------------------------------------------------------------------------

void CXRAG2::LogDump_Structure_NodeAction(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, CStr _BranchStr, const CXRAG2_State* _pState, int16 _NodeAction) const
{
	MAUTOSTRIP( CXRAG2_LogDump_Structure_NodeAction, MAUTOSTRIP_VOID );
	if (_NodeAction == AG2_NODEACTION_FAILPARSE)
	{
		_pFile->Writeln(CStrF("%s%s[FAIL]", (char*)_Indent, (char*)_BranchStr));
		return;
	}

	if ((_NodeAction & AG2_NODEACTION_TYPEMASK) == AG2_NODEACTION_ENDPARSE)
	{
		CAG2ActionIndex iBaseAction = _pState->GetBaseActionIndex();
		CAG2ActionIndex iAction = iBaseAction + (_NodeAction & AG2_NODEACTION_ACTIONMASK);
		const CXRAG2_Action* pAction = GetAction(iAction);
		int NumMoveTokens = pAction->GetNumMoveTokens();
		int iBaseMoveToken = pAction->GetBaseMoveTokenIndex();

		CStr MoveTokenStr = CStrF("%s%s[A%03d]",_Indent.Str(),_BranchStr.Str(),iAction);
		for (int32 i = 0; i< NumMoveTokens; i++)
		{
			const CXRAG2_MoveToken* pMoveToken = GetMoveToken(iBaseMoveToken + i);
			CAG2TokenID TokenID = pMoveToken->GetTokenID();
			CAG2StateIndex iTargetState = pMoveToken->GetTargetStateIndex();
			CAG2GraphBlockIndex iTargetGraphBlock = pMoveToken->GetTargetGraphBlockIndex();
			if (iTargetGraphBlock != AG2_GRAPHBLOCKINDEX_NULL || iTargetState != AG2_STATEINDEX_NULL)
			{
				CStr TokenName;
				if (TokenID == AG2_TOKENID_NULL)
					TokenName = "TokenCUR -> ";
				else
					TokenName = CStrF("Token%d -> ", TokenID);
				MoveTokenStr += TokenName;
			}
			if (iTargetGraphBlock != AG2_GRAPHBLOCKINDEX_NULL)
			{
				CStr TargetGBName;
				MoveTokenStr += CStrF("GB%d (%s)", iTargetGraphBlock, GetExportedGraphBlockName(iTargetGraphBlock).Str());
				
			}
			if (iTargetState != AG2_STATEINDEX_NULL)
			{
				CStr TargetStateName;
				if (iTargetState == AG2_STATEINDEX_TERMINATE)
					TargetStateName = "TERMINATE";
				else if (iTargetState == AG2_STATEINDEX_STARTAG)
					TargetStateName = "STARTAG";
				else
				{
					if (!pMoveToken->GetTargetStateType())
						TargetStateName = CStrF("State%d (%s(%f))", iTargetState, GetExportedStateName(iTargetState).Str(),pMoveToken->GetAnimBlendDuration());
					else
						TargetStateName = CStrF("SwitchState%d (%s(%f))", iTargetState, GetExportedSwitchStateName(iTargetState).Str(),pMoveToken->GetAnimBlendDuration());
				}

				MoveTokenStr += CStrF(" %s", (char*)TargetStateName);
			}
		}
		_pFile->Writeln(MoveTokenStr);

		if ((_DumpFlags & AG2_DUMPFLAGS_STATE_NODETREEEFFECTS) && (pAction->GetNumEffectInstances() > 0))
		{
			CStr EffectIndent = _Indent;
			if (_BranchStr != "")
			{
				if (_BranchStr == " |-")
					EffectIndent += " | ";
				else
					EffectIndent += "   ";
			}

			int nEffectInstances = pAction->GetNumEffectInstances();
			for (int jEffectInstance = 0; jEffectInstance < nEffectInstances; jEffectInstance++)
			{
				CAG2EffectInstanceIndex iEffectInstance = pAction->GetBaseEffectInstanceIndex() + jEffectInstance;
				const CXRAG2_EffectInstance* pEffectInstance = GetEffectInstance(iEffectInstance);
				CAG2EffectID EffectID = pEffectInstance->m_ID;

				CStr EffectName = GetExportedEffectNameFromID(EffectID);
				if (EffectName == "")
					EffectName = CStrF(" E%03d", iEffectInstance);
				else
					EffectName = CStrF("[E%03d]  %s", iEffectInstance, EffectName.Str());

				CStr EffectParamsStr;
				if (pEffectInstance->m_nParams > 0)
				{
					CXRAG2_ICallbackParams Params = GetICallbackParams(pEffectInstance->m_iParams, pEffectInstance->m_nParams);
					EffectParamsStr = CStrF("(%s)", Params.GetStr().Str());
				}

				if (jEffectInstance < (nEffectInstances - 1))
					_pFile->Writeln(CStrF("%s |-%s%s", EffectIndent.Str(), EffectName.Str(), EffectParamsStr.Str()));
				else
					_pFile->Writeln(CStrF("%s `-%s%s", EffectIndent.Str(), EffectName.Str(), EffectParamsStr.Str()));
			}
		}

		return;
	}

	if ((_NodeAction & AG2_NODEACTION_TYPEMASK) == AG2_NODEACTION_PARSENODE)
	{
		int16 iNode = _pState->GetBaseNodeIndex() + (_NodeAction & AG2_NODEACTION_NODEMASK);
		LogDump_Structure_Node(_pFile, _DumpFlags, _Indent, _BranchStr, _pState, iNode);
	}
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG2::LogDump_Actions(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_Actions, MAUTOSTRIP_VOID );
	_pFile->Writeln(CStrF("%sActions:", _Indent.Str()));
	_pFile->Writeln(CStrF("{", _Indent.Str()));
	CStr Indent = _Indent + IndentBlank;
	CStr IndentMT = Indent + IndentBlank;
	for (int16 iAction = 0; iAction < GetNumActions(); iAction++)
	{
		const CXRAG2_Action* pAction = GetAction(iAction);
		int NumMoveTokens = pAction->GetNumMoveTokens();
		int iBaseMoveToken = pAction->GetBaseMoveTokenIndex();
		CStr MoveTokenStr;
		for (int32 i = 0; i < NumMoveTokens; i++)
		{
			if (NumMoveTokens - i - 1 > 0)
				MoveTokenStr += CStrF("%d, ",iBaseMoveToken + i);
			else
				MoveTokenStr += CStrF("%d",iBaseMoveToken + i);
		}
		if (MoveTokenStr.Len() == 0)
			MoveTokenStr = "Empty";
		_pFile->Writeln(CStrF("%sAction[A%03d] MoveTokens: <%s>", Indent.Str(), iAction, MoveTokenStr.Str()));
	}
	_pFile->Writeln(CStrF("}", _Indent.Str()));

	_pFile->Writeln("");
}

void CXRAG2::LogDump_SwitchStateActionVals(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_Actions, MAUTOSTRIP_VOID );
	_pFile->Writeln(CStrF("%sSwitchStateActionVals:", _Indent.Str()));
	_pFile->Writeln(CStrF("{", _Indent.Str()));
	CStr Indent = _Indent + IndentBlank;
	CStr IndentMT = Indent + IndentBlank;
	for (int16 iAction = 0; iAction < GetNumSwitchStateActionVals(); iAction++)
	{
		const CXRAG2_SwitchStateActionVal* pActionVal = GetSwitchStateActionVal(iAction);
		int iBaseMoveToken = pActionVal->GetMoveTokenIndex();
		CStr MoveTokenStr = CStrF("%d",iBaseMoveToken);
		if (MoveTokenStr.Len() == 0)
			MoveTokenStr = "Empty";
		_pFile->Writeln(CStrF("%sSwitchStateAction[IAV%03d] MoveTokens: <%s>", Indent.Str(), iAction, MoveTokenStr.Str()));
	}
	_pFile->Writeln(CStrF("}", _Indent.Str()));

	_pFile->Writeln("");
}

void CXRAG2::LogDump_MoveTokens(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_Actions, MAUTOSTRIP_VOID );
	_pFile->Writeln(CStrF("%sMoveTokens:", _Indent.Str()));
	_pFile->Writeln(CStrF("{", _Indent.Str()));
	CStr Indent = _Indent + IndentBlank;
	int NumMoveTokens = GetNumMoveTokens();
	for (int32 i = 0; i < NumMoveTokens; i++)
	{
		const CXRAG2_MoveToken* pMoveToken = GetMoveToken(i);
		CStr TokenIDStr = ((pMoveToken->GetTokenID() != AG2_TOKENID_NULL) ? CStrF("%d", pMoveToken->GetTokenID()) : CStr("CUR"));
		int16 iTargetState = pMoveToken->GetTargetStateIndex();
		int16 iTargetGraphBlock = pMoveToken->GetTargetGraphBlockIndex();
		CStr TargetStateStr;
		CStr TargetGraphBlockStr;
		if (iTargetState == AG2_STATEINDEX_NULL)
			TargetStateStr = " (no move)";
		else if (iTargetState == AG2_STATEINDEX_TERMINATE)
			TargetStateStr = " -> TERMINATE";
		else if (iTargetState == AG2_STATEINDEX_STARTAG)
			TargetStateStr = " -> STARTAG";
		else
		{
			if (!pMoveToken->GetTargetStateType())
			{
				TargetStateStr = GetExportedStateName(iTargetState).Str();
				TargetStateStr = CStrF(" -> State%d (%s)", iTargetState, TargetStateStr.Str());
			}
			else
			{
				TargetStateStr = GetExportedSwitchStateName(iTargetState).Str();
				TargetStateStr = CStrF(" -> SwitchState%d (%s)", iTargetState, TargetStateStr.Str());
			}
		}

		if (iTargetGraphBlock == AG2_GRAPHBLOCKINDEX_NULL)
			TargetGraphBlockStr = ", (no move)";
		else
		{
			TargetGraphBlockStr  = GetExportedGraphBlockName(iTargetGraphBlock).Str();
			TargetGraphBlockStr  = CStrF(", -> GraphBlock%d (%s)", iTargetGraphBlock, TargetGraphBlockStr.Str());
		}

		CStr ActionStr = CStrF("%sMoveToken[M%03d] Token%s%s%s, ABDuration %3.3f", Indent.Str(),i,TokenIDStr.Str(), TargetStateStr.Str(), TargetGraphBlockStr.Str(), pMoveToken->GetAnimBlendDuration());
		_pFile->Writeln(ActionStr);
	}
	if (NumMoveTokens == 0)
		_pFile->Writeln(CStrF("%s<Empty>", Indent.Str()));
	_pFile->Writeln(CStrF("}", _Indent.Str()));

	_pFile->Writeln("");
}

void CXRAG2::LogDump_MoveAnimGraphs(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_MoveAnimGraphs, MAUTOSTRIP_VOID );
	_pFile->Writeln(CStrF("%sMoveAnimGraphs:", _Indent.Str()));
	_pFile->Writeln(CStrF("{", _Indent.Str()));
	CStr Indent = _Indent + IndentBlank;
	int NumMoveAnimGraphs = GetNumMoveAnimGraphs();
	for (int32 i = 0; i < NumMoveAnimGraphs; i++)
	{
		const CXRAG2_MoveAnimGraph* pMoveAnimGraph = GetMoveAnimGraph(i);
		CStr TokenIDStr = ((pMoveAnimGraph->GetTokenID() != AG2_TOKENID_NULL) ? CStrF("%d", pMoveAnimGraph->GetTokenID()) : CStr("CUR"));
		
		CStr TargetAnimGraphStr  = GetExportedAnimGraphName(i).Str();

		CStr ActionStr = CStrF("%sMoveAnimGraph[M%03d] Token%s -> %s, ABDuration %3.3f", Indent.Str(),i,TokenIDStr.Str(), TargetAnimGraphStr.Str(), pMoveAnimGraph->GetAnimBlendDuration());
		_pFile->Writeln(ActionStr);
	}
	if (NumMoveAnimGraphs == 0)
		_pFile->Writeln(CStrF("%s<Empty>", Indent.Str()));
	_pFile->Writeln(CStrF("}", _Indent.Str()));

	_pFile->Writeln("");
}

void CXRAG2::LogDump_Reactions(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_Reactions, MAUTOSTRIP_VOID );
	_pFile->Writeln(CStrF("%sReactions:", _Indent.Str()));
	_pFile->Writeln(CStrF("%s{", _Indent.Str()));
	CStr Indent = _Indent + IndentBlank;
	for (int16 iReaction = 0; iReaction < GetNumReactions(); iReaction++)
	{
		const CXRAG2_Reaction* pReaction = GetReaction(iReaction);
		int32 nMoveTokens = pReaction->GetNumMoveTokens();
		CStr MoveTokenStr("<");
		for (int32 i = 0; i < nMoveTokens; i++)
		{
			if (i < (nMoveTokens - 1))
				MoveTokenStr += CStrF("%d,",i + pReaction->GetBaseMoveTokenIndex());
			else
				MoveTokenStr += CStrF("%d",i + pReaction->GetBaseMoveTokenIndex());
			/*CStr TokenIDStr = ((pReaction->GetTokenID() != AG2_TOKENID_NULL) ? CStrF("%d", pReaction->GetTokenID()) : CStr("CUR"));
			int16 iTargetState = pReaction->GetTargetStateIndex();
			CStr TargetStateStr;
			if (iTargetState == AG2_STATEINDEX_NULL)
				TargetStateStr = " (no move)";
			else if (iTargetState == AG2_STATEINDEX_TERMINATE)
				TargetStateStr = " -> TERMINATE";
			else
			{
				TargetStateStr = GetExportedStateName(iTargetState).Str();
				TargetStateStr = CStrF(" -> State%d (%s)", iTargetState, TargetStateStr.Str());
			}

			int16 iTargetGraphBlock = pReaction->GetTargetGraphBlockIndex();
			CStr TargetGraphBlockStr;
			if (iTargetGraphBlock == AG2_GRAPHBLOCKINDEX_NULL)
				TargetGraphBlockStr = " (no move)";
			else if (iTargetState == AG2_STATEINDEX_TERMINATE)
				TargetGraphBlockStr = " -> TERMINATE";
			else
			{
				TargetGraphBlockStr = GetExportedGraphBlockName(iTargetGraphBlock).Str();
				TargetGraphBlockStr = CStrF(" -> GraphBlock%d (%s)", iTargetGraphBlock, TargetGraphBlockStr.Str());
			}*/
		}
		MoveTokenStr += CStr(">");

		//CStr ReactionStr = CStrF("Token%s%s%s, ABDuration %3.3f", TokenIDStr.Str(), TargetGraphBlockStr.Str(), TargetStateStr.Str(), pReaction->GetAnimBlendDuration());
		_pFile->Writeln(CStrF("%sReaction[R%03d] = %s", Indent.Str(), iReaction, MoveTokenStr.Str()));
	}
	_pFile->Writeln(CStrF("%s}", _Indent.Str()));

	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------

void CXRAG2::LogDump_ConditionNodes(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_ConditionNodes, MAUTOSTRIP_VOID );
	_pFile->Writeln(CStrF("%sConditionNodes: (N/I)", _Indent.Str()));
	/*_pFile->Writeln("");
	return;*/

	int nNodes = GetNumNodes();
	if (nNodes > 0)
	{
		_pFile->Writeln(CStrF("%sConditionNodes:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iConditionNode = 0; iConditionNode < nNodes; iConditionNode++)
		{
//			const CXRAG2_ConditionNodeV2* pNode = GetNode(iConditionNode);
			CStr ConditionStr = "N/I";
			_pFile->Writeln(CStrF("%sConditionNode[N%03d] = <%s>", Indent.Str(), iConditionNode, ConditionStr.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sConditionNodes: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------

void CXRAG2::LogDump_AnimLayers(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_AnimLayers, MAUTOSTRIP_VOID );
	int nAnimLayers = GetNumAnimLayers();
	if (nAnimLayers > 0)
	{
		_pFile->Writeln(CStrF("%sAnimLayers:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iAnimLayer = 0; iAnimLayer < nAnimLayers; iAnimLayer++)
		{
			const CXRAG2_AnimLayer* pAnimLayer = GetAnimLayer(iAnimLayer);
			CStr AnimLayerStr = CStrF("iAnim %d, iBaseJoint %d, TimeScale %3.3f Flags: 0x%04X", pAnimLayer->GetAnimIndex(), pAnimLayer->GetBaseJointIndex(), pAnimLayer->GetTimeScale(),pAnimLayer->GetAnimFlags());
			_pFile->Writeln(CStrF("%sAnimLayer[L%03d] = <%s>", Indent.Str(), iAnimLayer, AnimLayerStr.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sAnimLayers: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

void CXRAG2::LogDump_AnimNames(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_AnimLayers, MAUTOSTRIP_VOID );
	int nAnimNames = GetNumAnimNames();
	if (nAnimNames > 0)
	{
		_pFile->Writeln(CStrF("%sAnimNames:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iAnim = 0; iAnim < nAnimNames; iAnim++)
		{
			CStr AnimName = m_AnimContainerNames.GetContainerName(m_lAnimNames[iAnim].m_iContainerName);
			CStr AnimNameStr = CStrF("Name: %s:%d", AnimName.GetStr(),m_lAnimNames[iAnim].m_iAnimSeq);
			_pFile->Writeln(CStrF("%sAnimName[N%03d] = <%s>", Indent.Str(), iAnim, AnimNameStr.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sAnimNames: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------

void CXRAG2::LogDump_EffectInstances(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_EffectInstances, MAUTOSTRIP_VOID );
	int nEffectInstances = GetNumEffectInstances();
	if (nEffectInstances > 0)
	{
		_pFile->Writeln(CStrF("%sEffectInstances:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iEffectInstance = 0; iEffectInstance < nEffectInstances; iEffectInstance++)
		{
			const CXRAG2_EffectInstance* pEffectInstance = GetEffectInstance(iEffectInstance);
			CStr EffectInstanceStr;
			if (pEffectInstance->m_nParams > 0)
			{
				CXRAG2_ICallbackParams IParams = GetICallbackParams(pEffectInstance->m_iParams, pEffectInstance->m_nParams);
				EffectInstanceStr = CStrF("ID %d, iParams %d, nParams %d, Params (%s)", pEffectInstance->m_ID, pEffectInstance->m_iParams, pEffectInstance->m_nParams, IParams.GetStr().Str());
			}
			else
			{
				EffectInstanceStr = CStrF("ID %d (no params)", pEffectInstance->m_ID);
			}

			_pFile->Writeln(CStrF("%sEffectInstance[E%03d] = <%s>", Indent.Str(), iEffectInstance, EffectInstanceStr.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sEffectInstances: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------

void CXRAG2::LogDump_StateConstants(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_StateConstants, MAUTOSTRIP_VOID );
	int nStateConstants = GetNumStateConstants();
	if (nStateConstants > 0)
	{
		_pFile->Writeln(CStrF("%sStateConstants:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iStateConstant = 0; iStateConstant < nStateConstants; iStateConstant++)
		{
			const CXRAG2_StateConstant* pStateConstant = GetStateConstant(iStateConstant);
			CStr StateConstantStr = CStrF("ID %d, Value %3.3f", pStateConstant->m_ID, pStateConstant->m_Value);
			_pFile->Writeln(CStrF("%sStateConstant[C%03d] = <%s>", Indent.Str(), iStateConstant, StateConstantStr.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sStateConstants: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------

void CXRAG2::LogDump_CallbackParams(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_CallbackParams, MAUTOSTRIP_VOID );
	int nCallbackParams = m_lCallbackParams.Len();
	if (nCallbackParams > 0)
	{
		_pFile->Writeln(CStrF("%sCallbackParams:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iParam = 0; iParam < nCallbackParams; iParam++)
			_pFile->Writeln(CStrF("%sParam[P%03d] = %d", Indent.Str(), iParam, m_lCallbackParams[iParam]));
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sCallbackParams: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------

void CXRAG2::LogDump_ExportedActions(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_ExportedActions, MAUTOSTRIP_VOID );
	int nExportedActions = m_lExportedActionNames.Len();
	if (nExportedActions > 0)
	{
		_pFile->Writeln(CStrF("%sExportedActions:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iExportedActionName = 0; iExportedActionName < nExportedActions; iExportedActionName++)
		{
			CStr ExportedActionName = GetExportedActionName(iExportedActionName);
			uint32 ActionHashKey = StringToHash(ExportedActionName);
			int16 iAction = GetActionIndexFromHashKey(ActionHashKey);
			_pFile->Writeln(CStrF("%sAction[A%03d]: '%s' (%X)", Indent.Str(), iAction, ExportedActionName.Str(), ActionHashKey));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sExportedActions: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------

void CXRAG2::LogDump_ExportedStates(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_ExportedStates, MAUTOSTRIP_VOID );
	int nExportedStates = m_lExportedStateNames.Len();
	if (nExportedStates > 0)
	{
		_pFile->Writeln(CStrF("%sExportedStates:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iExportedState = 0; iExportedState < nExportedStates; iExportedState++)
		{
			CStr ExportedStateName = GetExportedStateName(iExportedState);
			_pFile->Writeln(CStrF("%sState%d: '%s'", Indent.Str(), iExportedState, ExportedStateName.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sExportedStates: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}


void CXRAG2::LogDump_ExportedSwitchStates(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG_LogDump_ExportedStates, MAUTOSTRIP_VOID );
	int nExportedStates = m_lExportedSwitchStateNames.Len();
	if (nExportedStates > 0)
	{
		_pFile->Writeln(CStrF("%sExportedSwitchStates:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iExportedState = 0; iExportedState < nExportedStates; iExportedState++)
		{
			CStr ExportedStateName = GetExportedSwitchStateName(iExportedState);
			_pFile->Writeln(CStrF("%sSwitchState%d: '%s'", Indent.Str(), iExportedState, ExportedStateName.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sExportedSwitchStates: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}
//--------------------------------------------------------------------------------

void CXRAG2::LogDump_ExportedPropertiesCondition(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_ExportedProperties, MAUTOSTRIP_VOID );
	int nExportedPropertys = m_lExportedPropertyConditionNames.Len();
	if (nExportedPropertys > 0)
	{
		_pFile->Writeln(CStrF("%sExportedPropertiesCondition:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iExportedProperty = 0; iExportedProperty < nExportedPropertys; iExportedProperty++)
		{
			CStr ExportedPropertyName = GetExportedPropertyConditionNameFromIndex(iExportedProperty);
			_pFile->Writeln(CStrF("%sProperty[P%d]: '%s' (ID N/A)", Indent.Str(), iExportedProperty, ExportedPropertyName.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sExportedPropertiesCondition: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

void CXRAG2::LogDump_ExportedPropertiesFunction(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_ExportedProperties, MAUTOSTRIP_VOID );
	int nExportedPropertys = m_lExportedPropertyFunctionNames.Len();
	if (nExportedPropertys > 0)
	{
		_pFile->Writeln(CStrF("%sExportedPropertiesFunction:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iExportedProperty = 0; iExportedProperty < nExportedPropertys; iExportedProperty++)
		{
			CStr ExportedPropertyName = GetExportedPropertyFunctionNameFromIndex(iExportedProperty);
			_pFile->Writeln(CStrF("%sProperty[P%d]: '%s' (ID N/A)", Indent.Str(), iExportedProperty, ExportedPropertyName.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sExportedPropertiesFunction: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

void CXRAG2::LogDump_ExportedPropertiesFloat(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_ExportedProperties, MAUTOSTRIP_VOID );
	int nExportedPropertys = m_lExportedPropertyFloatNames.Len();
	if (nExportedPropertys > 0)
	{
		_pFile->Writeln(CStrF("%sExportedPropertiesFloat:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iExportedProperty = 0; iExportedProperty < nExportedPropertys; iExportedProperty++)
		{
			CStr ExportedPropertyName = GetExportedPropertyFloatNameFromIndex(iExportedProperty);
			_pFile->Writeln(CStrF("%sProperty[P%d]: '%s' (ID N/A)", Indent.Str(), iExportedProperty, ExportedPropertyName.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sExportedPropertiesFloat: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

void CXRAG2::LogDump_ExportedPropertiesInt(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_ExportedProperties, MAUTOSTRIP_VOID );
	int nExportedPropertys = m_lExportedPropertyIntNames.Len();
	if (nExportedPropertys > 0)
	{
		_pFile->Writeln(CStrF("%sExportedPropertiesInt:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iExportedProperty = 0; iExportedProperty < nExportedPropertys; iExportedProperty++)
		{
			CStr ExportedPropertyName = GetExportedPropertyIntNameFromIndex(iExportedProperty);
			_pFile->Writeln(CStrF("%sProperty[P%d]: '%s' (ID N/A)", Indent.Str(), iExportedProperty, ExportedPropertyName.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sExportedPropertiesInt: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

void CXRAG2::LogDump_ExportedPropertiesBool(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_ExportedProperties, MAUTOSTRIP_VOID );
	int nExportedPropertys = m_lExportedPropertyBoolNames.Len();
	if (nExportedPropertys > 0)
	{
		_pFile->Writeln(CStrF("%sExportedPropertiesBool:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iExportedProperty = 0; iExportedProperty < nExportedPropertys; iExportedProperty++)
		{
			CStr ExportedPropertyName = GetExportedPropertyBoolNameFromIndex(iExportedProperty);
			_pFile->Writeln(CStrF("%sProperty[P%d]: '%s' (ID N/A)", Indent.Str(), iExportedProperty, ExportedPropertyName.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sExportedPropertiesBool: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------

void CXRAG2::LogDump_ExportedOperators(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_ExportedOperators, MAUTOSTRIP_VOID );
	int nExportedOperators = m_lExportedOperatorNames.Len();
	if (nExportedOperators > 0)
	{
		_pFile->Writeln(CStrF("%sExportedOperators:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iExportedOperator = 0; iExportedOperator < nExportedOperators; iExportedOperator++)
		{
			CStr ExportedOperatorName = GetExportedOperatorNameFromIndex(iExportedOperator);
			_pFile->Writeln(CStrF("%sOperator[O%d]: '%s' (ID N/A)", Indent.Str(), iExportedOperator, ExportedOperatorName.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sExportedOperators: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------

void CXRAG2::LogDump_ExportedEffects(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_ExportedEffects, MAUTOSTRIP_VOID );
	int nExportedEffects = m_lExportedEffectNames.Len();
	if (nExportedEffects > 0)
	{
		_pFile->Writeln(CStrF("%sExportedEffects:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iExportedEffect = 0; iExportedEffect < nExportedEffects; iExportedEffect++)
		{
			CStr ExportedEffectName = GetExportedEffectNameFromIndex(iExportedEffect);
			_pFile->Writeln(CStrF("%sEffect[E%03d]: '%s' (ID N/A)", Indent.Str(), iExportedEffect, ExportedEffectName.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sExportedEffects: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------

void CXRAG2::LogDump_ExportedStateConstants(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_ExportedStateConstants, MAUTOSTRIP_VOID );
	int nExportedStateConstants = m_lExportedStateConstantNames.Len();
	if (nExportedStateConstants > 0)
	{
		_pFile->Writeln(CStrF("%sExportedStateConstants:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iExportedStateConstant = 0; iExportedStateConstant < nExportedStateConstants; iExportedStateConstant++)
		{
			CStr ExportedStateConstantName = GetExportedStateConstantNameFromIndex(iExportedStateConstant);
			_pFile->Writeln(CStrF("%sStateConstant[C%03d]: '%s' (ID N/A)", Indent.Str(), iExportedStateConstant, ExportedStateConstantName.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sExportedStateConstants: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

void CXRAG2::LogDump_ExportedImpulseTypes(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_ExportedImpulseTypes, MAUTOSTRIP_VOID );
	int nExportedImpulseTypes = m_lExportedImpulseTypeNames.Len();
	if (nExportedImpulseTypes > 0)
	{
		_pFile->Writeln(CStrF("%sExportedImpulseTypes:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iExportedImpulseType = 0; iExportedImpulseType < nExportedImpulseTypes; iExportedImpulseType++)
		{
			CStr ExportedImpulseTypeName = GetExportedImpulseTypeNameFromIndex(iExportedImpulseType);
			_pFile->Writeln(CStrF("%sImpulseType[I%03d]: '%s' (Value: %d)", Indent.Str(), iExportedImpulseType, ExportedImpulseTypeName.Str(),GetExportedImpulseTypeValueFromIndex(iExportedImpulseType)));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sExportedImpulseTypes: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

void CXRAG2::LogDump_ExportedImpulseValues(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG2_LogDump_ExportedImpulseValues, MAUTOSTRIP_VOID );
	int nExportedImpulseValues = m_lExportedImpulseValueNames.Len();
	if (nExportedImpulseValues > 0)
	{
		_pFile->Writeln(CStrF("%sExportedImpulseValues:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iExportedImpulseValue = 0; iExportedImpulseValue < nExportedImpulseValues; iExportedImpulseValue++)
		{
			CStr ExportedImpulseValueName = GetExportedImpulseValueNameFromIndex(iExportedImpulseValue);
			_pFile->Writeln(CStrF("%sImpulseValue[I%03d]: '%s' (Value: %d)", Indent.Str(), iExportedImpulseValue, ExportedImpulseValueName.Str(),GetExportedImpulseValueFromIndex(iExportedImpulseValue)));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sExportedImpulseValues: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
#endif //M_RTM
