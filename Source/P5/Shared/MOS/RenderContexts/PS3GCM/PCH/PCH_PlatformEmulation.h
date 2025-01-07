
// CG emulation

typedef uint32 CGprogram;
typedef uint32 CGparameter;
typedef uint32 CGresource;

static M_INLINE void cellGcmCgInitProgram(CGprogram _Program) {}
static M_INLINE void cellGcmCgGetUCode(CGprogram _Program, void** _ppCode, uint32* _pCodeSize) {M_BREAKPOINT;}
static M_INLINE CGparameter cellGcmCgGetNamedParameter(CGprogram prog, const char *name) {M_BREAKPOINT; return 0;}
static M_INLINE CGresource cellGcmCgGetParameterResource(CGprogram prog, CGparameter _Param) {M_BREAKPOINT; return 0;}
static M_INLINE int cellGcmCgGetTotalBinarySize(CGprogram prog);
static M_INLINE int compile_program_from_string(const char*, const char*, const char*, const char**, char**);
static M_INLINE void free_compiled_program(char*);

// GCM emulation
enum
{
	CELL_OK = 0,

	CELL_GCM_MRT_MAXCOUNT = 4,

	CELL_GCM_FALSE = 0,
	CELL_GCM_TRUE = 1,

	CELL_GCM_TRANSFER_LOCAL_TO_LOCAL,
	CELL_GCM_TRANSFER_LOCAL_TO_MAIN,
	CELL_GCM_TRANSFER_MAIN_TO_LOCAL,

	CELL_GCM_CLEAR_R,
	CELL_GCM_CLEAR_G,
	CELL_GCM_CLEAR_B,
	CELL_GCM_CLEAR_A,
	CELL_GCM_CLEAR_Z,
	CELL_GCM_CLEAR_S,

	CELL_GCM_COLOR_MASK_R = 1,
	CELL_GCM_COLOR_MASK_G = 2,
	CELL_GCM_COLOR_MASK_B = 4,
	CELL_GCM_COLOR_MASK_A = 8,

	CELL_GCM_ZPASS_PIXEL_CNT,
	CELL_GCM_PRIMITIVE_TRIANGLES,
	CELL_GCM_PRIMITIVE_TRIANGLE_STRIP,
	CELL_GCM_PRIMITIVE_TRIANGLE_FAN,
	CELL_GCM_PRIMITIVE_POLYGON,
	CELL_GCM_PRIMITIVE_LINES,
	CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16,
	CELL_GCM_LOCATION_MAIN,
	CELL_GCM_LOCATION_LOCAL,
	CELL_GCM_INVALIDATE_TEXTURE,

	CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX,

	CELL_GCM_TEXTURE_DIMENSION_1,
	CELL_GCM_TEXTURE_DIMENSION_2,
	CELL_GCM_TEXTURE_DIMENSION_3,
	CELL_GCM_TEXTURE_NR,
	CELL_GCM_TEXTURE_SZ,
	CELL_GCM_TEXTURE_LN,
	CELL_GCM_TEXTURE_COMPRESSED_DXT1,
	CELL_GCM_TEXTURE_COMPRESSED_DXT23,
	CELL_GCM_TEXTURE_COMPRESSED_DXT45,
	CELL_GCM_TEXTURE_A1R5G5B5,
	CELL_GCM_TEXTURE_A8R8G8B8,
	CELL_GCM_TEXTURE_B8,
	CELL_GCM_TEXTURE_G8B8,
	CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT,
	CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT,
	CELL_GCM_TEXTURE_A4R4G4B4,
	
	CELL_GCM_TEXTURE_NEAREST,
	CELL_GCM_TEXTURE_LINEAR,
	CELL_GCM_TEXTURE_LINEAR_LINEAR,
	CELL_GCM_TEXTURE_NEAREST_LINEAR,
	CELL_GCM_TEXTURE_NEAREST_NEAREST,
	CELL_GCM_TEXTURE_LINEAR_NEAREST,

	CELL_GCM_TEXTURE_CLAMP,
	CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
	CELL_GCM_TEXTURE_WRAP,

	CELL_GCM_TEXTURE_REMAP_ZERO,
	CELL_GCM_TEXTURE_REMAP_ONE,
	CELL_GCM_TEXTURE_REMAP_FROM_A,
	CELL_GCM_TEXTURE_REMAP_FROM_R,
	CELL_GCM_TEXTURE_REMAP_FROM_G,
	CELL_GCM_TEXTURE_REMAP_FROM_B,
	CELL_GCM_TEXTURE_REMAP_REMAP,

	CELL_GCM_COMPMODE_DISABLED,
	CELL_GCM_COMPMODE_Z32_SEPSTENCIL,

	CELL_GCM_VERTEX_F,
	CELL_GCM_VERTEX_UB,
	CELL_GCM_VERTEX_S1,
	CELL_GCM_VERTEX_S32K,
	CELL_GCM_VERTEX_CMP,
	CELL_GCM_VERTEX_UB256,

	CELL_GCM_CCW,
	CELL_GCM_CW,

	CELL_GCM_POLYGON_MODE_LINE,
	CELL_GCM_POLYGON_MODE_FILL,

	CELL_GCM_USER_CLIP_PLANE_DISABLE,
	CELL_GCM_USER_CLIP_PLANE_ENABLE_GE,
	CELL_GCM_USER_CLIP_PLANE_ENABLE_LT,

	CELL_GCM_ZERO,
	CELL_GCM_ONE,
	CELL_GCM_KEEP,
	CELL_GCM_REPLACE,
	CELL_GCM_INVERT,
	CELL_GCM_INCR,
	CELL_GCM_DECR,
	CELL_GCM_INCR_WRAP,
	CELL_GCM_DECR_WRAP,

	CELL_GCM_COLOR_MASK_MRT1_R,
	CELL_GCM_COLOR_MASK_MRT1_G,
	CELL_GCM_COLOR_MASK_MRT1_B,
	CELL_GCM_COLOR_MASK_MRT1_A,
	CELL_GCM_COLOR_MASK_MRT2_R,
	CELL_GCM_COLOR_MASK_MRT2_G,
	CELL_GCM_COLOR_MASK_MRT2_B,
	CELL_GCM_COLOR_MASK_MRT2_A,
	CELL_GCM_COLOR_MASK_MRT3_R,
	CELL_GCM_COLOR_MASK_MRT3_G,
	CELL_GCM_COLOR_MASK_MRT3_B,
	CELL_GCM_COLOR_MASK_MRT3_A,

	CELL_GCM_SRC_COLOR,
	CELL_GCM_ONE_MINUS_SRC_COLOR,
	CELL_GCM_DST_COLOR,
	CELL_GCM_ONE_MINUS_DST_COLOR,
	CELL_GCM_SRC_ALPHA,
	CELL_GCM_ONE_MINUS_SRC_ALPHA,
	CELL_GCM_DST_ALPHA,
	CELL_GCM_ONE_MINUS_DST_ALPHA,
	CELL_GCM_SRC_ALPHA_SATURATE,

	CELL_GCM_NOTEQUAL,
	CELL_GCM_NEVER,
	CELL_GCM_ALWAYS,
	CELL_GCM_LESS,
	CELL_GCM_LEQUAL,
	CELL_GCM_EQUAL,
	CELL_GCM_GEQUAL,
	CELL_GCM_GREATER,

	CELL_GCM_FLAT,
	CELL_GCM_SMOOTH,
	CELL_GCM_FRONT,
	CELL_GCM_BACK,

	CELL_GCM_SURFACE_TARGET_NONE,
	CELL_GCM_SURFACE_TARGET_0,
	CELL_GCM_SURFACE_TARGET_MRT1,
	CELL_GCM_SURFACE_TARGET_MRT2,
	CELL_GCM_SURFACE_TARGET_MRT3,
	CELL_GCM_SURFACE_PITCH,
	CELL_GCM_SURFACE_CENTER_1,
	CELL_GCM_SURFACE_A8R8G8B8,
	CELL_GCM_SURFACE_Z24S8,
	CELL_GCM_SURFACE_F_W16Z16Y16X16,

	CELL_GCM_DEBUG_LEVEL2,
	CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8,
	CELL_VIDEO_OUT_PRIMARY,
	CELL_GCM_DISPLAY_HSYNC,
	CELL_GCM_DISPLAY_HSYNC_WITH_NOISE,
	CELL_GCM_DISPLAY_VSYNC	= (2),


	CELL_GCM_ZCULL_Z24S8,
	CELL_GCM_ZCULL_LESS,
	CELL_GCM_ZCULL_GREATER,
	CELL_GCM_ZCULL_LONES,

	CELL_GCM_ZCULL_STATS,
	CELL_GCM_ZCULL_STATS1,
	CELL_GCM_ZCULL_STATS2,
	CELL_GCM_ZCULL_STATS3,

};

typedef struct CellGcmConfig
{
	void		*localAddress;
	void		*ioAddress;
	uint32	localSize;
	uint32	ioSize;
	uint32	memoryFrequency;
	uint32	coreFrequency;
} CellGcmConfig;

typedef struct CellGcmSurface
{
	uint8		type;
	uint8		antialias;

	uint8		colorFormat;
	uint8		colorTarget;
	uint8		colorLocation[CELL_GCM_MRT_MAXCOUNT];
	uint32	colorOffset[CELL_GCM_MRT_MAXCOUNT];
	uint32	colorPitch[CELL_GCM_MRT_MAXCOUNT];

	uint8		depthFormat;
	uint8		depthLocation;
	uint8		_padding[2];
	uint32	depthOffset;
	uint32	depthPitch;

	uint16	width;
	uint16	height;
	uint16	x;
	uint16	y;
} CellGcmSurface;

typedef struct CellGcmTexture
{
	uint8		format;
	uint8		mipmap;
	uint8		dimension;
	uint8		cubemap;

	uint32	remap;

	uint16	width;
	uint16	height;
	uint16	depth;
	uint8		location;
	uint8		_padding;

	uint32	pitch;
	uint32	offset;
} CellGcmTexture;

struct CellGcmContextData;
typedef int32 (*CellGcmContextCallback)(struct CellGcmContextData *, uint32);

typedef struct CellGcmContextData{
	uint32 *M_RESTRICT begin;
	uint32 *M_RESTRICT end;
	uint32 *M_RESTRICT current;
	CellGcmContextCallback callback;
}CellGcmContextData;

typedef struct CellVideoOutColorInfo {
	uint16                      redX;
	uint16                      redY;
	uint16                      greenX;
	uint16                      greenY;
	uint16                      blueX;
	uint16                      blueY;
	uint16                      whiteX;
	uint16                      whiteY;
	uint32                      gamma;
} CellVideoOutColorInfo;

typedef struct CellVideoOutKSVList {
	uint8                       ksv[32*5];
	uint8                       reserved[4];
	uint32                      count;
} CellVideoOutKSVList;

typedef struct CellVideoOutDisplayMode {
	uint8                       resolutionId;
	uint8                       scanMode;
	uint8                       conversion;
	uint8                       aspect;
	uint8                       reserved[2];
	uint16                      refreshRates;
} CellVideoOutDisplayMode;

typedef struct CellVideoOutResolution {
	uint16                      width;
	uint16                      height;
} CellVideoOutResolution;

typedef struct CellVideoOutDeviceInfo {
	uint8                       portType;
	uint8                       colorSpace;
	uint16                      latency;
	uint8                       availableModeCount;
	uint8                       reserved[7];
	CellVideoOutColorInfo         colorInfo;
	CellVideoOutDisplayMode       availableModes[32];
	CellVideoOutKSVList           ksvList;
} CellVideoOutDeviceInfo;

typedef struct CellVideoOutState {
	uint8                       state;
	uint8                       colorSpace;
	uint8                       reserved[6];
	CellVideoOutDisplayMode       displayMode;
} CellVideoOutState;

typedef struct CellVideoOutConfiguration {
	uint8                       resolutionId;
	uint8                       format;
	uint8                       aspect;
	uint8                       reserved[9];
	uint32                      pitch;
} CellVideoOutConfiguration;

typedef struct CellVideoOutOption {
	uint32                      reserved;
} CellVideoOutOption;

static CellGcmContextData* gCellGcmCurrentContext = NULL;

static M_INLINE uint32 cellVideoOutConfigure(uint32 videoOut, CellVideoOutConfiguration* config, CellVideoOutOption* option, uint32 waitForEvent) {return CELL_OK;}
static M_INLINE void cellVideoOutGetState(uint32 videoOut, uint32 deviceIndex, CellVideoOutState* state) {}
static M_INLINE void cellVideoOutGetResolution(uint32 resolutionId, CellVideoOutResolution* resolution) {}
static M_INLINE void cellGcmSetDebugOutputLevel(int32 _Level) {}
static M_INLINE int32 cellGcmInit(const uint32 cmdSize, const uint32 ioSize, const void *ioAddress) {return CELL_OK;}
static M_INLINE void cellGcmGetConfiguration(struct CellGcmConfig* _pConfig) {}
static M_INLINE uint32* cellGcmGetLabelAddress(const uint8 index) {static uint32 value = 0; return &value;}
static M_INLINE uint32 cellGcmAddressToOffset(const void *address, uint32 *offset) {*offset = ((uint32&)address); return 0;}
static M_INLINE int32 cellGcmMapMainMemory(const void *address, const uint32 size, uint32 *offset) {return 0;}
static M_INLINE uint32 cellGcmGetTiledPitchSize(const uint32 _Size) {return 4096;}
static M_INLINE void cellGcmSetTile(const uint8 index, const uint8 location, const uint32 offset, const uint32 size, const uint32 pitch, const uint8 comp, const uint16 base, const uint8 bank) {}
static M_INLINE void cellGcmSetInvalidateTile(const uint8 index) {}
static M_INLINE uint32 cellGcmSetDisplayBuffer(const uint8 id, const uint32 offset, const uint32 pitch, const uint32 width, const uint32 height) {return 0;}
static M_INLINE void cellGcmSetViewport(struct CellGcmContextData* _pContext, const uint16 _X, const uint16 _Y, const uint16 _W, const uint16 _H, const fp32 _Min, const fp32 _Max, const fp32 _Scale[4], const fp32 _Offset[4]) {}
static M_INLINE void cellGcmSetSurface(struct CellGcmContextData* _pContext, const struct CellGcmSurface* _pSurface) {}
static M_INLINE void cellGcmFlush(struct CellGcmContextData* _pContext) {}
static M_INLINE void cellGcmFinish(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE uint32 cellGcmSetFlip(struct CellGcmContextData* _pContext, const uint8 id) {return 0;}
static M_INLINE void cellGcmSetFlipMode(const uint32 mode) {}
static M_INLINE uint32 cellGcmGetFlipStatus() {return 1;}
static M_INLINE void cellGcmResetFlipStatus() {}
static M_INLINE void cellGcmSetWaitFlip(struct CellGcmContextData* _pContext) {}
static M_INLINE int32 cellGcmSetPrepareFlip(struct CellGcmContextData* _pContext, const uint8 id) {return 1;}
static M_INLINE int32 cellGcmSetFlipImmediate(const uint8 id) {return 1;}
static M_INLINE void cellGcmSetWaitLabel(struct CellGcmContextData* _pContext, const uint8 index, const uint32 value) {}
static M_INLINE void cellGcmSetWriteBackEndLabel(struct CellGcmContextData* _pContext, const uint8 index, const uint32 value) {}
static M_INLINE void cellGcmSetWriteCommandLabel(struct CellGcmContextData* _pContext, const uint8 index, const uint32 value) {}
static M_INLINE void cellGcmSetWriteTextureLabel(struct CellGcmContextData* _pContext, const uint8 index, const uint32 value) {}
static M_INLINE void cellGcmSetZpassPixelCountEnable(struct CellGcmContextData* _pContext, const uint32 enable) {}
static M_INLINE void cellGcmSetClearReport(struct CellGcmContextData* _pContext, const uint32 _Type) {}
static M_INLINE uint32 cellGcmGetReport(const uint32 _Type, const uint32 _ReportID) {return 0;}
static M_INLINE void cellGcmSetReport(struct CellGcmContextData* _pContext, const uint32 _Type, const uint32 _ReportID) {}
static M_INLINE void cellGcmSetUserCommand(struct CellGcmContextData* _pContext, const uint32 _Command) {}
static M_INLINE void cellGcmSetUserHandler(void (*pfnHandler)(uint32)) {}

static M_INLINE void cellGcmSetVertexProgram(struct CellGcmContextData* _pContext, CGprogram _Program, const void* _pUCode) {}
static M_INLINE void cellGcmSetVertexProgramConstants(struct CellGcmContextData* _pContext, uint32 _iStart, uint32 _nCount, const fp32* _pData) {}
static M_INLINE void cellGcmSetVertexProgramParameter(struct CellGcmContextData* _pContext, const CGparameter _Param, const fp32* _pValue) {}
static M_INLINE void cellGcmSetVertexProgramParameterBlock(struct CellGcmContextData* _pContext, const uint32 _BaseConst, const uint32 _ConstCount, const fp32* _pValue) {}

static M_INLINE void cellGcmSetFragmentProgram(struct CellGcmContextData* _pContext, CGprogram _Program, const uint32 _Offset) {}
static M_INLINE void cellGcmSetUpdateFragmentProgramParameter(struct CellGcmContextData* _pContext, const uint32 _Offset) {}
static M_INLINE void cellGcmSetFragmentProgramParameter(struct CellGcmContextData* _pContext, const CGprogram _Program, const CGparameter _Param, const fp32* _pValue, const uint32 _Offset) {}

static M_INLINE void cellGcmSetVertexDataArray(struct CellGcmContextData* _pContext, const uint8 _Index, const uint16 _Frequency, const uint8 _Stride, const uint8 _Size, const uint8 _Type, const uint8 _Location, const uint32 _Offset) {}
static M_INLINE void cellGcmSetVertexData4f(struct CellGcmContextData* _pContext, const uint8 _Index, const fp32* _pValue) {}
static M_INLINE void cellGcmSetDrawIndexArray(struct CellGcmContextData* _pContext, const uint8 _Mode, const uint32 _Count, const uint8 _Type, const uint8 _Location, const uint32 _Indices) {}
static M_INLINE void cellGcmSetDrawArrays(struct CellGcmContextData* _pContext, const uint8 _Mode, const uint32 _First, const uint32 _Count) {}
static M_INLINE void cellGcmSetInvalidateVertexCache(struct CellGcmContextData* _pContext) {}

static M_INLINE void cellGcmSetFrontPolygonMode(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetBackPolygonMode(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetRestartIndex(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetRestartIndexEnable(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetFrontFace(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetCullFace(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetCullFaceEnable(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetClearSurface(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetClearColor(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetClearDepthStencil(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetColorMask(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetColorMaskMrt(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetDepthTestEnable(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetDepthFunc(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetDepthMask(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetBlendEnable(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetBlendEnableMrt(struct CellGcmContextData* _pContext, uint32 _Mrt1, uint32 _Mrt2, uint32 _Mrt3) {}
static M_INLINE void cellGcmSetBlendEquation(struct CellGcmContextData* _pContext, uint16 _Color, uint16 _Alpha) {}
static M_INLINE void cellGcmSetBlendFunc(struct CellGcmContextData* _pContext, uint16 _SrcColor, uint16 _DstColor, uint16 _SrcAlpha, uint16 _DstAlpha) {}
static M_INLINE void cellGcmSetShadeMode(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetStencilTestEnable(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetStencilFunc(struct CellGcmContextData* _pContext, uint32 _Func, uint32 _Ref, uint32 _Mask) {}
static M_INLINE void cellGcmSetStencilMask(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetStencilOp(struct CellGcmContextData* _pContext, uint32 _Fail, uint32 _DepthFail, uint32 _DepthPass) {}
static M_INLINE void cellGcmSetTwoSidedStencilTestEnable(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetBackStencilFunc(struct CellGcmContextData* _pContext, uint32 _Func, uint32 _Ref, uint32 _Mask) {}
static M_INLINE void cellGcmSetBackStencilMask(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetBackStencilOp(struct CellGcmContextData* _pContext, uint32 _Fail, uint32 _DepthFail, uint32 _DepthPass) {}
static M_INLINE void cellGcmSetAlphaTestEnable(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetAlphaFunc(struct CellGcmContextData* _pContext, uint32 _Func, uint32 _Ref) {}
static M_INLINE void cellGcmSetPolygonOffsetFillEnable(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetPolygonOffset(struct CellGcmContextData* _pContext, fp32 _Factor, fp32 _Units) {}
static M_INLINE void cellGcmSetScissor(struct CellGcmContextData* _pContext, uint16 _X, uint16 _Y, uint16 _W, uint16 _H) {}
static M_INLINE void cellGcmSetTexture(struct CellGcmContextData* _pContext, uint8 _Index, const CellGcmTexture* _pTexture) {}
static M_INLINE void cellGcmSetUserClipPlaneControl(struct CellGcmContextData* _pContext, uint32 _Clp0, uint32 _Clp1, uint32 _Clp2, uint32 _Clp3, uint32 _Clp4, uint32 _Clp5) {}
static M_INLINE void cellGcmSetTextureAddress(struct CellGcmContextData* _pContext, const uint8 _Index, const uint8 _WrapS, const uint8 _WrapT, const uint8 _WrapR, const uint8 _UnsignedRemap, const uint8 _ZFunc, const uint8 _Gamma) {}
static M_INLINE void cellGcmSetTextureBorderColor(struct CellGcmContextData* _pContext, const uint8 _Index, uint32 _Color) {}
static M_INLINE void cellGcmSetTextureControl(struct CellGcmContextData* _pContext, const uint8 _Index, const uint32 _Enable, const uint16 _MinLod, const uint16 _MaxLod, const uint8 _MaxAniso) {}
static M_INLINE void cellGcmSetTextureFilter(struct CellGcmContextData* _pContext, const uint8 _Index, const uint16 _Bias, const uint8 _Min, const uint8 _Max, const uint8 _Conv) {}
static M_INLINE void cellGcmSetTransferLocation(struct CellGcmContextData* _pContext, const uint8 _Location) {}
static M_INLINE uint32 cellGcmSetTransferImage(struct CellGcmContextData* _pContext, const uint8 _Mode, const uint32 _DstOffset, const uint32 _DstPitch, const uint32 _DstX, const uint32 _DstY, const uint32 _SrcOffset, const uint32 _SrcPitch, const uint32 _SrcX, const uint32 _SrcY, const uint32 _Width, const uint32 _Height, const uint32 _BytesPerPixel) {return CELL_OK;}
static M_INLINE uint32 cellGcmSetTransferData(struct CellGcmContextData* _pContext, const uint8 _Mode, const uint32 _DstOffset, const uint32 _DstPitch, const uint32 _SrcOffset, const uint32 _SrcPitch, const uint32 _BytesPerRow, const uint32 _RowCount) {return CELL_OK;}
static M_INLINE void cellGcmSetConvertSwizzleFormat(struct CellGcmContextData* _pContext, const uint32 _DstOffset, const uint32 _DstWidth, const uint32 _DstHeight, const uint32 _DstX, const uint32 _DstY, const uint32 _SrcOffset, const uint32 _SrcPitch, const uint32 _SrcX, const uint32 _SrcY, const uint32 _Width, const uint32 _Height, const uint32 _BytesPerPixel, const uint32 _Mode) {}
static M_INLINE void cellGcmConvertSwizzleFormat(void *swizzledTexture, const void *texture, const uint32 dstx0, const uint32 dsty0, const uint32 dstz0, const uint32 dstWidth, const uint32 dstHeight, const uint32 dstDepth, const uint32 srcx0, const uint32 srcy0, const uint32 srcz0, const uint32 srcx1, const uint32 srcy1, const uint32 srcz1, const uint32 srcWidth, const uint32 srcHeight, const uint32 srcDepth, const uint32 dstLog2cdepth, const uint32 srcColordepth, const uint32 border, const uint32 dim, void* _pData ) {}
static M_INLINE void cellGcmSetInvalidateTextureCache(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetNopCommand(struct CellGcmContextData* _pContext, uint32 _WordCount) {}
static M_INLINE void cellGcmSetZcull(const uint8 index, const uint32 offset, const uint32 width, const uint32 height, const uint32 cullStart, const uint32 zFormat, const uint32 aaFormat, const uint32 zCullDir, const uint32 zCullFormat, const uint32 sFunc, const uint32 sRef, const uint32 sMask) {}
static M_INLINE void cellGcmSetInvalidateZcull(struct CellGcmContextData* _pContext) {}
static M_INLINE void cellGcmSetZcullStatsEnable(struct CellGcmContextData* _pContext, uint32) {}
static M_INLINE void cellGcmSetZMinMaxControl(struct CellGcmContextData* _pContext, uint32, uint32, uint32) {}
static M_INLINE void cellGcmSetZcullLimit(struct CellGcmContextData* _pContext, uint16, uint16) {}
static M_INLINE void cellGcmSetZcullControl(struct CellGcmContextData* _pContext, uint32, uint32) {}
static M_INLINE void cellGcmSetScullControl(struct CellGcmContextData* _pContext, uint32, uint32, uint32) {}
static M_INLINE void cellGcmHUDInit() {}

static M_INLINE uint32 cellVideoOutConfigureInline(uint32 videoOut, CellVideoOutConfiguration* config, CellVideoOutOption* option, uint32 waitForEvent) {return CELL_OK;}
static M_INLINE void cellVideoOutGetStateInline(uint32 videoOut, uint32 deviceIndex, CellVideoOutState* state) {}
static M_INLINE void cellVideoOutGetResolutionInline(uint32 resolutionId, CellVideoOutResolution* resolution) {}
static M_INLINE void cellGcmSetDebugOutputLevelInline(int32 _Level) {}
static M_INLINE int32 cellGcmInitInline(const uint32 cmdSize, const uint32 ioSize, const void *ioAddress) {return CELL_OK;}
static M_INLINE void cellGcmGetConfigurationInline(struct CellGcmConfig* _pConfig) {}
static M_INLINE uint32* cellGcmGetLabelAddressInline(const uint8 index) {static uint32 value = 0; return &value;}
static M_INLINE uint32 cellGcmAddressToOffsetInline(const void *address, uint32 *offset) {*offset = ((uint32&)address); return 0;}
static M_INLINE int32 cellGcmMapMainMemoryInline(const void *address, const uint32 size, uint32 *offset) {return 0;}
static M_INLINE uint32 cellGcmGetTiledPitchSizeInline(const uint32 _Size) {return 4096;}
static M_INLINE void cellGcmSetTileInline(const uint8 index, const uint8 location, const uint32 offset, const uint32 size, const uint32 pitch, const uint8 comp, const uint16 base, const uint8 bank) {}
static M_INLINE void cellGcmSetInvalidateTileInline(const uint8 index) {}
static M_INLINE uint32 cellGcmSetDisplayBufferInline(const uint8 id, const uint32 offset, const uint32 pitch, const uint32 width, const uint32 height) {return 0;}
static M_INLINE void cellGcmSetViewportInline(struct CellGcmContextData* _pContext, const uint16 _X, const uint16 _Y, const uint16 _W, const uint16 _H, const fp32 _Min, const fp32 _Max, const fp32 _Scale[4], const fp32 _Offset[4]) {}
static M_INLINE void cellGcmSetSurfaceInline(struct CellGcmContextData* _pContext, const struct CellGcmSurface* _pSurface) {}
static M_INLINE void cellGcmFlushInline(struct CellGcmContextData* _pContext) {}
static M_INLINE void cellGcmFinishInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE uint32 cellGcmSetFlipInline(struct CellGcmContextData* _pContext, const uint8 id) {return 0;}
static M_INLINE void cellGcmSetFlipModeInline(const uint32 mode) {}
static M_INLINE uint32 cellGcmGetFlipStatusInline() {return 1;}
static M_INLINE void cellGcmResetFlipStatusInline() {}
static M_INLINE void cellGcmSetWaitFlipInline(struct CellGcmContextData* _pContext) {}
static M_INLINE int32 cellGcmSetPrepareFlipInline(struct CellGcmContextData* _pContext, const uint8 id) {return 1;}
static M_INLINE int32 cellGcmSetFlipImmediateInline(const uint8 id) {return 1;}
static M_INLINE void cellGcmSetWaitLabelInline(struct CellGcmContextData* _pContext, const uint8 index, const uint32 value) {}
static M_INLINE void cellGcmSetWriteBackEndLabelInline(struct CellGcmContextData* _pContext, const uint8 index, const uint32 value) {}
static M_INLINE void cellGcmSetWriteCommandLabelInline(struct CellGcmContextData* _pContext, const uint8 index, const uint32 value) {}
static M_INLINE void cellGcmSetWriteTextureLabelInline(struct CellGcmContextData* _pContext, const uint8 index, const uint32 value) {}
static M_INLINE void cellGcmSetZpassPixelCountEnableInline(struct CellGcmContextData* _pContext, const uint32 enable) {}
static M_INLINE void cellGcmSetClearReportInline(struct CellGcmContextData* _pContext, const uint32 _Type) {}
static M_INLINE uint32 cellGcmGetReportInline(const uint32 _Type, const uint32 _ReportID) {return 0;}
static M_INLINE void cellGcmSetReportInline(struct CellGcmContextData* _pContext, const uint32 _Type, const uint32 _ReportID) {}
static M_INLINE void cellGcmSetUserCommandInline(struct CellGcmContextData* _pContext, const uint32 _Command) {}
static M_INLINE void cellGcmSetUserHandlerInline(void (*pfnHandler)(uint32)) {}

static M_INLINE void cellGcmSetVertexProgramInline(struct CellGcmContextData* _pContext, CGprogram _Program, const void* _pUCode) {}
static M_INLINE void cellGcmSetVertexProgramConstantsInline(struct CellGcmContextData* _pContext, uint32 _iStart, uint32 _nCount, const fp32* _pData) {}
static M_INLINE void cellGcmSetVertexProgramParameterInline(struct CellGcmContextData* _pContext, const CGparameter _Param, const fp32* _pValue) {}
static M_INLINE void cellGcmSetVertexProgramParameterBlockInline(struct CellGcmContextData* _pContext, const uint32 _BaseConst, const uint32 _ConstCount, const fp32* _pValue) {}

static M_INLINE void cellGcmSetFragmentProgramInline(struct CellGcmContextData* _pContext, CGprogram _Program, const uint32 _Offset) {}
static M_INLINE void cellGcmSetUpdateFragmentProgramParameterInline(struct CellGcmContextData* _pContext, const uint32 _Offset) {}
static M_INLINE void cellGcmSetFragmentProgramParameterInline(struct CellGcmContextData* _pContext, const CGprogram _Program, const CGparameter _Param, const fp32* _pValue, const uint32 _Offset) {}

static M_INLINE void cellGcmSetVertexDataArrayInline(struct CellGcmContextData* _pContext, const uint8 _Index, const uint16 _Frequency, const uint8 _Stride, const uint8 _Size, const uint8 _Type, const uint8 _Location, const uint32 _Offset) {}
static M_INLINE void cellGcmSetVertexData4fInline(struct CellGcmContextData* _pContext, const uint8 _Index, const fp32* _pValue) {}
static M_INLINE void cellGcmSetDrawIndexArrayInline(struct CellGcmContextData* _pContext, const uint8 _Mode, const uint32 _Count, const uint8 _Type, const uint8 _Location, const uint32 _Indices) {}
static M_INLINE void cellGcmSetDrawArraysInline(struct CellGcmContextData* _pContext, const uint8 _Mode, const uint32 _First, const uint32 _Count) {}
static M_INLINE void cellGcmSetInvalidateVertexCacheInline(struct CellGcmContextData* _pContext) {}

static M_INLINE void cellGcmSetFrontPolygonModeInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetBackPolygonModeInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetRestartIndexInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetRestartIndexEnableInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetFrontFaceInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetCullFaceInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetCullFaceEnableInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetClearSurfaceInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetClearColorInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetClearDepthStencilInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetColorMaskInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetColorMaskMrtInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetDepthTestEnableInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetDepthFuncInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetDepthMaskInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetBlendEnableInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetBlendEnableMrtInline(struct CellGcmContextData* _pContext, uint32 _Mrt1, uint32 _Mrt2, uint32 _Mrt3) {}
static M_INLINE void cellGcmSetBlendEquationInline(struct CellGcmContextData* _pContext, uint16 _Color, uint16 _Alpha) {}
static M_INLINE void cellGcmSetBlendFuncInline(struct CellGcmContextData* _pContext, uint16 _SrcColor, uint16 _DstColor, uint16 _SrcAlpha, uint16 _DstAlpha) {}
static M_INLINE void cellGcmSetShadeModeInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetStencilTestEnableInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetStencilFuncInline(struct CellGcmContextData* _pContext, uint32 _Func, uint32 _Ref, uint32 _Mask) {}
static M_INLINE void cellGcmSetStencilMaskInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetStencilOpInline(struct CellGcmContextData* _pContext, uint32 _Fail, uint32 _DepthFail, uint32 _DepthPass) {}
static M_INLINE void cellGcmSetTwoSidedStencilTestEnableInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetBackStencilFuncInline(struct CellGcmContextData* _pContext, uint32 _Func, uint32 _Ref, uint32 _Mask) {}
static M_INLINE void cellGcmSetBackStencilMaskInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetBackStencilOpInline(struct CellGcmContextData* _pContext, uint32 _Fail, uint32 _DepthFail, uint32 _DepthPass) {}
static M_INLINE void cellGcmSetAlphaTestEnableInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetAlphaFuncInline(struct CellGcmContextData* _pContext, uint32 _Func, uint32 _Ref) {}
static M_INLINE void cellGcmSetPolygonOffsetFillEnableInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetPolygonOffsetInline(struct CellGcmContextData* _pContext, fp32 _Factor, fp32 _Units) {}
static M_INLINE void cellGcmSetScissorInline(struct CellGcmContextData* _pContext, uint16 _X, uint16 _Y, uint16 _W, uint16 _H) {}
static M_INLINE void cellGcmSetTextureInline(struct CellGcmContextData* _pContext, uint8 _Index, const CellGcmTexture* _pTexture) {}
static M_INLINE void cellGcmSetUserClipPlaneControlInline(struct CellGcmContextData* _pContext, uint32 _Clp0, uint32 _Clp1, uint32 _Clp2, uint32 _Clp3, uint32 _Clp4, uint32 _Clp5) {}
static M_INLINE void cellGcmSetTextureAddressInline(struct CellGcmContextData* _pContext, const uint8 _Index, const uint8 _WrapS, const uint8 _WrapT, const uint8 _WrapR, const uint8 _UnsignedRemap, const uint8 _ZFunc, const uint8 _Gamma) {}
static M_INLINE void cellGcmSetTextureBorderColorInline(struct CellGcmContextData* _pContext, const uint8 _Index, uint32 _Color) {}
static M_INLINE void cellGcmSetTextureControlInline(struct CellGcmContextData* _pContext, const uint8 _Index, const uint32 _Enable, const uint16 _MinLod, const uint16 _MaxLod, const uint8 _MaxAniso) {}
static M_INLINE void cellGcmSetTextureFilterInline(struct CellGcmContextData* _pContext, const uint8 _Index, const uint16 _Bias, const uint8 _Min, const uint8 _Max, const uint8 _Conv) {}
static M_INLINE void cellGcmSetTransferLocationInline(struct CellGcmContextData* _pContext, const uint8 _Location) {}
static M_INLINE uint32 cellGcmSetTransferImageInline(struct CellGcmContextData* _pContext, const uint8 _Mode, const uint32 _DstOffset, const uint32 _DstPitch, const uint32 _DstX, const uint32 _DstY, const uint32 _SrcOffset, const uint32 _SrcPitch, const uint32 _SrcX, const uint32 _SrcY, const uint32 _Width, const uint32 _Height, const uint32 _BytesPerPixel) {return CELL_OK;}
static M_INLINE uint32 cellGcmSetTransferDataInline(struct CellGcmContextData* _pContext, const uint8 _Mode, const uint32 _DstOffset, const uint32 _DstPitch, const uint32 _SrcOffset, const uint32 _SrcPitch, const uint32 _BytesPerRow, const uint32 _RowCount) {return CELL_OK;}
static M_INLINE void cellGcmSetConvertSwizzleFormatInline(struct CellGcmContextData* _pContext, const uint32 _DstOffset, const uint32 _DstWidth, const uint32 _DstHeight, const uint32 _DstX, const uint32 _DstY, const uint32 _SrcOffset, const uint32 _SrcPitch, const uint32 _SrcX, const uint32 _SrcY, const uint32 _Width, const uint32 _Height, const uint32 _BytesPerPixel, const uint32 _Mode) {}
static M_INLINE void cellGcmConvertSwizzleFormatInline(void *swizzledTexture, const void *texture, const uint32 dstx0, const uint32 dsty0, const uint32 dstz0, const uint32 dstWidth, const uint32 dstHeight, const uint32 dstDepth, const uint32 srcx0, const uint32 srcy0, const uint32 srcz0, const uint32 srcx1, const uint32 srcy1, const uint32 srcz1, const uint32 srcWidth, const uint32 srcHeight, const uint32 srcDepth, const uint32 dstLog2cdepth, const uint32 srcColordepth, const uint32 border, const uint32 dim, void* _pData ) {}
static M_INLINE void cellGcmSetInvalidateTextureCacheInline(struct CellGcmContextData* _pContext, uint32 _Value) {}
static M_INLINE void cellGcmSetNopCommandInline(struct CellGcmContextData* _pContext, uint32 _WordCount) {}
static M_INLINE void cellGcmSetZcullInline(const uint8 index, const uint32 offset, const uint32 width, const uint32 height, const uint32 cullStart, const uint32 zFormat, const uint32 aaFormat, const uint32 zCullDir, const uint32 zCullFormat, const uint32 sFunc, const uint32 sRef, const uint32 sMask) {}
static M_INLINE void cellGcmSetInvalidateZcullInline(struct CellGcmContextData* _pContext) {}
static M_INLINE void cellGcmSetZcullStatsEnableInline(struct CellGcmContextData* _pContext, uint32) {}
static M_INLINE void cellGcmSetZMinMaxControlInline(struct CellGcmContextData* _pContext, uint32, uint32, uint32) {}
static M_INLINE void cellGcmSetZcullLimitInline(struct CellGcmContextData* _pContext, uint16, uint16) {}
static M_INLINE void cellGcmSetZcullControlInline(struct CellGcmContextData* _pContext, uint32, uint32) {}
static M_INLINE void cellGcmSetScullControlInline(struct CellGcmContextData* _pContext, uint32, uint32, uint32) {}
static M_INLINE void cellGcmHUDInitInline() {}
