-- NetHack quest.lua	$NHDT-Date: 1652196288 2022/05/10 15:24:48 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.7 $
-- Copyright (c) 2021 by Pasi Kallinen
-- NetHack may be freely redistributed.  See license for details.
-- TODO:
--  - output = "verbalize"
--  - export the quest string replacements to lua, instead of %H etc
--  - allow checking if hero is carrying item (see comments for %Cp Arc 00042)
--  - fold quest_portal, quest_portal_again, quest_portal_demand into one
--  - write tests to check questtext validity?
--  - qt_pager hack(?): if (qt_msg->delivery == 'p' && strcmp(windowprocs.name, "X11"))




-- text = "something"
-- Text is shown to the user.

-- synopsis = "something"
-- Synopsis is inserted into the message history.
--
-- output = "pline" | "menu" | "text"
-- The output can be manually set by using output = "menu"
-- Valid values for output are "pline", "text", and "menu, defaulting to
-- pline, unless the text contains newlines, or is too long to fit a message buffer,
-- then will be shown as a text window instead.



questtext = {
   -- If a role doesn't have a specific message, try a fallback
   msg_fallbacks = {
      goal_alt = "goal_next"
   },
   common = {
      TEST_PATTERN = {
         output = "text",
         text = [[%p:	return(plname);
 %c:	return(pl_character);
 %r:	return((char *)rank_of(u.ulevel));
 %R:	return((char *)rank_of(MIN_QUEST_LEVEL));
 %s:	return((flags.female) ? "sister" : "brother" );
 %S:	return((flags.female) ? "daughter" : "son" );
 %l:	return((char *)ldrname());
 %i:	return(intermed());
 %o:	return(artiname());
 %O:	return(shortened(artiname()));
 %n:	return((char *)neminame());
 %g:	return((char *)guardname());
 %G:	return((char *)align_gtitle(u.ualignbase[1]));
 %H:	return((char *)homebase());
 %a:	return(Alignnam(u.ualignbase[1]));
 %A:	return(Alignnam(u.ualign.type));
 %d:	return((char *)align_gname(u.ualignbase[1]));
 %D:	return((char *)align_gname(A_LAWFUL));
 %C:	return("chaotic");
 %N:	return("neutral");
 %L:	return("lawful");
 %x:	return((Blind) ? "sense" : "see");
 %Z:	return("The Dungeons of Doom");
 %%:	return(percent_sign);
 a suffix:	return an(root);
 A suffix:	return An(root);
 C suffix:	return capitalized(root);
 h suffix:	return pronoun(he_or_she, mon_of(root)); /* for %l,%n,%d,%o */
 H suffix:	return capitalized(pronoun(he_or_she, mon_of(root)));
 i suffix:	return pronoun(him_or_her, mon_of(root));
 I suffix:	return capitalized(pronoun(him_or_her, mon_of(root)));
 j suffix:	return pronoun(his_or_her, mon_of(root));
 J suffix:	return capitalized(pronoun(his_or_her, mon_of(root)));
 p suffix:	return makeplural(root);
 P suffix:	return makeplural(capitalized(root));
 s suffix:	return s_suffix(root);
 S suffix:	return s_suffix(capitalized(root));
 t suffix:	return strip_the_prefix(root);]],
      },
      angel_cuss = {
         "\"Repent, and thou shalt be saved!\"",
         "\"Thou shalt pay for thine insolence!\"",
         "\"Very soon, my child, thou shalt meet thy maker.\"",
         "\"The great %D has sent me to make you pay for your sins!\"",
         "\"The wrath of %D is now upon you!\"",
         "\"Thy life belongs to %D now!\"",
         "\"Dost thou wish to receive thy final blessing?\"",
         "\"Thou art but a godless void.\"",
         "\"Thou art not worthy to seek the Amulet.\"",
         "\"No one expects the Spanish Inquisition!\"",
         "\"Judgment hath been passed upon thee, %p.\"",
         "\"Thy reckoning is at hand, %p.\"",
         "\"Thou shalt be brought before %D for thy crimes!\"",
         "\"With %D as my witness, I shall strike thee down.\"",
      },
      banished = {
         synopsis = "[You are banished from %H for betraying your allegiance to %d.]",
         output = "text",
         text = [["You have betrayed all those who hold allegiance to %d, as you once did.
My allegiance to %d holds fast and I cannot condone or accept what you
have done.

Leave this place.  You shall never set foot at %H again.
That which you seek is now lost forever, for without the Bell of Opening,
you will never be able to enter the place where he who has the Amulet
resides.

Go now!  You are banished from this place.]],
      },
      demon_cuss = {
         "\"I first mistook thee for a statue, when I regarded thy head of stone.\"",
         "\"Come here often?\"",
         "\"Doth pain excite thee?  Wouldst thou prefer the whip?\"",
         "\"Thinkest thou it shall tickle as I rip out thy lungs?\"",
         "\"Eat slime and die!\"",
         "\"Go ahead, fetch thy mama!  I shall wait.\"",
         "\"Go play leapfrog with a herd of unicorns!\"",
         "\"Hast thou been drinking, or art thou always so clumsy?\"",
         "\"This time I shall let thee off with a spanking, but let it not happen again.\"",
         "\"I've met smarter (and prettier) acid blobs.\"",
         "\"Look!  Thy bootlace is undone!\"",
         "\"Mercy!  Dost thou wish me to die of laughter?\"",
         "\"Run away!  Live to flee another day!\"",
         "\"Thou hadst best fight better than thou canst dress!\"",
         "\"Twixt thy cousin and thee, Medusa is the prettier.\"",
         "\"Methinks thou wert unnaturally stirred by yon corpse back there, eh, varlet?\"",
         "\"Up thy nose with a rubber hose!\"",
         "\"Verily, thy corpse could not smell worse!\"",
         "\"Wait!  I shall polymorph into a grid bug to give thee a fighting chance!\"",
         "\"Why search for the Amulet?  Thou wouldst but lose it, cretin.\"",
         "\"Thou ought to be a comedian, thy skills are so laughable!\"",
         "\"Thy gaze is so vacant, I thought thee a floating eye!\"",
         "\"Thy head is unfit for a mind flayer to munch upon!\"",
         "\"Only thy reflection could love thee!\"",
         "\"Hast thou considered masking thine odour?\"",
         "\"Hold! Thy face is a most exquisite torture!\"",
         "\"I should fart in thy direction, but it might improve thy smell!\"",
      },
      legacy = {
         synopsis = "[%dC has chosen you to recover the Amulet of Yendor for %dI.]",
         output = "menu",
         text = [[It is written in the Book of %d:

    After the Creation, the cruel god Moloch rebelled
    against the authority of Marduk the Creator.
    Moloch stole from Marduk the most powerful of all
    the artifacts of the gods, the Amulet of Yendor,
    and he hid it in the dark cavities of Gehennom, the
    Under World, where he now lurks, and bides his time.

Your %G %d seeks to possess the Amulet, and with it
to gain deserved ascendance over the other gods.

You, a newly trained %r, have been heralded
from birth as the instrument of %d.  You are destined
to recover the Amulet for your deity, or die in the
attempt.  Your hour of destiny has come.  For the sake
of us all:  Go bravely with %d!]],
      },
      quest_complete_no_bell = {
         text = [["The silver bell which was hoarded by %n will be
essential in locating the Amulet of Yendor."]],
      },
      quest_portal = {
         output = "pline",
         text = [[You receive a faint telepathic message from %l:
Your help is urgently needed at %H!
Look for a ...ic transporter.
You couldn't quite make out that last message.]],
      },
      quest_portal_again = {
         text = "You again sense %l pleading for help.",
      },
      quest_portal_demand = {
         text = "You again sense %l demanding your attendance.",
      },
   },
   Arc = {
      assignquest = {
         synopsis = "[%nC has stolen %o.  Locate %i, defeat %ni, and return %O.]",
         output = "text",
         text = [["Grave times have befallen the college, for %na has
stolen %o.  Without it, the board of directors of
the university will soon have no choice but to revoke our research grants.

"You must locate the entrance to %i.  Within it,
you will find %n.

"You must then defeat %n and return %o
to me.

"Only in this way will we be able to prevent the budget cuts that could
close this college.

"May the wisdom of %d be your guide."]],
      },
      badalign = {
         synopsis = "[\"%pC, you have strayed from the %a path.  Purify yourself!\"]",
         output = "text",
         text = [["%pC!  I've heard that you've been using sloppy techniques.  Your
results lately can hardly be called suitable for %ra!

"How could you have strayed from the %a path?  Go from here, and come
back only when you have purified yourself."]],
      },
      badlevel = {
         synopsis = "[%pC, a mere %r is too inexperienced.]",
         output = "text",
         text = [["%p, you are yet too inexperienced to undertake such a demanding
quest.  A mere %r could not possibly face the rigors demanded and
survive.  Go forth, and come here again when your adventures have further
taught you."]],
      },
      discourage = {
         "\"Try your best, %p.  You cannot defeat me.\"",
         "\"I shall rend the flesh from your body whilst you still breathe!\"",
         "\"First you, %p, then I shall destroy your mentor, %l.\"",
         "\"Tiring yet, %p?  I draw my power from my master and cannot falter!\"",
         "\"I shall rend thy soul from thy body and consume it!\"",
         "\"You are far too %a -- it weakens you.  You shall die in this place.\"",
         "\"%d has forsaken you!  You are lost now!\"",
         "\"A mere %r cannot hope to defeat me!\"",
         "\"If you are the best %l can send, I have nothing to fear.\"",
         "\"Die %c!  I shall exhibit your carcass as a trophy.\"",
      },
      encourage = {
         "\"Beware, for %n is powerful and cunning.\"",
         "\"To locate the entrance to %i, you must pass many traps.\"",
         "\"A %nt may be vulnerable to attacks by magical cold.\"",
         "\"Call upon %d when you encounter %n.\"",
         "\"You must destroy %n.  It will pursue you otherwise.\"",
         "\"%oC is a mighty talisman.  With it you can destroy %n.\"",
         "\"Go forth with the blessings of %d.\"",
         "\"I will have my %gP watch for your return.\"",
         "\"Remember not to stray from the true %a path.\"",
         "\"You may be able to sense %o when you are near.\"",
      },
      firsttime = {
         synopsis = "[You arrive at %H, but all is not well.]",
         output = "text",
         text = [[You are suddenly in familiar surroundings.  The buildings in the distance
seem to be those of your old alma mater, but something is wrong.  It feels
as if there has been a riot recently, or %H has
been under siege.

All of the windows are boarded up, and there are objects scattered around
the entrance.

Strange forbidding shapes seem to be moving in the distance.]],
      },
      goal_alt = {
         text = "You have returned to %ns lair.",
      },
      goal_first = {
         synopsis = "[This strange feeling must be the presence of %o.]",
         output = "text",
         text = [[A strange feeling washes over you, and you think back to things you
learned during the many lectures of %l.

You realize the feeling must be the presence of %o.]],
      },
      goal_next = {
         text = "The familiar presence of %o is in the ether.",
      },
      gotit = {
         synopsis = "[The power of %o flows through your body!  You must return it to %l.]",
         output = "text",
         text = [[The power of %o flows through your body!  You feel
as if you could now take on the Wizard of Yendor himself and win, but
you know you must return %o to %l.]],
      },
      guardtalk_after = {
         "\"Did you see Lash LaRue in 'Song of Old Wyoming' the other night?\"",
         "\"Hey man, got any potions of hallucination for sale?\"",
         "\"I guess you are guaranteed to make full professor now.\"",
         "\"So, what was worse, %n or your entrance exams?\"",
         "\"%oC is impressive, but nothing like the bones I dug up!\"",
      },
      guardtalk_before = {
         "\"Did you see Lash LaRue in 'Song of Old Wyoming' the other night?\"",
         "\"Hey man, got any potions of hallucination for sale?\"",
         "\"Did you see the artifact %l brought back from the last dig?\"",
         "\"So what species do *you* think we evolved from?\"",
         "\"So you're %ls prize pupil!  I don't know what he sees in you.\"",
      },
      hasamulet = {
         synopsis = "[Take the Amulet to the Astral Plane and sacrifice it at the altar of %d.]",
         output = "text",
         text = [["Congratulations, %p.  I wondered if anyone could prevail against
the Wizard and the minions of Moloch.  Now, you must embark on one
final adventure.

"Take the Amulet, and find your way onto the Astral Plane.
There you must find the altar of %d and sacrifice the
Amulet on that altar to fulfill your destiny.

"Remember, your path now should always be upwards."]],
      },
      killed_nemesis = {
         text = "The body of %n dissipates in a cloud of noxious fumes.",
      },
      leader_first = {
         synopsis = "[\"You have returned, %p, to a difficult task.\"]",
         output = "text",
         text = [["Finally you have returned, %p.  You were always
my most promising student.  Allow me to see if you are ready for the
most difficult task of your career."]],
      },
      leader_last = {
         synopsis = "[\"%pC, you have failed us.  Begone!\"]",
         output = "text",
         text = [["%p, you have failed us.  All of my careful training has been in
vain.  Begone!  Your tenure at this college has been revoked!

"You are a disgrace to the profession!"]],
      },
      leader_next = {
         text = [["Again, %p, you stand before me.
Let me see if you have gained experience in the interim."]],
      },
      leader_other = {
         text = [["Once more, %p, you have returned from the field.
Are you finally ready for the task that must be accomplished?"]],
      },
      locate_first = {
         synopsis = "[This foreboding edifice must hide the entrance to %i.]",
         output = "text",
         text = [[A plain opens before you.  Beyond the plain lies a foreboding edifice.

You have the feeling that you will soon find the entrance to
%i.]],
      },
      locate_next = {
         text = "Once again, you are near the entrance to %i.",
      },
      nemesis_first = {
         synopsis = "[\"Come, %p, I shall destroy you!\"]",
         output = "text",
         text = [["So, %p, you think that you can succeed in recovering
%o, when your teacher, %l, has already failed.

"Come, try your best!  I shall destroy you, and gnaw on your bones."]],
      },
      nemesis_next = {
         synopsis = "[\"Again you try to best me, %p?  You shall never recover %o.\"]",
         output = "text",
         text = [["Again you try to best me, eh %p?  Well, you shall fail again.

"You shall never recover %o.

"I shall bear your soul to the Plane of Origins for my master's pleasure."]],
      },
      nemesis_other = {
         text = "\"You persist yet %p!  Good.  Now, you shall die!\"",
      },
      nemesis_wantsit = {
         text = [["I shall have %o from you, %p, then feast
upon your entrails!"]],
      },
      nexttime = {
         text = "Once again, you are back at %H.",
      },
      offeredit = {
         synopsis = "[%lC instructs you to guard %o from now on.]",
         output = "text",
         text = [[%lC touches %o briefly, gazes into it,
then smiles at you and says:

"Well done, %p.  You have defeated %n and
recovered %o.  But I fear that it shall never be safe
here.

Please take %o with you.  You, %p, can
guard it now far better than I.

May the blessings of %d follow you and guard you."]],
      },
      offeredit2 = {
         synopsis = "[\"Resume your search for the Amulet beyond the magic portal to %Z.\"]",
         output = "text",
         text = [["Careful, %p!  %oC might break, and that would be
a tragic loss.  You are its keeper now, and the time has come to
resume your search for the Amulet.  %Z await your
return through the magic portal that brought you here."]],
      },
      othertime = {
         text = [[You are back at %H.
You have an odd feeling this may be the last time you ever come here.]],
      },
      posthanks = {
         synopsis = "[\"Have you progressed with your quest to regain the Amulet of Yendor for %d?\"]",
         output = "text",
         text = [["Welcome back, %p.  Have you progressed with your quest to
regain the Amulet of Yendor for %d?"]],
      },
   },
   Bar = {
      assignquest = {
         synopsis = "[\"Find %n, defeat %ni, and return %o to us.\"]",
         output = "text",
         text = [["The world is in great need of your assistance, %p.

"About six months ago, I learned that a mysterious sorcerer, known
as %n, had begun to gather a large group of cutthroats and brigands
about %ni.

"At about the same time, these people you once rode with `liberated' a
potent magical talisman, %o, from a Turanian caravan.

"%nC and %nj Black Horde swept down upon %i and defeated
the people there, driving them out into the desert.  He has taken
%o, and seeks to bend it to %nj will.  I detected the
subtle changes in the currents of fate, and joined these people.
Then I sent forth a summons for you.

"If %n can bend %o to %nj will, he will become
almost indestructible.  He will then be able to enslave the minds of
men across the world.  You are the only hope.  The gods smile upon you,
and with %d behind you, you alone can defeat %n.

"You must go to %i.  From there, you can track down
%n, defeat %ni, and return %o to us.  Only
then will the world be safe."]],
      },
      badalign = {
         synopsis = "[\"You have wandered from the path of the %a.  Come back when you have atoned.\"]",
         output = "text",
         text = [["%pC!  You have wandered from the path of the %a!
If you attempt to overcome %n in this state, he will surely
enslave your soul.  Your only hope, and ours, lies in your purification.
Go forth, and return when you feel ready."]],
      },
      badlevel = {
         synopsis = "[\"You are too inexperienced.  Come back when you are %Ra.\"]",
         output = "text",
         text = [["%p, I fear that you are as yet too inexperienced to face
%n.  Only %Ra with the help of %d could ever hope to
defeat %ni."]],
      },
      discourage = {
         "\"My pets will dine on your carcass tonight!\"",
         "\"You are a sorry excuse for %ra.\"",
         "\"Run while you can, %c.  My next spell will be your last.\"",
         "\"I shall use your very skin to bind my next grimoire.\"",
         "\"%d cannot protect you now.  Here, you die.\"",
         "\"Your %a nature makes you weak.  You cannot defeat me.\"",
         "\"Come, %c.  I shall kill you, then unleash the horde on your tribe.\"",
         "\"Once you are dead, my horde shall finish off %l, and your tribe.\"",
         "\"Fight, %c, or are you afraid of the mighty %n?\"",
         "\"You have failed, %c.  Now, my victory is complete.\"",
      },
      encourage = {
         "\"%nC is strong in the dark arts, but not immune to cold steel.\"",
         "\"Remember that %n is a great sorcerer.  He lived in the time of Atlantis.\"",
         "\"If you fail, %p, I will not be able to protect these people long.\"",
         "\"To enter %i, you must be very stealthy.  The horde will be on guard.\"",
         "\"Call upon %d in your time of need.\"",
         "\"May %d protect you, and guide your steps.\"",
         "\"If you can lay hands upon %o, carry it for good fortune.\"",
         "\"I cannot stand against %ns sorcery.  But %d will help you.\"",
         "\"Do not fear %n.  I know you can defeat %ni.\"",
         "\"You have a great road to travel, %p, but only after you defeat %n.\"",
      },
      firsttime = {
         synopsis = "[You reach the vicinity of %H, but sense evil magic nearby.]",
         output = "text",
         text = [[Warily you scan your surroundings, all of your senses alert for signs
of possible danger.  Off in the distance, you can %x the familiar shapes
of %H.

But why, you think, should %l be there?

Suddenly, the hairs on your neck stand on end as you detect the aura of
evil magic in the air.

Without thought, you ready your weapon, and mutter under your breath:

    "By %d, there will be blood spilt today."]],
      },
      goal_first = {
         synopsis = "[This is surely the lair of %n.]",
         output = "text",
         text = [[The hairs on the nape of your neck lift as you sense an energy in the
very air around you.  You fight down a primordial panic that seeks to
make you turn and run.  This is surely the lair of %n.]],
      },
      goal_next = {
         text = "Yet again you feel the air around you heavy with malevolent magical energy.",
      },
      gotit = {
         synopsis = "[You feel the power of %o flowing through your hands.]",
         output = "text",
         text = [[As you pick up %o, you feel the power of it
flowing through your hands.  It seems to be in two or more places
at once, even though you are holding it.]],
      },
      guardtalk_after = {
         "\"The battles here have been good -- our enemies' blood soaks the soil!\"",
         "\"Remember that glory is crushing your enemies beneath your feet!\"",
         "\"Times will be good again, now that the horde is vanquished.\"",
         "\"You have brought our clan much honor in defeating %n.\"",
         "\"You will be a worthy successor to %l.\"",
      },
      guardtalk_before = {
         "\"The battles here have been good -- our enemies' blood soaks the soil!\"",
         "\"Remember that glory is crushing your enemies beneath your feet!\"",
         "\"There has been little treasure to loot, since the horde arrived.\"",
         "\"The horde is mighty in numbers, but they have little courage.\"",
         "\"%lC is a strange one, but he has helped defend us.\"",
      },
      hasamulet = {
         synopsis = "[\"Take the Amulet to the altar of %d on the Astral Plane and offer it.\"]",
         output = "text",
         text = [["This is wondrous, %p.  I feared that you could not possibly
succeed in your quest, but here you are in possession of the Amulet
of Yendor!

"I have studied the texts of the magi constantly since you left.  In
the Book of Skelos, I found this:

    %d will cause a child to be sent into the world.  This child is to
    be made strong by trial of battle and magic, for %d has willed it so.
    It is said that the child of %d will recover the Amulet of Yendor
    that was stolen from the Creator at the beginning of time.

"As you now possess the amulet, %p, I suspect that the Book
speaks of you.

    The child of %d will take the Amulet, and travel to the Astral
    Plane, where the Great Temple of %d is to be found.  The Amulet
    will be sacrificed to %d, there on %dJ altar.  Then the child will
    stand by %d as champion of all %cP for eternity.

"This is all I know, %p.  I hope it will help you."]],
      },
      killed_nemesis = {
         synopsis = "[%nC curses you, but you feel the overpowering aura of magic fading.]",
         output = "text",
         text = [[%nC falls to the ground, and utters a last curse at you.  Then %nj
body fades slowly, seemingly dispersing into the air around you.  You
slowly become aware that the overpowering aura of magic in the air has
begun to fade.]],
      },
      leader_first = {
         synopsis = "[\"At last you have returned.  There is a great quest you must undertake.\"]",
         output = "text",
         text = [["Ah, %p.  You have returned at last.  The world is in dire
need of your help.  There is a great quest you must undertake.

"But first, I must see if you are ready to take on such a challenge."]],
      },
      leader_last = {
         synopsis = "[\"You have betrayed %d; soon %n will destroy us.  Begone!\"]",
         output = "text",
         text = [["Pah!  You have betrayed the gods, %p.  You will never attain
the glory which you aspire to.  Your failure to follow the true path has
closed this future to you.

"I will protect these people as best I can, but soon %n will overcome
me and destroy all who once called you %s.  Now begone!"]],
      },
      leader_next = {
         text = "\"%p, you are back.  Are you ready now for the challenge?\"",
      },
      leader_other = {
         text = "\"Again, you stand before me, %p.  Surely you have prepared yourself.\"",
      },
      locate_first = {
         synopsis = "[You have located %i.]",
         output = "text",
         text = [[The scent of water comes to you in the desert breeze.  You know that
you have located %i.]],
      },
      locate_next = {
         text = "Yet again you have a chance to infiltrate %i.",
      },
      nemesis_first = {
         synopsis = "[%nC boasts that %nh has slain many.  \"Prepare to die, %c.\"]",
         output = "text",
         text = [["So.  This is what that second rate sorcerer %l sends to do %lj bidding.
I have slain many before you.  You shall give me little sport.

"Prepare to die, %c."]],
      },
      nemesis_next = {
         text = "\"I have wasted too much time on you already.  Now, you shall die.\"",
      },
      nemesis_other = {
         text = "\"You return yet again, %c!  Are you prepared for death now?\"",
      },
      nemesis_wantsit = {
         text = [["I shall have %o back, you pitiful excuse for %ca.
And your life as well."]],
      },
      nexttime = {
         text = [[Once again, you near %H.  You know that %l
will be waiting.]],
      },
      offeredit = {
         synopsis = "[%lC tells you to guard %o, and to return when you have triumphed.]",
         output = "text",
         text = [[When %l sees %o, he smiles, and says:

    Well done, %p.  You have saved the world from certain doom.
    What, now, should be done with %o?

    These people, brave as they are, cannot hope to guard it from
    other sorcerers who will detect it, as surely as %n did.

    Take %o with you, %p.  It will guard you in
    your adventures, and you can best guard it.  You embark on a
    quest far greater than you realize.

    Remember me, %p, and return when you have triumphed.  I
    will tell you then of what you must do.  You will understand when the
    time comes.]],
      },
      offeredit2 = {
         synopsis = "[\"You keep %o.  Return to %Z to search for the Amulet.\"]",
         output = "text",
         text = [[%l gazes reverently at %o, then back at you.

"You are its keeper now, and the time has come to resume your search
for the Amulet.  %Z await your return through the
magic portal which brought you here."]],
      },
      othertime = {
         text = [[Again, and you think possibly for the last time, you approach
%H.]],
      },
      posthanks = {
         text = "\"Tell us, %p, have you fared well on your great quest?\"",
      },
   },
   Cav = {
      assignquest = {
         synopsis = "[Find and defeat %n, recover %o, and return with it.]",
         output = "text",
         text = [["You are indeed ready now, %p.  I shall tell you a tale of
great suffering among your people:

"Shortly after you left on your vision quest, the caves were invaded by
the creatures sent against us by %n.

"She, herself, could not attack us due to her great size, but her minions
have harassed us ever since.  In the first attacks, many died, and the
minions of %n managed to steal %o.
They took it to %i and there, none of our
%g warriors have been able to go.

"You must find %i, and within it wrest
%o from %n.  She guards it as
jealously as she guards all treasures she attains.  But with it,
we can make our caves safe once more.

"Please, %p, recover %o for us, and return it here."]],
      },
      badalign = {
         synopsis = "[\"You no longer follow the path of the %a.  Go, and purify yourself.\"]",
         output = "text",
         text = [["%pC!  You have deviated from my teachings.  You no longer follow
the path of the %a as you should.  I banish you from these caves, to
go forth and purify yourself.  Then, you might be able to accomplish this
quest."]],
      },
      badlevel = {
         synopsis = "[\"%rA is too inexperienced.  Come back when you have progressed.\"]",
         output = "text",
         text = [["Alas, %p, you are as yet too inexperienced to embark upon such
a difficult quest as that I propose to give you.

"%rA could not possibly survive the rigors demanded to find
%i, never mind to confront %n herself.

"Adventure some more, and you will learn the skills you will require.
%d decrees it."]],
      },
      discourage = {
         "\"You are weak, %c.  No challenge for the Mother of all Dragons.\"",
         "\"I grow hungry, %r.  You look like a nice appetizer!\"",
         "\"Join me for lunch?  You're the main course, %c.\"",
         "\"With %o, I am invincible!  You cannot succeed.\"",
         "\"Your mentor, %l has failed.  You are nothing to fear.\"",
         "\"You shall die here, %c.  %rA cannot hope to defeat me.\"",
         "\"You, a mere %r challenge the might of %n?  Hah!\"",
         "\"I am the Mother of all Dragons!  You cannot hope to defeat me.\"",
         "\"My claws are sharp now.  I shall rip you to shreds!\"",
         "\"%d has deserted you, %c.  This is my domain.\"",
      },
      encourage = {
         "\"%nC is immune to her own breath weapons. You should use magic upon her that she does not use herself.\"",
         "\"When you encounter %n, call upon %d for assistance.\"",
         "\"There will be nowhere to hide inside %ns inner sanctum.\"",
         "\"Your best chance with %n will be to keep moving.\"",
         "\"Do not be distracted by the great treasures in %ns lair. Concentrate on %o.\"",
         "\"%oC is the only object that %n truly fears.\"",
         "\"Do not be fooled by %ns size.  She is fast, and it is rumored that she uses magic.\"",
         "\"I would send a party of %gP with you, but we will need all of our strength to defend ourselves.\"",
         "\"Remember, be %a at all times.  This is your strength.\"",
         "\"If only we had an amulet of reflection, this would not have happened.\"",
      },
      firsttime = {
         synopsis = "[You arrive back at %H, but something is wrong here.]",
         output = "text",
         text = [[You descend through a barely familiar stairwell that you remember
%l showing you when you embarked upon your vision quest.

You arrive back at %H, but something seems
wrong here.  The usual smoke and glowing light of the fires of the
outer caves are absent, and an uneasy quiet fills the damp air.]],
      },
      goal_first = {
         synopsis = "[You enter a large cavern.  %nC is present.]",
         output = "text",
         text = [[You find yourself in a large cavern, with neatly polished walls, that
nevertheless show signs of being scorched by fire.

Bones litter the floor, and there are objects scattered everywhere.
The air is close with the stench of sulphurous fumes.

%nC is clearly visible, but %nh seems to be asleep.]],
      },
      goal_next = {
         text = "Once again, you find yourself in the lair of %n.",
      },
      gotit = {
         synopsis = "[%oC fills you with a feeling of power.]",
         output = "text",
         text = [[As you pick up %o it seems heavy at first, but as you
hold it strength flows into your arms.

You suddenly feel full of power, as if nothing could possibly stand
in your path.]],
      },
      guardtalk_after = {
         "\"The rains have returned and the land grows lush again.\"",
         "\"Peace has returned, give thanks to %d!\"",
         "\"Welcome back!  Did you find %o?\"",
         "\"So, %p, tell us the story of your fight with %n.\"",
         "\"%lC grows old.  Perhaps you will guide us after he ascends.\"",
      },
      guardtalk_before = {
         "\"We have not been able to gather as much food since the Giants sealed off our access to the outer world.\"",
         "\"Since %n sent her minions, we have been constantly fighting.\"",
         "\"I have heard your vision quest was successful.  Is this so?\"",
         "\"So, tell me, %p, how have you fared?\"",
         "\"%lC grows old.  We know not who will guide us after he ascends.\"",
      },
      hasamulet = {
         synopsis = "[\"Take the Amulet to the altar of %d on the Astral Plane and offer it.\"]",
         output = "text",
         text = [["You have been successful, I see, %p.

"Now that the Amulet of Yendor is yours, here is what you must do:

"Journey upwards to the open air.  The Amulet you carry will then
take you into the Astral Planes, where the Great Temple of %d
casts its influence throughout our world.

"Sacrifice the Amulet on the altar.  Thus shall %d become supreme!"]],
      },
      killed_nemesis = {
         text = [[%nC sinks to the ground, her heads flailing about.
As she dies, a cloud of noxious fumes billows about her.]],
      },
      leader_first = {
         synopsis = "[\"You have returned.  We are in dire need of your help.\"]",
         output = "text",
         text = [["You have returned from your vision quest, %p.  Thank %d.

"We are in dire need of your help, my %S.

"But first, I must see if you are yet capable of the quest I would
ask you to undertake."]],
      },
      leader_last = {
         synopsis = "[\"You have betrayed the %L.  Begone!\"]",
         output = "text",
         text = [["%pC!  You have sealed our fate.  You seem unable to reform yourself,
so I must select another to take your place.

"Begone from %H!  You have betrayed us by choosing
the path of the %C over the true path of the %L.

"You no longer live in our eyes."]],
      },
      leader_next = {
         text = "\"Again, you return to us, %p.  Let me see if you are ready now.\"",
      },
      leader_other = {
         text = "\"Ah, %p.  Are you finally ready?\"",
      },
      locate_first = {
         synopsis = "[You %x many large claw marks, smell carrion, and notice bones.]",
         output = "text",
         text = [[You %x many large claw marks on the ground.  The tunnels ahead
of you are larger than most of those in any cave complex you have
ever been in before.

Your nose detects the smell of carrion from within, and bones litter
the sides of the tunnels.]],
      },
      locate_next = {
         text = "Once again, you approach %i.",
      },
      nemesis_first = {
         synopsis = "[%nC threatens to eat you.]",
         output = "text",
         text = [["So, follower of %l, you seek to invade the lair of
%n.  Only my meals are allowed down here.  Prepare
to be eaten!"]],
      },
      nemesis_next = {
         text = [["So, again you face me, %c.  No one has ever before escaped me.
Now I shall kill you."]],
      },
      nemesis_other = {
         text = "\"You are getting annoying, %c.  Prepare to die.\"",
      },
      nemesis_wantsit = {
         text = "\"I'll have %o from you, %c.  You shall die.\"",
      },
      nexttime = {
         text = "Once again, you arrive back at %H.",
      },
      offeredit = {
         synopsis = "[\"Take %o with you.  It will help in your quest for the Amulet of Yendor.\"]",
         output = "text",
         text = [[%lC glimpses %o in your possession.
He smiles and says:

    You have done it!  We are saved.  But I fear that %o
    will always be a target for %C forces who will want it for their
    own.

    To prevent further trouble, I would like you, %p,
    to take %o away with you.  It will help you as you
    quest for the Amulet of Yendor.]],
      },
      offeredit2 = {
         synopsis = "[\"You are the keeper of %o now.  Return to %Z to search for the Amulet.]",
         output = "text",
         text = [[%l grasps %o proudly for a moment, then looks at you.

"You are its keeper now, and the time has come to resume your search
for the Amulet.  %Z await your return through the
magic portal which brought you here."]],
      },
      othertime = {
         text = [[For some reason, you think that this may be the last time you will
enter %H.]],
      },
      posthanks = {
         text = [["%pC!  Welcome back.
How goes your quest to recover the Amulet for %d?"]],
      },
   },
   Hea = {
      assignquest = {
         synopsis = "[Travel to %i on your way to recover %o from %n.]",
         output = "text",
         text = [[For the first time, you sense a smile on %ls face.

    "You have indeed learned as much as we can teach you in preparation
    for this task.  Let me tell you what I know of the symptoms and hope
    that you can provide a cure.

    "A short while ago, the dreaded %nt was fooled by the gods
    into thinking that %nh could use %o to find a
    cure for old age.  Think of it, eternal youth!  But %nj good
    health is accomplished by drawing the health from those around %ni.

    "He has exhausted %nj own supply of healthy people and now %nh seeks to
    extend %nj influence into our world.  You must recover from %ni
    %o and break the spell.

    "You must travel into the swamps to %i, and from there
    follow the trail to %ns island lair.  Be careful."]],
      },
      badalign = {
         synopsis = "[Return when you are more %a.]",
         output = "text",
         text = [["You have learned much of the remedies that benefit, but you must also
know which physic for which ail.  That is why %ds teachings are a
part of your training.

"Return to us when you have healed thyself."]],
      },
      badlevel = {
         synopsis = "[You are too inexperienced.  Return when you are %Ra.]",
         output = "text",
         text = [["Alas, %p, you are yet too inexperienced to deal with the rigors
of such a task.  You must be able to draw on the knowledge of botany,
alchemy and veterinary practices before I can send you on this quest 
with good conscience.

"Return when you wear %Ra's caduceus."]],
      },
      discourage = {
         "\"They might as well give scalpels to wizards as to let you try to use %o!\"",
         "\"If I could strike %l, surrounded by %lj %gP, imagine what I can do to you here by yourself.\"",
         "\"I will put my %Rp to work making a physic out of your ashes.\"",
         "\"As we speak, Hades gathers your patients to join you.\"",
         "\"After I'm done with you, I'll destroy %l as well.\"",
         "\"You will have to kill me if you ever hope to leave this place.\"",
         "\"I will impale your head on my caduceus for all to see.\"",
         "\"There is no materia medica in your sack which will cure you of me!\"",
         "\"Do not fight too hard, I want your soul strong, not weakened!\"",
         "\"You should have stopped studying at vetenary.\"",
      },
      encourage = {
         "\"Remember, %p, to always wash your hands before operating.\"",
         "\"%nC has no real magic of %nj own.  To this %nh is vulnerable.\"",
         "\"If you have been true to %d, you can draw on the power of %o.\"",
         "\"Bring with you antidotes for poisons.\"",
         "\"Remember this, %n can twist the powers of %o to hurt instead of heal.\"",
         "\"I have sent for Chiron, but I am afraid he will come too late.\"",
         "\"Maybe when you return the snakes will once again begin to shed.\"",
         "\"The plague grows worse as we speak.  Hurry, %p!\"",
         "\"Many times %n has caused trouble in these lands.  It is time that %nh was eradicated like the diseases %nh has caused.\"",
         "\"With but one eye, %n should be easy to blind.  Remember this.\"",
      },
      firsttime = {
         synopsis = "[You arrive back at %H and must find %l.]",
         output = "text",
         text = [[What sorcery has brought you back to %H?  The smell
of fresh funeral pyres tells you that something is amiss with the healing
powers that used to practice here.

No rhizotomists are tending the materia medica gardens, and where are the
common folk who used to come for the cures?

You know that you must quickly make your way to the collegium, and
%ls iatreion, and find out what has happened in your absence.]],
      },
      goal_first = {
         synopsis = "[You have reached the lair of %n.  Take %o away from %ni.]",
         output = "text",
         text = [[You stand within sight of the infamous Isle of %n.  Even
the words of %l had not prepared you for this.

Steeling yourself against the wails of the ill that pierce your ears,
you hurry on your task.  Maybe with %o you can
heal them on your return, but not now.]],
      },
      goal_next = {
         text = "Once again, you %x the Isle of %n in the distance.",
      },
      gotit = {
         synopsis = "[You feel the healing power of %o and should return it to %l.]",
         output = "text",
         text = [[As you pick up %o, you feel its healing begin to
warm your soul.  You curse Zeus for taking it from its rightful owner,
but at least you hope that %l can put it to good use once
again.]],
      },
      guardtalk_after = {
         "\"Did you read that new treatise on the therapeutic use of leeches?\"",
         "\"Paint a red caduceus on your shield and monsters won't hit you.\"",
         "\"How are you feeling?  Perhaps a good bleeding will improve your spirits.\"",
         "\"Have you heard the absurd new theory that diseases are caused by microscopic organisms, and not ill humors?\"",
         "\"I see that you bring %o, now you can cure this plague!\"",
      },
      guardtalk_before = {
         "\"Did you read that new treatise on the therapeutic use of leeches?\"",
         "\"Paint a red caduceus on your shield and monsters won't hit you.\"",
         "\"I passed handwriting so they are demoting me a rank.\"",
         "\"I've heard that even %l has not been able to cure Chiron.\"",
         "\"We think %n has used %nj alchemists, and %o, to unleash a new disease we call 'the cold' on Gehennom.\"",
      },
      hasamulet = {
         synopsis = "[\"You have recovered the Amulet.  Travel to the Astral Plane and return it to %d.\"]",
         output = "text",
         text = [["Ah, you have recovered the Amulet, %p.  Well done!

"Now, you should know that you must travel through the Elemental Planes
to the Astral, and there return the Amulet to %d.  Go forth and
may our prayers be as a wind upon your back."]],
      },
      killed_nemesis = {
         synopsis = "[%nC curses you as %nh dies.]",
         output = "text",
         text = [[The battered body of %n slumps to the ground and gasps
out one last curse:

    "You have defeated me, %p, but I shall have my revenge.
    How, I shall not say, but this curse shall be like a cancer
    on you."

With that %n dies.]],
      },
      leader_first = {
         synopsis = "[%l is weak from the struggle with %n.  %lH wants to examine you.]",
         output = "text",
         text = [[Feebly, %l raises %lj head to look at you.

"It is good to see you again, %p.  I see the concern in your
eyes, but do not worry for me.  I am not ready for Hades yet.  We have
exhausted much of our healing powers holding off %n.
I need your fresh strength to carry on our work.

"Come closer and let me lay hands on you, and determine if you have
the skills necessary to accomplish this mission."]],
      },
      leader_last = {
         synopsis = "[You are a failure as a healer.]",
         output = "text",
         text = [["You have failed us, %p.  You are a quack!  A charlatan!

"Hades will be happy to hear that you are once again practicing your
arts on the unsuspecting."]],
      },
      leader_next = {
         text = [["Again you return to me, %p.  I sense that each trip back
the pleurisy and maladies of our land begin to infect you.  Let us
hope and pray to %d that you become ready for your task before
you fall victim to the bad humors."]],
      },
      leader_other = {
         text = [["Chiron has fallen, Hermes has fallen, what else must I tell you to
impress upon you the importance of your mission!  I hope that you
have come prepared this time."]],
      },
      locate_first = {
         synopsis = "[You have reached %i but all is not well.]",
         output = "text",
         text = [[You stand before the entrance to %i.  Strange
scratching noises come from within the building.

The swampy ground around you seems to stink with disease.]],
      },
      locate_next = {
         text = "Once again you stand at the entrance to %i.",
      },
      nemesis_first = {
         synopsis = "[\"I will take your life, then defeat %l.\"]",
         output = "text",
         text = [["They have made a mistake in sending you, %p.

"When I add your youth to mine, it will just make it easier for me
to defeat %l."]],
      },
      nemesis_next = {
         text = "\"Unlike your patients, you seem to keep coming back, %p!\"",
      },
      nemesis_other = {
         text = "\"Which would you like, %p?  Boils, pleurisy, convulsions?\"",
      },
      nemesis_wantsit = {
         text = [["I'll have %o back from you, %r.  You are
not going to live to escape this place."]],
      },
      nexttime = {
         text = [[After your last experience you expected to be here, but you certainly
did not expect to see things so much worse.  This time you must succeed.]],
      },
      offeredit = {
         synopsis = "[%l touches %o and tells %lj %gP to do so too, then tells you to take it with you.]",
         output = "text",
         text = [[As soon as %l sees %o %lh summons %lj
%gP.

Gently, %l reaches out and touches %o.
He instructs each of the assembled to do the same.  When everyone
has finished %lh speaks to you.

    "Now that we have been replenished we can defeat this plague.  You must
    take %o with you and replenish the worlds you have
    been called upon to travel next.  I wish you could ride Chiron to the
    end of your journey, but I need him to help me spread the cure.  Go
    now and continue your journey."]],
      },
      offeredit2 = {
         synopsis = "[%l tells you to keep %o and return to %Z to search for the Amulet.]",
         output = "text",
         text = [[%l cautiously handles %o while watching you.

"You are its keeper now, and the time has come to resume your search
for the Amulet.  %Z await your return through the
magic portal which brought you here."]],
      },
      othertime = {
         text = [[Again, you %x %H in the distance.

The smell of death and disease permeates the air.  You do not have
to be %Ra to know that %n is on the verge of victory.]],
      },
      posthanks = {
         text = [["You have again returned to us, %p.  We have done well in your
absence, yes?  How fare you upon your quest for the Amulet?"]],
      },
   },
   Kni = {
      assignquest = {
         synopsis = "[Pass through %i to reach %n.  Destroy %ni and return with %o.]",
         output = "text",
         text = [["Ah, %p.  Thou art truly ready, as no %c before thee hath
been.  Hear now Our words:

"As thou noticed as thou approached %H, a great battle hath
been fought recently in these fields.  Know thou that Merlin himself
came to aid Us here as We battled the foul %n.  In the midst of that
battle, %n struck Merlin a great blow, felling him.  Then, as Our
forces were pressed back, %n stole %o.

"We eventually turned the tide, but lost many %cP in doing so.
Merlin was taken off by his apprentice, but hath not recovered.  We have
been told that so long as %n possesseth %o,
Merlin will not regain his health.

"We hereby charge thee with this most important of duties:

"Go forth from this place, to the fens, and there thou wilt find
%i.  From there, thou must track down %n.  Destroy the
beast, and return to Us %o.  Only then can
We restore Merlin to health."]],
      },
      badalign = {
         synopsis = "[Go and do penance.  Return when you are truly %a.]",
         output = "text",
         text = [["Thou dishonourest Us, %p!  Thou hast strayed from the path of
chivalry! Go from Our presence and do penance.  Only when thou art again
pure mayst thou return hence."]],
      },
      badlevel = {
         synopsis = "[You are not prepared to face %n.  Return when you are %Ra.]",
         output = "text",
         text = [["Verily, %p, thou hast done well.  That thou hast survived thus
far is a credit to thy valor, but thou art yet unprepared for
the demands required as Our Champion.  %rA, no matter how
pure, could never hope to defeat the foul %n.

"Journey forth from this place, and hone thy skills.  Return to
Our presence when thou hast attained the noble title of %R."]],
      },
      discourage = {
         "\"A mere %r can never withstand me!\"",
         "\"I shall kill thee now, and feast!\"",
         "\"Puny %c.  What manner of death dost thou wish?\"",
         "\"First thee, %p, then I shall feast upon %l.\"",
         "\"Hah!  Thou hast failed, %r.  Now thou shalt die.\"",
         "\"Die, %c.  Thou art as nothing against my might.\"",
         "\"I shall suck the marrow from thy bones, %c.\"",
         "\"Let's see...  Baked?  No.  Fried?  Nay.  Broiled?  Yea verily, that is the way I like my %c for dinner.\"",
         "\"Thy strength waneth, %p.  The time of thy death draweth near.\"",
         "\"Call upon thy precious %d, %p.  It shall not avail thee.\"",
      },
      encourage = {
         "\"Remember, %p, follow always the path of %d.\"",
         "\"Though %n is verily a mighty foe, We have confidence in thy victory.\"",
         "\"Beware, for %n hath surrounded %niself with hordes of foul creatures.\"",
         "\"Great treasure, 'tis said, is hoarded in the lair of %n.\"",
         "\"If thou possessest %o, %p, %ns magic shall therewith be thwarted.\"",
         "\"The gates of %i are guarded by forces unseen, %p. Go carefully.\"",
         "\"Return %o to Us quickly, %p.\"",
         "\"Destroy %n, %p, else %H shall surely fall.\"",
         "\"Call upon %d when thou art in need.\"",
         "\"To find %i, thou must keep thy heart pure.\"",
      },
      firsttime = {
         synopsis = "[Signs of battle include long gouges in the walls of %H.]",
         output = "text",
         text = [[You materialize in the shadows of %H.  Immediately, you notice
that something is wrong.  The fields around the castle are trampled and
withered, as if some great battle has been recently fought.

Exploring further, you %x long gouges in the walls of %H.
You know of only one creature that makes those kinds of marks...]],
      },
      goal_first = {
         synopsis = "[You %x the entrance to a cavern inside a hill.]",
         output = "text",
         text = [[As you exit the swamps, you %x before you a huge, gaping hole in the
side of a hill.  From within, you smell the foul stench of carrion.

The pools on either side of the entrance are fouled with blood, and
pieces of rusted metal and broken weapons show above the surface.]],
      },
      goal_next = {
         text = "Again, you stand at the entrance to %ns lair.",
      },
      gotit = {
         synopsis = "[You feel the magic of %o.]",
         output = "text",
         text = [[As you pick up %o, you feel its protective fields
form around your body.  You also feel a faint stirring in your mind, as
if you are in two places at once, and in the second, you are waking from
a long sleep.]],
      },
      guardtalk_after = {
         "\"Hail, %p!  Verily, thou lookest well.\"",
         "\"So, %p, didst thou find %n in the fens near %i?\"",
         "\"Worthy %p, hast thou proven thy right purpose on the body of %n?\"",
         "\"Verily, %l could have no better champion, %p.\"",
         "\"Hast thou indeed recovered %o?\"",
      },
      guardtalk_before = {
         "\"Hail, %p!  Verily, thou lookest well.\"",
         "\"There is word, %p, that %n hath been sighted in the fens near %i.\"",
         "\"Thou art our only hope now, %p.\"",
         "\"Verily, %l could have no better champion, %p.\"",
         "\"Many brave %cP died when %n attacked.\"",
      },
      hasamulet = {
         synopsis = "[Take the Amulet to the Astral Plane and deliver it to %d.]",
         output = "text",
         text = [["Thou hast succeeded, We see, %p!  Now thou art commanded to take
the Amulet to be sacrificed to %d in the Plane of the Astral.

"Merlin hath counseled Us that thou must travel always upwards through
the Planes of the Elements, to achieve this goal.

"Go with %d, %p."]],
      },
      killed_nemesis = {
         synopsis = "[%nC curses you as %nh dies.]",
         output = "text",
         text = [[As %n sinks to the ground, blood gushing from %nj open mouth, %nh
defiantly curses you and %l:

    "Thou hast not won yet, %r.  By the gods, I shall return
    and dog thy steps to the grave!"

%nJ tail flailing madly, %n tries to crawl towards you, but slumps
to the ground and dies in a pool of %nj own blood.]],
      },
      leader_first = {
         synopsis = "[%lC checks whether you are ready for a great undertaking.]",
         output = "text",
         text = [["Ah, %p.  We see thou hast received Our summons.
We are in dire need of thy prowess.  But first, We must needs
decide if thou art ready for this great undertaking."]],
      },
      leader_last = {
         synopsis = "[You are a disgrace as %ca.]",
         output = "text",
         text = [["Thou disgracest this noble court with thine impure presence.  We have been
lenient with thee, but no more.  Thy name shall be spoken no more.  We
hereby strip thee of thy title, thy lands, and thy standing as %ca.
Begone from Our sight!"]],
      },
      leader_next = {
         text = "\"Welcome again, %p.  We hope thou art ready now.\"",
      },
      leader_other = {
         text = "\"Once again, thou standest before Us, %p.  Art thou ready now?\"",
      },
      locate_first = {
         synopsis = "[You have reached %i and can %x a shrine.]",
         output = "text",
         text = [[You stand at the foot of %i.  Atop, you can %x a shrine.
Strange energies seem to be focused here, and the hair on the back of
your neck stands on end.]],
      },
      locate_next = {
         text = "Again, you stand at the foot of %i.",
      },
      nemesis_first = {
         synopsis = "[%nC taunts you and issues a threat against %H.]",
         output = "text",
         text = [["Hah!  Another puny %c seeks death.  I shall dine well tonight,
then tomorrow, %H shall fall!"]],
      },
      nemesis_next = {
         text = "\"Again, thou challengest me, %r?  So be it.  Thou wilt die here.\"",
      },
      nemesis_other = {
         text = "\"Thou art truly foolish, %r.  I shall dispatch thee anon.\"",
      },
      nemesis_wantsit = {
         text = [["So, thou darest touch MY property!  I shall have that bauble back,
puny %r.  Thou wilt die in agony!"]],
      },
      nexttime = {
         text = "Once again you stand in the shadows of %H.",
      },
      offeredit = {
         synopsis = "[%oC is yours now.  It will aid in your search for the Amulet.]",
         output = "text",
         text = [[As you approach %l, %lh beams at you and says:

    "Well done!  Thou art truly the Champion of %H.  We
    have received word that Merlin is recovering, and shall soon
    rejoin Us.

    "He hath instructed Us that thou art now to be the guardian of
    %o.  He feeleth that thou mayst have need of
    its powers in thine adventures.  It is Our wish that thou keepest
    %o with thee as thou searchest for the fabled
    Amulet of Yendor."]],
      },
      offeredit2 = {
         synopsis = "[You are the keeper of %o.  Return to %Z and find the Amulet.]",
         output = "text",
         text = [["Careful, %p!  %oC might break, and that would
be a tragic loss.  Thou art its keeper now, and the time hath come
to resume thy search for the Amulet.  %Z await thy
return through the magic portal that brought thee here."]],
      },
      othertime = {
         text = [[Again, you stand before %H.  You vaguely sense that this
may be the last time you stand before %l.]],
      },
      posthanks = {
         text = "\"Well met, %p.  How goeth thy search for the Amulet of Yendor?\"",
      },
   },
   Mon = {
      assignquest = {
         synopsis = "[Find %i, then continue to %ns lair.  Defeat %ni and return with %o.]",
         output = "text",
         text = [["Yes, %p.  You are truly ready now.  Attend to me and I shall
tell you of what has transpired:

"During one of the Great Meditations a short time ago, %n and
a legion of elementals invaded %H.  Many %gP
were killed, including the one bearing %o.

Now, there are barely enough %gP left to keep the elementals
at bay.

"We need you to find %i, then, from there,
travel to %ns lair.  If you can manage to defeat %n and
return %o here, we can then drive off the legions
of elementals that slay our students.

"Go with %d as your guide, %p."]],
      },
      badalign = {
         synopsis = "[You must atone.  Come back when you are worthy of %d.]",
         output = "text",
         text = [["This is terrible, %p.  You have deviated from the true path!
You know that %d requires the most strident devotion of this
order.  The %shood must stand for utmost piety.

"Go from here, atone for your sins against %d.  Return only when
you have purified yourself."]],
      },
      badlevel = {
         synopsis = "[You are not ready to face %n.  Come back when you are %Ra.]",
         output = "text",
         text = [["Alas, %p, it is not yet to be.  A mere %r could never
withstand the might of %n.  Go forth, again into the world, and
return when you have attained the post of %R."]],
      },
      discourage = {
         "\"Submit to my will, %c, and I shall spare you.\"",
         "\"Your puny powers are no match for me, %c.\"",
         "\"I shall have you turned into a zombie for my pleasure!\"",
         "\"Despair now, %r.  %d cannot help you.\"",
         "\"I shall feast upon your soul for many days, %c.\"",
         "\"Your death will be slow and painful.  That I promise!\"",
         "\"You cannot defeat %n, you fool.  I shall kill you now.\"",
         "\"Your precious %lt will be my next victim.\"",
         "\"I feel your powers failing you, %r.  You shall die now.\"",
         "\"With %o, nothing can stand in my way.\"",
      },
      encourage = {
         "\"You can prevail, if you rely on %d.\"",
         "\"Remember that %n has great magic at his command.\"",
         "\"Be pure, my %S.\"",
         "\"Beware, %i is surrounded by hordes of earth elementals.\"",
         "\"Remember your studies, and you will prevail!\"",
         "\"Acquire and wear %o if you can.  They will aid you against %n.\"",
         "\"Call upon %d when your need is greatest.  You will be answered.\"",
         "\"Remember to use the elementals' strength against them!\"",
         "\"Do not lose faith, %p.  If you do so, %n will grow stronger.\"",
         "\"Wear %o.  They will assist you in your efforts.\"",
      },
      firsttime = {
         synopsis = "[You have reached %H but something is wrong.  %lC needs your aid.]",
         output = "text",
         text = [[You find yourself standing in sight of %H.
Something is obviously wrong here.  Strange shapes lumber around
outside %H!

You realize that the %l needs your assistance!]],
      },
      goal_first = {
         synopsis = "[You are surrounded by brimstone, lava, and elementals.]",
         output = "text",
         text = [[The stench of brimstone is all about you, and the elementals close in
from all sides!

Ahead, there is a small clearing amidst the bubbling pits of lava...]],
      },
      goal_next = {
         text = "Again, you have invaded %ns domain.",
      },
      gotit = {
         synopsis = "[You feel the essence of %d and realize that you should take %o to %l.]",
         output = "text",
         text = [[As you pick up %o, you feel the essence of
%d fill your soul.  You know now why %n stole %oi from
%H, for with %oi, %ca of %d could
easily defeat his plans.

You sense a message from %d.  Though not verbal, you
get the impression that you must return to %l as soon
as possible.]],
      },
      guardtalk_after = {
         "\"Greetings, honorable %r.  It is good to see you again.\"",
         "\"Ah, %p!  Our deepest gratitude for all of your help.\"",
         "\"Greetings, %s.  Perhaps you will take some time to meditate with us?\"",
         "\"With this test behind you, may %d bring you enlightenment.\"",
         "\"May %d be with you, %s.\"",
      },
      guardtalk_before = {
         "\"Greetings, honorable %r.  It is good to see you.\"",
         "\"Ah, %p!  Surely you can help us in our hour of need.\"",
         "\"Greetings, %s.  %lC has great need of your help.\"",
         "\"Alas, it seems as if even %d has deserted us.\"",
         "\"May %d be with you, %s.\"",
      },
      hasamulet = {
         synopsis = "[Take the Amulet to the Astral Plane and deliver it to %d.]",
         output = "text",
         text = [["You have prevailed, %p!  %d is surely with you.  Now,
you must take the Amulet, and sacrifice it on %ds altar on
the Astral Plane.  I suspect that I shall never see you again in this
life, but I hope to at %ds feet."]],
      },
      killed_nemesis = {
         synopsis = "[As %n dies, %nh threatens to return.]",
         output = "text",
         text = [[%nC gasps:

    "You have only defeated this mortal body.  Know this: my spirit
    is strong.  I shall return and reclaim what is mine!"

With that, %n expires.]],
      },
      leader_first = {
         synopsis = "[%lC checks whether you are ready for the great challenge.]",
         output = "text",
         text = [["Ah, %p, my %S.  You have returned to us at last.
A great blow has befallen our order; perhaps you can help us.
First, however, I must determine if you are prepared for this
great challenge."]],
      },
      leader_last = {
         synopsis = "[You are a heretic and have failed utterly.]",
         output = "text",
         text = [["You are a heretic, %p!  How can you, %ra, deviate so from the
teachings of %d?  Begone from this temple.  You are no longer
%sa to this order.  We will pray to %d for other assistance,
as you have failed us utterly."]],
      },
      leader_next = {
         text = "\"Again, my %S, you stand before me.  Are you ready now to help us?\"",
      },
      leader_other = {
         text = "\"Once more, %p, you stand within the sanctum.  Are you ready now?\"",
      },
      locate_first = {
         synopsis = "[You have reached %i.  %nC lurks further ahead.]",
         output = "text",
         text = [[You remember the descriptions of %i, given
to you by the %l.  It is ahead that you will find
%n's trail.]],
      },
      locate_next = {
         text = "Again, you stand before %i.",
      },
      nemesis_first = {
         synopsis = "[You are no %g.  You shall never regain %o.]",
         output = "text",
         text = [["Ah, so %l has sent another %g to retrieve
%o.

"No, I see you are no %g.  Perhaps I shall have some fun today
after all.  Prepare to die, %r!  You shall never regain
%o."]],
      },
      nemesis_next = {
         text = "\"So, %r.  Again you challenge me.\"",
      },
      nemesis_other = {
         text = "\"Die now, %r.  %d has no power here to aid you.\"",
      },
      nemesis_wantsit = {
         text = "\"You shall die, %r, and I will have %o back.\"",
      },
      nexttime = {
         text = "Once again, you stand before %H.",
      },
      offeredit = {
         synopsis = "[Keep %o.  %oH will help you recover the Amulet of Yendor.]",
         output = "text",
         text = [["You have returned, %p.  And with %o, I see.
Congratulations.

"I have been in meditation, and have received direction from
a minion of %d.  %d commands that you retain
%o.  With %oi, you must recover the Amulet
of Yendor.

"Go forth, and let %d guide your steps."]],
      },
      offeredit2 = {
         synopsis = "[Keep %o and return to %Z to search for the Amulet.]",
         output = "text",
         text = [[%lC studies %o for a moment,
then returns his gaze to you.

"%oC must remain with you.  Use %oi
as you resume your search for the Amulet.
%Z await your return through the magic portal
that brought you here."]],
      },
      othertime = {
         text = [[Again you face %H.  Your intuition hints that this
may be the final time you come here.]],
      },
      posthanks = {
         text = "\"Welcome back, %p.  How is your quest for the Amulet going?\"",
      },
   },
   Pri = {
      assignquest = {
         synopsis = "[%nC invaded %H and captured %o.  Defeat %ni and retrieve %oh.]",
         output = "text",
         text = [["Yes, %p.  You are truly ready now.  Attend to me and I shall
tell you of what has transpired:

"At one of the Great Festivals a short time ago, %n and a legion
of undead invaded %H.  Many %gP were killed, including
the one carrying %o.

"As a final act of vengefulness, %n desecrated the altar here.
Without it, we could not mount a counter-attack.  Now, there are
barely enough %gP left to keep the undead at bay.

"We need you to find %i, then, from there, travel
to %ns lair.  If you can manage to defeat %n and return
%o here, we can then drive off the legions of
undead that befoul the land.

"Go with %d as your guide, %p."]],
      },
      badalign = {
         synopsis = "[You have deviated from the path.  Return when you have purified yourself.]",
         output = "text",
         text = [["This is terrible, %p.  You have deviated from the true path!
You know that %d requires the most strident devotion of this
order.  The %shood must stand for utmost piety.

"Go from here, atone for your sins against %d.  Return only when
you have purified yourself."]],
      },
      badlevel = {
         synopsis = "[%rA cannot withstand %n.  Come back when you are %Ra.]",
         output = "text",
         text = [["Alas, %p, it is not yet to be.  A mere %r could never
withstand the might of %n.  Go forth, again into the world, and return
when you have attained the post of %R."]],
      },
      discourage = {
         "\"Submit to my will, %c, and I shall spare you.\"",
         "\"Your puny powers are no match for me, %c.\"",
         "\"I shall have you turned into a zombie for my pleasure!\"",
         "\"Despair now, %r.  %d cannot help you.\"",
         "\"I shall feast upon your soul for many days, %c.\"",
         "\"Your death will be slow and painful.  That I promise!\"",
         "\"You cannot defeat %n, you fool.  I shall kill you now.\"",
         "\"Your precious %lt will be my next victim.\"",
         "\"I feel your powers failing you, %r.  You shall die now.\"",
         "\"With %o, nothing can stand in my way.\"",
      },
      encourage = {
         "\"You can prevail, if you rely on %d.\"",
         "\"Remember that %n has great magic at his command.\"",
         "\"Be pure, my %S.\"",
         "\"Beware, %i is surrounded by a great graveyard.\"",
         "\"You may be able to affect %n with magical cold.\"",
         "\"Acquire and wear %o if you can.  It will aid you against %n.\"",
         "\"Call upon %d when your need is greatest.  You will be answered.\"",
         "\"The undead legions are weakest during the daylight hours.\"",
         "\"Do not lose faith, %p.  If you do so, %n will grow stronger.\"",
         "\"Wear %o.  It will assist you against the undead.\"",
      },
      firsttime = {
         synopsis = "[You are at %H; the doors are closed.  %lC needs your help!]",
         output = "text",
         text = [[You find yourself standing in sight of %H.  Something
is obviously wrong here.  The doors to %H, which usually
stand open, are closed.  Strange human shapes shamble around
outside.

You realize that %l needs your assistance!]],
      },
      goal_first = {
         synopsis = "[The stench of brimstone surrounds you, the shrieks and moans are endless.]",
         output = "text",
         text = [[The stench of brimstone is all about you, and the shrieks and moans
of tortured souls assault your psyche.

Ahead, there is a small clearing amidst the bubbling pits of lava...]],
      },
      goal_next = {
         text = "Again, you have invaded %ns domain.",
      },
      gotit = {
         synopsis = "[You feel %d as you pick up %o; return %oh to %l.]",
         output = "text",
         text = [[As you pick up %o, you feel the essence of
%d fill your soul.  You know now why %n stole it from
%H, for with it, %ca of %d could
easily defeat his plans.

You sense a message from %d.  Though not verbal, you
get the impression that you must return to %l as soon
as possible.]],
      },
      guardtalk_after = {
         "\"Greetings, %r.  It is good to see you again.\"",
         "\"Ah, %p!  Our deepest gratitude for all of your help.\"",
         "\"Welcome back, %s!  With %o, no undead can stand against us.\"",
         "\"Praise be to %d, for delivering us from %n.\"",
         "\"May %d be with you, %s.\"",
      },
      guardtalk_before = {
         "\"Greetings, honored %r.  It is good to see you.\"",
         "\"Ah, %p!  Surely you can help us in our hour of need.\"",
         "\"Greetings, %s.  %lC has great need of your help.\"",
         "\"Alas, it seems as if even %d has deserted us.\"",
         "\"May %d be with you, %s.\"",
      },
      hasamulet = {
         synopsis = "[Take the Amulet to the Astral Plane and offer it on %ds altar.]",
         output = "text",
         text = [["You have prevailed, %p!  %d is surely with you.  Now,
you must take the amulet, and sacrifice it on %ds altar on
the Astral Plane.  I suspect that I shall never see you again in this
life, but I hope to at %ds feet."]],
      },
      killed_nemesis = {
         synopsis = "[%nC dies.  Moloch is aware of you and angry at %n.]",
         output = "text",
         text = [[You feel a wrenching shift in the ether as %ns body dissolves
into a cloud of noxious gas.

Suddenly, a voice booms out:

    "Thou hast defeated the least of my minions, %r.
    Know now that Moloch is aware of thy presence.
    As for thee, %n, I shall deal with thy failure
    at my leisure."

You then hear the voice of %n, screaming in terror...]],
      },
      leader_first = {
         synopsis = "[You have returned and we need your help.  Are you ready?]",
         output = "text",
         text = [["Ah, %p, my %S.  You have returned to us at last.
A great blow has befallen our order; perhaps you can help us.
First, however, I must determine if you are prepared for this
great challenge."]],
      },
      leader_last = {
         synopsis = "[You are a heretic who has deviated from the teachings of %d.]",
         output = "text",
         text = [["You are a heretic, %p!  How can you, %ra, deviate so from the
teachings of %d?  Begone from this temple.  You are no longer
%sa to this order.  We will pray to %d for other assistance,
as you have failed us utterly."]],
      },
      leader_next = {
         text = "\"Again, my %S, you stand before me.  Are you ready now to help us?\"",
      },
      leader_other = {
         text = "\"Once more, %p, you stand within the sanctum.  Are you ready now?\"",
      },
      locate_first = {
         synopsis = "[You have found %i.  The trail to %n lies ahead.]",
         output = "text",
         text = [[You stand facing a large graveyard.  The sky above is filled with clouds
that seem to get thicker closer to the center.  You sense the presence of
undead in larger numbers than you have ever encountered before.

You remember the descriptions of %i, given to you by
%l.  It is ahead that you will find %ns trail.]],
      },
      locate_next = {
         text = "Again, you stand before %i.",
      },
      nemesis_first = {
         synopsis = "[%lC has sent you, but you are no %gC.  I shall destroy you.]",
         output = "text",
         text = [["Ah, so %l has sent another %gC to retrieve
%o.

"No, I see you are no %gC.  Perhaps I shall have some fun today
after all.  Prepare to die, %r!  You shall never regain
%o."]],
      },
      nemesis_next = {
         text = "\"So, %r.  Again you challenge me.\"",
      },
      nemesis_other = {
         text = "\"Die now, %r.  %d has no power here to aid you.\"",
      },
      nemesis_wantsit = {
         text = "\"You shall die, %r, and I will have %o back.\"",
      },
      nexttime = {
         text = "Once again, you stand before %H.",
      },
      offeredit = {
         synopsis = "[Congratulations, %p.  Keep %o; go and recover the Amulet.]",
         output = "text",
         text = [["You have returned, %p.  And with %o, I see.
Congratulations.

"I have been in meditation, and have received direction from
a minion of %d.  %d commands that you retain
%o.  With it, you must recover the Amulet
of Yendor.

"Go forth, and let %d guide your steps."]],
      },
      offeredit2 = {
         synopsis = "[%oC is yours now.  Return to %Z and find the Amulet.]",
         output = "text",
         text = [[%lC reiterates that %o is yours now.

"The time has come to resume your search for the Amulet.
%Z await your return through the magic portal
that brought you here."]],
      },
      othertime = {
         text = [[Again you face %H.  Your intuition hints that this may be
the final time you come here.]],
      },
      posthanks = {
         text = "\"Welcome back, %p.  How is your quest for the Amulet going?\"",
      },
   },
   Ran = {
      assignquest = {
         synopsis = "[%nC has stolen %o.  Infiltrate %i and retrieve %oh for us.]",
         output = "text",
         text = [["You are indeed ready, %p.  I shall tell you what has transpired,
and why we so desperately need your help:

"A short time ago, the mountain centaurs to the east invaded
and enslaved the plains centaurs in this area.  The local
leader is now only a figurehead, and serves %n.

"During our last gathering of worship here, we were beset by hordes of
hostile centaurs, as you witnessed.  In the first onslaught a group,
headed by %n %niself, managed to breach the grove and steal
%o.

"Since then, we have been besieged.  We do not know how much longer
we will be able to maintain our magical barriers.

"If we are to survive, you, %p, must infiltrate
%i.  There, you will find a pathway down, to the
underground cavern of %n.  He has always coveted
%o, and will surely keep it.

"Recover %o for us, %p!  Only then will %d be safe."]],
      },
      badalign = {
         synopsis = "[You are not sufficiently %a.  Come back when you have purified yourself.]",
         output = "text",
         text = [["You have strayed, %p!  You know that %d requires that
we maintain a pure devotion to things %a!

"You must go from us.  Return when you have purified yourself."]],
      },
      badlevel = {
         synopsis = "[You are too inexperienced.  Come back when you are %Ra.]",
         output = "text",
         text = [["%p, you are yet too inexperienced to withstand the demands of that
which we need you to do.  %RA might just be able to do this thing.

"Return to us when you have learned more, my %S."]],
      },
      discourage = {
         "\"Your %d is nothing, %c.  You are mine now!\"",
         "\"Run away little %c!  You can never hope to defeat %n!\"",
         "\"My servants will rip you to shreds!\"",
         "\"I shall display your head as a trophy.  What do you think about that wall?\"",
         "\"I shall break your %ls grove, and destroy all the %gP!\"",
         "\"%d has abandoned you, %c.  You are doomed.\"",
         "\"%rA?  %lC sends a mere %r against me?  Hah!\"",
         "\"%lC has failed, %c.  %oC will never leave here.\"",
         "\"You really think you can defeat me, eh %c?  You are wrong!\"",
         "\"You weaken, %c.  I shall kill you now.\"",
      },
      encourage = {
         "\"It is rumored that the Forest and Mountain Centaurs have resolved their ancient feud and now band together against us.\"",
         "\"%nC is strong, and very smart.\"",
         "\"Use %o, when you find it.  It will help you survive to reach us.\"",
         "\"Remember, let %d be your guide.\"",
         "\"Call upon %d when you face %n. The very act of doing so will infuriate him, and give you advantage.\"",
         "\"%n and his kind have always hated us.\"",
         "\"We cannot hold the grove much longer, %p.  Hurry!\"",
         "\"To infiltrate %i, you must be very stealthy.\"",
         "\"Remember that %n is a braggart.  Trust not what he says.\"",
         "\"You can triumph, %p, if you trust in %d.\"",
      },
      firsttime = {
         synopsis = "[The ancient forest grove is surrounded by centaurs.]",
         output = "text",
         text = [[You arrive in familiar surroundings.  In the distance, you %x the
ancient forest grove, the place of worship to %d.

Something is wrong, though.  Surrounding the grove are centaurs!
And they've noticed you!]],
      },
      goal_first = {
         synopsis = "[You descend into a subterranean complex.  Hooves clatter in the distance.]",
         output = "text",
         text = [[You descend into a weird place, in which roughly cut cave-like walls
join with smooth, finished ones, as if someone was in the midst of
finishing off the construction of a subterranean complex.

Off in the distance, you hear a sound like the clattering of many
hooves on rock.]],
      },
      goal_next = {
         text = "Once again, you enter the distorted castle of %n.",
      },
      gotit = {
         synopsis = "[You pick up %o and feel power.  It's time to return %oh to %l.]",
         output = "text",
         text = [[As you pick up %o, it seems to glow, and a warmth
fills you completely.  You realize that its power is what has protected
your %sp against their enemies for so long.

You must now return it to %l without delay -- their lives depend
on your speed.]],
      },
      guardtalk_after = {
         "\"%pC!  I have not seen you in many moons.  How do you fare?\"",
         "\"Birdsong has returned to the grove, surely this means you have defeated %n.\"",
         "\"%lC seems to have regained some of his strength.\"",
         "\"So, tell us how you entered %i, in case some new evil arises there.\"",
         "\"Is that truly %o that I see you carrying?\"",
      },
      guardtalk_before = {
         "\"%pC!  I have not seen you in many moons.  How do you fare?\"",
         "\"%nC continues to threaten the grove.  But we hold fast.\"",
         "\"%lC is growing weak.  The magic required to defend the grove drains us.\"",
         "\"Remember %i is hard to enter.  Beware the distraction of leatherwings.\"",
         "\"We must regain %o.  Without it we will be overrun.\"",
      },
      hasamulet = {
         synopsis = "[You have the Amulet!  Take it to the Astral Plane and offer it to %d.]",
         output = "text",
         text = [["You have it!  You have recovered the Amulet of Yendor!
Now attend to me, %p, and I will tell you what must be done:

"The Amulet has within it magic, the capability to transport you to
the Astral Plane, where the primary circle of %d resides.

"To activate this magic, you must travel upwards as far as you can.
When you reach the temple, sacrifice the Amulet to %d.

"Thus will you fulfill your destiny."]],
      },
      killed_nemesis = {
         synopsis = "[%nC curses you as %nh dies.]",
         output = "text",
         text = [[%nC collapses to the ground, cursing you and %l, then says:

    "You have defeated me, %r!  But I curse you one final time, with
    my dying breath!  You shall die before you leave my castle!"]],
      },
      leader_first = {
         synopsis = "[You have returned, %p.  We need your help.  Are you ready?]",
         output = "text",
         text = [["%pC!  You have returned!  Thank %d.

"We have great need of you.  But first, I must see if you have the
required abilities to take on this responsibility."]],
      },
      leader_last = {
         synopsis = "[You are not sufficiently %a.  We renounce your %shood.]",
         output = "text",
         text = [["%pC!  You have doomed us all.  You fairly radiate %L influences
and weaken the power we have raised in this grove as a result!

"Begone!  We renounce your %shood with us!  You are an outcast now!"]],
      },
      leader_next = {
         text = "\"Once again, %p, you stand in our midst.  Are you ready now?\"",
      },
      leader_other = {
         text = "\"Ah, you are here again, %p.  Allow me to determine your readiness...\"",
      },
      locate_first = {
         synopsis = "[This is %i.  There are bats nearby.  Beware the wumpus!]",
         output = "text",
         text = [[This must be %i.

You are in a cave built of many different rooms, all interconnected
by tunnels.  Your quest is to find and shoot the evil wumpus that
resides elsewhere in the cave without running into any bottomless
pits or using up your limited supply of arrows.  Good luck.

You are in room 9 of the cave.  There are tunnels to rooms
5, 8, and 10.
*rustle* *rustle* (must be bats nearby.)
*sniff* (I can smell the evil wumpus nearby!)]],
      },
      locate_next = {
         synopsis = "[You are in %i.  There are pits.  There are bats nearby.]",
         output = "text",
         text = [[Once again, you descend into %i.

*whoosh* (I feel a draft from some pits.)
*rustle* *rustle* (must be bats nearby.)]],
      },
      nemesis_first = {
         synopsis = "[You have come to recover %o, but I shall keep %oh and you shall die.]",
         output = "text",
         text = [["So, %c.  %lC has sent you to recover %o.

"Well, I shall keep that bauble.  It pleases me.  You, %c, shall die."]],
      },
      nemesis_next = {
         text = "\"Back again, eh?  Well, a mere %r is no threat to me!  Die, %c!\"",
      },
      nemesis_other = {
         text = "\"You haven't learned your lesson, %c.  You can't kill me!  You shall die now.\"",
      },
      nemesis_wantsit = {
         text = [["I shall have %o from you, %r.  Then I shall
kill you."]],
      },
      nexttime = {
         text = "Once again, you stand before %H.",
      },
      offeredit = {
         synopsis = "[You have succeeded.  Take %o with you as you go to find the Amulet.]",
         output = "text",
         text = [["%pC!  You have succeeded!  I feared it was not possible!

"You have returned with %o!

"I fear, now, that the Centaurs will regroup and plot yet another raid.
This will take some time, but if you can recover the Amulet of Yendor
for %d before that happens, we will be eternally safe.

"Take %o with you.  It will aid in your quest for
the Amulet."]],
      },
      offeredit2 = {
         synopsis = "[You are the keeper of %o now.  Go and find the Amulet.]",
         output = "text",
         text = [[%l flexes %o reverently.

"With this wondrous bow, one need never run out of arrows.
You are its keeper now, and the time has come to resume your
search for the Amulet.  %Z await your return
through the magic portal that brought you here."]],
      },
      othertime = {
         text = [[You have the oddest feeling that this may be the last time you
are to enter %H.]],
      },
      posthanks = {
         text = [["Welcome, %p.  How have you fared on your quest for the Amulet
of Yendor?"]],
      },
   },
   Rog = {
      assignquest = {
         synopsis = "[Get %o from %n and bring it to %l.]",
         output = "text",
         text = [["Will everyone not going to retrieve %o from that
jerk, %n, take one step backwards.  Good choice,
%p, because I was going to send you anyway.  My other %gp
are too valuable to me.

"Here's the deal.  I want %o, %n
has %o.  You are going to get %o
and bring it back to me.  So simple an assignment even you can understand
it."]],
      },
      badalign = {
         synopsis = "[Come back when you are really %a.]",
         output = "text",
         text = [["Maybe I should chain you to my perch here for a while.  Perhaps watching
real %a men at work will bring some sense back to you.  I don't
think I could stand the sight of you for that long though.  Come back
when you can be trusted to act properly."]],
      },
      badlevel = {
         synopsis = "[%rA is not adequately trained to handle this job.]",
         output = "text",
         text = [["In the time that you've been gone you've only been able to master the
arts of %ra?  I've trained ten times again as many %Rp
in that time.  Maybe I should send one of them, no?  Where would that
leave you, %p?  Oh yeah, I remember, I was going to kill you!"]],
      },
      discourage = {
         "\"May I suggest a compromise.  Are you interested in gold or gems?\"",
         "\"Please don't force me to kill you.\"",
         "\"Grim times are upon us all.  Will you not see reason?\"",
         "\"I knew %l, and you're no %lt, thankfully.\"",
         "\"It is a shame that we are not meeting under more pleasant circumstances.\"",
         "\"I was once like you are now, %p.  Believe in me -- our way is better.\"",
         "\"Stay with me, and I will make you %os guardian.\"",
         "\"When you return, with or without %o, %l will have you killed.\"",
         "\"Do not be fooled; I am prepared to kill to defend %o.\"",
         "\"I can reunite you with the Twain.  Oh, the stories you can swap.\"",
      },
      encourage = {
         "\"You don't seem to understand, %o isn't here so neither should you be!\"",
         "\"May %d curse you with lead fingers.  Get going!\"",
         "\"We don't have all year.  GET GOING!\"",
         "\"How would you like a scar necklace?  I'm just the jeweler to do it!\"",
         "\"Lazy S.O.B.  Maybe I should call up someone else...\"",
         "\"Maybe I should open your skull and see if my instructions are inside?\"",
         "\"This is not a task you can complete in the afterlife, you know.\"",
         "\"Inside every living person is a dead person trying to get out, and I have your key!\"",
         "\"We're almost out of hell-hound chow, so why don't you just get moving!\"",
         "\"You know, %o isn't going to come when you whistle.  You must get it yourself.\"",
      },
      firsttime = {
         synopsis = "[You are in Ransmannsby, where you trained.  Find %l.]",
         output = "text",
         text = [[Unexpectedly, you find yourself back in Ransmannsby, where you trained to
be a thief.  Quickly you make the guild sign, hoping that you AND word
of your arrival reach %ls den.]],
      },
      goal_first = {
         synopsis = "[You sense %o.]",
         output = "text",
         text = [[You feel a great swelling up of courage, sensing the presence of
%o.  Or is it fear?]],
      },
      goal_next = {
         text = "The hairs on the back of your neck whisper -- it's fear.",
      },
      gotit = {
         synopsis = "[You pick up %o and know that %l should not have it.]",
         output = "text",
         text = [[As you pick up %o, the hairs on the back of your
neck fall out.  At once you realize why %n was
willing to die to keep it out of %ls hands.  Somehow
you know that you must do likewise.]],
      },
      guardtalk_after = {
         "\"I was sure wrong about Lady Tyvefelle's house; I barely got away with my life and lost my lock pick in the process.\"",
         "\"You're back?  Even the Twain don't come back anymore.\"",
         "\"Can you spare an old cutpurse a zorkmid for some grog?\"",
         "\"Fritz tried to join the other side, and now he's hell-hound chow.\"",
         "\"Be careful what you steal, I hear the boss has perfected turning rocks into worthless pieces of glass.\"",
      },
      guardtalk_before = {
         "\"I hear that Lady Tyvefelle's household is lightly guarded.\"",
         "\"You're back?  Even the Twain don't come back anymore.\"",
         "\"Can you spare an old cutpurse a zorkmid for some grog?\"",
         "\"Fritz tried to join the other side, and now he's hell-hound chow.\"",
         "\"Be careful what you steal, I hear the boss has perfected turning rocks into worthless pieces of glass.\"",
      },
      hasamulet = {
         synopsis = "[Take the Amulet to the Astral Plane and find %ds temple.]",
         output = "text",
         text = [["I see that with your abilities, and my brains, we could rule this world.

"All that we would need to be all-powerful is for you to take that little
trinket you've got there up to the Astral Plane.  From there, %d will
show you what to do with it.  Once that's done, we will be invincible!"]],
      },
      killed_nemesis = {
         synopsis = "[Before dying, %n tells you to use the %o wisely.]",
         output = "text",
         text = [["I know what you are thinking, %p.  It is not too late for you
to use %o wisely.  For the sake of your guild
%sp, do what is right."

You sit and wait for death to come for %n, and then you
brace yourself for your next meeting with %l!]],
      },
      leader_first = {
         synopsis = "[You owe back dues to your guild.  You can pay them off if you're up to the job.]",
         output = "text",
         text = [["Well, look who it is boys -- %p has come home.  You seem to have
fallen behind in your dues.  I should kill you as an example to these
other worthless cutpurses, but I have a better plan.  If you are ready
maybe you could work off your back dues by performing a little job for
me.  Let us just see if you are ready..."]],
      },
      leader_last = {
         synopsis = "[You must go.]",
         output = "text",
         text = [["Well %gp, it looks like our friend has forgotten who is the boss
around here.  Our friend seems to think that %rp have been put in
charge.  Wrong.  DEAD WRONG!"

Your sudden shift in surroundings prevents you from hearing the end
of %ls curse.]],
      },
      leader_next = {
         synopsis = "[Are you stupid or are you ready?]",
         output = "text",
         text = [["Well, I didn't expect to see you back.  It shows that you are either stupid,
or you are finally ready to accept my offer.  Let us hope for your sake it
isn't stupidity that brings you back."]],
      },
      leader_other = {
         text = [["Did you perhaps mistake me for some other %lt?  You must
think me as stupid as your behavior.  I warn you not to try my patience."]],
      },
      locate_first = {
         text = "Those damn little hairs tell you that you are nearer to %o.",
      },
      locate_next = {
         text = "Not wanting to face %l without having stolen %o, you continue.",
      },
      nemesis_first = {
         text = "\"Ah!  You must be %ls ... er, `hero'.  A pleasure to meet you.\"",
      },
      nemesis_next = {
         text = "\"We meet again.  Please reconsider your actions.\"",
      },
      nemesis_other = {
         synopsis = "[You cannot trust %l.]",
         output = "text",
         text = [["Surely, %p, you have learned that you cannot trust any bargains
that %l has made.  I can show you how to continue on
your quest without having to run into him again."]],
      },
      nemesis_wantsit = {
         synopsis = "[%lC should not have %o.]",
         output = "text",
         text = [["Please, think for a moment about what you are doing.  Do you truly
believe that %d would want %l to have
%o?"]],
      },
      nexttime = {
         text = [[Once again, you find yourself back in Ransmannsby.  Fond memories are
replaced by fear, knowing that %l is waiting for you.]],
      },
      offeredit = {
         synopsis = "[Take %o with you and go.]",
         output = "text",
         text = [["Well, I'll be damned.  You got it.  I am proud of you, a fine %r
you've turned out to be.

"While you were gone I got to thinking, you and %o
together could bring me more treasure than either of you apart, so why don't
you take it with you.  All I ask is a cut of whatever loot you come by.
That is a better deal than I offered %n.

"But, you see what happened to %n when he refused.
Don't make me find another to send after you this time."]],
      },
      offeredit2 = {
         synopsis = "[Take %o and acquire the Amulet.]",
         output = "text",
         text = [[%lC seems tempted to swap %o for
the mundane one you detect in his pocket, but noticing your alertness,
evidently chickens out.

"Go filch the Amulet before someone else beats you to it.
%Z are back the way you came, through the magic portal."]],
      },
      othertime = {
         text = [[You rub your hands through your hair, hoping that the little ones on
the back of your neck stay down, and prepare yourself for your meeting
with %l.]],
      },
      posthanks = {
         synopsis = "[How about trading %o for something?]",
         output = "text",
         text = [["Quite the little thief, aren't we, %p.  Can I interest you in a
swap for %o?  Look around, anything in the keep
is yours for the asking."]],
      },
   },
   Sam = {
      assignquest = {
         synopsis = "[You must enter %i, then regain %o from %n.]",
         output = "text",
         text = [["Domo %p-san, indeed you are ready.  I can now tell you what
it is that I require of you.

"The daimyo, %n, has betrayed us.  He has stolen from us
%o and taken it to his donjon deep within
%i.

"If I cannot show the emperor %o when he comes
for the festival he will know that I have failed in my duty, and
request that I commit seppuku.

"You must gain entrance to %i and retrieve the
emperor's property.  Be quick!  The emperor will be here for the
cha-no-you in 5 sticks.

"Wakarimasu ka?"]],
      },
      badalign = {
         synopsis = "[When you can think %a and act %a then return.]",
         output = "text",
         text = [["%p-san, you would do better to join the kyokaku.

"You have skills, but until you can call upon the bushido to know when and
how to use them you are not samurai.  When you can think %a and
act %a then return."]],
      },
      badlevel = {
         synopsis = "[\"I require %Ra to defeat %n.  Return when you are ready.\"]",
         output = "text",
         text = [["%p-san, you have learned well and honored your family.
I require the skills of %Ra in order to defeat %n.
Go and seek out teachers.  Learn what they have learned.  When you
are ready, return to me."]],
      },
      discourage = {
         "\"Ahh, I finally meet the daimyo of the kyokaku!\"",
         "\"There is no honor for me in your death.\"",
         "\"You know that I cannot resash my swords until they have killed.\"",
         "\"Your presence only compounds the dishonor of %l in not coming %liself.\"",
         "\"I will make tea with your hair and serve it to %l.\"",
         "\"Your fear shows in your eyes, coward!\"",
         "\"I have not heard of you, %p-san; has your life been that unworthy?\"",
         "\"If you will not obey me, you will die.\"",
         "\"Kneel now and make the two cuts of honor.  I will tell your %sp of your honorable death.\"",
         "\"Your master was a poor teacher.  You will pay for his mistakes in your teaching.\"",
      },
      encourage = {
         "\"To defeat %n you must overcome the seven emotions: hate, adoration, joy, anxiety, anger, grief, and fear.\"",
         "\"Remember your honor is my honor, you perform in my name.\"",
         "\"I will go to the temple and burn incense for your safe return.\"",
         "\"Sayonara.\"",
         "\"There can be honor in defeat, but no gain.\"",
         "\"Your kami must be strong in order to succeed.\"",
         "\"You are indeed a worthy %R, but now you must be a worthy samurai.\"",
         "\"If you fail, %n will be like a tai-fun on the land.\"",
         "\"If you are truly %a, %d will listen.\"",
         "\"Sharpen your swords and your wits for the task before you.\"",
      },
      firsttime = {
         synopsis = "[The banner of %n flies above town.  What has happened to %l?]",
         output = "text",
         text = [[Even before your senses adjust, you recognize the kami of
%H.

You %x the standard of your teki, %n, flying above
the town.  How could such a thing have happened?  Why are ninja
wandering freely; where are the samurai of your daimyo, %l?

You quickly say a prayer to Izanagi and Izanami and walk towards
town.]],
      },
      goal_alt = {
         text = "As you arrive once again at the home of %n.",
      },
      goal_first = {
         synopsis = "[You feel the taunts %n, but after offering a prayer to %d, you proceed.]",
         output = "text",
         text = [[In your mind, you hear the taunts of %n.

You become like the rice plant and bend to the ground, offering a
prayer to %d.  But when the wind has passed, you stand
proudly again.  Putting your kami in the hands of fate, you advance.]],
      },
      goal_next = {
         text = [[As you arrive once again at the home of %n, your thoughts
turn only to %o.]],
      },
      gotit = {
         synopsis = "[You feel the power of %o and are humbled.]",
         output = "text",
         text = [[As you pick up %o, you feel the strength of its karma.
You realize at once why so many good samurai had to die to defend it.
You are humbled knowing that you hold one of the artifacts of the
sun goddess.]],
      },
      guardtalk_after = {
         "\"Come, join us in celebrating with some sake.\"",
         "\"Ikaga desu ka?\"",
         "\"You have brought our clan and %l much honor.\"",
         "\"Please %r, sit for a while and tell us how you overcame the Ninja.\"",
         "\"%lC still lives!  You have saved us from becoming ronin.\"",
      },
      guardtalk_before = {
         "\"To succeed, you must walk like a butterfly on the wind.\"",
         "\"Ikaga desu ka?\"",
         "\"I fear for The Land of The Gods.\"",
         "\"%nC has hired the Ninja -- be careful.\"",
         "\"If %o is not returned, we will all be ronin.\"",
      },
      hasamulet = {
         synopsis = "[Take the Amulet to the Astral Plane to finish your task.]",
         output = "text",
         text = [["Ah, %p-sama.  You have wasted your efforts returning home.
Now that you are in possession of the Amulet, you are honor-bound to
finish the quest you have undertaken.  There will be plenty of time
for saki and stories when you have finished.

"Go now, and may our prayers be a wind at your back."]],
      },
      killed_nemesis = {
         synopsis = "[%nC dies without honor.]",
         output = "text",
         text = [[Your healing skills tell you that %ns wounds are mortal.

You know that the bushido tells you to finish him and let his kami
die with honor, but the thought of so many samurai dead due to this
man's dishonor prevents you from giving the final blow.

You order that his unwashed head be given to the crows and his body
thrown into the sea.]],
      },
      leader_first = {
         synopsis = "[%lC needs someone to lead %lj samurai against %n.  Are you ready?]",
         output = "text",
         text = [["Ah, %p-san, it is good to see you again.  I need someone who can
lead my samurai against %n.  If you are ready, you will be
that person."]],
      },
      leader_last = {
         synopsis = "[Leave and do not come back.]",
         output = "text",
         text = [["You are no longer my samurai, %p.

"Hara-kiri is denied.  You are ordered to shave your head and then to
become a monk.  Your fief and family are forfeit.  Wakarimasu ka?"]],
      },
      leader_next = {
         text = [["Once again, %p-san, you kneel before me.  Are you yet capable of
being my vassal?"]],
      },
      leader_other = {
         synopsis = "[Are you truly a samurai?]",
         output = "text",
         text = [["You begin to test my matsu, %p-san.
If you cannot determine what I want in a samurai, how can I rely on you
to figure out what I need from a samurai?"]],
      },
      locate_first = {
         text = [[You instinctively reach for your swords.  You do not recognize the
lay of this land, but you know that your teki are everywhere.]],
      },
      locate_next = {
         text = [[Thankful that your %sp at %H cannot see
your fear, you prepare again to advance.]],
      },
      nemesis_first = {
         text = [["Ah, so it is to be you, %p-san.  I offer you seppuku.
I will be your second if you wish."]],
      },
      nemesis_next = {
         text = [["I have offered you the honorable exit.  Now I will have your
head to send unwashed to %l."]],
      },
      nemesis_other = {
         text = "\"After I have dispatched you, I will curse your kami.\"",
      },
      nemesis_wantsit = {
         text = [["You have fought my samurai; surely you must know that you
will not be able to take %o back to
%H."]],
      },
      nexttime = {
         text = "Once again, you are back at %H.",
      },
      offeredit = {
         synopsis = "[The emperor wants you to take %o and recover the Amulet.]",
         output = "text",
         text = [[As you bow before %l, he welcomes you:

    "You have brought your family great honor, %p-sama.

    "While you have been gone the emperor's advisors have discovered in
    the ancient texts that the karma of the samurai who seeks to recover
    the Amulet and the karma of %o are joined
    as the seasons join to make a year.

    "Because you have shown such fidelity, the emperor requests
    that you take leave of other obligations and continue on the
    road that fate has set your feet upon.  I would consider it
    an honor if you would allow me to watch your household until
    you return with the Amulet."

With that, %l bows, and places his sword atop
%o.]],
      },
      offeredit2 = {
         synopsis = "[Take %o, return to %Z, and recover the Amulet.]",
         output = "text",
         text = [[%l holds %o tightly for a moment, then returns
his gaze to you.

"The time is ripe to recover the Amulet.  Return to %Z
through the magic portal that transported you here so that you may
achieve the destiny which awaits you."]],
      },
      othertime = {
         synopsis = "[%HC is threatened by %n.]",
         output = "text",
         text = [[You are back at %H.

Instantly you sense a subtle change in your karma.  You seem to know that
if you do not succeed in your quest, %n will have destroyed
the kami of %H before you return again.]],
      },
      posthanks = {
         text = "%lC bows.  \"%p-sama, tell us of your search for the Amulet.\"",
      },
   },
   Tou = {
      assignquest = {
         synopsis = "[Enter %i and recover %o from %n.]",
         output = "text",
         text = [["You have indeed proven yourself a worthy %c, %p.

"But now your kinfolk and I must ask you to put aside your travels and
help us in our time of need.  After you left us we elected a new mayor,
%n.  He proved to be a most heinous and vile creature.

"Soon after taking office he absconded with %o
and fled town, leaving behind his henchmen to rule over us.  In order
for us to regain control of our town, you must enter %i
and recover %o.

"Do not be distracted on your quest.  If you do not return quickly I fear
that all will be lost.  Let us both pray now that %d will guide you
and keep you safe."]],
      },
      badalign = {
         synopsis = "[You are not sufficiently %a.  Return when you are.]",
         output = "text",
         text = [["It would be an affront to %d to have one not true to the
%a path undertake her bidding.

"You must not return to us until you have purified yourself of these
bad influences on your actions.  Remember, only by following the %a
path can you hope to overcome the obstacles you will face."]],
      },
      badlevel = {
         synopsis = "[Return when you are %Ra.]",
         output = "text",
         text = [["There is still too much that you have to learn before you can undertake
the next step.  Return to us as a proven %R, and perhaps then
you will be ready.

"Go back now, and may the teachings of %d serve you well."]],
      },
      discourage = {
         "\"I defeated %l and I will defeat you, %p.\"",
         "\"Where is %d now!  You must realize no one can help you here.\"",
         "\"Beg for mercy now and I may be lenient on you.\"",
         "\"If you were not so %a, you might have stood a chance.\"",
         "\"Vengeance is mine at last, %p.\"",
         "\"I only wish that %l had a more worthy %r to send against me.\"",
         "\"With %o in my possession you cannot hope to defeat me.\"",
         "\"%nC has never been defeated, NEVER!\"",
         "\"Are you truly the best %H has to send against me?  I pity %l.\"",
         "\"How do you spell %p?  I want to ensure the marker on your grave is correct as a warning to your %sp.\"",
      },
      encourage = {
         "\"Do not be fooled by the false promises of %n.\"",
         "\"To enter %i you must pass many traps.\"",
         "\"If you do not return with %o, your quest will be in vain.\"",
         "\"Do not be afraid to call upon %d if you truly need help.\"",
         "\"If you do not destroy %n, he will follow you back here!\"",
         "\"Take %o from %n and you may be able to defeat him.\"",
         "\"You must hurry, %p!\"",
         "\"You are like %Sa to me, %p.  Do not let me down.\"",
         "\"If you are %a at all times you may succeed, %p.\"",
         "\"Let all who meet you on your journey know that you are on a quest for %l and grant safe passage.\"",
      },
      firsttime = {
         synopsis = "[You find yourself back at %H, but the quiet is ominous.]",
         output = "text",
         text = [[You breathe a sigh of relief as you find yourself back in the familiar
surroundings of %H.

You quickly notice that things do not appear the way they did when you
left.  The town is dark and quiet.  There are no sounds coming from
behind the town walls, and no campfires burning in the fields.  As a
matter of fact, you do not %x any movement in the fields at all, and
the crops seem as though they have been untended for many weeks.]],
      },
      goal_alt = {
         text = "You have returned to %ns lair.",
      },
      goal_first = {
         text = "You sense the presence of %o.",
      },
      goal_next = {
         text = [[You gain confidence, knowing that you may soon be united with
%o.]],
      },
      gotit = {
         synopsis = "[You pick up %o and feel relief.  Return it to %l.]",
         output = "text",
         text = [[As you pick up %o, you feel a great
weight has been lifted from your shoulders.  Your only thoughts are
to quickly return to %H and find %l.]],
      },
      guardtalk_after = {
         "\"Gehennom on 5 zorkmids a day -- more like 500 a day if you ask me.\"",
         "\"Do you know where I could find some nice postcards of The Gnomish Mines?\"",
         "\"Have you tried the weird toilets?\"",
         "\"If you stick around, I'll show you the pictures from my latest trip.\"",
         "\"Did you bring me back any souvenirs?\"",
      },
      guardtalk_before = {
         "\"Gehennom on 5 zorkmids a day -- more like 500 a day if you ask me.\"",
         "\"Do you know where I could find some nice postcards of The Gnomish Mines?\"",
         "\"Have you tried the weird toilets?\"",
         "\"Don't stay at the Inn, I hear the food is terrible and it has rats.\"",
         "\"They told me that this was the off season!\"",
      },
      hasamulet = {
         synopsis = "[You have the Amulet.  Take it to the Astral Plane to finish your task.]",
         output = "text",
         text = [["Stand back and let me look at you, %p.
Now that you have recovered the Amulet of Yendor, I'm afraid living
out your days in %H would seem pretty tame.

"You have come too far to stop now, for there are still more tasks that
our oral history foretells for you.  Forever more, though, your name shall
be spoken by the %gP with awe.  You are truly an inspiration to your
%sp!"]],
      },
      killed_nemesis = {
         synopsis = "[%nC curses at you as %nh dies.]",
         output = "text",
         text = [[You turn in the direction of %n.  As his earthly body begins
to vanish before your eyes, you hear him curse:

    "You shall never be rid of me, %p!
    I will find you where ever you go and regain what is rightly mine."]],
      },
      leader_first = {
         synopsis = "[Someone must defeat %n.  Are your ready?]",
         output = "text",
         text = [["Is it really you, %p!  I had given up hope for your return.
As you can %x, we are desperately in need of your talents.  Someone must
defeat %n if our town is to become what it once was.

"Let me see if you are ready to be that someone."]],
      },
      leader_last = {
         synopsis = "[Leave %H and never return.]",
         output = "text",
         text = [["It is too late, %p.  You are not even worthy to die amongst us.
Leave %H and never return."]],
      },
      leader_next = {
         text = "\"Things are getting worse, %p.  I hope that this time you are ready.\"",
      },
      leader_other = {
         text = "\"I hope that for the sake of %H you have prepared yourself this time.\"",
      },
      locate_first = {
         synopsis = "[You %x the handiwork of %ns henchlings.]",
         output = "text",
         text = [[Only your faith in %d keeps you from trembling.  You %x
the handiwork of %ns henchlings everywhere.]],
      },
      locate_next = {
         text = "You know that this time you must find and destroy %n.",
      },
      nemesis_first = {
         synopsis = "[%rA will not defeat me.]",
         output = "text",
         text = [["So, %p, %l thinks that you can wrest
%o from me!

"It only proves how desperate he has become that he sends %ra to
try and defeat me.  When this day is over, I will have you enslaved
in the mines where you will rue the day that you ever entered
%i."]],
      },
      nemesis_next = {
         text = [["I let you live the last time because it gave me pleasure.
This time I will destroy you, %p."]],
      },
      nemesis_other = {
         synopsis = "[Run away or you will suffer severely.]",
         output = "text",
         text = [["These meetings come to bore me.  You disturb my workings with
%o.

"If you do not run away now, I will inflict so much suffering on you that
%l will feel guilty for ever having sent his %S to me!"]],
      },
      nemesis_wantsit = {
         synopsis = "[\"Return %o to me and we will rule %H.\"]",
         output = "text",
         text = [["You fool.  You do not know how to call upon the powers of
%o.

"Return it to me and I will teach you how to use it, and together we
will rule %H.  But do so now, as my patience grows thin."]],
      },
      nexttime = {
         text = "Once again, you are back at %H.",
      },
      offeredit = {
         synopsis = "[Take %o and with %ds guidance, recover the Amulet.]",
         output = "text",
         text = [[As %l detects the presence of %o,
he almost smiles for the first time in many a full moon.

As he looks up from %o he says:

    "You have recovered %o.  You are its
    owner now, but not its master.  Let it work with you as you continue
    your journey.  With its help, and %d to guide you on the
    %a path, you may yet recover the Amulet of Yendor."]],
      },
      offeredit2 = {
         synopsis = "[Keep %o and return to %Z through the portal.]",
         output = "text",
         text = [["%oC is yours now.  %Z
await your return through the magic portal that brought you here."]],
      },
      othertime = {
         text = [[You are back at %H.
Things appear to have become so bad that you fear that soon
%H will not be here to return to.]],
      },
      posthanks = {
         text = [["I could not be more proud than if you were my own %S, %p!
Tell me of your adventures in quest of the Amulet of Yendor."]],
      },
   },
   Val = {
      assignquest = {
         synopsis = "[Find %i; defeat %n; return with %o.]",
         output = "text",
         text = [["It is not clear, %p, for my sight is limited without our relic.
But it is now likely that you can defeat %n, and recover
%o.

"A short time ago, %n and his minions attacked this place.  They
opened the huge volcanic vents you %x about the hill, and attacked.  I knew
that this was to come to pass, and had asked %d for a group of %gP
to help defend this place.  The few you %x here are the mightiest of
Valhalla's own, and are all that are left of one hundred %d sent.

"Despite the great and glorious battle we fought, %n managed at
last to steal %o.  This has upset the balance of the universe,
and unless %oh is returned into my care, %n may start Ragnarok.

"You must find the entrance to %i.  Travel downward
from there and you will find %ns lair.  Defeat him and
return %o to me."]],
      },
      badalign = {
         synopsis = "[You have strayed from the %a path.  Return after you purify yourself.]",
         output = "text",
         text = [["NO!  This is terrible.  I see you becoming an ally of %n, and
leading his armies in the final great battles.  This must not come to
pass!  You have strayed from the %a path.  You must purge yourself,
and return here only when you have regained a state of purity."]],
      },
      badlevel = {
         synopsis = "[Come back when you are %Ra.]",
         output = "text",
         text = [["I see you and %n fighting, %p.  But you are not prepared and
shall die at %ns hand if you proceed.  No.  This will not do.
Go back out into the world, and grow more experienced at the ways of war.
Only when you have returned %Ra will you be able to defeat %n."]],
      },
      discourage = {
         "\"I am your death, %c.\"",
         "\"You cannot prevail, %r.  I have foreseen your every move.\"",
         "\"With you out of the way, Valhalla will be mine for the taking.\"",
         "\"I killed scores of %ds best when I took %o. Do you really think that one %c can stand against me?\"",
         "\"Who bears the souls of %cP to Valhalla, %r?\"",
         "\"No, %d cannot help you here.\"",
         "\"Some instrument of %d you are, %p.  You are a weakling!\"",
         "\"Never have I seen %ca so clumsy in battle.\"",
         "\"You die now, little %s.\"",
         "\"Your body I destroy now, your soul when my hordes overrun Valhalla!\"",
      },
      encourage = {
         "\"Go with the blessings of %d.\"",
         "\"Call upon %d when you are in need.\"",
         "\"Use %o if you can.  It will protect you.\"",
         "\"Magical cold is very effective against %n.\"",
         "\"To face %n, you will need to be immune to fire.\"",
         "\"May %d strengthen your sword-arm.\"",
         "\"Trust in %d.  He will not desert you.\"",
         "\"It becomes more likely that Ragnarok will come with every passing moment. You must hurry, %p.\"",
         "\"If %n can master %o, he will be powerful enough to face %d far earlier than is fated.  This must not be!\"",
         "\"Remember your training, %p.  You can succeed.\"",
      },
      firsttime = {
         synopsis = "[You arrive below %H.  Something is wrong; there is lava present.]",
         output = "text",
         text = [[You materialize at the base of a snowy hill.  Atop the hill sits
a place you know well, %H.  You immediately realize
that something here is very wrong!

In places, the snow and ice have been melted into steaming pools of
water.  Fumaroles and pools of bubbling lava surround the hill.
The stench of sulphur is carried through the air, and you %x creatures
that should not be able to live in this environment moving towards you.]],
      },
      goal_first = {
         synopsis = "[This is the lair of %n.]",
         output = "text",
         text = [[Through clouds of sulphurous gasses, you %x a rock palisade
surrounded with a moat of bubbling lava.  You remember the description
from something that %l said.  This is the lair of %n.]],
      },
      goal_next = {
         text = "Once again, you stand in sight of %ns lair.",
      },
      gotit = {
         synopsis = "[You must return %o to %l.]",
         output = "text",
         text = [[As you pick up %o, your mind is suddenly filled with images,
and you perceive all of the possibilities of each potential choice you
could make.  As you begin to control and channel your thoughts, you
realize that you must return %o to %l immediately.]],
      },
      guardtalk_after = {
         "\"Hail, and well met, brave %c.\"",
         "\"May %d guide your steps, %p.\"",
         "\"%lC told us you had succeeded!\"",
         "\"You recovered %o just in time, %p.\"",
         "\"Hail %d, for delivering %o back to us.\"",
      },
      guardtalk_before = {
         "\"Hail, and well met, brave %c.\"",
         "\"May %d guide your steps, %p.\"",
         "\"%lC weakens.  Without %o, her foresight is dim.\"",
         "\"You must hurry, %p, else Ragnarok may well come.\"",
         "\"I would deal with this foul %n myself, but %d forbids it.\"",
      },
      hasamulet = {
         synopsis = "[Take the Amulet to %ds temple on the Astral Plane and offer it.]",
         output = "text",
         text = [["Excellent, %p.  I see you have recovered the Amulet!

"You must take the Amulet to the Great Temple of %d, on the Astral
Plane.  There you must offer the Amulet to %d.

"Go now, my %S.  I cannot tell you your fate, as the power of the
Amulet interferes with mine.  I hope for your success."]],
      },
      killed_nemesis = {
         synopsis = "[%nC dies.]",
         output = "text",
         text = [[A look of surprise and horror appears on %ns face.

    "No!!!  %o has lied to me!  I have been misled!"

Suddenly, %n grasps his head and screams in agony, then dies.]],
      },
      leader_first = {
         synopsis = "[We need your aid.  Are you ready?]",
         output = "text",
         text = [["Ah, %p, my %S.  You have returned to %H
at last.  We are in dire need of your aid, but I must determine if you
are yet ready for such an undertaking.

"Let me read your fate..."]],
      },
      leader_last = {
         synopsis = "[\"Begone from my presence and never return.\"]",
         output = "text",
         text = [["No, %p.  Your fate is sealed.  I must cast about for another
champion.  Begone from my presence, and never return.  Know this, that
you shall never succeed in this life, and Valhalla is denied to you."]],
      },
      leader_next = {
         text = [["Let me read the future for you now, %p, perhaps you have managed to
change it enough..."]],
      },
      leader_other = {
         text = [["Again, I shall read your fate, my %S.  Let us both hope that you have
made changes to become ready for this task..."]],
      },
      locate_first = {
         synopsis = "[This is the entrance to %i.]",
         output = "text",
         text = [[The ice and snow gives way to a valley floor.  You %x ahead of you
a huge round hill surrounded by pools of lava.  This then is the entrance
to %i.  It looks like you're not going to get in without
a fight though.]],
      },
      locate_next = {
         text = "Once again, you stand before the entrance to %i.",
      },
      nemesis_first = {
         synopsis = "[\"%oC has shown me that I must kill you.\"]",
         output = "text",
         text = [["So!  %lC has finally sent %ca to challenge me!

"I thought that mastering %o would enable me to challenge
%d, but it has shown me that first I must kill you!  So come, little
%s.  Once I defeat you, I can at last begin the final battle with %d."]],
      },
      nemesis_next = {
         text = "\"Again you challenge me, %r.  Good.  I will kill you now.\"",
      },
      nemesis_other = {
         text = "\"Have you not learned yet?  You cannot defeat %n!\"",
      },
      nemesis_wantsit = {
         text = "\"I will kill you, %c, and wrest %o from your mangled hands.\"",
      },
      nexttime = {
         text = "Once again, you are near the abode of %l.",
      },
      offeredit = {
         synopsis = "[Take %o.  Search for the Amulet.]",
         output = "text",
         text = [[As you approach, %l rises and touches %o.

"You may take %o with you, %p.  I have removed from
it the power to foretell the future, for that power no mortal should
have.  Its other abilities, however, you have at your disposal.

"You must now begin in %ds name to search for the Amulet of Yendor.
May your steps be guided by %d, my %S."]],
      },
      offeredit2 = {
         synopsis = "[You are %os keeper now.  Return through the portal and find the Amulet.]",
         output = "text",
         text = [["Careful, %p!  %oC might break, and that would be
a tragic loss.  You are its keeper now, and the time has come to
resume your search for the Amulet.  %Z await your
return through the magic portal that brought you here."]],
      },
      othertime = {
         text = [[Again you materialize near %ls abode.  You have a nagging feeling
that this may be the last time you come here.]],
      },
      posthanks = {
         text = [["Greetings, %p.  I have not been able to pay as much attention to
your search for the Amulet as I have wished.  How do you fare?"]],
      },
   },
   Wiz = {
      assignquest = {
         synopsis = "[Travel to %i; overcome %n; return with %o.]",
         output = "text",
         text = [["Yes, %p, you truly are ready for this dire task.  Listen,
carefully, for what I tell you now will be of vital importance.

"Since you left us to hone your skills in the world, we unexpectedly came
under attack by the forces of %n.  As you know, we thought
%n had perished at the end of the last age, but, alas, this was
not the case.

"%nC sent an army of abominations against us.  Among them was a
minion, mindless and ensorcelled, and thus, in the confusion, it was
able to penetrate our defenses.  Alas, this creature has stolen
%o and I fear has delivered %oh to %n.

"Over the years, I had woven most of my power into this amulet, and thus,
without it, I have but a shadow of my former power, and I fear that I
shall soon perish.

"You must travel to %i, and within its dungeons,
find and overcome %n, and return %o to me.

"Go now, with %d, and complete this quest before it is too late."]],
      },
      badalign = {
         synopsis = "[Go; come back when you are worthy of %d.]",
         output = "text",
         text = [["You amaze me, %p!  How many times did I tell you that the way of a mage
is an exacting one.  One must use the world with care, lest one leave it
in ruins and simplify the task of %n.

"You must go back and show your worthiness.  Do not return until you are
truly ready for this quest.  May %d guide you in this task."]],
      },
      badlevel = {
         synopsis = "[Go; return when you are %Ra.]",
         output = "text",
         text = [["Alas, %p, you have not yet shown your proficiency as a worthy
spellcaster.  As %ra, you would surely be overcome in the challenge
ahead.  Go, now, expand your horizons, and return when you have attained
renown as %Ra."]],
      },
      discourage = {
         "\"Your puny powers are no match for me, fool!\"",
         "\"When you are defeated, your torment will last for a thousand years.\"",
         "\"After your downfall, %p, I shall devour %l for dessert!\"",
         "\"Are you ready yet to beg for mercy?  I could be lenient...\"",
         "\"Your soul shall join the enslaved multitude I command!\"",
         "\"Your lack of will is evident, and you shall die as a result.\"",
         "\"Your faith in %d is for naught!  Come, submit to me now!\"",
         "\"A mere %r is nothing compared to my skill!\"",
         "\"So, you are the best hope of %l?  How droll.\"",
         "\"Feel my power, %c!  My victory is imminent!\"",
      },
      encourage = {
         "\"Beware, for %n is immune to most magical attacks.\"",
         "\"To enter %i you must pass many traps.\"",
         "\"%nC may be vulnerable to physical attacks.\"",
         "\"%d will come to your aid when you call.\"",
         "\"You must utterly destroy %n.  He will pursue you otherwise.\"",
         "\"%oC is a mighty artifact.  With it you can destroy %n.\"",
         "\"Go forth with the blessings of %d.\"",
         "\"I will have my %gP watch for your return.\"",
         "\"Feel free to take any items in that chest that might aid you.\"",
         "\"You will know when %o is near.  Proceed with care!\"",
      },
      firsttime = {
         synopsis = "[You have arrived at %ls tower but something is very wrong.]",
         output = "text",
         text = [[You are suddenly in familiar surroundings.  You notice what appears to
be a large, squat stone structure nearby.  Wait!  That looks like the
tower of your former teacher, %l.

However, things are not the same as when you were last here.  Mists and
areas of unexplained darkness surround the tower.  There is movement in
the shadows.

Your teacher would never allow such unaesthetic forms to surround the
tower...  unless something were dreadfully wrong!]],
      },
      goal_alt = {
         text = "You have returned to %ns lair.",
      },
      goal_first = {
         text = "You feel your mentor's presence; perhaps %o is nearby.",
      },
      goal_next = {
         text = "The aura of %o tingles at the edge of your perception.",
      },
      gotit = {
         synopsis = "[You feel %os power and know you should return %oh to %l.]",
         output = "text",
         text = [[As you touch %o, its comforting power infuses you
with new energy.  You feel as if you can detect others' thoughts flowing
through it.  Although you yearn to wear %o and
attack the Wizard of Yendor, you know you must return it to its rightful
owner, %l.]],
      },
      guardtalk_after = {
         "\"I have some eye of newt to trade, do you have a spare blind-worm's sting?\"",
         "\"The magic portal now seems like it will remain stable for quite some time.\"",
         "\"Have you noticed how much stronger %l is since %o was recovered?\"",
         "\"Thank %d!  We weren't positive you would defeat %n.\"",
         "\"I, too, will venture into the world, because %n was but one of many evils to be vanquished.\"",
      },
      guardtalk_before = {
         "\"Would you happen to have some eye of newt in that overstuffed pack, %s?\"",
         "\"Ah, the spell to create the magic portal worked.  Outstanding!\"",
         "\"Hurry!  %lC may not survive that casting of the portal spell!\"",
         "\"The spells of %n were just too powerful for us to withstand.\"",
         "\"I, too, will venture into the world, because %n is but one of many evils to be vanquished.\"",
      },
      hasamulet = {
         synopsis = "[Take the Amulet to %ds altar on the Astral Plane.]",
         output = "text",
         text = [["Congratulations, %p.  I always knew that if anyone could succeed
in defeating the Wizard of Yendor and his minions, it would be you.

"Go now, and take the Amulet to the Astral Plane.  Once there, present
the Amulet on the altar of %d.  Along the way you shall pass through
the four Elemental Planes.  These planes are like nothing you have ever
experienced before, so be prepared!

"For this you were born, %s!  I am very proud of you."]],
      },
      killed_nemesis = {
         synopsis = "[%nC curses you as %nh dies.]",
         output = "text",
         text = [[%nC, whose body begins to shrivel up, croaks out:

    "I shall haunt your progress until the end of time.  A thousand
    curses on you and %l."

Then, the body bursts into a cloud of choking dust, and blows away.]],
      },
      leader_first = {
         synopsis = "[You have come a long way, but are you ready for the task I require?]",
         output = "text",
         text = [["Come closer, %p, for my voice falters in my old age.
Yes, I see that you have come a long way since you went out into the
world, leaving the safe confines of this tower.  However, I must first
determine if you have all of the skills required to take on the task
I require of you."]],
      },
      leader_last = {
         synopsis = "[\"Get out of here!\"]",
         output = "text",
         text = [["You fool, %p!  Why did I waste all of those years teaching you
the esoteric arts?  Get out of here!  I shall find another."]],
      },
      leader_next = {
         text = "\"Well, %p, you have returned.  Perhaps you are now ready...\"",
      },
      leader_other = {
         text = [["This is getting tedious, %p, but perseverance is a sign of a true mage.
I certainly hope that you are truly ready this time!"]],
      },
      locate_first = {
         text = "Wisps of fog swirl nearby.  You feel that %ns lair is close.",
      },
      locate_next = {
         text = "You believe that you may once again invade %i.",
      },
      nemesis_first = {
         synopsis = "[\"Your destruction should make for good sport.\"]",
         output = "text",
         text = [["Ah, I recognize you, %p.  So, %l has sent you to steal
%o from me, hmmm?  Well, %lh is a fool to send such
a mental weakling against me.

"Your destruction, however, should make for good sport.  In the end, you
shall beg me to kill you!"]],
      },
      nemesis_next = {
         synopsis = "[\"Your soul shall soon be mine to command.\"]",
         output = "text",
         text = [["How nice of you to return, %p!  I enjoyed our last meeting.  Are you
still hungry for more pain?

"Come!  Your soul, like %o, shall soon be mine to command."]],
      },
      nemesis_other = {
         text = [["I'm sure that your perseverance shall be the subject of innumerable
ballads, but you shall not be around to hear them, I fear!"]],
      },
      nemesis_wantsit = {
         text = [["Thief!  %oC belongs to me, now.  I shall feed
your living flesh to my minions."]],
      },
      nexttime = {
         text = "Once again, you are back at %H.",
      },
      offeredit = {
         synopsis = "[Take %o with you in your quest for the Amulet.]",
         output = "text",
         text = [[%lC notices %o in your possession,
beams at you and says:

    "I knew you could defeat %n and retrieve
    %o.  We shall never forget this
    brave service.

    "Take %oh with you in your quest for the Amulet of Yendor.
    I can sense that it has attuned %oiself to you already.

    "May %d guide you in your quest, and keep you from harm."]],
      },
      offeredit2 = {
         synopsis = "[Keep %o, return through the portal to %Z; find the other Amulet.]",
         output = "text",
         text = [["You are the keeper of %o now.  It is time to
recover the /other/ Amulet.  %Z await your return through
the magic portal which brought you here."]],
      },
      othertime = {
         text = [[You are back at %H.
You have an odd feeling this may be the last time you ever come here.]],
      },
      posthanks = {
         text = [["Come near, my %S, and share your adventures with me.
So, have you succeeded in your quest for the Amulet of Yendor?"]],
      },
   },
}
