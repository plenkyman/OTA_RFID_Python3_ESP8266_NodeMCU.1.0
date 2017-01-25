#!/usr/bin/python3
import sys
import time
import socketserver
''' a tcp server that listens for input from one ip,
    then sends back an OK message to the same ip.
    logs messages to file
'''

# ADJUST THESE PATRAMETERS TO YOUR OWN SETUP
senderip = "192.168.1.50"
listenerip = "192.168.1.15"
listenerport = 9999
pathtolog = "ESP8266Door.log"

class MyTCPHandler(socketserver.BaseRequestHandler):

    def handle(self):
        # self.request is the TCP socket connected to the client
        self.data = self.request.recv(1024).strip()
        # check if message comes from ESP8266 ip 
        if self.client_address[0] == senderip:
            payload = self.data.decode()
            if payload != "Restarting NodeMCU_1.0":
                print(payload)
                self.request.sendall("OK".encode())
                with open(pathtolog,"a") as logit:
                    logit.write(time.strftime("%T %a %e %b %Y") +": " + payload + " \n")
            else:
                print(payload)
                with open(pathtolog,"a") as logit:
                    logit.write(time.strftime("%T %a %e %b %Y") +": " + payload + " \n")


                
        else:
            print("unknown host sending tcp message")
            with open(pathtolog,"a") as logit:
                logit.write(time.strftime("%T %a %e %b %Y") + ": unknown client is sending messages to server \n")
                    
            
if __name__ == "__main__":
    HOST, PORT = listenerip, listenerport
    
    try:
        # Create the server, binding to listenerip on port listenerport
        server = socketserver.TCPServer((HOST, PORT), MyTCPHandler)
        # Activate the server; this will keep running until you
        # interrupt the program with Ctrl-C
        server.serve_forever()
    except KeyboardInterrupt:
        print(" - - - >     - - > >    - > > >")
        print(__file__ +" quit by << Control C >>")
        print("                                        Au revoir, do good today!")
        sys.exit(0)
