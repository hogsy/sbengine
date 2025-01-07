#ifndef _INC_XRSky
#define _INC_XRSky

#include "../../XR/XREngine.h"
#include "../../MSystem/Raster/MTextureContainers.h"

#define SKY_CLOUDFREQUENCY (1.0f/15.0f)
#define SKY_FOGFREQUENCY (1.0f/12.0f)

class CTextureContainer_Sky : public CTextureContainer_Plain
{
public:

};

typedef TPtr<CTextureContainer_Sky> spCTextureContainer_Sky;

#ifdef NEVER
class CTextureContainer_Clouds : public CTextureContainer
{
protected:
	int m_TextureID;
	CImage m_Properties;

	int m_CloudCover;
	CPixel32 m_Diffuse;
	CPixel32 m_Specular;

	CMTime m_LastUpdate;

	TArray<spCImage> m_lspNoise1;
	TArray<spCImage> m_lspNoise2;
	TArray<spCImage> m_lspNoiseTemp;
	spCImage m_spNoiseComp;
	spCImage m_spNoiseShade;

public:
	CTextureContainer_Clouds()
	{
		m_TextureID = m_pTC->AllocID(m_iTextureClass, 0, "PROCEDURALCLOUDS0");
		m_Properties.CreateVirtual(256, 256, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);

		m_CloudCover = 192;
		m_LastUpdate = 0;
	}

	void Create()
	{
		m_lspNoise1.SetLen(4);
		m_lspNoise2.SetLen(4);
		m_lspNoiseTemp.SetLen(4);
		for(int i = 0; i < 4; i++)
		{
			m_lspNoise1[i] = DNew(CImage) CImage;
			m_lspNoise2[i] = DNew(CImage) CImage;
			m_lspNoiseTemp[i] = DNew(CImage) CImage;
			if (!m_lspNoise1[i] || !m_lspNoise2[i] || !m_lspNoiseTemp[i]) MemError("Create");

			m_lspNoiseTemp[i]->Create(256 >> i, 256 >> i, IMAGE_FORMAT_I8, IMAGE_MEM_IMAGE);
		}

		m_spNoiseComp = DNew(CImage) CImage;
		if (!m_spNoiseComp) MemError("Create");
		m_spNoiseComp->Create(256, 256, IMAGE_FORMAT_I8, IMAGE_MEM_IMAGE);

		m_spNoiseShade = DNew(CImage) CImage;
		if (!m_spNoiseShade) MemError("Create");
		m_spNoiseShade->Create(256, 256, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);

		CStr Path = "S:\\Surfaces\\World\\Sky\\Clouds\\";
		m_lspNoise1[0]->Read(CRct(0,0,-1,-1), Path + "Noise1_0.TGA", IMAGE_MEM_IMAGE);
		m_lspNoise1[1]->Read(CRct(0,0,-1,-1), Path + "Noise1_1.TGA", IMAGE_MEM_IMAGE);
		m_lspNoise1[2]->Read(CRct(0,0,-1,-1), Path + "Noise1_2.TGA", IMAGE_MEM_IMAGE);
		m_lspNoise1[3]->Read(CRct(0,0,-1,-1), Path + "Noise1_3.TGA", IMAGE_MEM_IMAGE);
		m_lspNoise2[0]->Read(CRct(0,0,-1,-1), Path + "Noise2_0.TGA", IMAGE_MEM_IMAGE);
		m_lspNoise2[1]->Read(CRct(0,0,-1,-1), Path + "Noise2_1.TGA", IMAGE_MEM_IMAGE);
		m_lspNoise2[2]->Read(CRct(0,0,-1,-1), Path + "Noise2_2.TGA", IMAGE_MEM_IMAGE);
		m_lspNoise2[3]->Read(CRct(0,0,-1,-1), Path + "Noise2_3.TGA", IMAGE_MEM_IMAGE);
	}

	void SetDiffuse(CPixel32 _Color)
	{
		m_Diffuse = _Color;
	}

	void SetSpecular(CPixel32 _Color)
	{
		m_Specular = _Color;
	}

	~CTextureContainer_Clouds()
	{
		if (m_TextureID) m_pTC->FreeID(m_TextureID);
	}

	virtual int GetNumLocal()
	{
		return 1;
	}

	virtual CStr GetName(int _iLocal)
	{
		return CStrF("PROCEDURALCLOUDS%d", _iLocal);
	}

	virtual int GetLocal(const char* _pName)
	{
		if (CStrBase::stricmp(_pName, "PROCEDURALCLOUDS0") == 0) return 0;
		return -1;
	}

	virtual int GetTextureID(int _iLocal)
	{
		return m_TextureID;	
	};

	virtual int GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
	{
		if (_iLocal != 0)
			Error("GetTextureDesc", CStrF("Invalid local ID %d", _iLocal));

		_pTargetImg->CreateVirtual(m_Properties.GetWidth(), m_Properties.GetHeight(), m_Properties.GetFormat(), IMAGE_MEM_IMAGE);
		_Ret_nMipmaps = 1;
		return 0;
	}

	virtual void GetTextureProperties(int _iLocal, CTC_TextureProperties& _Prop)
	{
		CTC_TextureProperties Prop;
		Prop.m_Flags = CTC_TEXTUREFLAGS_NOMIPMAP | 0;
		_Prop = Prop;
	}

	virtual void OnRefresh()
	{
		CMTime t = CMTime::GetCPU();
		if ((t - m_LastUpdate).GetTime() > SKY_CLOUDFREQUENCY)
		{
			CTextureContainer::OnRefresh();
			if (m_TextureID) m_pTC->MakeDirty(m_TextureID);
		}
	}

	spCImage CreateNoise(int _Seed)
	{
	}

	static void Blend(const uint8* _pSrc1, const uint8* _pSrc2, uint8* _pDst, int _nValues, int _Blend)
	{
		for(int i = 0; i < _nValues; i++)
		{
			int s0 = _pSrc1[i];
			int s1 = _pSrc2[i];
			_pDst[i] = s0 + (((s1 - s0) * _Blend) >> 8);
		}
	}

	static void Blend(CImage* _pSrc1, CImage* _pSrc2, CImage* _pDst, int _Blend)
	{
		if (_pSrc1->GetWidth() != _pSrc2->GetWidth() ||
			_pSrc1->GetWidth() != _pDst->GetWidth()) return;
		if (_pSrc1->GetHeight() != _pSrc2->GetHeight() ||
			_pSrc1->GetHeight() != _pDst->GetHeight()) return;
		if (_pSrc1->GetFormat() != _pSrc2->GetFormat() ||
			_pSrc1->GetFormat() != _pDst->GetFormat()) return;

		int _nBytes = _pSrc1->GetWidth() * _pSrc1->GetHeight() * _pSrc1->GetPixelSize();
		uint8* pSrc1 = (uint8*)_pSrc1->Lock();
		uint8* pSrc2 = (uint8*)_pSrc2->Lock();
		uint8* pDst = (uint8*)_pDst->Lock();
		Blend(pSrc1, pSrc2, pDst, _nBytes, _Blend);
		_pSrc1->Unlock();
		_pSrc2->Unlock();
		_pDst->Unlock();
	}

	void CompositeNoise(TArray<spCImage> _lspNoise1, TArray<spCImage> _lspNoise2, TArray<spCImage> _lspTemp, CImage* _pDst)
	{
		if (!_lspNoise1.Len()) return;
		if (!m_spNoiseComp) return;

		CMTime t = CMTime::GetCPU();
		m_LastUpdate = t;

		m_CloudCover = M_Sin(t.GetModulusScaled(0.1f, _PI2)) * 32.0f + 160.0f;
		int CloudCover = 255 - m_CloudCover;

		{
			for(int i = 0; i < _lspNoise1.Len(); i++)
				Blend(_lspNoise1[i], _lspNoise2[i], _lspTemp[i], 128.0f + 128.0f*M_Sin(2.0f*t / (7 - i*2)));
		}

		const uint8* pN0 = (uint8*)_lspTemp[0]->Lock();
		const uint8* pN1 = (uint8*)_lspTemp[1]->Lock();
		const uint8* pN2 = (uint8*)_lspTemp[2]->Lock();
		const uint8* pN3 = (uint8*)_lspTemp[3]->Lock();
		uint8* pND = (uint8*)_pDst->Lock();

		int xAnd0 = _lspNoise1[0]->GetWidth()-1;
		int yAnd0 = _lspNoise1[0]->GetHeight()-1;
		int xAnd1 = _lspNoise1[1]->GetWidth()-1;
		int yAnd1 = _lspNoise1[1]->GetHeight()-1;
		int xAnd2 = _lspNoise1[2]->GetWidth()-1;
		int yAnd2 = _lspNoise1[2]->GetHeight()-1;
		int xAnd3 = _lspNoise1[3]->GetWidth()-1;
		int yAnd3 = _lspNoise1[3]->GetHeight()-1;
		int yModShift0 = Log2(_lspNoise1[0]->GetWidth());
		int yModShift1 = Log2(_lspNoise1[1]->GetWidth());
		int yModShift2 = Log2(_lspNoise1[2]->GetWidth());
		int yModShift3 = Log2(_lspNoise1[3]->GetWidth());
		int yModShiftD = Log2(_pDst->GetWidth());

		int w = _pDst->GetWidth();
		int h = _pDst->GetHeight();
		for(int y = 0; y < h; y++)
		{
			int y0 = (y & yAnd0) << yModShift0;
			int y1 = (y & yAnd1) << yModShift1;
			int y2 = (y & yAnd2) << yModShift2;
			int y3 = (y & yAnd3) << yModShift3;
			int yd = y  << yModShiftD;
			for(int x = 0; x < w; x++)
			{
				int i0 = (x & xAnd0) + y0;
				int i1 = (x & xAnd1) + y1;
				int i2 = (x & xAnd2) + y2;
				int i3 = (x & xAnd3) + y3;

				int Noise = ((uint32(pN0[i0]) << 3) + (uint32(pN1[i1]) << 2) + (uint32(pN2[i2]) << 1) + (uint32(pN3[i3]))) >> 4;
				Noise -= CloudCover;
				if (Noise < 0)
					Noise = 0;
				else if (Noise > 255)
					Noise = 255;

				pND[yd + x] = Noise;
			}
		}

		_lspNoise1[0]->Unlock();
		_lspNoise1[1]->Unlock();
		_lspNoise1[2]->Unlock();
		_lspNoise1[3]->Unlock();
		_pDst->Unlock();

/*		_pDst->Blt(_pDst->GetClipRect(), _lspTemp[0], 0, CPnt(0, 0), 0);
		{
			for(int i = 1; i < _lspNoise1.Len(); i++)
			{
			}
		}*/
	}

	void ShadeNoise(CImage* _pSrc, CImage* _pDst)
	{
		const uint8* _pS = (uint8*)_pSrc->Lock();
		uint32* _pD = (uint32*)_pDst->Lock();
		int ModShift = Log2(_pSrc->GetWidth());
		int w = _pSrc->GetWidth();
		int h = _pSrc->GetHeight();

		int DiffR = m_Diffuse.GetR();
		int DiffG = m_Diffuse.GetG();
		int DiffB = m_Diffuse.GetB();
		int SpecR = m_Specular.GetR();
		int SpecG = m_Specular.GetG();
		int SpecB = m_Specular.GetB();

		for(int y = 0; y < h; y++)
			for(int x = 0; x < w; x++)
			{
				int i = x + (y << ModShift);
				int Noise = _pS[i];
/*				int Noise10 = _pS[(i + 1) & 0xffff];
				int Noise01 = _pS[(i + 256) & 0xffff];
				int Noise11 = _pS[(i + 1 + 256) & 0xffff];
				int Noise20 = _pS[(i + 2) & 0xffff];
				int Noise02 = _pS[(i + 512) & 0xffff];
				int Noise22 = _pS[(i + 2 + 512) & 0xffff];
				int Noise33 = _pS[(i + 3 + 768) & 0xffff];*/
				int Noise10 = _pS[(i + 3) & 0xffff];
				int Noise01 = _pS[(i + 768) & 0xffff];
				int Noise11 = _pS[(i + 3 + 768) & 0xffff];
				int Noise20 = _pS[(i + 6) & 0xffff];
				int Noise02 = _pS[(i + 1536) & 0xffff];
				int Noise22 = _pS[(i + 6 + 1536) & 0xffff];
				int Noise33 = _pS[(i + 5 + 1280) & 0xffff];
				int Diff = 
					(Noise - Noise10) + 
					(Noise - Noise01) + 
					(Noise - Noise11) + 
					(Noise - Noise20) +
					(Noise - Noise02) +
					(Noise - Noise22) + 
					(Noise - Noise33);
//				if (Diff < 0) Diff >>= 1;
				int IR, IG, IB;
				if (Diff > 0)
				{
					IR = (220-(Noise >> 1))*DiffR >> 8;
					IG = (220-(Noise >> 1))*DiffG >> 8;
					IB = (220-(Noise >> 1))*DiffB >> 8;
					IR += (Diff * SpecR) >> 8;
					IG += (Diff * SpecG) >> 8;
					IB += (Diff * SpecB) >> 8;
				}
				else
				{
//					Diff >>= 1;
					IR = (220-(Noise >> 1))*DiffR >> 8;
					IG = (220-(Noise >> 1))*DiffG >> 8;
					IB = (220-(Noise >> 1))*DiffB >> 8;
					IR += (Diff * SpecR) >> 8;
					IG += (Diff * SpecG) >> 8;
					IB += (Diff * SpecB) >> 8;
				}
				IR = Min(Max(IR, 0), 255);
				IG = Min(Max(IG, 0), 255);
				IB = Min(Max(IB, 0), 255);

//				int Illum = 220-(Noise >> 1) + (Diff >> 1);
//				Illum = Min(Max(Illum, 0), 255);
				int Alpha = (Noise*6);
				if (Alpha > 255) Alpha = 255;
				_pD[i] = (Alpha << 24) + (IB + (IG << 8) + (IR << 16));
//				_pD[i] = (Alpha << 24) + (Illum + (Illum << 8) + (Illum << 16));
//				_pD[i] = (Noise << 24) + 0xffffff;
//				_pD[i] = 0xff00ff00;
			}
	}

	virtual void BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _ConvertType, int _iStartMip, uint32 _BuildFlags)
	{
		MSCOPESHORT(CTextureContainer_Clouds::BuildInto);
		if (_nMipmaps != 1)
			Error("BuildInto", CStrF("Invalid number of MIPmaps.", _nMipmaps));
		if (_iStartMip)
			Error("BuildInto", "Only mipmap 0 is supported.");

		CompositeNoise(m_lspNoise1, m_lspNoise2, m_lspNoiseTemp, m_spNoiseComp);

		CImage* pImg = _ppImg[0];

//		pImg->Fill(pImg->GetClipRect(), 0xff00ff00);

		if (pImg->GetFormat() == IMAGE_FORMAT_BGRA8)
			ShadeNoise(m_spNoiseComp, pImg);
		else
		{
			ShadeNoise(m_spNoiseComp, m_spNoiseShade);
			CImage::Convert(m_spNoiseShade, pImg, IMAGE_CONVERT_RGBA);
		}
	}
};


typedef TPtr<CTextureContainer_Clouds> spCTextureContainer_Clouds;

#endif

// -------------------------------------------------------------------
//  CXR_Model_Sky
// -------------------------------------------------------------------
class CSkySurface;
typedef TPtr<CSkySurface> spCSkySurface;

class CSkyLayerKey;

#define SKY_FOGSIZE 16


class CSkyBoxFace
{
public:
	CVec2Dfp32 m_MinVis;
	CVec2Dfp32 m_MaxVis;
	CPlane3Dfp32 m_ClipPlanes[4];
	CMat4Dfp32 m_F2LMat;		// Face-2-local matrix
	CMat4Dfp32 m_L2FMat;		// Local-2-face matrix

	void Clear()
	{
		m_MinVis[0] = 2.0f;
		m_MinVis[1] = 2.0f;
		m_MaxVis[0] = -2.0f;
		m_MaxVis[1] = -2.0f;
	}

	void Open()
	{
		m_MinVis[0] = -1.0f;
		m_MinVis[1] = -1.0f;
		m_MaxVis[0] = 1.0f;
		m_MaxVis[1] = 1.0f;
	}
};


class CXR_Model_Sky : public CXR_Model, public CXR_SkyInterface
{
	MRTC_DECLARE;

protected:
	
	// Multiple viewcontexts is needed to keep track of necessary states across 
	// recursions. n viewcontexts is used for a maximum recursion depth of n.

	class CSky_ViewContext
	{
	public:
		CMat4Dfp32 m_BoxL2WMat;
		CMat4Dfp32 m_BoxW2LMat;
		CMat4Dfp32 m_BoxL2VMat;
		int m_nBoxFaces;
		CSkyBoxFace m_lBoxFaces[6];

		CSky_ViewContext();
	};

	int m_LastFogColor;

	spCSkySurface m_spSky;
	TArray<spCXW_Surface> m_lspSurfaces;
//	spCTextureContainer_Sky m_spProcTxt;
//	spCTextureContainer_Clouds m_spCloudTxt;

//	spCXR_Model m_spFlare;
//	CXR_AnimState m_FlareAnim;
		
	void Sky_RenderPoly_Surface(class CSky_RenderInstanceParamters* _pRenderParams, const CPlane3Dfp32* _pPlanes, int _nPlanes, const CVec3Dfp32* _pV, const CVec2Dfp32* _pTV, int _nVert, fp32 _Priority, CSkyLayerKey* _pKey);
	void Sky_Cube(class CSky_RenderInstanceParamters* _pRenderParams, const CPlane3Dfp32* _pPlanes, int _nPlanes, CSkyLayerKey* _pKey);
	void Sky_Plane(class CSky_RenderInstanceParamters* _pRenderParams, const CPlane3Dfp32* _pPlanes, int _nPlanes, CSkyLayerKey* _pKey);
	CVec3Dfp32 Sky_SpritePos(CSkyLayerKey* _pKey);
	CVec3Dfp32 Sky_Sprite(class CSky_RenderInstanceParamters* _pRenderParams, const CPlane3Dfp32* _pPlanes, int _nPlanes, CSkyLayerKey* _pKey, const CVec3Dfp32& _Pos);

	void RenderSkyBox(class CSky_RenderInstanceParamters* _pRenderParams, const CPlane3Dfp32* _pPlanes, int _nPlanes, CSkyLayerKey* _pKey);

	void PreRenderSky(class CSky_RenderInstanceParamters* _pRenderParams, CSkySurface* _pSky, CMTime _Time);
	void RenderSky(class CSky_RenderInstanceParamters* _pRenderParams, const CVec3Dfp32* _pV, const uint32* _piV, int _nVertices, CSkySurface* _pSky, CMTime _Time);

	void Sky_UpdateFog(class CSky_RenderInstanceParamters* _pRenderParams);

	TArray<CSky_ViewContext> m_lVC;

	CSky_ViewContext* Sky_GetVC(CXR_Engine* _pEngine);

public:

	DECLARE_OPERATOR_NEW

	CXR_Model_Sky();
	virtual void Create(const char* _pParam);
	virtual void Create(const char* _pName, TArray<uint8> _lSurfaceData, TArray<uint8> _lSkyData );

	// Bounding volumes in model-space
	virtual int GetRenderPass(const CXR_AnimState* _pAnimState = NULL) { return _pAnimState->m_Anim1-1; };

	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = NULL);

	virtual void PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);
	// Render
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
			const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);

	virtual void OnPrecache(CXR_Engine* _pEngine, int _iVariation);

	// CXR_SkyInterface overrides
	virtual CXR_SkyInterface* Sky_GetInterface() { return this; };
	virtual void Sky_PrepareFrame(CXR_Engine* _pEngine);
	virtual void Sky_ClearFrustrum(CXR_Engine* _pEngine);
	virtual void Sky_OpenFrustrum(CXR_Engine* _pEngine);
	virtual void Sky_ExpandFrustrum(CXR_Engine* _pEngine, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CVec3Dfp32* _pV, int _nV);
};

#endif // _INC_XRSky
