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
    os.remove("./TestFeature.sdb")

    if sdb:Open("./TestFeature.sdb") then
        log:info("--------TestFeature Init--------")
    else
        log:info("--------TestFeature Fail--------")
    end
end

function TestFini()
    sdb:Close()
    os.remove("./TestFeature.sdb")

    log:info("--------TestFeature Fini--------")
end

function NewRandomFeature(n)
    local data = ffi.new(ffi.typeof("char[?]"), n)

    for i = 0, n -1  do
        data[i] = math.random() % 255
    end

    return ffi.string(data, n)
end

function BytesCompare(a, b)
    if a == nil then return false end
    if b == nil then return false end

    if #a ~= #b then return false end
    
    for i = 1, #a do
        if a[i] ~= b[i] then
            return false
        end
    end
    
    return true
end

function TestFeature()
    log:info("--------TestFeature--------")

    local userid  = string.format("%s%d", "userid-random-", 20)
    local algmv   = "v2.2.1"
    local feature1 = NewRandomFeature(2048)
    local feature2 = NewRandomFeature(2048)

    -- insert
    local ret = sdb:FeatureInsert(userid, algmv, feature1)
    if not ret then
        log:warn("TestFeature -> FeatureInsert Fail")
        return false
    else
        log:info("TestFeature -> FeatureInsert Success")
    end

    -- lookup
    local fret = sdb:FeatureLookup(userid, algmv)
    -- Utils:DumpTable(fret)
    
    if fret == nil then
      log:warn("TestFeature -> FeatureLookup Fail-1")
      return false
    elseif BytesCompare(feature1,fret) then
      log:info("TestFeature -> FeatureLookup Success")
    else
      log:warn("TestFeature -> FeatureLookup Fail-2")
      return false
    end

    -- update
    ret = sdb:FeatureUpdate(userid, algmv, feature2)
    if not ret then
        log:warn("TestFeature FeatureUpdate Fail")
        return false
    end

    fret = sdb:FeatureLookup(userid, algmv)
    if fret == nil then
      log:warn("TestFeature -> FeatureUpdate Fail")
      return false
    elseif BytesCompare(fret,feature2) then
      log:info("TestFeature -> FeatureUpdate Success")
    else
      log:warn("TestFeature -> FeatureUpdate Fail")
      return false
    end

    --delete
    ret = sdb:FeatureDelete(userid, algmv)
    if not ret then
        log:warn("TestFeature FeatureDelete Fail")
        return false
    end

    fret = sdb:FeatureLookup(userid, algmv)
    if fret == nil then
        log:info("TestFeature -> FeatureDelete Success")
    else
        log:warn("TestFeature -> FeatureDelete Fail")
        return false
    end
end



TestInit()

TestFeature()

TestFini()
