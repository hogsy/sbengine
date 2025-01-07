
class MRTC_SystemInfo
{
public:

	static int32 Atomic_Increase(volatile int32 *_pDest);
	static int32 Atomic_Decrease(volatile int32 *_pDest);
	static int32 Atomic_Exchange(volatile int32 *_pDest, int32 _SetTo);
	static int32 Atomic_Add(volatile int32 *_pDest, int32 _Add);
	static int32 Atomic_IfEqualExchange(volatile int32 *_pDest, int32 _CompareTo, int32 _SetTo);

	static void OS_Assert(const char*, const char* _pFile = NULL, int _Line = 0);
	static void M_ARGLISTCALL OS_Trace(const char *, ...);
	static void OS_TraceRaw(const char *);

};
