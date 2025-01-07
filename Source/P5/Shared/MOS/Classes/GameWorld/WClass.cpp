#include "PCH.h"
#include "Client/WClient.h"

// #define ENABLE_CMDQUEUEWARNINGS

// -------------------------------------------------------------------
//  CNetMsg
// -------------------------------------------------------------------
CNetMsg::CNetMsg()
{
	MAUTOSTRIP(CNetMsg_ctor, MAUTOSTRIP_VOID);
	m_MsgType = 0;
	m_MsgSize = 0;
}

CNetMsg::CNetMsg(uint8 _MsgType)
{
	MAUTOSTRIP(CNetMsg_ctor_2, MAUTOSTRIP_VOID);
	m_MsgType = _MsgType;
	m_MsgSize = 0;
}

void CNetMsg::operator= (const CNetMsg& _Msg)
{
	MAUTOSTRIP(CNetMsg_operator_assign, MAUTOSTRIP_VOID);
	m_MsgType = _Msg.m_MsgType;
	m_MsgSize = _Msg.m_MsgSize;
	memcpy(m_Data, _Msg.m_Data, m_MsgSize);
}

// -----------------------------------
void CNetMsg::SetInt8(int8 _Val, int _Pos)
{
	MAUTOSTRIP(CNetMsg_SetInt8, MAUTOSTRIP_VOID);
	m_Data[_Pos] = _Val;
}

void CNetMsg::SetInt16(int16 _Val, int _Pos)
{
	MAUTOSTRIP(CNetMsg_SetInt16, MAUTOSTRIP_VOID);
	m_Data[_Pos] = _Val;
	m_Data[_Pos+1] = _Val >> 8;
}

void CNetMsg::SetInt32(int32 _Val, int _Pos)
{
	MAUTOSTRIP(CNetMsg_SetInt32, MAUTOSTRIP_VOID);
	m_Data[_Pos] = _Val;
	m_Data[_Pos+1] = _Val >> 8;
	m_Data[_Pos+2] = _Val >> 16;
	m_Data[_Pos+3] = _Val >> 24;
}

void CNetMsg::Setfp32(fp32 _Val, int _Pos)
{
	MAUTOSTRIP(CNetMsg_Setfp32, MAUTOSTRIP_VOID);
	uint8* pData = &m_Data[_Pos];
	PTR_PUTFP32(pData, _Val);
}

// -----------------------------------
bool CNetMsg::AddInt8(int8 _Val)
{
	MAUTOSTRIP(CNetMsg_AddInt8, false);
	if (1 + m_MsgSize > WPACKET_MAXPACKETSIZE) return false;
	m_Data[m_MsgSize++] = _Val;
	return true;
}

bool CNetMsg::AddInt16(int16 _Val)
{
	MAUTOSTRIP(CNetMsg_AddInt16, false);
	if (2 + m_MsgSize > WPACKET_MAXPACKETSIZE) return false;
	m_Data[m_MsgSize++] = _Val;
	m_Data[m_MsgSize++] = _Val >> 8;
	return true;
}

bool CNetMsg::AddInt32(int32 _Val)
{
	MAUTOSTRIP(CNetMsg_AddInt32, false);
	if (4 + m_MsgSize > WPACKET_MAXPACKETSIZE) return false;
	m_Data[m_MsgSize++] = _Val;
	m_Data[m_MsgSize++] = _Val >> 8;
	m_Data[m_MsgSize++] = _Val >> 16;
	m_Data[m_MsgSize++] = _Val >> 24;
	return true;
}

bool CNetMsg::Addfp32(fp32 _Val)
{
	MAUTOSTRIP(CNetMsg_Addfp32, false);
	if (4 + m_MsgSize > WPACKET_MAXPACKETSIZE) return false;
	uint8* pData = &m_Data[(m_MsgSize+=4)-4];
	PTR_PUTFP32(pData, _Val);
	return true;
}

bool CNetMsg::AddStr(CStr _Val)
{
	MAUTOSTRIP(CNetMsg_AddStr, false);
	
	if (!_Val.IsAnsi())
		Error_static("CNetMsg::AddStr", "String not ANSI.");

	int len = _Val.Len();
	if ((len > 255) || (len + 1 + m_MsgSize > WPACKET_MAXPACKETSIZE))
	{
		m_Data[m_MsgSize++] = 0;
		return false;
	}
	m_Data[m_MsgSize++] = len;
	memcpy(&m_Data[m_MsgSize], (const char*) _Val, len);
	m_MsgSize += len;
	return true;
}

bool CNetMsg::AddStrAny(CStr _Val)
{
	m_Data[m_MsgSize++] = _Val.IsAnsi();
	if(_Val.IsAnsi())
		return AddStr(_Val);
	else
		return AddStrW(_Val);
}

bool CNetMsg::AddStr(const char* _pStr)
{
	MAUTOSTRIP(CNetMsg_AddStr_2, false);
	int len = CStr::StrLen(_pStr);
	if ((len > 255) || (len + 1 + m_MsgSize > WPACKET_MAXPACKETSIZE))
	{
		m_Data[m_MsgSize++] = 0;
		return false;
	}
	m_Data[m_MsgSize++] = len;
	memcpy(&m_Data[m_MsgSize], _pStr, len);
	m_MsgSize += len;
	return true;
}

bool CNetMsg::AddStrW(CStr _Val)
{
	if (!_Val.IsUnicode())
		Error_static("CNetMsg::AddStr", "String not Unicode.");

	uint len = _Val.Len();
	if ((len > 255) || (2*(len + 1) + m_MsgSize > WPACKET_MAXPACKETSIZE)) 
	{
		m_Data[m_MsgSize++] = 0;
		return false;
	}
	m_Data[m_MsgSize++] = len;

	const wchar* pStr = _Val.StrW();
	M_ASSERT(sizeof(wchar) == sizeof(uint16), "wchar is not 16-bit!");
	for (uint i = 0; i < len; i++)
	{
		m_Data[m_MsgSize++] = pStr[i] & 255;
		m_Data[m_MsgSize++] = pStr[i] >> 8;
	}
	return true;
}

// -----------------------------------
int8 CNetMsg::GetInt8(int &_Pos) const
{
	MAUTOSTRIP(CNetMsg_GetInt8, 0);
	return m_Data[_Pos++];
}

int16 CNetMsg::GetInt16(int &_Pos) const
{
	MAUTOSTRIP(CNetMsg_GetInt16, 0);
	int16 Val = m_Data[_Pos] + (m_Data[_Pos+1] << 8);
	_Pos += 2;
	return Val;
}

int32 CNetMsg::GetInt32(int &_Pos) const
{
	MAUTOSTRIP(CNetMsg_GetInt32, 0);
	int32 Val = 
		(int)m_Data[_Pos] + ((int)m_Data[_Pos+1] << 8) + ((int)m_Data[_Pos+2] << 16) + ((int)m_Data[_Pos+3] << 24);
	_Pos += 4;
	return Val;
}

fp32 CNetMsg::Getfp32(int &_Pos) const
{
	MAUTOSTRIP(CNetMsg_Getfp32, 0.0f);
	const uint8* pData = &m_Data[(_Pos+=4)-4];
	fp32 Val;
	PTR_GETFP32(pData, Val); 
	return Val;
}

CStr CNetMsg::GetStrAny(int &_Pos) const
{
	if(m_Data[_Pos++])
		return GetStr(_Pos);
	else 
		return GetStrW(_Pos);
}

CStr CNetMsg::GetStr(int &_Pos) const
{
	MAUTOSTRIP(CNetMsg_GetStr, CStr());
	int len = m_Data[_Pos++];
	CStr s;
	s.Capture((const char*) &m_Data[_Pos], len);
	_Pos += len;
	return s;
}

CStr CNetMsg::GetStrW(int &_Pos) const
{
	int len = m_Data[_Pos++];
	CStr s;
	s.Capture((const wchar*) &m_Data[_Pos], len);
	_Pos += len * 2;

	M_ASSERT(sizeof(wchar) == sizeof(uint16), "wchar is not 16-bit!");
	SwitchArrayLE_uint16((uint16*)s.StrW(), len);

	return s;
}

void CNetMsg::GetStr(int &_Pos, char* _pDest) const
{
	MAUTOSTRIP(CNetMsg_GetStr_2, MAUTOSTRIP_VOID);
	int len = m_Data[_Pos++];
	memcpy(_pDest, &m_Data[_Pos], len);
	_pDest[len] = 0;
	_Pos += len;
}

// -----------------------------------
bool CNetMsg::AddVecInt8_Max128(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CNetMsg_AddVecInt8_Max128, false);
	// Caution: Works only for vectors with len < 255
	if (3 + m_MsgSize > WPACKET_MAXPACKETSIZE) return false;
	m_Data[m_MsgSize++] = uint8(_v.k[0]);
	m_Data[m_MsgSize++] = uint8(_v.k[1]);
	m_Data[m_MsgSize++] = uint8(_v.k[2]);
	return true;
}

CVec3Dfp32 CNetMsg::GetVecInt8_Max128(int &_Pos) const
{
	MAUTOSTRIP(CNetMsg_GetVecInt8_Max128, CVec3Dfp32());
	CVec3Dfp32 v;
	v.k[0] = int8(m_Data[_Pos++]);
	v.k[1] = int8(m_Data[_Pos++]);
	v.k[2] = int8(m_Data[_Pos++]);
	return v;
}

// -----------------------------------
bool CNetMsg::AddVecInt8_DynPrecision(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CNetMsg_AddVecInt8_DynPrecision, false);
	// Caution: Works only for vectors with len < 255
	if (4 + m_MsgSize > WPACKET_MAXPACKETSIZE) return false;
	int Len = int(Max(1.0f, _v.Length()));
	m_Data[m_MsgSize++] = Len;
	m_Data[m_MsgSize++] = uint8(_v.k[0]*127.0f / Len);
	m_Data[m_MsgSize++] = uint8(_v.k[1]*127.0f / Len);
	m_Data[m_MsgSize++] = uint8(_v.k[2]*127.0f / Len);
	return true;
}

CVec3Dfp32 CNetMsg::GetVecInt8_DynPrecision(int &_Pos) const
{
	MAUTOSTRIP(CNetMsg_GetVecInt8_DynPrecision, CVec3Dfp32());
	CVec3Dfp32 v;
	fp32 Len = fp32(m_Data[_Pos++]) / 127.0f;
	v.k[0] = fp32(int8(m_Data[_Pos++])) * Len;
	v.k[1] = fp32(int8(m_Data[_Pos++])) * Len;
	v.k[2] = fp32(int8(m_Data[_Pos++])) * Len;
	return v;
}

// -----------------------------------
bool CNetMsg::AddVecInt16_Max32768(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CNetMsg_AddVecInt16_Max32768, false);
	// Caution: Works only for vectors with len < 32768
	if (6 + m_MsgSize > WPACKET_MAXPACKETSIZE) return false;
	AddInt16(int(_v.k[0]));
	AddInt16(int(_v.k[1]));
	AddInt16(int(_v.k[2]));
	return true;
}

CVec3Dfp32 CNetMsg::GetVecInt16_Max32768(int &_Pos) const
{
	MAUTOSTRIP(CNetMsg_GetVecInt16_Max32768, CVec3Dfp32());
	CVec3Dfp32 v;
	v.k[0] = GetInt16(_Pos);
	v.k[1] = GetInt16(_Pos);
	v.k[2] = GetInt16(_Pos);
	return v;
}

// -----------------------------------
bool CNetMsg::AddVecInt16_DynPrecision(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CNetMsg_AddVecInt16_DynPrecision, false);
	// Caution: Works only for vectors with len < 64k
	if (8 + m_MsgSize > WPACKET_MAXPACKETSIZE) return false;
	int Len = int(Max(1.0f, _v.Length()));
	AddInt16(Len);
	AddInt16(int(_v.k[0]*32767.0f / Len));
	AddInt16(int(_v.k[1]*32767.0f / Len));
	AddInt16(int(_v.k[2]*32767.0f / Len));
	return true;
}

CVec3Dfp32 CNetMsg::GetVecInt16_DynPrecision(int &_Pos) const
{
	MAUTOSTRIP(CNetMsg_GetVecInt16_DynPrecision, CVec3Dfp32());
	CVec3Dfp32 v;
	fp32 Len = fp32(GetInt16(_Pos)) / 32767.0f;
	v.k[0] = fp32(GetInt16(_Pos)) * Len;
	v.k[1] = fp32(GetInt16(_Pos)) * Len;
	v.k[2] = fp32(GetInt16(_Pos)) * Len;
	return v;
}

// -----------------------------------
bool CNetMsg::AddData(const void* _pData, int _nBytes)
{
	MAUTOSTRIP(CNetMsg_AddData, false);
	if (_nBytes + m_MsgSize > WPACKET_MAXPACKETSIZE) return false;
	memcpy(&m_Data[m_MsgSize], _pData, _nBytes);
	m_MsgSize += _nBytes;
	return true;
}

bool CNetMsg::GetData(void* _pData, int _nBytes, int &_Pos) const
{
	MAUTOSTRIP(CNetMsg_GetData, false);
	if (_nBytes + _Pos > m_MsgSize) return false;
	memcpy(_pData, &m_Data[_Pos], _nBytes);
	_Pos += _nBytes;
	return true;
}

// -----------------------------------
void CNetMsg::Read(CCFile* _pF)
{
	MAUTOSTRIP(CNetMsg_Read, MAUTOSTRIP_VOID);
	_pF->ReadLE(m_MsgType);
	_pF->ReadLE(m_MsgSize);
	if (m_MsgSize > WPACKET_MAXPACKETSIZE)
		Error_static("CNetMsg::Read", CStrF("Invalid message size: %d", m_MsgSize));
	_pF->Read(m_Data, m_MsgSize);
}

void CNetMsg::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CNetMsg_Write, MAUTOSTRIP_VOID);
	_pF->WriteLE(m_MsgType);
	_pF->WriteLE(m_MsgSize);
	_pF->Write(m_Data, m_MsgSize);
}

// -------------------------------------------------------------------
//  CNetMsgPack
// -------------------------------------------------------------------
CNetMsgPack::CNetMsgPack()
{
	MAUTOSTRIP(CNetMsgPack_ctor, MAUTOSTRIP_VOID);
	m_Size = 0;
	m_ReadPtr = 0;
}

void CNetMsgPack::Clear()
{
	MAUTOSTRIP(CNetMsgPack_Clear, MAUTOSTRIP_VOID);
	m_Size = 0;
	m_ReadPtr = 0;
}

bool CNetMsgPack::AddMsg(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CNetMsgPack_AddMsg, false);
	if (_Msg.GetSize() + m_Size > NET_MAXPACKETSIZE) return false;
	memcpy(&m_MsgBuf[m_Size], &_Msg, _Msg.GetSize());
	m_Size += _Msg.GetSize();
	return true;
}

const CNetMsg* CNetMsgPack::GetMsg()
{
	MAUTOSTRIP(CNetMsgPack_GetMsg, NULL);
	if (m_ReadPtr >= m_Size) return NULL;
	CNetMsg* pMsg = (CNetMsg*) &m_MsgBuf[m_ReadPtr]; 
	m_ReadPtr += pMsg->GetSize();
	return pMsg;
}

