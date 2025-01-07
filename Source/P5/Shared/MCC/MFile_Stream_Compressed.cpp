
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#include "PCH.h"

#include "MCC.h"

#include "MFile_Stream_Compressed.h"

// -------------------------------------------------------------------
//  CStream_Compressed
// -------------------------------------------------------------------
//
// CLASS AUTHOR:	Daniel Hansson
// STARTED	   :	970311
// LAST UPDATED:	970315
// COMMENTS	   :	A "fake" stream that operates thru one of the
//					standard streams to handle compressed files as
//					if they we're normal files.
//

CStream_Compressed::CStream_Compressed()
{
	EOS=TRUE;
	Buffer=NULL;
	Target = NULL;
}

CStream_Compressed::~CStream_Compressed()
{
	if (Target)
		Close();
	
	if (Buffer!=NULL) delete[] Buffer;
}

void CStream_Compressed::Open(CStream* _Target, CCompress* _Compressor, int _mode)
{
	Target = _Target;
	Compressor = _Compressor;
	m_Mode = _mode;
	
	StartPos=Target->Pos();
	
	// Skip the header of a compressed file.
	Target->RelSeek(9);
	
	// Read mode or write mode?
	if ((_mode & CFILE_READ)!=0) {
		Target->Read(&Uncompressed_FileLength,4);
		if (Uncompressed_FileLength>0)
			EOS=FALSE;
		else
			EOS=TRUE;
		Buffer=DNew(uint8) uint8[COMPRESSION_BUFFER_SIZE+Compressor->GetDecompDestExtra()];
	}
	else {
		uint32 Dummy;
		Target->Write(&Dummy,4);
		Uncompressed_FileLength=0;
		EOS=TRUE;
		FileLength=0;
		Buffer=DNew(uint8) uint8[COMPRESSION_BUFFER_SIZE+Compressor->GetCompSrcExtra()];
	}
	
	if (Buffer==NULL) MemError("Open");
	
	BufferPos=0;
	BlockNr=0;
	BufferNeedsUpdate=TRUE;
}

void CStream_Compressed::Close()
{
	// In write mode?
	if ((m_Mode & CFILE_WRITE)!=0) {
		
		if (BufferPos>0) {
			// Compress the last block and save it to disk.
			void* Dst=Compressor->Compress(Buffer,NULL,BufferPos);
			uint32 CompressedLength=Compressor->GetCompressedLength();
			FileLength+=CompressedLength+4;
			Target->Write(&CompressedLength,4);
			Target->Write(Dst,CompressedLength);
			MRTC_MemFree(Dst);
		}
		
		// Write file length (Used to identify compressed files).
		// Also write total uncompressed length.
		FileLength+=4*3+1;	// Add the size of the following header.
		Target->Seek(4+StartPos);	// Skip "COMP".
		Target->Write(&FileLength,4);
		Target->RelSeek(1);	// Skip compression type byte.
		Target->Write(&Uncompressed_FileLength,4);
	}
	
	Target->Close();
	delete Target;
	delete Compressor;
	
	delete[] Buffer;
	Buffer=NULL;
	Target = NULL;
}

void CStream_Compressed::UpdateBuffer()
{
	BufferNeedsUpdate=FALSE;
	
	Target->Seek(4*3+1+StartPos);	// Seek past header.
	
	// Skip the blocks preceding the block we want.
	uint32 BlockSize;
	for (int i=0; i<BlockNr; i++) {
		Target->Read(&BlockSize,4);
		Target->RelSeek(BlockSize);
	}
	
	// Read and decompress the block.
	Target->Read(&BlockSize,4);
	uint8* CompressedBuffer=DNew(uint8) uint8[BlockSize+Compressor->GetDecompSrcExtra()];
	if (CompressedBuffer==NULL) MemError("UpdateBuffer");
	
	Target->Read(CompressedBuffer,BlockSize);
	
	Compressor->Decompress(CompressedBuffer,Buffer);
	
	delete[] CompressedBuffer;
}

void CStream_Compressed::Read(void* dest, mint size)
{
	if ((m_Mode & CFILE_WRITE)!=0)
		Error("Read","Can't read in a compressed stream when in write mode.");
	
	if (!EOS) {
		// Update buffer?
		if (BufferNeedsUpdate)
			UpdateBuffer();
		
		uint8* Dst=(uint8*)dest;
		uint32 Left=size;
		uint32 len;
		while (Left>0) {
			len=Min(Left,COMPRESSION_BUFFER_SIZE-BufferPos);
			memcpy(Dst,Buffer+BufferPos,len);
			Dst+=len;
			Left-=len;
			BufferPos+=len;
			
			// Read past end of file?
			if (Pos()>=(int32)Uncompressed_FileLength) {
				EOS=TRUE;
				break;
			}
			
			if (BufferPos==COMPRESSION_BUFFER_SIZE) {
				BufferPos=0;
				BlockNr++;
				
				UpdateBuffer();
			}
		}
	}
	
}

void CStream_Compressed::Write(const void* src, mint size)
{
	if ((m_Mode & CFILE_READ)!=0)
		Error("Write","Can't write in a compressed stream when in read mode.");
	
	Uncompressed_FileLength+=size;
	
	uint8* Src=(uint8*)src;
	uint32 Left=size;
	uint32 len;
	while (Left>0) {
		len=Min(Left,COMPRESSION_BUFFER_SIZE-BufferPos);
		memcpy(Buffer+BufferPos,Src,len);
		Src+=len;
		Left-=len;
		BufferPos+=len;
		if (BufferPos==COMPRESSION_BUFFER_SIZE) {
			// Compress this block and save it to disk.
			void* Dst=Compressor->Compress(Buffer,NULL,COMPRESSION_BUFFER_SIZE);
			uint32 CompressedLength=Compressor->GetCompressedLength();
			FileLength+=CompressedLength+4;
			Target->Write(&CompressedLength,4);
			Target->Write(Dst,CompressedLength);
			MRTC_MemFree(Dst);
			BufferPos=0;
			BlockNr++;
		}
	}
	
}

void CStream_Compressed::Seek(fint pos)
{
	if ((m_Mode & CFILE_WRITE)!=0)
		Error("Seek","Can't seek in a compressed stream when in write mode.");
	
	if (pos<0 || pos>(int32)Uncompressed_FileLength)
		Error("Seek","Attempted to seek outside the file.");
	
	fint OldBN=BlockNr;
	
	BlockNr=pos % COMPRESSION_BUFFER_SIZE;
	BufferPos=pos-(BlockNr*COMPRESSION_BUFFER_SIZE);
	
	if (BlockNr!=OldBN)
		BufferNeedsUpdate=TRUE;
	
	if (pos<(fint)Uncompressed_FileLength)
		EOS=FALSE;
	else
		EOS=TRUE;
}

void CStream_Compressed::SeekToEnd()
{
	if ((m_Mode & CFILE_WRITE)!=0)
		Error("SeekToEnd","Can't seek in a compressed stream when in write mode.");
	
	BlockNr=Uncompressed_FileLength % COMPRESSION_BUFFER_SIZE;
	BufferPos=Uncompressed_FileLength-(BlockNr*COMPRESSION_BUFFER_SIZE);
	EOS=TRUE;
}

void CStream_Compressed::RelSeek(fint pos)
{
	if ((m_Mode & CFILE_WRITE)!=0)
		Error("RelSeek","Can't seek in a compressed stream when in write mode.");
	
	fint wantedpos=Pos()+pos;
	
	if (wantedpos<0 || wantedpos>(fint)Uncompressed_FileLength)
		Error("RelSeek","Attempted to seek outside the file.");
	
	fint OldBN=BlockNr;
	
	BlockNr=wantedpos % COMPRESSION_BUFFER_SIZE;
	BufferPos=wantedpos-(BlockNr*COMPRESSION_BUFFER_SIZE);
	
	if (BlockNr!=OldBN)
		BufferNeedsUpdate=TRUE;
	
	if (wantedpos<(fint)Uncompressed_FileLength)
		EOS=FALSE;
	else
		EOS=TRUE;
}

fint CStream_Compressed::Pos()
{
	return (BlockNr*COMPRESSION_BUFFER_SIZE)+BufferPos;
}

fint CStream_Compressed::Length()
{
	return Uncompressed_FileLength;
}

bool CStream_Compressed::EndOfFile() 
{
	return EOS;
}
