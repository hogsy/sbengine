#include "PCH.h"

#ifndef PLATFORM_CONSOLE

#include "MTextureContainers.h"
#include "MImage.h"

class CTC_PostFilter : public CReferenceCount
{
	MRTC_DECLARE;
public:
	virtual void DoProcess(CTextureContainer_Plain* _pContainer, int _iLocal, CStr _Param) pure;
};

class CTC_PostFilter_ScaleDown : public CTC_PostFilter
{
	MRTC_DECLARE;
	friend class CTextureContainer_Plain;

public:
	virtual void DoProcess(CTextureContainer_Plain* _pContainer, int _iLocal, CStr _Param)
	{
		int iMipmap = _Param.Val_int();
		if(iMipmap > 0)
		{
			//Instead of doing the actual scaling we select a mipmap from the texture.
			uint8 aVersions[CTC_TEXTUREVERSION_MAX];
			int nVersions = _pContainer->EnumTextureVersions(_iLocal, aVersions, CTC_TEXTUREVERSION_MAX);
			for(int iVersion = 0; iVersion < nVersions; iVersion++)
			{
				spCTexture spTex = _pContainer->GetTextureMap(_iLocal + iVersion, CTC_TEXTUREVERSION_ANY);
				CImage* pMip = _pContainer->GetTexture(_iLocal + iVersion, iMipmap, CTC_TEXTUREVERSION_ANY);

				pMip->Duplicate(&spTex->m_LargestMap);
				for(int i = iMipmap + 1; i < spTex->m_nMipmaps; i++)
					spTex->m_lspMaps[i - iMipmap]	= spTex->m_lspMaps[i];

				for(int i = spTex->m_nMipmaps - iMipmap; i < CTC_MAXTEXTURESIZESHIFT; i++)
					spTex->m_lspMaps[i]	= 0;
				spTex->m_nMipmaps	-= iMipmap;
			}
		}
	}
};

MRTC_IMPLEMENT(CTC_PostFilter, CReferenceCount);
MRTC_IMPLEMENT_DYNAMIC(CTC_PostFilter_ScaleDown, CTC_PostFilter);

void CTextureContainer_Plain::FilterTexture(int _iTexture, const CTC_PostFilterParams& _Params)
{
	const CRegistry* pPostFilters = _Params.m_spFilters;

	for(int i = 0; i < pPostFilters->GetNumChildren(); i++)
	{
		CStr Key = pPostFilters->GetName(i);
		CStr Runtime = CStr("CTC_PostFilter_") + Key;
		TPtr<CTC_PostFilter> spFilter = TDynamicCast<CTC_PostFilter>(MRTC_GOM()->CreateObject(Runtime));
		spFilter->DoProcess(this, _iTexture, pPostFilters->GetValue(i));
	}
}
#endif // PLATFORM_CONSOLE
