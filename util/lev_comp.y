%{
/*	SCCS Id: @(#)lev_yacc.c	3.4	2000/01/17	*/
/*	Copyright (c) 1989 by Jean-Christophe Collet */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the Level Compiler code
 * It may handle special mazes & special room-levels
 */

/* In case we're using bison in AIX.  This definition must be
 * placed before any other C-language construct in the file
 * excluding comments and preprocessor directives (thanks IBM
 * for this wonderful feature...).
 *
 * Note: some cpps barf on this 'undefined control' (#pragma).
 * Addition of the leading space seems to prevent barfage for now,
 * and AIX will still see the directive.
 */
#ifdef _AIX
 #pragma alloca		/* keep leading space! */
#endif

#include "hack.h"
#include "sp_lev.h"

#define MAX_REGISTERS	10
#define ERR		(-1)
/* many types of things are put in chars for transference to NetHack.
 * since some systems will use signed chars, limit everybody to the
 * same number for portability.
 */
#define MAX_OF_TYPE	128

#define New(type)		\
	(type *) memset((genericptr_t)alloc(sizeof(type)), 0, sizeof(type))
#define NewTab(type, size)	(type **) alloc(sizeof(type *) * size)
#define Free(ptr)		free((genericptr_t)ptr)

extern void FDECL(yyerror, (const char *));
extern void FDECL(yywarning, (const char *));
extern int NDECL(yylex);
int NDECL(yyparse);

extern int FDECL(get_floor_type, (CHAR_P));
extern int FDECL(get_room_type, (char *));
extern int FDECL(get_trap_type, (char *));
extern int FDECL(get_monster_id, (char *,CHAR_P));
extern int FDECL(get_object_id, (char *,CHAR_P));
extern boolean FDECL(check_monster_char, (CHAR_P));
extern boolean FDECL(check_object_char, (CHAR_P));
extern char FDECL(what_map_char, (CHAR_P));
extern void FDECL(scan_map, (char *));
extern void NDECL(wallify_map);
extern boolean NDECL(check_subrooms);
extern void FDECL(check_coord, (int,int,const char *));
extern void NDECL(store_part);
extern void NDECL(store_room);
extern boolean FDECL(write_level_file, (char *,splev *,specialmaze *));
extern void FDECL(free_rooms, (splev *));

static struct reg {
	int x1, y1;
	int x2, y2;
}		current_region;

static struct coord {
	int x;
	int y;
}		current_coord, current_align;

static struct size {
	int height;
	int width;
}		current_size;

char tmpmessage[256];
digpos *tmppass[32];
char *tmpmap[ROWNO];

digpos *tmpdig[MAX_OF_TYPE];
region *tmpreg[MAX_OF_TYPE];
lev_region *tmplreg[MAX_OF_TYPE];
door *tmpdoor[MAX_OF_TYPE];
drawbridge *tmpdb[MAX_OF_TYPE];
walk *tmpwalk[MAX_OF_TYPE];

room_door *tmprdoor[MAX_OF_TYPE];
trap *tmptrap[MAX_OF_TYPE];
monster *tmpmonst[MAX_OF_TYPE];
object *tmpobj[MAX_OF_TYPE];
altar *tmpaltar[MAX_OF_TYPE];
lad *tmplad[MAX_OF_TYPE];
stair *tmpstair[MAX_OF_TYPE];
gold *tmpgold[MAX_OF_TYPE];
engraving *tmpengraving[MAX_OF_TYPE];
fountain *tmpfountain[MAX_OF_TYPE];
sink *tmpsink[MAX_OF_TYPE];
pool *tmppool[MAX_OF_TYPE];

mazepart *tmppart[10];
room *tmproom[MAXNROFROOMS*2];
corridor *tmpcor[MAX_OF_TYPE];

static specialmaze maze;
static splev special_lev;
static lev_init init_lev;

static char olist[MAX_REGISTERS], mlist[MAX_REGISTERS];
static struct coord plist[MAX_REGISTERS];

int n_olist = 0, n_mlist = 0, n_plist = 0;

unsigned int nlreg = 0, nreg = 0, ndoor = 0, ntrap = 0, nmons = 0, nobj = 0;
unsigned int ndb = 0, nwalk = 0, npart = 0, ndig = 0, nlad = 0, nstair = 0;
unsigned int naltar = 0, ncorridor = 0, nrooms = 0, ngold = 0, nengraving = 0;
unsigned int nfountain = 0, npool = 0, nsink = 0, npass = 0;

static int lev_flags = 0;

unsigned int max_x_map, max_y_map;

static xchar in_room;

extern int fatal_error;
extern int want_warnings;
extern const char *fname;

%}

%union
{
	int	i;
	char*	map;
	struct {
		xchar room;
		xchar wall;
		xchar door;
	} corpos;
}


%token	<i> CHAR INTEGER BOOLEAN PERCENT
%token	<i> MESSAGE_ID MAZE_ID LEVEL_ID LEV_INIT_ID GEOMETRY_ID NOMAP_ID
%token	<i> OBJECT_ID COBJECT_ID MONSTER_ID TRAP_ID DOOR_ID DRAWBRIDGE_ID
%token	<i> MAZEWALK_ID WALLIFY_ID REGION_ID FILLING
%token	<i> RANDOM_OBJECTS_ID RANDOM_MONSTERS_ID RANDOM_PLACES_ID
%token	<i> ALTAR_ID LADDER_ID STAIR_ID NON_DIGGABLE_ID NON_PASSWALL_ID ROOM_ID
%token	<i> PORTAL_ID TELEPRT_ID BRANCH_ID LEV CHANCE_ID
%token	<i> CORRIDOR_ID GOLD_ID ENGRAVING_ID FOUNTAIN_ID POOL_ID SINK_ID NONE
%token	<i> RAND_CORRIDOR_ID DOOR_STATE LIGHT_STATE CURSE_TYPE ENGRAVING_TYPE
%token	<i> DIRECTION RANDOM_TYPE O_REGISTER M_REGISTER P_REGISTER A_REGISTER
%token	<i> ALIGNMENT LEFT_OR_RIGHT CENTER TOP_OR_BOT ALTAR_TYPE UP_OR_DOWN
%token	<i> SUBROOM_ID NAME_ID FLAGS_ID FLAG_TYPE MON_ATTITUDE MON_ALERTNESS
%token	<i> MON_APPEARANCE
%token	<i> CONTAINED
%token	<i> ',' ':' '(' ')' '[' ']'
%token	<map> STRING MAP_ID
%type	<i> h_justif v_justif trap_name room_type door_state light_state
%type	<i> alignment altar_type a_register roomfill filling door_pos
%type	<i> door_wall walled secret amount chance
%type	<i> engraving_type flags flag_list prefilled lev_region lev_init
%type	<i> monster monster_c m_register object object_c o_register
%type	<map> string maze_def level_def m_name o_name
%type	<corpos> corr_spec
%start	file

%%
file		: /* nothing */
		| levels
		;

levels		: level
		| level levels
		;

level		: maze_level
		| room_level
		;

maze_level	: maze_def flags lev_init messages regions
		  {
			unsigned i;

			if (fatal_error > 0) {
				(void) fprintf(stderr,
				"%s : %d errors detected. No output created!\n",
					fname, fatal_error);
			} else {
				maze.flags = $2;
				(void) memcpy((genericptr_t)&(maze.init_lev),
						(genericptr_t)&(init_lev),
						sizeof(lev_init));
				maze.numpart = npart;
				maze.parts = NewTab(mazepart, npart);
				for(i=0;i<npart;i++)
				    maze.parts[i] = tmppart[i];
				if (!write_level_file($1, (splev *)0, &maze)) {
					yyerror("Can't write output file!!");
					exit(EXIT_FAILURE);
				}
				npart = 0;
			}
			Free($1);
		  }
		;

room_level	: level_def flags lev_init messages rreg_init rooms corridors_def
		  {
			unsigned i;

			if (fatal_error > 0) {
			    (void) fprintf(stderr,
			      "%s : %d errors detected. No output created!\n",
					fname, fatal_error);
			} else {
				special_lev.flags = (long) $2;
				(void) memcpy(
					(genericptr_t)&(special_lev.init_lev),
					(genericptr_t)&(init_lev),
					sizeof(lev_init));
				special_lev.nroom = nrooms;
				special_lev.rooms = NewTab(room, nrooms);
				for(i=0; i<nrooms; i++)
				    special_lev.rooms[i] = tmproom[i];
				special_lev.ncorr = ncorridor;
				special_lev.corrs = NewTab(corridor, ncorridor);
				for(i=0; i<ncorridor; i++)
				    special_lev.corrs[i] = tmpcor[i];
				if (check_subrooms()) {
				    if (!write_level_file($1, &special_lev,
							  (specialmaze *)0)) {
					yyerror("Can't write output file!!");
					exit(EXIT_FAILURE);
				    }
				}
				free_rooms(&special_lev);
				nrooms = 0;
				ncorridor = 0;
			}
			Free($1);
		  }
		;

level_def	: LEVEL_ID ':' string
		  {
			if (index($3, '.'))
			    yyerror("Invalid dot ('.') in level name.");
			if ((int) strlen($3) > 8)
			    yyerror("Level names limited to 8 characters.");
			$$ = $3;
			special_lev.nrmonst = special_lev.nrobjects = 0;
			n_mlist = n_olist = 0;
		  }
		;

lev_init	: /* nothing */
		  {
			/* in case we're processing multiple files,
			   explicitly clear any stale settings */
			(void) memset((genericptr_t) &init_lev, 0,
					sizeof init_lev);
			init_lev.init_present = FALSE;
			$$ = 0;
		  }
		| LEV_INIT_ID ':' CHAR ',' CHAR ',' BOOLEAN ',' BOOLEAN ',' light_state ',' walled
		  {
			init_lev.init_present = TRUE;
			init_lev.fg = what_map_char((char) $3);
			if (init_lev.fg == INVALID_TYPE)
			    yyerror("Invalid foreground type.");
			init_lev.bg = what_map_char((char) $5);
			if (init_lev.bg == INVALID_TYPE)
			    yyerror("Invalid background type.");
			init_lev.smoothed = $7;
			init_lev.joined = $9;
			if (init_lev.joined &&
			    init_lev.fg != CORR && init_lev.fg != ROOM)
			    yyerror("Invalid foreground type for joined map.");
			init_lev.lit = $11;
			init_lev.walled = $13;
			$$ = 1;
		  }
		;

walled		: BOOLEAN
		| RANDOM_TYPE
		;

flags		: /* nothing */
		  {
			$$ = 0;
		  }
		| FLAGS_ID ':' flag_list
		  {
			$$ = lev_flags;
			lev_flags = 0;	/* clear for next user */
		  }
		;

flag_list	: FLAG_TYPE ',' flag_list
		  {
			lev_flags |= $1;
		  }
		| FLAG_TYPE
		  {
			lev_flags |= $1;
		  }
		;

messages	: /* nothing */
		| message messages
		;

message		: MESSAGE_ID ':' STRING
		  {
			int i, j;

			i = (int) strlen($3) + 1;
			j = (int) strlen(tmpmessage);
			if (i + j > 255) {
			   yyerror("Message string too long (>256 characters)");
			} else {
			    if (j) tmpmessage[j++] = '\n';
			    (void) strncpy(tmpmessage+j, $3, i - 1);
			    tmpmessage[j + i - 1] = 0;
			}
			Free($3);
		  }
		;

rreg_init	: /* nothing */
		| rreg_init init_rreg
		;

init_rreg	: RANDOM_OBJECTS_ID ':' object_list
		  {
			if(special_lev.nrobjects) {
			    yyerror("Object registers already initialized!");
			} else {
			    special_lev.nrobjects = n_olist;
			    special_lev.robjects = (char *) alloc(n_olist);
			    (void) memcpy((genericptr_t)special_lev.robjects,
					  (genericptr_t)olist, n_olist);
			}
		  }
		| RANDOM_MONSTERS_ID ':' monster_list
		  {
			if(special_lev.nrmonst) {
			    yyerror("Monster registers already initialized!");
			} else {
			    special_lev.nrmonst = n_mlist;
			    special_lev.rmonst = (char *) alloc(n_mlist);
			    (void) memcpy((genericptr_t)special_lev.rmonst,
					  (genericptr_t)mlist, n_mlist);
			  }
		  }
		;

rooms		: /* Nothing  -  dummy room for use with INIT_MAP */
		  {
			tmproom[nrooms] = New(room);
			tmproom[nrooms]->name = (char *) 0;
			tmproom[nrooms]->parent = (char *) 0;
			tmproom[nrooms]->rtype = 0;
			tmproom[nrooms]->rlit = 0;
			tmproom[nrooms]->xalign = ERR;
			tmproom[nrooms]->yalign = ERR;
			tmproom[nrooms]->x = 0;
			tmproom[nrooms]->y = 0;
			tmproom[nrooms]->w = 2;
			tmproom[nrooms]->h = 2;
			in_room = 1;
		  }
		| roomlist
		;

roomlist	: aroom
		| aroom roomlist
		;

corridors_def	: random_corridors
		| corridors
		;

random_corridors: RAND_CORRIDOR_ID
		  {
			tmpcor[0] = New(corridor);
			tmpcor[0]->src.room = -1;
			ncorridor = 1;
		  }
		;

corridors	: /* nothing */
		| corridors corridor
		;

corridor	: CORRIDOR_ID ':' corr_spec ',' corr_spec
		  {
			tmpcor[ncorridor] = New(corridor);
			tmpcor[ncorridor]->src.room = $3.room;
			tmpcor[ncorridor]->src.wall = $3.wall;
			tmpcor[ncorridor]->src.door = $3.door;
			tmpcor[ncorridor]->dest.room = $5.room;
			tmpcor[ncorridor]->dest.wall = $5.wall;
			tmpcor[ncorridor]->dest.door = $5.door;
			ncorridor++;
			if (ncorridor >= MAX_OF_TYPE) {
				yyerror("Too many corridors in level!");
				ncorridor--;
			}
		  }
		| CORRIDOR_ID ':' corr_spec ',' INTEGER
		  {
			tmpcor[ncorridor] = New(corridor);
			tmpcor[ncorridor]->src.room = $3.room;
			tmpcor[ncorridor]->src.wall = $3.wall;
			tmpcor[ncorridor]->src.door = $3.door;
			tmpcor[ncorridor]->dest.room = -1;
			tmpcor[ncorridor]->dest.wall = $5;
			ncorridor++;
			if (ncorridor >= MAX_OF_TYPE) {
				yyerror("Too many corridors in level!");
				ncorridor--;
			}
		  }
		;

corr_spec	: '(' INTEGER ',' DIRECTION ',' door_pos ')'
		  {
			if ((unsigned) $2 >= nrooms)
			    yyerror("Wrong room number!");
			$$.room = $2;
			$$.wall = $4;
			$$.door = $6;
		  }
		;

aroom		: room_def room_details
		  {
			store_room();
		  }
		| subroom_def room_details
		  {
			store_room();
		  }
		;

subroom_def	: SUBROOM_ID ':' room_type ',' light_state ',' subroom_pos ',' room_size ',' string roomfill
		  {
			tmproom[nrooms] = New(room);
			tmproom[nrooms]->parent = $11;
			tmproom[nrooms]->name = (char *) 0;
			tmproom[nrooms]->rtype = $3;
			tmproom[nrooms]->rlit = $5;
			tmproom[nrooms]->filled = $12;
			tmproom[nrooms]->xalign = ERR;
			tmproom[nrooms]->yalign = ERR;
			tmproom[nrooms]->x = current_coord.x;
			tmproom[nrooms]->y = current_coord.y;
			tmproom[nrooms]->w = current_size.width;
			tmproom[nrooms]->h = current_size.height;
			in_room = 1;
		  }
		;

room_def	: ROOM_ID ':' room_type ',' light_state ',' room_pos ',' room_align ',' room_size roomfill
		  {
			tmproom[nrooms] = New(room);
			tmproom[nrooms]->name = (char *) 0;
			tmproom[nrooms]->parent = (char *) 0;
			tmproom[nrooms]->rtype = $3;
			tmproom[nrooms]->rlit = $5;
			tmproom[nrooms]->filled = $12;
			tmproom[nrooms]->xalign = current_align.x;
			tmproom[nrooms]->yalign = current_align.y;
			tmproom[nrooms]->x = current_coord.x;
			tmproom[nrooms]->y = current_coord.y;
			tmproom[nrooms]->w = current_size.width;
			tmproom[nrooms]->h = current_size.height;
			in_room = 1;
		  }
		;

roomfill	: /* nothing */
		  {
			$$ = 1;
		  }
		| ',' BOOLEAN
		  {
			$$ = $2;
		  }
		;

room_pos	: '(' INTEGER ',' INTEGER ')'
		  {
			if ( $2 < 1 || $2 > 5 ||
			    $4 < 1 || $4 > 5 ) {
			    yyerror("Room position should be between 1 & 5!");
			} else {
			    current_coord.x = $2;
			    current_coord.y = $4;
			}
		  }
		| RANDOM_TYPE
		  {
			current_coord.x = current_coord.y = ERR;
		  }
		;

subroom_pos	: '(' INTEGER ',' INTEGER ')'
		  {
			if ( $2 < 0 || $4 < 0) {
			    yyerror("Invalid subroom position !");
			} else {
			    current_coord.x = $2;
			    current_coord.y = $4;
			}
		  }
		| RANDOM_TYPE
		  {
			current_coord.x = current_coord.y = ERR;
		  }
		;

room_align	: '(' h_justif ',' v_justif ')'
		  {
			current_align.x = $2;
			current_align.y = $4;
		  }
		| RANDOM_TYPE
		  {
			current_align.x = current_align.y = ERR;
		  }
		;

room_size	: '(' INTEGER ',' INTEGER ')'
		  {
			current_size.width = $2;
			current_size.height = $4;
		  }
		| RANDOM_TYPE
		  {
			current_size.height = current_size.width = ERR;
		  }
		;

room_details	: /* nothing */
		| room_details room_detail
		;

room_detail	: room_name
		| room_chance
		| room_door
		| monster_detail
		| object_detail
		| trap_detail
		| altar_detail
		| fountain_detail
		| sink_detail
		| pool_detail
		| gold_detail
		| engraving_detail
		| stair_detail
		;

room_name	: NAME_ID ':' string
		  {
			if (tmproom[nrooms]->name)
			    yyerror("This room already has a name!");
			else
			    tmproom[nrooms]->name = $3;
		  }
		;

room_chance	: CHANCE_ID ':' INTEGER
		   {
			if (tmproom[nrooms]->chance)
			    yyerror("This room already assigned a chance!");
			else if (tmproom[nrooms]->rtype == OROOM)
			    yyerror("Only typed rooms can have a chance!");
			else if ($3 < 1 || $3 > 99)
			    yyerror("The chance is supposed to be percentile.");
			else
			    tmproom[nrooms]->chance = $3;
		   }
		;

room_door	: DOOR_ID ':' secret ',' door_state ',' door_wall ',' door_pos
		  {
			/* ERR means random here */
			if ($7 == ERR && $9 != ERR) {
		     yyerror("If the door wall is random, so must be its pos!");
			} else {
			    tmprdoor[ndoor] = New(room_door);
			    tmprdoor[ndoor]->secret = $3;
			    tmprdoor[ndoor]->mask = $5;
			    tmprdoor[ndoor]->wall = $7;
			    tmprdoor[ndoor]->pos = $9;
			    ndoor++;
			    if (ndoor >= MAX_OF_TYPE) {
				    yyerror("Too many doors in room!");
				    ndoor--;
			    }
			}
		  }
		;

secret		: BOOLEAN
		| RANDOM_TYPE
		;

door_wall	: DIRECTION
		| RANDOM_TYPE
		;

door_pos	: INTEGER
		| RANDOM_TYPE
		;

maze_def	: MAZE_ID ':' string ',' filling
		  {
			maze.filling = (schar) $5;
			if (index($3, '.'))
			    yyerror("Invalid dot ('.') in level name.");
			if ((int) strlen($3) > 8)
			    yyerror("Level names limited to 8 characters.");
			$$ = $3;
			in_room = 0;
			n_plist = n_mlist = n_olist = 0;
		  }
		;

filling		: CHAR
		  {
			$$ = get_floor_type((char)$1);
		  }
		| RANDOM_TYPE
		  {
			$$ = -1;
		  }
		;

regions		: aregion
		| aregion regions
		;

aregion		: map_definition reg_init map_details
		  {
			store_part();
		  }
		;

map_definition	: NOMAP_ID
		  {
			tmppart[npart] = New(mazepart);
			tmppart[npart]->halign = 1;
			tmppart[npart]->valign = 1;
			tmppart[npart]->nrobjects = 0;
			tmppart[npart]->nloc = 0;
			tmppart[npart]->nrmonst = 0;
			tmppart[npart]->xsize = 1;
			tmppart[npart]->ysize = 1;
			tmppart[npart]->map = (char **) alloc(sizeof(char *));
			tmppart[npart]->map[0] = (char *) alloc(1);
			tmppart[npart]->map[0][0] = STONE;
			max_x_map = COLNO-1;
			max_y_map = ROWNO;
		  }
		| map_geometry MAP_ID
		  {
			tmppart[npart] = New(mazepart);
			tmppart[npart]->halign = $<i>1 % 10;
			tmppart[npart]->valign = $<i>1 / 10;
			tmppart[npart]->nrobjects = 0;
			tmppart[npart]->nloc = 0;
			tmppart[npart]->nrmonst = 0;
			scan_map($2);
			Free($2);
		  }
		;

map_geometry	: GEOMETRY_ID ':' h_justif ',' v_justif
		  {
			$<i>$ = $<i>3 + ($<i>5 * 10);
		  }
		;

h_justif	: LEFT_OR_RIGHT
		| CENTER
		;

v_justif	: TOP_OR_BOT
		| CENTER
		;

reg_init	: /* nothing */
		| reg_init init_reg
		;

init_reg	: RANDOM_OBJECTS_ID ':' object_list
		  {
			if (tmppart[npart]->nrobjects) {
			    yyerror("Object registers already initialized!");
			} else {
			    tmppart[npart]->robjects = (char *)alloc(n_olist);
			    (void) memcpy((genericptr_t)tmppart[npart]->robjects,
					  (genericptr_t)olist, n_olist);
			    tmppart[npart]->nrobjects = n_olist;
			}
		  }
		| RANDOM_PLACES_ID ':' place_list
		  {
			if (tmppart[npart]->nloc) {
			    yyerror("Location registers already initialized!");
			} else {
			    register int i;
			    tmppart[npart]->rloc_x = (char *) alloc(n_plist);
			    tmppart[npart]->rloc_y = (char *) alloc(n_plist);
			    for(i=0;i<n_plist;i++) {
				tmppart[npart]->rloc_x[i] = plist[i].x;
				tmppart[npart]->rloc_y[i] = plist[i].y;
			    }
			    tmppart[npart]->nloc = n_plist;
			}
		  }
		| RANDOM_MONSTERS_ID ':' monster_list
		  {
			if (tmppart[npart]->nrmonst) {
			    yyerror("Monster registers already initialized!");
			} else {
			    tmppart[npart]->rmonst = (char *) alloc(n_mlist);
			    (void) memcpy((genericptr_t)tmppart[npart]->rmonst,
					  (genericptr_t)mlist, n_mlist);
			    tmppart[npart]->nrmonst = n_mlist;
			}
		  }
		;

object_list	: object
		  {
			if (n_olist < MAX_REGISTERS)
			    olist[n_olist++] = $<i>1;
			else
			    yyerror("Object list too long!");
		  }
		| object ',' object_list
		  {
			if (n_olist < MAX_REGISTERS)
			    olist[n_olist++] = $<i>1;
			else
			    yyerror("Object list too long!");
		  }
		;

monster_list	: monster
		  {
			if (n_mlist < MAX_REGISTERS)
			    mlist[n_mlist++] = $<i>1;
			else
			    yyerror("Monster list too long!");
		  }
		| monster ',' monster_list
		  {
			if (n_mlist < MAX_REGISTERS)
			    mlist[n_mlist++] = $<i>1;
			else
			    yyerror("Monster list too long!");
		  }
		;

place_list	: place
		  {
			if (n_plist < MAX_REGISTERS)
			    plist[n_plist++] = current_coord;
			else
			    yyerror("Location list too long!");
		  }
		| place
		  {
			if (n_plist < MAX_REGISTERS)
			    plist[n_plist++] = current_coord;
			else
			    yyerror("Location list too long!");
		  }
		 ',' place_list
		;

map_details	: /* nothing */
		| map_details map_detail
		;

map_detail	: monster_detail
		| object_detail
		| door_detail
		| trap_detail
		| drawbridge_detail
		| region_detail
		| stair_region
		| portal_region
		| teleprt_region
		| branch_region
		| altar_detail
		| fountain_detail
		| mazewalk_detail
		| wallify_detail
		| ladder_detail
		| stair_detail
		| gold_detail
		| engraving_detail
		| diggable_detail
		| passwall_detail
		;

monster_detail	: MONSTER_ID chance ':' monster_c ',' m_name ',' coordinate
		  {
			tmpmonst[nmons] = New(monster);
			tmpmonst[nmons]->x = current_coord.x;
			tmpmonst[nmons]->y = current_coord.y;
			tmpmonst[nmons]->class = $<i>4;
			tmpmonst[nmons]->peaceful = -1; /* no override */
			tmpmonst[nmons]->asleep = -1;
			tmpmonst[nmons]->align = - MAX_REGISTERS - 2;
			tmpmonst[nmons]->name.str = 0;
			tmpmonst[nmons]->appear = 0;
			tmpmonst[nmons]->appear_as.str = 0;
			tmpmonst[nmons]->chance = $2;
			tmpmonst[nmons]->id = NON_PM;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Monster");
			if ($6) {
			    int token = get_monster_id($6, (char) $<i>4);
			    if (token == ERR)
				yywarning(
			      "Invalid monster name!  Making random monster.");
			    else
				tmpmonst[nmons]->id = token;
			    Free($6);
			}
		  }
		 monster_infos
		  {
			if (++nmons >= MAX_OF_TYPE) {
			    yyerror("Too many monsters in room or mazepart!");
			    nmons--;
			}
		  }
		;

monster_infos	: /* nothing */
		| monster_infos monster_info
		;

monster_info	: ',' string
		  {
			tmpmonst[nmons]->name.str = $2;
		  }
		| ',' MON_ATTITUDE
		  {
			tmpmonst[nmons]->peaceful = $<i>2;
		  }
		| ',' MON_ALERTNESS
		  {
			tmpmonst[nmons]->asleep = $<i>2;
		  }
		| ',' alignment
		  {
			tmpmonst[nmons]->align = $<i>2;
		  }
		| ',' MON_APPEARANCE string
		  {
			tmpmonst[nmons]->appear = $<i>2;
			tmpmonst[nmons]->appear_as.str = $3;
		  }
		;

object_detail	: OBJECT_ID object_desc
		  {
		  }
		| COBJECT_ID object_desc
		  {
			/* 1: is contents of preceeding object with 2 */
			/* 2: is a container */
			/* 0: neither */
			tmpobj[nobj-1]->containment = 2;
		  }
		;

object_desc	: chance ':' object_c ',' o_name
		  {
			tmpobj[nobj] = New(object);
			tmpobj[nobj]->class = $<i>3;
			tmpobj[nobj]->corpsenm = NON_PM;
			tmpobj[nobj]->curse_state = -1;
			tmpobj[nobj]->name.str = 0;
			tmpobj[nobj]->chance = $1;
			tmpobj[nobj]->id = -1;
			if ($5) {
			    int token = get_object_id($5, $<i>3);
			    if (token == ERR)
				yywarning(
				"Illegal object name!  Making random object.");
			     else
				tmpobj[nobj]->id = token;
			    Free($5);
			}
		  }
		 ',' object_where object_infos
		  {
			if (++nobj >= MAX_OF_TYPE) {
			    yyerror("Too many objects in room or mazepart!");
			    nobj--;
			}
		  }
		;

object_where	: coordinate
		  {
			tmpobj[nobj]->containment = 0;
			tmpobj[nobj]->x = current_coord.x;
			tmpobj[nobj]->y = current_coord.y;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Object");
		  }
		| CONTAINED
		  {
			tmpobj[nobj]->containment = 1;
			/* random coordinate, will be overridden anyway */
			tmpobj[nobj]->x = -MAX_REGISTERS-1;
			tmpobj[nobj]->y = -MAX_REGISTERS-1;
		  }
		;

object_infos	: /* nothing */
		  {
			tmpobj[nobj]->spe = -127;
	/* Note below: we're trying to make as many of these optional as
	 * possible.  We clearly can't make curse_state, enchantment, and
	 * monster_id _all_ optional, since ",random" would be ambiguous.
	 * We can't even just make enchantment mandatory, since if we do that
	 * alone, ",random" requires too much lookahead to parse.
	 */
		  }
		| ',' curse_state ',' monster_id ',' enchantment optional_name
		  {
		  }
		| ',' curse_state ',' enchantment optional_name
		  {
		  }
		| ',' monster_id ',' enchantment optional_name
		  {
		  }
		;

curse_state	: RANDOM_TYPE
		  {
			tmpobj[nobj]->curse_state = -1;
		  }
		| CURSE_TYPE
		  {
			tmpobj[nobj]->curse_state = $1;
		  }
		;

monster_id	: STRING
		  {
			int token = get_monster_id($1, (char)0);
			if (token == ERR)	/* "random" */
			    tmpobj[nobj]->corpsenm = NON_PM - 1;
			else
			    tmpobj[nobj]->corpsenm = token;
			Free($1);
		  }
		;

enchantment	: RANDOM_TYPE
		  {
			tmpobj[nobj]->spe = -127;
		  }
		| INTEGER
		  {
			tmpobj[nobj]->spe = $1;
		  }
		;

optional_name	: /* nothing */
		| ',' NONE
		  {
		  }
		| ',' STRING
		  {
			tmpobj[nobj]->name.str = $2;
		  }
		;

door_detail	: DOOR_ID ':' door_state ',' coordinate
		  {
			tmpdoor[ndoor] = New(door);
			tmpdoor[ndoor]->x = current_coord.x;
			tmpdoor[ndoor]->y = current_coord.y;
			tmpdoor[ndoor]->mask = $<i>3;
			if(current_coord.x >= 0 && current_coord.y >= 0 &&
			   tmpmap[current_coord.y][current_coord.x] != DOOR &&
			   tmpmap[current_coord.y][current_coord.x] != SDOOR)
			    yyerror("Door decl doesn't match the map");
			ndoor++;
			if (ndoor >= MAX_OF_TYPE) {
				yyerror("Too many doors in mazepart!");
				ndoor--;
			}
		  }
		;

trap_detail	: TRAP_ID chance ':' trap_name ',' coordinate
		  {
			tmptrap[ntrap] = New(trap);
			tmptrap[ntrap]->x = current_coord.x;
			tmptrap[ntrap]->y = current_coord.y;
			tmptrap[ntrap]->type = $<i>4;
			tmptrap[ntrap]->chance = $2;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Trap");
			if (++ntrap >= MAX_OF_TYPE) {
				yyerror("Too many traps in room or mazepart!");
				ntrap--;
			}
		  }
		;

drawbridge_detail: DRAWBRIDGE_ID ':' coordinate ',' DIRECTION ',' door_state
		   {
		        int x, y, dir;

			tmpdb[ndb] = New(drawbridge);
			x = tmpdb[ndb]->x = current_coord.x;
			y = tmpdb[ndb]->y = current_coord.y;
			/* convert dir from a DIRECTION to a DB_DIR */
			dir = $5;
			switch(dir) {
			case W_NORTH: dir = DB_NORTH; y--; break;
			case W_SOUTH: dir = DB_SOUTH; y++; break;
			case W_EAST:  dir = DB_EAST;  x++; break;
			case W_WEST:  dir = DB_WEST;  x--; break;
			default:
			    yyerror("Invalid drawbridge direction");
			    break;
			}
			tmpdb[ndb]->dir = dir;
			if (current_coord.x >= 0 && current_coord.y >= 0 &&
			    !IS_WALL(tmpmap[y][x])) {
			    char ebuf[60];
			    Sprintf(ebuf,
				    "Wall needed for drawbridge (%02d, %02d)",
				    current_coord.x, current_coord.y);
			    yyerror(ebuf);
			}

			if ( $<i>7 == D_ISOPEN )
			    tmpdb[ndb]->db_open = 1;
			else if ( $<i>7 == D_CLOSED )
			    tmpdb[ndb]->db_open = 0;
			else
			    yyerror("A drawbridge can only be open or closed!");
			ndb++;
			if (ndb >= MAX_OF_TYPE) {
				yyerror("Too many drawbridges in mazepart!");
				ndb--;
			}
		   }
		;

mazewalk_detail : MAZEWALK_ID ':' coordinate ',' DIRECTION
		  {
			tmpwalk[nwalk] = New(walk);
			tmpwalk[nwalk]->x = current_coord.x;
			tmpwalk[nwalk]->y = current_coord.y;
			tmpwalk[nwalk]->dir = $5;
			nwalk++;
			if (nwalk >= MAX_OF_TYPE) {
				yyerror("Too many mazewalks in mazepart!");
				nwalk--;
			}
		  }
		;

wallify_detail	: WALLIFY_ID
		  {
			wallify_map();
		  }
		;

ladder_detail	: LADDER_ID ':' coordinate ',' UP_OR_DOWN
		  {
			tmplad[nlad] = New(lad);
			tmplad[nlad]->x = current_coord.x;
			tmplad[nlad]->y = current_coord.y;
			tmplad[nlad]->up = $<i>5;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Ladder");
			nlad++;
			if (nlad >= MAX_OF_TYPE) {
				yyerror("Too many ladders in mazepart!");
				nlad--;
			}
		  }
		;

stair_detail	: STAIR_ID ':' coordinate ',' UP_OR_DOWN
		  {
			tmpstair[nstair] = New(stair);
			tmpstair[nstair]->x = current_coord.x;
			tmpstair[nstair]->y = current_coord.y;
			tmpstair[nstair]->up = $<i>5;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Stairway");
			nstair++;
			if (nstair >= MAX_OF_TYPE) {
				yyerror("Too many stairs in room or mazepart!");
				nstair--;
			}
		  }
		;

stair_region	: STAIR_ID ':' lev_region
		  {
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = $3;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  }
		 ',' lev_region ',' UP_OR_DOWN
		  {
			tmplreg[nlreg]->del_islev = $6;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
			if($8)
			    tmplreg[nlreg]->rtype = LR_UPSTAIR;
			else
			    tmplreg[nlreg]->rtype = LR_DOWNSTAIR;
			tmplreg[nlreg]->rname.str = 0;
			nlreg++;
			if (nlreg >= MAX_OF_TYPE) {
				yyerror("Too many levregions in mazepart!");
				nlreg--;
			}
		  }
		;

portal_region	: PORTAL_ID ':' lev_region
		  {
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = $3;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  }
		 ',' lev_region ',' string
		  {
			tmplreg[nlreg]->del_islev = $6;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
			tmplreg[nlreg]->rtype = LR_PORTAL;
			tmplreg[nlreg]->rname.str = $8;
			nlreg++;
			if (nlreg >= MAX_OF_TYPE) {
				yyerror("Too many levregions in mazepart!");
				nlreg--;
			}
		  }
		;

teleprt_region	: TELEPRT_ID ':' lev_region
		  {
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = $3;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  }
		 ',' lev_region
		  {
			tmplreg[nlreg]->del_islev = $6;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
		  }
		teleprt_detail
		  {
			switch($<i>8) {
			case -1: tmplreg[nlreg]->rtype = LR_TELE; break;
			case 0: tmplreg[nlreg]->rtype = LR_DOWNTELE; break;
			case 1: tmplreg[nlreg]->rtype = LR_UPTELE; break;
			}
			tmplreg[nlreg]->rname.str = 0;
			nlreg++;
			if (nlreg >= MAX_OF_TYPE) {
				yyerror("Too many levregions in mazepart!");
				nlreg--;
			}
		  }
		;

branch_region	: BRANCH_ID ':' lev_region
		  {
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = $3;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  }
		 ',' lev_region
		  {
			tmplreg[nlreg]->del_islev = $6;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
			tmplreg[nlreg]->rtype = LR_BRANCH;
			tmplreg[nlreg]->rname.str = 0;
			nlreg++;
			if (nlreg >= MAX_OF_TYPE) {
				yyerror("Too many levregions in mazepart!");
				nlreg--;
			}
		  }
		;

teleprt_detail	: /* empty */
		  {
			$<i>$ = -1;
		  }
		| ',' UP_OR_DOWN
		  {
			$<i>$ = $2;
		  }
		;

lev_region	: region
		  {
			$$ = 0;
		  }
		| LEV '(' INTEGER ',' INTEGER ',' INTEGER ',' INTEGER ')'
		  {
/* This series of if statements is a hack for MSC 5.1.  It seems that its
   tiny little brain cannot compile if these are all one big if statement. */
			if ($3 <= 0 || $3 >= COLNO)
				yyerror("Region out of level range!");
			else if ($5 < 0 || $5 >= ROWNO)
				yyerror("Region out of level range!");
			else if ($7 <= 0 || $7 >= COLNO)
				yyerror("Region out of level range!");
			else if ($9 < 0 || $9 >= ROWNO)
				yyerror("Region out of level range!");
			current_region.x1 = $3;
			current_region.y1 = $5;
			current_region.x2 = $7;
			current_region.y2 = $9;
			$$ = 1;
		  }
		;

fountain_detail : FOUNTAIN_ID ':' coordinate
		  {
			tmpfountain[nfountain] = New(fountain);
			tmpfountain[nfountain]->x = current_coord.x;
			tmpfountain[nfountain]->y = current_coord.y;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Fountain");
			nfountain++;
			if (nfountain >= MAX_OF_TYPE) {
			    yyerror("Too many fountains in room or mazepart!");
			    nfountain--;
			}
		  }
		;

sink_detail : SINK_ID ':' coordinate
		  {
			tmpsink[nsink] = New(sink);
			tmpsink[nsink]->x = current_coord.x;
			tmpsink[nsink]->y = current_coord.y;
			nsink++;
			if (nsink >= MAX_OF_TYPE) {
				yyerror("Too many sinks in room!");
				nsink--;
			}
		  }
		;

pool_detail : POOL_ID ':' coordinate
		  {
			tmppool[npool] = New(pool);
			tmppool[npool]->x = current_coord.x;
			tmppool[npool]->y = current_coord.y;
			npool++;
			if (npool >= MAX_OF_TYPE) {
				yyerror("Too many pools in room!");
				npool--;
			}
		  }
		;

diggable_detail : NON_DIGGABLE_ID ':' region
		  {
			tmpdig[ndig] = New(digpos);
			tmpdig[ndig]->x1 = current_region.x1;
			tmpdig[ndig]->y1 = current_region.y1;
			tmpdig[ndig]->x2 = current_region.x2;
			tmpdig[ndig]->y2 = current_region.y2;
			ndig++;
			if (ndig >= MAX_OF_TYPE) {
				yyerror("Too many diggables in mazepart!");
				ndig--;
			}
		  }
		;

passwall_detail : NON_PASSWALL_ID ':' region
		  {
			tmppass[npass] = New(digpos);
			tmppass[npass]->x1 = current_region.x1;
			tmppass[npass]->y1 = current_region.y1;
			tmppass[npass]->x2 = current_region.x2;
			tmppass[npass]->y2 = current_region.y2;
			npass++;
			if (npass >= 32) {
				yyerror("Too many passwalls in mazepart!");
				npass--;
			}
		  }
		;

region_detail	: REGION_ID ':' region ',' light_state ',' room_type prefilled
		  {
			tmpreg[nreg] = New(region);
			tmpreg[nreg]->x1 = current_region.x1;
			tmpreg[nreg]->y1 = current_region.y1;
			tmpreg[nreg]->x2 = current_region.x2;
			tmpreg[nreg]->y2 = current_region.y2;
			tmpreg[nreg]->rlit = $<i>5;
			tmpreg[nreg]->rtype = $<i>7;
			if($<i>8 & 1) tmpreg[nreg]->rtype += MAXRTYPE+1;
			tmpreg[nreg]->rirreg = (($<i>8 & 2) != 0);
			if(current_region.x1 > current_region.x2 ||
			   current_region.y1 > current_region.y2)
			   yyerror("Region start > end!");
			if(tmpreg[nreg]->rtype == VAULT &&
			   (tmpreg[nreg]->rirreg ||
			    (tmpreg[nreg]->x2 - tmpreg[nreg]->x1 != 1) ||
			    (tmpreg[nreg]->y2 - tmpreg[nreg]->y1 != 1)))
				yyerror("Vaults must be exactly 2x2!");
			if(want_warnings && !tmpreg[nreg]->rirreg &&
			   current_region.x1 > 0 && current_region.y1 > 0 &&
			   current_region.x2 < (int)max_x_map &&
			   current_region.y2 < (int)max_y_map) {
			    /* check for walls in the room */
			    char ebuf[60];
			    register int x, y, nrock = 0;

			    for(y=current_region.y1; y<=current_region.y2; y++)
				for(x=current_region.x1;
				    x<=current_region.x2; x++)
				    if(IS_ROCK(tmpmap[y][x]) ||
				       IS_DOOR(tmpmap[y][x])) nrock++;
			    if(nrock) {
				Sprintf(ebuf,
					"Rock in room (%02d,%02d,%02d,%02d)?!",
					current_region.x1, current_region.y1,
					current_region.x2, current_region.y2);
				yywarning(ebuf);
			    }
			    if (
		!IS_ROCK(tmpmap[current_region.y1-1][current_region.x1-1]) ||
		!IS_ROCK(tmpmap[current_region.y2+1][current_region.x1-1]) ||
		!IS_ROCK(tmpmap[current_region.y1-1][current_region.x2+1]) ||
		!IS_ROCK(tmpmap[current_region.y2+1][current_region.x2+1])) {
				Sprintf(ebuf,
				"NonRock edge in room (%02d,%02d,%02d,%02d)?!",
					current_region.x1, current_region.y1,
					current_region.x2, current_region.y2);
				yywarning(ebuf);
			    }
			} else if(tmpreg[nreg]->rirreg &&
		!IS_ROOM(tmpmap[current_region.y1][current_region.x1])) {
			    char ebuf[60];
			    Sprintf(ebuf,
				    "Rock in irregular room (%02d,%02d)?!",
				    current_region.x1, current_region.y1);
			    yyerror(ebuf);
			}
			nreg++;
			if (nreg >= MAX_OF_TYPE) {
				yyerror("Too many regions in mazepart!");
				nreg--;
			}
		  }
		;

altar_detail	: ALTAR_ID ':' coordinate ',' alignment ',' altar_type
		  {
			tmpaltar[naltar] = New(altar);
			tmpaltar[naltar]->x = current_coord.x;
			tmpaltar[naltar]->y = current_coord.y;
			tmpaltar[naltar]->align = $<i>5;
			tmpaltar[naltar]->shrine = $<i>7;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Altar");
			naltar++;
			if (naltar >= MAX_OF_TYPE) {
				yyerror("Too many altars in room or mazepart!");
				naltar--;
			}
		  }
		;

gold_detail	: GOLD_ID ':' amount ',' coordinate
		  {
			tmpgold[ngold] = New(gold);
			tmpgold[ngold]->x = current_coord.x;
			tmpgold[ngold]->y = current_coord.y;
			tmpgold[ngold]->amount = $<i>3;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Gold");
			ngold++;
			if (ngold >= MAX_OF_TYPE) {
				yyerror("Too many golds in room or mazepart!");
				ngold--;
			}
		  }
		;

engraving_detail: ENGRAVING_ID ':' coordinate ',' engraving_type ',' string
		  {
			tmpengraving[nengraving] = New(engraving);
			tmpengraving[nengraving]->x = current_coord.x;
			tmpengraving[nengraving]->y = current_coord.y;
			tmpengraving[nengraving]->engr.str = $7;
			tmpengraving[nengraving]->etype = $<i>5;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Engraving");
			nengraving++;
			if (nengraving >= MAX_OF_TYPE) {
			    yyerror("Too many engravings in room or mazepart!");
			    nengraving--;
			}
		  }
		;

monster_c	: monster
		| RANDOM_TYPE
		  {
			$<i>$ = - MAX_REGISTERS - 1;
		  }
		| m_register
		;

object_c	: object
		| RANDOM_TYPE
		  {
			$<i>$ = - MAX_REGISTERS - 1;
		  }
		| o_register
		;

m_name		: string
		| RANDOM_TYPE
		  {
			$$ = (char *) 0;
		  }
		;

o_name		: string
		| RANDOM_TYPE
		  {
			$$ = (char *) 0;
		  }
		;

trap_name	: string
		  {
			int token = get_trap_type($1);
			if (token == ERR)
				yyerror("Unknown trap type!");
			$<i>$ = token;
			Free($1);
		  }
		| RANDOM_TYPE
		;

room_type	: string
		  {
			int token = get_room_type($1);
			if (token == ERR) {
				yywarning("Unknown room type!  Making ordinary room...");
				$<i>$ = OROOM;
			} else
				$<i>$ = token;
			Free($1);
		  }
		| RANDOM_TYPE
		;

prefilled	: /* empty */
		  {
			$<i>$ = 0;
		  }
		| ',' FILLING
		  {
			$<i>$ = $2;
		  }
		| ',' FILLING ',' BOOLEAN
		  {
			$<i>$ = $2 + ($4 << 1);
		  }
		;

coordinate	: coord
		| p_register
		| RANDOM_TYPE
		  {
			current_coord.x = current_coord.y = -MAX_REGISTERS-1;
		  }
		;

door_state	: DOOR_STATE
		| RANDOM_TYPE
		;

light_state	: LIGHT_STATE
		| RANDOM_TYPE
		;

alignment	: ALIGNMENT
		| a_register
		| RANDOM_TYPE
		  {
			$<i>$ = - MAX_REGISTERS - 1;
		  }
		;

altar_type	: ALTAR_TYPE
		| RANDOM_TYPE
		;

p_register	: P_REGISTER '[' INTEGER ']'
		  {
			if ( $3 >= MAX_REGISTERS )
				yyerror("Register Index overflow!");
			else
				current_coord.x = current_coord.y = - $3 - 1;
		  }
		;

o_register	: O_REGISTER '[' INTEGER ']'
		  {
			if ( $3 >= MAX_REGISTERS )
				yyerror("Register Index overflow!");
			else
				$<i>$ = - $3 - 1;
		  }
		;

m_register	: M_REGISTER '[' INTEGER ']'
		  {
			if ( $3 >= MAX_REGISTERS )
				yyerror("Register Index overflow!");
			else
				$<i>$ = - $3 - 1;
		  }
		;

a_register	: A_REGISTER '[' INTEGER ']'
		  {
			if ( $3 >= 3 )
				yyerror("Register Index overflow!");
			else
				$<i>$ = - $3 - 1;
		  }
		;

place		: coord
		;

monster		: CHAR
		  {
			if (check_monster_char((char) $1))
				$<i>$ = $1 ;
			else {
				yyerror("Unknown monster class!");
				$<i>$ = ERR;
			}
		  }
		;

object		: CHAR
		  {
			char c = $1;
			if (check_object_char(c))
				$<i>$ = c;
			else {
				yyerror("Unknown char class!");
				$<i>$ = ERR;
			}
		  }
		;

string		: STRING
		;

amount		: INTEGER
		| RANDOM_TYPE
		;

chance		: /* empty */
		  {
			$$ = 100;	/* default is 100% */
		  }
		| PERCENT
		  {
			if ($1 <= 0 || $1 > 100)
			    yyerror("Expected percentile chance.");
			$$ = $1;
		  }
		;

engraving_type	: ENGRAVING_TYPE
		| RANDOM_TYPE
		;

coord		: '(' INTEGER ',' INTEGER ')'
		  {
			if (!in_room && !init_lev.init_present &&
			    ($2 < 0 || $2 > (int)max_x_map ||
			     $4 < 0 || $4 > (int)max_y_map))
			    yyerror("Coordinates out of map range!");
			current_coord.x = $2;
			current_coord.y = $4;
		  }
		;

region		: '(' INTEGER ',' INTEGER ',' INTEGER ',' INTEGER ')'
		  {
/* This series of if statements is a hack for MSC 5.1.  It seems that its
   tiny little brain cannot compile if these are all one big if statement. */
			if ($2 < 0 || $2 > (int)max_x_map)
				yyerror("Region out of map range!");
			else if ($4 < 0 || $4 > (int)max_y_map)
				yyerror("Region out of map range!");
			else if ($6 < 0 || $6 > (int)max_x_map)
				yyerror("Region out of map range!");
			else if ($8 < 0 || $8 > (int)max_y_map)
				yyerror("Region out of map range!");
			current_region.x1 = $2;
			current_region.y1 = $4;
			current_region.x2 = $6;
			current_region.y2 = $8;
		  }
		;

%%

/*lev_comp.y*/
