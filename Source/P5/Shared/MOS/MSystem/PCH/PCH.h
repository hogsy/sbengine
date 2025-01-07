
#define JPEG_INTERNALS

#include "MRTC.h"
/*#ifdef _BASETSD_H_
#error "LKhKLJ"
#endif*/
#include "../SysInc.h"

#ifndef IMAGE_IO_NOJPG
#include "../Raster/JPGLib/jinclude.h"
#include "../Raster/JPGLib/jpeglib.h"
#endif
#include "MCC.h"
#include "../Raster/MDisplay.h"
#include "../Raster/MImage.h"
#include "../Raster/MRender.h"
#include "../Raster/MTexture.h"
#include "../Misc/MConsole.h"
#include "../Misc/MRegistry.h"
#include "../Script/MScript.h"
#include "MNetwork.h"
#include "../Input/MInput.h"
#include "../Input/MInputScankey.h"
#include "../MSystem.h"
