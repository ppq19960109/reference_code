--
-- Copyright (c) 2020, Bashi Tech. All rights reserved.
--
-- package.path = "../modules/?.lua;./?.bin;"
-- package.cpath = "../lib/?.so;"

local ffi   = require("ffi")
local log   = require("log")
local json  = require("json")
local utils = require("Utils")
local bs64  = require("base64")

local turbo = require("turbo")
local ioloop = turbo.ioloop.instance()
local TcpServer = turbo.tcpserver.TCPServer()

local BasePath = nil
local Pubkey = nil
local Prikey = nil

local Client = {
    State = "idle",
    Connection = nil,
    HeaderRecv = nil,
    HeaderSend = nil,
    QueueToSend = nil,
}

function Client:New()
    local obj = {}

    setmetatable(obj, self)
    self.__index = self

    self.HeaderSend = ffi.new("bcp_header_t")
    self.HeaderSend.mtype = 0
    self.HeaderSend.bsize = 0
    self.HeaderSend.seqence  = 0
    self.HeaderSend.session  = 0

    self.QueueToSend = turbo.structs.deque()

    return obj
end

function Client:TestCase(header, body)
    --[[
    local req = utils:ReadAll(BasePath.."/testcase/user_add.json")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_USER_REQ, 0, req)
    end
    rep = nil

    req = utils:ReadAll(BasePath.."/testcase/user_update.json")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_USER_REQ, 0, req)
    end
    rep = nil

    req = utils:ReadAll(BasePath.."/testcase/user_info.json")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_USER_REQ, 0, req)
    end
    rep = nil

    req = utils:ReadAll(BasePath.."/testcase/user_list.json")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_USER_REQ, 0, req)
    end
    rep = nil

    req = utils:ReadAll(BasePath.."/testcase/user_del.json")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_USER_REQ, 0, req)
    end
    rep = nil

    local req = utils:ReadAll(BasePath.."/testcase/visitor_insert.json")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_GUEST_REQ, 0, req)
    end
    rep = nil

    local req = utils:ReadAll(BasePath.."/testcase/visitor_delete.json")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_GUEST_REQ, 0, req)
    end
    rep = nil

    local req = utils:ReadAll(BasePath.."/testcase/config_get.json")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_CONF_REQ, 0, req)
    end
    rep = nil

    local req = utils:ReadAll(BasePath.."/testcase/config_set.json")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_CONF_REQ, 0, req)
    end
    rep = nil

    local req = utils:ReadAll(BasePath.."/testcase/config_list.json")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_CONF_REQ, 0, req)
    end
    rep = nil
    

    local req = utils:ReadAll(BasePath.."/testcase/dev_activate.jn")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_DEVICE_REQ, 0, req)
    end
    rep = nil

    local req = utils:ReadAll(BasePath.."/testcase/dev_reboot.jn")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_DEVICE_REQ, 0, req)
    end
    rep = nil

    local req = utils:ReadAll(BasePath.."/testcase/dev_reset.jn")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_DEVICE_REQ, 0, req)
    end
    rep = nil

    local req = utils:ReadAll(BasePath.."/testcase/dev_upgrade.jn")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_DEVICE_REQ, 0, req)
    end
    rep = nil

    local req = utils:ReadAll(BasePath.."/testcase/snapshot-act.jn")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_PULLDATA_REQ, 0, req)
    end
    rep = nil

    local req = utils:ReadAll(BasePath.."/testcase/user-read.jn")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_PULLDATA_REQ, 0, req)
    end
    rep = nil

    local req = utils:ReadAll(BasePath.."/testcase/logr_fetch.jn")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_LOGGER_REQ, 0, req)
    end
    rep = nil

    local req = utils:ReadAll(BasePath.."/testcase/logr_info.jn")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_LOGGER_REQ, 0, req)
    end
    rep = nil
    local req = utils:ReadAll(BasePath.."/testcase/logr_file.jn")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_PULLDATA_REQ, 0, req)
    end
    rep = nil

    local req = utils:ReadAll(BasePath.."/testcase/dev_reset.jn")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_DEVICE_REQ, 0, req)
    end
    rep = nil
    ]]

    local req = utils:ReadAll(BasePath.."/testcase/config_set.json")
    if req ~= nil then
        self:SendTo(ffi.C.BCP_PMT_CONF_REQ, 0, req)
    end
    rep = nil

end

function WriteToClientCallback(client)
    if client.QueueToSend:not_empty() then
        local buffer = client.QueueToSend:popleft()
        client.Connection:write(buffer, WriteToClientCallback, client)
    end
end


function Client:SendTo(mtype, session, body)
    local bs = #body
    local pad = 4 - (bs%4)
    local buffer = turbo.structs.buffer(ffi.sizeof("bcp_header_t") + bs + pad)

    local header = ffi.new("bcp_header_t")

    header.seqence = self.HeaderSend.seqence
    self.HeaderSend.seqence = self.HeaderSend.seqence  + 1
    header.mtype = mtype
    header.session = session
    header.bsize = bs + pad
    log:info("SendTo -> bsize:"..tostring(header.bsize)..", pad:"..tostring(pad))
    utils:ToNetwork(header)

    buffer:append_right(header, ffi.sizeof(header))
    buffer:append_right(body, bs)
    if pad > 0 then
        buffer:append_right("    ", pad)
    end

    if self.Connection:writing() then
        self.QueueToSend:append(tostring(buffer))
    else
        self.Connection:write(tostring(buffer), WriteToClientCallback, self)
    end
end

function ReadBodyCallback(client, bodybytes)
    local header = client.HeaderRecv
    client.HeaderRecv = nil

    local body = json.decode(bodybytes)

    if header.mtype == ffi.C.BCP_PMT_AUTH_REQ then
        utils:DumpTable(body)

        local devid = body["device-id"]
        local ts    = body["timestamp"]
        local sign  = body["signature"]

        if devid == nil or ts == nil or sign == nil then
            log:warn("ReadBodyCallback => auth req invalid")
            return ;
        end

        local ret = utils:ECDSA_Verify(Pubkey, devid, ts, sign)
        if not ret then
            log:warn("ReadBodyCallback => verify fail")
            return ;
        end

        log:info("ReadBodyCallback => client verify ok")

        body["timestamp"] = tostring(utils:Timestamp())
        body["gap"]       = 200
        body["signature"] = utils:ECDSA_Sign(Prikey, devid, body["timestamp"])
        if body["signature"] == nil then
            log:warn("ReadBodyCallback => sign fail")
            return ;
        end
        
        body["action"] = "authen"

        client:SendTo(ffi.C.BCP_PMT_AUTH_RSP, 0, json.encode(body))

        client:TestCase(nil, nil)
    else
        if header.mtype ~= ffi.C.BCP_PMT_HEARTBEAT_REQ then
            utils:DumpTable(body)
            -- client:TestCase(header, body)
        end
    end

    if not client.Connection:closed() then
        client.Connection:read_bytes(ffi.sizeof("bcp_header_t"), ReadHeaderCallback, client)
    end
end

function ReadHeaderCallback(client, header)
    local  h = ffi.cast("bcp_header_t *", header)
    utils:ToHost(h)
    
    client.HeaderRecv = h
    
    if not client.Connection:closed() then
        client.Connection:read_bytes(h.bsize, ReadBodyCallback, client)
    end
end

function Client:Running()
    if self.State == "connected" then
        self.Connection:read_bytes(ffi.sizeof("bcp_header_t"), ReadHeaderCallback, self)
    end
end


function CloseConnection(client)
    log:warn("tcpserver -> connection closed")

    client.Connection = nil
    client.State = "idle"
    client.HeaderRecv = nil
    client.HeaderSend = nil
    client.QueueToSend = nil
    client = nil
end

function TcpServer:handle_stream(stream, address)
    log:info("tcpserver -> connect from "..address)

    local c = Client:New()
    c.Connection = stream
    c.State = "connected"

    stream:set_close_callback(CloseConnection, c)
    
    c:Running()
end


function Startup(basepath)
    log:info("basepath : "..basepath)
    BasePath = basepath
    
    Pubkey = utils:ReadAll(basepath.."/data/pubkey.ecc")
    if Pubkey == nil then
        log:warn("Startup -> read pubkey fail")
        return false
    end

    Prikey = utils:ReadAll(basepath.."/data/prikey.ecc")
    if Prikey == nil then
        log:warn("Startup -> read prikey fail")
        return false
    end

    log:info("Startup listen server...")

    TcpServer:listen(8007, turbo.socket.INADDR_ANY, 10, turbo.socket.AF_INET)
    ioloop:start()
    return true
end

-- Startup("/Users/konghan/Workspace/Walnuts/targets/macos-amd64")
