
local configtests = {
 { test = "OPTIONS=color",  -- config string to parse
   result = { },  -- errors, result of parsing the config string
   extra = function() return nh.get_config("color") == "true" end  -- optional, function that returns boolean, and false means the test failed.
   },
 { test = "OPTIONS=!color",
   result = { },
   extra = function() return nh.get_config("color") == "false" end
   },
 { test = "OPTIONS=!color\nOPTIONS=color",
   result = { { line = 2, error = "boolean option specified multiple times: color" } }
   },
 { test = "OPTIONS=!color,color",
   result = { { line = 1, error = "boolean option specified multiple times: color" } }
   },
 { test = "OPTIONS=runmode:crawl",
   result = {  },
   extra = function() return nh.get_config("runmode") == "crawl" end
   },
 { test = "OPTIONS=runmode:foo",
   result = { { line = 1, error = "Unknown runmode parameter 'foo'" } },
   },
 { test = "OPTIONS=runmode",
   result = { { line = 1, error = "Value is mandatory for runmode" } },
   },
 { test = "OPTIONS=!runmode",
   result = {  },
   extra = function() return nh.get_config("runmode") == "teleport" end
   },
 { test = "OPTIONS=runmode:crawl\nOPTIONS=runmode:teleport",
   result = { { line = 2, error = "compound option specified multiple times: runmode" } }
   },
 { test = "OPTIONS=runmode:run,runmode:walk",
   result = { { line = 1, error = "compound option specified multiple times: runmode" } }
   },
};


function testtable(t1, t2)
   if type(t1) ~= type(t2) then return false end

   for k1, v1 in pairs(t1) do
      if type(v1) == "table" and type(t2[k1]) == "table" then
         if not testtable(v1, t2[k1]) then return false end
      else
         if v1 ~= t2[k1] then return false end
      end
   end

   return true
end



for k, v in pairs(configtests) do
   local cnf = configtests[k].test;
   local err = configtests[k].result;
   nh.parse_config(cnf);
   local res = nh.get_config_errors();

   if not testtable(err, res) then
      error("Config: Results don't match for test \"" .. cnf .. "\"");
   end

   if (type(configtests[k].extra) == "function") then
      if configtests[k].extra() == "false" then
         error("Config: Failed extra test.");
      end
   end

end
