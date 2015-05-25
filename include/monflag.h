/* NetHack 3.6	monflag.h	$NHDT-Date: 1432512778 2015/05/25 00:12:58 $  $NHDT-Branch: master $:$NHDT-Revision: 1.14 $ */
/* Copyright (c) 1989 Mike Threepoint				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MONFLAG_H
#define MONFLAG_H

#define MS_SILENT 0     /* makes no sound */
#define MS_BARK 1       /* if full moon, may howl */
#define MS_MEW 2        /* mews or hisses */
#define MS_ROAR 3       /* roars */
#define MS_GROWL 4      /* growls */
#define MS_SQEEK 5      /* squeaks, as a rodent */
#define MS_SQAWK 6      /* squawks, as a bird */
#define MS_HISS 7       /* hisses */
#define MS_BUZZ 8       /* buzzes (killer bee) */
#define MS_GRUNT 9      /* grunts (or speaks own language) */
#define MS_NEIGH 10     /* neighs, as an equine */
#define MS_WAIL 11      /* wails, as a tortured soul */
#define MS_GURGLE 12    /* gurgles, as liquid or through saliva */
#define MS_BURBLE 13    /* burbles (jabberwock) */
#define MS_ANIMAL 13    /* up to here are animal noises */
#define MS_SHRIEK 15    /* wakes up others */
#define MS_BONES 16     /* rattles bones (skeleton) */
#define MS_LAUGH 17     /* grins, smiles, giggles, and laughs */
#define MS_MUMBLE 18    /* says something or other */
#define MS_IMITATE 19   /* imitates others (leocrotta) */
#define MS_ORC MS_GRUNT /* intelligent brutes */
#define MS_HUMANOID 20  /* generic traveling companion */
#define MS_ARREST 21    /* "Stop in the name of the law!" (Kops) */
#define MS_SOLDIER 22   /* army and watchmen expressions */
#define MS_GUARD 23     /* "Please drop that gold and follow me." */
#define MS_DJINNI 24    /* "Thank you for freeing me!" */
#define MS_NURSE 25     /* "Take off your shirt, please." */
#define MS_SEDUCE 26    /* "Hello, sailor." (Nymphs) */
#define MS_VAMPIRE 27   /* vampiric seduction, Vlad's exclamations */
#define MS_BRIBE 28     /* asks for money, or berates you */
#define MS_CUSS 29      /* berates (demons) or intimidates (Wiz) */
#define MS_RIDER 30     /* astral level special monsters */
#define MS_LEADER 31    /* your class leader */
#define MS_NEMESIS 32   /* your nemesis */
#define MS_GUARDIAN 33  /* your leader's guards */
#define MS_SELL 34      /* demand payment, complain about shoplifters */
#define MS_ORACLE 35    /* do a consultation */
#define MS_PRIEST 36    /* ask for contribution; do cleansing */
#define MS_SPELL 37     /* spellcaster not matching any of the above */
#define MS_WERE 38      /* lycanthrope in human form */
#define MS_BOAST 39     /* giants */

#define MR_FIRE 0x01   /* resists fire */
#define MR_COLD 0x02   /* resists cold */
#define MR_SLEEP 0x04  /* resists sleep */
#define MR_DISINT 0x08 /* resists disintegration */
#define MR_ELEC 0x10   /* resists electricity */
#define MR_POISON 0x20 /* resists poison */
#define MR_ACID 0x40   /* resists acid */
#define MR_STONE 0x80  /* resists petrification */
/* other resistances: magic, sickness */
/* other conveyances: teleport, teleport control, telepathy */

/* individual resistances */
#define MR2_SEE_INVIS 0x0100 /* see invisible */
#define MR2_LEVITATE 0x0200  /* levitation */
#define MR2_WATERWALK 0x0400 /* water walking */
#define MR2_MAGBREATH 0x0800 /* magical breathing */
#define MR2_DISPLACED 0x1000 /* displaced */
#define MR2_STRENGTH 0x2000  /* gauntlets of power */
#define MR2_FUMBLING 0x4000  /* clumsy */

#define M1_FLY 0x00000001L         /* can fly or float */
#define M1_SWIM 0x00000002L        /* can traverse water */
#define M1_AMORPHOUS 0x00000004L   /* can flow under doors */
#define M1_WALLWALK 0x00000008L    /* can phase thru rock */
#define M1_CLING 0x00000010L       /* can cling to ceiling */
#define M1_TUNNEL 0x00000020L      /* can tunnel thru rock */
#define M1_NEEDPICK 0x00000040L    /* needs pick to tunnel */
#define M1_CONCEAL 0x00000080L     /* hides under objects */
#define M1_HIDE 0x00000100L        /* mimics, blends in with ceiling */
#define M1_AMPHIBIOUS 0x00000200L  /* can survive underwater */
#define M1_BREATHLESS 0x00000400L  /* doesn't need to breathe */
#define M1_NOTAKE 0x00000800L      /* cannot pick up objects */
#define M1_NOEYES 0x00001000L      /* no eyes to gaze into or blind */
#define M1_NOHANDS 0x00002000L     /* no hands to handle things */
#define M1_NOLIMBS 0x00006000L     /* no arms/legs to kick/wear on */
#define M1_NOHEAD 0x00008000L      /* no head to behead */
#define M1_MINDLESS 0x00010000L    /* has no mind--golem, zombie, mold */
#define M1_HUMANOID 0x00020000L    /* has humanoid head/arms/torso */
#define M1_ANIMAL 0x00040000L      /* has animal body */
#define M1_SLITHY 0x00080000L      /* has serpent body */
#define M1_UNSOLID 0x00100000L     /* has no solid or liquid body */
#define M1_THICK_HIDE 0x00200000L  /* has thick hide or scales */
#define M1_OVIPAROUS 0x00400000L   /* can lay eggs */
#define M1_REGEN 0x00800000L       /* regenerates hit points */
#define M1_SEE_INVIS 0x01000000L   /* can see invisible creatures */
#define M1_TPORT 0x02000000L       /* can teleport */
#define M1_TPORT_CNTRL 0x04000000L /* controls where it teleports to */
#define M1_ACID 0x08000000L        /* acidic to eat */
#define M1_POIS 0x10000000L        /* poisonous to eat */
#define M1_CARNIVORE 0x20000000L   /* eats corpses */
#define M1_HERBIVORE 0x40000000L   /* eats fruits */
#define M1_OMNIVORE 0x60000000L    /* eats both */
#ifdef NHSTDC
#define M1_METALLIVORE 0x80000000UL /* eats metal */
#else
#define M1_METALLIVORE 0x80000000L /* eats metal */
#endif

#define M2_NOPOLY 0x00000001L       /* players mayn't poly into one */
#define M2_UNDEAD 0x00000002L       /* is walking dead */
#define M2_WERE 0x00000004L         /* is a lycanthrope */
#define M2_HUMAN 0x00000008L        /* is a human */
#define M2_ELF 0x00000010L          /* is an elf */
#define M2_DWARF 0x00000020L        /* is a dwarf */
#define M2_GNOME 0x00000040L        /* is a gnome */
#define M2_ORC 0x00000080L          /* is an orc */
#define M2_DEMON 0x00000100L        /* is a demon */
#define M2_MERC 0x00000200L         /* is a guard or soldier */
#define M2_LORD 0x00000400L         /* is a lord to its kind */
#define M2_PRINCE 0x00000800L       /* is an overlord to its kind */
#define M2_MINION 0x00001000L       /* is a minion of a deity */
#define M2_GIANT 0x00002000L        /* is a giant */
#define M2_SHAPESHIFTER 0x00004000L /* is a shapeshifting species */
#define M2_MALE 0x00010000L         /* always male */
#define M2_FEMALE 0x00020000L       /* always female */
#define M2_NEUTER 0x00040000L       /* neither male nor female */
#define M2_PNAME 0x00080000L        /* monster name is a proper name */
#define M2_HOSTILE 0x00100000L      /* always starts hostile */
#define M2_PEACEFUL 0x00200000L     /* always starts peaceful */
#define M2_DOMESTIC 0x00400000L     /* can be tamed by feeding */
#define M2_WANDER 0x00800000L       /* wanders randomly */
#define M2_STALK 0x01000000L        /* follows you to other levels */
#define M2_NASTY 0x02000000L        /* extra-nasty monster (more xp) */
#define M2_STRONG 0x04000000L       /* strong (or big) monster */
#define M2_ROCKTHROW 0x08000000L    /* throws boulders */
#define M2_GREEDY 0x10000000L       /* likes gold */
#define M2_JEWELS 0x20000000L       /* likes gems */
#define M2_COLLECT 0x40000000L      /* picks up weapons and food */
#ifdef NHSTDC
#define M2_MAGIC 0x80000000UL /* picks up magic items */
#else
#define M2_MAGIC 0x80000000L /* picks up magic items */
#endif

#define M3_WANTSAMUL 0x0001 /* would like to steal the amulet */
#define M3_WANTSBELL 0x0002 /* wants the bell */
#define M3_WANTSBOOK 0x0004 /* wants the book */
#define M3_WANTSCAND 0x0008 /* wants the candelabrum */
#define M3_WANTSARTI 0x0010 /* wants the quest artifact */
#define M3_WANTSALL 0x001f  /* wants any major artifact */
#define M3_WAITFORU 0x0040  /* waits to see you or get attacked */
#define M3_CLOSE 0x0080     /* lets you close unless attacked */

#define M3_COVETOUS 0x001f /* wants something */
#define M3_WAITMASK 0x00c0 /* waiting... */

/* Infravision is currently implemented for players only */
#define M3_INFRAVISION 0x0100  /* has infravision */
#define M3_INFRAVISIBLE 0x0200 /* visible by infravision */

#define M3_DISPLACES 0x0400 /* moves monsters out of its way */

#define MZ_TINY 0          /* < 2' */
#define MZ_SMALL 1         /* 2-4' */
#define MZ_MEDIUM 2        /* 4-7' */
#define MZ_HUMAN MZ_MEDIUM /* human-sized */
#define MZ_LARGE 3         /* 7-12' */
#define MZ_HUGE 4          /* 12-25' */
#define MZ_GIGANTIC 7      /* off the scale */

/* Monster races -- must stay within ROLE_RACEMASK */
/* Eventually this may become its own field */
#define MH_HUMAN M2_HUMAN
#define MH_ELF M2_ELF
#define MH_DWARF M2_DWARF
#define MH_GNOME M2_GNOME
#define MH_ORC M2_ORC

/* for mons[].geno (constant during game) */
#define G_UNIQ 0x1000     /* generated only once */
#define G_NOHELL 0x0800   /* not generated in "hell" */
#define G_HELL 0x0400     /* generated only in "hell" */
#define G_NOGEN 0x0200    /* generated only specially */
#define G_SGROUP 0x0080   /* appear in small groups normally */
#define G_LGROUP 0x0040   /* appear in large groups normally */
#define G_GENO 0x0020     /* can be genocided */
#define G_NOCORPSE 0x0010 /* no corpse left ever */
#define G_FREQ 0x0007     /* creation frequency mask */

/* for mvitals[].mvflags (variant during game), along with G_NOCORPSE */
#define G_KNOWN 0x0004 /* have been encountered */
#define G_GONE (G_GENOD | G_EXTINCT)
#define G_GENOD 0x0002 /* have been genocided */
#define G_EXTINCT                       \
    0x0001 /* have been extinguished as \
              population control */
#define MV_KNOWS_EGG                        \
    0x0008 /* player recognizes egg of this \
              monster type */

#endif /* MONFLAG_H */
