


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Compile time string hash generation
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define MSH_CLWR(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) + 'a' - 'A') : ((c) >= 0xc0 && (c) <= 0xde) ? ((c) + 0xe0 - 0xc0) : (c))
#define MSH_CHAR0(c0) MSH_CLWR((c0) & 0xff)
#define MSH_CHAR1(c0) MSH_CLWR(((c0) >> 8) & 0xff)
#define MSH_CHAR2(c0) MSH_CLWR(((c0) >> 16) & 0xff)
#define MSH_CHAR3(c0) MSH_CLWR(((c0) >> 24) & 0xff)
#define MSH_C33 33
#define MSH_MUL(a,b) uint32(uint64(a) * uint64(b))

#define MSH_4(Hash, c0) (MSH_MUL(MSH_MUL(MSH_MUL(MSH_MUL(Hash, MSH_C33) + MSH_CHAR3(c0), MSH_C33) + MSH_CHAR2(c0), MSH_C33) + MSH_CHAR1(c0), MSH_C33) + MSH_CHAR0(c0))
#define MSH_3(Hash, c0) (MSH_MUL(MSH_MUL(MSH_MUL(Hash, MSH_C33) + MSH_CHAR2(c0), MSH_C33) + MSH_CHAR1(c0), MSH_C33) + MSH_CHAR0(c0))
#define MSH_2(Hash, c0) (MSH_MUL(MSH_MUL(Hash, MSH_C33) + MSH_CHAR1(c0), MSH_C33) + MSH_CHAR0(c0))
#define MSH_1(Hash, c0) (MSH_MUL(Hash, MSH_C33) + MSH_CHAR0(c0))
#define MSH_0(Hash) (Hash)

#define MSH(Hash, c0) (MSH_CHAR3(c0) ? MSH_4(Hash,c0) : (MSH_CHAR2(c0) ? MSH_3(Hash,c0) : (MSH_CHAR1(c0) ? MSH_2(Hash,c0) : (MSH_CHAR0(c0) ? MSH_1(Hash,c0) : MSH_0(Hash)))))

template<uint32 Hash, uint32 c0> class TStrHash
{
public:
	enum { HASH = MSH(Hash, c0)-5381 };
};

template<uint32 c0, uint32 c1> class TStrHash2
{
public:
	enum { HASH = TStrHash<TStrHash<5381, c0>::HASH + 5381, c1>::HASH };
};

template<uint32 c0, uint32 c1, uint32 c2> class TStrHash3
{
public:
	enum { HASH = TStrHash<TStrHash2<c0, c1>::HASH + 5381, c2>::HASH };
};

template<uint32 c0, uint32 c1, uint32 c2, uint32 c3> class TStrHash4
{
public:
	enum { HASH = TStrHash<TStrHash3<c0, c1, c2>::HASH + 5381, c3>::HASH };
};

template<uint32 c0, uint32 c1, uint32 c2, uint32 c3, uint32 c4> class TStrHash5
{
public:
	enum { HASH = TStrHash<TStrHash4<c0, c1, c2, c3>::HASH + 5381, c4>::HASH };
};

template<uint32 c0, uint32 c1, uint32 c2, uint32 c3, uint32 c4, uint32 c5> class TStrHash6
{
public:
	enum { HASH = TStrHash<TStrHash5<c0, c1, c2, c3, c4>::HASH + 5381, c5>::HASH };
};

template<uint32 c0, uint32 c1, uint32 c2, uint32 c3, uint32 c4, uint32 c5, uint32 c6> class TStrHash7
{
public:
	enum { HASH = TStrHash<TStrHash6<c0, c1, c2, c3, c4, c5>::HASH + 5381, c6>::HASH };
};

template<uint32 c0, uint32 c1, uint32 c2, uint32 c3, uint32 c4, uint32 c5, uint32 c6, uint32 c7> class TStrHash8
{
public:
	enum { HASH = TStrHash<TStrHash7<c0, c1, c2, c3, c4, c5, c6>::HASH + 5381, c7>::HASH };
};

#undef MSH_CLWR
#undef MSH_CHAR0
#undef MSH_CHAR1
#undef MSH_CHAR2
#undef MSH_CHAR3
#undef MSH_4
#undef MSH_3
#undef MSH_2
#undef MSH_1
#undef MSH_0
#undef MSH

// -------------------------------------------------------------------
// Use the macros below (not anything of the above), Ex:
//
//		MHASH2(MODE,L)
//		MHASH2(ORI,GIN)
//		MHASH4(CLIE,NT,FLA,GS)
//
// -------------------------------------------------------------------
#define MHASH1(c0) TStrHash<5381,c0>::HASH
#define MHASH2(c0,c1) TStrHash2<c0,c1>::HASH
#define MHASH3(c0,c1,c2) TStrHash3<c0,c1,c2>::HASH
#define MHASH4(c0,c1,c2,c3) TStrHash4<c0,c1,c2,c3>::HASH
#define MHASH5(c0,c1,c2,c3,c4) TStrHash5<c0,c1,c2,c3,c4>::HASH
#define MHASH6(c0,c1,c2,c3,c4,c5) TStrHash6<c0,c1,c2,c3,c4,c5>::HASH
#define MHASH7(c0,c1,c2,c3,c4,c5,c6) TStrHash7<c0,c1,c2,c3,c4,c5,c6>::HASH
#define MHASH8(c0,c1,c2,c3,c4,c5,c6,c7) TStrHash8<c0,c1,c2,c3,c4,c5,c6,c7>::HASH


