#!/usr/local/bin/node
/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

const request = require("request-promise");
var settings = require('./settings.js');
var session = require('./session.js');
//var authen = require('./authen.js');

var cf = settings.load();
var ses = session.load();

var methods = {};

methods.set = async function(key, val){

    var kv = Object();
    kv[key] = val;
    
    var body = {
        action:"set", 
        params:[
            kv
        ]
    }

    var options = {
        method: 'POST',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/config',
        headers:{
            'Content-Type':'application/json',
            'Token':ses.Token
        },

        body: body,
        json:true
    };

    return await request(options);
}

methods.get = async function(key){
    var body = {
        action:"get", 
        params:[
            key
        ]
    }

    var options = {
        method: 'POST',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/config',
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

    rsp = await methods.set('wifi-ipv4-address', '192.168.10.20');
    console.log(rsp);

    rsp = await methods.get('wifi-ipv4-address');
    console.log(rsp);


}

module.exports = methods;

selftest();
