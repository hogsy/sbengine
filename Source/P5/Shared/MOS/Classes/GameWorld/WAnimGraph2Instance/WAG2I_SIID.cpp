#include "PCH.h"
#include "WAG2I_SIID.h"



void CWAG2I_SIID::Write(CCFile* _pFile) const
{
	m_EnqueueTime.Write(_pFile);
	_pFile->WriteLE(m_iEnterMoveToken);
	_pFile->WriteLE(m_iAnimGraph);
}

void CWAG2I_SIID::Read(CCFile* _pFile)
{
	m_EnqueueTime.Read(_pFile);
	_pFile->ReadLE(m_iEnterMoveToken);
	_pFile->ReadLE(m_iAnimGraph);
}
