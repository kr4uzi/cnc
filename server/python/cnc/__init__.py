import host

print("initializing cnc");

def onClientConnect(mac, data):
    print(mac)
    print(data["mac"])

host.registerHandler("ClientConnect", onClientConnect)
