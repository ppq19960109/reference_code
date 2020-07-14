#!/bin/sh

./deps/LuaJIT-2.1.0-beta3/luajit -b -g -a arm lua/Utils.lua ./targets/linux-xm650/app/Utils.bin
./deps/LuaJIT-2.1.0-beta3/luajit -b -g -a arm lua/Basedb.lua ./targets/linux-xm650/app/Basedb.bin
./deps/LuaJIT-2.1.0-beta3/luajit -b -g -a arm lua/Logrdb.lua ./targets/linux-xm650/app/Logrdb.bin
./deps/LuaJIT-2.1.0-beta3/luajit -b -g -a arm lua/LogrGen.lua ./targets/linux-xm650/app/LogrGen.bin
./deps/LuaJIT-2.1.0-beta3/luajit -b -g -a arm lua/CPAdapter.lua ./targets/linux-xm650/app/CPAdapter.bin
./deps/LuaJIT-2.1.0-beta3/luajit -b -g -a arm lua/OSAdapter.lua ./targets/linux-xm650/app/OSAdapter.bin
./deps/LuaJIT-2.1.0-beta3/luajit -b -g -a arm lua/CloudApp.lua ./targets/linux-xm650/app/CloudApp.bin
./deps/LuaJIT-2.1.0-beta3/luajit -b -g -a arm lua/WebApp.lua ./targets/linux-xm650/app/WebApp.bin
./deps/LuaJIT-2.1.0-beta3/luajit -b -g -a arm lua/WalOS.lua ./targets/linux-xm650/app/WalOS.bin
