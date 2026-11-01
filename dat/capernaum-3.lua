-- Capernaum layout 3. Approved geometry, 76x21.
-- The whole town is lit. Peter's closet door is locked.
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noflip", "noautosearch")
des.map({ x = 1, y = 0, lit = 1, map = [[
|-------------------------------------------------------------------}}}}}}}}
|.--------..-----------..-------..--------..---------.......----....}}}}}}}}
|.|......|..|.........|..|.....|..|......|..|.......|...........|..}}}}}}}}}
|.|......|..|.........|..|.....|..|......|..|.......|.......----...}}}}}}}}}
|.|......|..|.........|..|.....|..|......|..|.......|.............}}---}}}}}
|.---+----..|.........|..---+---..---+----..----+----...........}}}|...|}}}}
|...........|.........|.......................................}}}}}--.--}}}}
|...........-----+-----..................................................}}}
|.-------.....................-------..........---------...}}}}}}}}}}}}}}}}}
|.|.....|.....................|.....|......-----.......|..}}}}}}}}}}}}}}}}}}
|.|.....+.........{......{....|.....|......|...+.......+..}}}}}}}}}}}}}}}}}}
|.|.....|.....................---+---......-----.......|..}}}}}}}}}}}}}}}}}}
|.-------............}}........................---------...}}}}}}}}}}}}}}}}}
|........................................................................}}}
|.--------..---+---............---+----.......................}}}}}--.--}}}}
|.|......|..|.....|..---+----..|......|...----+----.............}}}|...|}}}}
|.|......+..|.....|..|......|..|......|...|.......|...............}}---}}}}}
|.|......|..|.....|..|......|..|......|...|.......|................}}}}}}}}}
|.|......|..|.....|..|......|..|......|...|.......|................}}}}}}}}}
|.--------..-------..--------..--------...---------.................}}}}}}}}
|-------------------------------------------------------------------}}}}}}}}
]]})
des.region({ region={3,2, 8,4}, lit=1, type="armor shop", filled=1, joined=0 })
des.region({ region={26,2, 30,4}, lit=1, type="food shop", filled=1, joined=0 })
des.region({ region={35,2, 40,4}, lit=1, type="candle shop", filled=1, joined=0 })
des.region({ region={45,2, 51,4}, lit=1, type="shop", filled=1, joined=0 })
des.region({ region={3,9, 7,11}, lit=1, type="weapon shop", filled=1, joined=0 })
des.region({ region={3,15, 8,18}, lit=1, type="tool shop", filled=1, joined=0 })
des.region({ region={13,15, 17,18}, lit=1, type="health food shop", filled=1, joined=0 })
des.region({ region={22,16, 27,18}, lit=1, type="scroll shop", filled=1, joined=0 })
des.region({ region={32,15, 37,18}, lit=1, type="potion shop", filled=1, joined=0 })
des.region({ region={43,16, 49,18}, lit=1, type="ring shop", filled=1, joined=0 })
des.region({ region={13,2, 21,6}, lit=1, type="temple", filled=1, joined=0 })
des.altar({ x=17, y=4, align="law", type="shrine" })
des.door("closed", 5, 5)
des.door("closed", 28, 5)
des.door("closed", 37, 5)
des.door("closed", 48, 5)
des.door("closed", 17, 7)
des.door("closed", 8, 10)
des.door("locked", 47, 10)
des.door("closed", 55, 10)
des.door("closed", 33, 11)
des.door("closed", 15, 14)
des.door("closed", 34, 14)
des.door("closed", 24, 15)
des.door("closed", 46, 15)
des.door("closed", 9, 16)
des.stair("up", 4, 7)
des.stair("down", 54, 17)
local closet = selection.floodfill(45, 10)
des.non_diggable(closet)
des.non_passwall(closet)
local rim = selection.grow(closet, "all")
des.non_diggable(selection.filter_mapchar(rim, "w"))
des.non_passwall(selection.filter_mapchar(rim, "w"))
des.object({ id="chest", x=45, y=10, locked=0, trapped=0,
    contents = function()
        des.object({ id="gleaming armor", name="Breastplate of Righteousness",
                     spe=2, buc="blessed" })
    end })
des.monster({ id="human", name="Peter the Fisherman", peaceful=1,
              waiting=1, female=0, x=48, y=10 })
des.monster({ id="human", peaceful=1, x=57, y=4 })
des.monster({ id="housecat", peaceful=1, x=61, y=5 })
des.monster({ id="dog", peaceful=1, x=42, y=7 })
des.monster({ id="human", peaceful=1, x=53, y=7 })
des.monster({ id="human", peaceful=1, x=11, y=9 })
des.monster({ id="housecat", peaceful=1, x=14, y=9 })
des.monster({ id="human", peaceful=1, x=21, y=9 })
des.monster({ id="human", peaceful=1, x=39, y=9 })
des.monster({ id="dog", peaceful=1, x=10, y=12 })
des.monster({ id="human", peaceful=1, x=27, y=12 })
des.monster({ id="human", peaceful=1, x=19, y=13 })
des.monster({ id="housecat", peaceful=1, x=29, y=13 })
des.monster({ id="human", peaceful=1, x=40, y=13 })
des.monster({ id="human", peaceful=1, x=58, y=15 })
des.monster({ id="dog", peaceful=1, x=56, y=18 })
