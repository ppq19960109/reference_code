--
-- Copyright (c) 2020, Bashi Tech. All rights reserved.
--

local ffi       = require("ffi")
local log       = require("log")
local utils     = require("Utils")
local json      = require("json")
local bs64      = require("base64")

local socket    = require("socket")
local turbo     = require("turbo")
local lfs       = require("lfs")

local cpa       = require("CPAdapter")
local osa       = require("OSAdapter")

local cApp      = require("CloudApp")
local wApp      = require("WebApp")

local bdb       = require("Basedb")
local ldb       = require("Logrdb")

local ioloop = turbo.ioloop.instance()

local WalOS = {
    Basedb = nil,
    Logrdb = nil,
    CPAdapter = nil,
    OSAdapter = nil,
    PubKey = nil,
    PriKey = nil,
    BasePath = nil,

    DISCOVERY_SERVER_PORT   = 6543,
    EVENT_SERVER_PORT       = 6544,
    INTERVAL_SCHEDULED_TASKS= 1000 * 60 * 60, -- interval is 1 hour
}

--[[
    WalOS object
]]

function WalOS:New(o)
    o = o or {}

    setmetatable(o, self)
    self.__index = self

    return o
end

function WalOS:InitCP()
    local offset = 0
    local limit = 10
    local num = 0
    local users = {}
    local visitors = {}

    -- load user to control plane
    repeat
        users, num = self.Basedb:UserFetch(offset, limit)
        if users == nil then break end

        for i,v in ipairs(users) do
            local user = self.Basedb:UserLookup(v)
            if user ~= nil then
                user.feature = self.Basedb:FeatureLookup(v)
                if user.feature ~= nil then
                    self.CPAdapter:UserAppend(user)
                end
            end
        end

        offset = offset + limit
    until num == limit

    -- load visitor to control plane
    offset = 0
    repeat
        visitors,num = self.Basedb:VisitorFetch(offset, limit)
        if visitors == nil then break end

        for i,v in ipairs(visitors) do
            local visitor = self.Basedb:VisitorLookup(v)
            if visitor ~= nil then
                self.CPAdapter:VisitorInsert(visitor.vcid, visitor.vcode, visitor.create, visitor.expire)
            end
        end
    until num == limit

    -- FIXME : load rule to control plane
    -- FIXME : load advertising to control plane
end

function WalOS:Init()
    local ret

    log:info("WalOS Init ...")

    -- init adapter of Control plane & OS
    self.CPAdapter = cpa:New()
    self.OSAdapter = osa:New()

    -- open basedb
    self.Basedb = bdb:New()
    ret = self.Basedb:Open(self.BasePath.."/data/basedb.sdb")
    if not ret then
        log:fatal("WalOS open base db fail:"..self.BasePath.."/data/basedb.sdb")
        self:Fini()
        return false
    end

    -- open logrdb
    self.Logrdb = ldb:New()
    ret = self.Logrdb:Open(self.BasePath.."/data/logrdb.sdb")
    if not ret then
        log:fatal("WalOS open base db fail:"..self.BasePath.."/data/logr.sdb")
        self:Fini()
        return false
    end


    -- load default.json in first startup
    if self.Basedb:ConfGet("device-id") == nil then
        local dconf = utils:ReadAll(self.BasePath.."/data/default.json")
        if dconf == nil then
            log:fatal("WalOS read default config:"..self.BasePath.."/data/default.json fail")
            self:Fini()
            return false
        end

        local dj = json.decode(dconf)
        dconf = nil
        dconf = dj.items

        for i, item in ipairs(dconf) do
            local k = item["key"]
            local v = item["value"]
            local o = item["options"] or 0
            
            -- init basedb & set control plane settings
            self:ConfSet(k, v, o)
        end
    else
        local sl = self:ConfList()
        for i, item in ipairs(sl) do
            local k = item["key"]
            local v = item["value"]
            local o = item["options"] or 0

            -- set control plane settings
            self:ConfSet(k, v, o)
        end
    end

    -- init cp data => user feature repo & visitor
    self:InitCP()

    -- FIXME : KEY Should return by C interface
    -- load ecc pub & pri key
    local file = io.open(self.BasePath.."/data/pubkey.ecc", "rb")
    self.PubKey = file:read("a");
    file:close();
    if self.PubKey == nil then
        log:fatal("WalOS open pubkey fail:"..self.BasePath.."/data/pubkey.ecc")
        self:Fini()
        return false
    end

    file = io.open(self.BasePath.."/data/prikey.ecc", "rb")
    self.PriKey = file:read("a");
    file:close();
    if self.PriKey == nil then
        log:fatal("WalOS open prikey fail:"..self.BasePath.."/data/prikey.ecc")
        self:Fini()
        return false
    end

    self.devPubkey = self.PubKey
    self.devPrikey = self.PriKey
    self.cloudPubkey = self.PubKey

    return true
end

function WalOS:Fini()
    -- log:info("WalOS Fini ...")

    if self.Basedb ~= nil then
        self.Basedb:Close()
    end

    if self.Logrdb ~= nil then
        self.Logrdb:Close()
    end
end

function WalOS:EventHandler(evObj)
    if evObj.type == "logr" then
        
        -- append event to logrdb

        if self.Logrdb:Append(evObj) then
            evObj.seqnum = self.Logrdb:MaxSequence()
            --  log:info("EventServerHandler seqnum :"..tostring(evObj.seqnum))

            local em = self:ConfGet("event-mode")
            if em == nil or tonumber(em) == 0 then
                return
            end
            local mode = self:ConfGet("device-mode")
            
            if (mode == "cloud" or mode == "mixed") and self.cApp ~= nil then
                self.cApp:LogrEventHandler(evObj)
            end

            if (mode == "local" or mode == "mixed") and self.wApp ~= nil then
                self.wApp:LogrEventHandler(evObj)
            end
        end
    end
end

function EventServerHandler(Wn)
    local str = Wn.EventsServer:receive()
    local obj = json.decode(str)

    -- log:info("EventServerHandler receive :"..str)
    
    if obj.type == nil then
        log:warn("EventServerHandler receive wrong:"..str)
        return
    end

    Wn:EventHandler(obj)
end

function WalOS:StartEventsServer(ioloop)
    self.EventsServer = socket.udp()

    -- log:info("StartEventsServer ...")

    self.EventsServer:setsockname("127.0.0.1", self.EVENT_SERVER_PORT)
    self.EventsServer:settimeout(1)
    
    if turbo.platform.__DARWIN__ then
        ioloop:add_handler(self.EventsServer, turbo.ioloop.READ, EventServerHandler, self)
    elseif turbo.platform.__LINUX__ then
        ioloop:add_handler(self.EventsServer:getfd(), turbo.ioloop.READ, EventServerHandler, self)
    else
        ioloop:add_handler(self.EventsServer, turbo.ioloop.READ, EventServerHandler, self)
    end
end

function DiscoverServerHandler(Wn)
    local str, ip, port = Wn.DiscoverServer:receivefrom()
    local obj = json.decode(str)

    -- log:info("DiscoverServerHandler => "..str)
    
    if (obj.action ~= nil) and (obj.action == "scan") then
        local disc = {}

        disc["device-id"] = Wn.Basedb:ConfGet("device-id")
        disc["protocol-version"] = Wn.Basedb:ConfGet("protocol-version")
        Wn.DiscoverServer:sendto(json.encode(disc), ip, port)

        -- log:info("DiscoverServerHandler from : "..ip..":"..tostring(port))
    else
        log:warn("DiscoverServerHandler receive wrong:"..str)
    end
end

function WalOS:StartDiscoverServer(ioloop)
    self.DiscoverServer = socket.udp()

    self.DiscoverServer:setsockname('*', self.DISCOVERY_SERVER_PORT)
    self.DiscoverServer:settimeout(1)
    
    if turbo.platform.__DARWIN__ then
        ioloop:add_handler(self.DiscoverServer, turbo.ioloop.READ, DiscoverServerHandler, self)
    elseif turbo.platform.__LINUX__ then
        ioloop:add_handler(self.DiscoverServer:getfd(), turbo.ioloop.READ, DiscoverServerHandler, self)
    else
        ioloop:add_handler(self.EventsServer, turbo.ioloop.READ, EventServerHandler, self)
    end
end

function ScheduledTasks(Wn)
    Wn:LogrClean()
    Wn:VisitorClean()
    Wn:SnapshotClean()
end

function WalOS:StartScheduledTasks(ioloop)
    local interval = self.INTERVAL_SCHEDULED_TASKS
    ioloop:set_interval(interval, ScheduledTasks, self)
end

function  WalOS:Run()

    local mode = self:ConfGet("device-mode")

    if mode == "cloud" or mode == "mixed" then
        -- start cloud app
        self.cApp = cApp:New()
        self.cApp:Start(self, ioloop)
    else
        self.cApp = nil
    end

    if mode == "local" or mode == "mixed" then
        -- start web app
        self.wApp = wApp:New()
        self.wApp:Start(self, ioloop)
    else
        self.wApp = nil
    end

    -- start event server
    self:StartEventsServer(ioloop)

    -- start discovering server
    self:StartDiscoverServer(ioloop)

    -- start Scheduled Tasks : clean visitor table, logr images, snapshots ...
    self:StartScheduledTasks(ioloop)

    -- if activated, start CP
    local act = self:ConfGet("activation")
    if tonumber(act) > 0 then
        self.CPAdapter:Start()
    end

    ioloop:start()
end

function WalOS:ReadFile(fname, offset, size)
    local f = io.open(fname, "r")
    if f == nil then
        log:warn("WalOS:ReadFile -> open "..fname.."  fail")
        return nil, nil, nil
    end

    local fsize = f:seek("end")
    f:seek("set", offset)
    local data = f:read(size)
    f:close()

    if data == nil then
        log:warn("WalOS:WriteFile -> write "..fname.."  fail")
        return nil, nil, nil 
    end

    return data, #data, fsize
end

function WalOS:WriteFile(fname, offset, data)
    local f

    if lfs.attributes(fname) ~= nil then
        f = io.open(fname, "r+")
    else
        f = io.open(fname, "w")
    end

    if f == nil then 
        log:warn("WalOS:WriteFile -> open "..fname.."  fail")
        return false
    end

    f:seek("set", offset)
    local r = f:write(data)
    f:close()

    if r == nil then
        log:warn("WalOS:WriteFile -> write "..fname.."  fail")
        return false
    end

    return true
end

-- logr db & file ops
function WalOS:LogrReadImage(timestamp, offset, size)
    log:info("LogrReadImage -> "..tostring(timestamp))
    
    local sec = math.floor(timestamp/1000)
    local msec = math.fmod(timestamp, 1000)

    log:info("LogrReadImage -> "..tostring(timestamp))

    local path = self.BasePath..os.date("/data/logr/%Y%m%d/%H%M%S.", sec)..string.format("%03d.jpg", msec)
 
    -- log:info("LogrReadImage -> "..path)
    return self:ReadFile(path, offset, size)
end

function WalOS:LogrReadFile(timestamp, offset, size)
    -- log:info("LogrReadFile -> "..tostring(timestamp))

    local seconds = math.floor(timestamp/1000)
    local path = self.BasePath..os.date("/data/logr/%Y%m%d", seconds)..".log"

    -- read first 32kB
    return self:ReadFile(path, offset, size)
    -- log:info("LogrReadFile -> "..path)
end

function WalOS:LogrFetch(seqnum, count)
    local params = {}

    if seqnum == nil or seqnum == 0 or type(seqnum) ~= "number" then
        seqnum = self.Logrdb:MinSequence()
    end

    if count == nil or count < 0 or count > 10 or type(count) ~= "number" then
        count = 10
    end

    for i = seqnum, seqnum + count do
        local lr = self.Logrdb:Lookup(i)

        if lr ~= nil then
            table.insert(params, lr)
        else
            break
        end
    end

    return params
end

function WalOS:LogrInfo()
    return self.Logrdb:MaxSequence(), self.Logrdb:MinSequence()
end

function WalOS:LogrClean()
    local max, min = self:LogrInfo()
    local logrmax = self:ConfGet("user-logr-max")
    local count = tonumber(logrmax)

    if (max - min) > count then
        self.Logrdb:Clean(max - count)
    end
end

-- Test ops
function WalOS:TestWriteFile(fname, offset, size, data)
    -- log:info("LogrInfo -> ")

    local path = self.BasePath.."/data/test/"
    lfs.mkdir(path)
    return self:WriteFile(path..fname, offset, data)
end

-- Device ops
function WalOS:DeviceWriteUpgrade(fname, offset, size, data)
    -- log:info("DeviceWriteUpgrade -> "..fname)
    local path = self.BasePath.."/data/upgrade/"

    lfs.mkdir(path)

    return self:WriteFile(path..fname, offset, data)
end

function WalOS:DeviceUpgrade(fname)
    -- log:info("DeviceUpgrade -> "..fname)

    local path = self.BasePath.."/data/upgrade/"..fname
    local attr = lfs.attributes(path)
    if attr ~= nil then
        self.OSAdapter:Upgrade(path)
    else
        log:warn("DeviceUpgrade -> file not exist:"..fname)
    end

    return true
end

function WalOS:DeviceReset()
    -- log:info("DeviceReset -> ")
    local path = self.BasePath.."/data/"
    self.OSAdapter:Reset(path)
    return true
end

function WalOS:DeviceReboot()
    -- log:info("DeviceReboot -> ")
    self.OSAdapter:Reboot()
    return true
end

function WalOS:DeviceActivate()
    -- log:info("DeviceActivate -> ")
    self:ConfSet("activation", "1")
    self.CPAdapter:Start()
    return true
end

function WalOS:DeviceSetHWTime(timestamp)
    -- log:info("DeviceSetHWTime -> ")

    self.OSAdapter:SetSystemTime(timestamp)
    return true
end

function WalOS:DeviceApply(ctype)
    -- log:info("DeviceApply -> "..conftype)
    local cbody = {}

    if ctype == "eth" then
        cbody["ether-ipv4-address-mode"]    = self:ConfGet("ether-ipv4-address-mode")
        cbody["ether-ipv4-address"]         = self:ConfGet("ether-ipv4-address")
        cbody["ether-ipv4-mask"]            = self:ConfGet("ether-ipv4-mask")
        cbody["ether-ipv4-gw"]              = self:ConfGet("ether-ipv4-gw")
        cbody["ether-dns-server"]           = self:ConfGet("ether-dns-server")
    elseif ctype == "wifi" then
        cbody["wifi-ipv4-address-mode"] = self:ConfGet("wifi-ipv4-address-mode")
        cbody["wifi-ipv4-address"]      = self:ConfGet("wifi-ipv4-address")
        cbody["wifi-ipv4-mask"]         = self:ConfGet("wifi-ipv4-mask")
        cbody["wifi-ipv4-gw"]           = self:ConfGet("wifi-ipv4-gw")
        cbody["wifi-dns-server"]        = self:ConfGet("wifi-dns-server")
    else
        log:warn("WalOS:DeviceApply type unkown :"..ctype)
    end

    self.OSAdapter:Apply(ctype, cbody)

    return true
end

function WalOS:DeviceSnapshot()
    -- log:info("Snapshot -> ")

    local path = self.BasePath.."/data/snapshots/"
    lfs.mkdir(path)
    
    local ts = os.time()
    if self.CPAdapter:SnapShot(path, ts) then
        return tostring(ts)
    else
        return nil
    end
end

function WalOS:SnapshotRead(fname, offset, size)
    -- log:info("SnapshotRead -> "..fname)

    local path = self.BasePath.."/data/snapshots/"..fname
    
    return self:ReadFile(path, offset, size)
end

function WalOS:SnapshotClean()
    local path = self.BasePath.."/data/snapshots/"

    for filename in lfs.dir(path) do
        if filename ~= "." and filename ~= ".." then
            -- log:info("SnapshotClean => "..filename)
            -- snapshots file name : SNAP0-159388373.nv12
            local ts = string.match(filename, "[0-9]+", 6)      -- skip chanel id
            if ts ~= nil then
                local t = tonumber(ts)

                if (t + 60*60) < os.time() then
                    -- snapshots 1 hour ago, delete
                    os.remove(path..filename)
                end
            end
        end
    end
end

-- User ops
function WalOS:UserInsert(user)
    -- log:info("UserInsert -> "..user.userid)

    if self.Basedb:UserInsert(user) then
        if user.feature ~= nil then
            user.feature = bs64.decode(user.feature)
            self.Basedb:FeatureInsert(user.userid, user.algmv, user.feature)
        end

        -- append user to control plane
        self.CPAdapter:UserAppend(user)
        return true
    end

    return false
end

function WalOS:UserDelete(userid)
    --log:info("UserDelete -> "..userid)

    if not self.Basedb:UserDelete(userid) then
        return false
    end

    self.Basedb:FeatureDelete(userid)
    self:UserDeleteFile(userid)

    -- delete user to control plane
    self.CPAdapter:UserDelete(userid)

    return true
end

function WalOS:UserLookup(userid)
    --log:info("UserLookup -> "..userid)

    return self.Basedb:UserLookup(userid)
end

function WalOS:UserUpdate(user)
    --log:info("UserUpdate -> "..user.userid)

    if self.Basedb:UserUpdate(user) then
        if user.feature ~= nil then
            user.feature = bs64.decode(user.feature)
            self.Basedb:FeatureUpdate(user.userid, user.algmv, user.feature)
        end

        -- update user to control plane
        self.CPAdapter:UserUpdate(user)
        return true
    end

    return false
end

function WalOS:UserFetch(offset, limit)
    --log:info("UserFetch -> "..tostring(offset)..","..tostring(limit))

    return self.Basedb:UserFetch(offset, limit)
end

-- fname = userid.[nv12/bmp/au]
function WalOS:UserWriteFile(fname, fsize, offset, size, data)
    local ret = false

    -- subdir 
    local pos = string.find(fname, ".", -6, true)
    if pos == nil or pos < 2 then
        log:warn("UserWriteFile -> filename without surfix:"..fname)
        return false
    end

    local userid = string.sub(fname, 1, pos - 1)
    local total = 0
    for i = 1, #userid do
        total = total + string.byte(userid, i)
    end
    local subdir = math.fmod(total, 100)

    local path = self.BasePath.."/data/users/"..tostring(subdir)
    lfs.mkdir(path)
    ret = self:WriteFile(path.."/"..fname, offset, data)

    -- if nv12 and last slice, update feature db & gen ui
    if (fsize == (offset + size)) and (string.find(fname, ".nv12") ~= nil) then
        local fd, fs, sz = self:ReadFile(path.."/"..fname, 0, fsize)
        if sz == fsize then
            local buf = ffi.new("float[?]", utils.ALGM_FEATURE_FLOAT_SIZE)
            if self.CPAdapter:UserGenFeature(fd, sz, buf, utils.ALGM_FEATURE_BUFF_SIZE) == 0 then
                local fstr = ffi.string(buf, utils.ALGM_FEATURE_BUFF_SIZE)
                if self.Basedb:FeatureLookup(userid) then
                    ret = self.Basedb:FeatureUpdate(userid, nil, fstr)
                else
                    ret = self.Basedb:FeatureInsert(userid, nil, fstr)
                end
            else
                ret = false
            end
            local fui = path.."/"..fname
            fui = string.gsub(fui, ".nv12", ".bmp")
            self.CPAdapter:UserGenUI(fui, fd, sz)
        else
            log:warn("UserWriteFile => read .nv12 fail")
            ret = false
        end
    end

    return ret
end

function WalOS:UserReadFile(fname, offset, size)
    -- log:info("UserReadFile -> "..fname)

    -- subdir 
    local pos = string.find(fname, ".", -4, true)
    if pos == nil or pos < 2 then
        log:warn("UserReadFile -> filename without surfix:"..fname)
        return false
    end

    local userid = string.sub(fname, 1, pos - 1)
    local total = 0
    for i = 1, #userid do
        total = total + string.byte(userid, i)
    end
    local subdir = math.fmod(total, 100)

    local path = self.BasePath.."/data/users/"..tostring(subdir).."/"..fname
    return self:ReadFile(path, offset, size)
end

function WalOS:UserDeleteFile(userid)
    -- log:info("UserDeleteFile -> "..userid.."   len:"..tostring(#userid))

    -- subdir 
    local total = 0
    for i = 1, #userid do
        total = total + string.byte(userid, i)
    end
    local subdir = math.fmod(total, 100)

    local path = self.BasePath.."/data/users/"..tostring(subdir).."/"..userid..".*"
    os.execute("rm -f "..path)
    return true
end

-- visitor ops
function WalOS:VisitorInsert(visitor)
    -- log:info("VisitorAdd -> "..visitor.vcid)
    
    if self.Basedb:VisitorInsert(visitor.vcid, visitor.visicode, visitor.started, visitor.expired) then
        self.CPAdapter:VisitorInsert(visitor.vcid, visitor.visicode, visitor.started, visitor.expired)
        return true
    end

    return false
end

function WalOS:VisitorDelete(vcid)
    -- log:info("VisitorDel -> "..vcid)
    
    self.Basedb:VisitorDelete(vcid)
    self.CPAdapter:VisitorDelete(vcid)

    return true
end

function WalOS:VisitorClean()
    -- log:info("VisitorClean ...")
    self.Basedb:VisitorClean()
end

-- config ops
function WalOS:ConfSet(key, val, opts)
    -- log:info("ConfSet -> k:"..key..", v:"..val)

    local ret, o = self.Basedb:ConfSet(key, val, opts)

    if ret then 
        if o > 1 then
            self.CPAdapter:ConfSet(key, val)
        end

        return true
    end

    return false
end

function WalOS:ConfGet(key)
    -- log:info("ConfGet -> "..key)
    return self.Basedb:ConfGet(key)
end

function WalOS:ConfList()
    return self.Basedb:ConfList()
end

function WalOS:AdminInsert(name, passwd, acl)
    return self.Basedb:AdminInsert(name, passwd, acl)
end

function WalOS:AdminLookup(name)
    return self.Basedb:AdminLookup(name)
end

function WalOS:AdminUpdate(name, passwd, acl)
    return self.Basedb:AdminUpdate(name, passwd, acl)
end

function WalOS:AdminDelete(name)
    return self.Basedb:AdminDelete(name)
end

--[[
    call from WalOS program
]]
function ErrorHandler(errStr)
    print(string.format("\27[35m[FATAL %s] %s \27[0m", os.date("%H:%M:%S"), "---------ErrorHandler---------"))
    log:fatal(tostring(errStr))
    log:fatal(debug.traceback())
    print(string.format("\27[35m[FATAL %s] %s \27[0m", os.date("%H:%M:%S"), "---------EndofHandler---------"))
    os.exit(false, true)
end

function SIGINTHandler(signum, siginfo)
    log:info("CTRL-C, exit!")

    ioloop:close()
    WalOS:Fini()
end


function Startup(basepath)
    local wn = WalOS:New()

    wn.BasePath = basepath
    -- ioloop:add_signal_handler(turbo.SIGINT, SIGINTHandler)

    log:info("Startup => "..basepath)

    lfs.mkdir(basepath.."/data")
    lfs.mkdir(basepath.."/data/users")
    lfs.mkdir(basepath.."/data/logr")
    lfs.mkdir(basepath.."/data/snapshots")

    wn:Init()
    wn:Run()
    wn:Fini()
end

