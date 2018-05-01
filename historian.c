
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sqlite3.h>


char *getIP();
int main(int argc, char *argv[]) {
	if(argc < 2) {
		printf("usage: %s port\n", argv[0]);
		return 0;
	}


/*
------------------------------------------------------------------------------------
------------------------------SQL DATABASE SETUP------------------------------------
------------------------------------------------------------------------------------
*/
	sqlite3 *db;
    	char *err_msg = 0;
    
    	int rc = sqlite3_open("test.db", &db);
    
    	if (rc != SQLITE_OK) {
        
        	fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
       		sqlite3_close(db);
        
        	return 1;
    	}
    
    	char *sql = "DROP TABLE IF EXISTS Log;" 
                "CREATE TABLE Log(Test TEXT);";

    		rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    		if (rc != SQLITE_OK ) {
        
        	fprintf(stderr, "SQL error: %s\n", err_msg);
        
        	sqlite3_free(err_msg);        
        	sqlite3_close(db);
        
        	return 1;
    	} 
 /*
--------------------------------------------------------------------------------
----------------------------------SERVER SETUP----------------------------------
--------------------------------------------------------------------------------
*/   
    
	int n;
	struct in_addr ip;
	char buffer[40]; //message with size of 40
	socklen_t fromlen;
	int sock = socket(AF_INET, SOCK_DGRAM, 0); //create the socket
	if(sock < 0) {
		printf("Error opening socket\n");
		exit(-1);
	}
	struct sockaddr_in server;
	struct sockaddr_in addr;
	bzero(&server, sizeof(server)); //sets all values of server to 0
	server.sin_family = AF_INET; //Internet domain
	server.sin_addr.s_addr = INADDR_ANY;	//IP address on running machine
	server.sin_port = htons(atoi(argv[1]));	// get the port number from the user args
	

	if(bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) { //binds socket to IP address of host and the user entered port number
		printf("error binding socket\n");
		exit(-1);
	}
	
	int broadEnable = 1;
	int ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadEnable, sizeof(broadEnable)); //set socket to allow broadcast
	if(ret < 0 ) {
		printf("Error setting socket to broadcast\n");
		exit(-1);
	}
/*
--------------------------------------------------------------------------------
---------------------------------MAIN LOOP--------------------------------------
--------------------------------------------------------------------------------
*/
	fromlen = sizeof(struct sockaddr_in); //size of socket structure
	while(1) {
		bzero(buffer, 40); //refresh buffer
		n = recvfrom(sock, buffer, 40, 0, (struct sockaddr *)&addr, &fromlen); //receive messages from clients
		if (n < 0) {
			printf("Receiving error\n");
			exit(-1);
		}
		char *sql = "INSERT INTO Log VALUES('%q');", buffer;

   		rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    		if (rc != SQLITE_OK ) {
        
        		fprintf(stderr, "SQL error: %s\n", err_msg);
        
        		sqlite3_free(err_msg);        
        		sqlite3_close(db);
        
        		return 1;
   		 } 
	}
	
    	sqlite3_close(db);
	return 0;
}
char *getIP() {	
	int n;
    	struct ifreq ifr;
	char array[] = "wlan0";
	n = socket(AF_INET, SOCK_DGRAM, 0);
    	//Type of address to retrieve - IPv4 IP address
    	ifr.ifr_addr.sa_family = AF_INET;
    	//Copy the interface name in the ifreq structure
    	strncpy(ifr.ifr_name , array , IFNAMSIZ - 1);
    	ioctl(n, SIOCGIFADDR, &ifr);
    	close(n);
    	//display result
    return inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr);

}	
