/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			CXR_Model_Sky implementation

Author:			

Copyright:		

Contents:		Implementation of Sky Model and related classes

				Definition of CSkyLayerKey parameters:
					duration:		Duration of this key, in seconds
					priority:		Determines draw order
					depthfogscale:	Depthfog
					surface:		String ID of surface to use

				sky:
					Single XY-plane. Note that scrolling can also be handled in the surface.
					param 1:	Altitude of plane, in world units
					param 2:	Half-width (and depth) of plane
					param 3:	Texture-Coordinate scale factor
					param 4:	Scrolling speed of TexCoords


				envbox:	
					Skybox with 5 or 6 sides facing the viewer
					flags:
						1:		Only draw half a box
						2:		Box center is equal to viewer position
						4:		Flip Z coordinates
					param 1:	Texture-Coordinate scale factor
					param 2:	TCScale (seems to be unused)

				sprite:		
					A sprite/billboard, either projected at the back or with an absolute position
					flags:
						1:		Sprite is projected at the back instead of using pos
						2:		Sprite dimensions are fixed, given in screen width percentage instead of world units
						4:		Sprite is oriented towards the viewer's direction rather than the viewer.
								(When this is set, sprites do not get skewed or otherwise distorted close to the screen edges)
					pos:		The sprite's world position, or the direction in which the sprite is positioned (if flag 1 set)
					param 1:	Width of sprite, in world-units. Projected sprites are at distance 20000 from the viewer.
					param 2:	Height of sprite
					param 3:	Degree of rotation (0..1)

				fogcolor:
					Use of fog. 
					color:		Color of the depth fog

				lightstate:	
					Sets a light
					color:		Color of the light
					param 1:	Light ID

				box:
					Creates a skybox

Comments:		

History:		
2005-11-02: ae, Added this header
\*____________________________________________________________________*/


#include "PCH.h"

#include "WSky.h"
#include "../../XR/XR.h"
#include "../../XR/XRSurfaceContext.h"

#include "MFloat.h"

#define SKY_TEMP_CHEAPO

class CSky_RenderInstanceParamters
{
public:
	CSky_RenderInstanceParamters(CXR_Engine* _pEngine, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState) : m_pEngine(_pEngine), m_pVBM(_pVBM), m_pAnimState(_pAnimState)
	{
		m_pRC	= 0;
		m_pL2VMat	= 0;
		m_pVolumeFog	= 0;
		m_CurrentPass	= 0;

		m_CoordinateScale	= 1.0f;
	}

	CXR_Engine* m_pEngine;
	CXR_VBManager* m_pVBM;
	const CXR_AnimState* m_pAnimState;
	CRenderContext* m_pRC;

	CMat4Dfp32* m_pL2VMat;
	CMat4Dfp32 m_L2VMat;
	CMat4Dfp32 m_L2VMatInv;
	CMat4Dfp32 m_VMatInv;
	CVec3Dfp32 m_CurLocalVP;
	CVec3Dfp32 m_CurCamera;
	CXR_FogState* m_pVolumeFog;

	int m_CurrentPass;
	CXW_SurfaceKeyFrame m_BoxTmpSurfKey;

	fp32 m_CoordinateScale;
};

// -------------------------------------------------------------------
//  Sky enums
// -------------------------------------------------------------------
enum
{
	SKYLAYER_TYPE_VOID			= 0,
	SKYLAYER_TYPE_SKY			= 1,
	SKYLAYER_TYPE_ENVBOX		= 2,
	SKYLAYER_TYPE_SPRITE		= 3,
	SKYLAYER_TYPE_FOGCOLOR		= 4,
	SKYLAYER_TYPE_LIGHTSTATE	= 5,
	SKYLAYER_TYPE_BOX			= 6,

	SKYLAYER_NUMCOLORS			= 4,
};

// -------------------------------------------------------------------
class CSkyLayerKey
{
public:
	int m_Flags;
//	int m_RasterMode;
	fp32 m_Duration;
	CStr m_SurfaceName;
//	int m_SurfaceID;				// Surface context surface ID
	int m_iSurface;					// Surface index in CXR_Model_Sky::m_lspSurfaces
	spCXW_Surface m_spSurface;
	CPixel32 m_Colors[SKYLAYER_NUMCOLORS];
	CVec3Dfp32 m_Pos;
	fp32 m_Params[4];
	fp32 m_DepthFogScale;
	fp32 m_Priority;

	CSkyLayerKey()
	{
		m_Flags = 0;
		m_Duration = 1.0f;
//		m_SurfaceID = 0;
		m_iSurface = -1;
		m_Colors[0] = 0xffffffff;
		m_Colors[1] = 0xffffffff;
		m_Colors[2] = 0xffffffff;
		m_Colors[3] = 0xffffffff;
		m_Pos = 0;
		m_DepthFogScale = 1.0f;
		m_Priority = 0;
		FillChar(&m_Params, sizeof(m_Params), 0);
	}

	void ParseKeys(CRegistry* _pKeys)
	{
//LogFile("      Parsing key");
		for(int i = 0; i < _pKeys->GetNumChildren(); i++)
		{
			CStr Key = _pKeys->GetName(i);
			CStr Value = _pKeys->GetValue(i);

			const fp32 Valuef = Value.Val_fp64();

			if (Key == "FLAGS")
				m_Flags = Value.Val_int();
			else if (Key == "DURATION")
				m_Duration = Valuef;
			else if (Key == "PRIORITY")
				m_Priority = Valuef;
			else if (Key == "DEPTHFOGSCALE")
				m_DepthFogScale = Valuef;
			else if (Key == "SURFACE")
			{
				m_SurfaceName = Value;
			}
			else if (Key == "COLOR")
				m_Colors[0].Parse(Value);
			else if (Key == "COLOR2")
				m_Colors[1].Parse(Value);
			else if (Key == "COLOR3")
				m_Colors[2].Parse(Value);
			else if (Key == "COLOR4")
				m_Colors[0].Parse(Value);
			else if (Key == "POS")
				m_Pos.ParseString(Value);
			else if (Key == "PARAMS")
				for(int i = 0; i < 4; i++) m_Params[i] = (Value.GetStrSep(",")).Val_fp64();
		}
	}
};

// -------------------------------------------------------------------
const char* g_TypeTranslate[] = 
{
	"void", "sky", "envbox", "sprite", "fogcolor", "lightstate", "box", (char*)NULL
};

class CSkyLayer : public CReferenceCount
{
public:
	int m_Type;
	fp32 m_Duration;
	TArray<CSkyLayerKey> m_lKeys;

	void ParseKeys(CRegistry* _pKeys)
	{
//LogFile("   Parsing layer");
		m_Type = _pKeys->GetValue("TYPE").TranslateInt(g_TypeTranslate);

		CSkyLayerKey Key;
		for(int i = 0; i < _pKeys->GetNumChildren(); i++)
		{
			if (_pKeys->GetName(i).CompareNoCase("key") == 0)
			{
				Key.ParseKeys(_pKeys->GetChild(i));
				m_lKeys.Add(Key);
			}
		}

		// Sum up time
		{
			m_Duration = 0;
			for(int i = 0; i < m_lKeys.Len(); i++) m_Duration += m_lKeys[i].m_Duration;
		}
//LogFile(CStrF("    Time %f", m_Duration));
	}

	void GetValue(CMTime _Time, CSkyLayerKey& _Key)
	{
		fp32 TMod = _Time.GetTimeModulus(m_Duration);

		fp32 t = 0;
		int i;
		for(i = 0; i < m_lKeys.Len(); i++)
		{
			t += m_lKeys[i].m_Duration;
			if (TMod < t) break;
		}

		t -= m_lKeys[i].m_Duration;
		fp32 Frac = (TMod - t) / m_lKeys[i].m_Duration;

		if (i >= m_lKeys.Len()) i = 0;
		int i2 = (i + 1) % m_lKeys.Len();

		CSkyLayerKey* pK1 = &m_lKeys[i];
		CSkyLayerKey* pK2 = &m_lKeys[i2];

		_Key = *pK1;
		for(int k = 0; k < SKYLAYER_NUMCOLORS; k++)
			_Key.m_Colors[k] = pK2->m_Colors[k].AlphaBlendRGBA(pK1->m_Colors[k], (int)(Frac*256.0f));
		pK1->m_Pos.Lerp(pK2->m_Pos, Frac, _Key.m_Pos);
		for(i = 0; i < 4; i++)
			_Key.m_Params[i] = pK1->m_Params[i] + Frac*(pK2->m_Params[i] - pK1->m_Params[i]);
	}

	void InitSurfaces(TArray<spCXW_Surface>& _lspSurfaces)
	{
		for(int i = 0; i < m_lKeys.Len(); i++)
		{
			CSkyLayerKey* pK = &m_lKeys[i];
			if (pK->m_SurfaceName != "")
			{
				pK->m_iSurface = CXW_Surface::GetSurfaceIndex(_lspSurfaces, pK->m_SurfaceName);
				if (pK->m_iSurface >= 0)
				{
					pK->m_spSurface = _lspSurfaces[pK->m_iSurface];
					pK->m_spSurface->InitTextures(false);
				}

/*				pK->m_SurfaceID = _pSC->GetSurfaceID(pK->m_SurfaceName);
				CXW_Surface *pSurf = _pSC->GetSurface(pK->m_SurfaceID);
				if(pSurf)
					pSurf->InitTextures(false);	// Don't report failures.*/
			}
		}
	}

	void PrecacheSurfaces(CXR_Engine* _pEngine, CXR_SurfaceContext* _pSC)
	{
		for(int i = 0; i < m_lKeys.Len(); i++)
		{
			CSkyLayerKey* pK = &m_lKeys[i];
			if (pK->m_spSurface != NULL)
			{
				CXW_Surface *pSurf = pK->m_spSurface->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
				pSurf->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);

/*				pK->m_SurfaceID = _pSC->GetSurfaceID(pK->m_SurfaceName);
				CXW_Surface *pSurf = _pSC->GetSurface(pK->m_SurfaceID);
				if(pSurf)
					pSurf->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
*/
			}
		}
	}
};

typedef TPtr<CSkyLayer> spCSkyLayer;

// -------------------------------------------------------------------
class CSkySurface : public CReferenceCount
{
public:
	TArray<spCSkyLayer> m_lspLayers;

	CSkySurface()
	{
	}

	void Create(TArray<uint8> _lData);
	void Create(const char* _pFileName);
	void InitSurfaces(TArray<spCXW_Surface>&);
	void PrecacheSurfaces(CXR_Engine* _pEngine);
};


void CSkySurface::Create(TArray<uint8> _lData)
{
	MAUTOSTRIP(CSkySurface_Create, MAUTOSTRIP_VOID);
	//LogFile(CStrF("(CSkySurface::Create) Source script: %s", _pFileName));

	spCRegistry spRoot = REGISTRY_CREATE;
	if (!spRoot) MemError("ReadFromScript");

	CCFile file;
	file.Open(_lData, CFILE_READ);
	spRoot->XRG_Read(&file, "", TArray<CStr>::TArray(), true);
	file.Close();
	spRoot = spRoot->FindChild("skytrack");
	if (!spRoot)
		Error("ReadFromScript", CStr("Registry did not contain a skytrack."));

	for(int i = 0; i < spRoot->GetNumChildren(); i++)
	{
		if (spRoot->GetName(i).CompareNoCase("skylayer") == 0)
		{
			spCSkyLayer spL = MNew(CSkyLayer);
			spL->ParseKeys(spRoot->GetChild(i));
			m_lspLayers.Add(spL);
		}
	}
	//LogFile("(CSkySurface::Create) End.");
}

void CSkySurface::Create(const char* _pFileName)
{
	MAUTOSTRIP(CSkySurface_Create, MAUTOSTRIP_VOID);
	//LogFile(CStrF("(CSkySurface::Create) Source script: %s", _pFileName));

	spCRegistry spRoot = REGISTRY_CREATE;
	if (!spRoot) MemError("ReadFromScript");

	CStr FileName = _pFileName;
	spRoot->XRG_Read(FileName);
	spRoot = spRoot->FindChild("skytrack");
	if (!spRoot)
		Error("ReadFromScript", CStrF("Registry did not contain a skytrack. (%s)", _pFileName));

	for(int i = 0; i < spRoot->GetNumChildren(); i++)
	{
		if (spRoot->GetName(i).CompareNoCase("skylayer") == 0)
		{
			spCSkyLayer spL = MNew(CSkyLayer);
			spL->ParseKeys(spRoot->GetChild(i));
			m_lspLayers.Add(spL);
		}
	}

	//LogFile("(CSkySurface::Create) End.");
}

void CSkySurface::InitSurfaces(TArray<spCXW_Surface>& _lspSurfaces)
{
	MAUTOSTRIP(CSkySurface_InitSurfaces, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	for(int i = 0; i < m_lspLayers.Len(); i++)
		m_lspLayers[i]->InitSurfaces(_lspSurfaces);
}

void CSkySurface::PrecacheSurfaces(CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CSkySurface_PrecacheSurfaces, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	for(int i = 0; i < m_lspLayers.Len(); i++)
		m_lspLayers[i]->PrecacheSurfaces(_pEngine, pSC);
}

//int BSPModel_CutFace2(CVec3Dfp32* _pVerts, int _nv, const CPlane3Dfp32* _pPlanes, int _np, 
//	int _bInvertPlanes, CVec2Dfp32* _pTVerts1 = NULL, CVec2Dfp32* _pTVerts2 = NULL, CVec2Dfp32* _pTVerts3 = NULL);

// -------------------------------------------------------------------
//  CXR_Model_Sky
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Sky, CXR_Model);


IMPLEMENT_OPERATOR_NEW(CXR_Model_Sky);


CXR_Model_Sky::CXR_Model_Sky()
{
	MAUTOSTRIP(CXR_Model_Sky_ctor, MAUTOSTRIP_VOID);
}

void CXR_Model_Sky::Create(const char* _pName, TArray<uint8> _lSurfaceData, TArray<uint8> _lSkyData )
{
	MAUTOSTRIP(CXR_Model_Sky_Create2, MAUTOSTRIP_VOID);

	CStr Param(_pName);

	CStr SkyName(Param.GetStrSep(","));
	if (SkyName == "")
	{
		ConOutL("§cf80WARNING: (CXR_Model_Sky::Create) No creation params.");
		return;
	}

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) Error("-", "No system.");

	m_spSky = MNew(CSkySurface);
	if (!m_spSky) MemError("-");
	m_spSky->Create(_lSkyData);

	m_lspSurfaces = CXW_Surface::ReadScript(_lSurfaceData);

	m_spSky->InitSurfaces(m_lspSurfaces);

	m_LastFogColor = 0;
}

void CXR_Model_Sky::Create(const char* _pParam)
{
	MAUTOSTRIP(CXR_Model_Sky_Create, MAUTOSTRIP_VOID);
/*	try
	{
		m_spCloudTxt = DNew(CTextureContainer_Clouds) CTextureContainer_Clouds;
		if (!m_spCloudTxt) MemError("-");
		m_spCloudTxt->Create();
	}
	catch(CCException)
	{
		ConOutL("§cf80WARNING: Could not create procedural clouds.");
		m_spProcTxt = NULL;
	}*/

	CStr Param(_pParam);

	CStr SkyName(Param.GetStrSep(","));
	if (SkyName == "")
	{
		ConOutL("§cf80WARNING: (CXR_Model_Sky::Create) No creation params.");
		return;
	}

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) Error("-", "No system.");

	m_spSky = MNew(CSkySurface);
	if (!m_spSky) MemError("-");
	m_spSky->Create(SkyName);

	CStr SurfName = CStrF("%s%s%s%s", SkyName.GetPath().Str(), "Surf_", SkyName.GetFilenameNoExt().Str(), ".xtx");
	m_lspSurfaces = CXW_Surface::ReadScript(SurfName);

	m_spSky->InitSurfaces(m_lspSurfaces);

/*	m_spProcTxt = DNew(CTextureContainer_Sky) CTextureContainer_Sky;
	if (!m_spProcTxt) MemError("-");

	spCImage spFog = DNew(CImage) CImage;
	spFog->Create(SKY_FOGSIZE, SKY_FOGSIZE, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
	if (!spFog) MemError("-");
	int iLocal = m_spProcTxt->AddTexture(spFog);
	m_spProcTxt->GetTexture(iLocal)->m_Properties.m_Flags |= 16;
*/

//	spCReferenceCount spObj = (CReferenceCount*)MRTC_GOM()->CreateObject("CXR_Model_Flare");
//	m_spFlare = safe_cast<CXR_Model> ((CReferenceCount*)spObj);
//	if (m_spFlare != NULL) m_spFlare->Create((char*)CStrF("%f, %f, %f, %s", 9.0f, 2012.0f, 3000.0f, "SUN1"));

//	m_FlareAnim.m_Anim0 = 2012;
//	m_FlareAnim.m_Anim1 = 3000;

	m_LastFogColor = 0;

}

void CXR_Model_Sky::OnPrecache(CXR_Engine* _pEngine, int _iVariation)
{
	MAUTOSTRIP(CXR_Model_Sky_OnPrecache, MAUTOSTRIP_VOID);
	if(m_spSky != NULL)
		m_spSky->PrecacheSurfaces(_pEngine);
}


CXR_Model_Sky::CSky_ViewContext::CSky_ViewContext()
{
	MAUTOSTRIP(CXR_Model_Sky_CSky_ViewContext_ctor, MAUTOSTRIP_VOID);
	m_nBoxFaces = 6;

	{
		for(int f = 0; f < m_nBoxFaces; f++)
			m_lBoxFaces[f].m_F2LMat.Unit();
	}
	m_lBoxFaces[0].m_L2FMat.Unit();

	m_lBoxFaces[0].m_ClipPlanes[0].CreateNV(CVec3Dfp32(1, 1, 0), 0);
	m_lBoxFaces[0].m_ClipPlanes[1].CreateNV(CVec3Dfp32(1, -1, 0), 0);
	m_lBoxFaces[0].m_ClipPlanes[2].CreateNV(CVec3Dfp32(1, 0, 1), 0);
	m_lBoxFaces[0].m_ClipPlanes[3].CreateNV(CVec3Dfp32(1, 0, -1), 0);

	for(int i = 0; i < 6; i++)
	{
		CMat4Dfp32 Mat;
		Mat.Unit();
		switch(i)
		{
		case 0 :
			break;
		case 1 :
			Mat.M_x_RotZ(0.5f);
			break;
		case 2 :
			Mat.M_x_RotZ(0.75f);
			break;
		case 3 :
			Mat.M_x_RotZ(0.25f);
			break;
		case 4 :
			Mat.M_x_RotY(0.25f);
			break;
		case 5 :
			Mat.M_x_RotY(-0.25f);
			break;
		};

		m_lBoxFaces[i].m_F2LMat = Mat;
	}

/*	m_lBoxFaces[0].m_F2LMat.RotZ_x_M(0.0f);
	m_lBoxFaces[1].m_F2LMat.RotZ_x_M(-0.25f);
	m_lBoxFaces[2].m_F2LMat.RotZ_x_M(-0.50f);
	m_lBoxFaces[3].m_F2LMat.RotZ_x_M(-0.75f);
	m_lBoxFaces[1].m_F2LMat.RotZ_x_M(0.25f);
	m_lBoxFaces[2].m_F2LMat.RotZ_x_M(0.50f);
	m_lBoxFaces[3].m_F2LMat.RotZ_x_M(0.75f);
	m_lBoxFaces[4].m_F2LMat.RotY_x_M(-0.25f);
	m_lBoxFaces[5].m_F2LMat.RotY_x_M(0.25f);*/

/*	for(int i = 0; i < 6; i++)
	{
		m_lBoxFaces[i].m_F2LMat.k[0][1] = -m_lBoxFaces[i].m_F2LMat.k[0][1];
		m_lBoxFaces[i].m_F2LMat.k[1][1] = -m_lBoxFaces[i].m_F2LMat.k[1][1];
		m_lBoxFaces[i].m_F2LMat.k[2][1] = -m_lBoxFaces[i].m_F2LMat.k[2][1];
		m_lBoxFaces[i].m_F2LMat.k[3][1] = -m_lBoxFaces[i].m_F2LMat.k[3][1];
	}*/


	for(int f = 0; f < m_nBoxFaces; f++)
	{
		for(int i = 0; i < 4; i++)
		{
			m_lBoxFaces[f].m_ClipPlanes[i] = m_lBoxFaces[0].m_ClipPlanes[i];
			m_lBoxFaces[f].m_ClipPlanes[i].Transform(m_lBoxFaces[f].m_F2LMat);
		}
		m_lBoxFaces[f].m_F2LMat.InverseOrthogonal(m_lBoxFaces[f].m_L2FMat);
	}
}

CXR_Model_Sky::CSky_ViewContext* CXR_Model_Sky::Sky_GetVC(CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CXR_Model_Sky_Sky_GetVC, NULL);
	int Depth = _pEngine->GetVCDepth();
	m_lVC.QuickSetLen(Depth+1);

	return &m_lVC[Depth];
}

void CXR_Model_Sky::Sky_PrepareFrame(CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CXR_Model_Sky_Sky_PrepareFrame, MAUTOSTRIP_VOID);
	CXR_ViewContext* pVC = _pEngine->GetVC();
	CSky_ViewContext* pSkyVC = Sky_GetVC(_pEngine);

	pSkyVC->m_BoxL2WMat.Unit();
	CVec3Dfp32::GetMatrixRow(pVC->m_CameraWMat, 3).SetMatrixRow(pSkyVC->m_BoxL2WMat, 3);
	pSkyVC->m_BoxL2WMat.InverseOrthogonal(pSkyVC->m_BoxW2LMat);

	CMat4Dfp32 W2VMat;
	pVC->m_CameraWMat.InverseOrthogonal(W2VMat);
	pSkyVC->m_BoxL2WMat.Multiply(W2VMat, pSkyVC->m_BoxL2VMat);

	Sky_ClearFrustrum(_pEngine);
}

void CXR_Model_Sky::Sky_ClearFrustrum(CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CXR_Model_Sky_Sky_ClearFrustrum, MAUTOSTRIP_VOID);
	CSky_ViewContext* pSkyVC = Sky_GetVC(_pEngine);

	for(int i = 0; i < pSkyVC->m_nBoxFaces; i++)
		pSkyVC->m_lBoxFaces[i].Clear();
}

void CXR_Model_Sky::Sky_OpenFrustrum(CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CXR_Model_Sky_Sky_OpenFrustrum, MAUTOSTRIP_VOID);
	CSky_ViewContext* pSkyVC = Sky_GetVC(_pEngine);

	for(int i = 0; i < pSkyVC->m_nBoxFaces; i++)
		pSkyVC->m_lBoxFaces[i].Open();
}

void CXR_Model_Sky::Sky_ExpandFrustrum(CXR_Engine* _pEngine, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CVec3Dfp32* _pV, int _nV)
{
return;	// Since we do open frustrum always in OnRender we don't have to waste time on this.

	MAUTOSTRIP(CXR_Model_Sky_Sky_ExpandFrustrum, MAUTOSTRIP_VOID);
	CSky_ViewContext* pSkyVC = Sky_GetVC(_pEngine);

	CMat4Dfp32 Mat;
	_WMat.Multiply(pSkyVC->m_BoxW2LMat, Mat);

	CVec3Dfp32 V[32];
	CVec3Dfp32::MultiplyMatrix(_pV, V, Mat, _nV);

	for(int f = 0; f < pSkyVC->m_nBoxFaces; f++)
	{
		CSkyBoxFace* pF = &pSkyVC->m_lBoxFaces[f];

		CVec3Dfp32 VTmp[32];
		memcpy(VTmp, V, _nV*sizeof(CVec3Dfp32));

		int nv = CutFence(VTmp, _nV, pF->m_ClipPlanes, 4, false);
//		int nv = _nV;
		if (nv > 2)
		{
			CVec3Dfp32 FaceV[32];
//			CVec2Dfp32 FaceVProj[32];
			CVec3Dfp32::MultiplyMatrix(VTmp, FaceV, pF->m_L2FMat, nv);

			for(int v = 0; v < nv; v++)
			{
				fp32 x = FaceV[v][1] / FaceV[v][0];
				fp32 y = FaceV[v][2] / FaceV[v][0];
//ConOut(CStrF("BoxFace %d, v %d, %f, %f, Z %f", f, v, x, y, FaceV[v][0]));
				if (x < pF->m_MinVis[0]) pF->m_MinVis[0] = x;
				if (y < pF->m_MinVis[1]) pF->m_MinVis[1] = y;
				if (x > pF->m_MaxVis[0]) pF->m_MaxVis[0] = x;
				if (y > pF->m_MaxVis[1]) pF->m_MaxVis[1] = y;
			}
		}
	}
}

void CXR_Model_Sky::RenderSkyBox(CSky_RenderInstanceParamters* _pRenderParams, const CPlane3Dfp32* _pPlanes, int _nPlanes, CSkyLayerKey* _pKey)
{
	MAUTOSTRIP(CXR_Model_Sky_RenderSkyBox, MAUTOSTRIP_VOID);
	if (!_pRenderParams->m_pEngine) return;

	int nPrim = 6;
	uint16* pPrim = _pRenderParams->m_pVBM->Alloc_Int16(nPrim);
	if (!pPrim) return;
	pPrim[0] = 0;
	pPrim[1] = 1;
	pPrim[2] = 2;
	pPrim[3] = 0;
	pPrim[4] = 2;
	pPrim[5] = 3;

	CSky_ViewContext* pSkyVC = Sky_GetVC(_pRenderParams->m_pEngine);

/*	if (pSkyVC->m_nBoxFaces > 128)
		Error("CXR_Model_Sky::RenderSkyBox", "Stack would overflow");

	CVec3Dfp32 *pVertices[128];

	for (int i = 0; i < pSkyVC->m_nBoxFaces; ++i)
	{
		pVertices[i] = NULL;
	}*/

	fp32 Scale = 256.0f;
	for(int f = 0; f < pSkyVC->m_nBoxFaces; f++)
	{
		CSkyBoxFace* pF = &pSkyVC->m_lBoxFaces[f];

		CMat4Dfp32 Mat;
		pF->m_F2LMat.Multiply(pSkyVC->m_BoxL2VMat, Mat);

		CVec2Dfp32 vmin, vmax;
		vmin[0] = Max(-1.0f, pF->m_MinVis[0]);
		vmin[1] = Max(-1.0f, pF->m_MinVis[1]);
		vmax[0] = Min(1.0f, pF->m_MaxVis[0]);
		vmax[1] = Min(1.0f, pF->m_MaxVis[1]);

		if (vmin[0] >= vmax[0]) continue;
		if (vmin[1] >= vmax[1]) continue;

		vmin[0] -= 0.0002f;
		vmin[1] -= 0.0002f;
		vmax[0] += 0.0002f;
		vmax[1] += 0.0002f;
//		vmin[0] = -1;
//		vmin[1] = -1;
//		vmax[0] = 1;
//		vmax[1] = 1;

		CXR_VertexBuffer* pVB = _pRenderParams->m_pVBM->Alloc_VB(CXR_VB_VERTICES | CXR_VB_TVERTICES0, 4);
		if (!pVB) return;

		CXW_Surface* pSurf = NULL;

//		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
//		if (!pSC) Error("Create", "No surface-context available.");
//		pSurf = pSC->GetSurface(_pKey->m_SurfaceID);
		if (_pKey->m_spSurface != NULL)
			pSurf = _pKey->m_spSurface->GetSurface(_pRenderParams->m_pEngine->m_SurfOptions, _pRenderParams->m_pEngine->m_SurfCaps);
		if (!pSurf)
		{
			ConOut("(CXR_Model_Sky::RenderSkyBox) Surface not found: " + _pKey->m_SurfaceName);
			return;
		}

		CMTime AnimTime = CMTime::CreateFromSeconds(f+0.1f);
		CXW_SurfaceKeyFrame* pSurfKey = pSurf->GetFrame(0, AnimTime);


		CVec3Dfp32 Verts[4];

		Verts[0][1] = vmin[0] * Scale;
		Verts[0][2] = vmin[1] * Scale;
		Verts[0][0] = 1 * Scale;
		Verts[1][1] = vmax[0] * Scale;
		Verts[1][2] = vmin[1] * Scale;
		Verts[1][0] = 1 * Scale;
		Verts[2][1] = vmax[0] * Scale;
		Verts[2][2] = vmax[1] * Scale;
		Verts[2][0] = 1 * Scale;
		Verts[3][1] = vmin[0] * Scale;
		Verts[3][2] = vmax[1] * Scale;
		Verts[3][0] = 1 * Scale;

		CXR_VBChain *pChain = pVB->GetVBChain();
		CVec3Dfp32::MultiplyMatrix(Verts, pChain->m_pV, Mat, 4);
//		pVertices[f] = pChain->m_pV;

		CVec2Dfp32* pTV = (CVec2Dfp32*) pChain->m_pTV[0];
		vmin[0] = vmin[0]*0.5f + 0.5f;
		vmin[1] = -vmin[1]*0.5f + 0.5f;
		vmax[0] = vmax[0]*0.5f + 0.5f;
		vmax[1] = -vmax[1]*0.5f + 0.5f;
		pTV[0][0] = 1.0f-vmin[0];
		pTV[0][1] = vmin[1];
		pTV[1][0] = 1.0f-vmax[0];
		pTV[1][1] = vmin[1];
		pTV[2][0] = 1.0f-vmax[0];
		pTV[2][1] = vmax[1];
		pTV[3][0] = 1.0f-vmin[0];
		pTV[3][1] = vmax[1];

		pVB->Render_IndexedTriangles(pPrim, 2);

		CXR_RenderSurfExtParam Params;
		Params.m_DepthFogScale = _pKey->m_DepthFogScale;

		int Flags = RENDERSURFACE_DEPTHFOG;// | RENDERSURFACE_VERTEXFOG;
		CXR_Util::Render_Surface(Flags, AnimTime, pSurf, pSurfKey, _pRenderParams->m_pEngine, _pRenderParams->m_pVBM, (CMat4Dfp32*)NULL, (CMat4Dfp32*)NULL, (CMat4Dfp32*)NULL, pVB, 
			_pKey->m_Priority, 0.0001f, &Params);
	}

	/*// Loop through all point and merge almost equals
	for (int i = 0; i < pSkyVC->m_nBoxFaces; ++i)
	{
		if (pVertices[i])
		{
			for (int j = 0; j < pSkyVC->m_nBoxFaces; ++j)
			{
				if (pVertices[j])
				{
					for (int k = 0; k < 4; ++k)
					{
						if (pVertices[j][k].AlmostEqual(pVertices[i][k], 0.01f))
							pVertices[j][k] = pVertices[i][k];
					}
				}
			}
		}
	}*/

}

fp32 CXR_Model_Sky::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_Sky_GetBound_Sphere, 0.0f);
	return 1000000.0f;
}

void CXR_Model_Sky::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_Sky_GetBound_Box, MAUTOSTRIP_VOID);
	_Box.m_Min = -10000000.0f;
	_Box.m_Max = 10000000.0f;
}


void CXR_Model_Sky::Sky_RenderPoly_Surface(CSky_RenderInstanceParamters* _pRenderParams, const CPlane3Dfp32* _pPlanes, int _nPlanes, 
	const CVec3Dfp32* _pV, const CVec2Dfp32* _pTV, int _nVert, fp32 _Priority, CSkyLayerKey* _pKey)
{
	MSCOPESHORT(CXR_Model_Sky::Sky_RenderPoly_Surface); //AR-SCOPE
	if (!_pRenderParams->m_pEngine) return;

	CVec3Dfp32 V[32];
	CVec2Dfp32 TV[32];

	int nV = _nVert;

	// Clip and scale geometry...
	{
		for(int i = 0; i < _nVert; i++)
		{
			V[i] = _pV[i];
			TV[i] = _pTV[i];
		}

		// Bizarr:
		if (!_pRenderParams->m_pEngine->GetVCDepth()) nV = CutFence(V, _nVert, _pPlanes, _nPlanes, false, TV);
		if (nV < 3) return;

		fp32 s = _pRenderParams->m_CoordinateScale;
		for(int v = 0; v < nV; v++)
		{
			V[v].k[0] = (V[v].k[0] - _pRenderParams->m_CurLocalVP.k[0]) * s + _pRenderParams->m_CurLocalVP.k[0];
			V[v].k[1] = (V[v].k[1] - _pRenderParams->m_CurLocalVP.k[1]) * s + _pRenderParams->m_CurLocalVP.k[1];
			V[v].k[2] = (V[v].k[2] - _pRenderParams->m_CurLocalVP.k[2]) * s + _pRenderParams->m_CurLocalVP.k[2];
		}
	}

	// Render clipped poly with surface
	CXR_VertexBuffer* pVB = _pRenderParams->m_pVBM->Alloc_VB(CXR_VB_VERTICES | CXR_VB_TVERTICES0, nV);
	if (!pVB) 
		return;

	CXR_VBChain *pChain = pVB->GetVBChain();
	pChain->m_pN = _pRenderParams->m_pVBM->Alloc_V3(nV);
	if (!pChain->m_pN) return;

	memcpy(pChain->m_pV, V, sizeof(CVec3Dfp32)*nV);
	memcpy(pChain->m_pTV[0], TV, sizeof(CVec2Dfp32)*nV);

	for(int v = 0; v < nV; v++)
		pChain->m_pN[v] = CVec3Dfp32(0,0,-1);

	CXW_Surface* pSurf = NULL;
	if (_pKey->m_spSurface != NULL)
		pSurf = _pKey->m_spSurface->GetSurface(_pRenderParams->m_pEngine->m_SurfOptions, _pRenderParams->m_pEngine->m_SurfCaps);

/*	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC) Error("Create", "No surface-context available.");
	pSurf = pSC->GetSurface(_SurfaceID);
*/
	if (!pSurf)
	{
		ConOut("(CXR_Model_Sky::Sky_RenderPoly_Surface) Surface not found: " + _pKey->m_SurfaceName);
		return;
	}

	CXW_SurfaceKeyFrame* pSurfKey = pSurf->GetFrame(0, _pRenderParams->m_pAnimState->m_AnimTime0);

	int nPrim = nV+3;
	uint16* piPrim = _pRenderParams->m_pVBM->Alloc_Int16(nPrim);
	if (!piPrim) return;

	int iP = 0;
	piPrim[iP++] = CRC_RIP_TRIFAN + ((nV + 2) << 8);
	piPrim[iP++] = nV;
	for(int i = 0; i < nV; i++)
		piPrim[iP++] = i;
	piPrim[iP++] = CRC_RIP_END + (1 << 8);

	pVB->Render_IndexedPrimitives(piPrim, iP);

	CXR_RenderSurfExtParam Params;
	Params.m_DepthFogScale = _pKey->m_DepthFogScale;

	Params.m_DepthFogScale = 0.60f;

	int Flags = RENDERSURFACE_DEPTHFOG;// | RENDERSURFACE_VERTEXFOG;
	CXR_Util::Render_Surface(Flags, _pRenderParams->m_pAnimState->m_AnimTime0, pSurf, pSurfKey, _pRenderParams->m_pEngine, _pRenderParams->m_pVBM, (CMat4Dfp32*)NULL, (CMat4Dfp32*)NULL, (const CMat4Dfp32*)_pRenderParams->m_pL2VMat, pVB, _Priority, 0.0001f, &Params);
}



void CXR_Model_Sky::Sky_Cube(CSky_RenderInstanceParamters* _pRenderParams, const CPlane3Dfp32* _pPlanes, int _nPlanes, CSkyLayerKey* _pKey)
{
	MAUTOSTRIP(CXR_Model_Sky_Sky_Cube, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_Sky::Sky_Cube); //AR-SCOPE

	// Generate cube-vertices.
	bool bHalf = _pKey->m_Flags & 1;
//	bool bRelHead = (_pKey->m_Flags & 2) != 0;
	bool bZFlip = (_pKey->m_Flags & 4) != 0;
//	fp32 Size = _pKey->m_Params[1];
	fp32 SizeLow = _pRenderParams->m_pVBM->Viewport_Get()->GetBackPlane() / M_Sqrt(3);
	fp32 SizeTop = SizeLow; //*0.4f;
	fp32 z1 = SizeTop;
	fp32 z2 = (bHalf) ? SizeLow * 0.0f : -SizeLow;

	if (bZFlip) z1 = -z1;
	if (bZFlip) z2 = -z2;

//	if (bZFlip) ConOut("Flipbox");

//	_pRenderParams->m_CoordinateScale = 0.95f * m_pVBM->Viewport_Get()->GetBackPlane() / Length3(Size, Size, Size);
	_pRenderParams->m_CoordinateScale = 1.0f;

//	const fp32 TCScale = 1.0f/(_pKey->m_Params[1]*2.0f);
	CVec3Dfp32 Cube[8];
	Cube[0].k[0] = -SizeTop;	Cube[0].k[1] = -SizeTop;	Cube[0].k[2] = z1;
	Cube[1].k[0] = -SizeTop;	Cube[1].k[1] = SizeTop;	Cube[1].k[2] = z1;
	Cube[2].k[0] = SizeTop;	Cube[2].k[1] = SizeTop;	Cube[2].k[2] = z1;
	Cube[3].k[0] = SizeTop;	Cube[3].k[1] = -SizeTop;	Cube[3].k[2] = z1;

	Cube[4].k[0] = -SizeLow;	Cube[4].k[1] = -SizeLow;	Cube[4].k[2] = z2;
	Cube[5].k[0] = -SizeLow;	Cube[5].k[1] = SizeLow;	Cube[5].k[2] = z2;
	Cube[6].k[0] = SizeLow;	Cube[6].k[1] = SizeLow;	Cube[6].k[2] = z2;
	Cube[7].k[0] = SizeLow;	Cube[7].k[1] = -SizeLow;	Cube[7].k[2] = z2;

//	if (bRelHead) 
	for(int i = 0; i < 8; i++) Cube[i] += _pRenderParams->m_CurLocalVP;

	int Faces[6][5] = 
	{
		{3, 2, 1, 0, 2},
		{0, 1, 5, 4, 0},
		{1, 2, 6, 5, 1},
		{2, 3, 7, 6, 0},
		{3, 0, 4, 7, 1},	//3, 0, 4, 7
		{4, 5, 6, 7, 2},
	};

	CVec2Dfp32 TV[4];

	int nFaces = (bHalf) ? 5 : 6;
	for(int iFace = 0; iFace < nFaces; iFace++)
	{
		fp32 k = _pKey->m_Params[0];
		TV[0].k[0] = 0.0f; TV[0].k[1] = k;
		TV[1].k[0] = k; TV[1].k[1] = k;
		TV[2].k[0] = k; TV[2].k[1] = 0;
		TV[3].k[0] = 0.0f; TV[3].k[1] = 0;

		CVec3Dfp32 V[4];
		if (bHalf && (iFace >= 1))
		{
			TV[0].k[1] = k*0.5f;
			TV[1].k[1] = k*0.5f;
		}
		if (bZFlip && (iFace >= 1))
		{
			TV[0].k[1] = -TV[2].k[1];
			TV[1].k[1] = -TV[3].k[1];
		}

		V[0] = Cube[Faces[iFace][0]];
		V[1] = Cube[Faces[iFace][1]];
		V[2] = Cube[Faces[iFace][2]];
		V[3] = Cube[Faces[iFace][3]];
		Sky_RenderPoly_Surface(_pRenderParams, _pPlanes, _nPlanes, V, TV, 4, _pKey->m_Priority, _pKey);
	}

}

#define LERPF(a,b,t) ((a) + ((a) - (b)) * (t))

void CXR_Model_Sky::Sky_Plane(CSky_RenderInstanceParamters* _pRenderParams, const CPlane3Dfp32* _pPlanes, int _nPlanes, CSkyLayerKey* _pKey)
{
	MAUTOSTRIP(CXR_Model_Sky_Sky_Plane_2, MAUTOSTRIP_VOID);
#ifdef SKY_TEMP_CHEAPO
	const int SKYPLANEX = 2;
	const int SKYPLANEY = 2;
#else
	const int SKYPLANEX = 7;
	const int SKYPLANEY = 7;
#endif

	CVec3Dfp32 Sky[SKYPLANEY][SKYPLANEX];
	CVec2Dfp32 SkyTC[SKYPLANEY][SKYPLANEX];
	fp32 Alt = _pKey->m_Params[0];
	fp32 Size = _pKey->m_Params[1];
	fp32 Speed = _pKey->m_Params[3];

	_pRenderParams->m_CoordinateScale = 1.95f * _pRenderParams->m_pVBM->Viewport_Get()->GetBackPlane() / Length3(Size, Size, Alt);

//	fp32 SizeRecp = 1.0f / Size;
	fp32 SizeSqr = 2.0f*Size / (SKYPLANEX-1);
	int y;
	for(y = 0; y < SKYPLANEY; y++)
	{
		fp32 yp = -Size + y*SizeSqr;
		for(int x = 0; x < SKYPLANEX; x++)
		{
			Sky[y][x].k[0] = -Size + x*SizeSqr;
			Sky[y][x].k[1] = yp;

#ifdef SKY_TEMP_CHEAPO
			Sky[y][x].k[2] = Alt;
#else
			fp32 d = Length2(Sky[y][x].k[0], Sky[y][x].k[1]) * SizeRecp;
			d = Clamp01(d);

			Sky[y][x].k[2] = LERPF(Alt, 20000.0f, d);
#endif
		}
	}

//	int iSky = 0;
//	fp32 TCScale = 1.0f/(20.0f*1024.0f*fp32(1+_pKey->m_Params[2]*0.5f));
	fp32 TCScale = _pKey->m_Params[2];
//	fp32 TCScale = 1.0f/(20.0f*1024.0f*fp32(1+_pKey->m_Params[2]*0.5f));
	fp32 TCOfs = _pRenderParams->m_pAnimState->m_AnimTime0.GetTimeModulusScaled(Speed, 1.0f);

	for(y = 0; y < SKYPLANEY; y++)
		for(int x = 0; x < SKYPLANEX; x++)
		{
			SkyTC[y][x].k[0] = fp32(x) / fp32(SKYPLANEX-1) * TCScale + TCOfs;
			SkyTC[y][x].k[1] = fp32(y) / fp32(SKYPLANEY-1) * TCScale + TCOfs;
		}

//	Sky_RenderPoly(_pPlanes, _nP, Sky, SkyTC, 4);
//int BSPModel_CutFace2(CVec3Dfp32* _pVerts, int _nv, const CPlane3Dfp32* _pPlanes, int _np, 
//	int _bInvertPlanes, CVec2Dfp32* _pTVerts1 = NULL, CVec2Dfp32* _pTVerts2 = NULL, CVec2Dfp32* _pTVerts3 = NULL)

	for(y = 0; y < SKYPLANEY-1; y++)
		for(int x = 0; x < SKYPLANEX-1; x++)
		{
			CVec3Dfp32 V[4];
			CVec2Dfp32 TC[4];
			V[0] = Sky[y][x];
			V[1] = Sky[y][x+1];
			V[2] = Sky[y+1][x+1];
			V[3] = Sky[y+1][x];
			TC[0] = SkyTC[y][x];
			TC[1] = SkyTC[y][x+1];
			TC[2] = SkyTC[y+1][x+1];
			TC[3] = SkyTC[y+1][x];

			Sky_RenderPoly_Surface(_pRenderParams, _pPlanes, _nPlanes, V, TC, 4, _pKey->m_Priority, _pKey);
		}
}

CVec3Dfp32 CXR_Model_Sky::Sky_SpritePos(CSkyLayerKey* _pKey)
{
	MAUTOSTRIP(CXR_Model_Sky_Sky_SpritePos, CVec3Dfp32());
	CVec3Dfp32 Pos;
	if (_pKey->m_Flags & 1)
	{
		/* Method 1 - old method
		// Good for simple sun-like motion, fixed height might be a liability
		CVec3Dfp32 v(20000,0,0);
		RotateElements(v.k[0], v.k[2], -_pKey->m_Params[2] / _pKey->m_Params[3]);
		RotateElements(v.k[1], v.k[2], -0.125f);
		Pos = v;
		//*/

		//* Method 2 - direct positioning
		// Nice and simple, but not as intuitive as interpolating angles. LERP might also cause some problems.
		Pos = _pKey->m_Pos;
		Pos.Normalize();	// might be able to do some preprocessing here...
		Pos *= 20000.0f;
		//*/

		/* Method 3 - dual angle
		// Easier to do sun-like movement than method 2, but more flexible than method 1
		CVec3Dfp32 v(20000.0f,0,0);
		RotateElements(v.k[0],v.k[1], _pKey->m_Params[2]);
		Pos = v * _pKey->m_Pos[0];
		Pos.k[2] = M_Sqrt(20000.0f * 20000.0f - (Pos.k[0] * Pos.k[0] + Pos.k[1] * Pos.k[1]));
		CMat4Dfp32 Rotation;
		v.Normalize();
		v.CreateAxisRotateMatrix(_pKey->m_Params[3],Rotation);
		Pos.MultiplyMatrix(Rotation,v);
		Pos = v;
		//*/
	}
	else
		Pos = _pKey->m_Pos;
	return Pos;
}

CVec3Dfp32 CXR_Model_Sky::Sky_Sprite(CSky_RenderInstanceParamters* _pRenderParams, const CPlane3Dfp32* _pPlanes, int _nPlanes, CSkyLayerKey* _pKey, const CVec3Dfp32& _Pos)
{
	MAUTOSTRIP(CXR_Model_Sky_Sky_Sprite, CVec3Dfp32());
	CVec3Dfp32 Pos = _Pos;

	_pRenderParams->m_CoordinateScale = 0.95f * _pRenderParams->m_pVBM->Viewport_Get()->GetBackPlane() / 30000.0f;


	CVec3Dfp32 xv, yv, zv;
	if (_pKey->m_Flags & 4)
	{
		// Sprite
		xv = CVec3Dfp32::GetMatrixRow(_pRenderParams->m_L2VMatInv, 0);
		yv = CVec3Dfp32::GetMatrixRow(_pRenderParams->m_L2VMatInv, 1);
		zv = CVec3Dfp32::GetMatrixRow(_pRenderParams->m_L2VMatInv, 2);
	}
	else
	{
		// Billboard
		zv = (Pos - _pRenderParams->m_CurLocalVP).Normalize();
		xv = (zv / CVec3Dfp32(0,0, 1)).Normalize();
		yv = -(xv / zv).Normalize();
	}

	//Rotate
	{
		CMat4Dfp32	Mat;
		zv.CreateAxisRotateMatrix(_pKey->m_Params[2],Mat);
		xv.MultiplyMatrix3x3(Mat);
		yv.MultiplyMatrix3x3(Mat);
	}


	fp32 w,h;

	if (_pKey->m_Flags & 2)
	{
		const CMat4Dfp32 * Mat = _pRenderParams->m_pL2VMat;
		fp32 factor = Mat->k[0][2]*Pos.k[0] + Mat->k[1][2]*Pos.k[1] + Mat->k[2][2]*Pos.k[2] + Mat->k[3][2];
		w = _pKey->m_Params[0] * factor;
		h = _pKey->m_Params[1] * factor;
	}
	else
	{
		w = _pKey->m_Params[0] * 0.5f;
		h = _pKey->m_Params[1] * 0.5f;
	}

	CVec3Dfp32 V[4];
	V[0] = Pos;
	V[1] = Pos;
	V[2] = Pos;
	V[3] = Pos;

	V[0] += (xv*-w + yv*-h);
	V[1] += (xv*w + yv*-h);
	V[2] += (xv*w + yv*h);
	V[3] += (xv*-w + yv*h);

/*	V[1] += CVec3Dfp32(0, w, 0);
	V[2] += CVec3Dfp32(0, w, h);
	V[3] += CVec3Dfp32(0, 0, h);*/

	CVec2Dfp32 TV[4];
	int k = 1;
	TV[0].k[0] = 0.0f; TV[0].k[1] = 0.0f;
	TV[1].k[0] = k; TV[1].k[1] = 0.0f;
	TV[2].k[0] = k; TV[2].k[1] = k;
	TV[3].k[0] = 0.0f; TV[3].k[1] = k;


//CPixel32 Col(_pKey->m_Colors[0].GetR() >> 1, _pKey->m_Colors[0].GetG() >> 1, _pKey->m_Colors[0].GetB() >> 1, _pKey->m_Colors[0].GetA());
	Sky_RenderPoly_Surface(_pRenderParams, _pPlanes, _nPlanes, V, TV, 4, _pKey->m_Priority, _pKey);

	return Pos;
}

void CXR_Model_Sky::PreRenderSky(CSky_RenderInstanceParamters* _pRenderParams, CSkySurface* _pSky, CMTime _Time)
{
	MAUTOSTRIP(CXR_Model_Sky_PreRenderSky, MAUTOSTRIP_VOID);
	if (_pRenderParams->m_CurrentPass != 0) return;

	// Render sky-layers
	for(int i = 0; i < _pSky->m_lspLayers.Len(); i++)
	{
		CSkyLayer* pL = _pSky->m_lspLayers[i];
		CSkyLayerKey Key;
		pL->GetValue(_Time, Key);

		// Skip layer if alpha == 0.
		if (Key.m_Colors[0].GetA() == 0) continue;
		switch(pL->m_Type)
		{
		case SKYLAYER_TYPE_FOGCOLOR :
			{
				if (_pRenderParams->m_pEngine)
				{
					CXR_FogState* pFog = _pRenderParams->m_pEngine->m_pCurrentFogState;
					if (!pFog) break;
					if (_pRenderParams->m_pAnimState && (_pRenderParams->m_pAnimState->m_Anim0 & 1)) break;

					pFog->VertexFog_Init(_pRenderParams->m_VMatInv, 2048, 1024, Key.m_Colors[0], pFog->m_VtxFog_ReferenceHeight);
					pFog->DepthFog_Init(100000, 100010, Key.m_Colors[0]);
				}
			}
			break;
		case SKYLAYER_TYPE_LIGHTSTATE :
			{
				if (_pRenderParams->m_pEngine) _pRenderParams->m_pEngine->GetVC()->GetLightState()->Set((int)Key.m_Params[0], CVec3Dfp32(Key.m_Colors[0].GetR(), Key.m_Colors[0].GetG(), Key.m_Colors[0].GetB()) * (1/255.0f));
			}
			break;
		}

	}
}

static int RemoveColinearPoints(const CVec3Dfp32* _pV, CVec3Dfp32* _pDest, int _nV)
{
	MAUTOSTRIP(RemoveColinearPoints, 0);
	int nVerts = 0;
	for(int v = 0; v < _nV; v++)
	{
		int iv0 = (v + _nV-1) % _nV;
		int iv1 = v;
		int iv2 = (v+1) % _nV;
		CVec3Dfp32 e0, e1, n;
		_pV[iv2].Sub(_pV[iv1], e1);
		_pV[iv1].Sub(_pV[iv0], e0);
		e1.CrossProd(e0, n);
		if (n.LengthSqr() > 0.01f)
			_pDest[nVerts++] = _pV[iv1];
	}
	return nVerts;
}

void CXR_Model_Sky::RenderSky(CSky_RenderInstanceParamters* _pRenderParams, const CVec3Dfp32* _pV, const uint32* _piV, int _nVertices, CSkySurface* _pSky, CMTime _Time)
{
	MAUTOSTRIP(CXR_Model_Sky_RenderSky, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_Sky::RenderSky); //AR-SCOPE

	CRC_Attributes* pCurrentAttrib	= _pRenderParams->m_pVBM->Alloc_Attrib();
	if(!pCurrentAttrib)
		return;
	pCurrentAttrib->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
	
	CPlane3Dfp32 CutPlanes[16];
	int nPlanes = 0;

	if (_nVertices > 16)
	{
		ConOut("(CXR_Model_Sky::RenderSky) Too many cut-planes.");
		return;
	}

	// Create all clipping planes.
	{
		int nv = _nVertices;
		int iv2 = _piV[nv-1];
		for(int i = 0; i < nv; i++)
		{
			int iv = _piV[i];
			CutPlanes[nPlanes].Create(_pRenderParams->m_CurLocalVP, _pV[iv2], _pV[iv]);

			if (_pRenderParams->m_L2VMatInv.IsMirrored()) CutPlanes[nPlanes].Inverse();
//			CutPlanes[nPlanes].Inverse();
			nPlanes++;
			iv2 = iv;
		}

		// Front clipping plane.
		{
			CVec3Dfp32 n = CVec3Dfp32::GetMatrixRow(_pRenderParams->m_L2VMatInv, 2);
			CutPlanes[nPlanes].CreateNV(n, _pRenderParams->m_CurLocalVP + n*16.0f);
//			if (_pRenderParams->m_L2VMatInv.IsMirrored()) CutPlanes[nPlanes].Inverse();
//			CutPlanes[nPlanes].Inverse();
			nPlanes++;
		}
	}
	

	// Render sky-layers
	for(int i = 0; i < _pSky->m_lspLayers.Len(); i++)
	{
		CSkyLayer* pL = _pSky->m_lspLayers[i];
		CSkyLayerKey Key;
		pL->GetValue(_Time, Key);

		// Skip layer if alpha == 0.
		if (Key.m_Colors[0].GetA() == 0) continue;
//		Key.m_Color = CPixel32(Key.m_Color.GetR() >> 1, Key.m_Color.GetG() >> 1, Key.m_Color.GetB() >> 1, Key.m_Color.GetA());

		switch(pL->m_Type)
		{
		case SKYLAYER_TYPE_SKY :
			{
				if (_pRenderParams->m_CurrentPass == 0)
				{
//					ConOutL(CStr("Sky "));
					Sky_Plane(_pRenderParams, CutPlanes, nPlanes, &Key);
				}
			}
			break;
		case SKYLAYER_TYPE_ENVBOX :
			{
				if (_pRenderParams->m_CurrentPass == 0)
				{
//					ConOutL(CStr("Sky "));
					Sky_Cube(_pRenderParams, CutPlanes, nPlanes, &Key);
				}
			}
			break;

		case SKYLAYER_TYPE_BOX:
			{
				if (_pRenderParams->m_CurrentPass == 0)
				{
//					ConOutL(CStr("Sky "));
					RenderSkyBox(_pRenderParams, CutPlanes, nPlanes, &Key);
				}
			}
			break;

		case SKYLAYER_TYPE_SPRITE :
			{
				switch(_pRenderParams->m_CurrentPass)
				{
				case 0 :
					{
						CVec3Dfp32 Pos = Sky_SpritePos(&Key);
						Sky_Sprite(_pRenderParams, CutPlanes, nPlanes, &Key, Pos);
					}
					break;

				case 1 :
					{
						if (Key.m_Flags & 8)
							if (_pRenderParams->m_pEngine /*&& (m_spFlare != NULL)*/)
							{
								CVec3Dfp32 Pos = Sky_SpritePos(&Key);
		/*						Pos.Normalize();
								Pos *= 2000.0f;
								Pos *= m_L2VMat;
								Pos *= m_VMatInv;
								CMat4Dfp32 M;
								M.Unit();
								Pos.SetMatrixRow(M, 3);*/

//								CXR_VBManager* pVBM = _pRenderParams->m_pEngine->GetVBM();
								CXR_VertexBuffer* pVB = _pRenderParams->m_pVBM->Alloc_VB(CXR_VB_ATTRIB);
								if (!pVB) continue;

								pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
								pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
								pVB->m_pAttrib->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
//								pVB->m_pAttrib->Attrib_TextureID(0, Key.m_TextureID);
								pVB->m_Priority = CXR_VBPRIORITY_FLARE;

								CXR_Particle4 Particles[1];
								Pos.Normalize();
								Pos *= _pRenderParams->m_pVBM->Viewport_Get()->GetBackPlane() - 10;
								Particles[0].m_Pos = Pos;
								Particles[0].m_Dimensions.k[0] = 1200;
								Particles[0].m_Dimensions.k[1] = 1200;
								Particles[0].m_Color = 0xff7f7f7f;

		// PROBLEM: Sky är ett renderpass= -1 objekt, vilket innebär att det rendras före portaler och 
		//			att VBM'en flushas innan resten av scenen ritas. Mao så hamnar flaren först och inte sist. :(
		//			Man kanske skulle slänga in sky-modellen i CXR_Engine två ggr, fast med olika renderpass.
		//			Vilket renderpass som man ritar kan man lägga i animstate så att modellen vet vad som skall ritas.

								if (CXR_Util::Render_Flares(_pRenderParams->m_pRC, _pRenderParams->m_pVBM, pVB, _pRenderParams->m_L2VMat, Particles, 1, 10000, 0, 5, false))
								{
									_pRenderParams->m_pVBM->AddVB(pVB);
								}

		//						m_FlareAnim.m_Colors[0] = CPixel32(Key.m_Color.GetR(), Key.m_Color.GetG(), Key.m_Color.GetB(), Key.m_Color.GetA() * 0.75f);
		//						m_pEngine->Render_AddModel(m_spFlare, M, m_FlareAnim);
							}
					}
				}
			}
			break;
		case SKYLAYER_TYPE_FOGCOLOR :
			{
break;
				if (_pRenderParams->m_CurrentPass != 0) continue;
				if (_pRenderParams->m_pVolumeFog) _pRenderParams->m_pVolumeFog->m_DepthFogColor = Key.m_Colors[0];

				CVec3Dfp32 vx = CVec3Dfp32::GetMatrixRow(_pRenderParams->m_L2VMatInv, 0) * 10000.0f;
				CVec3Dfp32 vy = CVec3Dfp32::GetMatrixRow(_pRenderParams->m_L2VMatInv, 1) * 10000.0f;
				CVec3Dfp32 Poly[4];
				Poly[0] = Poly[1] = Poly[2] = Poly[3]= _pRenderParams->m_CurLocalVP + CVec3Dfp32::GetMatrixRow(_pRenderParams->m_L2VMatInv, 2)*32.0f;
				Poly[0] += vx;
				Poly[1] += vy;
				Poly[2] -= vx;
				Poly[3] -= vy;
				pCurrentAttrib->Attrib_TextureID(0, 0);
				pCurrentAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);
//				Sky_RenderPoly(CutPlanes, nPlanes, Poly, (CVec2Dfp32*) &Poly, 4, Key.m_Colors[0]);


				Sky_UpdateFog(_pRenderParams);
				// FIXME:
//				if (_pRenderParams->m_pVolumeFog) 
//					Sky_RenderPoly(CutPlanes, 0*nPlanes, _pV, (CVec2Dfp32*)_pV, _nVertices, _pRenderParams->m_pVolumeFog->m_DepthFogColor);
				//	m_pRC->Render_Polygon(_nVertices, _pV, (CVec2Dfp32*)_pV, g_IndexRamp32, g_IndexRamp32, _pRenderParams->m_pVolumeFog->m_DepthFogColor);
			}
			break;
		case SKYLAYER_TYPE_LIGHTSTATE :
			{
break;
/*				if (_pRenderParams->m_CurrentPass != 0) continue;
				if (m_pEngine) m_pEngine->GetVC()->m_spLightState->Set(Key.m_Params[0], CVec3Dfp32(Key.m_Colors[0].GetR(), Key.m_Colors[0].GetG(), Key.m_Colors[0].GetB()) * (1/255.0f));
				if (m_spCloudTxt!=NULL)
				{
					m_spCloudTxt->SetDiffuse(Key.m_Colors[0]);
					m_spCloudTxt->SetSpecular(Key.m_Colors[0] * 0.5f);
				}
*/
			}
			break;
		}
	}
}

void CXR_Model_Sky::Sky_UpdateFog(CSky_RenderInstanceParamters* _pRenderParams)
{
	MAUTOSTRIP(CXR_Model_Sky_Sky_UpdateFog, MAUTOSTRIP_VOID);
#ifdef NEVER
	if (!_pRenderParams->m_pVolumeFog) return;
	CPixel32 FogColor = _pRenderParams->m_pVolumeFog->m_DepthFogColor;
	if (m_LastFogColor == int(FogColor)) return;
	m_LastFogColor = FogColor;
	
/*	fp32 t = GetCPUClock() / GetCPUFrequency();
	if (t - m_LastFogUpdate < SKY_FOGFREQUENCY) return;
	m_LastFogUpdate = t;*/

	CImage* pFog = m_spProcTxt->GetTexture(0, 0);
	uint32* pBitmap = (uint32*) pFog->Lock();
	int Modulo = pFog->GetModulo() / sizeof(uint32);

	const fp32 Scale = 10000.0f;
	const fp32 Size = 2.0f;
	fp32 Step = Size / (SKY_FOGSIZE-1);

	CVec3Dfp32 Eye = CVec3Dfp32::GetMatrixRow(_pRenderParams->m_pVolumeFog->m_Eye, 3);
//	CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(_pRenderParams->m_pVolumeFog->m_Eye, 3);
//	CVec3Dfp32 Pos = m_CurCamera;

	CMat4Dfp32 Unit; Unit.Unit();
	CVec3Dfp32::GetMatrixRow(Unit, 3) = Eye;
	_pRenderParams->m_pVolumeFog->SetTransform(&Unit);

	CVec3Dfp32 Pos(0);
	Pos.k[0] -= (Size * 0.5f); //+ _pRenderParams->m_CurLocalVP.k[0];
	Pos.k[1] -= (Size * 0.5f); //+ _pRenderParams->m_CurLocalVP.k[1];
//	Pos.k[2] = 500; + _pRenderParams->m_CurLocalVP.k[2];

//	fp32 FogEndRecp = 1.0f / _pRenderParams->m_pVolumeFog->m_DepthFogEnd;
	fp32 FogEndRecp = 1.0f / 2048.0f;

	CVec3Dfp32 P;
	P.k[2] = 500;
	for(int y = 0; y < pFog->GetHeight(); y++)
	{
		CVec3Dfp32 PosX = Pos;
		uint32* pF = &pBitmap[Modulo*y];
		for(int x = 0; x < pFog->GetWidth(); x++)
		{
//			pF[x] = _pRenderParams->m_pVolumeFog->Trace((((*pS)-_pRenderParams->m_CurLocalVP)*_Scale) + _pRenderParams->m_CurLocalVP);
			P.k[0] = PosX.k[0] * Scale;
			P.k[1] = PosX.k[1] * Scale;
			fp32 Dist = Length2(P.k[0], P.k[1]);
			fp32 FogDist = Dist * FogEndRecp;
			fp32 Fog = (M_Sqrt(FogDist) + FogDist) * 0.5f;
			pF[x] = CPixel32(FogColor.GetR(), FogColor.GetG(), FogColor.GetB(), RoundToInt(Fog*255.0f));

//			pF[x] = _pRenderParams->m_pVolumeFog->Trace(P);
//			pF[x] = 0xff00ff00;
			PosX.k[0] += Step;
		}
		Pos.k[1] += Step;
	}

	pFog->Unlock();

//pFog->Write(pFog->GetRect(), "E:\\TEST\\FOG.TGA");

	int TxtID = m_spProcTxt->GetTextureID(0);
	if (TxtID)
	{
//		ConOut(CStrF("Dirty %d, %.8x", TxtID, pFogState->m_DepthFogColor));
		m_spProcTxt->GetTextureContext()->MakeDirty(TxtID);
	}
#endif
}

void CXR_Model_Sky::PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	if (m_spSky == NULL) return;

	CSky_RenderInstanceParamters RenderParams(_pEngine, _pEngine->GetVBM(), _pAnimState);
	CSky_RenderInstanceParamters* _pRenderParams = &RenderParams;
	_pRenderParams->m_CurrentPass = (_pRenderParams->m_pAnimState) ? _pRenderParams->m_pAnimState->m_Anim1 : -1;

	// TIMEFIX: Should animtime be divided by 20 ? and shouldn't CPUTime be also ?
//	CMTime Time = (m_pAnimState) ? m_pAnimState->m_AnimTime0 : (CMTime::GetCPU());
	CMTime Time;
	if( _pRenderParams->m_pAnimState )
		Time = _pRenderParams->m_pAnimState->m_AnimTime0;
	else
		Time.Snapshot();

	_VMat.InverseOrthogonal(_pRenderParams->m_VMatInv);
	_pRenderParams->m_CurCamera = CVec3Dfp32::GetMatrixRow(_pRenderParams->m_VMatInv, 3);

	PreRenderSky(_pRenderParams, m_spSky, Time);

//	PreRenderSky(m_spSky, (_pAnimState) ? _pAnimState->m_AnimTime0 : 0.0f);
//	Sky_UpdateFog(_pEngine);
}

// From XREngine.cpp
void TransformClipVolume(CRC_ClipVolume& _Clip, const CMat4Dfp32& _Mat);

void CXR_Model_Sky::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MSCOPESHORT(CXR_Model_Sky::OnRender); //AR-SCOPE

	if (!_pAnimState) return;
	if (!_pEngine) return;

	CSky_RenderInstanceParamters RenderParams(_pEngine, _pVBM, _pAnimState);
	CSky_RenderInstanceParamters* _pRenderParams = &RenderParams;
	RenderParams.m_pRC	= _pRender;

	_pRenderParams->m_pVolumeFog = (_pRenderParams->m_pEngine) ? _pRenderParams->m_pEngine->m_pCurrentFogState : NULL;
//m_pVolumeFog = NULL;
	_pRenderParams->m_CurrentPass = (_pRenderParams->m_pAnimState) ? _pRenderParams->m_pAnimState->m_Anim1 : -1;

/*	if (m_CurrentPass & 0x0100)
	{
		RenderSkyBox();
		return;
	}
	else*/
	{
		CMat4Dfp32 L2WMat, W2LMat, Camera;

		L2WMat.Unit();
		_VMat.InverseOrthogonal(Camera);
		CVec3Dfp32::GetMatrixRow(Camera, 3).SetMatrixRow(L2WMat, 3);
		L2WMat.InverseOrthogonal(W2LMat);

		CMat4Dfp32 L2VMat, L2VMatInv;
		L2WMat.Multiply(_VMat, L2VMat);
		L2VMat.InverseOrthogonal(L2VMatInv);

		_pRenderParams->m_L2VMat = L2VMat;
		_pRenderParams->m_L2VMatInv = L2VMatInv;

		_pRenderParams->m_CurLocalVP = CVec3Dfp32::GetMatrixRow(L2VMatInv, 3);

//		CMTime Time = (m_pAnimState) ? m_pAnimState->m_AnimTime0 : (CMTime::GetCPU());
		CMTime Time;
		if( _pRenderParams->m_pAnimState )
			Time = _pRenderParams->m_pAnimState->m_AnimTime0;
		else
			Time.Snapshot();

/*		{
			_pRender->Matrix_Push();
			_pRender->Matrix_Set(_VMat);
			CRC_ClipVolume Clip;
			Clip.Copy(m_pEngine->GetVC()->m_ClipW);
			for(int v = 0;v < Clip.m_nPlanes; v++)
				_pRender->Render_Wire(Clip.m_Vertices[v], Clip.m_Vertices[(v+1) % Clip.m_nPlanes], 0xff00ffff);

			_pRender->Matrix_Pop();
		}*/


		CRC_ClipVolume Clip;
		if (_pRenderParams->m_pEngine)
			Clip.CopyAndTransform(_pRenderParams->m_pEngine->GetVC()->m_ClipW, W2LMat);
		else
		{
			CRC_Viewport VP(*_pRenderParams->m_pVBM->Viewport_Get());
			VP.GetClipVolume(Clip, &_pRenderParams->m_VMatInv);
			Clip.Transform(L2WMat);
		}

		_pRenderParams->m_pL2VMat = _pRenderParams->m_pVBM->Alloc_M4(_pRenderParams->m_L2VMat);

//		_pRender->Attrib_Push();
//		_pRender->Matrix_Push();
//		_pRender->Matrix_Set(m_L2VMat);
//		_pRender->Attrib_Disable(CRC_FLAGS_ZWRITE);


//		if (!(_pAnimState->m_Anim0 & 2))	// 2 says it is a indoor sky. If it isn't we render everything.
			Sky_OpenFrustrum(_pEngine);

/*		if (m_CurrentPass & 0x0100 == 0)
		{
			RenderSky(Clip.m_Vertices, g_IndexRamp32, 
				Clip.m_nPlanes, m_spSky, Time);
		}
		else*/
		{
			CSky_ViewContext* pSkyVC = Sky_GetVC(_pRenderParams->m_pEngine);

			_pRenderParams->m_CurrentPass &= 0xff;
			const fp32 Scale = 16.0f;
			for(int f = 0; f < pSkyVC->m_nBoxFaces; f++)
			{
				CSkyBoxFace* pF = &pSkyVC->m_lBoxFaces[f];

	//			CMat4Dfp32 Mat;
	//			pF->m_F2LMat.Multiply(m_BoxL2VMat, Mat);

				CVec2Dfp32 vmin, vmax;
				vmin[0] = Max(-1.0f, pF->m_MinVis[0]);
				vmin[1] = Max(-1.0f, pF->m_MinVis[1]);
				vmax[0] = Min(1.0f, pF->m_MaxVis[0]);
				vmax[1] = Min(1.0f, pF->m_MaxVis[1]);

				if (vmin[0] >= vmax[0]) continue;
				if (vmin[1] >= vmax[1]) continue;

				CVec3Dfp32 FVerts[4];
				FVerts[0][1] = vmin[0] * Scale;
				FVerts[0][2] = vmin[1] * Scale;
				FVerts[0][0] = 1 * Scale;
				FVerts[1][1] = vmax[0] * Scale;
				FVerts[1][2] = vmin[1] * Scale;
				FVerts[1][0] = 1 * Scale;
				FVerts[2][1] = vmax[0] * Scale;
				FVerts[2][2] = vmax[1] * Scale;
				FVerts[2][0] = 1 * Scale;
				FVerts[3][1] = vmin[0] * Scale;
				FVerts[3][2] = vmax[1] * Scale;
				FVerts[3][0] = 1 * Scale;

				CVec3Dfp32 LVerts[32];
				CVec3Dfp32 LVerts2[32];
				CVec3Dfp32::MultiplyMatrix(FVerts, LVerts, pF->m_F2LMat, 4);

//			if (_pEngine->GetVCDepth()) ConOut(CStrF("Clipping face %d", f));

//				int nv = CutFence(LVerts, Clip.m_nPlanes, Clip.m_Planes, 4, true ^ m_L2VMatInv.IsMirrored());
				int nv = 4;
				nv = RemoveColinearPoints(LVerts, LVerts2, nv);
				if (nv > 2)
				{
//			if (_pEngine->GetVCDepth()) ConOut(CStrF("Rendering face %d, nV %d", f, nv));
					RenderSky(_pRenderParams, LVerts2, g_IndexRamp32, nv, m_spSky, Time);
				}
			}
		}

//		_pRender->Matrix_Pop();
//		_pRender->Attrib_Pop();
	}
}
