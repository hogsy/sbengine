
;-----------------------------------------------------
PUBLIC AMD64_GetRBP
PUBLIC AMD64_GetRDTSC

;-----------------------------------------------------
;_DATA SEGMENT DWORD PUBLIC USE32 'DATA'

;_DATA ENDS 

;-----------------------------------------------------
AMD64_CODE SEGMENT READ EXECUTE ALIGN(16)

;-----------------------------------------------------
AMD64_GetRBP PROC
; C

	mov rax, rbp
	ret

AMD64_GetRBP ENDP

AMD64_GetRDTSC PROC
; C

	rdtsc
;	mov dword ptr [rax], eax
;	mov dword ptr [rax+4], edx
	ret

AMD64_GetRDTSC ENDP

AMD64_CODE ENDS 

END
