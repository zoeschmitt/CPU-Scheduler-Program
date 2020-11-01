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

struct eventQueue {
  float time;
  int type; // arrival, departure, allocation, preemption

  struct eventQueue *next;
  struct processQueue *processes;
};

struct processQueue {
  float arrival;
  float start;
  float restart;
  float finish;
  float service;
  float remaining;
  struct processQueue *next;
};

struct cpu {
  float clock;
  bool idle;
  struct processQueue *process;
};

struct readyQueue {
  struct readyQueue *next;
  struct processQueue *process;
};

////////////////////////////////////////////////////////////////
// function definition
void init();
void generate_report();
float urand();
float genexp(float lambda);
int run_sim();
void popReadyQueue();
void popEvent();
void FCFS();

////////////////////////////////////////////////////////////////
//Global variables
cpu *cHead;
eventQueue *eHead;   // head of event queue
processQueue *pHead; // head of process queue
readyQueue *rHead;   // head of ready queue
int scheduler;
float clock; // simulation clock
int lambda;
float sTime;
float quantum;
float quantumClock;
int limit = 10000;

////////////////////////////////////////////////////////////////
void init() {
  // initialize all varilables, states, and end conditions
  // schedule first events
  cHead = new cpu;
  cHead->clock = 0.0;
  cHead->idle = true;
  cHead->process = NULL;

  eHead = new eventQueue;
  eHead->type = 1; //first one arriving
  eHead->next = NULL;

  pHead = new processQueue;
  pHead->arrival = genexp(float(lambda));
  pHead->start = 0.0;
  pHead->restart = 0.0;
  pHead->finish = 0.0;
  pHead->service = genexp(float(1/sTime));
  pHead->remaining = pHead->service;
  pHead->next = NULL;

  eHead->processes = pHead;
  eHead->time = pHead->arrival;
}
////////////////////////////////////////////////////////////////
void generate_report() {
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
  
  report << scheduler << "\t";
  report << lambda << "\t";
  report << sTime << "\t";
  report << avgTurnAroundTime() << "\t";
  report << getTotalThroughput() << "\t";
  report << cpuUtilization() << "\t";
  //report << avgnumproc() << "\t";
  report << quantum << endl;
  //report << "sc.\n";

  report.close();
}

////////////////////////////////////////////////////////////////
// returns a random number between 0 and 1
float urand() {
  return ((float)rand() / RAND_MAX);
}
/////////////////////////////////////////////////////////////
// returns a random number that follows an exp distribution
float genexp(float lambda) {
  float u, x;
  x = 0;
  while (x == 0)
  {
    u = urand();
    x = (-1 / lambda) * log(u);
  }
  return (x);
}

int run_sim() {
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

void popReadyQueue() {
  readyQueue *tempPtr = rHead;
  rHead = rHead->next;
  delete tempPtr;
}

void popEvent() {
  eventQueue *tempPtr = eHead;
  eHead = eHead->next;
  delete tempPtr;
}

void FCFS() {
  int departed = 0;

  while (departed < limit) {
    if (cHead->idle == true) {
      scheduleArrival();
      if (rHead) {
        scheduleAllocation();
      }
    } else {
      scheduleDeparture();
    }

    if (eHead->type == 1) 
      arrival();
    else if (eHead->type == 2) {
      departure();
      departed += 1;
    } else if (eHead->type == 3) 
      allocation();
    
  }
}

void SRTF() {
  int departed = 0;
  int arrived = 0;

  while (departed < limit) {
    if (arrived < limit*1.2) {
      scheduleArrival();
      arrived += 1;
    }
    if (cHead->idle == true) {
      if (rHead) 
        scheduleAllocation();   
    } else {
      if (eHead->type == 1) {
        if (eHead->time > finishTime()) 
          scheduleDeparture();
        else if (isPreemptive()) 
          schedulePreemption();
      }
    }

    if (eHead->type == 1) 
      arrival();
    else if (eHead->type == 2) {
      departure();
      departed += 1;
    } else if (eHead->type == 3) 
      allocation();
    else if (eHead->type == 4) 
      preemption();
    
  }
}

processQueue* srtfProcess() {
  readyQueue* process = rHead;
  processQueue* srt = process->process;
  float rem = process->process->remaining;

  while (process) {
    if (process->process->remaining < rem) {
      rem = process->process->remaining;
      srt = process->process;
    }
    process = process->next;
  }
  return srt;
}

void HRRN() {
  int departed = 0;

  while (departed < limit) {
    if (cHead->idle == true) {
      scheduleArrival();
      if (rHead) {
        scheduleAllocation();
      }
    } else 
        scheduleDeparture();
    
    if (eHead->type == 1) 
      arrival();
    else if (eHead->type == 2) {
      departure();
      departed += 1;
    } else if (eHead->type == 3) 
        allocation();
  }
}

processQueue* hrrnProcess() {
  readyQueue* proc = rHead;
  processQueue* hrrn = proc->process;
  float respRatio = responseRatio(hrrn);

  while (proc) {
    if (responseRatio(proc->process) > respRatio) {
      respRatio = responseRatio(proc->process);
      hrrn = proc->process;
    }
    proc = proc->next;
  }
  return hrrn;
}

float responseRatio(processQueue* process) {
  return ((cHead->clock - process->arrival) + process->service) / process->service;
}

void RR() {
  int departed = 0;
  int arrived = 0;
  while (departed < limit) {
    if (arrived < limit*1.2) {
      scheduleArrival();
      arrived += 1;
    }
    if (cHead->idle == true) {
      scheduleArrival();
      if (rHead) {
        scheduleQuantumAlloc();
      }
    } else {
      if (finishTime() < nextQuantum()) 
        scheduleQuantumDeparture();
      else {
        if (rHead) {
          if (rHead->process->arrival > finishTime()) {
            scheduleQuantumDeparture();
          } else {
            scheduleQuantumPreemption();
          }
        }
      }
    }

    if (eHead->type == 1) 
      arrival();
    else if (eHead->type == 2) {
      quantumDeparture();
      departed += 1;
      if (rHead != 0 && rHead->process->arrival < cHead->clock) {
        scheduleQuantumAlloc();
      }
    } else if (eHead->type == 3)
      quantumAllocation();
    else if (eHead->type == 4) 
      quantumPreemption();
    
  }
}

void scheduleQuantumAlloc() {
  eventQueue* eve = new eventQueue;
  processQueue* process;
  float quantumCount = 0.0;
  process = rHead->process;
  if (rHead) {
    if (rHead->process->arrival < cHead->clock) {
      eve->time = cHead->clock;
    } else {
      cHead->clock = rHead->process->arrival;
      quantumCount = quantumClock;
      while (quantumCount < cHead->clock) {
        quantumCount += quantum;
      }
      quantumClock = quantumCount;
      eve->time = nextQuantum();
    }
  }
  eve->type = 3;
  eve->processes = process;
  eve->next = NULL;

  insertEvent(eve);
}

float nextQuantum() {
  float nextQuantum = quantumClock;
  while (nextQuantum < rHead->process->arrival) {
    nextQuantum += quantum;
  }
  return nextQuantum;
}

void quantumAllocation () {
  cHead->process = eHead->processes;
  cHead->idle = false;

  if (cHead->process->start == 0) {
    cHead->process->start = eHead->time;
  } else {
    cHead->process->restart = eHead->time;
  }

  popReadyQueue();
  popEvent();
}

void scheduleQuantumDeparture() {
  eventQueue* eve = new eventQueue;
  eve->type = 2;
  eve->processes = cHead->process;
  eve->next = NULL;

  if (cHead->process->restart == 0) {
    eve->time = cHead->process->start + cHead->process->remaining;
  } else {
    eve->time = cHead->process->restart + cHead->process->remaining;
  }

  insertEvent(eve);
}

void quantumDeparture() {
  cHead->process->finish = eHead->time;
  cHead->process->remaining = 0.0;
  cHead->clock = eHead->time;
  cHead->idle = true;
  cHead->process = NULL;

  popEvent();
}

void scheduleQuantumPreemption() {
  eventQueue* eve = new eventQueue;
  float quantumCount = quantumClock;
  eve->type = 4;
  eve->next = NULL;
  cHead->clock = rHead->process->arrival;

  while (quantumCount < cHead->clock) {
    quantumCount += quantum;
  }

  quantumClock = quantumCount;
  eve->time = quantumClock + quantum;
  eve->processes = rHead->process;
  insertEvent(eve);
}

void quantumPreemption() {
  processQueue* prepro = cHead->process;
  eventQueue* preeve = new eventQueue;
  float quantumCount = quantumClock;
  cHead->process->remaining = finishTime() - eHead->time;
  cHead->process = eHead->processes;
  cHead->clock = eHead->time;

  if (cHead->process->start = 0.0)
    cHead->process->start = eHead->time;
  else
    cHead->process->restart = eHead->time;
  
  while (quantumCount < eHead->time)
    quantumCount += quantum;

  quantumClock = quantumCount;
  preeve->time = eHead->time;
  preeve->type = 1;
  preeve->processes = prepro;
  preeve->next = NULL;

  popEvent();
  popReadyQueue();
  insertEvent(preeve);
}

void scheduleArrival() {
  processQueue* process = pHead;
  eventQueue* event = new eventQueue;

  while (process->next != NULL) {
    process = process->next;
  }

  process->next = new processQueue;
  process->next->arrival = process->arrival + genexp(float(lambda));
  process->next->start = 0.0;
  process->next->finish = 0.0;
  process->next->restart = 0.0;
  process->next->service = genexp(1/sTime);
  process->next->remaining = process->next->service;
  process->next->next = NULL;

  event->time = process->next->arrival;
  event->type = 1; //arrival
  event->processes = process->next;
  event->next = NULL;

  insertEvent(event);
}

void arrival() {
  readyQueue* event = new readyQueue;
  event->process = eHead->processes;
  event->next = NULL;

  if (rHead) {
    readyQueue* eve = rHead;
    while (eve->next) {
      eve = eve->next;
    }
    eve->next = event;
  } else 
      rHead = event;
  
  popEvent();
}

void scheduleDeparture() {
  eventQueue* dep = new eventQueue;
  dep->type = 2;
  dep->processes = cHead->process;
  dep->next = NULL;

  if (scheduler == 1 || scheduler == 3) {
    dep->time = cHead->process->start + cHead->process->remaining;
  } else if (scheduler == 2) {
    if (cHead->process->restart == 0) {
      dep->time = cHead->process->start + cHead->process->remaining;
    } else {
      dep->time = cHead->process->restart + cHead->process->remaining;
    }
  }
  insertEvent(dep);
}

void departure() {
  cHead->clock = eHead->time;
  cHead->process->finish = cHead->clock;
  pHead->finish = cHead->process->finish;
  cHead->process->remaining = 0.0;
  cHead->process = NULL;
  cHead->idle = true;
  popEvent();
}

void scheduleAllocation() {
  eventQueue* eve = new eventQueue;
  processQueue* newProcess;

  if (scheduler == 1) {
    newProcess = rHead->process;
  } else if (scheduler == 2) {
    if (cHead->clock > rHead->process->arrival) {
      newProcess = srtfProcess();
    } else {
      newProcess = rHead->process;
    }
  } else if (scheduler == 3) {
    newProcess = hrrnProcess();
  } 

  if (cHead->clock < newProcess->arrival) {
    eve->time = newProcess->arrival;
  } else {
    eve->time = cHead->clock;
  }

  eve->type = 3;
  eve->processes = newProcess;
  eve->next = NULL;

  insertEvent(eve);
}

void allocation() {
  cHead->process = eHead->processes;

  if (scheduler == 2 || scheduler == 3) {
    readyQueue* eveHead = rHead;
    readyQueue* eve = rHead->next;

    if (eveHead->process->arrival != eHead->processes->arrival) {
      while (eve) {
        if (eve->process->arrival == eHead->processes->arrival) {
          eveHead->next = eve->next;
          eve->next = rHead;
          rHead = eve;
          break;
        }
        eve = eve->next;
        eveHead = eveHead->next;
      }
    }
  }
  popReadyQueue();
  popEvent();

  cHead->idle = false;
  if (cHead->clock < cHead->process->arrival) {
    cHead->clock = cHead->process->arrival;
  }
  if (cHead->process->start == 0) {
    cHead->process->restart = cHead->clock;
  } else {
    cHead->process->restart = cHead->clock;
  }
}


float finishTime() {
  float finishTime;
  float startTime = cHead->process->start;
  float restartTime = cHead->process->restart;
  float remainingTime = cHead->process->remaining;

  if (restartTime == 0) {
    finishTime = startTime + remainingTime;
  } else {
    finishTime = restartTime + remainingTime;
  }

  return finishTime;
}

bool isPreemptive() {
  float finish = finishTime();
  float remainingTime = finish - eHead->time;

  if (eHead->time < finish && eHead->processes->remaining < remainingTime) {
    return true;
  } else {
    return false;
  }
}

void schedulePreemption() {
  eventQueue* eve = new eventQueue;
  eve->time = eHead->time;
  eve->type = 4;
  eve->processes = eHead->processes;
  eve->next = NULL;
  popEvent();
  insertEvent(eve);
}

void preemption() {
  processQueue* prepro = cHead->process;
  eventQueue* preeve = new eventQueue;
  cHead->process->remaining = finishTime() - eHead->time;
  cHead->process = eHead->processes;
  cHead->clock = eHead->time;

  if (cHead->process->restart == 0) {
    cHead->process->start = eHead->time;
  } else {
    cHead->process->restart = eHead->time;
  }

  preeve->time = eHead->time;
  preeve->type = 1;
  preeve->processes = prepro;
  preeve->next =  NULL;

  popEvent();
  insertEvent(preeve);
}

void insertEvent(eventQueue* eve) {
  if (eHead)
    eHead = eve;
  else if (eHead->time > eve->time) {
    eve->next = eHead;
    eHead = eve;
  } else {
    eventQueue* newEve = eHead;
    while (newEve) {
      if (newEve->time < eve->time && newEve->next == NULL) {
        newEve->next = eve;
        break;
      } else if (newEve->time < eve->time && newEve->next->time > newEve->time) {
          eve->next = newEve->next;
          newEve->next = eve;
          break;
      } else {
        newEve = newEve->next;
      }
    }
  }
}

//do we need the counts or what?
float avgTurnAroundTime()
{
  processQueue *processes = pHead;
  float turnAround = 0.0;

  while (processes->next) {
    if (processes->finish == -1)
      continue;
    else
      turnAround += processes->finish - processes->arrival;
    
    processes = processes->next;
  }
  return turnAround / limit;
}

//is it even adding up the finished ones?
float getTotalThroughput() {
  processQueue *processes = pHead;
  float finishTime = 0.0;
  float totalProcesses = 0.0;

  while (processes->next) {
    if (processes->finish == -1)
      continue;
    else {
      finishTime = processes->finish;
      totalProcesses++;
    }
    processes = processes->next;
  }
  return totalProcesses / finishTime;
}

float cpuUtilization() {
  processQueue *processes = pHead;
  float busy = 0.0;
  float finishTime = 0.0;

  while (processes->next) {
    if (processes->finish == 0) 
      continue;

    finishTime = processes->finish;
    busy += processes->service;
    processes = processes->next;
  }

  return busy / finishTime;
}

// float avgInQueue() {
//   float time = 0.0;
//   processQueue* proc = pHead;

//   while (proc->next) {
//     if (proc->finish == 0)
//       continue;
//     time = proc->finish;
//     proc = proc->next;
//   }

// }



int main(int argc, char *argv[])
{
  // parse arguments
  scheduler = atoi(argv[1]);
  lambda = atoi(argv[2]);
  sTime = (float)atof(argv[3]);
  quantum = (float)atof(argv[4]);

  init();
  run_sim();
  generate_report();

  return 0;
}
