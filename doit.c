/* myexec.c */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <cstring>
#include <algorithm>
using namespace std;

//Prints usage stats of child process
int usageStats (rusage usage, double totalTime) {
  printf("User CPU time: %ld.%06ld seconds\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
        printf("System CPU time: %ld.%06ld seconds\n", usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
        printf("Elapsed wall clock: %f seconds\n", totalTime);
        printf("Involuntary context switches: %ld\n", usage.ru_nivcsw);
        printf("Voluntary context switches: %ld\n", usage.ru_nvcsw);
        printf("Major page faults: %ld\n", usage.ru_majflt);
        printf("Minor page faults: %ld\n", usage.ru_minflt);
        printf("Maximum resident set size: %ld kilobytes\n", usage.ru_maxrss);

        return 1;
}

char* convertString (const string &s) {
  char* newString = new char[s.size() + 1];
  strcpy (newString, s.c_str());
  return newString;
}

main(int argc, char *argv[]) {
/* argc -- number of arguments */
/* argv -- an array of strings */

  int pid;
  struct rusage usage;
  struct timeval tv1;
  struct timeval tv2;
  struct timezone tz;

  //If a command is put in upon initially calling doit
  if (argc != 1) {

    if ((pid = fork()) < 0) {
      fprintf(stderr, "Fork error\n");
      exit(1);
    } else if (pid == 0) {
      /* child process */
      if (execvp(argv[1], &argv[1]) < 0) {
        fprintf(stderr, "Execve error\n");
        exit(1);
      }
    }
    else {
      /* parent */
      gettimeofday(&tv1, &tz); //Getting the time before the cild process begins
      wait(0);/* wait for the child to finish */
      gettimeofday(&tv2, &tz); //Getting finished time value
      
      double tv1Total = tv1.tv_sec + (tv2.tv_usec/1000000.0);
      double tv2Total = tv1.tv_sec + (tv1.tv_usec/1000000.0);
      double totalTime = tv1Total - tv2Total;

      getrusage(RUSAGE_CHILDREN, &usage);
      usageStats(usage, totalTime);
    }
  } else { //If no command is put in initially, doit turns into a sort of shell
    bool running = true;
    while (running) {
      int pid2;

      //Getting the input
      cout << "==> ";
      string input;
      getline(cin, input);
      const char* inputArray;
      inputArray = input.c_str();
      vector<string> argvNew;
      vector<char*> argChars;
      string currentArgument = "";

      if (input == "exit") {
        running = false;
        break;
      }

      if ((pid = fork()) < 0) {
        fprintf(stderr, "Fork error\n");
        exit(1);
      } else if (pid == 0) {
        /* child process */
        int counter1 = 0;
        int counter2 = 0;

        for(int i = 0; i < input.length(); i++) {
          if ((int) inputArray[i] == 32) { //Checking for spaces
            argvNew.push_back(currentArgument);
            currentArgument = "";
          } else {
            currentArgument.append(string(1, inputArray[i]));
          }
        }
        currentArgument.append("\0");
        argvNew.push_back(currentArgument);

        //Running the command
        if(argvNew[0] == "cd") {
          chdir(argvNew[1].c_str());
        } else if (argvNew[0] == "exit") {
          break;
        } else if (argvNew[argvNew.size() - 1] == "&") {
          if ((pid2 = fork()) < 0) {
            fprintf(stderr, "Fork error\n");
          } else if (pid == 0) {
            std::transform (argvNew.begin(), argvNew.end(), back_inserter(argChars), convertString);

            if (execvp(argChars[0], &argChars[0]) < 0) {
              fprintf (stderr, "Execvp error\n");
              exit(1);
            }
          }
        } else {
          std::transform (argvNew.begin(), argvNew.end(), back_inserter(argChars), convertString);

          if (execvp(argChars[0], &argChars[0]) < 0) {
            fprintf(stderr, "Execvp error\n");
            exit(1);
          }
        }
      } else {
        /* parent */
      
        //Getting initial time value 
        gettimeofday(&tv1, &tz);
        wait(0);/* wait for the child to finish */
        gettimeofday(&tv2, &tz); //Getting finished time value
        double tv1Total = tv1.tv_sec + (tv1.tv_usec / 1000000.0);
        double tv2Total = tv2.tv_sec + (tv2.tv_usec/1000000.0);  
        double totalTime = tv2Total - tv1Total;

        getrusage(RUSAGE_CHILDREN, &usage);
        usageStats(usage, totalTime);
      }
    }
  }
}
