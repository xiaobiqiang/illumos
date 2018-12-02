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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * Add TSOL banner, trailer, page header/footers to a print job
 */

/* system header files */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <locale.h>
#include <tsol/label.h>

/* typedefs */

typedef int BOOL;

/* constants */

#ifndef FALSE
#define	FALSE 0
#endif
#ifndef TRUE
#define	TRUE 1
#endif

#define	ME "lp.tsol_separator"
#define	POSTSCRIPTLIB "/usr/lib/lp/postscript"
#define	SEPARATORPS "tsol_separator.ps"
#define	BANNERPS "tsol_banner.ps"
#define	TRAILERPS "tsol_trailer.ps"
#define	MAXUSERLEN 32
#define	MAXHOSTLEN 32

/* external variables */

int	optind;			/* Used by getopt */
char    *optarg;		/* Used by getopt */

/* prototypes for static functions */

static int ProcessArgs(int argc, char **argv);
static void Usage(void);
static void ParseUsername(char *input, char *user, char *host);
static void EmitPSFile(const char *name);
static BOOL EmitFile(FILE *file);
static void EmitJobData(void);
static void EmitPrologue(void);
static void EmitCommandLineInfo(void);
static void EmitClockBasedInfo(void);
static void EmitLabelInfo(void);
static void CopyStdin(void);

/* static variables */

static char *ArgSeparatorPS;
static char *ArgBannerPS;
static char *ArgTrailerPS;
static char *ArgPSLib;
static char *ArgPrinter;
static char *ArgJobID;
static char *ArgUser;
static char *ArgTitle;
static char *ArgFile;
static BOOL ArgReverse;
static BOOL ArgNoPageLabels;
static int ArgDebugLevel;
static FILE *ArgLogFile;
static m_label_t *FileLabel;
static char *remoteLabel;

int
main(int argc, char *argv[])
{
	int	err;
	/*
	 * Run immune from typical interruptions, so that
	 * we stand a chance to get the fault message.
	 * EOF (or startup error) is the only way out.
	 */
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTERM, SIG_IGN);

	(void) setlocale(LC_ALL, "");
#if !defined(TEXT_DOMAIN)
#define	TEXT_DOMAIN "SYS_TEST"
#endif
	(void) textdomain(TEXT_DOMAIN);

	if (ProcessArgs(argc, argv) != 0)
		exit(1);

	if ((FileLabel = m_label_alloc(MAC_LABEL)) == NULL)
		exit(1);
	/*
	 * If the job was submitted via remotely, the label of the
	 * remote peer will be set in the SLABEL environment variable
	 * by copying it out of the SECURE structure.
	 *
	 * If there is no SLABEL value, the job was submitted locally
	 * via the named pipe, and the file label can be determined
	 * from its pathname.
	 */
	if ((remoteLabel = getenv("SLABEL")) != NULL) {
		m_label_free(FileLabel);
		FileLabel = NULL;
		if (str_to_label(remoteLabel, &FileLabel, MAC_LABEL,
		    L_NO_CORRECTION, &err) == -1) {
			perror("str_to_label");
			exit(1);
		}
	} else if (getlabel(ArgFile, FileLabel) != 0) {
		(void) fprintf(ArgLogFile,
		    gettext("%1$s: cannot get label of %2$s: %3$s\n"),
		    ME, ArgFile, strerror(errno));
		exit(1);
	}

	/* All of these functions exit if they encounter an error */
	EmitJobData();
	EmitPSFile(ArgSeparatorPS);
	if (ArgReverse)
		EmitPSFile(ArgTrailerPS);
	else
		EmitPSFile(ArgBannerPS);
	CopyStdin();
	if (ArgReverse)
		EmitPSFile(ArgBannerPS);
	else
		EmitPSFile(ArgTrailerPS);
	if (ArgDebugLevel >= 1)
		(void) fprintf(ArgLogFile, gettext("Done.\n"));
	m_label_free(FileLabel);
	return (0);
}

static void
EmitJobData(void)
{
	EmitPrologue();
	EmitCommandLineInfo();
	EmitClockBasedInfo();
	EmitLabelInfo();

	/* Emit ending PostScript code */
	(void) printf("end\n\n");
	(void) printf("%%%% End of code generated by lp.tsol_separator\n\n");

}

static void
EmitPrologue(void)
{
	/* Emit preliminary PostScript code */
	(void) printf("%%!\n\n");
	(void) printf("%%%% Begin code generated by lp.tsol_separator\n\n");

	(void) printf("%%%% Create JobDict if it doesn't exist\n");
	(void) printf("userdict /JobDict known not {\n");
	(void) printf("  userdict /JobDict 100 dict put\n");
	(void) printf("} if\n\n");

	(void) printf("%%%% Define job parameters, including TSOL security "
	    "info\n");
	(void) printf("JobDict\n");
	(void) printf("begin\n");
}

/* Emit parameters obtained from command line options */

static void
EmitCommandLineInfo(void)
{
	char user[MAXUSERLEN + 1];
	char host[MAXHOSTLEN + 1];

	(void) printf("\t/Job_Printer (%s) def\n", ArgPrinter);
	ParseUsername(ArgUser, user, host);
	(void) printf("\t/Job_Host (%s) def\n", host);
	(void) printf("\t/Job_User (%s) def\n", user);
	(void) printf("\t/Job_JobID (%s) def\n", ArgJobID);
	(void) printf("\t/Job_Title (%s) def\n", ArgTitle);
	(void) printf("\t/Job_DoPageLabels (%s) def\n",
	    ArgNoPageLabels ? "NO" : "YES");
	(void) printf("\n");
}

/* Emit parameters generated from the system clock */

static void
EmitClockBasedInfo(void)
{
	char timebuf[80];
	struct timeval clockval;

	(void) gettimeofday(&clockval, NULL);
	(void) strftime(timebuf, sizeof (timebuf), NULL,
	    localtime(&clockval.tv_sec));
	(void) printf("\t/Job_Date (%s) def\n", timebuf);
	(void) printf("\t/Job_Hash (%ld) def\n", clockval.tv_usec % 100000L);
	(void) printf("\n");
}

/* Emit parameters derived from the SL and IL of the file being printed. */

static void
EmitLabelInfo(void)
{
	char	*header = NULL;		/* DIA banner page fields */
	char	*label = NULL;
	char	*caveats = NULL;
	char	*channels = NULL;
	char	*page_label = NULL;	/* interior pages label */

	if (label_to_str(FileLabel, &header, PRINTER_TOP_BOTTOM,
	    DEF_NAMES) != 0) {
		(void) fprintf(ArgLogFile,
		    gettext("%s: label_to_str PRINTER_TOP_BOTTOM: %s.\n"),
		    ME, strerror(errno));
		exit(1);
	}
	if (label_to_str(FileLabel, &label, PRINTER_LABEL,
	    DEF_NAMES) != 0) {
		(void) fprintf(ArgLogFile,
		    gettext("%s: label_to_str PRINTER_LABEL: %s.\n"),
		    ME, strerror(errno));
		exit(1);
	}
	if (label_to_str(FileLabel, &caveats, PRINTER_CAVEATS,
	    DEF_NAMES) != 0) {
		(void) fprintf(ArgLogFile,
		    gettext("%s: label_to_str PRINTER_CAVEATS: %s.\n"),
		    ME, strerror(errno));
		exit(1);
	}
	if (label_to_str(FileLabel, &channels, PRINTER_CHANNELS,
	    DEF_NAMES) != 0) {
		(void) fprintf(ArgLogFile,
		    gettext("%s: label_to_str PRINTER_CHANNELS: %s.\n"),
		    ME, strerror(errno));
		exit(1);
	}
	if (label_to_str(FileLabel, &page_label, M_LABEL,
	    LONG_NAMES) != 0) {
		(void) fprintf(ArgLogFile,
		    gettext("%s: label_to_str M_LABEL: %s.\n"),
		    ME, strerror(errno));
		exit(1);
	}

	(void) printf("\t/Job_Classification (%s) def\n", header);
	(void) printf("\t/Job_Protect (%s) def\n", label);
	(void) printf("\t/Job_Caveats (%s) def\n", caveats);
	(void) printf("\t/Job_Channels (%s) def\n", channels);
	(void) printf("\t/Job_SL_Internal (%s) def\n", page_label);

	/* Free memory allocated label_to_str */
	free(header);
	free(label);
	free(caveats);
	free(channels);
	free(page_label);
}

/*
 * Parse input "host!user" to separate host and user names.
 */

static void
ParseUsername(char *input, char *user, char *host)
{
	char *cp;

	if ((cp = strchr(input, '@')) != NULL) {
		/* user@host */
		(void) strlcpy(host, cp + 1, MAXHOSTLEN + 1);
		*cp = '\0';
		(void) strlcpy(user, input, MAXUSERLEN + 1);
		*cp = '@';
	} else if ((cp = strchr(input, '!')) != NULL) {
		/* host!user */
		(void) strlcpy(user, cp + 1, MAXUSERLEN + 1);
		*cp = '\0';
		(void) strlcpy(host, input, MAXHOSTLEN + 1);
		*cp = '!';
	} else {
		/* user */
		(void) strlcpy(user, input, MAXUSERLEN + 1);
		host[0] = '\0';
	}
}


static void
CopyStdin(void)
{
	if (!EmitFile(stdin)) {
		(void) fprintf(ArgLogFile,
		    gettext("%s: Error copying stdin to stdout\n"), ME);
		exit(1);
	}
}


static BOOL
EmitFile(FILE *file)
{
	int len;
#define	BUFLEN 1024
	char buf[BUFLEN];

	while ((len = fread(buf, 1, BUFLEN, file)) > 0) {
		if (fwrite(buf, 1, len, stdout) != len)
			return (FALSE);
	}
	if (!feof(file))
		return (FALSE);
	return (TRUE);
}


static void
EmitPSFile(const char *name)
{
	char path[PATH_MAX];
	FILE *file;
	BOOL emitted;

	if (name[0] != '/') {
		(void) strlcpy(path, ArgPSLib, sizeof (path));
		(void) strlcat(path, "/", sizeof (path));
		(void) strlcat(path, name, sizeof (path));
	} else {
		(void) strlcpy(path, name, sizeof (path));
	}

	file = fopen(path, "r");
	if (file == NULL) {
		(void) fprintf(ArgLogFile,
		    gettext("%s: Error opening PostScript file %s. %s.\n"),
		    ME, path, strerror(errno));
		exit(1);
	}

	emitted = EmitFile(file);
	(void) fclose(file);
	if (!emitted) {
		(void) fprintf(ArgLogFile, gettext(
		    "%s: Error copying PostScript file %s to stdout.\n"),
		    ME, path);
		exit(1);
	}
}


static int
ProcessArgs(int argc, char *argv[])
{
	int	option_letter;
	char	*options_string = "lrd:e:s:b:t:L:";

	/* set default values for arguments */
	ArgSeparatorPS = SEPARATORPS;
	ArgBannerPS = BANNERPS;
	ArgTrailerPS = TRAILERPS;
	ArgPSLib = POSTSCRIPTLIB;
	ArgNoPageLabels = ArgReverse = FALSE;
	ArgDebugLevel = 0;
	ArgLogFile = stderr;

	/* read switch arguments once to get error log file */
	while ((option_letter = getopt(argc, argv, options_string)) != EOF) {
		switch (option_letter) {
		case 'd':
			ArgDebugLevel = atoi(optarg);
			break;
		case 'e':
			ArgLogFile = fopen(optarg, "a");
			if (ArgLogFile == NULL) {
				(void) fprintf(stderr,
				    gettext("Cannot open log file %s\n"),
				    optarg);
				return (-1);
			}
			break;
		case '?':	/* ? or unrecognized option */
			Usage();
			return (-1);
		}
	}

	if (ArgDebugLevel > 0)
		(void) fprintf(ArgLogFile,
		    gettext("Processing switch arguments\n"));

	/* re-read switch arguments */
	optind = 1;
	while ((option_letter = getopt(argc, argv, options_string)) != EOF) {
		switch (option_letter) {
		case 'd':
			ArgDebugLevel = atoi(optarg);
			break;
		case 'e':
			/* This was handled in earlier pass through args */
			break;
		case 'l':
			ArgNoPageLabels = TRUE;
			break;
		case 'r':
			ArgReverse = TRUE;
			break;
		case 's':
			ArgSeparatorPS = optarg;
			break;
		case 'b':
			ArgBannerPS = optarg;
			break;
		case 't':
			ArgTrailerPS = optarg;
			break;
		case 'L':
			ArgPSLib = optarg;
			break;
		case '?':	/* ? or unrecognized option */
			Usage();
			return (-1);
		}
	}

	/* Adjust arguments to skip over options */
	argc -= optind;		/* Number of remaining(non-switch) args */
	argv += optind;		/* argv[0] is first(non-switch) args */

	if (argc != 5) {
		(void) fprintf(ArgLogFile,
		    gettext("Wrong number of arguments.\n\n"));
		Usage();
		return (-1);
	}

	ArgPrinter = argv++[0];
	ArgJobID = argv++[0];
	ArgUser = argv++[0];
	ArgTitle = argv++[0];
	ArgFile = argv++[0];

	if (ArgDebugLevel >= 1) {
		(void) fprintf(ArgLogFile, gettext("Arguments processed\n"));
		(void) fprintf(ArgLogFile, gettext("Printer: %s\n"),
		    ArgPrinter);
		(void) fprintf(ArgLogFile, gettext("Job ID: %s\n"), ArgJobID);
		(void) fprintf(ArgLogFile, gettext("User: %s\n"), ArgUser);
		(void) fprintf(ArgLogFile, gettext("Title: %s\n"), ArgTitle);
		(void) fprintf(ArgLogFile, gettext("File: %s\n"), ArgFile);
	}

	return (0);
}


static void
Usage(void)
{
	static const char *OPTFMT = "    %-8s %-9s %s\n";

	(void) fprintf(ArgLogFile,
	    gettext("Usage:  lp.tsol_separator [OPTIONS] %s\n"),
	    gettext("PRINTER JOBID HOST!USER TITLE FILE"));
	(void) fprintf(ArgLogFile, gettext("  OPTIONS:\n"));
	(void) fprintf(ArgLogFile, OPTFMT, "-r", gettext("Reverse"),
	    gettext("Reverse banner/trailer order"));
	(void) fprintf(ArgLogFile, OPTFMT, "-l", gettext("Labels"),
	    gettext("Suppress page header/footer labels"));
	(void) fprintf(ArgLogFile, OPTFMT, gettext("-b FILE"),
	    gettext("Banner"),
	    gettext("PostScript program for banner (default tsol_banner.ps)"));
	(void) fprintf(ArgLogFile, OPTFMT, gettext("-s FILE"),
	    gettext("Separator"),
	    gettext("PostScript program for separator "
	    "(default tsol_separator.ps)"));
	(void) fprintf(ArgLogFile, OPTFMT, gettext("-t FILE"),
	    gettext("Trailer"),
	    gettext("PostScript program for trailer "
	    "(default tsol_trailer.ps)"));
	(void) fprintf(ArgLogFile, OPTFMT, gettext("-L DIR"),
	    gettext("Library"),
	    gettext("Directory to search for PostScript programs"));
	(void) fprintf(ArgLogFile, OPTFMT, "", "",
	    gettext("(default /usr/lib/lp/postscript)"));
	(void) fprintf(ArgLogFile, OPTFMT, gettext("-d N"), gettext("Debug"),
	    gettext("Set debug level to N"));
	(void) fprintf(ArgLogFile, OPTFMT, gettext("-e FILE"),
	    gettext("Error File"),
	    gettext("Append error and debugging output to FILE"));
}
