/*	SCCS Id: @(#)quest.h	3.4	1992/11/15	*/
/* Copyright (c) Mike Stephenson 1991.				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef QUEST_H
#define QUEST_H

struct q_score {			/* Quest "scorecard" */
	Bitfield(first_start,1);	/* only set the first time */
	Bitfield(met_leader,1);		/* has met the leader */
	Bitfield(not_ready,3);		/* rejected due to alignment, etc. */
	Bitfield(pissed_off,1);		/* got the leader angry */
	Bitfield(got_quest,1);		/* got the quest assignment */

	Bitfield(first_locate,1);	/* only set the first time */
	Bitfield(met_intermed,1);	/* used if the locate is a person. */
	Bitfield(got_final,1);		/* got the final quest assignment */

	Bitfield(made_goal,3);		/* # of times on goal level */
	Bitfield(met_nemesis,1);	/* has met the nemesis before */
	Bitfield(killed_nemesis,1);	/* set when the nemesis is killed */
	Bitfield(in_battle,1);		/* set when nemesis fighting you */

	Bitfield(cheater,1);		/* set if cheating detected */
	Bitfield(touched_artifact,1);	/* for a special message */
	Bitfield(offered_artifact,1);	/* offered to leader */
	Bitfield(got_thanks,1);		/* final message from leader */

	/* keep track of leader presence/absence even if leader is
	   polymorphed, raised from dead, etc */
	Bitfield(leader_is_dead,1);
	unsigned leader_m_id;
};

#define MAX_QUEST_TRIES  7	/* exceed this and you "fail" */
#define MIN_QUEST_ALIGN 20	/* at least this align.record to start */
  /* note: align 20 matches "pious" as reported by enlightenment (cmd.c) */
#define MIN_QUEST_LEVEL 14	/* at least this u.ulevel to start */
  /* note: exp.lev. 14 is threshold level for 5th rank (class title, role.c) */

#endif /* QUEST_H */
