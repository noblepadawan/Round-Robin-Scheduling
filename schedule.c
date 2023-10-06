#include "schedule.h"



int main(int argc, char* argv[])
{
    // Exit if usage is wrong
    if (argc < 3) {
        fprintf(stderr, "Usage: schedule quantum [prog 1 [args] [: prog 2 [args] [: prog3 [args] [: ... ]]]]\n");
        exit(EXIT_FAILURE);
    }

    // Get quantum value from command line
    quantum = strtol(argv[1], NULL, 10);

    if (quantum == 0){
        fprintf(stderr, "Usage: schedule quantum [prog 1 [args] [: prog 2 [args] [: prog3 [args] [: ... ]]]]\n");
        exit(EXIT_FAILURE);
    }

    // Array for processes and their arguments
    char* processes[MAX_PROCESSES][MAX_ARGUMENTS];
    
    char* commandLine = NULL;

    // Construct the commandLine string by concatenating arguments
    for (int i = 2; i < argc; i++) {
        if (commandLine == NULL) {
            // Allocate memory for the first argument
            commandLine = (char*)malloc(strlen(argv[i]) + 1);
            strcpy(commandLine, argv[i]);
        } else {
            // Reallocate memory to include space for the next argument
            commandLine = (char*)realloc(commandLine, strlen(commandLine) + strlen(argv[i]) + 2);
            strcat(commandLine, " ");
            strcat(commandLine, argv[i]);
        }
    }
    // Parse the command line into the separate processes and arguments
    parseCommandline(commandLine, processes);
    // setup the signal handler
    setupSignalHandler();
    // Initialize the pid list to keep track of child processes
    head = initializeChildPidList();
    // Fork child processes
    for (int i = 0; i <= activeProcesses; i++)
    {
        int s; 
        pid_t child_pid = fork();
        if (child_pid == -1)
        {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (child_pid == 0)
        {
            // Child process
            // Immediately stop the child process
            raise(SIGSTOP);
            // Execute the process command
            if (execvp(processes[i][0], processes[i]) == -1)
            {
                // Execute the process command if the process is a file in the same directory
                char newCmd[256];  
                snprintf(newCmd, sizeof(newCmd), "./%s", processes[i][0]);
                if (execvp(newCmd, processes[i]) == -1) {
                    perror("Exec failed");
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            // Parent
            waitpid(child_pid, &s, WUNTRACED);
            addChild(head, child_pid);
        }
    }

    sleep(1);

    //Start the first process
    current = head->next;
    kill(current->pid, SIGCONT);

    while(1)
    {
        // Skip if the current process lands on head
        if (current->pid == 0) {
            current = current->next;
        } else {
            setupTimer();
            // Check if the process is terminated
            int status;
            pid_t terminated_pid = waitpid(current->pid, &status, WUNTRACED);
            if (terminated_pid > 0) 
            {
                // If terminated, move current to next node to avoid SEGFAULT
                current = current->next;
                // Remove child in child pid list
                removeChild(head, terminated_pid);
                // Check if list is empty
                if (isListEmpty(head))
                {
                    break;
                } 
                // If not empty, start the next process
                else
                {
                    if (current->pid == 0) {
                        current = current->next;
                        kill(current->pid, SIGCONT);
                    } else {
                        kill(current->pid, SIGCONT);
                    }
                }
            } 
            else if (quantumExpired) 
            {
                // If quantum has expired, stop the current process
                kill(current->pid, SIGSTOP);
                // // Move to the next process
                current = current->next;
                // Start the next process
                kill(current->pid, SIGCONT);
                // Reset the quantumExpired flag
                quantumExpired = 0;
            }
        }
    }

    // Clean up memory
    free(head);
    free(commandLine);
    return 0;
}

void parseCommandline(char* commandLine, char* processes[MAX_PROCESSES][MAX_ARGUMENTS])
{
    // int processIndex = 0;
    int argIndex = 0;

    // Break the commandLine into a series of tokens using " " as the delimiter
    char* token = strtok(commandLine, " ");

    while (token != NULL)
    {
        // Check if maximum number of arguments is reached
        if (activeProcesses >= MAX_PROCESSES)
        {
            fprintf(stderr, "Error: Maximum number of processes exceeded\n");
            exit(EXIT_FAILURE);
        }
        // Check if the token is a colon
        if (strcmp(token, ":") == 0) {
            // If so, move to the next process and reset the argument index
            activeProcesses++;
            argIndex = 0;
        } else{
            // Duplicate token to process array
            processes[activeProcesses][argIndex] = token;
            argIndex++;

            // Check if maximum number of arguments is reached
            if (argIndex >= MAX_ARGUMENTS) 
            {
                fprintf(stderr, "Error: Maximum number of arguments exceeded\n");
                exit(EXIT_FAILURE);
            }
        }
        // Move to next token
        token = strtok(NULL, " ");
    }
}

void setupTimer()
{
    long int sec = quantum / 1000;
    long int usec = (quantum % 1000) * 1000;
    
    static struct itimerval timer;

    timer.it_value.tv_sec = sec;
    timer.it_value.tv_usec = usec;
    timer.it_interval.tv_sec = sec;
    timer.it_interval.tv_usec = usec;
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
    {
        perror("Timer Inititialization failed");
        exit(EXIT_FAILURE);
    }
}

void setupSignalHandler() {
    struct sigaction sa;
    sa.sa_handler = timerHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, NULL) == -1)
    {
        perror("Signal handler registration failed");
        exit(EXIT_FAILURE);
    }
}

void timerHandler(int sig)
{
    quantumExpired = 1;
}


roundRobinNode* initializeChildPidList()
{
    roundRobinNode *head = (roundRobinNode*)malloc(sizeof(roundRobinNode));
    if (head == NULL){
        perror("Failed to allocate memory for head node");
        exit(EXIT_FAILURE);
    }
    head->pid = 0;
    head->next = head;
    head->prev = head;
    return head;
}

void addChild(roundRobinNode *head, pid_t newPid)
{
    roundRobinNode *newNode = (roundRobinNode*)malloc(sizeof(roundRobinNode));
    if (newNode == NULL){
        perror("Failed to allocate memory for new node");
        exit(EXIT_FAILURE);
    }
    newNode->pid = newPid;
    newNode->next = head;
    newNode->prev = head->prev;

    head->prev->next = newNode;
    head->prev = newNode;
}

void removeChild(roundRobinNode *head, pid_t targetPid)
{
    roundRobinNode *currentNode = head->next;
    while (currentNode != head){
        if (currentNode->pid == targetPid)
        {
            currentNode->prev->next = currentNode->next;
            currentNode->next->prev = currentNode->prev;
            free(currentNode);
            return;
        }
        currentNode = currentNode->next;
    }
}

void printList(roundRobinNode *head)
{
    roundRobinNode *currentNode = head->next;
    while (currentNode != head) {
        fprintf(stdout,"%d -> ", currentNode->pid);
        currentNode = currentNode->next;
    }
    fprintf(stdout, "NULL\n");
}

int isListEmpty(roundRobinNode *head)
{
    return head->next == head && head->prev == head;
}
