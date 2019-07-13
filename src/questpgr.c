/* NetHack 3.6	questpgr.c	$NHDT-Date: 1505172128 2017/09/11 23:22:08 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.38 $ */
/*      Copyright 1991, M. Stephenson                             */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"

/*  quest-specific pager routines. */

#include "qtext.h"

#define QTEXT_FILE "quest.dat"

#ifdef TTY_GRAPHICS
#include "wintty.h"
#endif

/* from sp_lev.c, for deliver_splev_message() */
extern char *lev_message;

static void NDECL(dump_qtlist);
static void FDECL(Fread, (genericptr_t, int, int, dlb *));
STATIC_DCL struct qtmsg *FDECL(construct_qtlist, (long));
STATIC_DCL const char *NDECL(intermed);
STATIC_DCL struct obj *FDECL(find_qarti, (struct obj *));
STATIC_DCL const char *NDECL(neminame);
STATIC_DCL const char *NDECL(guardname);
STATIC_DCL const char *NDECL(homebase);
STATIC_DCL void FDECL(qtext_pronoun, (CHAR_P, CHAR_P));
STATIC_DCL struct qtmsg *FDECL(msg_in, (struct qtmsg *, int));
STATIC_DCL void FDECL(convert_arg, (CHAR_P));
STATIC_DCL void FDECL(convert_line, (char *,char *));
STATIC_DCL void FDECL(deliver_by_pline, (struct qtmsg *));
STATIC_DCL void FDECL(deliver_by_window, (struct qtmsg *, int));
STATIC_DCL boolean FDECL(skip_pager, (BOOLEAN_P));

static char cvt_buf[64];
static struct qtlists qt_list;
static dlb *msg_file;
/* used by ldrname() and neminame(), then copied into cvt_buf */
static char nambuf[sizeof cvt_buf];

/* dump the character msg list to check appearance;
   build with DEBUG enabled and use DEBUGFILES=questpgr.c
   in sysconf file or environment */
static void
dump_qtlist()
{
#ifdef DEBUG
    struct qtmsg *msg;

    if (!explicitdebug(__FILE__))
        return;

    for (msg = qt_list.chrole; msg->msgnum > 0; msg++) {
        (void) dlb_fseek(msg_file, msg->offset, SEEK_SET);
        deliver_by_window(msg, NHW_MAP);
    }
#endif /* DEBUG */
    return;
}

static void
Fread(ptr, size, nitems, stream)
genericptr_t ptr;
int size, nitems;
dlb *stream;
{
    int cnt;

    if ((cnt = dlb_fread(ptr, size, nitems, stream)) != nitems) {
        panic("PREMATURE EOF ON QUEST TEXT FILE! Expected %d bytes, got %d",
              (size * nitems), (size * cnt));
    }
}

STATIC_OVL struct qtmsg *
construct_qtlist(hdr_offset)
long hdr_offset;
{
    struct qtmsg *msg_list;
    int n_msgs;

    (void) dlb_fseek(msg_file, hdr_offset, SEEK_SET);
    Fread(&n_msgs, sizeof(int), 1, msg_file);
    msg_list = (struct qtmsg *) alloc((unsigned) (n_msgs + 1)
                                      * sizeof (struct qtmsg));

    /*
     * Load up the list.
     */
    Fread((genericptr_t) msg_list, n_msgs * sizeof (struct qtmsg), 1,
          msg_file);

    msg_list[n_msgs].msgnum = -1;
    return msg_list;
}

void
load_qtlist()
{
    int n_classes, i;
    char qt_classes[N_HDR][LEN_HDR];
    long qt_offsets[N_HDR];

    msg_file = dlb_fopen(QTEXT_FILE, RDBMODE);
    if (!msg_file)
        panic("CANNOT OPEN QUEST TEXT FILE %s.", QTEXT_FILE);

    /*
     * Read in the number of classes, then the ID's & offsets for
     * each header.
     */

    Fread(&n_classes, sizeof (int), 1, msg_file);
    Fread(&qt_classes[0][0], sizeof (char) * LEN_HDR, n_classes, msg_file);
    Fread(qt_offsets, sizeof (long), n_classes, msg_file);

    /*
     * Now construct the message lists for quick reference later
     * on when we are actually paging the messages out.
     */

    qt_list.common = qt_list.chrole = (struct qtmsg *) 0;

    for (i = 0; i < n_classes; i++) {
        if (!strncmp(COMMON_ID, qt_classes[i], LEN_HDR))
            qt_list.common = construct_qtlist(qt_offsets[i]);
        else if (!strncmp(urole.filecode, qt_classes[i], LEN_HDR))
            qt_list.chrole = construct_qtlist(qt_offsets[i]);
#if 0 /* UNUSED but available */
        else if (!strncmp(urace.filecode, qt_classes[i], LEN_HDR))
            qt_list.chrace = construct_qtlist(qt_offsets[i]);
#endif
    }

    if (!qt_list.common || !qt_list.chrole)
        impossible("load_qtlist: cannot load quest text.");
    dump_qtlist();
    return; /* no ***DON'T*** close the msg_file */
}

/* called at program exit */
void
unload_qtlist()
{
    if (msg_file)
        (void) dlb_fclose(msg_file), msg_file = 0;
    if (qt_list.common)
        free((genericptr_t) qt_list.common), qt_list.common = 0;
    if (qt_list.chrole)
        free((genericptr_t) qt_list.chrole), qt_list.chrole = 0;
    return;
}

short
quest_info(typ)
int typ;
{
    switch (typ) {
    case 0:
        return urole.questarti;
    case MS_LEADER:
        return urole.ldrnum;
    case MS_NEMESIS:
        return urole.neminum;
    case MS_GUARDIAN:
        return urole.guardnum;
    default:
        impossible("quest_info(%d)", typ);
    }
    return 0;
}

/* return your role leader's name */
const char *
ldrname()
{
    int i = urole.ldrnum;

    Sprintf(nambuf, "%s%s", type_is_pname(&mons[i]) ? "" : "the ",
            mons[i].mname);
    return nambuf;
}

/* return your intermediate target string */
STATIC_OVL const char *
intermed()
{
    return urole.intermed;
}

boolean
is_quest_artifact(otmp)
struct obj *otmp;
{
    return (boolean) (otmp->oartifact == urole.questarti);
}

STATIC_OVL struct obj *
find_qarti(ochain)
struct obj *ochain;
{
    struct obj *otmp, *qarti;

    for (otmp = ochain; otmp; otmp = otmp->nobj) {
        if (is_quest_artifact(otmp))
            return otmp;
        if (Has_contents(otmp) && (qarti = find_qarti(otmp->cobj)) != 0)
            return qarti;
    }
    return (struct obj *) 0;
}

/* check several object chains for the quest artifact to determine
   whether it is present on the current level */
struct obj *
find_quest_artifact(whichchains)
unsigned whichchains;
{
    struct monst *mtmp;
    struct obj *qarti = 0;

    if ((whichchains & (1 << OBJ_INVENT)) != 0)
        qarti = find_qarti(invent);
    if (!qarti && (whichchains & (1 << OBJ_FLOOR)) != 0)
        qarti = find_qarti(fobj);
    if (!qarti && (whichchains & (1 << OBJ_MINVENT)) != 0)
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((qarti = find_qarti(mtmp->minvent)) != 0)
                break;
        }
    if (!qarti && (whichchains & (1 << OBJ_MIGRATING)) != 0) {
        /* check migrating objects and minvent of migrating monsters */
        for (mtmp = migrating_mons; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((qarti = find_qarti(mtmp->minvent)) != 0)
                break;
        }
        if (!qarti)
            qarti = find_qarti(migrating_objs);
    }
    if (!qarti && (whichchains & (1 << OBJ_BURIED)) != 0)
        qarti = find_qarti(level.buriedobjlist);

    return qarti;
}

/* return your role nemesis' name */
STATIC_OVL const char *
neminame()
{
    int i = urole.neminum;

    Sprintf(nambuf, "%s%s", type_is_pname(&mons[i]) ? "" : "the ",
            mons[i].mname);
    return nambuf;
}

STATIC_OVL const char *
guardname() /* return your role leader's guard monster name */
{
    int i = urole.guardnum;

    return mons[i].mname;
}

STATIC_OVL const char *
homebase() /* return your role leader's location */
{
    return urole.homebase;
}

/* replace deity, leader, nemesis, or artifact name with pronoun;
   overwrites cvt_buf[] */
STATIC_OVL void
qtext_pronoun(who, which)
char who,  /* 'd' => deity, 'l' => leader, 'n' => nemesis, 'o' => artifact */
    which; /* 'h'|'H'|'i'|'I'|'j'|'J' */
{
    const char *pnoun;
    int g;
    char lwhich = lowc(which); /* H,I,J -> h,i,j */

    /*
     * Invalid subject (not d,l,n,o) yields neuter, singular result.
     *
     * For %o, treat all artifacts as neuter; some have plural names,
     * which genders[] doesn't handle; cvt_buf[] already contains name.
     */
    if (who == 'o'
        && (strstri(cvt_buf, "Eyes ")
            || strcmpi(cvt_buf, makesingular(cvt_buf)))) {
        pnoun = (lwhich == 'h') ? "they"
                : (lwhich == 'i') ? "them"
                : (lwhich == 'j') ? "their" : "?";
    } else {
        g = (who == 'd') ? quest_status.godgend
            : (who == 'l') ? quest_status.ldrgend
            : (who == 'n') ? quest_status.nemgend
            : 2; /* default to neuter */
        pnoun = (lwhich == 'h') ? genders[g].he
                : (lwhich == 'i') ? genders[g].him
                : (lwhich == 'j') ? genders[g].his : "?";
    }
    Strcpy(cvt_buf, pnoun);
    /* capitalize for H,I,J */
    if (lwhich != which)
        cvt_buf[0] = highc(cvt_buf[0]);
    return;
}

STATIC_OVL struct qtmsg *
msg_in(qtm_list, msgnum)
struct qtmsg *qtm_list;
int msgnum;
{
    struct qtmsg *qt_msg;

    for (qt_msg = qtm_list; qt_msg->msgnum > 0; qt_msg++)
        if (qt_msg->msgnum == msgnum)
            return qt_msg;

    return (struct qtmsg *) 0;
}

STATIC_OVL void
convert_arg(c)
char c;
{
    register const char *str;

    switch (c) {
    case 'p':
        str = plname;
        break;
    case 'c':
        str = (flags.female && urole.name.f) ? urole.name.f : urole.name.m;
        break;
    case 'r':
        str = rank_of(u.ulevel, Role_switch, flags.female);
        break;
    case 'R':
        str = rank_of(MIN_QUEST_LEVEL, Role_switch, flags.female);
        break;
    case 's':
        str = (flags.female) ? "sister" : "brother";
        break;
    case 'S':
        str = (flags.female) ? "daughter" : "son";
        break;
    case 'l':
        str = ldrname();
        break;
    case 'i':
        str = intermed();
        break;
    case 'O':
    case 'o':
        str = the(artiname(urole.questarti));
        if (c == 'O') {
            /* shorten "the Foo of Bar" to "the Foo"
               (buffer returned by the() is modifiable) */
            char *p = strstri(str, " of ");

            if (p)
                *p = '\0';
        }
        break;
    case 'n':
        str = neminame();
        break;
    case 'g':
        str = guardname();
        break;
    case 'G':
        str = align_gtitle(u.ualignbase[A_ORIGINAL]);
        break;
    case 'H':
        str = homebase();
        break;
    case 'a':
        str = align_str(u.ualignbase[A_ORIGINAL]);
        break;
    case 'A':
        str = align_str(u.ualign.type);
        break;
    case 'd':
        str = align_gname(u.ualignbase[A_ORIGINAL]);
        break;
    case 'D':
        str = align_gname(A_LAWFUL);
        break;
    case 'C':
        str = "chaotic";
        break;
    case 'N':
        str = "neutral";
        break;
    case 'L':
        str = "lawful";
        break;
    case 'x':
        str = Blind ? "sense" : "see";
        break;
    case 'Z':
        str = dungeons[0].dname;
        break;
    case '%':
        str = "%";
        break;
    default:
        str = "";
        break;
    }
    Strcpy(cvt_buf, str);
}

STATIC_OVL void
convert_line(in_line, out_line)
char *in_line, *out_line;
{
    char *c, *cc;
    char xbuf[BUFSZ];

    cc = out_line;
    for (c = xcrypt(in_line, xbuf); *c; c++) {
        *cc = 0;
        switch (*c) {
        case '\r':
        case '\n':
            *(++cc) = 0;
            return;

        case '%':
            if (*(c + 1)) {
                convert_arg(*(++c));
                switch (*(++c)) {
                /* insert "a"/"an" prefix */
                case 'A':
                    Strcat(cc, An(cvt_buf));
                    cc += strlen(cc);
                    continue; /* for */
                case 'a':
                    Strcat(cc, an(cvt_buf));
                    cc += strlen(cc);
                    continue; /* for */

                /* capitalize */
                case 'C':
                    cvt_buf[0] = highc(cvt_buf[0]);
                    break;

                /* replace name with pronoun;
                   valid for %d, %l, %n, and %o */
                case 'h': /* he/she */
                case 'H': /* He/She */
                case 'i': /* him/her */
                case 'I':
                case 'j': /* his/her */
                case 'J':
                    if (index("dlno", lowc(*(c - 1))))
                        qtext_pronoun(*(c - 1), *c);
                    else
                        --c; /* default action */
                    break;

                /* pluralize */
                case 'P':
                    cvt_buf[0] = highc(cvt_buf[0]);
                    /*FALLTHRU*/
                case 'p':
                    Strcpy(cvt_buf, makeplural(cvt_buf));
                    break;

                /* append possessive suffix */
                case 'S':
                    cvt_buf[0] = highc(cvt_buf[0]);
                    /*FALLTHRU*/
                case 's':
                    Strcpy(cvt_buf, s_suffix(cvt_buf));
                    break;

                /* strip any "the" prefix */
                case 't':
                    if (!strncmpi(cvt_buf, "the ", 4)) {
                        Strcat(cc, &cvt_buf[4]);
                        cc += strlen(cc);
                        continue; /* for */
                    }
                    break;

                default:
                    --c; /* undo switch increment */
                    break;
                }
                Strcat(cc, cvt_buf);
                cc += strlen(cvt_buf);
                break;
            } /* else fall through */

        default:
            *cc++ = *c;
            break;
        }
    }
    if (cc > &out_line[BUFSZ-1])
        panic("convert_line: overflow");
    *cc = 0;
    return;
}

STATIC_OVL void
deliver_by_pline(qt_msg)
struct qtmsg *qt_msg;
{
    long size;
    char in_line[BUFSZ], out_line[BUFSZ];

    *in_line = '\0';
    for (size = 0; size < qt_msg->size; size += (long) strlen(in_line)) {
        (void) dlb_fgets(in_line, sizeof in_line, msg_file);
        convert_line(in_line, out_line);
        pline("%s", out_line);
    }
}

STATIC_OVL void
deliver_by_window(qt_msg, how)
struct qtmsg *qt_msg;
int how;
{
    long size;
    char in_line[BUFSZ], out_line[BUFSZ];
    boolean qtdump = (how == NHW_MAP);
    winid datawin = create_nhwindow(qtdump ? NHW_TEXT : how);

#ifdef DEBUG
    if (qtdump) {
        char buf[BUFSZ];

        /* when dumping quest messages at startup, all of them are passed to
         * deliver_by_window(), even if normally given to deliver_by_pline()
         */
        Sprintf(buf, "msgnum: %d, delivery: %c",
                qt_msg->msgnum, qt_msg->delivery);
        putstr(datawin, 0, buf);
        putstr(datawin, 0, "");
    }
#endif
    for (size = 0; size < qt_msg->size; size += (long) strlen(in_line)) {
        (void) dlb_fgets(in_line, sizeof in_line, msg_file);
        convert_line(in_line, out_line);
        putstr(datawin, 0, out_line);
    }
    display_nhwindow(datawin, TRUE);
    destroy_nhwindow(datawin);

    /* block messages delivered by window aren't kept in message history
       but have a one-line summary which is put there for ^P recall */
    *out_line = '\0';
    if (qt_msg->summary_size) {
        (void) dlb_fgets(in_line, sizeof in_line, msg_file);
        convert_line(in_line, out_line);
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
    } else if (qt_msg->delivery == 'c') { /* skip for 'qtdump' of 'p' */
        /* delivery 'c' and !summary_size, summary expected but not present;
           this doesn't prefix the number with role code vs 'general'
           but should be good enough for summary verification purposes */
        Sprintf(out_line, "[missing block message summary for #%05d]",
                qt_msg->msgnum);
#endif
    }
    if (*out_line)
        putmsghistory(out_line, FALSE);
}

STATIC_OVL boolean
skip_pager(common)
boolean common;
{
    /* WIZKIT: suppress plot feedback if starting with quest artifact */
    if (program_state.wizkit_wishing)
        return TRUE;
    if (!(common ? qt_list.common : qt_list.chrole)) {
        panic("%s: no %s quest text data available",
              common ? "com_pager" : "qt_pager",
              common ? "common" : "role-specific");
        /*NOTREACHED*/
        return TRUE;
    }
    return FALSE;
}

void
com_pager(msgnum)
int msgnum;
{
    struct qtmsg *qt_msg;

    if (skip_pager(TRUE))
        return;

    if (!(qt_msg = msg_in(qt_list.common, msgnum))) {
        impossible("com_pager: message %d not found.", msgnum);
        return;
    }

    (void) dlb_fseek(msg_file, qt_msg->offset, SEEK_SET);
    if (qt_msg->delivery == 'p')
        deliver_by_pline(qt_msg);
    else if (msgnum == 1)
        deliver_by_window(qt_msg, NHW_MENU);
    else
        deliver_by_window(qt_msg, NHW_TEXT);
    return;
}

void
qt_pager(msgnum)
int msgnum;
{
    struct qtmsg *qt_msg;

    if (skip_pager(FALSE))
        return;

    qt_msg = msg_in(qt_list.chrole, msgnum);
    if (!qt_msg) {
        /* some roles have an alternate message for return to the goal
           level when the quest artifact is absent (handled by caller)
           but some don't; for the latter, use the normal goal message;
           note: for first visit, artifact is assumed to always be
           present which might not be true for wizard mode but we don't
           worry about quest message references in that situation */
        if (msgnum == QT_ALTGOAL)
            qt_msg = msg_in(qt_list.chrole, QT_NEXTGOAL);
    }
    if (!qt_msg) {
        impossible("qt_pager: message %d not found.", msgnum);
        return;
    }

    (void) dlb_fseek(msg_file, qt_msg->offset, SEEK_SET);
    if (qt_msg->delivery == 'p' && strcmp(windowprocs.name, "X11"))
        deliver_by_pline(qt_msg);
    else
        deliver_by_window(qt_msg, NHW_TEXT);
    return;
}

struct permonst *
qt_montype()
{
    int qpm;

    if (rn2(5)) {
        qpm = urole.enemy1num;
        if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GENOD))
            return &mons[qpm];
        return mkclass(urole.enemy1sym, 0);
    }
    qpm = urole.enemy2num;
    if (qpm != NON_PM && rn2(5) && !(mvitals[qpm].mvflags & G_GENOD))
        return &mons[qpm];
    return mkclass(urole.enemy2sym, 0);
}

/* special levels can include a custom arrival message; display it */
void
deliver_splev_message()
{
    char *str, *nl, in_line[BUFSZ], out_line[BUFSZ];

    /* there's no provision for delivering via window instead of pline */
    if (lev_message) {
        /* lev_message can span multiple lines using embedded newline chars;
           any segments too long to fit within in_line[] will be truncated */
        for (str = lev_message; *str; str = nl + 1) {
            /* copying will stop at newline if one is present */
            copynchars(in_line, str, (int) (sizeof in_line) - 1);

            /* convert_line() expects encrypted input */
            (void) xcrypt(in_line, in_line);
            convert_line(in_line, out_line);
            pline("%s", out_line);

            if ((nl = index(str, '\n')) == 0)
                break; /* done if no newline */
        }

        free((genericptr_t) lev_message);
        lev_message = 0;
    }
}

/*questpgr.c*/
