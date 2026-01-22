-- Korean translation of tut-2.lua
-- 튜토리얼 레벨 2 한국어 번역

des.level_init({ style = "solidfill", fg = " " });
des.level_flags("mazelevel", "noflip",
                "nomongen", "nodeathdrops", "noautosearch");

des.map([[
--------------
|............|
|............|
|............|
|............|
|............|
|............|
--------------
]]);


des.region(selection.area(01,01, 73, 16), "lit");

des.stair({ dir = "up", coord = { 2,2 } });

des.engraving({ coord = { 1,1 }, type = "burn", text = "'" .. nh.eckey("up") .. "'로 계단을 올라가세요", degrade = false });


des.trap({ type = "magic portal", coord = { 11,5 }, seen = true });

des.non_diggable();
