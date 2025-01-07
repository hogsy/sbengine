#ifndef __WEVILTEMPLATE_H
#define __WEVILTEMPLATE_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Misc helper functions to evaluate templates

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		Entity_Transform_r
					Entity_GetPosition
					Entity_SetPosition
					Entity_EvalTemplate_r
\*____________________________________________________________________________________________*/

#include "MCC.h"
#include "../../MOS.h"

void Entity_Transform_r(CRegistry* _pObj, const CMat4Dfp32& _Mat);
CMat4Dfp32 Entity_GetPosition(const CRegistry* _pObj);
void Entity_SetPosition(CRegistry* _pObj, const CMat4Dfp32& _Mat);
void Entity_EvalTemplate_r(const CRegistry* _pTemplates, const CRegistry* _pObj, TArray<spCRegistry>& _spRetObj);

#endif // _INC_MEVILTEMPLATE
