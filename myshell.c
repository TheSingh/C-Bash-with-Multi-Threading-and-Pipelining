#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

//this is provided through a lex file (lex.l)
extern char **getline();

//checks to see if ">" special char is detected
int checkOutputRedirect(char **args) {
	int c;
	for(c=0; args[c] != NULL; ++c) {
		if(!strncmp(args[c], ">", 1)) return c;
	}
	return 0;
}

int checkInputRedirect(char **args) {
	int c;
	for(c=0; args[c] != NULL; ++c) {
		if(!strncmp(args[c], "<", 1)) return c;
	}
	return 0;
}

int checkForPipes(char **args) {
	int c;
	for(c=0; args[c] != NULL; ++c) {
		if(!strncmp(args[c], "|", 1)) return c;
	}
	return 0;
}

int main() {
	int i, j, k, status, fd, cin, cout;
	int hose[2];
	char *buffer[1025];
	pid_t pid, childpid;
	FILE* file;
	char **args; 
	
	//each loop, a command is read and executed
	while(1) {
		//read in commands from the input line
		args = getline();

		if (args[0] == NULL) continue;
		if((pipe(hose) == -1)) perror("Piping");

		//detect the exit command
		if(!strncmp(args[0],"exit",4)) exit(0);
		//create a child process to execute the command entered
		pid = fork();
		
		if(pid == -1) {				//forking failed
			perror("Fork");
		} else if(pid ==  0) {		//child 
			if((j = checkOutputRedirect(args))) {
				//duplicate STDOUT and close it.
				fd = dup(1);
				close(1);
				
				//open a new file as STDOUT
				file = fopen(args[j+1], "w");
				args[j] = NULL;
				
				//create a new child process that will execute the command
				if(!fork()) {
					execvp(args[0], args);		//it thinks it's writing to stdout
				} else {			
					wait(&status);
					//close the file, and move STDOUT back into it's place
					fclose(file);
					dup(fd);
					exit(0);
				}
				exit(1);
			} else if((j = checkInputRedirect(args))) {
				//duplicate STDIN and close it.
				fd = dup(0);
				close(0);
				
				//open a new file as STDIN
				file = fopen(args[j+1], "w");
				args[j] = NULL;
				
				//create a new child process that will execute the command
				if(!fork()) {
					execvp(args[0], args);		//it thinks it's reading from stdin
				} else {			
					wait(&status);
					//close the file, and move STDIN  back into it's place
					fclose(file);
					dup(fd);
					exit(0);
				}
				exit(1);
			} else if ((j = checkForPipes(args))) {
				//dupclicate
				if(args[j+1] == NULL) perror("Lacking pipe");
				
				childpid = fork();		//fork into the two commands
				switch(childpid) {
					case -1 :			//it's broke
						perror("Forking");
						break;
					case 0	:			//this will be the reader
						args[j] = NULL;	//tell it when to stop reading
						close(hose[0]);	//close unused pipe
						
						//duplicate stdout and close it to replace with hose
						cout = dup(1);
						close(1);
						dup(hose[1]);
						
						//fork and execute the command 
						if(!fork()) {
							execvp(args[0], args);
						} else {
							//restore stdout and close pipes
							wait(&status);
							close(1);
							close(hose[1]);
							dup(cout);
							exit(0);
						}
						exit(1);
						break;
					default	:
						++j;	//copy the second command as the first
						for(k=0; args[j] != NULL; ++k) {
							args[k] = args[j];
							++j;
						}
						args[k] = NULL;	//define the end point
					
						//duplicate stdin and close it to replace with a hose
						close(hose[1]);
						cin = dup(0);
						close(0);
						dup(hose[0]);
						
						//fork and execute the command
						if(!fork()) {
							execvp(args[0], args);
						} else {
							//restore stdin and close pipes
							wait(&status);
							close(0);
							close(hose[0]);
							dup(cin);
							exit(0);
						}
						exit(1);
						break;
				}
				exit(1);
			} else if(execvp(args[0], args) < 0) {
				perror("Execvp");
			}
			
		} else {					//parent
			//close the hose and exit
			close(hose[0]);
			close(hose[1]);
			wait(&status);
		}
	}
}
