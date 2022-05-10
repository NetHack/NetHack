-- NetHack mines minend-3.lua	$NHDT-Date: 1652196029 2022/05/10 15:20:29 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.2 $
--	Copyright (c) 1989-95 by Jean-Christophe Collet
--	Copyright (c) 1991-95 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--
-- "Catacombs" by Kelly Bailey
-- Relies on some very specific behavior of MAZEWALK.

des.level_init({ style = "solidfill", fg = "-" });

des.level_flags("mazelevel", "nommap")

des.map({ halign = "center", valign = "bottom", map = [[
 - - - - - - - - - - -- -- - - . - - - - - - - - - -- - - -- - - - - . - - |
------...---------.-----------...-----.-------.-------     ----------------|
 - - - - - - - - - - - . - - - . - - - - - - - - - - -- - -- - . - - - - - |
------------.---------...-------------------------.---   ------------------|
 - - - - - - - - - - . . - - --- - . - - - - - - - - -- -- - - - - |.....| |
--.---------------.......------------------------------- ----------|.....S-|
 - - - - |.. ..| - ....... . - - - - |.........| - - - --- - - - - |.....| |
----.----|.....|------.......--------|.........|--------------.------------|
 - - - - |..{..| - - -.... . --- - -.S.........S - - - - - - - - - - - - - |
---------|.....|--.---...------------|.........|---------------------------|
 - - - - |.. ..| - - - . - - - - - - |.........| - --- . - - - - - - - - - |
----------------------...-------.---------------------...------------------|
---..| - - - - - - - - . --- - - - - - - - - - - - - - . - - --- - - --- - |
-.S..|----.-------.------- ---------.-----------------...----- -----.-------
---..| - - - - - - - -- - - -- . - - - - - . - - - . - . - - -- -- - - - -- 
-.S..|--------.---.---       -...---------------...{.---------   ---------  
--|. - - - - - - - -- - - - -- . - - - --- - - - . . - - - - -- - - - - - - 
]] });

local place = { {1,15},{68,6},{1,13} }
shuffle(place)

des.non_diggable(selection.area(67,3,73,7))
des.non_diggable(selection.area(0,12,2,16))
des.feature("fountain", {12,08})
des.feature("fountain", {51,15})
des.region(selection.area(0,0,75,16),"unlit")
des.region(selection.area(38,6,46,10),"lit")
des.door("closed",37,8)
des.door("closed",47,8)
des.door("closed",73,5)
des.door("closed",2,15)
des.mazewalk({ x=36, y=8, dir="west", stocked=false })
des.stair("up", 42,8)
des.wallify()

-- Objects
des.object("diamond")
des.object("*")
des.object("diamond")
des.object("*")
des.object("emerald")
des.object("*")
des.object("emerald")
des.object("*")
des.object("emerald")
des.object("*")
des.object("ruby")
des.object("*")
des.object("ruby")
des.object("amethyst")
des.object("*")
des.object("amethyst")
des.object({ id="luckstone", coord=place[2], buc="not-cursed", achievement=1 })
des.object("flint",place[1])
des.object("?")
des.object("?")
des.object("?")
des.object("?")
des.object("?")
des.object("+")
des.object("+")
des.object("+")
des.object("+")
des.object()
des.object()
des.object()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
-- One-time annoyance factor
des.trap("level teleport",place[2])
des.trap("level teleport",place[1])
des.monster("M")
des.monster("M")
des.monster("M")
des.monster("M")
des.monster("M")
des.monster("ettin mummy")
des.monster("V")
des.monster("Z")
des.monster("Z")
des.monster("Z")
des.monster("Z")
des.monster("Z")
des.monster("V")
des.monster("e")
des.monster("e")
des.monster("e")
des.monster("e")
