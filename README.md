# concurrent-Collatz
This program calculates the number of steps within main loop of Collatz conjecture
needed to reduce given input down to 1. To do that, various concurrent methods
of programming are tested. TeamNewThreads creates a new thread for eacth calculation,
but no more than the number allowed. TeamNewProcesses works similarly, but for processes.
TeamConstThreads creates a constant, given number of threads that do the same amount
of work concurrently. TeamAsync uses functions from std::async and TeamPool uses 
a thread pool implemented in lib/pool. To run the program just type make and then ./main.
PDF wg429211 has a report of all teams' performances, however, it's in Polish.
This is a project for my university Course at MIM faculty of University of Warsaw.
I was given starting code from 'lib' and contest, generators and main. 
My work consisted of implementing all Teams and making sure they run in an optimal
and concurrent way.
