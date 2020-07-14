--
-- Copyright (c) 2019, Bashi Tech. All rights reserved.
--

local ffi = require("ffi")
local log = require("log")
local bs64 = require("base64")

Utils = {
    FDRSDK_ERRCODE_NOMEM              = 100,
    FDRSDK_ERRCODE_INVALID            = 101,
    FDRSDK_ERRCODE_DBOPS              = 102,
    FDRSDK_ERRCODE_NOEXIT             = 103,
    FDRSDK_ERRCODE_INVALID_FACE       = 104,
    FDRSDK_ERRCODE_TOKEN              = 105,
    FDRSDK_ERRCODE_OVERLOAD           = 106,
    FDRSDK_ERRCODE_CHECKSUM           = 107,
    FDRSDK_ERRCODE_SNAPSHOT           = 108,
    FDRSDK_ERRCODE_FILEOPS            = 109,

    ECDSA_PRIKEY_SIZE = 32,
    ECDSA_PUBKEY_SIZE = 33,
    ECDSA_SIGNATURE_SIZE = 64,

    ALGM_FEATURE_FLOAT_SIZE   = 512,
    ALGM_FEATURE_BUFF_SIZE    = 2048,
}

ffi.cdef[[
typedef struct bcp_header{
    uint16_t  mtype;
    uint16_t  bsize;
    uint32_t  seqence;
    uint64_t  session;
}__attribute__((packed)) bcp_header_t;

void bcpheader_tonetwork(bcp_header_t *bh);
void bcpheader_tohost(bcp_header_t *bh);

int ECDSA_Sign(const void *prikey, const char *devid, const char *ts, void *checksum);
int ECDSA_Verify(const void *pubkey, const char *devid, const char *ts, const char *checksum);

double Timestamp();
]]

function Utils:NewHeader()
    local header = ffi.new("bcp_header_t[1]")
    return header;
end

function Utils:ToHost(header)
    ffi.C.bcpheader_tohost(header)
end

function Utils:ToNetwork(header)
    ffi.C.bcpheader_tonetwork(header)
end

function Utils:Timestamp()
    return ffi.C.Timestamp()
end

function Utils:ECDSA_Sign(prikey, devid, ts)
    local signature = ffi.new("uint8_t[ECDSA_SIGNATURE_SIZE]")
    
    local ret = ffi.C.ECDSA_Sign(prikey, devid, ts, signature);
    local sign = ffi.string(signature, self.ECDSA_SIGNATURE_SIZE)
    signature = nil

    if ret == 0 then
        
        local s = bs64.encode(sign)
        sign = nil
        return s
    else 
        return nil
    end
end

function Utils:ECDSA_Verify(pubkey, devid, ts, signature)
    local sign = bs64.decode(signature)

    if #sign < self.ECDSA_SIGNATURE_SIZE then
        log:warn("ECDSA_Verify => size is :"..tostring(#sign))
        return false 
    end

    local ret = ffi.C.ECDSA_Verify(pubkey, devid, ts, sign);
    if ret == 0 then return true else return false end
end

function Utils:ReadAll(fname)
    local f = io.open(fname)
    if f == nil then
        log:warn("ReadAll -> open "..fname.." fail")
        return nil
    end

    local d = f:read("a")
    f:close()
    f = nil
    
    return d
end

--[[
print_dump是一个用于调试输出数据的函数，能够打印出nil,boolean,number,string,table类型的数据，以及table类型值的元表
参数data表示要输出的数据
参数showMetatable表示是否要输出元表
参数lastCount用于格式控制，用户请勿使用该变量
]]
function Utils:DumpTable(data, showMetatable, lastCount)
    if type(data) ~= "table" then
        --Value
        if type(data) == "string" then
            io.write("\"", data, "\"")
        else
            io.write(tostring(data))
        end
    else
        --Format
        local count = lastCount or 0
        count = count + 1
        io.write("{\n")
        --Key
        for key,value in pairs(data) do
            for i = 1,count do io.write("\t") end
            if type(key) == "string" then
                io.write("\"", key, "\" = ")
            elseif type(key) == "number" then
                io.write("[", key, "] = ")
            else
                io.write(tostring(key))
            end

            if type(value) ~= "table" then
                --Value
                if type(value) == "string" then
                    io.write("\"", value, "\"")
                else
                    io.write(tostring(value))
                end
            end

            io.write(",\n") 
        end
        --Format
        for i = 1,lastCount or 0 do io.write("\t") end
        io.write("}")
    end
    --Format
    if not lastCount then
        io.write("\n")
    end
end

function Utils:SplitToTable(inputstr, sep)
    if sep == nil then
            sep = "%s"
    end

    local t={}
    for str in string.gmatch(inputstr, "([^"..sep.."]+)") do
            table.insert(t, str)
    end

    return t
end

return  Utils