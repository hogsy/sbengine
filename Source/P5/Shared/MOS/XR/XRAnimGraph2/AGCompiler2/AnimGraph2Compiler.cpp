#include "PCH.h"

//--------------------------------------------------------------------------------

#include "../AnimGraph2Defs.h"
#include "AnimGraph2Compiler.h"

#ifdef PLATFORM_WIN
	#pragma  inline_depth(0)
#endif
//--------------------------------------------------------------------------------

#define AG2C_SCOPESEPARATOR " . "

//--------------------------------------------------------------------------------

#define AG2C_CONDITION_SORTEDBIT		(1<<15)

//--------------------------------------------------------------------------------

const char* ms_lpAG2CAnimTimeOffsetTypes[] = { "STATIC", "INHERIT", "ROLLBACKBLEND", NULL };


//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::ParseReg(const CRegistry* _pRootReg, CXR_AnimGraph2* _pAnimGraph, CStr _FileName, CStr& _DestFileName)
{
	bool bSuccess = true;
	
	if (_pRootReg == NULL)
		return false;

	if (_pAnimGraph == NULL)
		return false;

	_DestFileName = _FileName.GetPath() + _FileName.GetFilenameNoExt() + ".xag";

	m_pAnimGraph = _pAnimGraph;
	PushFileName(_FileName);
	OnNote(CStrF("Parsing file '%s'...", (char*)_FileName));

	m_Name = _FileName.GetFilenameNoExt();

	for (int iChild = 0; iChild < _pRootReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pRootReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "NAME")
		{
			m_Name = pChildReg->GetThisValue();
		}
		else if (pChildReg->GetThisName() == "STARTBLOCK")
		{
			m_StartBlock = pChildReg->GetThisValue().UpperCase();
		}
		else if (pChildReg->GetThisName() == "OUTPUTFILE")
		{
			_DestFileName = ResolvePath(_FileName.GetPath() + pChildReg->GetThisValue());
		}
		else if (pChildReg->GetThisName() == "DECLARATIONS")
		{
			if (!ParseDeclarationsReg(pChildReg))
			{
				bSuccess = false;
				break;
			}
		}
		else if (pChildReg->GetThisName() == "GRAPHBLOCK")
		{
			if (!ParseGraphBlockReg(pChildReg))
			{
				bSuccess = false;
				break;
			}
		}
		else if (pChildReg->GetThisName() == "EXTERNAL")
		{
			// External graphblocks
			if (!IncludeGraphBlockFile(GetFilePath() + pChildReg->GetThisValue()))
			{
				bSuccess = false;
				break;
			}
		}
		else if (pChildReg->GetThisName() == "EDITPROPERTIES")
		{
			// Ignore editor-only keys.
		}
		else
		{
			OnWarning(CStrF("Unrecognized keyword '%s' (root).", (char*)pChildReg->GetThisName()));
		}
	}

	if (bSuccess)
	{
		OnNote("Processing...");
		if (!Process())
			bSuccess = false;
	}

	if (bSuccess)
	{
		OnNote("Converting...");
		if (!Convert())
			bSuccess = false;
	}

	if (bSuccess)
	{

		CStr LogFileName = _FileName.GetPath() + _FileName.GetFilenameNoExt() + ".log";
		//DumpLog(LogFileName);
#ifndef M_RTM
		m_pAnimGraph->LogDump(LogFileName, AG2_DUMPFLAGS_ALL);
#endif

		OnNote("Checking consistency...");
		CStr Result = m_pAnimGraph->CheckConsistency(false);
		if (Result != "")
		{
			OnError("Inconsistent AnimGraph. " + Result);
			bSuccess = false;
		}
	}

	m_lPropertiesFloat.Clear();
	m_lPropertiesInt.Clear();
	m_lPropertiesBool.Clear();
	m_lPropertiesCondition.Clear();
	m_lOperators.Clear();
	m_lEffects.Clear();
	m_lspGraphBlocks.Clear();

	if (bSuccess)
		OnMsg("'" + _FileName + "' Done!");
	else
		OnMsg("Failed to convert '" + _FileName + "'.");
	
	return bSuccess;
}

bool CXR_AnimGraph2Compiler::ParseGraphBlockReg(const CRegistry* _pBlockReg)
{
	spCXRAG2C_GraphBlock spGraph = MNew(CXRAG2C_GraphBlock);
	if (!spGraph->ParseReg(this, _pBlockReg))
		return false;

	// Make sure it doesn't have a name that clashes
	for (int32 i = 0; i < m_lspGraphBlocks.Len(); i++)
	{
		if (m_lspGraphBlocks[i]->m_Name.UpperCase() == spGraph->m_Name.UpperCase())
		{
			OnError(CStrF("GraphBlock with same name not supported: %s",spGraph->m_Name.Str()));
			return false;
		}
	}

	m_lspGraphBlocks.Add(spGraph);

	/*for (int iChild = 0; iChild < _pBlockReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pBlockReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "EXTERNAL")
		{
			if (!IncludeGraphBlockFile(GetFilePath() + pChildReg->GetThisValue()))
				return false;
		}
		else
		{
			spCXRAG2C_GraphBlock spGraph = MNew(CXRAG2C_GraphBlock);
			if (!spGraph->ParseReg(this, pChildReg))
				return false;

			m_lspGraphBlocks.Add(spGraph);
		}
	}*/
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::IncludeGraphBlockFile(CStr _FileName)
{
	_FileName = ResolvePath(_FileName);
	if (_FileName == GetFileName())
	{
		OnError(CStrF("Cyclic inclusion of graphblock file '%s'.", (char*)_FileName));
		return false;
	}

	PushFileName(_FileName);

	M_TRY
	{
		CRegistry_Dynamic IncludeReg;
		IncludeReg.XRG_Read(_FileName);
		if ((IncludeReg.GetNumChildren() >= 1) &&
			(IncludeReg.GetChild(0)->GetThisName() == "ANIMGRAPH2"))
		{
			bool bStatesRegFound = false;
			for (int iChild = 0; iChild < IncludeReg.GetChild(0)->GetNumChildren(); iChild++)
			{
				if (IncludeReg.GetChild(0)->GetChild(iChild)->GetThisName() == "GRAPHBLOCK")
				{
					FILENAMESCOPE2(_FileName);
					bStatesRegFound = true;
					if (!ParseGraphBlockReg(IncludeReg.GetChild(0)->GetChild(iChild)))
					{
						PopFileName();
						return false;
					}
				}
				else if (IncludeReg.GetChild(0)->GetChild(iChild)->GetThisName() == "EXTERNAL")
				{
					if (!IncludeGraphBlockFile(GetFilePath() + IncludeReg.GetChild(0)->GetChild(iChild)->GetThisValue()))
					{
						PopFileName();
						return false;
					}
				}
				else if (IncludeReg.GetChild(0)->GetChild(iChild)->GetThisName() == "STATES")
				{
					// Include states in top graph block?? Error for now
					OnError(CStrF("AnimGraph file '%s' Has no GraphBlock", (char*)_FileName));
					/*if (m_lspGraphBlocks.Len() > 0)
						m_lspGraphBlocks[m_lspGraphBlocks.Len() - 1]->ParseStatesReg(IncludeReg.GetChild(0)->GetChild(iChild));*/
				}
			}

			if (bStatesRegFound)
				OnNote(CStrF("GraphBlock file '%s' successfully included (from '%s').", (char*)_FileName, (char*)GetFileName()));
			else
				OnWarning(CStrF("GraphBlock file '%s' is an AnimGraph, but with no states.", (char*)_FileName));
		}
		else
		{
			PopFileName();
			OnError(CStrF("GraphBlock file '%s' is not an AnimGraph.", (char*)_FileName));
			return false;
		}
		PopFileName();
		return true;
	}
	M_CATCH(
	catch (CCException)
	{
		PopFileName();
		OnError(CStrF("GraphBlock file '%s' can't be found.", (char*)_FileName));
		return false;
	}
	)
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::ParseDeclarationsReg(const CRegistry* _pDeclReg)
{
	if (_pDeclReg->GetThisValue() != "")
	{
		CFStr IncludeFile = GetFilePath() + _pDeclReg->GetThisValue();
		IncludeDeclarationsFile(IncludeFile);
	}

	M_TRY
	{
		for (int iChild = 0; iChild < _pDeclReg->GetNumChildren(); iChild++)
		{
			const CRegistry* pChildReg = _pDeclReg->GetChild(iChild);
			if (pChildReg->GetThisName() == "EXTERNAL")
			{
				if (!IncludeDeclarationsFile(GetFilePath() + pChildReg->GetThisValue()))
					return false;
			}
			else if (pChildReg->GetThisName() == "PROPERTIESCONDITION")
			{
				if (!ParsePropertiesConditionReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "PROPERTIESFUNCTION")
			{
				if (!ParsePropertiesFunctionReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "PROPERTIESFLOAT")
			{
				if (!ParsePropertiesFloatReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "PROPERTIESINT")
			{
				if (!ParsePropertiesIntReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "PROPERTIESBOOL")
			{
				if (!ParsePropertiesBoolReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "OPERATORS")
			{
				if (!ParseOperatorsReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "EFFECTS")
			{
				if (!ParseEffectsReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "STATECONSTANTS")
			{
				if (!ParseStateConstantsReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "STATEFLAGS")
			{
				if (!ParseStateFlagsReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "ANIMFLAGS")
			{
				if (!ParseAnimFlagsReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "ANIMATIONS")
			{
				if (!ParseAnimationsReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "IMPULSETYPES")
			{
				if (!ParseImpulseTypeReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "IMPULSEVALUES")
			{
				if (!ParseImpulseValueReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "FLOATCONSTANTS")
			{
				if (!ParseFloatConstantsReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "INTCONSTANTS")
			{
				if (!ParseIntConstantsReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "BOOLCONSTANTS")
			{
				if (!ParseBoolConstantsReg(pChildReg))
					return false;
			}
			else
			{
				OnWarning(CStrF("Unrecognized keyword '%s' (DECLARATIONS).", (char*)pChildReg->GetThisName()));
			}
		}
	}
	M_CATCH(
	catch (CCException)
	{
		OnError("Declaration parsing failed.");
		return false;
	}
	)

	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::IncludeDeclarationsFile(CStr _FileName)
{
	_FileName = ResolvePath(_FileName);
	M_TRY
	{
		CRegistry_Dynamic IncludeReg;
		IncludeReg.XRG_Read(_FileName);
		if ((IncludeReg.GetNumChildren() >= 1) &&
			(IncludeReg.GetChild(0)->GetThisName() == "ANIMGRAPH2"))
		{
			bool bDeclarationsRegFound = false;
			for (int iChild = 0; iChild < IncludeReg.GetChild(0)->GetNumChildren(); iChild++)
			{
				if (IncludeReg.GetChild(0)->GetChild(iChild)->GetThisName() == "DECLARATIONS")
				{
					FILENAMESCOPE2(_FileName);
					bDeclarationsRegFound = true;
					if (!ParseDeclarationsReg(IncludeReg.GetChild(0)->GetChild(iChild)))
						return false;
				}
			}

			if (bDeclarationsRegFound)
				OnNote(CStrF("Declarations file '%s' successfully included (from '%s').", (char*)_FileName, (char*)GetFileName()));
			else
				OnWarning(CStrF("Declarations file '%s' is an AnimGraph, but with no declarations.", (char*)_FileName));
		}
		else
		{
			OnError(CStrF("Declarations file '%s' is not an AnimGraph.", (char*)_FileName));
			return false;
		}
		return true;
	}
	M_CATCH(
	catch (CCException)
	{
		OnError(CStrF("Declarations file '%s' can't be found.", (char*)_FileName));
		return false;
	}
	)
}

//--------------------------------------------------------------------------------
bool CXR_AnimGraph2Compiler::ParsePropertiesConditionReg(const CRegistry* _pPropReg)
{
	for (int iChild = 0; iChild < _pPropReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pPropReg->GetChild(iChild);
		CXRAG2C_PropertyCondition Property(pChildReg->GetThisName(), pChildReg->GetThisValuei());
		int iProperty = GetPropertyConditionIndexByID(Property.m_ID);
		if (iProperty != -1)
		{
			if (Property.m_Name != m_lPropertiesCondition[iProperty].m_Name)
			{
				OnWarning(CStrF("New property '%s' replaces old property '%s' (slot %d)", (char*)Property.m_Name, (char*)m_lPropertiesCondition[iProperty].m_Name, Property.m_ID));
				m_lPropertiesCondition.Del(iProperty);
				m_lPropertiesCondition.Add(Property);
			}
		}
		else
		{
			m_lPropertiesCondition.Add(Property);
		}
	}
	return true;
}

bool CXR_AnimGraph2Compiler::ParsePropertiesFunctionReg(const CRegistry* _pPropReg)
{
	for (int iChild = 0; iChild < _pPropReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pPropReg->GetChild(iChild);
		CXRAG2C_PropertyFunction Property(pChildReg->GetThisName(), pChildReg->GetThisValuei());
		int iProperty = GetPropertyFunctionIndexByID(Property.m_ID);
		if (iProperty != -1)
		{
			if (Property.m_Name != m_lPropertiesFunction[iProperty].m_Name)
			{
				OnWarning(CStrF("New property '%s' replaces old property '%s' (slot %d)", (char*)Property.m_Name, (char*)m_lPropertiesFunction[iProperty].m_Name, Property.m_ID));
				m_lPropertiesFunction.Del(iProperty);
				m_lPropertiesFunction.Add(Property);
			}
		}
		else
		{
			m_lPropertiesFunction.Add(Property);
		}
	}
	return true;
}

bool CXR_AnimGraph2Compiler::ParsePropertiesFloatReg(const CRegistry* _pPropReg)
{
	for (int iChild = 0; iChild < _pPropReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pPropReg->GetChild(iChild);
		CXRAG2C_PropertyFloat Property(pChildReg->GetThisName(), pChildReg->GetThisValuei());
		int iProperty = GetPropertyFloatIndexByID(Property.m_ID);
		if (iProperty != -1)
		{
			if (Property.m_Name != m_lPropertiesFloat[iProperty].m_Name)
			{
				OnWarning(CStrF("New property '%s' replaces old property '%s' (slot %d)", (char*)Property.m_Name, (char*)m_lPropertiesFloat[iProperty].m_Name, Property.m_ID));
				m_lPropertiesFloat.Del(iProperty);
				m_lPropertiesFloat.Add(Property);
			}
		}
		else
		{
			m_lPropertiesFloat.Add(Property);
		}
	}
	return true;
}

bool CXR_AnimGraph2Compiler::ParsePropertiesIntReg(const CRegistry* _pPropReg)
{
	for (int iChild = 0; iChild < _pPropReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pPropReg->GetChild(iChild);
		CXRAG2C_PropertyInt Property(pChildReg->GetThisName(), pChildReg->GetThisValuei());
		int iProperty = GetPropertyIntIndexByID(Property.m_ID);
		if (iProperty != -1)
		{
			if (Property.m_Name != m_lPropertiesInt[iProperty].m_Name)
			{
				OnWarning(CStrF("New property '%s' replaces old property '%s' (slot %d)", (char*)Property.m_Name, (char*)m_lPropertiesInt[iProperty].m_Name, Property.m_ID));
				m_lPropertiesInt.Del(iProperty);
				m_lPropertiesInt.Add(Property);
			}
		}
		else
		{
			m_lPropertiesInt.Add(Property);
		}
	}
	return true;
}

bool CXR_AnimGraph2Compiler::ParsePropertiesBoolReg(const CRegistry* _pPropReg)
{
	for (int iChild = 0; iChild < _pPropReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pPropReg->GetChild(iChild);
		CXRAG2C_PropertyBool Property(pChildReg->GetThisName(), pChildReg->GetThisValuei());
		int iProperty = GetPropertyBoolIndexByID(Property.m_ID);
		if (iProperty != -1)
		{
			if (Property.m_Name != m_lPropertiesBool[iProperty].m_Name)
			{
				OnWarning(CStrF("New property '%s' replaces old property '%s' (slot %d)", (char*)Property.m_Name, (char*)m_lPropertiesBool[iProperty].m_Name, Property.m_ID));
				m_lPropertiesBool.Del(iProperty);
				m_lPropertiesBool.Add(Property);
			}
		}
		else
		{
			m_lPropertiesBool.Add(Property);
		}
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::ParseOperatorsReg(const CRegistry* _pOpReg)
{
	for (int iChild = 0; iChild < _pOpReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pOpReg->GetChild(iChild);
		CXRAG2C_Operator Operator(pChildReg->GetThisName(), pChildReg->GetThisValuei());
		int iOperator = GetOperatorIndexByID(Operator.m_ID);
		if (iOperator != -1)
		{
			if (Operator.m_Name != m_lOperators[iOperator].m_Name)
			{
				OnWarning(CStrF("New operator '%s' replaces old operator '%s' (slot %d)", (char*)Operator.m_Name, (char*)m_lOperators[iOperator].m_Name, Operator.m_ID));
				m_lOperators.Del(iOperator);
				m_lOperators.Add(Operator);
			}
		}
		else
		{
			m_lOperators.Add(Operator);
		}
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::ParseEffectsReg(const CRegistry* _pEffectReg)
{
	for (int iChild = 0; iChild < _pEffectReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pEffectReg->GetChild(iChild);
		CXRAG2C_Effect Effect(pChildReg->GetThisName(), pChildReg->GetThisValuei());
		int iEffect = GetEffectIndexByID(Effect.m_ID);
		if (iEffect != -1)
		{
			if (Effect.m_Name != m_lEffects[iEffect].m_Name)
			{
				OnWarning(CStrF("New effect '%s' replaces old effect '%s' (slot %d)", (char*)Effect.m_Name, (char*)m_lEffects[iEffect].m_Name, Effect.m_ID));
				m_lEffects.Del(iEffect);
				m_lEffects.Add(Effect);
			}
		}
		else
		{
			m_lEffects.Add(Effect);
		}
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::ParseStateConstantsReg(const CRegistry* _pStateConstantsReg)
{
	for (int iChild = 0; iChild < _pStateConstantsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pStateConstantsReg->GetChild(iChild);
		CXRAG2C_StateConstant StateConstant(pChildReg->GetThisName(), pChildReg->GetThisValuei());
		int iStateConstant = GetStateConstantIndexByID(StateConstant.m_ID);
		if (iStateConstant != -1)
		{
			if (StateConstant.m_Name != m_lStateConstants[iStateConstant].m_Name)
			{
				OnWarning(CStrF("New state constant '%s' replaces old state constant '%s' (slot %d)", (char*)StateConstant.m_Name, (char*)m_lStateConstants[iStateConstant].m_Name, StateConstant.m_ID));
				m_lFloatConstants.Del(iStateConstant);
				m_lStateConstants.Add(StateConstant);
			}
		}
		else
		{
			m_lStateConstants.Add(StateConstant);
		}
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::ParseFloatConstantsReg(const CRegistry* _pConstantsReg)
{
	CStr Location = CStr("DECLARATIONS") + AG2C_SCOPESEPARATOR + "FLOATCONSTANTS";
	for (int iChild = 0; iChild < _pConstantsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pConstantsReg->GetChild(iChild);
		fp32 ConstantValue;
		if (!EvalConstantExpressionFloat(pChildReg->GetThisValue(), ConstantValue, NULL, Location))
			return false;

		m_lFloatConstants.Add(CXRAG2C_Constant_Float(pChildReg->GetThisName(), ConstantValue));
	}
	return true;
}

bool CXR_AnimGraph2Compiler::ParseIntConstantsReg(const CRegistry* _pConstantsReg)
{
	CStr Location = CStr("DECLARATIONS") + AG2C_SCOPESEPARATOR + "INTCONSTANTS";
	for (int iChild = 0; iChild < _pConstantsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pConstantsReg->GetChild(iChild);
		int ConstantValue;
		if (!EvalConstantExpressionInt(pChildReg->GetThisValue(), ConstantValue, NULL, Location))
			return false;

		m_lIntConstants.Add(CXRAG2C_Constant_Int(pChildReg->GetThisName(), ConstantValue));
	}
	return true;
}

bool CXR_AnimGraph2Compiler::ParseBoolConstantsReg(const CRegistry* _pConstantsReg)
{
	CStr Location = CStr("DECLARATIONS") + AG2C_SCOPESEPARATOR + "BOOLCONSTANTS";
	for (int iChild = 0; iChild < _pConstantsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pConstantsReg->GetChild(iChild);
		bool ConstantValue;
		if (!EvalConstantExpressionBool(pChildReg->GetThisValue(), ConstantValue, NULL, Location))
			return false;

		m_lBoolConstants.Add(CXRAG2C_Constant_Bool(pChildReg->GetThisName(), ConstantValue));
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::ParseStateFlagsReg(const CRegistry* _pStateFlagsReg)
{
	for (int iChild = 0; iChild < _pStateFlagsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pStateFlagsReg->GetChild(iChild);
		m_lStateFlags.Add(CXRAG2C_StateFlag(pChildReg->GetThisName(), pChildReg->GetThisValuei()));
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::ParseAnimFlagsReg(const CRegistry* _pAnimFlagsReg)
{
	for (int iChild = 0; iChild < _pAnimFlagsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pAnimFlagsReg->GetChild(iChild);
		m_lAnimFlags.Add(CXRAG2C_AnimFlag(pChildReg->GetThisName(), pChildReg->GetThisValuei()));
	}
	return true;
}

bool CXR_AnimGraph2Compiler::ParseAnimationsReg(const CRegistry* _pAnimsReg)
{
	// Check if we already have this animation
	for (int iChild = 0; iChild < _pAnimsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pAnimsReg->GetChild(iChild);
		int32 iAnimIndex = GetAnimationIndex(pChildReg->GetThisName().UpperCase());
		if (iAnimIndex != -1)
		{
			// Replace the animation
			CStr AnimValue = pChildReg->GetThisValue().UpperCase();
			OnNote(CStrF("The animation: %s replaces %s", AnimValue.Str(), m_lAnimations[iAnimIndex].m_Value.Str()));
			m_lAnimations[iAnimIndex].m_Value = AnimValue;
		}
		else
		{
			// Just add the animation
			m_lAnimations.Add(CXRAG2C_Animation(pChildReg->GetThisName().UpperCase(), pChildReg->GetThisValue().UpperCase()));
		}
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::ParseImpulseValueReg(const CRegistry* _pImpulseReg)
{
	for (int iChild = 0; iChild < _pImpulseReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pImpulseReg->GetChild(iChild);
		int16 Val;
		if (!GetImpulseValue(pChildReg->GetThisName(),Val))
			m_lImpulseValues.Add(CXRAG2C_ImpulseConstant(pChildReg->GetThisName(), pChildReg->GetThisValuei()));
	}
	return true;
}

bool CXR_AnimGraph2Compiler::ParseImpulseTypeReg(const CRegistry* _pImpulseReg)
{
	for (int iChild = 0; iChild < _pImpulseReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pImpulseReg->GetChild(iChild);
		int16 Val;
		if (!GetImpulseType(pChildReg->GetThisName(),Val))
			m_lImpulseTypes.Add(CXRAG2C_ImpulseConstant(pChildReg->GetThisName(), pChildReg->GetThisValuei()));
	}
	return true;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraph2Compiler::GetPropertyConditionIndexByID(int32 _ID)
{
	for (int iProperty = 0; iProperty < m_lPropertiesCondition.Len(); iProperty++)
	{
		if (m_lPropertiesCondition[iProperty].m_ID == _ID)
		{
			m_lPropertiesCondition[iProperty].m_bReferenced = true;
			return iProperty;
		}
	}
	return -1;
}

int32 CXR_AnimGraph2Compiler::GetPropertyFunctionIndexByID(int32 _ID)
{
	for (int iProperty = 0; iProperty < m_lPropertiesFunction.Len(); iProperty++)
	{
		if (m_lPropertiesFunction[iProperty].m_ID == _ID)
		{
			m_lPropertiesFunction[iProperty].m_bReferenced = true;
			return iProperty;
		}
	}
	return -1;
}

int32 CXR_AnimGraph2Compiler::GetPropertyFloatIndexByID(int32 _ID)
{
	for (int iProperty = 0; iProperty < m_lPropertiesFloat.Len(); iProperty++)
	{
		if (m_lPropertiesFloat[iProperty].m_ID == _ID)
		{
			m_lPropertiesFloat[iProperty].m_bReferenced = true;
			return iProperty;
		}
	}
	return -1;
}

int32 CXR_AnimGraph2Compiler::GetPropertyIntIndexByID(int32 _ID)
{
	for (int iProperty = 0; iProperty < m_lPropertiesInt.Len(); iProperty++)
	{
		if (m_lPropertiesInt[iProperty].m_ID == _ID)
		{
			m_lPropertiesInt[iProperty].m_bReferenced = true;
			return iProperty;
		}
	}
	return -1;
}

int32 CXR_AnimGraph2Compiler::GetPropertyBoolIndexByID(int32 _ID)
{
	for (int iProperty = 0; iProperty < m_lPropertiesBool.Len(); iProperty++)
	{
		if (m_lPropertiesBool[iProperty].m_ID == _ID)
		{
			m_lPropertiesBool[iProperty].m_bReferenced = true;
			return iProperty;
		}
	}
	return -1;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraph2Compiler::GetOperatorIndexByID(int32 _ID)
{
	for (int iOperator = 0; iOperator < m_lOperators.Len(); iOperator++)
	{
		if (m_lOperators[iOperator].m_ID == _ID)
			return iOperator;
	}
	return -1;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraph2Compiler::GetEffectIndexByID(int32 _ID)
{
	for (int iEffect = 0; iEffect < m_lEffects.Len(); iEffect++)
	{
		if (m_lEffects[iEffect].m_ID == _ID)
			return iEffect;
	}
	return -1;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraph2Compiler::GetStateConstantIndexByID(int32 _ID)
{
	for (int iStateConstant = 0; iStateConstant < m_lStateConstants.Len(); iStateConstant++)
	{
		if (m_lStateConstants[iStateConstant].m_ID == _ID)
			return iStateConstant;
	}
	return -1;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraph2Compiler::GetPropertyConditionID(const char* _Name)
{
	for (int iProperty = 0; iProperty < m_lPropertiesCondition.Len(); iProperty++)
	{
		if (m_lPropertiesCondition[iProperty].m_Name == _Name)
		{
			m_lPropertiesCondition[iProperty].m_bReferenced = true;
			return m_lPropertiesCondition[iProperty].m_ID;
		}
	}
	return -1;
}

int32 CXR_AnimGraph2Compiler::GetPropertyFunctionID(const char* _Name)
{
	for (int iProperty = 0; iProperty < m_lPropertiesFunction.Len(); iProperty++)
	{
		if (m_lPropertiesFunction[iProperty].m_Name == _Name)
		{
			m_lPropertiesFunction[iProperty].m_bReferenced = true;
			return m_lPropertiesFunction[iProperty].m_ID;
		}
	}
	return -1;
}

int32 CXR_AnimGraph2Compiler::GetPropertyFloatID(const char* _Name)
{
	for (int iProperty = 0; iProperty < m_lPropertiesFloat.Len(); iProperty++)
	{
		if (m_lPropertiesFloat[iProperty].m_Name == _Name)
		{
			m_lPropertiesFloat[iProperty].m_bReferenced = true;
			return m_lPropertiesFloat[iProperty].m_ID;
		}
	}
	return -1;
}

int32 CXR_AnimGraph2Compiler::GetPropertyIntID(const char* _Name)
{
	for (int iProperty = 0; iProperty < m_lPropertiesInt.Len(); iProperty++)
	{
		if (m_lPropertiesInt[iProperty].m_Name == _Name)
		{
			m_lPropertiesInt[iProperty].m_bReferenced = true;
			return m_lPropertiesInt[iProperty].m_ID;	
		}
	}
	return -1;
}

int32 CXR_AnimGraph2Compiler::GetPropertyBoolID(const char* _Name)
{
	for (int iProperty = 0; iProperty < m_lPropertiesBool.Len(); iProperty++)
	{
		if (m_lPropertiesBool[iProperty].m_Name == _Name)
		{
			m_lPropertiesBool[iProperty].m_bReferenced = true;
			return m_lPropertiesBool[iProperty].m_ID;
		}
	}
	return -1;
}

int32 CXR_AnimGraph2Compiler::GetAnimationIndex(const char* _Name)
{
	for (int iAnim = 0; iAnim < m_lAnimations.Len(); iAnim++)
	{
		if (m_lAnimations[iAnim].m_Name == _Name)
		{
			return iAnim;
		}
	}
	return -1;
}

bool CXR_AnimGraph2Compiler::GetPropertyID(const char* _Name, int32& _ID, int32& _PropertyType)
{
	_PropertyType = -1;
	_ID = GetPropertyConditionID(_Name);
	if (_ID != -1)
	{
		_PropertyType = AG2_PROPERTYTYPE_CONDITION;
		return true;
	}
	_ID = GetPropertyFunctionID(_Name);
	if (_ID != -1)
	{
		_PropertyType = AG2_PROPERTYTYPE_FUNCTION;
		return true;
	}
	_ID = GetPropertyFloatID(_Name);
	if (_ID != -1)
	{
		_PropertyType = AG2_PROPERTYTYPE_FLOAT;
		return true;
	}
	_ID = GetPropertyIntID(_Name);
	if (_ID != -1)
	{
		_PropertyType = AG2_PROPERTYTYPE_INT;
		return true;
	}
	_ID = GetPropertyBoolID(_Name);
	if (_ID != -1)
	{
		_PropertyType = AG2_PROPERTYTYPE_BOOL;
		return true;
	}

	return false;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraph2Compiler::GetOperatorID(const char* _Name)
{
	for (int iOperator = 0; iOperator < m_lOperators.Len(); iOperator++)
	{
		if (m_lOperators[iOperator].m_Name == _Name)
			return m_lOperators[iOperator].m_ID;
	}
	return -1;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraph2Compiler::GetEffectID(const char* _Name)
{
	for (int iEffect = 0; iEffect < m_lEffects.Len(); iEffect++)
	{
		if (m_lEffects[iEffect].m_Name == _Name)
			return m_lEffects[iEffect].m_ID;
	}
	return -1;
}

bool CXR_AnimGraph2Compiler::GetAnimIndex(const CStr& _Name, int16& _iAnim, CXRAG2C_State* _pState, CStr _Location)
{
	for (int iAnim = 0; iAnim < m_lAnimations.Len(); iAnim++)
	{
		if (m_lAnimations[iAnim].m_Name == _Name)
		{
			_iAnim = iAnim;
			m_lAnimations[iAnim].m_nReferences++;
			return true;
		}
	}
	return false;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraph2Compiler::GetStateConstantID(const char* _Name)
{
	for (int iStateConstant = 0; iStateConstant < m_lStateConstants.Len(); iStateConstant++)
	{
		if (m_lStateConstants[iStateConstant].m_Name == _Name)
			return m_lStateConstants[iStateConstant].m_ID;
	}
	return -1;
}

int32 CXR_AnimGraph2Compiler::GetGraphBlockIndex(const char* _Name)
{
	for (int iGraphBlock = 0; iGraphBlock < m_lspGraphBlocks.Len(); iGraphBlock++)
	{
		if (m_lspGraphBlocks[iGraphBlock]->m_Name == _Name)
			return iGraphBlock;
	}

	return -1;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::GetConstantValueFloat(const char* _Name, fp32& _Value)
{
	for (int iConstant = 0; iConstant < m_lFloatConstants.Len(); iConstant++)
	{
		if (m_lFloatConstants[iConstant].m_Name == _Name)
		{
			_Value = m_lFloatConstants[iConstant].m_Value;
			return true;
		}
	}
	return false;
}

bool CXR_AnimGraph2Compiler::GetConstantValueInt(const char* _Name, int& _Value)
{
	for (int iConstant = 0; iConstant < m_lIntConstants.Len(); iConstant++)
	{
		if (m_lIntConstants[iConstant].m_Name == _Name)
		{
			_Value = m_lIntConstants[iConstant].m_Value;
			return true;
		}
	}
	return false;
}

bool CXR_AnimGraph2Compiler::GetConstantValueBool(const char* _Name, bool& _Value)
{
	for (int iConstant = 0; iConstant < m_lBoolConstants.Len(); iConstant++)
	{
		if (m_lBoolConstants[iConstant].m_Name == _Name)
		{
			_Value = m_lBoolConstants[iConstant].m_Value;
			return true;
		}
	}
	return false;
}

bool CXR_AnimGraph2Compiler::GetImpulseValue(const char* _Name, int16& _Value)
{
	for (int iImpulse = 0; iImpulse < m_lImpulseValues.Len(); iImpulse++)
	{
		if (m_lImpulseValues[iImpulse].m_Name == _Name)
		{
			_Value = m_lImpulseValues[iImpulse].m_Value;
			return true;
		}
	}
	return false;
}

bool CXR_AnimGraph2Compiler::GetImpulseType(const char* _Name, int16& _Value)
{
	for (int iImpulse = 0; iImpulse < m_lImpulseTypes.Len(); iImpulse++)
	{
		if (m_lImpulseTypes[iImpulse].m_Name == _Name)
		{
			_Value = m_lImpulseTypes[iImpulse].m_Value;
			return true;
		}
	}
	return false;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::EvalConstantExpressionFloat(CStr _ExprStr, fp32& _ConstantValue, CXRAG2C_State* _pState, CStr _Location)
{
	CXRAG2C_ConstExprParserFloat Parser(_ExprStr, this, _pState, _Location);
	return Parser.ParseExpr(_ConstantValue);
}

bool CXR_AnimGraph2Compiler::EvalConstantExpressionInt(CStr _ExprStr, int& _ConstantValue, CXRAG2C_State* _pState, CStr _Location)
{
	CXRAG2C_ConstExprParserInt Parser(_ExprStr, this, _pState, _Location);
	return Parser.ParseExpr(_ConstantValue);
}

bool CXR_AnimGraph2Compiler::EvalConstantExpressionBool(CStr _ExprStr, bool& _ConstantValue, CXRAG2C_State* _pState, CStr _Location)
{
	CXRAG2C_ConstExprParserBool Parser(_ExprStr, this, _pState, _Location);
	return Parser.ParseExpr(_ConstantValue);
}
bool CXR_AnimGraph2Compiler::EvalConstantExpressionByType(CStr _ExprStr, int _Type, fp32& _ConstantValueFloat, int& _ConstantValueInt, bool& _ConstantValueBool, CXRAG2C_State* _pState, CStr _Location)
{
	switch (_Type)
	{
	case AG2_PROPERTYTYPE_CONDITION:
	case AG2_PROPERTYTYPE_FLOAT:
		{
			return EvalConstantExpressionFloat(_ExprStr, _ConstantValueFloat, _pState, _Location);
		}
	case AG2_PROPERTYTYPE_FUNCTION:
	case AG2_PROPERTYTYPE_INT:
		{
			return EvalConstantExpressionInt(_ExprStr, _ConstantValueInt, _pState, _Location);
		}
	case AG2_PROPERTYTYPE_BOOL:
		{
			return EvalConstantExpressionBool(_ExprStr, _ConstantValueBool, _pState, _Location);
		}
	default:
		return false;
	}
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::ResolveConstantValueFloat(CStr _ConstantStr, fp32& _ConstantValue, CXRAG2C_State* _pState, CStr _Location)
{
	if (_ConstantStr.Left(2) == "0x")
	{
		_ConstantValue = _ConstantStr.Val_int();
		return true;
	}

	fp64 ConstantValueFP64;
	if (CStr::Val_fp64((char*)_ConstantStr, ConstantValueFP64) > 0)
	{
		_ConstantValue = ConstantValueFP64;
		return true;
	}

	if (_pState != NULL)
	{
		int32 StateConstantID = GetStateConstantID(_ConstantStr.UpperCase());
		if (StateConstantID != -1)
		{
			for (int iConstant = 0; iConstant < _pState->m_lConstants.Len(); iConstant++)
			{
				if (_pState->m_lConstants[iConstant].m_ID == StateConstantID)
				{
					_ConstantValue = _pState->m_lConstants[iConstant].m_Value;
					return true;
				}
			}
		}
	}

	CStr Upper = _ConstantStr.UpperCase();
	if (GetConstantValueFloat(Upper, _ConstantValue))
		return true;

	// If we haven't found any float constants, use the int ones
	int IntVal;
	if (GetConstantValueInt(Upper,IntVal))
	{
		_ConstantValue = (fp32)IntVal;
		return true;
	}
	// If that doesn't work, use impulsetypes/values as last resort
	int16 Val;
	if (GetImpulseType(Upper, Val) || GetImpulseValue(Upper, Val))
	{
		_ConstantValue = (fp32)Val;
		return true;
	}

	OnError(CStrF("Undefined/Invalid float constant '%s' (%s).", (char*)_ConstantStr, (char*)_Location));
	return false;
}

bool CXR_AnimGraph2Compiler::ResolveConstantValueInt(CStr _ConstantStr, int& _ConstantValue, CXRAG2C_State* _pState, CStr _Location)
{
	if (_ConstantStr.Left(2) == "0x")
	{
		_ConstantValue = _ConstantStr.Val_int();
		return true;
	}

	int ConstantValue;
	if (CStr::Val_int((char*)_ConstantStr, ConstantValue) > 0)
	{
		_ConstantValue = ConstantValue;
		return true;
	}

	// No stateconstant's if finding int's 
	/*if (_pState != NULL)
	{
		int32 StateConstantID = GetStateConstantID(_ConstantStr.UpperCase());
		if (StateConstantID != -1)
		{
			for (int iConstant = 0; iConstant < _pState->m_lConstants.Len(); iConstant++)
			{
				if (_pState->m_lConstants[iConstant].m_ID == StateConstantID)
				{
					_ConstantValue = _pState->m_lConstants[iConstant].m_Value;
					return true;
				}
			}
		}
	}*/

	if (GetConstantValueInt(_ConstantStr.UpperCase(), _ConstantValue))
		return true;
	// Get impulse types and values from int constants as well
	int16 Val = 0;
	if (GetImpulseType(_ConstantStr.UpperCase(), Val) || GetImpulseValue(_ConstantStr.UpperCase(), Val))
	{
		_ConstantValue = Val;
		return true;
	}

	OnError(CStrF("Undefined/Invalid int constant '%s' (%s).", (char*)_ConstantStr, (char*)_Location));
	return false;
}

bool CXR_AnimGraph2Compiler::ResolveConstantValueBool(CStr _ConstantStr, bool& _ConstantValue, CXRAG2C_State* _pState, CStr _Location)
{
	int ConstantValue;
	if (CStr::Val_int((char*)_ConstantStr, ConstantValue) > 0)
	{
		_ConstantValue = ConstantValue != 0;
		return true;
	}

	if (GetConstantValueBool(_ConstantStr.UpperCase(), _ConstantValue))
		return true;

	OnError(CStrF("Undefined/Invalid bool constant '%s' (%s).", (char*)_ConstantStr, (char*)_Location));
	return false;
}

bool CXR_AnimGraph2Compiler::ResolveImpulseValue(CStr _ConstantStr, int16& _ConstantValue, CStr _Location)
{
	if (_ConstantStr.Left(2) == "0x")
	{
		_ConstantValue = _ConstantStr.Val_int();
		return true;
	}

	int ConstantValue;
	if (CStr::Val_int((char*)_ConstantStr, ConstantValue) > 0)
	{
		_ConstantValue = ConstantValue;
		return true;
	}

	if (GetImpulseValue(_ConstantStr.UpperCase(), _ConstantValue))
		return true;

	OnError(CStrF("Undefined/Invalid impulsevalue '%s' (%s).", (char*)_ConstantStr, (char*)_Location));
	return false;
}

bool CXR_AnimGraph2Compiler::ResolveImpulseType(CStr _ConstantStr, int16& _ConstantValue, CStr _Location)
{
	if (_ConstantStr.Left(2) == "0x")
	{
		_ConstantValue = _ConstantStr.Val_int();
		return true;
	}
	if (_ConstantStr.Len() > 0)
	{
		int ConstantValue;
		if (CStr::Val_int((char*)_ConstantStr, ConstantValue) > 0)
		{
			_ConstantValue = ConstantValue;
			return true;
		}

		if (GetImpulseType(_ConstantStr.UpperCase(), _ConstantValue))
			return true;
	}

	OnError(CStrF("Undefined/Invalid impulsevalue '%s' (%s).", (char*)_ConstantStr, (char*)_Location));
	return false;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::GetStateFlag(const char* _Name, uint32& _Value)
{
	for (int iStateFlag = 0; iStateFlag < m_lStateFlags.Len(); iStateFlag++)
	{
		CStr DebugTemp = m_lStateFlags[iStateFlag].m_Name;
		if (DebugTemp == _Name)
		{
			_Value = m_lStateFlags[iStateFlag].m_Value;
			return true;
		}
	}
	return false;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::ParseStateFlags(CStr _FlagsStr, uint32& _Flags, uint32& _FlagsMask, CStr _Location)
{
//	uint32 ResultValue = 0;
	do
	{
		CStr FlagStr, Operator;
		int iOperatorPos = _FlagsStr.FindOneOf("+-");
		if (iOperatorPos == -1)
		{
			FlagStr = _FlagsStr.UpperCase();
			Operator = "";
			_FlagsStr = "";
		}
		else
		{
			FlagStr = _FlagsStr.Left(iOperatorPos).UpperCase();
			Operator = _FlagsStr.SubStr(iOperatorPos, 1);
			_FlagsStr = _FlagsStr.DelTo(iOperatorPos);
		}
		FlagStr.Trim();

		uint32 FlagValue;
		if (!GetStateFlag(FlagStr, FlagValue))
		{
			OnError(CStrF("Undefined stateflag '%s' (%s)", (char*)FlagStr, (char*)_Location));
			return false;
		}
		else
		{
			if ((Operator == "+") || (Operator == ""))
			{
				_Flags |= FlagValue;
				_FlagsMask |= FlagValue;
			}
			else if (Operator == "-")
			{
				_FlagsMask |= FlagValue;
			}
			else
				OnError(CStrF("Invalid symbol '%s' in stateflag syntax (%s).", (char*)Operator, (char*)_Location));
		}
	}
	while (_FlagsStr != "");

	return true;
}

bool CXR_AnimGraph2Compiler::GetAnimFlag(const char* _Name, uint32& _Value)
{
	for (int iAnimFlag = 0; iAnimFlag < m_lAnimFlags.Len(); iAnimFlag++)
	{
		CStr DebugTemp = m_lAnimFlags[iAnimFlag].m_Name;
		if (DebugTemp == _Name)
		{
			_Value = m_lAnimFlags[iAnimFlag].m_Value;
			return true;
		}
	}
	return false;
}

bool CXR_AnimGraph2Compiler::ParseAnimFlags(CStr _FlagsStr, uint32& _Flags, uint32& _FlagsMask, CStr _Location)
{
//	uint32 ResultValue = 0;
	do
	{
		CStr FlagStr, Operator;
		int iOperatorPos = _FlagsStr.FindOneOf("+-");
		if (iOperatorPos == -1)
		{
			FlagStr = _FlagsStr.UpperCase();
			Operator = "";
			_FlagsStr = "";
		}
		else
		{
			FlagStr = _FlagsStr.Left(iOperatorPos).UpperCase();
			Operator = _FlagsStr.SubStr(iOperatorPos, 1);
			_FlagsStr = _FlagsStr.DelTo(iOperatorPos);
		}
		FlagStr.Trim();

		uint32 FlagValue;
		if (!GetAnimFlag(FlagStr, FlagValue))
		{
			OnError(CStrF("Undefined animflag '%s' (%s)", (char*)FlagStr, (char*)_Location));
			return false;
		}
		else
		{
			if ((Operator == "+") || (Operator == ""))
			{
				_Flags |= FlagValue;
				_FlagsMask |= FlagValue;
			}
			else if (Operator == "-")
			{
				_FlagsMask |= FlagValue;
			}
			else
				OnError(CStrF("Invalid symbol '%s' in stateflag syntax (%s).", (char*)Operator, (char*)_Location));
		}
	}
	while (_FlagsStr != "");

	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::Process()
{
	if (m_lspGraphBlocks.Len() == 0)
	{
		if ((m_lPropertiesFloat.Len() == 0) && 
			(m_lPropertiesInt.Len() == 0)&& 
			(m_lPropertiesBool.Len() == 0) &&
			(m_lOperators.Len() == 0) && 
			(m_lEffects.Len() == 0) && 
			(m_lFloatConstants.Len() == 0) && 
			(m_lStateFlags.Len() == 0))
			OnError("AnimGraph contains no GraphBlock nor declarations.");
		else
			OnError("AnimGraph contains no GraphBlock (It's probably an AnimGraph declaration file).");
		return false;
	}
	
	// Process graphblocks before deleting original anim list
	// Sort graphblocks by impulse type/values
	// Sort reaction
	TArray<spCXRAG2C_GraphBlock> lGraphBlocks;
	for (int32 iBlock = 0; iBlock < m_lspGraphBlocks.Len(); iBlock++)
	{
		spCXRAG2C_GraphBlock spBlock = m_lspGraphBlocks[iBlock];
		// Add it last
		int32 iIndex = lGraphBlocks.Len();
		for (int32 i = 0; i < lGraphBlocks.Len(); i++)
		{
			// Check for mixed value/non value, not allowed (atm), screws up searching
			if ((spBlock->m_Condition.m_ImpulseType == lGraphBlocks[i]->m_Condition.m_ImpulseType) &&
				((spBlock->m_Condition.m_ImpulseValue == -1) || (lGraphBlocks[i]->m_Condition.m_ImpulseValue == -1)))
			{
				OnError(CStrF("Can't mix blocks with and without values '%s' && '%s'.", spBlock->m_Name.Str(), lGraphBlocks[i]->m_Name.Str()));
				return false;
			}
			// Check for same value
			if ((spBlock->m_Condition.m_ImpulseType == lGraphBlocks[i]->m_Condition.m_ImpulseType) && 
				(spBlock->m_Condition.m_ImpulseValue ==lGraphBlocks[i]->m_Condition.m_ImpulseValue))
			{
				OnError(CStrF("Already have this graphblock impulse value: %d:%d  '%s' && '%s'.", spBlock->m_Condition.m_ImpulseType, spBlock->m_Condition.m_ImpulseValue,spBlock->m_Name.GetStr(), lGraphBlocks[i]->m_Name.GetStr()));
				return false;
			}
			if ((spBlock->m_Condition.m_ImpulseType < lGraphBlocks[i]->m_Condition.m_ImpulseType) ||
				((spBlock->m_Condition.m_ImpulseType == lGraphBlocks[i]->m_Condition.m_ImpulseType) &&
				(spBlock->m_Condition.m_ImpulseValue < lGraphBlocks[i]->m_Condition.m_ImpulseValue)))
			{
				iIndex = i;
				break;
			}
		}
		lGraphBlocks.Insert(iIndex,spBlock);
	}

	m_lspGraphBlocks = lGraphBlocks;

	bool bVal = true;
	for (int32 i = 0; i < m_lspGraphBlocks.Len(); i++)
	{
		if (!m_lspGraphBlocks[i]->PreProcess())
			bVal = false;
	}
	if (!bVal)
		return false;

	bVal = true;
	for (int32 i = 0; i < m_lspGraphBlocks.Len(); i++)
	{
		if (!m_lspGraphBlocks[i]->ProcessPass1())
			bVal = false;
	}

	if (!bVal)
		return false;

	bVal = true;
	for (int32 i = 0; i < m_lspGraphBlocks.Len(); i++)
	{
		if (!m_lspGraphBlocks[i]->ProcessPostPass1())
			bVal = false;
	}

	if (!bVal)
		return false;

	bVal = true;
	for (int32 i = 0; i < m_lspGraphBlocks.Len(); i++)
	{
		if (!m_lspGraphBlocks[i]->ProcessPrePass2())
			bVal = false;
	}
	if (!bVal)
		return false;

	// Drop unreferenced animations
	TArray<CXRAG2C_Animation> lAnimations;
	int32 Len = m_lAnimations.Len();
	int32 iOffset = 0;
	for (int32 i = 0; i < Len; i++)
	{
		if (m_lAnimations[i].m_nReferences > 0)
		{
			m_lAnimations[i].m_iOffset = iOffset;
			// Get Container names
			CStr AnimSeq = m_lAnimations[i].m_Value;
			if(AnimSeq == "")
			{
				OnError(CStrF("Animation: %s missing container",m_lAnimations[i].m_Name.GetStr()).Str());
				return false;
			}
			CStr Container = AnimSeq.GetStrSep(":");
			m_lAnimations[i].m_iAnimSeq = AnimSeq.Val_int();
			m_lAnimations[i].m_iContainerName = m_ContainerNames.GetNameIndex(Container);
			lAnimations.Add(m_lAnimations[i]);
		}
		else
		{
			iOffset++;
		}
	}

	for (int32 i = 0; i < m_lspGraphBlocks.Len(); i++)
	{
		if (!m_lspGraphBlocks[i]->ProcessPass2())
			bVal = false;
	}


	m_lAnimations = lAnimations;

	// Find max properties
	m_MaxPropertyIDCondition = -1;
	for (int32 i = 0; i < m_lPropertiesCondition.Len(); i++)
		m_MaxPropertyIDCondition = Max(m_lPropertiesCondition[i].m_ID, m_MaxPropertyIDCondition);
	m_MaxPropertyIDFloat = -1;
	for (int32 i = 0; i < m_lPropertiesFloat.Len(); i++)
		m_MaxPropertyIDFloat = Max(m_lPropertiesFloat[i].m_ID, m_MaxPropertyIDFloat);
	m_MaxPropertyIDInt = -1;
	for (int32 i = 0; i < m_lPropertiesInt.Len(); i++)
		m_MaxPropertyIDInt = Max(m_lPropertiesInt[i].m_ID, m_MaxPropertyIDInt);
	m_MaxPropertyIDBool = -1;
	for (int32 i = 0; i < m_lPropertiesBool.Len(); i++)
		m_MaxPropertyIDBool = Max(m_lPropertiesBool[i].m_ID, m_MaxPropertyIDBool);

	// Resolve startblock
	if (m_StartBlock == "")
	{
		OnError("Starting block not specified.");
		return false;
	}
	else
	{
		m_iStartBlock = AG2_STATEINDEX_NULL;
		for (int iBlock = 0; iBlock < m_lspGraphBlocks.Len(); iBlock++)
		{
			if (m_StartBlock == m_lspGraphBlocks[iBlock]->m_Name)
				m_iStartBlock = iBlock;
		}

		if (m_iStartBlock == AG2_STATEINDEX_NULL)
		{
			OnError("Specified starting block '" + m_StartBlock + "' is not defined.");
			return false;
		}
	}

	return bVal;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::Convert()
{
	// Convert into CXR_AnimGraph2 classes...

	int nTotalGraphBlocks = 0;
	int nTotalStates = 0;
	int nTotalSwitchStates = 0;
	int nTotalAnimLayers = 0;
	int nTotalActions = 0; // Don't Count start action anymore
	int nTotalMoveTokens = 1; // New, add startmovetoken
	int nTotalMoveAnimGraphs = 0;
	int nTotalReactions = 0;
	int nTotalSwitchActionVals = 0;
	int nTotalCallbackParams = 0;
	int nTotalNodes = 0;
	int nTotalAnimNames = m_lAnimations.Len();
	int nTotalEffectInstances = 0;
	int nTotalStateConstants = 0;

	nTotalGraphBlocks = m_lspGraphBlocks.Len();
	// Every graphblock needs a startmovetoken
	nTotalMoveTokens += nTotalGraphBlocks;

	for (int32 iBlock = 0; iBlock < nTotalGraphBlocks; iBlock++)
	{
		CXRAG2C_GraphBlock* pBlock = m_lspGraphBlocks[iBlock];
		// Add total reaction count
		nTotalReactions += pBlock->m_Reactions.m_lReactions.Len();
		nTotalStateConstants += pBlock->m_lConstants.Len();

		// Add reaction movetokens
		nTotalEffectInstances += pBlock->m_Reactions.m_lEffectInstances.Len();
		for (int iEffectInstance = 0; iEffectInstance < pBlock->m_Reactions.m_lEffectInstances.Len(); iEffectInstance++)
		{
			CXRAG2C_EffectInstance* pEI = &(pBlock->m_Reactions.m_lEffectInstances[iEffectInstance]);
			nTotalCallbackParams += pEI->m_lParams.Len();
		}

		for (int32 i = 0; i < pBlock->m_Reactions.m_lReactions.Len(); i++)
		{
			nTotalMoveTokens += pBlock->m_Reactions.m_lReactions[i].m_lMoveTokens.Len();
			nTotalMoveAnimGraphs += pBlock->m_Reactions.m_lReactions[i].m_lMoveAnimGraphs.Len();
		}

		for (int32 iState = 0; iState < pBlock->m_lspStates.Len(); iState++)
		{
			CXRAG2C_State* pState = (pBlock->m_lspStates[iState]);
			nTotalStates++;
			nTotalAnimLayers += pBlock->m_lspStates[iState]->m_lStateAnims.Len();
			int ActionsLen = pState->m_lActions.Len();
			nTotalActions += ActionsLen;
			// Count movetokens
			for (int32 i = 0; i < ActionsLen; i++)
				nTotalMoveTokens += pState->m_lActions[i].m_lMoveTokens.Len();

			nTotalNodes += pState->m_lConditionNodes.Len();
			nTotalEffectInstances += pState->m_lEffectInstances.Len();
			nTotalStateConstants += pState->m_lConstants.Len();

			for (int iEffectInstance = 0; iEffectInstance < pState->m_lEffectInstances.Len(); iEffectInstance++)
			{
				CXRAG2C_EffectInstance* pEI = &(pState->m_lEffectInstances[iEffectInstance]);
				nTotalCallbackParams += pEI->m_lParams.Len();
			}
		}

		// Add switchstate stuffs
		for (int32 iState = 0; iState < pBlock->m_lspSwitchStates.Len(); iState++)
		{
			CXRAG2C_SwitchState* pState = (pBlock->m_lspSwitchStates[iState]);
			nTotalSwitchStates++;
			// COUNT DEFAULT ACTION AS WELL
			int ActionsLen = pState->m_lActions.Len();
			// Skip actions, movetoken moved to switchactionval
			//nTotalActions += ActionsLen + 1;
			nTotalSwitchActionVals += ActionsLen + 1;
			nTotalMoveTokens += ActionsLen + 1;
			// Count movetokens
			/*for (int32 i = 0; i < ActionsLen; i++)
				nTotalMoveTokens += pState->m_lActions[i].m_lMoveTokens.Len();
			nTotalMoveTokens += pState->m_DefaultAction.m_lMoveTokens.Len();*/

			// Skip effects for now as well
			/*nTotalEffectInstances += pState->m_lEffectInstances.Len();

			for (int iEffectInstance = 0; iEffectInstance < pState->m_lEffectInstances.Len(); iEffectInstance++)
			{
				CXRAG2C_EffectInstance* pEI = &(pState->m_lEffectInstances[iEffectInstance]);
				nTotalCallbackParams += pEI->m_lParams.Len();
			}*/
		}
	}

	m_pAnimGraph->m_Name = m_Name;

	m_pAnimGraph->m_lGraphBlocks.SetLen(nTotalGraphBlocks);
	m_pAnimGraph->m_lFullStates.SetLen(nTotalStates);
	m_pAnimGraph->m_lSwitchStates.SetLen(nTotalSwitchStates);
	m_pAnimGraph->m_lFullAnimLayers.SetLen(nTotalAnimLayers);
	//m_pAnimGraph->m_lAnimLayersMap.SetLen(nTotalAnimLayers);
	m_pAnimGraph->m_lFullActions.SetLen(nTotalActions);
	m_pAnimGraph->m_lSwitchStateActionVals.SetLen(nTotalSwitchActionVals);
	m_pAnimGraph->m_lMoveTokens.SetLen(nTotalMoveTokens);
	m_pAnimGraph->m_lMoveAnimGraphs.SetLen(nTotalMoveAnimGraphs);
	m_pAnimGraph->m_lFullReactions.SetLen(nTotalReactions);
	//m_pAnimGraph->m_lActionsMap.SetLen(nTotalActions);
	//m_pAnimGraph->m_lNodes.SetLen(nTotalNodes);
	m_pAnimGraph->m_lNodesV2.SetLen(nTotalNodes);
	m_pAnimGraph->m_lEffectInstances.SetLen(nTotalEffectInstances);
	m_pAnimGraph->m_lStateConstants.SetLen(nTotalStateConstants);
	m_pAnimGraph->m_lActionHashEntries.SetLen(nTotalActions);
	m_pAnimGraph->m_lCallbackParams.SetLen(nTotalCallbackParams);
	// Set animation name length and copy the names
	m_pAnimGraph->m_lAnimNames.SetLen(nTotalAnimNames);
	m_pAnimGraph->m_AnimContainerNames.m_lNames.SetLen(m_ContainerNames.m_lNames.Len());
	for (int32 i = 0; i < nTotalAnimNames; i++)
	{
		m_pAnimGraph->m_lAnimNames[i].m_iAnimSeq = m_lAnimations[i].m_iAnimSeq;
		m_pAnimGraph->m_lAnimNames[i].m_iContainerName = m_lAnimations[i].m_iContainerName;
	}
	for (int32 i = 0; i < m_ContainerNames.m_lNames.Len(); i++)
	{
		m_pAnimGraph->m_AnimContainerNames.m_lNames[i] = m_ContainerNames.m_lNames[i];
	}

	m_lActionHashNames.SetLen(nTotalActions);

#ifndef M_RTM
	m_pAnimGraph->m_lExportedActionNames.SetLen(nTotalActions);
	m_pAnimGraph->m_lExportedStateNames.SetLen(nTotalStates);
	m_pAnimGraph->m_lExportedReactionNames.SetLen(nTotalReactions);
	m_pAnimGraph->m_lExportedSwitchStateNames.SetLen(nTotalSwitchStates);
	// Exported Names
	{
		m_pAnimGraph->m_lExportedGraphBlockNames.SetLen(nTotalGraphBlocks);
		for (int iBlock = 0; iBlock < nTotalGraphBlocks; iBlock++)
			m_pAnimGraph->m_lExportedGraphBlockNames[iBlock] = m_lspGraphBlocks[iBlock]->m_Name;

		m_pAnimGraph->m_lExportedAnimGraphNames.SetLen(nTotalMoveAnimGraphs);

		m_pAnimGraph->m_lExportedPropertyConditionNames.SetLen(m_lPropertiesCondition.Len());
		for (int iProperty = 0; iProperty < m_lPropertiesCondition.Len(); iProperty++)
			m_pAnimGraph->m_lExportedPropertyConditionNames[iProperty] = CXRAG2_NameAndID(m_lPropertiesCondition[iProperty].m_Name, m_lPropertiesCondition[iProperty].m_ID);

		m_pAnimGraph->m_lExportedPropertyFunctionNames.SetLen(m_lPropertiesFunction.Len());
		for (int iProperty = 0; iProperty < m_lPropertiesFunction.Len(); iProperty++)
			m_pAnimGraph->m_lExportedPropertyFunctionNames[iProperty] = CXRAG2_NameAndID(m_lPropertiesFunction[iProperty].m_Name, m_lPropertiesFunction[iProperty].m_ID);

		m_pAnimGraph->m_lExportedPropertyFloatNames.SetLen(m_lPropertiesFloat.Len());
		for (int iProperty = 0; iProperty < m_lPropertiesFloat.Len(); iProperty++)
			m_pAnimGraph->m_lExportedPropertyFloatNames[iProperty] = CXRAG2_NameAndID(m_lPropertiesFloat[iProperty].m_Name, m_lPropertiesFloat[iProperty].m_ID);

		m_pAnimGraph->m_lExportedPropertyIntNames.SetLen(m_lPropertiesInt.Len());
		for (int iProperty = 0; iProperty < m_lPropertiesInt.Len(); iProperty++)
			m_pAnimGraph->m_lExportedPropertyIntNames[iProperty] = CXRAG2_NameAndID(m_lPropertiesInt[iProperty].m_Name, m_lPropertiesInt[iProperty].m_ID);

		m_pAnimGraph->m_lExportedPropertyBoolNames.SetLen(m_lPropertiesBool.Len());
		for (int iProperty = 0; iProperty < m_lPropertiesBool.Len(); iProperty++)
			m_pAnimGraph->m_lExportedPropertyBoolNames[iProperty] = CXRAG2_NameAndID(m_lPropertiesBool[iProperty].m_Name, m_lPropertiesBool[iProperty].m_ID);

		m_pAnimGraph->m_lExportedOperatorNames.SetLen(m_lOperators.Len());
		for (int iOperator = 0; iOperator < m_lOperators.Len(); iOperator++)
			m_pAnimGraph->m_lExportedOperatorNames[iOperator] = CXRAG2_NameAndID(m_lOperators[iOperator].m_Name, m_lOperators[iOperator].m_ID);

		m_pAnimGraph->m_lExportedEffectNames.SetLen(m_lEffects.Len());
		for (int iEffect = 0; iEffect < m_lEffects.Len(); iEffect++)
			m_pAnimGraph->m_lExportedEffectNames[iEffect] = CXRAG2_NameAndID(m_lEffects[iEffect].m_Name, m_lEffects[iEffect].m_ID);

		m_pAnimGraph->m_lExportedStateConstantNames.SetLen(m_lStateConstants.Len());
		for (int iStateConstant = 0; iStateConstant < m_lStateConstants.Len(); iStateConstant++)
			m_pAnimGraph->m_lExportedStateConstantNames[iStateConstant] = CXRAG2_NameAndID(m_lStateConstants[iStateConstant].m_Name, m_lStateConstants[iStateConstant].m_ID);
		
		m_pAnimGraph->m_lExportedImpulseTypeNames.SetLen(m_lImpulseTypes.Len());
		for (int iImpulseType = 0; iImpulseType < m_lImpulseTypes.Len(); iImpulseType++)
			m_pAnimGraph->m_lExportedImpulseTypeNames[iImpulseType] = CXRAG2_NameAndValue(m_lImpulseTypes[iImpulseType].m_Name, m_lImpulseTypes[iImpulseType].m_Value);

		m_pAnimGraph->m_lExportedImpulseValueNames.SetLen(m_lImpulseValues.Len());
		for (int iImpulseValue = 0; iImpulseValue < m_lImpulseValues.Len(); iImpulseValue++)
			m_pAnimGraph->m_lExportedImpulseValueNames[iImpulseValue] = CXRAG2_NameAndValue(m_lImpulseValues[iImpulseValue].m_Name, m_lImpulseValues[iImpulseValue].m_Value);
	}
#endif

	int iAction = 0;
	int iAnimLayer = 0;
	int iNode = 0;
	int iEffectInstance = 0;
	int iCallbackParam = 0;
	int iSwitchStateAG = 0;
	int iSwitchStateActionVal = 0;
	int iStateConstant = 0;
	int iExportedActionName = 0;
	int iExportedStateName = 0;
	int iExportedReactionName = 0;
	int iExportedSwitchStateName = 0;
	int32 iMoveToken = 0;
	int32 iMoveAnimGraph = 0;

	// Add start Movetoken.
	m_pAnimGraph->m_lMoveTokens[iMoveToken].m_iTargetGraphBlock = m_iStartBlock;
	//m_pAnimGraph->m_lMoveTokens[iMoveToken].m_TokenID = 0;
	iMoveToken++;
	//m_pAnimGraph->m_lFullActions[iAction].m_iTargetState = m_iStartBlock;
	//m_pAnimGraph->m_lActionsMap[iAction] = iAction;

	/*CStr ActionName = "STARTACTION";
	uint32 ActionHashKey = StringToHash(ActionName);
	CXRAG2_ActionHashEntry ActionHashEntry(ActionHashKey, iAction);
	m_lActionHashNames[iAction] = ActionName;
	m_pAnimGraph->m_lActionHashEntries[iAction] = ActionHashEntry;
#ifndef M_RTM
	m_pAnimGraph->m_lExportedActionNames[iExportedActionName++] = ActionName;
#endif

	iAction++;*/
	int iStateEffectInstanceBase = 0;
	int32 iStateAG = 0;
	int32 iReaction = 0;
	
	for (int32 iBlock = 0; iBlock < m_lspGraphBlocks.Len(); iBlock++)
	{
		CXRAG2C_GraphBlock* pBlock = m_lspGraphBlocks[iBlock];
		CXRAG2_GraphBlock* pAGBlock = &m_pAnimGraph->m_lGraphBlocks[iBlock];
		//pAGBlock->m_BlockID = iBlock;
		pAGBlock->m_iStateFullStart = iStateAG;
		pAGBlock->m_StateFullLen = pBlock->m_lspStates.Len();
		pAGBlock->m_iSwitchStateStart = iSwitchStateAG;
		pAGBlock->m_SwitchStateLen = pBlock->m_lspSwitchStates.Len();
		pAGBlock->m_ReactionFullLen = pBlock->m_Reactions.m_lReactions.Len();
		pAGBlock->m_iReactionFullStart = iReaction;
		pAGBlock->m_Condition.m_ImpulseType = pBlock->m_Condition.m_ImpulseType;
		pAGBlock->m_Condition.m_ImpulseValue = pBlock->m_Condition.m_ImpulseValue;
		pAGBlock->m_iStateConstantStart = iStateConstant;
		pAGBlock->m_StateConstantLen = pBlock->m_lConstants.Len();
		

		// StateConstants
		for (int jConstant = 0; jConstant < pBlock->m_lConstants.Len(); jConstant++, iStateConstant++)
		{
			m_pAnimGraph->m_lStateConstants[iStateConstant].m_ID = pBlock->m_lConstants[jConstant].m_ID;
			m_pAnimGraph->m_lStateConstants[iStateConstant].m_Value = pBlock->m_lConstants[jConstant].m_Value;
		}

		// Add startmovetoken for this block
		pAGBlock->m_iStartMoveToken = iMoveToken;
		m_pAnimGraph->m_lMoveTokens[iMoveToken].m_iTargetGraphBlock = iBlock;
		m_pAnimGraph->m_lMoveTokens[iMoveToken].m_AnimBlendDuration = pBlock->m_StartMoveToken.m_AnimBlendDuration;
		m_pAnimGraph->m_lMoveTokens[iMoveToken].m_TargetStateType = pBlock->m_StartMoveToken.m_TargetStateType;
		
		if (pBlock->m_StartMoveToken.m_TargetStateType)
			m_pAnimGraph->m_lMoveTokens[iMoveToken].m_iTargetState = iSwitchStateAG + pBlock->m_StartMoveToken.m_iTargetState;
		else
			m_pAnimGraph->m_lMoveTokens[iMoveToken].m_iTargetState = iStateAG + pBlock->m_StartMoveToken.m_iTargetState;

		m_pAnimGraph->m_lMoveTokens[iMoveToken].m_AnimBlendDelay = pBlock->m_StartMoveToken.m_AnimBlendDelay;
		m_pAnimGraph->m_lMoveTokens[iMoveToken].m_AnimTimeOffset = pBlock->m_StartMoveToken.m_AnimTimeOffset;
		iMoveToken++;

		// Set startstate of first movetoken
		if (m_pAnimGraph->m_lMoveTokens[0].m_iTargetGraphBlock == iBlock)
		{
			m_pAnimGraph->m_lMoveTokens[0].m_TargetStateType = pBlock->m_StartMoveToken.m_TargetStateType;
			// Set startstate
			if (pBlock->m_StartMoveToken.m_TargetStateType)
				m_pAnimGraph->m_lMoveTokens[0].m_iTargetState = iSwitchStateAG + pBlock->m_StartMoveToken.m_iTargetState;
			else
				m_pAnimGraph->m_lMoveTokens[0].m_iTargetState = iStateAG + pBlock->m_StartMoveToken.m_iTargetState;
		}

		// Reactions
		for (int32 iR = 0; iR < pBlock->m_Reactions.m_lReactions.Len(); iR++)
		{
//			int16 ImpulseType = pBlock->m_Reactions.m_lReactions[iR].m_Condition.m_ImpulseType;
//			int16 ImpulseValue = pBlock->m_Reactions.m_lReactions[iR].m_Condition.m_ImpulseValue;
			m_pAnimGraph->m_lFullReactions[iReaction].m_Impulse.m_ImpulseType = pBlock->m_Reactions.m_lReactions[iR].m_Condition.m_ImpulseType;
			m_pAnimGraph->m_lFullReactions[iReaction].m_Impulse.m_ImpulseValue = pBlock->m_Reactions.m_lReactions[iR].m_Condition.m_ImpulseValue;
			m_pAnimGraph->m_lFullReactions[iReaction].m_iBaseMoveToken = iMoveToken;
			m_pAnimGraph->m_lFullReactions[iReaction].m_nMoveTokens = pBlock->m_Reactions.m_lReactions[iR].m_lMoveTokens.Len();			
			m_pAnimGraph->m_lFullReactions[iReaction].m_iBaseMoveAnimgraph = iMoveAnimGraph;
			m_pAnimGraph->m_lFullReactions[iReaction].m_nMoveAnimgraph = pBlock->m_Reactions.m_lReactions[iR].m_lMoveAnimGraphs.Len();
#ifndef M_RTM
			m_pAnimGraph->m_lExportedReactionNames[iExportedReactionName++] = pBlock->m_Reactions.m_lReactions[iR].m_Name;
#endif

			for (int32 iMT = 0; iMT < pBlock->m_Reactions.m_lReactions[iR].m_lMoveTokens.Len(); iMT++)
			{
				m_pAnimGraph->m_lMoveTokens[iMoveToken].m_iTargetGraphBlock = pBlock->m_Reactions.m_lReactions[iR].m_lMoveTokens[iMT].m_iTargetGraphBlock;

				m_pAnimGraph->m_lMoveTokens[iMoveToken].m_TokenID = pBlock->m_Reactions.m_lReactions[iR].m_lMoveTokens[iMT].m_TokenID;
				if (pBlock->m_Reactions.m_lReactions[iR].m_lMoveTokens[iMT].m_iTargetState == AG2_STATEINDEX_TERMINATE)
				{
					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_iTargetState = AG2_STATEINDEX_TERMINATE;
				}
				else if (pBlock->m_Reactions.m_lReactions[iR].m_lMoveTokens[iMT].m_iTargetState == AG2_STATEINDEX_STARTAG)
				{
					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_iTargetState = AG2_STATEINDEX_STARTAG;
				}
				else if (pBlock->m_Reactions.m_lReactions[iR].m_lMoveTokens[iMT].m_iTargetState != AG2_STATEINDEX_NULL)
				{
					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_iTargetState = pAGBlock->m_iStateFullStart + pBlock->m_Reactions.m_lReactions[iR].m_lMoveTokens[iMT].m_iTargetState;
				}	
				
				m_pAnimGraph->m_lMoveTokens[iMoveToken].m_AnimBlendDuration = pBlock->m_Reactions.m_lReactions[iR].m_lMoveTokens[iMT].m_AnimBlendDuration;
				m_pAnimGraph->m_lMoveTokens[iMoveToken].m_AnimBlendDelay = pBlock->m_Reactions.m_lReactions[iR].m_lMoveTokens[iMT].m_AnimBlendDelay;
				m_pAnimGraph->m_lMoveTokens[iMoveToken].m_AnimTimeOffset = pBlock->m_Reactions.m_lReactions[iR].m_lMoveTokens[iMT].m_AnimTimeOffset;
				m_pAnimGraph->m_lMoveTokens[iMoveToken].m_TargetStateType = pBlock->m_Reactions.m_lReactions[iR].m_lMoveTokens[iMT].m_TargetStateType;
				iMoveToken++;
			}
			for (int32 iMAG = 0; iMAG < pBlock->m_Reactions.m_lReactions[iR].m_lMoveAnimGraphs.Len(); iMAG++)
			{
				m_pAnimGraph->m_lMoveAnimGraphs[iMoveAnimGraph].m_TokenID = pBlock->m_Reactions.m_lReactions[iR].m_lMoveAnimGraphs[iMAG].m_TokenID;
				m_pAnimGraph->m_lMoveAnimGraphs[iMoveAnimGraph].m_AnimGraphName = pBlock->m_Reactions.m_lReactions[iR].m_lMoveAnimGraphs[iMAG].m_TargetAnimGraphHash;
				m_pAnimGraph->m_lMoveAnimGraphs[iMoveAnimGraph].m_AnimBlendDuration = pBlock->m_Reactions.m_lReactions[iR].m_lMoveAnimGraphs[iMAG].m_AnimBlendDuration;
				m_pAnimGraph->m_lMoveAnimGraphs[iMoveAnimGraph].m_AnimBlendDelay = pBlock->m_Reactions.m_lReactions[iR].m_lMoveAnimGraphs[iMAG].m_AnimBlendDelay;
			#ifndef M_RTM
				m_pAnimGraph->m_lExportedAnimGraphNames[iMoveAnimGraph] = pBlock->m_Reactions.m_lReactions[iR].m_lMoveAnimGraphs[iMAG].m_TargetAnimGraph;
			#endif
				iMoveAnimGraph++;
			}

			// EffectInstances
			int iBaseEffectInstance = pBlock->m_Reactions.m_lReactions[iR].m_iBaseEffectInstance;
			int nEffectInstances = pBlock->m_Reactions.m_lReactions[iR].m_nEffectInstances;
			m_pAnimGraph->m_lFullReactions[iReaction].m_iBaseEffectInstance = iStateEffectInstanceBase + pBlock->m_Reactions.m_lReactions[iR].m_iBaseEffectInstance;
			m_pAnimGraph->m_lFullReactions[iReaction].m_nEffectInstances = pBlock->m_Reactions.m_lReactions[iR].m_nEffectInstances;
			for (int jEffectInstance = 0; jEffectInstance < nEffectInstances; jEffectInstance++)
			{
				int iAGEffectInstance = iStateEffectInstanceBase + iBaseEffectInstance + jEffectInstance;
				CXRAG2C_EffectInstance* pIMEffectInstance = &(pBlock->m_Reactions.m_lEffectInstances[iBaseEffectInstance + jEffectInstance]);
				CXRAG2_EffectInstance* pAGEffectInstance = &(m_pAnimGraph->m_lEffectInstances[iAGEffectInstance]);
				if (pAGEffectInstance->m_ID == AG2_EFFECTID_NULL)
				{
					pAGEffectInstance->m_ID = pIMEffectInstance->m_ID;
					pAGEffectInstance->m_iParams = iCallbackParam;
					int nParams = pIMEffectInstance->m_lParams.Len(); pAGEffectInstance->m_nParams = nParams;
					for (int iParam = 0; iParam < nParams; iParam++)
					{
						int16 Param = pIMEffectInstance->m_lParams[iParam];
						m_pAnimGraph->m_lCallbackParams[iCallbackParam + iParam] = Param;
					}
					iCallbackParam += nParams;
					iEffectInstance++;
				}
			}
			iReaction++;
		}
		iStateEffectInstanceBase += pBlock->m_Reactions.m_lEffectInstances.Len();
		// States
		for (int32 iState = 0; iState < pBlock->m_lspStates.Len(); iState++)
		{
	#ifndef M_RTM
			m_pAnimGraph->m_lExportedStateNames[iExportedStateName++] = pBlock->m_lspStates[iState]->m_Name;
	#endif

			// Misc
			CXRAG2_State* pState = &(m_pAnimGraph->m_lFullStates[iStateAG]);
			pState->m_lFlags[0] = pBlock->m_lspStates[iState]->m_Flags[0];
			pState->m_lFlags[1] = pBlock->m_lspStates[iState]->m_Flags[1];
			pState->m_Priority = pBlock->m_lspStates[iState]->m_Priority;

			// Animation
			int32 Len = pBlock->m_lspStates[iState]->m_lStateAnims.Len();
			m_pAnimGraph->m_lFullStates[iStateAG].m_iBaseAnimLayer = iAnimLayer;
			for (int32 i = 0; i < Len; i++)
			{
				CXRAG2_AnimLayer* pAnimLayer = &(m_pAnimGraph->m_lFullAnimLayers[iAnimLayer]);
				pAnimLayer->m_iAnim = pBlock->m_lspStates[iState]->m_lStateAnims[i].m_iAnim;
				if (pAnimLayer->m_iAnim < 0)
				{
					OnError(CStrF("AnimLayer corrupt! :%s", pBlock->m_lspStates[iState]->m_Name.Str()));
					return false;
				}
				pAnimLayer->m_AnimFlags = pBlock->m_lspStates[iState]->m_lStateAnims[i].m_AnimFlags;
				pAnimLayer->m_iBaseJoint = pBlock->m_lspStates[iState]->m_lStateAnims[i].m_iBaseJoint;
				pAnimLayer->m_TimeOffset = pBlock->m_lspStates[iState]->m_lStateAnims[i].m_TimeOffset;
				pAnimLayer->m_TimeScale = pBlock->m_lspStates[iState]->m_lStateAnims[i].m_TimeScale;
				pAnimLayer->m_Opacity = pBlock->m_lspStates[iState]->m_lStateAnims[i].m_Opacity;
				pAnimLayer->m_iTimeControlProperty = pBlock->m_lspStates[iState]->m_lStateAnims[i].m_iTimeControlProperty;
				pAnimLayer->m_iMergeOperator = pBlock->m_lspStates[iState]->m_lStateAnims[i].m_iMergeOperator;

				//m_pAnimGraph->m_lAnimLayersMap[iAnimLayer] = iAnimLayer;
				iAnimLayer++;
			}
			// Set num anim layers
			m_pAnimGraph->m_lFullStates[iStateAG].m_nAnimLayers = iAnimLayer - m_pAnimGraph->m_lFullStates[iStateAG].m_iBaseAnimLayer;

			// Actions
			m_pAnimGraph->m_lFullStates[iStateAG].m_iBaseAction = iAction;
			for (int jAction = 0; jAction < pBlock->m_lspStates[iState]->m_lActions.Len(); jAction++, iAction++)
			{
				CXRAG2C_Action* pAction = &(pBlock->m_lspStates[iState]->m_lActions[jAction]);

				CStr ActionName = pBlock->m_lspStates[iState]->m_Name + "." + pAction->m_Name;
				uint32 ActionHashKey = StringToHash(ActionName);
				CXRAG2_ActionHashEntry ActionHashEntry(ActionHashKey, iAction);
				m_lActionHashNames[iAction] = ActionName;
				m_pAnimGraph->m_lActionHashEntries[iAction] = ActionHashEntry;
	#ifndef M_RTM
				m_pAnimGraph->m_lExportedActionNames[iExportedActionName++] = ActionName;
	#endif

				// Movetokens
				m_pAnimGraph->m_lFullActions[iAction].m_iBaseMoveToken = iMoveToken;
				m_pAnimGraph->m_lFullActions[iAction].m_nMoveTokens = pAction->m_lMoveTokens.Len();
				for (int32 iMT = 0; iMT < pAction->m_lMoveTokens.Len(); iMT++)
				{
					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_TokenID = pAction->m_lMoveTokens[iMT].m_TokenID;

					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_iTargetGraphBlock = pAction->m_lMoveTokens[iMT].m_iTargetGraphBlock;
					
					// Moved outside (might reference a block that's not initiated yet)
					/*CXRAG2_GraphBlock* pBlock = pAction->m_lMoveTokens[iMT].m_iTargetGraphBlock != -1 ? &m_pAnimGraph->m_lGraphBlocks[pAction->m_lMoveTokens[iMT].m_iTargetGraphBlock] : pAGBlock;

					if (pAction->m_lMoveTokens[iMT].m_iTargetState == AG2_STATEINDEX_TERMINATE)
					{
						m_pAnimGraph->m_lMoveTokens[iMoveToken].m_iTargetState = AG2_STATEINDEX_TERMINATE;
					}
					else if (pAction->m_lMoveTokens[iMT].m_iTargetState == AG2_STATEINDEX_STARTAG)
					{
						m_pAnimGraph->m_lMoveTokens[iMoveToken].m_iTargetState = AG2_STATEINDEX_STARTAG;
					}
					else
					{
						m_pAnimGraph->m_lMoveTokens[iMoveToken].m_iTargetState = pBlock->m_iStateFullStart + pAction->m_lMoveTokens[iMT].m_iTargetState;
					}*/
						
					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_AnimBlendDuration = pAction->m_lMoveTokens[iMT].m_AnimBlendDuration;
					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_AnimBlendDelay = pAction->m_lMoveTokens[iMT].m_AnimBlendDelay;
					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_AnimTimeOffset = pAction->m_lMoveTokens[iMT].m_AnimTimeOffset;
					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_TargetStateType = pAction->m_lMoveTokens[iMT].m_TargetStateType;
					iMoveToken++;
				}
				//m_pAnimGraph->m_lActionsMap[iAction] = iAction;

				// EffectInstances
				int iBaseEffectInstance = pBlock->m_lspStates[iState]->m_lActions[jAction].m_iBaseEffectInstance;
				int nEffectInstances = pBlock->m_lspStates[iState]->m_lActions[jAction].m_nEffectInstances;
				m_pAnimGraph->m_lFullActions[iAction].m_iBaseEffectInstance = iStateEffectInstanceBase + iBaseEffectInstance;
				m_pAnimGraph->m_lFullActions[iAction].m_nEffectInstances = pBlock->m_lspStates[iState]->m_lActions[jAction].m_nEffectInstances;
				for (int jEffectInstance = 0; jEffectInstance < nEffectInstances; jEffectInstance++)
				{
					int iAGEffectInstance = iStateEffectInstanceBase + iBaseEffectInstance + jEffectInstance;
					CXRAG2C_EffectInstance* pIMEffectInstance = &(pBlock->m_lspStates[iState]->m_lEffectInstances[iBaseEffectInstance + jEffectInstance]);
					CXRAG2_EffectInstance* pAGEffectInstance = &(m_pAnimGraph->m_lEffectInstances[iAGEffectInstance]);
					if (pAGEffectInstance->m_ID == AG2_EFFECTID_NULL)
					{
						pAGEffectInstance->m_ID = pIMEffectInstance->m_ID;
						pAGEffectInstance->m_iParams = iCallbackParam;
						int nParams = pIMEffectInstance->m_lParams.Len(); pAGEffectInstance->m_nParams = nParams;
						for (int iParam = 0; iParam < nParams; iParam++)
						{
							int16 Param = pIMEffectInstance->m_lParams[iParam];
							m_pAnimGraph->m_lCallbackParams[iCallbackParam + iParam] = Param;
						}
						iCallbackParam += nParams;
						iEffectInstance++;
					}
				}
			}

			iStateEffectInstanceBase += pBlock->m_lspStates[iState]->m_lEffectInstances.Len();

			// Nodes
			if (pBlock->m_lspStates[iState]->m_lConditionNodes.Len() > 0)
			{
//				uint8 iStatePropertyParams = 0;
				m_pAnimGraph->m_lFullStates[iStateAG].m_iBasePropertyParam = iCallbackParam;
				m_pAnimGraph->m_lFullStates[iStateAG].m_iBaseNode = iNode;
				for (int jNode = 0; jNode < pBlock->m_lspStates[iState]->m_lConditionNodes.Len(); jNode++, iNode++)
				{
					CXRAG2_ConditionNodeV2* pAGNodeV2 = &(m_pAnimGraph->m_lNodesV2[iNode]);
					CXRAG2C_ConditionNode* pIMNode = &(pBlock->m_lspStates[iState]->m_lConditionNodes[jNode]);

					// New version
					pAGNodeV2->SetProperty(pIMNode->m_iProperty);
					pAGNodeV2->SetPropertyType(pIMNode->m_PropertyType);
					pAGNodeV2->SetOperator(pIMNode->m_iOperator);
					// Depending on propertytype, set constant
					switch (pIMNode->m_PropertyType)
					{
					case AG2_PROPERTYTYPE_CONDITION:
					case AG2_PROPERTYTYPE_FLOAT:
						{
							pAGNodeV2->SetConstantFloat(pIMNode->m_ConstantFloat);
							break;
						}
					case AG2_PROPERTYTYPE_FUNCTION:
					case AG2_PROPERTYTYPE_INT:
						{
							pAGNodeV2->SetConstantInt(pIMNode->m_ConstantInt);
							break;
						}
					case AG2_PROPERTYTYPE_BOOL:
						{
							pAGNodeV2->SetConstantBool(pIMNode->m_ConstantBool);
							break;
						}
					}	
					pAGNodeV2->SetTrueAction(pIMNode->m_TrueAction);
					pAGNodeV2->SetFalseAction(pIMNode->m_FalseAction);
				}
			}
			else
			{
				m_pAnimGraph->m_lFullStates[iStateAG].m_iBasePropertyParam = AG2_CALLBACKPARAMINDEX_NULL;
				m_pAnimGraph->m_lFullStates[iStateAG].m_iBaseNode = AG2_NODEINDEX_NULL;
			}

			// StateConstants
			m_pAnimGraph->m_lFullStates[iStateAG].m_iBaseConstant = iStateConstant;
			m_pAnimGraph->m_lFullStates[iStateAG].m_nConstants = pBlock->m_lspStates[iState]->m_lConstants.Len();
			for (int jConstant = 0; jConstant < pBlock->m_lspStates[iState]->m_lConstants.Len(); jConstant++, iStateConstant++)
			{
				m_pAnimGraph->m_lStateConstants[iStateConstant].m_ID = pBlock->m_lspStates[iState]->m_lConstants[jConstant].m_ID;
				m_pAnimGraph->m_lStateConstants[iStateConstant].m_Value = pBlock->m_lspStates[iState]->m_lConstants[jConstant].m_Value;
			}

			iStateAG++;
		}


		// Switch states
		for (int32 iSwitchState = 0; iSwitchState < pBlock->m_lspSwitchStates.Len(); iSwitchState++)
		{
#ifndef M_RTM
			m_pAnimGraph->m_lExportedSwitchStateNames[iExportedSwitchStateName++] = pBlock->m_lspSwitchStates[iSwitchState]->m_Name;
#endif
			// Misc
			CXRAG2_SwitchState* pState = &(m_pAnimGraph->m_lSwitchStates[iSwitchStateAG]);
			pState->m_Priority = pBlock->m_lspSwitchStates[iSwitchState]->m_Priority;
			pState->m_iProperty = pBlock->m_lspSwitchStates[iSwitchState]->m_iProperty;
			pState->m_PropertyType = pBlock->m_lspSwitchStates[iSwitchState]->m_PropertyType;
			pState->m_iBaseActionValIndex = iSwitchStateActionVal;
			// Count default action as well
			pState->m_NumActionVal = pBlock->m_lspSwitchStates[iSwitchState]->m_lActions.Len() + 1;

			int32 NumBaseActions = pBlock->m_lspSwitchStates[iSwitchState]->m_lActions.Len();
			for (int jAction = 0; jAction < NumBaseActions + 1; jAction++)
			{
				CXRAG2C_Action* pAction = jAction < NumBaseActions ? &(pBlock->m_lspSwitchStates[iSwitchState]->m_lActions[jAction]) : &(pBlock->m_lspSwitchStates[iSwitchState]->m_DefaultAction);
				// Set switchactionval (not needed for the default action I guess....)
				m_pAnimGraph->m_lSwitchStateActionVals[iSwitchStateActionVal].m_ConstantInt = pAction->m_ActionVal.m_ConstantValueInt;
				// REMOVE, NOT NEEDED I GUESS!!! (just baseindex + index when searching for it later)
				//m_pAnimGraph->m_lSwitchStateActionVals[iSwitchStateActionVal].m_iAction = jAction;
				
				

				// No actions
				/*CStr ActionName = pBlock->m_lspSwitchStates[iSwitchState]->m_Name + "." + pAction->m_Name;
				uint32 ActionHashKey = StringToHash(ActionName);
				CXRAG2_ActionHashEntry ActionHashEntry(ActionHashKey, iAction);
				m_lActionHashNames[iAction] = ActionName;
				m_pAnimGraph->m_lActionHashEntries[iAction] = ActionHashEntry;
#ifndef M_RTM
				m_pAnimGraph->m_lExportedActionNames[iExportedActionName++] = ActionName;
#endif
				*/

				// Movetokens
				//m_pAnimGraph->m_lFullActions[iAction].m_iBaseMoveToken = iMoveToken;
				//m_pAnimGraph->m_lFullActions[iAction].m_nMoveTokens = pAction->m_lMoveTokens.Len();
				// Use the first movetoken
				if (!pAction->m_lMoveTokens.Len())
					return false;

				{
					m_pAnimGraph->m_lSwitchStateActionVals[iSwitchStateActionVal].m_iMoveToken = iMoveToken;
					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_TokenID = pAction->m_lMoveTokens[0].m_TokenID;
					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_iTargetGraphBlock = pAction->m_lMoveTokens[0].m_iTargetGraphBlock;
					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_AnimBlendDuration = pAction->m_lMoveTokens[0].m_AnimBlendDuration;
					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_AnimBlendDelay = pAction->m_lMoveTokens[0].m_AnimBlendDelay;
					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_AnimTimeOffset = pAction->m_lMoveTokens[0].m_AnimTimeOffset;
					m_pAnimGraph->m_lMoveTokens[iMoveToken].m_TargetStateType = pAction->m_lMoveTokens[0].m_TargetStateType;
					iMoveToken++;
				}

				iSwitchStateActionVal++;
			}

			iStateEffectInstanceBase += pBlock->m_lspSwitchStates[iSwitchState]->m_lEffectInstances.Len();

			iSwitchStateAG++;
		}
	}

	// Remap states
	for (int32 iBlock = 0; iBlock < m_lspGraphBlocks.Len(); iBlock++)
	{
		CXRAG2C_GraphBlock* pBlock = m_lspGraphBlocks[iBlock];
		CXRAG2_GraphBlock* pAGBlock = &m_pAnimGraph->m_lGraphBlocks[iBlock];
		int32 iStateStart = pAGBlock->m_iStateFullStart;
		int32 iSwitchStateStart = pAGBlock->m_iSwitchStateStart;
		int32 iReactionStart = pAGBlock->m_iReactionFullStart;
		// Reactions
		for (int32 iReaction = 0; iReaction < pBlock->m_Reactions.m_lReactions.Len(); iReaction++)
		{
			CXRAG2C_Reaction* pReaction = &(pBlock->m_Reactions.m_lReactions[iReaction]);
			CXRAG2_Reaction* pAGReaction = &(m_pAnimGraph->m_lFullReactions[iReactionStart + iReaction]);
			int32 iReactionMT = pAGReaction->m_iBaseMoveToken;
			for (int32 iMT = 0; iMT < pReaction->m_lMoveTokens.Len(); iMT++)
			{
//				int32 iTargetGraphBlock = pReaction->m_lMoveTokens[iMT].m_iTargetGraphBlock;
				CXRAG2_GraphBlock* pAGTBlock = pReaction->m_lMoveTokens[iMT].m_iTargetGraphBlock != -1 ? &m_pAnimGraph->m_lGraphBlocks[pReaction->m_lMoveTokens[iMT].m_iTargetGraphBlock] : pAGBlock;
				if (pReaction->m_lMoveTokens[iMT].m_iTargetState == AG2_STATEINDEX_TERMINATE)
				{
					m_pAnimGraph->m_lMoveTokens[iReactionMT + iMT].m_iTargetState = AG2_STATEINDEX_TERMINATE;
				}
				else if (pReaction->m_lMoveTokens[iMT].m_iTargetState == AG2_STATEINDEX_STARTAG)
				{
					m_pAnimGraph->m_lMoveTokens[iReactionMT + iMT].m_iTargetState = AG2_STATEINDEX_STARTAG;
				}
				else if (pReaction->m_lMoveTokens[iMT].m_iTargetState == AG2_STATEINDEX_NULL)
				{
					CXRAG2_MoveToken* pToken = &m_pAnimGraph->m_lMoveTokens[iReactionMT + iMT];
					continue;
				}
				else
				{
//					int32 iTargetState = pReaction->m_lMoveTokens[iMT].m_iTargetState;
//					int32 iStateFullStart = pAGTBlock->m_iStateFullStart;
					if (pReaction->m_lMoveTokens[iMT].m_TargetStateType == AG2_STATETYPE_NORMAL)
					{
						m_pAnimGraph->m_lMoveTokens[iReactionMT + iMT].m_iTargetState = pAGTBlock->m_iStateFullStart + pReaction->m_lMoveTokens[iMT].m_iTargetState;
					}
					else
					{
						m_pAnimGraph->m_lMoveTokens[iReactionMT + iMT].m_iTargetState = pAGTBlock->m_iSwitchStateStart + pReaction->m_lMoveTokens[iMT].m_iTargetState;
					}
				}
			}
		}

		// States
		for (int32 iState = 0; iState < pBlock->m_lspStates.Len(); iState++)
		{
			int32 iBaseAction = m_pAnimGraph->m_lFullStates[iStateStart + iState].m_iBaseAction;
			for (int jAction = 0; jAction < pBlock->m_lspStates[iState]->m_lActions.Len(); jAction++)
			{
				CXRAG2C_Action* pAction = &(pBlock->m_lspStates[iState]->m_lActions[jAction]);
				CXRAG2_Action* pAGAction = &(m_pAnimGraph->m_lFullActions[iBaseAction + jAction]);
				int32 iStateMT = pAGAction->m_iBaseMoveToken;
				for (int32 iMT = 0; iMT < pAction->m_lMoveTokens.Len(); iMT++)
				{
//					int32 iTargetGraphBlock = pAction->m_lMoveTokens[iMT].m_iTargetGraphBlock;
					CXRAG2_GraphBlock* pAGTBlock = pAction->m_lMoveTokens[iMT].m_iTargetGraphBlock != -1 ? &m_pAnimGraph->m_lGraphBlocks[pAction->m_lMoveTokens[iMT].m_iTargetGraphBlock] : pAGBlock;
					if (pAction->m_lMoveTokens[iMT].m_iTargetState == AG2_STATEINDEX_TERMINATE)
					{
						m_pAnimGraph->m_lMoveTokens[iStateMT + iMT].m_iTargetState = AG2_STATEINDEX_TERMINATE;
					}
					else if (pAction->m_lMoveTokens[iMT].m_iTargetState == AG2_STATEINDEX_STARTAG)
					{
						m_pAnimGraph->m_lMoveTokens[iStateMT + iMT].m_iTargetState = AG2_STATEINDEX_STARTAG;
					}
					else
					{
//						int32 iTargetState = pAction->m_lMoveTokens[iMT].m_iTargetState;
//						int32 iStateFullStart = pAGTBlock->m_iStateFullStart;
						if (pAction->m_lMoveTokens[iMT].m_TargetStateType == AG2_STATETYPE_NORMAL)
						{
							m_pAnimGraph->m_lMoveTokens[iStateMT + iMT].m_iTargetState = pAGTBlock->m_iStateFullStart + pAction->m_lMoveTokens[iMT].m_iTargetState;
						}
						else
						{
							m_pAnimGraph->m_lMoveTokens[iStateMT + iMT].m_iTargetState = pAGTBlock->m_iSwitchStateStart + pAction->m_lMoveTokens[iMT].m_iTargetState;
						}
					}
				}
			}
		}

		// SwitchStates
		for (int32 iSwitchState = 0; iSwitchState < pBlock->m_lspSwitchStates.Len(); iSwitchState++)
		{
			int32 iBaseActionVal = m_pAnimGraph->m_lSwitchStates[iSwitchStateStart + iSwitchState].m_iBaseActionValIndex;
			int32 NumBaseActions = pBlock->m_lspSwitchStates[iSwitchState]->m_lActions.Len();
			for (int jAction = 0; jAction < NumBaseActions + 1; jAction++)
			{
				CXRAG2C_Action* pAction = jAction < NumBaseActions ? &(pBlock->m_lspSwitchStates[iSwitchState]->m_lActions[jAction]) : &(pBlock->m_lspSwitchStates[iSwitchState]->m_DefaultAction);
				CXRAG2_SwitchStateActionVal* pAGActionVal = &(m_pAnimGraph->m_lSwitchStateActionVals[iBaseActionVal+jAction]);
				int32 iStateMT = pAGActionVal->m_iMoveToken;
				for (int32 iMT = 0; iMT < 1; iMT++)
				{
//					int32 iTargetGraphBlock = pAction->m_lMoveTokens[iMT].m_iTargetGraphBlock;
					CXRAG2_GraphBlock* pAGTBlock = pAction->m_lMoveTokens[iMT].m_iTargetGraphBlock != -1 ? &m_pAnimGraph->m_lGraphBlocks[pAction->m_lMoveTokens[iMT].m_iTargetGraphBlock] : pAGBlock;
					if (pAction->m_lMoveTokens[iMT].m_iTargetState == AG2_STATEINDEX_TERMINATE)
					{
						m_pAnimGraph->m_lMoveTokens[iStateMT + iMT].m_iTargetState = AG2_STATEINDEX_TERMINATE;
					}
					else if (pAction->m_lMoveTokens[iMT].m_iTargetState == AG2_STATEINDEX_STARTAG)
					{
						m_pAnimGraph->m_lMoveTokens[iStateMT + iMT].m_iTargetState = AG2_STATEINDEX_STARTAG;
					}
					else
					{
//						int32 iTargetState = pAction->m_lMoveTokens[iMT].m_iTargetState;
//						int32 iStateFullStart = pAGTBlock->m_iStateFullStart;
						if (pAction->m_lMoveTokens[iMT].m_TargetStateType == AG2_STATETYPE_NORMAL)
						{
							m_pAnimGraph->m_lMoveTokens[iStateMT + iMT].m_iTargetState = pAGTBlock->m_iStateFullStart + pAction->m_lMoveTokens[iMT].m_iTargetState;
						}
						else
						{
							m_pAnimGraph->m_lMoveTokens[iStateMT + iMT].m_iTargetState = pAGTBlock->m_iSwitchStateStart + pAction->m_lMoveTokens[iMT].m_iTargetState;
						}
					}
				}
			}
		}
	}


	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraph2Compiler::ParseParamsStr(CStr _ParamsStr, TArray<int>& _lParams, CStr _Location)
{
	while (_ParamsStr != "")
	{
		CStr ParamStr = _ParamsStr.GetStrSep(",");
		int ParamValue = 0;
		if (!EvalConstantExpressionInt(ParamStr, ParamValue, NULL, _Location))
			return false;

		_lParams.Add(ParamValue);
	}
	return true;
}

//--------------------------------------------------------------------------------


//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXR_AnimGraph2Compiler::DumpLog(CStr _LogFileName)
{
	M_TRY
	{
		CCFile File;
		File.Open(_LogFileName, CFILE_WRITE);

		File.Writeln(CStrF("AnimGraph: %s", (char*)m_pAnimGraph->m_Name));
		File.Writeln(CStrF("nStates: %d", m_pAnimGraph->m_lFullStates.Len()));
		File.Writeln(CStrF("nNodesV2: %d", m_pAnimGraph->m_lNodesV2.Len()));
		File.Writeln(CStrF("nActions: %d", m_pAnimGraph->m_lFullActions.Len()));
		File.Writeln(CStrF("StartBlock: %d", m_iStartBlock));
		File.Writeln("");

		OnNote(CStrF("Dumping AnimGraph to logfile '%s'", (char*)_LogFileName));

		/*for (int iState = 0; iState < m_pAnimGraph->m_lFullStates.Len(); iState++)
			DumpLog_State(&File, iState);*/
		for (int iBlock = 0; iBlock < m_lspGraphBlocks.Len(); iBlock++)
			DumpLog_GraphBlock(&File, iBlock);

/*
		if (m_pAnimGraph->m_lAnimLayersMap.Len() > 0)
		{
			File.Writeln("");
			File.Writeln("AnimLayersMap:");

			for (int jAnimLayer = 0; jAnimLayer < m_pAnimGraph->m_lAnimLayersMap.Len(); jAnimLayer++)
			{
			int iAnimLayer = m_pAnimGraph->m_lAnimLayersMap[jAnimLayer];
			File.Writeln(CStrF("  %d -> %d", jAnimLayer, iAnimLayer));
			}
		}
*/

		if (m_pAnimGraph->m_lFullAnimLayers.Len() > 0)
		{
			File.Writeln("");
			File.Writeln("AnimLayers:");

			int nAnimLayers = m_pAnimGraph->GetNumAnimLayers();
			for (int iAnimLayer = 0; iAnimLayer < nAnimLayers; iAnimLayer++)
			{
				const CXRAG2_AnimLayer* pAnimLayer = m_pAnimGraph->GetAnimLayer(iAnimLayer);
				File.Writeln(CStrF("  %d: iAnim %d", iAnimLayer, pAnimLayer->GetAnimIndex()));
			}
		}

		if (m_pAnimGraph->m_lActionHashEntries.Len() > 0)
		{
			File.Writeln("");
			File.Writeln("ActionHash:");

			for (int iAction = 0; iAction < m_pAnimGraph->m_lActionHashEntries.Len(); iAction++)
			{
				File.Writeln(CStrF("Action %d: Name '%s', HashKey %X", 
								  m_pAnimGraph->m_lActionHashEntries[iAction].m_iAction,
//								  (char*)m_lActionHashNames[iAction], 
#ifndef M_RTM
								  (char*)m_pAnimGraph->m_lExportedActionNames[iAction], 
#endif
								  m_pAnimGraph->m_lActionHashEntries[iAction].m_HashKey));
			}
		}

		File.Writeln("");
		File.Writeln("<EndOfLog>");
		File.Close();
	}
	M_CATCH(
	catch (...)
	{
		OnWarning(CStrF("Failed to dump AnimGraph to logfile '%s'.", (char*)_LogFileName));
	}
	)
}

//--------------------------------------------------------------------------------

void CXR_AnimGraph2Compiler::DumpLog_GraphBlock(CCFile* _pFile, int _iBlock)
{
	CXRAG2C_GraphBlock* pBlock = m_lspGraphBlocks[_iBlock];
	_pFile->Writeln(CStrF("Block%d (%s)", _iBlock, pBlock->m_Name.GetStr()));
	_pFile->Writeln(CStr("{"));
	for (int32 i = 0; i < pBlock->m_lspStates.Len(); i++)
	{
		DumpLog_State(_pFile, _iBlock, i);
	}
	_pFile->Writeln(CStr("}"));
}

void CXR_AnimGraph2Compiler::DumpLog_State(CCFile* _pFile, int _iBlock, int _iState)
{
	//CXRAG2_State* pState = &(m_pAnimGraph->m_lFullStates[_iState]);
	CXRAG2C_GraphBlock* pBlock = m_lspGraphBlocks[_iBlock];
	CXRAG2C_State* pState = pBlock->m_lspStates[_iState];
	_pFile->Writeln(CStrF("State%d (%s)", _iState, pState->m_Name.Str()));
	_pFile->Writeln(CStr("{"));
/*	_pFile->Writeln(CStrF("  Priority: %d", pState->GetPriority()));

	const CXRAG2_AnimLayer* pAnimLayer = m_pAnimGraph->GetAnimLayer(pState->GetBaseAnimLayerIndex());
	if ((pAnimLayer != NULL) && (pAnimLayer->GetAnimIndex() != -1))
		_pFile->Writeln(CStrF("  Animation: %d", pAnimLayer->GetAnimIndex()));

	_pFile->Writeln(CStrF("  iBaseAction: %d", pState->GetBaseActionIndex()));
	_pFile->Writeln(CStrF("  nActions: %d", m_lpStates[_iState]->m_lActions.Len()));

	if (pState->GetNumConstants() > 0)
	{
		_pFile->Writeln(CStr(""));
		_pFile->Writeln(CStrF("  StateConstants: (iBaseConstant=%d)", pState->GetBaseConstantIndex()));
		for (int jStateConstant = 0; jStateConstant < pState->GetNumConstants(); jStateConstant++)
		{
			int iStateConstant = pState->GetBaseConstantIndex() + jStateConstant;
			_pFile->Writeln(CStrF("    %s = %3.3f", (char*)m_lpStates[_iState]->m_lConstants[jStateConstant].m_Name, 
												   m_pAnimGraph->m_lStateConstants[iStateConstant].m_Value));
		}
	}

	if (m_lpStates[_iState]->m_lConditions.Len() > 0)
	{
		_pFile->Writeln(CStr(""));
		_pFile->Writeln(CStr("  ConditionPrio:"));
		for (int jCondition = 0; jCondition < m_lpStates[_iState]->m_lConditions.Len(); jCondition++)
		{
			for (int iCondition = 0; iCondition < m_lpStates[_iState]->m_lConditions.Len(); iCondition++)
			{
				if ((m_lpStates[_iState]->m_lConditions[iCondition].m_SortedIndex & ~AG2C_CONDITION_SORTEDBIT) == jCondition)
				{
					CXRAG2_ICallbackParams IParams(m_lpStates[_iState]->m_lConditions[iCondition].m_lPropertyParams.GetBasePtr(), m_lpStates[_iState]->m_lConditions[iCondition].m_lPropertyParams.Len());
					_pFile->Writeln(CStrF("    %s", 
										 (char*)DumpLog_GetConditionStr(m_lpStates[_iState]->m_lConditions[iCondition].m_iProperty, &IParams,
																		m_lpStates[_iState]->m_lConditions[iCondition].m_iOperator, 
																		m_lpStates[_iState]->m_lConditions[iCondition].m_ConstantValue)));
					break;
				}
			}
		}
	}

	if (pState->GetBaseNodeIndex() != AG2_NODEINDEX_NULL)
	{
		_pFile->Writeln(CStr(""));
		_pFile->Writeln(CStrF("  NodeTree: (iBaseNode=%d, nNodes=%d)", pState->GetBaseNodeIndex(), m_lpStates[_iState]->m_lConditionNodes.Len()));
		DumpLog_Node(_pFile, pState->GetBaseActionIndex(), pState->GetBaseNodeIndex(), pState->GetBasePropertyParamIndex(), 0, "    ", "");
	}*/

	_pFile->Writeln(CStr("}"));
	_pFile->Writeln(CStr(""));
}

//--------------------------------------------------------------------------------

void CXR_AnimGraph2Compiler::DumpLog_Node(CCFile* _pFile, int _iBaseAction, int _iBaseNode, int _iStatePropertyParamsBase, int _iNode, CStr _Indent, CStr _BranchSymbol)
{
	const char* Indent = ((char*)_Indent != NULL) ? (char*)_Indent : "";
	const char* BranchSymbol = ((char*)_BranchSymbol != NULL) ? (char*)_BranchSymbol : "";
	CXRAG2_ConditionNodeV2* pNode = &(m_pAnimGraph->m_lNodesV2[_iBaseNode + _iNode]);
	_pFile->Writeln(CStrF("%s%s%s", (char*)Indent, (char*)BranchSymbol, (char*)DumpLog_GetNodeStr(_iBaseAction, _iBaseNode, _iStatePropertyParamsBase, _iNode)));

	if (_BranchSymbol != "")
	{
		if (_BranchSymbol == " |-")
			_Indent += " | ";
		else
			_Indent += "   ";
	}

	DumpLog_NodeAction(_pFile, _iBaseAction, pNode->GetTrueAction(), _iBaseNode, _iStatePropertyParamsBase, _iNode, _Indent , " |-");
	DumpLog_NodeAction(_pFile, _iBaseAction, pNode->GetFalseAction(), _iBaseNode, _iStatePropertyParamsBase, _iNode, _Indent, " '-");
}

//--------------------------------------------------------------------------------

void CXR_AnimGraph2Compiler::DumpLog_NodeAction(CCFile* _pFile, int _iBaseAction, int _NodeAction, int _iBaseNode, int _iStatePropertyParamsBase, int _iNode, CStr _Indent, CStr _BranchSymbol)
{
	/*if (_NodeAction == AG2_NODEACTION_FAILPARSE)
	{
		_pFile->Writeln(CStrF("%s%s[FAIL]", (char*)_Indent, (char*)_BranchSymbol));
		return;
	}

	if ((_NodeAction & AG2_NODEACTION_TYPEMASK) == AG2_NODEACTION_ENDPARSE)
	{
		int16 iAction = _iBaseAction + (_NodeAction & AG2_NODEACTION_ACTIONMASK);
		int8 TokenID = m_pAnimGraph->m_lFullActions[iAction].GetTokenID();
		int16 iTargetState = m_pAnimGraph->m_lFullActions[iAction].GetTargetStateIndex();
		if (iTargetState != AG2_STATEINDEX_NULL)
		{
			CStr TokenName;
			if (TokenID == AG2_TOKENID_NULL)
				TokenName = "CurToken";
			else
				TokenName = CStrF("Token%d", TokenID);

			if (iTargetState != AG2_STATEINDEX_TERMINATE)
				_pFile->Writeln(CStrF("%s%s[A%03d]  %s -> State%d (%s)", (char*)_Indent, (char*)_BranchSymbol, iAction, (char*)TokenName, iTargetState, (char*)m_lpStates[iTargetState]->m_Name));
			else
				_pFile->Writeln(CStrF("%s%s[A%03d]  %s -> %s", (char*)_Indent, (char*)_BranchSymbol, iAction, (char*)TokenName, "TERMINATE"));
		}
		else
			_pFile->Writeln(CStrF("%s%s[A%03d]", (char*)_Indent, (char*)_BranchSymbol, iAction));
		
		if (m_pAnimGraph->m_lFullActions[iAction].GetNumEffectInstances() > 0)
		{
			CStr EffectIndent = _Indent;
			if (_BranchSymbol != "")
			{
				if (_BranchSymbol == " |-")
					EffectIndent += " | ";
				else
					EffectIndent += "   ";
			}

			int nEffectInstances = m_pAnimGraph->m_lFullActions[iAction].GetNumEffectInstances();
			for (int jEffectInstance = 0; jEffectInstance < nEffectInstances; jEffectInstance++)
			{
				int iEffectInstance = m_pAnimGraph->m_lFullActions[iAction].GetBaseEffectInstanceIndex() + jEffectInstance;
				int EffectID = m_pAnimGraph->m_lEffectInstances[iEffectInstance].m_ID;
				CStr EffectName = m_lEffects[GetEffectIndexByID(EffectID)].m_Name;
				CXRAG2_ICallbackParams Params = m_pAnimGraph->GetICallbackParams(m_pAnimGraph->m_lEffectInstances[iEffectInstance].m_iParams, m_pAnimGraph->m_lEffectInstances[iEffectInstance].m_nParams);
				CStr EffectParamsStr = Params.GetStr();
				if (jEffectInstance < (nEffectInstances - 1))
					_pFile->Writeln(CStrF("%s |-[E%03d]  %s(%s)", (char*)EffectIndent, iEffectInstance, (char*)EffectName, (char*)EffectParamsStr));
				else
					_pFile->Writeln(CStrF("%s '-[E%03d]  %s(%s)", (char*)EffectIndent, iEffectInstance, (char*)EffectName, (char*)EffectParamsStr));
			}
		}

		return;
	}

	if ((_NodeAction & AG2_NODEACTION_TYPEMASK) == AG2_NODEACTION_PARSENODE)
		DumpLog_Node(_pFile, _iBaseAction, _iBaseNode, _iStatePropertyParamsBase, (_NodeAction & AG2_NODEACTION_NODEMASK), _Indent, _BranchSymbol);*/
}

//--------------------------------------------------------------------------------

CStr CXR_AnimGraph2Compiler::DumpLog_GetNodeStr(int _iBaseAction, int _iBaseNode, int _iStatePropertyParamsBase, int _iNode)
{
	CXRAG2_ConditionNodeV2* pNode = &(m_pAnimGraph->m_lNodesV2[_iBaseNode + _iNode]);
	CStr ConditionStr;
	switch (pNode->GetPropertyType())
	{
	case AG2_PROPERTYTYPE_CONDITION:
	case AG2_PROPERTYTYPE_FLOAT:
		{
			ConditionStr = DumpLog_GetConditionStrFloat(pNode->GetProperty(), pNode->GetOperator(), pNode->GetConstantFloat());
			break;
		}
	case AG2_PROPERTYTYPE_FUNCTION:
	case AG2_PROPERTYTYPE_INT:
		{
			ConditionStr = DumpLog_GetConditionStrInt(pNode->GetProperty(), pNode->GetOperator(), pNode->GetConstantInt());
			break;
		}
	case AG2_PROPERTYTYPE_BOOL:
		{
			ConditionStr = DumpLog_GetConditionStrBool(pNode->GetProperty(), pNode->GetOperator(), pNode->GetConstantBool());
			break;
		}
	default:
		break;
	}
	return CStrF("[N%03d]  %s  [%s | %s]", _iBaseNode + _iNode, 
				ConditionStr.Str(),
				(char*)(DumpLog_GetNodeActionStr(_iBaseAction, _iBaseNode, pNode->GetTrueAction())), 
				(char*)(DumpLog_GetNodeActionStr(_iBaseAction, _iBaseNode, pNode->GetFalseAction())));
}

//--------------------------------------------------------------------------------

CStr CXR_AnimGraph2Compiler::DumpLog_GetConditionStrFloat(int _PropertyID, int _OperatorID, fp32 _Constant)
{
	return CStrF("(%s %s %.2f)", 
			(char*)(m_lPropertiesFloat[GetPropertyFloatIndexByID(_PropertyID)].m_Name),
			(char*)(m_lOperators[GetOperatorIndexByID(_OperatorID)].m_Name),
			_Constant);
}

CStr CXR_AnimGraph2Compiler::DumpLog_GetConditionStrInt(int _PropertyID, int _OperatorID, int _Constant)
{
	return CStrF("(%s %s %d)", 
		(char*)(m_lPropertiesInt[GetPropertyIntIndexByID(_PropertyID)].m_Name),
		(char*)(m_lOperators[GetOperatorIndexByID(_OperatorID)].m_Name),
		_Constant);
}

CStr CXR_AnimGraph2Compiler::DumpLog_GetConditionStrBool(int _PropertyID, int _OperatorID, int _Constant)
{
	return CStrF("(%s %s %d)", 
		(char*)(m_lPropertiesBool[GetPropertyBoolIndexByID(_PropertyID)].m_Name),
		(char*)(m_lOperators[GetOperatorIndexByID(_OperatorID)].m_Name),
		_Constant);
}

//--------------------------------------------------------------------------------

CStr CXR_AnimGraph2Compiler::DumpLog_GetNodeActionStr(int _iBaseAction, int _iBaseNode, int _NodeAction)
{
	if (_NodeAction == AG2_NODEACTION_FAILPARSE)
		return "FAIL";

	if ((_NodeAction & AG2_NODEACTION_TYPEMASK) == AG2_NODEACTION_ENDPARSE)
		return CStrF("A%03d", _iBaseAction + (_NodeAction & AG2_NODEACTION_ACTIONMASK));

	if ((_NodeAction & AG2_NODEACTION_TYPEMASK) == AG2_NODEACTION_PARSENODE)
		return CStrF("N%03d", _iBaseNode + (_NodeAction & AG2_NODEACTION_NODEMASK));

	return "    ";
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

char CXRAG2C_ConstExprParserBase::NextChar()
{
	return (m_Source.Str()[0]);
}

char CXRAG2C_ConstExprParserBase::ReadChar()
{
	char c = NextChar();
	m_Source = m_Source.Right(m_Source.Len() - 1);
	return c;
}

void CXRAG2C_ConstExprParserBase::SkipWhitespace()
{
	while (NextChar() == ' ')
		ReadChar();
}

bool CXRAG2C_ConstExprParserBase::IsDigit(char c)
{
	return ((c >= '0') && (c <= '9'));
}

bool CXRAG2C_ConstExprParserBase::IsAlpha(char c)
{
	return (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || (c == '_'));
}

void CXRAG2C_ConstExprParserBase::OnError(CStr _Msg)
{
	ConOutL(CStrF("AGC ConstExprParser Error: %s (%s)", (char*)_Msg, (char*)m_Location));
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserFloat::ParseConstant(fp32& _Result)
{
	CStr ConstantStr;
	while (IsDigit(NextChar()) || (NextChar() == '.') || (NextChar() == '-') || (NextChar() == 'x'))
		ConstantStr += CStr(ReadChar());

	return m_pCompiler->ResolveConstantValueFloat(ConstantStr, _Result, m_pState, m_Location);
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserFloat::ParseIdentifier(fp32& _Result)
{
	CStr IdentifierStr;
	while (IsAlpha(NextChar()) || IsDigit(NextChar()))
		IdentifierStr += CStr(ReadChar());

	return m_pCompiler->ResolveConstantValueFloat(IdentifierStr, _Result, m_pState, m_Location);
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserFloat::ParseParantheses(fp32& _Result)
{
	ReadChar();
	SkipWhitespace();
	if (!ParseExpr(_Result))
		return false;
	SkipWhitespace();
	if (NextChar() == ')')
		ReadChar();
	else
	{
		OnError(CStr("Missing closing parantheses."));
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserFloat::ParseTerm(fp32& _Result)
{
	if (NextChar() == '-')
	{
		ReadChar();
		SkipWhitespace();
		if (!ParseTerm(_Result))
			return false;

		_Result = -_Result;
		return true;
	}
	else if (NextChar() == '(')
		return ParseParantheses(_Result);
	else if (IsDigit(NextChar()))
		return ParseConstant(_Result);
	else if (IsAlpha(NextChar()))
		return ParseIdentifier(_Result);

	OnError(CStrF("Invalid character '%c'.", NextChar()));
	return false;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserFloat::ParseMulDivExpr(fp32& _Result, bool _bShiftResult)
{
	if (!_bShiftResult)
		if (!ParseTerm(_Result))
			return false;

	SkipWhitespace();
	if (NextChar() == '*')
	{
		ReadChar();
		SkipWhitespace();

		fp32 RightOperand;
		if (!ParseTerm(RightOperand))
			return false;

		_Result *= RightOperand;
		return ParseMulDivExpr(_Result, true);
	}
	else if (NextChar() == '/')
	{
		ReadChar();
		SkipWhitespace();
		fp32 RightOperand;
		if (!ParseTerm(RightOperand))
			return false;

		_Result /= RightOperand;
		return ParseMulDivExpr(_Result, true);
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserFloat::ParseAddSubExpr(fp32& _Result, bool _bShiftResult)
{
	if (!_bShiftResult)
		if (!ParseMulDivExpr(_Result, false))
			return false;

	SkipWhitespace();
	if (NextChar() == '+')
	{
		ReadChar();
		SkipWhitespace();
		fp32 RightOperand;
		if (!ParseMulDivExpr(RightOperand, false))
			return false;

		_Result += RightOperand;
		return ParseAddSubExpr(_Result, true);
	}
	else if (NextChar() == '-')
	{
		ReadChar();
		SkipWhitespace();
		fp32 RightOperand;
		if (!ParseMulDivExpr(RightOperand, false))
			return false;

		_Result -= RightOperand;
		return ParseAddSubExpr(_Result, true);
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserFloat::ParseExpr(fp32& _Result)
{
	if (m_Source == "")
	{
		_Result = 0;
		return true;
	}

	return ParseAddSubExpr(_Result, false);
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------


bool CXRAG2C_ConstExprParserInt::ParseConstant(int& _Result)
{
	CStr ConstantStr;
	while (IsDigit(NextChar()) || (NextChar() == '.') || (NextChar() == '-') || (NextChar() == 'x'))
		ConstantStr += CStr(ReadChar());

	return m_pCompiler->ResolveConstantValueInt(ConstantStr, _Result, m_pState, m_Location);
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserInt::ParseIdentifier(int& _Result)
{
	CStr IdentifierStr;
	while (IsAlpha(NextChar()) || IsDigit(NextChar()))
		IdentifierStr += CStr(ReadChar());

	return m_pCompiler->ResolveConstantValueInt(IdentifierStr, _Result, m_pState, m_Location);
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserInt::ParseParantheses(int& _Result)
{
	ReadChar();
	SkipWhitespace();
	if (!ParseExpr(_Result))
		return false;
	SkipWhitespace();
	if (NextChar() == ')')
		ReadChar();
	else
	{
		OnError(CStr("Missing closing parantheses."));
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserInt::ParseTerm(int& _Result)
{
	if (NextChar() == '-')
	{
		ReadChar();
		SkipWhitespace();
		if (!ParseTerm(_Result))
			return false;

		_Result = -_Result;
		return true;
	}
	else if (NextChar() == '(')
		return ParseParantheses(_Result);
	else if (IsDigit(NextChar()))
		return ParseConstant(_Result);
	else if (IsAlpha(NextChar()))
		return ParseIdentifier(_Result);

	OnError(CStrF("Invalid character '%c'.", NextChar()));
	return false;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserInt::ParseMulDivExpr(int& _Result, bool _bShiftResult)
{
	if (!_bShiftResult)
		if (!ParseTerm(_Result))
			return false;

	SkipWhitespace();
	if (NextChar() == '*')
	{
		ReadChar();
		SkipWhitespace();

		int RightOperand;
		if (!ParseTerm(RightOperand))
			return false;

		_Result *= RightOperand;
		return ParseMulDivExpr(_Result, true);
	}
	else if (NextChar() == '/')
	{
		ReadChar();
		SkipWhitespace();
		int RightOperand;
		if (!ParseTerm(RightOperand))
			return false;

		_Result /= RightOperand;
		return ParseMulDivExpr(_Result, true);
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserInt::ParseAddSubExpr(int& _Result, bool _bShiftResult)
{
	if (!_bShiftResult)
		if (!ParseMulDivExpr(_Result, false))
			return false;

	SkipWhitespace();
	if (NextChar() == '+')
	{
		ReadChar();
		SkipWhitespace();
		int RightOperand;
		if (!ParseMulDivExpr(RightOperand, false))
			return false;

		_Result += RightOperand;
		return ParseAddSubExpr(_Result, true);
	}
	else if (NextChar() == '-')
	{
		ReadChar();
		SkipWhitespace();
		int RightOperand;
		if (!ParseMulDivExpr(RightOperand, false))
			return false;

		_Result -= RightOperand;
		return ParseAddSubExpr(_Result, true);
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserInt::ParseExpr(int& _Result)
{
	if (m_Source == "")
	{
		_Result = 0;
		return true;
	}

	return ParseAddSubExpr(_Result, false);
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------


bool CXRAG2C_ConstExprParserBool::ParseConstant(bool& _Result)
{
	CStr ConstantStr;
	while (IsDigit(NextChar()) || (NextChar() == '.') || (NextChar() == '-') || (NextChar() == 'x'))
		ConstantStr += CStr(ReadChar());

	return m_pCompiler->ResolveConstantValueBool(ConstantStr, _Result, m_pState, m_Location);
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserBool::ParseIdentifier(bool& _Result)
{
	CStr IdentifierStr;
	while (IsAlpha(NextChar()) || IsDigit(NextChar()))
		IdentifierStr += CStr(ReadChar());

	return m_pCompiler->ResolveConstantValueBool(IdentifierStr, _Result, m_pState, m_Location);
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserBool::ParseParantheses(bool& _Result)
{
	ReadChar();
	SkipWhitespace();
	if (!ParseExpr(_Result))
		return false;
	SkipWhitespace();
	if (NextChar() == ')')
		ReadChar();
	else
	{
		OnError(CStr("Missing closing parantheses."));
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserBool::ParseTerm(bool& _Result)
{
	if (NextChar() == '(')
		return ParseParantheses(_Result);
	else if (IsDigit(NextChar()))
		return ParseConstant(_Result);
	else if (IsAlpha(NextChar()))
		return ParseIdentifier(_Result);

	OnError(CStrF("Invalid character '%c'.", NextChar()));
	return false;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_ConstExprParserBool::ParseExpr(bool& _Result)
{
	if (m_Source == "")
	{
		_Result = 0;
		return true;
	}

	//return ParseAddSubExpr(_Result, false);
	return ParseTerm(_Result);
}
