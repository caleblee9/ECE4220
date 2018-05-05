
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
char buffer[50]; //receive buffer
int sock; 
struct sockaddr_in server;
struct sockaddr_in addr;
socklen_t fromlen;
typedef struct data {
	char event[16];
	char id[16];
	double time;
	int b1;
	int b2;
	int s1;
	int s2;
	int r;
	int y;
	int g;
	double volt;
} Data;

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
    
    	sql = "DROP TABLE IF EXISTS Log;" 
                "CREATE TABLE Log(Event TEXT, ID TEXT, TIME REAL, Button1 INT, Button2 INT, Switch1 INT, Switch2 INT, RED INT, YELLOW INT, GREEN INT, VOLTAGE REAL);";

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
    


	sock = socket(AF_INET, SOCK_DGRAM, 0); //create the socket
	if(sock < 0) {
		printf("Error opening socket\n");
		exit(-1);
	}
	bzero(&server, sizeof(server)); //sets all values of server to 0
	server.sin_family = AF_INET; //Internet domain
	server.sin_addr.s_addr = INADDR_ANY;	//IP address on running machine
	server.sin_port = htons(atoi(argv[1]));	// get the port number from the user args
	

	if(bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) { //binds socket to IP address of host and the user entered port number
		printf("error binding socket\n");
		exit(-1);
	}
	
	int broadEnable = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadEnable, sizeof(broadEnable)) < 0) {
		printf("error setting socket options\n");
		exit(-1);
	} //set socket to allow broadcast
	fromlen = sizeof(struct sockaddr_in); //size of socket structure

/*
--------------------------------------------------------------------------------
---------------------------------MAIN LOOP--------------------------------------
--------------------------------------------------------------------------------
*/
	
	pthread_t t1, t2;
	pthread_create(&t1, NULL, getInfo, NULL);
	pthread_create(&t2, NULL, menu, NULL);	
	pthread_join(t1, NULL);	
	pthread_join(t2, NULL);

	sqlite3_close(db);
	
	return 0;
}
/*
-----------------------------------------------------------------------------
--------------------------------GET IP---------------------------------------
-----------------------------------------------------------------------------
*/
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
/*
-----------------------------------------------------------------------------
--------------------------------CALLBACK-------------------------------------
-----------------------------------------------------------------------------
*/

int callback(void *NotUsed, int argc, char **argv, 
                    char **azColName) {
    
    NotUsed = 0;
    
    for (int i = 0; i < argc; i++) {

        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    
    printf("\n");
    
    return 0;
}
/*
-----------------------------------------------------------------------------
--------------------------------GETINFO--------------------------------------
-----------------------------------------------------------------------------
*/

void *getInfo(void *ptr) {
	int n;
	Data d;	
	while(1) {
		bzero(buffer, 50); //refresh buffer
		n = recvfrom(sock, buffer, 50, 0, (struct sockaddr *)&addr, &fromlen); //receive messages from clients
		if( strcmp(buffer, "RED") == 0 || strcmp(buffer, "YELLOW") == 0 || strcmp(buffer, "GREEN") == 0) 
		{
			continue;
		}
		if (n < 0) {
			printf("Receiving error\n");
			exit(-1);
		}
		sscanf(buffer, "%s %s %lf %d %d %d %d %d %d %d %lf", d.event, d.id, &d.time, &d.b1, &d.b2, &d.s1, &d.s2, &d.r, &d.y, &d.g, &d.volt);
		fflush(stdout);
		sql = sqlite3_mprintf("INSERT INTO Log VALUES ('%q', '%q', %lf, %d, %d, %d, %d, %d, %d, %d, %lf);", d.event, d.id, d.time, d.b1, d.b2, d.s1, d.s2, d.r, d.y, d.g, d.volt);	
		rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
   
   		if( rc != SQLITE_OK ){
      			fprintf(stderr, "SQL error: %s\n", err_msg);
      			sqlite3_free(err_msg);
   		}
		bzero(d.event, 16);
		bzero(d.id, 16);
	}
	pthread_exit(0);
}
/*
-----------------------------------------------------------------------------
--------------------------------USER MENU------------------------------------
-----------------------------------------------------------------------------
*/

void *menu(void *ptr) {
	int choice = 0;
	int LED = 0;
	int n;
	while(1) {
		printf("What would you like to do?\n");
		printf("1. Display Log\n");
		printf("2. Enable/Disable LEDs\n");
		printf("0. Exit\n");
		scanf("%d", &choice);
		switch(choice) {
			case 0:
				exit(0);
				break;
			case 1:
				sql = "SELECT * FROM Log";
        
    				rc = sqlite3_exec(db, sql, callback, 0, &err_msg);
    
    				if (rc != SQLITE_OK ) {
        
        				fprintf(stderr, "Failed to select data\n");
        				fprintf(stderr, "SQL error: %s\n", err_msg);

        				sqlite3_free(err_msg);
    				}
				break;
			case 2:	
				addr.sin_addr.s_addr = inet_addr("128.206.19.255"); 	//set IP to broadcast (.255)
				printf("1. RED\n");
				printf("2. Yellow\n");
				printf("3. GREEN\n");
				printf("4. Back\n");
				scanf("%d", &LED);
				if(LED == 1) {
					n = sendto(sock, "R", 3, 0, (struct sockaddr *)&addr, fromlen);
			 		if (n < 0){
						perror("Error: ");
						printf("Send error\n");
						exit(0);
					}			
				} else if(LED == 2) {
					n = sendto(sock, "Y", 6, 0, (struct sockaddr *)&addr, fromlen);
					if (n < 0){
						printf("Send error\n");
						exit(0);
					}			
				} else if(LED == 3) {
					n = sendto(sock, "G", 5, 0, (struct sockaddr *)&addr, fromlen);
					if (n < 0){
						printf("Send error\n");
						exit(0);
					}			
				} else if(LED == 4) {
					continue;
				}
				break;
			
		}
		

	}
	system("clear");
	exit(0);
	pthread_exit(0);
}
