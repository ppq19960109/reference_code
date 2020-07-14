--
-- Copyright (c) 2020, Bashi Tech. All rights reserved.
--

local sql   = require("sqlite3")
local log   = require("log")
local ffi   = require("ffi")
local utils = require("Utils")

local Basedb = {
    _dbs = nil,      -- sqlite3  opened db instance
    _state = 0,      -- state of this object, 0 - uninited, 1 - inited
    _stmt = nil,     -- stmt for user fetch next
    _algmv = nil
}

-- User = {
--    userid      = nil,
--    name        = nil,
--    desc        = nil,
--    others      = nil,
--    state       = 0,
--    rule        = 0,
--    expire_datetime     = 0,    -- seconds form 1970,1,1
--    create_datetime     = nil,
--    modify_datetime     = nil
-- }

-- Conf = {
--      key = nil,
--      val = nil,
--      modify_datetime = nil
-- }

-- Visitor = {
--      vcid = 0,
--      visicode = nil,
--      started  = nil,
--      expired = nil,
--      create_datetime = nil
-- }

function Basedb:New(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    o._algmv = "2.2.1"
    return o
end


function Basedb:Open(dbfile)
    self._dbs = sql.open(dbfile)

    if self._dbs ~= nil then
        self._dbs:exec[[
            CREATE TABLE IF NOT EXISTS visitor_table(
                vcid INTEGER  PRIMARY KEY,
                visicode  VARCHAR(16),
                started INTEGER,
                expired INTEGER,
                create_datetime VARCHAR(32) DEFAULT CURRENT_TIMESTAMP NOT NULL);
            
            CREATE TABLE IF NOT EXISTS user_table(
                userid  VARCHAR(32) PRIMARY KEY,
                name    VARCHAR(16),
                desc    VARCHAR(32),
                others  VARCHAR(32),
                state   INTEGER,
                rule    INTEGER,
                expire_datetime INTEGER,
                create_datetime VARCHAR(32),
                modify_datetime VARCHAR(32) DEFAULT CURRENT_TIMESTAMP NOT NULL);
                
            CREATE TABLE IF NOT EXISTS feature_table(
                userid  VARCHAR(32),
                algmv   VARCHAR(32),
                feature BLOB,
                PRIMARY KEY (userid, algmv));

            CREATE TABLE IF NOT EXISTS conf_table(
                key TEXT PRIMARY KEY, 
                val TEXT,
                options INTEGER,
                modify_datetime VARCHAR(32) DEFAULT CURRENT_TIMESTAMP NOT NULL);

            CREATE TABLE IF NOT EXISTS admin_table(
                name TEXT PRIMARY KEY, 
                passwd TEXT,
                acl INTEGER, 
                create_datetime VARCHAR(32),
                modify_datetime VARCHAR(32) DEFAULT CURRENT_TIMESTAMP NOT NULL);
        ]]

        self._state = 1

        return true
    end

    return false
end

function Basedb:Close()
    if self._state < 1 then return  end

    self._dbs:close()
    self._state = 0
end

function Basedb:UserExist(userid)

    if self._state < 1 then return nil end
    
    local stmt = self._dbs:prepare("SELECT * FROM user_table WHERE userid = ?")
    local val = stmt:reset():bind(userid):step()
    stmt:close()

    return val
end

function Basedb:UserCheckMaxValid(user)
    if user == nil then return false end
    if user.userid == nil then return false end
    if user.name == nil then return false end
    if user.desc == nil then return false end
    if user.rule == nil then user.rule = 1 end
    if user.expired == nil then user.expired = os.time() + 24*60*60*365 end

    return true
end

function Basedb:UserCheckMinValid(user)
    if user == nil then return false end
    if user.userid == nil then return false end
    return true
end

function Basedb:UserInsert(user)
    if self._state < 1 then return false end

    -- utils:DumpTable(user)

    if not self:UserCheckMaxValid(user) then
        log:warn("UserInsert -> check user max fail")
        return false
    end

    local u = self:UserExist(user.userid)
    if u ~= nil then
        log:warn("UserInsert -> user exist")
        return false
    end

    local stmt = self._dbs:prepare("INSERT INTO user_table VALUES(?1, ?2, ?3, ?4,?5, ?6, ?7,   \
                datetime(CURRENT_TIMESTAMP, 'localtime'), datetime(CURRENT_TIMESTAMP, 'localtime'))")
    local val = stmt:reset():bind(user.userid, user.name, user.desc, user.others, 0, user.rule, user.expired):step()
    stmt:close()

    if val == nil then
        return true
    end

    return false
end

function UserRowToTable(user)
    if user == nil then
        log:warn("RowToTable -> input is nil")
        return nil
    end

    local u = {}

    u.userid  = user[1]
    u.name    = user[2]
    u.desc    = user[3]
    u.others  = user[4]
    u.state   = tonumber(user[5])
    u.rule    = tonumber(user[6])
    u.expired = tonumber(user[7])   -- expire_datetime in db
    u.create_datetime = user[8]
    u.modify_datetime = user[9]

    return u
end

function Basedb:UserLookup(userid)
    if self._state < 1 then return nil end
    if userid == nil then return false end

    local stmt = self._dbs:prepare("SELECT * FROM user_table WHERE userid = ?1")
    local user = stmt:reset():bind(userid):step()
    stmt:close()

    if user == nil then
        -- log:warn("Lookup -> lookup "..userid.."not exist")
        return nil
    end

    return UserRowToTable(user)
end

function Basedb:UserDelete(userid)
    if self._state < 1 then return false end
    if userid == nil then return false end

    local stmt = self._dbs:prepare("DELETE FROM user_table WHERE userid = ?1")
    local done = stmt:reset():bind(userid):step()
    stmt:close()

    if done == nil then
        return true
    else
        return false
    end
end

function Basedb:UserUpdate(user)
    if self._state < 1 then return false end

    if not self:UserCheckMinValid(user) then
        log:warn("UserUpdate -> check user fail")
        return false
    end

    local u = self:UserLookup(user.userid)
    if u == nil then
        log:warn("UserUpdate -> user not exist:"..user.userid)
        return false
    end

    user.name = user.name or u.name
    user.desc = user.desc or u.desc
    user.others = user.others or u.others
    user.state = user.state or u.state
    user.rule = user.rule or u.rule
    user.expired = user.expired or u.expired

    local stmt = self._dbs:prepare("UPDATE user_table SET name = ?1, desc = ?2, others = ?3, state = ?4,  \
                        rule = ?5, expire_datetime = ?6, modify_datetime = datetime(CURRENT_TIMESTAMP, 'localtime') where userid = ?7")
    local val = stmt:reset():bind(user.name, user.desc, user.others, user.state, user.rule, user.expired, user.userid):step()
    stmt:close()

    if val == nil then
        return true
    end

    return false
end

function Basedb:UserFetch(offset, limit)    
    if self._state < 1 then return nil end

    if offset == nil then offset = 0 end
    if limit == nil then limit = 10 end

    local stmt = self._dbs:prepare("SELECT userid FROM user_table LIMIT ?1 OFFSET ?2")
    local userids, nr = stmt:reset():bind(limit, offset):resultset()
    stmt:close()

    if nr > 0 then
        return userids.userid, nr
    else
        return nil, 0
    end
end

function Basedb:FeatureExist(userid, algmv)
    if self._state < 1 then return false end
    if algmv == nil then algmv = self._algmv end

    local stmt = self._dbs:prepare("SELECT * FROM feature_table WHERE (userid = ?1 AND algmv = ?2)")
    local val = stmt:reset():bind(userid, algmv):step()
    stmt:close()

    if val == nil then return false end

    return true
end

function Basedb:FeatureInsert(userid, algmv, feature)
    if self._state < 1 then return false end
    if algmv == nil then algmv = self._algmv end

    local exist = self:FeatureExist(userid, algmv)
    if exist then
        log:warn("FeatureInsert -> exist")
        return false
    end

    local stmt = self._dbs:prepare("INSERT INTO feature_table VALUES(?1, ?2, ?3)")
    local val = stmt:reset():bind(userid, algmv, sql.blob(feature)):step()
    stmt:close()

    if val == nil then return true end

    return false
end


function Basedb:FeatureLookup(userid, algmv)
    if self._state < 1 then return nil end
    if algmv == nil then algmv = self._algmv end

    local stmt = self._dbs:prepare("SELECT * FROM feature_table WHERE (userid = ?1 AND algmv = ?2)")
    local val = stmt:reset():bind(userid, algmv):step()
    stmt:close()

    if val == nil then return nil end

    return val[3]
end

function Basedb:FeatureUpdate(userid, algmv, feature)
    if self._state < 1 then return false end
    if algmv == nil then algmv = self._algmv end

    local stmt = self._dbs:prepare("UPDATE feature_table SET feature = ?1  WHERE (userid = ?2 AND algmv = ?3)")
    stmt:reset():bind(sql.blob(feature), userid, algmv):step()
    stmt:close()
    
    return true
end

function Basedb:FeatureDelete(userid, algmv)
    if self._state < 1 then return false end

    local stmt = nil
    if algmv ~= nil then
        stmt = self._dbs:prepare("DELETE FROM feature_table WHERE (userid = ?1 AND algmv = ?2)")
        stmt:reset():bind(userid, algmv):step()
    else
        stmt = self._dbs:prepare("DELETE FROM feature_table WHERE userid = ?1")
        stmt:reset():bind(userid):step()
    end
    stmt:close()

    return true
end

function Basedb:VisitorInsert(vcid, visicode, started, expired)
    if self._state < 1 then return false end

    if vcid == nil or visicode == nil then return false end

    local stmt = self._dbs:prepare("SELECT * FROM visitor_table WHERE vcid = ?1")
    local val = stmt:reset():bind(vcid):step()
    stmt:close()

    if val ~= nil then return false end

    started = started or os.time()              -- default : from now
    expired = expired or os.time() + 24*60*60   -- default : 24 hours
    
    stmt = self._dbs:prepare("INSERT INTO visitor_table VALUES(?1, ?2, ?3, ?4, datetime(CURRENT_TIMESTAMP, 'localtime'))")
    val = stmt:reset():bind(vcid, visicode, started, expired):step()
    stmt:close()

    if val == nil then
        return true
    end

    return false
end

function Basedb:VisitorLookup(visicode, ocur)
    if self._state < 1 then return false end

    if visicode == nil then return false end

    ocur = ocur or os.time()
    local stmt = self._dbs:prepare("SELECT * FROM visitor_table WHERE (visicode = ?1 and started <= ?2 and expired >= ?3)")
    local val = stmt:reset():bind(visicode, ocur, ocur):step()
    stmt:close()

    if val ~= nil then return true end

    return false
end

function Basedb:VisitorDelete(vcid)
    if self._state < 1 then return false end

    if vcid == nil then return false end

    local stmt = self._dbs:prepare("DELETE FROM visitor_table WHERE vcid = ?1")
    local val = stmt:reset():bind(vcid):step()
    stmt:close()

    return true
end

function Basedb:VisitorClean(expired)
    if self._state < 1 then return false end

    expired = expired or os.time()

    local stmt = self._dbs:prepare("DELETE FROM visitor_table WHERE expired < ?1")
    local val = stmt:reset():bind(expired):step()
    stmt:close()

    return true
end

function Basedb:VisitorFetch(offset, limit)    
    if self._state < 1 then return nil end

    if offset == nil then offset = 0 end
    if limit == nil then limit = 10 end

    local stmt = self._dbs:prepare("SELECT vcid FROM visitor_table LIMIT ?1 OFFSET ?2")
    local vcids, nr = stmt:reset():bind(limit, offset):resultset()
    stmt:close()

    if nr > 0 then
        return vcids.vcid, nr
    else
        return nil, 0
    end
end

function Basedb:BytesCompare(a, b)
    if a == nil  or a.data == nil then return false end
    if b == nil  or b.data == nil then return false end

    if a.len ~= b.len then return false end
    if ffi.typeof(a.data) ~= ffi.typeof(b.data) then return false end
    
    for i = 0, a.len - 1 do
        if a.data[i] ~= b.data[i] then
            return false
        end
    end
    
    return true
end

function Basedb:ConfSet(keyStr, valStr, opts)
    if self._state < 1 then return false, 0 end
    if opts == nil then opts = 1 end

    local stmt = nil
    local ret = nil
    local rows, options = self:ConfGet(keyStr)

    if rows ~= nil then
        if  options > 0 then
            stmt = self._dbs:prepare("REPLACE INTO  conf_table VALUES(?1, ?2, ?3, datetime(CURRENT_TIMESTAMP, 'localtime'))")
            ret = stmt:reset():bind(keyStr, valStr, options):step()
        else
            return false, options
        end
    else
        stmt = self._dbs:prepare("INSERT INTO conf_table VALUES(?1, ?2, ?3, datetime(CURRENT_TIMESTAMP, 'localtime'))")
        ret = stmt:reset():bind(keyStr, valStr, opts):step()
        options = opts
    end

    stmt:close()

    if ret == nil then return true, options else return false, options end
end

function Basedb:ConfGet(keyStr)
    if self._state < 1 then return nil end

    local stmt = self._dbs:prepare("SELECT * FROM conf_table WHERE key = ?1")
    local val = stmt:reset():bind(keyStr):step()
    stmt:close()
    
    if val == nil then return nil else return val[2], tonumber(val[3]), val[4] end
end

function Basedb:ConfList()
    if self._state < 1 then return nil end

    local stmt = self._dbs:prepare("SELECT * FROM conf_table")
    local rows, nr =  stmt:resultset()
    stmt:close()

    local cs = {}
    
    for i = 1, nr do
        local kv = {}
        kv["key"] = rows[1][i]
        kv["value"] = rows[2][i]
        kv["options"]  = tonumber(rows[3][i])
        kv["modified"] = rows[4][i]
        table.insert(cs, kv)
        -- cs[rows[1][i]] = rows[2][i]
    end

    return cs, nr
end

function Basedb:AdminInsert(name, passwd, acl)
    if self._state < 1 then return false end

    if name == nil or passwd == nil then return false end
    if acl == nil then acl = 1 end

    local stmt = self._dbs:prepare("SELECT * FROM admin_table WHERE name = ?1")
    local val = stmt:reset():bind(name):step()
    stmt:close()

    if val ~= nil then return false end
    
    stmt = self._dbs:prepare("INSERT INTO admin_table VALUES(?1, ?2, ?3, datetime(CURRENT_TIMESTAMP, 'localtime'), datetime(CURRENT_TIMESTAMP, 'localtime'))")
    val = stmt:reset():bind(name, passwd, acl):step()
    stmt:close()

    if val == nil then
        return true
    end

    return false
end

function Basedb:AdminUpdate(name, passwd, acl)
    if self._state < 1 then return false end

    if name == nil then return false end

    local admin = self:AdminLookup(name)
    if admin == nil then return false end

    if passwd == nil then passwd = admin.passwd end
    if acl == nil then acl = admin.acl end

    local stmt = self._dbs:prepare("UPDATE admin_table SET passwd = ?1, acl = ?2 WHERE name = ?3")
    stmt:reset():bind(passwd, acl, name):step()
    stmt:close()
    
    return true
end


function Basedb:AdminLookup(name)
    if self._state < 1 then return false end

    if name == nil then return nil end

    local stmt = self._dbs:prepare("SELECT * FROM admin_table WHERE name = ?1")
    local val = stmt:reset():bind(name):step()
    stmt:close()

    if val ~= nil then
        local admin = {}

        admin.name      = val[1]
        admin.passwd    = val[2]
        admin.acl       = val[3]
        admin.create    = val[4]
        admin.modify    = val[5]

        return admin
    end

    return nil
end

function Basedb:AdminDelete(name)
    if self._state < 1 then return false end

    if name == nil then return false end

    local stmt = self._dbs:prepare("DELETE FROM admin_table WHERE name = ?1")
    local val = stmt:reset():bind(name):step()
    stmt:close()

    return true
end

return Basedb
