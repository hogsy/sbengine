
#include "PCH.h"

#include "MRenderPS3_Texture.h"

namespace NRenderPS3GCM
{

#define IDINFO_CUBEMAP M_Bit(1)

// Enable workarounds
const bool bCube_Pow2Required = true;

const char* TexTypes[] = {"void", "1d", "2d", "3d", "cube"};
const char* FormatStr[CTextureGCM::TEX_FORMAT_MAX] = 
{
		"TEX_FORMAT_VOID",

		"TEX_FORMAT_S3TC_DXT1",
		"TEX_FORMAT_S3TC_DXT23",
		"TEX_FORMAT_S3TC_DXT45",

		"TEX_FORMAT_BGR5",
		"TEX_FORMAT_BGR8",
		"TEX_FORMAT_BGRA8",
		"TEX_FORMAT_BGRX8",
		"TEX_FORMAT_RGBA8",
		"TEX_FORMAT_A8",
		"TEX_FORMAT_I8A8",
		"TEX_FORMAT_I8",
		"TEX_FORMAT_I16",
		"TEX_FORMAT_RGBA16_F",
		"TEX_FORMAT_RGB32_F",
		"TEX_FORMAT_RGBA32_F",
		"TEX_FORMAT_BGRA4",
		"TEX_FORMAT_Z24S8"
};

static uint ConvertSizeFromPixels(uint _PixelCount, uint32 _Format)
{
	switch(_Format)
	{
	case CTextureGCM::TEX_FORMAT_DXT1:
		{
			return _PixelCount >> 1;
		}
	case CTextureGCM::TEX_FORMAT_DXT23:
	case CTextureGCM::TEX_FORMAT_DXT45:
	case CTextureGCM::TEX_FORMAT_A8:
	case CTextureGCM::TEX_FORMAT_I8:
		{
			return _PixelCount;
		}
	case CTextureGCM::TEX_FORMAT_BGR5:
	case CTextureGCM::TEX_FORMAT_I8A8:
	case CTextureGCM::TEX_FORMAT_I16:
	case CTextureGCM::TEX_FORMAT_BGRA4:
		{
			return _PixelCount << 1;
		}

	case CTextureGCM::TEX_FORMAT_BGRA8:
	case CTextureGCM::TEX_FORMAT_BGRX8:
	case CTextureGCM::TEX_FORMAT_RGBA8:
	case CTextureGCM::TEX_FORMAT_Z24S8:
		{
			return _PixelCount << 2;
		}

	case CTextureGCM::TEX_FORMAT_RGBA16_F:
		{
			return _PixelCount << 3;
		}
	case CTextureGCM::TEX_FORMAT_RGB32_F:
		{
			return (_PixelCount << 2) + (_PixelCount << 3);
		}
	case CTextureGCM::TEX_FORMAT_RGBA32_F:
		{
			return _PixelCount << 4;
		}
	}

	M_BREAKPOINT;
	return 0;
}

static uint Pow2GE(uint _Value)
{
	uint Val = 1;
	while(Val < _Value)
		Val <<= 1;

	return Val;
}

// -------------------------------------------------------------------
//  CTextureUploader
// -------------------------------------------------------------------

uint32 CTextureUploader::AllocSpace(uint _Size)
{
	M_LOCK(m_Lock);
	uint DataSizeAligned = (_Size + 127) & ~127;

	volatile uint32* pLabel = gcmGetLabelAddress(GPU_LABEL_TEXLOADER);

	uint32 WritePos = m_WritePos;
	uint32 ReadPos = *pLabel;
	if(WritePos < ReadPos && (WritePos + DataSizeAligned) >= ReadPos)
	{
		// Tail caught up to head, need to do some waiting so the GPU can munge through the buffer
		// Make sure processing is being done
		if(MRTC_SystemInfo::OS_GetThreadID() == CDCPS3GCM::ms_This.m_pMainThread)
			gcmFlush();
		// not enough room, have to restart from begining
		while(WritePos < ReadPos && (WritePos + DataSizeAligned) >= ReadPos)
		{
			// GPU Hasn't fetched enough data yet
			ReadPos = *pLabel;
		}
	}

	if(WritePos + DataSizeAligned > m_Size)
	{
		// Need to wrap damnit
		{
			// No room in collectbuffer, 
			// Make sure processing is being done
			if(MRTC_SystemInfo::OS_GetThreadID() == CDCPS3GCM::ms_This.m_pMainThread)
				gcmFlush();
			// not enough room, have to restart from begining
			while(ReadPos <= DataSizeAligned)
			{
				// GPU Hasn't fetched enough data yet
				ReadPos = *pLabel;
			}
		}
		WritePos = 0;
	}

	m_WritePos = WritePos + DataSizeAligned;
	return WritePos;
}

void CTextureUploader::UploadS3TC(const CRenderPS3Resource_Texture* _pTex, uint _iFace, uint _nFace, uint _iMip, uint _Fmt, uint _TexFlags, void* _pData)
{
#ifndef PLATFORM_PS3
	return;
#endif
	// Only support Main -> Local for now
	{
		uint Width = Max(4, ((_pTex->m_RSXTexture.width >> _iMip) + 3) & ~3);		// Might have to clamp to 4
		uint Height = Max(4, ((_pTex->m_RSXTexture.height >> _iMip) + 3) & ~3);		// Might have to clamp to 4

		uint DestPitch = Width << 1;
		uint SrcPitch = Width << 1;
		uint RowCopyBytes = Width << 1;

		if(_Fmt != CTextureGCM::TEX_FORMAT_DXT1)
		{
			// DXT23 or DXT45, both are 1 byte per pixel
			DestPitch <<= 1;
			SrcPitch <<= 1;
			RowCopyBytes <<= 1;
		}

		if(_pTex->m_RSXTexture.pitch)
			DestPitch = _pTex->m_RSXTexture.pitch;

		uint DataSize = ConvertSizeFromPixels(Width * Height, _Fmt);
		uint32 MemOffset = AllocSpace(DataSize);
		void* pMem = ConvertToMemory(MemOffset);
		memcpy(pMem, _pData, DataSize);

		if(_TexFlags & CTextureGCM::TEX_FL_REQUIRETRANSFORM)
		{
			CTextureGCM::Transform(_Fmt, pMem, Width, Height);
		}

		if(MRTC_SystemInfo::OS_GetThreadID() != CDCPS3GCM::ms_This.m_pMainThread)
		{
			// Queue item since this isn't the rendering thread
			uint aDstOffset[6];
			for(uint iF = 0; iF < _nFace; iF++)
				aDstOffset[iF] = _pTex->GetMipOffset(_iFace + iF, _iMip);

			QueueTransfer(_nFace, CELL_GCM_TRANSFER_MAIN_TO_LOCAL, aDstOffset, DestPitch, ConvertToOffset(MemOffset), SrcPitch, RowCopyBytes, (Height + 3) >> 2, m_WritePos);
		}
		else
		{
			// Make sure queue is empty before we xfer anything
			Service(10);
			for(uint iF = 0; iF < _nFace; iF++)
			{
//				M_TRACEALWAYS("Uploading S3TC %x bytes from Main 0x%.8X to Local 0x%.8X\n", DataSize, ConvertToOffset(MemOffset), _pTex->GetMipOffset(_iFace + iF, _iMip));
				gcmSetTransferData(CELL_GCM_TRANSFER_MAIN_TO_LOCAL, _pTex->GetMipOffset(_iFace + iF, _iMip), DestPitch, ConvertToOffset(MemOffset), SrcPitch, RowCopyBytes, (Height + 3) >> 2);
			}
			gcmSetWriteBackEndLabel(GPU_LABEL_TEXLOADER, m_WritePos);
			gcmSetInvalidateTextureCache(CELL_GCM_INVALIDATE_TEXTURE);
		}
	}
}

void CTextureUploader::UploadSwizzle(const CRenderPS3Resource_Texture* _pTex, uint _iFace, uint _nFace, uint _iMip, uint _Fmt, uint _TexFlags, void* _pData)
{
#ifndef PLATFORM_PS3
	return;
#endif
	// Only support Main -> Local for now
	{
		uint Width = _pTex->m_RSXTexture.width >> _iMip;		// Might have to clamp to 4
		uint Height = _pTex->m_RSXTexture.height >> _iMip;		// Might have to clamp to 4

		uint DestPitch = ConvertSizeFromPixels(Width, _Fmt);
		uint SrcPitch = ConvertSizeFromPixels(Width, _Fmt);
		uint RowCopyBytes = ConvertSizeFromPixels(Width, _Fmt);

		uint DataSize = ConvertSizeFromPixels(Width * Height, _Fmt);
		uint32 MemOffset = AllocSpace(DataSize);
		void* pMem = ConvertToMemory(MemOffset);

		cellGcmConvertSwizzleFormat(pMem, _pData, 0, 0, 0, Width, Height, 1, 0, 0, 0, Width, Height, 1, Width, Height, 1, Log2(ConvertSizeFromPixels(1, _Fmt)), ConvertSizeFromPixels(1, _Fmt), CELL_GCM_FALSE, CELL_GCM_TEXTURE_DIMENSION_2, NULL);
		if(_TexFlags & CTextureGCM::TEX_FL_REQUIRETRANSFORM)
		{
			CTextureGCM::Transform(_Fmt, pMem, Width, Height);
		}

		if(MRTC_SystemInfo::OS_GetThreadID() != CDCPS3GCM::ms_This.m_pMainThread)
		{
			// Queue item since this isn't the rendering thread
			uint aDstOffset[6];
			for(uint iF = 0; iF < _nFace; iF++)
				aDstOffset[iF] = _pTex->GetMipOffset(_iFace + iF, _iMip);

			QueueTransfer(_nFace, CELL_GCM_TRANSFER_MAIN_TO_LOCAL, aDstOffset, DestPitch, ConvertToOffset(MemOffset), SrcPitch, RowCopyBytes, Height, m_WritePos);
		}
		else
		{
			Service(10);
			for(uint iF = 0; iF < _nFace; iF++)
			{
//				M_TRACEALWAYS("Uploading RSWZ %x bytes from Main 0x%.8X to Local 0x%.8X\n", DataSize, ConvertToOffset(MemOffset), _pTex->GetMipOffset(_iFace + iF, _iMip));
				gcmSetTransferData(CELL_GCM_TRANSFER_MAIN_TO_LOCAL, _pTex->GetMipOffset(_iFace + iF, _iMip), DestPitch, ConvertToOffset(MemOffset), SrcPitch, RowCopyBytes, Height);
			}
			gcmSetWriteBackEndLabel(GPU_LABEL_TEXLOADER, m_WritePos);
			gcmSetInvalidateTextureCache(CELL_GCM_INVALIDATE_TEXTURE);
		}
	}
}

void CTextureUploader::UploadLinear(const CRenderPS3Resource_Texture* _pTex, uint _iFace, uint _nFace, uint _iMip, uint _Fmt, uint _TexFlags, void* _pData)
{
#ifndef PLATFORM_PS3
	return;
#endif
	// Only support Main -> Local for now
	{
		uint Width = _pTex->m_RSXTexture.width >> _iMip;		// Might have to clamp to 4
		uint Height = _pTex->m_RSXTexture.height >> _iMip;		// Might have to clamp to 4

//			uint DestPitch = ConvertSizeFromPixels(Width, _Fmt);
		uint DestPitch = _pTex->m_RSXTexture.pitch;
		uint SrcPitch = ConvertSizeFromPixels(Width, _Fmt);
		uint RowCopyBytes = ConvertSizeFromPixels(Width, _Fmt);

		uint DataSize = ConvertSizeFromPixels(Width * Height, _Fmt);
		uint32 MemOffset = AllocSpace(DataSize);
		void* pMem = ConvertToMemory(MemOffset);
		memcpy(pMem, _pData, DataSize);

		if(_TexFlags & CTextureGCM::TEX_FL_REQUIRETRANSFORM)
		{
			CTextureGCM::Transform(_Fmt, pMem, Width, Height);
		}

		if(MRTC_SystemInfo::OS_GetThreadID() != CDCPS3GCM::ms_This.m_pMainThread)
		{
			uint aDstOffset[6];
			for(uint iF = 0; iF < _nFace; iF++)
				aDstOffset[iF] = _pTex->GetMipOffset(_iFace + iF, _iMip);

			QueueTransfer(_nFace, CELL_GCM_TRANSFER_MAIN_TO_LOCAL, aDstOffset, DestPitch, ConvertToOffset(MemOffset), SrcPitch, RowCopyBytes, Height, m_WritePos);
		}
		else
		{
			Service(10);
			for(uint iF = 0; iF < _nFace; iF++)
			{
//				M_TRACEALWAYS("Uploading RLIN %x bytes from Main 0x%.8X to Local 0x%.8X\n", DataSize, ConvertToOffset(MemOffset), _pTex->GetMipOffset(_iFace + iF, _iMip));
				gcmSetTransferData(CELL_GCM_TRANSFER_MAIN_TO_LOCAL, _pTex->GetMipOffset(_iFace + iF, _iMip), DestPitch, ConvertToOffset(MemOffset), SrcPitch, RowCopyBytes, Height);
			}
			gcmSetWriteBackEndLabel(GPU_LABEL_TEXLOADER, m_WritePos);
			gcmSetInvalidateTextureCache(CELL_GCM_INVALIDATE_TEXTURE);
		}
	}
}

// -------------------------------------------------------------------
//  CRenderPS3Resource_Texture_Local
// -------------------------------------------------------------------

void CRenderPS3Resource_Texture::CRenderPS3Resource_Texture_Local::Create(uint _Size)
{
	Alloc(_Size, GPU_SWIZZLEDTEXTURE_ALIGNMENT, &m_BlockInfo, false);
	m_BlockInfo.m_Size = _Size;
}

void CRenderPS3Resource_Texture::CRenderPS3Resource_Texture_Local::Create(uint _Size, uint _TilePitch, uint _CompMode)
{
	M_ASSERT(_TilePitch, "Don't call this without tiles");
	_Size = (_Size + 0xffff) & ~0xffff;
	Alloc(_Size, GPU_TILEDTEXTURE_ALIGNMENT, &m_BlockInfo, false);
	m_BlockInfo.m_Size = _Size;
	m_iTile = CDCPS3GCM::ms_This.AllocTile(GetOffset(), _TilePitch, _Size, _CompMode);
	M_TRACEALWAYS("Allocated tile texture Tile %d, Offset 0x%.8X, Size 0x%.8X, Pitch %d\n", m_iTile, GetOffset(), _Size, _TilePitch);
}

void CRenderPS3Resource_Texture::CRenderPS3Resource_Texture_Local::Destroy()
{
	if(m_iTile > 0)
	{
		CDCPS3GCM::ms_This.FreeTile(m_iTile);
		m_iTile = 0;
	}
	CRenderPS3Resource_Local<GPU_SWIZZLEDTEXTURE_ALIGNMENT>::Destroy();
}

// -------------------------------------------------------------------
//  CRenderPS3Resource_Texture
// -------------------------------------------------------------------

CRenderPS3Resource_Texture::CRenderPS3Resource_Texture(uint _TexID)
{
	M_ASSERT(CRCPS3GCM::ms_This.m_ContextTexture.m_lpTexResources[_TexID] == 0, "Already allocated this ID");

	memset(m_lMipOffset, 0, sizeof(uint32) * 12);
	memset(m_lMipSize, 0, sizeof(uint32) * 12);
	memset(m_lFaceOffset, 0, sizeof(uint32) * 6);
	memset(&m_RSXTexture, 0, sizeof(m_RSXTexture));
	m_RSXAddrMode = 0;
	m_RSXFilterMode = 0;
	m_MaxAnisoFiltering = 0;
	m_MinLOD = 0;
	m_MaxLOD = 1;
	m_TextureID = 0;
	m_iFirstMip = 0;
	m_TexVersion = 0;
	m_TextureID = _TexID;
}

CRenderPS3Resource_Texture::~CRenderPS3Resource_Texture()
{
}

uint32 CRenderPS3Resource_Texture::CalcSize2DLinear(uint _Width, uint _Height, uint _nMip, uint _Format)
{
	if((_Format >= CTextureGCM::TEX_FORMAT_DXT1) && (_Format <= CTextureGCM::TEX_FORMAT_DXT45))
		_Width = Max((uint)4, (_Width + 3) & ~3);

	uint Pitch = ConvertSizeFromPixels(_Width, _Format);
	uint iMip = 0;
	uint TextureSize = Pitch * _Height;
	
	if((_Format >= CTextureGCM::TEX_FORMAT_DXT1) && (_Format <= CTextureGCM::TEX_FORMAT_DXT45))
	{
		TextureSize = Pitch * Max((uint)4, (_Height + 3) & ~3);
		_Height = Max((uint)4, _Height & ~3);
	}
	
	m_lMipSize[0] = TextureSize;
	m_lMipOffset[0] = 0;
	while(_nMip > 1)
	{
		iMip++;
		_nMip--;
		_Height = Max((uint)1, _Height >> 1);
		uint MipSize = Pitch * _Height;

		if((_Format >= CTextureGCM::TEX_FORMAT_DXT1) && (_Format <= CTextureGCM::TEX_FORMAT_DXT45))
		{
			MipSize = Pitch * Max((uint)4, (_Height + 3) & ~3);
			_Height = Max((uint)4, _Height & ~3);
		}

		m_lMipSize[iMip] = MipSize;
		m_lMipOffset[iMip] = TextureSize;
		TextureSize += MipSize;
	}

	return TextureSize;
}

uint32 CRenderPS3Resource_Texture::CalcSize2DSwizzled(uint _Width, uint _Height, uint _nMip, uint _Format)
{
	if((_Format >= CTextureGCM::TEX_FORMAT_DXT1) && (_Format <= CTextureGCM::TEX_FORMAT_DXT45))
	{
		_Width = Max((uint)4, (_Width + 3) & ~3);
		_Height = Max((uint)4, (_Height + 3) & ~3);
	}
	uint iMip = 0;
	uint TextureSize = ConvertSizeFromPixels(_Width * _Height, _Format);
	m_lMipSize[0] = TextureSize;
	m_lMipOffset[0] = 0;
	while(_nMip > 1)
	{
		iMip++;
		_nMip--;
		_Width = Max((uint)1, _Width >> 1);
		_Height = Max((uint)1, _Height >> 1);
		if((_Format >= CTextureGCM::TEX_FORMAT_DXT1) && (_Format <= CTextureGCM::TEX_FORMAT_DXT45))
		{
			_Width = Max((uint)4, (_Width + 3) & ~3);
			_Height = Max((uint)4, (_Height + 3) & ~3);
		}
		uint32 MipSize = ConvertSizeFromPixels(_Width * _Height, _Format);

		m_lMipSize[iMip] = MipSize;
		m_lMipOffset[iMip] = TextureSize;
		TextureSize += MipSize;
	}

	return TextureSize;
}

void CRenderPS3Resource_Texture::SetFormat(uint _Format, uint _bLinear)
{
	uint32 LinearFlag = CELL_GCM_TEXTURE_SZ;
	if(_bLinear)
		LinearFlag = CELL_GCM_TEXTURE_LN;

	switch(_Format)
	{
	case CTextureGCM::TEX_FORMAT_DXT1:
		{
			m_RSXTexture.format = CELL_GCM_TEXTURE_COMPRESSED_DXT1 | CELL_GCM_TEXTURE_NR | LinearFlag;
			m_RSXTexture.remap = REMAP_INPUT(A, R, G, B) | REMAP_OUTPUT(REMAP, REMAP, REMAP, REMAP);
			break;
		}
	case CTextureGCM::TEX_FORMAT_DXT23:
		{
			m_RSXTexture.format = CELL_GCM_TEXTURE_COMPRESSED_DXT23 | CELL_GCM_TEXTURE_NR | LinearFlag;
			m_RSXTexture.remap = REMAP_INPUT(A, R, G, B) | REMAP_OUTPUT(REMAP, REMAP, REMAP, REMAP);
			break;
		}
	case CTextureGCM::TEX_FORMAT_DXT45:
		{
			m_RSXTexture.format = CELL_GCM_TEXTURE_COMPRESSED_DXT45 | CELL_GCM_TEXTURE_NR | LinearFlag;
			m_RSXTexture.remap = REMAP_INPUT(A, R, G, B) | REMAP_OUTPUT(REMAP, REMAP, REMAP, REMAP);
			break;
		}
	case CTextureGCM::TEX_FORMAT_BGR5:
		{
			m_RSXTexture.format = CELL_GCM_TEXTURE_A1R5G5B5 | CELL_GCM_TEXTURE_NR | LinearFlag;
			m_RSXTexture.remap = REMAP_INPUT(A, R, G, B) | REMAP_OUTPUT(ONE, REMAP, REMAP, REMAP);
			break;
		}
	case CTextureGCM::TEX_FORMAT_BGR8:
		{
			// These texture should be converted
			M_BREAKPOINT;
			break;
		}
	case CTextureGCM::TEX_FORMAT_BGRA8:
		{
			m_RSXTexture.format = CELL_GCM_TEXTURE_A8R8G8B8 | CELL_GCM_TEXTURE_NR | LinearFlag;
			m_RSXTexture.remap = REMAP_INPUT(B, G, R, A) | REMAP_OUTPUT(REMAP, REMAP, REMAP, REMAP);
			break;
		}
	case CTextureGCM::TEX_FORMAT_BGRX8:
		{
			m_RSXTexture.format = CELL_GCM_TEXTURE_A8R8G8B8 | CELL_GCM_TEXTURE_NR | LinearFlag;
			m_RSXTexture.remap = REMAP_INPUT(B, G, R, A) | REMAP_OUTPUT(ONE, REMAP, REMAP, REMAP);
			break;
		}
	case CTextureGCM::TEX_FORMAT_RGBA8:
		{
			m_RSXTexture.format = CELL_GCM_TEXTURE_A8R8G8B8 | CELL_GCM_TEXTURE_NR | LinearFlag;
			m_RSXTexture.remap = REMAP_INPUT(A, R, G, B) | REMAP_OUTPUT(REMAP, REMAP, REMAP, REMAP);
			break;
		}
	case CTextureGCM::TEX_FORMAT_A8:
		{
			m_RSXTexture.format = CELL_GCM_TEXTURE_B8 | CELL_GCM_TEXTURE_NR | LinearFlag;
			m_RSXTexture.remap = REMAP_INPUT(B, B, B, B) | REMAP_OUTPUT(REMAP, ONE, ONE, ONE);
			break;
		}
	case CTextureGCM::TEX_FORMAT_I8A8:
		{
			m_RSXTexture.format = CELL_GCM_TEXTURE_G8B8 | CELL_GCM_TEXTURE_NR | LinearFlag;
			m_RSXTexture.remap = REMAP_INPUT(B, G, G, G) | REMAP_OUTPUT(REMAP, REMAP, REMAP, REMAP);
			break;
		}
	case CTextureGCM::TEX_FORMAT_I8:
		{
			m_RSXTexture.format = CELL_GCM_TEXTURE_B8 | CELL_GCM_TEXTURE_NR | LinearFlag;
			m_RSXTexture.remap = REMAP_INPUT(B, B, B, B) | REMAP_OUTPUT(ONE, REMAP, REMAP, REMAP);
			break;
		}
	case CTextureGCM::TEX_FORMAT_I16:
		{
			M_BREAKPOINT;
			break;
		}
	case CTextureGCM::TEX_FORMAT_RGBA16_F:
		{
			m_RSXTexture.format = CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT | CELL_GCM_TEXTURE_NR | LinearFlag;
			m_RSXTexture.remap = REMAP_INPUT(A, R, G, B) | REMAP_OUTPUT(REMAP, REMAP, REMAP, REMAP);
			break;
		}
	case CTextureGCM::TEX_FORMAT_RGB32_F:
		{
			M_BREAKPOINT;
			break;
		}
	case CTextureGCM::TEX_FORMAT_RGBA32_F:
		{
			m_RSXTexture.format = CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT | CELL_GCM_TEXTURE_NR | LinearFlag;
			m_RSXTexture.remap = REMAP_INPUT(A, R, G, B) | REMAP_OUTPUT(REMAP, REMAP, REMAP, REMAP);
			break;
		}
	case CTextureGCM::TEX_FORMAT_BGRA4:
		{
			m_RSXTexture.format = CELL_GCM_TEXTURE_A4R4G4B4 | CELL_GCM_TEXTURE_NR | LinearFlag;
			m_RSXTexture.remap = REMAP_INPUT(B, G, R, A) | REMAP_OUTPUT(REMAP, REMAP, REMAP, REMAP);
			break;
		}
	case CTextureGCM::TEX_FORMAT_Z24S8:
		{
			m_RSXTexture.format = CELL_GCM_TEXTURE_A8R8G8B8 | CELL_GCM_TEXTURE_NR | LinearFlag;
			m_RSXTexture.remap = REMAP_INPUT(B, A, R, G) | REMAP_OUTPUT(REMAP, REMAP, REMAP, REMAP);
			break;
		}
	default:
		M_BREAKPOINT;
	}
}

void CRenderPS3Resource_Texture::AllocCompressed2D(uint _Width, uint _Height, uint _nMip, uint _Format, uint _AllocFlags)
{
	bool bLinear = !IsPow2(_Width) || !IsPow2(_Height) || ((_AllocFlags & GCM_TEXALLOC_FORCELINEAR) != 0);
	uint Size = 0;
	if(bLinear)
		Size = CalcSize2DLinear(_Width, _Height, _nMip, _Format);
	else
		Size = CalcSize2DSwizzled(_Width, _Height, _nMip, _Format);

	m_lFaceOffset[0] = 0;
	SetFormat(_Format, bLinear);
	m_RSXTexture.mipmap = _nMip;
	m_RSXTexture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
	m_RSXTexture.cubemap = CELL_GCM_FALSE;
	m_RSXTexture.width = _Width;
	m_RSXTexture.height = _Height;
	m_RSXTexture.depth = 1;
	if(bLinear)
		m_RSXTexture.pitch = ConvertSizeFromPixels((_Width + 3) & ~3, _Format) << 2;
	else
		m_RSXTexture.pitch = 0;

	if (_AllocFlags & GCM_TEXALLOC_MAINMEM)
	{
		m_ResourceMain.Create(Size);
		m_RSXTexture.location = CELL_GCM_LOCATION_MAIN;
		m_RSXTexture.offset = m_ResourceMain.GetOffset();
	}
	else
	{
		m_ResourceLocal.Create(Size);
		m_RSXTexture.location = CELL_GCM_LOCATION_LOCAL;
		m_RSXTexture.offset = m_ResourceLocal.GetOffset();
	}
}

void CRenderPS3Resource_Texture::Alloc2D(uint _Width, uint _Height, uint _nMip, uint _Format, uint _AllocFlags)
{
	bool bLinear = !IsPow2(_Width) || !IsPow2(_Height) || ((_AllocFlags & GCM_TEXALLOC_FORCELINEAR) != 0);
	uint Size = 0;
	if(bLinear)
		Size = CalcSize2DLinear(_Width, _Height, _nMip, _Format);
	else
		Size = CalcSize2DSwizzled(_Width, _Height, _nMip, _Format);

	m_lFaceOffset[0] = 0;
	SetFormat(_Format, bLinear);
	m_RSXTexture.mipmap = _nMip;
	m_RSXTexture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
	m_RSXTexture.cubemap = CELL_GCM_FALSE;
	m_RSXTexture.width = _Width;
	m_RSXTexture.height = _Height;
	m_RSXTexture.depth = 1;
	m_RSXTexture.pitch = ConvertSizeFromPixels(_Width, _Format);

	if (_AllocFlags & GCM_TEXALLOC_MAINMEM)
	{
		m_ResourceMain.Create(Size);
		m_RSXTexture.location = CELL_GCM_LOCATION_MAIN;
		m_RSXTexture.offset = m_ResourceMain.GetOffset();
	}
	else
	{
		uint TiledCompMode = CELL_GCM_COMPMODE_DISABLED;
		if(_AllocFlags & GCM_TEXALLOC_TILE)
		{
			M_ASSERT(_nMip > 1, "Don't fucking mipmap tiled textures, its not good for your health");
			uint TiledPitch = cellGcmGetTiledPitchSize(m_RSXTexture.pitch);
			// Tiled textures are a bitch
			Size = TiledPitch * _Height;
//			if(_Format == CTextureGCM::TEX_FORMAT_RGBA8)
//				TiledCompMode = CELL_GCM_COMPMODE_C32_2X1;
			m_ResourceLocal.Create(Size, TiledPitch, TiledCompMode);
			m_RSXTexture.pitch = TiledPitch;
		}
		else
			m_ResourceLocal.Create(Size);
		m_RSXTexture.location = CELL_GCM_LOCATION_LOCAL;
		m_RSXTexture.offset = m_ResourceLocal.GetOffset();
	}
}

void CRenderPS3Resource_Texture::AllocCompressedCube(uint _Size, uint _nMip, uint _Format, uint _AllocFlags)
{
	bool bLinear = !IsPow2(_Size) || ((_AllocFlags & GCM_TEXALLOC_FORCELINEAR) != 0);
	uint FaceSize = 0;
	if(bLinear)
		FaceSize = CalcSize2DLinear(_Size, _Size, _nMip, _Format);
	else
		FaceSize = CalcSize2DSwizzled(_Size, _Size, _nMip, _Format);

	FaceSize = ((FaceSize + 127) & ~127);

	m_lFaceOffset[0] = 0;
	m_lFaceOffset[1] = FaceSize;
	m_lFaceOffset[2] = FaceSize * 2;
	m_lFaceOffset[3] = FaceSize * 3;
	m_lFaceOffset[4] = FaceSize * 4;
	m_lFaceOffset[5] = FaceSize * 5;
	SetFormat(_Format, bLinear);
	m_RSXTexture.mipmap = _nMip;
	m_RSXTexture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
	m_RSXTexture.cubemap = CELL_GCM_TRUE;
	m_RSXTexture.width = _Size;
	m_RSXTexture.height = _Size;
	m_RSXTexture.depth = 1;

	uint Size = FaceSize * 6;
	m_ResourceLocal.Create(Size);
	m_RSXTexture.location = CELL_GCM_LOCATION_LOCAL;
	m_RSXTexture.offset = m_ResourceLocal.GetOffset();
	if(bLinear)
		m_RSXTexture.pitch = ConvertSizeFromPixels(_Size, _Format) << 2;
	else
		m_RSXTexture.pitch = 0;
}

void CRenderPS3Resource_Texture::AllocCube(uint _Size, uint _nMip, uint _Format, uint _AllocFlags)
{
	bool bLinear = !IsPow2(_Size) || ((_AllocFlags & GCM_TEXALLOC_FORCELINEAR) != 0);
	uint FaceSize = ((CalcSize2DSwizzled(_Size, _Size, _nMip, _Format) + 127) & ~127);

	m_lFaceOffset[0] = 0;
	m_lFaceOffset[1] = FaceSize;
	m_lFaceOffset[2] = FaceSize * 2;
	m_lFaceOffset[3] = FaceSize * 3;
	m_lFaceOffset[4] = FaceSize * 4;
	m_lFaceOffset[5] = FaceSize * 5;
	SetFormat(_Format, bLinear);
	m_RSXTexture.mipmap = _nMip;
	m_RSXTexture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
	m_RSXTexture.cubemap = CELL_GCM_TRUE;
	m_RSXTexture.width = _Size;
	m_RSXTexture.height = _Size;
	m_RSXTexture.depth = 1;

	uint Size = FaceSize * 6;
	m_ResourceLocal.Create(Size);
	m_RSXTexture.location = CELL_GCM_LOCATION_LOCAL;
	m_RSXTexture.offset = m_ResourceLocal.GetOffset();
	if(bLinear)
		m_RSXTexture.pitch = ConvertSizeFromPixels(_Size, _Format);
	else
		m_RSXTexture.pitch = 0;
}

// -------------------------------------------------------------------
//  CContext_Texture
// -------------------------------------------------------------------

CContext_Texture::CContext_Texture()
{
	m_pTC	= NULL;
	m_pTextures = NULL;
	m_MaxAnisotropy = 16.0f;
	m_Anisotropy = 1.0f;
	m_bAllowTextureLoad = true;
	m_bPrecaching = false;
	m_bMip0Streaming = true;
}

CStr CContext_Texture::GetDesc(uint32 _TextureID)
{
	if(m_lTextures[_TextureID].m_Flags & CTextureGCM::TEX_FL_BUILT)
	{
		CTextureGCM* pTex = &m_lTextures[_TextureID];
		return CStrF("Type '%s', Dim (%dx%dx%d), Fmt %d (%s), Side (%d), Flags %.4x, Mips %d (Skipped %d)", TexTypes[pTex->m_Type], pTex->m_Width, pTex->m_Height, pTex->m_Depth, pTex->m_Format, FormatStr[pTex->m_Format], pTex->m_CubeFace, pTex->m_Flags, pTex->m_MipCount, pTex->m_SkippedMips);
	}
	else
		return "(Unbuilt texture)";
}

uint32 CContext_Texture::GetWidth(uint32 _TexID)
{
	if(IsBuilt(_TexID))
	{
		CTextureGCM* pTex = &m_lTextures[_TexID];
		return pTex->m_Width;
	}
	else
		return 0;
}

uint32 CContext_Texture::GetHeight(uint32 _TexID)
{
	if(IsBuilt(_TexID))
	{
		CTextureGCM* pTex = &m_lTextures[_TexID];
		return pTex->m_Height;
	}
	else
		return 0;
}

uint CContext_Texture::GetLocation(uint32 _TexID)
{
	M_ASSERT(IsBuilt(_TexID), "!");
	return m_lpTexResources[_TexID]->m_RSXTexture.location;
}

uint CContext_Texture::GetOffset(uint32 _TexID)
{
	M_ASSERT(IsBuilt(_TexID), "!");
	return m_lpTexResources[_TexID]->m_RSXTexture.offset;
}

uint CContext_Texture::GetPitch(uint32 _TexID)
{
	M_ASSERT(IsBuilt(_TexID), "!");
	return m_lpTexResources[_TexID]->m_RSXTexture.pitch;
}

uint32 CContext_Texture::GetFormat(uint32 _TexID)
{
	if(IsBuilt(_TexID))
	{
		CTextureGCM* pTex = &m_lTextures[_TexID];
		return pTex->m_Format;
	}

	return IMAGE_FORMAT_VOID;
}

void CContext_Texture::Precache_Begin(uint32 _Count)
{
}

static bool Overlap(uint _S0, uint _L0, uint _S1, uint _L1)
{
	if((_S0 <= _S1) && ((_S0 + _L0) > _S1))
		return true;
	if((_S1 <= _S0) && ((_S1 + _L1) > _S0))
		return true;

	return false;
}

void CContext_Texture::Precache_End()
{
	// Validate textures, make sure there are no overlaps
#ifdef PLATFORM_PS3
	uint32 nTxt = m_lTextures.Len();
	for(uint32 i = 0; i < nTxt; i++)
	{
		CTextureGCM* pTexGCM = &m_lTextures[i];
		if(!(pTexGCM->m_Flags & CTextureGCM::TEX_FL_BUILT))
			continue;

		if(pTexGCM->m_SkippedMips > 0)
			M_TRACEALWAYS("Texture %d (%s) %dx%d, skipped %d mipmaps\r\n", i, m_pTC->GetName(i).GetStr(), pTexGCM->m_Width, pTexGCM->m_Height, pTexGCM->m_SkippedMips);
	}

	for(uint i = 1; i < nTxt; i++)
	{
		if(!(m_lTextures[i].m_Flags & CTextureGCM::TEX_FL_BUILT))
			continue;

		if(m_lTextures[i].IsChainChild())
			continue;

		if(!m_lpTexResources[i]->IsLocal())
			continue;

		uint SrcOffset = m_lpTexResources[i]->GetOffset();
		uint SrcSize = m_lpTexResources[i]->m_ResourceLocal.m_BlockInfo.GetRealSize();

		for(uint j = i + 1; j < nTxt; j++)
		{
			if(!(m_lTextures[j].m_Flags & CTextureGCM::TEX_FL_BUILT))
				continue;

			if(m_lTextures[j].IsChainChild())
				continue;

			if(!m_lpTexResources[j]->IsLocal())
				continue;

			uint SrcOffset2 = m_lpTexResources[j]->GetOffset();
			uint SrcSize2 = m_lpTexResources[j]->m_ResourceLocal.m_BlockInfo.GetRealSize();

			if(Overlap(SrcOffset, SrcSize, SrcOffset2, SrcSize2))
				M_TRACEALWAYS("!!!! Texture %d and %d overlap!\n", i, j);
		}
	}
#endif
	m_bPrecaching = false;
	if(m_lTexturesDynamicLoadList.GetFirst())
		m_DynamicLoadEvent.Signal();
}

void CContext_Texture::Precache_Flush()
{
	m_bPrecaching = true;
	M_LOCK(m_Lock);
	{
		// Move all textures from dynamic list into finished list so we can delete them
		M_LOCK(CRCPS3GCM::ms_This.m_pTC->m_DeleteContainerLock);
		{
			M_LOCK(m_DynamicLoadListLock);
			while(m_lTexturesDynamicLoadList.GetFirst())
				m_lTexturesList.Insert(m_lTexturesDynamicLoadList.GetFirst());


			CRenderPS3Resource_TextureIter Iter = m_lTexturesList;
			CTextureContext* pTC = m_pTC;
			while(Iter)
			{
				CRenderPS3Resource_Texture* pTexture = Iter;
				++Iter;
				uint Flags = pTC->GetTextureFlags(pTexture->m_TextureID);
				if ((Flags & CTC_TXTIDFLAGS_RESIDENT))
					;
				else if ((Flags & CTC_TXTIDFLAGS_WASDELETED) || !(Flags & CTC_TXTIDFLAGS_ALLOCATED) || !(Flags & CTC_TXTIDFLAGS_PRECACHE))
				{
					Destroy(pTexture->m_TextureID);
				}
			}
		}
	}
}

void CContext_Texture::Create(CTextureContext* _pTC, uint32 _Count, TPtr<CRC_TCIDInfo> _spTCIDInfo)
{
	M_ASSERT(CTextureGCM::BITCOUNT_PADDING >= 0, "Texture bitfield is whacked");
	M_ASSERT((1 << CTextureGCM::BITCOUNT_FORMAT) > CTextureGCM::TEX_FORMAT_MAX, "Too many texture formats");
	m_pTC = _pTC;
	m_spTCIDInfo = _spTCIDInfo;

	m_lTextures.SetLen(_Count);
	TAP_RCD<CTextureGCM> plTxt(m_lTextures);
	for(uint i = 0; i < _Count; i++)
		plTxt[i].Clear();

	m_lpTexResources.SetLen(_Count);

	memset(m_lpTexResources.GetBasePtr(), 0, m_lpTexResources.ListSize());

	m_pTextures = m_lTextures.GetBasePtr();
	m_ppTexResources = m_lpTexResources.GetBasePtr();

	MACRO_GetSystemEnvironment(pEnv);

	for (int i = 0; i < CRC_MAXPICMIPS; ++i)
	{
		m_lPicMips[i] = pEnv->GetValuei(CStrF("R_PICMIP%d", i), 0);
	}

	m_bMip0Streaming = pEnv->GetValuei("R_DYNAMICLOADMIPS", 1) != 0;

	m_TextureUploader.Create();
#ifdef PLATFORM_PS3
	Thread_Create(NULL, 16384, MRTC_THREAD_PRIO_BELOWNORMAL);
//	Thread_Create();
#endif
}

void CContext_Texture::SetCompressed(uint32 _TextureID)
{
	CTextureGCM* pTex = &m_lTextures[_TextureID];
	M_ASSERT(pTex->m_Flags & CTextureGCM::TEX_FL_BUILT, "CContext_Texture::SetCompressed - Need to be built to modify");
	pTex->m_Flags |= CTextureGCM::TEX_FL_COMPRESSED;
	if(pTex->m_Type == CTextureGCM::TEX_TYPE_CUBE)
	{
		M_ASSERT(pTex->m_Flags & CTextureGCM::TEX_FL_CUBEOWNER, "CContext_Texture::SetCompressed - Can only do operation on owner cube");
		// Set all possible children to compressed aswell
		if(pTex->m_Flags & CTextureGCM::TEX_FL_CUBECHAIN)
		{
			int nVersions = m_pTC->EnumTextureVersions(_TextureID, 0, CTC_TEXTUREVERSION_ANY);
			CTextureContainer* pTC = m_pTC->GetTextureContainer(_TextureID);
			uint32 iLocal = m_pTC->GetLocal(_TextureID);
			uint32 iLocal1 = iLocal + nVersions;
			uint32 iChild1 = pTC->GetTextureID(iLocal1);
			uint32 iLocal2 = iLocal1 + nVersions;
			uint32 iChild2 = pTC->GetTextureID(iLocal2);
			uint32 iLocal3 = iLocal2 + nVersions;
			uint32 iChild3 = pTC->GetTextureID(iLocal3);
			uint32 iLocal4 = iLocal3 + nVersions;
			uint32 iChild4 = pTC->GetTextureID(iLocal4);
			uint32 iLocal5 = iLocal4 + nVersions;
			uint32 iChild5 = pTC->GetTextureID(iLocal5);
			CTextureGCM* pChild1 = &m_lTextures[iChild1];
			CTextureGCM* pChild2 = &m_lTextures[iChild2];
			CTextureGCM* pChild3 = &m_lTextures[iChild3];
			CTextureGCM* pChild4 = &m_lTextures[iChild4];
			CTextureGCM* pChild5 = &m_lTextures[iChild5];

			M_ASSERT((pChild1->m_Flags & CTextureGCM::TEX_FL_BUILT) && (pChild1->m_Type == CTextureGCM::TEX_TYPE_CUBE) && (pChild1->m_CubeFace == 1), "!");
			M_ASSERT((pChild2->m_Flags & CTextureGCM::TEX_FL_BUILT) && (pChild2->m_Type == CTextureGCM::TEX_TYPE_CUBE) && (pChild2->m_CubeFace == 2), "!");
			M_ASSERT((pChild3->m_Flags & CTextureGCM::TEX_FL_BUILT) && (pChild3->m_Type == CTextureGCM::TEX_TYPE_CUBE) && (pChild3->m_CubeFace == 3), "!");
			M_ASSERT((pChild4->m_Flags & CTextureGCM::TEX_FL_BUILT) && (pChild4->m_Type == CTextureGCM::TEX_TYPE_CUBE) && (pChild4->m_CubeFace == 4), "!");
			M_ASSERT((pChild5->m_Flags & CTextureGCM::TEX_FL_BUILT) && (pChild5->m_Type == CTextureGCM::TEX_TYPE_CUBE) && (pChild5->m_CubeFace == 5), "!");
			pChild1->m_Flags |= CTextureGCM::TEX_FL_COMPRESSED;
			pChild2->m_Flags |= CTextureGCM::TEX_FL_COMPRESSED;
			pChild3->m_Flags |= CTextureGCM::TEX_FL_COMPRESSED;
			pChild4->m_Flags |= CTextureGCM::TEX_FL_COMPRESSED;
			pChild5->m_Flags |= CTextureGCM::TEX_FL_COMPRESSED;
		}
	}
}

void CContext_Texture::SetCompressedFormat(uint32 _TextureID, uint32 _Format)
{
	M_ASSERT((_Format >= CTextureGCM::TEX_FORMAT_DXT1) && (_Format <= CTextureGCM::TEX_FORMAT_DXT45), "Not a valid compressed format");

	CTextureGCM* pTexGCM = &m_lTextures[_TextureID];

	M_ASSERT(pTexGCM->m_Flags & CTextureGCM::TEX_FL_BUILT, "Only change format on already built textures");

	if(pTexGCM->m_Type == CTextureGCM::TEX_TYPE_CUBE)
	{
		if(pTexGCM->m_Flags & CTextureGCM::TEX_FL_CUBECHAIN)
		{
			// Cubemap chain
			M_ASSERT(pTexGCM->m_Flags & CTextureGCM::TEX_FL_CUBEOWNER, "Only valid to change format of chain owner");

			{
				// Change all children to reflect owners format
				int nVersions = m_pTC->EnumTextureVersions(_TextureID, 0, CTC_TEXTUREVERSION_ANY);
				CTextureContainer* pTC = m_pTC->GetTextureContainer(_TextureID);
				uint32 iLocal = m_pTC->GetLocal(_TextureID);
				for(int i = 1; i < 6; i++)
				{
					uint32 TexID = pTC->GetTextureID(iLocal + i * nVersions);
					CTextureCubeGCM* pTex = (CTextureCubeGCM*)(&m_lTextures[TexID]);
					pTex->m_Format = _Format;
				}
			}
		}
	}

	// Set texture format
	pTexGCM->m_Format = _Format;
}

void CContext_Texture::Build(uint32 _TextureID, uint32 _iFirstMip, uint32 _MipCount)
{
	CImage Desc;
	int nMipMaps = 0;
	m_pTC->GetTextureDesc(_TextureID, &Desc, nMipMaps);

	CTC_TextureProperties Properties;
	m_pTC->GetTextureProperties(_TextureID, Properties);
	uint32 TexFlags = Properties.m_Flags;
	uint32 TexType = (TexFlags & (CTC_TEXTUREFLAGS_CUBEMAP | CTC_TEXTUREFLAGS_CUBEMAPCHAIN))?CTextureGCM::TEX_TYPE_CUBE:CTextureGCM::TEX_TYPE_2D;

	CTextureGCM* pTexGCM = &m_lTextures[_TextureID];

	uint32 Width = Max(1, Desc.GetWidth() >> _iFirstMip);
	uint32 Height = Max(1, Desc.GetHeight() >> _iFirstMip);
	uint32 Format = Desc.GetFormat();
	if(TexFlags & CTC_TEXTUREFLAGS_RENDERUSEBACKBUFFERFORMAT)
	{
		if(!(Format & IMAGE_FORMAT_ZBUFFER))
			Format = CDCPS3GCM::ms_This.GetFrameBuffer()->GetFormat();
	}
	else if(TexFlags & CTC_TEXTUREFLAGS_BACKBUFFER)
	{
		// Need to do a bit of format conversion
		if(Format & (IMAGE_FORMAT_BGRA8 | IMAGE_FORMAT_RGBA8))
			Format = IMAGE_FORMAT_RGBA8;
		else if(!(Format & (IMAGE_FORMAT_RGBA16_F | IMAGE_FORMAT_ZBUFFER)))
			M_BREAKPOINT;
	}

	uint32 RequireTransform = CTextureGCM::RequireTransform(Format);
	Format = CTextureGCM::TranslateFormat(Format);

	if(pTexGCM->m_Flags & CTextureGCM::TEX_FL_BUILT)
	{
		// If this is a cube-child then we need to search for the owner
		if(pTexGCM->m_Type == CTextureGCM::TEX_TYPE_CUBE)
		{
			if((pTexGCM->m_Flags & (CTextureGCM::TEX_FL_CUBEOWNER | CTextureGCM::TEX_FL_CUBECHAIN)) == CTextureGCM::TEX_FL_CUBECHAIN)
			{
				// Change pTexGCM to owner?
				CTextureContainer* pTC = m_pTC->GetTextureContainer(_TextureID);
				uint32 iLocalOwner = m_pTC->GetLocal(_TextureID) - (pTexGCM->m_CubeFace * m_pTC->EnumTextureVersions(_TextureID, NULL, CTC_TEXTUREVERSION_ANY));
				uint32 OwnerID = pTC->GetTextureID(iLocalOwner);
				M_ASSERT((m_lTextures[OwnerID].m_Type == CTextureGCM::TEX_TYPE_CUBE) && ((m_lTextures[OwnerID].m_Flags & (CTextureGCM::TEX_FL_CUBECHAIN | CTextureGCM::TEX_FL_CUBEOWNER)) == (CTextureGCM::TEX_FL_CUBECHAIN | CTextureGCM::TEX_FL_CUBEOWNER)), "!");
				CTC_TextureProperties OwnerProperties;
				m_pTC->GetTextureProperties(OwnerID, OwnerProperties);
				TexType = (OwnerProperties.m_Flags & (CTC_TEXTUREFLAGS_CUBEMAP | CTC_TEXTUREFLAGS_CUBEMAPCHAIN))?CTextureGCM::TEX_TYPE_CUBE:CTextureGCM::TEX_TYPE_2D;
				RequireTransform = (m_lTextures[OwnerID].m_Flags & CTextureGCM::TEX_FL_REQUIRETRANSFORM);	// Copy parent's requiretransform flag
			}
		}

		if(TexType != pTexGCM->m_Type)
		{
			// Texture has changed type since it was built, need to destruct and re-create
			M_ASSERT(!pTexGCM->IsChainChild(), "Should never have to rebuild with a cube-child texture");
			Destroy(_TextureID);
		}
		else
		{
			bool bCompressedChange = ((Desc.GetMemModel() & IMAGE_MEM_COMPRESSED) != 0) ^ ((pTexGCM->m_Flags & CTextureGCM::TEX_FL_COMPRESSED) != 0);
			if(!bCompressedChange)
			{
				// Already built and of same type, check if format or size has changed
				switch(pTexGCM->m_Type)
				{
				case CTextureGCM::TEX_TYPE_VOID: M_ASSERT(0, "Invalid!"); break;
				case CTextureGCM::TEX_TYPE_1D: M_ASSERT(0, "1D texture not supported"); break;
				case CTextureGCM::TEX_TYPE_3D: M_ASSERT(0, "3D texture not supported"); break;

				case CTextureGCM::TEX_TYPE_2D:
					{
						CTexture2DGCM* pTex2DGCM = (CTexture2DGCM*)pTexGCM;
						if(!pTex2DGCM->NeedBuild(Width, Height, Format))
							return;
						break;
					}

				case CTextureGCM::TEX_TYPE_CUBE:
					{
						CTextureCubeGCM* pTexCubeGCM = (CTextureCubeGCM*)pTexGCM;
						if(!pTexCubeGCM->NeedBuild(Width, Height, Format))
							return;

						M_ASSERT(!pTexCubeGCM->IsChainChild(), "Should never have to rebuild with a cube-child texture");
						break;
					}
				}
			}
			M_ASSERT(!pTexGCM->IsChainChild(), "Should never have to rebuild with a cube-child texture");
			Destroy(_TextureID);
		}
	}


	switch(TexType)
	{
	case CTextureGCM::TEX_TYPE_2D:
		{
			CTexture2DGCM* pTex2DGCM = (CTexture2DGCM*)pTexGCM;
			pTex2DGCM->Build2D(Width, Height, Format);
			// If it was destroyed earlier then we have to re-bind it
			break;
		}
	case CTextureGCM::TEX_TYPE_CUBE:
		{
			CTextureCubeGCM* pTexCubeGCM = (CTextureCubeGCM*)pTexGCM;
			pTexCubeGCM->BuildCube(Width, Height, Format, 0, (Properties.m_Flags & CTC_TEXTUREFLAGS_CUBEMAPCHAIN) != 0);
			// If it was destroyed earlier then we have to re-bind it

			if(Properties.m_Flags & CTC_TEXTUREFLAGS_CUBEMAPCHAIN)
			{
				int nVersions = m_pTC->EnumTextureVersions(_TextureID, 0, CTC_TEXTUREVERSION_ANY);
				CTextureContainer* pTC = m_pTC->GetTextureContainer(_TextureID);
				uint32 iLocal = m_pTC->GetLocal(_TextureID);
				for(int i = 1; i < 6; i++)
				{
					uint32 TexID = pTC->GetTextureID(iLocal + i * nVersions);
					CTextureCubeGCM* pTex = (CTextureCubeGCM*)(&m_lTextures[TexID]);
					pTex->BuildCube(Width, Height, Format, i, true);
					pTex->m_Flags |= RequireTransform;			// Make sure all children require the same transform as the owner
				}
			}
			break;
		}
	}

	if(TexFlags & CTC_TEXTUREFLAGS_RENDER)
		pTexGCM->m_Flags |= CTextureGCM::TEX_FL_LINEAR;

//	if(TexFlags & CTC_TEXTUREFLAGS_BACKBUFFER)
	if(TexFlags & CTC_TEXTUREFLAGS_RENDERTARGET)
		pTexGCM->m_Flags |= CTextureGCM::TEX_FL_TILE;

	pTexGCM->m_Flags |= RequireTransform;

	if(Properties.m_Flags & CTC_TEXTUREFLAGS_NOMIPMAP)
		pTexGCM->m_MipCount	= 1;
	else
		pTexGCM->m_MipCount	= _MipCount;

	if(Desc.GetMemModel() & IMAGE_MEM_COMPRESSED)
	{
		SetCompressed(_TextureID);
		return;
	}

	if(bCube_Pow2Required && (TexType == CTextureGCM::TEX_TYPE_CUBE) && !(pTexGCM->m_Flags & CTextureGCM::TEX_FL_POW2))
	{
		// cubemaps have to be pow 2, calculate how many mips to skip
		// Not pow2, calculate how many mips to skip when loading
		uint32 w = Width;
		int mip = 0;
		while(!IsPow2(w))
		{
			mip++;
			w	= Max(w >> 1, (uint32)1);
		}

		pTexGCM->m_SkippedMips	= mip;
	}
}

void CContext_Texture::Destroy(uint32 _TextureID)
{
	M_LOCK(m_Lock);
	// Destroy texture (and children if cubemap chain)
	M_ASSERT(m_lTextures[_TextureID].m_Flags & CTextureGCM::TEX_FL_BUILT, "Destroy called with non-built texture");

	uint TexID = _TextureID;
	if(m_lTextures[_TextureID].m_Type == CTextureGCM::TEX_TYPE_CUBE)
	{
		M_ASSERT(m_lTextures[_TextureID].m_Flags & CTextureGCM::TEX_FL_CUBEOWNER, "Only owner can be destroyed");

		if(m_lTextures[_TextureID].m_Flags & CTextureGCM::TEX_FL_CUBECHAIN)
		{
			int nVersions = m_pTC->EnumTextureVersions(_TextureID, NULL, CTC_TEXTUREVERSION_ANY);
			CTextureContainer* pTC = m_pTC->GetTextureContainer(_TextureID);
			uint32 iLocal = m_pTC->GetLocal(_TextureID);
			uint32 iLocal1 = iLocal + nVersions;
			uint32 iChild1 = pTC->GetTextureID(iLocal1);
			uint32 iLocal2 = iLocal1 + nVersions;
			uint32 iChild2 = pTC->GetTextureID(iLocal2);
			uint32 iLocal3 = iLocal2 + nVersions;
			uint32 iChild3 = pTC->GetTextureID(iLocal3);
			uint32 iLocal4 = iLocal3 + nVersions;
			uint32 iChild4 = pTC->GetTextureID(iLocal4);
			uint32 iLocal5 = iLocal4 + nVersions;
			uint32 iChild5 = pTC->GetTextureID(iLocal5);
			m_lTextures[iChild1].Destroy();
			m_lTextures[iChild2].Destroy();
			m_lTextures[iChild3].Destroy();
			m_lTextures[iChild4].Destroy();
			m_lTextures[iChild5].Destroy();
		}
	}

	m_lTextures[_TextureID].Destroy();
	{
		M_LOCK(m_DynamicLoadListLock);
		CRenderPS3Resource_Texture* pTexture = m_lpTexResources[_TextureID];
		m_lTexturesDynamicLoadList.Remove(pTexture);
		m_lTexturesList.Remove(pTexture);

		m_lTexturesDestroyList.Insert(pTexture);
		m_lpTexResources[_TextureID] = 0;
	}
	m_spTCIDInfo->m_pTCIDInfo[_TextureID].m_Fresh &= ~1;
}

void CContext_Texture::SetTextureParameters(const CTextureGCM* _pTexGCM, CRenderPS3Resource_Texture* _pResource, const CTC_TextureProperties& _Properties)
{

	switch(_pTexGCM->m_Type)
	{
	case CTextureGCM::TEX_TYPE_1D:
	case CTextureGCM::TEX_TYPE_3D:
	case CTextureGCM::TEX_TYPE_VOID:	M_BREAKPOINT; break;

	case CTextureGCM::TEX_TYPE_2D:	break;
	case CTextureGCM::TEX_TYPE_CUBE:	break;
	}

	if(_Properties.m_Flags & CTC_TEXTUREFLAGS_RENDERTARGET)
	{
		// RenderTarget texture, flag it as tiled
//		glTexParameteri(GLObjectTarget, GL_TEXTURE_ALLOCATION_HINT_SCE, GL_TEXTURE_TILED_GPU_SCE);
	}
	else if(_Properties.m_Flags & CTC_TEXTUREFLAGS_PROCEDURAL)
	{
		// Procedural texture, flag it as linear system
//		glTexParameteri(GLObjectTarget, GL_TEXTURE_ALLOCATION_HINT_SCE, GL_TEXTURE_LINEAR_SYSTEM_SCE);
	}

	if(_pTexGCM->m_Format == CTextureGCM::TEX_FORMAT_Z24S8)
	{
		// ZBuffer is special case
		_pResource->m_MinFilter = CELL_GCM_TEXTURE_NEAREST;
		_pResource->m_MagFilter = CELL_GCM_TEXTURE_NEAREST;
		_pResource->m_ConvFilter = CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX;
		_pResource->m_WrapS = CELL_GCM_TEXTURE_CLAMP_TO_EDGE;
		_pResource->m_WrapT = CELL_GCM_TEXTURE_CLAMP_TO_EDGE;
		_pResource->m_WrapR = CELL_GCM_TEXTURE_CLAMP_TO_EDGE;
		return;
	}


	if(_pTexGCM->m_Type == CTextureGCM::TEX_TYPE_CUBE)
	{
		_pResource->m_WrapS = CELL_GCM_TEXTURE_CLAMP_TO_EDGE;
		_pResource->m_WrapT = CELL_GCM_TEXTURE_CLAMP_TO_EDGE;
		_pResource->m_WrapR = CELL_GCM_TEXTURE_CLAMP_TO_EDGE;
	}
	else
	{
		_pResource->m_WrapS = (_Properties.m_Flags & CTC_TEXTUREFLAGS_CLAMP_U)?CELL_GCM_TEXTURE_CLAMP_TO_EDGE:CELL_GCM_TEXTURE_WRAP;
		_pResource->m_WrapT = (_Properties.m_Flags & CTC_TEXTUREFLAGS_CLAMP_V)?CELL_GCM_TEXTURE_CLAMP_TO_EDGE:CELL_GCM_TEXTURE_WRAP;
		_pResource->m_WrapR = CELL_GCM_TEXTURE_CLAMP_TO_EDGE;
	}

	uint32 MagFilter = CELL_GCM_TEXTURE_LINEAR;
	uint32 MinFilter = CELL_GCM_TEXTURE_LINEAR_LINEAR;
	switch(CRCPS3GCM::ms_This.m_Mode.m_Filter)
	{
	case CRC_GLOBALFILTER_POINT:
		{
			MagFilter = CELL_GCM_TEXTURE_NEAREST;
			MinFilter = CELL_GCM_TEXTURE_NEAREST;
			break;
		}
	case CRC_GLOBALFILTER_BILINEAR:
		{
			MagFilter = CELL_GCM_TEXTURE_LINEAR;
			MinFilter = CELL_GCM_TEXTURE_LINEAR;
			break;
		}
	case CRC_GLOBALFILTER_TRILINEAR:
		{
			MagFilter = CELL_GCM_TEXTURE_LINEAR;
			MinFilter = CELL_GCM_TEXTURE_LINEAR_LINEAR;
			break;
		}
	}


	switch(_Properties.m_MagFilter)
	{
	case CTC_MAGFILTER_NEAREST : MagFilter = CELL_GCM_TEXTURE_NEAREST; break;
	case CTC_MAGFILTER_LINEAR : MagFilter = CELL_GCM_TEXTURE_LINEAR; break;
	case CTC_MAGFILTER_ANISOTROPIC : MagFilter = CELL_GCM_TEXTURE_LINEAR; break;
	}

	if ((_Properties.m_Flags & CTC_TEXTUREFLAGS_NOMIPMAP) || (_pTexGCM->m_MipCount == 1))
	{
		MinFilter = CELL_GCM_TEXTURE_LINEAR;
		// No MIPmaps.
		switch(_Properties.m_MinFilter)
		{
		case CTC_MINFILTER_NEAREST : MinFilter = CELL_GCM_TEXTURE_NEAREST; break;
		case CTC_MINFILTER_ANISOTROPIC :
		case CTC_MINFILTER_LINEAR : MinFilter = CELL_GCM_TEXTURE_LINEAR;  break;
		}
	}
	else
	{
		// MIP-Mapping
		switch(_Properties.m_MinFilter)
		{
		case CTC_MINFILTER_NEAREST : MinFilter = (_Properties.m_MIPFilter == CTC_MIPFILTER_LINEAR) ? CELL_GCM_TEXTURE_NEAREST_LINEAR : CELL_GCM_TEXTURE_NEAREST_NEAREST; break;
		case CTC_MINFILTER_ANISOTROPIC :
		case CTC_MINFILTER_LINEAR : MinFilter = (_Properties.m_MIPFilter == CTC_MIPFILTER_LINEAR) ? CELL_GCM_TEXTURE_LINEAR_LINEAR : CELL_GCM_TEXTURE_LINEAR_NEAREST; break;
		}
	}

	_pResource->m_MinFilter = MinFilter;
	_pResource->m_MagFilter = MagFilter;
	_pResource->m_ConvFilter = CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX;

	{
 		fp32 Anisotropy = Max(1.0f, Min(m_MaxAnisotropy, LERP(1.0f, m_MaxAnisotropy, m_Anisotropy)));
		if (_Properties.m_Flags & (CTC_TEXTUREFLAGS_NOMIPMAP | CTC_TEXTUREFLAGS_CLAMP_U | CTC_TEXTUREFLAGS_CLAMP_V))
			Anisotropy = 1;

		static uint8 AnisoLookup[] =
		{
			1,	// CELL_GCM_TEXTURE_MAX_ANISO_1	= (0),
			2,	// CELL_GCM_TEXTURE_MAX_ANISO_2	= (1),
			4,	// CELL_GCM_TEXTURE_MAX_ANISO_4	= (2),
			6,	// CELL_GCM_TEXTURE_MAX_ANISO_6	= (3),
			8,	// CELL_GCM_TEXTURE_MAX_ANISO_8	= (4),
			10,	// CELL_GCM_TEXTURE_MAX_ANISO_10	= (5),
			12,	// CELL_GCM_TEXTURE_MAX_ANISO_12	= (6),
			16,	// CELL_GCM_TEXTURE_MAX_ANISO_16	= (7),
		};

		uint Aniso = TruncToInt(Anisotropy);
		uint iAniso = 0;
		for(iAniso = 7; iAniso > 0; iAniso--)
		{
			if(AnisoLookup[iAniso] <= Aniso)
				break;
		}

		_pResource->m_MaxAnisoFiltering = iAniso;
	}
}

void CContext_Texture::DeleteTextures()
{
	M_LOCK(m_Lock);
	int nTxt = m_lTextures.Len();
	for(int i = 1; i < nTxt; i++)
	{
		int Flags = m_pTC->GetTextureFlags(i);

		if (Flags & CTC_TXTIDFLAGS_ALLOCATED)
		{
			CTC_TextureProperties Properties;
			m_pTC->GetTextureProperties(i, Properties);
			Destroy(i);

			if(Properties.m_Flags & CTC_TEXTUREFLAGS_CUBEMAPCHAIN)
				i	+= 5;
		}
	}
}

void CContext_Texture::DestroyTextures()
{
	M_LOCK(m_DynamicLoadListLock);
	DLinkD_List(CRenderPS3Resource_Texture, m_Link) Remaining;
	CRenderPS3Resource_Texture* pTexture = m_lTexturesDestroyList.Pop();
	while(pTexture)
	{
		uint32 LastUsedFrame;
		if(pTexture->IsLocal())
			LastUsedFrame = pTexture->m_ResourceLocal.m_LastUsedFrame;
		else
			LastUsedFrame = pTexture->m_ResourceMain.m_LastUsedFrame;

		if(LastUsedFrame < CRenderPS3Resource::ms_ResourceFrameIndex)
		{
			pTexture->Destroy();
			delete pTexture;
		}
		else
			Remaining.Insert(pTexture);
		pTexture = m_lTexturesDestroyList.Pop();
	}

	
	pTexture = Remaining.Pop();
	while(pTexture)
	{
		m_lTexturesDestroyList.Insert(pTexture);
		pTexture = Remaining.Pop();
	}
}

void CContext_Texture::PostDefrag()
{
	int nTxt = m_lTextures.Len();
	for(int i = 1; i < nTxt; i++)
	{
		if(m_lpTexResources[i])
			m_lpTexResources[i]->PostDefrag();
	}
}

static int GetMipMapLevels(uint _w, uint _h)
{
	uint l2w = Log2(_w);
	uint l2h = Log2(_h);
	return Max(l2w, l2h) + 1;
}

bool CContext_Texture::LoadTextureCubeMip0(const CRenderPS3Resource_Texture* _pResource, CTextureContainer* _pTC, uint _iLocal, uint _iFace)
{
	uint _TextureID = _pResource->m_TextureID;
	uint _iFirstMip = _pResource->m_iFirstMip;
	uint _TextureVersion = _pResource->m_TexVersion;

	uint iFace = _iFace;
	uint nFace = 1;
	if(_iFace == ~0)
	{
		iFace = 0;
		nFace = 6;
	}

	uint TextureFlags = m_lTextures[_TextureID].m_Flags;

	uint32 Fmt = m_lTextures[_TextureID].m_Format;

	{
		CImage* pTexture = _pTC->GetTextureNumMip(_iLocal, _iFirstMip, 1, _TextureVersion);
		uint32 MemModel = pTexture->GetMemModel();

		if(MemModel & IMAGE_MEM_COMPRESSED)
		{
			if(MemModel & IMAGE_MEM_COMPRESSTYPE_S3TC)
			{
				uint8* pLocked = (uint8*)pTexture->LockCompressed();
				CImage_CompressHeader_S3TC *pHeader = (CImage_CompressHeader_S3TC *)pLocked;
				uint8* pData = pLocked + pHeader->getOffsetData();

				// Need to build the texture
				int CompressType = pHeader->getCompressType();
				switch(CompressType)
				{
				case IMAGE_COMPRESSTYPE_S3TC_DXT1 : Fmt = CTextureGCM::TEX_FORMAT_DXT1; break;
				case IMAGE_COMPRESSTYPE_S3TC_DXT3 : Fmt = CTextureGCM::TEX_FORMAT_DXT23; break;
				case IMAGE_COMPRESSTYPE_S3TC_DXT5 : Fmt = CTextureGCM::TEX_FORMAT_DXT45; break;
				default : Error_static("CRenderContextPS3::LoadTexture", CStrF("Unsupported S3TC compression format: %d", pHeader->getCompressType())); break;
				}

				m_TextureUploader.UploadS3TC(_pResource, iFace, nFace, 0, Fmt, TextureFlags, pData);
				pTexture->Unlock();
			}
			else
				Error_static("CRenderContextPS3::LoadTexture", "Unsupported compression.");
		}
		else
		{
			uint8* pData = (uint8*)pTexture->Lock();
			if(TextureFlags & CTextureGCM::TEX_FL_LINEAR)
				m_TextureUploader.UploadLinear(_pResource, iFace, nFace, 0, Fmt, TextureFlags, pData);
			else
				m_TextureUploader.UploadSwizzle(_pResource, iFace, nFace, 0, Fmt, TextureFlags, pData);

			pTexture->Unlock();
		}
		_pTC->ReleaseTexture(_iLocal, _iFirstMip, _TextureVersion);
	}

	return true;
}

bool CContext_Texture::LoadTextureCube(CRenderPS3Resource_Texture* _pResource, uint32 _TextureID, CTextureContainer* _pTC, uint _iLocal, uint _TextureVersion, const CTC_TextureProperties& _Properties, uint _iFirstMip, uint _Width, uint _Height, uint _nMip)
{
	uint32 SkippedMips = m_lTextures[_TextureID].m_SkippedMips;
	_iFirstMip	+= SkippedMips;
	if(SkippedMips >= _nMip)
		_nMip = 1;
	else
		_nMip -= SkippedMips;

	_pResource->m_iFirstMip = _iFirstMip;
	_pResource->m_TexVersion = _TextureVersion;

	if(_nMip <= 0)
		M_BREAKPOINT;

	uint iFace = 0;
	uint nFace = 1;
	// Detect which faces to update
	if(m_lTextures[_TextureID].m_Flags & CTextureGCM::TEX_FL_CUBECHAIN)
	{
		iFace = m_lTextures[_TextureID].m_CubeFace;
	}
	else
		nFace = 6;

	uint TextureFlags = m_lTextures[_TextureID].m_Flags;

	uint32 Fmt = m_lTextures[_TextureID].m_Format;
	uint iMip = 1;
	if((TextureFlags & CTextureGCM::TEX_FL_MIP0LOADED) || _nMip == 1)
		iMip = 0;

	{
		CImage* pTexture = _pTC->GetTexture(_iLocal, _iFirstMip + iMip, _TextureVersion);
		uint32 MemModel = pTexture->GetMemModel();

//		Fmt = CTextureGCM::TranslateFormat(pTexture->GetFormat());
		if(MemModel & IMAGE_MEM_COMPRESSED)
		{
			if(MemModel & IMAGE_MEM_COMPRESSTYPE_S3TC)
			{
				uint8* pLocked = (uint8*)pTexture->LockCompressed();
				CImage_CompressHeader_S3TC *pHeader = (CImage_CompressHeader_S3TC *)pLocked;
				uint8* pData = pLocked + pHeader->getOffsetData();

				// Need to build the texture
				int CompressType = pHeader->getCompressType();
				switch(CompressType)
				{
				case IMAGE_COMPRESSTYPE_S3TC_DXT1 : Fmt = CTextureGCM::TEX_FORMAT_DXT1; break;
				case IMAGE_COMPRESSTYPE_S3TC_DXT3 : Fmt = CTextureGCM::TEX_FORMAT_DXT23; break;
				case IMAGE_COMPRESSTYPE_S3TC_DXT5 : Fmt = CTextureGCM::TEX_FORMAT_DXT45; break;
				default : Error_static("CRenderContextPS3::LoadTexture", CStrF("Unsupported S3TC compression format: %d", pHeader->getCompressType())); break;
				}

				if(TextureFlags & CTextureGCM::TEX_FL_CUBEOWNER)
				{
					SetCompressedFormat(_TextureID, Fmt);
					_pResource->AllocCompressedCube(_Width, _nMip, Fmt);
					_pResource->m_MinLOD = iMip;
					_pResource->m_MaxLOD = _nMip;
				}

				m_TextureUploader.UploadS3TC(_pResource, iFace, nFace, iMip, Fmt, TextureFlags, pData);
				pTexture->Unlock();
			}
			else
				Error_static("CRenderContextPS3::LoadTexture", "Unsupported compression.");
		}
		else
		{
			if(TextureFlags & CTextureGCM::TEX_FL_CUBEOWNER)
			{
				_pResource->AllocCube(_Width, _nMip, Fmt);
				_pResource->m_MinLOD = iMip;
				_pResource->m_MaxLOD = _nMip;
			}

			uint8* pData = (uint8*)pTexture->Lock();
			if(TextureFlags & CTextureGCM::TEX_FL_LINEAR)
				m_TextureUploader.UploadLinear(_pResource, iFace, nFace, iMip, Fmt, TextureFlags, pData);
			else
				m_TextureUploader.UploadSwizzle(_pResource, iFace, nFace, iMip, Fmt, TextureFlags, pData);

			pTexture->Unlock();
		}
		_pTC->ReleaseTexture(_iLocal, _iFirstMip + iMip, _TextureVersion);
		iMip++;
	}

	for(; iMip < _nMip; iMip++)
	{
		CImage* pTexture = _pTC->GetTexture(_iLocal, _iFirstMip + iMip, _TextureVersion);
		uint32 MemModel = pTexture->GetMemModel();

		uint32 w = pTexture->GetWidth();
		uint32 h = pTexture->GetHeight();
		if(MemModel & IMAGE_MEM_COMPRESSED)
		{
			if(MemModel & IMAGE_MEM_COMPRESSTYPE_S3TC)
			{
				uint8* pLocked = (uint8*)pTexture->LockCompressed();
				CImage_CompressHeader_S3TC *pHeader = (CImage_CompressHeader_S3TC *)pLocked;
				uint8* pData = pLocked + pHeader->getOffsetData();
				m_TextureUploader.UploadS3TC(_pResource, iFace, nFace, iMip, Fmt, TextureFlags, pData);
			}
			else
				Error_static("CRenderContextPS3::LoadTexture", "Unsupported compression.");
		}
		else
		{
			uint8* pData = (uint8*)pTexture->Lock();
			if(TextureFlags & CTextureGCM::TEX_FL_LINEAR)
				m_TextureUploader.UploadLinear(_pResource, iFace, nFace, iMip, Fmt, TextureFlags, pData);
			else
				m_TextureUploader.UploadSwizzle(_pResource, iFace, nFace, iMip, Fmt, TextureFlags, pData);
		}
		pTexture->Unlock();
		_pTC->ReleaseTexture(_iLocal, _iFirstMip + iMip, _TextureVersion);
	}

	return true;
}

bool CContext_Texture::LoadTexture(CRenderPS3Resource_Texture* _pResource, uint32 _TextureID, CTextureContainer* _pTC, uint _iLocal, uint _TextureVersion, const CTC_TextureProperties& _Properties, uint _iFirstMip, uint _Width, uint _Height, uint _nMip)
{
	uint32 SkippedMips = m_lTextures[_TextureID].m_SkippedMips;
	_iFirstMip	+= SkippedMips;
	if(SkippedMips >= _nMip)
		_nMip = 1;
	else
		_nMip -= SkippedMips;

	_pResource->m_iFirstMip = _iFirstMip;
	_pResource->m_TexVersion = _TextureVersion;

	if(_nMip <= 0)
		M_BREAKPOINT;

	uint TextureFlags = m_lTextures[_TextureID].m_Flags;

	uint32 Fmt;
	Fmt = m_lTextures[_TextureID].m_Format;
	uint iMip = 1;
	if((TextureFlags & CTextureGCM::TEX_FL_MIP0LOADED) || _nMip == 1)
		iMip = 0;

	{
		CImage* pTexture = _pTC->GetTexture(_iLocal, _iFirstMip + iMip, _TextureVersion);
		uint32 MemModel = pTexture->GetMemModel();

//		Fmt = CTextureGCM::TranslateFormat(pTexture->GetFormat());
		if(MemModel & IMAGE_MEM_COMPRESSED)
		{
			if(MemModel & IMAGE_MEM_COMPRESSTYPE_S3TC)
			{
				uint8* pLocked = (uint8*)pTexture->LockCompressed();
				CImage_CompressHeader_S3TC *pHeader = (CImage_CompressHeader_S3TC *)pLocked;
				uint8* pData = pLocked + pHeader->getOffsetData();

				// Need to build the texture
				int CompressType = pHeader->getCompressType();
				switch(CompressType)
				{
				case IMAGE_COMPRESSTYPE_S3TC_DXT1 : Fmt = CTextureGCM::TEX_FORMAT_DXT1; break;
				case IMAGE_COMPRESSTYPE_S3TC_DXT3 : Fmt = CTextureGCM::TEX_FORMAT_DXT23; break;
				case IMAGE_COMPRESSTYPE_S3TC_DXT5 : Fmt = CTextureGCM::TEX_FORMAT_DXT45; break;
				default : Error_static("CRenderContextPS3::LoadTexture", CStrF("Unsupported S3TC compression format: %d", pHeader->getCompressType())); break;
				}

				SetCompressedFormat(_TextureID, Fmt);
				_pResource->AllocCompressed2D(_Width, _Height, _nMip, Fmt);

				m_TextureUploader.UploadS3TC(_pResource, 0, 1, iMip, Fmt, TextureFlags, pData);
				pTexture->Unlock();
			}
			else
				Error_static("CRenderContextPS3::LoadTexture", "Unsupported compression.");
		}
		else
		{
			uint8* pData = (uint8*)pTexture->Lock();
			uint AllocFlags = 0;
			if(TextureFlags & CTextureGCM::TEX_FL_LINEAR)
				AllocFlags |= GCM_TEXALLOC_FORCELINEAR;
			if(TextureFlags & CTextureGCM::TEX_FL_TILE)
				AllocFlags |= GCM_TEXALLOC_TILE;
			if(TextureFlags & CTextureGCM::TEX_FL_LINEAR)
			{
				_pResource->Alloc2D(_Width, _Height, _nMip, Fmt, AllocFlags);

				m_TextureUploader.UploadLinear(_pResource, 0, 1, iMip, Fmt, TextureFlags, pData);
			}
			else
			{
				_pResource->Alloc2D(_Width, _Height, _nMip, Fmt, AllocFlags);

				if(TextureFlags & CTextureGCM::TEX_FL_POW2)
					m_TextureUploader.UploadSwizzle(_pResource, 0, 1, iMip, Fmt, TextureFlags, pData);
				else
					m_TextureUploader.UploadLinear(_pResource, 0, 1, iMip, Fmt, TextureFlags, pData);
			}
			pTexture->Unlock();
		}
		_pTC->ReleaseTexture(_iLocal, _iFirstMip + iMip, _TextureVersion);
		_pResource->m_MinLOD = iMip;
		_pResource->m_MaxLOD = _nMip;
		iMip++;
	}

	for(; iMip < _nMip; iMip++)
	{
		CImage* pTexture = _pTC->GetTexture(_iLocal, _iFirstMip + iMip, _TextureVersion);
		uint32 MemModel = pTexture->GetMemModel();

		uint32 w = pTexture->GetWidth();
		uint32 h = pTexture->GetHeight();
		if(MemModel & IMAGE_MEM_COMPRESSED)
		{
			if(MemModel & IMAGE_MEM_COMPRESSTYPE_S3TC)
			{
				uint8* pLocked = (uint8*)pTexture->LockCompressed();
				CImage_CompressHeader_S3TC *pHeader = (CImage_CompressHeader_S3TC *)pLocked;
				uint8* pData = pLocked + pHeader->getOffsetData();

				m_TextureUploader.UploadS3TC(_pResource, 0, 1, iMip, Fmt, TextureFlags, pData);
			}
			else
				Error_static("CRenderContextPS3::LoadTexture", "Unsupported compression.");
		}
		else
		{
			uint8* pData = (uint8*)pTexture->Lock();
			if((TextureFlags & CTextureGCM::TEX_FL_LINEAR) || !(TextureFlags & CTextureGCM::TEX_FL_POW2))
				m_TextureUploader.UploadLinear(_pResource, 0, 1, iMip, Fmt, TextureFlags, pData);
			else
				m_TextureUploader.UploadSwizzle(_pResource, 0, 1, iMip, Fmt, TextureFlags, pData);
		}
		pTexture->Unlock();
		_pTC->ReleaseTexture(_iLocal, _iFirstMip + iMip, _TextureVersion);
	}

	return true;
}

bool CContext_Texture::LoadTextureMip0(const CRenderPS3Resource_Texture* _pResource, CTextureContainer* _pTC, uint _iLocal)
{
	uint TextureID = _pResource->m_TextureID;
	uint TextureVersion = _pResource->m_TexVersion;
	uint iFirstMip = _pResource->m_iFirstMip;
	uint TextureFlags = m_lTextures[TextureID].m_Flags;

	uint32 Fmt;
	Fmt = m_lTextures[TextureID].m_Format;

	{
		CImage* pTexture = _pTC->GetTextureNumMip(_iLocal, iFirstMip, 1, TextureVersion);
		uint32 MemModel = pTexture->GetMemModel();

		if(MemModel & IMAGE_MEM_COMPRESSED)
		{
			if(MemModel & IMAGE_MEM_COMPRESSTYPE_S3TC)
			{
				uint8* pLocked = (uint8*)pTexture->LockCompressed();
				CImage_CompressHeader_S3TC *pHeader = (CImage_CompressHeader_S3TC *)pLocked;
				uint8* pData = pLocked + pHeader->getOffsetData();

				m_TextureUploader.UploadS3TC(_pResource, 0, 1, 0, Fmt, TextureFlags, pData);
				pTexture->Unlock();
			}
			else
				Error_static("CRenderContextPS3::LoadTexture", "Unsupported compression.");
		}
		else
		{
			uint8* pData = (uint8*)pTexture->Lock();
			if(TextureFlags & CTextureGCM::TEX_FL_LINEAR)
			{
				m_TextureUploader.UploadLinear(_pResource, 0, 1, 0, Fmt, TextureFlags, pData);
			}
			else
			{
				if(TextureFlags & CTextureGCM::TEX_FL_POW2)
					m_TextureUploader.UploadSwizzle(_pResource, 0, 1, 0, Fmt, TextureFlags, pData);
				else
					m_TextureUploader.UploadLinear(_pResource, 0, 1, 0, Fmt, TextureFlags, pData);
			}
			pTexture->Unlock();
		}
		_pTC->ReleaseTexture(_iLocal, iFirstMip, TextureVersion);
	}

	return true;
}

bool CContext_Texture::LoadTextureBuildInto(CRenderPS3Resource_Texture* _pResource, uint32 _TextureID, CTextureContainer* _pTC, uint _iLocal, const CImage& _TxtDesc, const CTC_TextureProperties& _Properties, uint _iFirstMip, uint _Width, uint _Height, uint _nMip)
{
	// Check textureflags, if backbuffer then allocate in videoram and maybe ignore buildinto call?
	if(_pResource->NeedRebuild(_Width, _Height, m_lTextures[_TextureID].m_Format))
	{
		_pResource->Destroy();
		if(m_lTextures[_TextureID].m_Type == CTextureGCM::TEX_TYPE_CUBE)
		{
			_pResource->AllocCube(Min(_Width, _Height), 1, m_lTextures[_TextureID].m_Format, GCM_TEXALLOC_FORCELINEAR);
			for(uint i = 0; i < 6; i++)
				memset(_pResource->GetMipMemory(i, 0), 0, _pResource->GetMipSize(0));
		}
		else
		{
			uint TextureFlags = m_lTextures[_TextureID].m_Flags;
			uint AllocFlags = 0;
			if(TextureFlags & CTextureGCM::TEX_FL_LINEAR)
				AllocFlags |= GCM_TEXALLOC_FORCELINEAR;
			if(TextureFlags & CTextureGCM::TEX_FL_TILE)
				AllocFlags |= GCM_TEXALLOC_TILE;

			M_ASSERT(AllocFlags & GCM_TEXALLOC_FORCELINEAR, "!");
			_pResource->Alloc2D(_Width, _Height, 1, m_lTextures[_TextureID].m_Format, AllocFlags);
		}
	}

	return true;
}

bool CContext_Texture::LoadTextureRender(CRenderPS3Resource_Texture* _pResource, uint32 _TextureID, CTextureContainer* _pTC, uint _iLocal, const CImage& _TxtDesc, const CTC_TextureProperties& _Properties, uint _iFirstMip, uint _Width, uint _Height, uint _nMip)
{
	int w = _Width;
	int h = _Height;
	int ww = CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_Width;
	int wh = CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_Height;
//	int ww = CDCPS3GCM::ms_This.m_FrontBuffer.m_Width;
//	int wh = CDCPS3GCM::ms_This.m_FrontBuffer.m_Height;

	if(_pResource->NeedRebuild(Min(w, ww), Min(h, wh), m_lTextures[_TextureID].m_Format))
	{
		_pResource->Destroy();
		if(m_lTextures[_TextureID].m_Type == CTextureGCM::TEX_TYPE_CUBE)
		{
			_pResource->AllocCube(Min(Min(w, ww), Min(h, wh)), 1, m_lTextures[_TextureID].m_Format, GCM_TEXALLOC_FORCELINEAR);
			for(uint i = 0; i < 6; i++)
				memset(_pResource->GetMipMemory(i, 0), 0, _pResource->GetMipSize(0));
		}
		else
		{
			uint TextureFlags = m_lTextures[_TextureID].m_Flags;
			uint AllocFlags = 0;
			if(TextureFlags & CTextureGCM::TEX_FL_LINEAR)
				AllocFlags |= GCM_TEXALLOC_FORCELINEAR;
			if(TextureFlags & CTextureGCM::TEX_FL_TILE)
				AllocFlags |= GCM_TEXALLOC_TILE;

			M_ASSERT(AllocFlags & GCM_TEXALLOC_FORCELINEAR, "!");
			_pResource->Alloc2D(Min(w, ww), Min(h, wh), 1, m_lTextures[_TextureID].m_Format, AllocFlags);
		}
	}

	uint32 Mode = CELL_GCM_TRANSFER_MAIN_TO_LOCAL;
	if(_pResource->IsLocal())
	{
		if(CDCPS3GCM::ms_This.m_FrontBuffer.IsLocal())
			Mode = CELL_GCM_TRANSFER_LOCAL_TO_LOCAL;
		else
			Mode = CELL_GCM_TRANSFER_LOCAL_TO_MAIN;
	}
	else
	{
		if(CDCPS3GCM::ms_This.m_FrontBuffer.IsLocal())
			Mode = CELL_GCM_TRANSFER_MAIN_TO_LOCAL;
		else
			M_BREAKPOINT;
	}

	uint Width = Min(ww, w);
	uint PixelSize = ConvertSizeFromPixels(1, m_lTextures[_TextureID].m_Format);
	while(PixelSize > 4)
	{
		PixelSize >>= 1;
		Width <<= 1;
	}

	uint SrcOffset = CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_ColorOffset[0];
//	uint SrcPitch = CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_Width * PixelSize;
	uint SrcPitch = CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_ColorPitch[0];
	if(m_lTextures[_TextureID].m_Format == CTextureGCM::TEX_FORMAT_Z24S8)
	{
		SrcOffset = CDCPS3GCM::ms_This.m_ZBuffer.GetOffset();
		SrcPitch = CDCPS3GCM::ms_This.m_ZBuffer.GetPitch();
	}

	gcmSetTransferImage(Mode,
					_pResource->GetOffset(), _pResource->GetPitch(), 0, 0,
					SrcOffset, SrcPitch, 0, wh - Min(wh, h),
					Width, Min(wh, h), PixelSize);
	gcmSetWriteBackEndLabel(GPU_LABEL_TRASH, 0);
	gcmSetInvalidateTextureCache(CELL_GCM_INVALIDATE_TEXTURE);
	return true;
}

void CContext_Texture::Build(uint32 _TextureID)
{
	M_LOCK(m_Lock);
	int TxtIDFlags = m_pTC->GetTextureFlags(_TextureID);

	if (!(TxtIDFlags & CTC_TXTIDFLAGS_ALLOCATED))
		return;

	CRC_IDInfo* pIDInfo = &m_spTCIDInfo->m_pTCIDInfo[_TextureID];
	if(pIDInfo->m_Fresh & 1)
		return;

	if(!m_bPrecaching)
	{
		M_TRACEALWAYS("WARNING: Runtime loading texture %d (%s)\n", _TextureID, m_pTC->GetName(_TextureID).GetStr());
	}

	CTC_TextureProperties Properties;
	m_pTC->GetTextureProperties(_TextureID, Properties);
	Build(_TextureID, Properties);
}


void CContext_Texture::Build(uint32 _TextureID, const CTC_TextureProperties& _Properties)
{
	if(IsBuilt(_TextureID))
		Destroy(_TextureID);

	int iPicMip = MinMT(MaxMT(0, _Properties.m_iPicMipGroup), CRC_MAXPICMIPS - 1);
	int PicMip = (_Properties.m_Flags & CTC_TEXTUREFLAGS_NOPICMIP) ? 0 : MaxMT(0, m_lPicMips[iPicMip] + _Properties.GetPicMipOffset());

	CImage TxtDesc;
	int nMipMaps;
	m_pTC->GetTextureDesc(_TextureID, &TxtDesc, nMipMaps);

	CTextureContainer *pTContainer = m_pTC->GetTextureContainer(_TextureID);
	int iLocal = m_pTC->GetLocal(_TextureID);

	if (!pTContainer)
		return;

	uint TexWidth = TxtDesc.GetWidth();
	uint TexHeight = TxtDesc.GetHeight();

	bool bProcedural = 
		(_Properties.m_Flags & CTC_TEXTUREFLAGS_PROCEDURAL)
		&& 
		!(_Properties.m_Flags & (CTC_TEXTUREFLAGS_CUBEMAPCHAIN | CTC_TEXTUREFLAGS_CUBEMAP))
		&& 
		(nMipMaps == 1);

	uint w = Max((uint)1, TexWidth >> PicMip);
	uint h = Max((uint)1, TexHeight >> PicMip);

	uint iFirstMipMap = 0;
	uint w2 = TexWidth;
	uint h2 = TexHeight;
	while (w2 > w || h2 > h)
	{
		w2 = w2 >> 1;
		h2 = h2 >> 1;
		iFirstMipMap++;
	}

	uint nMip = 1;
	if (_Properties.m_Flags & (CTC_TEXTUREFLAGS_NOMIPMAP | CTC_TEXTUREFLAGS_PROCEDURAL | CTC_TEXTUREFLAGS_RENDER))
		nMip = 1;
	else if (!bProcedural)
	{
		nMip = GetMipMapLevels(w, h);
	}

	Build(_TextureID, iFirstMipMap, nMip);
	CRenderPS3Resource_Texture* pTexResource = DNew(CRenderPS3Resource_Texture) CRenderPS3Resource_Texture(_TextureID);

	SetTextureParameters(&m_lTextures[_TextureID], pTexResource, _Properties);

	if(bProcedural)
	{
		// Make sure these aren't streamed
		m_lTextures[_TextureID].m_Flags |= CTextureGCM::TEX_FL_MIP0LOADED;
		if(!LoadTextureBuildInto(pTexResource, _TextureID, pTContainer, iLocal, TxtDesc, _Properties, iFirstMipMap, w, h, nMip))
		{
			delete pTexResource;
			return;
		}
	}
	else if (_Properties.m_Flags & CTC_TEXTUREFLAGS_RENDER)
	{
		// Make sure these aren't streamed
		m_lTextures[_TextureID].m_Flags |= CTextureGCM::TEX_FL_MIP0LOADED;
		m_pTC->BuildInto(_TextureID, &CRCPS3GCM::ms_This);
		if(!LoadTextureRender(pTexResource, _TextureID, pTContainer, iLocal, TxtDesc, _Properties, iFirstMipMap, w, h, nMip))
		{
			delete pTexResource;
			return;
		}
	}
	else
	{
		// if NOPICMIP then make sure to load all mips, otherwise skip mip0
		if (!m_bMip0Streaming || _Properties.m_Flags & CTC_TEXTUREFLAGS_NOPICMIP)
			m_lTextures[_TextureID].m_Flags |= CTextureGCM::TEX_FL_MIP0LOADED;

		uint8 Versions[CTC_TEXTUREVERSION_MAX];
		int nVersions = m_pTC->EnumTextureVersions(_TextureID, Versions, CTC_TEXTUREVERSION_MAX);
		if (!nVersions)
		{
			ConOut("EnumTextureVersions error 0");
			delete pTexResource;
			return;
		}

		uint8 TextureVersion = Versions[0];

		uint8 TextureVersionPriority[] = {CTC_TEXTUREVERSION_S3TC, CTC_TEXTUREVERSION_FLOAT, CTC_TEXTUREVERSION_RAW};

		int nPriority = sizeof(TextureVersionPriority) / sizeof(uint8);

		for (int i = 0; i < nPriority; ++i)
		{
			for (int j = 0; j < nVersions; ++j)
			{
                if (TextureVersionPriority[i] == Versions[j])
				{
					TextureVersion = Versions[j];
					i = nPriority;
					break;
				}
			}
		}

		if(m_lTextures[_TextureID].m_Type == CTextureGCM::TEX_TYPE_CUBE)
		{
			uint nTex = 1;
			if(m_lTextures[_TextureID].m_Flags & CTextureGCM::TEX_FL_CUBECHAIN)
			{
				nTex = 6;
			}
			CTextureContainer* pTC = m_pTC->GetTextureContainer(_TextureID);
			for(uint iT = 0; iT < nTex; iT++)
			{
				uint iL = iLocal + iT * nVersions;
				uint TexID = pTC->GetTextureID(iL);
				if (!m_bMip0Streaming || _Properties.m_Flags & CTC_TEXTUREFLAGS_NOPICMIP)
					m_lTextures[TexID].m_Flags |= CTextureGCM::TEX_FL_MIP0LOADED;

				if(!LoadTextureCube(pTexResource, TexID, pTContainer, iL, TextureVersion, _Properties, iFirstMipMap, w, h, nMip))
				{
					delete pTexResource;
					return;
				}
			}
		}
		else
		{
			if(!LoadTexture(pTexResource, _TextureID, pTContainer, iLocal, TextureVersion, _Properties, iFirstMipMap, w, h, nMip))
			{
				if(!LoadTextureBuildInto(pTexResource, _TextureID, pTContainer, iLocal, TxtDesc, _Properties, iFirstMipMap, w, h, nMip))
				{
					delete pTexResource;
					return;
				}
			}

		}

	}
	CRCPS3GCM::ms_This.m_ContextTexture.m_lpTexResources[_TextureID] = pTexResource;
	CRC_IDInfo* pIDInfo = &m_spTCIDInfo->m_pTCIDInfo[_TextureID];
	pIDInfo->m_Fresh |= 1;
	{
		M_LOCK(m_DynamicLoadListLock);
		if((m_lTextures[_TextureID].m_Flags & CTextureGCM::TEX_FL_MIP0LOADED) || nMip == 1)
		{
			m_lTexturesList.Insert(pTexResource);
		}
		else
		{
			m_lTexturesDynamicLoadList.Insert(pTexResource);
			m_DynamicLoadEvent.Signal();
		}
	}
}

CRenderPS3Resource_Texture* CContext_Texture::GetTexture(uint _TextureID)
{
	CRenderPS3Resource_Texture* pTex = m_lpTexResources[_TextureID];
	CRC_IDInfo* pIDInfo = &m_spTCIDInfo->m_pTCIDInfo[_TextureID];
	if (!pTex || !(pIDInfo->m_Fresh & 1))
	{
		M_LOCK(m_Lock);
		pTex = m_lpTexResources[_TextureID];
		if (!pTex || !(pIDInfo->m_Fresh & 1))
		{
			CTC_TextureProperties Properties;
			m_pTC->GetTextureProperties(_TextureID, Properties);
			if (!m_bAllowTextureLoad && !(Properties.m_Flags & (CTC_TEXTUREFLAGS_PROCEDURAL | CTC_TEXTUREFLAGS_RENDER)))
			{
				M_TRACEALWAYS("GL: Unprecached texture %s (%d)\n", m_pTC->GetName(_TextureID).Str(), _TextureID );
			}
			else
			{
				uint TexFlags = m_pTC->GetTextureFlags(_TextureID);
				M_TRACEALWAYS("GL: Unbuilt texture %s (%d), precache = %s\n", m_pTC->GetName(_TextureID).Str(), _TextureID, (TexFlags & CTC_TXTIDFLAGS_PRECACHE)?"true":"false" );

				Build(_TextureID, Properties);
				pTex = m_lpTexResources[_TextureID];
				M_ASSERT(pTex, "!");
			}
		}
	}

	if(pTex) pTex->Touch();
	return pTex;
}

void CContext_Texture::Bind(uint _iSampler, uint32 _TextureID)
{
	// Debug stuff
	CRCPS3GCM::ms_This.m_ContextStatistics.m_nBindTexture++;
	if(_TextureID)
	{
		CRenderPS3Resource_Texture* pTex = GetTexture(_TextureID);

		if(pTex)
		{
			pTex->Bind(_iSampler);
			return;
		}
	}

	// Disable sampler
	gcmSetTextureControl(_iSampler, CELL_GCM_FALSE, 0, 0, 0);
}

uint32 M_RESTRICT* CContext_Texture::RSX_Bind(uint32 M_RESTRICT* _pCmd, uint _iSampler, uint32 _TextureID)
{
	if(_TextureID)
	{
		// Debug stuff
		CRC_IDInfo* pIDInfo = &m_spTCIDInfo->m_pTCIDInfo[_TextureID];
		if (!(pIDInfo->m_Fresh & 1))
		{
			// Disable sampler
			RSX_TextureControlDisable(_pCmd, _iSampler);
		}
		else
			m_lpTexResources[_TextureID]->RSX_Bind(_pCmd, _iSampler);
	}
	else
	{
		// Disable sampler
		RSX_TextureControlDisable(_pCmd, _iSampler);
	}
	return _pCmd;
}

void CContext_Texture::RenderTarget_CopyToTexture(uint _TextureID, CRct _SrcRect, CPnt _Dest, uint _bContinueTiling, uint _Slice, uint _iMRT)
{
	Build(_TextureID);

	CRenderPS3Resource_Texture* pTex = GetTexture(_TextureID);

	uint32 Mode = CELL_GCM_TRANSFER_MAIN_TO_LOCAL;
	if(pTex->IsLocal())
	{
		if(CDCPS3GCM::ms_This.m_FrontBuffer.IsLocal())
			Mode = CELL_GCM_TRANSFER_LOCAL_TO_LOCAL;
		else
			Mode = CELL_GCM_TRANSFER_LOCAL_TO_MAIN;
	}
/*	else
	{
		if(CDCPS3GCM::ms_This.m_FrontBuffer.IsLocal())
			M_BREAKPOINT;
		else
			Mode = CELL_GCM_TRANSFER_MAIN_TO_LOCAL;
	}*/

	uint Width = Min((uint64)_SrcRect.GetWidth(), m_lTextures[_TextureID].m_Width);
	uint Height = Min((uint64)_SrcRect.GetHeight(), m_lTextures[_TextureID].m_Height);

	uint Offset = CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_ColorOffset[0];
	uint Pitch = CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_ColorPitch[0];
	if(m_lTextures[_TextureID].m_Format == CTextureGCM::TEX_FORMAT_Z24S8)
	{
		Offset = CDCPS3GCM::ms_This.m_ZBuffer.GetOffset();
		Pitch = CDCPS3GCM::ms_This.m_ZBuffer.GetPitch();
	}

	uint PixelSize = ConvertSizeFromPixels(1, m_lTextures[_TextureID].m_Format);
	while(PixelSize > 4)
	{
		PixelSize >>= 1;
		Width <<= 1;
	}

	if(m_lTextures[_TextureID].m_Type == CTextureGCM::TEX_TYPE_CUBE)
	{
		gcmSetTransferImage(Mode,
						pTex->GetMipOffset(_Slice, 0), pTex->GetPitch(), _Dest.x, _Dest.y,
						Offset, Pitch, _SrcRect.p0.x, _SrcRect.p0.y,
						Width, Height, PixelSize);
	}
	else
	{
		gcmSetTransferImage(Mode,
						pTex->GetOffset(), pTex->GetPitch(), _Dest.x, _Dest.y,
						Offset, Pitch, _SrcRect.p0.x, _SrcRect.p0.y,
						Width, Height, PixelSize);
	}
	gcmSetWriteBackEndLabel(GPU_LABEL_TRASH, 0);
	gcmSetInvalidateTextureCache(CELL_GCM_INVALIDATE_TEXTURE);
}

const char* CContext_Texture::Thread_GetName() const
{
	return "CContex_Texture::Mip0Loader";
}

int CContext_Texture::Thread_Main()
{
	m_QuitEvent.ReportTo(&m_DynamicLoadEvent);
	uint nLoadedTexture = 0;
	CMTime Timing;
	while(!Thread_IsTerminating())
	{
		bool bWait = true;

		if(!m_bPrecaching)
		{
			M_LOCK(CRCPS3GCM::ms_This.m_pTC->m_DeleteContainerLock);
			{
				CRenderPS3Resource_Texture* pTex = 0;
				{
					M_LOCK(m_DynamicLoadListLock);
					pTex = m_lTexturesDynamicLoadList.Pop();
				}

				if(pTex)
				{
					CTextureContainer* pTC = m_pTC->GetTextureContainer(pTex->m_TextureID);
					uint iLocal = m_pTC->GetLocal(pTex->m_TextureID);
					if(m_lTextures[pTex->m_TextureID].m_Type == CTextureGCM::TEX_TYPE_2D)
					{
						LoadTextureMip0(pTex, pTC, iLocal);
						pTex->m_MinLOD = 0;
					}
					else if(m_lTextures[pTex->m_TextureID].m_Type == CTextureGCM::TEX_TYPE_CUBE)
					{
						if(m_lTextures[pTex->m_TextureID].m_Flags & CTextureGCM::TEX_FL_CUBECHAIN)
						{
							// 6 separate textures
							uint8 lTexVersions[CTC_TEXTUREVERSION_MAX];
							uint Versions = m_pTC->EnumTextureVersions(pTex->m_TextureID, lTexVersions, CTC_TEXTUREVERSION_MAX);
							uint iL = iLocal;
							for(uint i = 0; i < 6; i++)
							{
								LoadTextureCubeMip0(pTex, pTC, iL, i);
								iL += Versions;
							}
						}
						else
							LoadTextureCubeMip0(pTex, pTC, iLocal, ~0);
						pTex->m_MinLOD = 0;
					}
					{
						M_LOCK(m_DynamicLoadListLock);
						m_lTexturesList.Insert(pTex);
					}
					bWait = false;
					if(nLoadedTexture == 0)
						Timing.Start();
					nLoadedTexture++;
				}
			}
		}

		if(bWait)
		{
			if(nLoadedTexture)
			{
				Timing.Stop();
				M_TRACEALWAYS("Finished loading %d textures in %f seconds\r\n", nLoadedTexture, Timing.GetTime());
				nLoadedTexture = 0;
			}
			m_DynamicLoadEvent.Wait();
		}
	}

	return 0;
}

// -------------------------------------------------------------------
// -------------------------------------------------------------------
static bool IsPow2(const CImage& _Img)
{
	int w = Log2(_Img.GetWidth());
	int h = Log2(_Img.GetHeight());

	return ((_Img.GetWidth() == (1 << w)) && (_Img.GetHeight() == (1 << h)));
}

static int NextPow2(int _Value)
{
	int i = 1;
	while(i < _Value)
		i	<<= 1;

	return i;
}

void CRCPS3GCM::Texture_Copy(int _SourceTexID, int _DestTexID, CRct _SrcRgn, CPnt _DstPos)
{
	MSCOPE(CRCPS3GCM::Texture_Copy, RENDER_PS3);
}

CRC_TextureMemoryUsage CContext_Texture::GetMem(int _TextureID)
{
	CRC_TextureMemoryUsage MemUsage;

	CRenderPS3Resource_Texture* pRes = m_lpTexResources[_TextureID];
	if(pRes)
	{
		CTextureGCM* pTex = &m_lTextures[_TextureID];
		MemUsage.m_pFormat = FormatStr[pTex->m_Format];
		MemUsage.m_BestCase = pRes->GetSize();
		MemUsage.m_Theoretical = pRes->GetSize();
		MemUsage.m_WorstCase = pRes->GetSize();
	}
	return MemUsage;
}

CRC_TextureMemoryUsage CRCPS3GCM::Texture_GetMem(int _TextureID)
{
	MSCOPE(CRCPS3GCM::Texture_GetMem, RENDER_PS3);
	return m_ContextTexture.GetMem(_TextureID);
}

int CRCPS3GCM::Texture_GetPicmipFromGroup(int _iPicmip)
{
	MSCOPE(CRCPS3GCM::Texture_GetPicmipFromGroup, RENDER_PS3);
	return m_ContextTexture.m_lPicMips[_iPicmip];
}

void CRCPS3GCM::Texture_Precache(int _TextureID)
{
	MSCOPE(CRCPS3GCM::Texture_Precache, RENDER_PS3);

	CTC_TextureProperties Props;
	m_pTC->GetTextureProperties(_TextureID, Props);
	if (!(Props.m_Flags & CTC_TEXTUREFLAGS_PROCEDURAL))
	{
		m_ContextTexture.Build(_TextureID);
	}
	else
	{
		M_TRACEALWAYS("Not precaching texture %d (%s)\n", _TextureID, m_pTC->GetName(_TextureID).GetStr());
	}
}

template <class tData>
void SwapLE_Array(void* _pData, uint _Count)
{
	tData* pData = (tData*)_pData;
	for(uint i = 0; i < _Count; i++)
		::SwapLE(pData[i]);
}

void CTextureGCM::Transform(uint _Fmt, void* _pData, uint _Width, uint _Height)
{
	switch(_Fmt)
	{
	case CTextureGCM::TEX_FORMAT_BGR5:
		{
			SwapLE_Array<uint16>(_pData, _Width * _Height);
			break;
		}

	default:
		M_ASSERT(0, "!");
	}
}

};	// namespace NRenderPS3GCM
