--
-- Copyright (c) 2020, Bashi Tech. All rights reserved.
--

package.path = "../template/modules/?.lua;./?.lua;"
package.cpath = "../templage/macosx-lib/?.so;./?.so;"

local log   = require("log")
local Basedb = require("Basedb")
local Utils  = require("Utils")

local sdb = nil

function TestInit()
    sdb = Basedb:New()
    os.remove("./TestUserdb.sdb")

    if sdb:Open("./TestUserdb.sdb") then
        log:info("--------TestUserdb Init--------")
    else
        log:info("--------TestUserdb Fail--------")
    end
end

function TestFini()
    sdb:Close()
    os.remove("./TestUserdb.sdb")

    log:info("--------TestUserdb Fini--------")
end

function DumpUser(user)
    log:info("----------------------")
    log:info("  user.userid:"..user.userid)
    log:info("  user.name:"..user.name)
    log:info("  user.desc:"..user.desc)
    log:info("  user.others:", user.others)
    log:info("  user.state:", user.state)
    log:info("  user.rule:", user.rule)
    log:info("  user.expire_datetime:"..os.date("%Y-%m-%d %H:%M:%S", user.expired))
    log:info("  user.create_datetime:"..user.create_datetime)
    log:info("  user.modify_datetime:"..user.modify_datetime)
    log:info("----------------------")
end

function TestUserCRUD()
    log:info("--------TestUserCRUD--------")
    
    local user = {}
    user.userid   = "userid-0x923ksldfs"
    user.name     = "Test User"
    user.desc     = "develop department"
    user.others   = nil
    user.state    = 0
    user.rule     = 1

    if sdb:UserInsert(user) then
        log:info("Insert Success")
    else
        log:warn("Insert Fail")
    end

    local u = sdb:UserLookup(user.userid)
    if u ~= nil then
        log:info("Lookup Success")
        DumpUser(u)
    else
        log:warn("Lookup Fail")
    end

    user.name = "New Name"
    user.desc = nil
    user.others = "others"
    if sdb:UserUpdate(user) then
        log:info("Update Success")
    else
        log:warn("Update Fail")
    end

    if sdb:UserDelete(user.userid) then
        if not sdb:UserLookup(user.userid) then
            log:info("Delete Success")
        else
            log:warn("Delete Fail")
        end
    else
        log:warn("Delete Fail")
    end

end

function TestUserFetch()
    local user = {}

    log:info("--------TestUserFetch--------")

    user.name     = "Test User"
    user.desc     = "develop department"
    user.others   = nil
    user.state    = 0
    user.rule     = 1

    for i = 1, 100, 1 do
        user.userid   = string.format("%s%d", "userid-random-", i)
        if not sdb:UserInsert(user) then
            log:warn("TestUserFetch Fail:"..user.userid)
        end
    end

    local count = 0
    for i = 1, 100, 10 do
        local ids, nr = sdb:UserFetch(i-1, 10)
        if ids == nil then
            break
        end

        Utils:DumpTable(ids)

        count = count + 1

        --[[
        for j = 1, nr, 1 do
            local u = sdb:Lookup(ids[1][j])
            if u ~= nil then
                DumpUser(u)
            end
        end
        ]]
    end

    if count ~= 10 then
        log:warn("TestUserFetch Fail:", count)
    else
        log:info("TestUserFetch Success")
    end
end


TestInit()

TestUserCRUD()

TestUserFetch()

TestFini()
