/*	SCCS Id: @(#)questpgr.c	3.4	2000/05/05	*/
/*	Copyright 1991, M. Stephenson		  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"

/*  quest-specific pager routines. */

#include "qtext.h"

#define QTEXT_FILE	"quest.dat"

/* #define DEBUG */	/* uncomment for debugging */

static void FDECL(Fread, (genericptr_t,int,int,dlb *));
STATIC_DCL struct qtmsg * FDECL(construct_qtlist, (long));
STATIC_DCL const char * NDECL(intermed);
STATIC_DCL const char * NDECL(neminame);
STATIC_DCL const char * NDECL(guardname);
STATIC_DCL const char * NDECL(homebase);
STATIC_DCL struct qtmsg * FDECL(msg_in, (struct qtmsg *,int));
STATIC_DCL void FDECL(convert_arg, (CHAR_P));
STATIC_DCL void NDECL(convert_line);
STATIC_DCL void FDECL(deliver_by_pline, (struct qtmsg *));
STATIC_DCL void FDECL(deliver_by_window, (struct qtmsg *,int));

static char	in_line[80], cvt_buf[64], out_line[128];
static struct	qtlists	qt_list;
static dlb	*msg_file;
/* used by ldrname() and neminame(), then copied into cvt_buf */
static char	nambuf[sizeof cvt_buf];

#ifdef DEBUG
static void NDECL(dump_qtlist);

static void
dump_qtlist()	/* dump the character msg list to check appearance */
{
	struct	qtmsg	*msg;
	long	size;

	for (msg = qt_list.chrole; msg->msgnum > 0; msg++) {
		pline("msgnum %d: delivery %c",
			msg->msgnum, msg->delivery);
		more();
		(void) dlb_fseek(msg_file, msg->offset, SEEK_SET);
		deliver_by_window(msg, NHW_TEXT);
	}
}
#endif /* DEBUG */

static void
Fread(ptr, size, nitems, stream)
genericptr_t	ptr;
int	size, nitems;
dlb	*stream;
{
	int cnt;

	if ((cnt = dlb_fread(ptr, size, nitems, stream)) != nitems) {

	    panic("PREMATURE EOF ON QUEST TEXT FILE! Expected %d bytes, got %d",
		    (size * nitems), (size * cnt));
	}
}

STATIC_OVL struct qtmsg *
construct_qtlist(hdr_offset)
long	hdr_offset;
{
	struct qtmsg *msg_list;
	int	n_msgs;

	(void) dlb_fseek(msg_file, hdr_offset, SEEK_SET);
	Fread(&n_msgs, sizeof(int), 1, msg_file);
	msg_list = (struct qtmsg *)
		alloc((unsigned)(n_msgs+1)*sizeof(struct qtmsg));

	/*
	 * Load up the list.
	 */
	Fread((genericptr_t)msg_list, n_msgs*sizeof(struct qtmsg), 1, msg_file);

	msg_list[n_msgs].msgnum = -1;
	return(msg_list);
}

void
load_qtlist()
{

	int	n_classes, i;
	char	qt_classes[N_HDR][LEN_HDR];
	long	qt_offsets[N_HDR];

	msg_file = dlb_fopen(QTEXT_FILE, RDBMODE);
	if (!msg_file)
	    panic("CANNOT OPEN QUEST TEXT FILE %s.", QTEXT_FILE);

	/*
	 * Read in the number of classes, then the ID's & offsets for
	 * each header.
	 */

	Fread(&n_classes, sizeof(int), 1, msg_file);
	Fread(&qt_classes[0][0], sizeof(char)*LEN_HDR, n_classes, msg_file);
	Fread(qt_offsets, sizeof(long), n_classes, msg_file);

	/*
	 * Now construct the message lists for quick reference later
	 * on when we are actually paging the messages out.
	 */

	qt_list.common = qt_list.chrole = (struct qtmsg *)0;

	for (i = 0; i < n_classes; i++) {
	    if (!strncmp(COMMON_ID, qt_classes[i], LEN_HDR))
	    	qt_list.common = construct_qtlist(qt_offsets[i]);
	    else if (!strncmp(urole.filecode, qt_classes[i], LEN_HDR))
	    	qt_list.chrole = construct_qtlist(qt_offsets[i]);
#if 0	/* UNUSED but available */
	    else if (!strncmp(urace.filecode, qt_classes[i], LEN_HDR))
	    	qt_list.chrace = construct_qtlist(qt_offsets[i]);
#endif
	}

	if (!qt_list.common || !qt_list.chrole)
	    impossible("load_qtlist: cannot load quest text.");
#ifdef DEBUG
	dump_qtlist();
#endif
	return;	/* no ***DON'T*** close the msg_file */
}

/* called at program exit */
void
unload_qtlist()
{
	if (msg_file)
	    (void) dlb_fclose(msg_file),  msg_file = 0;
	if (qt_list.common)
	    free((genericptr_t) qt_list.common),  qt_list.common = 0;
	if (qt_list.chrole)
	    free((genericptr_t) qt_list.chrole),  qt_list.chrole = 0;
	return;
}

short
quest_info(typ)
int typ;
{
	switch (typ) {
	    case 0:		return (urole.questarti);
	    case MS_LEADER:	return (urole.ldrnum);
	    case MS_NEMESIS:	return (urole.neminum);
	    case MS_GUARDIAN:	return (urole.guardnum);
	    default:		impossible("quest_info(%d)", typ);
	}
	return 0;
}

const char *
ldrname()	/* return your role leader's name */
{
	int i = urole.ldrnum;

	Sprintf(nambuf, "%s%s",
		type_is_pname(&mons[i]) ? "" : "the ",
		mons[i].mname);
	return nambuf;
}

STATIC_OVL const char *
intermed()	/* return your intermediate target string */
{
	return (urole.intermed);
}

boolean
is_quest_artifact(otmp)
struct obj *otmp;
{
	return((boolean)(otmp->oartifact == urole.questarti));
}

STATIC_OVL const char *
neminame()	/* return your role nemesis' name */
{
	int i = urole.neminum;

	Sprintf(nambuf, "%s%s",
		type_is_pname(&mons[i]) ? "" : "the ",
		mons[i].mname);
	return nambuf;
}

STATIC_OVL const char *
guardname()	/* return your role leader's guard monster name */
{
	int i = urole.guardnum;

	return(mons[i].mname);
}

STATIC_OVL const char *
homebase()	/* return your role leader's location */
{
	return(urole.homebase);
}

STATIC_OVL struct qtmsg *
msg_in(qtm_list, msgnum)
struct qtmsg *qtm_list;
int	msgnum;
{
	struct qtmsg *qt_msg;

	for (qt_msg = qtm_list; qt_msg->msgnum > 0; qt_msg++)
	    if (qt_msg->msgnum == msgnum) return(qt_msg);

	return((struct qtmsg *)0);
}

STATIC_OVL void
convert_arg(c)
char c;
{
	register const char *str;

	switch (c) {

	    case 'p':	str = plname;
			break;
	    case 'c':	str = (flags.female && urole.name.f) ?
	    			urole.name.f : urole.name.m;
			break;
	    case 'r':	str = rank_of(u.ulevel, Role_switch, flags.female);
			break;
	    case 'R':	str = rank_of(MIN_QUEST_LEVEL, Role_switch,
	    			flags.female);
			break;
	    case 's':	str = (flags.female) ? "sister" : "brother";
			break;
	    case 'S':	str = (flags.female) ? "daughter" : "son";
			break;
	    case 'l':	str = ldrname();
			break;
	    case 'i':	str = intermed();
			break;
	    case 'o':	str = the(artiname(urole.questarti));
			break;
	    case 'n':	str = neminame();
			break;
	    case 'g':	str = guardname();
			break;
	    case 'G':	str = align_gtitle(u.ualignbase[A_ORIGINAL]);
			break;
	    case 'H':	str = homebase();
			break;
	    case 'a':	str = align_str(u.ualignbase[A_ORIGINAL]);
			break;
	    case 'A':	str = align_str(u.ualign.type);
			break;
	    case 'd':	str = align_gname(u.ualignbase[A_ORIGINAL]);
			break;
	    case 'D':	str = align_gname(A_LAWFUL);
			break;
	    case 'C':	str = "chaotic";
			break;
	    case 'N':	str = "neutral";
			break;
	    case 'L':	str = "lawful";
			break;
	    case 'x':	str = Blind ? "sense" : "see";
			break;
	    case 'Z':	str = dungeons[0].dname;
			break;
	    case '%':	str = "%";
			break;
	     default:	str = "";
			break;
	}
	Strcpy(cvt_buf, str);
}

STATIC_OVL void
convert_line()
{
	char *c, *cc;
	char xbuf[BUFSZ];

	cc = out_line;
	for (c = xcrypt(in_line, xbuf); *c; c++) {

	    *cc = 0;
	    switch(*c) {

		case '\r':
		case '\n':
			*(++cc) = 0;
			return;

		case '%':
			if (*(c+1)) {
			    convert_arg(*(++c));
			    switch (*(++c)) {

					/* insert "a"/"an" prefix */
				case 'A': Strcat(cc, An(cvt_buf));
				    cc += strlen(cc);
				    continue; /* for */
				case 'a': Strcat(cc, an(cvt_buf));
				    cc += strlen(cc);
				    continue; /* for */

					/* capitalize */
				case 'C': cvt_buf[0] = highc(cvt_buf[0]);
				    break;

					/* pluralize */
				case 'P': cvt_buf[0] = highc(cvt_buf[0]);
				case 'p': Strcpy(cvt_buf, makeplural(cvt_buf));
				    break;

					/* append possessive suffix */
				case 'S': cvt_buf[0] = highc(cvt_buf[0]);
				case 's': Strcpy(cvt_buf, s_suffix(cvt_buf));
				    break;

					/* strip any "the" prefix */
				case 't': if (!strncmpi(cvt_buf, "the ", 4)) {
					Strcat(cc, &cvt_buf[4]);
					cc += strlen(cc);
					continue; /* for */
				    }
				    break;

				default: --c;	/* undo switch increment */
				    break;
			    }
			    Strcat(cc, cvt_buf);
			    cc += strlen(cvt_buf);
			    break;
			}	/* else fall through */

		default:
			*cc++ = *c;
			break;
	    }
	}
	if (cc >= out_line + sizeof out_line)
	    panic("convert_line: overflow");
	*cc = 0;
	return;
}

STATIC_OVL void
deliver_by_pline(qt_msg)
struct qtmsg *qt_msg;
{
	long	size;

	for (size = 0; size < qt_msg->size; size += (long)strlen(in_line)) {
	    (void) dlb_fgets(in_line, 80, msg_file);
	    convert_line();
	    pline(out_line);
	}

}

STATIC_OVL void
deliver_by_window(qt_msg, how)
struct qtmsg *qt_msg;
int how;
{
	long	size;
	winid datawin = create_nhwindow(how);

	for (size = 0; size < qt_msg->size; size += (long)strlen(in_line)) {
	    (void) dlb_fgets(in_line, 80, msg_file);
	    convert_line();
	    putstr(datawin, 0, out_line);
	}
	display_nhwindow(datawin, TRUE);
	destroy_nhwindow(datawin);
}

void
com_pager(msgnum)
int	msgnum;
{
	struct qtmsg *qt_msg;

	if (!(qt_msg = msg_in(qt_list.common, msgnum))) {
		impossible("com_pager: message %d not found.", msgnum);
		return;
	}

	(void) dlb_fseek(msg_file, qt_msg->offset, SEEK_SET);
	if (qt_msg->delivery == 'p') deliver_by_pline(qt_msg);
	else if (msgnum == 1) deliver_by_window(qt_msg, NHW_MENU);
	else		     deliver_by_window(qt_msg, NHW_TEXT);
	return;
}

void
qt_pager(msgnum)
int	msgnum;
{
	struct qtmsg *qt_msg;

	if (!(qt_msg = msg_in(qt_list.chrole, msgnum))) {
		impossible("qt_pager: message %d not found.", msgnum);
		return;
	}

	(void) dlb_fseek(msg_file, qt_msg->offset, SEEK_SET);
	if (qt_msg->delivery == 'p' && strcmp(windowprocs.name, "X11"))
		deliver_by_pline(qt_msg);
	else	deliver_by_window(qt_msg, NHW_TEXT);
	return;
}

struct permonst *
qt_montype()
{
	int qpm;

	if (rn2(5)) {
	    qpm = urole.enemy1num;
	    if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GENOD))
	    	return (&mons[qpm]);
	    return (mkclass(urole.enemy1sym, 0));
	}
	qpm = urole.enemy2num;
	if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GENOD))
	    return (&mons[qpm]);
	return (mkclass(urole.enemy2sym, 0));
}

/*questpgr.c*/
