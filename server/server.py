#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import socket
import json
import os
import time

pwd = os.path.dirname(os.path.abspath(__file__))
if os.path.exists(os.path.join(pwd, "msg.json")):
    db = json.load(open(os.path.join(pwd, "msg.json"), "r"))
"""
{"func":"get_time"}
{"func":"send_msg", "msg":{"from":"1", "to":"2", "time":"23/04/09 19:38", "content":"Hello, world!"}}
{"func":"get_msg_inbox", "id":"2"}
{"func":"get_msg_outbox", "id":"1"}
"""

msg = {
    "from": "",
    "to": "",
    "time": "",
    "content": "",
}

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("0.0.0.0", 6000))
print("UDP bound on port 6000...")

def sendto(msg, addr):
    s.sendto(f'{msg}\r\n'.encode("utf-8"), addr)
    
def send_msg(msg: dict):
    msg = msg.copy()
    # msg.pop("clbk")
    db['msgs'].append(msg)
    json.dump(db, open(os.path.join(pwd, "msg.json"), "w"))

    msg.update({"clbk": "recv_msg"})
    if msg['to'] in db['id']:
        sendto(json.dumps(msg), (db['id'][msg['to']][0], db['id'][msg['to']][1]))
        
def get_msg_inbox(id):
    res = {
        "len": 0,
        "msgs": [],
    }
    for msg in db['msgs']:
        if msg['to'] == id:
            res['msgs'].append(msg)
    res['len'] = len(res['msgs'])
    return res

def get_msg_outbox(id):
    res = {
        "len": 0,
        "msgs": [],
    }
    for msg in db['msgs']:
        if msg['from'] == id:
            res['msgs'].append(msg)
    res['len'] = len(res['msgs'])
    return res

def get_time():
    res = {
        "y": time.localtime().tm_year-2000,
        "m": time.localtime().tm_mon,
        "d": time.localtime().tm_mday,
        "h": time.localtime().tm_hour,
        "min": time.localtime().tm_min,
        "s": time.localtime().tm_sec,
        "w": time.localtime().tm_wday,
    }
    return res


while True:
    data, addr = s.recvfrom(1024)
    print("Receive from %s:%s" % addr)
	# if data == b"exit":
	# 	s.sendto(b"Good bye!\n", addr)
	# 	continue
	# s.sendto(b"Hello %s!\n" % data, addr)
    try:
        print(data)
        data = json.loads(data.decode("utf-8"))
        print(data)
        res = {"code": 200}
        if data['func'] == "get_time":
            res = get_time()
        elif data['func'] == "send_msg":
            send_msg(data['msg'])
        elif data['func'] == "get_msg_inbox":
            res.update(get_msg_inbox(data['id']))
        elif data['func'] == "get_msg_outbox":
            res.update(get_msg_outbox(data['id']))
        elif data['func'] == "reg":
            res = {"code": 200}
            db["id"][data["id"]] = addr
            json.dump(db, open(os.path.join(pwd, "msg.json"), "w"))
            print(f'{data["id"]} registered as {"%s:%s" % addr}')
        else:
            res = {"code": 0}
            sendto(json.dumps(res), addr)
            continue
        
        if "clbk" in data:
            res["clbk"] = data["clbk"]
        sendto(json.dumps(res), addr)
    except Exception as e:
        print(e)
        res = {"code": 0}
        sendto(json.dumps(res), addr)
        
