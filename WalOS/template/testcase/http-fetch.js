#!/usr/local/bin/node
/*
 * Copyright (c) 2020, Bashi Tech. All rights reserved.
 */

const request = require("request-promise");
const fs = require("fs")                                          
var buffer = require('buffer');
var udp = require('dgram');  

var rootpath = "./"

function Settings() {
    var cf = fs.readFileSync(rootpath + './settings.json');
    return JSON.parse(cf);  
}

function Session() {
    var cf = fs.readFileSync(rootpath + './session.json');
    return JSON.parse(cf);  
}

function ReadRequest(reqfile){
    var cf = fs.readFileSync(rootpath + reqfile);
    return cf;
    // return JSON.parse(cf);  
}

async function HTTPFetch(path, reqfile){
    var body = ReadRequest(reqfile)
    var settings = Settings()
    var session  = Session()

    var options = {
        method: 'POST',
        uri: 'http://' + settings.host + ':' + settings.port + '/api/' + path,
        headers:{
            'Content-Type':'application/json',
            'Content-Length': body.length,
            'Token':session.Token
        },

        body: body
    };

    return await request(options);
}

function CheckRsp(verbose, rsp){
    if (rsp.retcode != 0){
        console.log("fail => ", JSON.stringify(JSON.parse(rsp), null, 4));
    }else if(verbose){
        console.log("success => ", JSON.stringify(JSON.parse(rsp), null, 4));
    }
}

async function TestUser(verbose){
    var rsp;
    
    rsp = await HTTPFetch("user", "user_add.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("user", "user_info.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("user", "user_update.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("user", "user_info.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("user", "user_del.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }
}

async function TestVisitor(verbose){
    var rsp;
    
    rsp = await HTTPFetch("visitor", "visitor_add.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("visitor", "visitor_del.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }
}

async function TestConf(verbose){
    var rsp;
    
    rsp = await HTTPFetch("config", "config_get.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("config", "config_list.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("config", "config_set.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("config", "config_get.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }
}

async function TestLogr(verbose){
    var rsp;

    console.log("TestLogr")
    
    rsp = await HTTPFetch("logger", "logr_fetch.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("logger", "logr_info.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }
}

async function TestInfo(verbose){
    var rsp;
    
    rsp = await HTTPFetch("info", "info.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }
}

async function TestInfo(verbose){
    var rsp;
    
    rsp = await HTTPFetch("info", "info.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }
}


async function TestDevice(verbose){
    var rsp;
    

    rsp = await HTTPFetch("device", "dev_snapshot.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("device", "dev_time.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("device", "dev_apply.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("device", "dev_upgrade.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("device", "dev_reboot.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("device", "dev_reset.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

}

async function TestDownload(verbose){
    var rsp;
    
    rsp = await HTTPFetch("download", "download_logr.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }

    rsp = await HTTPFetch("download", "download_snapshot.json");
    if(verbose){
        console.log(JSON.stringify(JSON.parse(rsp), null, 4));
    }
}

async function UploadSlice(url, token, body){   
    var options = {
        method: 'POST',
        uri: url,
        headers:{
            'Content-Type':'application/json',
            'Content-Length': body.length,
            'Token':token
        },

        body: body
    };

    return await request(options);
}   

async function HTTPUpload(path, fname, uptype, datafile){
    var data = ReadRequest(datafile)
    var settings = Settings()
    var session  = Session()
    var slice_size = 32*1024

    var url = 'http://' + settings.host + ':' + settings.port + '/api/' + path
    var body = {
        action:"pushdata",
        type:uptype,
        filesize:data.length,
        filename:fname,
    }
                                                    
    var count = data.length / slice_size;                                      
    for(var i = 0; i < count; i++){                                             
        var slice = Buffer.alloc(slice_size);                                   
        var len = data.copy(slice, 0, i*slice_size, (i+1)*slice_size);         
        slice = slice.slice(0, len);

        body.offset = i*slice_size;
        body.size   = len
        body.data = slice.toString("base64")

        var rsp = await UploadSlice(url, session.Token, JSON.stringify(body));                     
        var ret = JSON.parse(rsp);

        if(ret.retcode != 0){
            console.log('upload fail : ' + rsp);
            break;
        }
        console.log("upload file:" + fname + " fsize:" + data.length + " offset:" + i*slice_size + "  size:" + len)
    }

    return true  
}

async function TestUpload(verbose){
    var rsp;
    
    // rsp = await HTTPUpload("upload", "BTA-3.0.1.bin", "upgrade", "test.jpg");

    rsp = await HTTPUpload("upload", "8ASDFASF0W238.nv12", "user", "test.jpg");
}

function TestLogrEvent(verbose){                                              
    var client = udp.createSocket('udp4');

    var evdata = ReadRequest("logr_event.json")
    var data = Buffer.from(evdata);                                    

    //client.on('message', function (message, remote) {
    //    console.log(remote.address + ':' + remote.port +' - ' + message);
    //});

    client.send(data,6666,'127.0.0.1',function(error){
        if(error){
            console.log('TestLogrEvent => event send fail');
        }else{
            console.log('TestLogrEvent => event send ok');
        }

        client.close();
    });
}

var verbose = true

//TestInfo(verbose)
//TestConf(verbose)
//TestUser(verbose)
// TestVisitor(verbose)
//TestLogr(verbose)
// TestDownload(verbose)
// TestLogrEvent(verbose)
TestDevice(verbose)

//TestUpload(verbose)
