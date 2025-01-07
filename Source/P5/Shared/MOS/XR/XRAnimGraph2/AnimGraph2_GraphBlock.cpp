#include "PCH.h"
#include "MRTC.h"
#include "MMemMgrHeap.h"
#include "MDA.h"
#include "AnimGraph2_GraphBlock.h"
#include "AnimGraph2.h"

CXRAG2_GraphBlock::CXRAG2_GraphBlock()
{
	Clear();
}

//--------------------------------------------------------------------------------

void CXRAG2_GraphBlock::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
	case XR_ANIMGRAPH2_VERSION3:
	case XR_ANIMGRAPH2_VERSION:
		{
			m_Condition.Read(_pFile);
			_pFile->ReadLE(m_iStartMoveToken);
			_pFile->ReadLE(m_iStateFullStart);
			_pFile->ReadLE(m_StateFullLen);
			_pFile->ReadLE(m_iSwitchStateStart);
			_pFile->ReadLE(m_SwitchStateLen);
			_pFile->ReadLE(m_iReactionFullStart);
			_pFile->ReadLE(m_ReactionFullLen);
			_pFile->ReadLE(m_iStateConstantStart);
			_pFile->ReadLE(m_StateConstantLen);
			break;
		}
	default:
		Error_static("CXRAG2_GraphBlock::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

//--------------------------------------------------------------------------------

void CXRAG2_GraphBlock::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	m_Condition.Write(_pFile);
	_pFile->WriteLE(m_iStartMoveToken);
	_pFile->WriteLE(m_iStateFullStart);
	_pFile->WriteLE(m_StateFullLen);
	_pFile->WriteLE(m_iSwitchStateStart);
	_pFile->WriteLE(m_SwitchStateLen);
	_pFile->WriteLE(m_iReactionFullStart);
	_pFile->WriteLE(m_ReactionFullLen);
	_pFile->WriteLE(m_iStateConstantStart);
	_pFile->WriteLE(m_StateConstantLen);
#endif	// PLATFORM_CONSOLE
}


//--------------------------------------------------------------------------------

CXRAG2_Impulse::CXRAG2_Impulse(CAG2ImpulseType _Type, CAG2ImpulseValue _Value)
{
	m_ImpulseType = _Type;
	m_ImpulseValue = _Value;
}

CXRAG2_Impulse::CXRAG2_Impulse()
{
	Clear();
}

void CXRAG2_Impulse::Clear()
{
	m_ImpulseType = XRAG2_IMPULSETYPE_UNDEFINED;
	m_ImpulseValue = XRAG2_IMPULSEVALUE_UNDEFINED;
}

bool CXRAG2_Impulse::Compare(const CXRAG2_Impulse& _Condition)
{
	return (m_ImpulseType == _Condition.m_ImpulseType) && (m_ImpulseValue == _Condition.m_ImpulseValue);
}

// IO
void CXRAG2_Impulse::Read(CCFile* _pDFile)
{
	_pDFile->ReadLE(m_ImpulseType);
	_pDFile->ReadLE(m_ImpulseValue);

}
void CXRAG2_Impulse::Write(CCFile* _pDFile)
{
#ifndef	PLATFORM_CONSOLE
	_pDFile->WriteLE(m_ImpulseType);
	_pDFile->WriteLE(m_ImpulseValue);
#endif	// PLATFORM_CONSOLE
}
