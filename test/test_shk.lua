
-- Test shop creation

function shk_test_region_irregular(doors)
des.map([[
---------
|...|...|
|...+...|
|...|+--|
|+---...|
|...|+--|
|...+...|
|...|...|
---------
]]);

des.door("open", 5, 3);
des.door("open", 5, 5);

if (doors == 1) then
  des.door("open", 4, 2);
  des.door("closed", 4, 6);
end

des.region({ region = { 1,1, 1,1 }, type = "shop", irregular=1, filled=1, lit=1 });
des.region({ region = { 1,5, 1,5 }, type = "shop", irregular=1, filled=1, lit=1 });

if (doors == 2) then
  des.door("open", 4, 2);
  des.door("closed", 4, 6);
end

end -- shk_test_region_irregular

function shk_test_region_irregular2(doors)
des.map([[
---------
|...|...|
|...+...|
|...|...|
|+---...|
|.......|
|.......|
|.......|
---------
]]);

if (doors == 1) then
  des.door("closed", 4, 2);
  des.door("closed", 1, 4);
end

des.region({ region = { 1,1, 1,1 }, type = "shop", irregular=1, filled=1, lit=1 });
des.region({ region = { 1,5, 1,5 }, type = "shop", irregular=1, filled=1, lit=1 });

if (doors == 2) then
  des.door("closed", 4, 2);
  des.door("closed", 1, 4);
end

end -- shk_test_region_irregular2




function shk_test_region_regular(doors)
des.map([[
---------
|...|...|
|...+...|
|...|+--|
|----...|
|...|+--|
|...+...|
|...|...|
---------
]]);

des.door("open", 5, 3);
des.door("open", 5, 5);

if (doors == 1) then
  des.door("open", 4, 2);
  des.door("closed", 4, 6);
end

des.region({ region = { 1,1, 3,3 }, type = "shop", irregular=0, filled=1, lit=1 });
des.region({ region = { 1,5, 3,7 }, type = "shop", irregular=0, filled=1, lit=1 });

if (doors == 2) then
  des.door("open", 4, 2);
  des.door("closed", 4, 6);
end

end -- shk_test_region_regular


-- test rooms. this layout is minetn-7 with all rooms changed to shops
function shk_test_rooms1(param)
des.room({ type="ordinary", lit=1, x=3,y=3,
           xalign="center",yalign="center", w=30,h=15,
           contents = function()

              des.room({ type="shop", x=2,y=2, w=4,h=2,
                         contents = function()
                            des.door({ state = "closed", wall="south" })
                         end
              })

              des.room({ type="shop", x=7,y=2, w=2,h=2,
                         contents = function()
                            des.door({ state = "closed", wall="north" })
                         end
              })

              des.room({ type="shop", x=7,y=5, w=2,h=2,
                         contents = function()
                            des.door({ state = "closed", wall="south" })
                         end
              })

              des.room({ type="shop", lit=1, x=10,y=2, w=3,h=4,
                         contents = function()
                            des.door({ state = "closed", wall="south" })
                         end
              })

              des.room({ type="shop", x=14,y=2, w=4,h=2,
                         contents = function()
                            des.door({ state = "closed", wall="south", pos=0 })
                         end
              })

              des.room({ type="shop", x=16,y=5, w=2,h=2,
                         contents = function()
                            des.door({ state = "closed", wall="south" })
                         end
              })

              des.room({ type="shop", lit=0, x=19,y=2, w=2,h=2,
                         contents = function()
                            des.door({ state = "locked", wall="east" })
                         end
              })

              des.room({ type=monkfoodshop(), lit=1, x=19,y=5, w=2,h=3,
                         contents = function()
                            des.door({ state = "closed", wall="south" })
                         end
              })

              des.room({ type="shop", x=2,y=7, w=2,h=2,
                         contents = function()
                            des.door({ state = "closed", wall="east" })
                         end
              })

              des.room({ type="tool shop", lit=1, x=2,y=10, w=2,h=3,
                         contents = function()
                            des.door({ state = "closed", wall="south" })
                         end
              })

              des.room({ type="candle shop", lit=1, x=5,y=10, w=3,h=3,
                         contents = function()
                            des.door({ state = "closed", wall="north" })
                         end
              })

              des.room({ type="shop", x=11,y=10, w=2,h=2,
                         contents = function()
                            des.door({ state = "locked", wall="west" })
                         end
              })

              des.room({ type="shop", lit=1, x=14,y=10, w=2,h=3,
                         contents = function()
                            des.door({ state = "closed", wall="north" })
                         end
              })

              des.room({ type="shop", x=17,y=11, w=4,h=2,
                         contents = function()
                            des.door({ state = "closed", wall="north" })
                         end
              })

              des.room({ type="shop", x=22,y=11, w=2,h=2,
                         contents = function()
                            des.door({ state = "closed", wall="south" })
                         end
              })

              des.room({ type=monkfoodshop(), lit=1, x=25,y=11, w=3,h=2,
                         contents = function()
                            des.door({ state = "closed", wall="east" })
                         end
              })

              des.room({ type="tool shop", lit=1, x=25,y=2, w=3,h=3,
                         contents = function()
                            des.door({ state = "closed", wall="west" })
                         end
              })

              des.room({ type="shop", lit=1, x=24,y=6, w=4,h=4,
                         contents = function()
                            des.door({ state = "closed", wall = "west" })
                         end
              })
           end
})
end -- shk_test_rooms1



function do_test(testfunc, param)
   des.reset_level();
   des.level_flags("noflip");
   des.level_init({ style = "solidfill", fg = " " });
   testfunc(param);
   des.finalize_level();
end

do_test(shk_test_region_irregular, 0);
do_test(shk_test_region_irregular, 1);
do_test(shk_test_region_irregular, 2);

do_test(shk_test_region_irregular2, 0);
do_test(shk_test_region_irregular2, 1);
do_test(shk_test_region_irregular2, 2);

do_test(shk_test_region_regular, 0);
do_test(shk_test_region_regular, 1);
do_test(shk_test_region_regular, 2);

do_test(shk_test_rooms1, 0);
