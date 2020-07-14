--
-- Copyright (c) 2020, Bashi Tech. All rights reserved.
--

-- package.path = "../template/modules/?.lua;./?.lua;"
-- package.cpath = "../templage/macosx-lib/?.so;./?.so;"

--local utils = require("Utils")
--local turbo = require("turbo")


function WriteFile(fname, offset, data)
    local f = io.open(fname, "r+")
    if f == nil then 
        print("Walnuts:WriteFile -> open "..fname.."  fail")
        return false
    end

    f:seek("set", offset)
    local r = f:write(data)
    f:close()

    if r == nil then
        print("Walnuts:WriteFile -> write "..fname.."  fail")
        return false 
    end

    return true
end

-- local str = "SNAP0-159388373.nv12"
-- local num = string.match(str, "[0-9]+", 6)
-- print(num)
local timestamp = os.time()*1000 + 34 - 20*60*1000
local sec = math.floor(timestamp/1000)
local msec = math.fmod(timestamp, 1000)
local path = os.date("/data/logr/%Y%m%d/%H%M%S.", sec)..string.format("%03d.jpg", msec)
print(path)

