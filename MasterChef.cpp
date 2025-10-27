#include <getopt.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include "StepList.h"

using namespace std;

// Global variables
StepList* recipeSteps;          // Pointer to the list of all recipe steps
vector<int>* completedSteps;    // Stores IDs of steps that have completed
int completeCount = 0;          // Total number of completed steps

// Prints usage information and exits the program
void PrintHelp() {
    cout << "Usage: ./MasterChef -i <file>\n\n";
    cout << "--------------------------------------------------------------------------\n";
    cout << "<file>:    csv file with Step, Dependencies, Time (m), Description\n";
    cout << "--------------------------------------------------------------------------\n";
    exit(1);
}

// Processes command line arguments and returns the input filename
string ProcessArgs(int argc, char** argv) {
    string result = "";

    // If fewer than expected arguments, show help
    if (argc < 3) {
        PrintHelp();
    }

    while (true) {
        const auto opt = getopt(argc, argv, "i:h");

        if (-1 == opt) break;  // No more options

        switch (opt) {
            case 'i':  // Input file
                result = string(optarg);
                break;
            case 'h':  // Help option
            default:
                PrintHelp();
                break;
        }
    }

    return result;
}

// Creates and starts a timer for a given step
// expire is the number of seconds until the step completes
void makeTimer(Step* timerID, int expire) {
    struct sigevent te;
    struct itimerspec its;

    // Set up timer to send SIGRTMIN to this process when expired
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = SIGRTMIN;
    te.sigev_value.sival_ptr = timerID;  // Pass the step pointer to the handler
    timer_create(CLOCK_REALTIME, &te, &(timerID->t_id));

    // Configure the timer: single-shot (no interval), expires after "expire" seconds
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = expire;
    its.it_value.tv_nsec = 0;
    timer_settime(timerID->t_id, 0, &its, NULL);
}

// Signal handler for when a step's timer expires
static void timerHandler(int sig, siginfo_t* si, void* uc) {
    Step* comp_item = (Step*)si->si_value.sival_ptr; // Retrieve pointer to the step

    if (comp_item != nullptr) {
        // Mark the step as completed
        completedSteps->push_back(comp_item->id);  // Add step ID to completed list
        completeCount++;                            // Increment total completed count

        comp_item->PrintComplete();                 // Print "Completed Step: ..."
        
        raise(SIGUSR1);                             // Notify dependency handler to update steps
    }
}

// Signal handler for removing completed steps from other steps' dependency lists
void RemoveDepHandler(int sig) {
    if (completedSteps == nullptr || recipeSteps == nullptr) return;

    // Remove each completed step from dependencies of all steps
    for (int id : *completedSteps) {
        recipeSteps->RemoveDependency(id);
    }

    // Clear the completed steps list for next batch
    completedSteps->clear();
}

int main(int argc, char **argv) {
    string input_file = ProcessArgs(argc, argv);
    if (input_file.empty()) {
        exit(1);
    }

    // Initialize global structures
    completedSteps = new vector<int>();
    recipeSteps = new StepList(input_file);

    // Set up signal handler for SIGRTMIN (timers)
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timerHandler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // Set up signal handler for SIGUSR1 (removing dependencies)
    if (signal(SIGUSR1, RemoveDepHandler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    int totalSteps = recipeSteps->Count();

    // Main loop: repeatedly check for steps ready to run
    while (completeCount < totalSteps) {
        // Retrieve all steps that are ready (no dependencies and not running)
        vector<Step*> ready = recipeSteps->GetReadySteps();

        // Sort steps by ID to ensure deterministic output order
        sort(ready.begin(), ready.end(), [](Step* a, Step* b){ return a->id < b->id; });

        // Start timers for ready steps
        for (Step* s : ready) {
            if (!s->running) {
                s->running = true;
                makeTimer(s, s->duration);
            }
        }

        // Sleep briefly to allow signals to be delivered
        usleep(100 * 1000); // 100ms
    }

    cout << "Enjoy!" << endl;

    return 0;
}