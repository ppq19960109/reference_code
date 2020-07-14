#!/usr/local/bin/node
/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

 
var fs = require('fs');
var rootpath = __dirname + '/../'

var methods = {};

methods.load = function(){
    var cf = fs.readFileSync(rootpath + 'configs/settings.json');

    return JSON.parse(cf);
}


function selftest(){
    var cf = methods.load();

    console.log(JSON.stringify(cf, null, 4));
}

module.exports = methods;

// selftest();
