/* NetHack 3.7	questpgr.c	$NHDT-Date: 1596498201 2020/08/03 23:43:21 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.72 $ */
/*      Copyright 1991, M. Stephenson                             */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"

/*  quest-specific pager routines. */

#define QTEXT_FILE "quest.lua"

#ifdef TTY_GRAPHICS
#include "wintty.h"
#endif

static const char *intermed(void);
static struct obj *find_qarti(struct obj *);
static const char *neminame(void);
static const char *guardname(void);
static const char *homebase(void);
static void qtext_pronoun(char, char);
static void convert_arg(char);
static void convert_line(char *,char *);
static void deliver_by_pline(const char *);
static void deliver_by_window(const char *, int);
static boolean skip_pager(boolean);
static boolean com_pager_core(const char *, const char *, boolean);

short
quest_info(int typ)
{
    switch (typ) {
    case 0:
        return g.urole.questarti;
    case MS_LEADER:
        return g.urole.ldrnum;
    case MS_NEMESIS:
        return g.urole.neminum;
    case MS_GUARDIAN:
        return g.urole.guardnum;
    default:
        impossible("quest_info(%d)", typ);
    }
    return 0;
}

/* return your role leader's name */
const char *
ldrname(void)
{
    int i = g.urole.ldrnum;

    Sprintf(g.nambuf, "%s%s", type_is_pname(&mons[i]) ? "" : "the ",
            mons[i].pmnames[NEUTRAL]);
    return g.nambuf;
}

/* return your intermediate target string */
static const char *
intermed(void)
{
    return g.urole.intermed;
}

boolean
is_quest_artifact(struct obj *otmp)
{
    return (boolean) (otmp->oartifact == g.urole.questarti);
}

static struct obj *
find_qarti(struct obj *ochain)
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
find_quest_artifact(unsigned whichchains)
{
    struct monst *mtmp;
    struct obj *qarti = 0;

    if ((whichchains & (1 << OBJ_INVENT)) != 0)
        qarti = find_qarti(g.invent);
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
        for (mtmp = g.migrating_mons; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((qarti = find_qarti(mtmp->minvent)) != 0)
                break;
        }
        if (!qarti)
            qarti = find_qarti(g.migrating_objs);
    }
    if (!qarti && (whichchains & (1 << OBJ_BURIED)) != 0)
        qarti = find_qarti(g.level.buriedobjlist);

    return qarti;
}

/* return your role nemesis' name */
static const char *
neminame(void)
{
    int i = g.urole.neminum;

    Sprintf(g.nambuf, "%s%s", type_is_pname(&mons[i]) ? "" : "the ",
            mons[i].pmnames[NEUTRAL]);
    return g.nambuf;
}

static const char *
guardname(void) /* return your role leader's guard monster name */
{
    int i = g.urole.guardnum;

    return mons[i].pmnames[NEUTRAL];
}

static const char *
homebase(void) /* return your role leader's location */
{
    return g.urole.homebase;
}

/* replace deity, leader, nemesis, or artifact name with pronoun;
   overwrites cvt_buf[] */
static void
qtext_pronoun(char who,   /* 'd' => deity, 'l' => leader, 'n' => nemesis,
                             'o' => artifact */
              char which) /* 'h'|'H'|'i'|'I'|'j'|'J' */
{
    const char *pnoun;
    int godgend;
    char lwhich = lowc(which); /* H,I,J -> h,i,j */

    /*
     * Invalid subject (not d,l,n,o) yields neuter, singular result.
     *
     * For %o, treat all artifacts as neuter; some have plural names,
     * which genders[] doesn't handle; cvt_buf[] already contains name.
     */
    if (who == 'o'
        && (strstri(g.cvt_buf, "Eyes ")
            || strcmpi(g.cvt_buf, makesingular(g.cvt_buf)))) {
        pnoun = (lwhich == 'h') ? "they"
                : (lwhich == 'i') ? "them"
                : (lwhich == 'j') ? "their" : "?";
    } else {
        godgend = (who == 'd') ? g.quest_status.godgend
            : (who == 'l') ? g.quest_status.ldrgend
            : (who == 'n') ? g.quest_status.nemgend
            : 2; /* default to neuter */
        pnoun = (lwhich == 'h') ? genders[godgend].he
                : (lwhich == 'i') ? genders[godgend].him
                : (lwhich == 'j') ? genders[godgend].his : "?";
    }
    Strcpy(g.cvt_buf, pnoun);
    /* capitalize for H,I,J */
    if (lwhich != which)
        g.cvt_buf[0] = highc(g.cvt_buf[0]);
    return;
}

static void
convert_arg(char c)
{
    register const char *str;

    switch (c) {
    case 'p':
        str = g.plname;
        break;
    case 'c':
        str = (flags.female && g.urole.name.f) ? g.urole.name.f : g.urole.name.m;
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
        str = the(artiname(g.urole.questarti));
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
        str = g.dungeons[0].dname;
        break;
    case '%':
        str = "%";
        break;
    default:
        str = "";
        break;
    }
    Strcpy(g.cvt_buf, str);
}

static void
convert_line(char *in_line, char *out_line)
{
    char *c, *cc;

    cc = out_line;
    for (c = in_line; *c; c++) {
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
                    Strcat(cc, An(g.cvt_buf));
                    cc += strlen(cc);
                    continue; /* for */
                case 'a':
                    Strcat(cc, an(g.cvt_buf));
                    cc += strlen(cc);
                    continue; /* for */

                /* capitalize */
                case 'C':
                    g.cvt_buf[0] = highc(g.cvt_buf[0]);
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
                    g.cvt_buf[0] = highc(g.cvt_buf[0]);
                    /*FALLTHRU*/
                case 'p':
                    Strcpy(g.cvt_buf, makeplural(g.cvt_buf));
                    break;

                /* append possessive suffix */
                case 'S':
                    g.cvt_buf[0] = highc(g.cvt_buf[0]);
                    /*FALLTHRU*/
                case 's':
                    Strcpy(g.cvt_buf, s_suffix(g.cvt_buf));
                    break;

                /* strip any "the" prefix */
                case 't':
                    if (!strncmpi(g.cvt_buf, "the ", 4)) {
                        Strcat(cc, &g.cvt_buf[4]);
                        cc += strlen(cc);
                        continue; /* for */
                    }
                    break;

                default:
                    --c; /* undo switch increment */
                    break;
                }
                Strcat(cc, g.cvt_buf);
                cc += strlen(g.cvt_buf);
                break;
            } /* else fall through */

        default:
            *cc++ = *c;
            break;
        }
        if (cc > &out_line[BUFSZ - 1])
            panic("convert_line: overflow");
    }
    *cc = 0;
    return;
}

static void
deliver_by_pline(const char *str)
{
    const char *msgp = str;
    const char *msgend = eos((char *)str);
    char in_line[BUFSZ], out_line[BUFSZ];

    while (msgp && msgp != msgend) {
        int i = 0;
        while (*msgp != '\0' && *msgp != '\n' && (i < BUFSZ-2)) {
            in_line[i] = *msgp;
            i++;
            msgp++;
        }
        if (*msgp == '\n')
            msgp++;
        in_line[i] = '\0';
        convert_line(in_line, out_line);
        pline("%s", out_line);
    }
}

static void
deliver_by_window(const char *msg, int how)
{
    const char *msgp = msg;
    const char *msgend = eos((char *)msg);
    char in_line[BUFSZ], out_line[BUFSZ];
    winid datawin = create_nhwindow(how);

    while (msgp && msgp != msgend) {
        int i = 0;
        while (*msgp != '\0' && *msgp != '\n' && (i < BUFSZ-2)) {
            in_line[i] = *msgp;
            i++;
            msgp++;
        }
        if (*msgp == '\n')
            msgp++;
        in_line[i] = '\0';
        convert_line(in_line, out_line);
        putstr(datawin, 0, out_line);
    }

    display_nhwindow(datawin, TRUE);
    destroy_nhwindow(datawin);
}

static boolean
skip_pager(boolean common UNUSED)
{
    /* WIZKIT: suppress plot feedback if starting with quest artifact */
    if (g.program_state.wizkit_wishing)
        return TRUE;
    return FALSE;
}

static boolean
com_pager_core(const char *section, const char *msgid, boolean showerror)
{
    static const char *const howtoput[] = {
        "pline", "window", "text", "menu", "default", NULL
    };
    static const int howtoput2i[] = { 1, 2, 2, 3, 0, 0 };
    int output;
    lua_State *L;
    char *text = NULL, *synopsis = NULL, *fallback_msgid = NULL;
    boolean res = FALSE;

    if (skip_pager(TRUE))
        return FALSE;

    L = nhl_init();

    if (!nhl_loadlua(L, QTEXT_FILE)) {
        if (showerror)
            impossible("com_pager: %s not found.", QTEXT_FILE);
        goto compagerdone;
    }

    lua_settop(L, 0);
    lua_getglobal(L, "questtext");
    if (!lua_istable(L, -1)) {
        if (showerror)
            impossible("com_pager: questtext in %s is not a lua table",
                       QTEXT_FILE);
        goto compagerdone;
    }

    lua_getfield(L, -1, section);
    if (!lua_istable(L, -1)) {
        if (showerror)
            impossible("com_pager: questtext[%s] in %s is not a lua table",
                       section, QTEXT_FILE);
        goto compagerdone;
    }

 tryagain:
    lua_getfield(L, -1, fallback_msgid ? fallback_msgid : msgid);
    if (!lua_istable(L, -1)) {
        if (!fallback_msgid) {
            /* Do we have questtxt[msg_fallbacks][<msgid>]? */
            lua_getfield(L, -3, "msg_fallbacks");
            if (lua_istable(L, -1)) {
                fallback_msgid = get_table_str_opt(L, msgid, NULL);
                lua_pop(L, 2);
                if (fallback_msgid)
                    goto tryagain;
            }
        }
        if (showerror) {
            if (!fallback_msgid)
                impossible(
                      "com_pager: questtext[%s][%s] in %s is not a lua table",
                           section, msgid, QTEXT_FILE);
            else
                impossible(
           "com_pager: questtext[%s][%s] and [][%s] in %s are not lua tables",
                           section, msgid, fallback_msgid, QTEXT_FILE);
        }
        goto compagerdone;
    }

    synopsis = get_table_str_opt(L, "synopsis", NULL);
    text = get_table_str_opt(L, "text", NULL);
    output = howtoput2i[get_table_option(L, "output", "default", howtoput)];

    if (!text) {
        int nelems;

        lua_len(L, -1);
        nelems = (int) lua_tointeger(L, -1);
        lua_pop(L, 1);
        if (nelems < 2) {
            if (showerror)
                impossible(
              "com_pager: questtext[%s][%s] in %s in not an array of strings",
                           section, fallback_msgid ? fallback_msgid : msgid,
                           QTEXT_FILE);
            goto compagerdone;
        }
        nelems = rn2(nelems) + 1;
        lua_pushinteger(L, nelems);
        lua_gettable(L, -2);
        text = dupstr(luaL_checkstring(L, -1));
    }

    if (output == 0 && (index(text, '\n') || (strlen(text) >= (BUFSZ - 1))))
        output = 2;

    if (output == 0 || output == 1)
        deliver_by_pline(text);
    else
        deliver_by_window(text, (output == 3) ? NHW_MENU : NHW_TEXT);

    if (synopsis) {
        char in_line[BUFSZ], out_line[BUFSZ];

#if 0   /* not yet -- brackets need to be removed from quest.lua */
        Sprintf(in_line, "[%.*s]",
                (int) (sizeof in_line - sizeof "[]"), synopsis);
#else
        Strcpy(in_line, synopsis);
#endif
        convert_line(in_line, out_line);
        /* bypass message delivery but be available for ^P recall */
        putmsghistory(out_line, FALSE);
    }
    res = TRUE;

 compagerdone:
    if (text)
        free((genericptr_t) text);
    if (synopsis)
        free((genericptr_t) synopsis);
    if (fallback_msgid)
        free((genericptr_t) fallback_msgid);
    nhl_done(L);
    return res;
}

void
com_pager(const char *msgid)
{
    com_pager_core("common", msgid, TRUE);
}

void
qt_pager(const char *msgid)
{
    if (!com_pager_core(g.urole.filecode, msgid, FALSE))
        com_pager_core("common", msgid, TRUE);
}

struct permonst *
qt_montype(void)
{
    int qpm;

    if (rn2(5)) {
        qpm = g.urole.enemy1num;
        if (qpm != NON_PM && rn2(5) && !(g.mvitals[qpm].mvflags & G_GENOD))
            return &mons[qpm];
        return mkclass(g.urole.enemy1sym, 0);
    }
    qpm = g.urole.enemy2num;
    if (qpm != NON_PM && rn2(5) && !(g.mvitals[qpm].mvflags & G_GENOD))
        return &mons[qpm];
    return mkclass(g.urole.enemy2sym, 0);
}

/* special levels can include a custom arrival message; display it */
void
deliver_splev_message(void)
{
    char *str, *nl, in_line[BUFSZ], out_line[BUFSZ];

    /* there's no provision for delivering via window instead of pline */
    if (g.lev_message) {
        /* lev_message can span multiple lines using embedded newline chars;
           any segments too long to fit within in_line[] will be truncated */
        for (str = g.lev_message; *str; str = nl + 1) {
            /* copying will stop at newline if one is present */
            copynchars(in_line, str, (int) (sizeof in_line) - 1);

            convert_line(in_line, out_line);
            pline("%s", out_line);

            if ((nl = index(str, '\n')) == 0)
                break; /* done if no newline */
        }

        free((genericptr_t) g.lev_message);
        g.lev_message = NULL;
    }
}

/*questpgr.c*/
