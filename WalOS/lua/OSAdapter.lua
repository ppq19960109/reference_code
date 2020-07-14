--
-- Copyright (c) 2020, Bashi Tech. All rights reserved.
--

local log = require("log")
local utils = require("Utils")

OSAdapter = {
}

function OSAdapter:New()
    local o = {}

    setmetatable(o, self)
    self.__index = self

    return o
end

function OSAdapter:ConfigEther()

    -- cbody["ether-ipv4-address-mode"]
    -- cbody["ether-ipv4-address"]
    -- cbody["ether-ipv4-mask"]
    -- cbody["ether-ipv4-gw"]
    -- cbody["ether-dns-server"]

    -- FIXME : ConfigEther to os
    log:warn("OSAdapter:ConfigEther => ")
end

function OSAdapter:ConfigWifi()
    -- cbody["wifi-ipv4-address-mode"]
    -- cbody["wifi-ipv4-address"]
    -- cbody["wifi-ipv4-mask"]
    -- cbody["wifi-ipv4-gw"]
    -- cbody["wifi-dns-server"]

    -- FIXME : ConfigWifi to os
    log:warn("OSAdapter:ConfigWifi => ")
end

function OSAdapter:Apply(ctype, cbody)

    -- utils:DumpTable(cbody)

    if ctype == "wifi" then
        self:ConfigWifi(cbody)
    elseif ctype == "eth" then
        self:ConfigEther(cbody)
    else
        log:warn("OSAdapter:Apply type unkown :"..ctype)
    end
end

function OSAdapter:SetSystemTime(ts)
    log:info("OSAdapter:SetSystemTime :"..tostring(ts))
    -- FIXME : set systemtime
    -- os.execute("hwclock ")
end

function OSAdapter:Upgrade(fname)
    log:info("OSAdapter:Upgrade :"..fname)

    -- FIXME : call upgrade script
end

function OSAdapter:Reset(path)
    
    log:info("OSAdapter:Reset :"..path)

    -- FIXME : call upgrade script
    --[[
    os.execute("rm -f "..path.."basedb.sdb")
    os.execute("rm -f "..path.."logrdb.sdb")

    os.execute("rm -rf "..path.."logr")
    os.execute("rm -rf "..path.."users")
    os.execute("rm -rf "..path.."snapshots")
    ]]

    self:Reboot()
end

function OSAdapter:Reboot()
    
    log:info("OSAdapter:Reboot")

end

return OSAdapter
