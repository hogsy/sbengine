#include "PCH.h"

void Prefetch8(void* _Ptr)
{
#ifdef NEVER
	__asm
	{
		mov esi, [_Ptr];
		prefetcht0 [esi]
	}
#endif
}

