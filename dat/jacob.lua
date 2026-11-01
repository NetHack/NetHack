-- Jacob's dream, above the end of Sokoban. No random monsters.
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "nomongen", "noteleport", "hardfloor", "noflip")

des.map([[
------------
|..........|
|..........|
|..........|
|..........|
|..........|
------------
]])
des.region(selection.area(00,00,11,06), "lit")
des.non_diggable(selection.area(00,00,11,06))
des.stair("down", 02, 03)
-- Genesis 28:12 KJV. The plan names this Jacob's dream and does not
-- quote the verse; the engraving is the verse that names the dream.
des.engraving({ x = 05, y = 02, type = "burn", degrade = false,
   text = "And he dreamed, and behold a ladder set up on the earth, and the top of it reached to heaven: and behold the angels of God ascending and descending on it." })
des.monster({ id = "Angel", x = 05, y = 03, peaceful = 1, name = "Angel of the Ladder" })
des.object({ id = "chest", x = 08, y = 03, locked = 0, trapped = 0,
             contents = function()
                des.object({ id = "diamond",
                             name = "PLACEHOLDER main-quest artifact" })
             end })
