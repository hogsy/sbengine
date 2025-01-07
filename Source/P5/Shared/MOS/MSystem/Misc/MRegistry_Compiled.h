
#ifndef _INC_MREGISTRY_COMPILED
#define _INC_MREGISTRY_COMPILED
#define M_ENABLE_REGISTRYCOMPILED

#ifdef M_ENABLE_REGISTRYCOMPILED

#include "MRegistry.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			CRegistry
					
	Author:			Magnus Högdahl
					
	Copyright:		Starbreeze Studios 2002

 	Creation Date:	2002-04-11

	Contents:		CRegistry
					
	Comments:
					
\*____________________________________________________________________________________________*/


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRegistryCompiled
|__________________________________________________________________________________________________
\*************************************************************************************************/


class CRegistryCompiledInternal;

class SYSTEMDLLEXPORT CRegistryCompiled : public CReferenceCount
{
	MRTC_DECLARE;

	CRegistryCompiledInternal *m_pInternal;

public:

	CRegistryCompiled();
	~CRegistryCompiled();

#ifndef PLATFORM_CONSOLE
	void Write(CDataFile* _pDFile);
	void Write_XCR(const char* _pFileName);
#endif
	void Compile(CRegistry* _pReg, bint _FastSearch);

	spCRegistry GetRoot();
	void Read(CDataFile* _pDFile);
	void Read_XCR(const char* _pFileName);
};

#endif
#endif // _INC_MREGISTRY_COMPILED

