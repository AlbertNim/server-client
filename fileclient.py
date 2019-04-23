###############################################################################
# Author: Albert Nim
# Date: 03/07/18
# Assignment: ftclient, a server written in Python that allows for file transfer
###############################################################################
#resources - https://docs.python.org/2/howto/sockets.html
#            https://docs.python.org/3.5/library/struct.html
#            http://www.bogotobogoself.                                     com/python/python_network_programming_server_client_file_transfer.php
# http://stackoverflow.com/questions/17667903/python-socket-receive-large-amount-of-data
from socket import *
import sys
import os
from os import path
from struct import *

#checks for the inputs 
def validate():
#checks for number of inputs
    if len(sys.argv) < 5 or len(sys.argv) > 6:
        print ("Please enter at least 5 or at most 6 arguments.\n")
        exit(1)
#checks for correct server
    if sys.argv[1] != "flip1":
        print("Please enter flip1 as the server.\n")
        exit(1)
# checks for correct port numbers
    if int(sys.argv[2]) not in range(1024,65536):
        print("Please enter in a server number between 1024 and 65536\n")
        exit(1)
    if int(sys.argv[-1]) not in range(1024,65536):
        print("Please enter in a data port number between 1024 and 65535\n")
        exit(1)
#ports should not be the same
    if sys.argv[2] == sys.argv[-1]:
        print("Please enter in different ports and data numbers\n")
        exit(1)
    if sys.argv[3] not in ("-l","-g"):
        print("Please enter in a valid command such as -l or -g\n")
        exit(1)
#checks for duplicate files and whether or not it is a text file
    if len(sys.argv) == 6:
        if not sys.argv[4].endswith(".txt"):
            print("Must transfer text files")
            exit(1)
        if os.path.isfile(sys.argv[4]):
            print("Duplicate file already exists. Ending program.\n")
            exit(1)

#creates a socket to connect using the port number provided
def initiateContact(host, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
#returns the socket
    return sock

#finds the contents of the server directory 
def getDir(sock):
    data_size = sock.recv(4)
    data_size = unpack("I", data_size)
    received = str(sock.recv(data_size[0]), encoding="UTF-8").split("\x00")
    for val in received:
        print(val)

#sends message to our server function
def sendMessage(sock, message):
    to_send = bytes(message, encoding="UTF-8")
    sock.sendall(to_send)

#sends integer values
def sendNumber(sock, message):
    to_send = pack('i', message)
    sock.send(to_send)

# gets a message from the server and utilizes another function to 
def receiveMesage(sock):
    data_size = sock.recv(4)
    data_size = unpack("I", data_size)
    return recvall(sock, data_size[0])

# function to help receive a large amount of data
def recvall(sock, n):
#emptry string to store the values  
    received = ""
#loops while the packet is not empty
    while len(received) < n:
        packet = str(sock.recv(n - len(received)), encoding="UTF-8")
        if not packet:
            return None
#adds to the string
        received += packet
    return received


def makeRequest(conn, cmd, data):
    sendMessage(conn, cmd + "\0")
    sendNumber(conn, data)

#receives and writes the files to our computer
def receiveFile(conn, filename):
    buffer = receiveMesage(conn)
    if path.isfile(filename):
        filename = filename.split(".")[0] + "_copy.txt"

    with open(filename, 'w') as f:
        f.write(buffer)

#main portion of the code
if __name__ == '__main__':
#first validates the inputs
    validate()
#stores the inputs in variables
    server = sys.argv[1]
    port = int(sys.argv[2])
    command = sys.argv[3]
    data_port = 0
    filename = ""
#if there are 5 inputs we simply find list
    if len(sys.argv) is 5:
        data_port = int(sys.argv[4])
#if there are 6 inputs we are downloading a file
    elif len(sys.argv) is 6:
        filename = sys.argv[4]
        data_port = int(sys.argv[5])

#initiates the server and makes a request
    server = initiateContact(host, port)
    makeRequest(server, command, data_port)

# -l command 
    sleep(1)
    data = initiateContact(host, data_port)
# receives list from directory
    print("Receiving directory structure from {}: {}".format(host, data_port))
    getDir(data)
    data.close()

# -g command downloads the file
    sendNumber(server, len(filename))
    sendMessage(server, filename + "\0")

    result = receiveMesage(server)
# in case the file is unable to be found
    if result == "FILE NOT FOUND!":
        print("{}: {} says {}".format(host, port, result))
# in case the files are found
    elif result == "FOUND!":
        print("Receiving \"{}\" from {}: {}".format(filename, host, data_port))
        sleep(1)
        data = initiateContact(host, data_port)
        receiveFile(data, filename)
        print("File transfer complete!")
        data.close()
#clears the files
    server.close()
