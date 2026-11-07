-- Jacob's dream, above the end of Sokoban. No random monsters.
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "nomongen", "noteleport", "hardfloor", "noflip")

des.map([[
---------------------------------------
|T...{...T...T...T...T...T...T...{...T|
|T..........{..T.......T.............T|
|.......T.......T.............T.......|
|T..T.............................T..T|
|.......T.............T.......T.......|
|T.............T.......T..{..........T|
|T...{...T...T...T...T...T...T...{...T|
---------------------------------------
]])
des.region(selection.area(00,00,38,08), "lit")
des.non_diggable(selection.area(00,00,38,08))
-- The branch stair back to the Sokoban prize. Not a second stair.
des.levregion({ type = "branch", region = {19,4,19,4} })
-- Back to the Sokoban entry, the level where the hero first arrives.
des.trap({ type = "magic portal", coord = {19,6}, seen = true })
-- Genesis 28:12 KJV. The plan names this Jacob's dream and does not
-- quote the verse; the engraving is the verse that names the dream.
des.engraving({ x = 12, y = 03, type = "burn", degrade = false,
   text = "And he dreamed, and behold a ladder set up on the earth, and the top of it reached to heaven: and behold the angels of God ascending and descending on it." })
des.monster({ id = "Angel", x = 12, y = 04, peaceful = 1, name = "Angel at the Ladder" })
des.object({ id = "chest", x = 26, y = 04, locked = 0, trapped = 0,
             contents = function()
                des.object({ id = "diamond",
                             name = "PLACEHOLDER main-quest artifact" })
             end })
des.feature("fountain", 05, 01)
des.feature("fountain", 33, 01)
des.feature("fountain", 12, 02)
des.feature("fountain", 26, 06)
des.feature("fountain", 05, 07)
des.feature("fountain", 33, 07)
local trees = {
   {1,1},{1,7},{9,1},{9,7},{13,1},{13,7},{17,1},{17,7},
   {21,1},{21,7},{25,1},{25,7},{29,1},{29,7},{37,1},{37,7},
   {1,2},{37,2},{1,4},{37,4},{1,6},{37,6},
   {8,3},{15,2},{23,2},{30,3},{8,5},{15,6},{23,6},{30,5},
   {4,4},{34,4},{16,3},{22,5},
}
for _, t in ipairs(trees) do
   des.feature({ x = t[1], y = t[2], type = "tree" })
end
