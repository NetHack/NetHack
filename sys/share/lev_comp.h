#define CHAR 257
#define INTEGER 258
#define BOOLEAN 259
#define PERCENT 260
#define MESSAGE_ID 261
#define MAZE_ID 262
#define LEVEL_ID 263
#define LEV_INIT_ID 264
#define GEOMETRY_ID 265
#define NOMAP_ID 266
#define OBJECT_ID 267
#define COBJECT_ID 268
#define MONSTER_ID 269
#define TRAP_ID 270
#define DOOR_ID 271
#define DRAWBRIDGE_ID 272
#define MAZEWALK_ID 273
#define WALLIFY_ID 274
#define REGION_ID 275
#define FILLING 276
#define RANDOM_OBJECTS_ID 277
#define RANDOM_MONSTERS_ID 278
#define RANDOM_PLACES_ID 279
#define ALTAR_ID 280
#define LADDER_ID 281
#define STAIR_ID 282
#define NON_DIGGABLE_ID 283
#define NON_PASSWALL_ID 284
#define ROOM_ID 285
#define PORTAL_ID 286
#define TELEPRT_ID 287
#define BRANCH_ID 288
#define LEV 289
#define CHANCE_ID 290
#define CORRIDOR_ID 291
#define GOLD_ID 292
#define ENGRAVING_ID 293
#define FOUNTAIN_ID 294
#define POOL_ID 295
#define SINK_ID 296
#define NONE 297
#define RAND_CORRIDOR_ID 298
#define DOOR_STATE 299
#define LIGHT_STATE 300
#define CURSE_TYPE 301
#define ENGRAVING_TYPE 302
#define DIRECTION 303
#define RANDOM_TYPE 304
#define O_REGISTER 305
#define M_REGISTER 306
#define P_REGISTER 307
#define A_REGISTER 308
#define ALIGNMENT 309
#define LEFT_OR_RIGHT 310
#define CENTER 311
#define TOP_OR_BOT 312
#define ALTAR_TYPE 313
#define UP_OR_DOWN 314
#define SUBROOM_ID 315
#define NAME_ID 316
#define FLAGS_ID 317
#define FLAG_TYPE 318
#define MON_ATTITUDE 319
#define MON_ALERTNESS 320
#define MON_APPEARANCE 321
#define CONTAINED 322
#define STRING 323
#define MAP_ID 324
typedef union
{
	int	i;
	char*	map;
	struct {
		xchar room;
		xchar wall;
		xchar door;
	} corpos;
} YYSTYPE;
extern YYSTYPE yylval;
