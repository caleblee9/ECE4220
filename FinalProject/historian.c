
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
#include <pthread.h>

char *getIP();
void *getInfo(void *);
void *menu(void *);
int callback(void *, int, char **, char **);


sqlite3 *db;
char *err_msg = 0;
int rc;  
const char *sql; 
char buffer[40]; //message with size of 40
typedef struct serverInfo {
	int socket;
	struct sockaddr_in address;
	socklen_t len;
}ServerInfo;

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
    	rc = sqlite3_open("test.db", &db);
    
    	if (rc != SQLITE_OK) {
        
        	fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        	return 1;
    	}
    
    	sql = "DROP TABLE IF EXISTS Log;" \
                "CREATE TABLE Log(" \
		"Event TEXT);";

    	rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    	if (rc != SQLITE_OK ) {
        	fprintf(stderr, "SQL error: %s\n", err_msg);
        	sqlite3_free(err_msg);         
    	} 
 /*
--------------------------------------------------------------------------------
----------------------------------SERVER SETUP----------------------------------
--------------------------------------------------------------------------------
*/   
    
	int n;
	struct in_addr ip;
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
	fromlen = sizeof(struct sockaddr_in); //size of socket structure

/*
--------------------------------------------------------------------------------
---------------------------------MAIN LOOP--------------------------------------
--------------------------------------------------------------------------------
*/
	
	pthread_t t1, t2;
	ServerInfo s1;
	s1.socket = sock;
	s1.address = addr;
	s1.len = fromlen;
	pthread_create(&t1, NULL, getInfo, &s1);
	pthread_create(&t2, NULL, menu, NULL);	
	pthread_join(t1, NULL);	
	pthread_join(t2, NULL);

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
int callback(void *NotUsed, int argc, char **argv, 
                    char **azColName) {
    
    NotUsed = 0;
    
    for (int i = 0; i < argc; i++) {

        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    
    printf("\n");
    
    return 0;
}
void *getInfo(void *ptr) {
	ServerInfo *s1 = (ServerInfo *) ptr;
	int n;
	
	while(1) {
		bzero(buffer, 40); //refresh buffer
		n = recvfrom(s1->socket, buffer, 40, 0, (struct sockaddr *)&s1->address, &s1->len); //receive messages from clients
		if (n < 0) {
			printf("Receiving error\n");
			exit(-1);
		}
		printf("%s\n", buffer);

		sql = sqlite3_mprintf("INSERT INTO Log VALUES ('%q');", buffer);	
		rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
   
   		if( rc != SQLITE_OK ){
      			fprintf(stderr, "SQL error: %s\n", err_msg);
      			sqlite3_free(err_msg);
   		}	
	}
	pthread_exit(0);
}
void *menu(void *ptr) {
	int choice = 0;
	while(1) {
		printf("What would you like to do?\n");
		printf("1. Display Log\n");
		printf("2. Enable/Disable LEDs");
		printf("0. Exit\n");
		scanf("%d", &choice);
		if(choice == 0) {
			break;
		}else if (choice == 1) {
			sql = "SELECT * FROM Log";
        
    			rc = sqlite3_exec(db, sql, callback, 0, &err_msg);
    
    			if (rc != SQLITE_OK ) {
        
        			fprintf(stderr, "Failed to select data\n");
        			fprintf(stderr, "SQL error: %s\n", err_msg);

        			sqlite3_free(err_msg);
    			}
		}

	}
	system("clear");
	exit(0);
	pthread_exit(0);
}
