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

// Optional raw Pi UART bridge to a host TCP service. Port 0 means disconnected.
let state="closed", queued=[], incoming=[], incomingBytes=0, config=host.configuration();
function reset() {
    host.network("close",-1); state="closed"; queued=[]; incoming=[]; incomingBytes=0;
    config=host.configuration(); host.cancelTimer(0); host.cancelTimer(1);
}
function receive(data) {
    if(incomingBytes+data.length>1024*1024) { host.log("Pi UART receive backlog exceeded 1 MiB"); reset(); return; }
    incoming.push(data); incomingBytes+=data.length; drain();
}
function drain() {
    while(incoming.length) {
        const data=incoming[0]; const accepted=host.receive(data); incomingBytes-=accepted;
        if(accepted<data.length) { incoming[0]=data.slice(accepted); host.setTimer(1,5); return; }
        incoming.shift();
    }
}
function flush() { if(state==="connected" && queued.length) { host.network("write",0,"",0,queued); queued=[]; } }
return {
    apiVersion:1,
    reset:reset,
    onByte:function(byte) {
        if(!config.port) return;
        if(queued.length>=65536) { host.log("Pi UART transmit backlog exceeded 64 KiB"); reset(); return; }
        if(state==="closed") { state="connecting"; host.network("open",0,config.host,config.port); }
        queued.push(byte);
        if(state==="connected") host.setTimer(0,0);
    },
    onTimer:function(id) { if(id===0) flush(); else if(id===1) drain(); },
    onNetwork:function(event) {
        if(event.type==="connected") { state="connected"; flush(); }
        else if(event.type==="data") receive(event.bytes);
        else if(event.type==="closed" || event.type==="error") reset();
    }
};
