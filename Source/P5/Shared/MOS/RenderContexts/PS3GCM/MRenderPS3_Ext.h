
#ifndef __INC_MRNDRGL_EXT
#define __INC_MRNDRGL_EXT

typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;

// -------------------------------------------------------------------
// GL 1.4
// -------------------------------------------------------------------
#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT32              0x81A7

#define	GL_GENERATE_MIPMAP					0x8191
#define	GL_GENERATE_MIPMAP_HINT				0x8192

// -------------------------------------------------------------------
// EXT_TEXTURE3D
// -------------------------------------------------------------------
#define GL_TEXTURE_WRAP_R					0x8072

// -------------------------------------------------------------------
// EXT_TEXTURE_EDGE_CLAMP
// -------------------------------------------------------------------
#define GL_CLAMP_TO_EDGE					0x812f

// -------------------------------------------------------------------
// ARB_MULTITEXTURE, SGIS_MULTITEXTURE & EXT_MULTITEXTURE functions
// -------------------------------------------------------------------
typedef void(APIENTRY* PFNMULTITEXCOORD2F)(GLenum target, GLfloat s, GLfloat t);
typedef void(APIENTRY* PFNMULTITEXCOORD2FV)(GLenum target, const GLfloat *v);
typedef void(APIENTRY* PFNMULTITEXCOORD3F)(GLenum target, GLfloat s, GLfloat t, GLfloat r);
typedef void(APIENTRY* PFNMULTITEXCOORD3FV)(GLenum target, const GLfloat *v);
typedef void(APIENTRY* PFNMULTITEXCOORD4F)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
typedef void(APIENTRY* PFNMULTITEXCOORD4FV)(GLenum target, const GLfloat *v);

typedef void(APIENTRY* PFNMULTITEXCOORDPOINTER)(GLenum target, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void(APIENTRY* PFNSELECTTEXTURE)(GLenum target);
typedef void(APIENTRY* PFNSELECTTEXTURECOORDSET)(GLenum target);

// -------------------------------------------------------------------
//  ARB_MULTITEXTURE
// -------------------------------------------------------------------

// Accepted by the <value> parameter of GetIntegerv,GetFloatv, and GetDoublev:
#define GL_ACTIVE_TEXTURE_ARB				0x84e0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB		0x84e1
#define GL_MAX_TEXTURE_UNITS_ARB			0x84e2

// Accepted by the <target> parameter of
//		glActiveTextureARB 
//		glClientActiveTextureARB

#define GL_TEXTURE0_ARB						0x84c0
#define GL_TEXTURE1_ARB						0x84c1
#define GL_TEXTURE2_ARB						0x84c2
#define GL_TEXTURE3_ARB						0x84c3
#define GL_TEXTURE4_ARB						0x84c4
#define GL_TEXTURE5_ARB						0x84c5
#define GL_TEXTURE6_ARB						0x84c6
#define GL_TEXTURE7_ARB						0x84c7
#define GL_TEXTURE8_ARB						0x84c8
#define GL_TEXTURE9_ARB						0x84c9
#define GL_TEXTURE10_ARB					0x84ca
#define GL_TEXTURE11_ARB					0x84cb
#define GL_TEXTURE12_ARB					0x84cc
#define GL_TEXTURE13_ARB					0x84cd
#define GL_TEXTURE14_ARB					0x84ce
#define GL_TEXTURE15_ARB					0x84cf
#define GL_TEXTURE16_ARB					0x84d0
#define GL_TEXTURE17_ARB					0x84d1
#define GL_TEXTURE18_ARB					0x84d2
#define GL_TEXTURE19_ARB					0x84d3
#define GL_TEXTURE20_ARB					0x84d4
#define GL_TEXTURE21_ARB					0x84d5
#define GL_TEXTURE22_ARB					0x84d6
#define GL_TEXTURE23_ARB					0x84d7
#define GL_TEXTURE24_ARB					0x84d8
#define GL_TEXTURE25_ARB					0x84d9
#define GL_TEXTURE26_ARB					0x84da
#define GL_TEXTURE27_ARB					0x84db
#define GL_TEXTURE28_ARB					0x84dc
#define GL_TEXTURE29_ARB					0x84dd
#define GL_TEXTURE30_ARB					0x84de
#define GL_TEXTURE31_ARB					0x84df

// -------------------------------------------------------------------
//  GL_EXT_COMPILED_VERTEX_ARRAY
// -------------------------------------------------------------------
typedef void(APIENTRY* PFNLOCKARRAYS)(GLint first, GLsizei count);
typedef void(APIENTRY* PFNUNLOCKARRAYS)(void);

// -------------------------------------------------------------------
//  ARB_TEXTURE_COMPRESSION
// -------------------------------------------------------------------

// Accepted by the <internalformat> parameter of TexImage1D, TexImage2D, TexImage3D, CopyTexImage1D, and CopyTexImage2D
#define GL_COMPRESSED_ALPHA_ARB				0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB			0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB	0x84EB
#define GL_COMPRESSED_INTENSITY_ARB			0x84EC
#define GL_COMPRESSED_RGB_ARB				0x84ED
#define GL_COMPRESSED_RGBA_ARB				0x84EE

// Accepted by the <target> parameter of Hint and the <value> parameter of GetIntegerv, GetBooleanv, GetFloatv, and GetDoublev:
#define GL_TEXTURE_COMPRESSION_HINT_ARB		0x84EF

// Accepted by the <value> parameter of GetTexLevelParameter:
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB 0x86A0
#define GL_TEXTURE_COMPRESSED_ARB			0x86A1

// Accepted by the <value> parameter of GetIntegerv, GetBooleanv, GetFloatv and GetDoublev:
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB	0x86A3

typedef void(APIENTRY* PFNCOMPRESSEDTEXIMAGE3DARB)(int target, int level, int internalformat, int width, int height, int depth, int border, int imageSize, const void *data);
typedef void(APIENTRY* PFNCOMPRESSEDTEXIMAGE2DARB)(int target, int level, int internalformat, int width, int height, int border,  int imageSize, const void *data);
typedef void(APIENTRY* PFNCOMPRESSEDTEXIMAGE1DARB)(int target, int level, int internalformat, int width, int border, int imageSize, const void *data);
typedef void(APIENTRY* PFNCOMPRESSEDTEXSUBIMAGE3DARB)(int target, int level, int xoffset, int yoffset, int zoffset, int width, int height, int depth, int format, int imageSize, const void *data);
typedef void(APIENTRY* PFNCOMPRESSEDTEXSUBIMAGE2DARB)(int target, int level, int xoffset, int yoffset, int width, int height, int format, int imageSize, const void *data);
typedef void(APIENTRY* PFNCOMPRESSEDTEXSUBIMAGE1DARB)(int target, int level, int xoffset, int width, int format, int imageSize, const void *data);
typedef void(APIENTRY* PFNGETCOMPRESSEDTEXIMAGEARB)(int target, int lod, const void *img);

// -------------------------------------------------------------------
//  EXT_TEXTURE_COMPRESSION_S3TC
// -------------------------------------------------------------------

enum
{
	GL_COMPRESSED_RGB_S3TC_DXT1_EXT			= 0x83f0,
	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT		= 0x83f1,
	GL_COMPRESSED_RGBA_S3TC_DXT3_EXT		= 0x83f2,
	GL_COMPRESSED_RGBA_S3TC_DXT5_EXT		= 0x83f3,
};

// -------------------------------------------------------------------
//  ARB_TEXTURE_CUBE_MAP
// -------------------------------------------------------------------

enum
{
	// TexGen modes
	GL_NORMAL_MAP_ARB						= 0x8511,
	GL_REFLECTION_MAP_ARB					= 0x8512,

	// Texture target
	GL_TEXTURE_CUBE_MAP_ARB					= 0x8513,

	// Getxxxx texture target
	GL_BINDING_CUBE_MAP_ARB					= 0x8514,

	// GetTex*** target
	GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB		= 0x8515,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB		= 0x8516,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB		= 0x8517,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB		= 0x8518,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB		= 0x8519,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB		= 0x851a,

	GL_PROXY_TEXTURE_CUBE_MAP_ARB			= 0x851b,

	GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB		= 0x851c,
};

// -------------------------------------------------------------------
//  DeviceGammaRamp
// -------------------------------------------------------------------
typedef void(APIENTRY* PFNSETDEVICEGAMMA)(void* _hDC, void* _pRamp);
typedef void(APIENTRY* PFNGETDEVICEGAMMA)(void* _hDC, void* _pRamp);

// -------------------------------------------------------------------
//  WGL_SWAP_CONTROL
// -------------------------------------------------------------------
typedef void(APIENTRY* PFNSWAPINTERVAL)(int);
typedef int(APIENTRY* PFNGETSWAPINTERVAL)(void);

// -------------------------------------------------------------------
//  EXT_SECONDARY_COLOR
// -------------------------------------------------------------------
typedef void(APIENTRY* PFNSECONDARYCOLOR3F)(GLfloat r, GLfloat g, GLfloat b);
typedef void(APIENTRY* PFNSECONDARYCOLOR3FV)(const GLfloat* p);
typedef void(APIENTRY* PFNSECONDARYCOLORPOINTER)(GLsizei size, GLenum type, GLsizei stride, GLvoid* pointer);

enum
{
	GL_COLOR_SUM_EXT						= 0x8458,		// Accepted by the <cap> parameter of Enable, Disable, and IsEnabled,
														// and by the <pname> parameter of GetBooleanv, GetIntegerv, GetFloatv, and GetDoublev:

	GL_CURRENT_SECONDARY_COLOR_EXT			= 0x8459,		// Accepted by the <pname> parameter of GetBooleanv, GetIntegerv, GetFloatv, and GetDoublev:
	GL_SECONDARY_COLOR_ARRAY_SIZE_EXT		= 0x845a,
	GL_SECONDARY_COLOR_ARRAY_TYPE_EXT		= 0x845b,
	GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT		= 0x845c,

	GL_SECONDARY_COLOR_ARRAY_POINTER_EXT	= 0x845D,		// Accepted by the <pname> parameter of GetPointerv:

	GL_SECONDARY_COLOR_ARRAY_EXT			= 0x845E		// Accepted by the <array> parameter of EnableClientState and DisableClientState:
};

// -------------------------------------------------------------------
//  EXT_FOG_COORD
// -------------------------------------------------------------------
typedef void(APIENTRY* PFNFOGCOORDF)(GLfloat FogZ);
typedef void(APIENTRY* PFNFOGCOORDPOINTER)(GLenum type, GLsizei stride, GLvoid* pointer);

enum
{
	GL_FOG_COORDINATE_SOURCE_EXT			= 0x8450,		// Accepted by the <pname> parameter of Fogi and Fogf

	GL_FOG_COORDINATE_EXT					= 0x8451,		// Accepted by the <param> parameter of Fogi and Fogf
	GL_FRAGMENT_DEPTH_EXT					= 0x8452,			

	GL_CURRENT_FOG_COORDINATE_EXT			= 0x8453,		// Accepted by the <pname> parameter of GetBooleanv, GetIntegerv, GetFloatv, and GetDoublev
	GL_FOG_COORDINATE_ARRAY_TYPE_EXT		= 0x8454,
	GL_FOG_COORDINATE_ARRAY_STRIDE_EXT		= 0x8455,

	GL_FOG_COORDINATE_ARRAY_POINTER_EXT		= 0x8456,		// Accepted by the <pname> parameter of GetPointerv
	GL_FOG_COORDINATE_ARRAY_EXT				= 0x8457		// Accepted by the <array> parameter of EnableClientState and DisableClientState
};

// -------------------------------------------------------------------
//  NV_VERTEX_ARRAY_RANGE
// -------------------------------------------------------------------
typedef void (APIENTRY* PFNVERTEXARRAYRANGENV)(int length, void *pointer);
typedef void (APIENTRY* PFNFLUSHVERTEXARRAYRANGENV)(void);

typedef void* (APIENTRY* PFNWGLALLOCATEMEMORYNV)(int size, float readFrequency, float writeFrequency, float priority);
typedef void (APIENTRY* PFNWGLFREEMEMORYNV)(void *pointer);

enum
{
	// Accepted by the <cap> parameter of EnableClientState, DisableClientState, and IsEnabled:
	GL_VERTEX_ARRAY_RANGE_NV				= 0x851d,

	// Accepted by the <pname> parameter of GetBooleanv, GetIntegerv,GetFloatv, and GetDoublev:
	GL_VERTEX_ARRAY_RANGE_LENGTH_NV			= 0x851e,
	GL_VERTEX_ARRAY_RANGE_VALID_NV			= 0x851f,
	GL_MAX_VERTEX_ARRAY_RANGE_ELEMENT_NV	= 0x8520,

	// Accepted by the <pname> parameter of GetPointerv:
	GL_VERTEX_ARRAY_RANGE_POINTER_NV		= 0x8521,
};

// -------------------------------------------------------------------
//  NV_FENCE
// -------------------------------------------------------------------
typedef void (APIENTRY* PFNGENFENCESNV)(GLsizei count, GLuint *fences);
typedef void (APIENTRY* PFNDELETEFENCESNV)(GLsizei count, GLuint *fences);
typedef void (APIENTRY* PFNSETFENCENV)(GLuint fence, GLenum condition);
typedef bool (APIENTRY* PFNTESTFENCENV)(GLuint fence);
typedef void (APIENTRY* PFNFINISHFENCENV)(GLuint fence);
typedef bool (APIENTRY* PFNISFENCENV)(GLuint fence);
typedef void (APIENTRY* PFNGETFENCEIVNV)(GLuint fence, GLenum pname, GLint* params);

enum
{
	GL_ALL_COMPLETED_NV						= 0x84f2,
	GL_FENCE_STATUS_NV						= 0x84f3,
	GL_FENCE_CONDITION_NV					= 0x84f4,
};

// -------------------------------------------------------------------
//  NV_REGISTER_COMBINERS
// -------------------------------------------------------------------
#include "../MSystem/Raster/MRender_NVidia.h"

enum
{
	GL_REGISTER_COMBINERS_NV				= 0x8522,

	GL_COMBINER_INPUT_NV					= 0x8542,
	GL_COMBINER_MAPPING_NV					= 0x8543,
	GL_COMBINER_COMPONENT_USAGE_NV			= 0x8544,

	GL_MAX_GENERAL_COMBINERS_NV				= 0x854d,

	GL_CONSTANT_COLOR0_NV					= 0x852a,
	GL_CONSTANT_COLOR1_NV					= 0x852b,
	GL_NUM_GENERAL_COMBINERS_NV				= 0x854e,
	GL_COLOR_SUM_CLAMP_NV					= 0x854f,

	// Accepted by the <stage> parameter of CombinerStageParameterfvNV and
	// GetCombinerStageParameterfvNV:
	GL_COMBINER0_NV							= 0x8550,
	GL_COMBINER1_NV,
	GL_COMBINER2_NV,
	GL_COMBINER3_NV,
	GL_COMBINER4_NV,
	GL_COMBINER5_NV,
	GL_COMBINER6_NV,
	GL_COMBINER7_NV,
};

typedef void(APIENTRY* PFNCOMBINERPARAMETERFVNV)(GLenum target, float*);
typedef void(APIENTRY* PFNCOMBINERPARAMETERIVNV)(GLenum target, int*);
typedef void(APIENTRY* PFNCOMBINERPARAMETERFNV)(GLenum target, float);
typedef void(APIENTRY* PFNCOMBINERPARAMETERINV)(GLenum target, int);
typedef void(APIENTRY* PFNCOMBINERINPUTNV)(GLenum target, GLenum portion, GLenum var, GLenum input, GLenum mapping, GLenum componentusage);
typedef void(APIENTRY* PFNCOMBINEROUTPUTNV)(GLenum stage, GLenum portion, GLenum abOut, GLenum cdOut, GLenum sumOut, GLenum scale, GLenum bias, bool abDot, bool cdDot, bool muxsum);
typedef void(APIENTRY* PFNFINALCOMBINERINPUTNV)(GLenum var, GLenum input, GLenum mapping, GLenum componentusage);
typedef void(APIENTRY* PFNGETCOMBINERINPUTPARAMETERFVNV)(GLenum stage, GLenum portion, GLenum var, GLenum pname, float* params);
typedef void(APIENTRY* PFNGETCOMBINERINPUTPARAMETERIVNV)(GLenum stage, GLenum portion, GLenum var, GLenum pname, int* params);
typedef void(APIENTRY* PFNGETCOMBINEROUTPUTPARAMETERFVNV)(GLenum stage, GLenum portion, GLenum pname, float* params);
typedef void(APIENTRY* PFNGETCOMBINEROUTPUTPARAMETERIVNV)(GLenum stage, GLenum portion, GLenum pname, int* params);
typedef void(APIENTRY* PFNGETFINALCOMBINERINPUTPARAMETERFVNV)(GLenum var, GLenum pname, float* params);
typedef void(APIENTRY* PFNGETFINALCOMBINERINPUTPARAMETERIVNV)(GLenum var, GLenum pname, int* params);

// -------------------------------------------------------------------
//  NV_REGISTER_COMBINERS2
// -------------------------------------------------------------------
enum
{
	// Accepted by the <cap> parameter of Disable, Enable, and IsEnabled,
	// and by the <pname> parameter of GetBooleanv, GetIntegerv, GetFloatv,
	// and GetDoublev:
	GL_PER_STAGE_CONSTANTS_NV				= 0x8535,
};

typedef void(APIENTRY* PFNCOMBINERSTAGEPARAMETERFVNV)(GLenum stage, GLenum pname, const GLfloat *params);
typedef void(APIENTRY* PFNGETCOMBINERSTAGEPARAMETERFVNV)(GLenum stage, GLenum pname, GLfloat *params);

// -------------------------------------------------------------------
//  NV_TEXTURE_SHADER
// -------------------------------------------------------------------
enum
{
	GL_RGBA_UNSIGNED_DOT_PRODUCT_MAPPING_NV	= 0x86D9,
	GL_UNSIGNED_INT_S8_S8_8_8_NV			= 0x86DA,
	GL_UNSIGNED_INT_S8_S8_8_8_REV_NV		= 0x86DB,
	GL_DSDT_MAG_INTENSITY_NV				= 0x86DC,
	GL_SHADER_CONSISTENT_NV					= 0x86DD,
	GL_TEXTURE_SHADER_NV					= 0x86DE,
	GL_SHADER_OPERATION_NV					= 0x86DF,
	GL_CULL_MODES_NV						= 0x86E0,
	GL_OFFSET_TEXTURE_2D_MATRIX_NV			= 0x86E1,
	GL_OFFSET_TEXTURE_2D_SCALE_NV			= 0x86E2,
	GL_OFFSET_TEXTURE_2D_BIAS_NV			= 0x86E3,
	GL_PREVIOUS_TEXTURE_INPUT_NV			= 0x86E4,
	GL_HILO_NV								= 0x86F4,
	GL_DSDT_NV								= 0x86F5,
	GL_DSDT_MAG_NV							= 0x86F6,
	GL_DSDT_MAG_VIB_NV						= 0x86F7,
	GL_HILO16_NV							= 0x86F8,
	GL_SIGNED_HILO_NV						= 0x86F9,
	GL_SIGNED_HILO16_NV						= 0x86FA,
	GL_SIGNED_RGBA_NV						= 0x86FB,
	GL_SIGNED_RGBA8_NV						= 0x86FC,
	GL_SIGNED_RGB_NV						= 0x86FE,
	GL_SIGNED_RGB8_NV						= 0x86FF,
	GL_SIGNED_LUMINANCE_NV					= 0x8701,
	GL_SIGNED_LUMINANCE8_NV					= 0x8702,
	GL_SIGNED_LUMINANCE_ALPHA_NV			= 0x8703,
	GL_SIGNED_LUMINANCE8_ALPHA8_NV			= 0x8704,
	GL_SIGNED_ALPHA_NV						= 0x8705,
	GL_SIGNED_ALPHA8_NV						= 0x8706,
	GL_SIGNED_INTENSITY_NV					= 0x8707,
	GL_SIGNED_INTENSITY8_NV					= 0x8708,
	GL_DSDT8_NV								= 0x8709,
	GL_DSDT8_MAG8_NV						= 0x870A,
	GL_DSDT8_MAG8_INTENSITY8_NV				= 0x870B,
	GL_SIGNED_RGB_UNSIGNED_ALPHA_NV			= 0x870C,
	GL_SIGNED_RGB8_UNSIGNED_ALPHA8_NV		= 0x870D,
	GL_HI_SCALE_NV							= 0x870E,
	GL_LO_SCALE_NV							= 0x870F,
	GL_DS_SCALE_NV							= 0x8710,
	GL_DT_SCALE_NV							= 0x8711,
	GL_MAGNITUDE_SCALE_NV					= 0x8712,
	GL_VIBRANCE_SCALE_NV					= 0x8713,
	GL_HI_BIAS_NV							= 0x8714,
	GL_LO_BIAS_NV							= 0x8715,
	GL_DS_BIAS_NV							= 0x8716,
	GL_DT_BIAS_NV							= 0x8717,
	GL_MAGNITUDE_BIAS_NV					= 0x8718,
	GL_VIBRANCE_BIAS_NV						= 0x8719,
	GL_TEXTURE_BORDER_VALUES_NV				= 0x871A,
	GL_TEXTURE_HI_SIZE_NV					= 0x871B,
	GL_TEXTURE_LO_SIZE_NV					= 0x871C,
	GL_TEXTURE_DS_SIZE_NV					= 0x871D,
	GL_TEXTURE_DT_SIZE_NV					= 0x871E,
	GL_TEXTURE_MAG_SIZE_NV					= 0x871F,
};

// -------------------------------------------------------------------
//  NV_VERTEX_PROGRAM
// -------------------------------------------------------------------
enum
{
	GL_VERTEX_PROGRAM_NV					= 0x8620,
	GL_VERTEX_STATE_PROGRAM_NV				= 0x8621,
	GL_ATTRIB_ARRAY_SIZE_NV					= 0x8623,
	GL_ATTRIB_ARRAY_STRIDE_NV				= 0x8624,
	GL_ATTRIB_ARRAY_TYPE_NV					= 0x8625,
	GL_CURRENT_ATTRIB_NV					= 0x8626,
	GL_PROGRAM_LENGTH_NV					= 0x8627,
	GL_PROGRAM_STRING_NV					= 0x8628,
	GL_MODELVIEW_PROJECTION_NV				= 0x8629,
	GL_IDENTITY_NV							= 0x862A,
	GL_INVERSE_NV							= 0x862B,
	GL_TRANSPOSE_NV							= 0x862C,
	GL_INVERSE_TRANSPOSE_NV					= 0x862D,
	GL_MAX_TRACK_MATRIX_STACK_DEPTH_NV		= 0x862E,
	GL_MAX_TRACK_MATRICES_NV				= 0x862F,
	GL_MATRIX0_NV							= 0x8630,
	GL_MATRIX1_NV							= 0x8631,
	GL_MATRIX2_NV							= 0x8632,
	GL_MATRIX3_NV							= 0x8633,
	GL_MATRIX4_NV							= 0x8634,
	GL_MATRIX5_NV							= 0x8635,
	GL_MATRIX6_NV							= 0x8636,
	GL_MATRIX7_NV							= 0x8637,
	GL_CURRENT_MATRIX_STACK_DEPTH_NV		= 0x8640,
	GL_CURRENT_MATRIX_NV					= 0x8641,
	GL_VERTEX_PROGRAM_POINT_SIZE_NV			= 0x8642,
	GL_VERTEX_PROGRAM_TWO_SIDE_NV			= 0x8643,
	GL_PROGRAM_PARAMETER_NV					= 0x8644,
	GL_ATTRIB_ARRAY_POINTER_NV				= 0x8645,
	GL_PROGRAM_TARGET_NV					= 0x8646,
	GL_PROGRAM_RESIDENT_NV					= 0x8647,
	GL_TRACK_MATRIX_NV						= 0x8648,
	GL_TRACK_MATRIX_TRANSFORM_NV			= 0x8649,
	GL_VERTEX_PROGRAM_BINDING_NV			= 0x864A,
	GL_PROGRAM_ERROR_POSITION_NV			= 0x864B,
	GL_VERTEX_ATTRIB_ARRAY0_NV				= 0x8650,
	GL_VERTEX_ATTRIB_ARRAY1_NV				= 0x8651,
	GL_VERTEX_ATTRIB_ARRAY2_NV				= 0x8652,
	GL_VERTEX_ATTRIB_ARRAY3_NV				= 0x8653,
	GL_VERTEX_ATTRIB_ARRAY4_NV				= 0x8654,
	GL_VERTEX_ATTRIB_ARRAY5_NV				= 0x8655,
	GL_VERTEX_ATTRIB_ARRAY6_NV				= 0x8656,
	GL_VERTEX_ATTRIB_ARRAY7_NV				= 0x8657,
	GL_VERTEX_ATTRIB_ARRAY8_NV				= 0x8658,
	GL_VERTEX_ATTRIB_ARRAY9_NV				= 0x8659,
	GL_VERTEX_ATTRIB_ARRAY10_NV				= 0x865A,
	GL_VERTEX_ATTRIB_ARRAY11_NV				= 0x865B,
	GL_VERTEX_ATTRIB_ARRAY12_NV				= 0x865C,
	GL_VERTEX_ATTRIB_ARRAY13_NV				= 0x865D,
	GL_VERTEX_ATTRIB_ARRAY14_NV				= 0x865E,
	GL_VERTEX_ATTRIB_ARRAY15_NV				= 0x865F,
	GL_MAP1_VERTEX_ATTRIB0_4_NV				= 0x8660,
	GL_MAP1_VERTEX_ATTRIB1_4_NV				= 0x8661,
	GL_MAP1_VERTEX_ATTRIB2_4_NV				= 0x8662,
	GL_MAP1_VERTEX_ATTRIB3_4_NV				= 0x8663,
	GL_MAP1_VERTEX_ATTRIB4_4_NV				= 0x8664,
	GL_MAP1_VERTEX_ATTRIB5_4_NV				= 0x8665,
	GL_MAP1_VERTEX_ATTRIB6_4_NV				= 0x8666,
	GL_MAP1_VERTEX_ATTRIB7_4_NV				= 0x8667,
	GL_MAP1_VERTEX_ATTRIB8_4_NV				= 0x8668,
	GL_MAP1_VERTEX_ATTRIB9_4_NV				= 0x8669,
	GL_MAP1_VERTEX_ATTRIB10_4_NV			= 0x866A,
	GL_MAP1_VERTEX_ATTRIB11_4_NV			= 0x866B,
	GL_MAP1_VERTEX_ATTRIB12_4_NV			= 0x866C,
	GL_MAP1_VERTEX_ATTRIB13_4_NV			= 0x866D,
	GL_MAP1_VERTEX_ATTRIB14_4_NV			= 0x866E,
	GL_MAP1_VERTEX_ATTRIB15_4_NV			= 0x866F,
	GL_MAP2_VERTEX_ATTRIB0_4_NV				= 0x8670,
	GL_MAP2_VERTEX_ATTRIB1_4_NV				= 0x8671,
	GL_MAP2_VERTEX_ATTRIB2_4_NV				= 0x8672,
	GL_MAP2_VERTEX_ATTRIB3_4_NV				= 0x8673,
	GL_MAP2_VERTEX_ATTRIB4_4_NV				= 0x8674,
	GL_MAP2_VERTEX_ATTRIB5_4_NV				= 0x8675,
	GL_MAP2_VERTEX_ATTRIB6_4_NV				= 0x8676,
	GL_MAP2_VERTEX_ATTRIB7_4_NV				= 0x8677,
	GL_MAP2_VERTEX_ATTRIB8_4_NV				= 0x8678,
	GL_MAP2_VERTEX_ATTRIB9_4_NV				= 0x8679,
	GL_MAP2_VERTEX_ATTRIB10_4_NV			= 0x867A,
	GL_MAP2_VERTEX_ATTRIB11_4_NV			= 0x867B,
	GL_MAP2_VERTEX_ATTRIB12_4_NV			= 0x867C,
	GL_MAP2_VERTEX_ATTRIB13_4_NV			= 0x867D,
	GL_MAP2_VERTEX_ATTRIB14_4_NV			= 0x867E,
	GL_MAP2_VERTEX_ATTRIB15_4_NV			= 0x867F,
};

typedef GLboolean (APIENTRY * PFNGLAREPROGRAMSRESIDENTNVPROC) (GLsizei n, const GLuint *programs, GLboolean *residences);
typedef void (APIENTRY * PFNGLBINDPROGRAMNVPROC) (GLenum target, GLuint id);
typedef void (APIENTRY * PFNGLDELETEPROGRAMSNVPROC) (GLsizei n, const GLuint *programs);
typedef void (APIENTRY * PFNGLEXECUTEPROGRAMNVPROC) (GLenum target, GLuint id, const GLfloat *params);
typedef void (APIENTRY * PFNGLGENPROGRAMSNVPROC) (GLsizei n, GLuint *programs);
typedef void (APIENTRY * PFNGLGETPROGRAMPARAMETERDVNVPROC) (GLenum target, GLuint index, GLenum pname, GLdouble *params);
typedef void (APIENTRY * PFNGLGETPROGRAMPARAMETERFVNVPROC) (GLenum target, GLuint index, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETPROGRAMIVNVPROC) (GLuint id, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETPROGRAMSTRINGNVPROC) (GLuint id, GLenum pname, GLubyte *program);
typedef void (APIENTRY * PFNGLGETTRACKMATRIXIVNVPROC) (GLenum target, GLuint address, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETVERTEXATTRIBDVNVPROC) (GLuint index, GLenum pname, GLdouble *params);
typedef void (APIENTRY * PFNGLGETVERTEXATTRIBFVNVPROC) (GLuint index, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETVERTEXATTRIBIVNVPROC) (GLuint index, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETVERTEXATTRIBPOINTERVNVPROC) (GLuint index, GLenum pname, GLvoid* *pointer);
typedef GLboolean (APIENTRY * PFNGLISPROGRAMNVPROC) (GLuint id);
typedef void (APIENTRY * PFNGLLOADPROGRAMNVPROC) (GLenum target, GLuint id, GLsizei len, const GLubyte *program);
typedef void (APIENTRY * PFNGLPROGRAMPARAMETER4DNVPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PFNGLPROGRAMPARAMETER4DVNVPROC) (GLenum target, GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLPROGRAMPARAMETER4FNVPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLPROGRAMPARAMETER4FVNVPROC) (GLenum target, GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLPROGRAMPARAMETERS4DVNVPROC) (GLenum target, GLuint index, GLsizei count, const GLdouble *v);
typedef void (APIENTRY * PFNGLPROGRAMPARAMETERS4FVNVPROC) (GLenum target, GLuint index, GLsizei count, const GLfloat *v);
typedef void (APIENTRY * PFNGLREQUESTRESIDENTPROGRAMSNVPROC) (GLsizei n, const GLuint *programs);
typedef void (APIENTRY * PFNGLTRACKMATRIXNVPROC) (GLenum target, GLuint address, GLenum matrix, GLenum transform);
typedef void (APIENTRY * PFNGLVERTEXATTRIBPOINTERNVPROC) (GLuint index, GLint fsize, GLenum type, GLsizei stride, const GLvoid *pointer);
/*
typedef void (APIENTRY * PFNGLVERTEXATTRIB1DNVPROC) (GLuint index, GLdouble x);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1DVNVPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1FNVPROC) (GLuint index, GLfloat x);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1FVNVPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1SNVPROC) (GLuint index, GLshort x);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1SVNVPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2DNVPROC) (GLuint index, GLdouble x, GLdouble y);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2DVNVPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2FNVPROC) (GLuint index, GLfloat x, GLfloat y);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2FVNVPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2SNVPROC) (GLuint index, GLshort x, GLshort y);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2SVNVPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3DNVPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3DVNVPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3FNVPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3FVNVPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3SNVPROC) (GLuint index, GLshort x, GLshort y, GLshort z);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3SVNVPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4DNVPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4DVNVPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4FNVPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4FVNVPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4SNVPROC) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4SVNVPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4UBVNVPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS1DVNVPROC) (GLuint index, GLsizei count, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS1FVNVPROC) (GLuint index, GLsizei count, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS1SVNVPROC) (GLuint index, GLsizei count, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS2DVNVPROC) (GLuint index, GLsizei count, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS2FVNVPROC) (GLuint index, GLsizei count, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS2SVNVPROC) (GLuint index, GLsizei count, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS3DVNVPROC) (GLuint index, GLsizei count, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS3FVNVPROC) (GLuint index, GLsizei count, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS3SVNVPROC) (GLuint index, GLsizei count, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS4DVNVPROC) (GLuint index, GLsizei count, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS4FVNVPROC) (GLuint index, GLsizei count, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS4SVNVPROC) (GLuint index, GLsizei count, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBS4UBVNVPROC) (GLuint index, GLsizei count, const GLubyte *v);
*/

// Vertex attribute mapping.
enum
{
	CRC_GL_VPREG_VERTEX			= 0,	// glVertex
	CRC_GL_VPREG_MATRIXWEIGHT	= 1,	//
	CRC_GL_VPREG_NORMAL			= 2,	// glNormal
	CRC_GL_VPREG_COLOR			= 3,	// glColor
	CRC_GL_VPREG_SPECULAR		= 4,	// glSecondaryColor
	CRC_GL_VPREG_FOG			= 5,	// glFogCoord
	CRC_GL_VPREG_MATRIXINDEX	= 6,
	__CRC_GL_VPREG_UNUSED0__	= 7,
	CRC_GL_VPREG_TVERTEX0		= 8,	// glMultiTexCoord
	CRC_GL_VPREG_TVERTEX1		= 9,	// glMultiTexCoord
	CRC_GL_VPREG_TVERTEX2		= 10,	// glMultiTexCoord
	CRC_GL_VPREG_TVERTEX3		= 11,	// glMultiTexCoord
	CRC_GL_VPREG_TVERTEX4		= 12,	// glMultiTexCoord
	CRC_GL_VPREG_TVERTEX5		= 13,	// glMultiTexCoord
	CRC_GL_VPREG_TVERTEX6		= 14,	// glMultiTexCoord
	CRC_GL_VPREG_TVERTEX7		= 15,	// glMultiTexCoord
};

// -------------------------------------------------------------------
//  NV_OCCLUSION_QUERY
// -------------------------------------------------------------------
typedef void(APIENTRY *PFNGENOCCLUSIONQUERIESNV)(GLsizei n, GLuint *ids);
typedef void(APIENTRY *PFNDELETEOCCLUSIONQUERIESNV)(GLsizei n, const GLuint *ids);
typedef BOOL(APIENTRY *PFNISOCCLUSIONQUERYNV)(GLuint id);
typedef void(APIENTRY *PFNBEGINOCCLUSIONQUERYNV)(GLuint id);
typedef void(APIENTRY *PFNENDOCCLUSIONQUERYNV)(void);
typedef void(APIENTRY *PFNGETOCCLUSIONQUERYIVNV)(GLuint id, GLenum, GLint *params);
typedef void(APIENTRY *PFNGETOCCLUSIONQUERYUIVNV)(GLuint id, GLenum, GLuint *params);

enum
{
	// Accepted by the <cap> parameter of Enable, Disable, and IsEnabled,
	// and by the <pname> parameter of GetBooleanv, GetIntegerv, GetFloatv,
	// and GetDoublev:
	GL_OCCLUSION_TEST_HP =					0x8165,

	// Accepted by the <pname> parameter of GetBooleanv, GetIntegerv,
	// GetFloatv, and GetDoublev:
	GL_OCCLUSION_TEST_RESULT_HP =			0x8166,
	GL_PIXEL_COUNTER_BITS_NV =				0x8864,
	GL_CURRENT_OCCLUSION_QUERY_ID_NV =		0x8865,

	// Accepted by the <pname> parameter of GetOcclusionQueryivNV and
	// GetOcclusionQueryuivNV:
	GL_PIXEL_COUNT_NV =						0x8866,
	GL_PIXEL_COUNT_AVAILABLE_NV =			0x8867,
};

// -------------------------------------------------------------------
//  EXT_TEXTURE_FILTER_ANISOTROPIC
// -------------------------------------------------------------------
enum
{
	GL_TEXTURE_MAX_ANISOTROPY_EXT =		0x84fe,
	GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT =	0x84ff,
};

// -------------------------------------------------------------------
//  EXT_TEXTURE_LOD_BIAS
// -------------------------------------------------------------------
enum
{
	// Accepted by the <target> parameters of GetTexEnvfv, GetTexEnviv,
	// TexEnvi, TexEnvf, Texenviv, and TexEnvfv:
	GL_TEXTURE_FILTER_CONTROL_EXT =			0x8500,

	// When the <target> parameter of GetTexEnvfv, GetTexEnviv, TexEnvi,
	// TexEnvf, TexEnviv, and TexEnvfv is TEXTURE_FILTER_CONTROL_EXT, then
	// the value of <pname> may be:
	GL_TEXTURE_LOD_BIAS_EXT =				0x8501,

	// Accepted by the <pname> parameters of GetBooleanv, GetIntegerv,
	// GetFloatv, and GetDoublev:
	GL_MAX_TEXTURE_LOD_BIAS_EXT =			0x84FD,
};

// -------------------------------------------------------------------
// ARB_VERTEX_PROGRAM
// -------------------------------------------------------------------
typedef void(APIENTRY *PFNVERTEXATTRIB1SARB)(GLuint index, short x);
typedef void(APIENTRY *PFNVERTEXATTRIB1FARB)(GLuint index, float x);
typedef void(APIENTRY *PFNVERTEXATTRIB1DARB)(GLuint index, double x);
typedef void(APIENTRY *PFNVERTEXATTRIB2SARB)(GLuint index, short x, short y);
typedef void(APIENTRY *PFNVERTEXATTRIB2FARB)(GLuint index, float x, float y);
typedef void(APIENTRY *PFNVERTEXATTRIB2DARB)(GLuint index, double x, double y);
typedef void(APIENTRY *PFNVERTEXATTRIB3SARB)(GLuint index, short x, short y, short z);
typedef void(APIENTRY *PFNVERTEXATTRIB3FARB)(GLuint index, float x, float y, float z);
typedef void(APIENTRY *PFNVERTEXATTRIB3DARB)(GLuint index, double x, double y, double z);
typedef void(APIENTRY *PFNVERTEXATTRIB4SARB)(GLuint index, short x, short y, short z, short w);
typedef void(APIENTRY *PFNVERTEXATTRIB4FARB)(GLuint index, float x, float y, float z, float w);
typedef void(APIENTRY *PFNVERTEXATTRIB4DARB)(GLuint index, double x, double y, double z, double w);
typedef void(APIENTRY *PFNVERTEXATTRIB4NUBARB)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
typedef void(APIENTRY *PFNVERTEXATTRIB1SVARB)(GLuint index, const short *v);
typedef void(APIENTRY *PFNVERTEXATTRIB1FVARB)(GLuint index, const float *v);
typedef void(APIENTRY *PFNVERTEXATTRIB1DVARB)(GLuint index, const double *v);
typedef void(APIENTRY *PFNVERTEXATTRIB2SVARB)(GLuint index, const short *v);
typedef void(APIENTRY *PFNVERTEXATTRIB2FVARB)(GLuint index, const float *v);
typedef void(APIENTRY *PFNVERTEXATTRIB2DVARB)(GLuint index, const double *v);
typedef void(APIENTRY *PFNVERTEXATTRIB3SVARB)(GLuint index, const short *v);
typedef void(APIENTRY *PFNVERTEXATTRIB3FVARB)(GLuint index, const float *v);
typedef void(APIENTRY *PFNVERTEXATTRIB3DVARB)(GLuint index, const double *v);
typedef void(APIENTRY *PFNVERTEXATTRIB4BVARB)(GLuint index, const byte *v);
typedef void(APIENTRY *PFNVERTEXATTRIB4SVARB)(GLuint index, const short *v);
typedef void(APIENTRY *PFNVERTEXATTRIB4IVARB)(GLuint index, const int *v);
typedef void(APIENTRY *PFNVERTEXATTRIB4UBVARB)(GLuint index, const GLubyte *v);
typedef void(APIENTRY *PFNVERTEXATTRIB4USVARB)(GLuint index, const GLushort *v);
typedef void(APIENTRY *PFNVERTEXATTRIB4UIVARB)(GLuint index, const GLuint *v);
typedef void(APIENTRY *PFNVERTEXATTRIB4FVARB)(GLuint index, const float *v);
typedef void(APIENTRY *PFNVERTEXATTRIB4DVARB)(GLuint index, const double *v);
typedef void(APIENTRY *PFNVERTEXATTRIB4NBVARB)(GLuint index, const byte *v);
typedef void(APIENTRY *PFNVERTEXATTRIB4NSVARB)(GLuint index, const short *v);
typedef void(APIENTRY *PFNVERTEXATTRIB4NIVARB)(GLuint index, const int *v);
typedef void(APIENTRY *PFNVERTEXATTRIB4NUBVARB)(GLuint index, const GLubyte *v);
typedef void(APIENTRY *PFNVERTEXATTRIB4NUSVARB)(GLuint index, const GLushort *v);
typedef void(APIENTRY *PFNVERTEXATTRIB4NUIVARB)(GLuint index, const GLuint *v);

typedef void(APIENTRY *PFNVERTEXATTRIBPOINTERARB)(GLuint index, int size, GLenum type, boolean normalized, GLsizei stride, const void *pointer);

typedef void(APIENTRY *PFNENABLEVERTEXATTRIBARRAYARB)(GLuint index);
typedef void(APIENTRY *PFNDISABLEVERTEXATTRIBARRAYARB)(GLuint index);

typedef void(APIENTRY *PFNPROGRAMSTRINGARB)(GLenum target, GLenum format, GLsizei len, const void *string); 

typedef void(APIENTRY *PFNBINDPROGRAMARB)(GLenum target, GLuint program);

typedef void(APIENTRY *PFNDELETEPROGRAMSARB)(GLsizei n, const GLuint *programs);

typedef void(APIENTRY *PFNGENPROGRAMSARB)(GLsizei n, GLuint *programs);

typedef void(APIENTRY *PFNPROGRAMENVPARAMETER4DARB)(GLenum target, GLuint index, double x, double y, double z, double w);
typedef void(APIENTRY *PFNPROGRAMENVPARAMETER4DVARB)(GLenum target, GLuint index, const double *params); 
typedef void(APIENTRY *PFNPROGRAMENVPARAMETER4FARB)(GLenum target, GLuint index, float x, float y, float z, float w);
typedef void(APIENTRY *PFNPROGRAMENVPARAMETER4FVARB)(GLenum target, GLuint index, const float *params);

typedef void(APIENTRY *PFNPROGRAMLOCALPARAMETER4DARB)(GLenum target, GLuint index, double x, double y, double z, double w);
typedef void(APIENTRY *PFNPROGRAMLOCALPARAMETER4DVARB)(GLenum target, GLuint index, const double *params);
typedef void(APIENTRY *PFNPROGRAMLOCALPARAMETER4FARB)(GLenum target, GLuint index, float x, float y, float z, float w);
typedef void(APIENTRY *PFNPROGRAMLOCALPARAMETER4FVARB)(GLenum target, GLuint index, const float *params);

typedef void(APIENTRY *PFNGETPROGRAMENVPARAMETERDVARB)(GLenum target, GLuint index, double *params);
typedef void(APIENTRY *PFNGETPROGRAMENVPARAMETERFVARB)(GLenum target, GLuint index, float *params);

typedef void(APIENTRY *PFNGETPROGRAMLOCALPARAMETERDVARB)(GLenum target, GLuint index, double *params);
typedef void(APIENTRY *PFNGETPROGRAMLOCALPARAMETERFVARB)(GLenum target, GLuint index, float *params);

typedef void(APIENTRY *PFNGETPROGRAMIVARB)(GLenum target, GLenum pname, int *params);

typedef void(APIENTRY *PFNGETPROGRAMSTRINGARB)(GLenum target, GLenum pname, void *string);

typedef void(APIENTRY *PFNGETVERTEXATTRIBDVARB)(GLuint index, GLenum pname, double *params);
typedef void(APIENTRY *PFNGETVERTEXATTRIBFVARB)(GLuint index, GLenum pname, float *params);
typedef void(APIENTRY *PFNGETVERTEXATTRIBIVARB)(GLuint index, GLenum pname, int *params);

typedef void(APIENTRY *PFNGETVERTEXATTRIBPOINTERVARB)(GLuint index, GLenum pname, void **pointer);

typedef boolean(APIENTRY *PFNISPROGRAMARB)(GLuint program);

enum
{
	/*
		Accepted by the <cap> parameter of Disable, Enable, and IsEnabled, by the
		<pname> parameter of GetBooleanv, GetIntegerv, GetFloatv, and GetDoublev,
		and by the <target> parameter of ProgramStringARB, BindProgramARB,
		ProgramEnvParameter4[df][v]ARB, ProgramLocalParameter4[df][v]ARB,
		GetProgramEnvParameter[df]vARB, GetProgramLocalParameter[df]vARB,
		GetProgramivARB, and GetProgramStringARB.
	*/
	GL_VERTEX_PROGRAM_ARB					= 0x8620,

	/*  Accepted by the <cap> parameter of Disable, Enable, and IsEnabled, and by
		the <pname> parameter of GetBooleanv, GetIntegerv, GetFloatv, and
		GetDoublev:
	*/
	GL_VERTEX_PROGRAM_POINT_SIZE_ARB		= 0x8642,
	GL_VERTEX_PROGRAM_TWO_SIDE_ARB			= 0x8643,
	GL_COLOR_SUM_ARB						= 0x8458,

	//    Accepted by the <format> parameter of ProgramStringARB:

	GL_PROGRAM_FORMAT_ASCII_ARB				= 0x8875,

	//    Accepted by the <pname> parameter of GetVertexAttrib[dfi]vARB:

	GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB		= 0x8622,
	GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB			= 0x8623,
	GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB		= 0x8624,
	GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB			= 0x8625,
	GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB	= 0x886A,
	GL_CURRENT_VERTEX_ATTRIB_ARB			= 0x8626,

	//    Accepted by the <pname> parameter of GetVertexAttribPointervARB:

	GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB		= 0x8645,

	//    Accepted by the <pname> parameter of GetProgramivARB:

	GL_PROGRAM_LENGTH_ARB					= 0x8627,
	GL_PROGRAM_FORMAT_ARB					= 0x8876,
	GL_PROGRAM_BINDING_ARB					= 0x8677,
	GL_PROGRAM_INSTRUCTIONS_ARB				= 0x88A0,
	GL_MAX_PROGRAM_INSTRUCTIONS_ARB			= 0x88A1,
	GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB		= 0x88A2,
	GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB	= 0x88A3,
	GL_PROGRAM_TEMPORARIES_ARB				= 0x88A4,
	GL_MAX_PROGRAM_TEMPORARIES_ARB			= 0x88A5,
	GL_PROGRAM_NATIVE_TEMPORARIES_ARB		= 0x88A6,
	GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB	= 0x88A7,
	GL_PROGRAM_PARAMETERS_ARB				= 0x88A8,
	GL_MAX_PROGRAM_PARAMETERS_ARB			= 0x88A9,
	GL_PROGRAM_NATIVE_PARAMETERS_ARB		= 0x88AA,
	GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB	= 0x88AB,
	GL_PROGRAM_ATTRIBS_ARB					= 0x88AC,
	GL_MAX_PROGRAM_ATTRIBS_ARB				= 0x88AD,
	GL_PROGRAM_NATIVE_ATTRIBS_ARB			= 0x88AE,
	GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB		= 0x88AF,
	GL_PROGRAM_ADDRESS_REGISTERS_ARB		= 0x88B0,
	GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB	= 0x88B1,
	GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB	= 0x88B2,
	GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB = 0x88B3,
	GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB		= 0x88B4,
	GL_MAX_PROGRAM_ENV_PARAMETERS_ARB		= 0x88B5,
	GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB		= 0x88B6,

	//    Accepted by the <pname> parameter of GetProgramStringARB:

	GL_PROGRAM_STRING_ARB					= 0x8628,

	//    Accepted by the <pname> parameter of GetBooleanv, GetIntegerv,
	//    GetFloatv, and GetDoublev:

	GL_PROGRAM_ERROR_POSITION_ARB			= 0x864B,
	GL_CURRENT_MATRIX_ARB					= 0x8641,
	GL_TRANSPOSE_CURRENT_MATRIX_ARB			= 0x88B7,
	GL_CURRENT_MATRIX_STACK_DEPTH_ARB		= 0x8640,
	GL_MAX_VERTEX_ATTRIBS_ARB				= 0x8869,
	GL_MAX_PROGRAM_MATRICES_ARB				= 0x862F,
	GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB	= 0x862E,

	//    Accepted by the <name> parameter of GetString:

	GL_PROGRAM_ERROR_STRING_ARB				= 0x8874,

	//    Accepted by the <mode> parameter of MatrixMode:

	GL_MATRIX0_ARB							= 0x88C0,
	GL_MATRIX1_ARB							= 0x88C1,
	GL_MATRIX2_ARB							= 0x88C2,
	GL_MATRIX3_ARB							= 0x88C3,
	GL_MATRIX4_ARB							= 0x88C4,
	GL_MATRIX5_ARB							= 0x88C5,
	GL_MATRIX6_ARB							= 0x88C6,
	GL_MATRIX7_ARB							= 0x88C7,
	GL_MATRIX8_ARB							= 0x88C8,
	GL_MATRIX9_ARB							= 0x88C9,
	GL_MATRIX10_ARB							= 0x88CA,
	GL_MATRIX11_ARB							= 0x88CB,
	GL_MATRIX12_ARB							= 0x88CC,
	GL_MATRIX13_ARB							= 0x88CD,
	GL_MATRIX14_ARB							= 0x88CE,
	GL_MATRIX15_ARB							= 0x88CF,
	GL_MATRIX16_ARB							= 0x88D0,
	GL_MATRIX17_ARB							= 0x88D1,
	GL_MATRIX18_ARB							= 0x88D2,
	GL_MATRIX19_ARB							= 0x88D3,
	GL_MATRIX20_ARB							= 0x88D4,
	GL_MATRIX21_ARB							= 0x88D5,
	GL_MATRIX22_ARB							= 0x88D6,
	GL_MATRIX23_ARB							= 0x88D7,
	GL_MATRIX24_ARB							= 0x88D8,
	GL_MATRIX25_ARB							= 0x88D9,
	GL_MATRIX26_ARB							= 0x88DA,
	GL_MATRIX27_ARB							= 0x88DB,
	GL_MATRIX28_ARB							= 0x88DC,
	GL_MATRIX29_ARB							= 0x88DD,
	GL_MATRIX30_ARB							= 0x88DE,
	GL_MATRIX31_ARB							= 0x88DF,
};


// -------------------------------------------------------------------
// ARB_FRAGMENT_PROGRAM
// -------------------------------------------------------------------

// Uses the same functions as ARB_VERTEX_PROGRAM
enum
{
	GL_FRAGMENT_PROGRAM_ARB                    = 0x8804,
	GL_PROGRAM_ALU_INSTRUCTIONS_ARB            = 0x8805,
	GL_PROGRAM_TEX_INSTRUCTIONS_ARB            = 0x8806,
	GL_PROGRAM_TEX_INDIRECTIONS_ARB            = 0x8807,
	GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB     = 0x8808,
	GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB     = 0x8809,
	GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB     = 0x880A,
	GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB        = 0x880B,
	GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB        = 0x880C,
	GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB        = 0x880D,
	GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB = 0x880E,
	GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB = 0x880F,
	GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB = 0x8810,
	GL_MAX_TEXTURE_COORDS_ARB                  = 0x8871,
	GL_MAX_TEXTURE_IMAGE_UNITS_ARB             = 0x8872,
};

// -------------------------------------------------------------------
//  ARB_TEXTURE_ENV_COMBINE & ARB_TEXTURE_ENV_DOT3
// -------------------------------------------------------------------
enum
{
	// Accepted by the <params> parameter of TexEnvf, TexEnvi, TexEnvfv,
    // and TexEnviv when the <pname> parameter value is TEXTURE_ENV_MODE

	GL_COMBINE_ARB                                  = 0x8570,

	// Accepted by the <pname> parameter of TexEnvf, TexEnvi, TexEnvfv,
	// and TexEnviv when the <target> parameter value is TEXTURE_ENV

	GL_COMBINE_RGB_ARB                              = 0x8571,
	GL_COMBINE_ALPHA_ARB                            = 0x8572,
	GL_SOURCE0_RGB_ARB                              = 0x8580,
	GL_SOURCE1_RGB_ARB                              = 0x8581,
	GL_SOURCE2_RGB_ARB                              = 0x8582,
	GL_SOURCE0_ALPHA_ARB                            = 0x8588,
	GL_SOURCE1_ALPHA_ARB                            = 0x8589,
	GL_SOURCE2_ALPHA_ARB                            = 0x858A,
	GL_OPERAND0_RGB_ARB                             = 0x8590,
	GL_OPERAND1_RGB_ARB                             = 0x8591,
	GL_OPERAND2_RGB_ARB                             = 0x8592,
	GL_OPERAND0_ALPHA_ARB                           = 0x8598,
	GL_OPERAND1_ALPHA_ARB                           = 0x8599,
	GL_OPERAND2_ALPHA_ARB                           = 0x859A,
	GL_RGB_SCALE_ARB                                = 0x8573,
//    GL_ALPHA_SCALE

    // Accepted by the <params> parameter of TexEnvf, TexEnvi, TexEnvfv,
    // and TexEnviv when the <pname> parameter value is COMBINE_RGB_ARB
    // or COMBINE_ALPHA_ARB

//    REPLACE
//    MODULATE
//    ADD
	GL_ADD_SIGNED_ARB                               = 0x8574,
	GL_INTERPOLATE_ARB                              = 0x8575,
	GL_SUBTRACT_ARB                                 = 0x84E7,

	GL_DOT3_RGB_ARB									= 0x86AE,
	GL_DOT3_RGBA_ARB								= 0x86AF,

    // Accepted by the <params> parameter of TexEnvf, TexEnvi, TexEnvfv,
    // and TexEnviv when the <pname> parameter value is SOURCE0_RGB_ARB,
    // SOURCE1_RGB_ARB, SOURCE2_RGB_ARB, SOURCE0_ALPHA_ARB,
    // SOURCE1_ALPHA_ARB, or SOURCE2_ALPHA_ARB

//    TEXTURE
	GL_CONSTANT_ARB                                 = 0x8576,
	GL_PRIMARY_COLOR_ARB                            = 0x8577,
	GL_PREVIOUS_ARB                                 = 0x8578,

    // Accepted by the <params> parameter of TexEnvf, TexEnvi, TexEnvfv,
    // and TexEnviv when the <pname> parameter value is
    // OPERAND0_RGB_ARB, OPERAND1_RGB_ARB, or OPERAND2_RGB_ARB

/*    SRC_COLOR
    ONE_MINUS_SRC_COLOR
    SRC_ALPHA
    ONE_MINUS_SRC_ALPHA     */

    // Accepted by the <params> parameter of TexEnvf, TexEnvi, TexEnvfv,
    // and TexEnviv when the <pname> parameter value is
    // OPERAND0_ALPHA_ARB, OPERAND1_ALPHA_ARB, or OPERAND2_ALPHA_ARB

/*    SRC_ALPHA
    ONE_MINUS_SRC_ALPHA     */

    // Accepted by the <params> parameter of TexEnvf, TexEnvi, TexEnvfv,
    // and TexEnviv when the <pname> parameter value is RGB_SCALE_ARB or
    // ALPHA_SCALE

/*    1.0
    2.0
    4.0*/
};



// -------------------------------------------------------------------
// ExtensionsStringEXT, ExtensionsStringARB
// -------------------------------------------------------------------
typedef const char *(APIENTRY* PFNGETEXTENSIONSSTRING)(void);

// -------------------------------------------------------------------
//  ATI_stencil_separate
// -------------------------------------------------------------------

typedef GLvoid (APIENTRY *PFNGLSTENCILOPSEPARATEATIPROC)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
typedef GLvoid (APIENTRY *PFNGLSTENCILFUNCSEPARATEATIPROC)(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);

enum
{
//	GL_KEEP									= 0x1E00,
//	GL_ZERO									= 0x0000,
//	GL_REPLACE								= 0x1E01,
//	GL_INCR									= 0x1E02,
//	GL_DECR									= 0x1E03,
//	GL_INVERT								= 0x150A,

//	GL_FRONT								= 0x0404,
//	GL_BACK									= 0x0405,
//	GL_FRONT_AND_BACK						= 0x0408,

	STENCIL_BACK_FUNC_ATI					= 0x8800,
	STENCIL_BACK_FAIL_ATI					= 0x8801,
	STENCIL_BACK_PASS_DEPTH_FAIL_ATI		= 0x8802,
	STENCIL_BACK_PASS_DEPTH_PASS_ATI		= 0x8803
};

// -------------------------------------------------------------------
//  EXT_stencil_wrap
// -------------------------------------------------------------------

enum
{
	GL_INCR_WRAP_EXT						= 0x8507,
	GL_DECR_WRAP_EXT						= 0x8508
};

// -------------------------------------------------------------------
//  EXT_stencil_two_side
// -------------------------------------------------------------------

typedef GLvoid (APIENTRY *PFNGLACTIVESTENCILFACEEXT)(GLenum face);
enum
{
	GL_STENCIL_TWO_SIDE_EXT					= 0x8910,
	GL_ACTIVE_STENCIL_FACE_EXT				= 0x8911
};

// -------------------------------------------------------------------
//  ATI_fragment_shader
// -------------------------------------------------------------------

typedef GLvoid (APIENTRY *PFNGLGENFRAGMENTSHADERSATI)(GLuint range);
typedef GLvoid (APIENTRY *PFNGLBINDFRAGMENTSHADERATI)(GLuint id);
typedef GLvoid (APIENTRY *PFNGLDELETEFRAGMENTSHADERATI)(GLuint id);
typedef GLvoid (APIENTRY *PFNGLBEGINFRAGMENTSHADERATI)(GLvoid);
typedef GLvoid (APIENTRY *PFNGLENDFRAGMENTSHADERATI)(GLvoid);
typedef GLvoid (APIENTRY *PFNGLPASSTEXCOORDATI)(GLuint dst, GLuint texcoord, GLint swizzle);
typedef GLvoid (APIENTRY *PFNGLSAMPLEMAPATI)(GLuint dst, GLuint interp, GLint swizzle);
typedef GLvoid (APIENTRY *PFNGLCOLORFRAGMENTOP1ATI)(GLint op, GLuint dst, GLint dstMsk, GLuint dstMod,
													GLuint arg1, GLuint arg1Rep, GLuint arg1Mod);
typedef GLvoid (APIENTRY *PFNGLCOLORFRAGMENTOP2ATI)(GLint op, GLuint dst, GLint dstMsk, GLuint dstMod,
													GLuint arg1, GLuint arg1Rep, GLuint arg1Mod,
													GLuint arg2, GLuint arg2Rep, GLuint arg2Mod);
typedef GLvoid (APIENTRY *PFNGLCOLORFRAGMENTOP3ATI)(GLint op, GLuint dst, GLint dstMsk, GLuint dstMod,
													GLuint arg1, GLuint arg1Rep, GLuint arg1Mod,
													GLuint arg2, GLuint arg2Rep, GLuint arg2Mod,
													GLuint arg3, GLuint arg3Rep, GLuint arg3Mod);
typedef GLvoid (APIENTRY *PFNGLALPHAFRAGMENTOP1ATI)(GLint op, GLuint dst, GLuint dstMod,
													GLuint arg1, GLuint arg1Rep, GLuint arg1Mod);
typedef GLvoid (APIENTRY *PFNGLALPHAFRAGMENTOP2ATI)(GLint op, GLuint dst, GLuint dstMod,
													GLuint arg1, GLuint arg1Rep, GLuint arg1Mod,
													GLuint arg2, GLuint arg2Rep, GLuint arg2Mod);
typedef GLvoid (APIENTRY *PFNGLALPHAFRAGMENTOP3ATI)(GLint op, GLuint dst, GLuint dstMod,
													GLuint arg1, GLuint arg1Rep, GLuint arg1Mod,
													GLuint arg2, GLuint arg2Rep, GLuint arg2Mod,
													GLuint arg3, GLuint arg3Rep, GLuint arg3Mod);
typedef GLvoid (APIENTRY *PFNGLSETFRAGMENTSHADERCONSTANTATI)(GLuint dst, GLfloat* interp);

enum
{
	GL_FRAGMENT_SHADER_ATI					= 0x8920,

	GL_REG_0_ATI							= 0x8921,
	GL_REG_1_ATI							= 0x8922,
	GL_REG_2_ATI							= 0x8923,
	GL_REG_3_ATI							= 0x8924,
	GL_REG_4_ATI							= 0x8925,
	GL_REG_5_ATI							= 0x8926,
	GL_REG_6_ATI							= 0x8927,
	GL_REG_7_ATI							= 0x8928,
	GL_REG_8_ATI							= 0x8929,
	GL_REG_9_ATI							= 0x892A,
	GL_REG_10_ATI							= 0x892B,
	GL_REG_11_ATI							= 0x892C,
	GL_REG_12_ATI							= 0x892D,
	GL_REG_13_ATI							= 0x892E,
	GL_REG_14_ATI							= 0x892F,
	GL_REG_15_ATI							= 0x8930,
	GL_REG_16_ATI							= 0x8931,
	GL_REG_17_ATI							= 0x8932,
	GL_REG_18_ATI							= 0x8933,
	GL_REG_19_ATI							= 0x8934,
	GL_REG_20_ATI							= 0x8935,
	GL_REG_21_ATI							= 0x8936,
	GL_REG_22_ATI							= 0x8937,
	GL_REG_23_ATI							= 0x8938,
	GL_REG_24_ATI							= 0x8939,
	GL_REG_25_ATI							= 0x893A,
	GL_REG_26_ATI							= 0x893B,
	GL_REG_27_ATI							= 0x893C,
	GL_REG_28_ATI							= 0x893D,
	GL_REG_29_ATI							= 0x893E,
	GL_REG_30_ATI							= 0x893F,
	GL_REG_31_ATI							= 0x8940,
 
	GL_CON_0_ATI							= 0x8941,
	GL_CON_1_ATI							= 0x8942,
	GL_CON_2_ATI							= 0x8943,
	GL_CON_3_ATI							= 0x8944,
	GL_CON_4_ATI							= 0x8945,
	GL_CON_5_ATI							= 0x8946,
	GL_CON_6_ATI							= 0x8947,
	GL_CON_7_ATI							= 0x8948,
	GL_CON_8_ATI							= 0x8949,
	GL_CON_9_ATI							= 0x894A,
	GL_CON_10_ATI							= 0x894B,
	GL_CON_11_ATI							= 0x894C,
	GL_CON_12_ATI							= 0x894D,
	GL_CON_13_ATI							= 0x894E,
	GL_CON_14_ATI							= 0x894F,
	GL_CON_15_ATI							= 0x8950,
	GL_CON_16_ATI							= 0x8951,
	GL_CON_17_ATI							= 0x8952,
	GL_CON_18_ATI							= 0x8953,
	GL_CON_19_ATI							= 0x8954,
	GL_CON_20_ATI							= 0x8955,
	GL_CON_21_ATI							= 0x8956,
	GL_CON_22_ATI							= 0x8957,
	GL_CON_23_ATI							= 0x8958,
	GL_CON_24_ATI							= 0x8959,
	GL_CON_25_ATI							= 0x895A,
	GL_CON_26_ATI							= 0x895B,
	GL_CON_27_ATI							= 0x895C,
	GL_CON_28_ATI							= 0x895D,
	GL_CON_29_ATI							= 0x895E,
	GL_CON_30_ATI							= 0x895F,
	GL_CON_31_ATI							= 0x8960,

	GL_MOV_ATI								= 0x8961,
	GL_ADD_ATI								= 0x8963,
	GL_MUL_ATI								= 0x8964,
	GL_SUB_ATI								= 0x8965,
	GL_DOT3_ATI								= 0x8966,
	GL_DOT4_ATI								= 0x8967,
	GL_MAD_ATI								= 0x8968,
	GL_LERP_ATI								= 0x8969,
	GL_CND_ATI								= 0x896A,
	GL_CND0_ATI								= 0x896B,
	GL_DOT2_ADD_ATI							= 0x896C,
	GL_SECONDARY_INTERPOLATOR_ATI			= 0x896D,

	GL_NUM_FRAGMENT_REGISTERS_ATI			= 0x896E,
	GL_NUM_FRAGMENT_CONSTANTS_ATI			= 0x896F,
	GL_NUM_PASSES_ATI						= 0x8970,
	GL_NUM_INSTRUCTIONS_PER_PASS_ATI		= 0x8971,
	GL_NUM_INSTRUCTIONS_TOTAL_ATI			= 0x8972,
	GL_NUM_INPUT_INTERPOLATOR_COMPONENTS_ATI= 0x8973,
	GL_NUM_LOOPBACK_COMPONENTS_ATI			= 0x8974,
	GL_COLOR_ALPHA_PAIRING_ATI				= 0x8975,

	GL_SWIZZLE_STR_ATI						= 0x8976,
	GL_SWIZZLE_STQ_ATI						= 0x8977,
	GL_SWIZZLE_STR_DR_ATI					= 0x8978,
	GL_SWIZZLE_STQ_DQ_ATI					= 0x8979,

	GL_RED_BIT_ATI							= 0x00000001,
	GL_GREEN_BIT_ATI						= 0x00000002,
	GL_BLUE_BIT_ATI							= 0x00000004,          
 
	GL_2X_BIT_ATI							= 0x00000001,
	GL_4X_BIT_ATI							= 0x00000002,
	GL_8X_BIT_ATI							= 0x00000004,
	GL_HALF_BIT_ATI							= 0x00000008,
	GL_QUARTER_BIT_ATI						= 0x00000010,
	GL_EIGHTH_BIT_ATI						= 0x00000020,
	GL_SATURATE_BIT_ATI						= 0x00000040,

	GL_COMP_BIT_ATI							= 0x00000002,
	GL_NEGATE_BIT_ATI						= 0x00000004,
	GL_BIAS_BIT_ATI							= 0x00000008,    

};


// -------------------------------------------------------------------
//  ARB_Vertex_Buffer_Object
// -------------------------------------------------------------------

enum
{
    /* Accepted by the <target> parameters of BindBufferARB, BufferDataARB,
    BufferSubDataARB, MapBufferARB, UnmapBufferARB,
    GetBufferSubDataARB, GetBufferParameterivARB, and
    GetBufferPointervARB: */

	GL_ARRAY_BUFFER_ARB							= 0x8892,
	GL_ELEMENT_ARRAY_BUFFER_ARB					= 0x8893,

    //Accepted by the <pname> parameter of GetBooleanv, GetIntegerv, GetFloatv, and GetDoublev:

	GL_ARRAY_BUFFER_BINDING_ARB					= 0x8894,
	GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB			= 0x8895,
	GL_VERTEX_ARRAY_BUFFER_BINDING_ARB			= 0x8896,
	GL_NORMAL_ARRAY_BUFFER_BINDING_ARB			= 0x8897,
	GL_COLOR_ARRAY_BUFFER_BINDING_ARB			= 0x8898,
	GL_INDEX_ARRAY_BUFFER_BINDING_ARB			= 0x8899,
	GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB	= 0x889A,
	GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB		= 0x889B,
	GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB	= 0x889C,
	GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB	= 0x889D,
	GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB			= 0x889E,

    // Accepted by the <pname> parameter of GetVertexAttribivARB:

	GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB	= 0x889F,

    // Accepted by the <usage> parameter of BufferDataARB:

	GL_STREAM_DRAW_ARB							= 0x88E0,
	GL_STREAM_READ_ARB							= 0x88E1,
	GL_STREAM_COPY_ARB							= 0x88E2,
	GL_STATIC_DRAW_ARB							= 0x88E4,
	GL_STATIC_READ_ARB							= 0x88E5,
	GL_STATIC_COPY_ARB							= 0x88E6,
	GL_DYNAMIC_DRAW_ARB							= 0x88E8,
	GL_DYNAMIC_READ_ARB							= 0x88E9,
	GL_DYNAMIC_COPY_ARB							= 0x88EA,

    // Accepted by the <access> parameter of MapBufferARB:

	GL_READ_ONLY_ARB							= 0x88B8,
	GL_WRITE_ONLY_ARB							= 0x88B9,
	GL_READ_WRITE_ARB							= 0x88BA,

    // Accepted by the <pname> parameter of GetBufferParameterivARB:

	GL_BUFFER_SIZE_ARB							= 0x8764,
	GL_BUFFER_USAGE_ARB							= 0x8765,
	GL_BUFFER_ACCESS_ARB						= 0x88BB,
	GL_BUFFER_MAPPED_ARB						= 0x88BC,

    // Accepted by the <pname> parameter of GetBufferPointervARB:

	GL_BUFFER_MAP_POINTER_ARB					= 0x88BD,
};


typedef void (APIENTRY *PFNGLBINDBUFFERARB)(GLenum target, GLuint buffer);
typedef void (APIENTRY *PFNGLDELETEBUFFERSARB)(GLsizei n, const GLuint *buffers);
typedef void (APIENTRY *PFNGLGENBUFFERSARB)(GLsizei n, GLuint *buffers);
typedef boolean (APIENTRY *PFNGLISBUFFERARB)(GLuint buffer);

typedef void (APIENTRY *PFNGLBUFFERDATAARB)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
typedef void (APIENTRY *PFNGLBUFFERSUBDATAARB)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
typedef void (APIENTRY *PFNGLGETBUFFERSUBDATAARB)(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid *data);

typedef void* (APIENTRY *PFNGLMAPBUFFERARB)(GLenum target, GLenum access);
typedef boolean (APIENTRY *PFNGLUNMAPBUFFERARB)(GLenum target);

typedef void (APIENTRY *PFNGLGETBUFFERPARAMETERIVARB)(GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLGETBUFFERPOINTERVARB)(GLenum target, GLenum pname, GLvoid **params);

// -------------------------------------------------------------------
//  ARB_multisample
// -------------------------------------------------------------------
enum
{
	GL_MULTISAMPLE_ARB							= 0x809d,
	GL_SAMPLE_ALPHA_TO_COVERAGE_ARB				= 0x809e,
	GL_SAMPLE_ALPHA_TO_ONE_ARB					= 0x809f,
	GL_SAMPLE_COVERAGE_ARB						= 0x80a0,

	GL_MULTISAMPLE_BIT_ARB						= 0x20000000,

	GL_SAMPLE_BUFFERS_ARB						= 0x80a8,
	GL_SAMPLES_ARB								= 0x80a9,
	GL_SAMPLE_COVERAGE_VALUE_ARB				= 0x80aa,
	GL_SAMPLE_COVERAGE_INVERT_ARB				= 0x80ab,
};

typedef void (APIENTRY *PFNGLSAMPLECOVERAGEARB)( GLclampf value, GLboolean invert );
// -------------------------------------------------------------------
//  NV_copy_depth_to_color
// -------------------------------------------------------------------

enum
{
	// Accepted by the <type> parameter of CopyPixels:

	GL_DEPTH_STENCIL_TO_RGBA_NV					= 0x886E,
	GL_DEPTH_STENCIL_TO_BGRA_NV					= 0x886F,
};

// -------------------------------------------------------------------
//  ARB_Window_Pos - 1.4 standard feature
// -------------------------------------------------------------------


typedef void (APIENTRY *PFNGLWINDOWPOS2F)(GLfloat, GLfloat);
typedef void (APIENTRY *PFNGLWINDOWPOS2I)(GLint, GLint);
typedef void (APIENTRY *PFNGLWINDOWPOS3F)(GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *PFNGLWINDOWPOS3I)(GLint, GLint, GLint);

/*	void WindowPos2dARB(double x, double y)
    void WindowPos2fARB(float x, float y)
    void WindowPos2iARB(int x, int y)
    void WindowPos2sARB(short x, short y)

    void WindowPos2dvARB(const double *p)
    void WindowPos2fvARB(const float *p)
    void WindowPos2ivARB(const int *p)
    void WindowPos2svARB(const short *p)

    void WindowPos3dARB(double x, double y, double z)
    void WindowPos3fARB(float x, float y, float z)
    void WindowPos3iARB(int x, int y, int z)
    void WindowPos3sARB(short x, short y, short z)

    void WindowPos3dvARB(const double *p)
    void WindowPos3fvARB(const float *p)
    void WindowPos3ivARB(const int *p)
    void WindowPos3svARB(const short *p)
*/

// -------------------------------------------------------------------
//  ATI_TEXTURE_COMPRESSION_3DC
// -------------------------------------------------------------------
enum
{
	GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI	= 0x8837
};

// -------------------------------------------------------------------
//  EXT_FRAMEBUFFER_OBJECT
// -------------------------------------------------------------------
enum
{
    // Accepted by the <target> parameter of BindFramebufferEXT,
    // CheckFramebufferStatusEXT, FramebufferTexture{1D|2D|3D}EXT, and
    // FramebufferRenderbufferEXT:

	GL_FRAMEBUFFER_EXT						= 0x8D40,

    // Accepted by the <target> parameter of BindRenderbufferEXT,
    // RenderbufferStorageEXT, and GetRenderbufferParameterivEXT, and
    // returned by GetFramebufferAttachmentParameterivEXT:

	GL_RENDERBUFFER_EXT						= 0x8D41,

    // Accepted by the <internalformat> parameter of
    // RenderbufferStorageEXT:

	GL_STENCIL_INDEX_EXT					= 0x8D45,
	GL_STENCIL_INDEX1_EXT					= 0x8D46,
	GL_STENCIL_INDEX4_EXT					= 0x8D47,
	GL_STENCIL_INDEX8_EXT					= 0x8D48,
	GL_STENCIL_INDEX16_EXT					= 0x8D49,

	// Accepted by the <pname> parameter of GetRenderbufferParameterivEXT:

	GL_RENDERBUFFER_WIDTH_EXT				= 0x8D42,
	GL_RENDERBUFFER_HEIGHT_EXT				= 0x8D43,
	GL_RENDERBUFFER_INTERNAL_FORMAT_EXT		= 0x8D44,

	// Accepted by the <pname> parameter of
	// GetFramebufferAttachmentParameterivEXT:

	GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT				= 0x8CD0,
	GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT				= 0x8CD1,
	GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT				= 0x8CD2,
	GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT		= 0x8CD3,
	GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT		= 0x8CD4,

	// Accepted by the <attachment> parameter of
	// FramebufferTexture{1D|2D|3D}EXT, FramebufferRenderbufferEXT, and
	// GetFramebufferAttachmentParameterivEXT

	GL_COLOR_ATTACHMENT0_EXT				= 0x8CE0,
	GL_COLOR_ATTACHMENT1_EXT				= 0x8CE1,
	GL_COLOR_ATTACHMENT2_EXT				= 0x8CE2,
	GL_COLOR_ATTACHMENT3_EXT				= 0x8CE3,
	GL_COLOR_ATTACHMENT4_EXT				= 0x8CE4,
	GL_COLOR_ATTACHMENT5_EXT				= 0x8CE5,
	GL_COLOR_ATTACHMENT6_EXT				= 0x8CE6,
	GL_COLOR_ATTACHMENT7_EXT				= 0x8CE7,
	GL_COLOR_ATTACHMENT8_EXT				= 0x8CE8,
	GL_COLOR_ATTACHMENT9_EXT				= 0x8CE9,
	GL_COLOR_ATTACHMENT10_EXT				= 0x8CEA,
	GL_COLOR_ATTACHMENT11_EXT				= 0x8CEB,
	GL_COLOR_ATTACHMENT12_EXT				= 0x8CEC,
	GL_COLOR_ATTACHMENT13_EXT				= 0x8CED,
	GL_COLOR_ATTACHMENT14_EXT				= 0x8CEE,
	GL_COLOR_ATTACHMENT15_EXT				= 0x8CEF,
	GL_DEPTH_ATTACHMENT_EXT					= 0x8D00,
	GL_STENCIL_ATTACHMENT_EXT				= 0x8D20,

	// Returned by CheckFramebufferStatusEXT():

	GL_FRAMEBUFFER_COMPLETE_EXT								= 0x8CD5,
	GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT				= 0x8CD6,
	GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT		= 0x8CD7,
	GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT		= 0x8CD8,
	GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT				= 0x8CD9,
	GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT					= 0x8CDA,
	GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT				= 0x8CDB,
	GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT				= 0x8CDC,
	GL_FRAMEBUFFER_UNSUPPORTED_EXT							= 0x8CDD,
	GL_FRAMEBUFFER_STATUS_ERROR_EXT							= 0x8CDE,

	// Accepted by GetIntegerv():

	GL_FRAMEBUFFER_BINDING_EXT				= 0x8CA6,
	GL_RENDERBUFFER_BINDING_EXT				= 0x8CA7,
	GL_MAX_COLOR_ATTACHMENTS_EXT			= 0x8CDF,
	GL_MAX_RENDERBUFFER_SIZE_EXT			= 0x84E8,

	// Returned by GetError():

	GL_INVALID_FRAMEBUFFER_OPERATION_EXT	= 0x0506,

};

typedef boolean (APIENTRY *PFNGLISRENDERBUFFEREXT)(GLuint renderbuffer);
typedef void (APIENTRY *PFNGLBINDRENDERBUFFEREXT)(GLenum target, GLuint renderbuffer);
typedef void (APIENTRY *PFNGLDELETERENDERBUFFERSEXT)(GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRY *PFNGLGENRENDERBUFFERSEXT)(GLsizei n, GLuint *renderbuffers);

typedef void (APIENTRY *PFNGLRENDERBUFFERSTORAGEEXT)(GLenum target, GLenum internalformat,
                            GLsizei width, GLsizei height);

typedef void (APIENTRY *PFNGLGETRENDERBUFFERPARAMETERIVEXT)(GLenum target, GLenum pname, GLint* params);

typedef boolean (APIENTRY *PFNGLISFRAMEBUFFEREXT)(GLuint framebuffer);
typedef void (APIENTRY *PFNGLBINDFRAMEBUFFEREXT)(GLenum target, GLuint framebuffer);
typedef void (APIENTRY *PFNGLDELETEFRAMEBUFFERSEXT)(GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *PFNGLGENFRAMEBUFFERSEXT)(GLsizei n, GLuint *framebuffers);

typedef GLenum (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUSEXT)(GLenum target);

typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE1DEXT)(GLenum target, GLenum attachment,
                                GLenum textarget, GLuint texture,
                                GLint level);
typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DEXT)(GLenum target, GLenum attachment,
                                GLenum textarget, GLuint texture,
                                GLint level);
typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE3DEXT)(GLenum target, GLenum attachment,
                                GLenum textarget, GLuint texture,
                                GLint level, GLint zoffset);

typedef void (APIENTRY *PFNGLFRAMEBUFFERRENDERBUFFEREXT)(GLenum target, GLenum attachment,
                                GLenum renderbuffertarget, GLuint renderbuffer);

typedef void (APIENTRY *PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXT)(GLenum target, GLenum attachment,
                                            GLenum pname, GLint *params);

typedef void (APIENTRY *PFNGLGENERATEMIPMAPEXT)(GLenum target);


#define GL_DEPTH_STENCIL_NV 0x84F9
#define GL_UNSIGNED_INT_24_8_NV 0x84FA




enum
{
	// Accepted by the <pname> parameters of GetBooleanv, GetIntegerv,
    // GetFloatv, and GetDoublev:

	GL_RGBA_FLOAT_MODE_ARB                     = 0x8820,

	// Accepted by the <target> parameter of ClampColorARB and the <pname>
	// parameter of GetBooleanv, GetIntegerv, GetFloatv, and GetDoublev.

	GL_CLAMP_VERTEX_COLOR_ARB                  = 0x891A,
	GL_CLAMP_FRAGMENT_COLOR_ARB                = 0x891B,
	GL_CLAMP_READ_COLOR_ARB                    = 0x891C,
};

#ifdef PLATFORM_WIN_PC

// -------------------------------------------------------------------
//  WGL_ARB_pixel_format
// -------------------------------------------------------------------
typedef BOOL(APIENTRY *PFNWGLGETPIXELFORMATATTRIBIVARB)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, int *piValues);
typedef BOOL(APIENTRY *PFNWGLGETPIXELFORMATATTRIBFVARB)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, FLOAT *pfValues);
typedef BOOL(APIENTRY *PFNWGLCHOOSEPIXELFORMATARB)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);

enum
{
	WGL_NUMBER_PIXEL_FORMATS_ARB			= 0x2000,

	WGL_DRAW_TO_WINDOW_ARB					= 0x2001,
	WGL_DRAW_TO_BITMAP_ARB					= 0x2002,
	WGL_ACCELERATION_ARB					= 0x2003,
	WGL_NEED_PALETTE_ARB					= 0x2004,
	WGL_NEED_SYSTEM_PALETTE_ARB				= 0x2005,
	WGL_SWAP_LAYER_BUFFERS_ARB				= 0x2006,
	WGL_SWAP_METHOD_ARB						= 0x2007,
	WGL_NUMBER_OVERLAYS_ARB					= 0x2008,
	WGL_NUMBER_UNDERLAYS_ARB				= 0x2009,
	WGL_TRANSPARENT_ARB						= 0x200A,
	WGL_TRANSPARENT_VALUE_ARB				= 0x200B,
	WGL_SHARE_DEPTH_ARB						= 0x200C,
	WGL_SHARE_STENCIL_ARB					= 0x200D,
	WGL_SHARE_ACCUM_ARB						= 0x200E,
	WGL_SUPPORT_GDI_ARB						= 0x200F,
	WGL_SUPPORT_OPENGL_ARB					= 0x2010,
	WGL_DOUBLE_BUFFER_ARB					= 0x2011,
	WGL_STEREO_ARB							= 0x2012,
	WGL_PIXEL_TYPE_ARB						= 0x2013,
	WGL_COLOR_BITS_ARB						= 0x2014,
	WGL_RED_BITS_ARB						= 0x2015,
	WGL_RED_SHIFT_ARB						= 0x2016,
	WGL_GREEN_BITS_ARB						= 0x2017,
	WGL_GREEN_SHIFT_ARB						= 0x2018,
	WGL_BLUE_BITS_ARB						= 0x2019,
	WGL_BLUE_SHIFT_ARB						= 0x201A,
	WGL_ALPHA_BITS_ARB						= 0x201B,
	WGL_ALPHA_SHIFT_ARB						= 0x201C,
	WGL_ACCUM_BITS_ARB						= 0x201D,
	WGL_ACCUM_RED_BITS_ARB					= 0x201E,
	WGL_ACCUM_GREEN_BITS_ARB				= 0x201F,
	WGL_ACCUM_BLUE_BITS_ARB					= 0x2020,
	WGL_ACCUM_ALPHA_BITS_ARB				= 0x2021,
	WGL_DEPTH_BITS_ARB						= 0x2022,
	WGL_STENCIL_BITS_ARB					= 0x2023,
	WGL_AUX_BUFFERS_ARB						= 0x2024,

	WGL_NO_ACCELERATION_ARB					= 0x2025,
	WGL_GENERIC_ACCELERATION_ARB			= 0x2026,
	WGL_FULL_ACCELERATION_ARB				= 0x2027,

	WGL_SWAP_EXCHANGE_ARB					= 0x2028,
	WGL_SWAP_COPY_ARB						= 0x2029,
	WGL_SWAP_UNDEFINED_ARB					= 0x202A,
	
	WGL_TYPE_RGBA_ARB						= 0x202B,
	WGL_TYPE_COLORINDEX_ARB					= 0x202C,
};

// -------------------------------------------------------------------
//  WGL_ARB_pbuffer
// -------------------------------------------------------------------
DECLARE_HANDLE(HPBUFFERARB);

enum
{
	WGL_DRAW_TO_PBUFFER_ARB						= 0x202D,
	WGL_MAX_PBUFFER_WIDTH_ARB					= 0x202F,
	WGL_MAX_PBUFFER_PIXELS_ARB					= 0x202E,
	WGL_MAX_PBUFFER_HEIGHT_ARB					= 0x2030,
	WGL_PBUFFER_LARGEST_ARB						= 0x2033,
	WGL_PBUFFER_WIDTH_ARB						= 0x2034,
	WGL_PBUFFER_HEIGHT_ARB						= 0x2035,
	WGL_PBUFFER_LOST_ARB						= 0x2036,
};

typedef HPBUFFERARB (APIENTRY *PFNWGLCREATEPBUFFERARB) (void* hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
typedef HDC (APIENTRY *PFNWGLGETPBUFFERDCARB) (HPBUFFERARB hPbuffer);
typedef int (APIENTRY *PFNWGLRELEASEPBUFFERDCARB) (HPBUFFERARB hPbuffer, HDC hDC);
typedef BOOL (APIENTRY *PFNWGLDESTROYPBUFFERARB) (HPBUFFERARB hPbuffer);
typedef BOOL (APIENTRY *PFNWGLQUERYPBUFFERARB) (HPBUFFERARB hPbuffer, int iAttribute, int *piValue);

// -------------------------------------------------------------------
//  WGL_ARB_render_texture
// -------------------------------------------------------------------
enum
{
	// Accepted by the <piAttributes> parameter of wglGetPixelFormatAttribivARB, 
	// wglGetPixelFormatAttribfvARB, and the <piAttribIList> and <pfAttribIList>
	// parameters of wglChoosePixelFormatARB: 
	WGL_BIND_TO_TEXTURE_RGB_ARB					= 0x2070,
	WGL_BIND_TO_TEXTURE_RGBA_ARB				= 0x2071,

	// Accepted by the <piAttribList> parameter of wglCreatePbufferARB and
	// by the <iAttribute> parameter of wglQueryPbufferARB: 
	WGL_TEXTURE_FORMAT_ARB						= 0x2072,
	WGL_TEXTURE_TARGET_ARB						= 0x2073,
	WGL_MIPMAP_TEXTURE_ARB						= 0x2074,

	// Accepted as a value in the <piAttribList> parameter of 
	// wglCreatePbufferARB and returned in the value parameter of
	// wglQueryPbufferARB when <iAttribute> is WGL_TEXTURE_FORMAT_ARB: 
	WGL_TEXTURE_RGB_ARB							= 0x2075,
	WGL_TEXTURE_RGBA_ARB						= 0x2076,
	WGL_NO_TEXTURE_ARB							= 0x2077,

	// Accepted as a value in the <piAttribList> parameter of 
	// wglCreatePbufferARB and returned in the value parameter of
	// wglQueryPbufferARB when <iAttribute> is WGL_TEXTURE_TARGET_ARB: 
	WGL_TEXTURE_CUBE_MAP_ARB					= 0x2078,
	WGL_TEXTURE_1D_ARB							= 0x2079,
	WGL_TEXTURE_2D_ARB							= 0x207A,
//	WGL_NO_TEXTURE_ARB							= 0x2077,

	// Accepted by the <piAttribList> parameter of wglSetPbufferAttribARB and 
	// by the <iAttribute> parameter of wglQueryPbufferARB: 
	WGL_MIPMAP_LEVEL_ARB						= 0x207B,
	WGL_CUBE_MAP_FACE_ARB						= 0x207C,

	// Accepted as a value in the <piAttribList> parameter of 
	// wglSetPbufferAttribARB and returned in the value parameter of
	// wglQueryPbufferARB when <iAttribute> is WGL_CUBE_MAP_FACE_ARB: 
	WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB			= 0x207D,
	WGL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB			= 0x207E,
	WGL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB			= 0x207F,
	WGL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB			= 0x2080,
	WGL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB			= 0x2081,
	WGL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB			= 0x2082,

	// Accepted by the <iBuffer> parameter of wglBindTexImageARB and 
	// wglReleaseTexImageARB: 
	WGL_FRONT_LEFT_ARB							= 0x2083,
	WGL_FRONT_RIGHT_ARB							= 0x2084,
	WGL_BACK_LEFT_ARB							= 0x2085,
	WGL_BACK_RIGHT_ARB							= 0x2086,
	WGL_AUX0_ARB								= 0x2087,
	WGL_AUX1_ARB								= 0x2088,
	WGL_AUX2_ARB								= 0x2089,
	WGL_AUX3_ARB								= 0x208A,
	WGL_AUX4_ARB								= 0x208B,
	WGL_AUX5_ARB								= 0x208C,
	WGL_AUX6_ARB								= 0x208D,
	WGL_AUX7_ARB								= 0x208E,
	WGL_AUX8_ARB								= 0x208F,
	WGL_AUX9_ARB								= 0x2090,
};

typedef BOOL (WINAPI * PFNWGLBINDTEXIMAGEARB) (HPBUFFERARB hPbuffer, int iBuffer);
typedef BOOL (WINAPI * PFNWGLRELEASETEXIMAGEARB) (HPBUFFERARB hPbuffer, int iBuffer);
typedef BOOL (WINAPI * PFNWGLSETPBUFFERATTRIBARB) (HPBUFFERARB hPbuffer, const int *piAttribList);

#endif	// PLATFORM_WIN_PC

// -------------------------------------------------------------------
//  WGL_ATI_pixel_format_float & ATI_Texture_Float
// -------------------------------------------------------------------
enum
{
	// Accepted by the <pname> parameters of GetBooleanv, GetIntegerv,
	// GetFloatv, and GetDoublev:

	WGL_RGBA_FLOAT_MODE_ATI                     = 0x8820,
	WGL_COLOR_CLEAR_UNCLAMPED_VALUE_ATI         = 0x8835,

	// Accepted as a value in the <piAttribIList> and <pfAttribFList>
	// parameter arrays of wglChoosePixelFormatARB, and returned in the
	// <piValues> parameter array of wglGetPixelFormatAttribivARB, and the
	// <pfValues> parameter array of wglGetPixelFormatAttribfvARB:

	WGL_TYPE_RGBA_FLOAT_ATI						= 0x21A0,


    // Accepted by the <internalFormat> parameter of TexImage1D,
    // TexImage2D, and TexImage3D:

	GL_RGBA_FLOAT32_ATI							= 0x8814,
	GL_RGB_FLOAT32_ATI							= 0x8815,
	GL_ALPHA_FLOAT32_ATI						= 0x8816,
	GL_INTENSITY_FLOAT32_ATI					= 0x8817,
	GL_LUMINANCE_FLOAT32_ATI					= 0x8818,
	GL_LUMINANCE_ALPHA_FLOAT32_ATI				= 0x8819,
	GL_RGBA_FLOAT16_ATI							= 0x881A,
	GL_RGB_FLOAT16_ATI							= 0x881B,
	GL_ALPHA_FLOAT16_ATI						= 0x881C,
	GL_INTENSITY_FLOAT16_ATI					= 0x881D,
	GL_LUMINANCE_FLOAT16_ATI					= 0x881E,
	GL_LUMINANCE_ALPHA_FLOAT16_ATI				= 0x881F,


    GL_TEXTURE_RECTANGLE_ARB					= 0x84F5,
};


#endif	// __INC_MRNDRGL_EXT
