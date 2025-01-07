
#ifndef __MIMAGECUBEMAPSAMPLER_H_INCLUDED
#define __MIMAGECUBEMAPSAMPLER_H_INCLUDED

enum
{
	IMAGE_CUBEMAPSIDE_POSX			= 0,
	IMAGE_CUBEMAPSIDE_NEGX			= 1,
	IMAGE_CUBEMAPSIDE_POSY			= 2,
	IMAGE_CUBEMAPSIDE_NEGY			= 3,
	IMAGE_CUBEMAPSIDE_POSZ			= 4,
	IMAGE_CUBEMAPSIDE_NEGZ			= 5,
};

class CImage;

class SYSTEMDLLEXPORT CImageCubemapSampler : public CReferenceCount
{
protected:
	CImage*		m_lpSides[6];
	bool		m_bSingleImage;
	bool		m_bHardwareAccurateEdgeSampling;

	void SmoothPixel(int _iSide0, int _iSide1, const CPnt& _Pos0, const CPnt& _Pos1);
	void SmoothPixel(int _iSide0, int _iSide1, int _iSide2, const CPnt& _Pos0, const CPnt& _Pos1, const CPnt& _Pos2);

	static void SmoothPixel(CImage* _pImg, const CPnt& _Pos0, const CPnt& _Pos1);
	static void SmoothPixel(CImage* _pImg, const CPnt& _Pos0, const CPnt& _Pos1, const CPnt& _Pos2, const CPnt& _Pos3);
public:
	DECLARE_OPERATOR_NEW

	CImageCubemapSampler();
	virtual ~CImageCubemapSampler();

	void Create();
	void Clear();
	void SetSingleImage(CImage* _pImg);
	void SetSideImage(int _iSide, CImage* _pImg);
	void SetHardwareAccurateEdgeSampling(bool _bMoomin);

	static int GetSideTransform(int _iSide);

	bool Validate();
	void SmoothCubemapEdges();
	static void SmoothCubemapEdges_SingleImage(CImage* _pImg);

	static CVec3Dfp32 CreateSampleVector(int _iSide, fp32 _a, fp32 _b)
	{
		switch(_iSide)
		{
		case IMAGE_CUBEMAPSIDE_POSX:	return CVec3Dfp32( 1.0f, -_a, _b);
		case IMAGE_CUBEMAPSIDE_NEGX:	return CVec3Dfp32(-1.0f, _a, _b);
		case IMAGE_CUBEMAPSIDE_POSY:	return CVec3Dfp32( _a,  1.0f, _b);
		case IMAGE_CUBEMAPSIDE_NEGY:	return CVec3Dfp32(-_a, -1.0f, _b);
		case IMAGE_CUBEMAPSIDE_POSZ:	return CVec3Dfp32(-_b,-_a, 1.0f);
		case IMAGE_CUBEMAPSIDE_NEGZ:	return CVec3Dfp32( _b,-_a, -1.0f);
		}

		return CVec3Dfp32(0, 0, 0);
	}
	CPixel32 SampleVector(const CVec3Dfp32& _Vector) const;
};


#endif
