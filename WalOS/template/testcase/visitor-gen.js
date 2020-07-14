#!/usr/local/bin/node
/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

const request = require("request-promise");
const uuidv1 = require('uuid/v1');
const uuidParse = require('uuid-parse');
const random = require('random')
var fs = require('fs');
const os = require('os');

var methods = {};

methods.GenID = function (){
    return Buffer.from(uuidParse.parse(uuidv1())).toString('base64');
}

methods.NewGCode = function(scodeid){
    var gcode = new Object();

    gcode.vcid = scodeid;
    gcode.visicode = random.int(100000, 999999).toString();
    gcode.started = new Date().getTime() / 1000 | 0;
    gcode.expired = gcode.started + 3600*24;

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
        gcdel.vcid = gc.vcid;

        delgcode.params.push(gcdel);
    }

    fs.writeFileSync('visitor_add.json', JSON.stringify(addgcode, null, 4));
    fs.writeFileSync('visitor_del.json', JSON.stringify(delgcode, null, 4));
}


function selftest(){
    methods.Gen(10);
}

module.exports = methods;

selftest();
