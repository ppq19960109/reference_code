#!/usr/local/bin/node
/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

const request = require("request-promise");
const uuidv1 = require('uuid/v1');
const uuidParse = require('uuid-parse');
const bs58 = require('bs58');

var settings = require('./settings.js');
var session = require('./session.js');
var fs = require('fs');

var cf = settings.load();
var ses = session.load();


var methods = {};

methods.GenID = function (){
    return bs58.encode(Buffer.from(uuidParse.parse(uuidv1())));
}

methods.add = async function(codeid, userid, code, expired){

    var body = {
        action:"add", 
        params:[
            {'scodeid':codeid, 'userid':userid, 'gcode':code, 'expired':expired }
        ]
    }

    var options = {
        method: 'POST',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/guest',
        headers:{
            'Content-Type':'application/json',
            'Token':ses.Token
        },

        body: body,
        json:true
    };

    console.log("add => " + JSON.stringify(body));

    return await request(options);
}

methods.del = async function(codeid){

    var body = {
        action:"del", 
        params:[
            {'scodeid':codeid }
        ]
    }

    var options = {
        method: 'POST',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/guest',
        headers:{
            'Content-Type':'application/json',
            'Token':ses.Token
        },

        body: body,
        json:true
    };

    return await request(options);
}

async function selftest(){
    var scodeid  = 12345;
    var userid = methods.GenID();
    var gcode = '547626';
    var expired = '2019-08-24 18:20:21';

    var rsp = await methods.add(scodeid, userid, gcode, expired);
    console.log(rsp);

    rsp = await methods.del(scodeid);
    console.log(rsp);
}

module.exports = methods;

selftest();
