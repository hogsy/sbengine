
#ifdef PLATFORM_PS3

M_INLINE int64 MRTC_SystemInfo::CPU_Clock()
{
	int64 result;
	 __asm__ volatile("1: mftb %[current_tb];"
					"cmpwi 7, %[current_tb], 0;"
					"beq-  7, 1b;"
     : [current_tb] "=&b" (result):
     :"cr7");

//	int64 result;
//	asm volatile("mftb %0": [result] "=b"(result));
	return result;
}

M_INLINE int64 MRTC_SystemInfo::OS_Clock()
{
	int64 result;
	 __asm__ volatile("1: mftb %[current_tb];"
					"cmpwi 7, %[current_tb], 0;"
					"beq-  7, 1b;"
     : [current_tb] "=r" (result):
     :"cr7");

//	asm volatile("mftb %0": [result] "=b"(result));
	return result;
}

M_INLINE int32 MRTC_SystemInfo::Atomic_Increase(volatile int32 *_pDest)
{
	int32 result, temp;
	asm volatile(
	".Latomic_inc_loop%=:\n"
		"lwarx %0, 0, %2\n"
		"addi %1, %0, 1\n"
		"stwcx. %1, 0, %2\n"
		"bne-	.Latomic_inc_loop%=\n"
		: "=&b"(result), "=&r"(temp)
		: "r"(_pDest)
		: "memory"
		);
	return result;
}

M_INLINE int32 MRTC_SystemInfo::Atomic_Decrease(volatile int32 *_pDest)
{
	int32 result, temp;
	asm volatile(
	".Latomic_dec_loop%=:\n"
		"lwarx %0, 0, %2\n"
		"subi %1, %0, 1\n"
		"stwcx. %1, 0, %2\n"
		"bne-	.Latomic_dec_loop%=\n"
		: "=&b"(result), "=&r"(temp)
		: "r"(_pDest)
		: "memory"
		);
	return result;
}

M_INLINE int32 MRTC_SystemInfo::Atomic_Add(volatile int32 *_pDest, int32 _Add)
{
	int32 result, temp;
	asm volatile(
	".Latomic_add_loop%=:\n"
		"lwarx %0, 0, %2\n"
		"add %1, %0, %2\n"
		"stwcx. %1, 0, %2\n"
		"bne-	.Latomic_add_loop%=\n"
		: "=&b"(result), "=&r"(temp)
		: "r"(_pDest)
		: "memory"
		);
	return result;
}

M_INLINE int32 MRTC_SystemInfo::Atomic_IfEqualExchange(volatile int32 *_pDest, int32 _CompareTo, int32 _SetTo)
{
	int32 result;
	asm volatile(
	".Latomic_iex_loop%=:\n"
		"lwarx %0, 0, %1\n"
		"cmpw %0, %2\n"
		"bne- .Latomic_iex_done%=\n"
		"stwcx. %3, 0, %1\n"
		"bne- .Latomic_iex_loop%=\n"
	".Latomic_iex_done%=:\n"
		: "=&b"(result)
		: "r"(_pDest), "r"(_CompareTo), "r"(_SetTo)
		: "memory"
		);
	
	return result;
}

M_INLINE int32 MRTC_SystemInfo::Atomic_Exchange(volatile int32 *_pDest, int32 _SetTo)
{
	int32 result;
	asm volatile(
	".Latomic_xch_loop%=:\n"
		"lwarx %0, 0, %1\n"
		"stwcx. %2, 0, %1\n"
		"bne-	.Latomic_xch_loop%=\n"
		: "=&b"(result)
		: "r"(_pDest), "r"(_SetTo)
		: "memory"
		);
	return result;
}

M_INLINE void* MRTC_SystemInfo::OS_GetThreadID()
{
//	uint64 result;
//	asm volatile("mr %0, 13" : "=&b"(result));
	void* result;
	asm volatile("clrldi %0, 13, 32" : "=&b"(result));
	return result;
}

#endif
