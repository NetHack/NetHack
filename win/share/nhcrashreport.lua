#!./nhlua
-- NetHack 3.7  nhcrashreport.lua       $NHDT-Date: 1693083824 2023/08/26 21:03:44 $  $NHDT-Branch: keni-crashweb2 $:$NHDT-Revision: 1.0 $
--      Copyright (c) 2023 Kenneth Lorber
-- NetHack may be freely redistributed.  See license for details.

-- Call with URL then field value pairs.  Opens a new browser window
-- to: URL?field=value+field=value.....
-- This program encodes the values; fieldnames don't require encoding.
--
-- Should be installed in the playground.

----
-- from
-- https://github.com/daurnimator/lua-http/blob/master/http/util.lua
-- Encodes a character as a percent encoded string
local function char_to_pchar(c)
	return string.format("%%%02X", c:byte(1,1))
end
-- encodeURIComponent escapes all characters except the following: alphabetic, decimal digits, - _ . ! ~ * ' ( )
local function encodeURIComponent(str)
	return (str:gsub("[^%w%-_%.%!%~%*%'%(%)]", char_to_pchar))
end
----

function un20(str)
    return str:gsub("%%20","+")
end

url = table.remove(arg,1) .. "?cos=1"; -- Start the query string and set mode
while #arg > 0 do
    local field = table.remove(arg,1)
    local value = table.remove(arg,1)
    url = url .. "&" .. field .. "=" .. un20(encodeURIComponent(value))
end
--print("url='"..url.."'")
cmd = '/usr/bin/xdg-open "'..url..'"'
os.execute(cmd)
os.exit()
