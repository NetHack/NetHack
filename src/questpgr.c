/* NetHack 3.7	questpgr.c	$NHDT-Date: 1687036548 2023/06/17 21:15:48 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.82 $ */
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
static boolean com_pager_core(const char *, const char *, boolean, char **);

short
quest_info(int typ)
{
    switch (typ) {
    case 0:
        return gu.urole.questarti;
    case MS_LEADER:
        return gu.urole.ldrnum;
    case MS_NEMESIS:
        return gu.urole.neminum;
    case MS_GUARDIAN:
        return gu.urole.guardnum;
    default:
        impossible("quest_info(%d)", typ);
    }
    return 0;
}

/* return your role leader's name */
const char *
ldrname(void)
{
    int i = gu.urole.ldrnum;

    Sprintf(gn.nambuf, "%s%s", type_is_pname(&mons[i]) ? "" : "the ",
            mons[i].pmnames[NEUTRAL]);
    return gn.nambuf;
}

/* return your intermediate target string */
static const char *
intermed(void)
{
    return gu.urole.intermed;
}

boolean
is_quest_artifact(struct obj *otmp)
{
    return (boolean) (otmp->oartifact == gu.urole.questarti);
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
        qarti = find_qarti(gi.invent);
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
        for (mtmp = gm.migrating_mons; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((qarti = find_qarti(mtmp->minvent)) != 0)
                break;
        }
        if (!qarti)
            qarti = find_qarti(gm.migrating_objs);
    }
    if (!qarti && (whichchains & (1 << OBJ_BURIED)) != 0)
        qarti = find_qarti(gl.level.buriedobjlist);

    return qarti;
}

/* return your role nemesis' name */
static const char *
neminame(void)
{
    int i = gu.urole.neminum;

    Sprintf(gn.nambuf, "%s%s", type_is_pname(&mons[i]) ? "" : "the ",
            mons[i].pmnames[NEUTRAL]);
    return gn.nambuf;
}

static const char *
guardname(void) /* return your role leader's guard monster name */
{
    int i = gu.urole.guardnum;

    return mons[i].pmnames[NEUTRAL];
}

static const char *
homebase(void) /* return your role leader's location */
{
    return gu.urole.homebase;
}

/* returns 1 if nemesis death message mentions noxious fumes, otherwise 0;
   does not display the message */
int
stinky_nemesis(struct monst *mon)
{
    char *mesg = 0;
    int res = 0;

#if 0
    /* get the quest text for dying nemesis; don't assume that mon is
       hero's own role's nemesis (overkill since m_detach() and nemdead()
       both make that assumption--valid for normal play but not necessarily
       valid for wizard mode) */
    int r, mndx = monsndx(mon->data);
    for (r = 0; roles[r].name.m || roles[r].name.f; ++r)
        if (roles[r].neminum == mndx) {
            (void) com_pager_core(roles[r].filecode, "killed_nemesis",
                                  FALSE, &mesg);
            break;
        }
#else
    nhUse(mon);
    /* since nemdead() just gave the message for hero's nemesis even if 'mon'
       is some other role's nemesis (feasible in wizard mode), base any gas
       cloud on the text that was shown even if not appropriate for 'mon' */
    (void) com_pager_core(gu.urole.filecode, "killed_nemesis", FALSE, &mesg);
#endif

    /* this is somewhat fragile; it assumes that when both {noxious or
       poisonous or toxic} and {gas or fumes} are present, the latter
       refers to the former rather than to something unrelated; it does
       make sure that fumes occurs after noxious rather than before */
    if (mesg) {
        char *p;

        /* change newlines into spaces to cope with "...noxious\nfumes..." */
        (void) strNsubst(mesg, "\n", " ", 0);

        if (((p = strstri(mesg, "noxious")) != 0
             || (p = strstri(mesg, "poisonous")) != 0
             || (p = strstri(mesg, "toxic")) != 0)
            && (strstri(p, " gas") || strstri(p, " fumes")))
            res = 1;

        free((genericptr_t) mesg);
    }
    return res;
}

/* replace deity, leader, nemesis, or artifact name with pronoun;
   overwrites cvt_buf[] */
static void
qtext_pronoun(
    char who,   /* 'd' => deity, 'l' => leader, 'n' => nemesis, 'o' => arti */
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
        && (strstri(gc.cvt_buf, "Eyes ")
            || strcmpi(gc.cvt_buf, makesingular(gc.cvt_buf)))) {
        pnoun = (lwhich == 'h') ? "they"
                : (lwhich == 'i') ? "them"
                : (lwhich == 'j') ? "their" : "?";
    } else {
        godgend = (who == 'd') ? gq.quest_status.godgend
            : (who == 'l') ? gq.quest_status.ldrgend
            : (who == 'n') ? gq.quest_status.nemgend
            : 2; /* default to neuter */
        pnoun = (lwhich == 'h') ? genders[godgend].he
                : (lwhich == 'i') ? genders[godgend].him
                : (lwhich == 'j') ? genders[godgend].his : "?";
    }
    Strcpy(gc.cvt_buf, pnoun);
    /* capitalize for H,I,J */
    if (lwhich != which)
        gc.cvt_buf[0] = highc(gc.cvt_buf[0]);
    return;
}

static void
convert_arg(char c)
{
    register const char *str;

    switch (c) {
    case 'p':
        str = gp.plname;
        break;
    case 'c':
        str = (flags.female && gu.urole.name.f) ? gu.urole.name.f
                                               : gu.urole.name.m;
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
        str = the(artiname(gu.urole.questarti));
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
        str = gd.dungeons[0].dname;
        break;
    case '%':
        str = "%";
        break;
    default:
        str = "";
        break;
    }
    Strcpy(gc.cvt_buf, str);
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
                    Strcat(cc, An(gc.cvt_buf));
                    cc += strlen(cc);
                    continue; /* for */
                case 'a':
                    Strcat(cc, an(gc.cvt_buf));
                    cc += strlen(cc);
                    continue; /* for */

                /* capitalize */
                case 'C':
                    gc.cvt_buf[0] = highc(gc.cvt_buf[0]);
                    break;

                /* replace name with pronoun;
                   valid for %d, %l, %n, and %o */
                case 'h': /* he/she */
                case 'H': /* He/She */
                case 'i': /* him/her */
                case 'I':
                case 'j': /* his/her */
                case 'J':
                    if (strchr("dlno", lowc(*(c - 1))))
                        qtext_pronoun(*(c - 1), *c);
                    else
                        --c; /* default action */
                    break;

                /* pluralize */
                case 'P':
                    gc.cvt_buf[0] = highc(gc.cvt_buf[0]);
                    /*FALLTHRU*/
                case 'p':
                    Strcpy(gc.cvt_buf, makeplural(gc.cvt_buf));
                    break;

                /* append possessive suffix */
                case 'S':
                    gc.cvt_buf[0] = highc(gc.cvt_buf[0]);
                    /*FALLTHRU*/
                case 's':
                    Strcpy(gc.cvt_buf, s_suffix(gc.cvt_buf));
                    break;

                /* strip any "the" prefix */
                case 't':
                    if (!strncmpi(gc.cvt_buf, "the ", 4)) {
                        Strcat(cc, &gc.cvt_buf[4]);
                        cc += strlen(cc);
                        continue; /* for */
                    }
                    break;

                default:
                    --c; /* undo switch increment */
                    break;
                }
                Strcat(cc, gc.cvt_buf);
                cc += strlen(gc.cvt_buf);
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
    char in_line[BUFSZ], out_line[BUFSZ];
    const char *msgp = str, *msgend = eos((char *) str);

    while (msgp < msgend) {
        /* copynchars() will stop at newline if it finds one */
        copynchars(in_line, msgp, (int) sizeof in_line - 1);
        msgp += strlen(in_line) + 1;

        convert_line(in_line, out_line);
        pline("%s", out_line);
    }
}

static void
deliver_by_window(const char *msg, int how)
{
    char in_line[BUFSZ], out_line[BUFSZ];
    const char *msgp = msg, *msgend = eos((char *) msg);
    winid datawin = create_nhwindow(how);

    while (msgp < msgend) {
        /* copynchars() will stop at newline if it finds one */
        copynchars(in_line, msgp, (int) sizeof in_line - 1);
        msgp += strlen(in_line) + 1;

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
    if (gp.program_state.wizkit_wishing)
        return TRUE;
    return FALSE;
}

static boolean
com_pager_core(
    const char *section,
    const char *msgid,
    boolean showerror,
    char **rawtext)
{
    static const char *const howtoput[] = {
        "pline", "window", "text", "menu", "default", NULL
    };
    static const int howtoput2i[] = { 1, 2, 2, 3, 0, 0 };
    int output;
    lua_State *L;
    char *text = NULL, *synopsis = NULL, *fallback_msgid = NULL;
    boolean res = FALSE;
    nhl_sandbox_info sbi = {NHL_SB_SAFE, 0, 0, 0};

    if (skip_pager(TRUE))
        return FALSE;

    L = nhl_init(&sbi);
    if (!L) {
        if (showerror)
            impossible("com_pager: nhl_init() failed");
        goto compagerdone;
    }

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

    text = get_table_str_opt(L, "text", NULL);
    if (rawtext) {
        *rawtext = dupstr(text);
        res = TRUE;
        goto compagerdone;
    }
    synopsis = get_table_str_opt(L, "synopsis", NULL);
    output = howtoput2i[get_table_option(L, "output", "default", howtoput)];

    if (!text) {
        int nelems;

        lua_len(L, -1);
        nelems = (int) lua_tointeger(L, -1);
        lua_pop(L, 1);
        if (nelems < 2) {
            if (showerror)
                impossible(
              "com_pager: questtext[%s][%s] in %s is not an array of strings",
                           section, fallback_msgid ? fallback_msgid : msgid,
                           QTEXT_FILE);
            goto compagerdone;
        }
        nelems = rn2(nelems) + 1;
        lua_pushinteger(L, nelems);
        lua_gettable(L, -2);
        text = dupstr(luaL_checkstring(L, -1));
    }

    /* switch from by_pline to by_window if line has multiple segments or
       is unreasonably long (the latter ought to checked after formatting
       conversions rather than before...) */
    if (output == 0 && (strchr(text, '\n') || strlen(text) >= BUFSZ - 1)) {
        output = 2;

        /*
         * FIXME:  should update quest.lua to include proper synopsis line
         * for any item subject to having its delivery converted to by_window.
         */
        if (!synopsis) {
            char tmpbuf[BUFSZ];

            Sprintf(tmpbuf, "[%.*s]", BUFSZ - 1 - 2, text);
            /* change every newline character to a space */
            (void) strNsubst(tmpbuf, "\n", " ", 0);
            synopsis = dupstr(tmpbuf);
        }
    }

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
    (void) com_pager_core("common", msgid, TRUE, (char **) 0);
}

void
qt_pager(const char *msgid)
{
    if (!com_pager_core(gu.urole.filecode, msgid, FALSE, (char **) 0))
        (void) com_pager_core("common", msgid, TRUE, (char **) 0);
}

struct permonst *
qt_montype(void)
{
    int qpm;

    if (rn2(5)) {
        qpm = gu.urole.enemy1num;
        if (qpm != NON_PM && rn2(5) && !(gm.mvitals[qpm].mvflags & G_GENOD))
            return &mons[qpm];
        return mkclass(gu.urole.enemy1sym, 0);
    }
    qpm = gu.urole.enemy2num;
    if (qpm != NON_PM && rn2(5) && !(gm.mvitals[qpm].mvflags & G_GENOD))
        return &mons[qpm];
    return mkclass(gu.urole.enemy2sym, 0);
}

/* special levels can include a custom arrival message; display it */
void
deliver_splev_message(void)
{
    /* there's no provision for delivering via window instead of pline */
    if (gl.lev_message) {
        deliver_by_pline(gl.lev_message);

        free((genericptr_t) gl.lev_message);
        gl.lev_message = NULL;
    }
}

/*questpgr.c*/
