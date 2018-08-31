import host

print("initializing cnc");

def onClientConnect(mac, data):
    print(mac)
    print(data)

host.registerHandler("ClientConnect", onClientConnect)
