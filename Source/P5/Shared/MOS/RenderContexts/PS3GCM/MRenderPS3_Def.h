#ifndef __INC_MRNDRGL_DEF
#define __INC_MRNDRGL_DEF

namespace NRenderPS3GCM
{

#define CRC_RIP_STREAM		-1
#define CRC_RIP_WIRES		-2

enum
{
	GPU_FRAMEBUFFER_ALIGNMENT		= 128,
	GPU_TILEDFRAMEBUFFER_ALIGNMENT	= 65536,
	GPU_VERTEXBUFFER_ALIGNMENT		= 16,
	GPU_FRAGMENTPROGRAM_ALIGNMENT	= 64,
	GPU_SWIZZLEDTEXTURE_ALIGNMENT	= 128,
	GPU_TILEDTEXTURE_ALIGNMENT		= 65536,


	GPU_REPORT_OCCLUSIONQUERY	= 128,
	GPU_REPORT_OCCLUSIONQUERY_MAX	= GPU_REPORT_OCCLUSIONQUERY + 128 * 3 * 2,
	GPU_REPORT_ZPASS_PIXELS_FRAME0,
	GPU_REPORT_ZPASS_PIXELS_FRAME1,
	GPU_REPORT_ZPASS_PIXELS_FRAME2,
	GPU_REPORT_ZPASS_PIXELS_FRAME3,
	GPU_REPORT_ZCULL_STATS_FRAME0,
	GPU_REPORT_ZCULL_STATS_FRAME1,
	GPU_REPORT_ZCULL_STATS_FRAME2,
	GPU_REPORT_ZCULL_STATS_FRAME3,
	GPU_REPORT_ZCULL_STATS1_FRAME0,
	GPU_REPORT_ZCULL_STATS1_FRAME1,
	GPU_REPORT_ZCULL_STATS1_FRAME2,
	GPU_REPORT_ZCULL_STATS1_FRAME3,
	GPU_REPORT_ZCULL_STATS2_FRAME0,
	GPU_REPORT_ZCULL_STATS2_FRAME1,
	GPU_REPORT_ZCULL_STATS2_FRAME2,
	GPU_REPORT_ZCULL_STATS2_FRAME3,
	GPU_REPORT_ZCULL_STATS3_FRAME0,
	GPU_REPORT_ZCULL_STATS3_FRAME1,
	GPU_REPORT_ZCULL_STATS3_FRAME2,
	GPU_REPORT_ZCULL_STATS3_FRAME3,

	GPU_REPORT_MAX,			// Must not exceed 2048

	GPU_LABEL_COLLECTER	= 64,
	GPU_LABEL_PAGEFLIP,
	GPU_LABEL_TRASH,
	GPU_LABEL_TEXLOADER,
	GPU_LABEL_OCCLUSIONQUERY,
	GPU_LABEL_OCCLUSIONQUERY_FRAME0 = GPU_LABEL_OCCLUSIONQUERY,
	GPU_LABEL_OCCLUSIONQUERY_FRAME1,
	GPU_LABEL_OCCLUSIONQUERY_FRAME2,
};

#define REMAP_INPUT(_a_, _r_, _g_, _b_) ((CELL_GCM_TEXTURE_REMAP_FROM_##_a_) | (CELL_GCM_TEXTURE_REMAP_FROM_##_r_ << 2) | (CELL_GCM_TEXTURE_REMAP_FROM_##_g_ << 4) | (CELL_GCM_TEXTURE_REMAP_FROM_##_b_ << 6))
#define REMAP_OUTPUT(_a_, _r_, _g_, _b_) ((CELL_GCM_TEXTURE_REMAP_##_a_ << 8) | (CELL_GCM_TEXTURE_REMAP_##_r_ << 10) | (CELL_GCM_TEXTURE_REMAP_##_g_ << 12) | (CELL_GCM_TEXTURE_REMAP_##_b_ << 14))


};

#endif // __INC_MRNDRGL_DEF
