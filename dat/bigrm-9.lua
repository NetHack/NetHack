-- NetHack bigroom bigrm-9.lua	$NHDT-Date: 1652196023 2022/05/10 15:20:23 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1990 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
des.level_init({ style = "solidfill", fg = " " });
des.level_flags("mazelevel", "noflip");

des.map([[
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}................}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}................................}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}............................................}}}}}}}}}}}}}}}
}}}}}}}}}}......................................................}}}}}}}}}}
}}}}}}}............................................................}}}}}}}
}}}}}.......................LLLLLLLLLLLLLLLLLL.......................}}}}}
}}}....................LLLLLLLLLLLLLLLLLLLLLLLLLLL.....................}}}
}....................LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL....................}
}....................LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL....................}
}....................LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL....................}
}}}....................LLLLLLLLLLLLLLLLLLLLLLLLLLL.....................}}}
}}}}}.......................LLLLLLLLLLLLLLLLLL.......................}}}}}
}}}}}}}............................................................}}}}}}}
}}}}}}}}}}......................................................}}}}}}}}}}
}}}}}}}}}}}}}}}............................................}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}................................}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}................}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
]]);

-- Unlit, except 3 mapgrids around the "pupil"
des.region(selection.area(00,00,73,18),"unlit");
des.region(selection.area(26,04,47,14),"lit");
des.region(selection.area(21,05,51,13),"lit");
des.region(selection.area(19,06,54,12),"lit");

des.stair("up");
des.stair("down");

des.non_diggable();

for i = 1,15 do
   des.object();
end

for i = 1,6 do
   des.trap();
end

for i = 1,28 do
  des.monster();
end
