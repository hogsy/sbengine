
#ifndef DInc_MScript_RegisterContext_h
#define DInc_MScript_RegisterContext_h

namespace NScript
{
	/*class TCArgPos0
	{
	public:
		enum 
		{ 
			ESize = 0
		}; 

		M_INLINE static int GetPos(int _Pos)
		{
			return 0;
		}
	};*/

	#define DRegClassFunctionCallery(_Index, _TypeNames, _TypeNames1, _TypeNames2, _TypeConvert, _TypeNames3, _TypeList)\
	template <typename t_CReturnType, typename t_CClassName DTempArgConv0 _TypeNames>\
	class TClassFunctionCaller##_Index : public CFunctionCaller\
	{\
		static const char *ms_lArgTypes[];\
		typedef t_CReturnType(t_CClassName::* CFunctionType)(_TypeNames1) ;\
		CFunctionType m_pFunction;\
		t_CClassName *m_pThis;\
	public:\
		\
		TClassFunctionCaller##_Index(t_CClassName *_pClass, CFunctionType _pFunction)\
		{\
			m_nArguments = _Index;\
			m_pThis = _pClass;\
			m_pFunction = _pFunction;\
			m_pArgTypes = ms_lArgTypes;\
			m_pReturnType = ScriptTypeName<t_CReturnType>();\
		}\
		void Call(void *_pArgs, void *_pReturn)\
		{\
			*(*((t_CReturnType **)_pReturn)) = (*m_pThis.*m_pFunction)(_TypeConvert);\
		}\
	};\
	template <typename t_CReturnType, typename t_CClassName DTempArgConv0 _TypeNames>\
	const char *TClassFunctionCaller##_Index<t_CReturnType, t_CClassName DTempArgConv0 _TypeNames2 >::ms_lArgTypes[] = _TypeList;\
	template <typename t_CClassName DTempArgConv0 _TypeNames>\
	class TClassFunctionCaller##_Index<void, t_CClassName DTempArgConv0 _TypeNames2> : public CFunctionCaller\
	{\
		static const char *ms_lArgTypes[];\
		typedef void (t_CClassName::* CFunctionType)(_TypeNames1) ;\
		CFunctionType m_pFunction;\
		t_CClassName *m_pThis;\
	public:\
		\
		TClassFunctionCaller##_Index(t_CClassName *_pClass, CFunctionType _pFunction)\
		{\
			m_nArguments = _Index;\
			m_pThis = _pClass;\
			m_pFunction = _pFunction;\
			m_pArgTypes = ms_lArgTypes;\
			m_pReturnType = ScriptTypeName<void>();\
		}\
		void Call(void *_pArgs, void *_pReturn)\
		{\
			(*m_pThis.*m_pFunction)(_TypeConvert);\
		}\
	};\
	template <typename t_CClassName DTempArgConv0 _TypeNames>\
	const char *TClassFunctionCaller##_Index<void, t_CClassName DTempArgConv0 _TypeNames2 >::ms_lArgTypes[] = _TypeList;\
	template <typename t_CReturnType, typename t_CClassName DTempArgConv0 _TypeNames>\
	CFunctionCaller *GetFunctionCallerClass(t_CClassName *_pThis, t_CReturnType(t_CClassName::*_pFunction)(_TypeNames1))\
	{\
		typedef TClassFunctionCaller##_Index<t_CReturnType, t_CClassName DTempArgConv0 _TypeNames2 > CIndex;\
		return DNew(CIndex) CIndex(_pThis, _pFunction);\
	}\
	template <typename t_CReturnType DTempArgConv0 _TypeNames>\
	class TFunctionCaller##_Index : public CFunctionCaller\
	{\
		static const char *ms_lArgTypes[];\
		typedef t_CReturnType(* CFunctionType)(_TypeNames1) ;\
		CFunctionType m_pFunction;\
	public:\
		TFunctionCaller##_Index(CFunctionType _pFunction)\
		{\
			m_nArguments = _Index;\
			m_pFunction = _pFunction;\
			m_pArgTypes = ms_lArgTypes;\
			m_pReturnType = ScriptTypeName<t_CReturnType>();\
		}\
		void Call(void *_pArgs, void *_pReturn)\
		{\
			*(*((t_CReturnType **)_pReturn)) = (*m_pFunction)(_TypeConvert);\
		}\
	};\
	template <typename t_CReturnType DTempArgConv0 _TypeNames>\
	const char *TFunctionCaller##_Index<t_CReturnType DTempArgConv0 _TypeNames2 >::ms_lArgTypes[] = _TypeList;\
	template <_TypeNames3>\
	class TFunctionCaller##_Index<void DTempArgConv0 _TypeNames2 > : public CFunctionCaller\
	{\
		static const char *ms_lArgTypes[];\
		typedef void (* CFunctionType)(_TypeNames1) ;\
		CFunctionType m_pFunction;\
	public:\
		TFunctionCaller##_Index(CFunctionType _pFunction)\
		{\
			m_nArguments = _Index;\
			m_pFunction = _pFunction;\
			m_pArgTypes = ms_lArgTypes;\
			m_pReturnType = ScriptTypeName<void>();\
		}\
		void Call(void *_pArgs, void *_pReturn)\
		{\
			(*m_pFunction)(_TypeConvert);\
		}\
	};\
	template <typename t_CReturnType DTempArgConv0 _TypeNames>\
	CFunctionCaller *GetFunctionCaller(t_CReturnType(*_pFunction)(_TypeNames1))\
	{\
		typedef TFunctionCaller##_Index<t_CReturnType DTempArgConv0 _TypeNames2 > CIndex; \
		return DNew(CIndex) CIndex(_pFunction);\
	}

	#define DRegClassFunctionCallerz(_Index, _TypeNames, _TypeNames1, _TypeNames2, _TypeConvert, _TypeNames3, _TypeList)\
	template <_TypeNames3>\
	const char *TFunctionCaller##_Index<void DTempArgConv0 _TypeNames2 >::ms_lArgTypes[] = _TypeList;

//	#define DRegClassFunctionCallerx(_Index, _Index1, _TypeNames, _TypeNames1, _TypeNames2, _TypeConvert, _TypeNames3, _TypeList, _LastArgPos) 
//	DRegClassFunctionCallery(_Index, (_TypeNames), (_TypeNames1), (_TypeNames2), (_TypeConvert), (_TypeNames3), (_TypeList))
//	DRegClassFunctionCallerz(_Index, (_TypeNames), (_TypeNames1), (_TypeNames2), (_TypeConvert), (_TypeNames3), (_TypeList))

	/*
	template <_TypeNames3> \
	class TCArgPos##_Index \
	{ \
	public: \
		enum \
		{ \
			ESizeLast = TCArgPos##_Index1 _LastArgPos ::ESize \
			,ESize = ESizeLast + sizeof(t_C##_Index1) \
		}; \
		M_INLINE static int GetPos(int _Pos)\
		{\
			if (_Pos == _Index1)\
				return ESizeLast;\
			else\
				return TCArgPos##_Index1 _LastArgPos ::GetPos(_Pos);\
		}\
	};\

	*/
	namespace NFunctionConvert
	{
	#define DTC(_x, _y) *(((t_C##_x **)(_pArgs))[_x])
	//#define DTC(_x, _y) *((t_C##_x *)((uint8 *)_pArgs + TCArgPos##_y< DTempArg1 >::GetPos(_x)))
	#define DTN(_x) ScriptTypeName<t_C##_x>()
	#define DTempArgConv0 

	#define DTempArg0 //
	#define DTempArg1 //
	#define DTempArg2 //
	#define DTempArg3 {0}
	DRegClassFunctionCallery(0, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);


	#undef DTempArgConv0
	#define DTempArgConv0 ,

	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3
	#define DTempArg0 typename t_C0
	#define DTempArg1 t_C0
	#define DTempArg2 DTC(0,1)
	#define DTempArg3 {DTN(0)}
	#define DTempArg4 
//	DRegClassFunctionCallerx(1, 0, DTempArgConv0(DTempArg0), DTempArg1, DTempArgConv0(DTempArg1), DTempArg2, DTempArg0, DTempArg3, DTempArg4);
	DRegClassFunctionCallery(1, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(1, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3
	#define DTempArg0 typename t_C0,typename t_C1 
	#define DTempArg1 t_C0, t_C1
	#define DTempArg2 DTC(0,2), DTC(1,2)
	#define DTempArg3 {DTN(0), DTN(1)}
	DRegClassFunctionCallery(2, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(2, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0, t_C1>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3
	#define DTempArg0 typename t_C0,typename t_C1, typename t_C2
	#define DTempArg1 t_C0, t_C1, t_C2
	#define DTempArg2 DTC(0,3), DTC(1,3), DTC(2,3)
	#define DTempArg3 {DTN(0), DTN(1), DTN(2)}
	DRegClassFunctionCallery(3, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(3, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0, t_C1, t_C2>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3
	#define DTempArg0 typename t_C0,typename t_C1, typename t_C2, typename t_C3
	#define DTempArg1 t_C0, t_C1, t_C2, t_C3
	#define DTempArg2 DTC(0,4), DTC(1,4), DTC(2,4), DTC(3,4)
	#define DTempArg3 {DTN(0), DTN(1), DTN(2), DTN(3)}
	DRegClassFunctionCallery(4, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(4, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0, t_C1, t_C2, t_C3>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3
	#define DTempArg0 typename t_C0,typename t_C1, typename t_C2, typename t_C3, typename t_C4
	#define DTempArg1 t_C0, t_C1, t_C2, t_C3, t_C4
	#define DTempArg2 DTC(0,5), DTC(1,5), DTC(2,5), DTC(3,5), DTC(4,5)
	#define DTempArg3 {DTN(0), DTN(1), DTN(2), DTN(3), DTN(4)}
	DRegClassFunctionCallery(5, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(5, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0, t_C1, t_C2, t_C3, t_C4>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3
	#define DTempArg0 typename t_C0,typename t_C1, typename t_C2, typename t_C3, typename t_C4, typename t_C5
	#define DTempArg1 t_C0, t_C1, t_C2, t_C3, t_C4, t_C5
	#define DTempArg2 DTC(0,6), DTC(1,6), DTC(2,6), DTC(3,6), DTC(4,6), DTC(5,6)
	#define DTempArg3 {DTN(0), DTN(1), DTN(2), DTN(3), DTN(4), DTN(5)}
	DRegClassFunctionCallery(6, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(6, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0, t_C1, t_C2, t_C3, t_C4, t_C5>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3
	#define DTempArg0 typename t_C0,typename t_C1, typename t_C2, typename t_C3, typename t_C4, typename t_C5, typename t_C6
	#define DTempArg1 t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6
	#define DTempArg2 DTC(0,7), DTC(1,7), DTC(2,7), DTC(3,7), DTC(4,7), DTC(5,7), DTC(6,7)
	#define DTempArg3 {DTN(0), DTN(1), DTN(2), DTN(3), DTN(4), DTN(5), DTN(6)}
	DRegClassFunctionCallery(7, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(7, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3
	#define DTempArg0 typename t_C0,typename t_C1, typename t_C2, typename t_C3, typename t_C4, typename t_C5, typename t_C6, typename t_C7
	#define DTempArg1 t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7
	#define DTempArg2 DTC(0,8), DTC(1,8), DTC(2,8), DTC(3,8), DTC(4,8), DTC(5,8), DTC(6,8), DTC(7,8)
	#define DTempArg3 {DTN(0), DTN(1), DTN(2), DTN(3), DTN(4), DTN(5), DTN(6), DTN(7)}
	DRegClassFunctionCallery(8, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(8, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3
	#define DTempArg0 typename t_C0,typename t_C1, typename t_C2, typename t_C3, typename t_C4, typename t_C5, typename t_C6, typename t_C7, typename t_C8
	#define DTempArg1 t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8
	#define DTempArg2 DTC(0,9), DTC(1,9), DTC(2,9), DTC(3,9), DTC(4,9), DTC(5,9), DTC(6,9), DTC(7,9), DTC(8,9)
	#define DTempArg3 {DTN(0), DTN(1), DTN(2), DTN(3), DTN(4), DTN(5), DTN(6), DTN(7), DTN(8)}
	DRegClassFunctionCallery(9, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(9, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3
	#define DTempArg0 typename t_C0,typename t_C1, typename t_C2, typename t_C3, typename t_C4, typename t_C5, typename t_C6, typename t_C7, typename t_C8, typename t_C9
	#define DTempArg1 t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8, t_C9
	#define DTempArg2 DTC(0,10), DTC(1,10), DTC(2,10), DTC(3,10), DTC(4,10), DTC(5,10), DTC(6,10), DTC(7,10), DTC(8,10), DTC(9,10)
	#define DTempArg3 {DTN(0), DTN(1), DTN(2), DTN(3), DTN(4), DTN(5), DTN(6), DTN(7), DTN(8), DTN(9)}
	DRegClassFunctionCallery(10, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(10, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8, t_C9>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3 
	#define DTempArg0 typename t_C0,typename t_C1, typename t_C2, typename t_C3, typename t_C4, typename t_C5, typename t_C6, typename t_C7, typename t_C8, typename t_C9, typename t_C10
	#define DTempArg1 t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8, t_C9, t_C10
	#define DTempArg2 DTC(0,11), DTC(1,11), DTC(2,11), DTC(3,11), DTC(4,11), DTC(5,11), DTC(6,11), DTC(7,11), DTC(8,11), DTC(9,11), DTC(10,11)
	#define DTempArg3 {DTN(0), DTN(1), DTN(2), DTN(3), DTN(4), DTN(5), DTN(6), DTN(7), DTN(8), DTN(9), DTN(10)}
	DRegClassFunctionCallery(11, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(11, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8, t_C9, t_C10>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3 
	#define DTempArg0 typename t_C0,typename t_C1, typename t_C2, typename t_C3, typename t_C4, typename t_C5, typename t_C6, typename t_C7, typename t_C8, typename t_C9, typename t_C10, typename t_C11
	#define DTempArg1 t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8, t_C9, t_C10, t_C11
	#define DTempArg2 DTC(0,12), DTC(1,12), DTC(2,12), DTC(3,12), DTC(4,12), DTC(5,12), DTC(6,12), DTC(7,12), DTC(8,12), DTC(9,12), DTC(10,12), DTC(11,12)
	#define DTempArg3 {DTN(0), DTN(1), DTN(2), DTN(3), DTN(4), DTN(5), DTN(6), DTN(7), DTN(8), DTN(9), DTN(10), DTN(11)}
	DRegClassFunctionCallery(12, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(12, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8, t_C9, t_C10, t_C11>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3 
	#define DTempArg0 typename t_C0,typename t_C1, typename t_C2, typename t_C3, typename t_C4, typename t_C5, typename t_C6, typename t_C7, typename t_C8, typename t_C9, typename t_C10, typename t_C11, typename t_C12
	#define DTempArg1 t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8, t_C9, t_C10, t_C11, t_C12
	#define DTempArg2 DTC(0,13), DTC(1,13), DTC(2,13), DTC(3,13), DTC(4,13), DTC(5,13), DTC(6,13), DTC(7,13), DTC(8,13), DTC(9,13), DTC(10,13), DTC(11,13), DTC(12,13)
	#define DTempArg3 {DTN(0), DTN(1), DTN(2), DTN(3), DTN(4), DTN(5), DTN(6), DTN(7), DTN(8), DTN(9), DTN(10), DTN(11), DTN(12)}
	DRegClassFunctionCallery(13, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(13, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8, t_C9, t_C10, t_C11, t_C12>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3 
	#define DTempArg0 typename t_C0,typename t_C1, typename t_C2, typename t_C3, typename t_C4, typename t_C5, typename t_C6, typename t_C7, typename t_C8, typename t_C9, typename t_C10, typename t_C11, typename t_C12, typename t_C13
	#define DTempArg1 t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8, t_C9, t_C10, t_C11, t_C12, t_C13
	#define DTempArg2 DTC(0,14), DTC(1,14), DTC(2,14), DTC(3,14), DTC(4,14), DTC(5,14), DTC(6,14), DTC(7,14), DTC(8,14), DTC(9,14), DTC(10,14), DTC(11,14), DTC(12,14), DTC(13,14)
	#define DTempArg3 {DTN(0), DTN(1), DTN(2), DTN(3), DTN(4), DTN(5), DTN(6), DTN(7), DTN(8), DTN(9), DTN(10), DTN(11), DTN(12), DTN(13)}
	DRegClassFunctionCallery(14, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(14, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8, t_C9, t_C10, t_C11, t_C12, t_C13>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3 
	#define DTempArg0 typename t_C0,typename t_C1, typename t_C2, typename t_C3, typename t_C4, typename t_C5, typename t_C6, typename t_C7, typename t_C8, typename t_C9, typename t_C10, typename t_C11, typename t_C12, typename t_C13, typename t_C14
	#define DTempArg1 t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8, t_C9, t_C10, t_C11, t_C12, t_C13, t_C14
	#define DTempArg2 DTC(0,15), DTC(1,15), DTC(2,15), DTC(3,15), DTC(4,15), DTC(5,15), DTC(6,15), DTC(7,15), DTC(8,15), DTC(9,15), DTC(10,15), DTC(11,15), DTC(12,15), DTC(13,15), DTC(14,15)
	#define DTempArg3 {DTN(0), DTN(1), DTN(2), DTN(3), DTN(4), DTN(5), DTN(6), DTN(7), DTN(8), DTN(9), DTN(10), DTN(11), DTN(12), DTN(13), DTN(14)}
	DRegClassFunctionCallery(15, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(15, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);

	#undef DTempArg4 
	#define DTempArg4 <t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8, t_C9, t_C10, t_C11, t_C12, t_C13, t_C14>
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3 
	#define DTempArg0 typename t_C0,typename t_C1, typename t_C2, typename t_C3, typename t_C4, typename t_C5, typename t_C6, typename t_C7, typename t_C8, typename t_C9, typename t_C10, typename t_C11, typename t_C12, typename t_C13, typename t_C14, typename t_C15
	#define DTempArg1 t_C0, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7, t_C8, t_C9, t_C10, t_C11, t_C12, t_C13, t_C14, t_C15
	#define DTempArg2 DTC(0,16), DTC(1,16), DTC(2,16), DTC(3,16), DTC(4,16), DTC(5,16), DTC(6,16), DTC(7,16), DTC(8,16), DTC(9,16), DTC(10,16), DTC(11,16), DTC(12,16), DTC(13,16), DTC(14,16), DTC(15,16)
	#define DTempArg3 {DTN(0), DTN(1), DTN(2), DTN(3), DTN(4), DTN(5), DTN(6), DTN(7), DTN(8), DTN(9), DTN(10), DTN(11), DTN(12), DTN(13), DTN(14), DTN(15)}
	DRegClassFunctionCallery(16, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);
	DRegClassFunctionCallerz(16, DTempArg0, DTempArg1, DTempArg1, DTempArg2, DTempArg0, DTempArg3);


	#undef DTempArg4 
	#undef DTempArg0 
	#undef DTempArg1 
	#undef DTempArg2 
	#undef DTempArg3 
	#undef DTC
	#undef DTN
	#undef DTempArgConv0
	}

	class CScriptUserClass : public CReferenceCount
	{
	public:
		int m_DataSize;
		virtual void Construct(void *_pMemory) pure;
		virtual void Destruct(void *_pMemory) pure;
		virtual void Copy(void *_pDest, const void *_pSource) pure;
	};

	typedef TPtr<CScriptUserClass> spCScriptUserClass;

	class CRegisterContext : public CReferenceCount
	{
	protected:

		CSymbol_NameSpace *m_pParent;


		class CFunctionSymbol : public CSymbol_Function
		{
		public:

			CFunctionCaller *m_pCaller;

			CFunctionSymbol(const char *_pName, CFunctionCaller *_pCaller) : CSymbol_Function(NULL, "")
			{
				m_pCaller = _pCaller;
				m_Name = _pName;	
				m_ClassType |= ESymbol_Function_Registered;

			}		

			~CFunctionSymbol()
			{
				delete m_pCaller;
			}

			void Call(CExecutionContext &_ExecutionContext, void *_pArgs, void *_pReturn)
			{
				m_pCaller->Call(_pArgs, _pReturn);
			}

			virtual void Evaluate(CExecutionContext &_Context);
		};

		class CClassSymbol : public CSymbol_Class
		{
		public:

			spCScriptUserClass m_spClassDesc;

			CClassSymbol(const char *_pName, spCScriptUserClass _spClassDesc, CContext *_pContext) : CSymbol_Class(NULL, "")
			{
				m_Name = _pName;	
				m_ClassType |= ESymbol_Class_Registered;
				m_spClassDesc = _spClassDesc;
				m_spRegContext = MNew(CRegisterContext);
				m_spRegContext->Create(this, _pContext);
				m_DataSize = _spClassDesc->m_DataSize;
			}		

			~CClassSymbol()
			{
				m_spRegContext = NULL;
			}

			TPtr<CRegisterContext> m_spRegContext;

			virtual void Construct(void *_pMemory)
			{
				//M_TRACEALWAYS("Construct 0x%08x\n", _pMemory);
				m_spClassDesc->Construct(_pMemory);
			}

			virtual void Destruct(void *_pMemory)
			{
				//M_TRACEALWAYS("Destruct 0x%08x\n", _pMemory);
				m_spClassDesc->Destruct(_pMemory);
			}

			virtual void Copy(void *_pDest, const void *_pSource)
			{
				//M_TRACEALWAYS("Copy 0x%08x = 0x%08x\n", _pDest, _pSource);
				m_spClassDesc->Copy(_pDest, _pSource);
			}
			
		};

		class CVariableSymbol : public CSymbol_Variable
		{
		public:
			CFunctionCaller *m_pSetFunction;
			CFunctionCaller *m_pGetFunction;

			CVariableSymbol(const char *_pName, CFunctionCaller *_pSetFunction, CFunctionCaller *_pGetFunction) : CSymbol_Variable(NULL, "")
			{
				m_Name = _pName;	
				m_pSetFunction = _pSetFunction;
				m_pGetFunction = _pGetFunction;
				m_ClassType |= ESymbol_Variable_Registered;
			}		

			~CVariableSymbol()
			{
				delete m_pSetFunction;
				delete m_pGetFunction;
			}

			virtual void Evaluate(CExecutionContext &_Context);
			virtual void EvaluateSet(CExecutionContext &_Context);
			
		};

		class CConstantSymbol : public CSymbol_Constant
		{
		public:
			CFunctionCaller *m_pGetFunction;

			CConstantSymbol(const char *_pName, CFunctionCaller *_pGetFunction) : CSymbol_Constant(NULL, "")
			{
				m_Name = _pName;	
				m_pGetFunction = _pGetFunction;
			}		

			~CConstantSymbol()
			{
				delete m_pGetFunction;
			}	

			virtual void Evaluate(CExecutionContext &_Context);

		};


		class CRegisteredSymbol
		{
		public:
			CSymbol_NameSpace *m_pParent;
			spCSymbol m_spSymbol;			
		};

		TArray<CRegisteredSymbol> m_lRegisteredSymbols;

		virtual void AddSymbol(spCSymbol _spSymbol);

		void AddFunction(const char *_pName, CFunctionCaller *_pFunction)
		{
			TPtr<CFunctionSymbol> spSymbol = MNew2(CFunctionSymbol, _pName, _pFunction);

			spCSymbol spClass = (CSymbol *)m_pParent->GetSymbol(_pFunction->m_pReturnType, true);
			if (!spClass || !(spClass->m_ClassType & ESymbol_Class))
				Error("AddFunction", "Must be a class");

			spSymbol->m_spReturnType = (CSymbol_Class *)(CSymbol*)spClass;

			spSymbol->m_splArgumentTypes.SetLen(_pFunction->m_nArguments);
			for (int i = 0; i < _pFunction->m_nArguments; ++i)
			{
				spClass = m_pParent->GetSymbol(_pFunction->m_pArgTypes[i], true);
				if (!spClass || !(spClass->m_ClassType & ESymbol_Class))
					Error("AddFunction", "Must be a class");
				spSymbol->m_splArgumentTypes[i] = (CSymbol_Class *)(CSymbol*)spClass;
			}
			spSymbol->m_nStackVariables = _pFunction->m_nArguments + 1;
			AddSymbol((CSymbol *)spSymbol);
		}

		CRegisterContext *AddClass(const char *_pName, spCScriptUserClass _spClassDesc)
		{
			CClassSymbol *pSymbol = MNew3(CClassSymbol, _pName, _spClassDesc, m_pContext);
			spCSymbol spSymbol = pSymbol;

			AddSymbol(spSymbol);

			return pSymbol->m_spRegContext;
		}

		void AddVariable(const char *_pName, CFunctionCaller *_pSetFunction, CFunctionCaller *_pGetFunction)
		{
			TPtr<CVariableSymbol> spSymbol = MNew3(CVariableSymbol, _pName, _pSetFunction, _pGetFunction);

			spCSymbol spClass = m_pParent->GetSymbol(_pGetFunction->m_pReturnType, true);

			if (!spClass || !(spClass->m_ClassType & ESymbol_Class))
				Error("AddVariable", "Must be a class");

			spSymbol->m_spClass = (CSymbol_Class *)(CSymbol*)spClass;

			AddSymbol((CSymbol *)spSymbol);
		}

		void AddConstant(const char *_pName, CFunctionCaller *_pGetFunction)
		{
			TPtr<CConstantSymbol> spSymbol = MNew2(CConstantSymbol, _pName, _pGetFunction);

			spCSymbol spClass = m_pParent->GetSymbol(_pGetFunction->m_pReturnType, true);

			if (!spClass || !(spClass->m_ClassType & ESymbol_Class))
				Error("AddVariable", "Must be a class");

			spSymbol->m_spClass = (CSymbol_Class *)(CSymbol*)spClass;

			AddSymbol((CSymbol *)spSymbol);
		}

		void AddOperator(CStr _Name, CSymbol_Function *_pFunction);
		

		void AddOperator(const char *_pName, int _OperatorType, CFunctionCaller *_pFunction)
		{
			CStr SymbolName = CStrF("%d", _OperatorType);
			for (int i = 0; i < _pFunction->m_nArguments; ++i)
			{
				SymbolName += '@';
				SymbolName += _pFunction->m_pArgTypes[i];
			}

			TPtr<CFunctionSymbol> spSymbol = MNew2(CFunctionSymbol, SymbolName, _pFunction);

			spCSymbol spClass = (CSymbol *)m_pParent->GetSymbol(_pFunction->m_pReturnType, true);
			if (!spClass || !(spClass->m_ClassType & ESymbol_Class))
				Error("AddFunction", "Must be a class");

			spSymbol->m_spReturnType = (CSymbol_Class *)(CSymbol*)spClass;

			spSymbol->m_splArgumentTypes.SetLen(_pFunction->m_nArguments);
			for (int i = 0; i < _pFunction->m_nArguments; ++i)
			{
				spClass = m_pParent->GetSymbol(_pFunction->m_pArgTypes[i], true);
				if (!spClass || !(spClass->m_ClassType & ESymbol_Class))
					Error("AddFunction", "Must be a class");
				spSymbol->m_splArgumentTypes[i] = (CSymbol_Class *)(CSymbol*)spClass;
			}
			spSymbol->m_nStackVariables = _pFunction->m_nArguments + 1;

			CStr Operator;
			Operator.Capture(_pName);
			AddOperator(Operator, (CSymbol_Function *)spSymbol);
		}

		void AddConversion(CFunctionCaller *_pFunction)
		{			
			if (strcmp(_pFunction->m_pArgTypes[0], _pFunction->m_pReturnType) == 0)
			{
				delete _pFunction;
				return;
			}

			CStr SymbolName = CStrF("@Convert@%s@%s", _pFunction->m_pArgTypes[0], _pFunction->m_pReturnType);

			AddFunction(SymbolName, _pFunction);
		}

	public:

		~CRegisterContext();

		CContext *m_pContext;

		void Create(CSymbol_NameSpace *_pParent, CContext *_pContext)
		{
			m_pContext = _pContext;
			m_pParent = _pParent;
		}

		template <typename t_CType>
		class TCAutoRegClass : public CScriptUserClass
		{
		public:
			TCAutoRegClass()
			{
				m_DataSize = sizeof(t_CType);
			}

			virtual void Construct(void *_pMemory)
			{

				new(_pMemory) t_CType;
			}

			virtual void Destruct(void *_pMemory)
			{
				((t_CType *)_pMemory)->~t_CType();
			}

			virtual void Copy(void *_pDest, const void *_pSource)
			{
				*((t_CType *)_pDest) = *((const t_CType *)_pSource);
			}
		};

		//////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////

		template <typename t_CClassName>
		CRegisterContext *RegClass(const char *_pName)
		{
			return AddClass(_pName, MNew(TCAutoRegClass<t_CClassName>));
		}

		CRegisterContext *RegClass(const char *_pName, spCScriptUserClass _spClassDesc)
		{
			return AddClass(_pName, _spClassDesc);
		}

		//////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////

		template <typename t_CClassName, typename t_CFunctionPtr>
		void RegFunction(const char *_pName, t_CClassName *_pThis, t_CFunctionPtr _FuncPtr)
		{
			AddFunction(_pName, NFunctionConvert::GetFunctionCallerClass(_pThis, _FuncPtr));
		}

		template <typename t_CFunctionPtr>
		void RegFunction(const char *_pName, t_CFunctionPtr _FuncPtr)
		{
			AddFunction(_pName, NFunctionConvert::GetFunctionCaller(_FuncPtr));
		}

		//////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////

		template <typename t_CVarType0, typename t_CVarType1>
		void RegConversion(t_CVarType0 ( *_FuncPtr)(t_CVarType1))
		{
			AddConversion(NFunctionConvert::GetFunctionCaller(_FuncPtr));
		}

		template <typename t_CFrom, typename t_CTo>
		class TCConversion : public CFunctionCaller
		{
		public:		
			TCConversion()
			{
				m_pReturnType = ScriptTypeName<t_CTo>();
				static const char *pType = ScriptTypeName<t_CFrom>();
				m_pArgTypes = &pType;
				m_nArguments = 1;
			}

			void Call(void *_pArgs, void *_pReturn)
			{
				*((t_CTo *)*((void **)_pReturn)) = (t_CTo)(*((t_CFrom *)*((void **)_pArgs)));
			}
		};

		template <typename t_CVarType0, typename t_CVarType1>
		void RegConversion()
		{
			typedef TCConversion<t_CVarType0, t_CVarType1> CConv;
			AddConversion(DNew(CConv) TCConversion<t_CVarType0, t_CVarType1>);
		}

		//////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////

		template <typename t_CVarType0, typename t_CVarType1>
		void RegUnaryPrefixOperator(const char *_pName, t_CVarType0 ( *_FuncPtr)(t_CVarType1))
		{
			AddOperator(_pName, EOperatorType_UnaryPrefix, NFunctionConvert::GetFunctionCaller(_FuncPtr));
		}
		
		template <typename t_CVarType0, typename t_CVarType1>
		void RegVarPostfixOperator(const char *_pName, t_CVarType0 ( *_FuncPtr)(t_CVarType1))
		{
			AddOperator(_pName, EOperatorType_VariablePostfix, NFunctionConvert::GetFunctionCaller(_FuncPtr));
		}

		template <typename t_CVarType0, typename t_CVarType1>
		void RegVarPrefixOperator(const char *_pName, t_CVarType0 ( *_FuncPtr)(t_CVarType1))
		{
			AddOperator(_pName, EOperatorType_VariablePrefix, NFunctionConvert::GetFunctionCaller(_FuncPtr));
		}
		
		template <typename t_CVarType0, typename t_CVarType1, typename t_CVarType2>
		void RegBinaryOperator(const char *_pName, t_CVarType0 ( *_FuncPtr)(t_CVarType1, t_CVarType2))
		{
			AddOperator(_pName, EOperatorType_Binary, NFunctionConvert::GetFunctionCaller(_FuncPtr));
		}

		template <typename t_CVarType0, typename t_CVarType1, typename t_CVarType2>
		void RegAssignmentOperator(const char *_pName, t_CVarType0 ( *_FuncPtr)(t_CVarType1, t_CVarType2))
		{
			AddOperator(_pName, EOperatorType_Assignment, NFunctionConvert::GetFunctionCaller(_FuncPtr));
		}

		//////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////

		// Varibale is read with get and written to with set
		template <typename t_CVarType>
		void RegVariable(const char *_pName, t_CVarType ( *_pGetFunc)(), void ( *_pSetFunc)(t_CVarType))
		{
			AddVariable(_pName, NFunctionConvert::GetFunctionCaller(_pGetFunc), NFunctionConvert::GetFunctionCaller(_pSetFunc));
		}

		template <typename t_CGetType>
		class TVariableGet : public CFunctionCaller
		{
		public:		
			t_CGetType *m_pType;

			TVariableGet(t_CGetType *_pType)
			{
				m_pType = _pType;
				m_pReturnType = ScriptTypeName<t_CGetType>();
				m_pArgTypes = NULL;
				m_nArguments = 0;
			}

			void Call(void *_pArgs, void *_pReturn)
			{
				*((t_CGetType *)*((void **)_pReturn)) = *m_pType;
			}
		};

		template <typename t_CSetType>
		class TVariableSet : public CFunctionCaller
		{
		public:		
			t_CSetType *m_pType;

			TVariableSet(t_CSetType *_pType)
			{
				m_pType = _pType;
				m_pReturnType = NULL;
				static const char *pType = ScriptTypeName<t_CSetType>();
				m_pArgTypes = &pType;
				m_nArguments = 1;
			}

			void Call(void *_pArgs, void *_pReturn)
			{
				*m_pType = *((t_CSetType *)*((void **)_pArgs));
			}
		};


		// Variable is read from pointer and set with function
		template <typename t_CVarType, typename t_CVarType1>
		void RegVariable(const char *_pName, t_CVarType &_Variable, void ( *_pSetFunc)(t_CVarType1))
		{
			AddVariable(_pName, DNew(TVariableGet<t_CVarType>) TVariableGet<t_CVarType>(&_Variable), NFunctionConvert::GetFunctionCaller(_pSetFunc));
		}

		// Variable is read to and written to directly to pointer
		template <typename t_CVarType>
		void RegVariable(const char *_pName, t_CVarType &_Variable)
		{
			AddVariable(_pName, DNew(TVariableGet<t_CVarType>) TVariableGet<t_CVarType>(&_Variable), new TVariableSet<t_CVarType>(&_Variable));
		}

		// Varibale is read with get and written to with set
		template <typename t_CClassName, typename t_CVarType>
		void RegVariable(const char *_pName, t_CClassName *_pThis, t_CVarType ( *_pGetFunc)(), void ( t_CClassName::*_pSetFunc)(t_CVarType))
		{
			AddVariable(_pName, NFunctionConvert::GetFunctionCallerClass(_pThis, _pGetFunc), NFunctionConvert::GetFunctionCallerClass(_pThis, _pSetFunc));
		}

		// Variable is read from pointer and set with function
		template <typename t_CClassName, typename t_CVarType, typename t_CVarType1>
		void RegVariable(const char *_pName, t_CClassName *_pThis, t_CVarType &_Variable, void ( t_CClassName::*_pSetFunc)(t_CVarType1))
		{
			AddVariable(_pName, DNew(TVariableGet<t_CVarType>) TVariableGet<t_CVarType>(&_Variable), NFunctionConvert::GetFunctionCallerClass(_pThis, _pSetFunc));
		}

		//////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////

		template <typename t_CConstant>
		class TConstant : public CFunctionCaller
		{
		public:		
			t_CConstant m_Constant;

			TConstant(t_CConstant &_Constant)
			{
				m_Constant = _Constant;
				m_pReturnType = ScriptTypeName<t_CConstant>();
				m_pArgTypes = NULL;
				m_nArguments = 0;
			}

			void Call(void *_pArgs, void *_pReturn)
			{
				*((t_CConstant *)*((void **)_pReturn)) = m_Constant;
			}
		};

		// Variable is read from pointer and set with function
		template <typename t_CVarType>
		void RegConstant(const char *_pName, t_CVarType _Variable)
		{
			AddConstant(_pName, DNew(TConstant<t_CVarType>) TConstant<t_CVarType>(_Variable));
		}

		template <typename t_CVarType>
		void RegConstant(const char *_pName, t_CVarType ( *_pGetFunc)())
		{
			AddConstant(_pName, NFunctionConvert::GetFunctionCaller(_pGetFunc));
		}

		template <typename t_CClassName, typename t_CVarType>
		void RegConstant(const char *_pName, t_CClassName *_pThis, t_CVarType ( *_pGetFunc)())
		{
			AddConstant(_pName, NFunctionConvert::GetFunctionCallerClass(_pThis, _pGetFunc));
		}

		CSymbol_NameSpace *GetSymbol()
		{
			return m_pParent;
		}

		//////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////

	//	void RegFunction(const char *CommandSpec, void *Command, eCallType CType=STANDARD, uint8 ReturnType=VOID_ID);
	//	void RegConstant(const char *_pName, const CScriptMultiType& _Value);



	};

	typedef TPtr<CRegisterContext> spCRegisterContext;
}

typedef NScript::CRegisterContext CScriptRegisterContext;
typedef NScript::spCRegisterContext spCScriptRegisterContext;
#endif //DInc_MScript_RegisterContext_h
