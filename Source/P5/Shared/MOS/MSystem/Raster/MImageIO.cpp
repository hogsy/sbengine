
#include "PCH.h"

#include "MImageIO.h"
#ifndef IMAGE_IO_NOJPG
#include "JPGLib/JPegLib.h"
#endif

#ifdef IMAGE_IO_PNG
#include "../../SDK/Png/Png.h"
#endif

// -------------------------------------------------------------------
//  CImageIO_GIF
// -------------------------------------------------------------------
//
// CLASS AUTHOR:	Daniel Hansson
// STARTED	   :	970213
// LAST UPDATED:	970215
// SUPPORTS	   :	Loading of 8-bit LZW compressed images.
//					Saving of 8-bit LZW compressed images.
//
// COMMENTS	   :	Loads both GIF87a and GIF89a but saves
//					only GIF87a.

#ifndef IMAGE_IO_NOGIF

class CImageIO_GIF : public CImageIO
{
	struct GIF_ScreenDescriptor {
		uint16 Width;
		uint16 Height;
		uint8 Misc;
		uint8 BackgroundIndex;
		uint8 Zero;

		void Read(CCFile* f)
		{
			f->ReadLE(Width);
			f->ReadLE(Height);
			f->Read(Misc);
			f->Read(BackgroundIndex);
			f->Read(Zero);
		}
		void Write(CCFile* f)
		{
			f->WriteLE(Width);
			f->WriteLE(Height);
			f->Write(Misc);
			f->Write(BackgroundIndex);
			f->Write(Zero);
		}
	};

	struct GIF_ImageDescriptor {
		uint16 Left;
		uint16 Top;
		uint16 Width;
		uint16 Height;
		uint8 Misc;

		void Read(CCFile* f)
		{
			f->ReadLE(Left);
			f->ReadLE(Top);
			f->ReadLE(Width);
			f->ReadLE(Height);
			f->Read(Misc);
		}
		void Write(CCFile* f)
		{
			f->WriteLE(Left);
			f->WriteLE(Top);
			f->WriteLE(Width);
			f->WriteLE(Height);
			f->Write(Misc);
		}
	};

	// Block codes.
	#define	GIF_IMAGE_SEPARATOR		0x2c
	#define	GIF_EXTENSION_BLOCK		0x21
	#define	GIF_TERMINATOR			0x3b

	// Bit masks
	#define GIF_GLOBAL_COLORMAP_EXISTS	0x80
	#define GIF_LOCAL_COLORMAP_EXISTS	0x80
	#define GIF_COLOR_RES_BITS_MASK		0x70
	#define GIF_BITS_PER_PIXEL_MASK		0x07
	#define GIF_IMAGE_INTERLACED		0x40

public:
	spCImage ReadInfo(const CStr& filename)
	{
		spCImage spImg = MNew(CImage);
		if (spImg == NULL) MemError("ReadInfo");

		CCFile f;
		f.Open(filename, CFILE_READ | CFILE_BINARY);

		// Is it really a GIF file?
		char GIFSignature[7]="      ";
		f.Read(GIFSignature,6);
		if (strcmp(GIFSignature,"GIF87a")!=0 && strcmp(GIFSignature,"GIF89a")!=0)
			Error("Read","Not a GIF file.");

		// Read screen descriptor.
		GIF_ScreenDescriptor sd;
		sd.Read(&f);

		if ((sd.Misc & GIF_GLOBAL_COLORMAP_EXISTS)==0)
			Error("Read","Global colormap is missing.");

		if ((sd.Misc & GIF_BITS_PER_PIXEL_MASK)!=GIF_BITS_PER_PIXEL_MASK)
				Error("Read","Supports only 8-bit GIF images.");

		// Skip global colormap.
		f.RelSeek(768);

		// Find first image descriptor and skip
		// all extension blocks preceeding it.
		uint8 BlockCode;
		uint8 BlockSize;
		f.Read(BlockCode);
		while (BlockCode==GIF_EXTENSION_BLOCK) {
			f.RelSeek(1);

			f.Read(BlockSize);
			while (BlockSize>0) {
				f.RelSeek(BlockSize);
				f.Read(BlockSize);
			}

			f.Read(BlockCode);
		}

		if (BlockCode==GIF_TERMINATOR)
			Error("Read","File contained no image.");

		if (BlockCode!=GIF_IMAGE_SEPARATOR)
			Error("Read","Unknown block code in file.");

		// Read image descriptor.
		GIF_ImageDescriptor id;
		id.Read(&f);

		spImg->CreateVirtual(id.Width, id.Height, IMAGE_FORMAT_CLUT8, IMAGE_MEM_SYSTEM);

		f.Close();
		return spImg;
	}

	// ---------------------------------
	void Read(const CStr& filename, CImage* img, int _MemModel)
	{
		CCFile f;
		f.Open(filename, CFILE_READ | CFILE_BINARY);

		// Is it really a GIF file?
		char GIFSignature[7]="      ";
		f.Read(GIFSignature,6);
		if (strcmp(GIFSignature,"GIF87a")!=0 && strcmp(GIFSignature,"GIF89a")!=0)
			Error("Read","Not a GIF file.");

		// Read screen descriptor.
		GIF_ScreenDescriptor sd;
		sd.Read(&f);

		if ((sd.Misc & GIF_GLOBAL_COLORMAP_EXISTS)==0)
			Error("Read","Global colormap is missing.");

		if ((sd.Misc & GIF_BITS_PER_PIXEL_MASK)!=GIF_BITS_PER_PIXEL_MASK)
				Error("Read","Supports only 8-bit GIF images.");

		// Read global colormap.
		uint8 RawPal[768];
		f.Read(RawPal,768);

		// Find first image descriptor and skip
		// all extension blocks preceeding it.
		uint8 BlockCode;
		uint8 BlockSize;
		f.Read(BlockCode);
		while (BlockCode==GIF_EXTENSION_BLOCK) {
			f.RelSeek(1);

			f.Read(BlockSize);
			while (BlockSize>0) {
				f.RelSeek(BlockSize);
				f.Read(BlockSize);
			}

			f.Read(BlockCode);
		}

		if (BlockCode==GIF_TERMINATOR)
			Error("Read","File contained no image.");

		// Make sure there's an image separator.
		if (BlockCode!=GIF_IMAGE_SEPARATOR)
			Error("Read","Unknown block code in file.");

		// Read image descriptor.
		GIF_ImageDescriptor id;
		id.Read(&f);

		if ((id.Misc & GIF_LOCAL_COLORMAP_EXISTS)>0)
			Error("Read","Local colormaps not supported.");

		bint Interlaced=(id.Misc & GIF_IMAGE_INTERLACED)>0;

		// Read the initial LZW code size.
		uint8 InitialCodeSize;
		f.Read(InitialCodeSize);	// Actually InitialCodeSize - 1 (hmm...)
		InitialCodeSize++;

		// Setup the LZW decompressor.
		CLZW lzw;
		CLZW_GIFSettings GIFSet;
		GIFSet.SetMinimumBits(InitialCodeSize);
		lzw.SetCompress(GIFSet);

		// Calculate the size of the compressed data.
		int32 Pos=f.Pos();
		uint32 Size=0;

		f.Read(BlockSize);
		while (BlockSize>0) {
			Size+=(uint32)BlockSize;
			f.RelSeek(BlockSize);
			f.Read(BlockSize);
		}

		// Allocate memory for source and destination.
		TThinArray<uint8> lSrc;
		TThinArray<uint8> lDst;
		lSrc.SetLen( Size+4+4 );
		lDst.SetLen( id.Width*id.Height+4 );
		uint8* Src = lSrc.GetBasePtr();
		uint8* Dst = lDst.GetBasePtr();
//		uint8* Src=DNew(uint8) uint8[Size+4+4];
//		uint8* Dst=DNew(uint8) uint8[id.Width*id.Height+4];
		if (Src==NULL || Dst==NULL) {
			MemError("Read");
		}

		try {
			// Read the compressed data.
			uint8* src=Src;
			*((uint32*)src)=id.Width*id.Height;		// Demanded by the decompressor.
			src+=4;
			f.Seek(Pos);
			f.Read(BlockSize);
			while (BlockSize>0) {
				f.Read(src,BlockSize);
				src+=(uint32)BlockSize;
				f.Read(BlockSize);
			}

			// Decompress the image data.
			lzw.Decompress(Src,Dst);
	
			Src=NULL;

			// Create the image object.
			img->Create(id.Width, id.Height, IMAGE_FORMAT_CLUT8, _MemModel);

			// Move the image data into the image object.
			if (!Interlaced) {
				// Image data is sequential.
				uint8* Data=Dst;
				for (uint16 y=0; y<id.Height; y++) {
					img->SetRAWData(CPnt(0,y), id.Width, Data);
					Data+=id.Width;
				}
			}
			else {
				// Image data is interlaced.
				uint8* Data=Dst;
				uint16 y=0;
				uint16 Step=8;
				int Pass=1;
				while (Pass<=4) {
					img->SetRAWData(CPnt(0,y), id.Width, Data);
					Data+=id.Width;
					y+=Step;
					if (y>=id.Height) {
						switch (Pass) {
						case 1: y=4; Step=8; break;
						case 2: y=2; Step=4; break;
						case 3: y=1; Step=2; break;
						}
						Pass++;
					}
				}
			}

			// Set the palette of the image.
			CPixel32 Pal[256];
			for (int i = 0; i < 256; i++)
				{ Pal[i] = CPixel32(RawPal[i*3+0], RawPal[i*3+1], RawPal[i*3+2]); };

			img->GetPalette()->SetPalette(Pal, 0, 256);
		}
		catch (...) {
			throw;
		}

		// Make sure there are no more images.
		f.Read(BlockCode);
		while (BlockCode==GIF_EXTENSION_BLOCK) {
			f.RelSeek(1);

			f.Read(BlockSize);
			while (BlockSize>0) {
				f.RelSeek(BlockSize);
				f.Read(BlockSize);
			}

			f.Read(BlockCode);
		}

		// Make sure there are no more images in the file.
		if (BlockCode==GIF_IMAGE_SEPARATOR)
			Error("Read","Multiple images not supported.");

		// Make sure the file terminates here.
		if (BlockCode!=GIF_TERMINATOR)
			Error("Read","GIF file is broken.");

		f.Close();
	}

	// ---------------------------------
	void Write(const CStr& filename, CImage* img)
	{
		// Get the image dimensions.
		uint8 PixelSize=img->GetPixelSize();

		uint16 Width=img->GetWidth();
		uint16 Height=img->GetHeight();

		int32 _w = Width;
		int32 _h = Height;

		// Color depth supported?
		if (PixelSize!=1)
			Error("Write", "Unsupported color depth.");

		CCFile f;
		f.Open(filename, CFILE_WRITE | CFILE_BINARY);

		// Write the GIF signature.
		char GIFSignature[7]="GIF87a";
		f.Write(GIFSignature,6);

		// Write screen descriptor.
		GIF_ScreenDescriptor sd;
		sd.Width=Width;
		sd.Height=Height;
		sd.Misc=GIF_GLOBAL_COLORMAP_EXISTS | GIF_COLOR_RES_BITS_MASK | GIF_BITS_PER_PIXEL_MASK;
		sd.BackgroundIndex=0;
		sd.Zero=0;
		sd.Write(&f);

		// Write the colormap.
		CPixel32 Pal[256];
		img->GetPalette()->GetPalette(Pal, 0, 256);
		int i;
		for (i=0; i<256; i++) {
			f.Write(Pal[i].GetR());
			f.Write(Pal[i].GetG());
			f.Write(Pal[i].GetB());
		}

		// Write image separator.
		uint8 is=GIF_IMAGE_SEPARATOR;
		f.Write(is);

		// Write image description.
		GIF_ImageDescriptor id;
		id.Left=0;
		id.Top=0;
		id.Width=Width;
		id.Height=Height;
		id.Misc=GIF_BITS_PER_PIXEL_MASK;
		id.Write(&f);

		// Write inital LZW code size - 1.
		uint8 ics=8;
		f.Write(ics);

		// Allocate memory for source.
		TThinArray<uint8>	lSrc;
		lSrc.SetLen( Width*Height+4 );
		uint8* Src = lSrc.GetBasePtr();
//		uint8* Src=DNew(uint8) uint8[Width*Height+4];
		if (Src==NULL)
			MemError("Write");

		uint8* Dst=NULL;
		try {
			// Get the image data.
			uint8* Data=Src;
			for (int y=0; y<Height; y++) {
				img->GetRAWData(CPnt(0,y),Width,Data);
				Data+=Width;
			}

			// Setup the LZW compressor.
			CLZW lzw;
			CLZW_GIFSettings GIFSet;
			lzw.SetCompress(GIFSet);

			// Compress the image data.
			Dst=(uint8*)lzw.Compress(Src,NULL,Width*Height);

			Src=NULL;

			// Save the compressed image data in blocks.
			int Size=lzw.GetCompressedLength()-4;
			Data=Dst+4;

			uint8 BlockSize;
			while (Size>0) {
				BlockSize=Min(Size, 255);
				f.Write(BlockSize);
				f.Write(Data,BlockSize);
				Data+=BlockSize;
				Size-=BlockSize;
			}
			BlockSize=0;
			f.Write(BlockSize);
		}
		catch (...) {
			throw;
		}

		// Write GIF terminator.
		uint8 gt=GIF_TERMINATOR;
		f.Write(gt);

		f.Close();
	}

	// ---------------------------------
	void ReadPalette(const CStr& filename, CImagePalette* destpal)
	{
		CCFile f;
		f.Open(filename, CFILE_READ | CFILE_BINARY);

		// Is it really a GIF file?
		char GIFSignature[7]="      ";
		f.Read(GIFSignature,6);
		if (strcmp(GIFSignature,"GIF87a")!=0 && strcmp(GIFSignature,"GIF89a")!=0)
			Error("Read","Not a GIF file.");

		// Read screen descriptor.
		GIF_ScreenDescriptor sd;
		sd.Read(&f);

		if ((sd.Misc & GIF_GLOBAL_COLORMAP_EXISTS)==0)
			Error("Read","Global colormap is missing.");

		if ((sd.Misc & GIF_BITS_PER_PIXEL_MASK)!=GIF_BITS_PER_PIXEL_MASK)
				Error("Read","Supports only 8-bit GIF images.");

		// Read global colormap.
		uint8 RawPal[768];
		f.Read(RawPal,768);

		CPixel32 Pal[256];
		for (int i = 0; i < 256; i++)
			{ Pal[i] = CPixel32(RawPal[i*3+0], RawPal[i*3+1], RawPal[i*3+2]); };

		destpal->SetPalette(Pal,0,256);

		f.Close();
	}

};

#endif

// -------------------------------------------------------------------
//  CImageIO_TGA
// -------------------------------------------------------------------
//
// CLASS AUTHOR:	Daniel Hansson
// STARTED	   :	961128
// LAST UPDATED:	970310
// SUPPORTS	   :	Loading of:	8-bit color/greyscale
//								15/16/24-bit RGB
//								32-bit RGBA
//					images (RLE encoded or uncompressed).
//
//					Saving of:	8-bit color/greyscale
//								15/16/24-bit RGB
//								32-bit RGBA
//					RLE encoded images.
//

#ifndef IMAGE_IO_NOTGA

class CImageIO_TGA : public CImageIO
{
	// ---------------------------------
	struct TGA_Head
	{
		uint8 IDLength;
		uint8 ColorMapType;
		uint8 ImageType;
		uint16 CMapStart;
		uint16 CMapLength;
		uint8 CMapDepth;
		uint16 XOffset;
		uint16 YOffset;
		uint16 Width;
		uint16 Height;
		uint8 PixelDepth;
		uint8 ImageDescriptor;

		void Read(CCFile* f)
		{
			f->Read(IDLength);
			f->Read(ColorMapType);
			f->Read(ImageType);
			f->ReadLE(CMapStart);
			f->ReadLE(CMapLength);
			f->Read(CMapDepth);
			f->ReadLE(XOffset);
			f->ReadLE(YOffset);
			f->ReadLE(Width);
			f->ReadLE(Height);
			f->Read(PixelDepth);
			f->Read(ImageDescriptor);
		};

		void Write(CCFile* f)
		{
			f->Write(IDLength);
			f->Write(ColorMapType);
			f->Write(ImageType);
			f->WriteLE(CMapStart);
			f->WriteLE(CMapLength);
			f->Write(CMapDepth);
			f->WriteLE(XOffset);
			f->WriteLE(YOffset);
			f->WriteLE(Width);
			f->WriteLE(Height);
			f->Write(PixelDepth);
			f->Write(ImageDescriptor);
		};
	};

	// ImageType ---------------------------------
	#define TGA_FORMAT_CLUT8		1
	#define TGA_FORMAT_RGB			2
	#define TGA_FORMAT_GREY			3
	#define TGA_FORMAT_CLUT8_RLE	9
	#define TGA_FORMAT_RGB_RLE		10
	#define TGA_FORMAT_GREY_RLE		11

	#define TGA_FORMAT_MASK			7
	#define TGA_FLAGS_RLE			8

	// Colormap types
	#define TGA_PALETTE				1
	#define TGA_NO_PALETTE			0

	// ImageDescriptor ---------------------------------
	#define TGA_DESCR_ATTRMASK	15
	#define TGA_DESCR_XORIGINRIGHT 16
	#define TGA_DESCR_YORIGINTOP 32

	// ---------------------------------
	int32 TGA_Format2CImageFormat(const TGA_Head& head)
	{
		switch (head.ImageType & TGA_FORMAT_MASK)
		{
			case TGA_FORMAT_GREY: return(IMAGE_FORMAT_I8);
			case TGA_FORMAT_CLUT8: return(IMAGE_FORMAT_CLUT8);
			case TGA_FORMAT_RGB:
				// RGBA32 ?
				if (head.PixelDepth==32) {
					if ((head.ImageDescriptor & TGA_DESCR_ATTRMASK)==8)
						return IMAGE_FORMAT_BGRA8;
					else
						return 0;
				}
				else
				{
					return (CImage::BPP2Format(head.PixelDepth));
					// - (head.ImageDescriptor & TGA_DESCR_ATTRMASK)
				}
			default : return 0;
		};
	};
	
	// ---------------------------------
	spCImage ReadInfo(const CStr& filename)
	{
		TGA_Head head;
		spCImage spImg = MNew(CImage);
		if (spImg == NULL) MemError("ReadInfo");

		CCFile f;
		f.Open(filename, CFILE_READ | CFILE_BINARY);
		head.Read(&f);
		int32 format = TGA_Format2CImageFormat(head);
		spImg->CreateVirtual(head.Width, head.Height, format, IMAGE_MEM_SYSTEM);

		f.Close();
		return spImg;
	};

	// ---------------------------------
	void Read(const CStr& filename, CImage* img, int _MemModel)
	{
		TGA_Head head;
		FillChar(&head, sizeof(head), 0);
		
		CCFile f;
		f.Open(filename, CFILE_READ | CFILE_BINARY);
		head.Read(&f);
		int format = TGA_Format2CImageFormat(head);
		
		// Format supported?
		int ImageType = head.ImageType & TGA_FORMAT_MASK;
		if (ImageType!=TGA_FORMAT_CLUT8 &&
			ImageType!=TGA_FORMAT_RGB &&
			ImageType!=TGA_FORMAT_GREY)
			Error("Read", "Unsupported Targa format.");
		
		// Color depth supported?
		if ((ImageType!=TGA_FORMAT_GREY &&
			(head.PixelDepth!=8 && head.PixelDepth!=16 && head.PixelDepth!=24 && head.PixelDepth!=32)) ||
			(ImageType==TGA_FORMAT_GREY && head.PixelDepth!=8))
			Error("Read", "Unsupported color depth.");
		
		if ((head.ImageDescriptor & TGA_DESCR_XORIGINRIGHT) != 0) 
			Error("Read","X-flipped file format not supported.");
		
		// Create the image object.
		int _w = head.Width;
		int _h = head.Height;
		img->Create(_w, _h, format, _MemModel);
		
		// Skip description field if any.
		if (head.IDLength>0)
			f.RelSeek(head.IDLength);
		
		// Is there any palette?
		if (head.ColorMapType == TGA_PALETTE)
		{
			// Is the palette needed?
			if (ImageType == TGA_FORMAT_CLUT8)
			{
				if ((head.CMapLength != 256) || (head.CMapDepth != 24 && head.CMapDepth != 32)) Error("Read", "Unsupported colormap type.");
				CPixel32 Pal[256];
				if(head.CMapDepth == 24 )
				{
					uint8 RawPal[768];
					f.Read(&RawPal, 768);
					for (int i = 0; i < 256; i++) { Pal[i] = CPixel32(RawPal[i*3+2], RawPal[i*3+1], RawPal[i*3+0]); };
				}
				else
				{
					uint8 RawPal[1024];
					f.Read(&RawPal, 1024);
					for (int i = 0; i < 256; i++) { Pal[i] = CPixel32(RawPal[i*4+2], RawPal[i*4+1], RawPal[i*4+0], RawPal[i*4+3]); };
				}
				img->GetPalette()->SetPalette(Pal, 0, 256);
			}
			else
				f.RelSeek(head.CMapLength);
		};
		
		// Is the image uncompressed or is it RLE encoded?
		if ((head.ImageType & TGA_FLAGS_RLE ) == 0)
		{
			//LogFile(CStrF("Uncompreseed: %s", CImage::GetFormatName(format)) );
			// Image is uncompressed (8/15/16/24-bit color,32-bit RGBA or 8-bit greyscale).
			int psize = img->GetPixelSize();
			int scanlinesize = head.Width * psize;
			
			if (scanlinesize > 0) 
			{
				if ((head.ImageDescriptor & TGA_DESCR_YORIGINTOP) != 0) 
				{
					// Image is saved forwards.
					int nlines = head.Height;
					
					int Modulo = img->GetModulo();
					uint8* pBitmap = (uint8*) img->Lock();
					for (int y = 0; y < nlines; y++)
					{
						// pBitmap points at current line
						f.Read(pBitmap, scanlinesize);
						pBitmap	+= Modulo;
						
#ifdef	CPU_BIGENDIAN
						if (format == IMAGE_FORMAT_BGRA8)
						{
							int nPix = scanlinesize/4;
							uint32* pPix32 = (uint32*) &pBitmap[Modulo * y];
							uint8* pPix8 = &pBitmap[Modulo * y];
							for(int i = 0; i < nPix; i++)
							{
								pPix32[i] = 
									((int)pPix8[i*4 + 0] << 0) + 
									((int)pPix8[i*4 + 1] << 8) + 
									((int)pPix8[i*4 + 2] << 16) + 
									((int)pPix8[i*4 + 3] << 24);
							}
						}
#endif	// CPU_BIGENDIAN
						//							img->SetRAWData(CPnt(0, y), linesize, scanline);
						
						//							f.RelSeek(scanlinesize-linesize);
//						if (f.EndOfFile())
//							Error("Read", CStrF("Unexpected end of file. (File pos: %d %d %d %d %d)", f.Pos(), scanlinesize, img->GetWidth(), img->GetHeight(), f.Length()));
					};
					img->Unlock();
				}
				else
				{
					// Image is saved backwards.
					int nlines = head.Height;

					uint8* pBitmap = (uint8*) img->Lock();
					int Modulo = img->GetModulo();
					pBitmap += Modulo * nlines;
					for (int y = 0; y < nlines; y++)
					{
						// pBitmap points at next line
						pBitmap	-= Modulo;
						f.Read(pBitmap, scanlinesize);

#ifdef	CPU_BIGENDIAN
						if (format == IMAGE_FORMAT_BGRA8)
						{
							int nPix = scanlinesize/4;
							uint32* pPix32 = (uint32*) &pBitmap[Modulo * nlines];
							uint8* pPix8 = &pBitmap[Modulo * nlines];
							for(int i = 0; i < nPix; i++)
							{
								pPix32[i] = 
									((int)pPix8[i*4 + 0] << 0) + 
									((int)pPix8[i*4 + 1] << 8) + 
									((int)pPix8[i*4 + 2] << 16) + 
									((int)pPix8[i*4 + 3] << 24);
							}
						}
#endif	//CPU_BIGENDIAN
					}
					img->Unlock();
				};
			};
			
		}
		else
		{
			//LogFile(CStrF("RLE: %s", CImage::GetFormatName(format)) );
			// RLE decode the 8/15/16/24-bit or 32-bit RGBA image.
			_w = head.Width;
			_h = head.Height;
			
//			int32 Before=0;
//			int32 After=0;
			
			int32 PixelSize=img->GetPixelSize();
//			uint32 PixelMask=(1 << (PixelSize*8))-1;
			
			int32 LineSize=_w*PixelSize;
//			uint8* Scanline = DNew(uint8) uint8[LineSize+4];
//			uint8* ScanlineEncoded = DNew(uint8) uint8[LineSize*2+4];	// *2 seems reasonable.
			TThinArray<uint8> lScanLine;
			TThinArray<uint8> lScanLineEncoded;
			lScanLine.SetLen( LineSize+4 );
			lScanLineEncoded.SetLen( LineSize*2+4 );
			uint8* Scanline = lScanLine.GetBasePtr();
			uint8* ScanlineEncoded = lScanLineEncoded.GetBasePtr();;
			if (Scanline == NULL || ScanlineEncoded == NULL)
				MemError("Read");
			
//			int32 FileLength=f.Length()-f.Pos();
			
			
			// This code doesn't v-flip the picture if neccesary
			DecodeRLEFile( f, *img, PixelSize );

			if((head.ImageDescriptor & TGA_DESCR_YORIGINTOP) == 0)
				img->FlipV();

//			try
//			{
//				bint Backwards=(head.ImageDescriptor & TGA_DESCR_YORIGINTOP) == 0;
//
//				f.Read(ScanlineEncoded,Min(int32(LineSize*2),FileLength));
//				FileLength-=LineSize*2;
//
//				int32 Len;
//				if (!Backwards)
//					Len=_h+org.y;
//				else
//					Len=head.Height-org.y;
//
//				int i;
//				int64 Val=-1;
//				int Count2=0,Count3=0,Count4=0;
//				for (i=0; i<Len; i++) {
//
//					int Count=0;
//					while (Count<LineSize) {
//
//						// Need a newer command yet?
//						if (Count3==0 && Count4==0) {
//							if ((*(ScanlineEncoded+Count2) & 0x80)>0)
//								Count3=(*(ScanlineEncoded+Count2++) & 0x7f) + 1;
//							else
//								Count4=*(ScanlineEncoded+Count2++) + 1;
//
//							if (Count2>=LineSize*2-PixelSize) {
//								if (f.EndOfFile())
//									Error("Read", CStrF("Unexpected end of file. (File pos: %d %d %d %d %d)", f.Pos(), LineSize, img->GetWidth(), img->GetHeight(), f.Length()));
//								f.RelSeek(Count2-LineSize*2);
//								f.Read(ScanlineEncoded,Min(LineSize*2,FileLength-(Count2-LineSize*2)));
//								FileLength-=Count2;
//								Count2=0;
//							}
//						}
//
//						if (Count3>0) {
//							if (Val==-1) {
//								Val=*((uint32*)(ScanlineEncoded+Count2)) & PixelMask;
//								Count2+=PixelSize;
//
//								if (Count2>=LineSize*2-PixelSize) {
//									if (f.EndOfFile())
//										Error("Read", CStrF("Unexpected end of file. (File pos: %d %d %d %d %d)", f.Pos(), LineSize, img->GetWidth(), img->GetHeight(), f.Length()));
//									f.RelSeek(Count2-LineSize*2);
//									f.Read(ScanlineEncoded,Min(LineSize*2,FileLength-(Count2-LineSize*2)));
//									FileLength-=Count2;
//									Count2=0;
//								}
//							}
//
//							while (Count3>0 && Count<LineSize) {
//								*((uint32*)(Scanline+Count))=Val;
//								Count+=PixelSize;
//								Count3--;
//							}
//						}
//						else {
//							while (Count4>0 && Count<LineSize) {
//								*((uint32*)(Scanline+Count))=*((uint32*)(ScanlineEncoded+Count2)) & PixelMask;
//								Count+=PixelSize;
//								Count2+=PixelSize;
//
//								if (Count2>=LineSize*2-PixelSize) {
//									if (f.EndOfFile())
//										Error("Read", CStrF("Unexpected end of file. (File pos: %d %d %d %d %d)", f.Pos(), LineSize, img->GetWidth(), img->GetHeight(), f.Length()));
//									f.RelSeek(Count2-LineSize*2);
//									f.Read(ScanlineEncoded,Min(LineSize*2,FileLength-(Count2-LineSize*2)));
//									FileLength-=Count2;
//									Count2=0;
//								}
//
//								Count4--;
//							}
//						}
//
//						if (Count3==0)
//							Val=-1;
//					}
//
//					if (format == IMAGE_FORMAT_BGRA8)
//					{
//						int n = _w-Before-After;
//						int b = Before;
//						for(int i = b; i < (n + b); i++)
//						{
///*							uint32 pix =
//								((int)Scanline[i*4 + 0] << 8) + 
//								((int)Scanline[i*4 + 1] << 16) + 
//								((int)Scanline[i*4 + 2] << 24) + 
//								((int)Scanline[i*4 + 3] << 0);*/
//							uint32 pix =
//								((int)Scanline[i*4 + 0] << 0) + 
//								((int)Scanline[i*4 + 1] << 8) + 
//								((int)Scanline[i*4 + 2] << 16) + 
//								((int)Scanline[i*4 + 3] << 24);
//							*((uint32*)&Scanline[i*4]) = pix;
//
///*							uint8 a = Scanline[i*4+3];
//							Scanline[i*4+3] = Scanline[i*4+2];
//							Scanline[i*4+2] = Scanline[i*4+1];
//							Scanline[i*4+1] = Scanline[i*4+0];
//							Scanline[i*4+0] = a;*/
//						}
//					}
//
//					if (!Backwards) {
//						if (i>=org.y)
//							img->SetRAWData(CPnt(0, i-org.y), (_w-Before-After)*PixelSize, Scanline+Before*PixelSize);
//					}
//					else {
//						if (i>=Len-_h)
//							img->SetRAWData(CPnt(0, Len-1-i), (_w-Before-After)*PixelSize, Scanline+Before*PixelSize);
//					}
//
//				}
//			}
//			catch(...)
//			{
//				throw;
//			};


		};

		f.Close();
	};
	// ---------------------------------
  bool DecodeRLEFile( CCFile &_File, CImage &_Img, int _iBytesPerPixel )
  {
    uint8 ControlByte;
    uint8 pPixelBytes[4];

    int iCurrentDestPos = 0;
    
    int w = _Img.GetWidth();
    int h = _Img.GetHeight();
//  int m = _Img.GetModulo();

    uint8 *pPixels = (uint8*)_Img.Lock();

/*
    do
    {
      // Read control byte - it will describe the raw/rle package
      _File.ReadLE( ControlByte );
      
      int iCount = ( ControlByte & 0x7f ) + 1;
      
      if( ControlByte & 0x80 )
      { // RLE
        _File.ReadLE( pPixelBytes , _iBytesPerPixel );
        int i,j;
        for( i=0; i < iCount; i++ )
        {
          for( j=0; j < _iBytesPerPixel; j++ )
            pPixels[ (iCurrentDestPos / w) * m + (iCurrentDestPos % w) * _iBytesPerPixel + j ] = pPixelBytes[j];
          iCurrentDestPos++;
        }
      }
      else
      { // RAW
        _File.ReadLE( pPixels + (iCurrentDestPos / w) * m + (iCurrentDestPos % w) * _iBytesPerPixel , _iBytesPerPixel * iCount );
        iCurrentDestPos += iCount;
      }
    }
    while( iCurrentDestPos < w * h );
*/
	do
	{
		// Read control byte - it will describe the raw/rle package
		_File.ReadLE( ControlByte );
      
		int iCount = ( ControlByte & 0x7f ) + 1;
		iCurrentDestPos	+= iCount;

		if( ControlByte & 0x80 )
		{ // RLE
			_File.ReadLE( pPixelBytes, _iBytesPerPixel );
			switch( _iBytesPerPixel )
			{
			case 1:
				{
					for( int i = 0; i < iCount; i++ )
						*pPixels++ = *pPixelBytes;
					break;
				}
			case 2:
				{
					for( int i = 0; i < iCount; i++ )
					{
						pPixels[0] = pPixelBytes[0];
						pPixels[1] = pPixelBytes[1];
						pPixels	+= 2;
					}
					break;
				}
			case 3:
				{
					for( int i = 0; i < iCount; i++ )
					{
						pPixels[0] = pPixelBytes[0];
						pPixels[1] = pPixelBytes[1];
						pPixels[2] = pPixelBytes[2];
						pPixels	+= 3;
					}
					break;
				}
			case 4:
				{
					for( int i = 0; i < iCount; i++ )
					{
						pPixels[0] = pPixelBytes[0];
						pPixels[1] = pPixelBytes[1];
						pPixels[2] = pPixelBytes[2];
						pPixels[3] = pPixelBytes[3];
						pPixels	+= 4;
					}
					break;
				}
			}
		}
		else
		{ // RAW
			int nBytes = iCount * _iBytesPerPixel;
			_File.ReadLE( pPixels, nBytes );
			pPixels	+= nBytes;
		}
    }
    while( iCurrentDestPos < w * h );

	_Img.Unlock();
    return true;
  }
	// ---------------------------------
	void Write(const CStr& filename, CImage* img)
	{
		TGA_Head head;

		CCFile f;
		f.Open(filename, CFILE_WRITE | CFILE_BINARY);

		// Get the image dimensions.
		int PixelSize=img->GetPixelSize();

		uint16 Width=img->GetWidth();
		uint16 Height=img->GetHeight();

		int32 _w = Width;
		int32 _h = Height;

		// Color depth supported?
		int Fmt = img->GetFormat();
		if (!((Fmt == IMAGE_FORMAT_I8) || (Fmt == IMAGE_FORMAT_CLUT8) ||
			(Fmt == IMAGE_FORMAT_B5G6R5) || (Fmt == IMAGE_FORMAT_BGR5) || (Fmt == IMAGE_FORMAT_BGR5A1) || 
			(Fmt == IMAGE_FORMAT_BGR8) || (Fmt == IMAGE_FORMAT_BGRX8) || (Fmt == IMAGE_FORMAT_BGRA8)))
			  Error("Write", CStrF("Unsupported color depth: %s", CImage::GetFormatName(Fmt)));

/*	Daniel!, Har du Paranoia????  =)

    IMAGE_FORMAT_I8 => (pixelsize = 1)

		if ((img->GetFormat()!=IMAGE_FORMAT_I8 && (PixelSize<1 || PixelSize>4)) ||
			(PixelSize==4 && img->GetFormat()!=IMAGE_FORMAT_BGRA8) ||
			(img->GetFormat()==IMAGE_FORMAT_I8 && PixelSize!=1))
				Error("Write", "Unsupported color depth.");*/

/*	Pixelsize == 1,  medför inte att det är index eller gråskala.
	Det ser ut som att du antagit det neröver, jag har ändrat en del men
	jag har nog missat desto mer.
*/

		// Fill in the header.
		if (img->GetFormat()!=IMAGE_FORMAT_I8) {
			// Color.
			head.IDLength=0;
			head.ColorMapType=(PixelSize==1) ? (TGA_PALETTE) : (TGA_NO_PALETTE);
			head.ImageType=(PixelSize==1) ? (TGA_FORMAT_CLUT8_RLE) : (TGA_FORMAT_RGB_RLE);
			head.CMapStart=0;
			head.CMapLength=(PixelSize==1) ? (256) : (0);
			head.CMapDepth=(PixelSize==1) ? (24) : (0);
			head.XOffset=0;
			head.YOffset=0;
			head.Width=_w;
			head.Height=_h;
			head.PixelDepth=PixelSize*8;
			head.ImageDescriptor=TGA_DESCR_YORIGINTOP;

			// Any attribute bits?
			switch(img->GetFormat())
			{
			case IMAGE_FORMAT_BGR5 :
			case IMAGE_FORMAT_BGR5A1 :
				head.ImageDescriptor |= 1;				// RGB555 is 16 bits with 1 unused.
				break;
			case IMAGE_FORMAT_B5G6R5 :
				head.ImageDescriptor |= 0;				// RGB565 is 16 bits with 0 unused. ????
				break;
			case IMAGE_FORMAT_BGRX8 :
			case IMAGE_FORMAT_BGRA8 :
				head.ImageDescriptor |= 8;				// RGBA32 is 32 bits with 8 for alpha.
				break;
			}
		}
		else {
			// Greyscale.
			head.IDLength=0;
			head.ColorMapType=TGA_NO_PALETTE;
			head.ImageType=TGA_FORMAT_GREY_RLE;
			head.CMapStart=0;
			head.CMapLength=0;
			head.CMapDepth=0;
			head.XOffset=0;
			head.YOffset=0;
			head.Width=_w;
			head.Height=_h;
			head.PixelDepth=PixelSize*8;
			head.ImageDescriptor=TGA_DESCR_YORIGINTOP;
		}
		head.Write(&f);

		// 8-bit color image?
		if (img->GetFormat() == IMAGE_FORMAT_CLUT8) 
		{
			// Save the palette.
			CPixel32 Palette[256];
			img->GetPalette()->GetPalette(Palette, 0, 256);

			int i;
			for (i=0; i<256; i++) {
				f.Write(Palette[i].GetB());
				f.Write(Palette[i].GetG());
				f.Write(Palette[i].GetR());
			}
		}

		// RLE encode the 8/15/16/24-bit or 32-bit RGBA image.
		TThinArray<uint8> lScanLine;
		TThinArray<uint8> lScanLineEncoded;
		lScanLine.SetLen( _w*PixelSize+4 );
		lScanLineEncoded.SetLen( 128*PixelSize+4 );
//		uint8* Scanline = DNew(uint8) uint8[_w*PixelSize+4];
//		uint8* ScanlineEncoded = DNew(uint8) uint8[128*PixelSize+4];
		uint8* Scanline = lScanLine.GetBasePtr();
		uint8* ScanlineEncoded = lScanLineEncoded.GetBasePtr();
		if (Scanline == NULL || ScanlineEncoded == NULL)
			MemError("Write");

		uint32 PixelMask = 0;
		switch(PixelSize)
		{
		case 4 : PixelMask = 0xffffffff; break;
		default :
			PixelMask = (1 << (PixelSize*8)) - 1;
		}
		M_TRY
		{
			int64 Last=-1;
			int Doing=0;
			int32 Count2=0;

			int i;
			for (i=0; i<_h; i++) {

				img->GetRAWData(CPnt(0, i), _w*PixelSize,Scanline);

				int32 Count=0;
				while (Count<_w*PixelSize) {

					// What to do?
					switch (Doing) {
					case 0: {
						if (Last==-1) {
							Last=*((uint32*)(Scanline+Count)) & PixelMask;
							*((uint32*)(ScanlineEncoded+Count2))=Last;
						}
						else {
							if (Last==(*((uint32*)(Scanline+Count)) & PixelMask))
								Doing=1;
							else {
								Doing=2;
								Last=*((uint32*)(Scanline+Count)) & PixelMask;
								*((uint32*)(ScanlineEncoded+Count2))=Last;
							}
						}
						Count+=PixelSize;
						Count2+=PixelSize;
					}
					break;
					case 1: {
						if (Last==(*((uint32*)(Scanline+Count)) & PixelMask))
							Count2+=PixelSize;

						if (Last!=(*((uint32*)(Scanline+Count)) & PixelMask) || Count2==128*PixelSize) {
							Doing=0;
							uint32 Val=Last;

							Last=(Count2==128*PixelSize) ? ((int64)-1) : (*((uint32*)(Scanline+Count)) & PixelMask);
							Count+=PixelSize;

							uint8 C2=((Count2/PixelSize)-1) | 0x80;
							f.Write(C2);
							f.Write(&Val,PixelSize);

							if (Last==-1)
								Count2=0;
							else {
								*((uint32*)(ScanlineEncoded))=Last;
								Count2=PixelSize;
							}
						}
						else
							Count+=PixelSize;

					}
					break;
					case 2: {
						if (Last!=(*((uint32*)(Scanline+Count)) & PixelMask)) {
							*((uint32*)(ScanlineEncoded+Count2))=*((uint32*)(Scanline+Count)) & PixelMask;
							Count2+=PixelSize;
						}

						if (Last==(*((uint32*)(Scanline+Count)) & PixelMask) || Count2==128*PixelSize) {
							uint32 Val=Last;

							Last=(Count2==128*PixelSize) ? ((int64)-1) : (*((uint32*)(Scanline+Count)) & PixelMask);
							Count+=PixelSize;

							if (Last!=-1) Count2-=PixelSize;
							uint8 C2=(Count2/PixelSize)-1;
							f.Write(C2);
							f.Write(ScanlineEncoded,Count2);
							if (Last==-1) {
								Count2=0;
								Doing=0;
							}
							else {
								*((uint32*)(ScanlineEncoded))=Val;
								*((uint32*)(ScanlineEncoded+PixelSize))=Last;
								Count2=2*PixelSize;
								Doing=1;
							}
						}
						else {
							Last=*((uint32*)(Scanline+Count)) & PixelMask;
							Count+=PixelSize;
						}
					}
					} // End switch

				}

			}

			// Anything left to save?
			if (Count2>0) {
				switch (Doing) {
				case 0:
				case 2: {
						uint8 C2=(Count2/PixelSize)-1;
						f.Write(C2);
						f.Write(ScanlineEncoded,Count2);
						}
					break;
				case 1: {
						uint8 C2=((Count2/PixelSize)-1) | 0x80;
						uint32 L=Last;
						f.Write(C2);
						f.Write(&L,PixelSize);
						}
				};
			}
		}
		M_CATCH(
		catch(...)
		{
			throw;
		};
		)

		f.Close();
	};

	// ---------------------------------
	void ReadPalette(const CStr& filename, CImagePalette* destpal)
	{
		TGA_Head head;

		CCFile f;
		f.Open(filename, CFILE_READ | CFILE_BINARY);
		head.Read(&f);
//		int32 format = TGA_Format2CImageFormat(head);

		// Skip description field if any.
		if (head.IDLength>0)
			f.RelSeek(head.IDLength);

		// Is there any palette?
		if (head.ColorMapType == TGA_PALETTE)
		{
			// Is the palette needed?
			if (head.ImageType==TGA_FORMAT_CLUT8 || head.ImageType==TGA_FORMAT_CLUT8_RLE) {
				if ((head.CMapLength != 256) || (head.CMapDepth != 24)) Error("Read", "Unsupported colormap type.");
				uint8 RawPal[768];
				f.Read(RawPal, 768);
				CPixel32 Pal[256];
				for (int i = 0; i < 256; i++) { Pal[i] = CPixel32(RawPal[i*3+2], RawPal[i*3+1], RawPal[i*3+0]); };
				destpal->SetPalette((CPixel32*) &Pal, 0, 256);
			}
		}
		else
			Error("ReadPalette", "Targa palette is missing.");

		f.Close();
	};
};

#endif

// -------------------------------------------------------------------
//  CImageIO_PCX
// -------------------------------------------------------------------
//
// CLASS AUTHOR:	Daniel Hansson
// STARTED	   :	961127
// LAST UPDATED:	970223
// SUPPORTS	   :	Loading/Saving of 8/24-bit RLE encoded images.

#ifndef IMAGE_IO_NOPCX

class CImageIO_PCX : public CImageIO
{
	// ---------------------------------
	struct PCX_Head
	{
		uint8 Identifier;
		uint8 Version;
		uint8 Encoding;
		uint8 BitsPerPixel;
		uint16 XStart;
		uint16 YStart;
		uint16 XEnd;
		uint16 YEnd;
		uint16 XResolution;
		uint16 YResolution;
		uint8 Reserved1;
		uint8 NumBitPlanes;
		uint16 BytesPerLine;
		uint16 PaletteType;
		uint16 Width;
		uint16 Height;


		void Read(CCFile* f)
		{
			f->Read(Identifier);
			f->Read(Version);
			f->Read(Encoding);
			f->Read(BitsPerPixel);
			f->ReadLE(XStart);
			f->ReadLE(YStart);
			f->ReadLE(XEnd);
			f->ReadLE(YEnd);
			f->ReadLE(XResolution);
			f->ReadLE(YResolution);

			f->RelSeek(48);

			f->Read(Reserved1);
			f->Read(NumBitPlanes);
			f->ReadLE(BytesPerLine);
			f->ReadLE(PaletteType);
			f->ReadLE(Width);
			f->ReadLE(Height);
		};

		void Write(CCFile* f)
		{
			f->Write(Identifier);
			f->Write(Version);
			f->Write(Encoding);
			f->Write(BitsPerPixel);
			f->WriteLE(XStart);
			f->WriteLE(YStart);
			f->WriteLE(XEnd);
			f->WriteLE(YEnd);
			f->WriteLE(XResolution);
			f->WriteLE(YResolution);

			uint8 Zero=0;
			int i;
			for (i=0; i<48; i++)
				f->Write(Zero);

			f->Write(Reserved1);
			f->Write(NumBitPlanes);
			f->WriteLE(BytesPerLine);
			f->WriteLE(PaletteType);
			f->WriteLE(Width);
			f->WriteLE(Height);

			for (i=0; i<54; i++)
				f->Write(Zero);

		};
	};

	// ---------------------------------
	#define PCX_IDENTIFIER		0x0a
	#define PCX_ENCODING_RLE	1

	// ---------------------------------
	int32 PCX_Format2CImageFormat(const PCX_Head& head)
	{
		switch (head.BitsPerPixel*head.NumBitPlanes)
		{
			case 8: return(IMAGE_FORMAT_CLUT8);
			case 24 : return(IMAGE_FORMAT_BGR8);
			default : return 0;
		};
		return 0;
	};
	
	// ---------------------------------
	spCImage ReadInfo(const CStr& filename)
	{
		spCImage spImg = MNew(CImage);
		if (spImg == NULL) MemError("ReadInfo");

		PCX_Head head;

		CCFile f;
		f.Open(filename, CFILE_READ | CFILE_BINARY);
		head.Read(&f);
		int32 format = PCX_Format2CImageFormat(head);

		uint16 Width=head.XEnd-head.XStart+1;
		uint16 Height=head.YEnd-head.YStart+1;

		spImg->CreateVirtual(Width, Height, format, IMAGE_MEM_SYSTEM);

		f.Close();

		return spImg;
	};

	// ---------------------------------
	void Read(const CStr& filename, CImage* img, int _MemModel)
	{
		PCX_Head head;

		CCFile f;
		f.Open(filename, CFILE_READ | CFILE_BINARY);
		head.Read(&f);
		int32 format = PCX_Format2CImageFormat(head);

		// Format supported?
		if (head.Identifier!=PCX_IDENTIFIER)
			Error("Read", "Not a PCX file.");

		if (head.Version!=5)
			Error("Read", "Unsupported PCX version.");

		if (head.Encoding!=PCX_ENCODING_RLE)
			Error("Read", "Unsupported PCX encoding.");

		// Create the image object.
		int Width=head.XEnd-head.XStart+1;
		int Height=head.YEnd-head.YStart+1;

		int _w = Width;
		int _h = Height;

		int Before=0;
		int After=0;

		img->Create(_w, _h, format, _MemModel);

		uint8 BPP=head.BitsPerPixel*head.NumBitPlanes;

		// Color depth supported?
		if (BPP!=8 && BPP!=24)
			Error("Read", "Unsupported color depth.");

		if (BPP==8) {

			// Get the palette at the end of the file.
			f.SeekToEnd();
			f.RelSeek(-769);

			uint8 PaletteId;
			f.Read(PaletteId);

			if (PaletteId!=12)
				Error("Read", "PCX palette is missing.");

			uint8 Pal[768];

			f.Read(Pal,768);

			CPixel32 Palette[256];
			int i;
			for (i=0; i<256; i++)
				Palette[i]=CPixel32(Pal[i*3],Pal[i*3+1],Pal[i*3+2]);

			img->GetPalette()->SetPalette(Palette, 0, 256);

			// Decode the 8-bit image.
			f.Seek(128);

			TThinArray<uint8> lScanLine;
			TThinArray<uint8> lScanLineEncoded;
			lScanLine.SetLen( head.BytesPerLine );
			lScanLineEncoded.SetLen( head.BytesPerLine*2 );
			uint8* Scanline = lScanLine.GetBasePtr();
			uint8* ScanlineEncoded = lScanLineEncoded.GetBasePtr();;

//			uint8* ScanlineEncoded=DNew(uint8) uint8[head.BytesPerLine*2];
//			uint8* Scanline=DNew(uint8) uint8[head.BytesPerLine];
			if (ScanlineEncoded==NULL || Scanline==NULL)
				MemError("Read");

			int32 FileLength=f.Length()-f.Pos();

			try
			{
				f.Read(ScanlineEncoded,MinMT(head.BytesPerLine*2,FileLength));
				FileLength-=head.BytesPerLine*2;

				int Count=0;
				for (i=0; i<_h; i++) {

					int Count2=0;
					while (Count2<head.BytesPerLine) {

						if (*(ScanlineEncoded+Count)<0xc0)
							*(Scanline+Count2++)=*(ScanlineEncoded+Count++);
						else {

							uint8 Len=*(ScanlineEncoded+Count++) & 0x3f;

							if (Count>=head.BytesPerLine*2) {
								if (f.EndOfFile())
									Error("Read", CStrF("Unexpected end of file. (File pos: %d %d %d %d %d)", f.Pos(), head.BytesPerLine, img->GetWidth(), img->GetHeight(), f.Length()));

								f.Read(ScanlineEncoded,MinMT(head.BytesPerLine*2,FileLength));
								FileLength-=head.BytesPerLine*2;
								Count=0;
							}

							uint8 Val=*(ScanlineEncoded+Count++);

							int j;
							for (j=0; j<Len; j++)
								*(Scanline+Count2++)=Val;
						}

						if (Count>=head.BytesPerLine*2) {
							if (f.EndOfFile())
								Error("Read", CStrF("Unexpected end of file. (File pos: %d %d %d %d %d)", f.Pos(), head.BytesPerLine, img->GetWidth(), img->GetHeight(), f.Length()));

							f.Read(ScanlineEncoded,MinMT(head.BytesPerLine*2,FileLength));
							FileLength-=head.BytesPerLine*2;
							Count=0;
						}
					}

					img->SetRAWData(CPnt(0, i), _w-Before-After, Scanline+Before);
				}
			}
			catch(CCException)
			{
				throw;
			};
		}
		else 
		{
			// Decode the 24-bit image.
			f.Seek(128);
			uint16 LineSize=head.BytesPerLine*3;

			TThinArray<uint8> lScanLine;
			TThinArray<uint8> lScanLineEncoded;
			lScanLine.SetLen( LineSize );
			lScanLineEncoded.SetLen( LineSize*2 );
			uint8* Scanline = lScanLine.GetBasePtr();
			uint8* ScanlineEncoded = lScanLineEncoded.GetBasePtr();;
//			uint8* ScanlineEncoded=DNew(uint8) uint8[LineSize*2];
//			uint8* Scanline=DNew(uint8) uint8[LineSize];
			if (ScanlineEncoded==NULL || Scanline==NULL)
				MemError("Read");

			int32 FileLength=f.Length()-f.Pos();

			try
			{
				f.Read(ScanlineEncoded,MinMT(LineSize*2,FileLength));
				FileLength -= MinMT(LineSize*2,FileLength);

				int Count=0;
				int i;
				for (i=0; i<_h; i++) {

					int j;
					for (j=2; j>=0; j--) {

						int Count2=j;
						while (Count2<LineSize) {

							if (*(ScanlineEncoded+Count)<0xc0) {
								*(Scanline+Count2)=*(ScanlineEncoded+Count++);
								Count2+=3;
							}
							else {

								uint8 Len=*(ScanlineEncoded+Count++) & 0x3f;

								if (Count>=LineSize*2) {
									if (f.EndOfFile())
										Error("Read", CStrF("Unexpected end of file. (File pos: %d %d %d %d %d)", f.Pos(), head.BytesPerLine, img->GetWidth(), img->GetHeight(), f.Length()));

									f.Read(ScanlineEncoded,MinMT(LineSize*2,FileLength));
									FileLength -= MinMT(LineSize*2,FileLength);
									Count=0;
								}

								uint8 Val=*(ScanlineEncoded+Count++);

								int j;
								for (j=0; j<Len; j++) {
									*(Scanline+Count2)=Val;
									Count2+=3;
								}

							}

							if (Count>=LineSize*2) {
								if (f.EndOfFile())
									Error("Read", CStrF("Unexpected end of file. (File pos: %d %d %d %d %d)", f.Pos(), head.BytesPerLine, img->GetWidth(), img->GetHeight(), f.Length()));

								f.Read(ScanlineEncoded,MinMT(LineSize*2,FileLength));
								FileLength -= MinMT(LineSize*2,FileLength);
								Count=0;
							}
						}

					}

						img->SetRAWData(CPnt(0, i), (_w-Before-After)*3, Scanline+Before*3);
				}
			}
			catch(CCException)
			{
				throw;
			};
		}

		f.Close();
	};

	// ---------------------------------
	void Write(const CStr& filename, CImage* img)
	{
		PCX_Head head;

		CCFile f;
		f.Open(filename, CFILE_WRITE | CFILE_BINARY);

		// Get the image dimensions.
		uint8 PixelSize=img->GetPixelSize();

		uint16 Width=img->GetWidth();
		uint16 Height=img->GetHeight();

		int32 _w = Width;
		int32 _h = Height;

		// Color depth supported?
		if (PixelSize!=1 && PixelSize!=3)
			Error("Write", "Unsupported color depth.");

		// Fill in the header.
		head.Identifier=PCX_IDENTIFIER;
		head.Version=5;
		head.Encoding=PCX_ENCODING_RLE;
		head.BitsPerPixel=8;	// Bits per pixel per plane (hmmm)
		head.XStart=0;
		head.YStart=0;
		head.XEnd=_w-1;
		head.YEnd=_h-1;
		head.XResolution=0;		// Original dimensions (crap)
		head.YResolution=0;
		head.Reserved1=0;
		head.NumBitPlanes=PixelSize;
		head.BytesPerLine=((_w & 1)>0) ? (_w+1) : (_w);	// Bytes per line per plane (hmmmm)
		head.PaletteType=1;	// Color palette
		head.Width=0;		// Screen dimensions (crap)
		head.Height=0;

		head.Write(&f);

		// 8-bit or 24-bit?
		if (PixelSize==1) {

			// RLE encode the 8-bit image.
//			uint8* ScanlineEncoded=DNew(uint8) uint8[head.BytesPerLine*2];
//			uint8* Scanline=DNew(uint8) uint8[head.BytesPerLine];
			TThinArray<uint8> lScanLine;
			TThinArray<uint8> lScanLineEncoded;
			lScanLine.SetLen( head.BytesPerLine );
			lScanLineEncoded.SetLen( head.BytesPerLine*2 );
			uint8* Scanline = lScanLine.GetBasePtr();
			uint8* ScanlineEncoded = lScanLineEncoded.GetBasePtr();;
			if (ScanlineEncoded==NULL || Scanline==NULL)
				MemError("Write");

			try
			{
				int i;
				for (i=0; i<_h; i++) {

					img->GetRAWData(CPnt(0, i), _w,Scanline);

					int Count=0;
					int Count3=0;
					while (Count<_w) {

						uint8 Val=*(Scanline+Count++);
						int Count2=1;
						while (Count<_w && Val==*(Scanline+Count) && Count2<63) {
							Count2++;
							Count++;
						}

						if (Count2==1) {

							if ((Val & 0xc0)!=0xc0)
								*(ScanlineEncoded+Count3++)=Val;
							else {
								*(ScanlineEncoded+Count3++)=0xc1;	// 0xc1 = 11000001b
								*(ScanlineEncoded+Count3++)=Val;
							}

						}
						else {
							*(ScanlineEncoded+Count3++)=0xc0 | Count2;
							*(ScanlineEncoded+Count3++)=Val;
						}
					}

					if (head.BytesPerLine>_w)
						*(ScanlineEncoded+Count3++)=0;

					f.Write(ScanlineEncoded,Count3);

				}
			}
			catch(CCException)
			{
				throw;
			};

			// Save the palette.
			uint8 PaletteId=12;
			f.Write(PaletteId);

			CPixel32 Palette[256];
			img->GetPalette()->GetPalette(Palette, 0, 256);

			for (int i=0; i<256; i++) {
				f.Write(Palette[i].GetR());
				f.Write(Palette[i].GetG());
				f.Write(Palette[i].GetB());
			}

		}
		else 
		{
			// RLE encode the 24-bit image.
			uint16 LineSize=_w*3;

			TThinArray<uint8> lScanLine;
			TThinArray<uint8> lScanLineEncoded;
			lScanLine.SetLen( LineSize );
			lScanLineEncoded.SetLen( LineSize*2 );
			uint8* Scanline = lScanLine.GetBasePtr();
			uint8* ScanlineEncoded = lScanLineEncoded.GetBasePtr();;
//			uint8* ScanlineEncoded=DNew(uint8) uint8[LineSize*2];
//			uint8* Scanline=DNew(uint8) uint8[LineSize];
			if (ScanlineEncoded==NULL || Scanline==NULL)
				MemError("Write");

			try
			{
				int i;
				for (i=0; i<_h; i++) {

					img->GetRAWData(CPnt(0, i), _w*3,Scanline);

					int Count3=0;

					int j;
					for (j=2; j>=0; j--) {
						int Count=j;
						while (Count<_w*3) {

							uint8 Val=*(Scanline+Count);
							Count+=3;

							int Count2=1;
							while (Count<_w*3 && Val==*(Scanline+Count) && Count2<63) {
								Count2++;
								Count+=3;
							}

							if (Count2==1) {

								if ((Val & 0xc0)!=0xc0)
									*(ScanlineEncoded+Count3++)=Val;
								else {
									*(ScanlineEncoded+Count3++)=0xc1;	// 0xc1 = 11000001b
									*(ScanlineEncoded+Count3++)=Val;
								}

							}
							else {
								*(ScanlineEncoded+Count3++)=0xc0 | Count2;
								*(ScanlineEncoded+Count3++)=Val;
							}
						}

						if (head.BytesPerLine>_w)
							*(ScanlineEncoded+Count3++)=0;

					}

					f.Write(ScanlineEncoded,Count3);

				}
			}
			catch(CCException)
			{
				throw;
			};
		}

		f.Close();
	};

	// ---------------------------------
	void ReadPalette(const CStr& filename, CImagePalette* destpal)
	{
		PCX_Head head;

		CCFile f;
		f.Open(filename, CFILE_READ | CFILE_BINARY);
		head.Read(&f);

		uint8 BPP=head.BitsPerPixel*head.NumBitPlanes;

		if (BPP!=8)
			Error("ReadPalette", "PCX palette is missing.");

		// Get the palette at the end of the file.
		f.SeekToEnd();
		f.RelSeek(-769);

		uint8 PaletteId;
		f.Read(PaletteId);

		if (PaletteId!=12)
			Error("ReadPalette", "PCX palette is missing.");

		uint8 Pal[768];

		f.Read(Pal,768);

		CPixel32 Palette[256];
		int i;
		for (i=0; i<256; i++)
			Palette[i]=CPixel32(Pal[i*3],Pal[i*3+1],Pal[i*3+2]);

		destpal->SetPalette(Palette, 0, 256);

		f.Close();
	};
};

#endif

// -------------------------------------------------------------------
//  CImageIO_JPG
// -------------------------------------------------------------------

#ifndef IMAGE_IO_NOJPG

void CImage_Convert_RGB24_BGR24(void* _pSrc, void* _pDest, CPixel32* _pSrcPal, int _nPixels);

spCImage CImageIO_JPG::ReadInfo(const CStr& filename)
{
	MAUTOSTRIP(CImageIO_JPG_ReadInfo, NULL);
	Error("ReadInfo", "Not implemented.");
	return NULL;
}

void CImageIO_JPG::ReadJPG(CCFile* _pFile, CImage* _pImg, bool _bAlpha, int _MemModel, int _Fmt)
{
	MAUTOSTRIP(CImageIO_JPG_ReadJPG, MAUTOSTRIP_VOID);
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, _pFile);

	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	int nComp = cinfo.output_components;
	int Fmt = _Fmt;

	if (!Fmt)
	switch(nComp)
	{
		case 1 : Fmt = IMAGE_FORMAT_I8; break; 
		case 3 : Fmt = IMAGE_FORMAT_BGR8; break; 
		default :
			Error_static("CImageIO_JPG::Read", "Invalid number of JPG components.");
	}

	if (!_bAlpha)
	{
		_pImg->Destroy();
		_pImg->Create(cinfo.output_width, cinfo.output_height, Fmt, IMAGE_MEM_IMAGE);
	}

	uint8* pImg = (uint8*) _pImg->Lock();
	int Mod = _pImg->GetModulo();
	int Width = _pImg->GetWidth();
	int PixelSize = _pImg->GetPixelSize();

	uint8 Buff[4096*3];
	if (_pImg->GetWidth() > 4096)
		Error_static("CImageIO_JPG::ReadJPG", "Max-width == 4096");

	while (cinfo.output_scanline < cinfo.output_height)
	{
		if (nComp == 1)
		{
			uint8* pBuff = Buff;
			uint8* pI = &pImg[cinfo.output_scanline * Mod];

			jpeg_read_scanlines(&cinfo, &pBuff, 1);
			for(int x = 0; x < Width; x++)
				pI[x * PixelSize + PixelSize-1] = Buff[x];
		}
		else
		{
			uint8* pBuff = Buff;
			uint8* pI = &pImg[cinfo.output_scanline * Mod];
			jpeg_read_scanlines(&cinfo, &pBuff, 1);

			for(int x = 0; x < Width; x++)
			{
				pI[x * PixelSize + 0] = Buff[x*3 + 2];
				pI[x * PixelSize + 1] = Buff[x*3 + 1];
				pI[x * PixelSize + 2] = Buff[x*3 + 0];
			}
		}
	}

	_pImg->Unlock();

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
}

void CImageIO_JPG::WriteJPG(CCFile* _pFile, CImage* _pImg, bool _bAlpha, fp32 _Quality)
{
	MAUTOSTRIP(CImageIO_JPG_WriteJPG, MAUTOSTRIP_VOID);
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	try
	{
		jpeg_stdio_dest(&cinfo, _pFile);

		cinfo.image_width = _pImg->GetWidth();
		cinfo.image_height = _pImg->GetHeight();
		cinfo.input_components = (_bAlpha) ? 1 : ((_pImg->GetFormat() == IMAGE_FORMAT_I8) ? 1 : 3);
		cinfo.in_color_space = (cinfo.input_components == 1) ? JCS_GRAYSCALE : JCS_RGB;

		jpeg_set_defaults(&cinfo);
		jpeg_set_quality(&cinfo, 100.0f * _Quality, TRUE /* limit to baseline-JPEG values */);
		jpeg_start_compress(&cinfo, TRUE);

		uint8* pImg = (uint8*) _pImg->Lock();
		int Mod = _pImg->GetModulo();
		int Width = _pImg->GetWidth();
		int PixelSize = _pImg->GetPixelSize();

		uint8 Buff[4096*3];
		if (Mod > 4096*3) Error_static("CImageIO_JPG::WriteJPG", "Max width == 4096");

		while (cinfo.next_scanline < cinfo.image_height)
		{
			if (cinfo.input_components == 1)
			{
				uint8* pBuff = Buff;
				uint8* pI = &pImg[cinfo.next_scanline * Mod];
				for(int x = 0; x < Width; x++)
				{
					Buff[x] = pI[x * PixelSize + PixelSize-1];
				}
				jpeg_write_scanlines(&cinfo, &pBuff, 1);
			}
			else
			{
				uint8* pBuff = Buff;
				uint8* pI = &pImg[cinfo.next_scanline * Mod];
				for(int x = 0; x < Width; x++)
				{
					Buff[x*3 + 2] = pI[x * PixelSize + 0];
					Buff[x*3 + 1] = pI[x * PixelSize + 1];
					Buff[x*3 + 0] = pI[x * PixelSize + 2];
				}
				jpeg_write_scanlines(&cinfo, &pBuff, 1);
			}
		}

		_pImg->Unlock();
		jpeg_finish_compress(&cinfo);
		jpeg_destroy_compress(&cinfo);
	}
	catch(CCException)
	{
		jpeg_destroy_compress(&cinfo);
		throw;
	}
}

void CImageIO_JPG::Read(const CStr& filename, CImage* _pImg, int _MemModel)
{
	MAUTOSTRIP(CImageIO_JPG_Read, MAUTOSTRIP_VOID);
	CCFile File;
	File.Open(filename, CFILE_READ | CFILE_BINARY);

	ReadJPG(&File, _pImg, false, _MemModel, 0);
}

void CImageIO_JPG::Write(const CStr& filename, CImage* _pImg)
{
	MAUTOSTRIP(CImageIO_JPG_Write, MAUTOSTRIP_VOID);
	CCFile File;
	File.Open(filename, CFILE_WRITE | CFILE_BINARY);

	if (_pImg->GetFormat() != IMAGE_FORMAT_BGR8 &&
		_pImg->GetFormat() != IMAGE_FORMAT_BGRX8 &&
		_pImg->GetFormat() != IMAGE_FORMAT_I8)
		Error("Write", "Unsupported image-format.");

	WriteJPG(&File, _pImg, false, 0.6f);
}

void CImageIO_JPG::ReadPalette(const CStr& filename, CImagePalette* destpal)
{
	MAUTOSTRIP(CImageIO_JPG_ReadPalette, MAUTOSTRIP_VOID);
	Error("ReadPalette", "Not implemented.");
}

#endif

// -------------------------------------------------------------------
//  CImageIO_PNG
// -------------------------------------------------------------------
//
// CLASS AUTHOR:	Jim Kjellin
// STARTED	   :	2004-10-11
// LAST UPDATED:	2004-10-11

#ifdef IMAGE_IO_PNG

class CImageIO_PNG : public CImageIO
{
protected:

	enum
	{
		FLIP_RGB_COMPONENTS	= 0x0001
	};

	int IHDRFormat2CImageFormat( png_structp png_ptr, png_infop info_ptr)
	{
		switch( info_ptr->color_type )
		{
		case PNG_COLOR_TYPE_GRAY:
			{
				if( info_ptr->bit_depth == 16 )
					return IMAGE_FORMAT_I16;
				else if( info_ptr->bit_depth == 8 )
					return IMAGE_FORMAT_I8;
				break;
			}

		case PNG_COLOR_TYPE_RGB:
			{
				if( info_ptr->bit_depth == 16 )
					return IMAGE_FORMAT_RGB16;
				else if( info_ptr->bit_depth == 8 )
					return IMAGE_FORMAT_BGR8;
				break;
			}

		case PNG_COLOR_TYPE_RGBA:
			{
				if( info_ptr->bit_depth == 16 )
					return IMAGE_FORMAT_BGRA16;
				else if( info_ptr->bit_depth == 8 )
					return IMAGE_FORMAT_BGRA8;
				break;
			}

		case PNG_COLOR_TYPE_GRAY_ALPHA:
			{
				if( info_ptr->bit_depth == 8 )
					return IMAGE_FORMAT_I8A8;
				break;
			}
		}
		return IMAGE_FORMAT_VOID;
	}

	void CImageFormat2IHDRFormat( CImage* _pImg, int& nFlags, int& nBitDepth, int& nColorType )
	{
		nFlags	= 0;
		switch( _pImg->GetFormat() )
		{
/*		case IMAGE_FORMAT_CLUT4:
			{
				nBitDepth	= 4;
				nColorType	= PNG_COLOR_TYPE_PALETTE;
				break;
			}*/

		case IMAGE_FORMAT_I8A8:
			{
				nBitDepth = 8;
				nColorType	= PNG_COLOR_TYPE_GRAY_ALPHA;
				break;
			}

		case IMAGE_FORMAT_CLUT8:
			{
				nBitDepth	= 8;
				nColorType	= PNG_COLOR_TYPE_PALETTE;
				break;
			}

		case IMAGE_FORMAT_I8:
			{
				nBitDepth	= 8;
				nColorType	= PNG_COLOR_TYPE_GRAY;
				break;
			}
		case IMAGE_FORMAT_I16:
			{
				nBitDepth	= 16;
				nColorType	= PNG_COLOR_TYPE_GRAY;
				break;
			}

		case IMAGE_FORMAT_BGR8:
			{
				nFlags	|= FLIP_RGB_COMPONENTS;
				nBitDepth	= 8;
				nColorType	= PNG_COLOR_TYPE_RGB;
				break;
			}

		case IMAGE_FORMAT_RGB8:
			{
				nBitDepth	= 8;
				nColorType	= PNG_COLOR_TYPE_RGB;
				break;
			}

		case IMAGE_FORMAT_RGB16:
			{
				nBitDepth	= 16;
				nColorType	= PNG_COLOR_TYPE_RGB;
				break;
			}

		case IMAGE_FORMAT_BGRA16:
			{
				nFlags	|= FLIP_RGB_COMPONENTS;
				nBitDepth	= 16;
				nColorType	= PNG_COLOR_TYPE_RGBA;
				break;
			}
		case IMAGE_FORMAT_BGRA8:
			{
				nFlags	|= FLIP_RGB_COMPONENTS;
				nBitDepth	= 8;
				nColorType	= PNG_COLOR_TYPE_RGBA;
				break;
			}
		}
	}
	static void PngRead(png_structp _png_ptr, png_bytep _png_bytep, png_size_t _png_bytes)
	{
		CCFile* pFile = (CCFile*)_png_ptr->io_ptr;
		pFile->Read( _png_bytep, _png_bytes );
	}

	static void PngWrite(png_structp _png_ptr, png_bytep _png_bytep, png_size_t _png_bytes)
	{
		CCFile* pFile = (CCFile*)_png_ptr->io_ptr;
		pFile->Write( _png_bytep, _png_bytes );
	}

	static void PngFlush(png_structp _png_ptr)
	{
	}
public:
	spCImage ReadInfo(const CStr& filename)
	{
		spCImage spImg;
		png_structp	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		M_ASSERT( png_ptr, "Out of memory!" );
		png_infop	info_ptr = png_create_info_struct(png_ptr);
		M_ASSERT( info_ptr, "Out of memory!" );

		if (setjmp(png_jmpbuf(png_ptr)))
		{
			/* Free all of the memory associated with the png_ptr and info_ptr */
			png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
			/* If we get here, we had a problem reading the file */
			return spImg;
		}

		spImg = MNew(CImage);

		CCFile File;

		File.Open(filename,CFILE_READ | CFILE_BINARY);

		png_set_read_fn(png_ptr, (void *)&File, PngRead);
		png_set_sig_bytes(png_ptr, 0);

		png_read_info(png_ptr, info_ptr);

		File.Close();

		spImg->CreateVirtual( info_ptr->width, info_ptr->height, IHDRFormat2CImageFormat(png_ptr, info_ptr), IMAGE_MEM_SYSTEM );

		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

		return spImg;
	}

	void Read(const CStr& filename, CImage* img, int _MemModel)
	{
		spCImage spImg = MNew(CImage);

		png_structp	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		M_ASSERT( png_ptr, "Out of memory!" );
		png_infop	info_ptr = png_create_info_struct(png_ptr);
		M_ASSERT( info_ptr, "Out of memory!" );

		if (setjmp(png_jmpbuf(png_ptr)))
		{
			/* Free all of the memory associated with the png_ptr and info_ptr */
			png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
			/* If we get here, we had a problem reading the file */
			Error_static("CImageIO_PNG::Read", "Failed to setup png reading" );
		}

		CCFile File;

		File.Open(filename,CFILE_READ | CFILE_BINARY);

		png_set_read_fn(png_ptr, (void *)&File, PngRead);
		png_set_sig_bytes(png_ptr, 0);

		png_set_bgr( png_ptr );

		png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

		File.Close();

		img->Create( info_ptr->width, info_ptr->height, IHDRFormat2CImageFormat(png_ptr, info_ptr), _MemModel );

		png_bytep pImagePtr = (png_bytep)img->Lock();
		uint32 height = info_ptr->height;
		for( uint32 i = 0; i < height; i++ )
			memcpy( pImagePtr + img->GetModulo() * i, info_ptr->row_pointers[i], info_ptr->rowbytes );

		img->Unlock();

		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

		return;
	}

	void Write(const CStr& filename, CImage* img)
	{
		png_structp	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

		/* Allocate/initialize the image information data.  REQUIRED */
		png_infop	info_ptr = png_create_info_struct(png_ptr);

		if (setjmp(png_jmpbuf(png_ptr)))
		{
			/* Free all of the memory associated with the png_ptr and info_ptr */
			png_destroy_write_struct(&png_ptr, &info_ptr);
			/* If we get here, we had a problem writing the file */
			Error_static("CImageIO_PNG::Write", "Failed to setup png writing" );
		}
		CCFile File;

		File.Open(filename, CFILE_WRITE | CFILE_BINARY);

		png_set_write_fn( png_ptr, (png_voidp)&File, PngWrite, PngFlush );

		//------------------------------------------------------------------------------------
		int nBitDepth = 8;
		int nColorType = 0;
		int nFlags = 0;
		CImageFormat2IHDRFormat( img, nFlags, nBitDepth, nColorType );
		png_set_IHDR( png_ptr, info_ptr, img->GetWidth(), img->GetHeight(), nBitDepth, nColorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE );
#ifdef PLATFORM_CONSOLE
		png_set_compression_level(png_ptr, Z_BEST_SPEED);
		png_set_compression_strategy(png_ptr, Z_HUFFMAN_ONLY);
#endif

		// Should do palette thingy here

		if( nFlags & FLIP_RGB_COMPONENTS )
			png_set_bgr( png_ptr );

		png_write_info(png_ptr, info_ptr);
		
		png_bytep pImagePtr = (png_bytep)img->Lock();
		uint32 height = img->GetHeight();
		TThinArray<png_bytep> lRowPointers;
		lRowPointers.SetLen(height);
		for( uint32 i = 0; i < height; i++ )
			lRowPointers[i]	= pImagePtr + i * img->GetModulo();

		png_write_image( png_ptr, lRowPointers.GetBasePtr() );
		img->Unlock();

		png_write_end(png_ptr, info_ptr);

		//------------------------------------------------------------------------------------

		png_destroy_write_struct(&png_ptr, &info_ptr);

		File.Close();

	}
	void ReadPalette(const CStr& filename, CImagePalette* destpal)
	{
	}

};

#endif	// IMAGE_IO_PNG


// -------------------------------------------------------------------
//  CImageIO
// -------------------------------------------------------------------
TPtr<CImageIO> CImageIO::GetImageIO(const CStr& filename)
{
	MAUTOSTRIP(CImageIO_GetImageIO, NULL);
	TPtr<CImageIO> spIO;

	CStr Ext = filename.GetFilenameExtenstion().UpperCase();
#ifndef IMAGE_IO_NOGIF
	if (Ext == CStr("GIF")) spIO = MNew(CImageIO_GIF);
	else
#endif
#ifndef IMAGE_IO_NOTGA
	if (Ext == CStr("TGA")) spIO = MNew(CImageIO_TGA);
	else
#endif
#ifndef IMAGE_IO_NOPCX
	if (Ext == CStr("PCX")) spIO = MNew(CImageIO_PCX);
	else
#endif
#ifndef IMAGE_IO_NOJPG
	if (Ext == CStr("JPG")) spIO = MNew(CImageIO_JPG);
	else
#endif
#ifdef IMAGE_IO_PNG
	if (Ext == CStr("PNG")) spIO = MNew(CImageIO_PNG);
	else
#endif
		Error_static("ImageIO::GetImageIO", "Unsupported file format: "+filename);

	if (!spIO) Error_static("ImageIO::GetImageIO", "Out of memory.");
	return spIO;
};

TPtr<CImageIO> CImageIO::CreateImageIO(const CStr& _Type)
{
	MAUTOSTRIP(CImageIO_CreateImageIO, NULL);
	TPtr<CImageIO> spIO;

#ifndef IMAGE_IO_NOGIF
	if (_Type == CStr("GIF")) spIO = MNew(CImageIO_GIF);
	else
#endif
#ifndef IMAGE_IO_NOTGA
	if (_Type == CStr("TGA")) spIO = MNew(CImageIO_TGA);
	else
#endif
#ifndef IMAGE_IO_NOPCX
	if (_Type == CStr("PCX")) spIO = MNew(CImageIO_PCX);
	else
#endif
#ifndef IMAGE_IO_NOJPG
	if (_Type == CStr("JPG")) spIO = MNew(CImageIO_JPG);
	else
#endif
#ifdef IMAGE_IO_PNG
	if (_Type == CStr("PNG")) spIO = MNew(CImageIO_PNG);
	else
#endif
		Error_static("ImageIO::GetImageIO", "Unsupported file format: "+_Type);

	if (!spIO) Error_static("ImageIO::GetImageIO", "Out of memory.");
	return spIO;
};

