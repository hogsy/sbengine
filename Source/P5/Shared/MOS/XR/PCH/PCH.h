
#ifndef	COMPILER_CODEWARRIOR

#include "MRTC.h"
#include "MMath.h"
#include "MMath_Vec128.h"
#include "../../MSystem/MSystem.h"
#include "../../MOS.h"
#include "../../XRModels/Model_TriMesh/WTriMesh.h"
#include "../../XRModels/Model_TriMesh/XMDCommn.h"
#include "../../XRModels/Model_BSP/WBSPModel.h"
#include "../../XRModels/Model_BSP/XWCommon.h"
//#include "../../XRModels/Model_Sky/WSky.h"
#include "../../Classes/BitString/MBitString.h"
#include "../XR.h"
//#include "../../XRModels/Model_BSP2/WBSP2Def.h"
//#include "../../XRModels/Model_BSP2/WBSP2Model.h"

#include "../Solids/MCSolid.h"
#ifndef PLATFORM_CONSOLE
#include "../Solids/XWSolid.h"
#endif
#include "../../XRModels/Model_BSP2/XW2Common.h"
#include "../../XRModels/Model_Flare/WModel_Flare.h"
#include "../../XRModels/Model_TriMesh/WTriMesh.h"
#include "../../XRModels/Model_TriMesh/XMDCommn.h"
#include "../../../MCC/MRTC.h"
#include "../XRAnim.h"
#include "../XRSkeleton.h"
#include "../XRBlockNavGrid.h"
#include "../XREngine.h"
#include "../XREngineVar.h"
#include "../XRFog.h"
#include "../XRPContainer.h"
#include "../XRSurf.h"
#include "../XRSurfaceContext.h"
#include "../XRVBContext.h"
#include "../XRVBManager.h"
#include "../XRVBOperators.h"
#include "../XRVBPrior.h"
//#include "../../Classes/Render/OgrRender.h"

#ifdef PLATFORM_XBOX1
#include "../../../MOS/RndrXbox/MRndrXbox.h"
#endif

#endif
