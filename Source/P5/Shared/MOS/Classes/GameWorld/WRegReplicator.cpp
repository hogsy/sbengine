#include "PCH.h"
#include "WClass.h"

/*
// -------------------------------------------------------------------
//	Registry replication protocol v2.0
// -------------------------------------------------------------------

NOTES:

* 16-bit indices/sizes are not implemented



[Registry]
{
	uint8 UpdateFlags
		0 : Name
		1 : Value
		2 : ChildCount
		3 : Children
		4..6 : Value type (0..7, 3 types unused, use one for 16-bit Integers?)
		7 : 16-bit indices/sizes.

  	[Name]
		uint8			StartChar
		uint8			Count
		uint8			Length
		char[Count]		Text

	[Value]
		case Integer
			uint32			Value

		case Float
			fp32				Value

		case String
			uint8			StartChar		16-bit?
			uint8			Count			16-bit?
			uint8			Length			16-bit?
			char[Count]		Text
			
		case WString
			uint8			StartChar		16-bit?
			uint8			Count			16-bit?
			uint8			Length			16-bit?
			wchar[Count]	Text
			
		case Data
			if (16-bit)						Too much work to look-ahead to be able to select 8/16 bit?
				uint16			StartPos
				uint16			Count
				uint16			Size
			else
				uint8			StartPos
				uint8			Count
				uint8			Size
			uint8[Len]		Data

  	[ChildCount]
		if (16-bit)
			uint16			nChildren
		else
			uint8			nChildren

	[Children]
		if (16-bit)
			uint16			nChangedChildren
		else
			uint8			nChangedChildren

		nChanged *
		{
			if (16-bit)
				uint16			ChildIndex
			else
				uint8			ChildIndex

			[Registry]		Updated registry
		}

}

// -------------------------------------------------------------------
*/

enum
{
	REGUPD_FLAGS_KEY =				1,
	REGUPD_FLAGS_VALUE =			2,
	REGUPD_FLAGS_CHILDCOUNT =		4,
	REGUPD_FLAGS_CHILDREN =			8,

	REGUPD_FLAGS2_WIDECHAR =		1,

	REGUPD_FLAGS_TYPESHIFT =		4,
};

static int GetTypeMask(const CRegistry* _pReg)
{
	MAUTOSTRIP(GetTypeMask, 0);
	return (_pReg->GetType() - REGISTRY_TYPE_STR) << REGUPD_FLAGS_TYPESHIFT;
}

static int GetMaskType(int _Flags)
{
	MAUTOSTRIP(GetMaskType, 0);
	return ((_Flags >> REGUPD_FLAGS_TYPESHIFT) & 7) + REGISTRY_TYPE_STR;
}

static bool WriteDataDiff(const uint8* _pStr, const uint8* _pOldStr, int _Len, int _OldLen, uint8*& _pData, bool _bForceWrite)
{
	MAUTOSTRIP(WriteDataDiff, false);
	if (!_OldLen)
	{
		if (!_Len && !_bForceWrite) return false;
		PTR_PUTINT8(_pData, 0);
		PTR_PUTINT8(_pData, _Len);
		PTR_PUTINT8(_pData, _Len);
		memcpy(_pData, _pStr, _Len);
		_pData += _Len;
		return true;
	}
	else if (!_Len)
	{
		if (!_OldLen && !_bForceWrite) return false;
		PTR_PUTINT8(_pData, 0);
		PTR_PUTINT8(_pData, 0);
		PTR_PUTINT8(_pData, 0);
		return true;
	}
	else
	{
		if (_Len == _OldLen)
		{
			int iStart = 0;
			for(; iStart < _Len; iStart++)
				if (_pStr[iStart] != _pOldStr[iStart]) break;

			if (iStart == _Len && !_bForceWrite) return false;

			int iEnd = _Len-1;
			for(; iEnd > iStart; iEnd--)
				if (_pStr[iEnd] != _pOldStr[iEnd]) break;

			iEnd++;

			PTR_PUTINT8(_pData, iStart);
			PTR_PUTINT8(_pData, iEnd - iStart);
			PTR_PUTINT8(_pData, _Len);
			memcpy(_pData, &_pStr[iStart], iEnd - iStart);
			_pData += iEnd - iStart;
			return true;
		}
		else
		{
			int LenMin = Min(_Len, _OldLen);

			int iStart = 0;
			for(; iStart < LenMin; iStart++)
				if (_pStr[iStart] != _pOldStr[iStart]) break;

			int iEnd = _Len-1;
			iEnd++;

			PTR_PUTINT8(_pData, iStart);
			PTR_PUTINT8(_pData, iEnd - iStart);
			PTR_PUTINT8(_pData, _Len);
			memcpy(_pData, &_pStr[iStart], iEnd - iStart);
			_pData += iEnd - iStart;
			return true;
		}

	}
}

template<class T>
static bool WriteStringDiff(const T* _pStr, const T* _pOldStr, uint8*& _pData, bool _bForceWrite)
{
	MAUTOSTRIP(WriteStringDiff, false);
	return WriteDataDiff((const uint8*)_pStr, (const uint8*)_pOldStr, CStr::StrLen(_pStr)*sizeof(T), CStr::StrLen(_pOldStr)*sizeof(T), _pData, _bForceWrite);
}

static bool WriteArrayDiff(const TArray<uint8>* _plData, const TArray<uint8>* _plOldData, uint8*& _pStream, bool _bForceWrite)
{
	MAUTOSTRIP(WriteArrayDiff, false);
	int Len = (_plData) ? _plData->Len() : 0;
	int OldLen = (_plOldData) ? _plOldData->Len() : 0;
	const uint8* pData = (_plData) ? _plData->GetBasePtr() : NULL;
	const uint8* pOldData = (_plData) ? _plOldData->GetBasePtr() : NULL;
	return WriteDataDiff(pData, pOldData, Len, OldLen, _pStream, _bForceWrite);
}

static void ReadStringDiff(CRegistry* _pReg, const uint8*& _pData, bool _bName)
{
	MAUTOSTRIP(ReadStringDiff, MAUTOSTRIP_VOID);
	int iStart, nCopy, Len;
	PTR_GETINT8(_pData, iStart);
	PTR_GETINT8(_pData, nCopy);
	PTR_GETINT8(_pData, Len);

	char Buffer[4096];
	const char* pStr = (_bName) ? _pReg->GetThisName() : _pReg->GetThisValue();
	int CurLen = CStr::StrLen(pStr);

	if (CurLen)
		memcpy(Buffer, pStr, CurLen);
	else
		Buffer[0] = 0;

	if(Len > CurLen)
		FillChar(&Buffer[CurLen], Len - CurLen, 0);

	memcpy(&Buffer[iStart], _pData, nCopy);
	_pData += nCopy;

	Buffer[Len] = 0;

	if (_bName)
		_pReg->SetThisName(Buffer);
	else
		_pReg->SetThisValue(Buffer);
}

static void ReadStringDiffW(CRegistry* _pReg, const uint8*& _pData)
{
	MAUTOSTRIP(ReadStringDiffW, MAUTOSTRIP_VOID);
	int iStart, nCopy, Len;
	PTR_GETINT8(_pData, iStart);
	PTR_GETINT8(_pData, nCopy);
	PTR_GETINT8(_pData, Len);

	char Buffer[4096];
	const wchar* pStr = _pReg->GetThisValue().StrW();
	int CurLen = CStr::StrLen(pStr)*2;

	if (CurLen)
		memcpy(Buffer, pStr, CurLen);
	else
		Buffer[0] = 0;

	if(Len > CurLen)
		FillChar(&Buffer[CurLen], Len - CurLen, 0);

	memcpy(&Buffer[iStart], _pData, nCopy);
	_pData += nCopy;

	Buffer[Len] = 0;	// We REALLY want a termination just in case something breaks down, so we need to fill a minumum of 3 bytes to be sure to fill a word with zero.
	Buffer[Len+1] = 0;
	Buffer[Len+2] = 0;

	_pReg->SetThisValue((wchar*)Buffer);
}

//#define PACKDEBUG

#ifdef PACKDEBUG
#define PackLog(s) LogFile(s)
#else
#define PackLog(s)
#endif

int PackDeltaRegistry_r(const CRegistry* _pReg, const CRegistry* _pOldReg, uint8* _pData, int _RecommendLen, int _MaxLen, int _RecurseDepth);
int PackDeltaRegistry_r(const CRegistry* _pReg, const CRegistry* _pOldReg, uint8* _pData, int _RecommendLen, int _MaxLen, int _RecurseDepth)
{
	MAUTOSTRIP(PackDeltaRegistry_r, 0);
	// ToDo: Check for _MaxLen overrun.
#ifdef PACKDEBUG
	CFStr Indent = CFStrF(' ', _RecurseDepth*4);
/*	PackLog(CStrF("(Pack) %sServer: %s = %s, nChildren %d", (char*) Indent, (const char*)_pReg->GetThisName(), (const char*)_pReg->GetThisValue(), _pReg->GetNumChildren() ));
	if (_pOldReg)
		PackLog(CStrF("(Pack) %sMirror: %s = %s, nChildren %d", (char*) Indent, (const char*)_pOldReg->GetThisName(), (const char*)_pOldReg->GetThisValue(), _pOldReg->GetNumChildren() ));
	else
		PackLog(CStrF("(Pack) %sMirror: N/A", (char*) Indent) );
*/
#endif

	if (_MaxLen <= 256*2+20)
		return 0;


	uint8* pOrgData = _pData;
	uint8& UpdateFlags = *_pData;
	_pData++;
	uint8& UpdateFlags2 = *_pData;
	_pData++;

	UpdateFlags = 0;
	UpdateFlags2 = 0;
	if(!_pOldReg)
	{
		// -------------------------------------------------------------------
		// No mirror registry (_pOldReg) present, write everything.
		UpdateFlags |= GetTypeMask(_pReg);

		// Write key
		if (WriteStringDiff((char*)_pReg->GetThisName(), (char*)NULL, _pData, false))
		{
			UpdateFlags |= REGUPD_FLAGS_KEY;
			PackLog(CStrF("(Pack) %s- Adding key name", (char*) Indent) );
		}

		// Write value
		switch(_pReg->GetType())
		{
		case REGISTRY_TYPE_VOID:
			break;

		case REGISTRY_TYPE_STR :
			{
				CStr Value = _pReg->GetThisValue();
				if (Value.IsAnsi())
				{
					if (WriteStringDiff((char*)_pReg->GetThisValue(), (char*)NULL, _pData, false))
					{
						UpdateFlags |= REGUPD_FLAGS_VALUE;
						PackLog(CStrF("(Pack) %s- Adding string value", (char*) Indent) );
					}
				}
				else
				{
					if (WriteStringDiff((wchar*)_pReg->GetThisValue(), (wchar*)NULL, _pData, false))
					{
						UpdateFlags |= REGUPD_FLAGS_VALUE;
						UpdateFlags2 |= REGUPD_FLAGS2_WIDECHAR;
						PackLog(CStrF("(Pack) %s- Adding wstring value", (char*) Indent) );
					}
				}
				break;
			}

		case REGISTRY_TYPE_UINT8 :
		case REGISTRY_TYPE_INT16 :
		case REGISTRY_TYPE_INT32 :
		case REGISTRY_TYPE_UINT32 :
			{
				PTR_PUTINT32(_pData, _pReg->GetThisValuei());
				UpdateFlags |= REGUPD_FLAGS_VALUE;
				PackLog(CStrF("(Pack) %s- Adding int value", (char*) Indent) );
				break;
			}

		case REGISTRY_TYPE_FP32 :
			{
				fp32 v = _pReg->GetThisValuef();
				PTR_PUTFP32(_pData, v);
				UpdateFlags |= REGUPD_FLAGS_VALUE;
				PackLog(CStrF("(Pack) %s- Adding float value", (char*) Indent) );
				break;
			}

		case REGISTRY_TYPE_DATA :
			{
				TArray<uint8> TempArray = _pReg->GetThisValued();
				if (WriteArrayDiff(&TempArray, NULL, _pData, false))
				{
					UpdateFlags |= REGUPD_FLAGS_VALUE;
					PackLog(CStrF("(Pack) %s- Adding data value", (char*) Indent) );
				}
				break;
			}

		default :
			ConOutL(CStrF("(::PackDeltaRegistry_r) Unsupported registry data-type: %d", _pReg->GetType()));
		}

		// Write children
		if (_pReg->GetNumChildren())
		{
			PTR_PUTINT8(_pData, _pReg->GetNumChildren());
			UpdateFlags |= REGUPD_FLAGS_CHILDCOUNT;
			PackLog(CStrF("(Pack) %s- Adding child count", (char*) Indent) );

			uint8* pChildCount = _pData;
			PTR_PUTINT8(_pData, 0);
			for(int i = 0; i < _pReg->GetNumChildren(); i++)
			{
				PTR_PUTINT8(_pData, i);
				int Size = PackDeltaRegistry_r(_pReg->GetChild(i), NULL, _pData, _RecommendLen, _MaxLen - (_pData - pOrgData), _RecurseDepth+1);
				if (Size)
				{
					_pData += Size;
					(*pChildCount)++;

					PackLog(CStrF("(Pack) %s- Added child %d, %d bytes", (char*) Indent, i, Size ) );
				}
			}

			UpdateFlags |= REGUPD_FLAGS_CHILDREN;
		}
	}
	else
	{
		// -------------------------------------------------------------------
		// Mirror registry (_pOldReg) present
		if (WriteStringDiff((char*)_pReg->GetThisName(), (char*)_pOldReg->GetThisName(), _pData, false))
		{
			UpdateFlags |= REGUPD_FLAGS_KEY;
			PackLog(CStrF("(Pack) %s- Key value changed", (char*) Indent) );
		}

		// Write value
		if (_pReg->GetType() != _pOldReg->GetType())
		{
			PackLog(CStrF("(Pack) %s- Data-type changed", (char*) Indent) );
			// Data type has changed, there is no need to compare to pOldReg.
			switch(_pReg->GetType())
			{
			case REGISTRY_TYPE_VOID:
				break;

			case REGISTRY_TYPE_STR :
				{
					CStr Value = _pReg->GetThisValue();
					if (Value.IsAnsi())
					{
						if (!WriteStringDiff((char*)_pReg->GetThisValue(), (char*)NULL, _pData, true))
							Error_static("PackDeltaUpdate_r", "Internal error.");
						UpdateFlags |= REGUPD_FLAGS_VALUE;
						PackLog(CStrF("(Pack) %s- Adding string value", (char*) Indent) );
					}
					else
					{
						if (!WriteStringDiff((wchar*)_pReg->GetThisValue(), (wchar*)NULL, _pData, true))
							Error_static("PackDeltaUpdate_r", "Internal error.");
						UpdateFlags |= REGUPD_FLAGS_VALUE;
						UpdateFlags2 |= REGUPD_FLAGS2_WIDECHAR;
						PackLog(CStrF("(Pack) %s- Adding wstring value", (char*) Indent) );
					}
					break;
				}
			case REGISTRY_TYPE_UINT8 :
			case REGISTRY_TYPE_INT16 :
			case REGISTRY_TYPE_INT32 :
			case REGISTRY_TYPE_UINT32 :
				{
					int32 v = _pReg->GetThisValuei();
					PTR_PUTINT32(_pData, v);
					UpdateFlags |= REGUPD_FLAGS_VALUE;
					PackLog(CStrF("(Pack) %s- Adding int value", (char*) Indent) );
					break;
				}

			case REGISTRY_TYPE_FP32 :
				{
					fp32 v = _pReg->GetThisValuef();
					PTR_PUTFP32(_pData, v);
					UpdateFlags |= REGUPD_FLAGS_VALUE;
					PackLog(CStrF("(Pack) %s- Adding float value", (char*) Indent) );
					break;
				}

			case REGISTRY_TYPE_DATA :
				{
					TArray<uint8> TempArray = _pReg->GetThisValued();
					if (!WriteArrayDiff(&TempArray, NULL, _pData, true))
						Error_static("PackDeltaUpdate_r", "Internal error.");
					UpdateFlags |= REGUPD_FLAGS_VALUE;
					PackLog(CStrF("(Pack) %s- Adding data value", (char*) Indent) );
					break;
				}

			default :
				ConOutL(CStrF("(::PackDeltaRegistry_r) Unsupported registry data-type: %d", _pReg->GetType()));
			}
		}
		else
		{
			// Compare value to _pOldReg, update if necessary.
			switch(_pReg->GetType())
			{
			case REGISTRY_TYPE_VOID:
				break;

			case REGISTRY_TYPE_STR :
				{
					CStr Value = _pReg->GetThisValue();
					if (Value.IsAnsi())
					{
						if (WriteStringDiff((char*)_pReg->GetThisValue(), (char*)_pOldReg->GetThisValue(), _pData, false))
						{
							UpdateFlags |= REGUPD_FLAGS_VALUE;
							PackLog(CStrF("(Pack) %s- String value changed", (char*) Indent) );
						}
					}
					else
					{
						if (WriteStringDiff((wchar*)_pReg->GetThisValue(), (wchar*)_pOldReg->GetThisValue(), _pData, false))
						{
							UpdateFlags |= REGUPD_FLAGS_VALUE;
							UpdateFlags2 |= REGUPD_FLAGS2_WIDECHAR;
							PackLog(CStrF("(Pack) %s- WString value changed", (char*) Indent) );
						}
					}
					break;
				}

			case REGISTRY_TYPE_UINT8 :
			case REGISTRY_TYPE_INT16 :
			case REGISTRY_TYPE_INT32 :
			case REGISTRY_TYPE_UINT32 :
				{
					if (_pReg->GetThisValuei() != _pOldReg->GetThisValuei())
					{
						PTR_PUTINT32(_pData, _pReg->GetThisValuei());
						UpdateFlags |= REGUPD_FLAGS_VALUE;
						PackLog(CStrF("(Pack) %s- Int value changed", (char*) Indent) );
					}
					break;
				}

			case REGISTRY_TYPE_FP32 :
				{
					if (_pReg->GetThisValuef() != _pOldReg->GetThisValuef())
					{
						fp32 v = _pReg->GetThisValuef();
						PTR_PUTFP32(_pData, v);
						UpdateFlags |= REGUPD_FLAGS_VALUE;
						PackLog(CStrF("(Pack) %s- Float value changed", (char*) Indent) );
					}
					break;
				}

			case REGISTRY_TYPE_DATA :
				{
					TArray<uint8> TempArray = _pReg->GetThisValued();
					TArray<uint8> TempArrayOld = _pReg->GetThisValued();
					if (WriteArrayDiff(&TempArray, &TempArrayOld, _pData, false))
					{
						UpdateFlags |= REGUPD_FLAGS_VALUE;
						PackLog(CStrF("(Pack) %s- Data value changed", (char*) Indent) );
					}
					break;
				}

			default :
				ConOutL(CStrF("(::PackDeltaRegistry_r) Unsupported registry data-type: %d", _pReg->GetType()));
			}
		}

		// Add data-type to flags only if value is updated.
		if (UpdateFlags & REGUPD_FLAGS_VALUE)
			UpdateFlags |= GetTypeMask(_pReg);

		// Check if child-count has changed.
		if (_pReg->GetNumChildren() != _pOldReg->GetNumChildren())
		{
			PTR_PUTINT8(_pData, _pReg->GetNumChildren());
			UpdateFlags |= REGUPD_FLAGS_CHILDCOUNT;
			PackLog(CStrF("(Pack) %s- Child count changed, %d != %d", (char*) Indent, _pReg->GetNumChildren(), _pOldReg->GetNumChildren() ) );
		}

		if (_pReg->GetNumChildren())
		{
			uint8& NumChildren = *_pData;
			_pData++;
			NumChildren = 0;
			for(int i = 0; i < _pReg->GetNumChildren(); i++)
			{
				const CRegistry* pOldChild = (_pOldReg->GetNumChildren() > i) ? _pOldReg->GetChild(i) : NULL;

				PTR_PUTINT8(_pData, i);	// Write index
				int Size = PackDeltaRegistry_r(_pReg->GetChild(i), pOldChild, _pData, _RecommendLen, _MaxLen - (_pData - pOrgData), _RecurseDepth+1);
				if (Size)
				{
					_pData += Size;
					NumChildren++;
					PackLog(CStrF("(Pack) %s- Child %d changed, %d bytes", (char*) Indent, i, Size ) );
				}
				else
					_pData--;	// Reverse, remove index
			}

			if(NumChildren)
				UpdateFlags |= REGUPD_FLAGS_CHILDREN;
			else
				_pData--;	// Reverse, remove child-count
		}
	}

	if (!UpdateFlags)
		_pData -= 2;	// Reverse, remove updateflags since nothing at all needed update.

	return (_pData - pOrgData);
}

#define UNPACK_PARANOIA

int UnpackDeltaRegistry_r(CRegistry* _pReg, const uint8* _pData, int _Len, int _RecurseDepth);
int UnpackDeltaRegistry_r(CRegistry* _pReg, const uint8* _pData, int _Len, int _RecurseDepth)
{
	MAUTOSTRIP(UnpackDeltaRegistry_r, 0);
#ifdef PACKDEBUG
	CFStr Indent = CFStrF(' ', _RecurseDepth*4);
	PackLog(CStrF("(Unpack) %s%s = %s, nChildren %d", (char*) Indent, (const char*)_pReg->GetThisName(), (const char*)_pReg->GetThisValue(), _pReg->GetNumChildren() ));

#endif
	const uint8* pOrgData = _pData;
	const uint8& UpdateFlags = *_pData;
	_pData++;
	const uint8& UpdateFlags2 = *_pData;
	_pData++;

	_pReg->SetThisUserFlags(1);


	// Read key name
	if (UpdateFlags & REGUPD_FLAGS_KEY)
	{
		ReadStringDiff(_pReg, _pData, true);
		PackLog(CStrF("(Unpack) %s- Key name changed: %s", (char*) Indent, (const char*)_pReg->GetThisName()) );
	}

	// Read value
	if (UpdateFlags & REGUPD_FLAGS_VALUE)
	{
		int Type = GetMaskType(UpdateFlags);
		switch(Type)
		{
		case REGISTRY_TYPE_VOID:
			break;

		case REGISTRY_TYPE_STR :
			{
				if (UpdateFlags2 & REGUPD_FLAGS2_WIDECHAR)
				{
					ReadStringDiffW(_pReg, _pData);
					PackLog(CStrF("(Unpack) %s- WString value changed: %s", (char*) Indent, _pReg->GetThisValue().Ansi().Str() ) );
				}
				else
				{
					ReadStringDiff(_pReg, _pData, false);
					PackLog(CStrF("(Unpack) %s- String value changed: %s", (char*) Indent, (const char*)_pReg->GetThisValue()) );
				}
				break;
			}

		case REGISTRY_TYPE_UINT8 :
		case REGISTRY_TYPE_INT16 :
		case REGISTRY_TYPE_INT32 :
		case REGISTRY_TYPE_UINT32 :
			{
				int32 v;
				PTR_GETINT32(_pData, v);
				_pReg->SetThisValuei(v, Type);
				PackLog(CStrF("(Unpack) %s- Integer value changed: %s", (char*) Indent, (const char*)_pReg->GetThisValue()) );
				break;
			}

		case REGISTRY_TYPE_FP32 :
			{
				fp32 v;
				PTR_GETFP32(_pData, v);
				_pReg->SetThisValuef(v, Type);
				PackLog(CStrF("(Unpack) %s- Float value changed: %s", (char*) Indent, (const char*)_pReg->GetThisValue()) );
				break;
			}

/*		case REGISTRY_TYPE_DATA :
			{
				break;
			}*/

		default :
			Error_static("::UnpackDeltaRegistry_r", CStrF("Unsupported registry data-type: %d", Type ));
		}
	}

	// Read child count
	if (UpdateFlags & REGUPD_FLAGS_CHILDCOUNT)
	{
		int nChildren;
		PTR_GETINT8(_pData, nChildren);
		_pReg->SetNumChildren(nChildren);
		PackLog(CStrF("(Unpack) %s- Child count changed: %d", (char*) Indent, _pReg->GetNumChildren()) );
	}

	// Read child updates
	if (UpdateFlags & REGUPD_FLAGS_CHILDREN)
	{
		int nChildren;
		PTR_GETINT8(_pData, nChildren);
		PackLog(CStrF("(Unpack) %s- %d Children changed", (char*) Indent, nChildren) );

		for(int i = 0; i < nChildren; i++)
		{
			int iChild;
			PTR_GETINT8(_pData, iChild);
			if (iChild >= 0 && iChild < _pReg->GetNumChildren())
			{
				CRegistry* pChild = _pReg->GetChild(iChild);
				int Size = UnpackDeltaRegistry_r(pChild, _pData, _Len - (_pData - pOrgData), _RecurseDepth+1);
				_pData += Size;
				PackLog(CStrF("(Unpack) %s- Child %d changed, %d bytes.", (char*) Indent, iChild, Size) );
			}
			else
			{
#ifdef UNPACK_PARANOIA
				Error_static("::UnpackDeltaRegistry_r", CStrF("Invalid child-index: %d/%d", iChild, _pReg->GetNumChildren()) );
#else
				_pReg->SetNumChildren(iChild+1);
				int Size = UnpackDeltaRegistry_r(_pReg->GetChild(iChild), _pData, _Len - (_pData - pOrgData), _RecurseDepth+1);
				_pData += Size;
#endif
			}
		}
	}

	return _pData - pOrgData;
}




