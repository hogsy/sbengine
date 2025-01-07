#include "PCH.h"
#include "../../MSystem.h"
#include "MCC.h"

int CFileRead(CCFile* _pFile, void* _pBuf, int _BufSize)
{
	M_TRY
	{
		int Size = MinMT(_BufSize, _pFile->Length()-_pFile->Pos());
		_pFile->Read(_pBuf, Size);
		return Size;
	}
	M_CATCH(
	catch(CCException)
	{
		return 0;
	}
	)
}

int CFileWrite(CCFile* _pFile, void* _pBuf, int _BufSize)
{
	M_TRY
	{
		_pFile->Write(_pBuf, _BufSize);
		return _BufSize;
	}
	M_CATCH(
	catch(CCException)
	{
		return 0;
	}
	)
}

