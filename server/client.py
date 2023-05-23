import socket

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect(("127.0.0.1", 6000))

s.send(b'{"func":"send_msg","msg":{"from":"666","to":"888","time":"23/04/10 03:29","content":"Hello world!"}}\r\n')