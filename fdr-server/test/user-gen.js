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

const default_count = 1000;
const feature_file = 'feature.bin';
const user_add_file = 'user_add.json';
const user_del_file = 'user_del.json';
const user_update_file = 'user_update.json';
const user_info_file = 'user_info.json';
const user_list_file = 'user_list.json';

var methods = {};

var names = [
    "张三",
    "张三.李四",
    "李四",
    "李四.张三",
    "王二麻子",
    "王二麻子.诺夫",
    "王二麻子.博格",
];

var descs = [
    "销售部",
    "市场部",
    "研发部",
    "国际贸易部",
    "新技术研究与开发部",
    "供应链",
];

var others = [
    "13457639976",
    "13663548853",
    "13763548853",
    "18563548853",
    "13263548853",
    "17763548853",
];

methods.GenID = function (){
    return bs58.encode(Buffer.from(uuidParse.parse(uuidv1())));
}

methods.NewUser = function(){
    var user = new Object();
    var fary = new Float32Array(512);
    //var d = new Date();

    user.userid  = methods.GenID();

    user.name   = names[random.int(0, names.length -1)];
    user.desc   = descs[random.int(0, descs.length -1)];
    user.others = others[random.int(0, others.length -1)];
    user.perm   = 1;
    user.expire = "2019-12-09 14:52:04";

    for(var i = 0; i < 512;  i++){
        fary[i] = random.float();
    }
    
    user.feature = bs58.encode(Buffer.from(fary.buffer));

    fs.appendFileSync(feature_file, Buffer.from(fary.buffer));

    //user.expire = Math.round(d.getTime()/1000) + 3600*24*365;

    return user;
}

methods.UpdateUser = function(user){
    //var d = new Date();

    user.desc   = descs[random.int(0, descs.length -1)];
    user.others = others[random.int(0, others.length -1)];
    user.perm   = 1;
    user.expire = "2019-12-09 14:52:04";

    //user.expire = Math.round(d.getTime()/1000) + 3600*24*365;

    return user;
}


methods.Gen = function(count){
    var addUser = new Object();
    var delUser = new Object();
    var infoUser = new Object();
    var updateUser = new Object();
    var listUser = new Object();

    var i;

    addUser = {
        action : "add",
        params : new Array()
    }

    delUser = {
        action : "del",
        params : new Array()
    }

    infoUser = {
        action : "info",
        params : new Array()
    }

    updateUser = {
        action : "update",
        params : new Array()
    }

    for(i = 0; i < count; i++){
        var au = methods.NewUser();
        var uu = Object.create(au);
        var ot = Object.create(au);
        var dl = new Object();

        uu.userid = au.userid;
        uu.name   = au.name;
        uu.create = au.create;
        uu.feature = au.feature;

        addUser.params.push(au);

        methods.UpdateUser(uu)
        updateUser.params.push(uu);

        dl.userid = ot.userid;
        delUser.params.push(dl);

        infoUser.params.push(dl);
    }

    fs.writeFileSync(user_add_file, JSON.stringify(addUser, null, 4));
    fs.writeFileSync(user_del_file, JSON.stringify(delUser, null, 4));
    fs.writeFileSync(user_update_file, JSON.stringify(updateUser, null, 4));
    fs.writeFileSync(user_info_file, JSON.stringify(infoUser, null, 4));

    listUser.action = "list";
    listUser.offset = 1;
    listUser.limit  = 6;

    fs.writeFileSync(user_list_file, JSON.stringify(listUser, null, 4));
}


function selftest(){

    if(fs.existsSync(feature_file))
        fs.unlinkSync(feature_file);
    
    if(fs.existsSync(user_add_file))
        fs.unlinkSync(user_add_file);

    if(fs.existsSync(user_del_file))
        fs.unlinkSync(user_del_file);

    if(fs.existsSync(user_update_file))
        fs.unlinkSync(user_update_file);

    if(fs.existsSync(user_info_file))
        fs.unlinkSync(user_info_file);
        
    if(fs.existsSync(user_list_file))
        fs.unlinkSync(user_list_file);

    methods.Gen(default_count);
}

module.exports = methods;

selftest();
