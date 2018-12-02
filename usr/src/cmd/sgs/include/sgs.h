/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 *	Copyright (c) 1988 AT&T
 *	  All Rights Reserved
 *
 *
 * Copyright (c) 1989, 2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Global include file for all sgs.
 */

#ifndef	_SGS_H
#define	_SGS_H

#ifdef	__cplusplus
extern "C" {
#endif

/* <assert.h> keys off of NDEBUG */
#ifdef	DEBUG
#undef	NDEBUG
#else
#define	NDEBUG
#endif

#ifndef	_ASM
#include <sys/types.h>
#include <sys/machelf.h>
#include <stdlib.h>
#include <stdarg.h>
#include <libelf.h>
#include <assert.h>
#include <alist.h>
#endif	/* _ASM */

/*
 * Software identification.
 */
#define	SGS		""
#define	SGU_PKG		"Software Generation Utilities"
#define	SGU_REL		"(SGU) Solaris-ELF (4.0)"


#ifndef _ASM

/*
 * link_ver_string[] contains a version string for use by the link-editor
 * and all other linker components. It is found in libconv, and is
 * generated by sgs/libconv/common/bld_vernote.ksh. That script produces
 * libconv/{plat}/vernote.s, which is in turn assembled/linked into
 * libconv.
 */
extern const char link_ver_string[];
/*
 * Macro to round to next double word boundary.
 */
#define	S_DROUND(x)	(((x) + sizeof (double) - 1) & ~(sizeof (double) - 1))

/*
 * General align and round macros.
 */
#define	S_ALIGN(x, a)	((x) & ~(((a) ? (a) : 1) - 1))
#define	S_ROUND(x, a)   ((x) + (((a) ? (a) : 1) - 1) & ~(((a) ? (a) : 1) - 1))

/*
 * Bit manipulation macros; generic bit mask and is `v' in the range
 * supportable in `n' bits?
 */
#define	S_MASK(n)	((1 << (n)) -1)
#define	S_INRANGE(v, n)	(((-(1 << (n)) - 1) < (v)) && ((v) < (1 << (n))))


/*
 * Yet another definition of the OFFSETOF macro, used with the AVL routines.
 */
#define	SGSOFFSETOF(s, m)	((size_t)(&(((s *)0)->m)))

/*
 * When casting between integer and pointer types, gcc will complain
 * if the integer type used is not large enough to hold the pointer
 * value without loss. Although a dubious practice in general, this
 * is sometimes done by design. In those cases, the general solution
 * is to introduce an intermediate cast to widen the integer value. The
 * CAST_PTRINT macro does this, and its use documents the fact that
 * the programmer is doing that sort of cast.
 */
#ifdef __GNUC__
#define	CAST_PTRINT(cast, value) ((cast)(uintptr_t)value)
#else
#define	CAST_PTRINT(cast, value) ((cast)value)
#endif

/*
 * General typedefs.
 */
typedef enum {
	FALSE = 0,
	TRUE = 1
} Boolean;

/*
 * Types of errors (used by veprintf()), together with a generic error return
 * value.
 */
typedef enum {
	ERR_NONE,		/* plain message */
	ERR_WARNING_NF,		/* warning that cannot be promoted to fatal */
	ERR_WARNING,		/* warning that can be promoted to fatal */
	ERR_GUIDANCE,		/* guidance warning that can be promoted */
	ERR_FATAL,		/* fatal error */
	ERR_ELF,		/* fatal libelf error */
	ERR_NUM			/* # of Error codes. Must be last */
} Error;

/*
 * Type used to represent line numbers within files, and a corresponding
 * printing macro for it.
 */
typedef ulong_t Lineno;
#define	EC_LINENO(_x) EC_XWORD(_x)			/* "llu" */


#if defined(_LP64) && !defined(_ELF64)
#define	S_ERROR		(~(uint_t)0)
#else
#define	S_ERROR		(~(uintptr_t)0)
#endif

/*
 * CTF currently does not handle automatic array variables sized via function
 * arguments (VLA arrays) properly, when the code is compiled with gcc.
 * Adding 1 to the size is a workaround.  VLA_SIZE, and its use, should be
 * pulled when CTF is fixed or replaced.
 */
#ifdef __GNUC__
#define	VLA_SIZE(_arg)	((_arg) + 1)
#else
#define	VLA_SIZE(_arg)	(_arg)
#endif

/*
 * Structure to maintain rejected files elf information.  Files that are not
 * applicable to the present link-edit are rejected and a search for an
 * appropriate file may be resumed.  The first rejected files information is
 * retained so that a better error diagnostic can be given should an appropriate
 * file not be located.
 */
typedef struct {
	ushort_t	rej_type;	/* SGS_REJ_ value */
	ushort_t	rej_flags;	/* additional information */
	uint_t		rej_info;	/* numeric and string information */
	const char	*rej_str;	/*	associated with error */
	const char	*rej_name;	/* object name - expanded library */
					/*	name and archive members */
} Rej_desc;

#define	SGS_REJ_NONE		0
#define	SGS_REJ_MACH		1	/* wrong ELF machine type */
#define	SGS_REJ_CLASS		2	/* wrong ELF class (32-bit/64-bit) */
#define	SGS_REJ_DATA		3	/* wrong ELF data format (MSG/LSB) */
#define	SGS_REJ_TYPE		4	/* bad ELF type */
#define	SGS_REJ_BADFLAG		5	/* bad ELF flags value */
#define	SGS_REJ_MISFLAG		6	/* mismatched ELF flags value */
#define	SGS_REJ_VERSION		7	/* mismatched ELF/lib version */
#define	SGS_REJ_HAL		8	/* HAL R1 extensions required */
#define	SGS_REJ_US3		9	/* Sun UltraSPARC III extensions */
					/*	required */
#define	SGS_REJ_STR		10	/* generic error - info is a string */
#define	SGS_REJ_UNKFILE		11	/* unknown file type */
#define	SGS_REJ_UNKCAP		12	/* unknown capabilities */
#define	SGS_REJ_HWCAP_1		13	/* hardware capabilities mismatch */
#define	SGS_REJ_SFCAP_1		14	/* software capabilities mismatch */
#define	SGS_REJ_MACHCAP		15	/* machine capability mismatch */
#define	SGS_REJ_PLATCAP		16	/* platform capability mismatch */
#define	SGS_REJ_HWCAP_2		17	/* hardware capabilities mismatch */
#define	SGS_REJ_ARCHIVE		18	/* archive used in invalid context */
#define	SGS_REJ_NUM		19

#define	FLG_REJ_ALTER		0x01	/* object name is an alternative */

/*
 * For those source files used both inside and outside of the
 * libld source base (tools/common/string_table.c) we can
 * automatically switch between the allocation models
 * based off of the 'cc -DUSE_LIBLD_MALLOC' flag.
 */
#ifdef	USE_LIBLD_MALLOC
#define	calloc(x, a)		libld_malloc(((size_t)x) * ((size_t)a))
#define	free			libld_free
#define	malloc			libld_malloc
#define	realloc			libld_realloc

#define	libld_calloc(x, a)	libld_malloc(((size_t)x) * ((size_t)a))
extern void			libld_free(void *);
extern void			*libld_malloc(size_t);
extern void			*libld_realloc(void *, size_t);
#endif

/*
 * Data structures (defined in libld.h).
 */
typedef	struct audit_desc	Audit_desc;
typedef	struct audit_info	Audit_info;
typedef	struct audit_list	Audit_list;
typedef struct cap_desc		Cap_desc;
typedef struct ent_desc		Ent_desc;
typedef	struct group_desc	Group_desc;
typedef struct ifl_desc		Ifl_desc;
typedef struct is_desc		Is_desc;
typedef struct isa_desc		Isa_desc;
typedef struct isa_opt		Isa_opt;
typedef struct os_desc		Os_desc;
typedef struct ofl_desc		Ofl_desc;
typedef	struct rel_cache	Rel_cache;
typedef	struct rel_cachebuf	Rel_cachebuf;
typedef	struct rel_aux_cachebuf	Rel_aux_cachebuf;
typedef struct rel_aux		Rel_aux;
typedef struct rel_desc		Rel_desc;
typedef	struct sdf_desc		Sdf_desc;
typedef	struct sdv_desc		Sdv_desc;
typedef struct sec_order	Sec_order;
typedef struct sg_desc		Sg_desc;
typedef struct sort_desc	Sort_desc;
typedef	struct sym_avlnode	Sym_avlnode;
typedef struct sym_aux		Sym_aux;
typedef struct sym_desc		Sym_desc;
typedef	struct uts_desc		Uts_desc;
typedef struct ver_desc		Ver_desc;
typedef struct ver_index	Ver_index;

/*
 * Data structures defined in rtld.h.
 */
typedef struct lm_list		Lm_list;
#ifdef _SYSCALL32
typedef struct lm_list32	Lm_list32;
#endif	/* _SYSCALL32 */

/*
 * For the various utilities that include sgs.h
 */
extern int	assfail(const char *, const char *, int);
extern void	eprintf(Lm_list *, Error, const char *, ...);
extern void	veprintf(Lm_list *, Error, const char *, va_list);
extern uint_t	sgs_str_hash(const char *);
extern uint_t	findprime(uint_t);

#endif /* _ASM */

#ifdef	__cplusplus
}
#endif


#endif /* _SGS_H */
