#!/usr/local/bin/node
/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

const request = require("request-promise");
var settings = require('./settings.js');
var session = require('./session.js');

var cf = settings.load();
var ses = session.load();

var methods = {};

methods.fetch = async function(start, number) {

    var body = {
        action:"fetch",
        "seqnum": start,
        "number": number
    }

    var options = {
        method: 'POST',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/logger',
        headers:{
            'Content-Type':'application/json',
            'Token':ses.Token
        },

        body: body,
        json:true
    };

    return await request(options);
}

methods.download = async function(timestamp){
    var body = {
        action : 'download',
        image : 'logger',
        timestamp: timestamp
    }

    var options = {
        method: 'POST',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/download',
        headers:{
            'Content-Type':'application/json',
            encoding: null,
            'Token':ses.Token
        },

        body: body,
        json:true
    };

    var rsp = await request(options);

    // to small rsp data, failed
    if(Buffer.byteLength(rsp) < 1024){
        return null;
    }
    
    return rsp;
}

async function selftest(){

    rsp = await methods.fetch(1, 10);
    console.log(rsp);

    rsp = await methods.download('1572004915630');
    console.log(Buffer.byteLength(rsp));


}

module.exports = methods;

selftest();
