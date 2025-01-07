
enum
{
	GL_RGB5,
	GL_CLIP_PLANE0,
	GL_STATIC_DRAW,
	GL_WRITE_ONLY,
	GL_ELEMENT_ARRAY_BUFFER,
	GL_ARRAY_BUFFER,
	GL_FRAMEBUFFER_OES,
	GL_RENDERBUFFER_OES,
	GL_COLOR_ATTACHMENT0_EXT,
	GL_DEPTH_ATTACHMENT_OES,
	PSGL_REPORT_ALL,
	PSGL_DEVICE_TYPE_AUTO,
	PSGL_TV_STANDARD_NONE,
	PSGL_TV_FORMAT_AUTO,
	PSGL_BUFFERING_MODE_SINGLE,
	PSGL_BUFFERING_MODE_DOUBLE,
	PSGL_BUFFERING_MODE_TRIPLE,
	GL_STENCIL_TEST_TWO_SIDE_EXT,
	GL_FILL,
	GL_ALPHA_TEST,
	GL_FOG_COLOR,
	GL_FOG_START,
	GL_FOG_END,
	GL_FOG_DENSITY,
	GL_BLEND,
	GL_BACK,
	GL_FOG,
	GL_POLYGON_OFFSET_FILL,
	GL_ZERO,
	GL_ONE,
	GL_SRC_COLOR,
	GL_DST_COLOR,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_ONE_MINUS_SRC_COLOR,
	GL_ONE_MINUS_DST_ALPHA,
	GL_ONE_MINUS_DST_COLOR,
	GL_SRC_ALPHA,
	GL_SRC_ALPHA_SATURATE,
	GL_DST_ALPHA,
	GL_NEVER,
	GL_NOTEQUAL,
	GL_ALWAYS,
	GL_EQUAL,
	GL_LESS,
	GL_LEQUAL,
	GL_GREATER,
	GL_GEQUAL,
	GL_KEEP,
	GL_REPLACE,
	GL_INVERT,
	GL_INCR,
	GL_DECR,
	GL_INCR_WRAP,
	GL_DECR_WRAP,
	GL_COLOR_BUFFER_BIT,
	GL_DEPTH_BUFFER_BIT,
	GL_STENCIL_BUFFER_BIT,
	GL_PROJECTION,
	GL_MODELVIEW,
	GL_DEPTH_TEST,
	GL_STENCIL_TEST,
	GL_SCISSOR_TEST,
	GL_NORMALIZE,
	GL_CCW,
	GL_CW,
	GL_FRONT,
	GL_CULL_FACE,
	GL_SMOOTH,
	GL_FLAT,
	GL_DITHER,
	GL_PERSPECTIVE_CORRECTION_HINT,
	GL_NICEST,
	GL_FOG_MODE,
	GL_PRIMITIVE_RESTART_NV,
	GL_STENCIL_INDEX,
	GL_RGB,
	GL_RED,
	GL_GREEN,
	GL_BLUE,
	GL_QUERY_RESULT_AVAILABLE_ARB,
	GL_QUERY_RESULT_ARB,
	GL_SAMPLES_PASSED_ARB,
	GL_NO_ERROR,
	GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
	GL_BGR,
	GL_BGRA,
	GL_RGBA,
	GL_ALPHA,
	GL_LUMINANCE_ALPHA,
	GL_LUMINANCE,
	GL_ABGR,
	GL_RGB8,
	GL_ARGB_SCE,
	GL_RGBA8,
	GL_ALPHA8,
	GL_LUMINANCE8_ALPHA8,
	GL_LUMINANCE8,
	GL_RGB16,
	GL_RGBA16,
	GL_RGBA16F_ARB,
	GL_RGB32F_ARB,
	GL_RGBA32F_ARB,
	GL_RGBA4,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_SHORT,
	GL_FLOAT,
	GL_DEPTH_COMPONENT24,
	GL_DEPTH_COMPONENT,
	GL_TEXTURE_MAG_FILTER,
	GL_TEXTURE_MIN_FILTER,
	GL_TEXTURE_WRAP_S,
	GL_TEXTURE_WRAP_T,
	GL_TEXTURE_WRAP_R,
	GL_NEAREST,
	GL_LINEAR,
	GL_NEAREST_MIPMAP_LINEAR,
	GL_NEAREST_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_LINEAR,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_CLAMP_TO_EDGE,
	GL_REPEAT,
	GL_TEXTURE_CUBE_MAP,
	GL_TEXTURE_MAX_ANISOTROPY_EXT,
	GL_TEXTURE_1D,
	GL_TEXTURE_2D,
	GL_TEXTURE_3D,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	GL_TEXTURE,
	GL_TEXTURE0,
	GL_TEXTURE_FILTER_CONTROL,
	GL_TEXTURE_LOD_BIAS,
	GL_VERTEX_ARRAY,
	GL_COLOR_ARRAY,
	GL_NORMAL_ARRAY,
	GL_TEXTURE_COORD_ARRAY,
	GL_FALSE,
	GL_TRUE,
	GL_TRIANGLES,
	GL_QUADS,
	GL_QUAD_STRIP,
	GL_LINES,
	GL_LINE,
	GL_LINE_STRIP,
	GL_TRIANGLE_FAN,
	GL_TRIANGLE_STRIP,

};


// PSGL emulation
typedef uint32			PSGLcontext;
typedef uint32			PSGLdevice;
typedef uint32			GLenum;
typedef	uint32			GLbitfield;
typedef int				GLint;
typedef int				GLsizeiptr;
typedef unsigned int	GLuint;
typedef int32			GLsizei;
typedef void			GLvoid;
typedef fp4				GLfloat;
typedef fp4				GLclampf;
typedef int				GLboolean;


typedef struct
{
	GLint width;
	GLint height;
	GLint colorBits;
	GLint alphaBits;
	GLint depthBits;
	GLint stencilBits;
	GLenum deviceType;
	GLenum TVStandard;
	GLenum TVFormat;
	GLenum bufferingMode;
	GLboolean antiAliasing;
} PSGLbufferParameters;

typedef struct PSGLinitOptions
{
	GLuint			enable;	// bitfield of options to set
	GLuint 			maxSPUs;
	GLboolean		initializeSPUs;
	GLuint			persistentMemorySize;
	GLuint			transientMemorySize;
	int				errorConsole;
	GLuint			fifoSize;
} PSGLinitOptions;


static M_INLINE void psglInit(PSGLinitOptions* options) {}
static M_INLINE void psglSetReportFunction(void* function) {}
static M_INLINE void psglEnableReport(GLenum report) {}
static M_INLINE void psglDestroyDevice(PSGLdevice* device) {}
static M_INLINE void psglDestroyContext(PSGLcontext* LContext) {}
static M_INLINE PSGLdevice* psglCreateDevice(const PSGLbufferParameters* params) {return 0;}
static M_INLINE PSGLcontext* psglCreateContext() {return 0;}
static M_INLINE void psglMakeCurrent(PSGLcontext* context, PSGLdevice* device) {}
static M_INLINE void psglSwap() {}

static M_INLINE void cgGLEnableAttrib(GLuint index) {}
static M_INLINE void cgGLDisableAttrib(GLuint index) {}
static M_INLINE void DebugBreak() {}
static M_INLINE void glFinish() {}
static M_INLINE void glDeleteTextures(GLsizei n, const GLuint *textures) {}
static M_INLINE void glBindTexture(GLenum target, GLuint texture) {}
static M_INLINE void glActiveTexture(GLenum texture) {}
static M_INLINE void glClientActiveTexture(GLenum texture) {}
static M_INLINE int glGetError() {return GL_NO_ERROR;}
static M_INLINE void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data) {}
static M_INLINE void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {}
static M_INLINE void glTexParameteri(GLenum target, GLenum pname, GLint param) {}
static M_INLINE void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {}
static M_INLINE void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {}
static M_INLINE void glTexEnvf(GLenum target, GLenum pname, GLfloat param) {}
static M_INLINE void glEnableClientState(GLenum array) {}
static M_INLINE void glDisableClientState(GLenum array) {}
static M_INLINE void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {}
static M_INLINE void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer) {}
static M_INLINE void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {}
static M_INLINE void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {}
static M_INLINE void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {}
static M_INLINE void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {}
static M_INLINE void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices) {}
static M_INLINE void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {}
static M_INLINE void glBeginQueryARB(GLenum target,GLuint id) {}
static M_INLINE void glEndQueryARB(GLenum target) {}
static M_INLINE int glGetQueryivARB(GLenum target,GLenum pname,GLint* params) {return 0;}
static M_INLINE int glIsQueryARB(GLuint id) {return 0;}
static M_INLINE void glMatrixMode(GLenum mode) {}
static M_INLINE void glLoadIdentity() {}
static M_INLINE void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {}
static M_INLINE void glEnable(GLenum cap) {}
static M_INLINE void glDisable(GLenum cap) {}
static M_INLINE void glFrontFace(GLenum mode) {}
static M_INLINE void glCullFace(GLenum mode) {}
static M_INLINE void glShadeModel(GLenum mode) {}
static M_INLINE void glHint(GLenum target, GLenum mode) {}
static M_INLINE void glFogiv(GLenum pname, const GLint* params) {}
static M_INLINE void glFogi(GLenum pname, GLint param) {}
static M_INLINE void glFogf(GLenum pname, GLfloat param) {}
static M_INLINE void glPrimitiveRestartIndexNV(GLuint index) {}
static M_INLINE void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {}
static M_INLINE void glLoadMatrixf(const GLfloat *m) {}
static M_INLINE void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {}
static M_INLINE void glClearDepthf(GLclampf depth) {}
static M_INLINE void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {}
static M_INLINE void glDepthMask(GLboolean flag) {}
static M_INLINE void glStencilMask(GLuint mask) {}
static M_INLINE void glClear(GLbitfield mask) {}
static M_INLINE void glClearStencil(GLint s) {}
static M_INLINE void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {}
static M_INLINE void glBlendFunc(GLenum sfactor, GLenum dfactor) {}
static M_INLINE void glDepthFunc(GLenum func) {}
static M_INLINE void glAlphaFunc(GLenum func, GLclampf ref) {}
static M_INLINE void glPolygonOffset(GLfloat factor, GLfloat units) {}
static M_INLINE void glPolygonMode(GLenum face, GLenum mode) {}
static M_INLINE void glActiveStencilFaceEXT(GLenum face) {}
static M_INLINE void glStencilFunc(GLenum func, GLint ref, GLuint mask) {}
static M_INLINE void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {}
static M_INLINE void glGenFramebuffersOES(GLsizei, GLuint *) {}
static M_INLINE void glBindFramebufferOES(GLenum, GLuint) {}
static M_INLINE void glGenRenderbuffersOES(GLsizei, GLuint *) {}
static M_INLINE void glBindRenderbufferOES(GLenum, GLuint) {}
static M_INLINE void glRenderbufferStorageOES(GLenum, GLenum, GLsizei, GLsizei) {}
static M_INLINE void glFramebufferRenderbufferOES(GLenum, GLenum, GLenum, GLuint) {}
static M_INLINE void glDeleteBuffers(GLsizei n, const GLuint *buffers) {}
static M_INLINE void glBindBuffer(GLenum target, GLuint name) {}
static M_INLINE void glBufferData(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage) {}
static M_INLINE void* glMapBuffer(GLenum target, GLenum access) {return 0;}
static M_INLINE void glUnmapBuffer(GLenum target) {}

// CG emulation
typedef uint32	CGcontext;
typedef	uint32	CGprogram;
typedef uint32	CGparameter;
typedef uint32	CGerror;
typedef	uint32	CGprofile;
typedef uint32	CGenum;

enum
{
	CG_NO_ERROR,
	CG_PROFILE_SCE_VP_TYPEC,
	CG_PROFILE_SCE_FP_TYPEC,
	CG_BINARY,
	CG_SOURCE,
};

static M_INLINE const char* cgGetErrorString(CGerror error) {return "";}
static M_INLINE CGerror cgGetError() {return CG_NO_ERROR;}
static M_INLINE CGcontext cgCreateContext() {return 0;}
static M_INLINE void cgDestroyContext(CGcontext ctx) {}
static M_INLINE void cgGLBindProgram(CGprogram program) {}
static M_INLINE void cgGLUnbindProgram(CGprogram program) {}
static M_INLINE void cgGLEnableProfile(CGprofile profile) {}
static M_INLINE void cgGLDisableProfile(CGprofile profile) {}
static M_INLINE CGparameter cgGLGetParameter4f(CGparameter param, float *v) {return 0;}
static M_INLINE void cgGLSetParameterArray4f(CGparameter param, long offset, long nelements, const float *v) {}
static M_INLINE CGprogram cgCreateProgram(CGcontext ctx, CGenum program_type, const char *program, CGprofile profile, const char *entry, const char **args) {return 0;}
static M_INLINE CGparameter cgGetNamedParameter(CGprogram prog, const char *name) {return 0;}
static M_INLINE int cgGetArraySize(CGparameter param, int dimension) {return 0;}
static M_INLINE void cgGLDisableClientState(CGparameter param) {}
static M_INLINE void cgGLEnableClientState(CGparameter param) {}
static M_INLINE void cgGLAttribPointer(GLuint index, GLint fsize, GLenum type, GLboolean normalize, GLsizei stride, const GLvoid *pointer) {}
static M_INLINE void cgDestroyProgram(CGprogram program) {}
static M_INLINE void cgSetErrorCallback(void* func) {}
static M_INLINE void cgRTCgcInit() {}
