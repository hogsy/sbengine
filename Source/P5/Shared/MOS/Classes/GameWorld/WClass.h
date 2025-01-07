#ifndef __WCLASS_H
#define __WCLASS_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Misc network helpers

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios, 2003

	Contents:		CNetMsg
					CNetMsgPack
\*_____________________________________________________________________________________________*/

#include "../../MOS.h"
#include "../../XR/XR.h"

// -------------------------------------------------------------------

#if defined(CPU_LITTLEENDIAN) && defined(CPU_UNALIGNED)
// platform is little endian and can access unaligned words

#define PTR_PUTINT8(Ptr, Value) { *((int8*)Ptr) = (Value); Ptr++; }
#define PTR_PUTINT16(Ptr, Value) { *((int16*)Ptr) = (Value); Ptr += 2; }
#define PTR_PUTINT32(Ptr, Value) { *((int32*)Ptr) = (Value); Ptr += 4; }
#define PTR_PUTUINT8(Ptr, Value) { *((uint8*)Ptr) = (Value); Ptr++; }
#define PTR_PUTUINT16(Ptr, Value) { *((uint16*)Ptr) = (Value); Ptr += 2; }
#define PTR_PUTUINT32(Ptr, Value) { *((uint32*)Ptr) = (Value); Ptr += 4; }
#define PTR_PUTFP32(Ptr, Value) { *((fp32*)Ptr) = (Value); Ptr += 4; }
#define PTR_PUTSTR(Ptr, Value) { int Len = Value.Len(); PTR_PUTINT32(Ptr, Len); memcpy(Ptr, Value.Str(), Len); Ptr += Len; }
#define PTR_PUTDATA(Ptr, pData, Len) { memcpy(Ptr, pData, (Len)); Ptr += (Len); }
#define PTR_PUTCMTIME(Ptr, Value) { (Value).Pack(Ptr);}

// Non portable
#define PTR_PUTMINT(Ptr, Value) { *((mint*)Ptr) = (Value); Ptr += sizeof(mint); }

#define PTR_GETINT8(Ptr, Value) { Value = *((int8*)Ptr); Ptr++; }
#define PTR_GETINT16(Ptr, Value) { Value = *((int16*)Ptr); Ptr += 2; }
#define PTR_GETINT32(Ptr, Value) { Value = *((int32*)Ptr); Ptr += 4; }
#define PTR_GETUINT8(Ptr, Value) { Value = *((uint8*)Ptr); Ptr++; }
#define PTR_GETUINT16(Ptr, Value) { Value = *((uint16*)Ptr); Ptr += 2; }
#define PTR_GETUINT32(Ptr, Value) { Value = *((uint32*)Ptr); Ptr += 4; }
#define PTR_GETFP32(Ptr, Value) { Value = *((fp32*)Ptr); Ptr += 4; }
#define PTR_GETSTR(Ptr, Value) { int Len; PTR_GETINT32(Ptr, Len); Value.Capture((char*)Ptr, Len); Ptr += Len; }
#define PTR_GETDATA(Ptr, pData, Len) { memcpy(pData, Ptr, (Len)); Ptr += (Len); }
#define PTR_GETCMTIME(Ptr, Value) { (Value).Unpack(Ptr);}

// Non portable
#define PTR_GETMINT(Ptr, Value) { Value = *((mint*)Ptr); Ptr += sizeof(mint); }

#else
// endian-safe and alignment-safe

union NetUnion
{
	uint32	m_UW;
	fp32	m_F;
};

#define PTR_PUTINT8(Ptr, Value)   { ((uint8*)Ptr)[0] = (Value)&0xff; Ptr++; }
#define PTR_PUTINT16(Ptr, Value)  { ((uint8*)Ptr)[0] = (Value)&0xff;  ((uint8*)Ptr)[1] = 0xff&((Value) >> 8); Ptr += 2; }
#define PTR_PUTINT32(Ptr, Value)  { ((uint8*)Ptr)[0] = (Value)&0xff;  ((uint8*)Ptr)[1] = 0xff&((Value) >> 8);  ((uint8*)Ptr)[2] = 0xff&((Value) >> 16);  ((uint8*)Ptr)[3] = 0xff&((Value) >> 24); Ptr += 4; }
#define PTR_PUTUINT8(Ptr, Value)  { ((uint8*)Ptr)[0] = (Value)&0xff; Ptr++; }
#define PTR_PUTUINT16(Ptr, Value) { ((uint8*)Ptr)[0] = (Value)&0xff;  ((uint8*)Ptr)[1] = 0xff&((Value) >> 8); Ptr += 2; }
#define PTR_PUTUINT32(Ptr, Value) { ((uint8*)Ptr)[0] = (Value)&0xff;  ((uint8*)Ptr)[1] = 0xff&((Value) >> 8);  ((uint8*)Ptr)[2] = 0xff&((Value) >> 16);  ((uint8*)Ptr)[3] = 0xff&((Value) >> 24); Ptr += 4; }
#define PTR_PUTFP32(Ptr, Value) {NetUnion u; u.m_F = Value; PTR_PUTUINT32(Ptr, u.m_UW);}

#define PTR_PUTSTR(Ptr, Value) { int Len = Value.Len(); PTR_PUTINT32(Ptr, Len); memcpy(Ptr, Value.Str(), Len); Ptr += Len; }
#define PTR_PUTDATA(Ptr, pData, Len) { memcpy(Ptr, pData, (Len)); Ptr += (Len); }
#define PTR_PUTCMTIME(Ptr, Value) { (Value).Pack(Ptr);}

// Non portable
#define PTR_PUTMINT(Ptr, Value) { *((mint*)Ptr) = (Value); Ptr += sizeof(mint); }


#define PTR_GETINT8(Ptr, Value) { Value = (int8)((uint8*)Ptr)[0]; Ptr++; }
#define PTR_GETINT16(Ptr, Value) { Value = (int16)(((uint8*)Ptr)[0] + (((uint8*)Ptr)[1] << 8)); Ptr += 2; }
#define PTR_GETINT32(Ptr, Value) { Value = (int32)(((uint8*)Ptr)[0] + (((uint8*)Ptr)[1] << 8) + (((uint8*)Ptr)[2] << 16) + (((uint8*)Ptr)[3] << 24)); Ptr += 4; }
#define PTR_GETUINT8(Ptr, Value) { Value = (uint8)((uint8*)Ptr)[0]; Ptr++; }
#define PTR_GETUINT16(Ptr, Value) { Value = (uint16)(((uint8*)Ptr)[0] + (((uint8*)Ptr)[1] << 8)); Ptr += 2; }
#define PTR_GETUINT32(Ptr, Value) { Value = (uint32)(((uint8*)Ptr)[0] + (((uint8*)Ptr)[1] << 8) + (((uint8*)Ptr)[2] << 16) + (((uint8*)Ptr)[3] << 24)); Ptr += 4; }
#define PTR_GETFP32(Ptr, Value) {NetUnion u; PTR_GETUINT32(Ptr, u.m_UW); Value = u.m_F;}

#define PTR_GETSTR(Ptr, Value) { int Len; PTR_GETINT32(Ptr, Len); Value.Capture((char*)Ptr, Len); Ptr += Len; }
#define PTR_GETDATA(Ptr, pData, Len) { memcpy(pData, Ptr, (Len)); Ptr += (Len); }
#define PTR_GETCMTIME(Ptr, Value) { (Value).Unpack(Ptr);}

// Non portable
#define PTR_GETMINT(Ptr, Value) { Value = *((mint*)Ptr); Ptr += sizeof(mint); }

#endif

// -------------------------------------------------------------------
extern int UnpackDeltaRegistry(CRegistry* _pReg, const uint8* _pData, int _Len);
extern int PackDeltaRegistry(int _RegNr, const CRegistry* _pReg, const CRegistry* _pOldReg, uint8* _pData, int _RecommendLen, int _MaxLen);

#define NET_MAXPACKETSIZE 256
#define WPACKET_MAXPACKETSIZE (NET_MAXPACKETSIZE-2)

//#define WADDITIONALCAMERACONTROL
//#define WADDITIONALCAMERACONTROL_NODIALOGLIGHT

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CNetMsg

	Comments:		Message class for sending packets between server
					and client
\*____________________________________________________________________*/
class CNetMsg
{
public:
	uint8 m_MsgType;
	uint8 m_MsgSize;
	uint8 m_Data[WPACKET_MAXPACKETSIZE];

	CNetMsg();
	CNetMsg(uint8 _MsgType);
	void operator= (const CNetMsg&);

	void SetInt8(int8 _Val, int _Pos);
	void SetInt16(int16 _Val, int _Pos);
	void SetInt32(int32 _Val, int _Pos);
	void Setfp32(fp32 _Val, int _Pos);

	bool AddInt8(int8 _Val);
	bool AddInt16(int16 _Val);
	bool AddInt32(int32 _Val);
	bool Addfp32(fp32 _Val);
	bool AddStr(CStr _Val);
	bool AddStr(const char* _pStr);
	bool AddStrW(CStr _Val);
	bool AddStrAny(CStr _Val);

	int8 GetInt8(int &_Pos) const;
	int16 GetInt16(int &_Pos) const;
	int32 GetInt32(int &_Pos) const;
	fp32 Getfp32(int &_Pos) const;
	CStr GetStr(int &_Pos) const;
	void GetStr(int &_Pos, char* _pDest) const;
	CStr GetStrW(int &_Pos) const;
	void GetStrW(int &_Pos, char* _pDest) const;
	CStr GetStrAny(int &_Pos) const;

	bool AddVecInt8_Max128(const CVec3Dfp32& _v);
	CVec3Dfp32 GetVecInt8_Max128(int &_Pos) const;
	bool AddVecInt8_DynPrecision(const CVec3Dfp32& _v);
	CVec3Dfp32 GetVecInt8_DynPrecision(int &_Pos) const;
	bool AddVecInt16_Max32768(const CVec3Dfp32& _v);
	CVec3Dfp32 GetVecInt16_Max32768(int &_Pos) const;
	bool AddVecInt16_DynPrecision(const CVec3Dfp32& _v);
	CVec3Dfp32 GetVecInt16_DynPrecision(int &_Pos) const;

	bool AddData(const void* _pData, int _nBytes);
	bool GetData(void* _pData, int _nBytes, int &_Pos) const;

	int GetSize() const { return m_MsgSize + 2; };
	int GetSpaceLeft() { return WPACKET_MAXPACKETSIZE-m_MsgSize; };

	void Read(CCFile* _pF);
	void Write(CCFile* _pF) const;
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CNetMsgPack

	Comments:		-
\*____________________________________________________________________*/
class CNetMsgPack
{
public:
	int m_Size;
	int m_ReadPtr;
	uint8 m_MsgBuf[NET_MAXPACKETSIZE];

	CNetMsgPack();
	void Clear();
	bool AddMsg(const CNetMsg& _Msg);
	const CNetMsg* GetMsg();
};

#endif //_WCLASS_H
