#include <fcntl.h> /*for the redirection of inputs and output*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"   /*include declarations for parse-related structs*/

enum BUILTIN_COMMANDS { NO_SUCH_BUILTIN=0, EXIT,HIST,EXECHIST,KILL,HELP,CHDIR,JOBS};

/*global declarations*/
int status;
int infile;
int stdinbak;
int outfile;
int stdoutbak;
/*To avoid memory leaks when getting the pwd for the prompt, due to repetative calls*/ 
char *cwd;

/*for the jobs functionality*/
int jobsset;
char *jobs[100][2];

char * buildPrompt()
{	
	cwd = malloc(400);
	getcwd(cwd,400);
	strcat(cwd, "-% ");
	return cwd;
}

int isBuiltInCommand(char * cmd){

	if ( strncmp(cmd, "exit", strlen("exit")) == 0){
		return EXIT;
	}
	if ( strncmp(cmd, "history", strlen("history")) == 0){
		return HIST;
	}
	if ( strncmp(cmd, "!", strlen("!")) == 0){
		return EXECHIST;
	}
	if ( strncmp(cmd, "cd", strlen("cd")) == 0){
		return CHDIR;
	}
	if ( strncmp(cmd, "jobs", strlen("jobs")) == 0){
		return JOBS;
	}
	if ( strncmp(cmd, "kill", strlen("kill")) == 0){
		return KILL;
	}
	if ( strncmp(cmd, "help", strlen("help")) == 0){
		return HELP;
	}

	return NO_SUCH_BUILTIN;
}

int isBackgroundJob(parseInfo * info){
	return info->boolBackground;
}

int executeCommand(parseInfo * info){
	int PID;
	struct commandType *com;
	com = &info->CommArray[0];
	if(PID = execvp(com->command, com->VarList)){
		perror(com->command);
		exit(-1);
	}
	return 0;
}

void initJobs(){
	int i;
	int j;
	int status;
	int pid;
	pid_t returnPid;
	if(jobsset!=1){
		for(i=0;i<100;i++){
			jobs[i][0]=malloc(10); jobs[i][0]="0";
			jobs[i][1]=malloc(100); jobs[i][1]="--";
		}
		jobsset=1;
	}
	else{
		char *tempjobs[100][2];
		/*fill temp jobs with blanks*/
		for(i=0;i<100;i++){
			tempjobs[i][0]=malloc(10); tempjobs[i][0]="0";
			tempjobs[i][1]=malloc(100); tempjobs[i][1]="--";
		}
		for(i=0,j=0;i<100;i++){
			pid = atoi(jobs[i][0]);
			if(pid == 0) returnPid = -1;
			else if(pid != 0) returnPid = waitpid(pid, &status, WNOHANG); /* WNOHANG def'd in wait.h */
			if (returnPid == -1) {
				/*printf("Error condition on checking job, not adding.\n");*/
			} else if (returnPid == 0) {
				tempjobs[j][0] = jobs[i][0];
				tempjobs[j][1] = jobs[i][1];
				j++;
			} else if (returnPid == pid) {
				/*printf("Child %i is finished with %i status.\n", pid, status);*/
			}
		}
		for(i=0;i<100;i++){
			jobs[i][0]=tempjobs[i][0]; jobs[i][1]=tempjobs[i][1];
		}
	}
}

void addJob(int cpid, char* command){
	char *jobpid;
	char *jobname;
	int i;
	for(i=0;jobs[i][0]!="0";i++){}
	if(i<100){
		jobpid = malloc(10);
		jobname = malloc(sizeof(jobs[i][1]));
		jobname = command;
		snprintf(jobpid, 10, "%d", cpid);
		jobs[i][0]=jobpid;
		jobs[i][1]=jobname;
	}
	else{printf("Cannot store any more jobs!\n");}
}

void printJobs(){
	int i,j;
	for(i=0,j=0;i<100;i++){
		if(jobs[i][0]!="0"){
			printf("%s: ", jobs[i][0]);
			printf("%s\n", jobs[i][1]);
			j++;
		}
	}
	if(j==0) printf("No jobs running.\n");
}

void killJob(char* jobnum){
	int result, i, j;
	for(i=0,j=0;i<100;i++){
		if(strcmp(jobs[i][0],jobnum)==0) j++;
	}
	if(j>0){
		pid_t pid = atoi(jobnum);
		result = kill(pid, SIGKILL);
		if(result = -1){
			printf("Kill command sent to job %i.\n", pid);
		}
		else{ printf("Unknown error code %i returned.\b", result); }
	}
	else printf("That job was not found in the list, nothing done.\n");
}

void exitShell(){
	int i;
	for(i=0;jobs[i][0]!="0";i++){}
	if(i>0) printf("There are background jobs running, please kill them.\n");
	else exit(1);
}

void printHelp(){
	printf("help: prints this help screen.\n\n");
	printf("background: append & to the end of any command to run it in the background.\n\n");
	printf("jobs: type 'jobs' to print out the list of running background jobs.\n\n");
	printf("kill: kill 'background job #' to kill a running background job.\n\n");
	printf("history: type 'history' to display a record of the last 20 commands typed.\n\n");
	printf("!: type '!' followed by a space and a history number to run\n\n");
	printf("the command in history under that number, or '!' followed by a space and\n\n");
	printf("a negative number to run previous commands (e.g. '! -1' to run last entered command.\n\n");
	printf("cd: change directory, type 'cd' followed by your desired directory name.\n\n");
	printf("exit: to exit the shell.\n");
}

int main (int argc, char **argv)
{
	int childPid;
	char * cmdLine;
	parseInfo *info; /*info stores all the information returned by parser.*/
	struct commandType *com; /*com stores command name and Arg list for one command.*/
	int i;

#ifdef UNIX

	fprintf(stdout, "This is the UNIX version\n");
#endif

#ifdef WINDOWS
	fprintf(stdout, "This is the WINDOWS version\n");
#endif

	while(1){
		/*insert your code to print prompt here*/

#ifdef UNIX
		cmdLine = readline(buildPrompt());
		if (cmdLine == NULL) {
			fprintf(stderr, "Unable to read command\n");
			continue;
		}
#endif

		/*insert your code about history and !x !-x here*/
		initHistory();
		addHistory(cmdLine);
		/*initialize/modify jobs structures*/
		initJobs();
		/*calls the parser*/
		info = parse(cmdLine);
		if (info == NULL){
			free(cmdLine);
			continue;
		}
		/*prints the info struct*/
		/*print_info(info);*/

		/*check to see if input or output is redirected and react appropriately*/
		if(info->boolInfile == 1){
			infile = open(info->inFile, O_RDONLY, 0666);
			stdinbak = dup(fileno(stdin));
			dup2(infile, fileno(stdin));
			close(infile);
		}
		if(info->boolOutfile == 1){
			outfile = open(info->outFile, O_CREAT | O_RDWR, 0666);
			stdoutbak = dup(fileno(stdout));
			dup2(outfile, fileno(stdout));
			close(outfile);
		}
		/*com contains the info. of the command before the first "|"*/
		com=&info->CommArray[0];
		if ((com == NULL)  || (com->command == NULL)) {
			free_info(info);
			free(cmdLine);
			continue;
		}

		/*com->command tells the command name of com*/
		if (isBuiltInCommand(com->command) == EXIT) exitShell();
		else if (isBuiltInCommand(com->command) == HIST) showHistory();
		else if (isBuiltInCommand(com->command) == CHDIR) chdir(com->VarList[1]);
		else if (isBuiltInCommand(com->command) == JOBS) printJobs();
		else if (isBuiltInCommand(com->command) == KILL) killJob(com->VarList[1]);
		else if (isBuiltInCommand(com->command) == HELP) printHelp();
		else if (isBuiltInCommand(com->command) == EXECHIST) execHistory(com->VarList[1]);
		else {
			childPid = fork();
			if (childPid == 0){
				if(isBackgroundJob(info)){
					setpgid(0, 0); /*linux only. sets the process to a group
						    	that does not recieve input signals,
						    	effectively disabling stdin for the bg process.*/	
					outfile = open("/dev/null", O_RDWR);
					dup2(outfile, fileno(stdout));
					close(outfile);
				}
				executeCommand(info); /*calls execvp*/
			}
			/*two below if statements restore the stdout*/
			if(info->boolInfile == 1){
				dup2(stdinbak, fileno(stdin));
				close(stdinbak);
			}
			if(info->boolOutfile == 1){
				dup2(stdoutbak, fileno(stdout));
				close(stdoutbak);
			}
			else {
				if(isBackgroundJob(info)){
					/*record in a list of background jobs*/
					char * prevcommand;
					prevcommand = (char*)com->command;
					addJob(childPid, prevcommand);
				}
				else {
					/*Odd workaround that will wait for the output better than waitpid()*/
					if(childPid =  waitpid(-1, &status, 0) == -1)
						perror("wait error");
					continue;
				}
			}
		}

		/*insert your code here.*/
		free(cwd);
		free_info(info);
		free(cmdLine);
	}/* while(1) */
}

