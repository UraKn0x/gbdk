/* lkrloc.c */

/*
 * (C) Copyright 1989-1995
 * All Rights Reserved
 *
 * Alan R. Baldwin
 * 721 Berkeley St.
 * Kent, Ohio  44240
 */

/*
 * Extensions: P. Felber
 */

#include <stdio.h>
#include <string.h>
#include <alloc.h>
#include "aslink.h"

/*)Module	lkrloc.c
 *
 *	The module lkrloc.c contains the functions which
 *	perform the relocation calculations.
 *
 *	lkrloc.c contains the following functions:
 *		addr_t	adb_b()
 *		addr_t	adb_lo()
 *		addr_t	adb_hi()
 *		addr_t	adw_w()
 *		addr_t	adw_lo()
 *		addr_t	adw_hi()
 *		VOID	erpdmp()
 *		VOID	errdmp()
 *		addr_t	evword()
 *		VOID	prntval()
 *		VOID	rele()
 *		VOID	relerr()
 *		VOID	relerp()
 *		VOID	reloc()
 *		VOID	relp()
 *		VOID	relr()
 *		VOID	relt()
 *
 *	lkrloc.c the local variable errmsg[].
 *
 */

/*)Function	VOID	reloc(c)
 *
 *			char c		process code
 *
 *	The function reloc() calls a particular relocation
 *	function determined by the process code.
 *
 *	local variable:
 *		none
 *
 *	global variables:
 *		int	lkerr		error flag
 *
 *	called functions:
 *		int	fprintf()	c_library
 *		VOID	rele()		lkrloc.c
 *		VOID	relp()		lkrloc.c
 *		VOID	relr()		lkrloc.c
 *		VOId	relt()		lkrloc.c
 *
 *	side effects:
 *		Refer to the called relocation functions.
 *
 */

VOID
reloc(c)
char c;
{
	switch(c) {

	case 'T':
		relt();
		break;

	case 'R':
		relr();
		break;

	case 'P':
		relp();
		break;

	case 'E':
		rele();
		break;

	default:
		fprintf(stderr, "Undefined Relocation Operation\n");
		lkerr++;
		break;

	}
}


/*)Function	VOID	relt()
 *
 *	The function relt() evaluates a T line read by
 *	the linker. Each byte value read is saved in the
 *	rtval[] array, rtflg[] is set, and the number of
 *	evaluations is maintained in rtcnt.
 *
 *		T Line 
 *
 *		T xx xx nn nn nn nn nn ...  
 *
 *
 *		In:	"T n0 n1 n2 n3 ... nn"
 *
 *		Out:	  0    1    2    ..  rtcnt
 *			+----+----+----+----+----+
 *		  rtval | n0 | n1 | n2 | .. | nn |
 *			+----+----+----+----+----+
 *		  rtflag|  1 |  1 |  1 |  1 |  1 |
 *			+----+----+----+----+----+
 *
 * 	The  T  line contains the assembled code output by the assem-
 *	bler with xx xx being the offset address from the  current  area
 *	base address and nn being the assembled instructions and data in
 *	byte format.  
 *
 *	local variable:
 *		none
 *
 *	global variables:
 *		int	rtcnt		number of values evaluated
 *		int	rtflg[]		array of evaluation flags
 *		int	rtval[]		array of evaluation values
 *
 *	called functions:
 *		int	eval()		lkeval.c
 *		int	more()		lklex.c
 *
 *	side effects:
 *		Linker input T line evaluated.
 *
 */

VOID
relt()
{
	rtcnt = 0;
	while (more()) {
		if (rtcnt < NTXT) {
			rtval[rtcnt] = eval();
			rtflg[rtcnt] = 1;
			rtcnt++;
		}
	}
}

/*)Function	VOID	relr()
 *
 *	The function relr() evaluates a R line read by
 *	the linker.  The R line data is combined with the
 *	previous T line data to perform the relocation of
 *	code and data bytes.  The S19 / IHX output and
 *	translation of the LST files to RST files may be
 *	performed.
 *
 *		R Line 
 *
 *		R 0 0 nn nn n1 n2 xx xx ...  
 *
 * 	The R line provides the relocation information to the linker.
 *	The nn nn value is the current area index, i.e.  which area  the
 *	current  values  were  assembled.  Relocation information is en-
 *	coded in groups of 4 bytes:  
 *
 *	1.  n1 is the relocation mode and object format 
 *	 	1.  bit 0 word(0x00)/byte(0x01) 
 *	 	2.  bit 1 relocatable area(0x00)/symbol(0x02) 
 *	 	3.  bit 2 normal(0x00)/PC relative(0x04) relocation 
 *	 	4.  bit  3  1-byte(0x00)/2-byte(0x08) object format for
 *		    byte data 
 *	 	5.  bit 4 signed(0x00)/unsigned(0x10) byte data 
 *	 	6.  bit 5 normal(0x00)/page '0'(0x20) reference 
 *	 	7.  bit 6 normal(0x00)/page 'nnn'(0x40) reference 
 *
 *	2.  n2  is  a byte index into the corresponding (i.e.  pre-
 *	 	ceeding) T line data (i.e.  a pointer to the data to be
 *	 	updated  by  the  relocation).   The T line data may be
 *	 	1-byte or  2-byte  byte  data  format  or  2-byte  word
 *	 	format.  
 *
 *	3.  xx xx  is the area/symbol index for the area/symbol be-
 *	 	ing referenced.  the corresponding area/symbol is found
 *		in the header area/symbol lists.  
 *
 *	The groups of 4 bytes are repeated for each item requiring relo-
 *	cation in the preceeding T line.  
 *
 *	local variable:
 *		areax	**a		pointer to array of area pointers
 *		int	aindex		area index
 *		char	*errmsg[]	array of pointers to error strings
 *		int	error		error code
 *		int	lkerr		error flag
 *		int	mode		relocation mode
 *		adrr_t	paga		paging base area address
 *		addr_t	pags		paging symbol address
 *		addr_t	pc		relocated base address
 *		addr_t	r		PCR relocation value
 *		addr_t	reli		relocation initial value
 *		addr_t	relv		relocation final value
 *		int	rindex		symbol / area index
 *		addr_t	rtbase		base code address
 *		addr_t	rtofst		rtval[] index offset
 *		int	rtp		index into T data
 *		sym	**s		pointer to array of symbol pointers
 *
 *	global variables:
 *		head	*hp		pointer to the head structure
 *		rerr	rerr		linker error structure
 *		FILE	*stderr		standard error device
 *
 *	called functions:
 *		addr_t	adb_b()		lkrloc.c
 *		addr_t	adb_lo()	lkrloc.c
 *		addr_t	adb_hi()	lkrloc.c
 *		addr_t	adw_w()		lkrloc.c
 *		addr_t	evword()	lkrloc.c
 *		int	eval()		lkeval.c
 *		int	fprintf()	c_library
 *		VOID	ihx()		lkihx.c
 *		int	lkulist		lklist.c
 *		int	more()		lklex.c
 *		VOID	relerr()	lkrloc.c
 *		VOID	s19()		lks19.c
 *		int	symval()	lksym.c
 *
 *	side effects:
 *		The R and T lines are combined to produce
 *		relocated code and data.  Output S19 / IHX
 *		and relocated listing files may be produced.
 *
 */

VOID
relr()
{
	register mode;
	register addr_t reli, relv;
	int aindex, rindex, rtp, error;
	addr_t r, rtbase, rtofst, paga, pags, pc;
	struct areax **a;
	struct sym **s;

	/*
	 * Get area and symbol lists
	 */
	a = hp->a_list;
	s = hp->s_list;

	/*
	 * Verify Area Mode
	 */
	if (eval() != (R_WORD | R_AREA) || eval()) {
		fprintf(stderr, "R input error\n");
		lkerr++;
	}

	/*
	 * Get area pointer
	 */
	aindex = evword();
	if (aindex >= hp->h_narea) {
		fprintf(stderr, "R area error\n");
		lkerr++;
		return;
	}

	/*
	 * Base values
	 */
	rtbase = adw_w(0, 0);
	rtofst = 2;

	/*
	 * Relocate address
	 */
	pc = adw_w(a[aindex]->a_addr, 0);

#ifdef GAMEBOY
	{
		char *s = strrchr(a[aindex]->a_bap->a_id, '_');
		if(s != NULL && isdigit(s[1]))
			current_rom_bank = atoi(s+1);
		else
			current_rom_bank = 0;
	}
#endif /* GAMEBOY */
	/*
	 * Do remaining relocations
	 */
	while (more()) {
		error = 0;
		mode = eval();
		rtp = eval();
		rindex = evword();

		/*
		 * R_SYM or R_AREA references
		 */
		if (mode & R_SYM) {
			if (rindex >= hp->h_nglob) {
				fprintf(stderr, "R symbol error\n");
				lkerr++;
				return;
			}
			reli = symval(s[rindex]);
		} else {
			if (rindex >= hp->h_narea) {
				fprintf(stderr, "R area error\n");
				lkerr++;
				return;
			}
			reli = a[rindex]->a_addr;
		}

		/*
		 * R_PCR addressing
		 */
		if (mode & R_PCR) {
			if (mode & R_BYTE) {
				reli -= (pc + (rtp-rtofst) + 1);
			} else {
				reli -= (pc + (rtp-rtofst) + 2);
			}
		}

		/*
		 * R_PAG0 or R_PAG addressing
		 */
		if (mode & (R_PAG0|R_PAG)) {
			paga  = sdp.s_area->a_addr;
			pags  = sdp.s_addr;
			reli -= paga + pags;
		}

		/*
		 * R_BYTE or R_WORD operation
		 */
		if (mode & R_BYTE) {
			if (mode & R_BYT2) {
				if (mode & R_MSB) {
					relv = adb_hi(reli, rtp);
				} else {
					relv = adb_lo(reli, rtp);
				}
			} else {
				relv = adb_b(reli, rtp);
			}
		} else {
			/*
			 * R_WORD with the R_BYT2 mode is flagged
			 * as an 'r' error by the assembler,
			 * but it is processed here anyway.
			 */
			if (mode & R_BYT2) {
				if (mode & R_MSB) {
					relv = adw_hi(reli, rtp);
				} else {
					relv = adw_lo(reli, rtp);
				}
			} else {
				relv = adw_w(reli, rtp);
			}
		}

		/*
		 * R_BYTE with R_BYT2 offset adjust
		 */
		if (mode & R_BYTE) {
			if (mode & R_BYT2) {
				rtofst += 1;
			}
		}

		/*
		 * Unsigned Byte Checking
		 */
		if (mode & R_USGN && mode & R_BYTE && relv & ~0xFF)
			error = 1;

		/*
		 * PCR Relocation Error Checking
		 */
		if (mode & R_PCR && mode & R_BYTE) {
			r = relv & ~0x7F;
			if (r != (addr_t) ~0x7F && r != 0)
				error = 2;
		}

		/*
		 * Page Relocation Error Checking
		 */
		if (mode & R_PAG0 && (relv & ~0xFF || paga || pags))
			error = 3;
		if (mode & R_PAG  && (relv & ~0xFF))
			error = 4;

		/*
		 * Error Processing
		 */
		if (error) {
			rerr.aindex = aindex;
			rerr.mode = mode;
			rerr.rtbase = rtbase + rtp - rtofst - 1;
			rerr.rindex = rindex;
			rerr.rval = relv - reli;
			relerr(errmsg[error-1]);
		}
	}
	if (uflag != 0) {
		lkulist(1);
	}
	if (oflag == 1) {
		ihx(1);
	} else
	if (oflag == 2) {
		s19(1);
#ifdef SDK
	} else
	if (oflag == 3) {
#ifdef GAMEGEAR
		gg(1);
#endif /* GAMEGEAR */
#ifdef GAMEBOY
		gb(1);
#endif /* GAMEBOY */
#endif /* SDK */
	}
}

char *errmsg[] = {
	"Unsigned Byte error",
	"Byte PCR relocation error",
	"Page0 relocation error",
	"Page Mode relocation error"
};


/*)Function	VOID	relp()
 *
 *	The function relp() evaluates a P line read by
 *	the linker.  The P line data is combined with the
 *	previous T line data to set the base page address
 *	and test the paging boundary and length.
 *
 *		P Line 
 *
 *		P 0 0 nn nn n1 n2 xx xx 
 *
 * 	The  P  line provides the paging information to the linker as
 *	specified by a .setdp directive.  The format of  the  relocation
 *	information is identical to that of the R line.  The correspond-
 *	ing T line has the following information:  
 *		T xx xx aa aa bb bb 
 *
 * 	Where  aa aa is the area reference number which specifies the
 *	selected page area and bb bb is the base address  of  the  page.
 *	bb bb will require relocation processing if the 'n1 n2 xx xx' is
 *	specified in the P line.  The linker will verify that  the  base
 *	address is on a 256 byte boundary and that the page length of an
 *	area defined with the PAG type is not larger than 256 bytes.  
 *
 *	local variable:
 *		areax	**a		pointer to array of area pointers
 *		int	aindex		area index
 *		int	mode		relocation mode
 *		addr_t	relv		relocation value
 *		int	rindex		symbol / area index
 *		int	rtp		index into T data
 *		sym	**s		pointer to array of symbol pointers
 *
 *	global variables:
 *		head	*hp		pointer to the head structure
 *		int	lkerr		error flag
 *		sdp	sdp		base page structure
 *		FILE	*stderr		standard error device
 *
 *	called functions:
 *		addr_t	adw_w()		lkrloc.c
 *		addr_t	evword()	lkrloc.c
 *		int	eval()		lkeval.c
 *		int	fprintf()	c_library
 *		int	more()		lklex.c
 *		int	symval()	lksym.c
 *
 *	side effects:
 *		The P and T lines are combined to set
 *		the base page address and report any
 *		paging errors.
 *
 */

VOID
relp()
{
	register aindex, rindex;
	int mode, rtp;
	addr_t relv;
	struct areax **a;
	struct sym **s;

	/*
	 * Get area and symbol lists
	 */
	a = hp->a_list;
	s = hp->s_list;

	/*
	 * Verify Area Mode
	 */
	if (eval() != (R_WORD | R_AREA) || eval()) {
		fprintf(stderr, "P input error\n");
		lkerr++;
	}

	/*
	 * Get area pointer
	 */
	aindex = evword();
	if (aindex >= hp->h_narea) {
		fprintf(stderr, "P area error\n");
		lkerr++;
		return;
	}

	/*
	 * Do remaining relocations
	 */
	while (more()) {
		mode = eval();
		rtp = eval();
		rindex = evword();

		/*
		 * R_SYM or R_AREA references
		 */
		if (mode & R_SYM) {
			if (rindex >= hp->h_nglob) {
				fprintf(stderr, "P symbol error\n");
				lkerr++;
				return;
			}
			relv = symval(s[rindex]);
		} else {
			if (rindex >= hp->h_narea) {
				fprintf(stderr, "P area error\n");
				lkerr++;
				return;
			}
			relv = a[rindex]->a_addr;
		}
		adw_w(relv, rtp);
	}

	/*
	 * Paged values
	 */
	aindex = adw_w(0,2);
	if (aindex >= hp->h_narea) {
		fprintf(stderr, "P area error\n");
		lkerr++;
		return;
	}
	sdp.s_areax = a[aindex];
	sdp.s_area = sdp.s_areax->a_bap;
	sdp.s_addr = adw_w(0,4);
	if (sdp.s_area->a_addr & 0xFF || sdp.s_addr & 0xFF)
		relerp("Page Definition Boundary Error");
}

/*)Function	VOID	rele()
 *
 *	The function rele() closes all open output files
 *	at the end of the linking process.
 *
 *	local variable:
 *		none
 *
 *	global variables:
 *		int	oflag		output type flag
 *		int	uflag		relocation listing flag
 *
 *	called functions:
 *		VOID	ihx()		lkihx.c
 *		VOID	lkulist()	lklist.c
 *		VOID	s19()		lks19.c
 *
 *	side effects:
 *		All open output files are closed.
 *
 */

VOID
rele()
{
	if (uflag != 0) {
		lkulist(0);
	}
	if (oflag == 1) {
		ihx(0);
	} else
	if (oflag == 2) {
		s19(0);
#ifdef SDK
	} else
	if (oflag == 3) {
#ifdef GAMEGEAR
		gg(0);
#endif /* GAMEGEAR */
#ifdef GAMEBOY
		gb(0);
#endif /* GAMEBOY */
#endif /* SDK */
	}
}

/*)Function	addr_t 	evword()
 *
 *	The function evword() combines two byte values
 *	into a single word value.
 *
 *	local variable:
 *		addr_t	v		temporary evaluation variable
 *
 *	global variables:
 *		hilo			byte ordering parameter
 *
 *	called functions:
 *		int	eval()		lkeval.c
 *
 *	side effects:
 *		Relocation text line is scanned to combine
 *		two byte values into a single word value.
 *
 */

addr_t
evword()
{
	register addr_t v;

	if (hilo) {
		v =  (eval() << 8);
		v +=  eval();
	} else {
		v =   eval();
		v += (eval() << 8);
	}
	return(v);
}

/*)Function	addr_t 	adb_b(v, i)
 *
 *		int	v		value to add to byte
 *		int	i		rtval[] index
 *
 *	The function adb_b() adds the value of v to
 *	the single byte value contained in rtval[i].
 *	The new value of rtval[i] is returned.
 *
 *	local variable:
 *		none
 *
 *	global variables:
 *		none
 *
 *	called functions:
 *		none
 *
 *	side effects:
 *		The value of rtval[] is changed.
 *
 */

addr_t
adb_b(v, i)
register addr_t v;
register int i;
{
	return(rtval[i] += v);
}

/*)Function	addr_t 	adb_lo(v, i)
 *
 *		int	v		value to add to byte
 *		int	i		rtval[] index
 *
 *	The function adb_lo() adds the value of v to the
 *	double byte value contained in rtval[i] and rtval[i+1].
 *	The new value of rtval[i] / rtval[i+1] is returned.
 *	The MSB rtflg[] is cleared.
 *
 *	local variable:
 *		addr_t	j		temporary evaluation variable
 *
 *	global variables:
 *		hilo			byte ordering parameter
 *
 *	called functions:
 *		none
 *
 *	side effects:
 *		The value of rtval[] is changed.
 *		The rtflg[] value corresponding to the
 *		MSB of the word value is cleared to reflect
 *		the fact that the LSB is the selected byte.
 *
 */

addr_t
adb_lo(v, i)
addr_t	v;
int	i;
{
	register addr_t j;

	j = adw_w(v, i);
	/*
	 * Remove Hi byte
	 */
	if (hilo) {
		rtflg[i] = 0;
	} else {
		rtflg[i+1] = 0;
	}
	return (j);
}

/*)Function	addr_t 	adb_hi(v, i)
 *
 *		int	v		value to add to byte
 *		int	i		rtval[] index
 *
 *	The function adb_hi() adds the value of v to the
 *	double byte value contained in rtval[i] and rtval[i+1].
 *	The new value of rtval[i] / rtval[i+1] is returned.
 *	The LSB rtflg[] is cleared.
 *
 *	local variable:
 *		addr_t	j		temporary evaluation variable
 *
 *	global variables:
 *		hilo			byte ordering parameter
 *
 *	called functions:
 *		none
 *
 *	side effects:
 *		The value of rtval[] is changed.
 *		The rtflg[] value corresponding to the
 *		LSB of the word value is cleared to reflect
 *		the fact that the MSB is the selected byte.
 *
 */

addr_t
adb_hi(v, i)
addr_t	v;
int	i;
{
	register addr_t j;

	j = adw_w(v, i);
	/*
	 * Remove Lo byte
	 */
	if (hilo) {
		rtflg[i+1] = 0;
	} else {
		rtflg[i] = 0;
	}
	return (j);
}

/*)Function	addr_t 	adw_w(v, i)
 *
 *		int	v		value to add to word
 *		int	i		rtval[] index
 *
 *	The function adw_w() adds the value of v to the
 *	word value contained in rtval[i] and rtval[i+1].
 *	The new value of rtval[i] / rtval[i+1] is returned.
 *
 *	local variable:
 *		addr_t	j		temporary evaluation variable
 *
 *	global variables:
 *		hilo			byte ordering parameter
 *
 *	called functions:
 *		none
 *
 *	side effects:
 *		The word value of rtval[] is changed.
 *
 */

addr_t
adw_w(v, i)
register addr_t v;
register int i;
{
	register addr_t j;

	if (hilo) {
		j = v + (rtval[i] << 8) + (rtval[i+1] & 0xff);
		rtval[i] = (j >> 8) & 0xff;
		rtval[i+1] = j & 0xff;
	} else {
		j = v + (rtval[i] & 0xff) + (rtval[i+1] << 8);
		rtval[i] = j & 0xff;
		rtval[i+1] = (j >> 8) & 0xff;
	}
	return(j);
}

/*)Function	addr_t 	adw_lo(v, i)
 *
 *		int	v		value to add to byte
 *		int	i		rtval[] index
 *
 *	The function adw_lo() adds the value of v to the
 *	double byte value contained in rtval[i] and rtval[i+1].
 *	The new value of rtval[i] / rtval[i+1] is returned.
 *	The MSB rtval[] is zeroed.
 *
 *	local variable:
 *		addr_t	j		temporary evaluation variable
 *
 *	global variables:
 *		hilo			byte ordering parameter
 *
 *	called functions:
 *		none
 *
 *	side effects:
 *		The value of rtval[] is changed.
 *		The MSB of the word value is cleared to reflect
 *		the fact that the LSB is the selected byte.
 *
 */

addr_t
adw_lo(v, i)
addr_t	v;
int	i;
{
	register addr_t j;

	j = adw_w(v, i);
	/*
	 * Clear Hi byte
	 */
	if (hilo) {
		rtval[i] = 0;
	} else {
		rtval[i+1] = 0;
	}
	return (j);
}

/*)Function	addr_t 	adw_hi(v, i)
 *
 *		int	v		value to add to byte
 *		int	i		rtval[] index
 *
 *	The function adw_hi() adds the value of v to the
 *	double byte value contained in rtval[i] and rtval[i+1].
 *	The new value of rtval[i] / rtval[i+1] is returned.
 *	The MSB and LSB values are interchanged.
 *	The MSB rtval[] is zeroed.
 *
 *	local variable:
 *		addr_t	j		temporary evaluation variable
 *
 *	global variables:
 *		hilo			byte ordering parameter
 *
 *	called functions:
 *		none
 *
 *	side effects:
 *		The value of rtval[] is changed.
 *		The MSB and LSB values are interchanged and
 *		then the MSB cleared.
 *
 */

addr_t
adw_hi(v, i)
addr_t	v;
int	i;
{
	register addr_t j;

	j = adw_w(v, i);
	/*
	 * LSB = MSB, Clear MSB
	 */
	if (hilo) {
		rtval[i+1] = rtval[i];
		rtval[i] = 0;
	} else {
		rtval[i] = rtval[i+1];
		rtval[i+1] = 0;
	}
	return (j);
}

/*)Function	VOID	relerr(str)
 *
 *		char	*str		error string
 *
 *	The function relerr() outputs the error string to
 *	stderr and to the map file (if it is open).
 *
 *	local variable:
 *		none
 *
 *	global variables:
 *		FILE	*mfp		handle for the map file
 *
 *	called functions:
 *		VOID	errdmp()	lkrloc.c
 *
 *	side effects:
 *		Error message inserted into map file.
 *
 */

VOID
relerr(str)
char *str;
{
	errdmp(stderr, str);
	if (mfp)
		errdmp(mfp, str);
}

/*)Function	VOID	errdmp(fptr, str)
 *
 *		FILE	*fptr		output file handle
 *		char	*str		error string
 *
 *	The function errdmp() outputs the error string str
 *	to the device specified by fptr.  Additional information
 *	is output about the definition and referencing of
 *	the symbol / area error.
 *
 *	local variable:
 *		int	mode		error mode
 *		int	aindex		area index
 *		int	lkerr		error flag
 *		int	rindex		error index
 *		sym	**s		pointer to array of symbol pointers
 *		areax	**a		pointer to array of area pointers
 *		areax	*raxp		error area extension pointer
 *
 *	global variables:
 *		sdp	sdp		base page structure
 *
 *	called functions:
 *		int	fprintf()	c_library
 *		VOID	prntval()	lkrloc.c
 *
 *	side effects:
 *		Error reported.
 *
 */

VOID
errdmp(fptr, str)
FILE *fptr;
char *str;
{
	int mode, aindex, rindex;
	struct sym **s;
	struct areax **a;
	struct areax *raxp;

	a = hp->a_list;
	s = hp->s_list;

	mode = rerr.mode;
	aindex = rerr.aindex;
	rindex = rerr.rindex;

	/*
	 * Print Error
	 */
	fprintf(fptr, "\n?ASlink-Warning-%s", str);
	lkerr++;

	/*
	 * Print symbol if symbol based
	 */
	if (mode & R_SYM) {
		fprintf(fptr, " for symbol  %.*s\n",
			NCPS, &s[rindex]->s_id[0]);
	} else {
		fprintf(fptr, "\n");
	}

	/*
	 * Print Ref Info
	 */
	fprintf(fptr,
		"         file        module      area        offset\n");
	fprintf(fptr,
		"  Refby  %-8.8s    %-8.8s    %-8.8s    ",
			hp->h_lfile->f_idp,
			&hp->m_id[0],
			&a[aindex]->a_bap->a_id[0]);
	prntval(fptr, rerr.rtbase);

	/*
	 * Print Def Info
	 */
	if (mode & R_SYM) {
		raxp = s[rindex]->s_axp;
	} else {
		raxp = a[rindex];
	}
	fprintf(fptr,
		"  Defin  %-8.8s    %-8.8s    %-8.8s    ",
			raxp->a_bhp->h_lfile->f_idp,
			&raxp->a_bhp->m_id[0],
			&raxp->a_bap->a_id[0]);
	if (mode & R_SYM) {
		prntval(fptr, s[rindex]->s_addr);
	} else {
		prntval(fptr, rerr.rval);
	}
}

/*)Function	VOID	prntval(fptr, v)
 *
 *		FILE	*fptr		output file handle
 *		addr_t	v		value to output
 *
 *	The function prntval() outputs the value v, in the
 *	currently selected radix, to the device specified
 *	by fptr.
 *
 *	local variable:
 *		none
 *
 *	global variables:
 *		int	xflag		current radix
 *
 *	called functions:
 *		int	fprintf()	c_library
 *
 *	side effects:
 *		none
 *
 */

VOID
prntval(fptr, v)
FILE *fptr;
addr_t v;
{
	if (xflag == 0) {
		fprintf(fptr, "%04X\n", v);
	} else
	if (xflag == 1) {
		fprintf(fptr, "%06o\n", v);
	} else
	if (xflag == 2) {
		fprintf(fptr, "%05u\n", v);
	}
}

/*)Function	VOID	relerp(str)
 *
 *		char	*str		error string
 *
 *	The function relerp() outputs the paging error string to
 *	stderr and to the map file (if it is open).
 *
 *	local variable:
 *		none
 *
 *	global variables:
 *		FILE	*mfp		handle for the map file
 *
 *	called functions:
 *		VOID	erpdmp()	lkrloc.c
 *
 *	side effects:
 *		Error message inserted into map file.
 *
 */

VOID
relerp(str)
char *str;
{
	erpdmp(stderr, str);
	if (mfp)
		erpdmp(mfp, str);
}

/*)Function	VOID	erpdmp(fptr, str)
 *
 *		FILE	*fptr		output file handle
 *		char	*str		error string
 *
 *	The function erpdmp() outputs the error string str
 *	to the device specified by fptr.
 *
 *	local variable:
 *		head	*thp		pointer to head structure
 *
 *	global variables:
 *		int	lkerr		error flag
 *		sdp	sdp		base page structure
 *
 *	called functions:
 *		int	fprintf()	c_library
 *		VOID	prntval()	lkrloc.c
 *
 *	side effects:
 *		Error reported.
 *
 */

VOID
erpdmp(fptr, str)
FILE *fptr;
char *str;
{
	register struct head *thp;

	thp = sdp.s_areax->a_bhp;

	/*
	 * Print Error
	 */
	fprintf(fptr, "\n?ASlink-Warning-%s\n", str);
	lkerr++;

	/*
	 * Print PgDef Info
	 */
	fprintf(fptr,
		"         file        module      pgarea      pgoffset\n");
	fprintf(fptr,
		"  PgDef  %-8.8s    %-8.8s    %-8.8s    ",
			thp->h_lfile->f_idp,
			&thp->m_id[0],
			&sdp.s_area->a_id[0]);
	prntval(fptr, sdp.s_area->a_addr + sdp.s_addr);
}
