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

StepList* recipeSteps;
vector<int>* completedSteps;
int completeCount = 0;

void PrintHelp() // Given
{

	cout << "Usage: ./MasterChef -i <file>\n\n";
	cout<<"--------------------------------------------------------------------------\n";
	cout<<"<file>:    "<<"csv file with Step, Dependencies, Time (m), Description\n";
	cout<<"--------------------------------------------------------------------------\n";
	exit(1);
}

string ProcessArgs(int argc, char** argv) // Given
{
	string result = "";
	// print help if odd number of args are provided
	if (argc < 3) {
		PrintHelp();
	}

	while (true)
	{
		const auto opt = getopt(argc, argv, "i:h");

		if (-1 == opt)
			break;

		switch (opt)
		{
		case 'i':
			result = std::string(optarg);
			break;
		case 'h': // -h or --help
		default:
			PrintHelp();
			break;
		}
	}

	return result;
}

// Creates and starts a timer given a pointer to the step to start and when it will expire in seconds.
void makeTimer( Step *timerID, int expire) // Given
{
    struct sigevent te;
    struct itimerspec its;

    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = SIGRTMIN;
    te.sigev_value.sival_ptr = timerID;
    timer_create(CLOCK_REALTIME, &te, &(timerID->t_id));

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = expire;
    its.it_value.tv_nsec = 0;
    timer_settime(timerID->t_id, 0, &its, NULL);
}

// Triggers when the time for the step has elapsed.
// the siginfo_t* pointer holds the step information in the si_value.sival_ptr
// You will need to print out that the step has completed
// Mark the step to be removed as a dependency, and trigger the remove dep handler.
// TO COMPLETE Section 2
static void timerHandler( int sig, siginfo_t *si, void *uc )
{
  // Retrieve timer pointer from the si->si_value
  Step* comp_item = (Step*)si->si_value.sival_ptr;
  
  /* TODO This Section - 2 */
  // Officially complete the step using completedSteps and completeCount
  
  if (comp_item != nullptr) {
    // Add to completedSteps first
    completedSteps->push_back(comp_item->id);
    completeCount++;
    
    // Remove dependencies
    raise(SIGUSR1);
    
    // Now print completion
    comp_item->PrintComplete();
    }
  
  /* End Section - 2 */
}

// Removes the copmleted steps from the dependency list of the step list.
// Utilize the completedSteps vector and the RemoveDependency method.
// To Complete - Section 3
void RemoveDepHandler(int sig) {
  /* TODO This Section - 3 */
  // Foreach step that has been completed since last run, remove it as a dependency
  
  if (completedSteps == nullptr || recipeSteps == nullptr) return;
  
  for (int id : *completedSteps) {
    recipeSteps->RemoveDependency(id);
  }
  
  // Clear completedSteps for next round
  completedSteps->clear();
  
  /* End Section - 3 */
}

// Associate the signals to the signal handlers as appropriate
// Continuously check what steps are ready to be run, and start timers for them with makeTimer()
// run until all steps are done.
// To Complete - Section 1

int main(int argc, char **argv)
{
  string input_file = ProcessArgs(argc, argv);
  if(input_file.empty()) {
    exit(1);
  }
  
  // Initialize global variables
  completedSteps = new vector<int>();
  recipeSteps = new StepList(input_file);
  
  /* Set up signal handler. */
  struct sigaction sa;
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = timerHandler;
  sigemptyset(&sa.sa_mask);
  
  /* TODO This Section - 1 */
  // Associate the signal SIGRTMIN with the sa using the sigaction function
  // Associate the appropriate handler with the SIGUSR1 signal, for removing dependencies
  
  if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }
  
  // Use the simple signal() API for SIGUSR1 as requested by the starter
  if (signal(SIGUSR1, RemoveDepHandler) == SIG_ERR) {
    perror("signal");
    exit(1);
  }
  
  // Until all steps have been completed, check if steps are ready to be run and create a timer for them if so
  
  int totalSteps = recipeSteps->Count();
  
  while (completeCount < totalSteps) {
    // Query ready steps
    vector<Step*> ready = recipeSteps->GetReadySteps();
    
    // Start timers for ready steps (don't start if already running)
    for (Step* s : ready) {
      if (!s->running) {
	s->running = true;
	cout << "Starting Step: " << s->id << " - " << s->description << " (duration " << s->duration << "s)" << endl;
	makeTimer(s, s->duration);
      }
    }
    
    // Sleep briefly to avoid busy spinning and allow signals to be delivered
    usleep(100 * 1000); // 100 ms
  }
  
  /* End Section - 1 */
  
  cout << "Enjoy!" << endl;
  
  // cleanup

  // Delete timers for all steps in stepList
  for (int i = 0; i < recipeSteps->Count(); i++) {
    Step* s = recipeSteps->GetReadySteps()[i];
    timer_delete(s->t_id);
  }

<<<<<<< HEAD
	// Until all steps have been completed, check if steps are ready to be run and create a timer for them if so
	
	int totalSteps = recipeSteps->Count();

    while (completeCount < totalSteps) {
        // Query ready steps
        vector<Step*> ready = recipeSteps->GetReadySteps();

        // Start timers for ready steps (don't start if already running)
        for (Step* s : ready) {
            if (!s->running) {
                s->running = true;
                cout << "Starting Step: " << s->id << " - " << s->description << " (duration " << s->duration << "s)" << endl;
                makeTimer(s, s->duration);
            }
        }

        // Sleep briefly to avoid busy spinning and allow signals to be delivered
        usleep(100 * 1000); // 100 ms
    }
	
	/* End Section - 1 */

	cout << "Enjoy!" << endl;

	// cleanup
    delete recipeSteps;
    delete completedSteps;

    return 0;
  
  delete recipeSteps;
  delete completedSteps;
  
  return 0;
}
