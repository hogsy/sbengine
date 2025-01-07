#include "PCH.h"

//--------------------------------------------------------------------------------

#include "../AnimGraph2Defs.h"
#include "AnimGraph2Compiler.h"

//--------------------------------------------------------------------------------

#define AG2C_SCOPESEPARATOR " . "

//--------------------------------------------------------------------------------

#define AG2C_CONDITION_SORTEDBIT		(1<<15)

extern const char* ms_lpAG2CAnimTimeOffsetTypes[];

//--------------------------------------------------------------------------------
bool CXRAG2C_GraphBlock::ParseReg(CXR_AnimGraph2Compiler* _pCompiler, const CRegistry* _pBlockReg)
{
	if (!_pCompiler || !_pBlockReg)
		return false;

	m_pCompiler = _pCompiler;
	m_Reactions.m_pAGCompiler = _pCompiler;

	m_Name = _pBlockReg->GetThisValue().UpperCase();

	CStr Location = CStrF("GraphBlock: %s",m_Name.GetStr());

	for (int iChild = 0; iChild < _pBlockReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pBlockReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "IMPULSETYPE")
		{
			// Find impulsetype
			if (!m_pCompiler->ResolveImpulseType(pChildReg->GetThisValue(),m_Condition.m_ImpulseType,Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "IMPULSEVALUE")
		{
			// Find impulsevalue
			if (!m_pCompiler->ResolveImpulseValue(pChildReg->GetThisValue(),m_Condition.m_ImpulseValue,Location))
				return false;
		}	
		else if (pChildReg->GetThisName() == "EXTERNAL")
		{
			if (!IncludeStatesFile(m_pCompiler->GetFilePath() + pChildReg->GetThisValue()))
				return false;
		}
		else if (pChildReg->GetThisName() == "STATES")
		{
			if (!ParseStatesReg(pChildReg))
				return false;
		}
		else if (pChildReg->GetThisName() == "SWITCHSTATES")
		{
			if (!ParseSwitchStatesReg(pChildReg))
				return false;
		}
		else if (pChildReg->GetThisName() == "IMPULSE")
		{
			if (!m_Condition.ParseReg(pChildReg))
				return false;
		}
		else if (pChildReg->GetThisName() == "STARTSTATE")
		{
			//m_StartState =  pChildReg->GetThisValue().UpperCase();
			// Parse movetoken for startstate
			if (!m_Reactions.ParseMoveTokenReg(pChildReg, Location,m_StartMoveToken))
				return false;
		}
		else if (pChildReg->GetThisName() == "DEPENDENCY")
		{
			// What other block this block depends on
			OnMsg(CStrF("Dependency on: %s Dependency not made yet!!",pChildReg->GetThisValue().GetStr()).GetStr());
		}
		else if (pChildReg->GetThisName() == "REACTIONS")
		{
			if (!m_Reactions.ParseReg(this, pChildReg))
				return false;
		}
		else if (pChildReg->GetThisName() == "CONSTANTS")
		{
			if (!ParseGraphBlockConstantsReg(pChildReg))
				return false;
		}
	}
	return true;
}

bool CXRAG2C_GraphBlock::ParseGraphBlockConstantsReg(const CRegistry* _pGBConstantsReg)
{
	if (_pGBConstantsReg == NULL)
		return false;

	CStr Location = "(" + m_Name + AG2C_SCOPESEPARATOR + "CONSTANTS)";

	for (int iChild = 0; iChild < _pGBConstantsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pGBConstantsReg->GetChild(iChild);
		CStr SCName = pChildReg->GetThisName();

		uint32 SCID = m_pCompiler->GetStateConstantID(SCName);
		if (SCID == -1)
		{
			m_pCompiler->OnError(CStrF("Undefined stateconstant '%s' (%s).", (char*)SCName, (char*)Location));
			return false;
		}

		fp32 SCValue;
		if (!m_pCompiler->EvalConstantExpressionFloat(pChildReg->GetThisValue(), SCValue,NULL, Location))
			return false;

		CXRAG2C_StateConstant StateConstant(SCName, SCID, SCValue);
		m_lConstants.Add(StateConstant);
	}
/*
	for (int iConstant = 0; iConstant < m_lConstants.Len(); iConstant++)
	{
		int32* pConstantID = &(m_lConstants[iConstant].m_ID);
		fp32* pConstantValue = &(m_lConstants[iConstant].m_Value);
		int Apa = 5;
	}
*/
	return true;
}
bool CXRAG2C_GraphBlock::AcquireTargetState(CXRAG2C_MoveToken& _MoveToken)
{
	for (int jState = 0; jState < m_lspStates.Len(); jState++)
	{
		CFStr StateName = m_lspStates[jState]->m_Name;
		if (_MoveToken.m_TargetState == StateName)
		{
			_MoveToken.m_iTargetState = jState;
			_MoveToken.m_TargetStateType = AG2_STATETYPE_NORMAL;
			m_lspStates[jState]->m_References++;
			return true;
		}
	}

	for (int jState = 0; jState < m_lspSwitchStates.Len(); jState++)
	{
		CFStr StateName = m_lspSwitchStates[jState]->m_Name;
		if (_MoveToken.m_TargetState == StateName)
		{
			_MoveToken.m_iTargetState = jState;
			_MoveToken.m_TargetStateType = AG2_STATETYPE_SWITCH;
			m_lspSwitchStates[jState]->m_References++;
			return true;
		}
	}

	return false;
}

bool CXRAG2C_GraphBlock::ParseStatesReg(const CRegistry* _pStatesReg)
{
	for (int iChild = 0; iChild < _pStatesReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pStatesReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "EXTERNAL")
		{
			if (!IncludeStatesFile(m_pCompiler->GetFilePath() + pChildReg->GetThisValue()))
				return false;
		}
		else
		{
			spCXRAG2C_State spState = MNew(CXRAG2C_State);
			if (!spState->ParseReg(this, pChildReg))
				return false;

			m_lspStates.Add(spState);
		}
	}
	return true;
}

bool CXRAG2C_GraphBlock::ParseSwitchStatesReg(const CRegistry* _pStatesReg)
{
	for (int iChild = 0; iChild < _pStatesReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pStatesReg->GetChild(iChild);

		// No externals here
		spCXRAG2C_SwitchState spState = MNew(CXRAG2C_SwitchState);
		if (!spState->ParseReg(this, pChildReg))
			return false;

		m_lspSwitchStates.Add(spState);
	}
	return true;
}

//--------------------------------------------------------------------------------
void CXRAG2C_Common::PushFileName(CStr _Filename)
{
	m_lFilenames.Insert(0, _Filename);
}

void CXRAG2C_Common::PopFileName()
{
	if (m_lFilenames.Len() > 0)
		m_lFilenames.Del(0);
}

CStr CXRAG2C_Common::GetFileName()
{
	if (m_lFilenames.Len() > 0)
		return m_lFilenames[0];
	else
		return "";
}

CStr CXRAG2C_Common::GetFilePath()
{
	return GetFileName().GetPath();
}

CStr CXRAG2C_Common::ResolvePath(CStr _Path)
{
	CStr Result;

	TArray<CFStr> Dirs;
	while (_Path != "")
	{
		CFStr Dir = _Path.GetStrSep("\\");
		if (Dir != "..")
			Dirs.Add(Dir);
		else
			Dirs.Del(Dirs.Len() - 1);
	}

	for (int iDir = 0; iDir < Dirs.Len(); iDir++)
	{
		if (Result == "")
			Result = Result + Dirs[iDir];
		else
			Result = Result + "\\" + Dirs[iDir];
	}

	return Result;
}

void CXRAG2C_Common::OnError(const char* _pMsg)
{
	ConOutL(CStrF("(AnimGraph2Compiler) ERROR: %s", _pMsg));
	//Error("AnimGraphCompiler", _pMsg);
}

void CXRAG2C_Common::OnWarning(const char* _pMsg)
{
	ConOutL(CStrF("(AnimGraph2Compiler) Warning: %s", _pMsg));
}

void CXRAG2C_Common::OnNote(const char* _pMsg)
{
	ConOutL(CStrF("(AnimGraph2Compiler) Note: %s", _pMsg));
}

void CXRAG2C_Common::OnMsg(const char* _pMsg)
{
	ConOutL(CStrF("(AnimGraph2Compiler) %s", _pMsg));
}

bool CXRAG2C_GraphBlock::ParseGeneratedStateReg(const CRegistry* _pStateReg)
{
	spCXRAG2C_State spState = MNew(CXRAG2C_State);
	m_lspStates.Add(spState);

	OnNote("Generating intermediate state '" + _pStateReg->GetThisName() + "'...");
	if (!spState->ParseReg(this, _pStateReg))
		return false;

	return true;
}

bool CXRAG2C_GraphBlock::IncludeStatesFile(CStr _FileName)
{
	_FileName = ResolvePath(_FileName);
	if (_FileName == GetFileName())
	{
		OnError(CStrF("Cyclic inclusion of state file '%s'.", (char*)_FileName));
		return false;
	}

	M_TRY
	{
		CRegistry_Dynamic IncludeReg;
		IncludeReg.XRG_Read(_FileName);
		if ((IncludeReg.GetNumChildren() >= 1) &&
			(IncludeReg.GetChild(0)->GetThisName() == "ANIMGRAPH2"))
		{
			bool bStatesRegFound = false;
			CStr Name = m_Name;
			for (int iChild = 0; iChild < IncludeReg.GetChild(0)->GetNumChildren(); iChild++)
			{
				if (IncludeReg.GetChild(0)->GetChild(iChild)->GetThisName() == "STATES")
				{
					FILENAMESCOPE2GB(_FileName);
					bStatesRegFound = true;
					if (!ParseStatesReg(IncludeReg.GetChild(0)->GetChild(iChild)))
						return false;
				}
				else if (IncludeReg.GetChild(0)->GetChild(iChild)->GetThisName() == "SWITCHSTATES")
				{
					FILENAMESCOPE2GB(_FileName);
					bStatesRegFound = true;
					if (!ParseSwitchStatesReg(IncludeReg.GetChild(0)->GetChild(iChild)))
						return false;
				}
				else if ((IncludeReg.GetChild(0)->GetChild(iChild)->GetThisName() == "GRAPHBLOCK") &&
					(IncludeReg.GetChild(0)->GetChild(iChild)->GetThisValue() == Name))
				{
					FILENAMESCOPE2GB(_FileName);
					// Find states reg in this graphblock, ignore the rest since it's just included
					bStatesRegFound = true;
					CRegistry* pReg = IncludeReg.GetChild(0)->FindChild("STATES");
					if (!pReg || !ParseStatesReg(pReg))
						return false;
				}
				else if ((IncludeReg.GetChild(0)->GetChild(iChild)->GetThisName() == "DECLARATIONS"))
				{
					FILENAMESCOPE2GB(_FileName);
					// Include declarations
					if (!m_pCompiler->ParseDeclarationsReg(IncludeReg.GetChild(0)->GetChild(iChild)))
						return false;
				}
			}

			if (bStatesRegFound)
				OnNote(CStrF("States file '%s' successfully included (from '%s').", (char*)_FileName, (char*)m_pCompiler->GetFileName()));
			else
				OnWarning(CStrF("States file '%s' is an AnimGraph, but with no states.", (char*)_FileName));
		}
		else
		{
			OnError(CStrF("States file '%s' is not an AnimGraph.", (char*)_FileName));
			return false;
		}
		return true;
	}
	M_CATCH(
	catch (CCException)
	{
		OnError(CStrF("States file '%s' can't be found.", (char*)_FileName));
		return false;
	}
	)
}

bool CXRAG2C_GraphBlock::PreProcess()
{
	// Resolve startstate.
	if (m_StartMoveToken.m_TargetState == "")
	{
		OnError("Starting state not specified.");
		return false;
	}
	else
	{
		m_StartMoveToken.m_iTargetState = AG2_STATEINDEX_NULL;
		AcquireTargetState(m_StartMoveToken);
		/*for (int iState = 0; iState < m_lspStates.Len(); iState++)
		{
			//			CStr Name = m_lspStates[iState]->m_Name;
			if (m_StartMoveToken.m_TargetState == m_lspStates[iState]->m_Name)
			{
				m_StartMoveToken.m_iTargetState = iState;
				m_lspStates[iState]->m_References++;
			}
		}*/

		if (m_StartMoveToken.m_iTargetState == AG2_STATEINDEX_NULL)
		{
			OnError("Specified starting state '" + m_StartMoveToken.m_TargetState + "' is not defined." + CStrF("(%s)",m_Name.Str()));
			return false;
		}
	}
	return true;
}

bool CXRAG2C_GraphBlock::ProcessPass1()
{

	// Check so that condition exists
	if (m_Condition.m_ImpulseType == AG2_IMPULSETYPE_UNDEFINED)
	{
		OnError(CStrF("ImpulseType not defined in GraphBlock: %s",m_Name.Str()).Str());
		return false;
	}

	for (int32 iState = 0; iState < m_lspStates.Len(); iState++)
	{
		if (!m_lspStates[iState]->ProcessPass1())
			return false;
	}

	for (int32 iSwitchState = 0; iSwitchState < m_lspSwitchStates.Len(); iSwitchState++)
	{
		if (!m_lspSwitchStates[iSwitchState]->ProcessPass1())
			return false;
	}

	// Process reactions (name same as the graphblock)
	m_Reactions.m_Name = m_Name;
	if (!m_Reactions.Process())
		return false;

	return true;
}

bool CXRAG2C_GraphBlock::ProcessPostPass1()
{
	// Remove states with no references to them (i.e. not a targetstate/startstate).
	{
		int jState = 0;
		// Build remap indices.
		for (int32 iState = 0; iState < m_lspStates.Len(); iState++)
		{
			if (m_lspStates[iState]->m_References == 0)
			{
				m_lspStates[iState]->m_Index = AG2_STATEINDEX_NULL;
				// Remove reference to animation
				for (int32 i = 0; i < m_lspStates[iState]->m_lStateAnims.Len(); i++)
					m_pCompiler->m_lAnimations[m_lspStates[iState]->m_lStateAnims[i].m_iAnim].m_nReferences--;
			}
			else
			{
				m_lspStates[iState]->m_Index = jState;
				jState++;
			}
		}

		jState = 0;
		// Build remap indices.
		for (int32 iSwitchState = 0; iSwitchState < m_lspSwitchStates.Len(); iSwitchState++)
		{
			if (m_lspSwitchStates[iSwitchState]->m_References == 0)
			{
				m_lspSwitchStates[iSwitchState]->m_Index = AG2_STATEINDEX_NULL;
			}
			else
			{
				m_lspSwitchStates[iSwitchState]->m_Index = jState;
				jState++;
			}
		}

		// Remap startstate.
		if (!m_StartMoveToken.m_TargetStateType)
			m_StartMoveToken.m_iTargetState = m_lspStates[m_StartMoveToken.m_iTargetState]->m_Index;
		else
			m_StartMoveToken.m_iTargetState = m_lspSwitchStates[m_StartMoveToken.m_iTargetState]->m_Index;
	}

	return true;
}

bool CXRAG2C_GraphBlock::ProcessPrePass2()
{
	// Remap states.
	for (int32 iState = 0; iState < m_lspStates.Len(); iState++)
	{
		for (int iAction = 0; iAction < m_lspStates[iState]->m_lActions.Len(); iAction++)
		{
//			CXRAG2C_Action* pAction = &(m_lspStates[iState]->m_lActions[iAction]);
			for (int32 iMT = 0; iMT < m_lspStates[iState]->m_lActions[iAction].m_lMoveTokens.Len(); iMT++)
			{
				int iTargetGraphBlock = m_pCompiler->GetGraphBlockIndex(m_lspStates[iState]->m_lActions[iAction].m_lMoveTokens[iMT].m_TargetGraphBlock);

				int OldIndex = m_lspStates[iState]->m_lActions[iAction].m_lMoveTokens[iMT].m_iTargetState;

				if (!(m_lspStates[iState]->m_lActions[iAction].m_lMoveTokens[iMT].m_TargetStateType))
				{
					TArray<spCXRAG2C_State>& lspStates = iTargetGraphBlock == -1 ? m_lspStates : m_pCompiler->m_lspGraphBlocks[iTargetGraphBlock]->m_lspStates;

					if (!lspStates.Len())
					{
						OnError(CStrF("Target Graphblock(%d): %s Has no states!",iTargetGraphBlock,m_pCompiler->m_lspGraphBlocks[iTargetGraphBlock]->m_Name.GetStr()));
						return false;
					}

					if ((OldIndex != AG2_STATEINDEX_NULL) && (OldIndex != AG2_STATEINDEX_TERMINATE) && (OldIndex != AG2_STATEINDEX_STARTAG))
					{
						int NewIndex = lspStates[OldIndex]->m_Index;
						m_lspStates[iState]->m_lActions[iAction].m_lMoveTokens[iMT].m_iTargetState = NewIndex;
					}
				}
				else
				{
					TArray<spCXRAG2C_SwitchState>& lspSwitchStates = iTargetGraphBlock == -1 ? m_lspSwitchStates : m_pCompiler->m_lspGraphBlocks[iTargetGraphBlock]->m_lspSwitchStates;

					if ((OldIndex != AG2_STATEINDEX_NULL) && (OldIndex != AG2_STATEINDEX_TERMINATE) && (OldIndex != AG2_STATEINDEX_STARTAG))
					{
						int NewIndex = lspSwitchStates[OldIndex]->m_Index;
						m_lspStates[iState]->m_lActions[iAction].m_lMoveTokens[iMT].m_iTargetState = NewIndex;
					}
				}
			}
		}
	}

	// Remap switch states.
	for (int32 iSwitchState = 0; iSwitchState < m_lspSwitchStates.Len(); iSwitchState++)
	{
		int32 NumActions = m_lspSwitchStates[iSwitchState]->m_lActions.Len();
		for (int iAction = 0; iAction < NumActions+1; iAction++)
		{
			CXRAG2C_Action* pAction = iAction < NumActions ? &(m_lspSwitchStates[iSwitchState]->m_lActions[iAction]) : &(m_lspSwitchStates[iSwitchState]->m_DefaultAction);
			for (int32 iMT = 0; iMT < pAction->m_lMoveTokens.Len(); iMT++)
			{
				int iTargetGraphBlock = m_pCompiler->GetGraphBlockIndex(pAction->m_lMoveTokens[iMT].m_TargetGraphBlock);
				int OldIndex = pAction->m_lMoveTokens[iMT].m_iTargetState;

				if (!(pAction->m_lMoveTokens[iMT].m_TargetStateType))
				{
					TArray<spCXRAG2C_State>& lspStates = iTargetGraphBlock == -1 ? m_lspStates : m_pCompiler->m_lspGraphBlocks[iTargetGraphBlock]->m_lspStates;	

					if ((OldIndex != AG2_STATEINDEX_NULL) && (OldIndex != AG2_STATEINDEX_TERMINATE) && (OldIndex != AG2_STATEINDEX_STARTAG))
					{
						int NewIndex = lspStates[OldIndex]->m_Index;
						pAction->m_lMoveTokens[iMT].m_iTargetState = NewIndex;
					}
				}
				else
				{
					TArray<spCXRAG2C_SwitchState>& lspSwitchStates = iTargetGraphBlock == -1 ? m_lspSwitchStates : m_pCompiler->m_lspGraphBlocks[iTargetGraphBlock]->m_lspSwitchStates;

					if ((OldIndex != AG2_STATEINDEX_NULL) && (OldIndex != AG2_STATEINDEX_TERMINATE) && (OldIndex != AG2_STATEINDEX_STARTAG))
					{
						int NewIndex = lspSwitchStates[OldIndex]->m_Index;
						pAction->m_lMoveTokens[iMT].m_iTargetState = NewIndex;
					}
				}
			}
		}
	}

	// Remap reactions
	for (int32 i = 0; i < m_Reactions.m_lReactions.Len(); i++)
	{
		for (int32 iMT = 0; iMT < m_Reactions.m_lReactions[i].m_lMoveTokens.Len(); iMT++)
		{
			int iTargetGraphBlock = m_pCompiler->GetGraphBlockIndex(m_Reactions.m_lReactions[i].m_lMoveTokens[iMT].m_TargetGraphBlock);
			TArray<spCXRAG2C_State>& lspStates = iTargetGraphBlock == -1 ? m_lspStates : m_pCompiler->m_lspGraphBlocks[iTargetGraphBlock]->m_lspStates;
			TArray<spCXRAG2C_SwitchState>& lspSwitchStates = iTargetGraphBlock == -1 ? m_lspSwitchStates : m_pCompiler->m_lspGraphBlocks[iTargetGraphBlock]->m_lspSwitchStates;

			int OldIndex = m_Reactions.m_lReactions[i].m_lMoveTokens[iMT].m_iTargetState;

			if ((OldIndex != AG2_STATEINDEX_NULL) && (OldIndex != AG2_STATEINDEX_TERMINATE) && (OldIndex != AG2_STATEINDEX_STARTAG))
			{
				int NewIndex = -1;
				if (!m_Reactions.m_lReactions[i].m_lMoveTokens[iMT].m_TargetStateType)
					NewIndex = lspStates[OldIndex]->m_Index;
				else
					NewIndex = lspSwitchStates[OldIndex]->m_Index;

				m_Reactions.m_lReactions[i].m_lMoveTokens[iMT].m_iTargetState = NewIndex;
			}
		}
	}

	return true;
}

bool CXRAG2C_GraphBlock::ProcessPass2()
{
	// Delete unref'ed states.
	for (int32 iState = 0; iState < m_lspStates.Len();)
	{
		if (m_lspStates[iState]->m_References == 0)
		{
			m_lspStates.Del(iState);
		}
		else
		{
			iState++;
		}
	}

	for (int32 iSwitchState = 0; iSwitchState < m_lspSwitchStates.Len();)
	{
		if (m_lspSwitchStates[iSwitchState]->m_References == 0)
		{
			m_lspSwitchStates.Del(iSwitchState);
		}
		else
		{
			iSwitchState++;
		}
	}


	for (int32 iState = 0; iState < m_lspStates.Len(); iState++)
	{
		if (!m_lspStates[iState]->ProcessPass2())
			return false;
	}

	for (int32 iSwitchState = 0; iSwitchState < m_lspSwitchStates.Len(); iSwitchState++)
	{
		if (!m_lspSwitchStates[iSwitchState]->ProcessPass2())
			return false;
	}

	return true;
}
