--
-- Copyright (c) 2020, Bashi Tech. All rights reserved.
--

package.path = "../template/modules/?.lua;./?.lua;"
package.cpath = "../templage/macosx-lib/?.so;./?.so;"

local log   = require("log")
local Basedb = require("Basedb")
local Utils  = require("Utils")
local ffi = require("ffi")

local sdb = nil

function TestInit()
    sdb = Basedb:New()
    os.remove("./TestVisitor.sdb")

    if sdb:Open("./TestVisitor.sdb") then
        log:info("--------TestVisitor Init--------")
    else
        log:info("--------TestVisitor Fail--------")
    end
end

function TestFini()
    sdb:Close()
    os.remove("./TestVisitor.sdb")

    log:info("--------TestVisitor Fini--------")
end


function TestVisitor()
    log:info("--------TestVisitor--------")

    local vcid = 100
    local visicode = "436789"
    local started  = os.time()
    local expired  = os.time() + 3600*24
    
    -- InsertVisitorCode
    if sdb:VisitorInsert(vcid, visicode, started, expired) then
        log:info("TestVisitor -> VisitorInsert Success")
    else
        log:warn("TestVisitor -> VisitorInsert Fail")
        return false
    end

    -- LookupVisitorCode
    if sdb:VisitorLookup(visicode) then
        log:info("TestVisitor -> VisitorLookup Success")
    else
        log:warn("TestVisitor -> VisitorLookup Fail")
        return false
    end

    if sdb:VisitorLookup(visicode, os.time() - 1024) then
        log:warn("TestVisitor -> VisitorLookup Fail")
        return false
    else
        log:info("TestVisitor -> VisitorLookup Success")
    end

    if sdb:VisitorLookup(visicode, os.time() + 10*24*3600) then
        log:warn("TestVisitor -> VisitorLookup Fail")
        return false
    else
        log:info("TestVisitor -> VisitorLookup Success")
    end

    -- DeleteVisitorCode
    if sdb:VisitorDelete(vcid) then
        log:info("TestVisitor -> VisitorDelete Success")
    else
        log:warn("TestVisitor -> VisitorDelete Fail")
        return false
    end

    if sdb:VisitorLookup(visicode) then
        log:warn("TestVisitor -> VisitorLookup Fail")
        return false
    else
        log:info("TestVisitor -> VisitorLookup Success")
    end

    -- CleanVisitorCode
    for i = 1, 10, 1 do
        visicode = "2348"..tostring(i)
        sdb:VisitorInsert(vcid, visicode, started, expired)
    end

    if sdb:VisitorClean(os.time() + 100*24*3600) then
        visicode = "2348"..tostring(5)
        if sdb:VisitorLookup(visicode) then
            log:warn("TestVisitor -> VisitorClean Fail")
            return false
        else
            log:info("TestVisitor -> VisitorClean Success")
        end
    else
        log:warn("TestVisitor -> VisitorClean Fail")
        return false
    end
end


TestInit()

TestVisitor()

TestFini()
