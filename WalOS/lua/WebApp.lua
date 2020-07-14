--
-- Copyright (c) 2020, Bashi Tech. All rights reserved.
--
local turbo = require("turbo")
local log   = require("log")
local utils = require("Utils")
local json  = require("json")
local bs64  = require("base64")
local socket= require("socket")

local WebAppHandler = class("WebAppHandler", turbo.web.RequestHandler)
local ApiAppHandler = class("ApiAppHandler", turbo.web.RequestHandler)

local ConfCheckArray = {}
local UserCheckArray = {
    ["userid"]  = 32,
    ["name"]    = 24,
    ["desc"]    = 32,
    ["others"]  = -32,
    ["rule"]    = 0
}
local VisitorCheckArray = {
    ["vcid"]    = 0,
    ["visicode"]   = 8
}

function LoadConfCheckArray(basepath)
    local jstr = utils:ReadAll(basepath.."/data/default.json")
    if jstr ~= nil then
        local sets = json.decode(jstr)
        ConfCheckArray = sets.items
    end
end

function CheckConfValid(key)
    if ConfCheckArray == nil then return 0 end

    for idx,item in ipairs(ConfCheckArray) do
        if item.key == key then
            return item.options
        end
    end

    return 0
end

function checkValid(ks, kary)
    for k,v in pairs(kary) do

        if ks[k] == nil and v >= 0 then
            log:warn("checkValid => key not exist:"..k)
            return false
        end
        
        local len = v
        if v < 0 then len = -v end
        if len ~= 0 then
            if #(ks[k]) > len then
                log:warn("checkValid => value"..k.." len:"..tostring(len).."=>"..tostring(#(ks[k])))
                return false
            end
        end
    end

    return true
end

function CheckUserValid(user)
    return checkValid(user, UserCheckArray)
end

function CheckVisitorValid(visitor)
    return checkValid(visitor, VisitorCheckArray)
end

function WebAppHandler:Init(walos, ioloop)
    self.walos = walos
    self.ioloop  = ioloop
end

function WebAppHandler:put()
end

function WebAppHandler:delete()
end

function WebAppHandler:get()
end

function WebAppHandler:post()
end

function ApiAppHandler:Init(walos, ioloop)
    self.walos = walos
    self.ioloop  = ioloop
    
    self.tokens = {}

    LoadConfCheckArray(walos.BasePath)
end

function ApiAppHandler:CheckAuthen()
    local token = self:GetHeader("Token")
    local find = false

    -- FIXME : for test 
    if token == nil then return true end

    for idx,item in ipairs(self.tokens) do
        if (item.timestamp + 60*10) < os.time() then
            table.remove(self.tokens, idx)
        elseif item.token == token then
            item.timestamp = os.time()
            find = true
        end
    end

    return find
end

function ApiAppHandler:NewToken()
    local token = {}
    
    token.timestamp = os.time()
    token.token = utils:ECDSA_Sign(self.walos.devPrikey, self.walos:ConfGet("device-id"), tostring(token.timestamp))
    if token.token ~= nil then
        table.insert(self.tokens, token)
        return token
    end

    return nil
end

function ApiAppHandler:put(path)
    error(turbo.web.HTTPError(400, "PUT not support, path : "..(path or "")))
end

function ApiAppHandler:delete(path)
    error(turbo.web.HTTPError(400, "DELETE not support, path : /"..(path or "")))
end

function ApiAppHandler:get(path)
    self:RequestHandler(path)
end

function ApiAppHandler:post(path)
    self:RequestHandler(path)
end

function ApiAppHandler:GetHeader(key)
    return self.request.headers[key]
end

function ApiAppHandler:InfoRequestHandler()
    local rsp = {}

    rsp.retcode = 0

    rsp["manufacturer"]     = self.walos:ConfGet("manufacturer")
    rsp["device-info"]      = self.walos:ConfGet("device-info")
    rsp["device-id"]        = self.walos:ConfGet("device-id")
    rsp["linux-version"]    = self.walos:ConfGet("linux-version")
    rsp["firmware-version"] = self.walos:ConfGet("firmware-version")
    rsp["hardware-version"] = self.walos:ConfGet("hardware-version")
    rsp["protocol-version"] = self.walos:ConfGet("protocol-version")
   
    self:write(rsp)
end

function ApiAppHandler:AuthenRequestHandler()
    local rsp = {}
    rsp.retcode = utils.FDRSDK_ERRCODE_INVALID

    -- check valid request
    local req = self:get_json(true)
    if req == nil or req.action == nil then
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
        self:write(rsp)
        return
    end

    if req.action == "active" then
        local ts = req.timestamp
        local sg = req.signature
        local name = req.user
        local passwd = req.passwd

        if ts ~= nil and sg ~= nil and name ~= nil and passwd ~= nil then
            if utils:ECDSA_Verify(self.walos.cloudPubkey, self.walos:ConfGet("device-id"), ts, sg) then
                if self.walos.AdminInsert(name, passwd) then
                    local token = self:NewToken()
                    rsp.token = token.token
                    rsp.timestamp = token.timestamp
                    rsp.retcode = 0
                else
                    rsp.retcode = utils.FDRSDK_ERRCODE_DBOPS
                end
            else
                rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
            end
        end
    elseif req.action == "auth" then
        local name = req.user
        local passwd = req.passwd

        if name ~= nil and passwd ~= nil then
            local admin = self.walos.AdminLookup(name)
            if admin ~= nil then
                if admin.passwd == passwd then
                    local token = self:NewToken()
                    if token ~= nil then
                        rsp.token = token.token
                        rsp.timestamp = token.timestamp
                        rsp.retcode = 0
                    end
                end
            end
        end

    elseif req.action == "passwd" then

        if self:CheckAuthen() then
            local name = req.user
            local passwd = req.passwd

            if name ~= nil and passwd ~= nil then
                if self.walos.AdminUpdate(name, passwd) then
                    rsp.retcode = 0
                end
            end
        end
    elseif req.action == "delete" then
        if self:CheckAuthen() then
            local name = req.user

            if name ~= nil then
                if self.walos.AdminDelete(name) then
                    rsp.retcode = 0
                end
            end
        end
    else
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
    end

    self:write(rsp)
end

function ApiAppHandler:ConfRequestHandler()
    local rsp = {}
    rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
    rsp.params = {}

    if not self:CheckAuthen() then
        rsp.retcode = utils.FDRSDK_ERRCODE_TOKEN
        self:write(rsp)
        return
    end

    -- check valid request
    local req = self:get_json(true)
    if req == nil or req.action == nil or (req.action ~= "list" and type(req.params) ~= "table") then
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
        self:write(rsp)
        return
    end

    -- log:info("ConfRequestHandler => "..req.action)

    if req.action == "set" then
        for idx, item in ipairs(req.params) do
            for k,v in pairs(item) do
                if CheckConfValid(k) > 0 then
                    if self.walos:ConfSet(k, v) then
                        local r = {}
                        r[k] = v
                        table.insert(rsp.params, r)
                        rsp.retcode = 0
                    end
                end
            end
        end
    elseif req.action == "get" then
        for idx, key in ipairs(req.params) do
            local val = self.walos:ConfGet(key)
            if val ~= nil then
                local item = {}
                item[key] = val
                table.insert(rsp.params, item)
                rsp.retcode = 0
            end
        end
        
    elseif req.action == "list" then
        rsp.params = self.walos:ConfList()
        rsp.retcode = 0
    else
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
    end

    self:write(rsp)
end

function ApiAppHandler:UserRequestHandler()
    local rsp = {}
    rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
    rsp.params = {}

    if not self:CheckAuthen() then
        rsp.retcode = utils.FDRSDK_ERRCODE_TOKEN
        self:write(rsp)
        return
    end

    -- check valid request
    local req = self:get_json(true)
    if req == nil or req.action == nil then
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
        self:write(rsp)
        return
    end

    -- log:info("ApiAppHandler:UserRequestHandler => "..req.action or "nil")

    if req.action ~= "list" then
        if req.params == nil or type(req.params) ~= "table" then
            rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
            self:write(rsp)
            return
        end
    end

    if req.action == "list" then
        if type(req.offset) ~= "number" then req.offset = 0 end
        if type(req.limit) ~= "number" then req.limit = 10 end

        rsp.params = self.walos:UserFetch(req.offset, req.limit)
        rsp.retcode = 0
    elseif req.action == "add" then
        for idx,item in ipairs(req.params) do
            if CheckUserValid(item) then
                if self.walos:UserInsert(item) then
                    rsp.retcode = 0
                    table.insert(rsp.params, item.userid)
                end
            end
        end
    elseif req.action == "del" then
        for idx,item in ipairs(req.params) do
            if item.userid ~= nil then
                if self.walos:UserDelete(item.userid) then
                    rsp.retcode = 0
                    table.insert(rsp.params, item.userid)
                end
            end
        end
    elseif req.action == "update" then
        for idx,item in ipairs(req.params) do
            if item.userid ~= nil then
                if self.walos:UserUpdate(item) then
                    rsp.retcode = 0
                    table.insert(rsp.params, item.userid)
                end
            end
        end
    elseif req.action == "info" then
        for idx,item in ipairs(req.params) do
            if item.userid ~= nil then
                local user = self.walos:UserLookup(item.userid)
                if user ~= nil then
                    rsp.retcode = 0
                    table.insert(rsp.params, user)
                end
            end
        end
    else
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
    end
    
    self:write(rsp)
end

function ApiAppHandler:VisitorRequestHandler()
    local rsp = {}
    rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
    local params = {}

    if not self:CheckAuthen() then
        rsp.retcode = utils.FDRSDK_ERRCODE_TOKEN
        self:write(rsp)
        return
    end

    -- check request valid
    local req = self:get_json(true)
    if req == nil or req.params == nil or type(req.params) ~= "table" or req.action == nil then
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
        self:write(rsp)
        return
    end

    -- log:info("VisitorRequestHandler => ")

    if req.action == "add" then
        for idx,item in ipairs(req.params) do
            if CheckVisitorValid(item) then
                if self.walos:VisitorInsert(item) then
                    rsp.retcode = 0
                    table.insert(params, item.vcid)
                end
            end
        end

        rsp.params = params
    elseif req.action == "del" then
        for idx,item in ipairs(req.params) do
            if item.vcid ~= nil then
                if self.walos:VisitorDelete(item.vcid) then
                    rsp.retcode = 0
                    table.insert(params, item.vcid)
                end
            end
        end
        rsp.params = params
    else
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
    end

    self:write(rsp)
end

function ApiAppHandler:LogrRequestHandler()
    local rsp = {}
    rsp.retcode = utils.FDRSDK_ERRCODE_INVALID

    if not self:CheckAuthen() then
        rsp.retcode = utils.FDRSDK_ERRCODE_TOKEN
        self:write(rsp)
        return
    end
    
    -- check request valid
    local req = self:get_json(true)
    if req == nil or req.action == nil then
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
        self:write(rsp)
        log:warn("ApiAppHandler:LogrRequestHandler => get_json return nil")
        return
    end

    if req.action == "fetch" then
        if type(req.seqnum) ~= "number" then req.seqnum = 0 end
        if type(req.count) ~= "number" then req.count =  10 end

        local logger = self.walos:LogrFetch(req.seqnum, req.count)
        rsp.retcode = 0
        rsp.logger = logger
    elseif req.action == "info" then
        local max,min = self.walos:LogrInfo()

        rsp.retcode = 0
        rsp.max = max
        rsp.min = min
    end

    self:write(rsp)
end

function ApiAppHandler:DeviceRequestHandler()
    local rsp = {}
    rsp.retcode = utils.FDRSDK_ERRCODE_INVALID

    if not self:CheckAuthen() then
        rsp.retcode = utils.FDRSDK_ERRCODE_TOKEN
        self:write(rsp)
        return
    end

    -- check request valid
    local req = self:get_json(true)
    if req == nil or req.action == nil then
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
        self:write(rsp)
        return
    end

    if req.action == "reboot" then
        self.walos:DeviceReboot()
        rsp.retcode = 0
    elseif req.action == "reset" then
        self.walos:DeviceReset()
        rsp.retcode = 0
    elseif req.action == "snapshot" then
        rsp.timestamp = self.walos:DeviceSnapshot()
        rsp.retcode = 0
    elseif req.action == "upgrade" then
        if req.filename ~= nil then
            if self.walos:DeviceUpgrade(req.filename) then
                rsp.retcode = 0
            else
                rsp.retcode = utils.FDRSDK_ERRCODE_FILEOPS
            end
        end
    elseif req.action == "apply" then
        local conftype = req.conftype or "eth"  -- eth or wifi
        self.walos:DeviceApply(conftype)
    elseif req.action == "time" then
        if req.timestamp ~= nil then
            self.walos:DeviceSetHWTime(req.timestamp)
        end
    else
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
    end

    self:write(rsp)
end

function ApiAppHandler:UploadRequestHandler()
    local rsp = {}
    rsp.retcode = utils.FDRSDK_ERRCODE_INVALID

    if not self:CheckAuthen() then
        rsp.retcode = utils.FDRSDK_ERRCODE_TOKEN
        self:write(rsp)
        return
    end

    -- check request valid
    local req = self:get_json(true)
    if req == nil or req.action ~= "pushdata" or req.data == nil or type(req.filename) ~= "string"
        or type(req.offset) ~= "number" or type(req.size) ~= "number" or type(req.filesize) ~= "number" then
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
        self:write(rsp)
        return
    end

    if req.type == "upgrade" then
        if self.walos:DeviceWriteUpgrade(req.filename, req.offset, req.size, bs64.decode(req.data)) then
            rsp.retcode = 0
        else
            rsp.retcode = utils.FDRSDK_ERRCODE_FILEOPS
        end
    elseif req.type == "user" then
        if self.walos:UserWriteFile(req.filename, req.filesize, req.offset, req.size, bs64.decode(req.data)) then
            rsp.retcode = 0
        else
            rsp.retcode = utils.FDRSDK_ERRCODE_FILEOPS
        end
    elseif req.type == "test" then
        if self.walos:TestWriteFile(req.filename, req.offset, req.size, bs64.decode(req.data)) then
            rsp.retcode = 0
        else
            rsp.retcode = utils.FDRSDK_ERRCODE_FILEOPS
        end
    else
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
    end

    self:write(rsp)
end

function ApiAppHandler:DownloadRequestHandler()
    local rsp = {}
    rsp.retcode = utils.FDRSDK_ERRCODE_INVALID

    if not self:CheckAuthen() then
        rsp.retcode = utils.FDRSDK_ERRCODE_TOKEN
        self:write(rsp)
        return
    end

    -- check request valid
    local req = self:get_json(true)
    if req == nil or req.action ~= "pulldata" or type(req.offset) ~= "number" or type(req.size) ~= "number"
        or (req.type == "logrimage" and req.timestamp == nil)
        or (req.type == "snapshot" and req.timestamp == nil) then
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
        self:write(rsp)
        return
    end

    -- logr image, snapshot image, logr file
    if req.type == "logrimage" then
        log:info("DownloadRequestHandler -> "..tostring(req.timestamp))

        local data, size, fsize = self.walos:LogrReadImage(req.timestamp, req.offset, req.size)

        if data ~= nil then
            rsp.data = bs64.encode(data)
            rsp.offset = req.offset
            rsp.size = size
            rsp.filesize = fsize
            rsp.timestamp = req.timestamp
            rsp.type = req.type
            rsp.retcode = 0
        else
            rsp.retcode = utils.FDRSDK_ERRCODE_FILEOPS
        end
    elseif req.type == "snapshot" then
        local fname
        if req.chanel == nil or req.chanel == 0 then
            fname = "SNAP0-"..tostring(req.timestamp)..".nv12"
        else
            fname = "SNAP1-"..tostring(req.timestamp)..".nv12"
        end

        local data, size, fsize = self.walos:SnapshotRead(fname, req.offset, req.size)
        if data ~= nil then
            rsp.data = bs64.encode(data)
            rsp.timestamp = req.timestamp
            rsp.type = req.type
            rsp.chanel = req.chanel
            rsp.offset = req.offset
            rsp.size = size
            rsp.filesize = fsize
            rsp.retcode = 0
        else
            rsp.retcode = utils.FDRSDK_ERRCODE_FILEOPS
        end
    else
        rsp.retcode = utils.FDRSDK_ERRCODE_INVALID
    end

    self:write(rsp)
end

function ApiAppHandler:RequestHandler(path)

    -- log:info("RequestHandler => "..(path or ""))

    if path == "info" or path == "" or path == nil then
        self:InfoRequestHandler()
    elseif path == "auth" then
        self:AuthRequestHandler()
    elseif path == "config" then
        self:ConfRequestHandler()
    elseif path == "user" then
        self:UserRequestHandler()
    elseif path == "visitor" then
        self:VisitorRequestHandler()
    elseif path == "logger" then
        self:LogrRequestHandler()
    elseif path == "device" then
        self:DeviceRequestHandler()
    elseif path == "upload" then
        self:UploadRequestHandler()
    elseif path == "download" then
        self:DownloadRequestHandler()
    else
        error(turbo.web.HTTPError(400, "GET/POST not support, path : /"..(path or "")))
    end

end

local WebApp = {}

function WebApp:New(o)
    o = o or {}

    setmetatable(o, self)
    self.__index = self

    self.ADDR = "192.168.2.170"             -- all address
    self.ADDR_PORT = 8040

    return o
end

function WebApp:LogrEventHandler(eobj)

    log:info("WebApp:LogrEventHandler receive :"..json.encode(eobj))

    if self.addr ~= nil and self.port ~= nil then

        local url = "http://"..self.addr..":"..self.port.."/"..self.path
        local kwargs = {
            ["method"]          = "POST",
            ["max_redirects"]   = 1,
            ["request_timeout"] = 1,
            ["body"]            = json.encode(eobj)
        }

        local client = turbo.async.HTTPClient(nil, self.ioloop, 1024)
        local rsp = client:fetch(url, kwargs)
        -- log:info("WebApp:LogrEventHandler http fetch to :"..tostring(rsp))
        --[[
        local res = coroutine.yield(turbo.async.HTTPClient():fetch(url, kwargs))
        if res.error then
            log:warn("Could report to "..url)
        else
            log:info("report ok")
        end
        ]]
    end

end

function WebApp:Start(walos, ioloop)

    ApiAppHandler:Init(walos, ioloop)
    WebAppHandler:Init(walos, ioloop)
    
    self.walos = walos
    self.ioloop  = ioloop

    self.addr = walos:ConfGet("manager-server-address")
    self.port = walos:ConfGet("manager-server-port")
    self.path = walos:ConfGet("manager-server-path")

    self.webapp = turbo.web.Application({
        {"^/static/(.*)$",  turbo.web.StaticFileHandler, "./"},
        {"^/api/(.*)$",     ApiAppHandler   },
        {"^/webapp/(.*)$",  WebAppHandler   },
        -- {"^/$",             AuthenHandler}
    })

    log:info("WebApp start at : "..self.ADDR..":"..tostring(self.ADDR_PORT))

    self.webapp:set_server_name("WalOS")
    self.webapp:listen(self.ADDR_PORT, socket.INADDR_ANY)
end

return WebApp
