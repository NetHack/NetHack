
-- Test shop creation

function shk_test1(doors)
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

des.teleport_region({ region = { 5,1, 7,7 }, dir = "both" });
des.stair("down", 6, 2);
des.stair("up", 6, 6);

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

end -- shk_test1


function do_test(testfunc, param)
    des.reset_level();
    des.level_init({ style = "solidfill", fg = " " });
    testfunc(param);
    des.finalize_level();
end

do_test(shk_test1, 0);
do_test(shk_test1, 1);
do_test(shk_test1, 2);

