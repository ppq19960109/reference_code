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

methods.add = async function(user){
    var body = {
        action:"add", 
        params:[
            user
        ]
    }

    var options = {
        method: 'POST',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/user',
        headers:{
            'Content-Type':'application/json',
            'Token':ses.Token
        },

        body: body,
        json:true
    };

    return await request(options);
}

methods.del = async function(user){
    var body = {
        action:"del", 
        params:[
            user
        ]
    }

    var options = {
        method: 'POST',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/user',
        headers:{
            'Content-Type':'application/json',
            'Token':ses.Token
        },

        body: body,
        json:true
    };
    return await request(options);
}

methods.update = async function(user){
    var body = {
        action:"update", 
        params:[
            user
        ]
    }

    var options = {
        method: 'POST',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/user',
        headers:{
            'Content-Type':'application/json',
            'Token':ses.Token
        },

        body: body,
        json:true
    };

    return await request(options);
}

methods.info = async function(user){
    var body = {
        action:"info", 
        params:[
            user
        ]
    }

    var options = {
        method: 'POST',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/user',
        headers:{
            'Content-Type':'application/json',
            'Token':ses.Token
        },

        body: body,
        json:true
    };

    return await request(options);
}

methods.list = async function(offset, limit){

    var body = {
        action:"list",
        offset:offset,
        limit:limit
    }

    var options = {
        method: 'POST',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/user',
        headers:{
            'Content-Type':'application/json',
            'Token':ses.Token
        },

        body: body,
        json:true
    };

    console.log(JSON.stringify(body))
    return await request(options);
}

methods.download = async function(userid){
    var body = {
        action : 'download',
        image : 'user',
        userid:userid
    }

    var options = {
        method: 'POST',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/download',
        headers:{
            'Content-Type':'application/json',
            encoding: null,
            Token:ses.Token
        },

        body: body,
        json:true
    };

    var info = await methods.info({"userid":userid});
    if(info.retcode == 0){
        if(info.params[0].status > 0){
            return await request(options);
        }
    }

    return null;
}

methods.upload = async function(userid, userimage) {

    var body = fs.readFileSync(userimage);
    if(Buffer.byteLength(body) < 1024){
        return { retcode : 1024 };
    }

    var options = {
        method:'PUT',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/upload',
        headers:{
            'Content-Type':'application/image',
            'User-ID': userid,
            'upload': 'user-image',
            'Token':ses.Token
        },

        body: body,
        json:false
    };

    return await request(options);
}

methods.addui = async function(userid, userui){

    var body = fs.readFileSync(userui);
    if(Buffer.byteLength(body) < 1024){
        return { retcode : 1024 };
    }

    var options = {
        method:'PUT',
        uri: 'http://' + cf.host + ':' + cf.port + '/api/upload',
        headers:{
            'Content-Type':'application/image',
            'User-ID': userid,
            'upload': 'ui-image',
            'Token':ses.Token
        },

        body: body,
        json:false
    };

    return await request(options);
}


async function selftest(){
    var userid = methods.GenID();

    const user_add = {
        "userid": userid,
        "name":"测试1", 
        "desc":"测试部/设备测试", 
        "others":"123948372", 
        "perm":1,
        "expire": '2019-08-24 18:20:21'
    };

    var rsp = await methods.add(user_add);
    console.log(rsp);


    const user_update = {
        "userid": userid,
        "name":"测试33", 
        "desc":"测试部/设备测试", 
        "others":"123948372", 
        "perm":1,
        "expire": '2019-08-24 18:20:21'
    };

    rsp = await methods.update(user_update);
    console.log(rsp);

    const user_info = {
        "userid":userid
    };

    rsp = await methods.info(user_info);
    console.log(rsp);


    rsp = await methods.list(0, 10);
    console.log(rsp);

/*
    var userid = '19283299';
    rsp = await methods.download(userid);
    if(rsp != null)
        fs.writeFileSync(userid + '.nv12', rsp);
*/

    const user_del = {
        "userid":userid,
    };

    rsp = await methods.del(user_del);
    console.log(rsp);
}

module.exports = methods;

console.log(methods.GenID())

selftest();
