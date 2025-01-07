#include "PCH.h"
#include "MRenderUtil.h"
#include "../../XR/XRVertexBuffer.h"
#include "../../XR/XRVBManager.h"
#include "MFloat.h"
#include "../../MSystem/Misc/MLocalizer.h"
#include "../../MSystem/Raster/MTextureContainerxtc2.h"

#define CRC_MAXSTRFORMAT	512


// -------------------------------------------------------------------
//  CRC_ConsoleViewport
// -------------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(CRC_ConsoleViewport);


CRC_ConsoleViewport::CRC_ConsoleViewport()
{
	MAUTOSTRIP(CRC_ConsoleViewport_ctor, MAUTOSTRIP_VOID);
	m_bRegister = false;
}

CRC_ConsoleViewport::~CRC_ConsoleViewport()
{
	MAUTOSTRIP(CRC_ConsoleViewport_dtor, MAUTOSTRIP_VOID);
	if (m_bRegister) RemoveFromConsole();
}

void CRC_ConsoleViewport::Create(bool _bRegister)
{
	MAUTOSTRIP(CRC_ConsoleViewport_Create, MAUTOSTRIP_VOID);
	m_bRegister = _bRegister;
	if (m_bRegister) AddToConsole();
	m_TargetFOV = 90;
}

void CRC_ConsoleViewport::Refresh()
{
	MAUTOSTRIP(CRC_ConsoleViewport_Refresh, MAUTOSTRIP_VOID);
//	CMTime Time = CMTime::GetCPU();
	CMTime Time;
	Time.Snapshot();
	fp32 dTime = Clamp01((Time - m_LastRefresh).GetTime());

	fp32 Zoom = dTime * 400.0f;

	if (m_TargetFOV < GetFOV())
		SetFOV(GetFOV() - Min(Zoom, GetFOV() - m_TargetFOV));
	else
		SetFOV(GetFOV() + Min(Zoom, m_TargetFOV - GetFOV()));

	m_LastRefresh = Time;
}


void CRC_ConsoleViewport::ReadSettings(CRegistry* _pReg)
{
	MAUTOSTRIP(CRC_ConsoleViewport_ReadSettings, MAUTOSTRIP_VOID);
	SetFOV(_pReg->GetValuef("VP_FOV", 95.0f));
	SetAspectRatio(_pReg->GetValuef("VP_ASPECTRATIO", 1.0f));
	SetFrontPlane(_pReg->GetValuef("VP_FRONTPLANE", 4.0f));
	SetBackPlane(_pReg->GetValuef("VP_BACKPLANE", 2048.0f));
	m_TargetFOV = GetFOV();
}

void CRC_ConsoleViewport::WriteSettings(CRegistry* _pReg)
{
	MAUTOSTRIP(CRC_ConsoleViewport_WriteSettings, MAUTOSTRIP_VOID);

	if (_pReg->GetValuef("VP_FOV", 95.0f) != GetFOV())
		_pReg->SetValuef("VP_FOV", GetFOV());
	else
		_pReg->DeleteKey("VP_FOV");

	if (_pReg->GetValuef("VP_FRONTPLANE", 4.0f) != GetFrontPlane())
		_pReg->SetValuef("VP_FRONTPLANE", GetFrontPlane());
	else
		_pReg->DeleteKey("VP_FRONTPLANE");

	if (_pReg->GetValuef("VP_BACKPLANE", 2048.0f) != GetBackPlane())
		_pReg->SetValuef("VP_BACKPLANE", GetBackPlane());
	else
		_pReg->DeleteKey("VP_BACKPLANE");
}

void CRC_ConsoleViewport::Con_SetFOV(fp32 _Value)
{
	MAUTOSTRIP(CRC_ConsoleViewport_Con_SetFOV, MAUTOSTRIP_VOID);
	SetFOV(_Value);
}

void CRC_ConsoleViewport::Con_Zoom(fp32 _Value)
{
	MAUTOSTRIP(CRC_ConsoleViewport_Con_Zoom, MAUTOSTRIP_VOID);
	m_TargetFOV = _Value;
//	m_LastRefresh = CMTime::GetCPU();
	m_LastRefresh.Snapshot();
}

void CRC_ConsoleViewport::Con_SetAspectRatio(fp32 _Value)
{
	MAUTOSTRIP(CRC_ConsoleViewport_Con_SetAspectRatio, MAUTOSTRIP_VOID);
	SetAspectRatio(_Value);
}

void CRC_ConsoleViewport::Con_SetFrontPlane(fp32 _Value)
{
	MAUTOSTRIP(CRC_ConsoleViewport_Con_SetFrontPlane, MAUTOSTRIP_VOID);
	SetFrontPlane(_Value);
}

void CRC_ConsoleViewport::Con_SetBackPlane(fp32 _Value)
{
	MAUTOSTRIP(CRC_ConsoleViewport_Con_SetBackPlane, MAUTOSTRIP_VOID);
	SetBackPlane(_Value);
}

void CRC_ConsoleViewport::Register(CScriptRegisterContext & _RegContext)
{
	MAUTOSTRIP(CRC_ConsoleViewport_Register, MAUTOSTRIP_VOID);
	_RegContext.RegFunction("vp_fov", this, &CRC_ConsoleViewport::Con_SetFOV);
	_RegContext.RegFunction("vp_zoom", this, &CRC_ConsoleViewport::Con_Zoom);
	_RegContext.RegFunction("vp_aspectratio", this, &CRC_ConsoleViewport::Con_SetAspectRatio);
	_RegContext.RegFunction("vp_frontplane", this, &CRC_ConsoleViewport::Con_SetFrontPlane);
	_RegContext.RegFunction("vp_backplane", this, &CRC_ConsoleViewport::Con_SetBackPlane);
};

// -------------------------------------------------------------------
//  CRC_FontChar
// -------------------------------------------------------------------
CRC_FontChar::CRC_FontChar()
{
	MAUTOSTRIP(CRC_FontChar_ctor, MAUTOSTRIP_VOID);
	m_xOfs = 0;
	m_yOfs = 0;
	m_TVec0 = 0;
	m_TVec1 = 0;
	m_Dimensions = 0;
	m_iLocal = -1;
	m_Spacing = 0;
	m_Char = 0;
}

void CRC_FontChar::Read_v0000(CCFile* _pFile)
{
	MAUTOSTRIP(CRC_FontChar_Read_v0000, MAUTOSTRIP_VOID);
	_pFile->ReadLE(m_xOfs);
	_pFile->ReadLE(m_yOfs);
	m_TVec0.Read(_pFile);
	m_TVec1.Read(_pFile);
	m_Dimensions.Read(_pFile);
	_pFile->ReadLE(m_iLocal);
	_pFile->ReadLE(m_Spacing);
}

void CRC_FontChar::Read_v0200(CCFile* _pFile)
{
	MAUTOSTRIP(CRC_FontChar_Read_v0200, MAUTOSTRIP_VOID);
	_pFile->ReadLE(m_xOfs);
	_pFile->ReadLE(m_yOfs);
	m_TVec0.Read(_pFile);
	m_TVec1.Read(_pFile);
	m_Dimensions.Read(_pFile);
	_pFile->ReadLE(m_iLocal);
	_pFile->ReadLE(m_Spacing);
	_pFile->ReadLE(m_Char);
}

void CRC_FontChar::Write_v0200(CCFile* _pFile)
{
	MAUTOSTRIP(CRC_FontChar_Write_v0200, MAUTOSTRIP_VOID);
	_pFile->WriteLE(m_xOfs);
	_pFile->WriteLE(m_yOfs);
	m_TVec0.Write(_pFile);
	m_TVec1.Write(_pFile);
	m_Dimensions.Write(_pFile);
	_pFile->WriteLE(m_iLocal);
	_pFile->WriteLE(m_Spacing);
	_pFile->WriteLE(m_Char);
}

// -------------------------------------------------------------------
//  CRC_Font, RenderContext-font
// -------------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(CRC_Font);



void CRC_Font::CRC_CharHash::Create(int _nChars)
{
	MAUTOSTRIP(CRC_Font_CRC_CharHash_Create, MAUTOSTRIP_VOID);
	m_lHash.Clear();
	m_lIDInfo.Clear();
	THash<int16, CRC_CharHashElem>::Create(_nChars, 256, false);
}

void CRC_Font::CRC_CharHash::Insert(int _Index, wchar _Char)
{
	MAUTOSTRIP(CRC_Font_CRC_CharHash_Insert, MAUTOSTRIP_VOID);
	THash<int16, CRC_CharHashElem>::Remove(_Index);
	THash<int16, CRC_CharHashElem>::Insert(_Index, int(_Char) & 0xff);
	CHashIDInfo* pID = &m_pIDInfo[_Index];
	pID->m_Char = _Char;
}

int CRC_Font::CRC_CharHash::GetIndex(wchar _Char)
{
	MAUTOSTRIP(CRC_Font_CRC_CharHash_GetIndex, 0);
	int ID = m_pHash[int(_Char) & 0xff];
	while(ID != -1)
	{
		if (m_pIDInfo[ID].m_Char == _Char) return ID;
		ID = m_pIDInfo[ID].m_iNext;
	}
	return -1;
}


CRC_Font::CRC_Font()
{
	MAUTOSTRIP(CRC_Font_ctor, MAUTOSTRIP_VOID);
	m_OriginalSize = 1.0f;
	m_OriginalSizeRcp = 1.0f;
	m_TextureID = 0;
	m_TexturePixelUV.SetScalar(1.0f);
	CXR_Util::Init();
}

CRC_Font::~CRC_Font()
{
	MAUTOSTRIP(CRC_Font_dtor, MAUTOSTRIP_VOID);
}

void CRC_Font::BuildCharDescHash()
{
	MAUTOSTRIP(CRC_Font_BuildCharDescHash, MAUTOSTRIP_VOID);
	m_CharDescHash.Create(m_lCharDesc.Len());
	for(int i = 0; i < m_lCharDesc.Len(); i++)
		m_CharDescHash.Insert(i, m_lCharDesc[i].m_Char);
}

CRC_FontChar& CRC_Font::GetCharDesc(wchar _Char)
{
	MAUTOSTRIP(CRC_Font_GetCharDesc, *((void*)NULL));
/*	if (!m_lCharDesc.ValidPos(_Char))
	{
		_Char = '?';
		if (!m_lCharDesc.ValidPos(_Char))
		{
			_Char = '*';
			if (!m_lCharDesc.ValidPos(_Char))
			{
				_Char = 0;
				if (!m_lCharDesc.ValidPos(_Char))
					Error("GetCharDesc", "Font have no characters.");
			}
		}
	}

	return m_lCharDesc[_Char];*/


	int iCharDesc = m_CharDescHash.GetIndex(_Char);
	if (iCharDesc == -1)
	{
		iCharDesc = m_CharDescHash.GetIndex(64);
		if (iCharDesc == -1)
		{
			iCharDesc = m_CharDescHash.GetIndex('?');
			if (iCharDesc == -1)
			{
				iCharDesc = m_CharDescHash.GetIndex('*');
				if (iCharDesc == -1)
				{
					iCharDesc = 0;
					if (!m_lCharDesc.ValidPos(iCharDesc))
						Error("GetCharDesc", "Font have no characters.");
				}
			}
		}
	}

	return m_lCharDesc[iCharDesc];
}

fp32 CRC_Font::GetOriginalSize()
{
	MAUTOSTRIP(CRC_Font_GetOriginalSize, 0.0f);
	return m_OriginalSize;
}

int CRC_Font::IsControlCode(const char *_pStr, int _iPos)
{
	MAUTOSTRIP(CRC_Font_IsControlCode, 0);
	int CodeLen = 0;

	if((uint8)_pStr[_iPos] == (uint8)'§')
	{
		CodeLen = 1;
		switch(_pStr[_iPos + 1])
		{
			case 'd':
			case 'D':
				CodeLen = 2;
				break;
			case 'a':
			case 'A':
				CodeLen = 3;
				break;

			case 'z':
			case 'Z':
				CodeLen = 4;
				break;

			case 'n':
			case 'N':
			case 't':
			case 'T':
			case 'c':
			case 'C':
			case 'x':
			case 'X':
			case 'y':
			case 'Y':
				CodeLen = 5;
		}
	}

	return CodeLen;
}

int CRC_Font::IsControlCode(const wchar *_pStr, int _iPos)
{
	MAUTOSTRIP(CRC_Font_IsControlCode_2, 0);
	int CodeLen = 0;

	if(_pStr[_iPos] == (uint8)'§')
	{
		CodeLen = 1;
		switch((char)_pStr[_iPos + 1])
		{
			case 'd':
			case 'D':
				CodeLen = 2;
				break;

			case 'a':
			case 'A':
				CodeLen = 3;
				break;

			case 'z':
			case 'Z':
				CodeLen = 4;
				break;

			case 'n':
			case 'N':
			case 't':
			case 'T':
			case 'c':
			case 'C':
			case 'x':
			case 'X':
			case 'y':
			case 'Y':
				CodeLen = 5;
		}
	}

	return CodeLen;
}

int CRC_Font::IsControlCode(CStr _Str, int _iPos)
{
	MAUTOSTRIP(CRC_Font_IsControlCode_3, 0);
	if (_Str.IsUnicode())
		return IsControlCode(_Str.StrW(), _iPos);
	else
		return IsControlCode(_Str.Str(), _iPos);
}

int CRC_Font::GetControlCodes(const char *_pStr, char *_pRes, int _ResLen)
{
	MAUTOSTRIP(CRC_Font_GetControlCodes, 0);
	if(_ResLen == 0)
		return 0;

	int iIndex = 0;

	int iLen = CStrBase::StrLen(_pStr);

	for(int i = 0; i < iLen; i++)
	{
		int CodeLen = IsControlCode(_pStr, i);
		if(CodeLen != 0)
		{
			if(iIndex + CodeLen + 1 >= _ResLen)
				return iIndex;

			for(int j = 0; j < CodeLen; j++)
				_pRes[iIndex++] = _pStr[i++];
			i -= 1;
		}
	}

	//Add terminator. Range checking for terminator already assured.
	_pRes[iIndex] = 0;

	return iIndex;
}

int CRC_Font::GetControlCodes(const wchar *_pStr, wchar *_pRes, int _ResLen)
{
	MAUTOSTRIP(CRC_Font_GetControlCodes_2, 0);
	if(_ResLen == 0)
		return 0;

	int iIndex = 0;

	int iLen = CStrBase::StrLen(_pStr);

	for(int i = 0; i < iLen; i++)
	{
		int CodeLen = IsControlCode(_pStr, i);
		if(CodeLen != 0)
		{
			if(iIndex + CodeLen + 1 >= _ResLen)
				return iIndex;

			for(int j = 0; j < CodeLen; j++)
				_pRes[iIndex++] = _pStr[i++];
			i -= 1;
		}
	}

	//Add terminator. Range checking for terminator already assured.
	_pRes[iIndex] = 0;

	return iIndex;
}

fp32 CRC_Font::GetWidth(fp32 _SizeX, const char* _pStr)
{
	MAUTOSTRIP(CRC_Font_GetWidth, 0.0f);
	fp32 SizeX = _SizeX;

	mint Len = strlen(_pStr);
	fp32 Pos = 0.0f;
	for(int i = 0; i < Len; i++)
	{
		int CodeLen = IsControlCode(_pStr, i);
		if(CodeLen > 0)
		{
			switch(_pStr[i + 1])
			{
			case 'z' :
				{
					// Relative size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 1.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 0.1f;
					SizeX = z * _SizeX;
					break;
				}
			case 'Z' :
				{
					// Absolute size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 10.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 1.0f;
					SizeX = z;
					break;
				}
			}
			i += CodeLen - 1;
		}
		else
		{
			CRC_FontChar& Desc = GetCharDesc(_pStr[i]);
			Pos += fp32(Desc.m_Spacing)*SizeX;
		}
	}
	return Pos * m_OriginalSizeRcp;
}

fp32 CRC_Font::GetWidthOfCL(int CharLength, const char* _pStr)
{
	MAUTOSTRIP(CRC_Font_GetWidthOfCL, 0.0f);
	fp32 SizeX = 0.0f, _SizeX = 0.0f;

	int Len = CharLength;
	fp32 Pos = 0.0f;
	for(int i = 0; i < Len; i++)
	{
		int CodeLen = IsControlCode(_pStr, i);
		if(CodeLen > 0)
		{
			switch(_pStr[i + 1])
			{
			case 'z' :
				{
					// Relative size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 1.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 0.1f;
					SizeX = z * _SizeX;
					break;
				}
			case 'Z' :
				{
					// Absolute size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 10.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 1.0f;
					SizeX = z;
					break;
				}
			}
			i += CodeLen - 1;
		}
		else
		{
			CRC_FontChar& Desc = GetCharDesc(_pStr[i]);
			Pos += fp32(Desc.m_Spacing)*SizeX;
		}
	}
	return Pos * m_OriginalSizeRcp;
}

fp32 CRC_Font::GetHeight(fp32 _Size, const char* _pStr)
{
	MAUTOSTRIP(CRC_Font_GetHeight, 0.0f);
	fp32 MaxSize = -1;

	mint Len = strlen(_pStr);
	for(int i = 0; i < Len; i++)
	{
		int CodeLen = IsControlCode(_pStr, i);
		if(CodeLen > 0)
		{
			switch(_pStr[i + 1])
			{
			case 'z' :
				{
					// Relative size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 1.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 0.1f;
					MaxSize = Max(MaxSize, z * _Size);
					break;
				}
			case 'Z' :
				{
					// Absolute size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 10.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 1.0f;
					MaxSize = Max(MaxSize, z);
					break;
				}
			}
			i += CodeLen - 1;
		}
		else
		{
//			CRC_FontChar& Desc = GetCharDesc(_pStr[i]);
		}
	}
	if (MaxSize < 0) MaxSize = _Size;
	return MaxSize;
}


int CRC_Font::GetFit(fp32 _Size, const char* _pStr, int FontWidth, bool _bWordWrap)
{
	MAUTOSTRIP(CRC_Font_GetFit, 0);
	// Returns the number of chars that will fit within FontWidth, with optional word-check.

	fp32 Size = _Size * m_OriginalSizeRcp;
	
	int i = 0;
	fp32 x = 0;
	int LastBreak = 0;
	while(_pStr[i])
	{
		int CodeLen = IsControlCode(_pStr, i);
		if(CodeLen > 0)
		{
			switch(_pStr[i + 1])
			{
			case 'z' :
				{
					// Relative size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 1.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 0.1f;
					Size = z * _Size * m_OriginalSizeRcp;
					break;
				}
			case 'Z' :
				{
					// Absolute size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 10.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 1.0f;
					Size = z * m_OriginalSizeRcp;
					break;
				}
			}
			i += CodeLen;
		}
		else
		{
			char ch = _pStr[i++];
			CRC_FontChar& Desc = GetCharDesc(ch);

			if (Desc.m_iLocal < 0) continue;

			fp32 w = fp32(Desc.m_Spacing) * Size;
			if (w + x > FontWidth)
			{
	//LogFile(CStrF("%f, %f, %d, %d, %d", w, w+x, FontWidth, LastBreak, i));
				if (_bWordWrap) 
					return LastBreak;
				else
					return i - 1;
			}

			if ((ch == 32) || (ch == '-') || (ch == '.') || (ch == ',')) LastBreak = i;
			x += w;
		}
	}
	return i;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| wchar versions
|__________________________________________________________________________________________________
\*************************************************************************************************/
fp32 CRC_Font::GetWidth(fp32 _SizeX, const wchar* _pStr)
{
	MAUTOSTRIP(CRC_Font_GetWidth_2, 0.0f);
	fp32 SizeX = _SizeX;

	int Len = CStrBase::StrLen(_pStr);
	fp32 Pos = 0.0f;
	for(int i = 0; i < Len; i++)
	{
		int CodeLen = IsControlCode(_pStr, i);
		if(CodeLen > 0)
		{
			switch(_pStr[i + 1])
			{
			case 'z' :
				{
					// Relative size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 1.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 0.1f;
					SizeX = z * _SizeX;
					break;
				}
			case 'Z' :
				{
					// Absolute size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 10.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 1.0f;
					SizeX = z;
					break;
				}
			}
			i += CodeLen - 1;
		}
		else
		{
			CRC_FontChar& Desc = GetCharDesc(_pStr[i]);
			Pos += fp32(Desc.m_Spacing)*SizeX;
		}
	}
	return Pos * m_OriginalSizeRcp;
}

fp32 CRC_Font::GetWidthOfCL(int CharLength, const wchar* _pStr)
{
	MAUTOSTRIP(CRC_Font_GetWidthOfCL_2, 0.0f);
	fp32 SizeX = 0.0f, _SizeX = 0.0f;

	int Len = CharLength;
	fp32 Pos = 0.0f;
	for(int i = 0; i < Len; i++)
	{
		int CodeLen = IsControlCode(_pStr, i);
		if(CodeLen > 0)
		{
			switch(_pStr[i + 1])
			{
			case 'z' :
				{
					// Relative size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 1.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 0.1f;
					SizeX = z * _SizeX;
					break;
				}
			case 'Z' :
				{
					// Absolute size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 10.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 1.0f;
					SizeX = z;
					break;
				}
			}
			i += CodeLen - 1;
		}
		else
		{
			CRC_FontChar& Desc = GetCharDesc(_pStr[i]);
			Pos += fp32(Desc.m_Spacing)*SizeX;
		}
	}
	return Pos * m_OriginalSizeRcp;
}

#if 0
fp32 CRC_Font::GetHeight(fp32 _Size, const wchar* _pStr)
{
	MAUTOSTRIP(CRC_Font_GetHeight_2, 0.0f);
	fp32 MaxSize = -1;

	int Len = CStrBase::StrLen(_pStr);
	for(int i = 0; i < Len; i++)
	{
		int CodeLen = IsControlCode(_pStr, i);
		if(CodeLen > 0)
		{
			switch(_pStr[i + 1])
			{
			case 'z' :
				{
					// Relative size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 1.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 0.1f;
					MaxSize = Max(MaxSize, z * _Size);
					break;
				}
			case 'Z' :
				{
					// Absolute size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 10.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 1.0f;
					MaxSize = Max(MaxSize, z);
					break;
				}
			}
			i += CodeLen - 1;
		}
		else
		{
//			CRC_FontChar& Desc = GetCharDesc(_pStr[i]);
		}
	}
	if (MaxSize < 0) MaxSize = _Size;
	return MaxSize;
}

#else

fp32 CRC_Font::GetHeight(fp32 _Size, const wchar* _pStr)
{
	MAUTOSTRIP(CRC_Font_GetHeight_2, 0.0f);
	vec128 MaxSize = M_VScalar(-1.0f);
	vec128 VSize = M_VLdScalar(_Size);

	int Len = CStrBase::StrLen(_pStr);
	for(int i = 0; i < Len; i++)
	{
		int CodeLen = IsControlCode(_pStr, i);
		if(CodeLen > 0)
		{
			switch(_pStr[i + 1])
			{
			case 'z' :
				{
					// Relative size
//					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 1.0f;
//					z += fp32((uint8)_pStr[i + 3] - '0') * 0.1f;
					vec128 chars = M_VCnv_i32_f32(M_VCnvL_u16_u32(M_VSubs_u16(M_VMrgL_u16(M_VLdScalar_u16((uint16&)_pStr[i + 2]), M_VLdScalar_u16((uint16&)_pStr[i + 3])), M_VScalar_u16('0'))));
					vec128 tmp = M_VMul(chars, M_VConst(1.0f, 0.1f, 0.0f, 0.0f));
					vec128 vz = M_VAdd(M_VSplatX(tmp), M_VSplatY(tmp));
					MaxSize = M_VMax(MaxSize, M_VMul(vz, VSize));
//					MaxSize = Max(MaxSize, z * _Size);
					break;
				}
			case 'Z' :
				{
					// Absolute size
/*					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 10.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 1.0f;
					MaxSize = Max(MaxSize, z);*/
					vec128 chars = M_VCnvL_u16_u32(M_VSubs_u16(M_VMrgL_u16(M_VLdScalar_u16((uint16&)_pStr[i + 2]), M_VLdScalar_u16((uint16&)_pStr[i + 3])), M_VScalar_u16('0')));
					vec128 tmp = M_VMul(M_VCnv_i32_f32(chars), M_VConst(10.0f, 1.0f, 0.0f, 0.0f));
					vec128 vz = M_VAdd(M_VSplatX(tmp), M_VSplatY(tmp));
					MaxSize = M_VMax(MaxSize, vz);
					break;
				}
			}
			i += CodeLen - 1;
		}
		else
		{
			//			CRC_FontChar& Desc = GetCharDesc(_pStr[i]);
		}
	}
	MaxSize = M_VSelMsk(M_VCmpLTMsk(MaxSize, M_VZero()), VSize, MaxSize);
	fp32 Ret;
	M_VStAny32(MaxSize, &Ret);
	return Ret;
//	if (MaxSize < 0) MaxSize = _Size;
//	return MaxSize;
}

#endif


int CRC_Font::GetFit(fp32 _Size, const wchar* _pStr, int FontWidth, bool _bWordWrap)
{
	MAUTOSTRIP(CRC_Font_GetFit_2, 0);
	// Returns the number of chars that will fit within FontWidth, with optional word-check.

	fp32 Size = _Size * m_OriginalSizeRcp;
	
	int i = 0;
	fp32 x = 0;
	int LastBreak = 0;
	while(_pStr[i])
	{
		int CodeLen = IsControlCode(_pStr, i);
		if(CodeLen > 0)
		{
			switch(_pStr[i + 1])
			{
			case 'z' :
				{
					// Relative size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 1.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 0.1f;
					Size = z * _Size * m_OriginalSizeRcp;
					break;
				}
			case 'Z' :
				{
					// Absolute size
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 10.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 1.0f;
					Size = z * m_OriginalSizeRcp;
					break;
				}
			}
			i += CodeLen;
		}
		else
		{
			wchar ch = _pStr[i++];
			CRC_FontChar& Desc = GetCharDesc(ch);

			if (Desc.m_iLocal < 0) continue;

			fp32 w = fp32(Desc.m_Spacing) * Size;
			if (w + x > FontWidth)
			{
	//LogFile(CStrF("%f, %f, %d, %d, %d", w, w+x, FontWidth, LastBreak, i));
				if (_bWordWrap) 
					return LastBreak;
				else
					return i - 1;
			}

			if ((ch == 32) || (ch == '-') || (ch == '.') || (ch == ',')) LastBreak = i;
			x += w;
		}
	}
	return i;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStr versions
|__________________________________________________________________________________________________
\*************************************************************************************************/
fp32 CRC_Font::GetWidth(fp32 _Size, CStr _Str)
{
	MAUTOSTRIP(CRC_Font_GetWidth_3, 0.0f);
	if (_Str.IsUnicode())
		return GetWidth(_Size, _Str.StrW());
	else
		return GetWidth(_Size, _Str.Str());
}

fp32 CRC_Font::GetHeight(fp32 _Size, CStr _Str)
{
	MAUTOSTRIP(CRC_Font_GetHeight_3, 0.0f);
	if (_Str.IsUnicode())
		return GetHeight(_Size, _Str.StrW());
	else
		return GetHeight(_Size, _Str.Str());
}

fp32 CRC_Font::GetWidthOfCL(int CharLength, CStr _Str)
{
	MAUTOSTRIP(CRC_Font_GetWidthOfCL_3, 0.0f);
	if (_Str.IsUnicode())
		return GetWidthOfCL(CharLength, _Str.StrW());
	else
		return GetWidthOfCL(CharLength, _Str.Str());
}

int CRC_Font::GetFit(fp32 _Size, CStr _Str, int _Width, bool _bWordWrap)
{
	MAUTOSTRIP(CRC_Font_GetFit_3, 0);
	if (_Str.IsUnicode())
		return GetFit(_Size, _Str.StrW(), _Width, _bWordWrap);
	else
		return GetFit(_Size, _Str.Str(), _Width, _bWordWrap);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| IO Stuff
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CRC_Font::ReadFromFile(CStr _Filename)
{
	MAUTOSTRIP(CRC_Font_ReadFromFile, MAUTOSTRIP_VOID);
	CDataFile DFile;
	DFile.Open(_Filename);
	ReadFromXFC(&DFile);
	DFile.Close();

	{
		MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");

		for(int i = 0; i < m_spTC->GetNumTextures(); i++)
		{
			CTexture* pTex = m_spTC->GetTextureMap(i, CTC_TEXTUREVERSION_ANY, false);
			if (pTex != NULL)
			{
				pTex->m_Properties.m_Flags |= CTC_TEXTUREFLAGS_CLAMP_U;
				pTex->m_Properties.m_Flags |= CTC_TEXTUREFLAGS_CLAMP_V;
				pTex->m_Properties.m_Flags |= CTC_TEXTUREFLAGS_NOPICMIP;
			}

			// Make the font texture resident.
			int TxtID = m_spTC->GetTextureID(i);
			pTC->SetTextureParam(TxtID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_RESIDENT);
		}
	}
}

void CRC_Font::ReadFromXFC(CDataFile* _pDFile)
{
	MAUTOSTRIP(CRC_Font_ReadFromXFC, MAUTOSTRIP_VOID);
	_pDFile->PushPosition();

	spCTextureContainer_VirtualXTC spTC = MNew(CTextureContainer_VirtualXTC);
	if (!spTC) MemError("ReadFromXFC");
	spTC->Create(_pDFile);
	m_spTC = spTC;


/*	if (!_pDFile->GetNext("IMAGELIST")) Error("ReadFromXFC", "Currupt font-file.");
	if (!_pDFile->GetSubDir()) Error("ReadFromXFC", "Currupt font-file.");
	m_spTC = DNew(CTextureContainer_Plain) CTextureContainer_Plain;
	if (!m_spTC) MemError("ReadFromXFC");
	m_spTC->AddFromImageList(_pDFile);*/
	_pDFile->PopPosition();

	_pDFile->PushPosition();
	if (!_pDFile->GetNext("CHARDESC")) Error("ReadFromXFC", "Currupt font-file.");
	m_lCharDesc.SetLen(_pDFile->GetUserData());
	{
//		if (_pDFile->GetEntrySize() != m_lCharDesc.ListSize()) Error("ReadFromXFC", "Char-description record size missmatch.");
		if (_pDFile->GetUserData2() == 0)
		{
			for(int i = 0; i < m_lCharDesc.Len(); i++)
			{
				m_lCharDesc[i].Read_v0000(_pDFile->GetFile());
				m_lCharDesc[i].m_Char = i;
			}
		}
		else if (_pDFile->GetUserData2() == 0x0200)
		{
			for(int i = 0; i < m_lCharDesc.Len(); i++)
				m_lCharDesc[i].Read_v0200(_pDFile->GetFile());
		}
		else
			Error("ReadFromXFC", CStrF("Invalid version %.4x", _pDFile->GetUserData2()));
	}
	_pDFile->PopPosition();

	_pDFile->PushPosition();
	if (_pDFile->GetNext("ORIGINALSIZE"))
	{
		_pDFile->GetFile()->ReadLE(m_OriginalSize);
		m_OriginalSizeRcp = 1.0f / m_OriginalSize;
	}
	else
	{
		m_OriginalSize = 1.0f;
		m_OriginalSizeRcp = 1.0f;
	}
	_pDFile->PopPosition();

	m_TextureID = m_spTC->GetTextureID(0);
	CImage Desc; int nMipMaps = 0;
	m_spTC->GetTextureDesc(0, &Desc, nMipMaps);
	m_TexturePixelUV = CVec2Dfp32(1.0f / Desc.GetWidth(), 1.0f / Desc.GetHeight());
	

/*	Vad är det här för nonsense?

	_pDFile->PushPosition();
	if (_pDFile->GetNext("TEXTURENAME"))
	{
		m_TextureName.Read(_pDFile->GetFile());

		MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
		if (pTC) m_TextureID = pTC->GetTextureID(m_TextureName);
		if (!m_TextureID)
			LogFile(CStrF("§cf80WARNING: Unable to obtain texture ID for font-texture %s", (char*)m_TextureName));
		else
		{
			int LocalID = m_spTC->GetLocal(m_TextureName);
			spCTexture Tex = m_spTC->GetTexture(LocalID);
			if (Tex != NULL)
			{
				Tex->m_Properties.m_Flags |= CTC_TEXTUREFLAGS_CLAMP_U;
				Tex->m_Properties.m_Flags |= CTC_TEXTUREFLAGS_CLAMP_V;
			}
		}
	}
	_pDFile->PopPosition();
*/
	BuildCharDescHash();
}

#ifndef PLATFORM_CONSOLE
void CRC_Font::ReadFromScript(CStr _Filename)
{
	MAUTOSTRIP(CRC_Font_ReadFromScript, MAUTOSTRIP_VOID);
	m_spTC = MNew(CTextureContainer_Plain);
	if(!m_spTC) MemError("ReadFromScript");
	
	int CurSpacing = 0;
	int CurHeight = 0;
	int CurWidth = 0;
	CVec2Dfp32 CurOffset = 0;
	int CurTexture = -1;

	m_lCharDesc.SetLen(256);

	CCFile File;
	File.Open(_Filename, CFILE_READ);
	CStr Path = _Filename.GetPath();

	while(!File.EndOfFile())
	{
		CStr s(File.Readln().LTrim().RTrim());
		s = s.GetStrSep("\\\\");
		if (s.Copy(0,1) == (const char *)";") continue;
//		if (s.Copy(0,2) == "\\\\") continue;

		if (s.Len())
		{
			CStr Orgs(s);
			CStr KeyW((s.GetStrSep(",")).UpperCase().LTrim().RTrim());
			if(KeyW == (const char *)"WIDTH")
			{
				CurWidth = (s.GetStrSep(",")).Val_int();
			}
			else if(KeyW == (const char *)"HEIGHT")
			{
				CurHeight = (s.GetStrSep(",")).Val_int();
			}
			else if(KeyW == (const char *)"SPACING")
			{
				CurSpacing = (s.GetStrSep(",")).Val_int();
			}
			else if(KeyW == (const char *)"TEXTURENR")
			{
				CurTexture = s.GetStrSep(",").Val_int();
				if ((CurTexture < 0) || (CurTexture >= m_spTC->GetNumTextures()))
					Error("ReadFromScript", CStrF("Invalid texture nr. (%d)", CurTexture));
			}
			else if(KeyW == (const char *)"TEXTURENAME")
			{
				m_TextureName = s;
			}
			else if(KeyW == (const char *)"ROW")	// ,x, y, txtspacing, startchar, numchar
			{
				CRC_FontChar ch;
				ch.m_xOfs = CurOffset.k[0];
				ch.m_yOfs = CurOffset.k[1];
				ch.m_TVec0.k[0] = (s.GetStrSep(",")).Val_int();
				ch.m_TVec0.k[1] = (s.GetStrSep(",")).Val_int();
				fp32 TxtSp = (s.GetStrSep(",")).Val_fp64();
				ch.m_Dimensions.k[0] = CurWidth;
				ch.m_Dimensions.k[1] = CurHeight;
				ch.m_Spacing = CurSpacing;
				ch.m_iLocal = CurTexture;
				int Start = (s.GetStrSep(",")).Val_int();
				int Num = (s.GetStrSep(",")).Val_int();

				for(int i = 0; i < Num; i++)
				{
					m_lCharDesc[i + Start] = ch;
					ch.m_TVec0.k[0] += TxtSp;
				}
			}
			else if(KeyW == (const char *)"CHARPOS")	// charnr, w, h, spacing
			{
				int iChar = (s.GetStrSep(",")).Val_int();
				int x = (s.GetStrSep(",")).Val_int();
				int y = (s.GetStrSep(",")).Val_int();

				if (x >= 0) m_lCharDesc[iChar].m_TVec0.k[0] = x;
				if (y >= 0) m_lCharDesc[iChar].m_TVec0.k[1] = y;
			}
			else if(KeyW == (const char *)"CHARSIZE")	// charnr, w, h, spacing
			{
				int iChar = (s.GetStrSep(",")).Val_int();
				int w = (s.GetStrSep(",")).Val_int();
				int h = (s.GetStrSep(",")).Val_int();
				int sp = (s.GetStrSep(",")).Val_int();

				CRC_FontChar ch = m_lCharDesc[iChar];
				ch.m_Dimensions.k[0] = w;
				ch.m_Dimensions.k[1] = h;
				ch.m_Spacing = sp;
				m_lCharDesc[iChar] = ch;
			}
			else if(KeyW == (const char *)"CHARTAB_X")	// charnr, nchars, x0, x1, x2, ...
			{
				int iChar = (s.GetStrSep(",")).Val_int();
				int nChar = (s.GetStrSep(",")).Val_int();
				for(int ch = iChar; ch < iChar+nChar; ch++)
					m_lCharDesc[ch].m_TVec0.k[0] = (s.GetStrSep(",")).Val_int();
			}
			else if(KeyW == (const char *)"CHARTAB_WIDTH")	// charnr, nchars, w0, w1, w2, ...
			{
				int iChar = (s.GetStrSep(",")).Val_int();
				int nChar = (s.GetStrSep(",")).Val_int();
				for(int ch = iChar; ch < iChar+nChar; ch++)
					m_lCharDesc[ch].m_Dimensions.k[0] = (s.GetStrSep(",")).Val_int();
			}
			else if(KeyW == (const char *)"CHARTAB_SPACING")	// charnr, nchars, s0, s1, s2, ...
			{
				int iChar = (s.GetStrSep(",")).Val_int();
				int nChar = (s.GetStrSep(",")).Val_int();
				for(int ch = iChar; ch < iChar+nChar; ch++)
					m_lCharDesc[ch].m_Spacing = (s.GetStrSep(",")).Val_int();
			}
			else if (KeyW == (const char *)"$")
			{
				CurTexture = m_spTC->AddFromScriptLine(s, Path);
				if (CurTexture > 0)
					Error("ReadFromScript", "Sorry, but fonts can only have a sole texture nowdays.");
/*				CStr TxtName = s.LTrim().RTrim();
				if (TxtName.GetDevice() == "") TxtName = Path + TxtName;
				m_spTC->AddTexture(TxtName);
				CurTexture++;*/
			}
			else if (KeyW == (const char *)"TEXTURE")
			{
				CKeyContainer Keys;
				while(s != "")
				{
					CStr key = s.GetStrSep("=");
					CStr value = s.GetStrSep(";");
					key.Trim();
					value.Trim();
					Keys.AddKey(key, value);
				}

				CurTexture = m_spTC->AddFromKeys(&Keys, Path);
				if (CurTexture > 0)
					Error("ReadFromScript", "Sorry, but fonts can only have a sole texture nowdays.");
/*				CStr TxtName = s.LTrim().RTrim();
				if (TxtName.GetDevice() == "") TxtName = Path + TxtName;
				m_spTC->AddTexture(TxtName);
				CurTexture++;*/
			}
			else if(KeyW == (const char *)"ORGSIZE")
			{
				m_OriginalSize = s.GetStrSep(",").Val_fp64();
				m_OriginalSizeRcp = 1.0f / m_OriginalSize;
			}
			else
				Error("ReadFromScript", CStrF("Invalid command: '%s'", (char*)KeyW));
		}

	}

	// Convert texture-coordinates to homogenous coordinates.
	for(int i = 0; i < m_lCharDesc.Len(); i++)
	{
		CRC_FontChar ch = m_lCharDesc[i];
		if (ch.m_iLocal >= 0)
		{
			CImage Desc;
			int nMipmaps;
			m_spTC->GetTextureDesc(ch.m_iLocal, &Desc, nMipmaps);

			fp32 ws = 1.0f/Desc.GetWidth();
			fp32 hs = 1.0f/Desc.GetHeight();
			ch.m_TVec0.k[0] *= ws;
			ch.m_TVec0.k[1] *= hs;
			ch.m_TVec1.k[0] = ch.m_Dimensions.k[0] * ws + ch.m_TVec0.k[0];
			ch.m_TVec1.k[1] = ch.m_Dimensions.k[1] * hs + ch.m_TVec0.k[1];
//			m_Dimensions.k[0] *= 1.0f/m_OriginalSize;
//			m_Dimensions.k[1] *= 1.0f/m_OriginalSize;
		}

//LogFile(CStrF("%d, %d, %d, %d, (%.2f %.2f)-(%.2f %.2f), %.2f, %.2f", i, ch.m_iLocal, ch.m_Spacing, 0,
//			ch.m_TVec0.k[0], ch.m_TVec0.k[1], ch.m_TVec1.k[0], ch.m_TVec1.k[1], ch.m_Dimensions.k[0], ch.m_Dimensions.k[1]));

		ch.m_Char = i;
		m_lCharDesc[i] = ch;
	}

	File.Close();
	BuildCharDescHash();
}
#endif

#ifdef PLATFORM_WIN_PC

#include <Windows.h>

void CRC_Font::ReadFromScriptWin32(CStr _Filename)
{
	MAUTOSTRIP(CRC_Font_ReadFromScriptWin32, MAUTOSTRIP_VOID);
	spCRegistry spTempReg = REGISTRY_CREATE;
	if (!spTempReg)
		MemError("ReadFromScriptWin32");

	try
	{
		spTempReg->ReadRegistryDir("Settings", _Filename);
	}
	catch(CCException)
	{
		return;
	}

	spCRegistry spEnv = spTempReg->Find("Settings");
	
	if (!spEnv)
		return;

//	CStr OutputFile = spEnv->GetValue("OutputFile", "C:\\Test.xfc");
//	CStr OutputTextureContainer = spEnv->GetValue("OutputTextureContainer", "C:\\Test.xtc");
	CStr OutputTextureName = spEnv->GetValue("OutputTextureName", "FontTexture");
	CStr CharSetStr = spEnv->GetValue("CharSet", "Western");
	CStr FontName = spEnv->GetValue("FontName", "Times Roman");
	int FontHeight = spEnv->GetValuei("Height", 10);
	int FontWidth = spEnv->GetValuei("Width", 0);
	int FontWeight = spEnv->GetValuei("Weight", FW_NORMAL);
	int FontAntiAlias = spEnv->GetValuei("Antialias", 0);
	int FontItalic = spEnv->GetValuei("Italic", 0);
	int FontOutline = spEnv->GetValuei("Outline", 0);
	float FontWidthSpace = spEnv->GetValuef("CharSpace", 1.0);
	float FontWidthSpaceAdd = spEnv->GetValuef("CharSpaceAdd", 0);
	CStr FontPitchStr = spEnv->GetValue("Pitch", "Default");
	CStr WriteImage = spEnv->GetValue("WriteImage", "");
	float Offy = spEnv->GetValuef("OffsetY", 0.0);
	CStr Chars = spEnv->GetValue("Chars");
	Chars = Chars.Unicode();
//	LogFile("Chars = " + Chars);
	TArray<wchar> lChars;
	int Len = Chars.Len();
	int nDefaultCharFirst = spEnv->GetValuei("DefaultCharFirst", ' ');
	int nDefaultCharLast = spEnv->GetValuei("DefaultCharLast", '?');
	int nShadow = spEnv->GetValuei("Shadow", 0);
	int nDefaultChars = nDefaultCharLast-nDefaultCharFirst + 1;
	lChars.SetLen(nDefaultChars + Len);

	int i;
	for(i = nDefaultCharFirst; i <= nDefaultCharLast; i++) lChars[i-nDefaultCharFirst] = i;
	for(i = 0; i < Len; i++)
	{
		lChars[i+nDefaultChars] = Chars.StrW()[i]; 
//		LogFile(CStrF("%.4x", Chars.StrW()[i]));
	}
	
	// Font
	
	HDC ScreenDC;
	HDC TempDC;
//	HBITMAP TempBitmap;

	int StartOffset = 2;

	ScreenDC = GetDC(NULL);
//	TempDC = CreateCompatibleDC(ScreenDC);
//	TempBitmap = CreateCompatibleBitmap(TempDC,400,300);
//	SelectObject(TempDC,TempBitmap);

	TempDC = ScreenDC;


	HBRUSH TempBrush;

	TempBrush = CreateSolidBrush(RGB(0,0,0));

	SelectObject(TempDC,TempBrush);

	Rectangle(TempDC,0,0,400, 300);
	
	BYTE nQuality;
	HFONT Font;

	nQuality = 0;
	Font = NULL;
	
	if ( FontAntiAlias ) 
		nQuality = ANTIALIASED_QUALITY;
	else
		nQuality = NONANTIALIASED_QUALITY;

	int CharSet;
	
	if (!CharSetStr.CompareNoCase("Dos"))
		CharSet = OEM_CHARSET;
	else if (!CharSetStr.CompareNoCase("Arabic"))
		CharSet = ARABIC_CHARSET;
	else if (!CharSetStr.CompareNoCase("Baltic"))
		CharSet = BALTIC_CHARSET;
	else if (!CharSetStr.CompareNoCase("Eastern Europe"))
		CharSet = EASTEUROPE_CHARSET;
	else if (!CharSetStr.CompareNoCase("Russian"))
		CharSet = RUSSIAN_CHARSET;
	else if (!CharSetStr.CompareNoCase("Greek"))
		CharSet = GREEK_CHARSET;
	else if (!CharSetStr.CompareNoCase("Hewbrew"))
		CharSet = ANSI_CHARSET;
	else if (!CharSetStr.CompareNoCase("Thai"))
		CharSet = THAI_CHARSET;
	else if (!CharSetStr.CompareNoCase("Turkish"))
		CharSet = TURKISH_CHARSET;
	else if (!CharSetStr.CompareNoCase("Western"))
		CharSet = ANSI_CHARSET;
	else if (!CharSetStr.CompareNoCase("Vietnamese"))
		CharSet = VIETNAMESE_CHARSET;
	else
		CharSet = ANSI_CHARSET;

	int Pitch;
	int OverideWidth = 0;

	if (!FontPitchStr.CompareNoCase("Default"))
		Pitch = DEFAULT_PITCH;
	else if (!FontPitchStr.CompareNoCase("Fixed"))
		Pitch = FIXED_PITCH;
	else if (!FontPitchStr.CompareNoCase("Variable"))
		Pitch = VARIABLE_PITCH;
	else 
	{
		Pitch = FIXED_PITCH;
		OverideWidth = FontPitchStr.Val_int();
	}
	
	Font = CreateFont(-FontHeight, FontWidth,
		0, 0,
		FontWeight,
		FontItalic,
		FALSE,
		FALSE,
		CharSet,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		nQuality,
		Pitch | FF_DONTCARE,
		FontName);
	
	SelectObject(ScreenDC,Font);
	SelectObject(TempDC,Font);

	SetTextAlign(TempDC, TA_LEFT | TA_TOP/* | TA_NOUPDATECP */);
	
	// Create font;
	
	SetBkColor(TempDC,RGB(0,0,0));
	SetTextColor(TempDC, RGB(255,255,255));
	
	SIZE Size;
	
	int Width = 0;
	int Height = 0;
	
	wchar_t TempChar [2];
	TempChar[1] = 0;
	int x;
	int y;
//	bool Add;
//	int Start[256];

	TArray<int> Widths;
	Widths.SetLen(lChars.Len());
	
	// Get Size of Data
	
	int MaxWidth = 0;
	int MinY = 100000;
	int MaxY = 0;

	int ExtraSize = 0;
	if (FontOutline)
		ExtraSize = 2;
	else if (nShadow)
		ExtraSize = nShadow - 1;

	
	for (i=0; i< lChars.Len(); ++i)
	{
		TempChar[0] = lChars[i];
			
		if (Pitch != FIXED_PITCH)
		{
			if (GetTextExtentPoint32W(TempDC,TempChar,1,&Size))
			{
					
				if (ExtraSize+Size.cy > Height)
					Height = ExtraSize+Size.cy;
					
				x = Size.cx - 1;
				y;
//				Add = false;
					
				Rectangle(TempDC,0,0,StartOffset*2 + Size.cx+11, StartOffset*2 + Height+1);
				TextOutW(TempDC,StartOffset,StartOffset,TempChar,1);

				int Largest = 0;
					
				for (y = 0; y < Height; ++y)
				{
					for (x = 0; x < Size.cx+10; ++x)
					{
						if (GetPixel(TempDC,StartOffset + x,StartOffset + y) != 0)
						{
							Largest = Max(x, Largest);
							MinY    = Min(y, MinY);
							MaxY    = Max(y+1, MaxY);
						}
					}
				}
					
				if (((Largest + 1) * FontWidthSpace) + FontWidthSpaceAdd > Size.cx)
				{
					Size.cx = ((Largest + 1) * FontWidthSpace) + FontWidthSpaceAdd;
				}
					
//				Start[i] = Width;
				Widths[i] = ExtraSize + Size.cx;
					
				Width += Widths[i];
			}
			else
			{
					
				Widths[i] = 0;
//					Start[i] = Width;
					
			}
		}
		else
		{
				
			if (FontWidth == 0 || OverideWidth)
			{
					
				if (GetTextExtentPoint32W(TempDC,TempChar,1,&Size))
				{
					
					if (ExtraSize+Size.cy > Height)
						Height = ExtraSize+Size.cy;
						
					if (((ExtraSize+Size.cx) * FontWidthSpace) + FontWidthSpaceAdd > MaxWidth)
						MaxWidth = ((ExtraSize+Size.cx) * FontWidthSpace) + FontWidthSpaceAdd;
						
				}
					
			}
			else
			{
				if (GetTextExtentPoint32W(TempDC,TempChar,1,&Size))
				{
						
					if (ExtraSize+Size.cy > Height)
						Height = ExtraSize+Size.cy;
						
				}
					
//					Start[i] = Width;
				Widths[i] = FontWidth;
					
				Width += FontWidth;
			}
				
			MinY = 0;
			MaxY = Height;
		}
	}
	Height = MaxY - MinY + ExtraSize;
	
	if (Pitch == FIXED_PITCH)
	{
		if (OverideWidth)
		{
			for (int i=0; i< lChars.Len(); ++i)
			{
				
				//			Start[i] = Width;
				Widths[i] = OverideWidth;
				Width += OverideWidth;
			}
		}		
		else if (FontWidth == 0)
		{
			for (int i=0; i< lChars.Len(); ++i)
			{
				
				//			Start[i] = Width;
				Widths[i] = MaxWidth;
				Width += MaxWidth;
			}
		}
	}

	int NeededLines = 1;
	int CurrentLineWidth = 256;
	int TexturBorder = 1;
	
	for (i = 0; i < lChars.Len(); ++i)
	{
		if ((Widths[i] + TexturBorder * 2) > CurrentLineWidth)
		{
			++NeededLines;
			CurrentLineWidth = 256;
		}

		CurrentLineWidth -= (Widths[i] + TexturBorder * 2);
	}

	int CurrentWidth = 256;

	int NeededHeight = (Height + TexturBorder * 2) * NeededLines;


	while (NeededHeight > CurrentWidth)
	{
		CurrentWidth = CurrentWidth << 1;

		//AR-REMOVE: this doesn't work:  NeededLines = NeededLines >> 1;
		//AR-ADD: recalculate needed number of lines
		NeededLines = 1;
		CurrentLineWidth = CurrentWidth;
		for (i = 0; i < lChars.Len(); ++i)
		{
			if ((Widths[i] + TexturBorder * 2) > CurrentLineWidth)
			{
				++NeededLines;
				CurrentLineWidth = CurrentWidth;
			}
			CurrentLineWidth -= (Widths[i] + TexturBorder * 2);
		}
		NeededHeight = (Height + TexturBorder * 2) * NeededLines;
	}

	if (CurrentWidth > 4096)
		Error("ReadFromScriptWin32", "Font does not fit on a 4096x4096 texture.")

	int TextureWidth = CurrentWidth;
	int TextureHeight = 1;
	while (TextureHeight < NeededHeight)
		TextureHeight = TextureHeight << 1;


	spCImage TextureImage;
	
	if (nShadow || FontOutline)
		TextureImage = MNew5(CImage, TextureWidth, TextureHeight, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE | IMAGE_MEM_LOCKABLE, 0);
	else
		TextureImage = MNew5(CImage, TextureWidth, TextureHeight, IMAGE_FORMAT_A8, IMAGE_MEM_IMAGE | IMAGE_MEM_LOCKABLE, 0);

	CClipRect WholeTexture(0, 0, TextureWidth, TextureHeight);
	// Create Data

	if (nShadow)
		TextureImage->Fill(WholeTexture, CPixel32(0,0,0,0));
	else
		TextureImage->Fill(WholeTexture, CPixel32(255,255,255,0));

///	uint8 *LockedTexture = (uint8 *)TextureImage->Lock();
	
//	memset(LockedTexture, 0, TextureWidth * TextureHeight * 4);

//	TextureImage->Unlock();
	
//	uint8 *TempData;

	m_lCharDesc.SetLen(lChars.Len());
	
	// Create The Font Data

	m_OriginalSize = Height / spEnv->GetValuef("Size", 1.0);	
	m_OriginalSizeRcp = 1.0f / m_OriginalSize;
	int OffsetY = m_OriginalSize * Offy;

	int CurrentX = TexturBorder;
	int CurrentY = TexturBorder;
	
	for (i=0; i< lChars.Len(); ++i)
	{
		if ((CurrentX + Widths[i] + TexturBorder * 2) > TextureWidth)
		{
			CurrentY += Height + TexturBorder * 2;
			CurrentX = TexturBorder;
		}
	
		TempChar[0] = lChars[i];
	
		CRC_FontChar ch;
		
		Rectangle(TempDC,0,0,StartOffset*2 + Widths[i]+1, StartOffset*2 + Height+1);
		
		TextOutW(TempDC,StartOffset,StartOffset-MinY,TempChar,1);

		ch.m_Dimensions.k[0] = Widths[i];
		ch.m_Dimensions.k[1] = Height;
		ch.m_yOfs = OffsetY;

		ch.m_Spacing = Widths[i];

		ch.m_TVec0.k[0] = CurrentX;
		ch.m_TVec0.k[1] = CurrentY;

		{
			
			if (FontOutline)
			{
				for (y = 1; y < Height + 1; y++)
				{
					for (x = 1; x < Widths[i] + 1; x++)
					{
						int PixVal = GetRValue(GetPixel(TempDC,StartOffset + x - 1,StartOffset + y - 1));
						if (PixVal)
						{
							TextureImage->SetPixel(WholeTexture, CPnt(CurrentX + x + 0, CurrentY + y + 1), CPixel32(0,0,0,PixVal));
							TextureImage->SetPixel(WholeTexture, CPnt(CurrentX + x + 1, CurrentY + y + 0), CPixel32(0,0,0,PixVal));
							TextureImage->SetPixel(WholeTexture, CPnt(CurrentX + x + 1, CurrentY + y + 1), CPixel32(0,0,0,PixVal));
							TextureImage->SetPixel(WholeTexture, CPnt(CurrentX + x + 0, CurrentY + y - 1), CPixel32(0,0,0,PixVal));
							TextureImage->SetPixel(WholeTexture, CPnt(CurrentX + x - 1, CurrentY + y + 0), CPixel32(0,0,0,PixVal));
							TextureImage->SetPixel(WholeTexture, CPnt(CurrentX + x - 1, CurrentY + y - 1), CPixel32(0,0,0,PixVal));
							TextureImage->SetPixel(WholeTexture, CPnt(CurrentX + x + 1, CurrentY + y - 1), CPixel32(0,0,0,PixVal));
							TextureImage->SetPixel(WholeTexture, CPnt(CurrentX + x - 1, CurrentY + y + 1), CPixel32(0,0,0,PixVal));
						}
					}
				}				
				for (y = 1; y < Height + 1; y++)
				{
					for (x = 1; x < Widths[i] + 1; x++)
					{
						int PixVal = GetRValue(GetPixel(TempDC,StartOffset + x - 1,StartOffset + y - 1));
						if (PixVal)
							TextureImage->SetPixel(WholeTexture, CPnt(CurrentX + x, CurrentY + y), CPixel32(255,255,255,PixVal));
					}
				}
			}
			else
			{
				if (nShadow)
				{
					for (y = 0; y < Height; y++)
					{
						for (x = 0; x < Widths[i]; x++)
						{
							int PixVal = GetRValue(GetPixel(TempDC,StartOffset + x,StartOffset + y));
							if (PixVal)
								TextureImage->SetPixel(WholeTexture, CPnt(CurrentX + x + nShadow, CurrentY + y + nShadow), CPixel32(0,0,0,PixVal));
						}
					}				
				}
				for (y = 0; y < Height; y++)
				{
					for (x = 0; x < Widths[i]; x++)
					{
						int PixVal = GetRValue(GetPixel(TempDC,StartOffset + x,StartOffset + y));
						if (PixVal)
							TextureImage->SetPixel(WholeTexture, CPnt(CurrentX + x, CurrentY + y), CPixel32(255,255,255,PixVal));
					}
				}
			}
			
		}

		ch.m_Char = lChars[i];
		m_lCharDesc[i] = ch;

		CurrentX += Widths[i] + TexturBorder * 2;
		// Get Pixels
	}	

//	CTextureContainer_Plain TempContainer;
//	try 
//	{
//		TempContainer.AddFromXTC(OutputTextureContainer);
//	}
//	catch(CCException)
//	{
//	}

	if (WriteImage.Len())
	{
		//		ms_lspPhongMaps[iBump]->Write(ms_lspPhongMaps[iBump]->GetRect(), CStrF("E:\\TEST\\PHONG%d.TGA", iBump) );

		spCImage spImg = TextureImage->Convert(IMAGE_FORMAT_BGRA8);
		spImg->Write(WriteImage);
	}

	m_spTC = MNew(CTextureContainer_Plain);
	if(!m_spTC) MemError("ReadFromScript");

	int LocalTextureID = m_spTC->GetLocal(OutputTextureName);

	if (LocalTextureID >= 0)
		m_spTC->DeleteTexture(LocalTextureID);

	CTC_TextureProperties Properties;
	m_spTC->AddTexture(TextureImage, Properties, 0, OutputTextureName);
//	TempContainer.WriteXTC(OutputTextureContainer);

	LocalTextureID = m_spTC->GetLocal(OutputTextureName);
	m_TextureName = OutputTextureName;
	m_TextureID = m_spTC->GetTextureID(LocalTextureID);
	m_TexturePixelUV = CVec2Dfp32(1.0f / TextureWidth, 1.0f / TextureHeight);
	CTexture *Tex = m_spTC->GetTextureMap(LocalTextureID, CTC_TEXTUREVERSION_ANY);
	Tex->m_Properties.m_Flags |= CTC_TEXTUREFLAGS_HIGHQUALITY | CTC_TEXTUREFLAGS_NOPICMIP | CTC_TEXTUREFLAGS_NOCOMPRESS;

	// Fix 
	{		
		for(int i = 0; i < m_lCharDesc.Len(); i++)
		{
			CRC_FontChar ch = m_lCharDesc[i];
			ch.m_iLocal = LocalTextureID;
			
			ch.m_TVec0.k[0] *= m_TexturePixelUV[0];
			ch.m_TVec0.k[1] *= m_TexturePixelUV[1];
			ch.m_TVec1.k[0] = ch.m_Dimensions.k[0] * m_TexturePixelUV[0] + ch.m_TVec0.k[0];
			ch.m_TVec1.k[1] = ch.m_Dimensions.k[1] * m_TexturePixelUV[1] + ch.m_TVec0.k[1];
			
			m_lCharDesc[i] = ch;
		}		
	}


	if (Font)
		DeleteObject(Font);

//	WriteXFC(OutputFile);
	
	DeleteObject(TempBrush);
//	DeleteObject(TempBitmap);
//	DeleteDC(TempDC);
	::ReleaseDC(NULL,ScreenDC);	

	BuildCharDescHash();
}

#endif

#ifndef PLATFORM_CONSOLE
void CRC_Font::WriteXFC(CDataFile* _pDFile)
{
	MAUTOSTRIP(CRC_Font_WriteXFC, MAUTOSTRIP_VOID);
	_pDFile->BeginEntry("IMAGELIST");
	_pDFile->EndEntry(0);
	_pDFile->BeginSubDir();
		m_spTC->WriteImageList(_pDFile);
	_pDFile->EndSubDir();
	_pDFile->BeginEntry("CHARDESC");
	{
		for(int i = 0; i < m_lCharDesc.Len(); i++)
			m_lCharDesc[i].Write_v0200(_pDFile->GetFile());
	}
	_pDFile->EndEntry(m_lCharDesc.Len(), CRC_CHARDESC_VERSION);

	_pDFile->BeginEntry("ORIGINALSIZE");
	_pDFile->GetFile()->WriteLE(m_OriginalSize);
	_pDFile->EndEntry(0);

	if (m_TextureName != "")
	{
		_pDFile->BeginEntry("TEXTURENAME");
		m_TextureName.Write(_pDFile->GetFile());
		_pDFile->EndEntry(0);
	}
}
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Rendering
|__________________________________________________________________________________________________
\*************************************************************************************************/

#if 0

int CRC_Font::Write(int _MaxV, CVec3Dfp32* _pV, CVec2Dfp32* _pTV, CPixel32* _pCol, uint16* _piPrim, 
		CPixel32 _Color, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const wchar* _pStr, 
		const CVec2Dfp32& _Size, const CVec2Dfp32& _MinLimit, const CVec2Dfp32& _MaxLimit)
{
	MAUTOSTRIP(CRC_Font_Write, 0);
	// _piPrim must have space for _MaxV*3/2 indices.

	int Len = CStrBase::StrLen(_pStr);
	if (Len < 1) return 0;
	if (Len*4 > _MaxV) return 0;

	fp32 InvOriginalSize = m_OriginalSizeRcp;
	CVec3Dfp32 p(_Pos);
	fp32 SizeX = _Size.k[0] * InvOriginalSize;
	fp32 SizeY = _Size.k[1] * InvOriginalSize;
	CVec2Dfp32 SizeRcp = CVec2Dfp32(1.0f / _Size[0], 1.0f / _Size[1]);
	fp32 SizeXRcp = m_OriginalSize * SizeRcp[0];
	fp32 SizeYRcp = m_OriginalSize * SizeRcp[1];

//	uint32 IndexRamp[4] = { 0, 1, 2, 3 };

	int nP = 0;
	int nV = 0;

	CVec3Dfp32 Pos(_Pos);

	CPixel32 CurColor = _Color;

	fp32 x = 0;
	fp32 y = 0;
	for(int i = 0; i < Len; i++)
	{
		int CodeLen = IsControlCode(_pStr, i);
		if(CodeLen > 0)
		{
			switch(_pStr[i + 1])
			{
			case 'a' :
			case 'A' :
				{
					int v = (uint8)_pStr[i + 2] - '0';
					if(v > 9) v -= 'a' - '0' - 10;
					int A = v * 255 / 15;
					CurColor = CPixel32(CurColor.GetR(), CurColor.GetG(), CurColor.GetB(), A);
					break;
				}
			case 'c' :
				{
					// Multiply
					int v = (uint8)_pStr[i + 2] - '0'; if(v > 9) v += -'a' + '0' + 10; int R = v * 255 / 15;
					v = (uint8)_pStr[i + 3] - '0'; if(v > 9) v += -'a' + '0' + 10; int G = v * 255 / 15;
					v = (uint8)_pStr[i + 4] - '0'; if(v > 9) v += -'a' + '0' + 10; int B = v * 255 / 15;

					CurColor = _Color;
					CurColor *= CPixel32(R, G, B, 255);
					break;
				}
			case 'C' :
				{
					// Absolute
					int v = (uint8)_pStr[i + 2] - '0'; if(v > 9) v += -'a' + '0' + 10; int R = v * 255 / 15;
					v = (uint8)_pStr[i + 3] - '0'; if(v > 9) v += -'a' + '0' + 10; int G = v * 255 / 15;
					v = (uint8)_pStr[i + 4] - '0'; if(v > 9) v += -'a' + '0' + 10; int B = v * 255 / 15;
					CurColor = CPixel32(R, G, B, CurColor.GetA());
					break;
				}
			case 'd' :
			case 'D' :
				{
					CurColor = _Color;
					break;
				}
			case 'n' :
			case 'N' :
				{
					int dy = ((uint8)_pStr[i + 2] - '0') * 100;
					dy += ((uint8)_pStr[i + 3] - '0') * 10;
					dy += ((uint8)_pStr[i + 4] - '0') * 1;

					x = 0;
					y += dy;
					break;
				}
			case 't' :
			case 'T' :
				{
					int dy = ((uint8)_pStr[i + 2] - '0') * 100;
					dy += ((uint8)_pStr[i + 3] - '0') * 10;
					dy += ((uint8)_pStr[i + 4] - '0') * 1;

					x = dy;
					break;
				}
			case 'x' :
			case 'X' :
				{
					int dx = ((uint8)_pStr[i + 2] - '0') * 100;
					dx += ((uint8)_pStr[i + 3] - '0') * 10;
					dx += ((uint8)_pStr[i + 4] - '0') * 1;
					x += dx;
//					_Pos.Combine(_Dir, x, Pos);
//					Pos.Combine(_VDown, y, Pos);
					break;
				}
			case 'y' :
			case 'Y' :
				{
					int dy = ((uint8)_pStr[i + 2] - '0') * 100;
					dy += ((uint8)_pStr[i + 3] - '0') * 10;
					dy += ((uint8)_pStr[i + 4] - '0') * 1;
					y += dy;
//					_Pos.Combine(_Dir, x, Pos);
//					Pos.Combine(_VDown, y, Pos);
					break;
				}
			case 'z' :
				{
					// Relative
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 1.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 0.1f;
					fp32 zrcp = 1.0f / z;
					SizeX = z * _Size.k[0] * InvOriginalSize;
					SizeY = z * _Size.k[1] * InvOriginalSize;
					SizeXRcp = m_OriginalSize * SizeRcp[0] * zrcp;
					SizeYRcp = m_OriginalSize * SizeRcp[1] * zrcp;
					break;
				}
			case 'Z' :
				{
					// Absolute
					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 10.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 1.0f;
					fp32 zrcp = 1.0f / z;
					SizeX = z * InvOriginalSize;
					SizeY = z * InvOriginalSize;
					SizeXRcp = m_OriginalSize * zrcp;
					SizeYRcp = m_OriginalSize * zrcp;
					break;
				}
			}

			i += CodeLen - 1;
			continue;
		}

		CRC_FontChar& Desc = GetCharDesc(_pStr[i]);
		if (Desc.m_iLocal < 0) continue;

		fp32 ws = Desc.m_Dimensions.k[0] * SizeX;
		fp32 hs = Desc.m_Dimensions.k[1] * SizeY;
		fp32 CurX = x + Desc.m_xOfs * SizeX;
		fp32 CurY = y + Desc.m_yOfs * SizeY;
		_Pos.Combine(_Dir, CurX, Pos);
		Pos.Combine(_VDown, CurY, Pos);


		if (CurX+ws < _MinLimit.k[0]) goto Cont;
		if (CurX > _MaxLimit.k[0]) goto Cont;
		if (CurY+hs < _MinLimit.k[1]) goto Cont;
		if (CurY > _MaxLimit.k[1]) goto Cont;

		{
			bool bXMinClip = (CurX < _MinLimit.k[0]);
			bool bXMaxClip = (CurX+ws > _MaxLimit.k[0]);
			bool bYMinClip = (CurY < _MinLimit.k[1]);
			bool bYMaxClip = (CurY+hs > _MaxLimit.k[1]);

			bool bClip = bXMinClip | bXMaxClip | bYMinClip | bYMaxClip;

			{
				if (!bClip)
				{
					_pV[nV + 0] = Pos;
					Pos.Combine(_Dir, ws, _pV[nV + 1]);
					Pos.Combine(_Dir, ws, _pV[nV + 2]); _pV[nV + 2].Combine(_VDown, hs, _pV[nV + 2]);
					Pos.Combine(_VDown, hs, _pV[nV + 3]);

	/*				float AddX = M_Fabs(0.5f * ws);
					float AddY = M_Fabs(0.5f * hs);

					_pV[nV + 0].k[0] += - AddX; 
					_pV[nV + 3].k[0] += - AddX;
					_pV[nV + 0].k[1] += - AddY;
					_pV[nV + 1].k[1] += - AddY; 

					_pV[nV + 1].k[0] += + AddX; 
					_pV[nV + 2].k[0] += + AddX;
					_pV[nV + 2].k[1] += + AddY;
					_pV[nV + 3].k[1] += + AddY;*/


				/*	AddX = 0.0* (0.5f * ((Desc.m_TVec0.k[0] - Desc.m_TVec1.k[0]) / Desc.m_Dimensions.k[0]));
					AddY = 0.0* (0.5f * ((Desc.m_TVec0.k[1] - Desc.m_TVec1.k[1]) / Desc.m_Dimensions.k[1]));
*/
					_pTV[nV + 0].k[0] = Desc.m_TVec0.k[0];// - AddX; 
					_pTV[nV + 3].k[0] = Desc.m_TVec0.k[0];// - AddX; 
					_pTV[nV + 0].k[1] = Desc.m_TVec0.k[1];// - AddY;
					_pTV[nV + 1].k[1] = Desc.m_TVec0.k[1];// - AddY;

					_pTV[nV + 1].k[0] = Desc.m_TVec1.k[0];// + AddX; 
					_pTV[nV + 2].k[0] = Desc.m_TVec1.k[0];// + AddX; 
					_pTV[nV + 2].k[1] = Desc.m_TVec1.k[1];// + AddY;
					_pTV[nV + 3].k[1] = Desc.m_TVec1.k[1];// + AddY;

					_pCol[nV + 0] = CurColor;
					_pCol[nV + 1] = CurColor;
					_pCol[nV + 2] = CurColor;
					_pCol[nV + 3] = CurColor;
				}
				else
				{
					fp32 tx0 = (bXMinClip) ? (_MinLimit.k[0] - CurX) : 0;
					fp32 tx1 = (bXMaxClip) ? (_MaxLimit.k[0] - CurX) : ws;
					fp32 ty0 = (bYMinClip) ? (_MinLimit.k[1] - CurY) : 0;
					fp32 ty1 = (bYMaxClip) ? (_MaxLimit.k[1] - CurY) : hs;

					Pos.Combine(_Dir, tx0, _pV[nV + 0]); _pV[nV + 0].Combine(_VDown, ty0, _pV[nV + 0]);
					Pos.Combine(_Dir, tx1, _pV[nV + 1]); _pV[nV + 1].Combine(_VDown, ty0, _pV[nV + 1]);
					Pos.Combine(_Dir, tx1, _pV[nV + 2]); _pV[nV + 2].Combine(_VDown, ty1, _pV[nV + 2]);
					Pos.Combine(_Dir, tx0, _pV[nV + 3]); _pV[nV + 3].Combine(_VDown, ty1, _pV[nV + 3]);

					CVec2Dfp32 dTV(m_TexturePixelUV[0] * SizeXRcp, m_TexturePixelUV[1] * SizeYRcp);

/*					CVec2Dfp32 dTV(Desc.m_TVec1.k[0] - Desc.m_TVec0.k[0], Desc.m_TVec1.k[1] - Desc.m_TVec0.k[1]);
					fp32 wsrcp = 1.0f / ws;
					fp32 hsrcp = 1.0f / hs;
					tx0 *= wsrcp;
					tx1 *= wsrcp;
					ty0 *= hsrcp;
					ty1 *= hsrcp;*/
					_pTV[nV + 0][0] = Desc.m_TVec0[0] + dTV[0] * tx0;		_pTV[nV + 0][1] = Desc.m_TVec0[1] + dTV[1] * ty0;
					_pTV[nV + 1][0] = Desc.m_TVec0[0] + dTV[0] * tx1;		_pTV[nV + 1][1] = Desc.m_TVec0[1] + dTV[1] * ty0;
					_pTV[nV + 2][0] = Desc.m_TVec0[0] + dTV[0] * tx1;		_pTV[nV + 2][1] = Desc.m_TVec0[1] + dTV[1] * ty1;
					_pTV[nV + 3][0] = Desc.m_TVec0[0] + dTV[0] * tx0;		_pTV[nV + 3][1] = Desc.m_TVec0[1] + dTV[1] * ty1;

					_pCol[nV + 0] = CurColor;
					_pCol[nV + 1] = CurColor;
					_pCol[nV + 2] = CurColor;
					_pCol[nV + 3] = CurColor;
				}


				nV += 4;
			}
		}
Cont:
		fp32 dx = fp32(Desc.m_Spacing) * SizeX;
		x += dx;
		CurX += dx;
		Pos.Combine(_Dir, dx, Pos);

		if (CurX > _MaxLimit.k[0]) break;
	}

	// Build triangle-list
	if (_piPrim)
	{
		int iV = 0;
		int nChars = nV >> 2;
		while(nChars)
		{
			_piPrim[nP + 0] = iV;
			_piPrim[nP + 1] = iV+1;
			_piPrim[nP + 2] = iV+3;

			_piPrim[nP + 3] = iV+3;
			_piPrim[nP + 4] = iV+1;
			_piPrim[nP + 5] = iV+2;
			nP += 6;
			iV += 4;
			nChars--;
		}
	}

	return nV / 2;	// Num triangles
}

#else

//M_FORCEINLINE void M_VStR(vec128 _a, void *__restrict _pDest) { __stvx(_a, _pDest, 0); }

int CRC_Font::Write(int _MaxV, CVec3Dfp32* _pV, CVec2Dfp32* _pTV, CPixel32* _pCol, uint16* _piPrim, 
					CPixel32 _Color, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const wchar* _pStr, 
					const CVec2Dfp32& _Size, const CVec2Dfp32& _MinLimit, const CVec2Dfp32& _MaxLimit)
{
	MAUTOSTRIP(CRC_Font_Write, 0);
	// _piPrim must have space for _MaxV*3/2 indices.

	uint Len = CStrBase::StrLen(_pStr);
	if (!Len) return 0;
	if (Len*4 > _MaxV) return 0;

	vec128 SizeParam = M_VLd(_Size[0], _Size[1], _Size[0], _Size[1]);
	vec128 SizeParamRcp = M_VRcp(SizeParam);
	vec128 OriginalSize = M_VLdScalar(m_OriginalSize);
	vec128 OriginalSizeRcp = M_VLdScalar(m_OriginalSizeRcp);
	vec128 Size = M_VMul(SizeParam, OriginalSizeRcp);
	vec128 SizeRcp = M_VMul(SizeParamRcp, OriginalSize);
	vec128 MaxLimit = M_VLd(_MaxLimit[0], _MaxLimit[1], _MaxLimit[0], _MaxLimit[1]);
	vec128 MinLimit = M_VLd(_MinLimit[0], _MinLimit[1], _MinLimit[0], _MinLimit[1]);
	vec128 PosParam = M_VLd_P3_Slow(&_Pos);
	vec128 VRight = M_VLd_V3_Slow(&_Dir);
	vec128 VDown = M_VLd_V3_Slow(&_VDown);
	vec128 TexturePixelUV = M_VLd(m_TexturePixelUV[0], m_TexturePixelUV[1], m_TexturePixelUV[0], m_TexturePixelUV[1]);

/*	fp32 InvOriginalSize = m_OriginalSizeRcp;
	CVec3Dfp32 p(_Pos);
	fp32 SizeX = _Size.k[0] * InvOriginalSize;
	fp32 SizeY = _Size.k[1] * InvOriginalSize;
//	CVec2Dfp32 SizeRcp = CVec2Dfp32(1.0f / _Size[0], 1.0f / _Size[1]);
	fp32 SizeXRcp = m_OriginalSize * SizeRcp[0];
	fp32 SizeYRcp = m_OriginalSize * SizeRcp[1];
*/
	//	uint32 IndexRamp[4] = { 0, 1, 2, 3 };

	uint nP = 0;
	uint nV = 0;

//	CVec3Dfp32 Pos(_Pos);

	vec128 Pos2D = M_VZero();

	vec128 VCurColor = M_VLdScalar_u32(_Color);
	CPixel32 CurColor = _Color;

//	fp32 x = 0;
//	fp32 y = 0;
	for(uint i = 0; i < Len; i++)
	{
		uint CodeLen = IsControlCode(_pStr, i);
		if(CodeLen > 0)
		{
			switch(_pStr[i + 1])
			{
			case 'a' :
			case 'A' :
				{
					int v = (uint8)_pStr[i + 2] - '0';
					if(v > 9) v -= 'a' - '0' - 10;
					int A = v * 255 / 15;
					CurColor = CPixel32(CurColor.GetR(), CurColor.GetG(), CurColor.GetB(), A);
					VCurColor = M_VLdScalar_u32(CurColor);
					break;
				}
			case 'c' :
				{
					// Multiply
					int v = (uint8)_pStr[i + 2] - '0'; if(v > 9) v += -'a' + '0' + 10; int R = v * 255 / 15;
					v = (uint8)_pStr[i + 3] - '0'; if(v > 9) v += -'a' + '0' + 10; int G = v * 255 / 15;
					v = (uint8)_pStr[i + 4] - '0'; if(v > 9) v += -'a' + '0' + 10; int B = v * 255 / 15;

					CurColor = _Color;
					CurColor *= CPixel32(R, G, B, 255);
					VCurColor = M_VLdScalar_u32(CurColor);
					break;
				}
			case 'C' :
				{
					// Absolute
					int v = (uint8)_pStr[i + 2] - '0'; if(v > 9) v += -'a' + '0' + 10; int R = v * 255 / 15;
					v = (uint8)_pStr[i + 3] - '0'; if(v > 9) v += -'a' + '0' + 10; int G = v * 255 / 15;
					v = (uint8)_pStr[i + 4] - '0'; if(v > 9) v += -'a' + '0' + 10; int B = v * 255 / 15;
					CurColor = CPixel32(R, G, B, CurColor.GetA());
					VCurColor = M_VLdScalar_u32(CurColor);
					break;
				}
			case 'd' :
			case 'D' :
				{
					CurColor = _Color;
					VCurColor = M_VLdScalar_u32(CurColor);
					break;
				}
			case 'n' :
			case 'N' :
				{
					uint32 dy = ((uint8)_pStr[i + 2] - '0') * 100;
					dy += ((uint8)_pStr[i + 3] - '0') * 10;
					dy += ((uint8)_pStr[i + 4] - '0') * 1;

					vec128 vdy = M_VCnv_i32_f32(M_VLdScalar_u32(dy));
					Pos2D = M_VAnd(M_VAdd(Pos2D, vdy), M_VConstMsk(0,1,0,1));

//					x = 0;
//					y += dy;
					break;
				}
			case 't' :
			case 'T' :
				{
					int dy = ((uint8)_pStr[i + 2] - '0') * 100;
					dy += ((uint8)_pStr[i + 3] - '0') * 10;
					dy += ((uint8)_pStr[i + 4] - '0') * 1;

					vec128 vdy = M_VCnv_i32_f32(M_VLdScalar_u32(dy));
					Pos2D = M_VSelMsk(M_VConstMsk(1,0,1,0), vdy, Pos2D);
//					x = dy;
					break;
				}
			case 'x' :
			case 'X' :
				{
					int dx = ((uint8)_pStr[i + 2] - '0') * 100;
					dx += ((uint8)_pStr[i + 3] - '0') * 10;
					dx += ((uint8)_pStr[i + 4] - '0') * 1;
//					x += dx;
					vec128 vdx = M_VCnv_i32_f32(M_VLdScalar_u32(dx));
					Pos2D = M_VAdd(Pos2D, M_VAnd(M_VConstMsk(1,0,1,0), vdx));
					//					_Pos.Combine(_Dir, x, Pos);
					//					Pos.Combine(_VDown, y, Pos);
					break;
				}
			case 'y' :
			case 'Y' :
				{
					int dy = ((uint8)_pStr[i + 2] - '0') * 100;
					dy += ((uint8)_pStr[i + 3] - '0') * 10;
					dy += ((uint8)_pStr[i + 4] - '0') * 1;
//					y += dy;
					vec128 vdy = M_VCnv_i32_f32(M_VLdScalar_u32(dy));
					Pos2D = M_VAdd(Pos2D, M_VAnd(M_VConstMsk(0,1,0,1), vdy));
					//					_Pos.Combine(_Dir, x, Pos);
					//					Pos.Combine(_VDown, y, Pos);
					break;
				}
			case 'z' :
				{
					// Relative
/*					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 1.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 0.1f;
					vec128 vz = M_VLdScalar(z);*/
					vec128 chars = M_VCnv_i32_f32(M_VCnvL_u16_u32(M_VSub_u16(M_VMrgL_u16(M_VLdScalar_u16((uint16&)_pStr[i + 2]), M_VLdScalar_u16((uint16&)_pStr[i + 3])), M_VScalar_u16('0'))));
					vec128 tmp = M_VMul(chars, M_VConst(1.0f, 0.1f, 0.0f, 0.0f));
					vec128 vz = M_VAdd(M_VSplatX(tmp), M_VSplatY(tmp));

					vec128 vzrcp = M_VRcp(vz);
					Size = M_VMul(M_VMul(vz, SizeParam), OriginalSizeRcp);
					SizeRcp = M_VMul(M_VMul(vzrcp, SizeParamRcp), OriginalSize);
/*					fp32 zrcp = 1.0f / z;
					SizeX = z * _Size.k[0] * InvOriginalSize;
					SizeY = z * _Size.k[1] * InvOriginalSize;
					SizeXRcp = m_OriginalSize * SizeRcp[0] * zrcp;
					SizeYRcp = m_OriginalSize * SizeRcp[1] * zrcp;*/
					break;
				}
			case 'Z' :
				{
					// Absolute
/*					fp32 z = fp32((uint8)_pStr[i + 2] - '0') * 10.0f;
					z += fp32((uint8)_pStr[i + 3] - '0') * 1.0f;
					vec128 vz = M_VLdScalar(z);*/

					vec128 chars = M_VCnv_i32_f32(M_VCnvL_u16_u32(M_VSub_u16(M_VMrgL_u16(M_VLdScalar_u16((uint16&)_pStr[i + 2]), M_VLdScalar_u16((uint16&)_pStr[i + 3])), M_VScalar_u16('0'))));
					vec128 tmp = M_VMul(chars, M_VConst(10.0f, 1.0f, 0.0f, 0.0f));
					vec128 vz = M_VAdd(M_VSplatX(tmp), M_VSplatY(tmp));

					vec128 vzrcp = M_VRcp(vz);
					Size = M_VMul(vz, OriginalSizeRcp);
					SizeRcp = M_VMul(vzrcp, OriginalSize);
/*					fp32 zrcp = 1.0f / z;
					SizeX = z * InvOriginalSize;
					SizeY = z * InvOriginalSize;
					SizeXRcp = m_OriginalSize * zrcp;
					SizeYRcp = m_OriginalSize * zrcp;*/
					break;
				}
			}

			i += CodeLen - 1;
			continue;
		}

		CRC_FontChar& Desc = GetCharDesc(_pStr[i]);
		if (Desc.m_iLocal < 0) continue;

/*		fp32 ws = Desc.m_Dimensions.k[0] * SizeX;
		fp32 hs = Desc.m_Dimensions.k[1] * SizeY;
		fp32 CurX = x + Desc.m_xOfs * SizeX;
		fp32 CurY = y + Desc.m_yOfs * SizeY;
		_Pos.Combine(_Dir, CurX, Pos);
		Pos.Combine(_VDown, CurY, Pos);
*/
		vec128 CharSize = M_VMul(M_VLd64(&Desc.m_Dimensions), Size);
		vec128 Descofs = M_VCnv_i32_f32(M_VShuf(M_VCnvL_i16_i32(M_VLdScalar_u32(*((uint32*)&Desc.m_xOfs))), M_VSHUF(0,1,0,1)) );
		vec128 Cur = M_VMAdd(Descofs, Size, Pos2D);

		vec128 zero = M_VZero();
/*		vec128 Cmp = M_VOr(M_VCmpGEMsk(Cur, MaxLimit), M_VCmpLEMsk(M_VAdd(Cur, CharSize), MinLimit));
		if (M_VCmpAnyGT_u32(Cmp, zero))
			goto Cont;
*/
//		if (M_VCmpAnyGE(Cur, MaxLimit)) goto Cont;
//		if (M_VCmpAnyLE(M_VAdd(Cur, CharSize), MinLimit)) goto Cont;

		{
			vec128 t0 = M_VMin(M_VMax(M_VSub(MinLimit, Cur), zero), CharSize);
			vec128 t1 = M_VMin(M_VMax(M_VSub(MaxLimit, Cur), zero), CharSize);

	//		vec128 Pos = M_VAdd(PosParam, M_VAdd(M_VMul(M_VSplatX(Cur), VRight), M_VMul(M_VSplatY(Cur), VDown)));
			vec128 Pos = M_VMAdd(M_VSplatX(Cur), VRight, M_VMAdd(M_VSplatY(Cur), VDown, PosParam));

			vec128 v0 = M_VMAdd(VDown, M_VSplatY(t0), M_VMAdd(VRight, M_VSplatX(t0), Pos));
			vec128 v1 = M_VMAdd(VDown, M_VSplatY(t0), M_VMAdd(VRight, M_VSplatX(t1), Pos));
			vec128 v2 = M_VMAdd(VDown, M_VSplatY(t1), M_VMAdd(VRight, M_VSplatX(t1), Pos));
			vec128 v3 = M_VMAdd(VDown, M_VSplatY(t1), M_VMAdd(VRight, M_VSplatX(t0), Pos));

			vec128 dTV = M_VMul(TexturePixelUV, SizeRcp);

			vec128 txy01 = M_VMrgXY(t0, t1);		// = tx0, tx1, ty0, ty1
			vec128 tvec0 = M_VLd64(&Desc.m_TVec0);
			vec128 tv01 = M_VMAdd(dTV, M_VShuf(txy01, M_VSHUF(0, 2, 1, 2)), tvec0);
			vec128 tv23 = M_VMAdd(dTV, M_VShuf(txy01, M_VSHUF(1, 3, 0, 3)), tvec0);

			CVec3Dfp32 *M_RESTRICT pV = _pV+nV;
			CVec2Dfp32 *M_RESTRICT pTV = _pTV+nV;
			CPixel32 *M_RESTRICT pCol = _pCol+nV;
			M_VSt_V3x4(pV, v0, v1, v2, v3);
			M_VSt(tv01, pTV);
			M_VSt(tv23, pTV + 2);
			M_VSt(VCurColor, pCol);
/*			pCol[0] = CurColor;
			pCol[1] = CurColor;
			pCol[2] = CurColor;
			pCol[3] = CurColor;
*/
/*			M_VSt_V3x4(_pV+nV, v0, v1, v2, v3);
			M_VSt(tv01, _pTV + nV);
			M_VSt(tv23, _pTV + nV + 2);

			_pCol[nV + 0] = CurColor;
			_pCol[nV + 1] = CurColor;
			_pCol[nV + 2] = CurColor;
			_pCol[nV + 3] = CurColor;*/
		}

		nV += 4;
//Cont:
		vec128 dx = M_VAnd(M_VConstMsk(1,0,1,0), M_VMul(M_VCnv_i32_f32(M_VCnvL_i16_i32(M_VLdScalar_i16(Desc.m_Spacing))), Size));
		Pos2D = M_VAdd(dx, Pos2D);
		
//		fp32 dx = fp32(Desc.m_Spacing) * SizeX;
//		x += dx;
//		CurX += dx;
//		Pos.Combine(_Dir, dx, Pos);

//		if (CurX > _MaxLimit.k[0]) break;
	}

	// Build triangle-list
	if (_piPrim)
	{
		int iV = 0;
		int nChars = nV >> 2;
		while(nChars)
		{
			_piPrim[nP + 0] = iV;
			_piPrim[nP + 1] = iV+1;
			_piPrim[nP + 2] = iV+3;

			_piPrim[nP + 3] = iV+3;
			_piPrim[nP + 4] = iV+1;
			_piPrim[nP + 5] = iV+2;
			nP += 6;
			iV += 4;
			nChars--;
		}
	}

	return nV / 2;	// Num triangles
}

#endif

int CRC_Font::Write(int _MaxV, CVec3Dfp32* _pV, CVec2Dfp32* _pTV, CPixel32* _pCol, uint16* _piPrim, 
		CPixel32 _Color, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const char* _pStr, 
		const CVec2Dfp32& _Size, const CVec2Dfp32& _MinLimit, const CVec2Dfp32& _MaxLimit)
{
	MAUTOSTRIP(CRC_Font_Write_2, 0);
	wchar Buffer[CRC_MAXSTRFORMAT];
	mint Len = MinMT(CRC_MAXSTRFORMAT-1, CStrBase::StrLen(_pStr));
	CStrBase::mfsncpy(Buffer, CSTR_FMT_UNICODE, _pStr, CSTR_FMT_ANSI, Len);
	Buffer[Len] = 0;

	return Write(_MaxV, _pV, _pTV, _pCol, _piPrim, _Color, _Pos, _Dir, _VDown, Buffer, _Size, _MinLimit, _MaxLimit);
}

void CRC_Font::Write(CRenderContext* _pRC, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const wchar* _pStr, 
		const CVec2Dfp32& _Size, CPixel32 _Color, const CVec2Dfp32& _MinLimit, const CVec2Dfp32& _MaxLimit)
{
	MAUTOSTRIP(CRC_Font_Write_3, MAUTOSTRIP_VOID);
	int len = CStr::StrLen(_pStr);
	if (len < 1) return;

	int nV = len*4;
	int nP = len*6;

	const int MaxChar = 256;
	const int MaxV = MaxChar*4;
	const int MaxP = MaxChar*6;

	if (nV > MaxV) return;
	if (nP > MaxP) return;

	static	CVec3Dfp32 Verts[MaxV];
	static	CVec2Dfp32 TVerts[MaxV];
	static	CPixel32 Colors[MaxV];
	static	uint16 Prim[MaxP];

	int nTri = Write(nV, Verts, TVerts, Colors, Prim, _Color, _Pos, _Dir, _VDown, _pStr, _Size, _MinLimit, _MaxLimit);
	if (!nTri) return;

	_pRC->Attrib_Push();

		if (m_TextureID)
			_pRC->Attrib_TextureID(0, m_TextureID);
		else
			_pRC->Attrib_TextureID(0, m_spTC->GetTextureID(0));
		_pRC->Geometry_Clear();
		_pRC->Geometry_VertexArray(Verts, nTri * 2, true);
		_pRC->Geometry_TVertexArray(TVerts, 0);
		_pRC->Geometry_ColorArray(Colors);

		_pRC->Render_IndexedTriangles(Prim, nTri);

		_pRC->Geometry_Clear();

	_pRC->Attrib_Pop();
}

bool CRC_Font::Write(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const wchar* _pStr, 
		const CVec2Dfp32& _Size, CPixel32 _Color, const CVec2Dfp32& _MinLimit, const CVec2Dfp32& _MaxLimit)
{
	MAUTOSTRIP(CRC_Font_Write_4, false);
	int len = CStr::StrLen(_pStr);
	if (len < 1) return false;

	int nV = len*4;
	int nP = len*6;

	CVec3Dfp32* pV = _pVBM->Alloc_V3(nV);
	CVec2Dfp32* pTV = _pVBM->Alloc_V2(nV);
	CPixel32* pCol = _pVBM->Alloc_CPixel32(nV);

	if (!pV || !pTV || !pCol) return false;
	int nTri = Write(nV, pV, pTV, pCol, NULL, _Color, _Pos, _Dir, _VDown, _pStr, _Size, _MinLimit, _MaxLimit);
	if (!nTri) return false;

	if (!_pVB->AllocVBChain(_pVBM, false))
		return false;
	if (nTri > CXR_Util::MAXPARTICLES*2)
		return false;
	_pVB->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, nTri);
//	_pVB->Render_IndexedTriangles(pPrim, nTri);
	_pVB->Geometry_VertexArray(pV, nTri * 2, true);
	_pVB->Geometry_TVertexArray(pTV, 0);
	_pVB->Geometry_ColorArray(pCol);

	if (_pVB->m_pAttrib)
	{
		if (m_TextureID)
			_pVB->m_pAttrib->Attrib_TextureID(0, m_TextureID);
		else
			_pVB->m_pAttrib->Attrib_TextureID(0, m_spTC->GetTextureID(0));
	}

	return true;
}

void CRC_Font::Write(CRenderContext* _pRC, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const char* _pStr, 
		const CVec2Dfp32& _Size, CPixel32 _Color, const CVec2Dfp32& _MinLimit, const CVec2Dfp32& _MaxLimit)
{
	MAUTOSTRIP(CRC_Font_Write_5, MAUTOSTRIP_VOID);
	wchar Buffer[CRC_MAXSTRFORMAT];
	mint Len = MinMT(CRC_MAXSTRFORMAT-1, CStrBase::StrLen(_pStr));
	CStrBase::mfsncpy(Buffer, CSTR_FMT_UNICODE, _pStr, CSTR_FMT_ANSI, Len);
	Buffer[Len] = 0;

	Write(_pRC, _Pos, _Dir, _VDown, Buffer, _Size, _Color, _MinLimit, _MaxLimit);
}

bool CRC_Font::Write(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const char* _pStr, 
		const CVec2Dfp32& _Size, CPixel32 _Color, const CVec2Dfp32& _MinLimit, const CVec2Dfp32& _MaxLimit)
{
	MAUTOSTRIP(CRC_Font_Write_6, false);
	wchar Buffer[CRC_MAXSTRFORMAT];
	mint Len = MinMT(CRC_MAXSTRFORMAT-1, CStrBase::StrLen(_pStr));
	CStrBase::mfsncpy(Buffer, CSTR_FMT_UNICODE, _pStr, CSTR_FMT_ANSI, Len);
	Buffer[Len] = 0;

	return Write(_pVBM, _pVB, _Pos, _Dir, _VDown, Buffer, _Size, _Color, _MinLimit, _MaxLimit);
}

void CRC_Font::Write(CRenderContext* _pRC, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const CStr& _Str, 
		const CVec2Dfp32& _Size, CPixel32 _Color, const CVec2Dfp32& _MinLimit, const CVec2Dfp32& _MaxLimit)
{
	MAUTOSTRIP(CRC_Font_Write_7, MAUTOSTRIP_VOID);
	if (_Str.IsUnicode())
		Write(_pRC, _Pos, _Dir, _VDown, _Str.StrW(), _Size, _Color, _MinLimit, _MaxLimit);
	else
		Write(_pRC, _Pos, _Dir, _VDown, _Str.Str(), _Size, _Color, _MinLimit, _MaxLimit);
}

bool CRC_Font::Write(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const CStr& _Str, 
		const CVec2Dfp32& _Size, CPixel32 _Color, const CVec2Dfp32& _MinLimit, const CVec2Dfp32& _MaxLimit)
{
	MAUTOSTRIP(CRC_Font_Write_8, false);
	if (_Str.IsUnicode())
		return Write(_pVBM, _pVB, _Pos, _Dir, _VDown, _Str.StrW(), _Size, _Color, _MinLimit, _MaxLimit);
	else
		return Write(_pVBM, _pVB, _Pos, _Dir, _VDown, _Str.Str(), _Size, _Color, _MinLimit, _MaxLimit);
}

