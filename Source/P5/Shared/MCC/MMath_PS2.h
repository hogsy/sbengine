
#ifndef PLATFORM_PS2

template <>
void CQuatfp4::Multiply(const TQuaternion& _Quat2, TQuaternion& _QDest) const
{
	float _a0 = k[0];
	float _a1 = k[1];
	float _a2 = k[2];
	float _a3 = k[3];
	float _b0 = _Quat2.k[0];
	float _b1 = _Quat2.k[1];
	float _b2 = _Quat2.k[2];
	float _b3 = _Quat2.k[3];
	fp4 ret1, ret2, ret3, ret4;
	asm("
			mula.s	_a3,_b0
			madda.s	_a0,_b3
			madda.s	_a1,_b2
			msub.s	ret1,_a2,_b1

			mula.s	_a3,_b1
			madda.s	_a1,_b3
			madda.s	_a2,_b0
			msub.s	ret2,_a0,_b2

			mula.s	_a3,_b2
			madda.s	_a2,_b3
			madda.s	_a0,_b1
			msub.s	ret3,_a1,_b0

			mula.s	_a3,_b3
			msuba.s	_a0,_b0
			msuba.s	_a1,_b1
			msub.s	ret4,_a2,_b2
		");
	_QDest.k[0]	= ret1;
	_QDest.k[1]	= ret2;
	_QDest.k[2]	= ret3;
	_QDest.k[3]	= ret4;
}

template <>
void CQuatfp4::Multiply(const TQuaternion& _Quat2)
{
	float _a0 = k[0];
	float _a1 = k[1];
	float _a2 = k[2];
	float _a3 = k[3];
	float _b0 = _Quat2.k[0];
	float _b1 = _Quat2.k[1];
	float _b2 = _Quat2.k[2];
	float _b3 = _Quat2.k[3];
	fp4 ret1, ret2, ret3, ret4;
	asm("
			mula.s	_a3,_b0
			madda.s	_a0,_b3
			madda.s	_a1,_b2
			msub.s	ret1,_a2,_b1

			mula.s	_a3,_b1
			madda.s	_a1,_b3
			madda.s	_a2,_b0
			msub.s	ret2,_a0,_b2

			mula.s	_a3,_b2
			madda.s	_a2,_b3
			madda.s	_a0,_b1
			msub.s	ret3,_a1,_b0

			mula.s	_a3,_b3
			msuba.s	_a0,_b0
			msuba.s	_a1,_b1
			msub.s	ret4,_a2,_b2
		");
	k[0]	= ret1;
	k[1]	= ret2;
	k[2]	= ret3;
	k[3]	= ret4;
}

template <>
fp4 CQuatfp4::DotProd(const TQuaternion& _Q) const
{
	fp4 _a0 = k[0];
	fp4 _a1 = k[1];
	fp4 _a2 = k[2];
	fp4 _a3 = k[3];
	fp4 _b0 = _Q.k[0];
	fp4 _b1 = _Q.k[1];
	fp4 _b2 = _Q.k[2];
	fp4 _b3 = _Q.k[3];
	fp4 ret;
	
	asm ("
			mula.s	_a0, _b0
			madda.s	_a1, _b1
			madda.s	_a2, _b2
			madd.s	ret, _a3, _b3
			"
		);
	
	return ret;
}

template <>
void CQuatfp4::Interpolate(const TQuaternion& _Other, TQuaternion& _Dest, fp4 _t) const
{
	fp4 _a0, _b0, _c0, _d0;
	fp4 _a1, _b1, _c1, _d1;
	fp4 reta, retb, retc, retd;		// these will probably share register with _x1 values since those aren't required once these start being used
	fp4 tmp, tmp0 = 0.0f, tmp1 = 1.0f;
	_a0 = k[0];
	_b0 = k[1];
	_c0 = k[2];
	_d0 = k[3];

	_a1 = _Other.k[0];
	_b1 = _Other.k[1];
	_c1 = _Other.k[2];
	_d1 = _Other.k[3];

	asm("
		mula.s	_a0, _a1				// DotProd
		madda.s	_b0, _b1
		madda.s	_c0, _c1
		madd.s	tmp, _d0, _d1
		c.le.s	tmp0, tmp
		bc1t	no_neg
		
		neg.s	_a0, _a0				// Negate if DotProd < 0
		neg.s	_b0, _b0
		neg.s	_c0, _c0
		neg.s	_d0, _d0

no_neg:
		sub.s	_a1, _a1, _a0			// Lerp
		sub.s	_b1, _b1, _b0
		sub.s	_c1, _c1, _c0
		sub.s	_d1, _d1, _d0

		mul.s	reta, _a1, _t
		mul.s	retb, _b1, _t
		mul.s	retc, _c1, _t
		mul.s	retd, _d1, _t

		add.s	reta, reta, _a0
		add.s	retb, retb, _b0
		add.s	retc, retc, _c0
		add.s	retd, retd, _d0

		mula.s	reta, reta				// Normalize
		madda.s	retb, retb
		madda.s	retc, retc
		madd.s	tmp, retd, retd
		rsqrt.s tmp, tmp1, tmp			// 1.0f / M_Sqrt( Sqr( Quat ) )
		mul.s	reta, reta, tmp
		mul.s	retb, retb, tmp
		mul.s	retc, retc, tmp
		mul.s	retd, retd, tmp
		");
	_Dest.k[0]	= reta;
	_Dest.k[1]	= retb;
	_Dest.k[2]	= retc;
	_Dest.k[3]	= retd;
}

extern "C" void CMat4Dfp4_ASM_Multiply3x3(const CMat4Dfp4 *m1, const CMat4Dfp4 *m2, CMat4Dfp4 *m3);
extern "C" void CMat4Dfp4_ASM_Multiply4x4(const CMat4Dfp4 *m1, const CMat4Dfp4 *m2, CMat4Dfp4 *m3);
extern "C" void CVec3Dfp4Normalize(CVec3Dfp4 *pVec);
extern "C" void CVec3Dfp4_ASM_Mul_Mat4D(CVec3Dfp4 *pVec, const CMat4Dfp4 *m1);

// Added by Joacim Jonsson

//template<>
inline void CMat4Dfp4::Multiply3x3(const CMat4Dfp4& m, CMat4Dfp4& DestMat) const
{
	CMat4Dfp4_ASM_Multiply3x3( this, &m, &DestMat );
	DestMat.UnitNot3x3();
}

//template<>
inline void CMat4Dfp4::Multiply(const CMat4Dfp4& m, CMat4Dfp4& DestMat) const
{
//	CMat4Dfp4_ASM_Multiply4x4(this, &m, &DestMat);
	uint128 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5;
	asm("
			mtsab %0, 0
			lq tmp0, 0x00(%0)
			lq tmp1, 0x10(%0)
			lq tmp2, 0x20(%0)
			lq tmp3, 0x30(%0)
			lq tmp4, 0x40(%0)
			qfsrv tmp0, tmp1, tmp0
			qfsrv tmp1, tmp2, tmp1
			qfsrv tmp2, tmp3, tmp2
			qfsrv tmp3, tmp4, tmp3
			qmtc2 tmp0, vf01
			qmtc2 tmp1, vf02
			qmtc2 tmp2, vf03
			qmtc2 tmp3, vf04

			mtsab %1, 0
			lq tmp0, 0x00(%1)
			lq tmp1, 0x10(%1)
			lq tmp2, 0x20(%1)
			lq tmp3, 0x30(%1)
			lq tmp4, 0x40(%1)
			qfsrv tmp0, tmp1, tmp0
			qfsrv tmp1, tmp2, tmp1
			qfsrv tmp2, tmp3, tmp2
			qfsrv tmp3, tmp4, tmp3
			qmtc2 tmp0, vf05
			qmtc2 tmp1, vf06
			qmtc2 tmp2, vf07
			qmtc2 tmp3, vf08

			vmulax.xyzw     ACC, vf05,vf01
			vmadday.xyzw    ACC, vf06,vf01
			vmaddaz.xyzw    ACC, vf07,vf01
			vmaddw.xyzw     vf01, vf08,vf01

			vmulax.xyzw     ACC, vf05,vf02
			vmadday.xyzw    ACC, vf06,vf02
			vmaddaz.xyzw    ACC, vf07,vf02
			vmaddw.xyzw     vf02, vf08,vf02

			vmulax.xyzw     ACC, vf05,vf03
			vmadday.xyzw    ACC, vf06,vf03
			vmaddaz.xyzw    ACC, vf07,vf03
			vmaddw.xyzw     vf03, vf08,vf03

			vmulax.xyzw     ACC, vf05,vf04
			vmadday.xyzw    ACC, vf06,vf04
			vmaddaz.xyzw    ACC, vf07,vf04
			vmaddw.xyzw     vf04, vf08,vf04

			andi	tmp0, %2, 15
			beq		tmp0, zero, noshift4x4

			mtsab	%2, 0
			lq		tmp0, 0x00(%2)
			lq		tmp5, 0x40(%2)
			qfsrv	tmp0, tmp0, tmp0
			qfsrv	tmp5, tmp5, tmp5
			
			nop
			nop
			
			addi	tmp1, %2, -1
			mtsab	tmp1, -1
			
			qmfc2	tmp1, vf01
			qmfc2	tmp2, vf02
			qmfc2	tmp3, vf03
			qmfc2	tmp4, vf04
			
			qfsrv	tmp0, tmp1, tmp0
			qfsrv	tmp1, tmp2, tmp1
			qfsrv	tmp2, tmp3, tmp2
			qfsrv	tmp3, tmp4, tmp3
			qfsrv	tmp4, tmp5, tmp4
			
			sq		tmp0, 0x00(%2)
			sq		tmp1, 0x10(%2)
			sq		tmp2, 0x20(%2)
			sq		tmp3, 0x30(%2)
			sq		tmp4, 0x40(%2)

			b 		_finished

noshift4x4:
			sqc2	vf01, 0x00(%2)
			sqc2	vf02, 0x10(%2)
			sqc2	vf03, 0x20(%2)
			sqc2	vf04, 0x30(%2)
_finished:
		"
		:
		: "r"(this), "r"(&m), "r"(&DestMat)
		: "vf01", "vf02", "vf03", "vf04", "vf05", "vf06", "vf07", "vf08", "memory"
		);
}

// Added by Martin Gustafsson
inline void CMat4Dfp4::InverseOrthogonal(M& DestMat) const
{
	asm volatile ("
		lq				t0,	0x00(%0)
		mtsab			%0,	0
		lq				t1,	0x10(%0)
		lq				t2,	0x20(%0)
		qfsrv			t0,	t1,	t0
		lq				t3,	0x30(%0)
		qfsrv			t1,	t2,	t1
		lq				t4,	0x40(%0)
		qfsrv			t2,	t3,	t2
		qmtc2			t0,	vf01
		qfsrv			t3,	t4,	t3
		qmtc2			t1,	vf02
		qmtc2			t2,	vf03
		qmtc2			t3,	vf04

		vmr32.w			vf01w, vf00					
		vmr32.w			vf02w, vf00
		vmr32.w			vf03w, vf00
		vmove.w			vf04w, vf00w

		pref			0, 0x00(%1)
													# REG		x		y		z		w
		vaddx.y 		vf05y, vf00y, vf02x			# VF05: 	0		m21		0		0
		vmove.zw 		vf07zw, vf03zw              # VF07: 	0		0		m33		m34
		vaddx.z 		vf05z, vf00z, vf03x			# VF05: 	0		m21		m31		0
		vmr32.x 		vf06x, vf01					# VF06: 	m12		0		0		0
		vaddy.z 		vf06z, vf00z, vf03y			# VF06: 	m12		0		m32		0
		vmove.xw 		vf05xw, vf01xw				# VF05: 	m11		m21		m31		m14
		vaddz.x 		vf07x, vf00x, vf01z			# VF07: 	m13		0		m33		m34
		vmove.yw 		vf06yw, vf02yw				# VF06: 	m12		m22		m32		m24
		vnop
		vmr32.y 		vf07y, vf02					# VF07: 	m13		m23		m33		m34
		vadda 			ACC, vf00, vf00				# Clear ACC
		vmove.w 		vf08w, vf04w                # VF08:	0		0		0		m44
		vmsubax 		ACC, vf05, vf04x
		vmsubay 		ACC, vf06, vf04y
		vmsubz.xyz 		vf08, vf07, vf04z			# VF08:	-T*R1	-T*R2	-T*R3	m44	

		andi			t0, %1, 15
		bne				t0, zero, unaligned
		nop

		# adress aligned, store as is	
		sqc2 			vf05, 0x00(%1)
		sqc2 			vf06, 0x10(%1)
		sqc2 			vf07, 0x20(%1)
		sqc2 			vf08, 0x30(%1)

		b				done
		nop

	unaligned:

		# adress not aligned, fuck around
		lq				t0, 0x00(%1)
		mtsab			%1, 0
		lq				t5, 0x40(%1)
		qfsrv			t0, t0, t0
		qfsrv			t5, t5, t5

		nop
		nop
		
		addi			t1, %1,-1
		mtsab			t1,	-1

		qmfc2			t1, vf05
		qmfc2			t2, vf06
		qmfc2			t3, vf07
		qmfc2			t4, vf08

		qfsrv			t0, t1, t0
		sq				t0,0x00(%1)
		qfsrv			t1, t2, t1
		sq				t1,0x10(%1)
		qfsrv			t2, t3, t2
		sq				t2,0x20(%1)
		qfsrv			t3, t4, t3
		sq				t3,0x30(%1)
		qfsrv			t4, t5, t4
		sq				t4,0x40(%1)

	done:
		"
		:
		: "r" (this), "r" (&DestMat)
		: "cc", "t0", "t1", "t2", "t3", "t4", "t5", "memory"
	);
}

inline void CVec3Dfp4::CrossProd(const V& a, V& dest) const
{
	fp4 _x1 = k[0];
	fp4 _y1 = k[1];
	fp4 _z1 = k[2];
	fp4 _x2 = a.k[0];
	fp4 _y2 = a.k[1];
	fp4 _z2 = a.k[2];

	fp4 _d0, _d1, _d2;
	asm ("
			mula.s	_y1, _z2
			msub.s	_d0, _z1, _y2
			
			mula.s	_z1, _x2
			msub.s	_d1, _x1, _z2
			
			mula.s	_x1, _y2
			msub.s	_d2, _y1, _x2
		 "
		);
	
	dest.k[0]	= _d0;
	dest.k[1]	= _d1;
	dest.k[2]	= _d2;
/*
	asm volatile ("
		lwc1 $f4, 0(%2)
		lwc1 $f5, 4(%2)
		lwc1 $f6, 8(%2)
		lwc1 $f1, 0(%1)
		lwc1 $f2, 4(%1)
		lwc1 $f3, 8(%1)

		mula.s $f2, $f6			# ACC = y1*z2
		msub.s $f7, $f3, $f5	# F7 = ACC - z1*y2
		swc1 $f7, 0(%0)

		mula.s $f3, $f4			# ACC = z1*x2
		msub.s $f8, $f1, $f6	# F8 = ACC - x1*z2 
		swc1 $f8, 4(%0)

		mula.s $f1, $f5			# ACC = x1*y2
		msub.s $f9, $f2, $f4	# F9 = ACC - y1*x2
		swc1 $f9, 8(%0)
		"
		:
		: "r" (&dest), "r" (this), "r" (&a)
		: "cc", "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7", "$f8", "$f9", "memory"
	);
*/
}

//template<>
inline void CVec3Dfp4::operator*=(const CMat4Dfp4 &M)
{
	asm volatile ("

	    lq				t2,	0x00(%1)			# Load Matrix (t1-t4)
	    mtsab			%1,	0					# shift count to align matrix
	    lq				t3,	0x10(%1)			# Load Matrix
	    lq				t4,	0x20(%1)			# Load Matrix
	    qfsrv			t1,	t3,	t2				# Align matrix
	    lq				t5,	0x30(%1)			# Load Matrix
		qfsrv			t2,	t4,	t3				# Align matrix
	    lq				t6,	0x40(%1)			# Load Matrix
	    qfsrv			t3,	t5,	t4				# Align matrix
	    qmtc2			t1,	vf05				# Move matrix to COP2 VF05-VF08
	    qfsrv			t4,	t6,	t5				# Align matrix
	    qmtc2			t2,	vf06
	    qmtc2			t3,	vf07
	    qmtc2			t4,	vf08
	    
	    lq				t0,	0x00(%0)			# Load Vector (t0)
	    mtsab			%0,	0					# shift count to align vector
	    lq				t1, 0x10(%0)			# Load Vector
	    qfsrv			t0,	t1,	t0				# Align vector

	    qmtc2			t0,	vf04				# Move vector to COP2 VF04

		vmulax.xyz		ACC, vf05, vf04			# VF04 =  vx * m_row0
		vmadday.xyz		ACC, vf06, vf04			# 		+ vy * m_row1
		vmaddaz.xyz		ACC, vf07, vf04			# 		+ vz * m_row2
		vmaddw.xyz		vf04,vf08, vf00			# 		+  1 * m_row3

		qmfc2      		t2, vf04             	# Get result to t2
		sw         		t2, 0x00(%0)        	# Store x
		prot3w     		t1, t2              	# Rotate y into LSW of t1
		sw         		t1, 0x04(%0)        	# Store y
		prot3w     		t2, t1              	# Rotate z into LSW of t2
		sw         		t2, 0x08(%0)        	# Store z
		"
		:
		: "r" (this), "r" (&M)
		: "cc", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "vf04", "vf05", "vf06", "vf07", "vf08", "memory"
	);
}

inline void CVec3Dfp4::Lerp(const V& a, fp4 t, V& dest) const
{
	asm volatile ("
	    lq			$2,	0x00(%1)	# Load/align src1
	    mtsab		%1,	0
	    lq			$3, 0x10(%1)
	    qfsrv		$2,	$3,	$2

		lw			$10, 0(%3)		# Load time

	    lq			$4,	0x00(%2)    # Load/align src2
	    lq			$5, 0x10(%2)
	    mtsab		%2,	0
	    qfsrv		$3,	$5,	$4

	    qmtc2		$2,	vf01
	    qmtc2		$3,	vf02
	    qmtc2		$10,vf04

		vsub.xyz	vf03, vf02, vf01
		vmulx.xyz	vf03, vf03, vf04x
		vadd.xyz	vf03, vf01, vf03

		qmfc2      	$3, vf03             	# Get result to t2
		sw         	$3, 0x00(%0)        	# Store x
		prot3w     	$2, $3              	# Rotate y into LSW of t1
		sw         	$2, 0x04(%0)        	# Store y
		prot3w     	$3, $2              	# Rotate z into LSW of t2
		sw         	$3, 0x08(%0)        	# Store z
		"
		:
		: "r" (&dest), "r" (this), "r" (&a), "r" (&t)
		: "cc", "$2", "$3", "$4", "$5", "$10", "vf01", "vf02", "vf03", "vf04", "memory"
	);
}

/*
inline void CVec3Dfp4::CompMul(const V& a, V& dest) const
{
	asm volatile ("
		lwc1		$f2, 0x00(%1)
		lwc1		$f4, 0x04(%1)
		lwc1		$f6, 0x08(%1)
		lwc1		$f3, 0x00(%2)
		lwc1		$f5, 0x04(%2)
		lwc1		$f7, 0x08(%2)

		mul.s		$f10, $f2, $f3
		swc1		$f10, 0x00(%0)
		mul.s		$f11, $f4, $f5
		swc1		$f11, 0x04(%0)
		mul.s		$f12, $f6, $f7
		swc1		$f12, 0x08(%0)
		"
		:
		: "r" (&dest), "r" (this), "r" (&a)
		: "cc", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7", "$f10", "$f11", "$f12", "memory"
	);
}
	
inline CVec3Dfp4 &CVec3Dfp4::Normalize()
{
	CVec3Dfp4Normalize(this);
	return *this;
}
*/

#endif	// PLATFORM_PS2
