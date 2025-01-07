//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MDA.h"
#include "MFile.h"
#include "AnimGraph2_Reaction.h"

//--------------------------------------------------------------------------------
//- Action_Full ------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG2_Reaction::Clear()
{
	m_Impulse.Clear();
	m_iBaseEffectInstance = GetDefaultBaseEffectInstanceIndex();
	m_iBaseMoveToken = GetDefaultMoveTokenIndex();
	m_iBaseMoveAnimgraph = GetDefaultMoveAnimgraphIndex();
	m_nMoveTokens = 0;
	m_nMoveAnimgraph = 0;
	m_nEffectInstances = 0;
}

//--------------------------------------------------------------------------------

void CXRAG2_Reaction::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case XR_ANIMGRAPH2_VERSION3:
		case XR_ANIMGRAPH2_VERSION:
		{
			m_Impulse.Read(_pFile);

			_pFile->ReadLE(m_iBaseEffectInstance);
			_pFile->ReadLE(m_iBaseMoveToken);
			_pFile->ReadLE(m_iBaseMoveAnimgraph);
			_pFile->ReadLE(m_nEffectInstances);
			_pFile->ReadLE(m_nMoveTokens);
			_pFile->ReadLE(m_nMoveAnimgraph);
		}
		break;

		default:
			Error_static("CXRAG2_Reaction::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

//--------------------------------------------------------------------------------

void CXRAG2_Reaction::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	m_Impulse.Write(_pFile);

	_pFile->WriteLE(m_iBaseEffectInstance);
	_pFile->WriteLE(m_iBaseMoveToken);
	_pFile->WriteLE(m_iBaseMoveAnimgraph);
	_pFile->WriteLE(m_nEffectInstances);
	_pFile->WriteLE(m_nMoveTokens);
	_pFile->WriteLE(m_nMoveAnimgraph);
	
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------

#ifndef CPU_LITTLEENDIAN
void CXRAG2_Reaction::SwapLE()
{
	::SwapLE(m_iBaseEffectInstance);
	::SwapLE(m_iBaseMoveToken);
	::SwapLE(m_iBaseMoveAnimgraph);
}
#endif

//--------------------------------------------------------------------------------
