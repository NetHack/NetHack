-- Korean translation of tut-1.lua
-- 튜토리얼 레벨 1 한국어 번역

local tut_ctrl_key = nil;
local tut_alt_key = nil;

function tut_key(command)
   local s = nh.eckey(command);
   local m = s:match("^^([A-Z])$"); -- ^X is Ctrl-X
   if (m ~= nil) then
      tut_ctrl_key = m;
      return "Ctrl-" .. m;
   end

   m = s:match("^M%-([A-Z])$"); -- M-X is Alt-X
   if (m ~= nil) then
      tut_alt_key = m;
      return "Alt-" .. m;
   end

   return s;
end

function tut_key_help(x, y)
   if (tut_ctrl_key ~= nil) then
      des.engraving({ coord = { x,y }, type = "engrave", text = "참고: 튜토리얼 밖에서는 Ctrl 키 조합이 '^" .. tut_ctrl_key .. "'처럼 캐럿으로 표시됩니다", degrade = false });
      tut_ctrl_key = nil;
   end
end

des.level_init({ style = "solidfill", fg = " " });
des.level_flags("mazelevel", "noflip",
                "nomongen", "nodeathdrops", "noautosearch");

des.map([[
---------------------------------------------------------------------------
|-.--|.......|......|..S....|.F.......|.............|.......|.............|
|.-..........|......|--|....|.F.....|.|S-------.....|.....................|
||.--|.......|..T......|....|.F.....|.|.......|.....|.......|.............|
||.|.|.......|......|-.|....|.F.....|.|.......|.....|--------.............|
||.|.|.......|......||.|-.-----------.-.......|-S----.....................|
|-+-S---------..---.||........................|...|.......................|
|......|          |.-------------------.......|...|....--S----............|
|......|  ######  |.........|      |..S.......|...|....|.....|............|
|----.-| -+-   #  |.....---.|######+..|.......S...|....|.....|............|
|----+----.----+---.|.--|.|.|#     ------------...|....|.....F............|
|........|.|......|.|...F...|#  ........|.....+...|....|.....|............|
|.P......-S|......|------.---# .........|.....|LLL|..................|..| |
|..........|......+.|...|.|.S# ..--S-----.....|LLL|..................|..--|
|.W......---......|.|.|.|.|.|# ..|......|.....|...|..................|..|.|
|....Z.L.S.F......|.|.|.|.---#   |......+.....|...+..................||...|
|........|--......|...|.....|####+......|.....|...+..................||...|
---------------------------------------------------------------------------
]]);


des.region(selection.area(01,01, 73, 16), "lit");

des.non_diggable();

des.teleport_region({ region = { 9,3, 9,3 } });

-- turn on some newbie-friendly options
nh.parse_config("OPTIONS=mention_walls");
nh.parse_config("OPTIONS=mention_decor");
nh.parse_config("OPTIONS=lit_corridor");

local movekeys = tut_key("movewest") .. " " ..
   tut_key("movesouth") .. " " ..
   tut_key("movenorth") .. " " ..
   tut_key("moveeast");

local diagmovekeys = tut_key("movesouthwest") .. " " ..
   tut_key("movenortheast") .. " " ..
   tut_key("movesoutheast") .. " " ..
   tut_key("movenorthwest");

des.engraving({ coord = { 9,3 }, type = "engrave", text = movekeys .. " 키로 이동하세요", degrade = false });
des.engraving({ coord = { 5,2 }, type = "engrave", text = diagmovekeys .. " 키로 대각선 이동", degrade = false });

if (u.role == "Knight") then
   des.engraving({ coord = { 12,1 }, type = "engrave", text = "기사는 '" .. tut_key("jump") .. "'로 점프할 수 있습니다", degrade = false });
end

--

des.engraving({ coord = { 2,4 }, type = "engrave", text = "일부 행동은 성공하기 전에 여러 번 시도해야 할 수 있습니다", degrade = false });
des.engraving({ coord = { 2,5 }, type = "engrave", text = "문 쪽으로 이동하면 문이 열립니다", degrade = false });
des.door({ coord = { 2,6 }, state = "closed" });

des.engraving({ coord = { 2,7 }, type = "engrave", text = "'" .. tut_key("close") .. "'로 문을 닫으세요", degrade = false });


--

des.engraving({ coord = { 4,5 }, type = "engrave", text = "마법 포탈을 통해 튜토리얼을 나갈 수 있습니다.", degrade = false });
des.trap({ type = "magic portal", coord = { 4,4 }, seen = true });

--

des.engraving({ coord = { 5,9 }, type = "engrave", text = "이 문은 잠겨 있습니다. '" .. tut_key("kick") .. "'로 발로 차세요", degrade = false });
des.door({ coord = { 5,10 }, state = "locked" });

-- by default, kick is the first command that can be a ctrl-key combo
tut_key_help(6, 8);


des.engraving({ coord = { 5,12 }, type = "engrave", text = "'" .. tut_key("glance") .. "'로 지도를 둘러보세요. 끝나면 ESC를 누르세요", degrade = false });

--

des.engraving({ coord = { 10,13 }, type = "engrave", text = "'" .. tut_key("search") .. "'로 비밀문을 찾으세요", degrade = false });

des.engraving({ coord = { 10,15 }, type = "engrave", text = "잘못된 비밀", degrade = false });

--

des.engraving({ coord = { 10,10 }, type = "engrave", text = "이 문 뒤에는 어두운 복도가 있습니다", degrade = false });
des.door({ coord = { 10,9 }, state = percent(50) and "locked" or "closed" });
des.region(selection.match("#"), "unlit");
des.region(selection.match(" "), "unlit");
des.door({ coord = { 15,10 }, state = percent(50) and "locked" or "closed" });

--

des.engraving({ coord = { 15,11 }, type = "engrave", text = "주변에 4개의 함정이 있습니다! 찾아보세요.", degrade = false });
local locs = { {14,11}, {14,12}, {15,12}, {16,12}, {16,11} };
shuffle(locs);
for i = 1, 4 do
   des.trap({ type = percent(50) and "sleep gas" or "board",
              coord = locs[i], victim = false });
end

des.engraving({ coord = { 15,15 }, type = "engrave", text = "일부 함정은 '" .. tut_key("untrap") .. "'로 해제할 수 있습니다", degrade = false });
des.trap({ coord = { 15,16 }, type = "web", spider_on_web = false });

--

des.door({ coord = { 18,13 }, state = "closed" });

des.engraving({ coord = { 19,13 }, type = "engrave", text = "'" .. tut_key("pickup") .. "'로 아이템을 주우세요", degrade = false });

local armor = (u.role == "Monk") and "leather gloves" or "leather armor";

des.object({ id = armor, spe = 0, buc = "cursed", coord = { 19,14} });

des.engraving({ coord = { 19,15 }, type = "engrave", text = "'" .. tut_key("wear") .. "'로 갑옷을 입으세요", degrade = false });

des.object({ id = "dagger", spe = 0, buc = "not-cursed", coord = { 21,15} });

des.engraving({ coord = { 21,14 }, type = "engrave", text = "'" .. tut_key("wield") .. "'로 무기를 장착하세요", degrade = false });


des.engraving({ coord = { 22,13 }, type = "engrave", text = "몬스터에게 걸어가면 공격합니다.", degrade = false });

des.monster({ id = "lichen", coord = { 23,15 }, waiting = true, countbirth = false });

--

des.engraving({ coord = { 24,16 }, type = "engrave", text = "이제 기본을 배웠습니다. 마법 포탈로 튜토리얼을 나갈 수 있습니다.", degrade = false });

des.engraving({ coord = { 26,16 }, type = "engrave", text = "이 포탈에 들어가면 튜토리얼을 나갑니다", degrade = false });
des.trap({ type = "magic portal", coord = { 27,16 }, seen = true });

--

des.engraving({ coord = { 25,13 }, type = "engrave", text = "바위를 향해 이동하면 밀 수 있습니다", degrade = false });
des.object({ id = "boulder", coord = {25,12} });

--

des.engraving({ coord = { 27,9 }, type = "engrave", text = "'" .. tut_key("takeoff") .. "'로 갑옷을 벗으세요", degrade = false });

--

des.object({ class = "?", id = "remove curse", buc = "blessed", coord = {23,11} })
des.engraving({ coord = { 22,11 }, type = "engrave", text = "일부 아이템은 게임마다 다른 랜덤 설명을 가집니다", degrade = false });
des.engraving({ coord = { 23,11 }, type = "engrave", text = "이 두루마리를 줍고 '" .. tut_key("read") .. "'로 읽은 후 갑옷을 다시 벗어보세요", degrade = false });

--

des.engraving({ coord = { 19,10 }, type = "engrave", text = "또 다른 마법 포탈, 튜토리얼을 나가는 방법입니다", degrade = false });
des.trap({ type = "magic portal", coord = { 19,11 }, seen = true });

--

-- rock fall
des.object({ coord = {14, 5}, id = "rock", quantity = math.random(50,99) });
des.object({ coord = {15, 5}, id = "rock", quantity = math.random(10,30) });
des.object({ coord = {14, 4}, id = "rock", quantity = math.random(10,30) });
des.object({ coord = {15, 6}, id = "rock", quantity = math.random(30,60) });
des.object({ coord = {14, 6}, id = "rock", quantity = math.random(30,60) });
des.object({ coord = {14, 6}, id = "boulder" });

des.door({ coord = { 20,3 }, state = percent(50) and "open" or "closed" });

des.engraving({ coord = { 21,3 }, type = "engrave", text = "짐이 너무 무거우면 느려집니다", degrade = false });
des.engraving({ coord = { 22,3 }, type = "engrave", text = "'" .. tut_key("drop") .. "'로 아이템을 버리세요", degrade = false });
des.engraving({ coord = { 22,4 }, type = "engrave", text = "아이템 슬롯 문자 앞에 숫자를 붙여 일부만 버릴 수 있습니다", degrade = false });

--

des.monster({ id = "yellow mold", coord = { 26,2 }, waiting = true, countbirth = false });

des.engraving({ coord = { 25,5 }, type = "engrave", text = "'" .. tut_key("throw") .. "'로 아이템을 던지세요", degrade = false });

des.trap({ type = "magic portal", coord = { 21,1 }, seen = true });

--

des.monster({ id = "wolf", coord = { 29,2 }, peaceful = 0, waiting = true, countbirth = false });

des.engraving({ coord = { 37,4 }, type = "engrave", text = "돌 같은 투사체는 적절한 발사기로 쏘면 더 효과적입니다", degrade = false });

des.object({ coord = { 37,3 }, id = "sling", buc = "not-cursed", spe = 9 });
des.engraving({ coord = { 37,3 }, type = "engrave", text = "새총을 장착하세요", degrade = false });
des.engraving({ coord = { 36,1 }, type = "engrave", text = "'" .. tut_key("fire") .. "'로 장착한 발사기로 투사체를 발사하세요", degrade = false });

des.engraving({ coord = { 35,4 }, type = "engrave", text = "발사는 화살통의 아이템을 사용합니다. '" .. tut_key("quiver") .. "'로 화살통에 넣으세요", degrade = false });

des.engraving({ coord = { 33,4 }, type = "engrave", text = "'" .. tut_key("wait") .. "'로 한 턴 대기할 수 있습니다", degrade = false });


--

des.door({ coord = { 38,6 }, state = "closed" });

des.engraving({ coord = { 39,6 }, type = "engrave", text = "'" .. tut_key("loot") .. "'로 상자를 열어보세요", degrade = false });

des.object({ coord = { 41,6 }, id = "large box", broken = true, trapped = false,
             contents = function(obj)
                des.object({ id = "secret door detection", class = "/", spe = 30 }); end
});
des.engraving({ coord = { 42,6 }, type = "engrave", text = "'" .. tut_key("tip") .. "'로 상자를 기울여 비울 수도 있습니다", degrade = false });

des.engraving({ coord = { 45,6 }, type = "engrave", text = "'" .. tut_key("zap") .. "'로 마법 지팡이를 사용하세요", degrade = false });

--

des.door({ coord = { 35,9 }, state = "nodoor" });
des.engraving({ coord = { 34,9 }, type = "engrave", text = "'" .. tut_key("run") .. "'를 이동 키 앞에 붙이면 달릴 수 있습니다", degrade = false });

--

des.door({ coord = { 33,16 }, state = "nodoor" });
des.engraving({ coord = { 35,15 }, type = "engrave", text = "'" .. tut_key("travel") .. "'로 레벨을 가로질러 이동할 수 있습니다", degrade = false });

--

des.trap({ type = "magic portal", coord = { 27,14 }, seen = true });

--

des.engraving({ coord = { 48,1 }, type = "burn", text = "'" .. tut_key("eat") .. "'로 먹을 수 있는 것을 먹으세요", degrade = false });

des.object({ coord = { 50,3 }, id = "apple", buc = "not-cursed"  });
des.object({ coord = { 50,3 }, id = "candy bar", buc = "not-cursed"  });

des.object({ coord = { 50,3 }, id = "corpse", montype = "lichen", buc = "not-cursed" });

--

des.door({ coord = { 46,11 }, state = "closed" });

des.engraving({ coord = { 43,11 }, type = "burn", text = "'" .. tut_key("twoweapon") .. "'로 쌍수 전투를 할 수 있습니다", degrade = false });
des.object({ coord = { 43,13 }, id = "knife", buc = "uncursed" });
des.object({ coord = { 43,14 }, id = "dagger", buc = "blessed" });

des.engraving({ coord = { 43,16 }, type = "burn", text = "'" .. tut_key("swap") .. "'로 빠르게 무기를 교체하세요", degrade = false });

des.door({ coord = { 40,15 }, state = "random" });

--

des.object({ coord = { 48,7 }, id = "ring of levitation", buc = "not-cursed" });

des.engraving({ coord = { 48,10 }, type = "burn", text = "'" .. tut_key("puton") .. "'로 장신구를 착용하세요", degrade = false });

des.engraving({ coord = { 48,16 }, type = "burn", text = "'" .. tut_key("remove") .. "'로 장신구를 벗으세요", degrade = false });

des.door({ coord = { 50,16 }, state = "closed" });


--

des.engraving({ coord = { 58,9 }, type = "burn", text = "'" .. tut_key("down") .. "'로 계단을 내려가세요", degrade = false });
des.stair({ dir = "down", coord = { 58,10 } });

--

-- one more ctrl-key help, if needed
tut_key_help(64, 4);

des.engraving({ coord = { 65,3 }, type = "burn", text = "공사 중", degrade = false });

des.trap({ type = "magic portal", coord = { 66,2 }, seen = true });

--

-- squeezing through small gaps

des.engraving({ coord = { 69,12 }, type = "burn", text = "지나갈 수 없나요? 짐이 너무 많습니다.", degrade = false });

-- try to squeeze over boulders, find a trap door

des.object({ id = "boulder", coord = {71,16} });
des.object({ id = "boulder", coord = {72,16} });
des.object({ id = "boulder", coord = {73,16} });
des.trap({ type = "trap door", coord = { 73,15 } });

--

des.engraving({ coord = { 60,2 }, type = "engrave", text = "주문 시전", degrade = false });
if (u.uenmax < 5) then
   des.engraving({ coord = { 59,2 }, type = "engrave", text = "아쉽게도 주문을 시전할 에너지가 부족합니다.", degrade = false });
end
des.engraving({ coord = { 57,2 }, type = "engrave", text = "'" .. tut_key("pickup") .. "'로 마법서를 주우세요", degrade = false });
des.object({ coord = { 57,2 }, id = "spellbook of light", buc = "blessed" });
des.engraving({ coord = { 55,2 }, type = "engrave", text = "'" .. tut_key("read") .. "'로 마법서를 읽으세요", degrade = false });
des.engraving({ coord = { 53,2 }, type = "engrave", text = "'" .. tut_key("cast") .. "'로 주문을 시전하세요", degrade = false });
des.region(selection.area(53,01, 59, 3), "unlit");

--

des.engraving({ coord = { 72,2 }, type = "engrave", text = "'" .. tut_key("quaff") .. "'로 물약을 마시세요", degrade = false });
des.object({ coord = { 72,2 }, id = "potion of object detection", buc = "blessed" });
