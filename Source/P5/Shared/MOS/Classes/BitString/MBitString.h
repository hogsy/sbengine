#include "MRTC.h"

int CBS_Compress(const uint8* _pBits, int _Len, uint8* _pOut);
int CBS_Uncompress(const uint8* _pBits, int _Len, uint8* _pOut);

void CBS_ConvertByteString(const uint8* _pBytes, int _Len, uint8* _pOut);	// Convert a string of bytes to a bit-string.
CStr CBS_GetString(const uint8* _pBits, int _Len);

