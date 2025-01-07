

#include "PCH.h"
#include "../../MOS.h"

#include "../../XR/XRVBContext.h"

CRenderContextPS3 CRenderContextPS3::ms_This;

// 0x500 offsetted
const char* aGLESErrorMessages[] = 
{
	"GL_INVALID_ENUM",
	"GL_INVALID_VALUE",
	"GL_INVALID_OPERATION",
	"GL_STACK_OVERFLOW",
	"GL_STACK_UNDERFLOW",
	"GL_OUT_OF_MEMORY",
	"GL_INVALID_FRAMEBUFFER_OPERATION_OES"
};

const char* aGLCGErrorMessages[] =
{
#ifndef WIN32
#define CG_ERROR_MACRO(code, enum_name, msg)\
	msg,
#include <cg/cg_errors.h>
#else
	"dummy"
#endif
};


// -------------------------------------------------------------------
//  OpenGL Render Context	
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CRenderContextPS3, CRC_Core);

CRenderContextPS3::CRenderContextPS3()
{
	MSCOPE(CRenderContextPS3::-, RENDER_GL);
	MRTC_AddRef();
	MRTC_AddRef();
	MRTC_AddRef();

	m_pCurrentFPProgram = NULL;
	m_pCurrentVPProgram = NULL;
	m_nUpdateVPConst = 0;
	m_CGContext = 0;
	m_pContext = NULL;

	m_VP_iConstantColor = 0;
	FillChar(m_VP_liTexMatrixReg, sizeof(m_VP_liTexMatrixReg), 0);

	m_iVBCtxRC = -1;
	m_iTC = -1;


#ifdef CRCGL_POLYGONOFFSETPROJECTIONMATRIX
	m_CurrentProjectionPolygonOffset = 0;
#endif
	m_CurrentAttribGeomColor = 0xffffffff;

	m_CurrentVPHeight = 0;
	m_CurrentVPWidth = 0;
	m_CurrentVPPos = CPnt(0,0);
//	m_CurrentRenderTargetHeight = 0;
//	m_CurrentRenderTargetWidth = 0;
	m_CurrentWindowHeight = 0;
	m_CurrentWindowWidth = 0;


	FillChar(m_bInternalTexMatrixEnable, sizeof(m_bInternalTexMatrixEnable), 0);

	m_bLog = true;
	m_bLogVP = true;

	for(int i = 0; i < CRC_MAXPICMIPS; i++)
	{
		m_lPicMips[i] = 0;
	}

	m_Matrix_ProjectionGL.Unit();
	m_Matrix_ProjectionTransposeGL.Unit();
	m_Matrix_Unit.Unit();

	m_bPendingTextureParamUpdate = 0;
	m_bPendingProgramReload = 0;
	m_bResourceLog = false;
	
	m_VAEnable = 0;
	m_VAMinVertexCount = CRCGL_DEFAULTVAMINVERTEXCOUNT;
#ifdef _DEBUG
	m_VADebugState.Clear();
#endif


	m_Anisotropy = 1.0f;
	m_MaxAnisotropy = 16.0f;

	m_VSync = 1;
};

CRenderContextPS3::~CRenderContextPS3()
{
	MSCOPE(CRenderContextPS3::~, RENDER_GL);

	VP_DeleteAllObjects();
	VB_DeleteAll();

	if (m_pTC)
		GL_DeleteTextures();
	if (m_iVBCtxRC >= 0) { m_pVBCtx->RemoveRenderContext(m_iVBCtxRC); m_iVBCtxRC = -1; };
	if (m_iTC >= 0) { m_pTC->RemoveRenderContext(m_iTC); m_iTC = -1; };
};

void CRenderContextPS3::Create(CObj* _pContext, const char* _pParams)
{
	MSCOPE(CRenderContextPS3::Create, RENDER_GL);

	CRC_Core::Create(_pContext, _pParams);

	MACRO_GetSystem;

	m_Caps_TextureFormats = -1;
	m_Caps_DisplayFormats = -1;
	m_Caps_ZFormats = -1;
	m_Caps_StencilDepth = 8;
	m_Caps_AlphaDepth = 8;
	m_Caps_Flags	= CRC_CAPS_FLAGS_HWAPI
					| CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP
					| CRC_CAPS_FLAGS_SPECULARCOLOR
					| CRC_CAPS_FLAGS_CUBEMAP
					| CRC_CAPS_FLAGS_MATRIXPALETTE
//					| CRC_CAPS_FLAGS_OFFSCREENTEXTURERENDER
					| CRC_CAPS_FLAGS_OCCLUSIONQUERY
					| CRC_CAPS_FLAGS_TEXGENMODE_ENV
					| CRC_CAPS_FLAGS_TEXGENMODE_ENV2
					| CRC_CAPS_FLAGS_TEXGENMODE_LINEARNHF
					| CRC_CAPS_FLAGS_TEXGENMODE_BOXNHF
					| CRC_CAPS_FLAGS_ARBITRARY_TEXTURE_SIZE
					| CRC_CAPS_FLAGS_SEPARATESTENCIL
					| CRC_CAPS_FLAGS_FRAGMENTPROGRAM20
					| CRC_CAPS_FLAGS_FRAGMENTPROGRAM30
					| CRC_CAPS_FLAGS_PRIMITIVERESTART;

	m_Caps_nMultiTexture = Min(32, (int)CRC_MAXTEXTURES);
	m_Caps_nMultiTextureCoords = Min(8, (int)CRC_MAXTEXCOORDS);
	m_Caps_nMultiTextureEnv = Min(8, (int)CRC_MAXTEXCOORDS);

	m_iTC = m_pTC->AddRenderContext(this);
	m_iVBCtxRC = m_pVBCtx->AddRenderContext(this);

	MACRO_GetSystemEnvironment(pEnv);

	for (int i = 0; i < CRC_MAXPICMIPS; ++i)
	{
		m_lPicMips[i] = pEnv->GetValuei(CStrF("R_PICMIP%d", i), 2);
	}

	m_VP_ProgramGenerator.Create("System/GL/VP.xrg", "System/GL/VPDefines_PS3.xrg", false, CRC_MAXTEXCOORDS);
}

extern const char* gs_TexEnvPrograms[9];
extern uint32 gs_TexEnvProgramHash[9];

void CRenderContextPS3::GL_InitSettings()
{
	// Set matrices
	glnMatrixMode(GL_TEXTURE);
	glnLoadIdentity();
	glnMatrixMode(GL_MODELVIEW);
	glnLoadIdentity();

	// Some settings.
	glnClearColor(0.0, 0.0, 0.0, 0.0);
	glnEnable(GL_DEPTH_TEST);
	GLErr("BeginScene");

	glnDisable(GL_NORMALIZE);
	glnFrontFace(GL_CCW);
	glnCullFace(GL_FRONT);
	glnDisable(GL_CULL_FACE);
	glnShadeModel(GL_SMOOTH);
	glnEnable(GL_DITHER);
	GLErr("BeginScene");

	glnHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	GLErr("BeginScene");

	glnFogi(GL_FOG_MODE, GL_LINEAR);

	glPrimitiveRestartIndexNV(0xffff);
	glEnableClientState(GL_PRIMITIVE_RESTART_NV);
	GLErr("glEnableClientState(GL_PRIMITIVE_RESTART_NV)");

	for(int i = 0; i < 9; i++)
	{
		gs_TexEnvProgramHash[i] = StringToHash(gs_TexEnvPrograms[i]);
	}

	CRC_Attributes Default;
	Default.SetDefault();
	Attrib_SetAbsolute(&Default);

	VP_LoadCache();
	FP_LoadCache();
}

void CRenderContextPS3::Texture_PrecacheFlush( )
{
	int nTxt = m_pTC->GetIDCapacity();
	for(int i = 1; i < nTxt; i++)
	{
		int Flags = m_pTC->GetTextureFlags(i);

		if (!(Flags & CTC_TXTIDFLAGS_ALLOCATED))
		{
			GL_UnloadTexture(i);
		}
		else
		{
			CTC_TextureProperties Properties;
			m_pTC->GetTextureProperties(i, Properties);

			if (!(Flags & CTC_TXTIDFLAGS_PRECACHE) && !(Properties.m_Flags & CTC_TEXTUREFLAGS_RENDER))
			{
				GL_UnloadTexture(i);
			}
		}
	}
}

void CRenderContextPS3::Texture_PrecacheBegin( int _Count )
{
	M_TRACEALWAYS("#########################\r\n");
	M_TRACEALWAYS("####Texture_PrecacheBegin\r\n");
}

void CRenderContextPS3::Texture_PrecacheEnd()
{
	M_TRACEALWAYS("####Texture_PrecacheEnd\r\n");
	M_TRACEALWAYS("#########################\r\n");
}

void CRenderContextPS3::Texture_Precache(int _TextureID)
{
	MSCOPE(CRenderContextPS3::Texture_Precache, RENDER_GL);

	CTC_TextureProperties Props;
	m_pTC->GetTextureProperties(_TextureID, Props);
	if (!(Props.m_Flags & CTC_TEXTUREFLAGS_PROCEDURAL))
	{
//		GL_SetTexture(_TextureID, 0, true);
		GL_BuildTexture(_TextureID, _TextureID, Props);
		m_CurrentAttrib.m_TextureID[0] = _TextureID;

		GL_SetTexture(0, 0);
		m_CurrentAttrib.m_TextureID[0] = 0;
		m_AttribChanged |= CRC_ATTRCHG_TEXTUREID;
	}
	else
	{
		M_TRACEALWAYS("Not precaching texture %d (%s)\n", _TextureID, m_pTC->GetName(_TextureID).GetStr());
	}
}

void CRenderContextPS3::Texture_Copy(int _SourceTexID, int _DestTexID, CRct _SrcRgn, CPnt _DstPos)
{
}

CRC_TextureMemoryUsage CRenderContextPS3::Texture_GetMem(int _TextureID)
{
	CImage Img;
	int nMip;
	m_pTC->GetTextureDesc(_TextureID, &Img, nMip);
	
	CTC_TextureProperties Properties;
	m_pTC->GetTextureProperties(_TextureID, Properties);

//	int Mem = Img.GetWidth() * Img.GetHeight();

//	int MinWidth = 1;
//	if(!(Properties.m_Flags & CTC_TEXTUREFLAGS_NOCOMPRESS))
//		MinWidth = 4;


	int Mem = 0;
	{
		int w = Img.GetWidth();
		int h = Img.GetHeight();
		if (!(Properties.m_Flags & CTC_TEXTUREFLAGS_NOPICMIP))
		{
			int PicMip = MaxMT(0, m_lPicMips[Properties.m_iPicMipGroup] + Properties.GetPicMipOffset());

			w = Max(1, w >> PicMip);
			h = Max(1, h >> PicMip);
		}
		while((w > 1) && (h > 1))
		{
			if(Properties.m_Flags & CTC_TEXTUREFLAGS_NOCOMPRESS)
				Mem += w * h;
			else
				Mem += Max(4, w) * Max(4, h);

			if (Properties.m_Flags & CTC_TEXTUREFLAGS_NOMIPMAP)
				break;

			w	= Max(1, w >> 1);
			h	= Max(1, h >> 1);
		}
	}

//	if(!(Properties.m_Flags & CTC_TEXTUREFLAGS_NOPICMIP))
//		Mem >>= (m_lPicMips[Properties.m_iPicMip]*2);
	
	if(Properties.m_Flags & CTC_TEXTUREFLAGS_NOCOMPRESS)
	{
		if (Img.GetPixelSize() == 2)
		{
			Mem <<= 1;
		}
		else if (Img.GetPixelSize() > 2)
		{
			if(Properties.m_Flags & CTC_TEXTUREFLAGS_HIGHQUALITY)
				Mem <<= 2;
			else
				Mem <<= 1;
		}
	}
	else
	{
		if(Img.GetMemModel() & IMAGE_MEM_COMPRESSTYPE_S3TC)
		{
			if(!(Img.GetFormat() & IMAGE_FORMAT_ALPHA))
				Mem >>= 1;
		}
	}
	

	CRC_TextureMemoryUsage Ret;
	Ret.m_BestCase = Ret.m_Theoretical = Ret.m_WorstCase = Mem + 12+8+128;
	return Ret;
}

int CRenderContextPS3::Texture_GetPicmipFromGroup(int _iPicmip)
{
	return m_lPicMips[_iPicmip];
}

void CRenderContextPS3::GL_DestroyAllPrograms()
{
	FP_Disable();
	VP_Disable();
	{
		CCGFPProgram* pProg = m_FPProgramTree.GetRoot();
		while(pProg)
		{
			cgDestroyProgram(pProg->m_Program);
			m_FPProgramTree.f_Remove(pProg);
			pProg = m_FPProgramTree.GetRoot();
		}
	}

	{
		CCGVPProgram* pProg = m_ProgramTree.GetRoot();
		while(pProg)
		{
			cgDestroyProgram(pProg->m_Program);
			m_ProgramTree.f_Remove(pProg);
			pProg = m_ProgramTree.GetRoot();
		}
	}
}

int NextPow2(int _Value);

//----------------------------------------------------------------
void CRenderContextPS3::BeginScene(CRC_Viewport* _pVP)
{
	MSCOPE(CRenderContextPS3::BeginScene, RENDER_GL);

	m_StartFrame.Snapshot();

	GLErr("BeginScene (PrevErr)");
/*
	glnMatrixMode(GL_PROJECTION);
	glnPushMatrix();
	GLErr("BeginScene");

	glnMatrixMode(GL_MODELVIEW);
	glnPushMatrix();
	GLErr("BeginScene");
*/
	if (m_bPendingTextureParamUpdate)
	{
		m_bPendingTextureParamUpdate = 0;
		GL_UpdateAllTextureParameters();
	}
	if(m_bPendingProgramReload)
	{
		m_bPendingProgramReload = 0;
		GL_DestroyAllPrograms();
	}

	Attrib_DeferredGlobalUpdate();
	GLErr("BeginScene (Attrib_DeferredGlobalUpdate)");
	CRC_Core::BeginScene(_pVP);

//	m_CurrentRenderTargetHeight = 0;
//	m_CurrentRenderTargetWidth = 0;
//	m_CurrentWindowHeight = 0;
//	m_CurrentWindowWidth = 0;


	m_nBindTexture = 0;
	m_nTriTotal = 0;
	m_nTriCulled = 0;
	m_nVertices = 0;
	m_nAttribSet = 0;

	m_nStateEnableColorWrite = 0;
	m_nStateEnableZWrite = 0;
	m_nStateEnableZTest = 0;
	m_nStateEnableBlend = 0;
	m_nStateEnableSmooth = 0;
	m_nStateEnableCull = 0;
	m_nStateEnableCullCCW = 0;
	m_nStateEnableFog = 0;
	m_nStateEnableAlphaTest = 0;

	m_nStateBlendFunc = 0;
	m_nStateZFunc = 0;
	m_nStateAlphaFunc = 0;
	m_nStateStencil = 0;
	m_nStateFogColor = 0;
	m_nStateFogStart = 0;
	m_nStateFogEnd = 0;
	m_nStateFogDensity = 0;
	m_nStatePolygonOffset = 0;

	m_nStateVAAddress = 0;
	m_nStateVADrawElements = 0;
	m_nStateVAEnable = 0;
	m_nStateVP = 0;
	m_nStateVPPipeline = 0;
	m_nStateFP = 0;
	m_nStateParamTransform = 0;
	m_nStateParamVP = 0;
	m_nStateParamFP = 0;
/*
	VP_Init();

	// Set matrices
	glnMatrixMode(GL_TEXTURE);
	glnLoadIdentity();
	glnMatrixMode(GL_MODELVIEW);
	glnLoadIdentity();

	// Some settings.
	glnClearColor(0.0, 0.0, 0.0, 0.0);
	glnEnable(GL_DEPTH_TEST);
	GLErr("BeginScene");

	glnDisable(GL_NORMALIZE);
	glnFrontFace(GL_CCW);
	glnCullFace(GL_BACK);
	glnDisable(GL_CULL_FACE);
	glnShadeModel(GL_SMOOTH);
	glnEnable(GL_DITHER);
	GLErr("BeginScene");

	glnHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	GLErr("BeginScene");

	glnFogi(GL_FOG_MODE, GL_LINEAR);

	m_pCurrentExtAttrNV10 = NULL;
	m_pCurrentExtAttrNV20 = NULL;
	m_pCurrentExtAttrPixelShader = NULL;
	m_CurrentAttrib.SetDefault();
	Attrib_SetAbsolute(&m_CurrentAttrib);
	Attrib_Push();
	Matrix_SetMode(CRC_MATRIX_TEXTURE);
	Matrix_Push();
	Matrix_SetMode(CRC_MATRIX_MODEL);
	Matrix_Push();
*/
	GLErr("BeginScene");

	OcclusionQuery_PrepareFrame();
	GLErr("BeginScene (0)");

//	glBindFramebufferOES(GL_FRAMEBUFFER_OES, CDisplayContextPS3::ms_This.m_CurrentBackbufferContext.m_FBO);
//	GLErr("BeginScene (1)");
};

void CRenderContextPS3::EndScene()
{
	MSCOPE(CRenderContextPS3::EndScene, RENDER_GL);

//	glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
//	GLErr("EndScene (0)");

	PreEndScene();

	GLErr("EndScene (1)");
/*
	Internal_VA_Disable();
	GLErr("EndScene (0)");

	VP_Disable();
	GLErr("EndScene (1)");

	Attrib_SetAbsolute(&m_lAttribStack[0]);
	GLErr("EndScene (2)");

	Matrix_SetMode(CRC_MATRIX_TEXTURE);
	Matrix_Pop();
	GLErr("EndScene (3)");
	Matrix_SetMode(CRC_MATRIX_MODEL);
	Matrix_Pop();
	GLErr("EndScene (4)");
	Attrib_Pop();
//	glnViewport(0,0, 640, 480);

	glnMatrixMode(GL_MODELVIEW);
	v("EndScene (8)");
	glnPopMatrix();
	GLErr("EndScene (9)");

	glnMatrixMode(GL_PROJECTION);
	GLErr("EndScene (10)");
	glnPopMatrix();
	GLErr("EndScene (11)");
*/
	CRC_Core::EndScene();


	{
		// Update of m_Stats
		CMTime Now;
		Now.Snapshot();
		m_Stats.m_Time_FrameTime.AddData((Now - m_StartFrame).GetTime());
		m_Stats.m_CPUUsage.AddData((Now - m_StartFrame).GetTime() / (Now - m_LastFrame).GetTime() * 100.0f);
		m_Stats.m_GPUUsage.AddData(100.0f);
		m_Stats.m_nPixlesTotal.AddData(0);
		m_Stats.m_nPixlesZPass.AddData(0);
		m_LastFrame = Now;
	}

	if(m_bDirtyFPCache)
		FP_SaveCache();

	if(m_bDirtyVPCache)
		VP_SaveCache();
};

const char * CRenderContextPS3::GetRenderingStatus()
{
	MSCOPE(CRenderContextPS3::GetRenderingStatus, RENDER_GL);

	static char TempChar[512];
/*	return CStrF("OpenGL iT %d, P %d, T %d, TC %d, W %d, Ptl %d, nBind %d, Txt %d, %d", 
		m_nTriangles, m_nPolygons, m_nTriTotal, m_nTriCulled, m_nWires, m_nParticles, 
		m_nBindTexture, m_TextureUseSize[0], m_TextureUseSize[1]);*/
	CStrBase::snprintf(TempChar, 512, "Playstation 3,Tri %d Vert %d,Strip %d (%d cull),Wire %d,"
			"ClipV %d ClipTri %d ClipVis %d,"
			"TexSw %d,"
			"nStates %d,  Cw %d Zw %d Zt %d Zf %d,"
			"B %d Bf %d Af %d At %d,"
			"F %d Fc %d Fs %d Fe %d Fd %d,"
			"C %d %d PO %d St %d,"
			"VAdr %d VAD %d VA %d VP %d FP %d,%d,"
			"Params (%d Tranf %d VP %d FP)"
			, 
		m_nTriTotal, m_nVertices, m_nTriangles, m_nTriCulled, m_nWires, 
		m_nClipVertices, m_nClipTriangles, m_nClipTrianglesDrawn,
		m_nBindTexture,
		m_nAttribSet, m_nStateEnableColorWrite, m_nStateEnableZWrite, m_nStateEnableZTest, m_nStateZFunc, m_nStateEnableBlend, m_nStateBlendFunc, m_nStateAlphaFunc, m_nStateEnableAlphaTest,
		m_nStateEnableFog, m_nStateFogColor, m_nStateFogStart, m_nStateFogEnd, m_nStateFogDensity, 
		m_nStateEnableCull, m_nStateEnableCullCCW, m_nStatePolygonOffset, m_nStateStencil,
		m_nStateVAAddress, m_nStateVADrawElements, m_nStateVAEnable, m_nStateVP, m_nStateFP, m_nStateVPPipeline,
		m_nStateParamTransform, m_nStateParamVP, m_nStateParamFP
		);
	return TempChar;
};

void CRenderContextPS3::Viewport_Update()
{
	MSCOPE(CRenderContextPS3::Viewport_Update, RENDER_GL);

	GLErr("Viewport_Update");

	CRC_Core::Viewport_Update();
	CRC_Viewport* pVP = Viewport_Get();

	// Set projection
	CMat4Dfp4 ProjMat = pVP->GetProjectionMatrix();

	// Set viewport
	CRct rectclipped(pVP->GetViewArea());
	int h = CDisplayContextPS3::ms_This.m_CurrentBackbufferContext.m_Setup.m_Height;
	glnViewport(rectclipped.p0.x, h-rectclipped.p0.y-rectclipped.GetHeight(), rectclipped.GetWidth(), rectclipped.GetHeight());
	CRct Viewport(rectclipped.p0.x, h-rectclipped.p0.y-rectclipped.GetHeight(), rectclipped.GetWidth(), rectclipped.GetHeight());
	DebugNop(&Viewport);
	GLErr("Viewport_Update (0)");

	m_CurrentVPHeight = rectclipped.GetHeight();
	m_CurrentVPWidth = rectclipped.GetWidth();
	m_CurrentVPPos = rectclipped.p0;

	glnMatrixMode(GL_PROJECTION);
	GLErr("Viewport_Update (1)");

	fp4 xs = 2.0/rectclipped.GetWidth();
	fp4 ys = 2.0/rectclipped.GetHeight();
	ProjMat.k[0][0] *= xs;	ProjMat.k[1][0] *= xs;	ProjMat.k[2][0] *= xs;	ProjMat.k[3][0] *= xs;
	ProjMat.k[0][1] *= ys;	ProjMat.k[1][1] *= ys;	ProjMat.k[2][1] *= ys;	ProjMat.k[3][1] *= ys;

#ifdef CRCGL_POLYGONOFFSETPROJECTIONMATRIX
	ProjMat.k[2][2] += 0.01f * m_CurrentProjectionPolygonOffset * m_BackPlaneInv;
#endif

	glnLoadMatrixf((fp4*) &ProjMat.k[0]);
	GLErr("Viewport_Update (3)");

	m_Matrix_ProjectionGL = ProjMat;
	ProjMat.Transpose();
	m_Matrix_ProjectionTransposeGL = ProjMat;

	CRenderContextPS3::ms_This.m_MatrixChanged |= 1 << CRC_MATRIX_MODEL;
};

CDisplayContext* CRenderContextPS3::GetDC()
{
	return &CDisplayContextPS3::ms_This;
}

void CRenderContextPS3::DebugNop(void* _pArg)
{
	int a = 0;
}

void ReportPSGL(GLenum _reportEnum, GLuint _reportClassMask, const char* _pString)
{
	MRTC_SystemInfo::OS_Trace(CStrF("GLERR: %s\n", _pString));
}

void ReportCG(void)
{
	CGerror err = cgGetError();
	MRTC_SystemInfo::OS_Trace(CStrF("CGERR: %s\n", cgGetErrorString(err)));
}

