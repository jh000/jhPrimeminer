#include"global.h"

int BN2_num_bits_word(BN_ULONG l)
{
	static const unsigned char bits[256]={
		0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
		5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	};

#if defined(SIXTY_FOUR_BIT_LONG)
	if (l & 0xffffffff00000000L)
	{
		if (l & 0xffff000000000000L)
		{
			if (l & 0xff00000000000000L)
			{
				return(bits[(int)(l>>56)]+56);
			}
			else	return(bits[(int)(l>>48)]+48);
		}
		else
		{
			if (l & 0x0000ff0000000000L)
			{
				return(bits[(int)(l>>40)]+40);
			}
			else	return(bits[(int)(l>>32)]+32);
		}
	}
	else
#else
#ifdef SIXTY_FOUR_BIT
	if (l & 0xffffffff00000000LL)
	{
		if (l & 0xffff000000000000LL)
		{
			if (l & 0xff00000000000000LL)
			{
				return(bits[(int)(l>>56)]+56);
			}
			else	return(bits[(int)(l>>48)]+48);
		}
		else
		{
			if (l & 0x0000ff0000000000LL)
			{
				return(bits[(int)(l>>40)]+40);
			}
			else	return(bits[(int)(l>>32)]+32);
		}
	}
	else
#endif
#endif
	{
#if defined(THIRTY_TWO_BIT) || defined(SIXTY_FOUR_BIT) || defined(SIXTY_FOUR_BIT_LONG)
		if (l & 0xffff0000L)
		{
			if (l & 0xff000000L)
				return(bits[(int)(l>>24L)]+24);
			else	return(bits[(int)(l>>16L)]+16);
		}
		else
#endif
		{
#if defined(THIRTY_TWO_BIT) || defined(SIXTY_FOUR_BIT) || defined(SIXTY_FOUR_BIT_LONG)
			if (l & 0xff00L)
				return(bits[(int)(l>>8)]+8);
			else	
#endif
				return(bits[(int)(l   )]  );
		}
	}
}

int BN2_num_bits(const BIGNUM *a)
{
	int i = a->top - 1;
	bn_check_top(a);

	if (BN_is_zero(a)) return 0;
	return ((i*BN_BITS2) + BN2_num_bits_word(a->d[i]));
}

int inline BN2_is_bit_set(const BIGNUM *a, int n)
{
	int i,j;

	bn_check_top(a);
	if (n < 0) return 0;
	i=n/BN_BITS2;
	j=n%BN_BITS2;
	if (a->top <= i) return 0;
	return (int)(((a->d[i])>>j)&((BN_ULONG)1));
}

/*
 * Counts the number of set bits starting at a value of 1 then 2, 4, 8, 16...
 * If a bit is not set, the method exits and returns the number of bits up to this bit
 * a must not be zero
 */
int inline BN2_nz_num_unset_bits_from_lsb(const BIGNUM *a)
{
#ifdef _WIN32
	sint32 bIdx = 0;
	uint32 idx = 0;
	sint32 maxIdx = a->top-1;
	do 
	{
		_BitScanForward(&idx, a->d[bIdx]);
		if( idx==0 )
		{
			if( bIdx >= maxIdx )
				break;
			continue;
		}
		else
			break;
	}while(true);
	return bIdx*32+idx;
	// _BitScanReverse
#else
	needs implementation
#endif
}


int BN2_sub(BIGNUM *r, const BIGNUM *a, const BIGNUM *b)
{
	int max;
	int add=0,neg=0;
	const BIGNUM *tmp;

	bn_check_top(a);
	bn_check_top(b);

	/*  a -  b	a-b
	*  a - -b	a+b
	* -a -  b	-(a+b)
	* -a - -b	b-a
	*/
	if (a->neg)
	{
		if (b->neg)
		{ tmp=a; a=b; b=tmp; }
		else
		{ add=1; neg=1; }
	}
	else
	{
		if (b->neg) { add=1; neg=0; }
	}

	if (add)
	{
		if (!BN2_uadd(r,a,b)) return(0);
		r->neg=neg;
		return(1);
	}

	/* We are actually doing a - b :-) */

	max=(a->top > b->top)?a->top:b->top;
	if (bn_wexpand(r,max) == NULL) return(0);
	if (BN_ucmp(a,b) < 0)
	{
		if (!BN_usub(r,b,a)) return(0);
		r->neg=1;
	}
	else
	{
		if (!BN_usub(r,a,b)) return(0);
		r->neg=0;
	}
	bn_check_top(r);
	return(1);
}

int BN2_nnmod(BIGNUM *r, const BIGNUM *m, const BIGNUM *d, BN_CTX *ctx)
{
	/* like BN_mod, but returns non-negative remainder
	* (i.e.,  0 <= r < |d|  always holds) */

	if (!(BN2_div(NULL,r,m,d)))
		return 0;
	if (!r->neg)
		return 1;
	/* now   -|d| < r < 0,  so we have to set  r := r + |d| */
	return (d->neg ? BN2_sub : BN_add)(r, r, d);
}

int BN2_rshift1(BIGNUM *r, const BIGNUM *a)
{
	BN_ULONG *ap,*rp,t,c;
	int i,j;

	bn_check_top(r);
	bn_check_top(a);

	if (BN_is_zero(a))
	{
		BN_zero(r);
		return(1);
	}
	i = a->top;
	ap= a->d;
	j = i-(ap[i-1]==1);
	if (a != r)
	{
		if (bn_wexpand(r,j) == NULL) return(0);
		r->neg=a->neg;
	}
	rp=r->d;
	t=ap[--i];
	c=(t&1)?BN_TBIT:0;
	if (t>>=1) rp[i]=t;
	while (i>0)
	{
		t=ap[--i];
		rp[i]=((t>>1)&BN_MASK2)|c;
		c=(t&1)?BN_TBIT:0;
	}
	r->top=j;
	bn_check_top(r);
	return(1);
}

int BN2_rshift(BIGNUM *r, const BIGNUM *a, int n)
{
	int i,j,nw,lb,rb;
	BN_ULONG *t,*f;
	BN_ULONG l,tmp;

	bn_check_top(r);
	bn_check_top(a);

	nw=n/BN_BITS2;
	rb=n%BN_BITS2;
	lb=BN_BITS2-rb;
	if (nw >= a->top || a->top == 0)
	{
		BN_zero(r);
		return(1);
	}
	i = (BN_num_bits(a)-n+(BN_BITS2-1))/BN_BITS2;
	if (r != a)
	{
		r->neg=a->neg;
		if (bn_wexpand(r,i) == NULL) return(0);
	}
	else
	{
		if (n == 0)
			return 1; /* or the copying loop will go berserk */
	}

	f= &(a->d[nw]);
	t=r->d;
	j=a->top-nw;
	r->top=i;

	if (rb == 0)
	{
		for (i=j; i != 0; i--)
			*(t++)= *(f++);
	}
	else
	{
		l= *(f++);
		for (i=j-1; i != 0; i--)
		{
			tmp =(l>>rb)&BN_MASK2;
			l= *(f++);
			*(t++) =(tmp|(l<<lb))&BN_MASK2;
		}
		if ((l = (l>>rb)&BN_MASK2)) *(t) = l;
	}
	bn_check_top(r);
	return(1);
}

int BN2_lshift1(BIGNUM *r, const BIGNUM *a)
{
	register BN_ULONG *ap,*rp,t,c;
	int i;

	bn_check_top(r);
	bn_check_top(a);

	if (r != a)
	{
		r->neg=a->neg;
		if (bn_wexpand(r,a->top+1) == NULL) return(0);
		r->top=a->top;
	}
	else
	{
		if (bn_wexpand(r,a->top+1) == NULL) return(0);
	}
	ap=a->d;
	rp=r->d;
	c=0;
	for (i=0; i<a->top; i++)
	{
		t= *(ap++);
		*(rp++)=((t<<1)|c)&BN_MASK2;
		c=(t & BN_TBIT)?1:0;
	}
	if (c)
	{
		*rp=1;
		r->top++;
	}
	bn_check_top(r);
	return(1);
}

int BN2_lshift(BIGNUM *r, const BIGNUM *a, int n)
{
	int i,nw,lb,rb;
	BN_ULONG *t,*f;
	BN_ULONG l;

	bn_check_top(r);
	bn_check_top(a);

	r->neg=a->neg;
	nw=n/BN_BITS2;
	if (bn_wexpand(r,a->top+nw+1) == NULL) return(0);
	lb=n%BN_BITS2;
	rb=BN_BITS2-lb;
	f=a->d;
	t=r->d;
	t[a->top+nw]=0;
	if (lb == 0)
		for (i=a->top-1; i>=0; i--)
			t[nw+i]=f[i];
	else
		for (i=a->top-1; i>=0; i--)
		{
			l=f[i];
			t[nw+i+1]|=(l>>rb)&BN_MASK2;
			t[nw+i]=(l<<lb)&BN_MASK2;
		}
		memset(t,0,nw*sizeof(t[0]));
		/*	for (i=0; i<nw; i++)
		t[i]=0;*/
		r->top=a->top+nw+1;
		bn_correct_top(r);
		bn_check_top(r);
		return(1);
}

/* unsigned add of b to a */
int BN2_uadd(BIGNUM *r, const BIGNUM *a, const BIGNUM *b)
{
	int max,min,dif;
	BN_ULONG *ap,*bp,*rp,carry,t1,t2;
	const BIGNUM *tmp;

	bn_check_top(a);
	bn_check_top(b);

	if (a->top < b->top)
	{ tmp=a; a=b; b=tmp; }
	max = a->top;
	min = b->top;
	dif = max - min;

	if (bn_wexpand(r,max+1) == NULL)
		return 0;

	r->top=max;


	ap=a->d;
	bp=b->d;
	rp=r->d;

	carry=bn_add_words(rp,ap,bp,min);
	rp+=min;
	ap+=min;
	bp+=min;

	if (carry)
	{
		while (dif)
		{
			dif--;
			t1 = *(ap++);
			t2 = (t1+1) & BN_MASK2;
			*(rp++) = t2;
			if (t2)
			{
				carry=0;
				break;
			}
		}
		if (carry)
		{
			/* carry != 0 => dif == 0 */
			*rp = 1;
			r->top++;
		}
	}
	if (dif && rp != ap)
		while (dif--)
			/* copy remaining words if ap != rp */
			*(rp++) = *(ap++);
	r->neg = 0;
	bn_check_top(r);
	return 1;
}

BIGNUM *BN2_mod_inverse(BIGNUM *in,
						const BIGNUM *a, const BIGNUM *n, BN_CTX *ctx)
{
	BIGNUM *A,*B,*X,*Y,*M,*D,*T,*R=NULL;
	BIGNUM *ret=NULL;
	int sign;

	bn_check_top(a);
	bn_check_top(n);

	BN_CTX_start(ctx);
	A = BN_CTX_get(ctx);
	B = BN_CTX_get(ctx);
	X = BN_CTX_get(ctx);
	D = BN_CTX_get(ctx);
	M = BN_CTX_get(ctx);
	Y = BN_CTX_get(ctx);
	T = BN_CTX_get(ctx);
	if (T == NULL) goto err;

	if (in == NULL)
		R=BN_new();
	else
		R=in;
	if (R == NULL) goto err;

	BN_one(X);
	BN_zero(Y);
	if (BN_copy(B,a) == NULL) goto err;
	if (BN_copy(A,n) == NULL) goto err;
	A->neg = 0;
	if (B->neg || (BN_ucmp(B, A) >= 0))
	{
		if (!BN2_nnmod(B, B, A, ctx)) goto err;
	}
	sign = -1;
	/* From  B = a mod |n|,  A = |n|  it follows that
	*
	*      0 <= B < A,
	*     -sign*X*a  ==  B   (mod |n|),
	*      sign*Y*a  ==  A   (mod |n|).
	*/

	if (BN_is_odd(n) && (BN2_num_bits(n) <= (BN_BITS <= 32 ? 450 : 2048)))
	{
		/* Binary inversion algorithm; requires odd modulus.
		* This is faster than the general algorithm if the modulus
		* is sufficiently small (about 400 .. 500 bits on 32-bit
		* sytems, but much more on 64-bit systems) */
		int shift;

		while (!BN_is_zero(B))
		{
			/*
			*      0 < B < |n|,
			*      0 < A <= |n|,
			* (1) -sign*X*a  ==  B   (mod |n|),
			* (2)  sign*Y*a  ==  A   (mod |n|)
			*/

			/* Now divide  B  by the maximum possible power of two in the integers,
			* and divide  X  by the same value mod |n|.
			* When we're done, (1) still holds. */
			uint32 shiftAmount = BN2_nz_num_unset_bits_from_lsb(B);
			shift = shiftAmount;
			for(uint32 sa=0; sa<shiftAmount; sa++)
			{
				if (BN_is_odd(X))
				{
					BN2_uadd(X, X, n);
				}
				/* now X is even, so we can easily divide it by two */
				BN2_rshift1(X, X);
			}
			if (shift > 0)
			{
				if (!BN2_rshift(B, B, shift)) goto err;
			}
			/* Same for  A  and  Y.  Afterwards, (2) still holds. */
			shiftAmount = BN2_nz_num_unset_bits_from_lsb(A);
			shift = shiftAmount;
			for(uint32 sa=0; sa<shiftAmount; sa++)
			{
				if (BN_is_odd(Y))
				{
					if (!BN2_uadd(Y, Y, n)) goto err;
				}
				/* now Y is even */
				if (!BN2_rshift1(Y, Y)) goto err;
			}
			if (shift > 0)
			{
				if (!BN2_rshift(A, A, shift)) goto err;
			}


			/* We still have (1) and (2).
			* Both  A  and  B  are odd.
			* The following computations ensure that
			*
			*     0 <= B < |n|,
			*      0 < A < |n|,
			* (1) -sign*X*a  ==  B   (mod |n|),
			* (2)  sign*Y*a  ==  A   (mod |n|),
			*
			* and that either  A  or  B  is even in the next iteration.
			*/
			if (BN_ucmp(B, A) >= 0)
			{
				/* -sign*(X + Y)*a == B - A  (mod |n|) */
				if (!BN2_uadd(X, X, Y)) goto err;
				/* NB: we could use BN_mod_add_quick(X, X, Y, n), but that
				* actually makes the algorithm slower */
				if (!BN_usub(B, B, A)) goto err;
			}
			else
			{
				/*  sign*(X + Y)*a == A - B  (mod |n|) */
				if (!BN2_uadd(Y, Y, X)) goto err;
				/* as above, BN_mod_add_quick(Y, Y, X, n) would slow things down */
				if (!BN_usub(A, A, B)) goto err;
			}
		}
	}
	else
	{
		/* general inversion algorithm */

		while (!BN_is_zero(B))
		{
			BIGNUM *tmp;

			/*
			*      0 < B < A,
			* (*) -sign*X*a  ==  B   (mod |n|),
			*      sign*Y*a  ==  A   (mod |n|)
			*/

			/* (D, M) := (A/B, A%B) ... */
			if (BN2_num_bits(A) == BN2_num_bits(B))
			{
				if (!BN_one(D)) goto err;
				if (!BN2_sub(M,A,B)) goto err;
			}
			else if (BN2_num_bits(A) == BN2_num_bits(B) + 1)
			{
				/* A/B is 1, 2, or 3 */
				if (!BN2_lshift1(T,B)) goto err;
				if (BN_ucmp(A,T) < 0)
				{
					/* A < 2*B, so D=1 */
					if (!BN_one(D)) goto err;
					if (!BN2_sub(M,A,B)) goto err;
				}
				else
				{
					/* A >= 2*B, so D=2 or D=3 */
					if (!BN2_sub(M,A,T)) goto err;
					if (!BN_add(D,T,B)) goto err; /* use D (:= 3*B) as temp */
					if (BN_ucmp(A,D) < 0)
					{
						/* A < 3*B, so D=2 */
						if (!BN_set_word(D,2)) goto err;
						/* M (= A - 2*B) already has the correct value */
					}
					else
					{
						/* only D=3 remains */
						if (!BN_set_word(D,3)) goto err;
						/* currently  M = A - 2*B,  but we need  M = A - 3*B */
						if (!BN2_sub(M,M,B)) goto err;
					}
				}
			}
			else
			{
				if (!BN2_div(D,M,A,B)) goto err;
			}

			/* Now
			*      A = D*B + M;
			* thus we have
			* (**)  sign*Y*a  ==  D*B + M   (mod |n|).
			*/

			tmp=A; /* keep the BIGNUM object, the value does not matter */

			/* (A, B) := (B, A mod B) ... */
			A=B;
			B=M;
			/* ... so we have  0 <= B < A  again */

			/* Since the former  M  is now  B  and the former  B  is now  A,
			* (**) translates into
			*       sign*Y*a  ==  D*A + B    (mod |n|),
			* i.e.
			*       sign*Y*a - D*A  ==  B    (mod |n|).
			* Similarly, (*) translates into
			*      -sign*X*a  ==  A          (mod |n|).
			*
			* Thus,
			*   sign*Y*a + D*sign*X*a  ==  B  (mod |n|),
			* i.e.
			*        sign*(Y + D*X)*a  ==  B  (mod |n|).
			*
			* So if we set  (X, Y, sign) := (Y + D*X, X, -sign),  we arrive back at
			*      -sign*X*a  ==  B   (mod |n|),
			*       sign*Y*a  ==  A   (mod |n|).
			* Note that  X  and  Y  stay non-negative all the time.
			*/

			/* most of the time D is very small, so we can optimize tmp := D*X+Y */
			if (BN_is_one(D))
			{
				if (!BN_add(tmp,X,Y)) goto err;
			}
			else
			{
				if (BN_is_word(D,2))
				{
					if (!BN2_lshift1(tmp,X)) goto err;
				}
				else if (BN_is_word(D,4))
				{
					if (!BN2_lshift(tmp,X,2)) goto err;
				}
				else if (D->top == 1)
				{
					if (!BN_copy(tmp,X)) goto err;
					if (!BN_mul_word(tmp,D->d[0])) goto err;
				}
				else
				{
					if (!BN_mul(tmp,D,X,ctx)) goto err;
				}
				if (!BN_add(tmp,tmp,Y)) goto err;
			}

			M=Y; /* keep the BIGNUM object, the value does not matter */
			Y=X;
			X=tmp;
			sign = -sign;
		}
	}

	/*
	* The while loop (Euclid's algorithm) ends when
	*      A == gcd(a,n);
	* we have
	*       sign*Y*a  ==  A  (mod |n|),
	* where  Y  is non-negative.
	*/

	if (sign < 0)
	{
		if (!BN2_sub(Y,n,Y)) goto err;
	}
	/* Now  Y*a  ==  A  (mod |n|).  */


	if (BN_is_one(A))
	{
		/* Y*a == 1  (mod |n|) */
		if (!Y->neg && BN_ucmp(Y,n) < 0)
		{
			if (!BN_copy(R,Y)) goto err;
		}
		else
		{
			if (!BN2_nnmod(R,Y,n,ctx)) goto err;
		}
	}
	else
	{
		// BNerr(BN_F_BN_MOD_INVERSE,BN_R_NO_INVERSE);
		goto err;
	}
	ret=R;
err:
	if ((ret == NULL) && (in == NULL)) BN_free(R);
	BN_CTX_end(ctx);
	bn_check_top(ret);
	return(ret);
}