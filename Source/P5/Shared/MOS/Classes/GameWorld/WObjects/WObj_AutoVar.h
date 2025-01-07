#ifndef __WOBJ_AUTOVAR_H
#define __WOBJ_AUTOVAR_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			AutoVar system and implementations

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CAutoVarContainer
					TAutoVar
					CAutoVar_CVec3Dfp32
					CAutoVar_CMat4Dfp32
					CAutoVar_CMat4Dfp32
					CAutoVar_uint16_Resource
					CAutoVar_CVec3Dfp32_Packed32
					TStaticArray
					TAutoVar_StaticArray
					TAutoVar_uint16StaticArray_Resource
\*____________________________________________________________________________________________*/

#include "../WMapData.h"

/////////////////////////////////////////////////////////////////
// AutoVar
//
// Class for helping with serializing and replicating variables
// in a client/server environment.
//
// Complete:
// * No extra memory consuption
// * No initialization time
// * Linked dirtymasks
// * Better support of inheritance
// * Disk IO
// * Disk IO uses dirtymasks to be more backwardcompatible
//
// Todo:
// * Allow the use of clientflags as serialize option
// * Less places to define variables on?
// * Automatically definition of masks?
// * Warnings
// * Macro for creating autovar-arrays
// * Optimization

#ifndef M_RTM
//#define AUTOVAR_LOG
#endif


////////////////////////////////////////////////////////////////
#ifdef AUTOVAR_LOG
void AutoVarLog(const char *_str);
void DumpAutoVarLog();

# define AUTOVAR_LOG_NAMETYPE const char* m_pName;
# define AUTOVAR_LOG_NAMEDATA , #Var
#else
# define AUTOVAR_LOG_NAMETYPE
# define AUTOVAR_LOG_NAMEDATA
#endif




////////////////////////////////////////////////////////////////
// CAutoVarContainer

enum
{
	AUTOVAR_TYPE_PACK			= 0,
	AUTOVAR_TYPE_UNPACK			= 1,
	AUTOVAR_TYPE_COPY			= 2,
	AUTOVAR_TYPE_GETDIRTYMASK	= 3,
};

class CAutoVarContainer
{
public:
	enum
	{
		DIRTYMASK_0_0 = M_Bit(0), // The smaller mask you are using, the smaller the network overhead is.
		DIRTYMASK_0_1 = M_Bit(1),
		DIRTYMASK_0_2 = M_Bit(2),
		DIRTYMASK_0_3 = M_Bit(3),
		DIRTYMASK_0_4 = M_Bit(4),
		DIRTYMASK_0_5 = M_Bit(5),
		DIRTYMASK_0_6 = M_Bit(6),
		DIRTYMASK_1_0 = M_Bit(7),
		DIRTYMASK_1_1 = M_Bit(8),
		DIRTYMASK_1_2 = M_Bit(9),
		DIRTYMASK_1_3 = M_Bit(10),
		DIRTYMASK_1_4 = M_Bit(11),
		DIRTYMASK_1_5 = M_Bit(12),
		DIRTYMASK_1_6 = M_Bit(13),
		DIRTYMASK_2_0 = M_Bit(14),
		DIRTYMASK_2_1 = M_Bit(15),
	};
	uint32 m_DirtyMask;

public:
	CAutoVarContainer()
		: m_DirtyMask(0) {}

	bool AutoVar_IsDirty() const
	{
		return (m_DirtyMask != 0);
	}

	uint32 AutoVar_RefreshDirtyMask()
	{
		uint32 Mask = m_DirtyMask;
		m_DirtyMask = 0;
		return Mask;
	}

	bool AutoVar_Pack(int _DirtyMask, uint8 *&_pD, CMapData* _pMapData, int _Debug = 0) const;
	bool AutoVar_Unpack(const uint8 *&_pD, CMapData* _pMapData, int _Debug = 0);
	void AutoVar_Write(CCFile *_pFile, CMapData* _pMapData);
	void AutoVar_Read(CCFile* _pFile, CMapData* _pMapData);

	virtual void AutoVar_OnOperate(int _iType, int _DirtyMask, void* _pContext, CMapData* _pMapData) {}
};



// Helper class to add static OnOperate-functions for an existing Autovar-type
template<typename t_Class>
class TAutoVar_OpWrapper : public t_Class
{
public:
	static void SCopyFrom(void* _pThis, const void* _pFrom)
	{
		((t_Class*)_pThis)->CopyFrom( *(const t_Class*)_pFrom );
	}
	static void SPack(const void* _pThis, uint8*& _pD, CMapData* _pMapData)
	{
		((const t_Class*)_pThis)->Pack(_pD, _pMapData);
	}
	static void SUnpack(void* _pThis, const uint8*& _pD, CMapData* _pMapData)
	{
		((t_Class*)_pThis)->Unpack(_pD, _pMapData);
	}

	// File I/O
	void Write(CCFile* _pFile, CMapData* _pMapData)
	{
		const int BufSize = sizeof(t_Class) * 2;
		uint8 buf[BufSize];
		uint8* pBuf = buf;
		t_Class::Pack(pBuf, _pMapData);
		mint nSize = (pBuf - buf);
		M_ASSERT(nSize < BufSize, "TAutoVar_Write buffer overrun");
		_pFile->Write(buf, nSize);
	}

	void Read(CCFile* _pFile, CMapData* _pMapData)
	{
		const int BufSize = sizeof(t_Class) * 2;
		fint Size = _pFile->Length();
		fint Pos = _pFile->Pos();
		int nMax = Min(BufSize, int(Size - Pos));

		uint8 buf[BufSize];
		_pFile->Read(buf, nMax);
		const uint8* pBuf = buf;
		t_Class::Unpack(pBuf, _pMapData);
		mint nSize = (pBuf - buf);
		M_ASSERT(nSize < BufSize, "TAutoVar_Read buffer overrun");

		_pFile->RelSeek(nSize - nMax);
	}
};

///////////////////////////////////////////////////////////////////////////////////////
// These macros are used to automatically create the AutoVar_OnOperate functions etc

#define AUTOVAR_SETCLASS(Class, BaseClass)	 \
	typedef Class __AutoVar_ContainerClass;	 \
	typedef BaseClass __AutoVar_BaseClass;

#define AUTOVAR_OFFSETOF(Member) (((mint)&((const __AutoVar_ContainerClass*)0x10000)->Member) - 0x10000)

#ifdef AUTOVAR_LOG
#define AUTOVAR_LOG_NAME const char *m_pName;
#define AUTOVAR_LOG_VAR(Var) { AUTOVAR_OFFSETOF(Var), Var.EDirtyMask, Var.SCopyFrom, Var.SPack, Var.SUnpack, #Var },
#define AUTOVAR_LOG_PACK if(_iType & 0x1000) AutoVarLog(CStrF("Pack %s\n", s_VarList[i].m_pName));
#define AUTOVAR_LOG_UNPACK if(_iType & 0x1000) AutoVarLog(CStrF("Unpack %s\n", s_VarList[i].m_pName));
#else
#define AUTOVAR_LOG_NAME
#define AUTOVAR_LOG_VAR(Var) { AUTOVAR_OFFSETOF(Var), Var.EDirtyMask, Var.SCopyFrom, Var.SPack, Var.SUnpack },
#define AUTOVAR_LOG_PACK
#define AUTOVAR_LOG_UNPACK
#endif
#define AUTOVAR_PACK_BEGIN \
										void AutoVar_CopyFrom(const __AutoVar_ContainerClass& _CD)\
										{\
											MSCOPESHORT(AutoVar_CopyFrom);\
											AutoVar_OnOperate(AUTOVAR_TYPE_COPY, 0, const_cast<__AutoVar_ContainerClass*>(&_CD), NULL);\
										}\
										\
										virtual void AutoVar_OnOperate(int _iType, int _DirtyMask, void* _pContext, CMapData* _pMapData)\
										{\
											__AutoVar_BaseClass::AutoVar_OnOperate(_iType, _DirtyMask, _pContext, _pMapData);\
											__AutoVar_ContainerClass* pObj = (__AutoVar_ContainerClass*)_pContext; \
											\
											const static struct\
											{\
												uint32 m_VarOffset;\
												uint32 m_DirtyMask;\
												void(* m_pCopyFrom)(void*, const void*);\
												void(* m_pPack)(const void*, uint8 *&, CMapData*);\
												void(* m_pUnpack)(void*, const uint8 *&, CMapData*);\
												AUTOVAR_LOG_NAME\
											} s_VarList[] = {

#define AUTOVAR_PACK_VAR(Var) \
											AUTOVAR_LOG_VAR(Var)

#define AUTOVAR_PACK_END \
										};\
										\
										int Type = _iType & 0xff;\
										if (Type == AUTOVAR_TYPE_PACK)\
										{\
											for (int i=0; i<sizeof(s_VarList)/sizeof(s_VarList[0]); i++)\
												if (_DirtyMask & s_VarList[i].m_DirtyMask)\
												{\
													s_VarList[i].m_pPack((uint8*)this + s_VarList[i].m_VarOffset, *(uint8**)_pContext, _pMapData);\
													AUTOVAR_LOG_PACK\
												}\
										}\
										else if (Type == AUTOVAR_TYPE_UNPACK)\
										{\
											for (int i=0; i<sizeof(s_VarList)/sizeof(s_VarList[0]); i++)\
												if (_DirtyMask & s_VarList[i].m_DirtyMask)\
												{\
													s_VarList[i].m_pUnpack((uint8*)this + s_VarList[i].m_VarOffset, *(const uint8**)_pContext, _pMapData);\
													AUTOVAR_LOG_UNPACK\
												}\
										}\
										else if (Type == AUTOVAR_TYPE_COPY)\
										{\
											for (int i=0; i<sizeof(s_VarList)/sizeof(s_VarList[0]); i++)\
												s_VarList[i].m_pCopyFrom((uint8*)this + s_VarList[i].m_VarOffset, (uint8*)pObj + s_VarList[i].m_VarOffset );\
										}\
										else if (Type == AUTOVAR_TYPE_GETDIRTYMASK)\
										{\
											for (int i=0; i<sizeof(s_VarList)/sizeof(s_VarList[0]); i++)\
												(*(int *)_pContext) |= s_VarList[i].m_DirtyMask;\
										}\
									}


/////////////////////////////////////////////////////////////////////////////////
// Shared code for CAUTOVAR() and CAUTOVAR_OP() macros
#define __CAUTOVAR_base(Type, Variable, Mask)									\
class AutoVar_##Variable : public TAutoVar_OpWrapper<Type >						\
{																				\
	typedef __AutoVar_ContainerClass Class;										\
public:																			\
	enum { EDirtyMask = Mask };													\
																				\
	M_FORCEINLINE void MakeDirty()												\
	{																			\
		const int Offset = AUTOVAR_OFFSETOF(Variable);							\
		((Class *)((mint)this - Offset))->m_DirtyMask |= Mask;					\
	}

/////////////////////////////////////////////////////////////////////////////////
// Macro to automatically create a AutoVar-class that has a MakeDirty function
#define CAUTOVAR(Type, Variable, Mask)											\
	typedef Type Type_##Variable;												\
	__CAUTOVAR_base(Type_##Variable, Variable, Mask)							\
} Variable

/////////////////////////////////////////////////////////////////////////////////
// Same as CAUTOVAR() but also creates a functional operator=
#define CAUTOVAR_OP(Type, Variable, Mask)										\
	typedef Type Type_##Variable;												\
	__CAUTOVAR_base(Type_##Variable, Variable, Mask)							\
																				\
	M_FORCEINLINE Type::AccessType& operator=(const Type::AccessType& _Val)		\
	{																			\
		if (*this != _Val)														\
		{																		\
			m_Value = _Val;		/* Type::operator=(_Val); */					\
			MakeDirty();														\
		}																		\
		return m_Value;															\
	}																			\
} Variable






////////////////////////////////////////////////////////////////////
// Methods for reading/writing to a memory buffer
// (actually they're just PTR_PUTxx wrappers and could perhaps be moved to WClass.h?)
//
template<typename T> M_FORCEINLINE void TAutoVar_Pack(const T& _Var, uint8*& _pD)   { _Var.Pack(_pD); }
template<typename T> M_FORCEINLINE void TAutoVar_Unpack(T& _Var, const uint8*& _pD) { _Var.Unpack(_pD); }

template<> M_FORCEINLINE void TAutoVar_Pack(const fp32& _Var, uint8*& _pD)     { PTR_PUTFP32(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Pack(const int8& _Var, uint8*& _pD)     { PTR_PUTINT8(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Pack(const uint8& _Var, uint8*& _pD)    { PTR_PUTINT8(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Pack(const int16& _Var, uint8*& _pD)    { PTR_PUTINT16(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Pack(const uint16& _Var, uint8*& _pD)   { PTR_PUTINT16(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Pack(const int32& _Var, uint8*& _pD)    { PTR_PUTINT32(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Pack(const uint32& _Var, uint8*& _pD)   { PTR_PUTINT32(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Pack(const CStr& _Var, uint8*& _pD)     { PTR_PUTSTR(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Unpack(fp32& _Var, const uint8*& _pD)   { PTR_GETFP32(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Unpack(int8& _Var, const uint8*& _pD)   { PTR_GETINT8(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Unpack(uint8& _Var, const uint8*& _pD)  { PTR_GETINT8(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Unpack(int16& _Var, const uint8*& _pD)  { PTR_GETINT16(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Unpack(uint16& _Var, const uint8*& _pD) { PTR_GETINT16(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Unpack(int32& _Var, const uint8*& _pD)  { PTR_GETINT32(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Unpack(uint32& _Var, const uint8*& _pD) { PTR_GETINT32(_pD, _Var); }
template<> M_FORCEINLINE void TAutoVar_Unpack(CStr& _Var, const uint8*& _pD)   { PTR_GETSTR(_pD, _Var); }


// Support for TFStr<>
template<int t_Len> 
M_FORCEINLINE void TAutoVar_Pack(const TFStr<t_Len>& _Var, uint8*& _pD)   { PTR_PUTSTR(_pD, _Var); }
template<int t_Len> 
M_FORCEINLINE void TAutoVar_Unpack(TFStr<t_Len>& _Var, const uint8*& _pD) { PTR_GETSTR(_pD, _Var); }


// Support for some MMath.h types
template <class T>
M_FORCEINLINE void TAutoVar_Pack(const TVector2<T>& _Var, uint8*& _pD)
{
	TAutoVar_Pack(_Var.k[0], _pD);
	TAutoVar_Pack(_Var.k[1], _pD);
}
template <class T>
M_FORCEINLINE void TAutoVar_Unpack(TVector2<T>& _Var, const uint8*& _pD)
{
	TAutoVar_Unpack(_Var.k[0], _pD);
	TAutoVar_Unpack(_Var.k[1], _pD);
}


template <class T>
M_FORCEINLINE void TAutoVar_Pack(const TVector3<T>& _Var, uint8*& _pD)
{
	TAutoVar_Pack(_Var.k[0], _pD);
	TAutoVar_Pack(_Var.k[1], _pD);
	TAutoVar_Pack(_Var.k[2], _pD);
}
template <class T>
M_FORCEINLINE void TAutoVar_Unpack(TVector3<T>& _Var, const uint8*& _pD)
{
	TAutoVar_Unpack(_Var.k[0], _pD);
	TAutoVar_Unpack(_Var.k[1], _pD);
	TAutoVar_Unpack(_Var.k[2], _pD);
}


template <class T>
M_FORCEINLINE void TAutoVar_Pack(const TVector3Aggr<T>& _Var, uint8*& _pD)
{
	TAutoVar_Pack(_Var.k[0], _pD);
	TAutoVar_Pack(_Var.k[1], _pD);
	TAutoVar_Pack(_Var.k[2], _pD);
}
template <class T>
M_FORCEINLINE void TAutoVar_Unpack(TVector3Aggr<T>& _Var, const uint8*& _pD)
{
	TAutoVar_Unpack(_Var.k[0], _pD);
	TAutoVar_Unpack(_Var.k[1], _pD);
	TAutoVar_Unpack(_Var.k[2], _pD);
}


template<> M_FORCEINLINE void TAutoVar_Pack(const CQuatfp32& _Var, uint8*& _pD)
{
	TAutoVar_Pack(_Var.k[0], _pD);
	TAutoVar_Pack(_Var.k[1], _pD);
	TAutoVar_Pack(_Var.k[2], _pD);
	TAutoVar_Pack(_Var.k[3], _pD);
}
template<> M_FORCEINLINE void TAutoVar_Unpack(CQuatfp32& _Var, const uint8*& _pD)
{
	TAutoVar_Unpack(_Var.k[0], _pD);
	TAutoVar_Unpack(_Var.k[1], _pD);
	TAutoVar_Unpack(_Var.k[2], _pD);
	TAutoVar_Unpack(_Var.k[3], _pD);
}


template<> M_FORCEINLINE void TAutoVar_Pack(const CMat4Dfp32& _Var, uint8*& _pD)
{
	CQuatfp32 Quat;
	Quat.Create(_Var);
	TAutoVar_Pack(Quat, _pD);
	TAutoVar_Pack(_Var.GetRow(3), _pD);
}
template<> M_FORCEINLINE void TAutoVar_Unpack(CMat4Dfp32& _Var, const uint8*& _pD)
{
	CQuatfp32 Quat;
	TAutoVar_Unpack(Quat, _pD);
	Quat.CreateMatrix(_Var);
	TAutoVar_Unpack(_Var.GetRow(3), _pD);
}


template<> M_FORCEINLINE void TAutoVar_Pack(const CMat43fp32& _Var, uint8*& _pD)
{
	CQuatfp32 Quat;
	Quat.Create(_Var);
	TAutoVar_Pack(Quat, _pD);
	TAutoVar_Pack(_Var.GetRow(3), _pD);
}
template<> M_FORCEINLINE void TAutoVar_Unpack(CMat43fp32& _Var, const uint8*& _pD)
{
	CQuatfp32 Quat;
	TAutoVar_Unpack(Quat, _pD);
	Quat.CreateMatrix(_Var);
	TAutoVar_Unpack(_Var.GetRow(3), _pD);
}


template<class T>
M_FORCEINLINE void TAutoVar_Pack(const TAxisRot<T>& _Var, uint8*& _pD)
{
	TAutoVar_Pack(_Var.m_Axis, _pD);
	TAutoVar_Pack(_Var.m_Angle, _pD);
}
template<class T>
M_FORCEINLINE void TAutoVar_Unpack(TAxisRot<T>& _Var, const uint8*& _pD)
{
	TAutoVar_Unpack(_Var.m_Axis, _pD);
	TAutoVar_Unpack(_Var.m_Angle, _pD);
}







/////////////////////////////////////////////////////////////////////////////////
template<typename T>
class TAutoVar
{
public:
	typedef T AccessType;

	T m_Value;

	M_FORCEINLINE TAutoVar() {}
	M_FORCEINLINE TAutoVar(const T& _Value) : m_Value(_Value) {}

	operator const T&() const
	{
		return m_Value;
	}

	// Override these for special data types
	M_FORCEINLINE void CopyFrom(const TAutoVar& _From)
	{
		m_Value = _From.m_Value;
	}
	M_FORCEINLINE void Pack(uint8*& _pD, CMapData*) const
	{
		TAutoVar_Pack(m_Value, _pD);
	}
	M_FORCEINLINE void Unpack(const uint8*& _pD, CMapData*)
	{
		TAutoVar_Unpack(m_Value, _pD);
	}
};



#define AUTOVAR_INITVALUE(type, value) \
	template<> M_INLINE TAutoVar<type>::TAutoVar() : m_Value(value) {}


template<typename T>
class TAutoVar_Compare : public TAutoVar<T>
{
public:
	bool operator!= (const T& _Other) const 
	{
		return TAutoVar<T>::m_Value != _Other; 
	}
};


typedef TAutoVar<int8> CAutoVar_int8;
typedef TAutoVar<int16> CAutoVar_int16;
typedef TAutoVar<int32> CAutoVar_int32;
typedef TAutoVar<uint8> CAutoVar_uint8;
typedef TAutoVar<uint16> CAutoVar_uint16;
typedef TAutoVar<uint32> CAutoVar_uint32;
typedef TAutoVar<fp32> CAutoVar_fp32;
typedef TAutoVar<CFStr> CAutoVar_CFStr;

typedef TAutoVar_Compare<CVec2Dfp32> CAutoVar_CVec2Dfp32;
typedef TAutoVar_Compare<CVec3Dfp32> CAutoVar_CVec3Dfp32;
typedef TAutoVar_Compare<CVec3Duint8> CAutoVar_CVec3Duint8;


AUTOVAR_INITVALUE(int8, 0);
AUTOVAR_INITVALUE(int16, 0);
AUTOVAR_INITVALUE(int32, 0);
AUTOVAR_INITVALUE(uint8, 0);
AUTOVAR_INITVALUE(uint16, 0);
AUTOVAR_INITVALUE(uint32, 0);
AUTOVAR_INITVALUE(fp32, 0.0f);
AUTOVAR_INITVALUE(CVec2Dfp32, 0.0f);
AUTOVAR_INITVALUE(CVec3Dfp32, 0.0f);
AUTOVAR_INITVALUE(CVec3Duint8, 0);
AUTOVAR_INITVALUE(CAxisRotfp32, CAxisRotfp32(0.0f, 0.0f));



//
// CAutoVar_CMTime
//
class CAutoVar_CMTime : public TAutoVar<CMTime>
{
public:
	bool operator!= (const CMTime &_Time) const
	{
		return m_Value.Compare(_Time) != 0;
	}
};


//
// CAutoVar_CAxisRotfp32
//
class CAutoVar_CAxisRotfp32 : public TAutoVar<CAxisRotfp32>
{
public:
	bool operator!=(const CAxisRotfp32& _Other) const
	{
		return (m_Value.m_Axis != _Other.m_Axis || m_Value.m_Angle != _Other.m_Angle);
	}
};


//
// CAutoVar_CQuatfp32
//
class CAutoVar_CQuatfp32 : public TAutoVar<CQuatfp32>
{
public:
	CAutoVar_CQuatfp32()
	{
		m_Value.Unit();
	}

	bool operator!= (const CQuatfp32& _Quat) const
	{
		return (m_Value.k[0] != _Quat.k[0] || m_Value.k[1] != _Quat.k[1] ||
		        m_Value.k[2] != _Quat.k[2] || m_Value.k[3] != _Quat.k[3]);
	}
};


//
// CAutoVar_CMat4Dfp32
//
class CAutoVar_CMat4Dfp32 : public TAutoVar<CMat4Dfp32>
{
public:
	CAutoVar_CMat4Dfp32()
	{
		m_Value.Unit();
	}

	bool operator!= (const CMat4Dfp32& _Mat) const
	{
		return !m_Value.AlmostEqual(_Mat, 0.0001f);
	}
};

class CAutoVar_CMat43fp32 : public TAutoVar<CMat43fp32>
{
public:
	CAutoVar_CMat43fp32()
	{
		m_Value.Unit();
	}
	
	bool operator!= (const CMat43fp32& _Mat) const
	{
		return !m_Value.AlmostEqual(_Mat, 0.0001f);
	}
};


//
// CAutoVar_uint16_Resource
//
class CAutoVar_uint16_Resource : public CAutoVar_int16
{
public:
	// If _pMapData != NULL resource remapping should be performed. (i.e Read/Write mode instead of Pack/Unpack)
	void Pack(uint8*& _pD, CMapData* _pMapData) const
	{
		if (_pMapData)
		{
			CStr Name = _pMapData->GetResourceName(m_Value);
			TAutoVar_Pack(Name, _pD);
		}
		else
			TAutoVar_Pack(m_Value, _pD);
	}

	void Unpack(const uint8*& _pD, CMapData* _pMapData)
	{
		if (_pMapData)
		{
			CStr Name;
			TAutoVar_Unpack(Name, _pD);
			m_Value = _pMapData->GetResourceIndex(Name);
		}
		else
			TAutoVar_Unpack(m_Value, _pD);
	}
};


//
// CAutoVar_CVec3Dfp32_Packed32
//
template<int _Max>
class CAutoVar_CVec3Dfp32_Packed32 : public CAutoVar_CVec3Dfp32
{
public:
	void Pack(uint8*& _pD, CMapData* ) const
	{
		uint32 Val = m_Value.Pack32((fp32)_Max);
		TAutoVar_Pack(Val, _pD);
	}

	void Unpack(const uint8*& _pD, CMapData* )
	{
		uint32 Val;
		TAutoVar_Unpack(Val, _pD);
		m_Value.Unpack32(Val, (fp32)_Max);
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TAutoVar_StaticArray
|__________________________________________________________________________________________________
\*************************************************************************************************/
template <class T, int MaxLen>
class TAutoVar_StaticArray : public TStaticArray<T, MaxLen>
{
	typedef TStaticArray<T, MaxLen> parent;

public:
	void CopyFrom(const TAutoVar_StaticArray& _From)
	{
		*this = _From;
	}

	void Pack(uint8*& _pD, CMapData* ) const
	{
		typename parent::LenType nElems = parent::Len();
		TAutoVar_Pack(nElems, _pD);
		for (uint i = 0; i < nElems; i++)
			TAutoVar_Pack(parent::m_Data[i], _pD);
	}

	void Unpack(const uint8*& _pD, CMapData* )
	{
		typename parent::LenType nElems = 0;
		TAutoVar_Unpack(nElems, _pD);
		TStaticArray<T, MaxLen>::SetLen(nElems);
		for (uint i = 0; i < nElems; i++)
			TAutoVar_Unpack(parent::m_Data[i], _pD);
	}
};


template<uint8 MaxLen>
class TAutoVar_uint16StaticArray_Resource : public TStaticArray<CAutoVar_uint16_Resource, MaxLen>
{
	typedef TStaticArray<CAutoVar_uint16_Resource, MaxLen> parent;

public:
	void Pack(uint8*& _pD, CMapData* _pMapData) const
	{
		uint8 nElems = parent::Len();
		TAutoVar_Pack(nElems, _pD);

		for (int i = 0; i < nElems; i++)
			parent::m_Data[i].Pack(_pD, _pMapData);
	}

	void Unpack(const uint8*& _pD, CMapData* _pMapData)
	{
		uint8 nElems = 0;
		TAutoVar_Unpack(nElems, _pD);
		parent::SetLen(nElems);

		for (int i = 0; i < nElems; i++)
			parent::m_Data[i].Unpack(_pD, _pMapData);
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TAutoVar_ThinArray
|__________________________________________________________________________________________________
\*************************************************************************************************/
template<class T>
class TAutoVar_ThinArray : public TThinArray<T>
{
	typedef TThinArray<T> parent;

public:
	void CopyFrom(const TAutoVar_ThinArray& _From)
	{
		TAP<const T> pSrc = _From;
		TAP<T> pDest = *this;

		if (pSrc.Len() == pDest.Len())
		{
			// no realloc, just copy
			for (uint i = 0; i < pSrc.Len(); i++)
				pDest[i] = pSrc[i];
		}
		else
		{
			*this = _From;
		}
	}

	void Pack(uint8*& _pD, CMapData* ) const
	{
		TAP<const T> pElems = *this;
		M_ASSERT( pElems.Len() < 65536, "Too many elements in TAutoVar_ThinArray!");

		TAutoVar_Pack(uint16(pElems.Len()), _pD);
		for (uint i = 0; i < pElems.Len(); i++)
			TAutoVar_Pack(pElems[i], _pD);
	}

	void Unpack(const uint8*& _pD, CMapData* )
	{
		uint16 nElems = 0;
		TAutoVar_Unpack(nElems, _pD);
		parent::SetLen(nElems);
		TAP<T> pElems = *this;
		for (uint i = 0; i < nElems; i++)
			TAutoVar_Unpack(pElems[i], _pD);
	}
};



// These macros are needed because the C-preprocessor doesn't handle <,>  but it handles (,)
#define TAutoVar_StaticArray_(TYPE, LEN) TAutoVar_StaticArray<TYPE, LEN > 
#define TAutoVar_ThinArray_(TYPE) TAutoVar_ThinArray<TYPE > 


#endif


