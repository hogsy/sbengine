//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MDA.h"
#include "MFile.h"
#include "AnimGraph.h"

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

void CXRAG::LogDump(CStr _LogFileName, uint32 _DumpFlags) const
{
#ifndef PLATFORM_CONSOLE
	try
	{
		CStr Indent = IndentBlank;

		CCFile File;
		File.Open(_LogFileName, CFILE_WRITE);

		File.Writeln(CStrF("AnimGraph %s", m_Name.Str()));
		File.Writeln(CStr("{"));
		File.Writeln(CStrF("%snStates = %d", Indent.Str(), GetNumStates()));
		File.Writeln(CStrF("%snActions = %d", Indent.Str(), GetNumActions()));
		File.Writeln(CStrF("%snAnimLayers = %d", Indent.Str(), GetNumAnimLayers()));
		File.Writeln(CStrF("%snNodes = %d", Indent.Str(), GetNumNodes()));
		File.Writeln(CStrF("%snEffectInstances = %d", Indent.Str(), GetNumEffectInstances()));
		File.Writeln(CStrF("%snCallbackParams = %d", Indent.Str(), GetNumCallbackParams()));
		File.Writeln(CStrF("%snStateConstants = %d", Indent.Str(), GetNumStateConstants()));
		File.Writeln(CStr("}"));
		File.Writeln(CStr(""));

		if (_DumpFlags & AG_DUMPFLAGS_STATES)
			LogDump_Structure(&File, _DumpFlags, "");

		if (_DumpFlags & AG_DUMPFLAGS_ACTIONS)
			LogDump_Actions(&File, _DumpFlags, "");

		if (_DumpFlags & AG_DUMPFLAGS_CONDITIONNODES)
			LogDump_ConditionNodes(&File, _DumpFlags, "");

		if (_DumpFlags & AG_DUMPFLAGS_ANIMLAYERS)
			LogDump_AnimLayers(&File, _DumpFlags, "");

		if (_DumpFlags & AG_DUMPFLAGS_EFFECTINSTANCES)
			LogDump_EffectInstances(&File, _DumpFlags, "");

		if (_DumpFlags & AG_DUMPFLAGS_CALLBACKPARAMS)
			LogDump_CallbackParams(&File, _DumpFlags, "");

		if (_DumpFlags & AG_DUMPFLAGS_EXPORTEDACTIONS)
			LogDump_ExportedActions(&File, _DumpFlags, "");

		if (_DumpFlags & AG_DUMPFLAGS_EXPORTEDSTATES)
			LogDump_ExportedStates(&File, _DumpFlags, "");

		if (_DumpFlags & AG_DUMPFLAGS_EXPORTEDPROPERTIES)
			LogDump_ExportedProperties(&File, _DumpFlags, "");

		if (_DumpFlags & AG_DUMPFLAGS_EXPORTEDOPERATORS)
			LogDump_ExportedOperators(&File, _DumpFlags, "");

		if (_DumpFlags & AG_DUMPFLAGS_EXPORTEDEFFECTS)
			LogDump_ExportedEffects(&File, _DumpFlags, "");

		if (_DumpFlags & AG_DUMPFLAGS_EXPORTEDSTATECONSTANTS)
			LogDump_ExportedStateConstants(&File, _DumpFlags, "");
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

void CXRAG::LogDump_Structure(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG_LogDump_Structure, MAUTOSTRIP_VOID );
	_pFile->Writeln(CStrF("%sStates:", _Indent.Str()));
	_pFile->Writeln(CStrF("%s{", _Indent.Str()));

	CStr Indent = _Indent + IndentBlank;
	
	int nStates = GetNumStates();
	for (int iState = 0; iState < nStates; iState++)
	{
		const CXRAG_State* pState = GetState(iState);
		LogDump_Structure_State(_pFile, _DumpFlags, Indent, iState, pState);

		if (iState < (nStates - 1))
		{
			_pFile->Writeln("");
			_pFile->Writeln(CStrF("%s- - - - - - - - - - - - - - - - - - - -", Indent.Str()));
			_pFile->Writeln("");
		}
	}

	//_pFile->Writeln(CStrF("%s} /* States */", _Indent.Str()));
	_pFile->Writeln(CStrF("%s}", _Indent.Str()));
	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------

void CXRAG::LogDump_Structure_State(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, int16 _iState, const CXRAG_State* _pState) const
{
	MAUTOSTRIP( CXRAG_LogDump_Structure_State, MAUTOSTRIP_VOID );
	CStr StateName = GetExportedStateName(_iState);
	if (StateName != "")
		_pFile->Writeln(CStrF("%sState%d (%s)", _Indent.Str(), _iState, StateName.Str()));
	else
		_pFile->Writeln(CStrF("%sState%d", _Indent.Str(), _iState));

	_pFile->Writeln(CStrF("%s{", _Indent.Str()));
	CStr Indent = _Indent + IndentBlank;

	if (_DumpFlags & AG_DUMPFLAGS_STATE_PROPERTIES)
	{
		_pFile->Writeln(CStrF("%sFlags[0] = 0x%08X <%s>", Indent.Str(), _pState->GetFlags(0), FlagsStr(_pState->GetFlags(0)).Str()));
		_pFile->Writeln(CStrF("%sFlags[1] = 0x%08X <%s>", Indent.Str(), _pState->GetFlags(1), FlagsStr(_pState->GetFlags(1)).Str()));
		_pFile->Writeln("");
	}

	if (_DumpFlags & AG_DUMPFLAGS_STATE_PROPERTIES)
	{
		_pFile->Writeln(CStrF("%siBaseConstant = %d", Indent.Str(), _pState->GetBaseConstantIndex()));
		_pFile->Writeln(CStrF("%snBaseConstants = %d", Indent.Str(), _pState->GetNumConstants()));
		_pFile->Writeln("");
	}

	if (_DumpFlags & AG_DUMPFLAGS_STATE_STATECONSTANTS)
	{
		if (_pState->GetNumConstants() > 0)
		{
			_pFile->Writeln(CStrF("%sStateConstants:", Indent.Str()));
			CStr Indent2 = Indent + IndentBlank;
			for (int16 jStateConstant = 0; jStateConstant < _pState->GetNumConstants(); jStateConstant++)
			{
				int16 iStateConstant = _pState->GetBaseConstantIndex() + jStateConstant;
				const CXRAG_StateConstant* pStateConstant = GetStateConstant(iStateConstant);
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

	if (_DumpFlags & AG_DUMPFLAGS_STATE_PROPERTIES)
	{
		_pFile->Writeln(CStrF("%sPriority = %d", Indent.Str(), _pState->GetPriority()));
		_pFile->Writeln(CStrF("%snAnimLayers = %d", Indent.Str(), _pState->GetNumAnimLayers()));
		_pFile->Writeln(CStrF("%siBaseAnimLayer = %d", Indent.Str(), _pState->GetBaseAnimLayerIndex()));
		_pFile->Writeln(CStrF("%siLoopControlAnimLayer = %d", Indent.Str(), _pState->GetLoopControlAnimLayerIndex()));
		_pFile->Writeln("");
	}

	if (_DumpFlags & AG_DUMPFLAGS_STATE_ANIMLAYERS)
	{
		if (_pState->GetNumAnimLayers() > 0)
		{
			_pFile->Writeln(CStrF("%sAnimLayers:", Indent.Str()));
			CStr Indent2 = Indent + IndentBlank;
			for (int16 jAnimLayer = 0; jAnimLayer < _pState->GetNumAnimLayers(); jAnimLayer++)
			{
				int16 iAnimLayer = _pState->GetBaseAnimLayerIndex() + jAnimLayer;
				const CXRAG_AnimLayer* pAnimLayer = GetAnimLayer(iAnimLayer);
				CStr AnimLayerStr = CStrF("iAnim %d, iBaseJoint %d, TimeScale %3.3f", pAnimLayer->GetAnimIndex(), pAnimLayer->GetBaseJointIndex(), pAnimLayer->GetTimeScale());
				_pFile->Writeln(CStrF("%s[L%03d]  <%s>", Indent2.Str(), iAnimLayer, AnimLayerStr.Str()));
			}
		}
		else
		{
			_pFile->Writeln(CStrF("%sAnimLayers: (empty)", Indent.Str()));
		}
		_pFile->Writeln("");
	}

	if (_DumpFlags & AG_DUMPFLAGS_STATE_PROPERTIES)
	{
		_pFile->Writeln(CStrF("%siBaseNode = %d", Indent.Str(), _pState->GetBaseNodeIndex()));
		_pFile->Writeln(CStrF("%siBaseAction = %d", Indent.Str(), _pState->GetBaseActionIndex()));
		_pFile->Writeln(CStrF("%siBasePropertyParam = %d", Indent.Str(), _pState->GetBasePropertyParamIndex()));
		_pFile->Writeln("");
	}

	if (_DumpFlags & AG_DUMPFLAGS_STATE_NODETREES)
	{
		int16 iNode = _pState->GetBaseNodeIndex();
		if (iNode != AG_NODEINDEX_NULL)
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

//--------------------------------------------------------------------------------

void CXRAG::LogDump_Structure_Node(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, CStr _BranchStr, const CXRAG_State* _pState, int16 _iNode) const
{
	MAUTOSTRIP( CXRAG_LogDump_Structure_Node, MAUTOSTRIP_VOID );
	const CXRAG_ConditionNode* pNode = GetNode(_iNode);

	CStr ConditionStr;
	CStr PropertyParamsStr;
	if (pNode->GetNumPropertyParams() > 0)
	{
		int16 iParams = _pState->GetBasePropertyParamIndex() + pNode->GetPropertyParamsIndex();
		uint8 nParams = pNode->GetNumPropertyParams();
		const CXRAG_ICallbackParams IParams = GetICallbackParams(iParams, nParams);
		PropertyParamsStr = CStrF("(%s)", IParams.GetStr().Str());
	}

	CStr PropertyName = GetExportedPropertyNameFromID(pNode->GetProperty());
	if (PropertyName == "")
		CStrF("P%d", pNode->GetProperty());

	CStr OperatorName = GetExportedOperatorNameFromID(pNode->GetOperator());
	if (OperatorName == "")
		CStrF("O%d", pNode->GetOperator());

	ConditionStr = CStrF("%s%s %s %3.3f", PropertyName.Str(), PropertyParamsStr.Str(), OperatorName.Str(), pNode->GetConstant());

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

void CXRAG::LogDump_Structure_NodeAction(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, CStr _BranchStr, const CXRAG_State* _pState, int16 _NodeAction) const
{
	MAUTOSTRIP( CXRAG_LogDump_Structure_NodeAction, MAUTOSTRIP_VOID );
	if (_NodeAction == AG_NODEACTION_FAILPARSE)
	{
		_pFile->Writeln(CStrF("%s%s[FAIL]", (char*)_Indent, (char*)_BranchStr));
		return;
	}

	if ((_NodeAction & AG_NODEACTION_TYPEMASK) == AG_NODEACTION_ENDPARSE)
	{
		CAGActionIndex iBaseAction = _pState->GetBaseActionIndex();
		CAGActionIndex iAction = iBaseAction + (_NodeAction & AG_NODEACTION_ACTIONMASK);
		const CXRAG_Action* pAction = GetAction(iAction);
		CAGTokenID TokenID = pAction->GetTokenID();
		CAGStateIndex iTargetState = pAction->GetTargetStateIndex();
		if (iTargetState != AG_STATEINDEX_NULL)
		{
			CStr TokenName;
			if (TokenID == AG_TOKENID_NULL)
				TokenName = "TokenCUR";
			else
				TokenName = CStrF("Token%d", TokenID);

			CStr TargetStateName;
			if (iTargetState == AG_STATEINDEX_TERMINATE)
				TargetStateName = "TERMINATE";
			else
				TargetStateName = CStrF("State%d (%s)", iTargetState, GetExportedStateName(iTargetState).Str());

			_pFile->Writeln(CStrF("%s%s[A%03d]  %s -> %s", (char*)_Indent, (char*)_BranchStr, iAction, (char*)TokenName, (char*)TargetStateName));
		}
		else
			_pFile->Writeln(CStrF("%s%s[A%03d]", (char*)_Indent, (char*)_BranchStr, iAction));

		if ((_DumpFlags & AG_DUMPFLAGS_STATE_NODETREEEFFECTS) && (pAction->GetNumEffectInstances() > 0))
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
				CAGEffectInstanceIndex iEffectInstance = pAction->GetBaseEffectInstanceIndex() + jEffectInstance;
				const CXRAG_EffectInstance* pEffectInstance = GetEffectInstance(iEffectInstance);
				CAGEffectID EffectID = pEffectInstance->m_ID;

				CStr EffectName = GetExportedEffectNameFromID(EffectID);
				if (EffectName == "")
					EffectName = CStrF(" E%03d", iEffectInstance);
				else
					EffectName = CStrF("[E%03d]  %s", iEffectInstance, EffectName.Str());

				CStr EffectParamsStr;
				if (pEffectInstance->m_nParams > 0)
				{
					CXRAG_ICallbackParams Params = GetICallbackParams(pEffectInstance->m_iParams, pEffectInstance->m_nParams);
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

	if ((_NodeAction & AG_NODEACTION_TYPEMASK) == AG_NODEACTION_PARSENODE)
	{
		int16 iNode = _pState->GetBaseNodeIndex() + (_NodeAction & AG_NODEACTION_NODEMASK);
		LogDump_Structure_Node(_pFile, _DumpFlags, _Indent, _BranchStr, _pState, iNode);
	}
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG::LogDump_Actions(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG_LogDump_Actions, MAUTOSTRIP_VOID );
	_pFile->Writeln(CStrF("%sActions:", _Indent.Str()));
	_pFile->Writeln(CStrF("{", _Indent.Str()));
	CStr Indent = _Indent + IndentBlank;
	for (int16 iAction = 0; iAction < GetNumActions(); iAction++)
	{
		const CXRAG_Action* pAction = GetAction(iAction);
		CStr TokenIDStr = ((pAction->GetTokenID() != AG_TOKENID_NULL) ? CStrF("%d", pAction->GetTokenID()) : CStr("CUR"));
		int16 iTargetState = pAction->GetTargetStateIndex();
		CStr TargetStateStr;
		if (iTargetState == AG_STATEINDEX_NULL)
			TargetStateStr = " (no move)";
		else if (iTargetState == AG_STATEINDEX_TERMINATE)
			TargetStateStr = " -> TERMINATE";
		else
		{
			TargetStateStr = GetExportedStateName(iTargetState).Str();
			TargetStateStr = CStrF(" -> State%d (%s)", iTargetState, TargetStateStr.Str());
		}

		CStr ActionStr = CStrF("Token%s%s, ABDuration %3.3f", TokenIDStr.Str(), TargetStateStr.Str(), pAction->GetAnimBlendDuration());
		_pFile->Writeln(CStrF("%sAction[A%03d] = <%s>", Indent.Str(), iAction, ActionStr.Str()));
	}
	_pFile->Writeln(CStrF("}", _Indent.Str()));

	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------

void CXRAG::LogDump_ConditionNodes(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG_LogDump_ConditionNodes, MAUTOSTRIP_VOID );
	_pFile->Writeln(CStrF("%sConditionNodes: (N/I)", _Indent.Str()));
	_pFile->Writeln("");
	return;

	int nNodes = GetNumNodes();
	if (nNodes > 0)
	{
		_pFile->Writeln(CStrF("%sConditionNodes:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iConditionNode = 0; iConditionNode < nNodes; iConditionNode++)
		{
			const CXRAG_ConditionNode* pNode = GetNode(iConditionNode);
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

void CXRAG::LogDump_AnimLayers(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG_LogDump_AnimLayers, MAUTOSTRIP_VOID );
	int nAnimLayers = GetNumAnimLayers();
	if (nAnimLayers > 0)
	{
		_pFile->Writeln(CStrF("%sAnimLayers:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iAnimLayer = 0; iAnimLayer < nAnimLayers; iAnimLayer++)
		{
			const CXRAG_AnimLayer* pAnimLayer = GetAnimLayer(iAnimLayer);
			CStr AnimLayerStr = CStrF("iAnim %d, iBaseJoint %d, TimeScale %3.3f", pAnimLayer->GetAnimIndex(), pAnimLayer->GetBaseJointIndex(), pAnimLayer->GetTimeScale());
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

//--------------------------------------------------------------------------------

void CXRAG::LogDump_EffectInstances(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG_LogDump_EffectInstances, MAUTOSTRIP_VOID );
	int nEffectInstances = GetNumEffectInstances();
	if (nEffectInstances > 0)
	{
		_pFile->Writeln(CStrF("%sEffectInstances:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iEffectInstance = 0; iEffectInstance < nEffectInstances; iEffectInstance++)
		{
			const CXRAG_EffectInstance* pEffectInstance = GetEffectInstance(iEffectInstance);
			CStr EffectInstanceStr;
			if (pEffectInstance->m_nParams > 0)
			{
				CXRAG_ICallbackParams IParams = GetICallbackParams(pEffectInstance->m_iParams, pEffectInstance->m_nParams);
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

void CXRAG::LogDump_StateConstants(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG_LogDump_StateConstants, MAUTOSTRIP_VOID );
	int nStateConstants = GetNumStateConstants();
	if (nStateConstants > 0)
	{
		_pFile->Writeln(CStrF("%sStateConstants:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iStateConstant = 0; iStateConstant < nStateConstants; iStateConstant++)
		{
			const CXRAG_StateConstant* pStateConstant = GetStateConstant(iStateConstant);
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

void CXRAG::LogDump_CallbackParams(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG_LogDump_CallbackParams, MAUTOSTRIP_VOID );
	int nCallbackParams = m_lCallbackParams.Len();
	if (nCallbackParams > 0)
	{
		_pFile->Writeln(CStrF("%sCallbackParams:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iParam = 0; iParam < nCallbackParams; iParam++)
			_pFile->Writeln(CStrF("%sParam[P%03d] = %3.3f", Indent.Str(), iParam, m_lCallbackParams[iParam]));
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sCallbackParams: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------

void CXRAG::LogDump_ExportedActions(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG_LogDump_ExportedActions, MAUTOSTRIP_VOID );
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

void CXRAG::LogDump_ExportedStates(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG_LogDump_ExportedStates, MAUTOSTRIP_VOID );
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

//--------------------------------------------------------------------------------

void CXRAG::LogDump_ExportedProperties(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG_LogDump_ExportedProperties, MAUTOSTRIP_VOID );
	int nExportedPropertys = m_lExportedPropertyNames.Len();
	if (nExportedPropertys > 0)
	{
		_pFile->Writeln(CStrF("%sExportedProperties:", _Indent.Str()));
		_pFile->Writeln(CStrF("{", _Indent.Str()));
		CStr Indent = _Indent + IndentBlank;
		for (int16 iExportedProperty = 0; iExportedProperty < nExportedPropertys; iExportedProperty++)
		{
			CStr ExportedPropertyName = GetExportedPropertyNameFromIndex(iExportedProperty);
			_pFile->Writeln(CStrF("%sProperty[P%d]: '%s' (ID N/A)", Indent.Str(), iExportedProperty, ExportedPropertyName.Str()));
		}
		_pFile->Writeln(CStrF("}", _Indent.Str()));
	}
	else
	{
		_pFile->Writeln(CStrF("%sExportedProperties: (Empty)", _Indent.Str()));
	}

	_pFile->Writeln("");
}

//--------------------------------------------------------------------------------

void CXRAG::LogDump_ExportedOperators(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG_LogDump_ExportedOperators, MAUTOSTRIP_VOID );
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

void CXRAG::LogDump_ExportedEffects(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG_LogDump_ExportedEffects, MAUTOSTRIP_VOID );
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

void CXRAG::LogDump_ExportedStateConstants(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const
{
	MAUTOSTRIP( CXRAG_LogDump_ExportedStateConstants, MAUTOSTRIP_VOID );
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

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
#endif //M_RTM