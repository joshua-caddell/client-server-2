Joshua Caddell
CS 372 Project 2

Instructions to use ftclient.py and ftserver.c

1. Save ftclient.py and ftserver.c in separate directories

2. Compile ftserver.c:

	 gcc -o ftserver ftserver.c

3. Run ftserver:

	ftserver <port number>

4. In separate terminal window, run ftclient.py;
	
	python ftclient.py <host> <server port> <command -l|-g> [filename] <data port>

5. ftclient will close all sockets and terminate once the command has been executed.
   ftserver must be terminated with ctr+c.

Note: If the user requests a file that already exists in ftclient's directory, 
      ftclient will overwrite the contents of that file with the new data from
      the server.
