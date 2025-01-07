
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/

#ifndef __INC_MDA_REFERENCE
#define __INC_MDA_REFERENCE

#include "mrtc.h"

class MCCDLLEXPORT CDA_Referable
{
	friend class CDA_Reference;

public:
	class MCCDLLEXPORT CDA_Reference
	{
	public:
		M_INLINE CDA_Reference()
		{

			Reference = NULL;
		}

		M_INLINE CDA_Reference(CDA_Referable *Referee)
		{
			Reference = NULL;

			Refer(Referee);
		}

		M_INLINE ~CDA_Reference()
		{

			if (Reference)
				Refer(NULL);

		}

		void M_INLINE Refer(CDA_Referable *Referee)
		{

			if (Reference)
			{
				if (Next)
					Next->Prev = Prev;

				if (Prev)
					Prev->Next = Next;
				else
					Reference->DA_Referable_First = Next;
			}

			Reference = Referee;

			if (Reference)
			{
				Prev = NULL;

				if (Reference->DA_Referable_First)
					Reference->DA_Referable_First->Prev = this;

				Next = Reference->DA_Referable_First;
				Reference->DA_Referable_First = this;
			}
		}

		void MoveToLoc(CDA_Reference *NewLoc);

	public:
		CDA_Referable *Reference;
		CDA_Reference *Prev;
		CDA_Reference *Next;
	};

	CDA_Referable()
	{
		DA_Referable_First = NULL;
	}

	~CDA_Referable()
	{

		while (DA_Referable_First)
		{
			DA_Referable_First->Reference = NULL;
			DA_Referable_First = DA_Referable_First->Next;
		}
		
	}

	void RemoveFromAllRefs()
	{
		while (DA_Referable_First)
		{
			DA_Referable_First->Reference = NULL;
			DA_Referable_First = DA_Referable_First->Next;
		}
	}


	void MoveToLoc(CDA_Referable *NewLoc)
	{

		*NewLoc = *this;

		CDA_Reference *Current = NewLoc->DA_Referable_First;

		while (Current)
		{
			Current->Reference = this;

			Current = Current->Next;
		}
	}

	CDA_Reference *DA_Referable_First;
};


class MCCDLLEXPORT CDA_AutoDeleteReference : public CDA_Referable::CDA_Reference
{
public:
	virtual ~CDA_AutoDeleteReference()
	{
		if (Reference)
			delete Reference;
	}

};

template <class MemberType> class TCDA_ReferenceType : public CDA_Referable::CDA_Reference
{
public:

	M_INLINE operator MemberType* () 
	{
		return (MemberType *)Reference;
	};

	M_INLINE operator const MemberType* () const
	{
		return (MemberType *)Reference;
	};

	M_INLINE MemberType& operator* () 
	{ 
		return *((MemberType *)Reference);
	};

	M_INLINE MemberType* operator-> () 
	{ 
		return (MemberType *)Reference; 
	};

	M_INLINE const MemberType* operator-> () const 
	{ 
		return (MemberType *)Reference; 
	};

	M_INLINE bool operator! () const
	{
		return (Reference == NULL);
	}

	M_INLINE bool operator == (MemberType* _p) const { return ((MemberType *)Reference == _p); };
	M_INLINE bool operator != (MemberType* _p) const { return ((MemberType *)Reference != _p); };
	M_INLINE bool operator == (const TCDA_ReferenceType& _sp) const { return (Reference == _sp.Reference); };
	M_INLINE bool operator != (const TCDA_ReferenceType& _sp) const { return (Reference != _sp.Reference); };


	M_INLINE TCDA_ReferenceType& operator= (MemberType* _p) 
	{
		Refer(_p);
		return *this;
	};

	M_INLINE TCDA_ReferenceType& operator= (const TCDA_ReferenceType& _p)
	{
		Refer(_p.Reference);
		return *this;
	};

};

#endif //__INC_MDA_REFERENCE
