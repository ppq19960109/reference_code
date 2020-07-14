--
-- Copyright (c) 2020, Bashi Tech. All rights reserved.
--

local sql   = require("sqlite3")
local log   = require("log")

Logrdb = {
    _dbs = nil,      -- sqlite3  opened db instance
    _state = 0       -- state of this object, 0 - uninited, 1 - inited
}


function Logrdb:New(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end


function Logrdb:Open(dbfile)
    self._dbs = sql.open(dbfile)

    if self._dbs ~= nil then
        self._dbs:exec[[
            CREATE TABLE IF NOT EXISTS logr_table(
                seqnum  INTEGER PRIMARY KEY AUTOINCREMENT,
                occtime INT8,
                face_x  INT2, face_y  INT2, face_w  INT2, face_h  INT2,
                sharp   REAL, score   REAL,
                therm REAL,
                userid  VARCHAR(16));           
        ]]

        self._state = 1

        return true
    end

    return false
end

function Logrdb:Close()
    if self._state < 1 then
        return 
    end

    self._dbs:close()
    self._state = 0
end

function Logrdb:MaxSequence()
    if self._state < 1 then
        return 0
    end

    local lines, nr = self._dbs:exec[[
        SELECT MAX(seqnum) FROM logr_table;         
        ]]

    if nr ~= nil and nr ~= 0 then
        if lines[1][1] == nil then
            return 0
        else
            return tonumber(lines[1][1])
        end
    end

    return 0
end


function Logrdb:MinSequence()
    if self._state < 1 then
        return 0
    end

    local lines, nr = self._dbs:exec[[
        SELECT MIN(seqnum) FROM logr_table;      
        ]]

    if nr ~= nil and nr ~= 0 then
        if lines[1][1] == nil then
            return 0
        else
            return tonumber(lines[1][1])
        end
    end
    
    return 0
end

function Logrdb:Count()
    if self._state < 1 then
        return 0
    end

    local lines, nr = self._dbs:exec[[
        SELECT COUNT(*) FROM logr_table;    
        ]]

    if nr ~= nil and nr ~= 0 then
        if lines[1][1] == nil then
            return 0
        else
            return tonumber(lines[1][1])
        end
    end
        
    return 0
end

function Logrdb:Append(logr)
    if self._state < 1 then
        return false
    end

    if logr == nil and logr.occtime == nil then
        return false
    end

    logr.face_x = logr.face_x or 0
    logr.face_y = logr.face_y or 0
    logr.face_w = logr.face_w or 0
    logr.face_h = logr.face_h or 0

    logr.sharp  = logr.sharp or 0.0
    logr.score  = logr.score or 0.0
    logr.therm  = logr.therm or 0.0

    logr.userid = logr.userid or ("0000"..tostring(logr.occtime))

    local stmt = self._dbs:prepare("INSERT INTO logr_table(occtime, face_x, face_y, face_w, face_h, \
                                    sharp, score, therm, userid) VALUES(?1, ?2, ?3, ?4,?5, ?6, ?7, ?8, ?9)")
    local val = stmt:reset():bind(logr.occtime, logr.face_x, logr.face_y, logr.face_w, logr.face_h,
                                logr.sharp, logr.score, logr.therm,logr.userid):step()
    stmt:close()

    if val == nil then
        return true
    end

    return false
end

function Logrdb:Lookup(seqnum)
    if self._state < 1 then
        return nil
    end

    local stmt = self._dbs:prepare("SELECT * FROM logr_table WHERE seqnum = ?1")
    local row = stmt:reset():bind(seqnum):step()
    stmt:close()

    if row == nil then
        return nil
    end

    local logr = {}
    logr.seqnum     = tonumber(row[1])
    logr.occtime    = tonumber(row[2])
    logr.face_x     = tonumber(row[3])
    logr.face_y     = tonumber(row[4])
    logr.face_w     = tonumber(row[5])
    logr.face_h     = tonumber(row[6])
    logr.sharp      = tonumber(row[7])
    logr.score      = tonumber(row[8])
    logr.therm      = tonumber(row[9])
    logr.userid     = row[10]

    return logr
end

function Logrdb:Clean(seqnum)
    if self._state < 1 then
        return false
    end

    local stmt = self._dbs:prepare("DELETE FROM logr_table WHERE seqnum < ?1")
    local row = stmt:reset():bind(seqnum):step()
    stmt:close()

    if row == nil then
        return true
    end

    return false
end

return  Logrdb