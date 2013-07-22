#include"global.h"

#define bn_clear_top2max(a)

#define LBITS(a)	((a)&BN_MASK2l)
#define HBITS(a)	(((a)>>BN_BITS4)&BN_MASK2l)
#define	L2HBITS(a)	(((a)<<BN_BITS4)&BN_MASK2)


#define mul64(l,h,bl,bh) \
	{ \
	BN_ULONG m,m1,lt,ht; \
	\
	lt=l; \
	ht=h; \
	m =(bh)*(lt); \
	lt=(bl)*(lt); \
	m1=(bl)*(ht); \
	ht =(bh)*(ht); \
	m=(m+m1)&BN_MASK2; if (m < m1) ht+=L2HBITS((BN_ULONG)1); \
	ht+=HBITS(m); \
	m1=L2HBITS(m); \
	lt=(lt+m1)&BN_MASK2; if (lt < m1) ht++; \
	(l)=lt; \
	(h)=ht; \
	}

//#define LLBITS(a)	((a)&BN_MASKl)
//#define LHBITS(a)	(((a)>>BN_BITS2)&BN_MASKl)
//#define	LL2HBITS(a)	((BN_ULLONG)((a)&BN_MASKl)<<BN_BITS2)


/* BN_div computes  dv := num / divisor,  rounding towards
 * zero, and sets up rm  such that  dv*divisor + rm = num  holds.
 * Thus:
 *     dv->neg == num->neg ^ divisor->neg  (unless the result is zero)
 *     rm->neg == num->neg                 (unless the remainder is zero)
 * If 'dv' or 'rm' is NULL, the respective value is not returned.
 */
int BN2_div(BIGNUM *dv, BIGNUM *rm, const BIGNUM *num, const BIGNUM *divisor)
	{
	int norm_shift,i,loop;
	BIGNUM *tmp,wnum,*snum,*sdiv,*res;
	BN_ULONG *resp,*wnump;
	BN_ULONG d0,d1;
	int num_n,div_n;
	int no_branch=0;


	/* Invalid zero-padding would have particularly bad consequences
	 * in the case of 'num', so don't just rely on bn_check_top() for this one
	 * (bn_check_top() works only for BN_DEBUG builds) */
	if (num->top > 0 && num->d[num->top - 1] == 0)
		{
		//BNerr(BN_F_BN_DIV,BN_R_NOT_INITIALIZED);
		return 0;
		}

	bn_check_top(num);

	bn_check_top(dv);
	bn_check_top(rm);
	/* bn_check_top(num); */ /* 'num' has been checked already */
	bn_check_top(divisor);

	if (BN_is_zero(divisor))
		{
		//BNerr(BN_F_BN_DIV,BN_R_DIV_BY_ZERO);
		return(0);
		}

	if (!no_branch && BN_ucmp(num,divisor) < 0)
		{
		if (rm != NULL)
			{ if (BN_copy(rm,num) == NULL) return(0); }
		if (dv != NULL) BN_zero(dv);
		return(1);
		}




	uint32 bignumData_d1[0x200/4];
	uint32 bignumData_d2[0x200/4];
	uint32 bignumData_d3[0x200/4];
	uint32 bignumData_d4[0x200/4];
	//uint32 bignumData_d5[0x200/4];
	BIGNUM bn_d1;
	BIGNUM bn_d2;
	BIGNUM bn_d3;
	BIGNUM bn_d4;
	//BIGNUM bn_d5;
	fastInitBignum(bn_d1, bignumData_d1);
	fastInitBignum(bn_d2, bignumData_d2);
	fastInitBignum(bn_d3, bignumData_d3);
	fastInitBignum(bn_d4, bignumData_d4);
	//fastInitBignum(bn_d5, bignumData_d5);


	//BN_CTX_start(ctx);
	tmp=&bn_d1;
	snum=&bn_d2;
	sdiv=&bn_d3;
	if (dv == NULL)
		res=&bn_d4;//__debugbreak();//res=BN_CTX_get(ctx);
	else
		res=dv;

	if (sdiv == NULL || res == NULL || tmp == NULL || snum == NULL)
		goto err;

	/* First we normalise the numbers */
	norm_shift=BN_BITS2-((BN2_num_bits(divisor))%BN_BITS2);
	if (!(BN2_lshift(sdiv,divisor,norm_shift))) goto err;
	sdiv->neg=0;
	norm_shift+=BN_BITS2;
	if (!(BN2_lshift(snum,num,norm_shift))) goto err;
	snum->neg=0;

	if (no_branch)
		{
		/* Since we don't know whether snum is larger than sdiv,
		 * we pad snum with enough zeroes without changing its
		 * value. 
		 */
		if (snum->top <= sdiv->top+1) 
			{
			if (bn_wexpand(snum, sdiv->top + 2) == NULL) goto err;
			for (i = snum->top; i < sdiv->top + 2; i++) snum->d[i] = 0;
			snum->top = sdiv->top + 2;
			}
		else
			{
			if (bn_wexpand(snum, snum->top + 1) == NULL) goto err;
			snum->d[snum->top] = 0;
			snum->top ++;
			}
		}

	div_n=sdiv->top;
	num_n=snum->top;
	loop=num_n-div_n;
	/* Lets setup a 'window' into snum
	 * This is the part that corresponds to the current
	 * 'area' being divided */
	wnum.neg   = 0;
	wnum.d     = &(snum->d[loop]);
	wnum.top   = div_n;
	/* only needed when BN_ucmp messes up the values between top and max */
	wnum.dmax  = snum->dmax - loop; /* so we don't step out of bounds */

	/* Get the top 2 words of sdiv */
	/* div_n=sdiv->top; */
	d0=sdiv->d[div_n-1];
	d1=(div_n == 1)?0:sdiv->d[div_n-2];

	/* pointer to the 'top' of snum */
	wnump= &(snum->d[num_n-1]);

	/* Setup to 'res' */
	res->neg= (num->neg^divisor->neg);
	if (!bn_wexpand(res,(loop+1))) goto err;
	res->top=loop-no_branch;
	resp= &(res->d[loop-1]);

	/* space for temp */
	if (!bn_wexpand(tmp,(div_n+1))) goto err;

	if (!no_branch)
		{
		if (BN_ucmp(&wnum,sdiv) >= 0)
			{
			/* If BN_DEBUG_RAND is defined BN_ucmp changes (via
			 * bn_pollute) the const bignum arguments =>
			 * clean the values between top and max again */
			bn_clear_top2max(&wnum);
			bn_sub_words(wnum.d, wnum.d, sdiv->d, div_n);
			*resp=1;
			}
		else
			res->top--;
		}

	/* if res->top == 0 then clear the neg value otherwise decrease
	 * the resp pointer */
	if (res->top == 0)
		res->neg = 0;
	else
		resp--;

	for (i=0; i<loop-1; i++, wnump--, resp--)
		{
		BN_ULONG q,l0;
		/* the first part of the loop uses the top two words of
		 * snum and sdiv to calculate a BN_ULONG q such that
		 * | wnum - sdiv * q | < sdiv */
#if defined(BN_DIV3W) && !defined(OPENSSL_NO_ASM)
		BN_ULONG bn_div_3_words(BN_ULONG*,BN_ULONG,BN_ULONG);
		q=bn_div_3_words(wnump,d1,d0);
#else
		BN_ULONG n0,n1,rem=0;

		n0=wnump[0];
		n1=wnump[-1];
		if (n0 == d0)
			q=BN_MASK2;
		else 			/* n0 < d0 */
			{
#ifdef BN_LLONG
			BN_ULLONG t2;

#if defined(BN_LLONG) && defined(BN_DIV2W) && !defined(bn_div_words)
			q=(BN_ULONG)(((((BN_ULLONG)n0)<<BN_BITS2)|n1)/d0);
#else
			q=bn_div_words(n0,n1,d0);
#ifdef BN_DEBUG_LEVITTE
			fprintf(stderr,"DEBUG: bn_div_words(0x%08X,0x%08X,0x%08\
X) -> 0x%08X\n",
				n0, n1, d0, q);
#endif
#endif

#ifndef REMAINDER_IS_ALREADY_CALCULATED
			/*
			 * rem doesn't have to be BN_ULLONG. The least we
			 * know it's less that d0, isn't it?
			 */
			rem=(n1-q*d0)&BN_MASK2;
#endif
			t2=(BN_ULLONG)d1*q;

			for (;;)
				{
				if (t2 <= ((((BN_ULLONG)rem)<<BN_BITS2)|wnump[-2]))
					break;
				q--;
				rem += d0;
				if (rem < d0) break; /* don't let rem overflow */
				t2 -= d1;
				}
#else /* !BN_LLONG */
			BN_ULONG t2l,t2h;

			q=bn_div_words(n0,n1,d0);
#ifdef BN_DEBUG_LEVITTE
			fprintf(stderr,"DEBUG: bn_div_words(0x%08X,0x%08X,0x%08\
X) -> 0x%08X\n",
				n0, n1, d0, q);
#endif
#ifndef REMAINDER_IS_ALREADY_CALCULATED
			rem=(n1-q*d0)&BN_MASK2;
#endif

#if defined(BN_UMULT_LOHI)
			BN_UMULT_LOHI(t2l,t2h,d1,q);
#elif defined(BN_UMULT_HIGH)
			t2l = d1 * q;
			t2h = BN_UMULT_HIGH(d1,q);
#else
			{
			BN_ULONG ql, qh;
			t2l=LBITS(d1); t2h=HBITS(d1);
			ql =LBITS(q);  qh =HBITS(q);
			mul64(t2l,t2h,ql,qh); /* t2=(BN_ULLONG)d1*q; */
			}
#endif

			for (;;)
				{
				if ((t2h < rem) ||
					((t2h == rem) && (t2l <= wnump[-2])))
					break;
				q--;
				rem += d0;
				if (rem < d0) break; /* don't let rem overflow */
				if (t2l < d1) t2h--; t2l -= d1;
				}
#endif /* !BN_LLONG */
			}
#endif /* !BN_DIV3W */

		l0=bn_mul_words(tmp->d,sdiv->d,div_n,q);
		tmp->d[div_n]=l0;
		wnum.d--;
		/* ingore top values of the bignums just sub the two 
		 * BN_ULONG arrays with bn_sub_words */
		if (bn_sub_words(wnum.d, wnum.d, tmp->d, div_n+1))
			{
			/* Note: As we have considered only the leading
			 * two BN_ULONGs in the calculation of q, sdiv * q
			 * might be greater than wnum (but then (q-1) * sdiv
			 * is less or equal than wnum)
			 */
			q--;
			if (bn_add_words(wnum.d, wnum.d, sdiv->d, div_n))
				/* we can't have an overflow here (assuming
				 * that q != 0, but if q == 0 then tmp is
				 * zero anyway) */
				(*wnump)++;
			}
		/* store part of the result */
		*resp = q;
		}
	bn_correct_top(snum);
	if (rm != NULL)
		{
		/* Keep a copy of the neg flag in num because if rm==num
		 * BN_rshift() will overwrite it.
		 */
		int neg = num->neg;
		BN2_rshift(rm,snum,norm_shift);
		if (!BN_is_zero(rm))
			rm->neg = neg;
		bn_check_top(rm);
		}
	if (no_branch)	bn_correct_top(res);
	//BN_CTX_end(ctx);
	return(1);
err:
	bn_check_top(rm);
	//BN_CTX_end(ctx);
	return(0);
	}