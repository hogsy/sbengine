#include "PCH.h"

#include "MDispGL.h"
#include "MRndrGL.h"
#include "MRndrGL_Def.h"

void CRenderContextGL::LogExtensionState()
{
	// -------------------------------------------------------------------
	if(m_bLog) ConOutL("---------------------------------------------");
	if(m_bLog) ConOutL(CStr("MaxTextureSize:§x250%d/%d", m_MaxTextureSize, m_MaxTextureSizeReal));
	if(m_bLog) ConOutL(CStr("TextureDepth:§x250%d", m_RGBATextureDepth));
	if(m_bLog) ConOutL(CStr("TxtPalette:   §x250%s", (m_bTxtPalette256) ? "Yes" : "-"));
#ifdef CRCGL_SHAREDPALETTE
	if(m_bLog) ConOutL(CStr("SharedTxtPalette:§x250%s", (m_bSharedTxtPalette) ? "Yes" : "-"));
#endif
#ifdef CRCGL_ALPHALIGHTMAPS
	if(m_bLog) ConOutL(CStr("Alpha LM: §x250%s", (m_bAlphaLightMaps) ? "Yes" : "-"));
#endif
	if(m_bLog) ConOutL(CStr("PicMip:§x250%d, %d, %d, %d", m_lPicMips[0], m_lPicMips[1], m_lPicMips[2], m_lPicMips[3]) );
	if(m_bLog) ConOutL(CStr("Stencil bits:§x250%d", m_Caps_StencilDepth) );
	if(m_bLog) ConOutL(CStr("Alpha bits:§x250%d", m_Caps_AlphaDepth) );
	if(m_bLog) ConOutL(CStr("Max Texture Units:§x250%d", m_nMultiTexture) );
	if(m_bLog) ConOutL(CStr("Max Texture Env:§x250%d", m_nMultiTextureEnv) );
	if(m_bLog) ConOutL(CStr("Max Texture Coordinates:§x250%d", m_nMultiTextureCoords) );
	if(m_bLog) ConOutL(CStr("ARB MultiTexture:§x250%s, %s", (m_Extensions & CRCGL_EXT_MULTITEXTURE) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & CRCGL_EXT_MULTITEXTURE) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("NV CopyDepthToColor: §x250%s, %s", (m_Extensions & CRCGL_EXT_NV_COPYDEPTHTOCOLOR) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & CRCGL_EXT_NV_COPYDEPTHTOCOLOR) ? "§c0f0Active§d" : "§cf00Inactive"));
	if(m_bLog) ConOutL(CStr("ARB WindowPos: §x250%s, %s", (m_Extensions & CRCGL_EXT_WINDOWPOS) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & CRCGL_EXT_WINDOWPOS) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("ARB Non Power of two textures: §x250%s, %s", (m_Extensions & CRCGL_ARB_TEXTURE_NON_POWER_OF_TWO) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & CRCGL_ARB_TEXTURE_NON_POWER_OF_TWO) ? "§c0f0Active§d" : "§cf00Inactive§d"));
//	if(m_bLog) ConOutL(CStr("EXT Compiled Vertex Array:§x250%s, %s", (m_Extensions & CRCGL_EXT_COMPILEDVERTEXARRAYS) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & CRCGL_EXT_COMPILEDVERTEXARRAYS) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("EXT BGRA:         §x250%s, %s", (m_Extensions & CRCGL_EXT_BGRA) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & CRCGL_EXT_BGRA) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("EXT Secondary Color:§x250%s, %s", (m_Extensions & CRCGL_EXT_SECONDARYCOLOR) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & CRCGL_EXT_SECONDARYCOLOR) ? "§c0f0Active§d" : "§cf00Inactive§d"));
//	if(m_bLog) ConOutL(CStr("EXT Fog Coord:§x250%s, %s", (m_Extensions & CRCGL_EXT_FOGCOORD) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & CRCGL_EXT_FOGCOORD) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("ARB Texture Compression:§x250%s, %s", (m_Extensions & CRCGL_EXT_TEXTURECOMPRESSION) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & CRCGL_EXT_TEXTURECOMPRESSION) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("ARB Texture Cube Map:§x250%s, %s  (%d/%d size)", (m_Extensions & CRCGL_EXT_CUBEMAP) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_CUBEMAP) ? "§c0f0Active§d" : "§cf00Inactive§d", m_MaxCubeTextureSizeReal, m_MaxCubeTextureSize));
	if(m_bLog) ConOutL(CStr("EXT Texture Compression S3TC:§x250%s, %s", (m_Extensions & CRCGL_EXT_S3TC) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & CRCGL_EXT_TEXTURECOMPRESSION) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("EXT Texture Filter Anisotropic:§x250%s, %s  (Max anisotropy %f)", (m_Extensions & CRCGL_EXT_ANISOTROPIC) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_ANISOTROPIC) ? "§c0f0Active§d" : "§cf00Inactive§d", m_MaxAnisotropy));
	if(m_bLog) ConOutL(CStr("EXT Texture LOD Bias:§x250%s, %s", (m_Extensions & CRCGL_EXT_TEXTURE_LOD_BIAS) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_TEXTURE_LOD_BIAS) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("NV RegisterCombiners:§x250%s, %s  (%d combiners)", (m_Extensions & CRCGL_EXT_NV_REGCOMBINERS) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_NV_REGCOMBINERS) ? "§c0f0Active§d" : "§cf00Inactive§d", m_RegCombiners_MaxCombiners));
	if(m_bLog) ConOutL(CStr("NV RegisterCombiners2:§x250%s, %s", (m_Extensions & CRCGL_EXT_NV_REGCOMBINERS2) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_NV_REGCOMBINERS2) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("NV TextureShader:§x250%s, %s", (m_Extensions & CRCGL_EXT_NV_TEXTURESHADER) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_NV_TEXTURESHADER) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("NV VertexProgram:§x250%s, %s  (Always: %d)", (m_Extensions & CRCGL_EXT_NV_VERTEXPROGRAM) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_NV_VERTEXPROGRAM) ? "§c0f0Active§d" : "§cf00Inactive§d", m_VP_bUseAlways));
	if(m_bLog) ConOutL(CStr("NV Occlusion Query:§x250%s, %s", (m_Extensions & CRCGL_EXT_NV_OCCLUSIONQUERY) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_NV_OCCLUSIONQUERY) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("ARB VertexProgram:§x250%s, %s  (Always: %d)", (m_Extensions & CRCGL_EXT_VERTEXPROGRAM) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_VERTEXPROGRAM) ? "§c0f0Active§d" : "§cf00Inactive§d", m_VP_bUseAlways));
//	if(m_bLog) ConOutL(CStr("ATI VertexArrayObject:§x250%s, %s", (m_Extensions & CRCGL_EXT_ATI_VERTEXARRAYOBJECT) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_ATI_VERTEXARRAYOBJECT) ? "§c0f0Active§d" : "§cf00Inactive§d"));
//	if(m_bLog) ConOutL(CStr("ATI VertexAttribArrayObject:§x250%s, %s", (m_Extensions & CRCGL_EXT_ATI_VERTEXATTRIBARRAYOBJECT) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_ATI_VERTEXATTRIBARRAYOBJECT) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("ARB TextureEnvCombine + Dot3:§x250%s, %s", (m_Extensions & CRCGL_ARB_TEXTURE_ENV_DOT3) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_ARB_TEXTURE_ENV_DOT3) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("ARB FragmentProgram:§x250%s, %s (%d,%d)", (m_Extensions & CRCGL_EXT_FRAGMENTPROGRAM) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_FRAGMENTPROGRAM) ? "§c0f0Active§d" : "§cf00Inactive§d", m_nMultiTexture, m_nMultiTextureCoords));
	if(m_bLog) ConOutL(CStr("ATI SeparateStencil:§x250%s, %s", (m_Extensions & CRCGL_EXT_ATI_SEPARATESTENCIL) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_ATI_SEPARATESTENCIL) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("EXT StencilTwoSide:§x250%s, %s", (m_Extensions & CRCGL_EXT_STENCILTWOSIDE) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_STENCILTWOSIDE) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("ATI FragmentShader:§x250%s, %s", (m_Extensions & CRCGL_EXT_ATI_FRAGMENTSHADER) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_ATI_FRAGMENTSHADER) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("ARB VertexBufferObject:§x250%s, %s", (m_Extensions & CRCGL_EXT_VERTEXBUFFEROBJECT) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_VERTEXBUFFEROBJECT) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("NV FragmentProgramOption:§x250%s, %s", (m_Extensions & CRCGL_EXT_NV_FRAGMENT_PROGRAM) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_NV_FRAGMENT_PROGRAM) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("NV FragmentProgram2Option:§x250%s, %s", (m_Extensions & CRCGL_EXT_NV_FRAGMENT_PROGRAM2) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_EXT_NV_FRAGMENT_PROGRAM2) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("ATI Texture Compression 3DC:§x250%s, %s", (m_Extensions & CRCGL_EXT_TEXTURE_COMPRESSION_3DC) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & CRCGL_EXT_TEXTURE_COMPRESSION_3DC) ? "§c0f0Active§d" : "§cf00Inactive§d"));
	if(m_bLog) ConOutL(CStr("EXT FrameBuffer Object:§x250%s, %s (MaxColorBuffers %d, MaxBufferSize %d)", (m_Extensions & CRCGL_EXT_FRAMEBUFFER_OBJECT) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & CRCGL_EXT_FRAMEBUFFER_OBJECT) ? "§c0f0Active§d" : "§cf00Inactive§d", m_FBO_MaxColorBuffers, m_FBO_MaxRenderBufferSize));
	if(m_bLog) ConOutL(CStr("ARB Pbuffer:§x250%s, %s", (m_Extensions & CRCGL_ARB_PBUFFER) ? "§c0f0Yes§d" : "-", (m_ExtensionsActive & m_Extensions & CRCGL_ARB_PBUFFER) ? "§c0f0Active§d" : "§cf00Inactive§d"));

	if(m_bLog) ConOutL("---------------------------------------------");
}

void CRenderContextGL::GetExtension_ExtensionsStringEXT()
{
	PFNGETEXTENSIONSSTRING pFn = (PFNGETEXTENSIONSSTRING)wglGetProcAddress("wglGetExtensionsStringEXT");
	if(!pFn) return;
	m_WGLExtensions = pFn();
}

void CRenderContextGL::GetExtension_ExtensionsStringARB()
{
/*	PFNGETEXTENSIONSSTRING pFn = (PFNGETEXTENSIONSSTRING)wglGetProcAddress("wglGetExtensionsStringARB");
	if(!pFn)
		return;
	m_WGLExtensions = pFn();*/
}

void CRenderContextGL::GetExtension_Palette()
{
	m_fp_ColorTableEXT = 0;
	m_fp_ColorSubTableEXT = 0;
	m_bTxtPalette256 = false;

    m_fp_ColorTableEXT = (PFNGLCOLORTABLEEXTPROC) wglGetProcAddress("glColorTableEXT");
    if (!m_fp_ColorTableEXT) return;

#ifdef CRCGL_EXTENSIVE_PALETTE_CHECK

    m_fp_ColorSubTableEXT = (PFNGLCOLORSUBTABLEEXTPROC) wglGetProcAddress("glColorSubTableEXT");
    if (!m_pfnColorSubTableEXT) return;

    // Check color table size
    PFNGLGETCOLORTABLEPARAMETERIVEXTPROC pfnGetColorTableParameterivEXT = 
		(PFNGLGETCOLORTABLEPARAMETERIVEXTPROC) wglGetProcAddress("glGetColorTableParameterivEXT");
    if (!pfnGetColorTableParameterivEXT) return;

    // For now, the only paletted textures supported in this lib are TEX_A8,
    // with 256 color table entries.  Make sure the device supports this.
    m_pfnColorTableEXT(GL_PROXY_TEXTURE_2D, GL_RGBA, 256, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL );
	int Size;
    pfnGetColorTableParameterivEXT(GL_PROXY_TEXTURE_2D, GL_COLOR_TABLE_WIDTH_EXT, &Size );
    if (Size != 256) return;

#endif
	m_bTxtPalette256 = true;
}

void CRenderContextGL::GetExtension_MultiTextureARB()
{
	if(m_bLog) LogFile("Checking extension GL_ARB_MULTITEXTURE...");
	if (m_GLExtensions.Find("GL_ARB_MULTITEXTURE") < 0) return;

	m_fp_glSelectTexture=
		(PFNSELECTTEXTURE) wglGetProcAddress("glActiveTextureARB");

	m_fp_glSelectTextureCoordSet =
		(PFNSELECTTEXTURECOORDSET) wglGetProcAddress("glClientActiveTextureARB");

	m_fp_glMTexCoord2f =
		(PFNMULTITEXCOORD2F) wglGetProcAddress("glMultiTexCoord2fARB");
	m_fp_glMTexCoord2fv =
		(PFNMULTITEXCOORD2FV) wglGetProcAddress("glMultiTexCoord2fvARB");
	m_fp_glMTexCoord3f =
		(PFNMULTITEXCOORD3F) wglGetProcAddress("glMultiTexCoord3fARB");
	m_fp_glMTexCoord3fv =
		(PFNMULTITEXCOORD3FV) wglGetProcAddress("glMultiTexCoord3fvARB");
	m_fp_glMTexCoord4f =
		(PFNMULTITEXCOORD4F) wglGetProcAddress("glMultiTexCoord4fARB");
	m_fp_glMTexCoord4fv =
		(PFNMULTITEXCOORD4FV) wglGetProcAddress("glMultiTexCoord4fvARB");

	m_nMultiTexture = 0;
	{
		glnGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &m_nMultiTexture);
		if (glnGetError() != GL_NO_ERROR) m_nMultiTexture = 0;
	}

	int m_nMultiTextureReal = m_nMultiTexture;
	if (m_nMultiTexture > CRC_MAXTEXTURES)
		m_nMultiTexture = CRC_MAXTEXTURES;

	m_nMultiTextureEnv = Min((int)CRC_MAXTEXTUREENV, m_nMultiTexture);
	m_nMultiTextureCoords = m_nMultiTexture;

	if(m_bLog) LogFile(CStr("    glActiveTextureARB             %.8x", m_fp_glSelectTexture));
	if(m_bLog) LogFile(CStr("    glClientActiveTextureARB       %.8x", m_fp_glSelectTextureCoordSet));
	if(m_bLog) LogFile(CStr("    glMultiTexCoord2fARB           %.8x", m_fp_glMTexCoord2f));
	if(m_bLog) LogFile(CStr("    glMultiTexCoord2fvARB          %.8x", m_fp_glMTexCoord2fv));
	if(m_bLog) LogFile(CStr("    glMultiTexCoord3fARB           %.8x", m_fp_glMTexCoord3f));
	if(m_bLog) LogFile(CStr("    glMultiTexCoord3fvARB          %.8x", m_fp_glMTexCoord3fv));
	if(m_bLog) LogFile(CStr("    glMultiTexCoord4fARB           %.8x", m_fp_glMTexCoord4f));
	if(m_bLog) LogFile(CStr("    glMultiTexCoord4fvARB          %.8x", m_fp_glMTexCoord4fv));
	if(m_bLog) LogFile(CStr("    TextureUnitsUsed               %d (%d avail)", m_nMultiTexture, m_nMultiTextureReal ));

	if ((m_nMultiTexture < 2) ||
		!m_fp_glSelectTexture ||
		!m_fp_glSelectTextureCoordSet ||
		!m_fp_glMTexCoord2f || !m_fp_glMTexCoord2fv ||
		!m_fp_glMTexCoord3f || !m_fp_glMTexCoord3fv ||
		!m_fp_glMTexCoord4f || !m_fp_glMTexCoord4fv)
	{
		m_nMultiTexture = 1;
		m_fp_glMultiTexCoordPointer= NULL;
		m_fp_glSelectTexture = NULL;
		m_fp_glSelectTextureCoordSet = NULL;
		m_fp_glMTexCoord2f = NULL;
		m_fp_glMTexCoord2fv = NULL;
		m_fp_glMTexCoord3f = NULL;
		m_fp_glMTexCoord3fv = NULL;
		m_fp_glMTexCoord4f = NULL;
		m_fp_glMTexCoord4fv = NULL;
		return;
	}

	if(m_bLog) ConOutL("    Enabling GL_ARB_MULTITEXTURE...");
	m_Extensions |= CRCGL_EXT_MULTITEXTURE;
}

void CRenderContextGL::GetExtension_SecondaryColorEXT()
{
	if(m_bLog) LogFile("Checking extension GL_EXT_SECONDARY_COLOR...");
	if (m_GLExtensions.Find("GL_EXT_SECONDARY_COLOR") < 0) return;

	m_fp_glSecondaryColor3f = (PFNSECONDARYCOLOR3F) wglGetProcAddress("glSecondaryColor3fEXT");
	m_fp_glSecondaryColor3fv = (PFNSECONDARYCOLOR3FV) wglGetProcAddress("glSecondaryColor3fvEXT");
	m_fp_glSecondaryColorPointer = (PFNSECONDARYCOLORPOINTER) wglGetProcAddress("glSecondaryColorPointerEXT");

	if(m_bLog) LogFile(CStr("    glSecondaryColor3f             %.8x", m_fp_glSecondaryColor3f));
	if(m_bLog) LogFile(CStr("    glSecondaryColor3fv            %.8x", m_fp_glSecondaryColor3fv));
	if(m_bLog) LogFile(CStr("    glSecondaryColorPointer        %.8x", m_fp_glSecondaryColorPointer));

	if (m_fp_glSecondaryColor3f && m_fp_glSecondaryColorPointer)
	{
		m_Extensions |= CRCGL_EXT_SECONDARYCOLOR;
		if(m_bLog) ConOutL("    Enabling GL_EXT_SECONDARY_COLOR...");
	}
}

/*void CRenderContextGL::GetExtension_FogCoordEXT()
{
	if(m_bLog) LogFile("Checking extension GL_EXT_FOG_COORD...");
	if (m_GLExtensions.Find("GL_EXT_FOG_COORD") < 0) return;

	m_fp_glFogCoordf = (PFNFOGCOORDF) wglGetProcAddress("glFogCoordfEXT");
	m_fp_glFogCoordPointer = (PFNFOGCOORDPOINTER) wglGetProcAddress("glFogCoordPointerEXT");

	if(m_bLog) LogFile(CStr("    glFogCoordf                    %.8x", m_fp_glFogCoordf));
	if(m_bLog) LogFile(CStr("    glFogCoordPointer              %.8x", m_fp_glFogCoordPointer));

	if (m_fp_glFogCoordf && m_fp_glFogCoordPointer)
	{
		m_Extensions |= CRCGL_EXT_FOGCOORD;
		if(m_bLog) ConOutL("    Enabling GL_EXT_FOG_COORD...");
	}
}*/

void CRenderContextGL::GetExtension_TextureCompressionARB()
{
	if (m_GLExtensions.Find("GL_ARB_TEXTURE_COMPRESSION") < 0) return;
	if(m_bLog) LogFile("Checking extension GL_ARB_TEXTURE_COMPRESSION...");

	m_fp_glCompressedTexImage3D = (PFNCOMPRESSEDTEXIMAGE3DARB) wglGetProcAddress("glCompressedTexImage3DARB");
	m_fp_glCompressedTexImage2D = (PFNCOMPRESSEDTEXIMAGE2DARB) wglGetProcAddress("glCompressedTexImage2DARB");
	m_fp_glCompressedTexImage1D = (PFNCOMPRESSEDTEXIMAGE1DARB) wglGetProcAddress("glCompressedTexImage1DARB");
	m_fp_glCompressedTexSubImage3D = (PFNCOMPRESSEDTEXSUBIMAGE3DARB) wglGetProcAddress("glCompressedTexSubImage3DARB");
	m_fp_glCompressedTexSubImage2D = (PFNCOMPRESSEDTEXSUBIMAGE2DARB) wglGetProcAddress("glCompressedTexSubImage2DARB");
	m_fp_glCompressedTexSubImage1D = (PFNCOMPRESSEDTEXSUBIMAGE1DARB) wglGetProcAddress("glCompressedTexSubImage1DARB");
	m_fp_glGetCompressedTexImage = (PFNGETCOMPRESSEDTEXIMAGEARB) wglGetProcAddress("glGetCompressedTexImageARB");
	if(m_bLog) LogFile(CStr("    m_fp_glCompressedTexImage3D = %.8x", m_fp_glCompressedTexImage3D));
	if(m_bLog) LogFile(CStr("    m_fp_glCompressedTexImage2D = %.8x", m_fp_glCompressedTexImage2D));
	if(m_bLog) LogFile(CStr("    m_fp_glCompressedTexImage1D = %.8x", m_fp_glCompressedTexImage1D));
	if(m_bLog) LogFile(CStr("    m_fp_glCompressedTexSubImage3D = %.8x", m_fp_glCompressedTexSubImage3D));
	if(m_bLog) LogFile(CStr("    m_fp_glCompressedTexSubImage2D = %.8x", m_fp_glCompressedTexSubImage2D));
	if(m_bLog) LogFile(CStr("    m_fp_glCompressedTexSubImage1D = %.8x", m_fp_glCompressedTexSubImage1D));
	if(m_bLog) LogFile(CStr("    m_fp_glGetCompressedTexImage = %.8x", m_fp_glGetCompressedTexImage));

	if (m_fp_glCompressedTexImage3D &&
		m_fp_glCompressedTexImage2D &&
		m_fp_glCompressedTexImage1D &&
		m_fp_glCompressedTexSubImage3D &&
		m_fp_glCompressedTexSubImage2D &&
		m_fp_glCompressedTexSubImage1D &&
		m_fp_glGetCompressedTexImage)
	{
		if(m_bLog) ConOutL("    Enabling GL_ARB_TEXTURE_COMPRESSION...");
		m_Extensions |= CRCGL_EXT_TEXTURECOMPRESSION;
	}
}

void CRenderContextGL::GetExtension_DeviceGammaRamp()
{
	// Get SetDeviceGammaRamp function
	bool b3dfxGamma = false;

	m_fp_SetDeviceGammaRamp =
		(PFNSETDEVICEGAMMA) wglGetProcAddress("wglSetDeviceGammaRamp3DFX");
	if (!m_fp_SetDeviceGammaRamp) 
		m_fp_SetDeviceGammaRamp = (PFNSETDEVICEGAMMA) SetDeviceGammaRamp;
	else
		b3dfxGamma = true;
	if(m_bLog) LogFile(CStr("    m_fp_SetDeviceGammaRamp = %.8x", m_fp_SetDeviceGammaRamp));

	// Get GetDeviceGammaRamp function
	m_fp_GetDeviceGammaRamp =
		(PFNGETDEVICEGAMMA) wglGetProcAddress("wglGetDeviceGammaRamp3DFX");
	if (!m_fp_GetDeviceGammaRamp) m_fp_GetDeviceGammaRamp = (PFNGETDEVICEGAMMA) GetDeviceGammaRamp;
	if(m_bLog) LogFile(CStr("    m_fp_GetDeviceGammaRamp = %.8x", m_fp_GetDeviceGammaRamp));

	if (m_fp_SetDeviceGammaRamp && m_fp_GetDeviceGammaRamp)
	{
		m_Caps_Flags |= CRC_CAPS_FLAGS_GAMMARAMP;

		// Get current gamma ramp.
		HWND hWnd = (HWND)m_pDisplay->Win32_GethWnd();
		if (hWnd)
		{
			HDC hDC = ::GetDC(hWnd);
			m_fp_GetDeviceGammaRamp(hDC, m_OldGammaRamp);
			ReleaseDC(hWnd, hDC);
		}
		if(m_bLog)
		{
			if (b3dfxGamma)
				ConOutL("    Enabling 3Dfx DeviceGammaRamp...");
			else
				ConOutL("    Enabling Win32 DeviceGammaRamp...");
		}
	}
}

void CRenderContextGL::VSync_Update()
{
	if (m_fp_SwapInterval)
	{
		int Interval = (m_VSync) ? 1 : 0;
		m_fp_SwapInterval(Interval);
	}
}

void CRenderContextGL::GetExtension_SwapControl()
{
	m_fp_SwapInterval =
		(PFNSWAPINTERVAL) wglGetProcAddress("wglSwapIntervalEXT");
	m_fp_GetSwapInterval =
		(PFNGETSWAPINTERVAL) wglGetProcAddress("wglGetSwapIntervalEXT");

	if (m_fp_SwapInterval)
	{
		if(m_bLog) ConOutL(CStr("    Enabling WGL_SWAP_CONTROL (VSync %s)", (m_VSync) ? "On" : "Off"));
		VSync_Update();
	}

}


void CRenderContextGL::GetExtension_RegisterCombinersNV()
{
	if(m_bLog) LogFile("Checking extension GL_NV_REGISTER_COMBINERS...");
	if (m_GLExtensions.Find("GL_NV_REGISTER_COMBINERS") < 0) return;

	m_fp_glCombinerParameterfvNV =			(PFNCOMBINERPARAMETERFVNV) wglGetProcAddress("glCombinerParameterfvNV");
	m_fp_glCombinerParameterivNV =			(PFNCOMBINERPARAMETERIVNV) wglGetProcAddress("glCombinerParameterivNV");
	m_fp_glCombinerParameterfNV =			(PFNCOMBINERPARAMETERFNV) wglGetProcAddress("glCombinerParameterfNV");
	m_fp_glCombinerParameteriNV =			(PFNCOMBINERPARAMETERINV) wglGetProcAddress("glCombinerParameteriNV");
	m_fp_glCombinerInputNV =				(PFNCOMBINERINPUTNV) wglGetProcAddress("glCombinerInputNV");
	m_fp_glCombinerOutputNV =				(PFNCOMBINEROUTPUTNV) wglGetProcAddress("glCombinerOutputNV");
	m_fp_glFinalCombinerInputNV =			(PFNFINALCOMBINERINPUTNV) wglGetProcAddress("glFinalCombinerInputNV");
	m_fp_glGetCombinerInputParameterfvNV =	(PFNGETCOMBINERINPUTPARAMETERFVNV) wglGetProcAddress("glGetCombinerInputParameterfvNV");
	m_fp_glGetCombinerInputParameterfiNV =	(PFNGETCOMBINERINPUTPARAMETERIVNV) wglGetProcAddress("glGetCombinerInputParameterivNV");
	m_fp_glGetCombinerOutputParameterfvNV =	(PFNGETCOMBINEROUTPUTPARAMETERFVNV) wglGetProcAddress("glGetCombinerOutputParameterfvNV");
	m_fp_glGetCombinerOutputParameterfiNV =	(PFNGETCOMBINEROUTPUTPARAMETERIVNV) wglGetProcAddress("glGetCombinerOutputParameterivNV");
	m_fp_glGetFinalCombinerInputParameterfvNV = (PFNGETFINALCOMBINERINPUTPARAMETERFVNV) wglGetProcAddress("glGetCombinerInputParameterfvNV");
	m_fp_glGetFinalCombinerInputParameterfiNV = (PFNGETFINALCOMBINERINPUTPARAMETERIVNV) wglGetProcAddress("glGetCombinerInputParameterivNV");


	if(m_bLog) LogFile(CStr("    m_fp_glCombinerParameterfvNV = %.8x", m_fp_glCombinerParameterfvNV));
	if(m_bLog) LogFile(CStr("    m_fp_glCombinerParameterivNV = %.8x", m_fp_glCombinerParameterivNV));
	if(m_bLog) LogFile(CStr("    m_fp_glCombinerParameterfNV = %.8x", m_fp_glCombinerParameterfNV));
	if(m_bLog) LogFile(CStr("    m_fp_glCombinerParameteriNV = %.8x", m_fp_glCombinerParameteriNV));
	if(m_bLog) LogFile(CStr("    m_fp_glCombinerInputNV = %.8x", m_fp_glCombinerInputNV));
	if(m_bLog) LogFile(CStr("    m_fp_glCombinerOutputNV = %.8x", m_fp_glCombinerOutputNV));
	if(m_bLog) LogFile(CStr("    m_fp_glFinalCombinerInputNV = %.8x", m_fp_glFinalCombinerInputNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetCombinerInputParameterfvNV = %.8x", m_fp_glGetCombinerInputParameterfvNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetCombinerInputParameterfiNV = %.8x", m_fp_glGetCombinerInputParameterfiNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetCombinerInputParameterfvNV = %.8x", m_fp_glGetCombinerOutputParameterfvNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetCombinerInputParameterfiNV = %.8x", m_fp_glGetCombinerOutputParameterfiNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetFinalCombinerInputParameterfvNV = %.8x", m_fp_glGetFinalCombinerInputParameterfvNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetFinalCombinerInputParameterfiNV = %.8x", m_fp_glGetFinalCombinerInputParameterfiNV));

	GLint MaxCombiners;
	glnGetIntegerv((GLenum)GL_MAX_GENERAL_COMBINERS_NV, &MaxCombiners);
	if(m_bLog) LogFile(CStr("    GL_MAX_GENERAL_COMBINERS_NV = %d", MaxCombiners));

	if (m_fp_glCombinerParameterfvNV &&
		m_fp_glCombinerParameterivNV &&
		m_fp_glCombinerParameterfNV &&
		m_fp_glCombinerParameteriNV &&
		m_fp_glCombinerInputNV &&
		m_fp_glCombinerOutputNV &&
		m_fp_glFinalCombinerInputNV &&
		m_fp_glGetCombinerInputParameterfvNV &&
		m_fp_glGetCombinerInputParameterfiNV &&
		m_fp_glGetCombinerOutputParameterfvNV &&
		m_fp_glGetCombinerOutputParameterfiNV &&
		m_fp_glGetFinalCombinerInputParameterfvNV &&
		m_fp_glGetFinalCombinerInputParameterfiNV &&
		MaxCombiners >= 2)
	{
		m_RegCombiners_MaxCombiners = MaxCombiners;

		if(m_bLog) ConOutL("    Enabling GL_NV_REGISTER_COMBINERS...");
		m_Extensions |= CRCGL_EXT_NV_REGCOMBINERS;
	}
}

void CRenderContextGL::GetExtension_RegisterCombinersNV2()
{
	if(m_bLog) LogFile("Checking extension GL_NV_REGISTER_COMBINERS2...");
	if (m_GLExtensions.Find("GL_NV_REGISTER_COMBINERS2") < 0) return;

	m_fp_glCombinerStageParameterfvNV =			(PFNCOMBINERSTAGEPARAMETERFVNV) wglGetProcAddress("glCombinerStageParameterfvNV");
	m_fp_glGetCombinerStageParameterfvNV =			(PFNGETCOMBINERSTAGEPARAMETERFVNV) wglGetProcAddress("glGetCombinerStageParameterfvNV");

	if(m_bLog) LogFile(CStr("    m_fp_glCombinerStageParameterfvNV = %.8x", m_fp_glCombinerParameterfvNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetCombinerStageParameterfvNV = %.8x", m_fp_glCombinerParameterivNV));

	if (m_fp_glCombinerStageParameterfvNV &&
		m_fp_glGetCombinerStageParameterfvNV)
	{
		if(m_bLog) ConOutL("    Enabling GL_NV_REGISTER_COMBINERS2...");
		m_Extensions |= CRCGL_EXT_NV_REGCOMBINERS2;
	}
}

void CRenderContextGL::GetExtension_TextureShaderNV()
{
	if (m_bLog) LogFile("Checking extension GL_NV_TEXTURE_SHADER...");
	if (m_GLExtensions.Find("GL_NV_TEXTURE_SHADER") < 0) return;
	if(m_bLog) ConOutL("    Enabling GL_NV_TEXTURE_SHADER...");
	m_Extensions |= CRCGL_EXT_NV_TEXTURESHADER;
}

void CRenderContextGL::GetExtension_VertexProgramNV()
{
	if (m_bLog) LogFile("Checking extension GL_NV_VERTEX_PROGRAM...");
	if (m_GLExtensions.Find("GL_NV_VERTEX_PROGRAM") < 0) return;

	m_fp_glAreProgramsResidentNV =			(PFNGLAREPROGRAMSRESIDENTNVPROC)	wglGetProcAddress("glAreProgramsResidentNV");
	m_fp_glBindProgramNV =					(PFNGLBINDPROGRAMNVPROC)			wglGetProcAddress("glBindProgramNV");
	m_fp_glDeleteProgramsNV =				(PFNGLDELETEPROGRAMSNVPROC)			wglGetProcAddress("glDeleteProgramsNV");
	m_fp_glExecuteProgramNV =				(PFNGLEXECUTEPROGRAMNVPROC)			wglGetProcAddress("glExecuteProgramNV");
	m_fp_glGenProgramsNV =					(PFNGLGENPROGRAMSNVPROC)			wglGetProcAddress("glGenProgramsNV");
	m_fp_glGetProgramParameterdvNV =		(PFNGLGETPROGRAMPARAMETERDVNVPROC)	wglGetProcAddress("glGetProgramParameterdvNV");
	m_fp_glGetProgramParameterfvNV =		(PFNGLGETPROGRAMPARAMETERFVNVPROC)	wglGetProcAddress("glGetProgramParameterfvNV");
	m_fp_glGetProgramivNV =				(PFNGLGETPROGRAMIVNVPROC)			wglGetProcAddress("glGetProgramivNV");
	m_fp_glGetProgramStringNV =			(PFNGLGETPROGRAMSTRINGNVPROC)		wglGetProcAddress("glGetProgramStringNV");
	m_fp_glGetTrackMatrixivNV =			(PFNGLGETTRACKMATRIXIVNVPROC)		wglGetProcAddress("glGetTrackMatrixivNV");
	m_fp_glGetVertexAttribdvNV =			(PFNGLGETVERTEXATTRIBDVNVPROC)		wglGetProcAddress("glGetVertexAttribdvNV");
	m_fp_glGetVertexAttribfvNV =			(PFNGLGETVERTEXATTRIBFVNVPROC)		wglGetProcAddress("glGetVertexAttribfvNV");
	m_fp_glGetVertexAttribivNV =			(PFNGLGETVERTEXATTRIBIVNVPROC)		wglGetProcAddress("glGetVertexAttribivNV");
	m_fp_glGetVertexAttribPointervNV =		(PFNGLGETVERTEXATTRIBPOINTERVNVPROC)wglGetProcAddress("glGetVertexAttribPointervNV");
	m_fp_glIsProgramNV =					(PFNGLISPROGRAMNVPROC)				wglGetProcAddress("glIsProgramNV");
	m_fp_glLoadProgramNV =					(PFNGLLOADPROGRAMNVPROC)			wglGetProcAddress("glLoadProgramNV");
	m_fp_glProgramParameter4dNV =			(PFNGLPROGRAMPARAMETER4DNVPROC)		wglGetProcAddress("glProgramParameter4dNV");
	m_fp_glProgramParameter4dvNV =			(PFNGLPROGRAMPARAMETER4DVNVPROC	)	wglGetProcAddress("glProgramParameter4dvNV");
	m_fp_glProgramParameter4fNV =			(PFNGLPROGRAMPARAMETER4FNVPROC)		wglGetProcAddress("glProgramParameter4fNV");
	m_fp_glProgramParameter4fvNV =			(PFNGLPROGRAMPARAMETER4FVNVPROC)	wglGetProcAddress("glProgramParameter4fvNV");
	m_fp_glProgramParameters4dvNV =		(PFNGLPROGRAMPARAMETERS4DVNVPROC)	wglGetProcAddress("glProgramParameters4dvNV");
	m_fp_glProgramParameters4fvNV =		(PFNGLPROGRAMPARAMETERS4FVNVPROC)	wglGetProcAddress("glProgramParameters4fvNV");
	m_fp_glRequestResidentProgramsNV =		(PFNGLREQUESTRESIDENTPROGRAMSNVPROC)wglGetProcAddress("glRequestResidentProgramsNV");
	m_fp_glTrackMatrixNV =					(PFNGLTRACKMATRIXNVPROC)			wglGetProcAddress("glTrackMatrixNV");
	m_fp_glVertexAttribPointerNV =			(PFNGLVERTEXATTRIBPOINTERNVPROC)	wglGetProcAddress("glVertexAttribPointerNV");

	if(m_bLog) LogFile(CStr("    m_fp_glAreProgramsResidentNV = %.8x", m_fp_glAreProgramsResidentNV));
	if(m_bLog) LogFile(CStr("    m_fp_glBindProgramNV = %.8x", m_fp_glBindProgramNV));
	if(m_bLog) LogFile(CStr("    m_fp_glDeleteProgramsNV = %.8x", m_fp_glDeleteProgramsNV));
	if(m_bLog) LogFile(CStr("    m_fp_glExecuteProgramNV = %.8x", m_fp_glExecuteProgramNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGenProgramsNV = %.8x", m_fp_glGenProgramsNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetProgramParameterdvNV = %.8x", m_fp_glGetProgramParameterdvNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetProgramParameterfvNV = %.8x", m_fp_glGetProgramParameterfvNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetProgramivNV = %.8x", m_fp_glGetProgramivNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetProgramStringNV = %.8x", m_fp_glGetProgramStringNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetTrackMatrixivNV = %.8x", m_fp_glGetTrackMatrixivNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetVertexAttribdvNV = %.8x", m_fp_glGetVertexAttribdvNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetVertexAttribfvNV = %.8x", m_fp_glGetVertexAttribfvNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetVertexAttribivNV = %.8x", m_fp_glGetVertexAttribivNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetVertexAttribPointervNV = %.8x", m_fp_glGetVertexAttribPointervNV));
	if(m_bLog) LogFile(CStr("    m_fp_glIsProgramNV = %.8x", m_fp_glIsProgramNV));
	if(m_bLog) LogFile(CStr("    m_fp_glLoadProgramNV = %.8x", m_fp_glLoadProgramNV));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramParameter4dNV = %.8x", m_fp_glProgramParameter4dNV));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramParameter4dvNV = %.8x", m_fp_glProgramParameter4dvNV));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramParameter4fNV = %.8x", m_fp_glProgramParameter4fNV));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramParameter4fvNV = %.8x", m_fp_glProgramParameter4fvNV));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramParameters4dvNV = %.8x", m_fp_glProgramParameters4dvNV));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramParameters4fvNV = %.8x", m_fp_glProgramParameters4fvNV));
	if(m_bLog) LogFile(CStr("    m_fp_glRequestResidentProgramsNV = %.8x", m_fp_glRequestResidentProgramsNV));
	if(m_bLog) LogFile(CStr("    m_fp_glTrackMatrixNV = %.8x", m_fp_glTrackMatrixNV));
	if(m_bLog) LogFile(CStr("    m_fp_glVertexAttribPointerNV = %.8x", m_fp_glVertexAttribPointerNV));

	if (m_fp_glAreProgramsResidentNV &&
		m_fp_glBindProgramNV &&
		m_fp_glDeleteProgramsNV &&
		m_fp_glExecuteProgramNV &&
		m_fp_glGenProgramsNV &&
		m_fp_glGetProgramParameterdvNV &&
		m_fp_glGetProgramParameterfvNV &&
		m_fp_glGetProgramivNV &&
		m_fp_glGetProgramStringNV &&
		m_fp_glGetTrackMatrixivNV &&
		m_fp_glGetVertexAttribdvNV &&
		m_fp_glGetVertexAttribfvNV &&
		m_fp_glGetVertexAttribivNV &&
//		m_fp_glGetVertexAttribPointervNV &&	// 11.01 driver bug
		m_fp_glIsProgramNV &&
		m_fp_glLoadProgramNV &&
		m_fp_glProgramParameter4dNV &&
		m_fp_glProgramParameter4dvNV &&
		m_fp_glProgramParameter4fNV &&
		m_fp_glProgramParameter4fvNV &&
		m_fp_glProgramParameters4dvNV &&
		m_fp_glProgramParameters4fvNV &&
		m_fp_glRequestResidentProgramsNV &&
		m_fp_glTrackMatrixNV &&
		m_fp_glVertexAttribPointerNV)
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (!pSys) Error("GetExtension_VertexProgramNV", "No system.");
		
		if (CDiskUtil::FileExists(m_pDisplay->m_DataPath + "VP.xrg") &&
			CDiskUtil::FileExists(m_pDisplay->m_DataPath + "VPDefines.xrg"))
		{
			if(m_bLog) ConOutL("    Enabling GL_NV_VERTEX_PROGRAM...");
			m_Extensions |= CRCGL_EXT_NV_VERTEXPROGRAM;
			
			m_VP_ProgramGenerator.Create(m_pDisplay->m_DataPath + "VP.xrg", m_pDisplay->m_DataPath + "VPDefines.xrg", false, CRC_MAXTEXCOORDS);

	//		Just code to debug the VP generator.

	/*		CRC_VPFormat Format;
			Format.m_iConstant_Lights = 4;
			Format.m_iConstant_MP = 16;
			Format.SetMWComp(3);
			CRC_Light Light;
			Format.SetLights(&Light, 1);
			Format.SetTexMatrix(2);

			CStr VP = m_VP_ProgramGenerator.CreateVP(Format);

			m_ExtensionsActive |= CRCGL_EXT_NV_VERTEXPROGRAM;
			VP_Load(VP, "HIRR", 1);
	*/

	//		VP_Bind(Format);

	//		LogFile(VP);

		}
		else
		{
			ConOutL("    WARNING: No vertex-program found.");
		}
	}
}

void CRenderContextGL::GetExtension_OcclusionQueryNV()
{
	if(m_bLog) LogFile("Checking extension GL_NV_OCCLUSION_QUERY...");
	if (m_GLExtensions.Find("GL_NV_OCCLUSION_QUERY") < 0) return;

	m_fp_glGenOcclusionQueriesNV =			(PFNGENOCCLUSIONQUERIESNV) wglGetProcAddress("glGenOcclusionQueriesNV");
	m_fp_glDeleteOcclusionQueriesNV =		(PFNDELETEOCCLUSIONQUERIESNV) wglGetProcAddress("glDeleteOcclusionQueriesNV");
	m_fp_glIsOcclusionQueryNV =			(PFNISOCCLUSIONQUERYNV) wglGetProcAddress("glIsOcclusionQueryNV");
	m_fp_glBeginOcclusionQueryNV =			(PFNBEGINOCCLUSIONQUERYNV) wglGetProcAddress("glBeginOcclusionQueryNV");
	m_fp_glEndOcclusionQueryNV =			(PFNENDOCCLUSIONQUERYNV) wglGetProcAddress("glEndOcclusionQueryNV");
	m_fp_glGetOcclusionQueryivNV =			(PFNGETOCCLUSIONQUERYIVNV) wglGetProcAddress("glGetOcclusionQueryivNV");
	m_fp_glGetOcclusionQueryuivNV =		(PFNGETOCCLUSIONQUERYUIVNV) wglGetProcAddress("glGetOcclusionQueryuivNV");

	if(m_bLog) LogFile(CStr("    m_fp_glGenOcclusionQueriesNV = %.8x", m_fp_glGenOcclusionQueriesNV));
	if(m_bLog) LogFile(CStr("    m_fp_glDeleteOcclusionQueriesNV = %.8x", m_fp_glDeleteOcclusionQueriesNV));
	if(m_bLog) LogFile(CStr("    m_fp_glIsOcclusionQueryNV = %.8x", m_fp_glIsOcclusionQueryNV));
	if(m_bLog) LogFile(CStr("    m_fp_glBeginOcclusionQueryNV = %.8x", m_fp_glBeginOcclusionQueryNV));
	if(m_bLog) LogFile(CStr("    m_fp_glEndOcclusionQueryNV = %.8x", m_fp_glEndOcclusionQueryNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetOcclusionQueryivNV = %.8x", m_fp_glGetOcclusionQueryivNV));
	if(m_bLog) LogFile(CStr("    m_fp_glGetOcclusionQueryuivNV = %.8x", m_fp_glGetOcclusionQueryuivNV));

	if (m_fp_glGenOcclusionQueriesNV &&
		m_fp_glDeleteOcclusionQueriesNV &&
		m_fp_glIsOcclusionQueryNV &&
		m_fp_glBeginOcclusionQueryNV &&
		m_fp_glEndOcclusionQueryNV &&
		m_fp_glGetOcclusionQueryivNV &&
		m_fp_glGetOcclusionQueryuivNV)
	{
		if(m_bLog) ConOutL("    Enabling GL_NV_OCCLUSION_QUERY...");
		OcclusionQuery_Init();
		m_Extensions |= CRCGL_EXT_NV_OCCLUSIONQUERY;
	}
}

void CRenderContextGL::GetExtension_TextureCubeMapARB()
{
	if(m_bLog) LogFile("Checking extension GL_ARB_TEXTURE_CUBE_MAP...");
	if (m_GLExtensions.Find("GL_ARB_TEXTURE_CUBE_MAP") < 0) return;

	glnGetIntegerv((GLenum)GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &m_MaxCubeTextureSize);
	m_MaxCubeTextureSizeReal = Max(0, Min(2048, m_MaxCubeTextureSize));
	if(m_bLog) LogFile(CStr("    GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB = %d", m_MaxCubeTextureSize));

	if (m_MaxCubeTextureSize >= 512)
	{
		m_Extensions |= CRCGL_EXT_CUBEMAP;
		if(m_bLog) ConOutL("    Enabling GL_ARB_TEXTURE_CUBE_MAP...");
	}
}

void CRenderContextGL::GetExtension_TextureCompressionS3TC()
{
	if(m_bLog) LogFile("Checking extension GL_EXT_TEXTURE_COMPRESSION_S3TC...");
	if (m_GLExtensions.Find("GL_EXT_TEXTURE_COMPRESSION_S3TC") < 0) return;

	m_Extensions |= CRCGL_EXT_S3TC;
	if(m_bLog) ConOutL("    Enabling GL_EXT_TEXTURE_COMPRESSION_S3TC...");
}

void CRenderContextGL::GetExtension_TextureCompression3DC()
{
	if(m_bLog) LogFile("Checking extension GL_ATI_TEXTURE_COMPRESSION_3DC...");
	if (m_GLExtensions.Find("GL_ATI_TEXTURE_COMPRESSION_3DC") < 0) return;

	m_Extensions |= CRCGL_EXT_TEXTURE_COMPRESSION_3DC;
	if(m_bLog) ConOutL("    Using GL_ATI_TEXTURE_COMPRESSION_3DC...");
}

void CRenderContextGL::GetExtension_TextureFilterAnisotropic()
{
	if(m_bLog) LogFile("Checking extension GL_EXT_TEXTURE_FILTER_ANISOTROPIC...");
	if (m_GLExtensions.Find("GL_EXT_TEXTURE_FILTER_ANISOTROPIC") < 0) return;

	m_Extensions |= CRCGL_EXT_ANISOTROPIC;

	m_MaxAnisotropy = 2.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &m_MaxAnisotropy);

	if(m_bLog) ConOutL("    Enabling GL_EXT_TEXTURE_FILTER_ANISOTROPIC...");
}

void CRenderContextGL::GetExtension_TextureLODBias()
{
	if(m_bLog) LogFile("Checking extension GL_EXT_TEXTURE_LOD_BIAS...");
	if (m_GLExtensions.Find("GL_EXT_TEXTURE_LOD_BIAS") < 0) return;

	m_Extensions |= CRCGL_EXT_TEXTURE_LOD_BIAS;

	if(m_bLog) ConOutL("    Enabling GL_EXT_TEXTURE_LOD_BIAS...");
}

void CRenderContextGL::GetExtension_VertexProgramARB()
{
	if (m_bLog) LogFile("Checking extension GL_ARB_VERTEX_PROGRAM...");
	if (m_GLExtensions.Find("GL_ARB_VERTEX_PROGRAM") < 0) return;

	m_fp_glBindProgramARB =				(PFNBINDPROGRAMARB)					wglGetProcAddress("glBindProgramARB");
	m_fp_glDeleteProgramsARB =				(PFNDELETEPROGRAMSARB)				wglGetProcAddress("glDeleteProgramsARB");
	m_fp_glGenProgramsARB =				(PFNGENPROGRAMSARB)					wglGetProcAddress("glGenProgramsARB");
	m_fp_glGetProgramEnvParameterdvARB =	(PFNGETPROGRAMENVPARAMETERDVARB)	wglGetProcAddress("glGetProgramEnvParameterdvARB");
	m_fp_glGetProgramEnvParameterfvARB =	(PFNGETPROGRAMENVPARAMETERFVARB)	wglGetProcAddress("glGetProgramEnvParameterfvARB");
	m_fp_glGetProgramivARB =				(PFNGETPROGRAMIVARB)				wglGetProcAddress("glGetProgramivARB");
	m_fp_glGetProgramStringARB =			(PFNGETPROGRAMSTRINGARB)			wglGetProcAddress("glGetProgramStringARB");
	m_fp_glGetVertexAttribdvARB =			(PFNGETVERTEXATTRIBDVARB)			wglGetProcAddress("glGetVertexAttribdvARB");
	m_fp_glGetVertexAttribfvARB =			(PFNGETVERTEXATTRIBFVARB)			wglGetProcAddress("glGetVertexAttribfvARB");
	m_fp_glGetVertexAttribivARB =			(PFNGETVERTEXATTRIBIVARB)			wglGetProcAddress("glGetVertexAttribivARB");
	m_fp_glGetVertexAttribPointervARB =	(PFNGETVERTEXATTRIBPOINTERVARB)		wglGetProcAddress("glGetVertexAttribPointervARB");
	m_fp_glIsProgramARB =					(PFNISPROGRAMARB)					wglGetProcAddress("glIsProgramARB");
	m_fp_glProgramStringARB =				(PFNPROGRAMSTRINGARB)				wglGetProcAddress("glProgramStringARB");
	m_fp_glProgramEnvParameter4dARB =		(PFNPROGRAMENVPARAMETER4DARB)		wglGetProcAddress("glProgramEnvParameter4dARB");
	m_fp_glProgramEnvParameter4dvARB =		(PFNPROGRAMENVPARAMETER4DVARB)		wglGetProcAddress("glProgramEnvParameter4dvARB");
	m_fp_glProgramEnvParameter4fARB =		(PFNPROGRAMENVPARAMETER4FARB)		wglGetProcAddress("glProgramEnvParameter4fARB");
	m_fp_glProgramEnvParameter4fvARB =		(PFNPROGRAMENVPARAMETER4FVARB)		wglGetProcAddress("glProgramEnvParameter4fvARB");
	m_fp_glVertexAttribPointerARB =		(PFNVERTEXATTRIBPOINTERARB)			wglGetProcAddress("glVertexAttribPointerARB");
	m_fp_glEnableVertexAttribArrayARB =	(PFNENABLEVERTEXATTRIBARRAYARB)		wglGetProcAddress("glEnableVertexAttribArrayARB");
	m_fp_glDisableVertexAttribArrayARB =	(PFNDISABLEVERTEXATTRIBARRAYARB)	wglGetProcAddress("glDisableVertexAttribArrayARB");

	m_VP_nMaxProgramEnvParameters;
	m_fp_glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &m_VP_nMaxProgramEnvParameters);

	if(m_bLog) LogFile(CStr("    m_fp_glBindProgramARB = %.8x", m_fp_glBindProgramARB));
	if(m_bLog) LogFile(CStr("    m_fp_glDeleteProgramsARB = %.8x", m_fp_glDeleteProgramsARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGenProgramsARB = %.8x", m_fp_glGenProgramsARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetProgramEnvParameterdvARB = %.8x", m_fp_glGetProgramEnvParameterdvARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetProgramEnvParameterfvARB = %.8x", m_fp_glGetProgramEnvParameterfvARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetProgramivARB = %.8x", m_fp_glGetProgramivARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetProgramStringARB = %.8x", m_fp_glGetProgramStringARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetVertexAttribdvARB = %.8x", m_fp_glGetVertexAttribdvARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetVertexAttribfvARB = %.8x", m_fp_glGetVertexAttribfvARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetVertexAttribivARB = %.8x", m_fp_glGetVertexAttribivARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetVertexAttribPointervARB = %.8x", m_fp_glGetVertexAttribPointervARB));
	if(m_bLog) LogFile(CStr("    m_fp_glIsProgramARB = %.8x", m_fp_glIsProgramARB));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramStringARB = %.8x", m_fp_glProgramStringARB));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramEnvParameter4dARB = %.8x", m_fp_glProgramEnvParameter4dARB));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramEnvParameter4dvARB = %.8x", m_fp_glProgramEnvParameter4dvARB));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramEnvParameter4fARB = %.8x", m_fp_glProgramEnvParameter4fARB));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramEnvParameter4fvARB = %.8x", m_fp_glProgramEnvParameter4fvARB));
	if(m_bLog) LogFile(CStr("    m_fp_glVertexAttribPointerARB = %.8x", m_fp_glVertexAttribPointerARB));
	if(m_bLog) LogFile(CStr("    m_fp_glEnableVertexAttribArrayARB = %.8x", m_fp_glEnableVertexAttribArrayARB));
	if(m_bLog) LogFile(CStr("    m_fp_glDisableVertexAttribArrayARB = %.8x", m_fp_glDisableVertexAttribArrayARB));
	if(m_bLog) LogFile(CStr("    MaxProgramEnvParameters = %d", m_VP_nMaxProgramEnvParameters));

	if (m_fp_glBindProgramARB &&
		m_fp_glDeleteProgramsARB &&
		m_fp_glGenProgramsARB &&
		m_fp_glGetProgramEnvParameterdvARB &&
		m_fp_glGetProgramEnvParameterfvARB &&
		m_fp_glGetProgramivARB &&
		m_fp_glGetProgramStringARB &&
		m_fp_glGetVertexAttribdvARB &&
		m_fp_glGetVertexAttribfvARB &&
		m_fp_glGetVertexAttribivARB &&
		m_fp_glGetVertexAttribPointervARB &&
		m_fp_glIsProgramARB &&
		m_fp_glProgramStringARB &&
		m_fp_glProgramEnvParameter4dARB &&
		m_fp_glProgramEnvParameter4dvARB &&
		m_fp_glProgramEnvParameter4fARB &&
		m_fp_glProgramEnvParameter4fvARB &&
		m_fp_glVertexAttribPointerARB &&
		m_fp_glEnableVertexAttribArrayARB &&
		m_fp_glDisableVertexAttribArrayARB &&
		(m_VP_nMaxProgramEnvParameters >= 96))
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (!pSys) Error("GetExtension_VertexProgramARB", "No system.");
		
		if (CDiskUtil::FileExists(m_pDisplay->m_DataPath + "VP.xrg") &&
			CDiskUtil::FileExists(m_pDisplay->m_DataPath + "VPDefines_ARB.xrg"))
		{
			if(m_bLog) ConOutL("    Enabling GL_ARB_VERTEX_PROGRAM...");
			m_Extensions |= CRCGL_EXT_VERTEXPROGRAM;
			
			m_VP_ProgramGenerator.Create(m_pDisplay->m_DataPath + "VP.xrg", m_pDisplay->m_DataPath + "VPDefines_ARB.xrg", false, CRC_MAXTEXCOORDS);

	//		Just code to debug the VP generator.

	/*		CRC_VPFormat Format;
			Format.m_iConstant_Lights = 4;
			Format.m_iConstant_MP = 16;
			Format.SetMWComp(3);
			CRC_Light Light;
			Format.SetLights(&Light, 1);
			Format.SetTexMatrix(2);

			CStr VP = m_VP_ProgramGenerator.CreateVP(Format);

			m_ExtensionsActive |= CRCGL_EXT_NV_VERTEXPROGRAM;
			VP_Load(VP, "HIRR", 1);
	*/

	//		VP_Bind(Format);

	//		LogFile(VP);


		}
		else
		{
			ConOutL("    WARNING: No vertex-program found.");
		}
	}
}

void CRenderContextGL::GetExtension_FragmentProgramARB()
{
	if (m_bLog) LogFile("Checking extension GL_ARB_FRAGMENT_PROGRAM...");
	if (m_GLExtensions.Find("GL_ARB_FRAGMENT_PROGRAM") < 0) return;

	m_fp_glBindProgramARB =				(PFNBINDPROGRAMARB)					wglGetProcAddress("glBindProgramARB");
	m_fp_glDeleteProgramsARB =				(PFNDELETEPROGRAMSARB)				wglGetProcAddress("glDeleteProgramsARB");
	m_fp_glGenProgramsARB =				(PFNGENPROGRAMSARB)					wglGetProcAddress("glGenProgramsARB");
	m_fp_glGetProgramEnvParameterdvARB =	(PFNGETPROGRAMENVPARAMETERDVARB)	wglGetProcAddress("glGetProgramEnvParameterdvARB");
	m_fp_glGetProgramEnvParameterfvARB =	(PFNGETPROGRAMENVPARAMETERFVARB)	wglGetProcAddress("glGetProgramEnvParameterfvARB");
	m_fp_glGetProgramivARB =				(PFNGETPROGRAMIVARB)				wglGetProcAddress("glGetProgramivARB");
	m_fp_glGetProgramStringARB =			(PFNGETPROGRAMSTRINGARB)			wglGetProcAddress("glGetProgramStringARB");
	m_fp_glGetVertexAttribdvARB =			(PFNGETVERTEXATTRIBDVARB)			wglGetProcAddress("glGetVertexAttribdvARB");
	m_fp_glGetVertexAttribfvARB =			(PFNGETVERTEXATTRIBFVARB)			wglGetProcAddress("glGetVertexAttribfvARB");
	m_fp_glGetVertexAttribivARB =			(PFNGETVERTEXATTRIBIVARB)			wglGetProcAddress("glGetVertexAttribivARB");
	m_fp_glGetVertexAttribPointervARB =	(PFNGETVERTEXATTRIBPOINTERVARB)		wglGetProcAddress("glGetVertexAttribPointervARB");
	m_fp_glIsProgramARB =					(PFNISPROGRAMARB)					wglGetProcAddress("glIsProgramARB");
	m_fp_glProgramStringARB =				(PFNPROGRAMSTRINGARB)				wglGetProcAddress("glProgramStringARB");
	m_fp_glProgramEnvParameter4dARB =		(PFNPROGRAMENVPARAMETER4DARB)		wglGetProcAddress("glProgramEnvParameter4dARB");
	m_fp_glProgramEnvParameter4dvARB =		(PFNPROGRAMENVPARAMETER4DVARB)		wglGetProcAddress("glProgramEnvParameter4dvARB");
	m_fp_glProgramEnvParameter4fARB =		(PFNPROGRAMENVPARAMETER4FARB)		wglGetProcAddress("glProgramEnvParameter4fARB");
	m_fp_glProgramEnvParameter4fvARB =		(PFNPROGRAMENVPARAMETER4FVARB)		wglGetProcAddress("glProgramEnvParameter4fvARB");
	m_fp_glVertexAttribPointerARB =		(PFNVERTEXATTRIBPOINTERARB)			wglGetProcAddress("glVertexAttribPointerARB");
	m_fp_glEnableVertexAttribArrayARB =	(PFNENABLEVERTEXATTRIBARRAYARB)		wglGetProcAddress("glEnableVertexAttribArrayARB");
	m_fp_glDisableVertexAttribArrayARB =	(PFNDISABLEVERTEXATTRIBARRAYARB)	wglGetProcAddress("glDisableVertexAttribArrayARB");

	int nMax_ShaderTextures	= 0;
	int nMax_ShaderTexcoords	= 0;
	{
		glnGetIntegerv((GLenum)GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &nMax_ShaderTextures);
		if (glnGetError() != GL_NO_ERROR) 
			nMax_ShaderTextures = 0;
		glnGetIntegerv((GLenum)GL_MAX_TEXTURE_COORDS_ARB, &nMax_ShaderTexcoords);
		if (glnGetError() != GL_NO_ERROR) 
			nMax_ShaderTexcoords = 0;
	}

	if (!m_fp_glGetProgramivARB)
		return;

	int nMaxProgramInstructions;
	int nMaxProgramNativeInstructions;
	int nMaxProgramALUInstructions;
	int nMaxProgramTEXInstructions;
	int nMaxProgramTEXIndirections;
	int nMaxProgramNativeALUInstructions;
	int nMaxProgramNativeTEXInstructions;
	int nMaxProgramNativeTEXIndirections;
	int nMaxProgramNativeTemporaries;
	int nMaxProgramNativeParameters;
	int nMaxProgramNativeAttribs;
	int nMaxProgramLocalParameters;
	int nMaxProgramEnvParameters;

	m_fp_glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_INSTRUCTIONS_ARB, &nMaxProgramInstructions);
	m_fp_glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &nMaxProgramNativeInstructions);
	m_fp_glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB, &nMaxProgramALUInstructions);
	m_fp_glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB, &nMaxProgramTEXInstructions);
	m_fp_glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB, &nMaxProgramTEXIndirections);
	m_fp_glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB, &nMaxProgramNativeALUInstructions);
	m_fp_glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB, &nMaxProgramNativeTEXInstructions);
	m_fp_glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB, &nMaxProgramNativeTEXIndirections);
	m_fp_glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, &nMaxProgramNativeTemporaries);
	m_fp_glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, &nMaxProgramNativeParameters);
	m_fp_glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB, &nMaxProgramNativeAttribs);
	m_fp_glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB, &nMaxProgramLocalParameters);
	m_fp_glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &nMaxProgramEnvParameters);

	m_nMultiTextureCoords = Min((int)CRC_MAXTEXCOORDS, nMax_ShaderTexcoords);
	m_nMultiTexture = Min((int)CRC_MAXTEXTURES, nMax_ShaderTextures);

	if(m_bLog) LogFile(CStr("    m_fp_glBindProgramARB = %.8x", m_fp_glBindProgramARB));
	if(m_bLog) LogFile(CStr("    m_fp_glDeleteProgramsARB = %.8x", m_fp_glDeleteProgramsARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGenProgramsARB = %.8x", m_fp_glGenProgramsARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetProgramEnvParameterdvARB = %.8x", m_fp_glGetProgramEnvParameterdvARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetProgramEnvParameterfvARB = %.8x", m_fp_glGetProgramEnvParameterfvARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetProgramivARB = %.8x", m_fp_glGetProgramivARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetProgramStringARB = %.8x", m_fp_glGetProgramStringARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetVertexAttribdvARB = %.8x", m_fp_glGetVertexAttribdvARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetVertexAttribfvARB = %.8x", m_fp_glGetVertexAttribfvARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetVertexAttribivARB = %.8x", m_fp_glGetVertexAttribivARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetVertexAttribPointervARB = %.8x", m_fp_glGetVertexAttribPointervARB));
	if(m_bLog) LogFile(CStr("    m_fp_glIsProgramARB = %.8x", m_fp_glIsProgramARB));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramStringARB = %.8x", m_fp_glProgramStringARB));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramEnvParameter4dARB = %.8x", m_fp_glProgramEnvParameter4dARB));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramEnvParameter4dvARB = %.8x", m_fp_glProgramEnvParameter4dvARB));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramEnvParameter4fARB = %.8x", m_fp_glProgramEnvParameter4fARB));
	if(m_bLog) LogFile(CStr("    m_fp_glProgramEnvParameter4fvARB = %.8x", m_fp_glProgramEnvParameter4fvARB));
	if(m_bLog) LogFile(CStr("    m_fp_glVertexAttribPointerARB = %.8x", m_fp_glVertexAttribPointerARB));
	if(m_bLog) LogFile(CStr("    m_fp_glEnableVertexAttribArrayARB = %.8x", m_fp_glEnableVertexAttribArrayARB));
	if(m_bLog) LogFile(CStr("    m_fp_glDisableVertexAttribArrayARB = %.8x", m_fp_glDisableVertexAttribArrayARB));
	if(m_bLog) LogFile(CStr("    ShaderTextures                       %d (%d avail)", m_nMultiTexture, nMax_ShaderTextures));
	if(m_bLog) LogFile(CStr("    ShaderTexcoords                      %d (%d avail)", m_nMultiTextureCoords, nMax_ShaderTexcoords));
	if(m_bLog) LogFile(CStr("    MaxProgramInstructions               %d", nMaxProgramInstructions));
	if(m_bLog) LogFile(CStr("    MaxProgramNativeInstructions         %d", nMaxProgramNativeInstructions));
	if(m_bLog) LogFile(CStr("    MaxProgramALUInstructions            %d", nMaxProgramALUInstructions));
	if(m_bLog) LogFile(CStr("    MaxProgramTEXInstructions            %d", nMaxProgramTEXInstructions));
	if(m_bLog) LogFile(CStr("    MaxProgramTEXIndirections            %d", nMaxProgramTEXIndirections));
	if(m_bLog) LogFile(CStr("    MaxProgramNativeALUInstructions      %d", nMaxProgramNativeALUInstructions));
	if(m_bLog) LogFile(CStr("    MaxProgramNativeTEXInstructions      %d", nMaxProgramNativeTEXInstructions));
	if(m_bLog) LogFile(CStr("    MaxProgramNativeTEXIndirections      %d", nMaxProgramNativeTEXIndirections));

	if(m_bLog) LogFile(CStr("    MaxProgramNativeTemporaries          %d", nMaxProgramNativeTemporaries));
	if(m_bLog) LogFile(CStr("    MaxProgramNativeParameters           %d", nMaxProgramNativeParameters));
	if(m_bLog) LogFile(CStr("    MaxProgramNativeAttribs              %d", nMaxProgramNativeAttribs));
	if(m_bLog) LogFile(CStr("    MaxProgramLocalParameters            %d", nMaxProgramLocalParameters));
	if(m_bLog) LogFile(CStr("    MaxProgramEnvParameters              %d", nMaxProgramEnvParameters));


	if (m_fp_glBindProgramARB &&
		m_fp_glDeleteProgramsARB &&
		m_fp_glGenProgramsARB &&
		m_fp_glGetProgramEnvParameterdvARB &&
		m_fp_glGetProgramEnvParameterfvARB &&
		m_fp_glGetProgramivARB &&
		m_fp_glGetProgramStringARB &&
		m_fp_glGetVertexAttribdvARB &&
		m_fp_glGetVertexAttribfvARB &&
		m_fp_glGetVertexAttribivARB &&
		m_fp_glGetVertexAttribPointervARB &&
		m_fp_glIsProgramARB &&
		m_fp_glProgramStringARB &&
		m_fp_glProgramEnvParameter4dARB &&
		m_fp_glProgramEnvParameter4dvARB &&
		m_fp_glProgramEnvParameter4fARB &&
		m_fp_glProgramEnvParameter4fvARB &&
		m_fp_glVertexAttribPointerARB &&
		m_fp_glEnableVertexAttribArrayARB &&
		m_fp_glDisableVertexAttribArrayARB)
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (!pSys) Error("GetExtension_FragmentProgramARB", "No system.");

		m_Extensions |= CRCGL_EXT_FRAGMENTPROGRAM;
	}
}

void CRenderContextGL::GetExtension_TextureEnvDot3ARB()
{
	if(m_bLog) LogFile("Checking extension GL_ARB_TEXTURE_ENV_COMBINE & GL_ARB_TEXTURE_ENV_DOT3...");
	if (m_GLExtensions.Find("GL_ARB_TEXTURE_ENV_COMBINE") < 0) return;
	if (m_GLExtensions.Find("GL_ARB_TEXTURE_ENV_DOT3") < 0) return;

	m_Extensions |= CRCGL_ARB_TEXTURE_ENV_DOT3;
	if(m_bLog) ConOutL("    Enabling GL_ARB_TEXTURE_ENV_COMBINE & GL_ARB_TEXTURE_ENV_DOT3...");
}

void CRenderContextGL::GetExtension_ATISeparateStencil()
{
	if(m_bLog) LogFile("Checking extension GL_ATI_SEPARATE_STENCIL...");
	if (m_GLExtensions.Find("GL_ATI_SEPARATE_STENCIL") < 0) return;
	if (m_GLExtensions.Find("GL_EXT_STENCIL_WRAP") < 0) return;

	m_fp_glStencilFuncSeparateATI	= (PFNGLSTENCILFUNCSEPARATEATIPROC) wglGetProcAddress("glStencilFuncSeparateATI");
	m_fp_glStencilOpSeparateATI	= (PFNGLSTENCILOPSEPARATEATIPROC) wglGetProcAddress("glStencilOpSeparateATI");

	if(m_bLog) LogFile(CStr("    m_fp_glStencilFuncSeparateATI = %.8x", m_fp_glStencilFuncSeparateATI));
	if(m_bLog) LogFile(CStr("    m_fp_glStencilOpSeparateATI = %.8x", m_fp_glStencilOpSeparateATI));

	if( m_fp_glStencilFuncSeparateATI &&
		m_fp_glStencilOpSeparateATI )
	{
		if(m_bLog) ConOutL("    Enabling GL_ATI_SEPARATE_STENCIL...");
		m_Extensions |= CRCGL_EXT_ATI_SEPARATESTENCIL;
	}
}


void CRenderContextGL::GetExtension_StencilTwoSide()
{
	if(m_bLog) LogFile("Checking extension GL_EXT_STENCIL_TWO_SIDE...");
	if (m_GLExtensions.Find("GL_EXT_STENCIL_TWO_SIDE") < 0) return;
	if (m_GLExtensions.Find("GL_EXT_STENCIL_WRAP") < 0) return;

	m_fp_glActiveStencilFaceEXT	= (PFNGLACTIVESTENCILFACEEXT) wglGetProcAddress("glActiveStencilFaceEXT");

	if(m_bLog) LogFile(CStr("    m_fp_glActiveStencilFaceEXT = %.8x", m_fp_glActiveStencilFaceEXT));

	if( m_fp_glActiveStencilFaceEXT )
	{
		if(m_bLog) ConOutL("    Enabling GL_EXT_STENCIL_TWO_SIDE...");
		m_Extensions |= CRCGL_EXT_STENCILTWOSIDE;
	}
}

void CRenderContextGL::GetExtension_ATIFragmentShader()
{
	if(m_bLog) LogFile("Checking extension GL_ATI_FRAGMENT_SHADER...");
	if (m_GLExtensions.Find("GL_ATI_FRAGMENT_SHADER") < 0) return;

	m_fp_glGenFragmentShadersATI			= (PFNGLGENFRAGMENTSHADERSATI) wglGetProcAddress("glGenFragmentShadersATI");
	m_fp_glBindFragmentShaderATI			= (PFNGLBINDFRAGMENTSHADERATI) wglGetProcAddress("glBindFragmentShaderATI");
	m_fp_glDeleteFragmentShaderATI			= (PFNGLDELETEFRAGMENTSHADERATI) wglGetProcAddress("glDeleteFragmentShaderATI");
	m_fp_glBeginFragmentShaderATI			= (PFNGLBEGINFRAGMENTSHADERATI) wglGetProcAddress("glBeginFragmentShaderATI");
	m_fp_glEndFragmentShaderATI			= (PFNGLENDFRAGMENTSHADERATI) wglGetProcAddress("glEndFragmentShaderATI");
	m_fp_glPassTexCoordATI					= (PFNGLPASSTEXCOORDATI) wglGetProcAddress("glPassTexCoordATI");
	m_fp_glSampleMapATI					= (PFNGLSAMPLEMAPATI) wglGetProcAddress("glSampleMapATI");
	m_fp_glColorFragmentOp1ATI				= (PFNGLCOLORFRAGMENTOP1ATI) wglGetProcAddress("glColorFragmentOp1ATI");
	m_fp_glColorFragmentOp2ATI				= (PFNGLCOLORFRAGMENTOP2ATI) wglGetProcAddress("glColorFragmentOp2ATI");
	m_fp_glColorFragmentOp3ATI				= (PFNGLCOLORFRAGMENTOP3ATI) wglGetProcAddress("glColorFragmentOp3ATI");
	m_fp_glAlphaFragmentOp1ATI				= (PFNGLALPHAFRAGMENTOP1ATI) wglGetProcAddress("glAlphaFragmentOp1ATI");
	m_fp_glAlphaFragmentOp2ATI				= (PFNGLALPHAFRAGMENTOP2ATI) wglGetProcAddress("glAlphaFragmentOp2ATI");
	m_fp_glAlphaFragmentOp3ATI				= (PFNGLALPHAFRAGMENTOP3ATI) wglGetProcAddress("glAlphaFragmentOp3ATI");
	m_fp_glSetFragmentShaderConstantATI	= (PFNGLSETFRAGMENTSHADERCONSTANTATI) wglGetProcAddress("glSetFragmentShaderConstantATI");
	

	if(m_bLog) LogFile(CStr("    m_fp_glGenFragmentShadersATI = %.8x", m_fp_glGenFragmentShadersATI));
	if(m_bLog) LogFile(CStr("    m_fp_glBindFragmentShaderATI = %.8x", m_fp_glBindFragmentShaderATI));
	if(m_bLog) LogFile(CStr("    m_fp_glDeleteFragmentShaderATI = %.8x", m_fp_glDeleteFragmentShaderATI));
	if(m_bLog) LogFile(CStr("    m_fp_glBeginFragmentShaderATI = %.8x", m_fp_glBeginFragmentShaderATI));
	if(m_bLog) LogFile(CStr("    m_fp_glEndFragmentShaderATI = %.8x", m_fp_glEndFragmentShaderATI));
	if(m_bLog) LogFile(CStr("    m_fp_glPassTexCoordATI = %.8x", m_fp_glPassTexCoordATI));
	if(m_bLog) LogFile(CStr("    m_fp_glSampleMapATI = %.8x", m_fp_glSampleMapATI));
	if(m_bLog) LogFile(CStr("    m_fp_glColorFragmentOp1ATI = %.8x", m_fp_glColorFragmentOp1ATI));
	if(m_bLog) LogFile(CStr("    m_fp_glColorFragmentOp2ATI = %.8x", m_fp_glColorFragmentOp2ATI));
	if(m_bLog) LogFile(CStr("    m_fp_glColorFragmentOp3ATI = %.8x", m_fp_glColorFragmentOp3ATI));
	if(m_bLog) LogFile(CStr("    m_fp_glAlphaFragmentOp1ATI = %.8x", m_fp_glAlphaFragmentOp1ATI));
	if(m_bLog) LogFile(CStr("    m_fp_glAlphaFragmentOp2ATI = %.8x", m_fp_glAlphaFragmentOp2ATI));
	if(m_bLog) LogFile(CStr("    m_fp_glAlphaFragmentOp3ATI = %.8x", m_fp_glAlphaFragmentOp3ATI));
	if(m_bLog) LogFile(CStr("    m_fp_glSetFragmentShaderConstantATI = %.8x", m_fp_glSetFragmentShaderConstantATI));


	int nMaxFragmentRegisters;
	int nMaxFragmentConstants;
	int nMaxPasses;
	int nMaxInstructionsPerPass;
	int nMaxInstructionsTotal;
	int nMaxInputInterpolatorComponents;
	int nMaxLoopBackComponents;
	int nMaxColorAlphaPairing;
	glnGetIntegerv((GLenum)GL_NUM_FRAGMENT_REGISTERS_ATI, &nMaxFragmentRegisters);
	glnGetIntegerv((GLenum)GL_NUM_FRAGMENT_CONSTANTS_ATI, &nMaxFragmentConstants);
	glnGetIntegerv((GLenum)GL_NUM_PASSES_ATI, &nMaxPasses);
	glnGetIntegerv((GLenum)GL_NUM_INSTRUCTIONS_PER_PASS_ATI, &nMaxInstructionsPerPass);
	glnGetIntegerv((GLenum)GL_NUM_INSTRUCTIONS_TOTAL_ATI, &nMaxInstructionsTotal);
	glnGetIntegerv((GLenum)GL_NUM_INPUT_INTERPOLATOR_COMPONENTS_ATI, &nMaxInputInterpolatorComponents);
	glnGetIntegerv((GLenum)GL_NUM_LOOPBACK_COMPONENTS_ATI, &nMaxLoopBackComponents);
	glnGetIntegerv((GLenum)GL_COLOR_ALPHA_PAIRING_ATI, &nMaxColorAlphaPairing);

	if(m_bLog) LogFile(CStr("    GL_NUM_FRAGMENT_REGISTERS_ATI              %d", nMaxFragmentRegisters));
	if(m_bLog) LogFile(CStr("    GL_NUM_FRAGMENT_CONSTANTS_ATI              %d", nMaxFragmentConstants));
	if(m_bLog) LogFile(CStr("    GL_NUM_PASSES_ATI                          %d", nMaxPasses));
	if(m_bLog) LogFile(CStr("    GL_NUM_INSTRUCTIONS_PER_PASS_ATI           %d", nMaxInstructionsPerPass));
	if(m_bLog) LogFile(CStr("    GL_NUM_INSTRUCTIONS_TOTAL_ATI              %d", nMaxInstructionsTotal));
	if(m_bLog) LogFile(CStr("    GL_NUM_INPUT_INTERPOLATOR_COMPONENTS_ATI   %d", nMaxInputInterpolatorComponents));
	if(m_bLog) LogFile(CStr("    GL_NUM_LOOPBACK_COMPONENTS_ATI             %d", nMaxLoopBackComponents));
	if(m_bLog) LogFile(CStr("    GL_COLOR_ALPHA_PAIRING_ATI                 %d", nMaxColorAlphaPairing));

	if(	m_fp_glGenFragmentShadersATI &&
		m_fp_glBindFragmentShaderATI &&
		m_fp_glDeleteFragmentShaderATI &&
		m_fp_glBeginFragmentShaderATI &&
		m_fp_glEndFragmentShaderATI &&
		m_fp_glPassTexCoordATI &&
		m_fp_glSampleMapATI &&
		m_fp_glColorFragmentOp1ATI &&
		m_fp_glColorFragmentOp2ATI &&
		m_fp_glColorFragmentOp3ATI &&
		m_fp_glAlphaFragmentOp1ATI &&
		m_fp_glAlphaFragmentOp2ATI &&
		m_fp_glAlphaFragmentOp3ATI &&
		m_fp_glSetFragmentShaderConstantATI )
	{
		if(m_bLog) ConOutL("    Enabling GL_ATI_FRAGMENT_SHADER...");
		m_Extensions |= CRCGL_EXT_ATI_FRAGMENTSHADER;

		m_nMultiTextureEnv = Max(6, m_nMultiTextureEnv);

		m_ExtensionsActive |= CRCGL_EXT_ATI_FRAGMENTSHADER;
		ATIFS_Parse("ATI_Fragment_Shader/XRShader_SpecDiffuse", 1);
//		ATIFS_Parse("ATI_Fragment_Shader/XRShader_Diffuse", 1);

	}
}

void CRenderContextGL::GetExtension_ARBVertexBufferObject()
{
	if(m_bLog) LogFile("Checking extension GL_ARB_VERTEX_BUFFER_OBJECT...");
	if (m_GLExtensions.Find("GL_ARB_VERTEX_BUFFER_OBJECT") < 0)
		return;

	m_fp_glBindBufferARB					= (PFNGLBINDBUFFERARB) wglGetProcAddress("glBindBufferARB");
	m_fp_glDeleteBuffersARB				= (PFNGLDELETEBUFFERSARB) wglGetProcAddress("glDeleteBuffersARB");
	m_fp_glGenBuffersARB					= (PFNGLGENBUFFERSARB) wglGetProcAddress("glGenBuffersARB");
	m_fp_glIsBufferARB						= (PFNGLISBUFFERARB) wglGetProcAddress("glIsBufferARB");
	m_fp_glBufferDataARB					= (PFNGLBUFFERDATAARB) wglGetProcAddress("glBufferDataARB");
	m_fp_glBufferSubDataARB				= (PFNGLBUFFERSUBDATAARB) wglGetProcAddress("glBufferSubDataARB");
	m_fp_glGetBufferSubDataARB				= (PFNGLGETBUFFERSUBDATAARB) wglGetProcAddress("glGetBufferSubDataARB");
	m_fp_glMapBufferARB					= (PFNGLMAPBUFFERARB) wglGetProcAddress("glMapBufferARB");
	m_fp_glUnmapBufferARB					= (PFNGLUNMAPBUFFERARB) wglGetProcAddress("glUnmapBufferARB");
	m_fp_glGetBufferParameterivARB			= (PFNGLGETBUFFERPARAMETERIVARB) wglGetProcAddress("glGetBufferParameterivARB");
	m_fp_glGetBufferPointervARB			= (PFNGLGETBUFFERPOINTERVARB) wglGetProcAddress("glGetBufferPointervARB");

	if(m_bLog) LogFile(CStr("    m_fp_glBindBufferARB = %.8x", m_fp_glBindBufferARB));
	if(m_bLog) LogFile(CStr("    m_fp_glDeleteBuffersARB = %.8x", m_fp_glDeleteBuffersARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGenBuffersARB = %.8x", m_fp_glGenBuffersARB));
	if(m_bLog) LogFile(CStr("    m_fp_glIsBufferARB = %.8x", m_fp_glIsBufferARB));
	if(m_bLog) LogFile(CStr("    m_fp_glBufferDataARB = %.8x", m_fp_glBufferDataARB));
	if(m_bLog) LogFile(CStr("    m_fp_glBufferSubDataARB = %.8x", m_fp_glBufferSubDataARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetBufferSubDataARB = %.8x", m_fp_glGetBufferSubDataARB));
	if(m_bLog) LogFile(CStr("    m_fp_glMapBufferARB = %.8x", m_fp_glMapBufferARB));
	if(m_bLog) LogFile(CStr("    m_fp_glUnmapBufferARB = %.8x", m_fp_glUnmapBufferARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetBufferParameterivARB = %.8x", m_fp_glGetBufferParameterivARB));
	if(m_bLog) LogFile(CStr("    m_fp_glGetBufferPointervARB = %.8x", m_fp_glGetBufferPointervARB));

	if( m_fp_glBindBufferARB &&
		m_fp_glDeleteBuffersARB &&
		m_fp_glGenBuffersARB &&
		m_fp_glIsBufferARB &&
		m_fp_glBufferDataARB &&
		m_fp_glBufferSubDataARB &&
		m_fp_glGetBufferSubDataARB &&
		m_fp_glMapBufferARB &&
		m_fp_glUnmapBufferARB &&
		m_fp_glGetBufferParameterivARB &&
		m_fp_glGetBufferPointervARB)
	{
		if(m_bLog) ConOutL("    Enabling GL_ARB_VERTEX_BUFFER_OBJECT...");
		m_Extensions |= CRCGL_EXT_VERTEXBUFFEROBJECT;
	}
}

void CRenderContextGL::GetExtension_NVFragmentProgram()
{
	if(m_bLog) LogFile("Checking extension GL_NV_FRAGMENT_PROGRAM_OPTION...");
	if (m_GLExtensions.Find("GL_NV_FRAGMENT_PROGRAM_OPTION") < 0)
		return;
	if (m_GLExtensions.Find("GL_NV_FRAGMENT_PROGRAM") < 0)
		return;

	if(m_bLog) ConOutL("    Enabling GL_NV_FRAGMENT_PROGRAM_OPTION...");
	m_Extensions |= CRCGL_EXT_NV_FRAGMENT_PROGRAM;
}

void CRenderContextGL::GetExtension_NVFragmentProgram2()
{
	if(m_bLog) LogFile("Checking extension GL_NV_FRAGMENT_PROGRAM2_OPTIONS...");
	if (m_GLExtensions.Find("GL_NV_FRAGMENT_PROGRAM_OPTION") < 0)
		return;
	if (m_GLExtensions.Find("GL_NV_FRAGMENT_PROGRAM") < 0)
		return;
	if (m_GLExtensions.Find("GL_NV_FRAGMENT_PROGRAM2") < 0)
		return;

	if(m_bLog) ConOutL("    Enabling GL_NV_FRAGMENT_PROGRAM2_OPTION...");
	m_Extensions |= CRCGL_EXT_NV_FRAGMENT_PROGRAM2;
}

void CRenderContextGL::GetExtension_NVCopyDepthToColor()
{
	if(m_bLog) LogFile("Checking extension GL_NV_COPY_DEPTH_TO_COLOR...");
	if (m_GLExtensions.Find("GL_NV_COPY_DEPTH_TO_COLOR") < 0)
		return;

	if(m_bLog) ConOutL("    Enabling GL_NV_COPY_DEPTH_TO_COLOR...");
	m_Extensions |= CRCGL_EXT_NV_COPYDEPTHTOCOLOR;
}

void CRenderContextGL::GetExtension_ARBWindowPos()
{
	if(m_bLog) LogFile("Checking extension GL_ARB_WINDOW_POS...");

	m_fp_glWindowPos2fARB					= (PFNGLWINDOWPOS2F) wglGetProcAddress("glWindowPos2f");
	m_fp_glWindowPos2iARB					= (PFNGLWINDOWPOS2I) wglGetProcAddress("glWindowPos2i");
	m_fp_glWindowPos3fARB					= (PFNGLWINDOWPOS3F) wglGetProcAddress("glWindowPos3f");
	m_fp_glWindowPos3iARB					= (PFNGLWINDOWPOS3I) wglGetProcAddress("glWindowPos3i");

	if(m_bLog) LogFile(CStr("    m_fp_glWindowPos2fARB = %.8x", m_fp_glWindowPos2fARB));
	if(m_bLog) LogFile(CStr("    m_fp_glWindowPos2iARB = %.8x", m_fp_glWindowPos2iARB));
	if(m_bLog) LogFile(CStr("    m_fp_glWindowPos3fARB = %.8x", m_fp_glWindowPos3fARB));
	if(m_bLog) LogFile(CStr("    m_fp_glWindowPos3iARB = %.8x", m_fp_glWindowPos3iARB));

	if( m_fp_glWindowPos2fARB &&
		m_fp_glWindowPos2iARB &&
		m_fp_glWindowPos3fARB &&
		m_fp_glWindowPos3iARB)
	{
		if(m_bLog) ConOutL("    Enabling GL_ARB_WINDOW_POS...");
		m_Extensions |= CRCGL_EXT_WINDOWPOS;
	}
//	else
//		Error("GetExtension_ARBWindowPos", "GL_ARB_WINDOW_POS must be supported.");
}

void CRenderContextGL::GetExtension_ARBTextureNonPowerOfTwo()
{
	if(m_bLog) LogFile("Checking extension GL_ARB_TEXTURE_NON_POWER_OF_TWO...");
	if (m_GLExtensions.Find("GL_ARB_TEXTURE_NON_POWER_OF_TWO") < 0)
		return;

	if(m_bLog) ConOutL("    Enabling GL_ARB_TEXTURE_NON_POWER_OF_TWO...");
	m_Extensions	|= CRCGL_ARB_TEXTURE_NON_POWER_OF_TWO;
}

void CRenderContextGL::GetExtension_EXTFrameBufferObject()
{
	if(m_bLog) LogFile("Checking extension GL_EXT_FRAMEBUFFER_OBJECT...");
	if (m_GLExtensions.Find("GL_EXT_FRAMEBUFFER_OBJECT") < 0)
		return;

	m_fp_glIsRenderbufferEXT				= (PFNGLISRENDERBUFFEREXT) wglGetProcAddress("glIsRenderbufferEXT");
	m_fp_glBindRenderbufferEXT				= (PFNGLBINDRENDERBUFFEREXT) wglGetProcAddress("glBindRenderbufferEXT");
	m_fp_glDeleteRenderbuffersEXT			= (PFNGLDELETERENDERBUFFERSEXT) wglGetProcAddress("glDeleteRenderbuffersEXT");
	m_fp_glGenRenderbuffersEXT				= (PFNGLGENRENDERBUFFERSEXT) wglGetProcAddress("glGenRenderbuffersEXT");
	m_fp_glRenderbufferStorageEXT			= (PFNGLRENDERBUFFERSTORAGEEXT) wglGetProcAddress("glRenderbufferStorageEXT");
	m_fp_glGetRenderbufferParameterivEXT	= (PFNGLGETRENDERBUFFERPARAMETERIVEXT) wglGetProcAddress("glGetRenderbufferParameterivEXT");
	m_fp_glIsFramebufferEXT					= (PFNGLISFRAMEBUFFEREXT) wglGetProcAddress("glIsFramebufferEXT");
	m_fp_glBindFramebufferEXT				= (PFNGLBINDFRAMEBUFFEREXT) wglGetProcAddress("glBindFramebufferEXT");
	m_fp_glDeleteFramebuffersEXT			= (PFNGLDELETEFRAMEBUFFERSEXT) wglGetProcAddress("glDeleteFramebuffersEXT");
	m_fp_glGenFramebuffersEXT				= (PFNGLGENFRAMEBUFFERSEXT) wglGetProcAddress("glGenFramebuffersEXT");
	m_fp_glCheckFramebufferStatusEXT		= (PFNGLCHECKFRAMEBUFFERSTATUSEXT) wglGetProcAddress("glCheckFramebufferStatusEXT");
	m_fp_glFramebufferTexture1DEXT			= (PFNGLFRAMEBUFFERTEXTURE1DEXT) wglGetProcAddress("glFramebufferTexture1DEXT");
	m_fp_glFramebufferTexture2DEXT			= (PFNGLFRAMEBUFFERTEXTURE2DEXT) wglGetProcAddress("glFramebufferTexture2DEXT");
	m_fp_glFramebufferTexture3DEXT			= (PFNGLFRAMEBUFFERTEXTURE3DEXT) wglGetProcAddress("glFramebufferTexture3DEXT");
	m_fp_glFramebufferRenderbufferEXT		= (PFNGLFRAMEBUFFERRENDERBUFFEREXT) wglGetProcAddress("glFramebufferRenderbufferEXT");
	m_fp_glGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXT) wglGetProcAddress("glGetFramebufferAttachmentParameterivEXT");
	m_fp_glGenerateMipmapEXT				= (PFNGLGENERATEMIPMAPEXT) wglGetProcAddress("glGenerateMipmapEXT");

	glnGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT, &m_FBO_MaxColorBuffers);
	glnGetIntegerv(GL_MAX_RENDERBUFFER_SIZE_EXT, &m_FBO_MaxRenderBufferSize);

	if (m_bLog) LogFile(CStr("    m_fp_glIsRenderbufferEXT = %.8x", m_fp_glIsRenderbufferEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glBindRenderbufferEXT = %.8x", m_fp_glBindRenderbufferEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glDeleteRenderbuffersEXT = %.8x", m_fp_glDeleteRenderbuffersEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glGenRenderbuffersEXT = %.8x", m_fp_glGenRenderbuffersEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glRenderbufferStorageEXT = %.8x", m_fp_glRenderbufferStorageEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glGetRenderbufferParameterivEXT = %.8x", m_fp_glGetRenderbufferParameterivEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glIsFramebufferEXT = %.8x", m_fp_glIsFramebufferEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glBindFramebufferEXT = %.8x", m_fp_glBindFramebufferEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glDeleteFramebuffersEXT = %.8x", m_fp_glDeleteFramebuffersEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glGenFramebuffersEXT = %.8x", m_fp_glGenFramebuffersEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glCheckFramebufferStatusEXT = %.8x", m_fp_glCheckFramebufferStatusEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glFramebufferTexture1DEXT = %.8x", m_fp_glFramebufferTexture1DEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glFramebufferTexture2DEXT = %.8x", m_fp_glFramebufferTexture2DEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glFramebufferTexture3DEXT = %.8x", m_fp_glFramebufferTexture3DEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glFramebufferRenderbufferEXT = %.8x", m_fp_glFramebufferRenderbufferEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glGetFramebufferAttachmentParameterivEXT = %.8x", m_fp_glGetFramebufferAttachmentParameterivEXT));
	if (m_bLog) LogFile(CStr("    m_fp_glGenerateMipmapEXT = %.8x", m_fp_glGenerateMipmapEXT));

	if (m_fp_glIsRenderbufferEXT &&
		m_fp_glBindRenderbufferEXT &&
		m_fp_glDeleteRenderbuffersEXT &&
		m_fp_glGenRenderbuffersEXT &&
		m_fp_glRenderbufferStorageEXT &&
		m_fp_glGetRenderbufferParameterivEXT &&
		m_fp_glIsFramebufferEXT &&
		m_fp_glBindFramebufferEXT &&
		m_fp_glDeleteFramebuffersEXT &&
		m_fp_glGenFramebuffersEXT &&
		m_fp_glCheckFramebufferStatusEXT &&
		m_fp_glFramebufferTexture1DEXT &&
		m_fp_glFramebufferTexture2DEXT &&
		m_fp_glFramebufferTexture3DEXT &&
		m_fp_glFramebufferRenderbufferEXT &&
		m_fp_glGetFramebufferAttachmentParameterivEXT &&
		m_fp_glGenerateMipmapEXT)
	{
		if(m_bLog) ConOutL("    Enabling GL_EXT_FRAMEBUFFER_OBJECT...");
		if(m_bLog) ConOutL(CStr("        MaxColorBuffers = %d", m_FBO_MaxColorBuffers));
		if(m_bLog) ConOutL(CStr("        MaxRenderBufferSize = %d", m_FBO_MaxRenderBufferSize));
		m_Extensions |= CRCGL_EXT_FRAMEBUFFER_OBJECT;
	}
}


void CRenderContextGL::GetExtension_ARBPbuffer()
{
	if(m_bLog) LogFile("Checking extension WGL_ARB_PBUFFER & WGL_ARB_Render_Texture & WGL_ARB_Pixel_Format...");
	if (m_WGLExtensions.Find("GL_ARB_PBUFFER") < 0)
		return;
	if (m_WGLExtensions.Find("WGL_ARB_RENDER_TEXTURE") < 0)
		return;
	if (m_WGLExtensions.Find("WGL_ARB_PIXEL_FORMAT") < 0)
		return;

	m_fp_wglGetPixelFormatAttribivARB			= (PFNWGLGETPIXELFORMATATTRIBIVARB) wglGetProcAddress("wglGetPixelFormatAttribivARB");
	m_fp_wglGetPixelFormatAttribfvARB			= (PFNWGLGETPIXELFORMATATTRIBFVARB) wglGetProcAddress("wglGetPixelFormatAttribfvARB");
	m_fp_wglChoosePixelFormatARB			= (PFNWGLCHOOSEPIXELFORMATARB) wglGetProcAddress("wglChoosePixelFormatARB");
	m_fp_wglCreatePbufferARB			= (PFNWGLCREATEPBUFFERARB) wglGetProcAddress("wglCreatePbufferARB");
	m_fp_wglGetPbufferDCARB			= (PFNWGLGETPBUFFERDCARB) wglGetProcAddress("wglGetPbufferDCARB");
	m_fp_wglReleasePbufferDCARB			= (PFNWGLRELEASEPBUFFERDCARB) wglGetProcAddress("wglReleasePbufferDCARB");
	m_fp_wglDestroyPbufferARB			= (PFNWGLDESTROYPBUFFERARB) wglGetProcAddress("wglDestroyPbufferARB");
	m_fp_wglQueryPbufferARB			= (PFNWGLQUERYPBUFFERARB) wglGetProcAddress("wglQueryPbufferARB");
	m_fp_wglBindTexImageARB			= (PFNWGLBINDTEXIMAGEARB) wglGetProcAddress("wglBindTexImageARB");
	m_fp_wglReleaseTexImageARB			= (PFNWGLRELEASETEXIMAGEARB) wglGetProcAddress("wglReleaseTexImageARB");
	m_fp_wglSetPbufferAttribARB			= (PFNWGLSETPBUFFERATTRIBARB) wglGetProcAddress("wglSetPbufferAttribARB");

	if (m_bLog) LogFile(CStr("    m_fp_wglGetPixelFormatAttribivARB = %.8x", m_fp_wglGetPixelFormatAttribivARB));
	if (m_bLog) LogFile(CStr("    m_fp_wglGetPixelFormatAttribfvARB = %.8x", m_fp_wglGetPixelFormatAttribfvARB));
	if (m_bLog) LogFile(CStr("    m_fp_wglChoosePixelFormatARB = %.8x", m_fp_wglChoosePixelFormatARB));
	if (m_bLog) LogFile(CStr("    m_fp_wglCreatePbufferARB = %.8x", m_fp_wglCreatePbufferARB));
	if (m_bLog) LogFile(CStr("    m_fp_wglGetPbufferDCARB = %.8x", m_fp_wglGetPbufferDCARB));
	if (m_bLog) LogFile(CStr("    m_fp_wglReleasePbufferDCARB = %.8x", m_fp_wglReleasePbufferDCARB));
	if (m_bLog) LogFile(CStr("    m_fp_wglDestroyPbufferARB = %.8x", m_fp_wglDestroyPbufferARB));
	if (m_bLog) LogFile(CStr("    m_fp_wglQueryPbufferARB = %.8x", m_fp_wglQueryPbufferARB));
	if (m_bLog) LogFile(CStr("    m_fp_wglBindTexImageARB = %.8x", m_fp_wglBindTexImageARB));
	if (m_bLog) LogFile(CStr("    m_fp_wglReleaseTexImageARB = %.8x", m_fp_wglReleaseTexImageARB));
	if (m_bLog) LogFile(CStr("    m_fp_wglSetPbufferAttribARB = %.8x", m_fp_wglSetPbufferAttribARB));

	if (m_fp_wglGetPixelFormatAttribivARB &&
		m_fp_wglGetPixelFormatAttribfvARB &&
		m_fp_wglChoosePixelFormatARB &&
		m_fp_wglCreatePbufferARB &&
		m_fp_wglGetPbufferDCARB &&
		m_fp_wglReleasePbufferDCARB &&
		m_fp_wglDestroyPbufferARB &&
		m_fp_wglQueryPbufferARB &&
		m_fp_wglBindTexImageARB &&
		m_fp_wglReleaseTexImageARB &&
		m_fp_wglSetPbufferAttribARB)
	{
		if(m_bLog) ConOutL("    Enabling WGL_ARB_PBUFFER & WGL_ARB_Render_Texture & WGL_ARB_Pixel_Format...");
		m_Extensions |= CRCGL_ARB_PBUFFER;
	}
}
