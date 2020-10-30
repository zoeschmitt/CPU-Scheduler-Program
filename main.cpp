#include <cstdlib>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
using namespace std;

struct eventQueue
{
  float time;
  int type;

  struct eventQueue *next;
  struct processQueue *processes;
};

struct processQueue
{
  float arrival;
  float start;
  float restart;
  float finish;
  float service;
  float remaining;
  struct processQueue *next;
};

struct cpu
{
  float clock;
  bool idle;
  struct processQueue *process;
};

struct readyQueue
{
  struct readyQueue *next;
  struct processQueue *process;
};

////////////////////////////////////////////////////////////////
// function definition
void init();
int run_sim();
void generate_report();
// int schedule_event(struct event*);
// int process_event1(struct event* eve);
// int process_event2(struct event* eve);

////////////////////////////////////////////////////////////////
//Global variables
cpu *cHead;
eventQueue *eHead;   // head of event queue
processQueue *pHead; // head of process queue
readyQueue *rHead;   // head of ready queue
int scheduler;
float clock; // simulation clock
int lambda;
float avgTA;
float quantum;
int limit = 10000;

////////////////////////////////////////////////////////////////
void init()
{
  // initialize all varilables, states, and end conditions
  // schedule first events
}
////////////////////////////////////////////////////////////////
void generate_report()
{
  ofstream report;
  string schedulerType;
  report.open("report.txt");
  
  if (scheduler == 1) {
    schedulerType = "FCFS";
  } else if (scheduler == 2) {
    schedulerType = "SRTF";
  } else if (scheduler == 3) {
    schedulerType = "HRRN";
  } else {
    schedulerType = "RR";
  }
  
  
  report << "Writing this to a file.\n";
  report.close();
}
//////////////////////////////////////////////////////////////// //schedules an event in the future
int schedule_event()
{
  // insert event in the event queue in its order of time
  
}
////////////////////////////////////////////////////////////////
// returns a random number between 0 and 1
float urand()
{
  return ((float)rand() / RAND_MAX);
}
/////////////////////////////////////////////////////////////
// returns a random number that follows an exp distribution
float genexp(float lambda)
{
  float u, x;
  x = 0;
  while (x == 0)
  {
    u = urand();
    x = (-1 / lambda) * log(u);
  }
  return (x);
}

int run_sim()
{

  switch (scheduler)
  {

  case 1:
    FCFS();
    break;
  case 2:
    SRTF();
    break;
  case 3:
    HRRN();
    break;
  case 4:
    RR();
    break;
  default:
    cout << "Error" << endl;
  }

  return 0;
}

void popReadyQueue()
{
  readyQueue *tempPtr = rHead;
  rHead = rHead->next;
  delete tempPtr;
}

void popEvent()
{
  eventQueue *tempPtr = eHead;
  eHead = eHead->next;
  delete tempPtr;
}

void FCFS()
{
}

void SRTF()
{
}

void HRRN()
{
}

void RR()
{
}

//do we need the counts or what?
float avgTurnAroundTime()
{
  processQueue *processes = pHead;
  float turnAround = 0.0;

  while (processes->next != NULL)
  {
    if (processes->finish == -1)
      continue;
    else
    {
      turnAround += processes->finish - processes->arrival;
      // count++;
    }
    processes = processes->next;
  }
  return turnAround / limit;
}

//is it even adding up the finished ones?
float getTotalThroughput()
{
  processQueue *processes = pHead;
  float finishTime = 0.0;
  float totalProcesses = 0.0;

  while (processes->next != NULL)
  {
    if (processes->finish == -1)
      continue;
    else
    {
      finishTime = processes->finish;
      totalProcesses++;
    }

    processes = processes->next;
  }
  cout << "Total Time: " << finishTime << endl;
  return (totalProcesses / finishTime);
}

float cpuUtilization()
{
  processQueue *processes = pHead;
  float busy = 0.0;
  float finishTime = 0.0;

  while (processes->finish == 0)
  {
    finishTime = processes->finish;
    busy += processes->service;

    processes = processes->next;
  }
  cout << "finishTime: " << finishTime << endl;
  cout << "busy: " << busy << endl;
  return busy / finishTime;
}

float avgInQueue()
{
}



int main(int argc, char *argv[])
{
  // parse arguments
  scheduler = atoi(argv[1]);
  lambda = atoi(argv[2]);
  avgTA = (float)atof(argv[3]);
  quantum = (float)atof(argv[4]);

  init();
  run_sim();
  generate_report();

  return 0;
}
