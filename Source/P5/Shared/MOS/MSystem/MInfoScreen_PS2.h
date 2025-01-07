/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Crash infoscreen for PS2
					
	Author:			Jim Kjellin
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		030506:		Added comments
\*_____________________________________________________________________________________________*/

#if !defined __INC_MINFOSCREEN_PS2 && defined PLATFORM_PS2
#define __INC_MINFOSCREEN_PS2


class CInfoScreenPS2
{
	static CInfoScreenPS2		*ms_pSingleton;
	class CDisplayContextPS2	*m_pDC;
	int					 		 m_cd;

	CInfoScreenPS2();
protected:
	void StackTrace( char *_pszOutput, uint32 *_pc, uint32 *_ra, uint32 *_sp );
public:
	static CInfoScreenPS2 &Get();
	void SetDisplayContext( class CDisplayContextPS2 *_pDC );
	void OutputText(const char *_pText, uint32 *_pc = NULL, uint32 *_ra = NULL, uint32 *_sp = NULL );
};


#endif	// __INC_MINFOSCREEN_PS2
