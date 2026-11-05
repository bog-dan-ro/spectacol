/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Spectacol UART hook API v1. Evaluated inside a factory with a narrow `host` API.
// Commands operate on host TCP/UDP sockets; this is an ESP AT protocol model.
let line = "", echo = true, mux = 0, mode = 1, joined = true;
let pending = null, sockets = {}, connecting = {}, closing = {};
let receiveQueue=[], receiveBytes=0;
// These describe the virtual station; sockets still use the host network.
let station = ["127.0.0.1", "127.0.0.1", "255.0.0.0"], dns = ["8.8.8.8", "8.8.4.4"], dhcp = 1;
function ipv4(value) {
    return /^(?:\d{1,3}\.){3}\d{1,3}$/.test(value) && value.split(".").every(n => Number(n)<=255);
}
function deliver(data) {
    if(receiveBytes+data.length>1024*1024) {
        host.log("ESP UART receive backlog exceeded 1 MiB"); host.network("close",-1); sockets={}; receiveQueue=[]; receiveBytes=0; return;
    }
    receiveQueue.push(data); receiveBytes+=data.length; drain();
}
function drain() {
    while(receiveQueue.length) {
        const data=receiveQueue[0]; const accepted=host.receive(data);
        receiveBytes-=accepted;
        if(accepted<data.length) { receiveQueue[0]=data.slice(accepted); host.setTimer(900,5); return; }
        receiveQueue.shift();
    }
}
function bytes(text) { const result=[]; for(let i=0;i<text.length;++i) result.push(text.charCodeAt(i)&255); return result; }
function reply(text) { deliver(bytes(text)); }
function ok() { reply("\r\nOK\r\n"); }
function error() { reply("\r\nERROR\r\n"); }
function args(text) {
    const values=[]; let value="", quoted=false, escape=false;
    for(const c of text) {
        if(escape) { value+=c; escape=false; }
        else if(c==="\\") escape=true;
        else if(c==='"') quoted=!quoted;
        else if(c==="," && !quoted) { values.push(value); value=""; }
        else value+=c;
    }
    values.push(value); return values;
}
function connectedText(id, text) { return (mux ? id+"," : "")+text+"\r\n"; }
function command(cmd) {
    if(cmd==="AT") return ok();
    if(cmd==="ATE0" || cmd==="ATE1") { echo=cmd==="ATE1"; return ok(); }
    if(cmd==="AT+RST" || cmd==="AT+RESTORE") { ok(); return reset(); }
    if(cmd==="AT+GMR") { reply("\r\nAT version:Spectacol UART API 1\r\nSDK version:host sockets\r\n"); return ok(); }
    if(cmd==="AT+CIPMUX?") { reply("\r\n+CIPMUX:"+mux+"\r\n"); return ok(); }
    if(/^AT\+CIPMUX=[01]$/.test(cmd)) { mux=Number(cmd.slice(-1)); return ok(); }
    if(/^AT\+CWMODE(?:_CUR|_DEF)?=/.test(cmd)) { mode=Number(cmd.split("=")[1]); return mode>=1&&mode<=3 ? ok() : error(); }
    if(/^AT\+CWMODE(?:_CUR|_DEF)?\?$/.test(cmd)) { reply("\r\n+CWMODE:"+mode+"\r\n"); return ok(); }
    if(/^AT\+CWJAP(?:_CUR|_DEF)?=/.test(cmd)) { joined=true; reply("\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\n"); return ok(); }
    if(/^AT\+CWJAP(?:_CUR|_DEF)?\?$/.test(cmd)) { reply(joined?'\r\n+CWJAP:"Spectacol","00:00:00:00:00:00",1,-40\r\n':"\r\nNo AP\r\n"); return ok(); }
    if(cmd==="AT+CWQAP") { joined=false; host.network("close",-1); sockets={}; return ok(); }
    if(cmd==="AT+CWLAP") { reply('\r\n+CWLAP:(0,"Spectacol",-40,"00:00:00:00:00:00",1)\r\n'); return ok(); }
    const config = /^AT\+(CIPSTA|CIPDNS|CWDHCP)(_CUR|_DEF)?([?=])(.*)$/.exec(cmd);
    if(config) {
        const name=config[1], prefix="+"+name+(config[2] || ""), query=config[3]==="?", a=args(config[4]);
        if(query && config[4]!=="") return error();
        if(name==="CIPSTA") {
            if(query) {
                ["ip","gateway","netmask"].forEach((key,i) => reply("\r\n"+prefix+":"+key+':"'+(joined ? station[i] : "0.0.0.0")+'"\r\n'));
            } else {
                if(a.length<1 || a.length>3 || !a.every(ipv4)) return error();
                station=a.concat(station.slice(a.length)); dhcp=0;
            }
        } else if(name==="CIPDNS") {
            if(query) dns.forEach(address => reply("\r\n"+prefix+":"+address+"\r\n"));
            else {
                if(!["0","1"].includes(a[0]) || a.length>3 || (a[0]==="1" && a.length<2) || !a.slice(1).every(ipv4)) return error();
                dns=a[0]==="0" ? ["8.8.8.8","8.8.4.4"] : a.slice(1).concat(dns.slice(a.length-1));
            }
        } else {
            if(query) reply("\r\n"+prefix+":"+dhcp+"\r\n");
            else {
                if(a.length!==2 || !["0","1"].includes(a[0]) || !["1","2","3"].includes(a[1])) return error();
                dhcp=Number(a[0]);
                if(dhcp) station=["127.0.0.1","127.0.0.1","255.0.0.0"];
            }
        }
        return ok();
    }
    if(cmd==="AT+CIFSR") { reply('\r\n+CIFSR:STAIP,"'+(joined ? station[0] : "0.0.0.0")+'"\r\n+CIFSR:STAMAC,"00:00:00:00:00:00"\r\n'); return ok(); }
    if(cmd==="AT+CIPSTATUS") {
        reply("\r\nSTATUS:"+(Object.keys(sockets).length ? 3 : joined ? 2 : 5)+"\r\n");
        for(const id of Object.keys(sockets)) { const s=sockets[id]; reply('+CIPSTATUS:'+id+',"'+s.type+'","'+s.host+'",'+s.port+',0,0\r\n'); }
        return ok();
    }
    if(cmd.startsWith("AT+CIPSTART=")) {
        const a=args(cmd.slice(12)); const id=mux ? Number(a.shift()) : 0;
        const type=a[0], address=a[1], port=Number(a[2]);
        if(!joined || id<0 || id>4 || sockets[id] || !["TCP","UDP"].includes(type) || !address || port<1 || port>65535) return error();
        sockets[id]={type:type,host:address,port:port}; connecting[id]=true;
        host.network("open",id,address,port,[],type==="UDP"); return;
    }
    if(cmd.startsWith("AT+CIPSEND=")) {
        const a=args(cmd.slice(11)); const id=mux ? Number(a.shift()) : 0; const length=Number(a[0]);
        if(!sockets[id] || connecting[id] || !Number.isInteger(length) || length<0 || length>65536) return error();
        pending={id:id,length:length,bytes:[]}; reply("\r\nOK\r\n> ");
        if(!length) { host.network("write",id,"",0,[]); pending=null; }
        return;
    }
    if(cmd==="AT+CIPCLOSE" || cmd.startsWith("AT+CIPCLOSE=")) {
        const id=mux ? Number(cmd.split("=")[1]) : 0;
        if(!sockets[id]) return error();
        host.network("close",id); delete sockets[id]; reply(connectedText(id,"CLOSED")); return ok();
    }
    if(cmd.startsWith("AT+CIPDOMAIN=")) { host.network("resolve",0,args(cmd.slice(13))[0]); return; }
    if(/^AT\+UART(?:_CUR|_DEF)?=/.test(cmd) || cmd==="AT+CIPMODE=0" || cmd==="AT+CIPDINFO=0" || cmd==="AT+SLEEP=0" || /^AT\+SYSSTORE=[01]$/.test(cmd)) return ok();
    error();
}
function reset() {
    host.network("close",-1); line=""; echo=true; mux=0; mode=1; joined=true;
    pending=null; sockets={}; connecting={}; closing={}; receiveQueue=[]; receiveBytes=0; host.cancelTimer(900); reply("\r\nready\r\n");
}
return {
    apiVersion:1,
    reset:reset,
    onByte:function(byte) {
        if(pending) {
            pending.bytes.push(byte);
            if(pending.bytes.length===pending.length) {
                reply("\r\nRecv "+pending.length+" bytes\r\n");
                host.network("write",pending.id,"",0,pending.bytes); pending=null;
            }
            return;
        }
        if(echo) deliver([byte]);
        if(byte===10) { const cmd=line; line=""; if(cmd) command(cmd); }
        else if(byte!==13) { if(line.length<4096) line+=String.fromCharCode(byte); else { line=""; error(); } }
    },
    onTimer:function(id) { if(id===900) drain(); },
    onNetwork:function(event) {
        const id=event.id;
        if(event.type!=="resolved" && !sockets[id]) return;
        if(event.type==="connected") { delete connecting[id]; reply(connectedText(id,"CONNECT")); ok(); }
        else if(event.type==="sent") reply("\r\nSEND OK\r\n");
        else if(event.type==="data") {
            reply("\r\n+IPD,"+(mux ? id+"," : "")+event.bytes.length+":"); deliver(event.bytes);
        } else if(event.type==="resolved") {
            if(event.address) { reply('+CIPDOMAIN:"'+event.address+'"\r\n'); ok(); } else error();
        } else if(event.type==="closed" || event.type==="error") {
            if(!sockets[id]) return;
            delete sockets[id]; const wasConnecting=connecting[id]; delete connecting[id];
            if(wasConnecting) error(); else reply(connectedText(id,"CLOSED"));
        }
    }
};
