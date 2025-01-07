
#ifndef _INC_VPU_PLATFORM
#define _INC_VPU_PLATFORM


#ifdef TARGET_XENON
#include "VPUTarget_Xenon.h"

#elif defined TARGET_WIN32_X86
#include "VPUTarget_Win32.h"

#elif defined TARGET_WIN32_X86_SSE
#include "VPUTarget_Win32.h"

#elif defined TARGET_WIN32_X86_SSE2
#include "VPUTarget_Win32.h"

#elif defined TARGET_WIN64_AMD64
#include "VPUTarget_Win32.h"

#elif defined(TARGET_PS3_SPU) || defined(SN_TARGET_PS3_SPU)
#include "VPUTarget_SPU.h"

#else
#error "No target platform defined."

#endif


#endif

