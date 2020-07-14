#!/usr/local/bin/node
/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */
var fs = require('fs');

function selftest(){
    var def = fs.readFileSync("config.default")

    var d = JSON.parse(def)
    // console.log(d)
    
    var setcfg = new Object();
    setcfg.action = "set";
    setcfg.params = new Array();
    var items = d.items

    for (let [key, value] of Object.entries(items)) {
        if(value.options > 0){
            var s = new Object();
            s[value.key] = value.value

            setcfg.params.push(s)
        }
    }
    fs.writeFileSync('config_set.json', JSON.stringify(setcfg, null, 4));

    var getcfg = new Object();
    getcfg.action = "get";
    getcfg.params = new Array();

    for (let [key, value] of Object.entries(items)) {
        getcfg.params.push(value.key)
    }
    fs.writeFileSync('config_get.json', JSON.stringify(getcfg, null, 4));

    var listcfg = new Object()
    listcfg.action = "list"
    fs.writeFileSync('config_list.json', JSON.stringify(listcfg, null, 4));
}

selftest();
