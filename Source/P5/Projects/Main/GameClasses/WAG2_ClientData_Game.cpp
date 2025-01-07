//--------------------------------------------------------------------------------

#include "PCH.h"

#include "WObj_Char.h"
#include "WObj_CharMsg.h"
#include "WAG2_ClientData_Game.h"
//#include "WAG_ClientData_Game.h"
#include "WObj_Misc/WObj_Ledge.h"
#include "WRPG/WRPGChar.h"
#include "WObj_Char/WObj_CharDarkling_ClientData.h"
#include "CConstraintSystem.h"
// Weapon anim stuff

#ifndef M_RTM
static bool s_bDebugOut = false;
#endif



void CWO_Clientdata_WeaponAnim_AnimGraph2::SetInitialProperties(const CWAG2I_Context* _pContext)
{
	// Set "type" (jackie/swat/normal)
	int32 Type = 0;
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)
	{
		pCD->m_AnimGraph2.GetAG2I()->AcquireAllResources(_pContext);
		const CXRAG2* pAG = pCD->m_AnimGraph2.GetAG2I()->GetAnimGraph(0);
		const CFStr Name = pAG ? pAG->m_Name : "";
		if (Name == "AG2Swat")
			Type = 1;
		else if (Name == "AGJackie")
			Type = 2;
	}
	SetPropertyInt(PROPERTYWEAPONAG_INT_TYPE, Type);
}

void CWO_Clientdata_WeaponAnim_AnimGraph2::UpdateImpulseState(const CWAG2I_Context* _pContext)
{
	// Not much here atm
}

void CWO_Clientdata_WeaponAnim_AnimGraph2::UpdateItemProperties(CRPG_Object* _pRoot, CRPG_Object_Item* _pItem, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD)
{
	SetPropertyInt(PROPERTYWEAPONAG_INT_ISCROUCHING, CWObject_Character::Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH);
	if (!_pItem)
		return;

	// Update item properties from given item
	if (_pItem->m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS)
		SetPropertyInt(PROPERTYWEAPONAG_INT_AMMOLOAD,_pCD->m_Darkness >= _pItem->GetAmmoDraw());
	else
		SetPropertyInt(PROPERTYWEAPONAG_INT_AMMOLOAD,_pItem->GetAmmo(_pRoot));
}





// Character stuff

void CWO_Clientdata_Character_AnimGraph2::AG2_RefreshStateInstanceProperties(const CWAG2I_Context* _pContext, const CWAG2I_StateInstance* _pStateInstance)
{
	CWO_ClientData_AnimGraph2Interface::AG2_RefreshStateInstanceProperties(_pContext, _pStateInstance);
}

void CWO_Clientdata_Character_AnimGraph2::AG2_OnEnterState(const CWAG2I_Context* _pContext, CAG2TokenID _TokenID, CAG2StateIndex _iState, CAG2AnimGraphID _iAnimGraph, CAG2ActionIndex _iEnterAction)
{
	// Get state flags and state constants that are useful to us
	//bool bGotPrev = (m_StateFlagsHi & AG2_STATEFLAGHI_EXACTDESTPOSITION) != 0;
	m_ImportantAGEvent = 0;
	CWAG2I* pAG2I = GetAG2I();

	if (!pAG2I || !pAG2I->AcquireAllResources(_pContext))
		return;

	if (_TokenID == AG2_TOKEN_MAIN)
	{
		//ConOut(CStrF("%s OnEnterstate, iState: %d iAction: %d",(_pContext->m_pWPhysState->IsServer() ? "Server" : "Client"),_iState, _iEnterAction));
		m_ForcedAimingType = -1;
		m_AnimphysMoveType = 0;
		m_AnimMoveScale = 1.0f;
		m_AdjustmentOffset = 0.0f;
		m_AdjustmentCutOff = 0.0f;
		m_MaxBodyOffset = 160;
		m_StopLength = 50.0f;
		m_ClothAnimScale = -1.0f;
		m_MaxLookAngleZ = 0.25f;
		m_MaxLookAngleY = 0.25f;
		m_OffsetX = 0.0f;
		m_OffsetY = 0.0f;
		m_ClothSimFreq = -1;
		m_IdleTurnThreshold = AG2_DEFAULTIDLETURNTHRESHOLD;
		m_StateFlagsLo = 0;
		m_StateFlagsHi = 0;
		m_iExactPositionState = -1;

		if (_iState >= 0)
		{
			const CXRAG2* pAG = pAG2I->GetAnimGraph(_iAnimGraph);
			const CXRAG2_State* pState = pAG->GetState(_iState);
			const CWAG2I_Token* pToken = m_pAG2I->GetToken(AG2_TOKEN_MAIN);
			const CXRAG2_GraphBlock* pBlock = pAG->GetGraphBlock(pToken->GetGraphBlock());
			CXRAG2_Impulse BlockCondition;
			if (pBlock)
				BlockCondition = pBlock->GetCondition();
			M_ASSERT(pState,"INVALID STATE");
			int32 NumConstants = pState->GetNumConstants();
			for (int jStateConstant = 0; jStateConstant < NumConstants; jStateConstant++)
			{
				int16 iStateConstant = pState->GetBaseConstantIndex() + jStateConstant;
				const CXRAG2_StateConstant* pStateConstant = pAG->GetStateConstant(iStateConstant);
				M_ASSERT(pStateConstant,"INVALID STATECONSTANT");
				switch (pStateConstant->m_ID)
				{
				case AG2_CONSTANT_ANIMMOVESCALE:
					{
						m_AnimMoveScale = pStateConstant->m_Value;
						continue;
					}
				case AG2_CONSTANT_STOPLENGTH:
					{
						m_StopLength = pStateConstant->m_Value;
						continue;
					}
				case AG2_CONSTANT_ANIMPHYSMOVETYPE:
					{
						m_AnimphysMoveType = (uint8)pStateConstant->m_Value;
						continue;
					}
				case AG2_CONSTANT_MAXBODYOFFSET:
					{
						m_MaxBodyOffset = (uint8)pStateConstant->m_Value;
						continue;
					}
				case AG2_CONSTANT_AIMINGTYPE:
					{
						m_ForcedAimingType = (int8)pStateConstant->m_Value;
						continue;
					}
				case AG2_CONSTANT_TURNTHRESHOLD:
					{
						m_IdleTurnThreshold = (uint8)pStateConstant->m_Value;
						continue;
					}
				case AG2_CONSTANT_MAXLOOKANGLEZ:
					{
						m_MaxLookAngleZ = pStateConstant->m_Value;
						continue;
					}
				case AG2_CONSTANT_MAXLOOKANGLEY:
					{
						m_MaxLookAngleY = pStateConstant->m_Value;
						continue;
					}
				case AG2_CONSTANT_ADJUSTMENTOFFSET:
					{
						m_AdjustmentOffset = pStateConstant->m_Value;
						continue;
					}
				case AG2_CONSTANT_ADJUSTMENTCUTOFF:
					{
						m_AdjustmentCutOff = pStateConstant->m_Value;
						continue;
					}
				case AG2_CONSTANT_CLOTHANIMSCALE:
					{
						m_ClothAnimScale = pStateConstant->m_Value;
						continue;
					}
				case AG2_CONSTANT_CLOTHSIMFREQ:
					{
						m_ClothSimFreq = (int32)pStateConstant->m_Value;
						continue;
					}
				case AG2_CONSTANT_OFFSETX:
					{
						m_OffsetX = pStateConstant->m_Value;
						continue;
					}
				case AG2_CONSTANT_OFFSETY:
					{
						m_OffsetY = pStateConstant->m_Value;
						continue;
					}
				default:
					continue;
				}
			}	
			
			m_StateFlagsLo = pState->GetFlags(0);
			m_StateFlagsHi = pState->GetFlags(1);
			// Set vocap info, we want weapon type and "safe" level baked into one
			// Behavior<|weapontype>|safe type (idle/walk/run/sit...?) (skip weapon for now)
			// ok then, behavioractive flag to know behavior is running,
			int VocapInfo = 0;
			VocapInfo |= M_AG2_VOCAP_TEST_BEHAVIOR(m_StateFlagsLo);
			VocapInfo |= M_AG2_VOCAP_TEST_IDLE(m_StateFlagsHi);
			VocapInfo |= M_AG2_VOCAP_TEST_WALK(m_StateFlagsHi);
			VocapInfo |= M_AG2_VOCAP_TEST_CROUCHED(m_StateFlagsHi);
			VocapInfo |= M_AG2_VOCAP_TEST_RUN(m_StateFlagsHi);
			VocapInfo |= M_AG2_VOCAP_TEST_ONLYFACE(m_StateFlagsHi);
			VocapInfo |= M_AG2_VOCAP_TEST_ONLYUPPERBODY(m_StateFlagsHi);
			if (m_StateFlagsLo & AG2_STATEFLAG_BEHAVIORACTIVE)
				VocapInfo |= (BlockCondition.m_ImpulseValue) << AG2_VOCAP_FLAG_BEHAVIORTYPESHIFT;
			SetPropertyInt(PROPERTY_INT_VOCAPINFO, VocapInfo);

			// If this start needs exact start/endposition set it
			m_iExactPositionState = (m_StateFlagsHi & (AG2_STATEFLAGHI_EXACTDESTPOSITION |AG2_STATEFLAGHI_EXACTSTARTPOSITION) ? _iState : -1);
			if (m_iExactPositionState != -1 && !(m_StateFlagsHi & AG2_STATEFLAGHI_EXACTSTARTPOSITION))
			{
				m_MStartPosition = _pContext->m_pObj->GetPositionMatrix();
				//m_MStartPosition.GetRow(2) = GetUpVector();//CVec3Dfp32(0.0f,0.0f,1.0f);
				//m_MStartPosition.RecreateMatrix(2,0);
				/*m_MDestination = _pContext->m_pObj->GetPositionMatrix();
				CVec3Dfp32 Dir = m_MDestination.GetRow(0);
				m_MDestination.GetRow(0) = -m_MDestination.GetRow(0);
				Dir.k[2] = 0.0f;
				Dir.Normalize();
				m_MDestination.GetRow(2) = CVec3Dfp32(0,0,1);
				m_MDestination.GetRow(3) -= Dir * 56.5f;
				m_MDestination.RecreateMatrix(2,0);*/
			}
			/*else if (!bGotPrev)
			{
				m_MDestination = _pContext->m_pObj->GetPositionMatrix();
				CVec3Dfp32 Dir = m_MDestination.GetRow(0);
				Dir.k[2] = 0.0f;
				Dir.Normalize();
				m_MDestination.GetRow(3) += Dir * 60.0f;

			}*/
			/*	m_MDestination = _pContext->m_pObj->GetPositionMatrix();
				CVec3Dfp32 Dir = m_MDestination.GetRow(0);
				Dir.k[2] = 0.0f;
				Dir.Normalize();
				m_MDestination.GetRow(2) = CVec3Dfp32(0,0,1);
				m_MDestination.GetRow(3) -= Dir * 60.0f;
				m_MDestination.RecreateMatrix(2,0);
			}
			else
			{
				m_MDestination = _pContext->m_pObj->GetPositionMatrix();
				CVec3Dfp32 Dir = m_MDestination.GetRow(0);
				Dir.k[2] = 0.0f;
				Dir.Normalize();
				m_MDestination.GetRow(3) += Dir * 60.0f;

			}*/
		}
	}
	else if (_TokenID == AG2_TOKEN_EFFECT)
	{
		if (_iState < 0)
		{
			m_StateFlagsLoToken2 = 0;
			m_StateFlagsHiToken2 = 0;
			SetPropertyInt(PROPERTY_INT_VOCAPINFO2, 0);
		}
		else
		{
			const CXRAG2_State* pState = pAG2I->GetState(_iState,_iAnimGraph);
			M_ASSERT(pState,"INVALID STATE");
			m_StateFlagsLoToken2 = pState->GetFlags(0);
			m_StateFlagsHiToken2 = pState->GetFlags(1);
			int VocapInfo = 0;
			VocapInfo |= M_AG2_VOCAP_TEST_BEHAVIOR(m_StateFlagsLoToken2);
			VocapInfo |= M_AG2_VOCAP_TEST_IDLE(m_StateFlagsHiToken2);
			VocapInfo |= M_AG2_VOCAP_TEST_WALK(m_StateFlagsHiToken2);
			VocapInfo |= M_AG2_VOCAP_TEST_CROUCHED(m_StateFlagsHiToken2);
			VocapInfo |= M_AG2_VOCAP_TEST_RUN(m_StateFlagsHiToken2);
			VocapInfo |= M_AG2_VOCAP_TEST_ONLYFACE(m_StateFlagsHiToken2);
			VocapInfo |= M_AG2_VOCAP_TEST_ONLYUPPERBODY(m_StateFlagsHiToken2);
			SetPropertyInt(PROPERTY_INT_VOCAPINFO2, VocapInfo);
		}
	}
	else if (_TokenID == AG2_TOKEN_UNEQUIP)
	{
		if (_iState < 0)
		{
			m_StateFlagsLoToken3 = 0;
			// Temp for now
			m_StateFlagsHiToken2 = 0;
		}
		else
		{
			const CXRAG2_State* pState = pAG2I->GetState(_iState,_iAnimGraph);
			M_ASSERT(pState,"INVALID STATE");
			m_StateFlagsLoToken3 = pState->GetFlags(0);
			// Temp for now
			m_StateFlagsHiToken2 = pState->GetFlags(1);
		}
	}
}

bool CWO_Clientdata_Character_AnimGraph2::GetStateConstantsRequestPtrs(CWAG2I*& _pAG2I, const CXRAG2_State*& _pState)
{
	_pAG2I = GetAG2I();
	_pState = NULL;

	if (!_pAG2I)
		return false;

	const CWAG2I_Token* pToken = _pAG2I->GetTokenFromID(AG2_TOKEN_MAIN);
	if (!pToken)
		return false;

	const CWAG2I_StateInstance* pStateInstance = pToken->GetTokenStateInstance();
	if (pStateInstance)
		_pState = _pAG2I->GetState(pStateInstance->GetStateIndex(), pStateInstance->GetAnimGraphIndex());

	return (_pState ? _pState->GetNumConstants() > 0 : false);
}

bool CWO_Clientdata_Character_AnimGraph2::StanceSupported(int32 _WeaponType, int32 _Stance)
{
	M_ASSERT(_Stance < AG2_NUMSTANCES, "Invalid stance!");
	int32 Shift = _WeaponType + _Stance * AG2_IMPULSEVALUE_NUMWEAPONTYPES;
	int32 iBox = Shift / 8;
	M_ASSERT(iBox < 5,"WEAPONTYPE OR STANCE INVALID");
	Shift = Shift % 8;
	return (m_SupportedStances[iBox] & (1 << Shift)) != 0;
}

void CWO_Clientdata_Character_AnimGraph2::SetSupportedStances(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAnimGraph, bool _bReset)
{
	if (m_bSupportedStancesSet)
		return;

	m_bSupportedStancesSet = true;

	// Clear out old supported stances
	for (int32 i = 0; i < 5; i++)
		m_SupportedStances[i] = 0;

	// Go through and check which stances are supported?
	if (!m_pAG2I->AcquireAllResources(_pContext))
		return;
	// Only check main type, skip crouch (should always be there...?)
	for (int32 i = 0; i < AG2_NUMSTANCES; i++)
	{
		for (int32 j = 0; j < AG2_IMPULSEVALUE_NUMWEAPONTYPES; j++)
		{
			CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_WEAPONTYPE,j + i * AG2_IMPULSEVALUE_STANCETYPEOFFSET);
			// Check if graphblock exists
			if (m_pAG2I->GraphBlockExists(Impulse,_iAnimGraph) != -1)
			{
				int32 Shift = (j + i * AG2_IMPULSEVALUE_NUMWEAPONTYPES);
				int32 iBox = Shift / 8;
				Shift = Shift % 8;
				m_SupportedStances[iBox] |= 1 << Shift;
			}
		}
	}
}

void CWO_Clientdata_Character_AnimGraph2::SetInitialProperties(const CWAG2I_Context* _pContext)
{
	// Set some initial properties
	SetPropertyBool(PROPERTY_BOOL_ISSERVER, _pContext->m_pWPhysState->IsServer());
	SetPropertyBool(PROPERTY_BOOL_ALWAYSTRUE,true);
	SetPropertyInt(PROPERTY_INT_STANCE,0);
	SetPropertyInt(PROPERTY_INT_MOVEVARIATION,0);
	SetPropertyInt(PROPERTY_INT_LOCALPLAYER,0);
}

// Matches weapontype to impulsevalues (including crouch/notcrouched, stance types etc...)
CXRAG2_Impulse CWO_Clientdata_Character_AnimGraph2::CheckWeaponType(const CWAG2I_Context* _pContext, CXRAG2_Impulse _Impulse, int32& _NewStanceStanding)
{
	CXRAG2_Impulse FinalImpulse;
	FinalImpulse = _Impulse;

	// Get current stance
	int32 Stance = GetPropertyInt(PROPERTY_INT_STANCE);

	// Change weapontype if needed
	if (_pContext->m_pWPhysState->IsServer())
	{
		// Just set new itemtype, should be same as ag item type
		FinalImpulse.m_ImpulseValue = _pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETEQUIPPEDANIMTYPE,true), _pContext->m_pObj->m_iObject);
	}
	else
	{
		// Calculate weapontype... (not crouched or in different stances..)
		FinalImpulse.m_ImpulseValue = (FinalImpulse.m_ImpulseValue % AG2_IMPULSEVALUE_STANCETYPEOFFSET);// - AG2_IMPULSEVALUE_NUMWEAPONTYPES;
		if (FinalImpulse.m_ImpulseValue >= AG2_IMPULSEVALUE_NUMWEAPONTYPES)
			FinalImpulse.m_ImpulseValue -= AG2_IMPULSEVALUE_NUMWEAPONTYPES;
	}
	if (FinalImpulse.m_ImpulseValue == 15)
	{
		// Ancient weapons, set ag type to gun
		FinalImpulse.m_ImpulseValue = AG2_IMPULSEVALUE_WEAPONTYPE_GUN;
	}
	
	// If we're idle gun, use normal explore!
	if (!GetPropertyBool(PROPERTY_BOOL_ISPLAYER) && 
		((FinalImpulse.m_ImpulseValue + Stance * AG2_IMPULSEVALUE_STANCETYPEOFFSET) == (AG2_IMPULSEVALUE_WEAPONTYPE_GUN + AG2_IMPULSEVALUE_STANCEOFFSET_IDLE)))
	{
		FinalImpulse.m_ImpulseValue = AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED;
	}
	
	// If we're a melee character and not in combat, go to idle unarmed...
	if (FinalImpulse.m_ImpulseValue == AG2_IMPULSEVALUE_WEAPONTYPE_MELEE && Stance != AG2_STANCETYPE_COMBAT)
	{
		FinalImpulse.m_ImpulseValue = AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED;
		Stance = AG2_STANCETYPE_IDLE;
	}
	else if (!StanceSupported(FinalImpulse.m_ImpulseValue, Stance)) // Check so that the stance is supported
	{
		// If wanted stance isn't supported check if any other stances will work
		Stance = -1;
		for (int32 i = 0; i < AG2_NUMSTANCES; i++)
		{
			if (StanceSupported(FinalImpulse.m_ImpulseValue,i))
			{
				Stance = i;
				break;
			}
		}
		if (Stance == -1)
			return _Impulse;
	}


	bool bIsCrouching = CWObject_Character::Char_GetPhysType(_pContext->m_pObj) == PLAYER_PHYS_CROUCH;
	if (bIsCrouching && FinalImpulse.m_ImpulseValue <= AG2_IMPULSEVALUE_NUMWEAPONTYPES)
	{
		// Set crouch on
		FinalImpulse.m_ImpulseValue += AG2_IMPULSEVALUE_NUMWEAPONTYPES;
	}
	else if (!bIsCrouching && FinalImpulse.m_ImpulseValue > AG2_IMPULSEVALUE_NUMWEAPONTYPES)
	{
		// Set crouch off
		FinalImpulse.m_ImpulseValue -= AG2_IMPULSEVALUE_NUMWEAPONTYPES;
	}
	// Add stanceoffset
	FinalImpulse.m_ImpulseValue += Stance * AG2_IMPULSEVALUE_STANCETYPEOFFSET;

	// NewStanceStanding doesn't care about crouch
	_NewStanceStanding = FinalImpulse.m_ImpulseValue % AG2_IMPULSEVALUE_NUMWEAPONTYPES; // Weapontype without crouch
	_NewStanceStanding += Stance * AG2_IMPULSEVALUE_STANCETYPEOFFSET;

	return FinalImpulse;
}

// Only done on token 0 for now...
void CWO_Clientdata_Character_AnimGraph2::UpdateImpulseState(const CWAG2I_Context* _pContext)
{
	if (!_pContext || !m_pAG2I->GetNumTokens() || !m_pAG2I->AcquireAllResources(_pContext))
		return;

	if (m_StateFlagsLo & AG2_STATEFLAG_HURTACTIVE)
		return;

	// Get current graphblock impulse type and values, and see if it matches current gameplay
	// state
	const CWAG2I_Token* pToken = m_pAG2I->GetToken(0);
	CAG2AnimGraphID iAnimGraph = pToken ? pToken->GetAnimGraphIndex() : -1;
	const CXRAG2_GraphBlock* pGraphBlock = iAnimGraph != -1 ? m_pAG2I->GetGraphBlock(pToken->GetGraphBlock(),iAnimGraph) : NULL;
	if (!pGraphBlock)
		return;

	CXRAG2_Impulse Impulse = pGraphBlock->GetCondition();

	int32 ControlMode = CWObject_Character::Char_GetControlMode(_pContext->m_pObj);
	switch (ControlMode)
	{
	case PLAYER_CONTROLMODE_ACTIONCUTSCENE:
		{
			// Current acs type
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
			int32 ACSType = (int)pCD->m_ControlMode_Param2;
			// Check so we're in correct actioncutscene
			CXRAG2_Impulse NewImpulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,ACSType);

			if ((ACSType != 0) && (Impulse != NewImpulse))
			{
				// Send impulse
				// Send the impulse
#ifndef M_RTM
				if (s_bDebugOut)
					ConOutL(CStrF("%s: GT: %f Sending Out impulse", _pContext->m_pWPhysState->IsServer() ? "S":"C", _pContext->m_GameTime.GetTime()));
#endif
				if (m_pAG2I->SendImpulse(_pContext, NewImpulse, 0))
				{
					// Only refresh token0
					m_pAG2I->RefreshToken(_pContext,0,0);
				}
			}
			break;
		}
	default:
		{
			CXRAG2_Impulse Impulse = pGraphBlock->GetCondition();
			switch (Impulse.m_ImpulseType)
			{
			case AG2_IMPULSETYPE_CONTROLMODE:
				{
					// Check so controlmode matches (crouch/notcrouch)?
					break;
				}
			case AG2_IMPULSETYPE_WEAPONTYPE:
				{
					// Check so that weapontype matches (only on server)
					int32 NewStanceStanding = 0;
					CXRAG2_Impulse NewImpulse = CheckWeaponType(_pContext, Impulse, NewStanceStanding);
					if (NewImpulse != Impulse)
					{
						// Send the impulse
#ifndef M_RTM
						if (s_bDebugOut)
							ConOutL(CStrF("%s: GT: %f Sending Out impulse", _pContext->m_pWPhysState->IsServer() ? "S":"C", _pContext->m_GameTime.GetTime()));
#endif
						if (m_pAG2I->SendImpulse(_pContext, NewImpulse, 0))
						{
							// Only refresh token0
							SetPropertyInt(PROPERTY_INT_STANCESTANDING,NewStanceStanding);
							m_pAG2I->Refresh(_pContext);
						}
					}
					break;
				}
			default:
				break;
			}
		}
	}
}

bool CWO_Clientdata_Character_AnimGraph2::ForceStance(const CWAG2I_Context* _pContext)
{
	if (!_pContext || !m_pAG2I->GetNumTokens() || !m_pAG2I->AcquireAllResources(_pContext))
		return false;

	// Get current graphblock impulse type and values, and see if it matches current gameplay
	// state
	const CWAG2I_Token* pToken = m_pAG2I->GetToken(0);
	CAG2AnimGraphID iAnimGraph = pToken ? pToken->GetAnimGraphIndex() : -1;
	CXRAG2* pAG = m_pAG2I->GetAnimGraph(iAnimGraph);
	if (!pAG)
		return false;

	CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_WEAPONTYPE,0);
	// Force ourselves to a stance
	int32 NewStanceStanding = 0;
	CXRAG2_Impulse NewImpulse = CheckWeaponType(_pContext, Impulse, NewStanceStanding);
	CAG2GraphBlockIndex iGB = pAG->GetMatchingGraphBlock(NewImpulse);
	if (iGB < 0)
		return false;
	const CXRAG2_GraphBlock* pBlock = pAG->GetGraphBlock(iGB);
	M_ASSERTHANDLER(pBlock,"GraphBlock Error", return false);
	m_pAG2I->MoveGraphBlock(_pContext,iAnimGraph,0,pBlock->GetStartMoveTokenIndex());
	
	return true;
}


bool CWO_Clientdata_Character_AnimGraph2::SendHurtImpulse(const CWAG2I_Context* _pContext, CXRAG2_Impulse _HurtImpulse, CAG2TokenID _iToken)
{
	// Check if we can send hurt impulses atm
	if (m_StateFlagsLo & AG2_STATEFLAG_HURTACTIVE)
		return false;

	return m_pAG2I->SendImpulse(_pContext,_HurtImpulse,_iToken);
}

bool CWO_Clientdata_Character_AnimGraph2::SendJumpImpulse(const CWAG2I_Context* _pContext, CXRAG2_Impulse _JumpImpulse, CAG2TokenID _iToken)
{
	// Check if we can send hurt impulses atm
	if ((_JumpImpulse.m_ImpulseValue == AG2_IMPULSEVALUE_JUMP_LAND) && !(m_StateFlagsLo & AG2_STATEFLAG_JUMPACTIVE))
		return false;

	return m_pAG2I->SendImpulse(_pContext,_JumpImpulse,_iToken);
}

bool CWO_Clientdata_Character_AnimGraph2::SendAttackImpulse(const CWAG2I_Context* _pContext, CXRAG2_Impulse _AttackImpulse, CAG2TokenID _iToken)
{
	// Check if we can send hurt impulses atm
	if ((m_StateFlagsLo|m_StateFlagsLoToken2|m_StateFlagsLoToken3) & (AG2_STATEFLAG_EQUIPPING|AG2_STATEFLAG_HURTACTIVE))
		return false;

	return m_pAG2I->SendImpulse(_pContext,_AttackImpulse,_iToken);
}

bool CWO_Clientdata_Character_AnimGraph2::SendAttackImpulseDualWield(const CWAG2I_Context* _pContext, CXRAG2_Impulse _AttackImpulse, CAG2TokenID _iToken)
{
	// Check if we can send hurt impulses atm
	if ((m_StateFlagsLo|m_StateFlagsLoToken3) & AG2_STATEFLAG_EQUIPPING)
		return false;

	return m_pAG2I->SendImpulse(_pContext,_AttackImpulse,_iToken);
}

bool CWO_Clientdata_Character_AnimGraph2::SendBehaviorImpulse(const CWAG2I_Context* _pContext, int32 _WantedBehavior, bool _bTurnIn)
{
	// Check if we can send behavior impulses atm
	/*if (m_StateFlagsLo & AG2_STATEFLAG_MELEEATTACKACTIVE)
		return false;*/
	// Set wanted behavior
	SetPropertyInt(PROPERTY_INT_WANTEDBEHAVIOR,_WantedBehavior);
	CAG2ImpulseType Type = _WantedBehavior == AG2_IMPULSEVALUE_BEHAVIOR_EXIT || _WantedBehavior == AG2_IMPULSEVALUE_BEHAVIOR_EXITMH ? AG2_IMPULSETYPE_BEHAVIORCONTROL : AG2_IMPULSETYPE_BEHAVIOR;
	if (_bTurnIn && _WantedBehavior != AG2_IMPULSEVALUE_BEHAVIOR_EXIT)
	{
		// Turn in first before starting the real behavior
		return m_pAG2I->SendImpulse(_pContext,CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR,7633),0);
	}
	/*if (_WantedBehavior == AG2_IMPULSEVALUE_BEHAVIOR_EXIT)
		ConOut(CStrF("Got Exit behavior impulse: iObj: %d Tick: %d",_pContext->m_pObj->m_iObject,_pContext->m_pWPhysState->GetGameTick()));
	else
		ConOut(CStrF("Got Behavior impulse: Beh: %d iObj: %d Tick: %d",_WantedBehavior,_pContext->m_pObj->m_iObject,_pContext->m_pWPhysState->GetGameTick()));*/

	return m_pAG2I->SendImpulse(_pContext,CXRAG2_Impulse(Type,_WantedBehavior),0);
}

bool CWO_Clientdata_Character_AnimGraph2::SendWalkStopImpulse(const CWAG2I_Context* _pContext)
{
	// Check if we can send behavior impulses atm
	if (m_StateFlagsHi & AG2_STATEFLAGHI_NOWALKSTOP)
		return false;
	// Send impulse
	return m_pAG2I->SendImpulse(_pContext, CXRAG2_Impulse(AG2_IMPULSETYPE_RESPONSE,AG2_IMPULSEVALUE_RESPONSE_WALKSTOP),0);
}

bool CWO_Clientdata_Character_AnimGraph2::SendWalkStartImpulse(const CWAG2I_Context* _pContext)
{
	// Check if we can send behavior impulses atm
	if (m_StateFlagsHi & AG2_STATEFLAGHI_NOWALKSTOP)
		return false;
	// Send impulse
	return m_pAG2I->SendImpulse(_pContext, CXRAG2_Impulse(AG2_IMPULSETYPE_RESPONSE,AG2_IMPULSEVALUE_RESPONSE_WALKSTART),0);
}

void CWO_Clientdata_Character_AnimGraph2::SendFacialImpulse(const CWAG2I_Context* _pContext, int16 _Group, int16 _Type)
{
	if (!m_pAG2I->AcquireAllResources(_pContext))
		return;
	SetPropertyInt(PROPERTY_INT_FACIAL_GROUP, _Group);
	SetPropertyInt(PROPERTY_INT_FACIAL_TYPE, _Type);
	// Ok, then a little more specialised, check if we have the token already
	const CWAG2I_Token* pToken = GetAG2I()->GetTokenFromID(AG2_TOKEN_FACIAL);
	if (pToken)
	{
		// Just send the impulse
		m_pAG2I->SendImpulse(_pContext,CXRAG2_Impulse(AG2_IMPULSETYPE_FACIAL,AG2_IMPULSEVALUE_FACIAL_CHANGETYPE),AG2_TOKEN_FACIAL);
	}
	else
	{
		// Create a token
		const CXRAG2* pAG = m_pAG2I->GetAnimGraph(0);
		if (pAG)
		{
			CAG2GraphBlockIndex iBlock = pAG->GetMatchingGraphBlock(CXRAG2_Impulse(AG2_IMPULSETYPE_FACIAL,0));
			if (iBlock != -1)
			{
				CAG2ReactionIndex iReaction = pAG->GetMatchingReaction(iBlock,CXRAG2_Impulse(AG2_IMPULSETYPE_FACIAL,AG2_IMPULSEVALUE_FACIAL_START));
				const CXRAG2_Reaction* pReaction = pAG->GetReaction(iReaction);
				if (pReaction)
				{
					m_pAG2I->MoveGraphBlock(_pContext,0,AG2_TOKEN_FACIAL,pReaction->GetBaseMoveTokenIndex());
				}
			}
		}
	}
}


#define AG2MOVEANGLETHRESHOLD			(0.08f)
#define Explore_TurnAngleFwd			(0.0f)
#define Explore_TurnAngleLeft45			(0.875f)
#define Explore_TurnAngleLeft90			(0.75f)
#define Explore_TurnAngleLeft135		(0.625f)
#define Explore_TurnAngleLeft180		(0.5f)
#define Explore_TurnAngleMaxRangeLeft	(0.125f)
#define Explore_HalfTurnAngleMaxRangeLeft (0.0625f)
#define Explore_HalfTurnAngleMaxRangeLeft4 (0.125f)

#define Explore_TurnAngleLeft90Crouch		(0.70f)
#define Explore_TurnAngleRight90Crouch		(0.30f)


#define Explore_TurnAngleRight45		(0.125f)
#define Explore_TurnAngleRight90		(0.25f)
#define Explore_TurnAngleRight135		(0.375f)
#define Explore_TurnAngleRight180		(0.5f)
#define Explore_TurnAngleMaxRangeRight	(0.125f)
#define Explore_HalfTurnAngleMaxRangeRight (0.0625f)
#define Explore_HalfTurnAngleMaxRangeRight4 (0.125f)

#define Explore_WalkAngleFwd			(0.0f)
#define Explore_WalkAngleBackward		(0.5f)
#define Explore_WalkAngleStrafeLeft		(0.75f)
#define Explore_WalkAngleStrafeRight	(0.25f)
#define Explore_WalkRangeFwd			(0.125f)
#define Explore_WalkRangeBwd			(0.125f)
#define Explore_WalkStrafeLeftRange (0.125f)
#define Explore_WalkStrafeRightRange (0.125f)

#define ANGLE4_RIGHT_START		0.125f
#define ANGLE4_BWD_START		0.375f
#define ANGLE4_LEFT_START		0.625f
#define ANGLE4_FWD_START		0.875f

#define ANGLE8_RIGHT45_START	0.0625f
#define ANGLE8_RIGHT90_START	0.1875f
#define ANGLE8_RIGHT135_START	0.3125f
#define ANGLE8_BWD_START		0.4375f
#define ANGLE8_LEFT135_START	0.5625f
#define ANGLE8_LEFT90_START		0.6825f
#define ANGLE8_LEFT45_START		0.8125f
#define ANGLE8_FWD_START		0.9375f


#define WALKANGLE_RIGHT_START 0.24f
#define WALKANGLE_BWD_START 0.38f
#define WALKANGLE_LEFT_START 0.62f
#define WALKANGLE_FWD_START 0.76f

//extern fp32 AngleAdjust(fp32 _AngleA, fp32 _AngleB);
fp32 AngleAdjust(fp32 _AngleA, fp32 _AngleB)
{
	fp32 Diff = (_AngleB - _AngleA);
	if (Diff > 0.5f)
		return -1.0f;
	else if (Diff < -0.5f)
		return 1.0f;
	else
		return 0.0f;
}

int HardcodedIFindAngle8(fp32 _MoveAngleUnit)
{
	if (_MoveAngleUnit > ANGLE8_LEFT135_START)
	{
		if (_MoveAngleUnit > ANGLE8_LEFT45_START)
		{
			if (_MoveAngleUnit > ANGLE8_FWD_START)
				return EXPLORE_ACTIVE_FWD;

			return EXPLORE_ACTIVE_LEFT45;
		}
		else if (_MoveAngleUnit > ANGLE8_LEFT90_START)
		{
			return EXPLORE_ACTIVE_LEFT90;
		}
		return 	EXPLORE_ACTIVE_LEFT135;
	}
	else if (_MoveAngleUnit > ANGLE8_RIGHT45_START)
	{
		if (_MoveAngleUnit > ANGLE8_RIGHT135_START)
		{
			if (_MoveAngleUnit > ANGLE8_BWD_START)
				return _MoveAngleUnit <= 0.5f ? EXPLORE_ACTIVE_RIGHT180 : EXPLORE_ACTIVE_LEFT180;
			
			return EXPLORE_ACTIVE_RIGHT135;
		}
		else if (_MoveAngleUnit > ANGLE8_RIGHT90_START)
		{
			return EXPLORE_ACTIVE_RIGHT90;
		}
		return EXPLORE_ACTIVE_RIGHT45;
	}

	return EXPLORE_ACTIVE_FWD;
}

// Left90/left180/right180/right90 fwd
int HardcodedIFindAngleDiff5(fp32 _MoveAngleUnit)
{
	if ((_MoveAngleUnit >= (Explore_TurnAngleLeft90 - Explore_HalfTurnAngleMaxRangeLeft4)) && 
		(_MoveAngleUnit <= (Explore_TurnAngleLeft90 + Explore_HalfTurnAngleMaxRangeLeft4)))
	{
		//ConOut("LEFT");
		return EXPLORE_ACTIVE_LEFT90;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleLeft180)) && 
		(_MoveAngleUnit <= (Explore_TurnAngleLeft180 + Explore_HalfTurnAngleMaxRangeLeft4)))
	{
		//ConOut("BWD");
		return EXPLORE_ACTIVE_LEFT180;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleRight90 - Explore_HalfTurnAngleMaxRangeRight4)) && 
		(_MoveAngleUnit <= (Explore_TurnAngleRight90 + Explore_HalfTurnAngleMaxRangeRight4)))
	{
		//ConOut("RIGHT");
		return EXPLORE_ACTIVE_RIGHT90;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleRight180 - Explore_HalfTurnAngleMaxRangeRight4)) && 
		(_MoveAngleUnit <= (Explore_TurnAngleRight180)))
	{
		return EXPLORE_ACTIVE_RIGHT180;
	}
	else
	{
		fp32 AngleTempFwd1 = Explore_TurnAngleFwd - Explore_HalfTurnAngleMaxRangeLeft4;
		AngleTempFwd1 = MACRO_ADJUSTEDANGLE(AngleTempFwd1);
		fp32 AngleTempFwd2 = Explore_TurnAngleFwd + Explore_HalfTurnAngleMaxRangeLeft4;
		AngleTempFwd2 = MACRO_ADJUSTEDANGLE(AngleTempFwd2);

		fp32 Adjust = AngleAdjust(_MoveAngleUnit, AngleTempFwd1);
		AngleTempFwd1 += Adjust;
		Adjust = AngleAdjust(_MoveAngleUnit, AngleTempFwd2);
		AngleTempFwd2 += Adjust;

		if ((_MoveAngleUnit >= AngleTempFwd1) &&
			(_MoveAngleUnit <= AngleTempFwd2))
		{
			//ConOut("FWD");
			return EXPLORE_ACTIVE_FWD;	
		}
	}

	//ConOut("UNDEFINED");

	// Shouldn't happen...
	ConOut("HardcodedFindAngle4: FAILED TO FIND ANGLE");
	return EXPLORE_ACTIVE_UNDEFINED;
}

int HardcodedIFindAngle4(fp32 _MoveAngleUnit)
{
	if (_MoveAngleUnit > ANGLE4_LEFT_START)
	{
		if (_MoveAngleUnit > ANGLE4_FWD_START)
			return EXPLORE_ACTIVE_FWD;

		return EXPLORE_ACTIVE_LEFT90;
	}
	else if (_MoveAngleUnit > ANGLE4_RIGHT_START)
	{
		if (_MoveAngleUnit > ANGLE4_BWD_START)
			return EXPLORE_ACTIVE_LEFT180;

		return EXPLORE_ACTIVE_RIGHT90;
	}

	return EXPLORE_ACTIVE_FWD;
}

int HardcodedIFindAngle4Crouch(fp32 _MoveAngleUnit)
{
	if ((_MoveAngleUnit >= (Explore_TurnAngleLeft90Crouch - Explore_HalfTurnAngleMaxRangeLeft4)) && 
		(_MoveAngleUnit <= (Explore_TurnAngleLeft90Crouch + Explore_HalfTurnAngleMaxRangeLeft4)))
	{
		//ConOut("LEFT");
		return EXPLORE_ACTIVE_LEFT90;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleLeft180 - Explore_HalfTurnAngleMaxRangeLeft4)) && 
		(_MoveAngleUnit <= (Explore_TurnAngleLeft180 + Explore_HalfTurnAngleMaxRangeLeft4)))
	{
		//ConOut("BWD");
		return EXPLORE_ACTIVE_LEFT180;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleRight90Crouch - Explore_HalfTurnAngleMaxRangeRight4)) && 
		(_MoveAngleUnit <= (Explore_TurnAngleRight90Crouch + Explore_HalfTurnAngleMaxRangeRight4)))
	{
		//ConOut("RIGHT");
		return EXPLORE_ACTIVE_RIGHT90;
	}
	else
	{
		return EXPLORE_ACTIVE_FWD;	
		/*fp32 AngleTempFwd1 = Explore_TurnAngleFwd - Explore_HalfTurnAngleMaxRangeLeft4;
		AngleTempFwd1 = MACRO_ADJUSTEDANGLE(AngleTempFwd1);
		fp32 AngleTempFwd2 = Explore_TurnAngleFwd + Explore_HalfTurnAngleMaxRangeLeft4;
		AngleTempFwd2 = MACRO_ADJUSTEDANGLE(AngleTempFwd2);

		fp32 Adjust = AngleAdjust(_MoveAngleUnit, AngleTempFwd1);
		AngleTempFwd1 += Adjust;
		Adjust = AngleAdjust(_MoveAngleUnit, AngleTempFwd2);
		AngleTempFwd2 += Adjust;

		if ((_MoveAngleUnit >= AngleTempFwd1) &&
			(_MoveAngleUnit <= AngleTempFwd2))
		{
			//ConOut("FWD");
			
		}*/
	}

	//ConOut("UNDEFINED");

	// Shouldn't happen...
	ConOut("HardcodedFindAngle4: FAILED TO FIND ANGLE");
	return EXPLORE_ACTIVE_UNDEFINED;
}

int HardCodedIFindWalkAngle(fp32 _MoveAngleUnit)
{
	if (_MoveAngleUnit > WALKANGLE_LEFT_START)
	{
		if (_MoveAngleUnit > WALKANGLE_FWD_START)
			return EXPLORE_ACTIVE_FWD;

		return EXPLORE_ACTIVE_LEFT90;
	}
	else if (_MoveAngleUnit > WALKANGLE_RIGHT_START)
	{
		if (_MoveAngleUnit > WALKANGLE_BWD_START)
			return EXPLORE_ACTIVE_LEFT180;

		return EXPLORE_ACTIVE_RIGHT90;
	}

	return EXPLORE_ACTIVE_FWD;
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_FixedMoveAngle8(const CWAG2I_Context* _pContext)
{
	if (GetPropertyFloat(PROPERTY_FLOAT_MOVEVELOCITY) > AG2MOVEANGLETHRESHOLD)
		return CAG2Val::From(HardcodedIFindAngle8(GetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNIT)));

	return CAG2Val::From((int)EXPLORE_ACTIVE_UNDEFINED);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_FixedMoveAngle4(const CWAG2I_Context* _pContext)
{
	if (GetPropertyFloat(PROPERTY_FLOAT_MOVEVELOCITY) > AG2MOVEANGLETHRESHOLD)
		return CAG2Val::From(HardcodedIFindAngle4(GetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNIT)));

	return CAG2Val::From(EXPLORE_ACTIVE_UNDEFINED);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_FixedMoveAngle4Crouch(const CWAG2I_Context* _pContext)
{
	if (GetPropertyFloat(PROPERTY_FLOAT_MOVEVELOCITY) > AG2MOVEANGLETHRESHOLD)
		return CAG2Val::From(HardcodedIFindAngle4Crouch(GetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNIT)));

	return CAG2Val::From(EXPLORE_ACTIVE_UNDEFINED);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_FixedMoveAngleControl8(const CWAG2I_Context* _pContext)
{
	if (GetPropertyFloat(PROPERTY_FLOAT_MOVERADIUSCONTROL) > AG2MOVEANGLETHRESHOLD)
		return CAG2Val::From(HardcodedIFindAngle8(GetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITCONTROL)));

	return CAG2Val::From((int)EXPLORE_ACTIVE_UNDEFINED);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_FixedMoveAngleControl4(const CWAG2I_Context* _pContext)
{
	if (GetPropertyFloat(PROPERTY_FLOAT_MOVERADIUSCONTROL) > AG2MOVEANGLETHRESHOLD)
		return CAG2Val::From(HardcodedIFindAngle4(GetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITCONTROL)));

	return CAG2Val::From((int)EXPLORE_ACTIVE_UNDEFINED);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_FixedWalkAngle4(const CWAG2I_Context* _pContext)
{
	if (GetPropertyFloat(PROPERTY_FLOAT_MOVERADIUSCONTROL) > AG2MOVEANGLETHRESHOLD)
		return CAG2Val::From(HardCodedIFindWalkAngle(GetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITCONTROL)));

	return CAG2Val::From((int)0);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_FixedMoveAngleDiff8(const CWAG2I_Context* _pContext)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	fp32 AngleDiff = -(pCD ? pCD->m_TurnCorrectionTargetAngle : 0.0f);
	AngleDiff = MACRO_ADJUSTEDANGLE(AngleDiff);
	int Value =  HardcodedIFindAngle8(AngleDiff);
	//ConOut(CStrF("Value: %f",Value));
	return CAG2Val::From(Value);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_FixedMoveAngleDiff5(const CWAG2I_Context* _pContext)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	fp32 AngleDiff = -(pCD ? pCD->m_TurnCorrectionTargetAngle : 0.0f);
	AngleDiff = MACRO_ADJUSTEDANGLE(AngleDiff);
	int Value =  HardcodedIFindAngleDiff5(AngleDiff);
	//ConOut(CStrF("Value: %f",Value));
	return CAG2Val::From(Value);
}

#define EXPLORE_MOVEANGLE_EXTRAANGLE 0.04166f
CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_WalkAngleFwd(const CWAG2I_Context* _pContext)
{
	// Angle of movestick
	fp32 Angle = GetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITCONTROL);

	if ((Angle > (WALKANGLE_FWD_START - EXPLORE_MOVEANGLE_EXTRAANGLE)) ||
		(Angle < WALKANGLE_RIGHT_START + EXPLORE_MOVEANGLE_EXTRAANGLE))
	{
		return CAG2Val::From((int)1);
	}

	return CAG2Val::From((int)0);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_WalkAngleRight(const CWAG2I_Context* _pContext)
{
	// Angle of movestick
	fp32 Angle = GetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITCONTROL);

	if ((Angle > (WALKANGLE_RIGHT_START - EXPLORE_MOVEANGLE_EXTRAANGLE)) &&
		(Angle < (WALKANGLE_BWD_START + EXPLORE_MOVEANGLE_EXTRAANGLE)))
	{
		return CAG2Val::From((int)1);
	}

	return CAG2Val::From((int)0);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_WalkAngleBwd(const CWAG2I_Context* _pContext)
{
	// Angle of movestick
	fp32 Angle = GetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITCONTROL);

	if ((Angle > (WALKANGLE_BWD_START - EXPLORE_MOVEANGLE_EXTRAANGLE)) &&
		(Angle < (WALKANGLE_LEFT_START + EXPLORE_MOVEANGLE_EXTRAANGLE)))
	{
		return CAG2Val::From((int)1);
	}

	return CAG2Val::From((int)0);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_WalkAngleLeft(const CWAG2I_Context* _pContext)
{
	// Angle of movestick
	fp32 Angle = GetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITCONTROL);

	if ((Angle > (WALKANGLE_LEFT_START - EXPLORE_MOVEANGLE_EXTRAANGLE)) &&
		(Angle < (WALKANGLE_FWD_START + EXPLORE_MOVEANGLE_EXTRAANGLE)))
	{
		return CAG2Val::From((int)1);
	}

	return CAG2Val::From((int)0);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_CanEndACS(const CWAG2I_Context* _pContext)
{
	if (_pContext->m_pWPhysState->IsClient())
		return CAG2Val::From((int)0);

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)
	{
		int iACS = (int)pCD->m_ControlMode_Param0;

		// Check for if the object really exists
		CWObject_CoreData* pObj = _pContext->m_pWPhysState->Object_GetCD(iACS);
		if (pObj)
		{
			CWObject_Message Msg(OBJMSG_ACTIONCUTSCENE_CANENDACS);
			return CAG2Val::From((int)_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, iACS));
		}

		return CAG2Val::From((int)1);
	}

	return CAG2Val::From((int)0);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_CanActivateItem(const CWAG2I_Context* _pContext)
{
	CWObject_Message Msg(OBJMSG_CHAR_CANACTIVATEITEM);
	return CAG2Val::From((int)_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject));
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_NeedReload(const CWAG2I_Context* _pContext)
{
	if (!_pContext->m_pWPhysState->IsServer())
		return CAG2Val::From((int)0);

	CWObject_Character* pChar = safe_cast<CWObject_Character>(_pContext->m_pObj);
	if (pChar)
	{
		CRPG_Object_Item *pItem = pChar->Char()->GetEquippedItem(0);
		if(pItem && pItem->NeedReload(pChar->m_iObject))
			return CAG2Val::From((int)1);
	}

	return CAG2Val::From((int)0);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_CanReload(const CWAG2I_Context* _pContext)
{
	if (!_pContext->m_pWPhysState->IsServer())
		return CAG2Val::From((int)0);

	CWObject_Character* pChar = safe_cast<CWObject_Character>(_pContext->m_pObj);
	if (pChar)
	{
		CRPG_Object_Item *pItem = pChar->Char()->GetEquippedItem(0);
		if(pItem && pItem->NeedReload(pChar->m_iObject,true))
			return CAG2Val::From((int)1);
	}

	return CAG2Val::From((int)0);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_LadderEndPoint(const CWAG2I_Context* _pContext)
{
	// Find endpoint in ladder
	int ControlMode = CWObject_Character::Char_GetControlMode(_pContext->m_pObj);
	if (ControlMode == PLAYER_CONTROLMODE_LADDER)
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
		int iLadder = (int)pCD->m_ControlMode_Param0;
		int EndPoint = 0;

		// Check which type of endpoint check we want (first one is more expensive)
		EndPoint = _pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_LADDER_GETCURRENTENDPOINT), iLadder);

		//MACRO_PHOBOSDEBUG(ConOut(CStrF("Endpoint: %d", EndPoint));)
		return CAG2Val::From((int)EndPoint);
	}

	return CAG2Val::From((int)LADDER_ENDPOINT_NOENDPOINT);
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_ValveState(const CWAG2I_Context* _pContext)
{
	if (_pContext->m_pWPhysState->IsClient())
		return CAG2Val::From(0);

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)
	{
		int iACS = (int)pCD->m_ControlMode_Param0;

		// Check for if the object really exists
		CWObject_CoreData* pObj = _pContext->m_pWPhysState->Object_GetCD(iACS);
		if (pObj)
		{
			CWObject_Message Msg(OBJMSG_ACTIONCUTSCENE_VALVE_CANTURN);
			return CAG2Val::From(_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, iACS) ? 1 : 0);
		}
	}

	return CAG2Val::From(0);
}

#define WALKSTART_FWD_BOTTOM (0.9f)
#define WALKSTART_FWD_TOP (0.1f)
#define WALKSTART_RIGHT45_BOTTOM (0.1f)
#define WALKSTART_RIGHT45_TOP (0.1875f)
#define WALKSTART_RIGHT90_BOTTOM (0.1875f)
#define WALKSTART_RIGHT90_TOP (0.375f)
#define WALKSTART_BWD_BOTTOM (0.375f)
#define WALKSTART_BWD_TOP (0.625f)
#define WALKSTART_LEFT90_BOTTOM (0.625f)
#define WALKSTART_LEFT90_TOP (0.8125f)
#define WALKSTART_LEFT45_BOTTOM (0.8125f)
#define WALKSTART_LEFT45_TOP (0.9f)

/*CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_WalkStartDirection(const CWAG2I_Context* _pContext)
{
	// If it's not set just return forward (or should we go directly to walk instead..?)
	if (m_MDestination.GetRow(3).k[0] == _FP32_MAX)
		return CAG2Val::From((int)AG2_WALKSTARTDIRECTION_FWD);

	// Check position we want to go to relative our own
	CVec3Dfp32 CharDir = CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
	CVec3Dfp32 Dir = m_MDestination.GetRow(3) - _pContext->m_pObj->GetPosition();
	fp32 Len = Dir.Length();
	if (Len <= 0)
		return CAG2Val::From((int)AG2_WALKSTARTDIRECTION_FWD);

	Dir = Dir / Len;

	CMat4Dfp32 Meh;
	CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),2).SetMatrixRow(Meh,2);
	Dir.SetMatrixRow(Meh,0);
	Meh.RecreateMatrix(2,0);

	// Must take current "up" into consideration (for darklings mostly....:/) hopefully positionmat is correct
	fp32 X = CVec3Dfp32::GetMatrixRow(Meh,0).k[0];//Dir * CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
	fp32 Y = CVec3Dfp32::GetMatrixRow(Meh,0).k[1];//Dir * CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),1);

	fp32 Angle = CVec2Dfp32(X, -Y).GetAngle();
	fp32 CharAngle = CVec2Dfp32(CharDir[0], -CharDir[1]).GetAngle();
	Angle -= CharAngle;
	Angle = MACRO_ADJUSTEDANGLE(Angle);

	
	// Divide into 6 ranges (fwd, left45, left90, right45, right90, bwd)
	if (Angle > 0.5f)
	{
		// fwd/left45/right90/bwd
		if (Angle > 0.75f)
		{
			// fwd/left45/left90
			if (Angle < WALKSTART_LEFT90_TOP)
			{
				// Left90
				return CAG2Val::From((int)AG2_WALKSTARTDIRECTION_LEFT90);
			}
			else
			{
				// fwd/left45
				return CAG2Val::From((int)(Angle > WALKSTART_LEFT45_TOP ? AG2_WALKSTARTDIRECTION_FWD : AG2_WALKSTARTDIRECTION_LEFT45));
			}
		}
		else
		{
			// bwd/left90
			return CAG2Val::From((int)(Angle > WALKSTART_BWD_TOP ? AG2_WALKSTARTDIRECTION_LEFT90 : AG2_WALKSTARTDIRECTION_LEFT180));
		}
	}
	else
	{
		// fwd/right45/right90/bwd
		if (Angle < 0.25f)
		{
			// fwd/right45/right90
			if (Angle > WALKSTART_RIGHT45_TOP)
			{
				return CAG2Val::From((int)AG2_WALKSTARTDIRECTION_RIGHT90);
			}
			else
			{
				// fwd/right45
				return CAG2Val::From((int)(Angle > WALKSTART_RIGHT45_BOTTOM ? AG2_WALKSTARTDIRECTION_RIGHT45 : AG2_WALKSTARTDIRECTION_FWD));
			}
		}
		else
		{
			// right90/bwd
			return CAG2Val::From((int)(Angle > WALKSTART_RIGHT90_TOP ? AG2_WALKSTARTDIRECTION_RIGHT180 : AG2_WALKSTARTDIRECTION_RIGHT90));
		}
	}
}*/

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_JumpStartDirection(const CWAG2I_Context* _pContext)
{
	// Check position we want to go to relative our own
	if (m_JumpDirection.k[2] > 0.75f)
		return CAG2Val::From((int)AG2_JUMPSTARTDIRECTION_UP);

	CVec3Dfp32 Dir = m_JumpDirection;
	Dir.k[2] = 0.0f;
	Dir.Normalize();
	fp32 X = m_JumpDirection.k[0];
	fp32 Y = m_JumpDirection.k[1];

	fp32 Angle = CVec2Dfp32(X, -Y).GetAngle();
	// Should already be in char space?
	//fp32 CharAngle = CVec2Dfp32(CharDir[0], -CharDir[1]).GetAngle();
	//Angle -= CharAngle;
	//Angle = MACRO_ADJUSTEDANGLE(Angle);

	// Divide into 6 ranges (fwd, left45, left90, right45, right90, bwd)
	if (Angle > 0.5f)
	{
		// fwd/left45/right90/bwd
		if (Angle > 0.75f)
		{
			// fwd/left45/left90
			if (Angle < WALKSTART_LEFT90_TOP)
			{
				// Left90
				return CAG2Val::From((int)AG2_JUMPSTARTDIRECTION_LEFT90);
			}
			else
			{
				// fwd/left45
				return CAG2Val::From((int)(Angle > WALKSTART_LEFT45_TOP ? AG2_JUMPSTARTDIRECTION_FWD : AG2_JUMPSTARTDIRECTION_LEFT45));
			}
		}
		else
		{
			// bwd/left90
			return CAG2Val::From((int)(Angle > WALKSTART_BWD_TOP ? AG2_JUMPSTARTDIRECTION_LEFT90 : AG2_JUMPSTARTDIRECTION_LEFT180));
		}
	}
	else
	{
		// fwd/right45/right90/bwd
		if (Angle < 0.25f)
		{
			// fwd/right45/right90
			if (Angle > WALKSTART_RIGHT45_TOP)
			{
				return CAG2Val::From((int)AG2_JUMPSTARTDIRECTION_RIGHT90);
			}
			else
			{
				// fwd/right45
				return CAG2Val::From((int)(Angle > WALKSTART_RIGHT45_BOTTOM ? AG2_JUMPSTARTDIRECTION_RIGHT45 : AG2_JUMPSTARTDIRECTION_FWD));
			}
		}
		else
		{
			// right90/bwd
			return CAG2Val::From((int)(Angle > WALKSTART_RIGHT90_TOP ? AG2_JUMPSTARTDIRECTION_RIGHT180 : AG2_JUMPSTARTDIRECTION_RIGHT90));
		}
	}
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_RandFromParam(const CWAG2I_Context* _pContext)
{
	fp32 NumRands = (fp32)GetPropertyInt(PROPERTY_INT_AGPARAM);
	return CAG2Val::From(RoundToInt(Random * NumRands));
}

CAG2Val CWO_Clientdata_Character_AnimGraph2::Property_TurnInAngleDiff(const CWAG2I_Context* _pContext)
{
	// Find diff from current lookangle to destination and get turnanglediff thingie from it
	CMat4Dfp32 PosMat = _pContext->m_pObj->GetLocalPositionMatrix();
	CVec3Dfp32 Look = CWObject_Character::GetLook(PosMat);
	CVec3Dfp32 Target = CWObject_Character::GetLook(m_MDestination);
	fp32 AngleDiff = Target.k[2] - Look.k[2];
	AngleDiff = MACRO_ADJUSTEDANGLE(AngleDiff);
	int Value =  HardcodedIFindAngle8(AngleDiff);
	//ConOut(CStrF("Value: %f",Value));
	return CAG2Val::From(Value);
}


// EFFECTS ----------------------------

void CWO_Clientdata_Character_AnimGraph2::Effect_SwitchWeapon(const CWAG2I_Context* _pContext, 
															  const CXRAG2_ICallbackParams* _pParams)
{
	if (_pContext->m_pWPhysState->IsClient())
		return;
	// Deactivate weapon
	// ConOut(CStrF("Deactivating item GameTime: %f", _pContext->m_pWPhysState->GetGameTime()));
	CWObject_Message Msg(OBJMSG_CHAR_DEACTIVATEITEM,AG2_ITEMSLOT_WEAPONS,1);
	_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);
	int32 NumExtraTicks = (int32)(_pParams->GetParam(0) * (_pContext->m_pWPhysState->GetGameTicksPerSecond() / 20.0f));

	// Bleh, I kinda hate it that the AG calls this more than once
	// Send impulse
	int16 Type = _pParams->GetParam(1);
	// "Force equip" param
	int32 Force = _pParams->GetParam(3);
	if (Type == 0)
	{
		int Identifier = GetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER);
		// "Reload" primary
		if (Identifier != 0 && _pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBGMSG_CHAR_SWITCHITEMBYIDENTIFIER, 
			AG2_ITEMSLOT_WEAPONS, Identifier, NumExtraTicks,0,0,0,0,Force),_pContext->m_pObj->m_iObject))
		{
			//ConOut("DOING NEXT ITEM!!!");
			// Send impulse that we should switch weapon
			//SetWeaponIdentifier();
			// Send equip impulse (incase we're in the same weapon type...)
			/*if (_pParams->GetParam(1))
				m_pAG2I->SendImpulse(_pContext,CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_EQUIP),0);*/
			UpdateImpulseState(_pContext);		
			//m_pAG2I->SendImpulse(_pContext,CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETEQUIPPEDANIMTYPE,true), _pContext->m_pObj->m_iObject)),AG2_TOKEN_MAIN);
			SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,false);
			SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,0);
		}
	}
	else if (Type == 1)
	{
		int Identifier = GetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIERDUALWIELD);
		// Reload secondary
		if (Identifier != 0 && _pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBGMSG_CHAR_SWITCHITEMBYIDENTIFIER, 
			AG2_ITEMSLOT_DUALWIELD, Identifier, NumExtraTicks,0,0,0,0,Force),_pContext->m_pObj->m_iObject))
		{
			//ConOut("DOING NEXT ITEM!!!");
			// Send impulse that we should switch weapon
			//SetWeaponIdentifier();
			// Send equip impulse (incase we're in the same weapon type...)
			/*if (_pParams->GetParam(1))
			m_pAG2I->SendImpulse(_pContext,CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_EQUIP),0);*/
			UpdateImpulseState(_pContext);		
			//m_pAG2I->SendImpulse(_pContext,CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETEQUIPPEDANIMTYPE,true), _pContext->m_pObj->m_iObject)),AG2_TOKEN_MAIN);
			SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUEDDUALWIELD,false);
			SetPropertyInt(PROPERTY_BOOL_WEAPONSELECTQUEUEDDUALWIELD,0);
		}
	}
	else if (Type == 3)
	{
		int Identifier = GetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER);
		// Unequip both weapons
		// "Reload" primary
		if (Identifier != 0 && _pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBGMSG_CHAR_SWITCHITEMBYIDENTIFIER, 
			AG2_ITEMSLOT_WEAPONS, GetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER),NumExtraTicks,_pParams->GetParam(2),0,0,0,Force),_pContext->m_pObj->m_iObject))
		{
			//ConOut("DOING NEXT ITEM!!!");
			// Send impulse that we should switch weapon
			//SetWeaponIdentifier();
			// Send equip impulse (incase we're in the same weapon type...)
			/*if (_pParams->GetParam(1))
			m_pAG2I->SendImpulse(_pContext,CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_EQUIP),0);*/
			UpdateImpulseState(_pContext);		
			//m_pAG2I->SendImpulse(_pContext,CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETEQUIPPEDANIMTYPE,true), _pContext->m_pObj->m_iObject)),AG2_TOKEN_MAIN);
			SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,false);
			SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUEDDUALWIELD,false);
			SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,0);
			SetPropertyInt(PROPERTY_BOOL_WEAPONSELECTQUEUEDDUALWIELD,0);
		}
	}
}

void CWO_Clientdata_Character_AnimGraph2::Effect_ActivateItem(const CWAG2I_Context* _pContext, 
															  const CXRAG2_ICallbackParams* _pParams)
{
	if ((_pParams == NULL) || (_pParams->GetNumParams() == 0) || 
		!_pContext->m_pWPhysState->IsServer())
		return;

	int ActivationMode = AG2_ITEMACTIVATION_NORMAL;
	if (_pParams->GetNumParams() > 1)
		ActivationMode = _pParams->GetParam(1);

	if (ActivationMode == AG2_ITEMACTIVATION_FAIL)
	{
		//Don't activate if swtching weapon
		int Input = (_pParams->GetNumParams() > 2 ? _pParams->GetParam(2) : 0);
		// ConOut(CStrF("Activating item GameTime: %f", _pContext->m_pWPhysState->GetGameTime()));
		CWObject_Message Msg(OBJMSG_CHAR_ACTIVATEITEM, _pParams->GetParam(0), Input,-1,true);
		_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);
	}
	else if (ActivationMode == AG2_ITEMACTIVATION_RELOAD)
	{
		CWObject_Message Msg(OBJMSG_CHAR_RELOADITEM, _pParams->GetParam(0));
		_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);
	}
	else
	{
		int Input = (_pParams->GetNumParams() > 2 ? _pParams->GetParam(2) : 0);
		// ConOut(CStrF("Activating item GameTime: %f", _pContext->m_pWPhysState->GetGameTime()));
		CWObject_Message Msg(OBJMSG_CHAR_ACTIVATEITEM, _pParams->GetParam(0), Input);
		_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);
	}
}

void CWO_Clientdata_Character_AnimGraph2::Effect_ActionCutsceneSwitch(const CWAG2I_Context* _pContext, 
																	 const CXRAG2_ICallbackParams* _pParams)
{
	// Only on server (for now...)
	if ((_pParams == NULL) || (_pParams->GetNumParams() == 0))
		return;

	// Do actioncutscene stuff, type is given in param0
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);

	if (!pCD)
		return;

	int iACS, ACSActionType;
	iACS = (int)pCD->m_ControlMode_Param0;

	// Get type of action to perform from the animgraph
	ACSActionType = _pParams->GetParam(0);

	if (_pContext->m_pWPhysState->IsClient())
	{
		if (ACSActionType == AG2_ACSACTIONTYPE_ENDANIMATION)
		{
			//pCD->m_CameraUtil.Clear();
			//pCD->m_CameraUtil.MakeDirty();
			CActionCutsceneCamera::SetInvalidToPcd(pCD);
			pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_THIRDPERSON;
		}
		return;
	}

	switch (ACSActionType)
	{
	case AG2_ACSACTIONTYPE_STARTANIMATION:
		{
			//ConOut("ACS: START OF ANIM");
			// At the start of an animation, set thirdperson, set startposition and disable physics
			int ACSMode = pCD->m_ControlMode_Param4;

			if (ACSMode & ACTIONCUTSCENE_OPERATION_USETHIRDPERSON)
			{
				// Send a message that finds a new cutscenecamera
				/*CActionCutsceneCamera* pCamera = NULL;
				if (_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETNEWCUTSCENECAMERA, 0, 0, 
					_pContext->m_pObj->m_iObject,0,0,0,&pCamera), iACS) && pCamera != NULL)
				{
					// Activate the thirdperson camera

					// Use the given cutscene camera somehow (override first person cam)
					// (don't forget to enable it, and disable in when returning to first person)
					if (pCamera)
					{
						pCamera->SetActive();
						pCamera->OnRefresh();
						pCamera->SetValid(pCD);
					}

					// Set thirdperson flag so the head will be shown
					// Ugly hack for now (we know it's on the server atleast)
					CWObject* pObj = (CWObject*)_pContext->m_pObj;
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_THIRDPERSON;
					pObj->m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
				}*/
				CCameraUtil* pCamUtil = &(pCD->m_CameraUtil);
				if (_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETNEWCUTSCENECAMERA, 0, 0, 
					_pContext->m_pObj->m_iObject,0,0,0,pCamUtil), iACS))
				{
					pCD->m_CameraUtil.MakeDirty();
					// Set thirdperson flag so the head will be shown
					// Ugly hack for now (we know it's on the server atleast)
					CWObject* pObj = (CWObject*)_pContext->m_pObj;
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_THIRDPERSON;
					pObj->m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
					//pCD->m_ActionCutSceneCameraMode = CActionCutsceneCamera::ACS_CAMERAMODE_ACTIVE;
				}

				//pCD->m_CameraUtil.SetACSCamera(CAMERAUTIL_MODE_ACS|CAMERAUTIL_MODE_ORIGIN_FIXEDPOSITION,iACS,pCamera->GetObject(),pCamera->GetCharacter());
				//pCD->m_CameraUtil.MakeDirty();
			}

			if (ACSMode & ACTIONCUTSCENE_OPERATION_DISABLEPHYSICS)
			{
				// Add these flags the the client flags
				int DisablePhysicsFlags = PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK | 
					PLAYER_CLIENTFLAGS_NOGRAVITY;

				_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_CHAR_SETCLIENTFLAGS, DisablePhysicsFlags, 0, 
					_pContext->m_pObj->m_iObject), _pContext->m_pObj->m_iObject);
			}

			if (ACSMode & ACTIONCUTSCENE_OPERATION_SETSTARTPOSMASK)
			{
				CMat4Dfp32 PosMat;
				if (CWObject_ActionCutscene::GetStartPosition(PosMat, pCD, _pContext->m_pWPhysState))
				{
					CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(PosMat,3);
					// Make sure we don't collide with floor
					Pos.k[2] += 0.001f;
					Pos.SetMatrixRow(PosMat, 3);
					_pContext->m_pWPhysState->Object_SetPosition(_pContext->m_pObj->m_iObject, 
						PosMat);
					_pContext->m_pWPhysState->Object_SetVelocity(_pContext->m_pObj->m_iObject,
						CVec3Dfp32(0.0f));
				}
			}

			break;
		}
	case AG2_ACSACTIONTYPE_ENDANIMATION:
		{
			// At the end of an animation, set firstperson, set endposition enable physics, and 
			// set crouch
			//ConOut("ACS: END OF ANIM");

			// At the start of an animation, set thirdperson, set startposition and disable physics
			int ACSMode = pCD->m_ControlMode_Param4;
			// go back to "focus" camera if any
			CCameraUtil* pCamUtil = &(pCD->m_CameraUtil);
			if (ACSMode & ACTIONCUTSCENE_OPERATION_USETHIRDPERSON)
				pCamUtil->Clear();
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_ACTIONCUTSCENE_REUSEFOCUSCUTSCENECAMERA, 0, 0, 
				_pContext->m_pObj->m_iObject,0,0,0,pCamUtil), iACS);
			pCD->m_CameraUtil.MakeDirty();

			if (true)//(ACSMode & ACTIONCUTSCENE_OPERATION_USETHIRDPERSON)
			{
				// Remove thirdperson flag so the head won't be shown
				// Don't forget to disable cutscene camera....!!!
				//pCD->m_CameraUtil.Clear();
				//pCD->m_CameraUtil.MakeDirty();
				CActionCutsceneCamera::SetInvalidToPcd(pCD);
				pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_THIRDPERSON;
				pCD->m_ActionCutSceneCameraMode = 0;
				// Ugly hack for now (we now it's on the server atleast)
				/*CWObject* pObj = (CWObject*)_pContext->m_pObj;
				pObj->m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;*/
			}

			if (ACSMode & ACTIONCUTSCENE_OPERATION_DISABLEPHYSICS)
			{
				// Remove these flags the the client flags
				int DisablePhysicsFlags = PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK | 
					PLAYER_CLIENTFLAGS_NOGRAVITY | PLAYER_CLIENTFLAGS_FORCECROUCH;
				_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_CHAR_SETCLIENTFLAGS, 0, DisablePhysicsFlags, 
					_pContext->m_pObj->m_iObject), _pContext->m_pObj->m_iObject);
			}

			if (ACSMode & ACTIONCUTSCENE_OPERATION_CROUCHATEND)
			{
				// Crouch the character
				// Make the target character crouch somehow.... server might not be needed (check it)
				CWObject_Character::Char_SetPhysics(_pContext->m_pObj, _pContext->m_pWPhysState, 
					NULL, PLAYER_PHYS_CROUCH, false, false);
			}

			if (ACSMode & ACTIONCUTSCENE_OPERATION_SETENDPOSMASK)
			{
				// Set the position of the character from object defined the actioncutscene
				int iEndPosition = _pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETENDPOSITION), iACS);

				if (iEndPosition > 0)
				{
					CMat4Dfp32 PosMat;
					if (!_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETCURRENTMATRIX,aint(&PosMat)),iEndPosition))
						PosMat = _pContext->m_pWPhysState->Object_GetPositionMatrix(iEndPosition);
					CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(PosMat,3);
					// Make sure we don't collide with floor
					Pos.k[2] += 0.001f;
					Pos.SetMatrixRow(PosMat, 3);
					_pContext->m_pWPhysState->Object_SetPosition(_pContext->m_pObj->m_iObject, 
						PosMat);
					_pContext->m_pWPhysState->Object_SetVelocity(_pContext->m_pObj->m_iObject,
						CVec3Dfp32(0.0f));
				}
			}
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_ACTIONCUTSCENE_ONENDACS,0,0,_pContext->m_pObj->m_iObject), iACS);

			CWObject_Character::Char_SetControlMode(_pContext->m_pObj,PLAYER_CONTROLMODE_FREE);
			//m_pAG2I->SendImpulse(_pContext, CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED), 0);
			break;
		}
		// SEPARATE IMPLEMENTATIONS, to be used by other than actioncutscene
	case AG2_ACSACTIONTYPE_DISABLEPHYSICS:
		{
			//ConOut("DISABLE PHYS");
			// Add these flags the the client flags
			int DisablePhysicsFlags = PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK | 
				PLAYER_CLIENTFLAGS_NOGRAVITY;

			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_CHAR_SETCLIENTFLAGS, DisablePhysicsFlags, 0, 
				_pContext->m_pObj->m_iObject), _pContext->m_pObj->m_iObject);
			if (m_StateFlagsLo & AG2_STATEFLAG_NOPHYSFLAG)
			{
				// Set physics
				CWObject_Character::Char_SetPhysics(_pContext->m_pObj, _pContext->m_pWPhysState, 
					NULL, CWObject_Character::Char_GetPhysType(_pContext->m_pObj), false, true);
			}
			break;
		}
	case AG2_ACSACTIONTYPE_ENABLEPHYSICS:
		{
			//ConOut("ENABLE PHYS");
			// Add these flags the the client flags
			int DisablePhysicsFlags = PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK | 
				PLAYER_CLIENTFLAGS_NOGRAVITY | PLAYER_CLIENTFLAGS_FORCECROUCH;
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_CHAR_SETCLIENTFLAGS, 0, DisablePhysicsFlags, 
				_pContext->m_pObj->m_iObject), _pContext->m_pObj->m_iObject);
			break;
		}
	case AG2_ACSACTIONTYPE_SETFIRSTPERSON:
		{
			// Remove thirdperson flag so the head won't be shown
			// Don't forget to disable cutscene camera....!!!
			//pCD->m_CameraUtil.Clear();
			//pCD->m_CameraUtil.MakeDirty();
			CActionCutsceneCamera::SetInvalidToPcd(pCD);
			pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_THIRDPERSON;
			// Send OnEndAcs message (this should only be used by ladders/ledges/acs anyway)	
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_ACTIONCUTSCENE_ONENDACS,0,0,_pContext->m_pObj->m_iObject), iACS);
			break;
		}
	case AG2_ACSACTIONTYPE_CROUCH:
		{
			// Crouch the character
			// Make the target character crouch somehow.... server might not be needed (check it)
			CWObject_Character::Char_SetPhysics(_pContext->m_pObj, _pContext->m_pWPhysState, 
				NULL, PLAYER_PHYS_CROUCH, false, false);
			break;
		}
	case AG2_ACSACTIONTYPE_SETTHIRDPERSON:
		{
			// Set thirdperson flag so the head will be shown
			pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_THIRDPERSON;
			break;
		}
	case AG2_ACSACTIONTYPE_PICKUPITEM:
		{
			// Pickup the item
			_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_RPG_AVAILABLEPICKUP, 
				_pContext->m_pObj->m_iObject, 0, iACS), iACS);

			/*int Type = (int)pCD->m_ControlMode_Param2;
			if ((Type == ACTIONCUTSCENE_TYPE_PICKUPITEMGROUND) || 
			(Type == ACTIONCUTSCENE_TYPE_PICKUPITEMTABLE))
			{
			// Pickup the item
			_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_RPG_AVAILABLEPICKUP, 
			_pContext->m_pObj->m_iObject, 0, iACS), iACS);
			}*/
			break;
		}
		// Activates the triggers (temporary for success for now)
	case AG2_ACSACTIONTYPE_DOTRIGGER:
		{
			//ConOut(CStrF("Time: %f Doing trigger!!",_pContext->m_GameTime));
			// Send the trigger messages in the actioncutscene
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_ACTIONCUTSCENE_DOTRIGGER,0,0,_pContext->m_pObj->m_iObject), iACS);
			break;
		}
	case AG2_ACSACTIONTYPE_DOTRIGGERMIDDLE:
		{
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_ACTIONCUTSCENE_DOTRIGGERMIDDLE,0,0,_pContext->m_pObj->m_iObject), iACS);
			break;
		}
	case AG2_ACSACTIONTYPE_DOTRIGGERREFILL:
		{
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_ACTIONCUTSCENE_DOTRIGGERREFILL,0,0,_pContext->m_pObj->m_iObject), iACS);
			break;
		}
	case AG2_ACSACTIONTYPE_SETLEVERSTATE:
		{
			if (_pParams->GetNumParams() > 1)
			{
				int32 State = _pParams->GetParam(1);
				_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_LEVER_SETSTATE,State,0,_pContext->m_pObj->m_iObject), iACS);
			}
			break;
		}
	case AG2_ACSACTIONTYPE_ONCHANGEVALVESTATE:
		{
			if (_pParams->GetNumParams() > 1)
			{
				int32 State = _pParams->GetParam(1);
				_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_VALVE_ONCHANGESTATE,State,0,_pContext->m_pObj->m_iObject), iACS);
			}
			break;
		}
	case AG2_ACSACTIONTYPE_USEACSCAMERA:
		{
			//ConOut("ACS: START OF ANIM");
			// At the start of an animation, set thirdperson, set startposition and disable physics
			int ACSMode = pCD->m_ControlMode_Param4;

			if (ACSMode & ACTIONCUTSCENE_OPERATION_USETHIRDPERSON)
			{
				// Send a message that finds a new cutscenecamera
				/*CActionCutsceneCamera* pCamera = NULL;
				if (_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETNEWCUTSCENECAMERA, 0, 0, 
					_pContext->m_pObj->m_iObject,0,0,0,&pCamera), iACS) && pCamera != NULL)
				{
					// Activate the thirdperson camera

					// Use the given cutscene camera somehow (override first person cam)
					// (don't forget to enable it, and disable in when returning to first person)
					if (pCamera)
					{
						pCamera->SetActive();
						pCamera->OnRefresh();
						pCamera->SetValid(pCD);
					}

					// Set thirdperson flag so the head will be shown
					// Ugly hack for now (we now it's on the server atleast)
					CWObject* pObj = (CWObject*)_pContext->m_pObj;
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_THIRDPERSON;
					pObj->m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
				}*/
				CCameraUtil* pCamUtil = &(pCD->m_CameraUtil);
				if (_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETNEWCUTSCENECAMERA, 0, 0, 
					_pContext->m_pObj->m_iObject,0,0,0,pCamUtil), iACS))
				{
					pCD->m_CameraUtil.MakeDirty();
					// Set thirdperson flag so the head will be shown
					// Ugly hack for now (we know it's on the server atleast)
					CWObject* pObj = (CWObject*)_pContext->m_pObj;
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_THIRDPERSON;
					pObj->m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
				}
			}
			break;
		}
	case AG2_ACSACTIONTYPE_ENABLEPHYSFLAGS:
		{
			pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~(PLAYER_PHYSFLAGS_NOGRAVITY | PLAYER_PHYSFLAGS_NOCHARACTERCOLL | PLAYER_PHYSFLAGS_NOWORLDCOLL);
			CWObject_Character::Char_SetPhysics(_pContext->m_pObj, _pContext->m_pWPhysState, 
				NULL, CWObject_Character::Char_GetPhysType(_pContext->m_pObj), false, true);
			break;
		}
	default:
		{
			break;
		}
	}
}

void CWO_Clientdata_Character_AnimGraph2::Effect_LadderMove(const CWAG2I_Context* _pContext, 
															const CXRAG2_ICallbackParams* _pParams)
{
	if (!_pParams || _pParams->GetNumParams() <= 0)
		return;

	int32 Move = _pParams->GetParam(0);
	CWObject_Phys_Ladder::MakeLadderMove(_pContext->m_pWPhysState, _pContext->m_pObj,Move);
}


void CWO_Clientdata_Character_AnimGraph2::Effect_RelAnimMoveSignal(const CWAG2I_Context* _pContext, 
																  const CXRAG2_ICallbackParams* _pParams)
{
	if (!_pParams || _pParams->GetNumParams() <= 0)
		return;

	// Ok, signal start/stop to relanimmove (only server?)
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)
	{
		/*CWAG2I_Context AGContext(_pContext->m_pObj, _pContext->m_pWPhysState,_pContext->m_GameTime,_pContext->m_TimeSpan);
		if (pCD->m_RelAnimPos.Signal(_pParams->GetParam(0),&AGContext, pCD->m_AnimGraph2.GetAG2I(), (_pParams->GetParam(1) != 0.0f)))
			pCD->m_RelAnimPos.MakeDirty();*/
		if (_pParams->GetParam(0) == RELATIVEANIMPOS_SIGNAL_STOP)
		{
			pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~PLAYER_PHYSFLAGS_NOCHARACTERCOLL;
			// Must set the physics as well
			CWObject_Character::Char_SetPhysics(_pContext->m_pObj, _pContext->m_pWPhysState, NULL, 
				CWObject_Character::Char_GetPhysType(_pContext->m_pObj), false, true);
		}
	}
}

void CWO_Clientdata_Character_AnimGraph2::Effect_SetControlMode(const CWAG2I_Context* _pContext,
															   const CXRAG2_ICallbackParams* _pParams)
{
	// Set new controlmode
	if ((_pParams == NULL) || (_pParams->GetNumParams() == 0))
		return;

	CWObject_Character::Char_SetControlMode(_pContext->m_pObj, _pParams->GetParam(0));
}

void CWO_Clientdata_Character_AnimGraph2::Effect_AIBehaviorSignal(const CWAG2I_Context* _pContext, 
																 const CXRAG2_ICallbackParams* _pParams)
{
	if (_pParams == NULL || _pParams->GetNumParams() == 0)
		return;

	// Ok signal the AI somehow....
	int32 Signal = (int32)_pParams->GetParam(0);

	switch (Signal)
	{
	case AG2_BEHAVIOR_SIGNAL_ENDOFBEHAVIOR:
		{
			// Signal the AI associated with m_iObject that the behaviour animation has finished
			// Param0 is the behaviour and iSender is (naturally) the object id
			int32 Behavior = (int32)_pParams->GetParam(1);
			CWObject_Message Msg(OBJMSG_AIEFFECT_ENDOFBEHAVIOR,Behavior,_pParams->GetParam(2),_pContext->m_pObj->m_iObject);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg,_pContext->m_pObj->m_iObject);

			break;
		}
	case AG2_BEHAVIOR_SIGNAL_STARTOFBEHAVIOR:
		{
			//Signal gamesystem that the main animation of the current behaviour has started
			// Param0 is the behaviour and iSender is (naturally) the object id
			int32 Behavior = (int32)_pParams->GetParam(1);
			CWObject_Message Msg(OBJMSG_AIEFFECT_STARTOFBEHAVIOR,Behavior,0,_pContext->m_pObj->m_iObject);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg,_pContext->m_pObj->m_iObject);
			break;
		}
	default:
		break;
	};
}

void CWO_Clientdata_Character_AnimGraph2::Effect_SetRagdoll(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams)
{
	// Remove the ragdoll
	_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETRAGDOLL,_pParams->GetParam(0)),_pContext->m_pObj->m_iObject);
}

void CWO_Clientdata_Character_AnimGraph2::Effect_CreateGrabbableItem(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams)
{
	if (_pContext->m_pWPhysState->IsServer())
		_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_CREATEGRABBABLEITEMOBJECT,0,1), _pContext->m_pObj->m_iObject);
}

void CWO_Clientdata_Character_AnimGraph2::Effect_SetDestination(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams)
{
	if (!_pParams->GetParam(0))
	{
		ClearDestinationLock();
		CMat4Dfp32 Mat;
		CVec3Dfp32(_FP32_MAX).SetRow(Mat,3);
		SetDestination(Mat);
	}
	else
	{
		// Special case...
		if (_pParams->GetParam(0) == 2)
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
			CWO_CharDarkling_ClientData* pDarklingCD = pCD ? pCD->IsDarkling() : NULL;
			if (pDarklingCD)
			{
				pDarklingCD->m_UpVector = CVec3Dfp32(0.0f,0.0f,1.0f);
				CMat4Dfp32 Pos = _pContext->m_pObj->GetPositionMatrix();
				pDarklingCD->m_UpVector.m_Value.SetMatrixRow(Pos,2);
				Pos.RecreateMatrix(2,0);
				_pContext->m_pWPhysState->Object_SetPosition(_pContext->m_pObj->m_iObject,Pos);
			}
		}
		CMat4Dfp32 Mat = _pContext->m_pObj->GetPositionMatrix();
		m_UpVector.SetRow(Mat,2);
		Mat.RecreateMatrix(2,0);
		SetDestinationLock(Mat);
	}
}

void CWO_Clientdata_Character_AnimGraph2::Effect_ResetLook(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams)
{
	// Reset play look so he keeps hands level in "creepmode" anim
	CMat4Dfp32 Pos = _pContext->m_pWPhysState->Object_GetPositionMatrix(_pContext->m_pObj->m_iObject);
	CVec3Dfp32(0.0,0.0,1.0f).SetRow(Pos,2);
	Pos.RecreateMatrix(2,0);
	_pContext->m_pWPhysState->Object_SetPosition(_pContext->m_pObj->m_iObject, Pos);
}

void CWO_Clientdata_Character_AnimGraph2::Effect_PlayDialog(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams)
{
	// Play dialog
	_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_RPG_SPEAK_OLD,_pParams->GetParam(0)),_pContext->m_pObj->m_iObject);
}

void CWO_Clientdata_Character_AnimGraph2::Effect_SendImpulse(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams)
{
	// Send impulse to target char
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	CWObject_CoreData* pTarget = _pContext->m_pWPhysState->Object_GetCD(pCD->m_iFightingCharacter);
	CWO_Character_ClientData* pTargetCD = pTarget ? CWObject_Character::GetClientData(pTarget) : NULL;
	if (pTargetCD)
	{
		CXRAG2_Impulse Impulse(_pParams->GetParam(0),_pParams->GetParam(1));
		CWAG2I_Context AGContext(pTarget,_pContext->m_pWPhysState,_pContext->m_GameTime);
		pTargetCD->m_AnimGraph2.GetAG2I()->SendImpulse(&AGContext,Impulse,0);
	}
}

void CWO_Clientdata_Character_AnimGraph2::Effect_ToggleCrouch(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams)
{
	int32 GameTick = _pContext->m_pWPhysState->GetGameTick();
	if ((GameTick - m_LastToggleCrouch) > 4 && _pContext->m_pWPhysState->IsServer())
	{
		m_LastToggleCrouch = GameTick;
		ConExecute("togglecrouch()");
	}
}

void CWO_Clientdata_Character_AnimGraph2::Effect_FindRagDollAgonyType(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams)
{
	CWObject_Character* pChar = CWObject_Character::IsCharacter(_pContext->m_pObj);
#ifdef INCLUDE_OLD_RAGDOLL
	if (pChar && pChar->m_spRagdoll)
		SetPropertyInt(PROPERTY_INT_RAGDOLLAGONYTYPE, pChar->m_spRagdoll->GetOrientation());
	else
#endif // INCLUDE_OLD_RAGDOLL
		SetPropertyInt(PROPERTY_INT_RAGDOLLAGONYTYPE, 0);
}

void CWO_Clientdata_Character_AnimGraph2::Effect_SetParam(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams)
{
	SetPropertyInt(PROPERTY_INT_AGPARAM, _pParams->GetParam(0));
}

void CWO_Clientdata_Character_AnimGraph2::Effect_LedgeMode(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams)
{
	CWObject_Ledge::MakeLedgeMove(_pContext,_pParams->GetParam(0));
	// Check if we should move again
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	CWObject_Ledge::OnPress(_pContext,pCD,pCD->m_Control_Press,pCD->m_Control_LastPress);
}

void CWO_Clientdata_Character_AnimGraph2::Effect_ResendBehavior(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams)
{
	int32 WantedBehavior = GetPropertyInt(PROPERTY_INT_WANTEDBEHAVIOR);
	CAG2ImpulseType Type = WantedBehavior == AG2_IMPULSEVALUE_BEHAVIOR_EXIT || WantedBehavior == AG2_IMPULSEVALUE_BEHAVIOR_EXITMH ? AG2_IMPULSETYPE_BEHAVIORCONTROL : AG2_IMPULSETYPE_BEHAVIOR;
	m_pAG2I->SendImpulse(_pContext,CXRAG2_Impulse(Type,WantedBehavior),0);
}

// END EFFECTS ------------------------

//--------------------------------------------------------------------------------


PFN_ANIMGRAPH2_CONDITION CWO_Clientdata_Character_AnimGraph2::ms_lpfnConditions_Server[MAX_ANIMGRAPH2_DEFCONDITIONS] =
{
	/* 00 */ (PFN_ANIMGRAPH2_CONDITION)&CWO_Clientdata_Character_AnimGraph2::Condition_StateTime,
	/* 01 */ (PFN_ANIMGRAPH2_CONDITION)NULL,//&CWO_ClientData_AnimGraph2Interface::Condition_AnimExitPoint,
	/* 02 */ (PFN_ANIMGRAPH2_CONDITION)&CWO_Clientdata_Character_AnimGraph2::Condition_AnimLoopCount,
	/* 03 */ (PFN_ANIMGRAPH2_CONDITION)&CWO_Clientdata_Character_AnimGraph2::Condition_AnimLoopCountOffset,
	/* 04 */ (PFN_ANIMGRAPH2_CONDITION)NULL,
	/* 05 */ (PFN_ANIMGRAPH2_CONDITION)NULL,
	/* 06 */ (PFN_ANIMGRAPH2_CONDITION)NULL,
	/* 07 */ (PFN_ANIMGRAPH2_CONDITION)&CWO_Clientdata_Character_AnimGraph2::Condition_StateTimeOffset,
	/* 08 */ (PFN_ANIMGRAPH2_CONDITION)NULL,
	/* 09 */ (PFN_ANIMGRAPH2_CONDITION)NULL,
};

PFN_ANIMGRAPH2_PROPERTY CWO_Clientdata_Character_AnimGraph2::ms_lpfnProperties_Server[MAX_ANIMGRAPH2_DEFPROPERTIES] =
{
	/* 00 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_FixedMoveAngle8,
	/* 01 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_FixedMoveAngle4,
	/* 02 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_FixedMoveAngleControl8,
	/* 03 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_FixedMoveAngleControl4,
	/* 04 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_CanReload,
	/* 05 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_FixedMoveAngleDiff8,
	/* 06 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_FixedWalkAngle4,
	/* 07 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_CanEndACS,
	/* 08 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_NeedReload,
	/* 09 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_ValveState,
	/* 10 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_JumpStartDirection,
	/* 11 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_FixedMoveAngle4Crouch,
	/* 12 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_WalkAngleFwd,
	/* 13 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_WalkAngleRight,
	/* 14 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_WalkAngleBwd,
	/* 15 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_WalkAngleLeft,
	/* 16 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_CanActivateItem,
	/* 17 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_RandFromParam,
	/* 18 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_Clientdata_Character_AnimGraph2::Property_TurnInAngleDiff,
	/* 19 */ NULL,
};


//--------------------------------------------------------------------------------

PFN_ANIMGRAPH2_OPERATOR CWO_Clientdata_Character_AnimGraph2::ms_lpfnOperators_Server[MAX_ANIMGRAPH2_DEFOPERATORS] =
{
	/* 00 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_Clientdata_Character_AnimGraph2::OperatorEQ,
	/* 01 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_Clientdata_Character_AnimGraph2::OperatorGT,
	/* 02 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_Clientdata_Character_AnimGraph2::OperatorLT,
	/* 03 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_Clientdata_Character_AnimGraph2::OperatorGE,
	/* 04 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_Clientdata_Character_AnimGraph2::OperatorLE,
	/* 05 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_Clientdata_Character_AnimGraph2::OperatorNE,
	/* 06 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_Clientdata_Character_AnimGraph2::OperatorMOD,
	/* 07 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_Clientdata_Character_AnimGraph2::OperatorAND,
	/* 08 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_Clientdata_Character_AnimGraph2::OperatorNOTAND,
};

//--------------------------------------------------------------------------------

PFN_ANIMGRAPH2_EFFECT CWO_Clientdata_Character_AnimGraph2::ms_lpfnEffects_Server[MAX_ANIMGRAPH2_DEFEFFECTS] =
{
	/* 00 */ NULL,
	/* 01 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_ActionCutsceneSwitch,
	/* 02 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_ActivateItem,
	/* 03 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_CreateGrabbableItem,
	/* 04 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_SwitchWeapon,
	/* 05 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_RelAnimMoveSignal,
	/* 06 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_LadderMove,
	/* 07 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_SetControlMode,
	/* 08 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_AIBehaviorSignal,
	/* 09 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_SetRagdoll,

	/* 10 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_SetDestination,
	/* 11 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_ResetLook,
	/* 12 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_PlayDialog,
	/* 13 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_SendImpulse,
	/* 14 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_ToggleCrouch,
	/* 15 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_FindRagDollAgonyType,
	/* 16 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_SetParam,
	/* 17 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_LedgeMode,
	/* 18 */ (PFN_ANIMGRAPH2_EFFECT)&CWO_Clientdata_Character_AnimGraph2::Effect_ResendBehavior,
	/* 19 */ NULL,

	/* 20 */ NULL,
	/* 21 */ NULL,
	/* 22 */ NULL,
	/* 23 */ NULL,
	/* 24 */ &CWO_Clientdata_Character_AnimGraph2::Effect_RegisterHandledResponse,
};


#define NUMINTTOSAVE 29
static int16 s_lIntToSave[NUMINTTOSAVE] =
{
	PROPERTY_INT_AGPARAM,
	PROPERTY_INT_WEAPONCLASS,
	PROPERTY_INT_LEDGETYPE,
	PROPERTY_INT_FACIAL_GROUP,
	PROPERTY_INT_FACIAL_TYPE,
	PROPERTY_INT_ANIMGRIPTYPERIGHT,
	PROPERTY_INT_ANIMGRIPTYPELEFT,
	PROPERTY_INT_ACTIONCUTSCENETYPE,
	PROPERTY_INT_WANTEDBEHAVIOR,
	PROPERTY_INT_DIALOGGESTURE,
	PROPERTY_INT_DIALOGBLENDTYPE,
	PROPERTY_INT_DIALOGMOOD,
	PROPERTY_INT_DIALOGFACIAL,
	PROPERTY_INT_DIALOGFACIALGESTURE1,
	PROPERTY_INT_DIALOGFACIALGESTURE2,
	PROPERTY_INT_ITEMANIMTYPE,
	PROPERTY_INT_WEAPONIDENTIFIER,
	PROPERTY_INT_BEHAVIOR_ENTEREXITTYPE,
	PROPERTY_INT_BEHAVIOR_GENERAL,
	PROPERTY_INT_BEHAVIOR_LOOPMODE,
	PROPERTY_INT_STANCE,
	PROPERTY_INT_WALKSTOPTYPE,
	PROPERTY_INT_EQUIPPEDITEMCLASS,
	PROPERTY_INT_STANCESTANDING,
	PROPERTY_INT_MOVEVARIATION,
	PROPERTY_INT_EQUIPPEDITEMTYPE,
	PROPERTY_INT_WEAPONIDENTIFIERDUALWIELD,
	PROPERTY_INT_WALKSTARTTYPE,
	PROPERTY_INT_RAGDOLLAGONYTYPE,
};

void CWO_Clientdata_Character_AnimGraph2::Write(CCFile* _pFile)
{
	// Save stuff
	TAP_RCD<int> lPropertiesInt = m_lPropertiesInt;
	for (int32 i = 0; i < NUMINTTOSAVE; i++)
	{
		int32 Val = lPropertiesInt[s_lIntToSave[i]];
		_pFile->WriteLE(Val);
	}
}

void CWO_Clientdata_Character_AnimGraph2::Read(CCFile* _pFile)
{
	// Load stuff
	TAP_RCD<int> lPropertiesInt = m_lPropertiesInt;
	for (int32 i = 0; i < NUMINTTOSAVE; i++)
	{
		int32 Val;
		_pFile->ReadLE(Val);
		lPropertiesInt[s_lIntToSave[i]] = Val;
	}
}

