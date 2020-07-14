--
-- Copyright (c) 2020, Bashi Tech. All rights reserved.
--
local ffi = require("ffi")

local log   = require("log")
local json  = require("json")
local bs64  = require("base64")
local utils = require("Utils")
local turbo = require("turbo")

local BCPTypes = {
    BCP_PMT_INFO_REQ    = 0,
    BCP_PMT_INFO_RSP    = 1,
    BCP_PMT_AUTH_REQ    = 2,
    BCP_PMT_AUTH_RSP    = 3,
    BCP_PMT_USER_REQ    = 4,
    BCP_PMT_USER_RSP    = 5,
    BCP_PMT_CONF_REQ    = 6,
    BCP_PMT_CONF_RSP    = 7,
    BCP_PMT_LOGGER_REQ  = 8,
    BCP_PMT_LOGGER_RSP  = 9,
    BCP_PMT_GUEST_REQ   = 10,
    BCP_PMT_GUEST_RSP   = 11,
    BCP_PMT_EVENT_REQ   = 12,
    BCP_PMT_EVENT_RSP   = 13,
    BCP_PMT_PUSHDATA_REQ    = 14,
    BCP_PMT_PUSHDATA_RSP    = 15,
    BCP_PMT_PULLDATA_REQ    = 16,
    BCP_PMT_PULLDATA_RSP    = 17,
    BCP_PMT_DATATRANS_REQ   = 18,
    BCP_PMT_DATATRANS_RSP   = 19,
    BCP_PMT_DEVICE_REQ      = 20,
    BCP_PMT_DEVICE_RSP      = 21,
    BCP_PMT_HEARTBEAT_REQ   = 22,
    BCP_PMT_HEARTBEAT_RSP   = 23
}

local CloudApp = {
    ioLoop     = nil,
    walos    = nil,
    Connection = nil,
    LastHeader = nil,
    HeaderLen  = nil,
    WriteQueue = nil,
    Handlers   = nil, 
    State      = "idle"     -- idle, connected, authened
}

function HandlerUserRequest(ca, body)
    local wn = ca.walos
    local rsp = { retcode = 0, params = {}}

    if body.action == nil or (body.action ~= "list" and body.params == nil) then
        log:warn("HandlerUserRequest -> request invalid")
        return { retcode = utils.FDRSDK_ERRCODE_INVALID }
    end

    if body.action == "add" then
        for i,v in ipairs(body.params) do
            if wn:UserInsert(v) then
                table.insert(rsp.params, { userid = v.userid, ops =0 } )
            else
                table.insert(rsp.params, { userid = v.userid, ops = utils.FDRSDK_ERRCODE_DBOPS } )
            end
        end
    elseif body.action == "del" then
        for i,v in ipairs(body.params) do
            if wn:UserDelete(v.userid) then
                table.insert(rsp.params, { userid = v.userid, ops = 0 })
            else
                table.insert(rsp.params, { userid = v.userid, ops = utils.FDRSDK_ERRCODE_DBOPS })
            end
        end
    elseif body.action == "info" then
        for i,v in ipairs(body.params) do
            local u = wn:UserLookup(v.userid)
            -- FIXME : returen user feature ??
            if u ~= nil then
                u.ops = 0
                table.insert(rsp.params, u)
            else
                table.insert(rsp.params, { userid = v.userid, ops = utils.FDRSDK_ERRCODE_DBOPS })
            end
        end
    elseif body.action == "update" then
        for i,v in ipairs(body.params) do
            if wn:UserUpdate(v) then
                table.insert(rsp.params, { userid = v.userid, ops = 0 })
            else
                table.insert(rsp.params, { userid = v.userid, ops = utils.FDRSDK_ERRCODE_DBOPS})
            end
        end
    elseif body.action == "list" then
        local ids, nr = wn:UserFetch(body.offset, body.limit)
        if nr > 0 then
            -- utils:DumpTable(ids)
            for i,v in ipairs(ids) do
                table.insert(rsp.params, { userid = v })
            end
        else
            rsp.retcode = utils.FDRSDK_ERRCODE_DBOPS
        end
    else
        return { retcode = utils.FDRSDK_ERRCODE_INVALID }
    end

    return rsp
end

function HandlerLogrRequest(ca, body)
    local wn = ca.walos
    local rsp = { retcode = 0, params = {}}
    -- log:info("HandlerLogrRequest -> request")

    if body.action == nil then
        log:warn("HandlerLogrRequest -> request invalid")
        return { retcode = utils.FDRSDK_ERRCODE_INVALID }
    end

    if body.seqnum == nil then body.seqnum = 0 end
    if body.count == nil then body.count = 10 end

    if body.action == "fetch" then
        rsp.params = wn:LogrFetch(body.seqnum, body.count)
    elseif body.action == "info" then
        rsp.max, rsp.min = wn:LogrInfo()
    else
        return { retcode = utils.FDRSDK_ERRCODE_INVALID }
    end

    return rsp
end

function CheckConfValid(key)
    return true
end

function HandlerConfRequest(ca, body)
    local wn = ca.walos
    local rsp = { retcode = 0, params = {}}

    if body.action == nil or (body.action ~= "list" and body.params == nil) then
        log:warn("HandlerConfRequest -> request invalid")
        return { retcode = utils.FDRSDK_ERRCODE_INVALID }
    end

    if body.action == "get" then
        for i,k in ipairs(body.params) do
            if CheckConfValid(k) then
                local v = wn:ConfGet(k)
                if v ~= nil then
                    table.insert(rsp.params, { [k] = v } )
                end
            end
        end
    elseif body.action == "set" then
        for i,item in ipairs(body.params) do
            for k,v in pairs(item) do
                if CheckConfValid(k) then
                    if type(v) ~= "string" then
                        v = tostring(v)
                    end

                    log:warn("config set:"..k.."   val:"..v)

                    if wn:ConfSet(k, v) then
                        table.insert(rsp.params, { [k] = v } )
                        log:warn("table insert"..json.encode({[k] = v}))
                    end
                end
            end
        end
    elseif body.action == "list" then
        rsp.params = wn:ConfList()
    else
        return { retcode = utils.FDRSDK_ERRCODE_INVALID }
    end

    return rsp
end

function CheckVisitorValid(v)
    if v == nil then return false end

    if type(v.vcid) ~= "number" then return false end
    if type(v.visicode) ~= "string" then return false end
    if type(v.started) ~= "number" then return false end
    if type(v.expired) ~= "number" then return false end

    return true
end

function HandlerVisitorRequest(ca, body)
    local wn = ca.walos
    local rsp = { retcode = 0, params = {}}

    if body.action == nil or body.params == nil then
        log:warn("HandlerGuestRequest -> request invalid")
        return { retcode = utils.FDRSDK_ERRCODE_INVALID }
    end

    if body.action == "add" then
        for i,v in ipairs(body.params) do
            if CheckVisitorValid(v) and wn:VisitorInsert(v) then
                table.insert(rsp.params, { vcid = v.vcid, ops = 0 })
            else
                table.insert(rsp.params, { vcid = v.vcid, ops = utils.FDRSDK_ERRCODE_DBOPS })
            end
        end
    elseif body.action == "del" then
        for i,v in ipairs(body.params) do
            if v.vcid ~= nil and wn:VisitorDelete(v.vcid) then
                table.insert(rsp.params, { vcid = v.vcid, ops = 0 })
            else
                table.insert(rsp.params, { vcid = v.vcid, ops = utils.FDRSDK_ERRCODE_DBOPS })
            end
        end
    else
        return { retcode = utils.FDRSDK_ERRCODE_INVALID }
    end

    return rsp
end

function HandlerPushDataRequest(ca, body)
    local rsp = { retcode = 0 }
    local ret

    if body.action ~= "pushdata" or body.type == nil or body.filename == nil 
       or body.filesize == nil or body.size == nil  or body.data == nil then
        log:warn("HandlerPushDataRequest -> request invalid")
        return { retcode = utils.FDRSDK_ERRCODE_INVALID }
    end

    if body.offset == nil then 
        body.offset = 0
    end

    local data = bs64.decode(body.data)
    
    if body.type == "upgrade" then
        ret = ca.walos:DeviceWriteUpgrade(body.filename, body.offset, body.size, data)
    elseif body.type == "userdata" then
        ret = ca.walos:UserWriteFile(body.filename, body.filesize, body.offset, body.size, data)
    elseif body.type == "test" then
        ret = ca.walos:TestWriteFile(body.filename, body.offset, body.size, data)
    else
        return { retcode = utils.FDRSDK_ERRCODE_INVALID }
    end

    if ret then rsp.retcode = 0 else rsp.retcode = utils.FDRSDK_ERRCODE_FILEOPS end
    return rsp
end

function HandlerPullDataRequest(ca, body)
    local rsp = { retcode = 0 }

    if body.action ~= "pulldata" or body.timestamp == nil  then
        log:warn("HandlerPushDataRequest -> request invalid")
        return { retcode = utils.FDRSDK_ERRCODE_INVALID }
    end

    if body.offset == nil then
        body.offset = 0
    end
    
    if body.size == nil then
        body.size = 1024*32  -- default is 32KB
    end

    if body.type == "snapshot" then
        local fname
        if body.chanel == nil then
            fname = "SNAP0-"..tostring(body.timestamp)..".nv12"
        else
            fname = "SNAP1-"..tostring(body.timestamp)..".nv12"
        end
        
        local data,size,fsize = ca.walos:SnapshotRead(fname, body.offset, body.size)
        if data ~= nil then
            rsp.data = bs64.encode(data)
            rsp.size = size
            rsp.offset = body.offset
            rsp.filesize = fsize
        else
            rsp.retcode = utils.FDRSDK_ERRCODE_FILEOPS
        end
    elseif body.type == "logrimage" then
        local data, size, fsize = ca.walos:LogrReadImage(body.timestamp, body.offset, body.size)
        if data ~= nil then
            rsp.data = bs64.encode(data)
            rsp.size = size
            rsp.filesize = fsize
            rsp.offset = body.offset
        else
            rsp.retcode = utils.FDRSDK_ERRCODE_FILEOPS
        end
    elseif body.type == "logrfile" then
        local data, size, fsize = ca.walos:LogrReadFile(body.timestamp)
        if data ~= nil then
            rsp.data = bs64.encode(data)
            rsp.size = size
            rsp.filesize = fsize
            rsp.offset = body.offset
        else
            rsp.retcode = utils.FDRSDK_ERRCODE_FILEOPS
        end
    elseif body.type == "userdata" then
        local data, size, fsize = ca.walos:UserReadFile(body.filename, body.offset, body.size)
        if data ~= nil then
            rsp.data = bs64.encode(data)
            rsp.size = size
            rsp.filesize = fsize
            rsp.offset = body.offset
        else
            rsp.retcode = utils.FDRSDK_ERRCODE_FILEOPS
        end
    else
        return { retcode = utils.FDRSDK_ERRCODE_INVALID }
    end

    return rsp
end

function HandlerDeviceRequest(ca, body)
    local rsp = { retcode = 0 }
    local ret = false

    if body.action == nil or (body.action == "upgrade" and body.filename == nil) then
        log:warn("HandlerDeviceRequest -> request invalid")
        return { retcode = utils.FDRSDK_ERRCODE_INVALID }
    end

    if body.action == "reboot" then
        ret= ca.walos:DeviceReboot()
    elseif body.action == "reset" then
        ret = ca.walos:DeviceReset()
    elseif body.action == "upgrade" then
        ret = ca.walos:DeviceUpgrade(body.filename)
    elseif body.action == "activate" then
        ret = ca.walos:DeviceActivate()
    elseif body.action == "snapshot" then
        ret = ca.walos:DeviceSnapshot()
        if ret ~= nil then
            rsp.timestamp = ret
            ret = true
        else
            ret = false
        end
    elseif body.action == "apply" then
        local conftype = body.conftype or "eth"  -- eth or wifi
        ca.walos:DeviceApply(conftype)
    else
        return { retcode = utils.FDRSDK_ERRCODE_INVALID }
    end
    if not ret then rsp.retcode = utils.FDRSDK_ERRCODE_INVALID end
    return rsp
end

function CloudApp:SendToCloud(mtype, seqence, session, body)

    local body_len = string.len(body)
    local body_pad = 4 - (body_len % 4)

    local header = ffi.new("bcp_header_t")
    header.mtype = mtype
    header.bsize = body_len + body_pad
    header.seqence  = seqence
    header.session  = session
    utils:ToNetwork(header)

    local buffer = turbo.structs.buffer(self.HeaderLen + body_len + body_pad)

    buffer:append_right(header, self.HeaderLen)
    buffer:append_right(body, body_len)
    if body_pad ~= 0 then
        buffer:append_right("    ", body_pad)
    end

    if self.Connection:writing() then
        self.WriteQueue:append(tostring(buffer))
    else
        self.Connection:write(tostring(buffer), WriteCallback, self)
    end
end

function CloudApp:AuthenToCloud()

    local ts = tostring(utils:Timestamp())
    local devid = self.walos.Basedb:ConfGet("device-id")
    if devid == nil then
        log:warn("AuthenToCloud -> get device-id fail")
        CloseConnection(self)
        return false
    end

    local sign = utils:ECDSA_Sign(self.walos.PriKey, devid, ts)
    if sign == nil then
        log:warn("AuthenToCloud -> Sign fail")
        CloseConnection(self)
        return false
    end

    local body = {}

    body["action"]      = "auth"
    body["timestamp"]   = ts
    body["device-id"]   = devid
    body["signature"]   = sign
    body["version"]     = "1.0.2"

    self:SendToCloud(BCPTypes.BCP_PMT_AUTH_REQ, 0, 0, json.encode(body))

    self:ReadNextHeader()
end

function CloudApp:ReadNextHeader()
    self.LastHeader = nil
    self.Connection:read_bytes(self.HeaderLen, ReadHeaderCallback, self)
end

function CloudApp:RequestHandler(header, body)
    if self.State ~= "authened" then
        log:fatal("RequestHandler -> not authed")
        CloseConnection(self)
        return false
    end

    local mt = header.mtype
    local sq = header.seqence
    local se = header.session

    local handler = self.Handlers[mt]
    if handler ~= nil then
        local rsp = handler(self, body)
        if rsp == nil then 
            log:warn("request handler return error")
            rsp = { retcode = utils.FDRSDK_ERRCODE_DBOPS }
        end

        -- utils:DumpTable(rsp)

        self:SendToCloud(mt + 1, sq, se, json.encode(rsp))
    else
        log:warn("RequestHandler -> type : "..tostring(header.mtype).." without handler")
    end
end

function CloudApp:HeartBeat()
    local body = {
        action = "heartbeat"
    }

    self:SendToCloud(BCPTypes.BCP_PMT_HEARTBEAT_REQ, 0, 0, json.encode(body))
end

function CloudApp:New()
    local o = {}

    setmetatable(o, self)
    self.__index = self

    self.State      = "idle"
    self.HeaderLen  = ffi.sizeof("bcp_header_t")

    self.WriteQueue = turbo.structs.deque()
    self.Handlers = {
            [BCPTypes.BCP_PMT_USER_REQ]        = HandlerUserRequest,
            [BCPTypes.BCP_PMT_LOGGER_REQ]      = HandlerLogrRequest,
            [BCPTypes.BCP_PMT_CONF_REQ]        = HandlerConfRequest,
            [BCPTypes.BCP_PMT_GUEST_REQ]       = HandlerVisitorRequest,
            [BCPTypes.BCP_PMT_PUSHDATA_REQ]    = HandlerPushDataRequest,
            [BCPTypes.BCP_PMT_PULLDATA_REQ]    = HandlerPullDataRequest,
            [BCPTypes.BCP_PMT_DEVICE_REQ]      = HandlerDeviceRequest
        }

    return o
end

function CloudApp:Start(walos, ioloop)

    ioloop:set_interval(3000, TimeoutCallback, self)
    ioloop:add_callback(TimeoutCallback, self)

    self.ioLoop    = ioloop
    self.walos   = walos
end

function CloseConnection(ca)
    log:warn("CloudApp -> connection closed")

    if not ca.Connection:closed() then
        ca.Connection:close()
    end

    ca.State = "idle"
    ca.Socket = nil
    ca.Connection = nil
end

function ReadHeaderCallback(ca, h)
    local header = ffi.cast("bcp_header_t*", h)
    utils:ToHost(header)

    if header.mtype == BCPTypes.BCP_PMT_HEARTBEAT_RSP then
        -- log:info("BCP_PMT_HEARTBEAT_RSP -> ")
        if header.bsize == 0 then
            ca:ReadNextHeader()
            return
        end
    end

    if header.bsize == 0 then
        log:warn("ReadHeaderCallback -> recv body.size == 0 message")
        ca:ReadNextHeader()
        return 
    end
    ca.LastHeader = header
    ca.Connection:read_bytes(header.bsize, ReadBodyCallback, ca)
end

function ReadBodyCallback(ca, b)
    -- log:info("ReadBodyCallback --> "..b)
    
    local body = json.decode(b)

    if ca.State == "authened" then
        ca:RequestHandler(ca.LastHeader, body)
        ca:ReadNextHeader()
        return true
    end

    if ca.State == "connected" and ca.LastHeader.mtype == BCPTypes.BCP_PMT_AUTH_RSP then
        local devid = ca.walos.Basedb:ConfGet("device-id")
        if body.timestamp == nil or body.signature == nil or body["device-id"] ~= devid then
            log:warn("ReadBodyCallback -> authen response invalid")
            CloseConnection(ca)
            return false
        end

        local ret = utils:ECDSA_Verify(ca.walos.PubKey, devid, body.timestamp, body.signature)
        if ret then
            ca.State = "authened"
            log:info("Connect to cloud --> verify ok")
            ca:ReadNextHeader()

            local ts = tonumber(body.timestamp)
            local gap = body.gap
            if gap == nil then gap = 0 end
            local tnow = tonumber(utils:Timestamp())
            tnow = tnow + math.floor((tnow - ts + gap) / 2)
            ca.walos:DeviceSetHWTime(tnow)

            return true
        else
            log:warn("Connect to cloud -> verify fail")
            CloseConnection(ca)
            return false
        end
    else
        log:warn("ReadBodyCallback -> recv other package before authen")
        CloseConnection(ca)
    end
end

function WriteCallback(ca)
    if ca.WriteQueue:not_empty() then
        -- log:warn("WriteCallback ============ write next")
        local buffer = ca.WriteQueue:popleft()
        ca.Connection:write(buffer, WriteCallback, ca)
    end
end

function ConnectedCallback(ca)
    ca.State = "connected"

    ca.Connection:set_close_callback(CloseConnection, ca)
    ca:AuthenToCloud()
end

function ConnectFailCallback(ca)
    log:warn("connect to cloud -> fail")
    ca.State = "idle"
    ca.Socket = nil
    ca.Connection = nil
end

function ConnectToCloud(ca)
    if ca.Socket == nil then
        local sock, msg = turbo.socket.new_nonblock_socket(
                    turbo.socket.AF_INET, turbo.socket.SOCK_STREAM, 0)
        if sock == -1 then
            log:fatal("ConnectToCloud => Could not create socket")
            return false
        end
        ca.Socket = sock
    end

    if ca.Connection == nil then
        ca.Connection = turbo.iostream.IOStream(ca.Socket, ca.ioLoop, 1024*1024)
        if ca.Connection == nil then
            log:fatal("ConnectToCloud => create iostream fail")
            return false
        end
    end

    local addr = ca.walos.Basedb:ConfGet("cloud-server-address")
    local port = ca.walos.Basedb:ConfGet("cloud-server-port")

    if addr == nil then addr  = "devs.bashisense.com" end
    if port == nil then port = 8080 end

    -- log:info("Connect to cloud -> "..addr..":"..tostring(port).." ...")

    ca.Connection:connect(addr, tonumber(port), turbo.socket.AF_INET,
                            ConnectedCallback, ConnectFailCallback, ca)

    ca.State = "connecting"
end

function TimeoutCallback(ca)
    
    if ca.State == "idle" then
        ConnectToCloud(ca)
        return
    end

    if ca.State ~= "idle" then
        if ca.Connection:closed() then
            ca.State = "idle"
            ca.Connection:close()
            return
        end

        if ca.State == "authened" then
            ca:HeartBeat()
            return
        end
    end
end

function CloudApp:LogrEventHandler(eobj)
    log:info("CloudApp:LogrEventHandler receive :"..tostring(eobj))

    if self.state == "authened" then
        self:SendToCloud(BCPTypes.BCP_PMT_EVENT_REQ, 0, 0, json.encode(eobj))
    end
end

return CloudApp;
