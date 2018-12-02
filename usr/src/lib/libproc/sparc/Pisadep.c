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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/stack.h>
#include <sys/regset.h>
#include <sys/frame.h>
#include <sys/sysmacros.h>
#include <sys/machelf.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "Pcontrol.h"
#include "Pstack.h"
#include "Pisadep.h"

#define	SYSCALL32 0x91d02008	/* 32-bit syscall (ta 8) instruction */

#ifndef	WINDOWSIZE32
#define	WINDOWSIZE32	(16 * sizeof (int32_t))
#endif

const char *
Ppltdest(struct ps_prochandle *P, uintptr_t pltaddr)
{
	map_info_t *mp = Paddr2mptr(P, pltaddr);

	uintptr_t r_addr;
	file_info_t *fp;
	Elf32_Rela r;
	size_t i;

	if (mp == NULL || (fp = mp->map_file) == NULL ||
	    fp->file_plt_base == 0 || pltaddr < fp->file_plt_base ||
	    pltaddr >= fp->file_plt_base + fp->file_plt_size) {
		errno = EINVAL;
		return (NULL);
	}

	i = (pltaddr - fp->file_plt_base -
	    M_PLT_XNumber * M32_PLT_ENTSIZE) / M32_PLT_ENTSIZE;

	r_addr = fp->file_jmp_rel + i * sizeof (Elf32_Rela);

	if (Pread(P, &r, sizeof (r), r_addr) == sizeof (r) &&
	    (i = ELF32_R_SYM(r.r_info)) < fp->file_dynsym.sym_symn) {

		Elf_Data *data = fp->file_dynsym.sym_data_pri;
		Elf32_Sym *symp = &(((Elf32_Sym *)data->d_buf)[i]);

		return (fp->file_dynsym.sym_strs + symp->st_name);
	}

	return (NULL);
}

int
Pissyscall(struct ps_prochandle *P, uintptr_t addr)
{
	instr_t sysinstr;
	instr_t instr;

	sysinstr = SYSCALL32;

	if (Pread(P, &instr, sizeof (instr), addr) != sizeof (instr) ||
	    instr != sysinstr)
		return (0);
	else
		return (1);
}

int
Pissyscall_prev(struct ps_prochandle *P, uintptr_t addr, uintptr_t *dst)
{
	uintptr_t prevaddr = addr - sizeof (instr_t);

	if (Pissyscall(P, prevaddr)) {
		if (dst)
			*dst = prevaddr;
		return (1);
	}

	return (0);
}

/* ARGSUSED */
int
Pissyscall_text(struct ps_prochandle *P, const void *buf, size_t buflen)
{
	instr_t sysinstr;

	sysinstr = SYSCALL32;

	if (buflen >= sizeof (instr_t) &&
	    memcmp(buf, &sysinstr, sizeof (instr_t)) == 0)
		return (1);
	else
		return (0);
}

/*
 * For gwindows_t support, we define a structure to pass arguments to
 * a Plwp_iter() callback routine.
 */
typedef struct {
	struct ps_prochandle *gq_proc;	/* libproc handle */
	struct rwindow *gq_rwin;	/* rwindow destination buffer */
	uintptr_t gq_addr;		/* stack address to match */
} gwin_query_t;

static int
find_gwin(gwin_query_t *gqp, const lwpstatus_t *psp)
{
	gwindows_t gwin;
	struct stat64 st;
	char path[64];
	ssize_t n;
	int fd, i;
	int rv = 0; /* Return value for skip to next lwp */

	(void) snprintf(path, sizeof (path), "/proc/%d/lwp/%d/gwindows",
	    (int)gqp->gq_proc->pid, (int)psp->pr_lwpid);

	if (stat64(path, &st) == -1 || st.st_size == 0)
		return (0); /* Nothing doing; skip to next lwp */

	if ((fd = open64(path, O_RDONLY)) >= 0) {
		/*
		 * Zero out the gwindows_t because the gwindows file only has
		 * as much data as needed to represent the saved windows.
		 */
		(void) memset(&gwin, 0, sizeof (gwin));
		n = read(fd, &gwin, sizeof (gwin));

		if (n > 0) {
			/*
			 * If we actually found a non-zero gwindows file and
			 * were able to read it, iterate through the buffers
			 * looking for a stack pointer match; if one is found,
			 * copy out the corresponding register window.
			 */
			for (i = 0; i < gwin.wbcnt; i++) {
				if (gwin.spbuf[i] == (greg_t *)gqp->gq_addr) {
					(void) memcpy(gqp->gq_rwin,
					    &gwin.wbuf[i],
					    sizeof (struct rwindow));

					rv = 1; /* We're done */
					break;
				}
			}
		}
		(void) close(fd);
	}

	return (rv);
}

static int
read_gwin(struct ps_prochandle *P, struct rwindow *rwp, uintptr_t sp)
{
	gwin_query_t gq;

	if (P->state == PS_DEAD) {
		core_info_t *core = P->data;
		lwp_info_t *lwp = list_next(&core->core_lwp_head);
		uint_t n;
		int i;

		for (n = 0; n < core->core_nlwp; n++, lwp = list_next(lwp)) {
			gwindows_t *gwin = lwp->lwp_gwins;

			if (gwin == NULL)
				continue; /* No gwindows for this lwp */

			/*
			 * If this lwp has gwindows associated with it, iterate
			 * through the buffers looking for a stack pointer
			 * match; if one is found, copy out the register window.
			 */
			for (i = 0; i < gwin->wbcnt; i++) {
				if (gwin->spbuf[i] == (greg_t *)sp) {
					(void) memcpy(rwp, &gwin->wbuf[i],
					    sizeof (struct rwindow));
					return (0); /* We're done */
				}
			}
		}

		return (-1); /* No gwindows match found */

	}

	gq.gq_proc = P;
	gq.gq_rwin = rwp;
	gq.gq_addr = sp;

	return (Plwp_iter(P, (proc_lwp_f *)find_gwin, &gq) ? 0 : -1);
}

static void
ucontext_n_to_prgregs(const ucontext_t *src, prgregset_t dst)
{
	const greg_t *gregs = &src->uc_mcontext.gregs[0];

	dst[R_PSR] = gregs[REG_PSR];
	dst[R_PC] = gregs[REG_PC];
	dst[R_nPC] = gregs[REG_nPC];
	dst[R_Y] = gregs[REG_Y];

	dst[R_G1] = gregs[REG_G1];
	dst[R_G2] = gregs[REG_G2];
	dst[R_G3] = gregs[REG_G3];
	dst[R_G4] = gregs[REG_G4];
	dst[R_G5] = gregs[REG_G5];
	dst[R_G6] = gregs[REG_G6];
	dst[R_G7] = gregs[REG_G7];

	dst[R_O0] = gregs[REG_O0];
	dst[R_O1] = gregs[REG_O1];
	dst[R_O2] = gregs[REG_O2];
	dst[R_O3] = gregs[REG_O3];
	dst[R_O4] = gregs[REG_O4];
	dst[R_O5] = gregs[REG_O5];
	dst[R_O6] = gregs[REG_O6];
	dst[R_O7] = gregs[REG_O7];
}

int
Pstack_iter(struct ps_prochandle *P, const prgregset_t regs,
	proc_stack_f *func, void *arg)
{
	prgreg_t *prevfp = NULL;
	uint_t pfpsize = 0;
	int nfp = 0;
	prgregset_t gregs;
	long args[6];
	prgreg_t fp;
	int i;
	int rv;
	uintptr_t sp;
	ssize_t n;
	uclist_t ucl;
	ucontext_t uc;

	init_uclist(&ucl, P);
	(void) memcpy(gregs, regs, sizeof (gregs));

	for (;;) {
		fp = gregs[R_FP];
		if (stack_loop(fp, &prevfp, &nfp, &pfpsize))
			break;

		for (i = 0; i < 6; i++)
			args[i] = gregs[R_I0 + i];
		if ((rv = func(arg, gregs, 6, args)) != 0)
			break;

		gregs[R_PC] = gregs[R_I7];
		gregs[R_nPC] = gregs[R_PC] + 4;
		(void) memcpy(&gregs[R_O0], &gregs[R_I0], 8*sizeof (prgreg_t));
		if ((sp = gregs[R_FP]) == 0)
			break;

		sp += STACK_BIAS;

		if (find_uclink(&ucl, sp + SA(sizeof (struct frame))) &&
		    Pread(P, &uc, sizeof (uc), sp +
		    SA(sizeof (struct frame))) == sizeof (uc)) {
			ucontext_n_to_prgregs(&uc, gregs);
			sp = gregs[R_SP] + STACK_BIAS;
		}

		n = Pread(P, &gregs[R_L0], sizeof (struct rwindow), sp);

		if (n == sizeof (struct rwindow))
			continue;

		/*
		 * If we get here, then our Pread of the register window
		 * failed.  If this is because the address was not mapped,
		 * then we attempt to read this window via any gwindows
		 * information we have.  If that too fails, abort our loop.
		 */
		if (n > 0)
			break;	/* Failed for reason other than not mapped */

		if (read_gwin(P, (struct rwindow *)&gregs[R_L0], sp) == -1)
			break;	/* No gwindows match either */
	}

	if (prevfp)
		free(prevfp);

	free_uclist(&ucl);
	return (rv);
}

uintptr_t
Psyscall_setup(struct ps_prochandle *P, int nargs, int sysindex, uintptr_t sp)
{
	sp -= (nargs > 6)?
	    WINDOWSIZE32 + sizeof (int32_t) * (1 + nargs) :
	    WINDOWSIZE32 + sizeof (int32_t) * (1 + 6);
	sp = PSTACK_ALIGN32(sp);

	P->status.pr_lwp.pr_reg[R_G1] = sysindex;
	P->status.pr_lwp.pr_reg[R_SP] = sp;
	P->status.pr_lwp.pr_reg[R_PC] = P->sysaddr;
	P->status.pr_lwp.pr_reg[R_nPC] = P->sysaddr + sizeof (instr_t);

	return (sp + WINDOWSIZE32 + sizeof (int32_t));
}

int
Psyscall_copyinargs(struct ps_prochandle *P, int nargs, argdes_t *argp,
    uintptr_t ap)
{
	uint32_t arglist[MAXARGS+2];
	int i;
	argdes_t *adp;

	for (i = 0, adp = argp; i < nargs; i++, adp++) {
		arglist[i] = adp->arg_value;

		if (i < 6)
			(void) Pputareg(P, R_O0+i, adp->arg_value);
	}

	if (nargs > 6 &&
	    Pwrite(P, &arglist[0], sizeof (int32_t) * nargs,
	    (uintptr_t)ap) != sizeof (int32_t) * nargs)
		return (-1);

	return (0);
}

/* ARGSUSED */
int
Psyscall_copyoutargs(struct ps_prochandle *P, int nargs, argdes_t *argp,
    uintptr_t ap)
{
	/* Do nothing */
	return (0);
}