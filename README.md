Distributed File Server:-

Create a different folder for DFC and DFS server and client to segregate the two environments - say /server and /client

Use gcc dfs.c -o dfs to compile the code for server.
Use gcc dfc.c -o dfc to compile the code for server.
Run server by following command from /server-
# ./dfs DFS1 10001 &
# ./dfs DFS2 10002 &
# ./dfs DFS3 10003 &
# ./dfs DFS4 10004 &

run client by following command /client
#./dfc

The server will read the dfs.conf file and parse out username and password for authentication
The client will read the dfc.conf file and parse out DFS server root directory, IP address of server, port number on which server is running,
username and password for authentication, to be sent to the server

Testing:
When you run the client it gives you a list of following commands -
get <filename>
put <filename>
list
exit
mkdir <subfoldername>

What is my DFS doing -
- it is optimising traffic while getting files from server and also while listing files from server.
- it is encrypting the file to be sent to server, and once you try to get a file from server, it is decrypting it. Key is user's password
- this code is authenticating, everytime client tries to connect to the server and does not execute any client requests if authentication is unsuccessful
- mkdir command creates a directory inside the server folders DFS1, DFS2, DFS3, DFS4
