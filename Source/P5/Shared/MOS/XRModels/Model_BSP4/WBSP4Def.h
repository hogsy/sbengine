
#define MODEL_BSP_PARAM_GLOBALFLAGS		0x0200

#define MODEL_BSP_ENABLE_BUMP	1
#define MODEL_BSP_ENABLE_DETAIL	2
#define MODEL_BSP_ENABLE_MINTESSELATION 4
//#define MODEL_BSP_ENABLE_FOG	4
//#define MODEL_BSP_ENABLE_SKY	8
#define MODEL_BSP_ENABLE_NOPORTALSTATS	16
#define MODEL_BSP_ENABLE_SPLINEWIRE 32
#define MODEL_BSP_ENABLE_FULLBRIGHT	64
#define MODEL_BSP_ENABLE_PVSCULL	128
#define MODEL_BSP_ENABLE_SPLINEBRUSHES 256


#define XW_PORTALLEAF_FOGPORTALDISABLE	0x10000
//#define XW_PORTALLEAF_FOGPORTAL			0x20000


// -------------------------------------------------------------------
//#define PORTAL_NO_BSP_CULL				// Unused

/*
BSP-Culling doesn't work if the ClipVolume's vertices aren't scaled to lay 
on the backplane. This is normaly done in CClipVolume::CreateFromVertices(), but
is now disabled to save portal-time.
*/

#define MODEL_BSP_MAXPORTALLEAFFACES 4096
#define MODEL_BSP_BUMP
//#define MODEL_BSP_MICROTEXTURE

#define MODEL_BSP_CFV3					// Use CreateFromVertices3

#define MODEL_BSP_PORTALFACECULL		// Exact face-culling to portals
#define MODEL_BSP_DYNAMIC_SHADOWS		// Real-time shadows allowed.
#define MODEL_BSP_RAYTRACE_STRUCTURE	// Real-time shadows from structure only
#define MODEL_BSP_PORTALBOUNDBOX

#define MODEL_BSP_LIGHTATTENUATIONCOS	// 1/4 Cosine or 1/2 Sine attenuation.
#define MODEL_BSP_LIGHTATTENUATIONMIN -1.0f // 1/4 Cosine or 1/2 Sine attenuation.

#define MODEL_BSP_MAXDYNAMICLIGHTFACES 2048	// Maximum number of faces that can be lit by 
										// dynamic lights.

//#define MODEL_BSP_SORTLEAVES			// Render leaves in correct order.
//#define MODEL_BSP_CULLDETAILBSP			// Cull bsp-splits on detail-level.

//#define MODEL_BSP_USEPLANETYPES

#if defined(PLATFORM_PS2) 
// || defined(PLATFORM_XBOX) The algorithm does not produce the polygons in the same order every time which leads to bad z fighting
	#define MODEL_BSP_USEKNITSTRIP
#endif

#define MODEL_BSP_MAXKNITSTRIP		253

#ifndef PLATFORM_PS2
	#define MODEL_BSP_KNITSTRIPCULLFIX
#endif

#ifndef	PLATFORM_PS2
#define	BSP4_MODEL_PHYS_RENDER
#endif
// -------------------------------------------------------------------
//#define MODEL_BSP_RENDERNODEPORTALS		// Debug: Render portals in camera's portal-leaf.
#define MODEL_BSP_PARANOIA				// Debug: Extended error handling.

//#define MODEL_BSP_ALLPORTALSOPEN			// Debug: All portal IDs are ignored.

//#define MODEL_BSP_POLYGONTIMING			// Debug
#ifdef M_Profile
#define MODEL_BSP_WRITEPORTALTIMING		// Debug: Log's portal-traversal stats to the console.
#endif
//#define MODEL_BSP_ONETEXTURE			// Debug: All faces uses texture 0.

//#define MODEL_BSP_RENDERALL
//#define MODEL_BSP_CLIP2PORTAL			// Clip polygons to portals



