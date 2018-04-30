/* NetHack 3.6	sp_lev.h	$NHDT-Date: 1524287214 2018/04/21 05:06:54 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.23 $ */
/* Copyright (c) 1989 by Jean-Christophe Collet			  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SP_LEV_H
#define SP_LEV_H

/* wall directions */
#define W_NORTH 1
#define W_SOUTH 2
#define W_EAST 4
#define W_WEST 8
#define W_ANY (W_NORTH | W_SOUTH | W_EAST | W_WEST)

/* MAP limits */
#define MAP_X_LIM 76
#define MAP_Y_LIM 21

/* Per level flags */
#define NOTELEPORT 0x00000001L
#define HARDFLOOR 0x00000002L
#define NOMMAP 0x00000004L
#define SHORTSIGHTED 0x00000008L
#define ARBOREAL 0x00000010L
#define MAZELEVEL 0x00000020L
#define PREMAPPED 0x00000040L /* premapped level & sokoban rules */
#define SHROUD 0x00000080L
#define GRAVEYARD 0x00000100L
#define ICEDPOOLS 0x00000200L /* for ice locations: ICED_POOL vs ICED_MOAT \
                                 */
#define SOLIDIFY 0x00000400L  /* outer areas are nondiggable & nonpasswall */
#define CORRMAZE 0x00000800L  /* for maze levels only */
#define CHECK_INACCESSIBLES 0x00001000L /* check for inaccessible areas and
   generate ways to escape from them */

/* different level layout initializers */
enum lvlinit_types {
    LVLINIT_NONE = 0,
    LVLINIT_SOLIDFILL,
    LVLINIT_MAZEGRID,
    LVLINIT_MINES,
    LVLINIT_ROGUE
};

/* max. layers of object containment */
#define MAX_CONTAINMENT 10

/* max. # of random registers */
#define MAX_REGISTERS 10

/* max. nested depth of subrooms */
#define MAX_NESTED_ROOMS 5

/* max. # of opcodes per special level */
#define SPCODER_MAX_RUNTIME 65536

/* Opcodes for creating the level
 * If you change these, also change opcodestr[] in util/lev_main.c
 */
enum opcode_defs {
    SPO_NULL = 0,
    SPO_MESSAGE,
    SPO_MONSTER,
    SPO_OBJECT,
    SPO_ENGRAVING,
    SPO_ROOM,
    SPO_SUBROOM,
    SPO_DOOR,
    SPO_STAIR,
    SPO_LADDER,
    SPO_ALTAR,
    SPO_FOUNTAIN,
    SPO_SINK,
    SPO_POOL,
    SPO_TRAP,
    SPO_GOLD,
    SPO_CORRIDOR,
    SPO_LEVREGION,
    SPO_DRAWBRIDGE,
    SPO_MAZEWALK,
    SPO_NON_DIGGABLE,
    SPO_NON_PASSWALL,
    SPO_WALLIFY,
    SPO_MAP,
    SPO_ROOM_DOOR,
    SPO_REGION,
    SPO_MINERALIZE,
    SPO_CMP,
    SPO_JMP,
    SPO_JL,
    SPO_JLE,
    SPO_JG,
    SPO_JGE,
    SPO_JE,
    SPO_JNE,
    SPO_TERRAIN,
    SPO_REPLACETERRAIN,
    SPO_EXIT,
    SPO_ENDROOM,
    SPO_POP_CONTAINER,
    SPO_PUSH,
    SPO_POP,
    SPO_RN2,
    SPO_DEC,
    SPO_INC,
    SPO_MATH_ADD,
    SPO_MATH_SUB,
    SPO_MATH_MUL,
    SPO_MATH_DIV,
    SPO_MATH_MOD,
    SPO_MATH_SIGN,
    SPO_COPY,
    SPO_END_MONINVENT,
    SPO_GRAVE,
    SPO_FRAME_PUSH,
    SPO_FRAME_POP,
    SPO_CALL,
    SPO_RETURN,
    SPO_INITLEVEL,
    SPO_LEVEL_FLAGS,
    SPO_VAR_INIT, /* variable_name data */
    SPO_SHUFFLE_ARRAY,
    SPO_DICE,

    SPO_SEL_ADD,
    SPO_SEL_POINT,
    SPO_SEL_RECT,
    SPO_SEL_FILLRECT,
    SPO_SEL_LINE,
    SPO_SEL_RNDLINE,
    SPO_SEL_GROW,
    SPO_SEL_FLOOD,
    SPO_SEL_RNDCOORD,
    SPO_SEL_ELLIPSE,
    SPO_SEL_FILTER,
    SPO_SEL_GRADIENT,
    SPO_SEL_COMPLEMENT,

    MAX_SP_OPCODES
};

/* MONSTER and OBJECT can take a variable number of parameters,
 * they also pop different # of values from the stack. So,
 * first we pop a value that tells what the _next_ value will
 * mean.
 */
/* MONSTER */
enum sp_mon_var_flags {
    SP_M_V_PEACEFUL = 0,
    SP_M_V_ALIGN,
    SP_M_V_ASLEEP,
    SP_M_V_APPEAR,
    SP_M_V_NAME,
    SP_M_V_FEMALE,
    SP_M_V_INVIS,
    SP_M_V_CANCELLED,
    SP_M_V_REVIVED,
    SP_M_V_AVENGE,
    SP_M_V_FLEEING,
    SP_M_V_BLINDED,
    SP_M_V_PARALYZED,
    SP_M_V_STUNNED,
    SP_M_V_CONFUSED,
    SP_M_V_SEENTRAPS,

    SP_M_V_END
};

/* OBJECT */
enum sp_obj_var_flags {
    SP_O_V_SPE = 0,
    SP_O_V_CURSE,
    SP_O_V_CORPSENM,
    SP_O_V_NAME,
    SP_O_V_QUAN,
    SP_O_V_BURIED,
    SP_O_V_LIT,
    SP_O_V_ERODED,
    SP_O_V_LOCKED,
    SP_O_V_TRAPPED,
    SP_O_V_RECHARGED,
    SP_O_V_INVIS,
    SP_O_V_GREASED,
    SP_O_V_BROKEN,
    SP_O_V_COORD,

    SP_O_V_END
};

/* When creating objects, we need to know whether
 * it's a container and/or contents.
 */
#define SP_OBJ_CONTENT 0x1
#define SP_OBJ_CONTAINER 0x2

/* SPO_FILTER types */
#define SPOFILTER_PERCENT 0
#define SPOFILTER_SELECTION 1
#define SPOFILTER_MAPCHAR 2

/* gradient filter types */
#define SEL_GRADIENT_RADIAL 0
#define SEL_GRADIENT_SQUARE 1

/* variable types */
#define SPOVAR_NULL 0x00
#define SPOVAR_INT 0x01      /* l */
#define SPOVAR_STRING 0x02   /* str */
#define SPOVAR_VARIABLE 0x03 /* str (contains the variable name) */
#define SPOVAR_COORD \
    0x04 /* coordinate, encoded in l; use SP_COORD_X() and SP_COORD_Y() */
#define SPOVAR_REGION 0x05  /* region, encoded in l; use SP_REGION_X1() etc \
                               */
#define SPOVAR_MAPCHAR 0x06 /* map char, in l */
#define SPOVAR_MONST                                                         \
    0x07 /* monster class & specific monster, encoded in l; use SP_MONST_... \
            */
#define SPOVAR_OBJ                                                 \
    0x08 /* object class & specific object type, encoded in l; use \
            SP_OBJ_... */
#define SPOVAR_SEL 0x09   /* selection. char[COLNO][ROWNO] in str */
#define SPOVAR_ARRAY 0x40 /* used in splev_var & lc_vardefs, not in opvar */

#define SP_COORD_IS_RANDOM 0x01000000
/* Humidity flags for get_location() and friends, used with
 * SP_COORD_PACK_RANDOM() */
#define DRY         0x01
#define WET         0x02
#define HOT         0x04
#define SOLID       0x08
#define ANY_LOC     0x10 /* even outside the level */
#define NO_LOC_WARN 0x20 /* no complaints and set x & y to -1, if no loc */
#define SPACELOC    0x40 /* like DRY, but accepts furniture too */

#define SP_COORD_X(l) (l & 0xff)
#define SP_COORD_Y(l) ((l >> 16) & 0xff)
#define SP_COORD_PACK(x, y) (((x) & 0xff) + (((y) & 0xff) << 16))
#define SP_COORD_PACK_RANDOM(f) (SP_COORD_IS_RANDOM | (f))

#define SP_REGION_X1(l) (l & 0xff)
#define SP_REGION_Y1(l) ((l >> 8) & 0xff)
#define SP_REGION_X2(l) ((l >> 16) & 0xff)
#define SP_REGION_Y2(l) ((l >> 24) & 0xff)
#define SP_REGION_PACK(x1, y1, x2, y2) \
    (((x1) & 0xff) + (((y1) & 0xff) << 8) + (((x2) & 0xff) << 16) \
     + (((y2) & 0xff) << 24))

/* permonst index, object index, and lit value might be negative;
 * add 10 to accept -1 through -9 while forcing non-negative for bit shift
 */
#define SP_MONST_CLASS(l) ((l) & 0xff)
#define SP_MONST_PM(l) ((((l) >> 8) & 0xffff) - 10)
#define SP_MONST_PACK(pm, cls) (((10 + (pm)) << 8) | ((cls) & 0xff))

#define SP_OBJ_CLASS(l) ((l) & 0xff)
#define SP_OBJ_TYP(l) ((((l) >> 8) & 0xffff) - 10)
#define SP_OBJ_PACK(ob, cls) (((10 + (ob)) << 8) | ((cls) & 0xff))

#define SP_MAPCHAR_TYP(l) ((l) & 0xff)
#define SP_MAPCHAR_LIT(l) ((((l) >> 8) & 0xffff) - 10)
#define SP_MAPCHAR_PACK(typ, lit) (((10 + (lit)) << 8) | ((typ) & 0xff))

struct splev_var {
    struct splev_var *next;
    char *name;
    xchar svtyp; /* SPOVAR_foo */
    union {
        struct opvar *value;
        struct opvar **arrayvalues;
    } data;
    long array_len;
};

struct splevstack {
    long depth;
    long depth_alloc;
    struct opvar **stackdata;
};

struct sp_frame {
    struct sp_frame *next;
    struct splevstack *stack;
    struct splev_var *variables;
    long n_opcode;
};

struct sp_coder {
    struct splevstack *stack;
    struct sp_frame *frame;
    int premapped;
    boolean solidify;
    struct mkroom *croom;
    struct mkroom *tmproomlist[MAX_NESTED_ROOMS + 1];
    boolean failed_room[MAX_NESTED_ROOMS + 1];
    int n_subroom;
    boolean exit_script;
    int lvl_is_joined;
    boolean check_inaccessibles;

    int opcode;          /* current opcode */
    struct opvar *opdat; /* current push data (req. opcode == SPO_PUSH) */
};

/* special level coder CPU flags */
#define SP_CPUFLAG_LT 1
#define SP_CPUFLAG_GT 2
#define SP_CPUFLAG_EQ 4
#define SP_CPUFLAG_ZERO 8

/*
 * Structures manipulated by the special levels loader & compiler
 */

#define packed_coord long
typedef struct {
    xchar is_random;
    long getloc_flags;
    int x, y;
} unpacked_coord;

typedef struct {
    int cmp_what;
    int cmp_val;
} opcmp;

typedef struct {
    long jmp_target;
} opjmp;

typedef union str_or_len {
    char *str;
    int len;
} Str_or_Len;

typedef struct {
    xchar init_style; /* one of LVLINIT_foo */
    long flags;
    schar filling;
    boolean init_present, padding;
    char fg, bg;
    boolean smoothed, joined;
    xchar lit, walled;
    boolean icedpools;
} lev_init;

typedef struct {
    xchar wall, pos, secret, mask;
} room_door;

typedef struct {
    packed_coord coord;
    xchar x, y, type;
} spltrap;

typedef struct {
    Str_or_Len name, appear_as;
    short id;
    aligntyp align;
    packed_coord coord;
    xchar x, y, class, appear;
    schar peaceful, asleep;
    short female, invis, cancelled, revived, avenge, fleeing, blinded,
        paralyzed, stunned, confused;
    long seentraps;
    short has_invent;
} monster;

typedef struct {
    Str_or_Len name;
    int corpsenm;
    short id, spe;
    packed_coord coord;
    xchar x, y, class, containment;
    schar curse_state;
    int quan;
    short buried;
    short lit;
    short eroded, locked, trapped, recharged, invis, greased, broken;
} object;

typedef struct {
    packed_coord coord;
    xchar x, y;
    aligntyp align;
    xchar shrine;
} altar;

typedef struct {
    xchar x1, y1, x2, y2;
    xchar rtype, rlit, rirreg;
} region;

typedef struct {
    xchar ter, tlit;
} terrain;

typedef struct {
    xchar chance;
    xchar x1, y1, x2, y2;
    xchar fromter, toter, tolit;
} replaceterrain;

/* values for rtype are defined in dungeon.h */
typedef struct {
    struct {
        xchar x1, y1, x2, y2;
    } inarea;
    struct {
        xchar x1, y1, x2, y2;
    } delarea;
    boolean in_islev, del_islev;
    xchar rtype, padding;
    Str_or_Len rname;
} lev_region;

typedef struct {
    struct {
        xchar room;
        xchar wall;
        xchar door;
    } src, dest;
} corridor;

typedef struct _room {
    Str_or_Len name;
    Str_or_Len parent;
    xchar x, y, w, h;
    xchar xalign, yalign;
    xchar rtype, chance, rlit, filled, joined;
} room;

typedef struct {
    schar zaligntyp;
    schar keep_region;
    schar halign, valign;
    char xsize, ysize;
    char **map;
} mazepart;

typedef struct {
    int opcode;
    struct opvar *opdat;
} _opcode;

typedef struct {
    _opcode *opcodes;
    long n_opcodes;
} sp_lev;

typedef struct {
    xchar x, y, direction, count, lit;
    char typ;
} spill;

/* only used by lev_comp */
struct lc_funcdefs_parm {
    char *name;
    char parmtype;
    struct lc_funcdefs_parm *next;
};

struct lc_funcdefs {
    struct lc_funcdefs *next;
    char *name;
    long addr;
    sp_lev code;
    long n_called;
    struct lc_funcdefs_parm *params;
    long n_params;
};

struct lc_vardefs {
    struct lc_vardefs *next;
    char *name;
    long var_type; /* SPOVAR_foo */
    long n_used;
};

struct lc_breakdef {
    struct lc_breakdef *next;
    struct opvar *breakpoint;
    int break_depth;
};

/*
 * Quick! Avert your eyes while you still have a chance!
 */
#ifdef SPEC_LEV
/* compiling lev_comp rather than nethack */
#ifdef USE_OLDARGS
#ifndef VA_TYPE
typedef const char *vA;
#define VA_TYPE
#endif
#undef VA_ARGS  /* redefine with the maximum number actually used */
#undef VA_SHIFT /* ditto */
#undef VA_PASS1
#define VA_ARGS                                                         \
    arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, \
        arg12, arg13, arg14
/* Unlike in the core, lev_comp's VA_SHIFT should be completely safe,
   because callers always pass all these arguments. */
#define VA_SHIFT()                                                       \
    (arg1 = arg2, arg2 = arg3, arg3 = arg4, arg4 = arg5, arg5 = arg6,    \
     arg6 = arg7, arg7 = arg8, arg8 = arg9, arg9 = arg10, arg10 = arg11, \
     arg11 = arg12, arg12 = arg13, arg13 = arg14, arg14 = 0)
/* standard NULL may be either (void *)0 or plain 0, both of
   which would need to be explicitly cast to (char *) here */
#define VA_PASS1(a1)                                                         \
    (vA) a1, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, \
        (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0
#define VA_PASS2(a1, a2)                                              \
    (vA) a1, (vA) a2, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, \
        (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0
#define VA_PASS3(a1, a2, a3)                                           \
    (vA) a1, (vA) a2, (vA) a3, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, \
        (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0
#define VA_PASS4(a1, a2, a3, a4)                                        \
    (vA) a1, (vA) a2, (vA) a3, (vA) a4, (vA) 0, (vA) 0, (vA) 0, (vA) 0, \
        (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0
#define VA_PASS5(a1, a2, a3, a4, a5)                                     \
    (vA) a1, (vA) a2, (vA) a3, (vA) a4, (vA) a5, (vA) 0, (vA) 0, (vA) 0, \
        (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0
#define VA_PASS7(a1, a2, a3, a4, a5, a6, a7)                               \
    (vA) a1, (vA) a2, (vA) a3, (vA) a4, (vA) a5, (vA) a6, (vA) a7, (vA) 0, \
        (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0
#define VA_PASS8(a1, a2, a3, a4, a5, a6, a7, a8)                            \
    (vA) a1, (vA) a2, (vA) a3, (vA) a4, (vA) a5, (vA) a6, (vA) a7, (vA) a8, \
        (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0
#define VA_PASS9(a1, a2, a3, a4, a5, a6, a7, a8, a9)                        \
    (vA) a1, (vA) a2, (vA) a3, (vA) a4, (vA) a5, (vA) a6, (vA) a7, (vA) a8, \
        (vA) a9, (vA) 0, (vA) 0, (vA) 0, (vA) 0, (vA) 0
#define VA_PASS14(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13,   \
                  a14)                                                      \
    (vA) a1, (vA) a2, (vA) a3, (vA) a4, (vA) a5, (vA) a6, (vA) a7, (vA) a8, \
        (vA) a9, (vA) a10, (vA) a11, (vA) a12, (vA) a13, (vA) a14
#else /*!USE_OLDARGS*/
/* USE_STDARG and USE_VARARGS don't need to pass dummy arguments
   or cast real ones */
#define VA_PASS1(a1) a1
#define VA_PASS2(a1, a2) a1, a2
#define VA_PASS3(a1, a2, a3) a1, a2, a3
#define VA_PASS4(a1, a2, a3, a4) a1, a2, a3, a4
#define VA_PASS5(a1, a2, a3, a4, a5) a1, a2, a3, a4, a5
#define VA_PASS7(a1, a2, a3, a4, a5, a6, a7) a1, a2, a3, a4, a5, a6, a7
#define VA_PASS8(a1, a2, a3, a4, a5, a6, a7, a8) \
    a1, a2, a3, a4, a5, a6, a7, a8
#define VA_PASS9(a1, a2, a3, a4, a5, a6, a7, a8, a9) \
    a1, a2, a3, a4, a5, a6, a7, a8, a9
#define VA_PASS14(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, \
                  a14)                                                    \
    a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14
#endif /*?USE_OLDARGS*/
/* You were warned to avert your eyes.... */
#endif /*SPEC_LEV*/

#endif /* SP_LEV_H */
