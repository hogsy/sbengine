
#include "MCC.h"
#include "MFloat.h"
#include "MMath_Vec128.h"
#include "../../../../Shared/MOS/MOS.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"
#include "../../../../Shared/MOS/Classes/GameWorld/Client/WClient.h"
#include "../../../../Shared/MOS/Classes/GameWorld/Server/WServer.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WMapData.h"

#include "../../../../Shared/MOS/Classes/GameWorld/WDataRes_Sound.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"
#include "../../../../Shared/MOS/Classes/GameWorld/Client/WClient_Core.h"

#include "../WObj_Char.h"

//#include "../../GameWorld/WClientMod.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WDataRes_Sound.h"
#include "../../../../Shared/MOS/XR/XRBlockNav.h"

#include "../WObj_Game/WObj_GameMessages.h"

#include "../WRPG/WRPGItem.h"
#include "../WRPG/WRPGInitParams.h"

#include "../WObj_AI/AICore.h"
#include "../WObj_AI/WObj_Aux/WObj_Team.h"

#ifdef PLATFORM_XBOX1
#include "../../../../Shared/MOS/RndrXbox/MRndrXbox.h"
#endif
