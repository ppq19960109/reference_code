#!/usr/local/bin/node
/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

const request = require("request-promise");
const uuidv1 = require('uuid/v1');
const uuidParse = require('uuid-parse');
const bs58 = require('bs58');
const random = require('random')
var fs = require('fs');

var methods = {};

methods.GenID = function (){
    return bs58.encode(Buffer.from(uuidParse.parse(uuidv1())));
}

methods.NewGCode = function(scodeid){
    var gcode = new Object();

    gcode.scodeid = scodeid;
    gcode.userid  = methods.GenID();

    gcode.gcode = random.int(100000, 999999).toString();
    gcode.expired = "2019-12-09 14:52:04";

    return gcode;
}


methods.Gen = function(total){
    var addgcode = new Object();
    var delgcode = new Object();
    var i;

    addgcode.action = "add";
    addgcode.params = new Array();

    delgcode.action = "del";
    delgcode.params = new Array();

    for(i = 0; i < total; i++){

        var gc =  methods.NewGCode(i);
        addgcode.params.push(gc);

        var gcdel = Object.create(gc);
        gcdel.scodeid = gc.scodeid;

        delgcode.params.push(gcdel);
    }

    fs.writeFileSync('guest_insert.json', JSON.stringify(addgcode, null, 4));
    fs.writeFileSync('guest_delete.json', JSON.stringify(delgcode, null, 4));
}


function selftest(){
    methods.Gen(100);
}

module.exports = methods;

selftest();
