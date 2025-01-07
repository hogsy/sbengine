#include "PCH.h"
#include "MImage.h"
#include "MImageIO.h"

void CImage::Compress_JPG(fp32 _Quality, CImage* _pDestImg)
{
#ifdef IMAGE_IO_NOJPG
	Error("Compress_JPG", "JPG support disabled in this build.");
#else
	if (!CanCompress(IMAGE_MEM_COMPRESSTYPE_JPG))
		Error("Compress", "Image format not supported by JPG compressed format.");

//_Quality = 0.6f;

//CImageIO_JPG IO;
//IO.Write(GetRect(), "S:\\Test.jpg", this);

	TArray<uint8> lFile;
	CCFile File;
	File.Open(lFile, CFILE_WRITE | CFILE_BINARY);

	uint32 RGBOffset = 8;
	uint32 AlphaOffset = 0;
	File.WriteLE(RGBOffset);
	File.WriteLE(AlphaOffset);

	CImageIO_JPG::WriteJPG(&File, this, false, _Quality);

/*CCFile F;
F.Open("S:\\Test2.jpg", CFILE_WRITE | CFILE_BINARY);
F.Write(&lFile[8], File.Length() - 8);
F.Close();*/


	if (GetFormat() & IMAGE_FORMAT_ALPHA)
	{
//Error("Uncompress", "Should not be here.");
		uint32 FPos = File.Pos();
		AlphaOffset = File.Pos();
		File.Seek(4);
		File.WriteLE(AlphaOffset);
		File.Seek(FPos);
		CImageIO_JPG::WriteJPG(&File, this, true, _Quality);
	}

//LogFile(CStrF("Compress: RGBOffset %d, AlphaOffset %d, FileLen %d, ListLen %d", RGBOffset, AlphaOffset, File.Length(), lFile.Len() ));

	_pDestImg->Destroy();
	_pDestImg->m_pBitmap = DNew(uint8) uint8[File.Length()];
	if (!_pDestImg->m_pBitmap) MemError("Compress");
	_pDestImg->m_AllocSize = File.Length();
	_pDestImg->m_Width = GetWidth();
	_pDestImg->m_Height = GetHeight();
	_pDestImg->m_Format = GetFormat();
	_pDestImg->m_Memmodel = GetMemModel() | IMAGE_MEM_COMPRESSED | IMAGE_MEM_COMPRESSTYPE_JPG;
	_pDestImg->m_Memmodel &= ~IMAGE_MEM_LOCKABLE;
	_pDestImg->UpdateFormat();
	memcpy(_pDestImg->m_pBitmap, lFile.GetBasePtr(), _pDestImg->m_AllocSize);
#endif
}

void CImage::Decompress_JPG(CImage* _pDestImg)
{
#ifdef IMAGE_IO_NOJPG
	Error("Decompress_JPG", "JPG support disabled in this build.");
#else
	CStream_Memory Stream((uint8*)m_pBitmap, m_AllocSize, m_AllocSize);
	CCFile File;
	File.Open(&Stream, CFILE_READ | CFILE_BINARY);

	uint32 RGBOffset;
	uint32 AlphaOffset;
	File.ReadLE(RGBOffset);
	File.ReadLE(AlphaOffset);
//LogFile(CStrF("Decompress: RGBOffset %d, AlphaOffset %d", RGBOffset,AlphaOffset));

	File.Seek(RGBOffset);
	CImageIO_JPG::ReadJPG(&File, _pDestImg, false, IMAGE_MEM_IMAGE, m_Format);
	if (AlphaOffset)
	{
//		Error("Uncompress", "Should not be here.");
		File.Seek(AlphaOffset);
		CImageIO_JPG::ReadJPG(&File, _pDestImg, true, IMAGE_MEM_IMAGE, m_Format);
	}
#endif
}

