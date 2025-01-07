
;-----------------------------------------------------
PUBLIC AMD64_RunProc_int
PUBLIC AMD64_RunProc_float
PUBLIC AMD64_RunProc_double
PUBLIC AMD64_RunProc_ptr

;-----------------------------------------------------
AMD64_RDATA SEGMENT READ ALIGN(16)

Const EQU 80

AMD64_RDATA ENDS 

;-----------------------------------------------------
AMD64_CODE SEGMENT READ EXECUTE ALIGN(16)

;-----------------------------------------------------
;int AMD64_RunProc_int(void* _pOwner, void* _pCommand, CCallStackEntry* _pCallStack, int _Params);
;{
;	void* valueptr;
;	int type;
;	for (int t=0; t<_Params; t++)
;	{
;		type=(_pCallStack->m_Pos) & 0xffff;
;		valueptr=_pCallStack->m_pValue;
;		switch(type)
;		{
;			case FORCE_INTEGER_ID:
;				__asm 
;				{
;					mov eax,valueptr
;					push dword ptr [eax]
;				}
;				break;
;
;			case FORCE_STRING_ID:
;				__asm push dword ptr [valueptr];
;				break;
;
;			case FORCE_FLOAT_ID:
;				__asm 
;				{
;					mov eax,valueptr
;					push dword ptr [eax]
;				}
;				break;
;
;			case FORCE_DOUBLE_ID:
;				__asm 
;				{
;					mov eax,valueptr
;					push dword ptr [eax+4]
;					push dword ptr [eax]
;				}
;				break;
;		};
;		_pCallStack--;
;	}
;
;	int _tmp_storage;
;	__asm 
;	{
;		mov ecx,dword ptr [_pOwner]
;		call dword ptr [_pCommand]
;		mov [_tmp_storage], eax
;	}
;
;	return _tmp_storage;
;}

;class CCallStackEntry
;{
;public:
;	mint m_Pos;
;	void* m_pValue;
;};

FORCE_INTEGER_ID EQU	20
FORCE_FLOAT_ID	EQU	21
FORCE_DOUBLE_ID	EQU	22
FORCE_STRING_ID	EQU	23


CCallStackEntry STRUCT
  m_Pos		QWORD	?
  m_pValue  QWORD	?
CCallStackEntry ENDS

PDWORD TYPEDEF PTR DWORD
PCCallStackEntry TYPEDEF PTR CCallStackEntry


AMD64_RunProc_int PROC USES r10 r11 r12 r13 r14 r15
	LOCAL _pOwner:PTR, _pCommand:PTR, _pCallStack:PCCallStackEntry, _Params:QWORD 

;	mov _pOwner, rcx // not needed
	mov _pCommand, rdx
	mov _pCallStack, r8
;	mov _Params, r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	mov r11, r9
	sub r11, 1
	cmp r11, 0
	jl ParamsEnd
ParamsLoop:
		mov r10, _pCallStack
		mov r12, (CCallStackEntry PTR[r10]).m_pValue
		mov r13, (CCallStackEntry PTR[r10]).m_Pos
		mov r14, 0ffffh
		and r13, r14
		cmp r11, 2
		jg StackParams
			; ///////////////////  FORCE_INTEGER_ID
			cmp r13, FORCE_INTEGER_ID
			jne StringIdReg
				cmp r11, 0
				jne	IntIdReg0
				mov edx, DWORD PTR [r12]
				jmp StackParamsEnd
				IntIdReg0:
				cmp r11, 1
				jne	IntIdReg1
				mov r8d, DWORD PTR [r12]
				jmp StackParamsEnd
				IntIdReg1:
				mov r9d, DWORD PTR [r12]
			jmp StackParamsEnd

			; ///////////////////  FORCE_STRING_ID
			StringIdReg:
			cmp r13, FORCE_STRING_ID
			jne FloatIdReg
				cmp r11, 0
				jne	StringIdReg0
				mov rdx, r12
				jmp StackParamsEnd
				StringIdReg0:
				cmp r11, 1
				jne	StringIdReg1
				mov r8, r12
				jmp StackParamsEnd
				StringIdReg1:
				mov r9, r12
			jmp StackParamsEnd
			
			; ///////////////////  FORCE_FLOAT_ID
			FloatIdReg:			
			cmp r13, FORCE_FLOAT_ID
			jne DoubleIdReg
				cmp r11, 0
				jne	FloatIdRegReg0
				movss xmm1, REAL4 PTR [r12]
				jmp StackParamsEnd
				FloatIdRegReg0:
				cmp r11, 1
				jne	FloatIdRegReg1
				movss xmm2, REAL4 PTR [r12]
				jmp StackParamsEnd
				FloatIdRegReg1:
				movss xmm3, REAL4 PTR [r12]
			jmp StackParamsEnd

			; ///////////////////  FORCE_DOUBLE_ID
			DoubleIdReg:			
			cmp r13, FORCE_DOUBLE_ID
			jne StackParamsEnd
				cmp r11, 0
				jne	DoubleIdReg0
				movsd xmm1, REAL8 PTR [r12]
				jmp StackParamsEnd
				DoubleIdReg0:
				cmp r11, 1
				jne	DoubleIdReg1
				movsd xmm2, REAL8 PTR [r12]
				jmp StackParamsEnd
				DoubleIdReg1:
				movsd xmm3, REAL8 PTR [r12]
			jmp StackParamsEnd
		StackParams:
		int 3
		StackParamsEnd:
	mov r15, -SIZEOF CCallStackEntry
	mov r14, _pCallStack
	add r14, r15
	mov _pCallStack, r14
	dec r11
	cmp r11, 0
	jge ParamsLoop
	ParamsEnd:
	
	;mov rcx, _pOwner	 // Already in rcx
	call _pCommand
	
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	ret
	
AMD64_RunProc_int ENDP

;-----------------------------------------------------
;float AMD64_RunProc_float(void* _pOwner, void* _pCommand, CCallStackEntry* _pCallStack, int _Params);
AMD64_RunProc_float PROC

	int 3
	ret
AMD64_RunProc_float ENDP

;-----------------------------------------------------
;double AMD64_RunProc_double(void* _pOwner, void* _pCommand, CCallStackEntry* _pCallStack, int _Params);
AMD64_RunProc_double PROC

	int 3

	ret
AMD64_RunProc_double ENDP

;-----------------------------------------------------
;void *AMD64_RunProc_ptr(void* _pOwner, void* _pCommand, CCallStackEntry* _pCallStack, int _Params);
AMD64_RunProc_ptr PROC

	int 3

	ret
AMD64_RunProc_ptr ENDP

AMD64_CODE ENDS 

END
