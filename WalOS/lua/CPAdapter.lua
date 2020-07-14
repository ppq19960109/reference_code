--
-- Copyright (c) 2020, Bashi Tech. All rights reserved.
--

local log = require("log")
local utils = require("Utils")

local ffi = require("ffi")

ffi.cdef [[
    int user_append(const char *uid, const char *uname, const char *udesc, int rule, long expired, const void *feature);
    int user_update(const char *uid, const char *uname, const char *udesc, int rule, long expired, const void *feature);
    int user_delete(const char *uid);

    int user_genfeature(const void *nvdata, int nvsize, void *feature, int fsize);
    int user_genui(const char *file, const void *nvdata, int nvsize);

    int config_set(const char *key, const char *val);

    int visitor_insert(int vcid, const char *vcode, long start, long expired);
    int visitor_delete(int vcid);

    int cpa_snapshot(const char *path, double timestamp);
    int cpa_start();
]]

CPAdapter = {
}

function CPAdapter:New(o)
    o = o or {}

    setmetatable(o, self)
    self.__index = self

    return o
end

function CPAdapter:UserAppend(user)
    -- log:info("CPAdapter:UserAppend => "..user.userid.."  feature : "..(user.feature and "have") or "nil")
    -- utils:DumpTable(user)

    return ffi.C.user_append(user.userid, user.name, user.desc, user.rule, user.expired, user.feature)
end

function CPAdapter:UserUpdate(user)
    -- log:info("CPAdapter:UserUpdate")
    -- utils:DumpTable(user)

    return ffi.C.user_update(user.userid, user.name, user.desc, user.rule, user.expired, user.feature)
end

function CPAdapter:UserDelete(userid)
    -- log:info("CPAdapter:UserDelete:"..userid)

    return ffi.C.user_delete(userid)
end

function CPAdapter:UserGenFeature(nvdata, nvsize, feature, fsize)
    -- log:info("CPAdapter:UserGenFeature ")
    return ffi.C.user_genfeature(nvdata, nvsize, feature, fsize)
end

function CPAdapter:UserGenUI(fui, nvdata, nvsize)
    -- log:info("CPAdapter:UserGenUI ")
    return ffi.C.user_genui(fui, nvdata, nvsize)
end

function CPAdapter:VisitorInsert(vcid, visicode, started, expired)
    --log:info("CPAdapter:VisitorInsert:"..tostring(vcid).." code:"..visicode.." start:"..tostring(started).." expired:"..tostring(expired))
    return ffi.C.visitor_insert(vcid, visicode, started, expired)
end

function CPAdapter:VisitorDelete(vcid)
    --log:info("CPAdapter:VisitorDelete:"..tostring(vcid))

    return ffi.C.visitor_delete(vcid)
end

function CPAdapter:ConfSet(keystr, valstr)
    --log:info("CPAdapter:ConfSet key:"..keystr.."  val:"..valstr)

    return ffi.C.config_set(keystr, valstr)
end

function CPAdapter:SnapShot(path, timestamp)
    --log:info("CPAdapter:SnapShot:"..path)
    --log:info("CPAdapter:SnapShot at:"..tostring(timestamp))

    if ffi.C.cpa_snapshot(path, timestamp) == 0 then
        return true
    else
        return false
    end
end


function CPAdapter:Start()
    log:info("CPAdapter:Start")

    return ffi.C.cpa_start()
end

return CPAdapter
