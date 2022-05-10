-- NetHack core nhcore.lua	$NHDT-Date: 1652196284 2022/05/10 15:24:44 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.0 $
--	Copyright (c) 2021 by Pasi Kallinen
-- NetHack may be freely redistributed.  See license for details.
-- This file contains lua code used by NetHack core.
-- Is it loaded once, at game start, and kept in memory until game exit.


-- This is an example of generating an external file during gameplay,
-- which is updated periodically.
-- Intended for public servers using dgamelaunch as their login manager.
local prev_dgl_extrainfo = 0;
function mk_dgl_extrainfo()
    if ((prev_dgl_extrainfo == 0) or (prev_dgl_extrainfo + 50 < u.moves)) then
        local filename = nh.dump_fmtstr("/tmp/nethack.%n.%d.log");
        local extrai, err = io.open(filename, "w");
        if extrai then
            local sortval = 0;
            local dname = nh.dnum_name(u.dnum);
            local dstr = "";
            local astr = " ";
            if u.uhave_amulet == 1 then
                sortval = sortval + 1024;
                astr = "A";
            end
            if dname == "Fort Ludios" then
                dstr = "Knx";
                sortval = sortval + 245;
            elseif dname == "The Quest" then
                dstr = "Q" .. u.dlevel;
                sortval = sortval + 250 + u.dlevel;
            elseif dname == "The Elemental Planes" then
                dstr = "End";
                sortval = sortval + 256;
            elseif dname == "Vlad's Tower" then
                dstr = "T" .. u.dlevel;
                sortval = sortval + 235 + u.depth;
            elseif dname == "Sokoban" then
                dstr = "S" .. u.dlevel;
                sortval = sortval + 225 + u.depth;
            elseif dname == "The Gnomish Mines" then
                dstr = "M" .. u.dlevel;
                sortval = sortval + 215 + u.dlevel;
            else
                dstr = "D" .. u.depth;
                sortval = sortval + u.depth;
            end
            local str = sortval .. "|" .. astr .. " " .. dstr;

            extrai:write(str);
            extrai:close();
        else
            -- failed to open the file.
            nh.pline("Failed to open dgl extrainfo file: " .. err);
        end
        prev_dgl_extrainfo = u.moves;
    end
end


-- Callback functions
nhcore = {
    -- start_new_game called once, when starting a new game
    -- after "Welcome to NetHack" message has been given.
    -- start_new_game = function() nh.pline("NEW GAME!"); end,

    -- restore_old_game called once, when restoring a saved game
    -- after "Welcome back to NetHack" message has been given.
    -- restore_old_game = function() nh.pline("RESTORED OLD GAME!"); end,

    -- moveloop_turn is called once per turn.
    -- moveloop_turn = mk_dgl_extrainfo,

    -- game_exit is called when the game exits (quit, saved, ...)
    -- game_exit = function() end,
};

