#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <readline/readline.h>
#include "parse.h"

enum BUILTIN_COMMANDS { NO_SUCH_BUILTIN=0, EXIT,HIST,EXECHIST,KILL,HELP,CHDIR,JOBS};

char *histArray[20];
FILE *histfile;
int status;

void showHistory(){
	int i;
	for(i=0;i<20;i++){
		printf("%i: ", 20-i);
		printf("%s\n", histArray[i]);
	}
}

void execHistory(char* index){
	int childPid;
	parseInfo *info;
	struct commandType *com;

	if(atoi(index) >= 0){
		info = parse(histArray[21-atoi(index)]);
	}
	else if(atoi(index) < 0){	
		info = parse(histArray[abs(atoi(index))]);
	}

	com=&info->CommArray[0];
	if (isBuiltInCommand(com->command) == EXIT) exitShell();
	else if (isBuiltInCommand(com->command) == HIST) showHistory();
	else if (isBuiltInCommand(com->command) == CHDIR) chdir(com->VarList[1]);
	else if (isBuiltInCommand(com->command) == JOBS) printJobs();
	else if (isBuiltInCommand(com->command) == KILL) killJob(com->VarList[1]);
	else if (isBuiltInCommand(com->command) == HELP) printHelp();
	else if (isBuiltInCommand(com->command) == EXECHIST){ 
		printf("This is a history command, for the sake of your sanity don't repeat it.\n");
	}
	else {
		childPid = fork();
		if (childPid == 0){
			if (isBackgroundJob(info)) printf("To avoid accidental backgrounds, job will be placed in the foreground.\n");
			fprintf(stderr,"execute_exit_code:%i\n", executeCommand(info));
			/*calls execvp*/
		}else {
			/*Odd workaround that will wait for the output better than waitpid()*/
			if(childPid = waitpid(-1, &status, 0) == -1) perror("wait error");
		}
		/*executeCommand(parse(histArray[21-atoi(index)]));*/
	}
}

void addHistory(char* command){
	char* temparray[20];
	int i;
	for(i=0;i<20;i++){
		temparray[i]="";
	}

	temparray[0]=malloc(strlen(command)+1);
	strcpy(temparray[0], command);	

	for(i=0;i<19;i++){
		temparray[i+1]=malloc(strlen(histArray[i])+1);
		strcpy(temparray[i+1],histArray[i]);
	}

	histfile = fopen("/tmp/shell_history.txt", "w");
	/*write the new history to file*/
	for(i=0;i<20;i++){
		fprintf(histfile, "%s\n", temparray[i]);
	}
	fclose(histfile);
	/*printf("History Written\n");*/
}

void initHistory(){
	int n;
	char line[200];
	histfile = fopen ("/tmp/shell_history.txt", "a+");
	for(n=0; n<20; n++){
		histArray[n]="";
	}
	n=0;
	while(fgets(line, sizeof(line), histfile) != NULL)
	{
		if(n>=20) break;
		/*to strip the newline off of the read input*/
		line[strlen(line)-1]='\0';
		histArray[n]=malloc(strlen(line) + 1);
		strcpy(histArray[n], line);
		n++;
	}

	fclose(histfile);
	/*printf("History File Initialized.\n");*/
}

