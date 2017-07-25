"""
Joshua Caddell
CS 372 Project 2
Description: ftclient sends a request to ftserver for either
	a listing of the ftserver's directory or a file in
	ftserver.

Source of socket code is:
 https://docs.python.org/2/howto/sockets.html
 http://ilab.cs.byu.edu/python/socket/exceptions.html
"Example Application: TCP Client" from the lecture 15 slides 
"Example Application: TCP Server" from the Lecture 15 slides
 
"""

import sys
import socket

"""
Main function.
"""
def main(arg):
	v = validateArgs(arg)
	
	#if v is -1, there was an syntax error on command line
	#if 5 then command was for directory listing
	#if 6 then command was for a file
	if v == -1:
		print"\nUsage Error. Try ftclient.py <host> <port> <command> <file> <port>"
		sys.exit(1)
	elif v == 5:
		host = arg[1]
		port = int(arg[2])
		cmd = arg[3]	
		dataport = int(arg[4])
		file = ""
		message = cmd + ' ' + str(dataport)
	else:
		host = arg[1]
		port = int(arg[2])
		cmd = arg[3]	
		file = arg[4]
		dataport = int(arg[5])
		message = cmd + ' ' + file + ' ' + str(dataport)

	if dataport < 1024 or dataport > 65535 or dataport == 30021 or dataport == 30021:
		print "\nInvalid data port number"
		sys.exit(1)	
	
	controlSocket = makeControlSocket(host, port)
	
	#send command to the server
	# and receive acknowledgment	
	controlSocket.send(message)
	message = controlSocket.recv(16)
	
	#acknowledgment from server will be either
	#	"valid" or "invalid"
	if(message == 'invalid'):
		print "\nInvalid command sent to server"
		sys.exit(1)
	
	#run the data connection session
	dataSocket(dataport, cmd, file)
	
	controlSocket.close()
"""
Function to establish a data connection and recieve data
	over that connection
Parameters: port number, string with command, and file name)
Returns: nothing
"""
def dataSocket(dataport, cmd, file):
	try: #create a socket and start listening
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.bind(('', dataport))
		s.listen(1)
	except socket.error: #catch an errors with making socket
		print"\nError opening socket"
		sys.exit(1)
	
	datasock, server = s.accept()
	
	if cmd == '-g': 
		getfile(datasock,file, server)
	else:
		print "\nReceiving directory listing from " + str(server)
		getlisting(datasock)
		
	datasock.close()
	s.close()

"""
Function to get the directory listing form the server
	and print it
Parameters: socket file descriptor
Returns nothing
"""
def getlisting(s):
	text = recvall(s)
	print "\n" + text 

"""
Function to get the contents of the requested file from
	the server and store locally in new file of 
	same name
Parameters: socket file descriptor, file name, server name
Returns nothing but does create a new file in the local directory
"""
def getfile(s, filename, server):
	print "\nReceiving file from " + str(server)
	text = recvall(s)
	
	if text == "file not found":
		print "\n" + filename + " not found on server."
		return
	else:	
		#passing 'w' to open ensures that any existing 
		#file of the same name will be overwritten, thus
		#avoiding errors if the file already exists.
		file = open(filename, 'w')
		file.write(text)
		print "\nFile transfer complete."
		file.close()

"""
Function to receive data from a socket
Parameter: socket file descriptor
Returns: data from socket
"""
def recvall(s):
	n = 1
	text = ''
	
	while n > 0:
		t = s.recv(1024)
		n = len(t)
		text = text + t	
	return text
		
"""
Function to validate command line argguments
parameter: list of command line arguments
Returns: -1 on syntax error, 5 if -l, 6 if -g
"""
def validateArgs(arg):
	if (len(arg) < 4): #too few arguments
		return -1	
	elif arg[3] == '-l':
		if len(arg) != 5: #too few/many arguments
			return -1
		elif not (arg[2].isdigit() and arg[4].isdigit()):
			return -1  #no port numbers
		return 5
	elif arg[3] == '-g':
		if len(arg) != 6: #too few/many arguments
			return -1
		elif not (arg[2].isdigit() and arg[5].isdigit()):
			return -1
		return 6		
	else:
		return -1

"""
Function to establish initial connection to ftserver
Parementers: server host name and port number from commnand line
Returns: control Socket file descriptor
"""
def makeControlSocket(host, port):
	try: #create socket and connect to server
		controlSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		controlSocket.connect((host, port))
	except socket.error: #catch any errors
		if controlSocket:
			controlSocket.close()
		print 'Error opening socket'
		sys.exit(1)
	
	return controlSocket
	
	
if __name__ == "__main__":	
	main(sys.argv)
