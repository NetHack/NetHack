-- Capernaum layout 2. Approved geometry, 76x21.
-- The whole town is lit. Peter's closet door is locked.
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noflip", "noautosearch")
des.map({ x = 1, y = 0, lit = 1, map = [[
|--------------------------------------------------------------------------|
|.--------..--------..---------...------------..--------..-------.--------.|
|.|......|..|......|..|.......|...|..........|..|......|..|.....|.|......|.|
|.|......|..|......|..|.......|...|..........|..|......|..|.....|.|......|.|
|.|......|..|......|..----+----...|..........|..|......|..|.....|.|......|.|
|.|......|..|......|..............|..........|..|......|..---+---.|......|.|
|.---+----..---+----......-----...|..........|..---+----..........---+----.|
|.........................|...|...-----+------.............................|
|.--------................|...|..................................---------.|
|.|......|..--------....----+----...................-------......|.......|.|
|.|......+..|......|....|.......|.....{.....{.......|.....|......+.......|.|
|.|......|..|......|....|.......|...................|.....|......|.......|.|
|.--------..|......|....|.......|.......}}}.........|.....|......---------.|
|...........---+----....|.......|...................---+---...-----........|
|.......................----+----.............................|............|
|.............................................................-----........|
}}}}}}........}}}}}}....}}}}}}}}}}......}}}}}}.}......}}}}}}}}}}......}}}}}}
}}}}}}}}}}}}.|---}}}}}}}}}}}}}}}}}}}}}}}}}---|.}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}.....|}}}}}}}}}}}}}}}}}}}}}}}|.....}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}.|---}}}}}}}}}}}}}}}}}}}}}}}}}---|.}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
]]})
des.region({ region={3,2, 8,5}, lit=1, type="food shop", filled=1, joined=0 })
des.region({ region={13,2, 18,5}, lit=1, type="candle shop", filled=1, joined=0 })
des.region({ region={23,2, 29,3}, lit=1, type="scroll shop", filled=1, joined=0 })
des.region({ region={49,2, 54,5}, lit=1, type="shop", filled=1, joined=0 })
des.region({ region={59,2, 63,4}, lit=1, type="potion shop", filled=1, joined=0 })
des.region({ region={67,2, 72,5}, lit=1, type="tool shop", filled=1, joined=0 })
des.region({ region={3,9, 8,11}, lit=1, type="armor shop", filled=1, joined=0 })
des.region({ region={53,10, 57,12}, lit=1, type="ring shop", filled=1, joined=0 })
des.region({ region={66,9, 72,11}, lit=1, type="weapon shop", filled=1, joined=0 })
des.region({ region={35,2, 44,6}, lit=1, type="temple", filled=1, joined=0 })
des.altar({ x=39, y=4, align="law", type="shrine" })
des.door("closed", 26, 4)
des.door("closed", 61, 5)
des.door("closed", 5, 6)
des.door("closed", 15, 6)
des.door("closed", 51, 6)
des.door("closed", 69, 6)
des.door("closed", 39, 7)
des.door("locked", 28, 9)
des.door("closed", 9, 10)
des.door("closed", 65, 10)
des.door("closed", 15, 13)
des.door("closed", 55, 13)
des.door("closed", 28, 14)
des.stair("up", 21, 3)
des.stair("down", 60, 8)
local closet = selection.floodfill(28, 7)
des.non_diggable(closet)
des.non_passwall(closet)
local rim = selection.grow(closet, "all")
des.non_diggable(selection.filter_mapchar(rim, "w"))
des.non_passwall(selection.filter_mapchar(rim, "w"))
des.object({ id="chest", x=28, y=7, locked=0, trapped=0,
    contents = function()
        des.object({ id="gleaming armor", name="Breastplate of Righteousness",
                     spe=2, buc="blessed" })
    end })
des.monster({ id="human", name="Peter the Fisherman", peaceful=1,
              waiting=1, female=0, x=28, y=10 })
des.monster({ id="dog", peaceful=1, x=31, y=5 })
des.monster({ id="human", peaceful=1, x=11, y=7 })
des.monster({ id="human", peaceful=1, x=64, y=7 })
des.monster({ id="human", peaceful=1, x=35, y=9 })
des.monster({ id="human", peaceful=1, x=47, y=9 })
des.monster({ id="housecat", peaceful=1, x=48, y=12 })
des.monster({ id="human", peaceful=1, x=10, y=14 })
des.monster({ id="human", peaceful=1, x=41, y=14 })
des.monster({ id="housecat", peaceful=1, x=68, y=14 })
des.monster({ id="housecat", peaceful=1, x=11, y=15 })
des.monster({ id="human", peaceful=1, x=20, y=15 })
des.monster({ id="dog", peaceful=1, x=23, y=15 })
des.monster({ id="human", peaceful=1, x=33, y=15 })
des.monster({ id="human", peaceful=1, x=50, y=15 })
des.monster({ id="dog", peaceful=1, x=57, y=15 })
