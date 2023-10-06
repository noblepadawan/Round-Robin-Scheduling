#define _POSIX_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define MAX_PROCESSES 100
#define MAX_ARGUMENTS 10

typedef struct roundRobinNode 
{
    pid_t pid;
    struct roundRobinNode *next;
    struct roundRobinNode *prev;
} roundRobinNode;

// Globals
roundRobinNode *head = NULL;
roundRobinNode *current = NULL;
int activeProcesses = 0;
long int quantum = 0;
int quantumExpired = 0;


roundRobinNode* initializeChildPidList();
void addChild(roundRobinNode*, pid_t);
void removeChild(roundRobinNode*, pid_t);
void printList(roundRobinNode*);
int isListEmpty(roundRobinNode *);

void parseCommandline(char* commandLine, char* procesess[MAX_PROCESSES][MAX_ARGUMENTS]);
void setupTimer();
void setupSignalHandler();
void timerHandler(int sig);
