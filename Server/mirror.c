#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>

// Define the constants
#define PORT 8888
#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64
#define BUFFER_SIZE 1024

// Global variables
int totalClients = 0;
char *home;				   // Home directory
char *commands[MAX_ARGS];  // Array to store the commands
int numOfArgs = 0;

// Function to split the string into tokens
void splitString(char *str)
{
	numOfArgs = 0;
	char *token = strtok(str, " ");
	while (token != NULL)
	{
		commands[numOfArgs] = token;
		token = strtok(NULL, " ");
		numOfArgs++;
	}
}

// Function to run the command
int runCommand(char *command)
{
	printf("%s", command);
	// Execute find command
	int status = system(command);
	if (status != 0)
	{
		printf("\nFailed to run command...\n");
		return 1;
	}
	else
	{
		printf("\nCommand successful...\n");
		return 0;
	}
}

// Function to check if the command generated a file
int checkIfFileGenerated(char *filename)
{
	// check if file exists
	if (access(filename, F_OK) != -1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

// Function to send the response to the client
void sendResponseToClient(int newSocket, int sendFile, char *filename)
{
	if (sendFile)
	{
		// send the sendFile flag to client like SENDFILE=0
		char flag[10] = "SENDFILE=1";
		while (1)
		{
			printf("\nSending flag: %s\n", flag);
			send(newSocket, flag, strlen(flag), 0);
			// wait for acknowledgement
			char ack[12];
			printf("Waiting for acknowledgement...\n");
			recv(newSocket, ack, 12, 0);
			printf("Acknowledgement received: %s\n", ack);
			if (strncmp(ack, "flagReceived", 12) == 0)
			{
				break;
			}
		}

		// Open temp.tar.gz archive for reading
		int fd = open(filename, O_RDONLY);
		if (fd == -1)
		{
			printf("Error opening archive...\n");
			exit(0);
		}

		// send file size
		struct stat st;
		if (stat(filename, &st) >= 0)
		{
			off_t buffer_size = st.st_size;
			send(newSocket, &buffer_size, sizeof(buffer_size), 0);
		}
		else
		{
			perror("stat");
			exit(EXIT_FAILURE);
		}

		// Read archive into buffer and send to client over socket
		char buffer[BUFFER_SIZE];

		ssize_t bytes_read;
		while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0)
		{
			ssize_t bytes_sent = send(newSocket, buffer, bytes_read, 0);
			if (bytes_sent < bytes_read)
			{
				printf("Error sending data...\n");
				exit(0);
			}
		}

		// remove tar after sending it to client
		remove(filename);

		// Close file and socket
		close(fd);
		printf("File sent successfully...\n");
	}
	else
	{
		// send the sendFile flag to client like SENDFILE=0
		char flag[10] = "SENDFILE=0";
		while (1)
		{
			printf("\nSending flag: %s\n", flag);
			send(newSocket, flag, strlen(flag), 0);
			// wait for acknowledgement
			char ack[12];
			printf("Waiting for acknowledgement...\n");
			recv(newSocket, ack, 12, 0);
			printf("Acknowledgement received: %s\n", ack);
			if (strncmp(ack, "flagReceived", 12) == 0)
			{
				break;
			}
		}

		// send the error message to client
		char *error = "Error: File not found";
		send(newSocket, error, strlen(error), 0);
		printf("Message sent successfully...\n");
	}
}

// Function to get status of file
int findFile(int newSocket){
	struct stat fileStat;
	char command[MAX_COMMAND_LENGTH];
    char output[MAX_COMMAND_LENGTH];
	snprintf(command, sizeof(command), "find %s -name \"%s\" -type f -print -exec ls -lh --time-style=+\"%%Y-%%m-%%d %%H:%%M:%%S\" {} \\; 2>/dev/null | head -n1",home, commands[1]);
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen failed");
    }
    if (fgets(output, sizeof(output), fp) != NULL) {
		output[strcspn(output, "\n")] = 0;
		stat(output, &fileStat);
		printf( "\nFile: %s\nSize: %ld bytes\nDate created: %s", output, fileStat.st_size, ctime(&fileStat.st_birthtime));
		char *message=malloc(MAX_COMMAND_LENGTH * sizeof(char));
		sprintf(message, "File: %s\nSize: %ld bytes\nDate created: %s", output, fileStat.st_size, ctime(&fileStat.st_birthtime));
		printf("%s", message);
		send(newSocket, message, strlen(message), 0);
	}
	else
	{
		char *message = "File not found";
		send(newSocket, message, strlen(message), 0);
	}
	pclose(fp);
}

// Function to cater to sgetfiles command
void getFilesBySize(int newSocket)
{
	char *operationType = "sgetfiles";
	char command[MAX_COMMAND_LENGTH];
	char filename[500];
	int sendFile = 0; // Flag to indicate that a file will be sent in response to the command

	// find Command
	char *findCommand = malloc(MAX_COMMAND_LENGTH * sizeof(char));
	sprintf(findCommand, "find %s -type f -name '*.*' -not -path '%s/Library/*' -size +%dc -size -%dc -print0 ", home, home, atoi(commands[1]), atoi(commands[2]));
	// complete command with tar
	sprintf(command, "%s | if grep -q . ; then %s | tar -czf %s_%d.tar.gz --null -T -  ; fi", findCommand, findCommand, operationType, newSocket);

	// run the command
	if (runCommand(command) == 1)
	{
		exit(1);
	}
	else if (runCommand(command) == 0)
	{
		// filename
		sprintf(filename, "%s_%d.tar.gz", operationType, newSocket);

		// check if file was generated
		if (checkIfFileGenerated(filename) == 1)
		{
			sendFile = 1;
		}
		else
		{
			sendFile = 0;
		}

		// send response to client
		sendResponseToClient(newSocket, sendFile, filename);
	}
}

// Function to cater to dgetfiles command
void getFilesByDate(int newSocket)
{
	char *operationType = "dgetfiles";
	char command[MAX_COMMAND_LENGTH];
	char filename[500];
	int sendFile = 0; // Flag to indicate that a file will be sent in response to the command

	// find Command
	char *findCommand = malloc(MAX_COMMAND_LENGTH * sizeof(char));
	sprintf(findCommand, "find %s -type f -name '*.*' -not -path '%s/Library/*' -newermt '%s 00:00:00' ! -newermt '%s 23:59:59' -print0", home, home, commands[1], commands[2]);
	sprintf(command, "%s | if grep -q . ; then %s | tar -czf %s_%d.tar.gz --null -T - ; fi", findCommand, findCommand, operationType, newSocket);

	// run the command
	if (runCommand(command) == 1)
	{
		exit(1);
	}
	else if (runCommand(command) == 0)
	{
		// filename
		sprintf(filename, "%s_%d.tar.gz", operationType, newSocket);

		// check if file was generated
		if (checkIfFileGenerated(filename) == 1)
		{
			sendFile = 1;
		}
		else
		{
			sendFile = 0;
		}

		// send response to client
		sendResponseToClient(newSocket, sendFile, filename);
	}
}

// Function to cater to getfiles command
void getFiles(int newSocket)
{
	char *operationType = "getfiles";
	char command[MAX_COMMAND_LENGTH];
	char output[MAX_COMMAND_LENGTH];
	char filename[500];
	char files[500];
	int sendFile = 0;

	if (strcmp(operationType, "getfiles") == 0)
	{
		sprintf(command, "find %s -type f '('", home);
		if (strcmp(commands[numOfArgs - 1], "-u") == 0)
		{
			for (int i = 1; i < numOfArgs - 1; i++)
			{
				if (i != numOfArgs - 2)
				{
					sprintf(files, " -name '%s' -o", commands[i]);
				}
				else
				{
					sprintf(files, " -name '%s'", commands[i]);
				}
				strcat(command, files);
			}
		}
		else
		{
			for (int i = 1; i < numOfArgs; i++)
			{
				if (i != numOfArgs - 1)
				{
					sprintf(files, " -name '%s' -o", commands[i]);
				}
				else
				{
					sprintf(files, " -name '%s'", commands[i]);
				}
				strcat(command, files);
			}
		}
		sprintf(files, " ')' -print0 ");
		strcat(command, files);
		sprintf(files, " | if grep -q . ; then %s |", command);
		strcat(command, files);
		sprintf(files, " tar -czf %s_%d.tar.gz --null -T - ; fi", operationType, newSocket);
		strcat(command, files);
	}
	// run the command
	if (runCommand(command) == 1)
	{
		exit(1);
	}
	else if (runCommand(command) == 0)
	{
		// filename
		sprintf(filename, "%s_%d.tar.gz", operationType, newSocket);

		// check if file was generated
		if (checkIfFileGenerated(filename) == 1)
		{
			sendFile = 1;
		}
		else
		{
			sendFile = 0;
		}

		// send response to client
		sendResponseToClient(newSocket, sendFile, filename);
	}
}

// Function to cater to gettargz command
void getFilesByExtensions(int newSocket)
{
	char *operationType = "gettargz";
	char command[MAX_COMMAND_LENGTH];
	char output[MAX_COMMAND_LENGTH];
	char filename[500];
	char files[500];
	int sendFile = 0;

	if (strcmp(operationType, "gettargz") == 0)
	{
		sprintf(command, "find %s -type f '('", home);
		if (strcmp(commands[numOfArgs - 1], "-u") == 0)
		{
			for (int i = 1; i < numOfArgs - 1; i++)
			{
				if (i != numOfArgs - 2)
				{
					sprintf(files, " -name '*.%s' -o", commands[i]);
				}
				else
				{
					sprintf(files, " -name '*.%s'", commands[i]);
				}
				strcat(command, files);
			}
		}
		else
		{
			for (int i = 1; i < numOfArgs; i++)
			{
				if (i != numOfArgs - 1)
				{
					sprintf(files, " -name '*.%s' -o", commands[i]);
				}
				else
				{
					sprintf(files, " -name '*.%s'", commands[i]);
				}
				strcat(command, files);
			}
		}
		sprintf(files, " ')' -print0 ");
		strcat(command, files);
		sprintf(files, " | if grep -q . ; then %s |", command);
		strcat(command, files);
		sprintf(files, "tar -czf %s_%d.tar.gz --null -T - ; fi", operationType, newSocket);
		strcat(command, files);
	}
	// run the command
	if (runCommand(command) == 1)
	{
		exit(1);
	}
	else if (runCommand(command) == 0)
	{
		// filename
		sprintf(filename, "%s_%d.tar.gz", operationType, newSocket);

		// check if file was generated
		if (checkIfFileGenerated(filename) == 1)
		{
			sendFile = 1;
		}
		else
		{
			sendFile = 0;
		}

		// send response to client
		sendResponseToClient(newSocket, sendFile, filename);
	}
}

// a function for handling connections for each client
void handleClient(int serverSocket, int newSocket, struct sockaddr_in newAddr)
{
	char buffer[1024];
	// create a child process to handle the client
	pid_t childPid = fork();
	if (childPid == 0)
	{
		close(serverSocket);
		// handle client communication
		while (1)
		{
			// clear the buffer
			memset(buffer, '\0', sizeof(buffer));
			// receive the client request
			recv(newSocket, buffer, BUFFER_SIZE, 0);
			if (strlen(buffer) == 0)
			{
				continue;
			}
			printf("\nClient request: %s\n", buffer);

			// check if the client wants to quit
			if (strcmp(buffer, "quit") == 0)
			{
				printf("Disconnected from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
				close(serverSocket);
				exit(0);
				break;
			}
			else
			{
				// split the client request into command and arguments
				splitString(buffer);
				char *cmd = malloc(strlen(commands[0]) + 1); // +1 for the null terminator
				strcpy(cmd, commands[0]);
				cmd[strlen(cmd)] = '\0';
				char *message = malloc(MAX_COMMAND_LENGTH * sizeof(char));

				// handle the client request based on the command
				if (strcmp(cmd, "findfile") == 0)
				{
					// if (findFile(newSocket, commands[1], home) != 0)
					// {
					// 	message = "No files found";
					// 	send(newSocket, message, strlen(message), 0);
					// }
					findFile(newSocket);
				}
				else if (strcmp(cmd, "sgetfiles") == 0)
				{
					getFilesBySize(newSocket);
				}
				else if (strcmp(cmd, "dgetfiles") == 0)
				{
					getFilesByDate(newSocket);
				}
				else if (strcmp(cmd, "getfiles") == 0)
				{
					getFiles(newSocket);
				}
				else if (strcmp(cmd, "gettargz") == 0)
				{
					getFilesByExtensions(newSocket);
				}
				// clear the message buffer
				memset(message, 0, sizeof(char) * sizeof(message));
			}
		}
	}
}

// A function for creating and then starting the server finally returning the server socket
int createAndStartServer()
{
	int serverSocket;
	int opt = 1;
	struct sockaddr_in serverAddr;

	// Create and verify socket
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket < 0)
	{
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Mirror Socket is created.\n");

	// prepare the sockaddr_in structure
	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	//serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	// set socket options
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
	{
		printf("setsockopt failed");
		exit(1);
	}
	// bind the socket to defined options
	if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		printf("[-]Error in binding.\n");
		exit(1);
	}
	printf("[+]Bind to port %d\n", PORT);

	// start Listening
	if (listen(serverSocket, 10) == 0)
	{
		printf("[+]Listening....\n");
	}
	else
	{
		printf("[-]Error in listening....\n");
		exit(1);
	}
	return serverSocket;
}

// Main function
int main()
{
	// Variable declaration
	home = getenv("HOME");
	int serverSocket;
	int newSocket;
	struct sockaddr_in newAddr;
	socklen_t addr_size;

	serverSocket = createAndStartServer();

	// accept the connection in a loop
	while (1)
	{
		// accept the connection
		newSocket = accept(serverSocket, (struct sockaddr *)&newAddr, &addr_size);
		if (newSocket < 0)
		{
			exit(1);
		}
		printf("%d. Connection accepted from %s:%d\n", totalClients + 1, inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
		totalClients++;
		// handle the client
		handleClient(serverSocket, newSocket, newAddr);
	}
	close(newSocket);
	return 0;
}