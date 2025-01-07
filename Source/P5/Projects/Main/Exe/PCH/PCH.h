#include "../../../../Shared/Mos/MMain.h"

// #define NOMOS

#ifdef PLATFORM_XBOX
	#include <xtl.h>
	#ifdef PLATFORM_XBOX1
		#ifndef M_DEMO_XBOX
			#include <Xonline.h>
		#ifdef M_Profile
			#pragma comment(lib, "xonline.lib")
		#else
			#pragma comment(lib, "xonlinels.lib")
		#endif
	#endif

#endif

#elif defined PLATFORM_WIN_PC
	#include <windows.h>
	#include <vfw.h>
	#include "../../../../Shared/Mos/Classes/Video/MVideo.h"

#else
	#ifdef PLATFORM_SHINOBI
		#include <shinobi.h>
		#include <sg_syhw.h>
		#include <usrsnasm.h>
		#include <NMWException.h>
		#include <actypes.h>
		#include <cw_malloc.h>
	#endif
#endif

#include "../../../../Shared/Mos/MOS.h"
#include "../../../../Shared/Mos/Classes/Render/MRenderCapture.h"
#include "../../../../Shared/MOS/Classes/Win/MWinGrph.h"
#include "../../../../Shared/Mos/Classes/Render/MRenderUtil.h"
#include "../../../../Shared/Mos/XR/XRVertexBuffer.h"
#include "../../../../Shared/Mos/XR/XRVBManager.h"
#include "MFloat.h"

#include "../../../../Shared/Mos/Classes/GameContext/WGameContext.h"
#include "../../../../Shared/Mos/Classes/GameWorld/Server/WServer.h"
#include "../../../../Shared/Mos/Classes/GameWorld/WMapData.h"
//#include "../../../../Shared/Mos/XRModels/Model_BSP/WBSPDef.h"

#include "../WGameContextMain.h"

#ifdef PLATFORM_XBOX1
#include "../../../../Shared/MOS/RndrXbox/MRndrXbox.h"
#endif

#ifdef PLATFORM_XBOX
#include "../../GameClasses/PCH/PCH.h"
#include "../../GameWorld/PCH/PCH.h"
#endif


