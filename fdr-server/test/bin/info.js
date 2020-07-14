#!/usr/local/bin/node
/*
 * Copyright (c) 2019, Bashi Tech. All rights reserved.
 */

 
const request = require("request-promise");
var settings = require('./settings.js');

var cf = settings.load();

var methods = {};


methods.info = async function(){

    var options = {
        method: 'GET',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/info',
        headers:{
            'Content-Type':'application/json'
        },
        body: {
            action: 'info'
        },
        json: true // Automatically stringifies the body to JSON
    };

    return await request(options);
}

async function selftest(){
    var info = await methods.info();

    console.log(JSON.stringify(info, null, 4));
}

module.exports = methods;

selftest();


