#ifndef __MRENDERPS3_CELLWRAPPERS_H_INCLUDED
#define __MRENDERPS3_CELLWRAPPERS_H_INCLUDED

#if defined(CELL_SDK_VERSION) && (CELL_SDK_VERSION < 0x090000)
#define OLD_SDK
#define POST_090_SDK_FUNC
#define POST_090_SDK_FUNC_NO_ARG
#else
#define POST_090_SDK_FUNC			gCellGcmCurrentContext, 
#define POST_090_SDK_FUNC_NO_ARG	gCellGcmCurrentContext
#endif

#ifdef M_RTM
	#define WRAPPERINLINE M_FORCEINLINE
#else
	#define WRAPPERINLINE static
#endif


#define GCMSHADOW(a, b)
//#define GCMSHADOW(a, b) gs_GCMShadow.a.Shadow b;


extern void* ms_pRCOwner;

#define GCMSYNC
//#define GCMSYNC {gcmFinish();}
//#define GCMSYNC if(gs_GCMSync) {gcmFinish();}

#define GCM_NOTTHREADSAFE
//#define GCM_NOTTHREADSAFE M_ASSERT(MRTC_SystemInfo::OS_GetThreadID() == ms_pRCOwner, "!");

#define GCMCALLGRAPH(x)
//#define GCMCALLGRAPH(x) if(gs_GCMGraph) {M_TRACEALWAYS("[GCM-GRAPH]: "); M_TRACEALWAYS x; M_TRACEALWAYS("\n");}

enum
{
	CELL_GCM_CLEAR_RGBA	= (CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A),
	CELL_GCM_CLEAR_RGB	= (CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B),
};


class CGCMShadow
{
public:
	CGCMShadow()
	{
		memset(this, 0, sizeof(*this));
	}
	struct  
	{
		uint16	m_lFrequency[16];
		uint8	m_lStride[16];
		uint8	m_lSize[16];
		uint8	m_lType[16];
		uint8	m_lLocation[16];
		uint32	m_lOffset[16];

		void Shadow(uint32 _Index, uint16 _Frequency, uint8 _Stride, uint8 _Size, uint8 _Type, uint8 _Location, uint32 _Offset)
		{
			m_lFrequency[_Index] = _Frequency;
			m_lStride[_Index] = _Stride;
			m_lSize[_Index] = _Size;
			m_lType[_Index] = _Type;
			m_lLocation[_Index] = _Location;
			m_lOffset[_Index] = _Offset;
		}
	} m_VertexDataArray;

	struct
	{
		uint8 m_Mode;
		uint32 m_Count;
		uint8 m_Type;
		uint8 m_Location;
		uint32 m_Offset;

		void Shadow(const uint8 mode, uint32 count, const uint8 type, const uint8 location, const uint32 indicies)
		{
			m_Mode = mode;
			m_Count = count;
			m_Type = type;
			m_Location = location;
			m_Offset = indicies;
		}
	} m_DrawIndexArray;

	struct
	{
		uint8 m_Mode;
		uint32 m_First;
		uint32 m_Count;

		void Shadow(uint8 _Mode, uint32 _First, uint32 _Count)
		{
			m_Mode = _Mode;
			m_First = _First;
			m_Count = _Count;
		}
	} m_DrawArrays;
};

extern CGCMShadow gs_GCMShadow;
extern uint32 gs_GCMSync;
extern uint32 gs_GCMGraph;

WRAPPERINLINE uint32 gcmInit(const uint32 cmdSize, const uint32 ioSize, const void *ioAddress)
{
	GCMCALLGRAPH( ("gcmInit(%.8x, %.8x, %.8x", cmdSize, ioSize, ioAddress) )
	GCM_NOTTHREADSAFE
	return cellGcmInit(cmdSize, ioSize, ioAddress);
}

WRAPPERINLINE void gcmGetConfiguration(CellGcmConfig *_pConfig)
{
	GCMCALLGRAPH( ("gcmGetConfiguration(%.8x)", _pConfig) )
	GCM_NOTTHREADSAFE
	cellGcmGetConfiguration(_pConfig);
}

WRAPPERINLINE uint32 gcmSetDisplayBuffer(const uint8 id, const uint32 offset, const uint32 pitch, const uint32 width, const uint32 height)
{
	GCMCALLGRAPH( ("gcmSetDisplayBuffer(%x, %.8x, %.8x, %.4x, %.4x)", id, offset, pitch, width, height) )
	GCM_NOTTHREADSAFE
	return cellGcmSetDisplayBuffer(id, offset, pitch, width, height);
}

WRAPPERINLINE void gcmFlush()
{
	GCMCALLGRAPH( ("gcmFlush()") )
	GCM_NOTTHREADSAFE
	cellGcmFlush(POST_090_SDK_FUNC_NO_ARG);
}

WRAPPERINLINE void gcmFinish()
{
	GCMCALLGRAPH( ("gcmFinish()") )
	GCM_NOTTHREADSAFE
#ifdef OLD_SDK
	cellGcmFinish();
#else
	cellGcmFinish(POST_090_SDK_FUNC 1);
	cellGcmFinish(POST_090_SDK_FUNC 0);
#endif
}

WRAPPERINLINE uint32 gcmSetFlip(const uint8 id)
{
	GCMCALLGRAPH( ("gcmSetFlip(%x)", id) )
	GCM_NOTTHREADSAFE
	uint32 ret = cellGcmSetFlip(POST_090_SDK_FUNC id);
	GCMSYNC

	return ret;
}

WRAPPERINLINE void gcmSetFlipMode(const uint32 mode)
{
	GCMCALLGRAPH( ("gcmSetFlipMode(%.8x)", mode) )
	GCM_NOTTHREADSAFE
	cellGcmSetFlipMode(mode);
	GCMSYNC
}

WRAPPERINLINE uint32 gcmGetFlipStatus()
{
	GCMCALLGRAPH( ("gcmGetFlipStatus()") )
	GCM_NOTTHREADSAFE
	return cellGcmGetFlipStatus();
}

WRAPPERINLINE void gcmResetFlipStatus()
{
	GCMCALLGRAPH( ("gcmResetFlipStatus()") )
	GCM_NOTTHREADSAFE
	cellGcmResetFlipStatus();
}

WRAPPERINLINE void gcmSetWaitFlip()
{
	GCMCALLGRAPH( ("gcmSetWaitFlip()") )
	GCM_NOTTHREADSAFE
	cellGcmSetWaitFlip(POST_090_SDK_FUNC_NO_ARG);
}

WRAPPERINLINE int32 gcmSetPrepareFlip(const uint8 id)
{
	GCMCALLGRAPH( ("gcmSetPrepareFlip(%x)", id) )
	GCM_NOTTHREADSAFE
	return cellGcmSetPrepareFlip(POST_090_SDK_FUNC id);
}

WRAPPERINLINE int32 gcmSetFlipImmediate(const uint8 id)
{
	GCMCALLGRAPH( ("gcmSetFlipImmediate(%x)", id))
	GCM_NOTTHREADSAFE
	return cellGcmSetFlipImmediate(id);
}

WRAPPERINLINE void gcmEmulateNewSwitch()
{
	GCMCALLGRAPH( ("gcmEmulateNewSwitch") )
	GCM_NOTTHREADSAFE
//	cellGcmEmulateNewSwitch(POST_090_SDK_FUNC_NO_ARG);
	GCMSYNC
}

WRAPPERINLINE void gcmSetWaitLabel(const uint8 index, const uint32 value)
{
	GCMCALLGRAPH( ("gcmSetWaitLabel(%x, %.8x)", index, value) )
	GCM_NOTTHREADSAFE
	cellGcmSetWaitLabel(POST_090_SDK_FUNC index, value);
	GCMSYNC
}

WRAPPERINLINE void gcmSetWriteBackEndLabel(const uint8 index, const uint32 value)
{
	GCMCALLGRAPH( ("gcmSetWriteBackEndLabel(%x, %.8x)", index, value) )
	GCM_NOTTHREADSAFE
	cellGcmSetWriteBackEndLabelInline(POST_090_SDK_FUNC index, value);
	GCMSYNC
}

WRAPPERINLINE void gcmSetWriteCommandLabel(const uint8 index, const uint32 value)
{
	GCMCALLGRAPH( ("gcmSetWriteCommandLabel(%x, %.8x)", index, value) )
	GCM_NOTTHREADSAFE
	cellGcmSetWriteCommandLabelInline(POST_090_SDK_FUNC index, value);
	GCMSYNC
}

WRAPPERINLINE void gcmSetWriteTextureLabel(const uint8 index, const uint32 value)
{
	GCMCALLGRAPH( ("gcmSetWriteTextureLabel(%x, %.8x)", index, value) )
	GCM_NOTTHREADSAFE
	cellGcmSetWriteTextureLabelInline(POST_090_SDK_FUNC index, value);
	GCMSYNC
}

WRAPPERINLINE uint32* gcmGetLabelAddress(const uint8 index)
{
	GCMCALLGRAPH( ("gcmGetLabelAddress(%x)", index) )
	return cellGcmGetLabelAddress(index);
}

WRAPPERINLINE uint32 gcmAddressToOffset(const void *address)
{
	GCMCALLGRAPH( ("gcmAddressToOffset(%.8x)", address) )
	uint32 Offset;
	if(cellGcmAddressToOffset(address, &Offset) != CELL_OK)
		M_BREAKPOINT;
	return Offset;
}

WRAPPERINLINE int32 gcmMapMainMemory(const void *address, const uint32 size, uint32 *offset)
{
	GCMCALLGRAPH( ("gcmMapMainMemory(%.8x, %.8x, %.8x)", address, size, offset) )
	GCM_NOTTHREADSAFE
	return cellGcmMapMainMemory(address, size, offset);
}

WRAPPERINLINE void gcmSetTile(const uint8 index, const uint8 location, const uint32 offset, const uint32 size, const uint32 pitch, const uint8 comp, const uint16 base, const uint8 bank)
{
	GCMCALLGRAPH( ("gcmSetTile(%x, %x, %.8x, %.8x, %.8x, %x, %.4x, %x)", index, location, offset, size, pitch, comp, base, bank) )
	GCM_NOTTHREADSAFE
	cellGcmSetTile(index, location, offset, size, pitch, comp, base, bank);
	GCMSYNC
}

WRAPPERINLINE void gcmSetInvalidateTile(const uint8 _iTile)
{
	GCMCALLGRAPH( ("gcmSetInvalidateTile(%x)", _iTile) )
	GCM_NOTTHREADSAFE
	cellGcmSetInvalidateTile(_iTile);
	GCMSYNC
}

WRAPPERINLINE void gcmSetZcull(uint32 _Index, uint32 _Offset, uint32 _Width, uint32 _Height, uint32 _CullStart, uint32 _ZFormat, uint32 _AAFormat, uint32 _ZCullDir, uint32 _ZCullFormat, uint32 _SFunc, uint32 _SRef, uint32 _SMask)
{
	GCMCALLGRAPH( ("gcmSetZcull(%x, %.8x, %.4x, %.4x, %.8x, %x, %x, %x, %x, %x, %x, %x)", _Index, _Offset, _Width, _Height, _CullStart, _ZFormat, _AAFormat, _ZCullDir, _ZCullFormat, _SFunc, _SRef, _SMask) )
	GCM_NOTTHREADSAFE
	cellGcmSetZcull(_Index, _Offset, _Width, _Height, _CullStart, _ZFormat, _AAFormat, _ZCullDir, _ZCullFormat, _SFunc, _SRef, _SMask);
	GCMSYNC
}

WRAPPERINLINE void gcmSetInvalidateZcull()
{
	GCMCALLGRAPH( ("gcmSetInvalidateZcull()") )
	GCM_NOTTHREADSAFE
	cellGcmSetInvalidateZcullInline(POST_090_SDK_FUNC_NO_ARG);
	GCMSYNC
}

WRAPPERINLINE void gcmSetZPassPixelCountEnable(const uint32 _Enable)
{
	GCMCALLGRAPH( ("gcmSetZPassPixelCountEnable(%x)", _Enable) )
	GCM_NOTTHREADSAFE
	cellGcmSetZpassPixelCountEnableInline(POST_090_SDK_FUNC _Enable);
	GCMSYNC
}

WRAPPERINLINE void gcmSetZcullStatsEnable(const uint32 _Enable)
{
	GCMCALLGRAPH( ("gcmSetZcullStatsEnable(%x)", _Enable) )
	GCM_NOTTHREADSAFE
	cellGcmSetZcullStatsEnableInline(POST_090_SDK_FUNC _Enable);
	GCMSYNC
}

WRAPPERINLINE void gcmSetClearReport(const uint32 _Type)
{
	GCMCALLGRAPH( ("gcmSetClearReport(%x)", _Type) )
	GCM_NOTTHREADSAFE
	cellGcmSetClearReportInline(POST_090_SDK_FUNC _Type);
	GCMSYNC
}

WRAPPERINLINE uint32 gcmGetReport(const uint32 _Type, const uint32 _ReportID)
{
	GCMCALLGRAPH( ("gcmGetReport(%x, %x)", _Type, _ReportID) )
	GCM_NOTTHREADSAFE
	uint32 value = cellGcmGetReport(_Type, _ReportID);
	GCMSYNC

	return value;
}

WRAPPERINLINE void gcmSetReport(const uint32 _Type, const uint32 _ReportID)
{
	GCMCALLGRAPH( ("gcmSetReport(%x, %x)", _Type, _ReportID) )
	GCM_NOTTHREADSAFE
	cellGcmSetReportInline(POST_090_SDK_FUNC _Type, _ReportID);
	GCMSYNC
}

WRAPPERINLINE void gcmSetUserCommand(const uint32 _Command)
{
	GCMCALLGRAPH( ("gcmSetUserCommand(%x)", _Command) )
	GCM_NOTTHREADSAFE
	cellGcmSetUserCommand(POST_090_SDK_FUNC _Command);
	GCMSYNC
}

WRAPPERINLINE void gcmSetUserHandler(void (*_pfnHandler)(uint32))
{
	GCMCALLGRAPH( ("gcmSetUserHandler(%.8x)", _pfnHandler) )
	GCM_NOTTHREADSAFE
	cellGcmSetUserHandler(_pfnHandler);
	GCMSYNC
}

//##########################################################################################

WRAPPERINLINE void gcmCgInitProgram(CGprogram _Prog)
{
	GCMCALLGRAPH( ("gcmCgInitProgram(%.8x)", _Prog) )
	GCM_NOTTHREADSAFE
	cellGcmCgInitProgram(_Prog);
}

WRAPPERINLINE void gcmCgGetUCode(CGprogram prog, void **pUCode, uint32 *pUCodeSize)
{
	GCMCALLGRAPH( ("gcmCgGetUCode(%.8x, %.8x, %.8x)", prog, pUCode, pUCodeSize) )
	GCM_NOTTHREADSAFE
	cellGcmCgGetUCode(prog, pUCode, pUCodeSize);
}

WRAPPERINLINE CGparameter gcmCgGetNamedParameter(CGprogram prog, const char *name)
{
	GCMCALLGRAPH( ("gcmCgGetNamedParameter(%.8x, %.8x)", prog, name) )
	GCM_NOTTHREADSAFE
	return cellGcmCgGetNamedParameter(prog, name);
}

WRAPPERINLINE CGresource gcmCgGetParameterResource(CGprogram prog, CGparameter param)
{
	GCMCALLGRAPH( ("gcmCgGetParameterResource(%.8x, %.8x)", prog, param) )
	GCM_NOTTHREADSAFE
	return cellGcmCgGetParameterResource(prog, param);
}

//##########################################################################################

WRAPPERINLINE void gcmSetVertexProgramConstants(uint32 _iStart, uint32 _nCount, const fp32* _pData)
{
	GCMCALLGRAPH( ("gcmSetVertexProgramConstants(%.8x, %.8x, %.8x)", _iStart, _nCount, _pData) )
	GCM_NOTTHREADSAFE
	cellGcmSetVertexProgramConstantsInline(POST_090_SDK_FUNC _iStart, _nCount, _pData);
	GCMSYNC
}

WRAPPERINLINE void gcmSetVertexProgram(const CGprogram prog, const void *ucode)
{
	GCMCALLGRAPH( ("gcmSetVertexProgram(%.8x, %.8x)", prog, ucode) )
	GCM_NOTTHREADSAFE
	cellGcmSetVertexProgram(POST_090_SDK_FUNC prog, ucode);
	GCMSYNC
}

WRAPPERINLINE void gcmSetVertexProgramParameter(const CGparameter param, const float *value)
{
	GCMCALLGRAPH( ("gcmSetVertexProgramParameter(%.8x, %.8x)", param, value) )
	GCM_NOTTHREADSAFE
	cellGcmSetVertexProgramParameter(POST_090_SDK_FUNC param, value);
	GCMSYNC
}

WRAPPERINLINE void gcmSetVertexProgramParameterBlock(const uint32 baseConst, uint32 constCount, const float *value)
{
	GCMCALLGRAPH( ("gcmSetVertexProgramParameterBlock(%.8x, %.8x, %.8x)", baseConst, constCount, value) )
	GCM_NOTTHREADSAFE
	cellGcmSetVertexProgramParameterBlockInline(POST_090_SDK_FUNC baseConst, constCount, value);
	GCMSYNC
}

WRAPPERINLINE void gcmSetFragmentProgram(const CGprogram prog, const uint32 offset)
{
	GCMCALLGRAPH( ("gcmSetFragmentProgram(%.8x, %.8x)", prog, offset) )
	GCM_NOTTHREADSAFE
	cellGcmSetFragmentProgram(POST_090_SDK_FUNC prog, offset);
	GCMSYNC
}

WRAPPERINLINE void gcmSetUpdateFragmentProgramParameter(const uint32 offset)
{
	GCMCALLGRAPH( ("gcmSetUpdateFragmentProgramParameter(%.8x)", offset) )
	GCM_NOTTHREADSAFE
	cellGcmSetUpdateFragmentProgramParameterInline(POST_090_SDK_FUNC offset);
	GCMSYNC
}

WRAPPERINLINE void gcmSetFragmentProgramParameter(const CGprogram prog, const CGparameter param, const float *value, const uint32 offset)
{
	GCMCALLGRAPH( ("gcmSetFragmentProgramParameter(%.8x, %.8x, %.8x, %.8x)", prog, param, value, offset) )
	GCM_NOTTHREADSAFE
	cellGcmSetFragmentProgramParameter(POST_090_SDK_FUNC prog, param, value, offset);
	GCMSYNC
}

WRAPPERINLINE void gcmSetTransferLocation(const uint8 _Location)
{
	GCMCALLGRAPH( ("gcmSetTransferLocation(%x)", _Location) )
	GCM_NOTTHREADSAFE
	cellGcmSetTransferLocation(POST_090_SDK_FUNC _Location);
	GCMSYNC
}

//##########################################################################################

WRAPPERINLINE void gcmSetVertexDataArray(const uint8 index, const uint16 frequency, const uint8 stride, const uint8 size, const uint8 type, const uint8 location, const uint32 offset)
{
	GCMCALLGRAPH( ("gcmSetVertexDataArray(%x, %.4x, %x, %x, %x, %x, %.8x)", index, frequency, stride, size, type, location, offset) )
	GCM_NOTTHREADSAFE
	cellGcmSetVertexDataArrayInline(POST_090_SDK_FUNC index, frequency, stride, size, type, location, offset);
	GCMSHADOW(m_VertexDataArray, (index, frequency, stride, size, type, location, offset));
	GCMSYNC
}

WRAPPERINLINE void gcmSetVertexData4f(uint8 index, const fp32* _pData)
{
	GCMCALLGRAPH( ("gcmSetVertexData4f(%x, %.8x)", index, _pData) )
	GCM_NOTTHREADSAFE
	cellGcmSetVertexData4fInline(POST_090_SDK_FUNC index, _pData);
	GCMSYNC
}

WRAPPERINLINE void gcmSetDrawIndexArray(const uint8 mode, uint32 count, const uint8 type, const uint8 location, const uint32 indicies)
{
	GCMCALLGRAPH( ("gcmSetDrawIndexArray(%x, %.8x, %x, %x, %.8x)", mode, count, type, location, indicies) )
	GCM_NOTTHREADSAFE
	cellGcmSetDrawIndexArrayInline(POST_090_SDK_FUNC mode, count, type, location, indicies);
	GCMSHADOW(m_DrawArrays, (0, 0, 0))
	GCMSHADOW(m_DrawIndexArray, (mode, count, type, location, indicies))
	GCMSYNC
}

WRAPPERINLINE void gcmSetDrawArrays(const uint8 mode, uint32 first, uint32 count)
{
	GCMCALLGRAPH( ("gcmSetDrawArrays(%x, %.8x, %.8x)", mode, first, count) )
	GCM_NOTTHREADSAFE
	cellGcmSetDrawArraysInline(POST_090_SDK_FUNC mode, first, count);
	GCMSHADOW(m_DrawIndexArray, (0, 0, 0, 0, 0))
	GCMSHADOW(m_DrawArrays, (mode, first, count))
	GCMSYNC
}

WRAPPERINLINE void gcmSetInvalidateVertexCache(void)
{
	GCMCALLGRAPH( ("gcmSetInvalidateVertexCache()") )
	GCM_NOTTHREADSAFE
	cellGcmSetInvalidateVertexCacheInline(POST_090_SDK_FUNC_NO_ARG);
	GCMSYNC
}

//##########################################################################################

WRAPPERINLINE void gcmSetFrontPolygonMode(uint _Value)
{
	GCMCALLGRAPH( ("gcmSetFrontPolygonMode(%x)", _Value) )
	GCM_NOTTHREADSAFE
	cellGcmSetFrontPolygonModeInline(POST_090_SDK_FUNC _Value);
	GCMSYNC
}

WRAPPERINLINE void gcmSetBackPolygonMode(uint _Value)
{
	GCMCALLGRAPH( ("gcmSetBackPolygonMode(%x)", _Value) )
	GCM_NOTTHREADSAFE
	cellGcmSetBackPolygonModeInline(POST_090_SDK_FUNC _Value);
	GCMSYNC
}

WRAPPERINLINE void gcmSetRestartIndex(const uint32 value)
{
	GCMCALLGRAPH( ("gcmSetRestartIndex(%x)", value) )
	GCM_NOTTHREADSAFE
	cellGcmSetRestartIndexInline(POST_090_SDK_FUNC value);
	GCMSYNC
}

WRAPPERINLINE void gcmSetRestartIndexEnable(const uint32 enable)
{
	GCMCALLGRAPH( ("gcmSetRestartIndexEnable(%x)", enable) )
	GCM_NOTTHREADSAFE
	cellGcmSetRestartIndexEnableInline(POST_090_SDK_FUNC enable);
	GCMSYNC
}

WRAPPERINLINE void gcmSetFrontFace(const uint32 dir)
{
	GCMCALLGRAPH( ("gcmSetFrontFace(%x)", dir) )
	GCM_NOTTHREADSAFE
	cellGcmSetFrontFaceInline(POST_090_SDK_FUNC dir);
	GCMSYNC
}

WRAPPERINLINE void gcmSetCullFace(const uint32 cfm)
{
	GCMCALLGRAPH( ("gcmSetCullFace(%x)", cfm) )
	GCM_NOTTHREADSAFE
	cellGcmSetCullFaceInline(POST_090_SDK_FUNC cfm);
	GCMSYNC
}

WRAPPERINLINE void gcmSetCullFaceEnable(const uint32 enable)
{
	GCMCALLGRAPH( ("gcmSetCullFaceEnable(%x)", enable) )
	GCM_NOTTHREADSAFE
	cellGcmSetCullFaceEnableInline(POST_090_SDK_FUNC enable);
	GCMSYNC
}

WRAPPERINLINE void gcmSetClearSurface(const uint32 mask)
{
	GCMCALLGRAPH( ("gcmSetClearSurface(%x)", mask) )
	GCM_NOTTHREADSAFE
	cellGcmSetClearSurfaceInline(POST_090_SDK_FUNC mask);
	GCMSYNC
}

WRAPPERINLINE void gcmSetClearColor(const uint32 color)
{
	GCMCALLGRAPH( ("gcmSetClearColor(%.8x)", color) )
	GCM_NOTTHREADSAFE
	cellGcmSetClearColorInline(POST_090_SDK_FUNC color);
	GCMSYNC
}

WRAPPERINLINE void gcmSetClearDepthStencil(uint32 _Value)
{
	GCMCALLGRAPH( ("gcmSetClearDepthStencil(%.8x)", _Value) )
	GCM_NOTTHREADSAFE
	cellGcmSetClearDepthStencilInline(POST_090_SDK_FUNC _Value);
	GCMSYNC
}

WRAPPERINLINE void gcmSetSurface(const CellGcmSurface *surface)
{
	GCMCALLGRAPH( ("gcmSetSurface(%.8x)", surface) )
	GCM_NOTTHREADSAFE
	cellGcmSetSurfaceInline(POST_090_SDK_FUNC surface);
	GCMSYNC
}

WRAPPERINLINE void gcmSetColorMask(const uint32 mask)
{
	GCMCALLGRAPH( ("gcmSetColorMask(%x)", mask) )
	GCM_NOTTHREADSAFE
	cellGcmSetColorMaskInline(POST_090_SDK_FUNC mask);
	GCMSYNC
}

WRAPPERINLINE void gcmSetColorMaskMrt(const uint32 mask)
{
	GCMCALLGRAPH( ("gcmSetColorMaskMrt(%x)", mask) )
	GCM_NOTTHREADSAFE
	cellGcmSetColorMaskMrtInline(POST_090_SDK_FUNC mask);
	GCMSYNC
}

WRAPPERINLINE void gcmSetViewport(const uint16 x, const uint16 y, const uint16 w, const uint16 h, const float min, const float max, const float scale[4], const float offset[4])
{
	GCMCALLGRAPH( ("gcmSetViewport(%.4, %.4x, %.4x, %.4x, %f, %f, {%f, %f, %f, %f}, {%f, %f, %f, %f})", x, y, w, h, min, max, scale[0], scale[1], scale[2], scale[3], offset[0], offset[1], offset[2], offset[3]) )
	GCM_NOTTHREADSAFE
	cellGcmSetViewportInline(POST_090_SDK_FUNC x, y, w, h, min, max, scale, offset);
	GCMSYNC
}

WRAPPERINLINE void gcmSetDepthTestEnable(const uint32 enable)
{
	GCMCALLGRAPH( ("gcmSetDepthTestEnable(%x)", enable) )
	GCM_NOTTHREADSAFE
	cellGcmSetDepthTestEnableInline(POST_090_SDK_FUNC enable);
	GCMSYNC
}

WRAPPERINLINE void gcmSetDepthFunc(const uint32 zf)
{
	GCMCALLGRAPH( ("gcmSetDepthFunc(%x)", zf) )
	GCM_NOTTHREADSAFE
	cellGcmSetDepthFuncInline(POST_090_SDK_FUNC zf);
	GCMSYNC
}

WRAPPERINLINE void gcmSetDepthMask(const uint32 enable)
{
	GCMCALLGRAPH( ("gcmSetDepthFunc(%x)", enable) )
	GCM_NOTTHREADSAFE
	cellGcmSetDepthMaskInline(POST_090_SDK_FUNC enable);
	GCMSYNC
}

WRAPPERINLINE void gcmSetZMinMaxControl(const uint32 _CullNearFarEnabled, const uint32 _ZClampEnabled, const uint32 _CullIgnoreW)
{
	GCMCALLGRAPH( ("gcmSetZMinMaxControl(%x, %x, %x)", _CullNearFarEnabled, _ZClampEnabled, _CullIgnoreW) )
	GCM_NOTTHREADSAFE
	cellGcmSetZMinMaxControlInline(POST_090_SDK_FUNC _CullNearFarEnabled, _ZClampEnabled, _CullIgnoreW);
	GCMSYNC
}

WRAPPERINLINE void gcmSetZcullLimit(const uint16 _MoveForwardLimit, const uint16 _PushBackLimit)
{
	GCMCALLGRAPH( ("gcmSetZcullLimit(%x, %x)", _MoveForwardLimit, _PushBackLimit) )
	GCM_NOTTHREADSAFE
	cellGcmSetZcullLimitInline(POST_090_SDK_FUNC _MoveForwardLimit, _PushBackLimit);
	GCMSYNC
}

WRAPPERINLINE void gcmSetZcullControl(const uint8 _CullDir, const uint8 _CullFormat)
{
	GCMCALLGRAPH( ("gcmSetZcullControl(%x, %x)", _CullDir, _CullFormat) )
	GCM_NOTTHREADSAFE
	cellGcmSetZcullControlInline(POST_090_SDK_FUNC _CullDir, _CullFormat);
	GCMSYNC
}

WRAPPERINLINE void gcmSetScullControl(const uint8 _Func, const uint8 _Ref, const uint8 _Mask)
{
	GCMCALLGRAPH( ("gcmSetScullControl(%x, %x, %x)", _Func, _Ref, _Mask) )
	GCM_NOTTHREADSAFE
	cellGcmSetScullControlInline(POST_090_SDK_FUNC _Func, _Ref, _Mask);
	GCMSYNC
}

WRAPPERINLINE void gcmSetBlendEnable(const uint32 enable)
{
	GCMCALLGRAPH( ("gcmSetBlendEnable(%x)", enable) )
	GCM_NOTTHREADSAFE
	cellGcmSetBlendEnableInline(POST_090_SDK_FUNC enable);
	GCMSYNC
}

WRAPPERINLINE void gcmSetBlendEnableMrt(const uint32 mrt1, const uint32 mrt2, const uint32 mrt3)
{
	GCMCALLGRAPH( ("gcmSetBlendEnableMrt(%x, %x, %x)", mrt1, mrt2, mrt3) )
	GCM_NOTTHREADSAFE
	cellGcmSetBlendEnableMrtInline(POST_090_SDK_FUNC mrt1, mrt2, mrt3);
	GCMSYNC
}

WRAPPERINLINE void gcmSetBlendEquation(const uint16 color, const uint16 alpha)
{
	GCMCALLGRAPH( ("gcmSetBlendEquation(%x, %x)", color, alpha) )
	GCM_NOTTHREADSAFE
	cellGcmSetBlendEquationInline(POST_090_SDK_FUNC color, alpha);
	GCMSYNC
}

WRAPPERINLINE void gcmSetBlendFunc(const uint16 sfcolor, const uint16 dfcolor, const uint16 sfalpha, const uint16 dfalpha)
{
	GCMCALLGRAPH( ("gcmSetBlendFunc(%x, %x, %x, %x)", sfcolor, dfcolor, sfalpha, dfalpha) )
	GCM_NOTTHREADSAFE
	cellGcmSetBlendFuncInline(POST_090_SDK_FUNC sfcolor, dfcolor, sfalpha, dfalpha);
	GCMSYNC
}

WRAPPERINLINE void gcmSetShadeMode(const uint32 sm)
{
	GCMCALLGRAPH( ("gcmSetShadeMode(%x)", sm) )
	GCM_NOTTHREADSAFE
	cellGcmSetShadeModeInline(POST_090_SDK_FUNC sm);
	GCMSYNC
}

WRAPPERINLINE void gcmSetStencilTestEnable(const uint32 enable)
{
	GCMCALLGRAPH( ("gcmSetStencilTestEnable(%x)", enable) )
	GCM_NOTTHREADSAFE
	cellGcmSetStencilTestEnableInline(POST_090_SDK_FUNC enable);
	GCMSYNC
}

WRAPPERINLINE void gcmSetStencilFunc(const uint32 func, const int32 ref, const uint32 mask)
{
	GCMCALLGRAPH( ("gcmSetStencilFunc(%x, %x, %x)", func, ref, mask) )
	GCM_NOTTHREADSAFE
	cellGcmSetStencilFuncInline(POST_090_SDK_FUNC func, ref, mask);
	GCMSYNC
}

WRAPPERINLINE void gcmSetStencilMask(const uint32 sm)
{
	GCMCALLGRAPH( ("gcmSetStencilMask(%x)", sm) )
	GCM_NOTTHREADSAFE
	cellGcmSetStencilMaskInline(POST_090_SDK_FUNC sm);
	GCMSYNC
}

WRAPPERINLINE void gcmSetStencilOp(const uint32 fail, const uint32 depthFail, const uint32 depthPass)
{
	GCMCALLGRAPH( ("gcmSetStencilOp(%x, %x, %x)", fail, depthFail, depthPass) )
	GCM_NOTTHREADSAFE
	cellGcmSetStencilOpInline(POST_090_SDK_FUNC fail, depthFail, depthPass);
	GCMSYNC
}

WRAPPERINLINE void gcmSetTwoSidedStencilTestEnable(const uint32 enable)
{
	GCMCALLGRAPH( ("gcmSetTwoSidedStencilTestEnable(%x)", enable) )
	GCM_NOTTHREADSAFE
	cellGcmSetTwoSidedStencilTestEnableInline(POST_090_SDK_FUNC enable);
	GCMSYNC
}

WRAPPERINLINE void gcmSetBackStencilFunc(const uint32 func, const int32 ref, const uint32 mask)
{
	GCMCALLGRAPH( ("gcmSetBackStencilFunc(%x, %x, %x)", func, ref, mask) )
	GCM_NOTTHREADSAFE
	cellGcmSetBackStencilFuncInline(POST_090_SDK_FUNC func, ref, mask);
	GCMSYNC
}

WRAPPERINLINE void gcmSetBackStencilMask(const uint32 sm)
{
	GCMCALLGRAPH( ("gcmSetBackStencilMask(%x)", sm) )
	GCM_NOTTHREADSAFE
	cellGcmSetBackStencilMaskInline(POST_090_SDK_FUNC sm);
	GCMSYNC
}

WRAPPERINLINE void gcmSetBackStencilOp(const uint32 fail, const uint32 depthFail, const uint32 depthPass)
{
	GCMCALLGRAPH( ("gcmSetBackStencilOp(%x, %x, %x)", fail, depthFail, depthPass) )
	GCM_NOTTHREADSAFE
	cellGcmSetBackStencilOpInline(POST_090_SDK_FUNC fail, depthFail, depthPass);
	GCMSYNC
}

WRAPPERINLINE void gcmSetAlphaTestEnable(const uint32 enable)
{
	GCMCALLGRAPH( ("gcmSetAlphaTestEnable(%x)", enable) )
	GCM_NOTTHREADSAFE
	cellGcmSetAlphaTestEnableInline(POST_090_SDK_FUNC enable);
	GCMSYNC
}

WRAPPERINLINE void gcmSetAlphaFunc(const uint32 af, const uint32 ref)
{
	GCMCALLGRAPH( ("gcmSetAlphaFunc(%x, %x)", af, ref) )
	GCM_NOTTHREADSAFE
	cellGcmSetAlphaFuncInline(POST_090_SDK_FUNC af, ref);
	GCMSYNC
}

WRAPPERINLINE void gcmSetPolygonOffsetFillEnable(const uint32 enable)
{
	GCMCALLGRAPH( ("gcmSetPolygonOffsetFillEnable(%x)", enable) )
	GCM_NOTTHREADSAFE
	cellGcmSetPolygonOffsetFillEnableInline(POST_090_SDK_FUNC enable);
	GCMSYNC
}

WRAPPERINLINE void gcmSetPolygonOffset(const float factor, const float units)
{
	GCMCALLGRAPH( ("gcmSetPolygonOffset(%f, %f)", factor, units) )
	GCM_NOTTHREADSAFE
	cellGcmSetPolygonOffsetInline(POST_090_SDK_FUNC factor, units);
	GCMSYNC
}

WRAPPERINLINE void gcmSetScissor(const uint16 x, const uint16 y, const uint16 w, const uint16 h)
{
	GCMCALLGRAPH( ("gcmSetScissor(%x, %x, %x, %x)", x, y, w, h) )
	GCM_NOTTHREADSAFE
	cellGcmSetScissorInline(POST_090_SDK_FUNC x, y, w, h);
	GCMSYNC
}

WRAPPERINLINE void gcmSetTexture(const uint8 index, const CellGcmTexture *texture)
{
	GCMCALLGRAPH( ("gcmSetTexture(%x, %.8x)", index, texture) )
	GCM_NOTTHREADSAFE
	cellGcmSetTextureInline(POST_090_SDK_FUNC index, texture);
	GCMSYNC
}

WRAPPERINLINE void gcmSetUserClipPlaneControl(uint _Clip0, uint _Clip1, uint _Clip2, uint _Clip3, uint _Clip4, uint _Clip5)
{
	GCMCALLGRAPH( ("gcmSetUserClipPlaneControl(%x, %x, %x, %x, %x, %x)", _Clip0, _Clip1, _Clip2, _Clip3, _Clip4, _Clip5) )
	GCM_NOTTHREADSAFE
	cellGcmSetUserClipPlaneControlInline(POST_090_SDK_FUNC _Clip0, _Clip1, _Clip2, _Clip3, _Clip4, _Clip5);
	GCMSYNC
}

WRAPPERINLINE void gcmSetTextureAddress(const uint8 index, const uint8 wraps, const uint8 wrapt, const uint8 wrapr, const uint8 unsignedRemap, const uint8 zfunc, const uint8 gamma)
{
	GCMCALLGRAPH( ("gcmSetTextureAddress(%x, %x, %x, %x, %x, %x, %x)", index, wraps, wrapt, wrapr, unsignedRemap, zfunc, gamma) )
	GCM_NOTTHREADSAFE
	cellGcmSetTextureAddressInline(POST_090_SDK_FUNC index, wraps, wrapt, wrapr, unsignedRemap, zfunc, gamma);
	GCMSYNC
}

WRAPPERINLINE void gcmSetTextureBorderColor(const uint8 index, const uint32 color)
{
	GCMCALLGRAPH( ("gcmSetTextureBorderColor(%x, %x)", index, color) )
	GCM_NOTTHREADSAFE
	cellGcmSetTextureBorderColorInline(POST_090_SDK_FUNC index, color);
	GCMSYNC
}

WRAPPERINLINE void gcmSetTextureControl(const uint8 index, const uint32 enable, const uint16 minlod, const uint16 maxlod, const uint8 maxaniso)
{
	GCMCALLGRAPH( ("gcmSetTextureControl(%x, %x, %x, %x, %x)", index, enable, minlod, maxlod, maxaniso) )
	GCM_NOTTHREADSAFE
	cellGcmSetTextureControlInline(POST_090_SDK_FUNC index, enable, minlod, maxlod, maxaniso);
	GCMSYNC
}

WRAPPERINLINE void gcmSetTextureFilter(const uint8 index, const uint16 bias, const uint8 min, const uint8 mag, const uint8 conv)
{
	GCMCALLGRAPH( ("gcmSetTextureFilter(%x, %x, %x, %x, %x)", index, bias, min, mag, conv) )
	GCM_NOTTHREADSAFE
	cellGcmSetTextureFilterInline(POST_090_SDK_FUNC index, bias, min, mag, conv);
	GCMSYNC
}

WRAPPERINLINE uint32 gcmSetTransferImage(const uint8 mode, const uint32 dstOffset, const uint32 dstPitch, const uint32 dstX, const uint32 dstY, const uint32 srcOffset, const uint32 srcPitch, const uint32 srcX, const uint32 srcY, const uint32 width, const uint32 height, const uint32 bytesPerPixel)
{
	GCMCALLGRAPH( ("gcmSetTransferImage(%x, %.8x, %x, %x, %x, %.8x, %x, %x, %x, %x, %x, %x)", mode, dstOffset, dstPitch, dstX, dstY, srcOffset, srcPitch, srcX, srcY, width, height, bytesPerPixel) )
	GCM_NOTTHREADSAFE
	uint32 ret = cellGcmSetTransferImageInline(POST_090_SDK_FUNC mode, dstOffset, dstPitch, dstX, dstY, srcOffset, srcPitch, srcX, srcY, width, height, bytesPerPixel);
	GCMSYNC
	return ret;
}

WRAPPERINLINE uint32 gcmSetTransferData(const uint8 mode, const uint32 dstOffset, const uint32 dstPitch, const uint32 srcOffset, const uint32 srcPitch, const uint32 bytesPerRow, const uint32 rowCount)
{
	GCMCALLGRAPH( ("gcmSetTransferData(%x, %.8x, %x, %.8x, %x, %x, %x)", mode, dstOffset, dstPitch, srcOffset, srcPitch, bytesPerRow, rowCount) )
	GCM_NOTTHREADSAFE
	uint32 ret = cellGcmSetTransferDataInline(POST_090_SDK_FUNC mode, dstOffset, dstPitch, srcOffset, srcPitch, bytesPerRow, rowCount);
	GCMSYNC
	return ret;
}

WRAPPERINLINE void gcmSetConvertSwizzleFormat(const uint32 dstOffset, const uint32 dstWidth, const uint32 dstHeight, const uint32 dstX, const uint32 dstY, const uint32 srcOffset, const uint32 srcPitch, const uint32 srcX, const uint32 srcY, const uint32 width, const uint32 height, const uint32 bytesPerPixel, const uint32 mode)
{
	GCMCALLGRAPH( ("gcmSetConvertSwizzleFormat(%.8x, %x, %x, %x, %x, %.8x, %x, %x, %x, %x, %x, %x, %x)", dstOffset, dstWidth, dstHeight, dstX, dstY, srcOffset, srcPitch, srcX, srcY, width, height, bytesPerPixel, mode) )
	GCM_NOTTHREADSAFE
	cellGcmSetConvertSwizzleFormatInline(POST_090_SDK_FUNC dstOffset, dstWidth, dstHeight, dstX, dstY, srcOffset, srcPitch, srcX, srcY, width, height, bytesPerPixel, mode);
	GCMSYNC
}

WRAPPERINLINE void gcmSetInvalidateTextureCache(const uint32 value)
{
	GCMCALLGRAPH( ("gcmSetInvalidateTextureCache(%x)", value) )
	GCM_NOTTHREADSAFE
	cellGcmSetInvalidateTextureCacheInline(POST_090_SDK_FUNC value);
	GCMSYNC
}

WRAPPERINLINE void gcmSetNopCommand(uint32 _wordcount)
{
	GCMCALLGRAPH( ("gcmSetNopCommand(%x)", _wordcount) )
	GCM_NOTTHREADSAFE
	cellGcmSetNopCommandInline(POST_090_SDK_FUNC _wordcount);
	GCMSYNC
}

WRAPPERINLINE void gcmDump()
{
	// This causes a GPU exception, can be used to analyze things
	gcmFinish();
	gcmSetDrawIndexArray(CELL_GCM_PRIMITIVE_TRIANGLES, 65536, CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16, CELL_GCM_LOCATION_MAIN, ~0);
	gcmFinish();
}

#endif // __MRENDERPS3_CELLWRAPPERS_H_INCLUDED

