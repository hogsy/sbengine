#include "PCH.h"
#include "WEvilTemplate.h"

void Entity_Transform_r(CRegistry* _pObj, const CMat4Dfp32& _Mat)
{
	MAUTOSTRIP(Entity_Transform_r, MAUTOSTRIP_VOID);
	CMat4Dfp32 Pos = Entity_GetPosition(_pObj);
	CMat4Dfp32 NewPos;
	Pos.Multiply(_Mat, NewPos);
	Entity_SetPosition(_pObj, NewPos);

	for(int i = 0; i < _pObj->GetNumChildren(); i++)
		if (_pObj->GetChild(i)->Find("CLASSNAME"))
			Entity_Transform_r(_pObj->GetChild(i), _Mat);
}


CMat4Dfp32 Entity_GetPosition(const CRegistry* _pObj)
{
	MAUTOSTRIP(Entity_GetPosition, CMat4Dfp32());
	// Extracts position and orientation matrix from keys.

	CMat4Dfp32 Mat;
	Mat.Unit();

	const CRegistry* pAngle = _pObj->FindChild("ANGLE");
	const CRegistry* pAngles = _pObj->FindChild("ANGLES");
	if (!pAngles) pAngles = _pObj->FindChild("ROTATION");

	if (pAngle)
	{ 
		Mat.SetZRotation3x3(pAngle->GetThisValuef() / 360.0f);
	}
	else if (pAngles)
	{
		CVec3Dfp32 v; 
		v.ParseString(pAngles->GetThisValue());
		v *= 1.0f/360.0f;
		v.CreateMatrixFromAngles(0, Mat);
	}

	const CRegistry* pOrigin = _pObj->FindChild("ORIGIN");
	if (pOrigin) 
	{
		pOrigin->GetThisValueaf(3, CVec3Dfp32::GetMatrixRow(Mat, 3).k);
//		CVec3Dfp32::GetMatrixRow(Mat, 3).ParseString(pOrigin->GetThisValue());
	}
	return Mat;
}

void Entity_SetPosition(CRegistry* _pObj, const CMat4Dfp32& _Mat)
{
	MAUTOSTRIP(Entity_SetPosition, MAUTOSTRIP_VOID);
	// Sets position and orientation in keys.

//	_pObj->DeleteKey("ORIGIN");
	_pObj->DeleteKey("ANGLE");
	_pObj->DeleteKey("ANGLES");
//	_pObj->DeleteKey("ROTATION");

	_pObj->SetValueaf("ORIGIN", 3, CVec3Dfp32::GetMatrixRow(_Mat, 3).k);
	CVec3Dfp32 Rot;
	Rot.CreateAnglesFromMatrix(0, _Mat);
	Rot = Rot * 360;
	_pObj->SetValue("ROTATION", Rot.GetFilteredString().Str() );
}

void Entity_EvalTemplate_r(const CRegistry* _pTemplates, const CRegistry* _pObj, TArray<spCRegistry>& _spRetObj)
{
	MAUTOSTRIP(Entity_EvalTemplate_r, MAUTOSTRIP_VOID);
	spCRegistry spObj;
	if (_pTemplates)
		spObj = _pTemplates->EvalTemplate_r(_pObj);
	else
		spObj = _pObj->Duplicate();

	if (spObj!=NULL)
	{
		CMat4Dfp32 Pos = Entity_GetPosition(spObj);
		{
			int nKeys = spObj->GetNumChildren();
			{
				for(int iKey = 0; iKey < nKeys; iKey++)
				{
					const CRegistry* pKey = spObj->GetChild(iKey);
					if (pKey->FindChild("CLASSNAME"))
					{
						spCRegistry spChild = pKey->Duplicate();
						CMat4Dfp32 ChildPos = Entity_GetPosition(spChild);
						CMat4Dfp32 Mat;
						ChildPos.Multiply(Pos, Mat);
						Entity_SetPosition(spChild, Mat);
						Entity_EvalTemplate_r(_pTemplates, spChild, _spRetObj);
					}
				}
			}

			// Delete all child-objects
			{
				for(int iKey = nKeys-1; iKey >= 0; iKey--)
				{
					const CRegistry* pKey = spObj->GetChild(iKey);
					if (pKey->GetThisName().CompareSubStr("OBJECT") == 0 && pKey->FindChild("CLASSNAME"))
					{
						_spRetObj.Add(spObj->GetChild(iKey));
						spObj->DeleteKey(iKey);
					}
				}
			}
		}
	}

	_spRetObj.Add(spObj);
}


