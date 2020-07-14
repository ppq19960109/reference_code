--
-- Copyright (c) 2020, Bashi Tech. All rights reserved.
--

local ffi   = require("ffi")
local log   = require("log")
local lfs   = require("lfs")

local LogrGen = {}

function LogrGen:New(o)
    o = o or {}

    setmetatable(o, self)
    self.__index = self

    return o
end

function LogrGen:Dump(logr)
    if logr == nil then
        return 
    end

    log:info("---------------")
    log:info("  logr.seqnum:",  logr.seqnum)
    log:info("  logr.occtime:", logr.occtime)

    log:info("  logr.face_x:", logr.face_x)
    log:info("  logr.face_y:", logr.face_y)
    log:info("  logr.face_w:", logr.face_w)
    log:info("  logr.face_h:", logr.face_h)

    log:info("  logr.sharp:", logr.sharp)
    log:info("  logr.score:", logr.score)
    log:info("  logr.bbt:",   logr.bbt)

    log:info("  logr.userid:"..logr.userid)
    log:info("---------------")
end

function LogrGen:NewLogr()
    local logr = {}

    logr.occtime    = math.floor(os.time()*1000 + (math.random()*1000))
    logr.face_x     = math.random() * 100
    logr.face_y     = math.random() * 100
    logr.face_w     = math.random() * 1000
    logr.face_h     = math.random() * 1000

    logr.sharp      = 0.98626
    logr.score      = 0.8600
    logr.bbt        = 37.5

    logr.userid     = "USERID0X928="

    return logr
end

function LogrGen:NewData(BasePath)
    local f = io.open(BasePath.."/data/test.jpg")
    if f ~= nil then
        local d = f:read("a")
        f:close()
        f = nil
    
        return d
    else
        local data = "nil"
        local size = 8*1024
        for i = 1, size do
            data = data..tostring(math.random())
        end
        data = data.."nil"

        return data
    end
end

function LogrGen:WriteLogrImage(BasePath, timestamp, data)
    log:info("WriteLogrImage -> "..tostring(timestamp))
    
    local seconds = math.floor(timestamp/1000)
    local path = BasePath..os.date("/data/logr/%Y%m/", seconds)
    lfs.mkdir(path)

    path = BasePath..os.date("/data/logr/%Y%m/%d/", seconds)
    lfs.mkdir(path)

    path = path..tostring(timestamp)..".jpg"
    
    log:info("WriteLogrImage -> "..path)
    local f = io.open(path, "w")
    if f == nil then return nil, nil, nil end

    f:seek("set", 0)
    local r = f:write(data)
    f:close()

    if r == nil then
        log:info("WriteLogrImage -> fail")
        return false 
    else
        log:info("WriteLogrImage -> ok")
        return true
    end

end

function LogrGen:Gen(BasePath, ldb, count)

    local  data = self:NewData(BasePath)

    for i = 1, count do
        local lr = self:NewLogr()

        if ldb:Append(lr) then
            log:info("Append Success:"..tostring(i))
        else
		    log:warn("Append Fail:"..tostring(i))
        end

        self:Dump(lr)
        self:WriteLogrImage(BasePath, lr.occtime, data)
    end
end

return LogrGen
