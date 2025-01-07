
#ifndef __MRENDERPS3_TEXTURE_H_INCLUDED
#define __MRENDERPS3_TEXTURE_H_INCLUDED


namespace NRenderPS3GCM
{

	class CTextureGCM
	{
	public:
		enum
		{
			BITCOUNT_TYPE	= 3,
			BITCOUNT_FLAGS	= 10,
			BITCOUNT_WIDTH	= 12,
			BITCOUNT_HEIGHT	= 12,
			BITCOUNT_DEPTH	= 9,
			BITCOUNT_FORMAT	= 5,
			BITCOUNT_MIPS	= 4,
			BITCOUNT_SKIPPEDMIPS	= 4,
			BITCOUNT_CUBEFACE	= 3,
			BITCOUNT_PADDING = (64 - BITCOUNT_TYPE - BITCOUNT_FLAGS - BITCOUNT_WIDTH - BITCOUNT_HEIGHT - BITCOUNT_DEPTH - BITCOUNT_FORMAT - BITCOUNT_MIPS - BITCOUNT_SKIPPEDMIPS - BITCOUNT_CUBEFACE),


			TEX_TYPE_VOID	= 0,
			TEX_TYPE_1D		= 1,
			TEX_TYPE_2D		= 2,
			TEX_TYPE_3D		= 3,
			TEX_TYPE_CUBE	= 4,

			TEX_FL_BUILT		= M_Bit(0),	// Texture has been built
			TEX_FL_POW2			= M_Bit(1),	// Texture is POW2 in all directions (directions determined by type)
			TEX_FL_CUBEOWNER	= M_Bit(2),	// This is the "master" cube texture
			TEX_FL_CUBECHAIN	= M_Bit(3),	// This flag is set if texture is part of a cube chain
			TEX_FL_MIP0LOADED	= M_Bit(4),	// Used to flag texture that top-level mipmap has been loaded so it is ok to use it, otherwise flag textures maxlod as 1 (not 0)
			TEX_FL_COMPRESSED	= M_Bit(5),	// Texture is compressed
			TEX_FL_REQUIRETRANSFORM = M_Bit(6),	// Texture requires transform before uploading (usually a SwapLE call)
			TEX_FL_LINEAR		= M_Bit(7),	// Texture must be allocated linear (usually a rendertarget), should do these tiled somehow
			TEX_FL_TILE			= M_Bit(8),	// Texture must be allocated linear (usually a rendertarget), should do these tiled somehow


			TEX_FORMAT_VOID	= 0,
			TEX_FORMAT_DXT1,
			TEX_FORMAT_DXT23,
			TEX_FORMAT_DXT45,

			TEX_FORMAT_BGR5,
			TEX_FORMAT_BGR8,
			TEX_FORMAT_BGRA8,
			TEX_FORMAT_BGRX8,
			TEX_FORMAT_RGBA8,
			TEX_FORMAT_A8,
			TEX_FORMAT_I8A8,
			TEX_FORMAT_I8,
			TEX_FORMAT_I16,
			TEX_FORMAT_RGBA16_F,
			TEX_FORMAT_RGB32_F,
			TEX_FORMAT_RGBA32_F,
			TEX_FORMAT_BGRA4,
			TEX_FORMAT_Z24S8,

			TEX_FORMAT_MAX,
		};
		uint64 m_Type:BITCOUNT_TYPE;	// Void, 1D, 2D, 3D, Cube
		uint64 m_Flags:BITCOUNT_FLAGS;
		
		uint64 m_Width:BITCOUNT_WIDTH;		// HW Max-Width is 4096
		uint64 m_Height:BITCOUNT_HEIGHT;	// HW Max-Height is 4096
		uint64 m_Depth:BITCOUNT_DEPTH;		// HW Max-Depth is 512 (max 3D texture is 512x512x512)
		uint64 m_Format:BITCOUNT_FORMAT;	// 32-formats supported for now
		uint64 m_MipCount:BITCOUNT_MIPS;	// Number of mips for this texture (1 means no mipmaps)
		uint64 m_SkippedMips:BITCOUNT_SKIPPEDMIPS;	// Number of mips skipped due to non-pow2 (broken texture if this is non-0)
		uint64 m_CubeFace:BITCOUNT_CUBEFACE;	// Which cubeface is this texture
		uint64 m_PADDING:BITCOUNT_PADDING;

		static uint32 RequireTransform(uint32 _ImageFormat)
		{
			return (_ImageFormat & IMAGE_FORMAT_BGR5)?TEX_FL_REQUIRETRANSFORM:0;
		}
		static void Transform(uint _Fmt, void* _pData, uint _Width, uint _Height);

		static uint32 TranslateFormat(uint32 _ImageFormat)
		{
			switch(_ImageFormat)
			{
				case IMAGE_FORMAT_BGR5: return TEX_FORMAT_BGR5;
				case IMAGE_FORMAT_BGR8: return TEX_FORMAT_BGR8;
				case IMAGE_FORMAT_BGRA8: return TEX_FORMAT_BGRA8;
				case IMAGE_FORMAT_BGRX8: return TEX_FORMAT_BGRX8;
				case IMAGE_FORMAT_RGBA8: return TEX_FORMAT_RGBA8;
				case IMAGE_FORMAT_A8: return TEX_FORMAT_A8;
				case IMAGE_FORMAT_I8A8: return TEX_FORMAT_I8A8;
				case IMAGE_FORMAT_I8: return TEX_FORMAT_I8;
				case IMAGE_FORMAT_I16: return TEX_FORMAT_I16;
				case IMAGE_FORMAT_RGBA16_F: return TEX_FORMAT_RGBA16_F;
				case IMAGE_FORMAT_RGB32_F: return TEX_FORMAT_RGB32_F;
				case IMAGE_FORMAT_RGBA32_F: return TEX_FORMAT_RGBA32_F;
				case IMAGE_FORMAT_BGRA4: return TEX_FORMAT_BGRA4;
				case IMAGE_FORMAT_Z24S8: return TEX_FORMAT_Z24S8;
			}

			return TEX_FORMAT_VOID;
		}

		void BuildSetup()
		{
			m_MipCount = 0;
			m_SkippedMips = 0;
		}

		void Clear()
		{
			m_Type = TEX_TYPE_VOID;
			m_Flags = 0;
			m_Width	= 0;
			m_Height = 0;
			m_Depth = 0;
			m_Format = 0;
			m_MipCount = 0;
			m_SkippedMips = 0;
			m_CubeFace = 0;
			m_PADDING = 0;
		}

		void Destroy()
		{
			m_Type = TEX_TYPE_VOID;
			m_Flags = 0;
			m_CubeFace = 0;
		}

		bool IsChainChild() const
		{
			return (m_Flags & (TEX_FL_CUBEOWNER | TEX_FL_CUBECHAIN)) == TEX_FL_CUBECHAIN;
		}
	};

	class CTexureVoidGCM : public CTextureGCM
	{
	public:
		// Assert in every damn thing
	};

	class CTexture1DGCM : public CTextureGCM
	{
	public:
		void Build1D(uint32 _Width, uint32 _Format)
		{
			CTextureGCM::BuildSetup();

			m_Type	= CTextureGCM::TEX_TYPE_1D;
			m_Width	= _Width;
			m_Format	= _Format;

			m_Flags	= TEX_FL_BUILT + ((IsPow2(_Width))?TEX_FL_POW2:0);
		}

		bool NeedBuild(uint32 _Width, uint32 _Format) const
		{
			return (!(m_Flags & TEX_FL_BUILT)) || ((m_Width != _Width) || (m_Format != _Format) || (m_Type != CTextureGCM::TEX_TYPE_1D));
		}
	};

	class CTexture2DGCM : public CTextureGCM
	{
	public:
		void Build2D(uint32 _Width, uint32 _Height, uint32 _Format)
		{
			CTextureGCM::BuildSetup();

			m_Type	= CTextureGCM::TEX_TYPE_2D;
			m_Width	= _Width;
			m_Height	= _Height;
			m_Format	= _Format;

			m_Flags	= TEX_FL_BUILT + ((IsPow2(_Width) && IsPow2(_Height))?TEX_FL_POW2:0);
		}

		bool NeedBuild(uint32 _Width, uint32 _Height, uint32 _Format) const
		{
			return (!(m_Flags & TEX_FL_BUILT)) || ((m_Width != _Width) || (m_Height != _Height) || (m_Format != _Format) || (m_Type != CTextureGCM::TEX_TYPE_2D));
		}
	};

	class CTexture3DGCM : public CTextureGCM
	{
	public:
		void Build3D(uint32 _Width, uint32 _Height, uint32 _Depth, uint32 _Format)
		{
			CTextureGCM::BuildSetup();

			m_Type	= CTextureGCM::TEX_TYPE_3D;
			m_Width	= _Width;
			m_Height	= _Height;
			m_Depth	= _Depth;
			m_Format	= _Format;

			m_Flags	= TEX_FL_BUILT + ((IsPow2(_Width) && IsPow2(_Height) && IsPow2(_Depth))?TEX_FL_POW2:0);
		}

		bool NeedBuild(uint32 _Width, uint32 _Height, uint32 _Depth, uint32 _Format) const
		{
			return (!(m_Flags & TEX_FL_BUILT)) || ((m_Width != _Width) || (m_Height != _Height) || (m_Depth != _Depth) || (m_Format != _Format) || (m_Type != CTextureGCM::TEX_TYPE_3D));
		}
	};

	class CTextureCubeGCM : public CTextureGCM
	{
	public:
		void BuildCube(uint32 _Width, uint32 _Height, uint32 _Format, int _iSide, bool _bChain)
		{
			CTextureGCM::BuildSetup();

			m_Type	= CTextureGCM::TEX_TYPE_CUBE;
			m_Width	= _Width;
			m_Height	= _Height;
			m_Format	= _Format;
			m_CubeFace	= _iSide;

			m_Flags	= TEX_FL_BUILT + ((IsPow2(_Width) && IsPow2(_Height))?TEX_FL_POW2:0) + ((m_CubeFace == 0)?TEX_FL_CUBEOWNER:0) + (_bChain?TEX_FL_CUBECHAIN:0);
		}

		bool NeedBuild(uint32 _Width, uint32 _Height, uint32 _Format) const
		{
			return (!(m_Flags & TEX_FL_BUILT)) || ((m_Width != _Width) || (m_Height != _Height) || (m_Format != _Format) || (m_Type != CTextureGCM::TEX_TYPE_CUBE));
		}
	};



#define REMAP_INPUT(_a_, _r_, _g_, _b_) ((CELL_GCM_TEXTURE_REMAP_FROM_##_a_) | (CELL_GCM_TEXTURE_REMAP_FROM_##_r_ << 2) | (CELL_GCM_TEXTURE_REMAP_FROM_##_g_ << 4) | (CELL_GCM_TEXTURE_REMAP_FROM_##_b_ << 6))
#define REMAP_OUTPUT(_a_, _r_, _g_, _b_) ((CELL_GCM_TEXTURE_REMAP_##_a_ << 8) | (CELL_GCM_TEXTURE_REMAP_##_r_ << 10) | (CELL_GCM_TEXTURE_REMAP_##_g_ << 12) | (CELL_GCM_TEXTURE_REMAP_##_b_ << 14))

	enum
	{
		GCM_TEXALLOC_MAINMEM = 1,
		GCM_TEXALLOC_FORCELINEAR = 2,
		GCM_TEXALLOC_TILE = 4,
	};


	class CRenderPS3Resource_Texture
	{
	public:
		class CRenderPS3Resource_Texture_Local : public CRenderPS3Resource_Local<GPU_SWIZZLEDTEXTURE_ALIGNMENT>
		{
			class CRenderPS3Resource_Texture_Local_BlockInfo : public CRenderPS3Resource_Local<GPU_SWIZZLEDTEXTURE_ALIGNMENT>::CRenderPS3Resource_Local_BlockInfo
			{
			public:
				uint32	m_Size;
				M_FORCEINLINE uint32 GetRealSize()
				{
					return m_Size;
				}
			};
		public:
			CRenderPS3Resource_Texture_Local_BlockInfo m_BlockInfo;
			uint m_iTile;

			CRenderPS3Resource_Texture_Local() : m_iTile(0)
			{
			}

			void Create(uint _Size);
			void Create(uint _Size, uint _TilePitch, uint _CompMode);
			void Destroy();
		};

		class CRenderPS3Resource_Texture_Main : public CRenderPS3Resource_Main
		{
		public:
			uint32 m_Size;

			M_INLINE void Create(uint _Size)
			{
				// Swizzled texture
				m_Size = _Size;
				Alloc(_Size, GPU_SWIZZLEDTEXTURE_ALIGNMENT);
			}
		};

		CRenderPS3Resource_Texture(uint _TexID);
		~CRenderPS3Resource_Texture();

		CRenderPS3Resource_Texture_Local m_ResourceLocal;
		CRenderPS3Resource_Texture_Main m_ResourceMain;

		uint32 m_lMipOffset[12];
		uint32 m_lMipSize[12];
		uint32 m_lFaceOffset[6];	// For cubemaps

		CellGcmTexture m_RSXTexture;

		union
		{
			// This structure has the same layout as the RSX texture addressing mode command, so don't change it.
			struct
			{
				uint32 __AddrModePad3 : 1;	// msb
				uint32 m_DepthFunc:3;
				uint32 __AddrModePad2 : 4;
				uint32 m_sRGBGamma:4;
				uint32 m_WrapR:4;
				uint32 __AddrModePad1 : 3;
				uint32 m_URemap:1;			// Unsigned normal remap, 0 = Normal [0 - 1.0], 1 = Biased [-1.0 - 1.0]
				uint32 m_WrapT:4;
				uint32 __AddrModePad0 : 4;
				uint32 m_WrapS:4;			// lsb
			};
			uint32 m_RSXAddrMode;
		};

		union
		{
			// This structure has the same layout as the RSX filter mode command, so don't change it.
			struct
			{
				uint32 __FilterModePad0 : 5;
				uint32 m_MagFilter:3;
				uint32 __FilterModePad1 : 5;
				uint32 m_MinFilter:3;
				uint32 __FilterModePad2 : 1;
				uint32 m_ConvFilter : 2;
				uint32 m_Bias : 13;
			};
			uint32 m_RSXFilterMode;
		};

		uint64 m_MaxAnisoFiltering:3;
//		uint64 m_LODBias:16;
		uint64 m_MinLOD:16;
		uint64 m_MaxLOD:16;
		uint64 m_TextureID:16;
		uint64 m_iFirstMip:4;
		uint64 m_TexVersion:3;
		DLinkD_Link(CRenderPS3Resource_Texture, m_Link);

		uint32 CalcSize2DSwizzled(uint _Width, uint _Height, uint _nMip, uint _Format);
		uint32 CalcSize2DLinear(uint _Width, uint _Height, uint _nMip, uint _Format);
		void SetFormat(uint _Format, uint _bLinear);
		void AllocCompressed2D(uint _Width, uint _Height, uint _nMip, uint _Format, uint _AllocFlags = 0);
		void Alloc2D(uint _Width, uint _Height, uint _nMip, uint _Format, uint _AllocFlags = 0);
		void AllocCompressedCube(uint _Size, uint _nMip, uint _Format, uint _AllocFlags = 0);
		void AllocCube(uint _Size, uint _nMip, uint _Format, uint _AllocFlags = 0);

		void Touch()
		{
			m_ResourceLocal.Touch();
			m_ResourceMain.Touch();
		}
		void Destroy()
		{
			m_ResourceLocal.Destroy();
			m_ResourceMain.Destroy();
			m_RSXTexture.width = 0;
			m_RSXTexture.height = 0;
			m_RSXTexture.depth = 0;
		}

		void PostDefrag()
		{
			m_ResourceLocal.PostDefrag();
		}

		M_INLINE bool IsLocal() const
		{
			// Maybe we should actually check the resources...
			return (m_RSXTexture.location == CELL_GCM_LOCATION_LOCAL);
		}

		M_INLINE bool NeedRebuild(uint _Width, uint _Height, uint _Format) const
		{
			// Ignore Format for now...
			return ((_Width != m_RSXTexture.width) || (_Height != m_RSXTexture.height));
		}

		M_INLINE void Bind(uint _iSampler)
		{
			gcmSetTexture(_iSampler, &m_RSXTexture);
			gcmSetTextureAddress(_iSampler, m_WrapS, m_WrapT, m_WrapR, m_URemap, m_DepthFunc, m_sRGBGamma);
			gcmSetTextureControl(_iSampler, CELL_GCM_TRUE, m_MinLOD << 8, m_MaxLOD << 8, m_MaxAnisoFiltering);
			gcmSetTextureFilter(_iSampler, 0, m_MinFilter, m_MagFilter, m_ConvFilter);

/*			M_TRACEALWAYS("Texture: %.2x%.2x%.2x%.2x, %.8x, %.4xx%.4x, %.4x, %.2x, %.2x, %.8x, %.8x\n",
				m_RSXTexture.format, m_RSXTexture.mipmap, m_RSXTexture.dimension, m_RSXTexture.cubemap, m_RSXTexture.remap, m_RSXTexture.width,
				m_RSXTexture.height, m_RSXTexture.depth, m_RSXTexture.location, m_RSXTexture._padding, m_RSXTexture.pitch, m_RSXTexture.offset);
			M_TRACEALWAYS("AddrMode: %.2x, %.2x, %.2x, %.2x, %.1x, %.2x, %.2x\n", _iSampler, m_WrapS, m_WrapT, m_WrapR, m_URemap, m_DepthFunc, m_sRGBGamma);
			M_TRACEALWAYS("Filter: %.2x, %.2x, %.2x, %.2x\n", _iSampler, m_MinFilter, m_MagFilter, m_ConvFilter);
*/			//		gcmDump();
		}

		M_FORCEINLINE void RSX_Bind(uint32 M_RESTRICT*& _pCmd, uint _iSampler)
		{
			RSX_Texture(_pCmd, _iSampler, m_RSXTexture);
			RSX_TextureAddress(_pCmd, _iSampler, m_RSXAddrMode);
//			RSX_TextureAddress(_pCmd, _iSampler, m_WrapS, m_WrapT, m_WrapR, m_URemap, m_DepthFunc, m_sRGBGamma);
			RSX_TextureControl(_pCmd, _iSampler, 0 << 8, m_RSXTexture.mipmap << 8, m_MaxAnisoFiltering);
			RSX_TextureFilter(_pCmd, _iSampler, m_RSXFilterMode);
//			RSX_TextureFilter(_pCmd, _iSampler, 0, m_MinFilter, m_MagFilter, m_ConvFilter);
		}

		M_INLINE uint32 GetOffset() const
		{
			return m_ResourceLocal.GetOffset();
		}

		M_INLINE uint32 GetPitch() const
		{
			return m_RSXTexture.pitch;
		}

		M_INLINE uint32 GetMipOffset(uint32 _iFace, uint32 _iMip) const
		{
			M_ASSERT(_iMip < m_RSXTexture.mipmap, "Mipmap out of range");
			M_ASSERT((_iFace == 0) || (_iFace != 0 && m_RSXTexture.cubemap != 0), "Non 0 cubemap face is only valid for cubemaps");
			return m_ResourceLocal.GetOffset() +  + m_lFaceOffset[_iFace] + m_lMipOffset[_iMip];
		}

		M_INLINE void* GetMipMemory(uint _iFace, uint _iMip) const
		{
			M_ASSERT(_iMip < m_RSXTexture.mipmap, "Mipmap out of range");
			M_ASSERT((_iFace == 0) || (_iFace != 0 && m_RSXTexture.cubemap != 0), "Non 0 cubemap face is only valid for cubemaps");
			return (void*)((uint8*)m_ResourceLocal.GetMemory() + m_lFaceOffset[_iFace] + m_lMipOffset[_iMip]);
		}

		M_INLINE uint GetMipSize(uint _iMip) const
		{
			M_ASSERT(_iMip < m_RSXTexture.mipmap, "Mipmap out of range");
			return m_lMipSize[_iMip];
		}

		M_INLINE uint GetSize()
		{
			if(IsLocal())
				return m_ResourceLocal.m_BlockInfo.GetRealSize();
			else
				return m_ResourceMain.m_Size;
		}
	};

	class CTextureUploader : public CRenderPS3Resource_Main
	{
	protected:
		uint32 m_WritePos;
		uint32 m_Size;

		uint32 AllocSpace(uint _Size);

		void* ConvertToMemory(uint32 _WritePos) const
		{
			return (uint8*)m_pMemory + _WritePos;
		}

		uint32 ConvertToOffset(uint32 _WritePos) const
		{
			return m_MainOffset + _WritePos;
		}

		template <class tStore, int tSize>
		class TLockFreeFifo
		{
		public:
			// Use uint64 to make sure it never wraps
			volatile uint64 m_iHead;
			volatile uint64 m_iTail;
			TThinArray<tStore> m_lQueue;

			TLockFreeFifo() : m_iHead(0), m_iTail(0) {m_lQueue.SetLen(tSize);}

			bool Put(tStore& _Item)
			{
				if(m_iHead - m_iTail < tSize)
				{
					m_lQueue[m_iHead & (tSize - 1)] = _Item;
					M_EXPORTBARRIER;
					m_iHead++;
					M_EXPORTBARRIER;
					return true;
				}

				return false;
			}

			bool Get(tStore& _Item)
			{
				if(m_iHead > m_iTail)
				{
					_Item = m_lQueue[m_iTail & (tSize - 1)];
					M_EXPORTBARRIER;
					m_iTail++;
					M_EXPORTBARRIER;
					return true;
				}
				return false;
			}
		};

		class CQueueItem
		{
		public:
			uint m_nDst;
			uint m_Type;
			uint m_DstOffset[6];
			uint m_DstPitch;
			uint m_SrcOffset;
			uint m_SrcPitch;
			uint m_RowCopy;
			uint m_Cols;
			uint m_WriteLabel;
		};

		TLockFreeFifo<CQueueItem, 256> m_lQueue;

		void QueueTransfer(uint _nDst, uint _Type, uint _DstOffset[6], uint _DstPitch, uint _SrcOffset, uint _SrcPitch, uint _RowCopy, uint _Cols, uint _WriteLabel)
		{
			CQueueItem Item;
			Item.m_nDst = _nDst;
			Item.m_Type = _Type;
			Item.m_DstOffset[0] = _DstOffset[0];
			Item.m_DstOffset[1] = _DstOffset[1];
			Item.m_DstOffset[2] = _DstOffset[2];
			Item.m_DstOffset[3] = _DstOffset[3];
			Item.m_DstOffset[4] = _DstOffset[4];
			Item.m_DstOffset[5] = _DstOffset[5];
			Item.m_DstPitch = _DstPitch;
			Item.m_SrcOffset = _SrcOffset;
			Item.m_SrcPitch = _SrcPitch;
			Item.m_RowCopy = _RowCopy;
			Item.m_Cols = _Cols;
			Item.m_WriteLabel = _WriteLabel;
			while(!m_lQueue.Put(Item))
			{
				MRTC_SystemInfo::OS_Sleep(1);
			}
		}

	public:
		MRTC_CriticalSection m_Lock;

		void Create()
		{
			m_WritePos = 0;
			m_Size = 2048 * 2048 * 4;
			Alloc(m_Size, 128);

			gcmSetWriteBackEndLabel(GPU_LABEL_TEXLOADER, 0);
		}

	//###############################################################################
	//###############################################################################
	//###############################################################################
	//###############################################################################

		void UploadS3TC(const CRenderPS3Resource_Texture* _pTex, uint _iFace, uint _nFace, uint _iMip, uint _Fmt, uint _TexFlags, void* _pData);
		void UploadSwizzle(const CRenderPS3Resource_Texture* _pTex, uint _iFace, uint _nFace, uint _iMip, uint _Fmt, uint _TexFlags, void* _pData);
		void UploadLinear(const CRenderPS3Resource_Texture* _pTex, uint _iFace, uint _nFace, uint _iMip, uint _Fmt, uint _TexFlags, void* _pData);

		void Service(int _nItems)
		{
			// This is serviced by the main thread
			CQueueItem Item;
			while(m_lQueue.Get(Item)/* && _nItems-- > 0*/)
			{
				for(uint i = 0; i < Item.m_nDst; i++)
					gcmSetTransferData(Item.m_Type, Item.m_DstOffset[i], Item.m_DstPitch, Item.m_SrcOffset, Item.m_SrcPitch, Item.m_RowCopy, Item.m_Cols);
				gcmSetWriteBackEndLabel(GPU_LABEL_TEXLOADER, Item.m_WriteLabel);
			}
			gcmSetInvalidateTextureCache(CELL_GCM_INVALIDATE_TEXTURE);
		}
	};

	class CContext_Texture : public MRTC_Thread
	{
	public:
		MRTC_CriticalSection m_DynamicLoadListLock;
		NThread::CEventAutoResetReportable m_DynamicLoadEvent;

		const char* Thread_GetName() const;
		int Thread_Main();

		DLinkD_List(CRenderPS3Resource_Texture, m_Link) m_lTexturesList;
		DLinkD_List(CRenderPS3Resource_Texture, m_Link) m_lTexturesDynamicLoadList;
		DLinkD_List(CRenderPS3Resource_Texture, m_Link) m_lTexturesDestroyList;
		typedef DLinkD_Iter(CRenderPS3Resource_Texture, m_Link) CRenderPS3Resource_TextureIter;

		MRTC_CriticalSection m_Lock;
		CTextureContext* m_pTC;
		TPtr<CRC_TCIDInfo> m_spTCIDInfo;	// TextureID info

		CTextureUploader m_TextureUploader;

		TThinArray<CTextureGCM> m_lTextures;
		TThinArray<CRenderPS3Resource_Texture*> m_lpTexResources;
		CTextureGCM* m_pTextures;	// Used for debugging
		CRenderPS3Resource_Texture** m_ppTexResources;

		uint8 m_lPicMips[CRC_MAXPICMIPS];

		fp32 m_MaxAnisotropy;
		fp32 m_Anisotropy;

		uint32 m_bAllowTextureLoad:1;
		uint32 m_bPrecaching:1;
		uint32 m_bMip0Streaming:1;

		CContext_Texture();

		void Bind(uint _Sampler, uint32 _TextureID);
		uint32 M_RESTRICT* RSX_Bind(uint32 M_RESTRICT* _pCmd, uint _iSampler, uint32 _TextureID);

		void Build(uint32 _TextureID);
		void Destroy(uint32 _TextureID);
		void SetTextureParameters(const CTextureGCM* _pTexGCM, CRenderPS3Resource_Texture* _pResource, const CTC_TextureProperties& _Properties);

		void Create(CTextureContext* _pTC, uint32 _Count, TPtr<CRC_TCIDInfo> _spTCIDInfo);
		void Precache_Flush();
		void Precache_Begin(uint32 _Count);
		void Precache_End();
		void Precache(uint32 _ID);
		CStr GetDesc(uint32 _TextureID);

		uint32 GetWidth(uint32 _TexID);
		uint32 GetHeight(uint32 _TexID);
		uint32 GetFormat(uint32 _TexID);

		uint GetLocation(uint32 _TexID);
		uint GetOffset(uint32 _TexID);
		uint GetPitch(uint32 _TexID);

		bool IsBuilt(uint32 _TextureID) const
		{
			if(_TextureID >= m_lTextures.Len()) return false;
			return (m_lTextures[_TextureID].m_Flags & CTextureGCM::TEX_FL_BUILT) != 0;
		}

		void DeleteTextures();

		void PostDefrag();

		void RenderTarget_CopyToTexture(uint _TextureID, CRct _SrcRect, CPnt _Dest, uint _bContinueTiling, uint _Slice, uint _iMRT);

		void Service(int _nItems)
		{
			DestroyTextures();
			m_TextureUploader.Service(_nItems);
		}

		CRC_TextureMemoryUsage GetMem(int _TextureID);

	protected:
		void DestroyTextures();

		void SetCompressed(uint32 _TextureID);
		void Build(uint32 _TextureID, uint32 _iFirstMip, uint32 _MipCount);
		void Build(uint32 _TextureID, const CTC_TextureProperties& _Properties);
		void SetCompressedFormat(uint32 _TextureID, uint32 _Format);
		bool NeedBuild(uint32 _TextureID);

		bool LoadTextureMip0(const CRenderPS3Resource_Texture* _pTexture, CTextureContainer* _pTC, uint _iLocal);
		bool LoadTextureCubeMip0(const CRenderPS3Resource_Texture* _pTexture, CTextureContainer* _pTC, uint _iLocal, uint _iFace);
		bool LoadTexture(CRenderPS3Resource_Texture* _pTexture, uint32 _TextureID, CTextureContainer* _pTC, uint _iLocal, uint _TexVersion, const CTC_TextureProperties& _Properties, uint _iFirstMip, uint _Width, uint _Height, uint _nMip);
		bool LoadTextureCube(CRenderPS3Resource_Texture* _pTexture, uint32 _TextureID, CTextureContainer* _pTC, uint _iLocal, uint _TexVersion, const CTC_TextureProperties& _Properties, uint _iFirstMip, uint _Width, uint _Height, uint _nMip);
		bool LoadTextureBuildInto(CRenderPS3Resource_Texture* _pTexture, uint32 _TextureID, CTextureContainer* _pTC, uint _iLocal, const CImage& _TxtDesc, const CTC_TextureProperties& _Properties, uint _iFirstMip, uint _Width, uint _Height, uint _nMip);
		bool LoadTextureRender(CRenderPS3Resource_Texture* _pTexture, uint32 _TextureID, CTextureContainer* _pTC, uint _iLocal, const CImage& _TxtDesc, const CTC_TextureProperties& _Properties, uint _iFirstMip, uint _Width, uint _Height, uint _nMip);

		CRenderPS3Resource_Texture* GetTexture(uint _TextureID);
	};
};

#endif	// __MRENDERPS3_TEXTURE_H_INCLUDED
