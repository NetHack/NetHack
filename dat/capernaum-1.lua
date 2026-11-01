-- Capernaum layout 1. Approved geometry, 76x21.
-- The whole town is lit. Peter's closet door is locked.
des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "nomongen", "noflip", "noautosearch")
des.map({ x = 1, y = 0, lit = 1, map = [[
}}}}}}---------------------------------------------------------------------|
}}}}}}}......--------...-------..---------..------..-----------..--------..|
}}}}}}}......|......|...|.....|..|.......|..|....|..|.........|..|......|..|
}}}}}}}}.....|......|...|.....|..|.......|..|....|..|.........|..|......|..|
}............|......|...|.....|..|.......|..--+---..|.........|..|......|..|
}--.--.......---+----...---+---..----+----..........|.........|..---+----..|
}|...|..............................................|.........|............|
}}---}}...--------..................................-----+-----............|
}}}}}}}}..|......-----.....................................................|
}}}}}}}}..|......|...|..........................................----------.|
}}}}}}}...+......+...|..............{.......{.........--------..|........|.|
}}---}}...|......-----................................|......|..+........|.|
}|...|....--------.....................}}}............+......|..|........|.|
}--.--..................................}.............|......|..----------.|
}.....................................................|......|.............|
}}}}}}}}................--+---...........---+----.....--------..----+-----.|
}}}}}}}.................|....|..--+---...|......|...............|........|.|
}}}}}}}.-----...........|....|..|....|...|......|...............|........|.|
}}}}}}..|...............|....|..|....|...|......|...............|........|.|
}}}}}}..-----...........------..------...--------...............----------.|
}}}}}}}--------------------------------------------------------------------|
]]})
des.region({ region={25,2, 29,4}, lit=1, type="food shop", filled=1, joined=0 })
des.region({ region={34,2, 40,4}, lit=1, type="shop", filled=1, joined=0 })
des.region({ region={55,11, 60,14}, lit=1, type="tool shop", filled=1, joined=0 })
des.region({ region={45,2, 48,3}, lit=1, type="candle shop", filled=1, joined=0 })
des.region({ region={65,10, 72,12}, lit=1, type="armor shop", filled=1, joined=0 })
des.region({ region={66,2, 71,4}, lit=1, type="weapon shop", filled=1, joined=0 })
des.region({ region={42,16, 47,18}, lit=1, type="scroll shop", filled=1, joined=0 })
des.region({ region={33,17, 36,18}, lit=1, type="potion shop", filled=1, joined=0 })
des.region({ region={65,16, 72,18}, lit=1, type="ring shop", filled=1, joined=0 })
des.region({ region={25,16, 28,18}, lit=1, type="health food shop", filled=1, joined=0 })
des.region({ region={53,2, 61,6}, lit=1, type="temple", filled=1, joined=0 })
des.altar({ x=57, y=3, align="law", type="shrine" })
des.door("closed", 46, 4)
des.door("closed", 16, 5)
des.door("closed", 27, 5)
des.door("closed", 37, 5)
des.door("closed", 68, 5)
des.door("closed", 57, 7)
des.door("closed", 10, 10)
des.door("locked", 17, 10)
des.door("closed", 64, 11)
des.door("closed", 54, 12)
des.door("closed", 26, 15)
des.door("closed", 44, 15)
des.door("closed", 68, 15)
des.door("closed", 34, 16)
des.stair("up", 14, 15)
des.stair("down", 71, 7)
local closet = selection.floodfill(19, 10)
des.non_diggable(closet)
des.non_passwall(closet)
local rim = selection.grow(closet, "all")
des.non_diggable(selection.filter_mapchar(rim, "w"))
des.non_passwall(selection.filter_mapchar(rim, "w"))
des.object({ id="chest", x=19, y=10, locked=0, trapped=0,
    contents = function()
        des.object({ id="gleaming armor", name="Breastplate of Righteousness",
                     spe=2, buc="blessed" })
    end })
des.monster({ id="human", name="Peter the Fisherman", peaceful=1,
              waiting=1, female=0, x=16, y=10 })
des.monster({ id="human", peaceful=1, x=9, y=3 })
des.monster({ id="human", peaceful=1, x=40, y=7 })
des.monster({ id="dog", peaceful=1, x=66, y=7 })
des.monster({ id="housecat", peaceful=1, x=23, y=8 })
des.monster({ id="human", peaceful=1, x=8, y=9 })
des.monster({ id="human", peaceful=1, x=33, y=9 })
des.monster({ id="human", peaceful=1, x=62, y=9 })
des.monster({ id="human", peaceful=1, x=47, y=11 })
des.monster({ id="housecat", peaceful=1, x=51, y=13 })
des.monster({ id="human", peaceful=1, x=20, y=14 })
des.monster({ id="housecat", peaceful=1, x=31, y=14 })
des.monster({ id="dog", peaceful=1, x=38, y=14 })
des.monster({ id="human", peaceful=1, x=12, y=15 })
des.monster({ id="dog", peaceful=1, x=8, y=16 })
des.monster({ id="human", peaceful=1, x=50, y=17 })
