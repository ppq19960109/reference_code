--
-- Copyright (c) 2020, Bashi Tech. All rights reserved.
--

package.path = "../template/modules/?.lua;./?.lua;"
package.cpath = "../templage/macosx-lib/?.so;./?.so;"

local log   = require("log")
local Basedb = require("Basedb")
local Utils = require("Utils")

local sdb = nil

function TestInit()
  	sdb = Basedb:New()
  	os.remove("./TestConfdb.sdb")

  	if sdb:Open("./TestConfdb.sdb") then
      	log:info("--------TestConfdb Init--------")
  	else
      	log:info("--------TestConfdb Fail--------")
  	end
end

function TestFini()
  	sdb:Close()
  	os.remove("./TestConfdb.sdb")

  	log:info("--------TestConfdb Fini--------")
end

function TestConfdb()

  	if sdb:ConfSet("key001", "val001") then
		log:info("Set Success")
  	else
		log:warn("Set Fail")
		return false
	end

  	if sdb:ConfSet("key002", "val002-2") then
		log:info("Set Success")
  	else
		log:warn("Set Fail")
		return false
	end

	local row, options, modify = sdb:ConfGet("key002")
	log:info("val:"..row.." opitons:"..tostring(options).." modify:"..modify)
	if row == nil then
		log:warn("Get Fail")
		return false
	end
	
	if row ~= "val002-2" then
		log:warn("Get Fail")
		return false
	end

	log:info("Get Success")
	
	local sets = sdb:ConfList()
	
	Utils:DumpTable(sets)

end

TestInit()

TestConfdb()

TestFini()
