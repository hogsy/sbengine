
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/

#ifndef __INC_MDA_STACK
#define __INC_MDA_STACK

#include "MDA_LinkedList.h"

class MCCDLLEXPORT CDA_Stack
{
public:
	CDA_Stack();
	virtual ~CDA_Stack();

	void Push(CDA_Linkable *Object);
	CDA_Linkable *Get();
	void Pop();
	
	void MoveToLoc(CDA_Stack *NewLoc);	

public:
	CDA_LinkedList List;
};

template <class MemberType> class TCDA_Stack : public CDA_Stack
{
public:

#ifdef _DEBUG
	MemberType *DCurrent;
#endif

	M_INLINE operator MemberType* () 
	{
		return (MemberType *)Get();
	};

	M_INLINE operator const MemberType* () const
	{
		return (MemberType *)Get();
	};

	M_INLINE MemberType& operator* () 
	{ 
		return *((MemberType *)Get());
	};

	M_INLINE MemberType* operator-> () 
	{ 
		return (MemberType *)Get(); 
	};

	M_INLINE const MemberType* operator-> () const 
	{ 
		return (MemberType *)Get(); 
	};

	M_INLINE void operator += (MemberType* _p) 
	{
		Push(_p);
#ifdef _DEBUG
		DCurrent = (MemberType *)Get();;
#endif
	};

	M_INLINE void operator -- () 
	{
		Pop();
#ifdef _DEBUG
		DCurrent = (MemberType *)Get();;
#endif
	};


};


class MCCDLLEXPORT CDA_CircularStack
{
public:
	CDA_CircularStack();
	virtual ~CDA_CircularStack();

	void Add(CDA_Linkable *Object);
	void Remove(CDA_Linkable *Object);
	CDA_Linkable *Get();
	void Next();
	void Prev();

	void MoveToLoc(CDA_CircularStack *NewLoc);	

public:
	CDA_LinkedList List;
};

template <class MemberType> class TCDA_CircularStack : public CDA_CircularStack
{
public:

	M_INLINE operator MemberType* () 
	{
		return (MemberType *)Get();
	};

	M_INLINE operator const MemberType* () const
	{
		return (MemberType *)Get();
	};

	M_INLINE MemberType& operator* () 
	{ 
		return *((MemberType *)Get());
	};

	M_INLINE MemberType* operator-> () 
	{ 
		return (MemberType *)Get(); 
	};

	M_INLINE const MemberType* operator-> () const 
	{ 
		return (MemberType *)Get(); 
	};

	M_INLINE void operator += (MemberType* _p) 
	{
		Add(_p);
	};

	M_INLINE void operator -= (MemberType* _p) 
	{
		Remove(_p);
	};

	M_INLINE void operator -- () 
	{
		Prev();
	};
	M_INLINE void operator ++ () 
	{
		Next();
	};


};

#endif
