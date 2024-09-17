#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ctype.h>

// Defining the constants
#define PRIMARY_SERVER_PORT 4444
#define MIRROR_SERVER_PORT 8888
#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64
#define BUFFER_SIZE 1024
char *PRIMARY_SERVER_IP = "127.0.0.1";
char *MIRROR_SERVER_IP = "127.0.0.1";
char *commands[MAX_ARGS];
int numOfArgs = 0;

// A function to strip leading and trailing spaces from a string
void stripSpaces(char *str)
{
	int i = 0, j = 0;
	while (str[i] == ' ')
	{
		i++;
	}
	while (str[i] != '\0')
	{
		str[j] = str[i];
		i++;
		j++;
	}
	str[j] = '\0';
	j--;
	while (str[j] == ' ')
	{
		j--;
	}
	str[j + 1] = '\0';
}

// A function to split a string into tokens
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

// A function to check if a given string is a number
int isNumber(char *str)
{
	for (int i = 0; str[i] != '\0'; i++)
	{
		if (!isdigit(str[i]))
		{
			return 0; // not a digit
		}
	}
	return 1; // is digit
}

// A function to check if a given string is a valid date
int isValidDate(char *str)
{
	int day, month, year;
	if (sscanf(str, "%d-%d-%d", &year, &month, &day) != 3)
	{
		return 0;
	}
	if (day < 1 || month < 1 || month > 12 || year < 0)
	{
		return 0;
	}
	int maxDays = 31;
	if (month == 4 || month == 6 || month == 9 || month == 11)
	{
		maxDays = 30;
	}
	else if (month == 2)
	{
		if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
		{
			maxDays = 29;
		}
		else
		{
			maxDays = 28;
		}
	}
	if (day > maxDays)
	{
		return 0;
	}
	return 1;
}

// A function to compare two dates, returns 0 if date1 is greater, 1 if date2 is greater
int dateComparison(char *date1, char *date2)
{
	int day1, month1, year1, day2, month2, year2;
	sscanf(date1, "%d-%d-%d", &year1, &month1, &day1);
	sscanf(date2, "%d-%d-%d", &year2, &month2, &day2);

	if (year1 > year2)
	{
		return 0; // date1 is greater
	}
	else if (year1 == year2 && month1 > month2)
	{
		return 0; // date1 is greater
	}
	else if (year1 == year2 && month1 == month2 && day1 > day2)
	{
		return 0; // date1 is greater
	}
	else
	{
		return 1; // date2 is greater
	}
}

// A function to validate the commands
int validateCommands()
{
	if (strcmp(commands[0], "findfile") == 0 ||
		strcmp(commands[0], "sgetfiles") == 0 ||
		strcmp(commands[0], "dgetfiles") == 0 ||
		strcmp(commands[0], "getfiles") == 0 ||
		strcmp(commands[0], "gettargz") == 0 ||
		strcmp(commands[0], "quit") == 0)
	{
		if (strcmp(commands[0], "findfile") == 0)
		{
			// ensure that the number of arguments is 2
			if (numOfArgs != 2)
			{
				// Incorrect number of Args ;print the usage
				printf("\n<usage>: findfile <filename>\n\n");
				return 0;
			}
			// validated
			return 1;
		}
		else if (strcmp(commands[0], "sgetfiles") == 0)
		{
			// ensure that the number of arguments is 3 or 4
			if (numOfArgs < 3 || numOfArgs > 4)
			{
				// Incorrect number of Args ;print the usage
				printf("\n<usage>: sgetfiles <size1> <size2> <-u>\n\n");
				return 0;
			}
			else if (isNumber(commands[1]) == 0 || isNumber(commands[2]) == 0)
			{
				// size1 or size2 is not a number
				printf("\n<usage>: sgetfiles <size1> <size2> <-u>\n");
				printf("Size should be integer\n\n");
				return 0;
			}
			else if (atoi(commands[1]) >= atoi(commands[2]))
			{
				// size1 is greater than size2
				printf("\n<usage>: sgetfiles <size1> <size2> <-u>\n");
				printf("Size1 should be less than size2\n\n");
				return 0;
			}
			else if (numOfArgs == 4 && strcmp(commands[numOfArgs - 1], "-u") != 0)
			{
				// last argument is not -u
				printf("\n<usage>: sgetfiles <size1> <size2> <-u>\n\n");
				return 0;
			}
			return 1;
		}
		else if (strcmp(commands[0], "dgetfiles") == 0)
		{
			// ensure that the number of arguments is 3 or 4
			if (numOfArgs < 3 || numOfArgs > 4)
			{
				// Incorrect number of Args ;print the usage
				printf("\n<usage>: dgetfiles <date1> <date2> <-u>\n\n");
				return 0;
			}
			else if (isValidDate(commands[1]) == 0 || isValidDate(commands[2]) == 0)
			{
				// date1 or date2 is not a valid date
				printf("\n<usage>: dgetfiles <date1> <date2> <-u>\n");
				printf("Enter valid date\n\n");
				return 0;
			}
			else if (dateComparison(commands[1], commands[2]) == 0)
			{
				// date1 is greater than date2
				printf("\n<usage>: dgetfiles <date1> <date2> <-u>\n");
				printf("Enter valid date with date1 should be older than date2\n\n");
				return 0;
			}
			else if (numOfArgs == 4 && strcmp(commands[numOfArgs - 1], "-u") != 0)
			{
				// last argument is not -u
				printf("\n<usage>: sgetfiles <size1> <size2> <-u>\n\n");
				return 0;
			}
			return 1;
		}
		else if (strcmp(commands[0], "getfiles") == 0)
		{
			// ensure that the number of arguments is 2 to 8
			if (numOfArgs < 2 || numOfArgs > 8)
			{
				// Incorrect number of Args ;print the usage
				printf("\n<usage>: getfiles <file1> <file2> <file3> <file4> <file5> <file6> <-u>\n\n");
				return 0;
			}
			else if (numOfArgs == 8 && strcmp(commands[numOfArgs - 1], "-u") != 0)
			{
				// last argument is not -u
				printf("\n<usage>: getfiles <file1> <file2> <file3> <file4> <file5> <file6> <-u>\n\n");
				return 0;
			}
			return 1;
		}
		else if (strcmp(commands[0], "gettargz") == 0)
		{
			// ensure that the number of arguments is 2 to 8
			if (numOfArgs < 2 || numOfArgs > 8)
			{
				// Incorrect number of Args ;print the usage
				printf("\n<usage>: gettargz <extension1> <extension2> <extension3> <extension4> <extension5> <extension6> <-u>\n\n");
				return 0;
			}
			else if (numOfArgs == 8 && strcmp(commands[numOfArgs - 1], "-u") != 0)
			{
				// last argument is not -u
				printf("\n<usage>: gettargz <extension1> <extension2> <extension3> <extension4> <extension5> <extension6> <-u>\n\n");
				return 0;
			}
			// validated
			return 1;
		}
		// validated
		return 1;
	}
	else
	{
		// Invalid command
		printf("Invalid command\n");
		return 0;
	}
}

// A function to send commands to server and receive the response; show message or store file as per the response
void handleServerResponse(int clientSocket, char *fileName)
{
	char read_buffer[BUFFER_SIZE];
	off_t total_bytes_read = 0;
	off_t buffer_size;
	char read_buffer_for_Flag[BUFFER_SIZE];

	// Read dataFlag from socket to determine whether server is sending message or file
	printf("Waiting for dataFlag...\n");
	recv(clientSocket, &read_buffer_for_Flag, sizeof(read_buffer_for_Flag), 0);
	char receivedFlag[10];
	strncpy(receivedFlag, read_buffer_for_Flag, 10);

	// flags to determine whether server is sending message or file
	char fileTypeFlag[10] = "SENDFILE=1";
	char dataFlag[10] = "SENDFILE=0";

	// if dataFlag SENDFILE=0
	if (strncmp(receivedFlag, dataFlag, 10) == 0)
	{
		printf("Received the Data Flag!\n");
		// wait for 1 second
		sleep(1);
		// send acknowledgement to server
		send(clientSocket, "flagReceived", 12, 0);

		// read data from socket and print
		recv(clientSocket, &read_buffer, sizeof(read_buffer), 0);
		printf(">>%s\n", read_buffer);
		return;
	}
	else if (strncmp(receivedFlag, fileTypeFlag, 10) == 0)
	{
		printf("Received the File Flag!\n");
		// wait for 1 second
		sleep(1);
		// send acknowledgement to server
		send(clientSocket, "flagReceived", 12, 0);

		int fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		if (fd == -1)
		{
			printf("Error opening file...\n");
			close(fd);
		}
		else
		{
			// Read file size from socket
			recv(clientSocket, &buffer_size, sizeof(buffer_size), 0);

			// Read data from socket and write to file
			while (total_bytes_read < buffer_size)
			{
				// Read data from socket
				ssize_t bytes_read = recv(clientSocket, read_buffer, BUFFER_SIZE, 0);
				if (bytes_read == -1)
				{
					printf("Error in receiving data");
					break;
				}
				// Write data to file
				ssize_t bytes_written = write(fd, read_buffer, bytes_read);
				if (bytes_written == -1)
				{
					printf("Error in writing data");
					break;
				}
				if (bytes_written < bytes_read)
				{
					printf("Error writing data to file...\n");
					break;
				}
				total_bytes_read = total_bytes_read + bytes_read;
			}
			close(fd);

			// if -u flag is present, extract the file
			if (strcmp(commands[numOfArgs - 1], "-u") == 0)
			{
				char str[200];
				sprintf(str, "tar -xzf %s", fileName);
				system(str);
			}
		}
	}
}

// A function to connect to servers that takes an argument "M" or "P" and return the socket
int connectToServer(char *serverType)
{
	int clientSocket, ret;
	struct sockaddr_in mirrorServerAddr;
	struct sockaddr_in primaryServerAddr;

	// if serverType is M (mirror server)
	if (strcmp(serverType, "M") == 0) {
		// create new socket
		clientSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (clientSocket < 0)
		{
			printf("[-]Error in connection.\n");
			exit(1);
		}

		// create mirrorServerAddress
		memset(&mirrorServerAddr, '\0', sizeof(mirrorServerAddr));
		mirrorServerAddr.sin_family = AF_INET;
		mirrorServerAddr.sin_port = htons(MIRROR_SERVER_PORT);
		mirrorServerAddr.sin_addr.s_addr = inet_addr(MIRROR_SERVER_IP);
		// connect to mirror server
		ret = connect(clientSocket, (struct sockaddr *)&mirrorServerAddr, sizeof(mirrorServerAddr));
		if (ret < 0)
		{
			printf("[-]Error in connection with MIRROR SERVER\n");
			exit(1);
		}
		printf("[+]Connected to MIRROR SERVER.\n");
	}
	// if serverType is P (primary server)
	else if (strcmp(serverType, "P") == 0) {
		// create new socket
		clientSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (clientSocket < 0)
		{
			printf("[-]Error in connection.\n");
			exit(1);
		}
		printf("[+]Client Socket is created.\n");

		// create primaryServerAddress
		memset(&primaryServerAddr, '\0', sizeof(primaryServerAddr));
		primaryServerAddr.sin_family = AF_INET;
		primaryServerAddr.sin_port = htons(PRIMARY_SERVER_PORT);
		primaryServerAddr.sin_addr.s_addr = inet_addr(PRIMARY_SERVER_IP);
		// connect to primary server
		ret = connect(clientSocket, (struct sockaddr *)&primaryServerAddr, sizeof(primaryServerAddr));
		if (ret < 0)
		{
			printf("[-]Error in connection with PRIMARY SERVER\n");
			exit(1);
		}
	}
	else {
		printf("Invalid server type\n");
		exit(1);
	}
	return clientSocket;
}

// Main function
int main(int argc, char* argv[])
{
	if (argc != 3 && argc != 1) {
		printf("Usage: %s <Server IP> <Mirror IP>\n", argv[0]);
		exit(1);
	}else if(argc==3){
		PRIMARY_SERVER_IP=argv[1];
		MIRROR_SERVER_IP=argv[2];
	}
	int clientSocket;
	char buffer[MAX_COMMAND_LENGTH];
	char str[MAX_COMMAND_LENGTH];
	char read_buffer[BUFFER_SIZE];

	// Connect to primary server
	clientSocket = connectToServer("P");

	// Check if Primary Server has responded to connect with MIRROR SERVER or PRIMARY SERVER
	// send a char to server to check if it is primary or mirror
	write(clientSocket, "c", 1);
	// read a char from server to check if it is primary or mirror
	int n = read(clientSocket, read_buffer, 1);
	read_buffer[n] = '\0';
	
	// check if the server responded with P or M in first char of read_buffer
	// if P, then continue s already connected to Pimary server; if M, then connect to mirror server
	if (strcmp(read_buffer, "P") == 0)
	{
		printf("[+]Connected to PRIMARY SERVER.\n");
	}
	else if (strcmp(read_buffer, "M") == 0)
	{
		// close previous socket
		close(clientSocket);
		// connect to mirror server
		clientSocket = connectToServer("M");
	}
	else
	{
		// if the server responded with anything else, then exit
		printf("[-]PRIMARY SERVER malfunction: Responded with unknown command\n");
		exit(1);
	}

	// Keep looping to prompt user for commands and communicate with server
	while (1)
	{
		memset(read_buffer, 0, BUFFER_SIZE);

		// prompt user for command
		printf("C$ ");
		// get command from user
		scanf(" %[^\n]s", &buffer[0]);
		// testing : set buffer to quit to exit
		// strcpy(buffer, "getfiles check.txt");

		strcpy(str, buffer);
		// strip the str of any trailing or leading spaces
		stripSpaces(str);
		splitString(str);

		// Validate commands
		int isValid = validateCommands();

		// If command is valid, send it to server
		if (isValid == 1)
		{
			send(clientSocket, buffer, strlen(buffer), 0);
		}
		else
		{
			continue;
		}

		// If command is quit, then exit
		if (strcmp(buffer, "quit") == 0)
		{
			close(clientSocket);
			printf("[-]Disconnected from server.\n");
			exit(1);
		}

		// call handleServerResponse function to handle server response
		if (strcmp(commands[0], "sgetfiles") == 0)
		{
			handleServerResponse(clientSocket, "temp.tar.gz");
		}
		else if (strcmp(commands[0], "dgetfiles") == 0)
		{
			handleServerResponse(clientSocket, "temp.tar.gz");
		}
		else if (strcmp(commands[0], "getfiles") == 0)
		{
			handleServerResponse(clientSocket, "temp.tar.gz");
		}
		else if (strcmp(commands[0], "gettargz") == 0)
		{
			handleServerResponse(clientSocket, "temp.tar.gz");
		}
		else
		{
			if (recv(clientSocket, read_buffer, BUFFER_SIZE, 0) < 0)
			{
				printf("[-]Error in receiving data.\n");
			}
			else
			{
				printf("%s\n", read_buffer);
			}
		}
		// printf("\n++++++++++++++++++++++\n");
	}
	return 0;
}