#include "PCH.h"
#include "MMd5.h"
const aint CMD5::msc_TransformOrder[] = 
{
	0, 3, 2, 1
};

const aint CMD5::msc_Rotate0[] = 
{
	7, 12, 17, 22
};

const aint CMD5::msc_Rotate1[] = 
{
	5, 9, 14, 20
};

const aint CMD5::msc_Rotate2[] = 
{
	4, 11, 16, 23
};

const aint CMD5::msc_Rotate3[] = 
{
	6, 10, 15, 21
};

const uint32 CMD5::msc_Add0[] = 
{
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821
};

const uint32 CMD5::msc_Add1[] = 
{
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a
};

const uint32 CMD5::msc_Add2[] = 
{
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665
};

const uint32 CMD5::msc_Add3[] = 
{
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1, 
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

const aint CMD5::msc_DataOrder1[] = 
{
	1, 6, 11, 0, 5, 10, 15, 4,
	9, 14, 3, 8, 13, 2, 7, 12,
};

const aint CMD5::msc_DataOrder2[] = 
{
	5, 8, 11, 14, 1, 4, 7, 10,
	13, 0, 3, 6, 9, 12, 15, 2,
};

const aint CMD5::msc_DataOrder3[] = 
{
	0, 7, 14, 5, 12, 3, 10, 1,
	8, 15, 6, 13, 4, 11, 2, 9,
};

CMD5Digest GetFileMD5(CStr _File)
{
	CCFile File;
	File.Open(_File, CFILE_READ|CFILE_BINARY);
 
	int Len = File.Length();

	CMD5 Md5;

	TThinArray<uint8> Temp;
	Temp.SetLen(32768);

	while (Len)
	{
		int ToGet = MinMT(Len, Temp.Len());
		File.Read(Temp.GetBasePtr(), ToGet);
		Len -= ToGet;
		Md5.f_AddData(Temp.GetBasePtr(), ToGet);
	}

	return CMD5Digest(Md5);
}

void CMD5::fv_AddData(const void* _pData, mint _Len)
{
	f_AddData(_pData, _Len);
}
void CMD5::fv_GetDigest(uint8 * _pDest) const
{
	f_GetDigest(_pDest);
}
