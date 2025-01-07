
#include "../Platform/Platform.h"

#ifdef PLATFORM_PS3

#define MOSMain_ShowError(Err) printf( "%s\n", (char*)Err)


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| PS3_Main
|__________________________________________________________________________________________________
\*************************************************************************************************/
//#include "MSystem/MSystem_PS3.h"



void InputContextPS3_InitLibs();
void InputContextPS3_ShutdownLibs();

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
const char* g_pModuleName = NULL;
#endif

int PS3_MainThread(int _argc, char** _argv, const char* _pAppClassName)
{
	InputContextPS3_InitLibs();
	MRTC_GetObjectManager()->SetDllLoading(false);

//	MRTC_CClassRegistry* pCR = MRTC_GetObjectManager()->GetClassRegistry();
//	MRTC_ClassContainerLink* pCCL = (MRTC_ClassContainerLink*) *(((uint32*)pCR) + 2);
//	while(pCCL)
//	{
//		pCCL->m_pClassContainer->Dump_r(pCCL->m_pClassContainer->m_pClassRoot, 0);
//		pCCL = pCCL->m_pNext;
//	}

	MRTC_ObjectManager* pObjMgr = MRTC_GetObjectManager();

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	{
		g_pModuleName = _argv[0];
		MRTC_GetRD()->ModuleFinish();
		MRTC_GetRD()->Create(15888);
	}

#endif

	// -------------------------------------------------------------------
	// Create exception-log
	M_TRY
	{
		spCReferenceCount spObj = pObjMgr->CreateObject("CCExceptionLog", "SYSTEM.EXCEPTIONLOG");
		if (spObj != NULL)
		{
			MACRO_GetRegisterObject(CCExceptionLog, pLog, "SYSTEM.EXCEPTIONLOG");
			if (!pLog) Error_static("PS3_Main", "Unable to create exception-log object.");
		}
	}
	M_CATCH(
	catch(CCException _Ex)
	{
		MOSMain_ShowError(_Ex.GetExceptionInfo().GetString());
		exit(0);
	}
	);

	// -------------------------------------------------------------------
	// Create logfile object
	M_TRY
	{
		spCReferenceCount spObj = pObjMgr->CreateObject("CLogFile", "SYSTEM.LOG");
		if (spObj != NULL)
		{
			ILogFile* pLog = TDynamicCast<ILogFile>((CReferenceCount*)spObj);
			if (!pLog) Error_static("PS3_Main", "Unable to create logfile object.");
		}
	}
	M_CATCH(
	catch(CCException _Ex)
	{
		MOSMain_ShowError(_Ex.GetExceptionInfo().GetString());
		exit(0);
	}
	);

	M_TRY
	{
		// -------------------------------------------------------------------
		M_TRY
		{
			// -------------------------------------------------------------------
			//  Create CSystem
			spCReferenceCount spObj;
			
			spObj = (CReferenceCount*)pObjMgr->CreateObject("CSystemPS3");

			if (!spObj) Error_static("PS3_Main", "Unable to instance CSystemPS3 object.");

			TPtr<CSystem> spSys = safe_cast<CSystem>((CReferenceCount*)spObj);
			if (!spSys) Error_static("PS3_Main", "CSystemPS3 object was not a class of CSystem.");
			spObj = NULL;

			MRTC_GOM()->RegisterObject((CReferenceCount*)spSys, "SYSTEM");		
			M_TRY
			{
				spSys->Create(NULL, NULL, NULL, NULL, _pAppClassName);
				spSys->DoModal();
			}
			M_CATCH(
			catch(CCException)
			{
				spSys->Destroy();
				MRTC_GOM()->UnregisterObject((CReferenceCount*)spSys, "SYSTEM");
				throw;
			}
			);
			spSys->Destroy();
			MRTC_GOM()->UnregisterObject((CReferenceCount*)spSys, "SYSTEM");
		}
		M_CATCH(
		catch(CCException _Ex)
		{
			MACRO_GetRegisterObject(CCExceptionLog, pLog, "SYSTEM.EXCEPTIONLOG");
			if (pLog) 
			{
				pLog->DisplayFatal();
			}
			else
				MOSMain_ShowError(_Ex.GetExceptionInfo().GetString());
		}
		);

//		MRTC_GetObjectManager()->UnregisterAll();
	}
	M_CATCH(
	catch(CCException _Ex)
	{
		MOSMain_ShowError(_Ex.GetExceptionInfo().GetString());
	}
	);

	MRTC_GOM()->UnregisterObject(NULL, "SYSTEM.LOG");
	MRTC_GOM()->UnregisterObject(NULL, "SYSTEM.EXCEPTIONLOG");

	InputContextPS3_ShutdownLibs();
	// Should never be executed.
	M_ASSERT(0, "End of program.");

	return 0;
}

int PS3_Main(int argc, char**argv, const char* _pAppClassName)
{
	return PS3_MainThread(argc, argv, _pAppClassName);
}

#endif
