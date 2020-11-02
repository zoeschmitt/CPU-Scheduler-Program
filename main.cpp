#include <iostream>
#include <time.h>
#include <fstream>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iomanip>
using namespace std;

////////////////////////////////////////////////////////////////
//Structures
struct readyQueue {
   struct processNode *processes;
   struct readyQueue *next;
};

struct eventQueue {
   float time;
   int type;

   struct eventQueue *next;
   struct processNode *processes;
};

struct cpu {
   float clock;
   bool busy;

   struct processNode *processes;
};

struct processNode {
   float arrival;
   float start;
   float restart;
   float finish;
   float service;
   float remaining;

   struct processNode *next;
};

////////////////////////////////////////////////////////////////
//Global variables
int scheduler;
int lambda;
float serviceTime;
float quantum;
int limit = 10000;
float quantumClock;
eventQueue *eHead;
processNode *pHead;
readyQueue *rHead;
cpu *cHead;

////////////////////////////////////////////////////////////////
// function declerations
void insertEvent(eventQueue *);
float ResponseRatio(processNode *);
processNode *srtfProcess();
processNode *hrrnProcess();
float genexp(float val);
void FCFS();
void SRTF();
void HRRN();
void RR();
void arrival();
void scheduleDeparture();
void allocate();
void scheduleAllocation();
void scheduleArrival();
void departure();
void popEvent();
bool isPreemptive();
float nextQuantum();
void popReadyQueue();
float finishTime();
void preempt();

// Setting default values.
void init() {
   quantumClock = 0.0;

   pHead = new processNode;
   pHead->arrival = genexp((float)lambda);
   pHead->start = 0.0;
   pHead->restart = 0.0;
   pHead->finish = 0.0;
   pHead->service = genexp((float)1.0 / serviceTime);
   pHead->remaining = pHead->service;
   pHead->next = NULL;

   eHead = new eventQueue;
   eHead->time = pHead->arrival;
   eHead->type = 1;
   eHead->next = NULL;
   eHead->processes = pHead;

   cHead = new cpu;
   cHead->clock = 0.0;
   cHead->busy = false;
   cHead->processes = NULL;
}

////////////////////////////////////////////////////////////////
// returns a random number between 0 and 1
float urand() {
  return ((float)rand() / RAND_MAX);
}
/////////////////////////////////////////////////////////////
// returns a random number that follows an exp distribution
float genexp(float lambda) {
  float u = 0.0;
  float x = 0.0;

  while (x == 0)
  {
    u = urand();
    x = (-1 / lambda) * log(u);
  }
  return (x);
}

float finishTime() {
   float finish;
   float start;
   float restart;
   float remaining;

   if (cHead->processes) {
      start = cHead->processes->start;
      restart = cHead->processes->restart;
      remaining = cHead->processes->remaining;
   } else {
      start = 0.0;
      restart = 0.0;
      remaining = 0.0;
   }

   if (restart == 0.0)
      finish = start + remaining;
   else
      finish = restart + remaining;

   return finish;
}

bool isPreemptive() {
   float finish = finishTime();
   float cpuRemainingTime = finish - eHead->time;

   if ((eHead->time < finish) && (eHead->processes->remaining < cpuRemainingTime))
      return true;
   else
      return false;
}

void scheduleArrival() {
   processNode *proc = pHead;
   while (proc->next != NULL)
      proc = proc->next;
   
   proc->next = new processNode;
   proc->next->arrival = proc->arrival + genexp((float)lambda);
   proc->next->start = 0.0;
   proc->next->restart = 0.0;
   proc->next->finish = 0.0;
   proc->next->service = genexp((float)1.0 / serviceTime);
   proc->next->remaining = proc->next->service;
   proc->next->next = NULL;

   eventQueue *eve = new eventQueue;
   eve->time = proc->next->arrival;
   eve->type = 1;
   eve->processes = proc->next;
   eve->next = NULL;

   insertEvent(eve);
}

void scheduleAllocation() {
   eventQueue *eve = new eventQueue;
   processNode *nextProc;
   if (scheduler == 1)
      nextProc = rHead->processes;
   else if (scheduler == 2) {
      if (cHead->clock > rHead->processes->arrival)
         nextProc = srtfProcess();
      else
         nextProc = rHead->processes;  
   } else if (scheduler == 3)
      nextProc = hrrnProcess();
   
   if (cHead->clock < nextProc->arrival)
      eve->time = nextProc->arrival;
   else
      eve->time = cHead->clock;
   
   eve->type = 3;
   eve->next = NULL;
   eve->processes = nextProc;

   insertEvent(eve);
}

void scheduleDeparture() {
   eventQueue *eve = new eventQueue;
   eve->type = 2;
   eve->next = 0;
   eve->processes = cHead->processes;

   if (scheduler == 1 || scheduler == 3)
      eve->time = cHead->processes->start + cHead->processes->remaining;
   else if (scheduler == 2) {
      if (cHead->processes->restart == 0)
         eve->time = cHead->processes->start + cHead->processes->remaining;
      else
         eve->time = cHead->processes->restart + cHead->processes->remaining;
   }
   insertEvent(eve);
}

void arrival() {
   readyQueue *ready = new readyQueue;
   ready->processes = eHead->processes;
   ready->next = NULL;

   if (rHead == NULL)
      rHead = ready;
   else {
      readyQueue* ready1 = rHead;
      while (ready1->next != 0)
         ready1 = ready1->next;
      
      ready1->next = ready;
   }
   popEvent();
}

void allocate() {
   cHead->processes = eHead->processes;
   if (scheduler == 2 || scheduler == 3) {
      readyQueue *ready = rHead->next;
      readyQueue *readyHead = rHead;
      if (readyHead->processes->arrival != eHead->processes->arrival) {
         while (ready != 0) {
            if (ready->processes->arrival == eHead->processes->arrival) {
               readyHead->next = ready->next;
               ready->next = rHead;
               rHead = ready;
               break;
            }
            ready = ready->next;
            readyHead = readyHead->next;
         }
      }
   }
   popReadyQueue();
   popEvent();
   cHead->busy = true;
   if (cHead->clock < cHead->processes->arrival)
      cHead->clock = cHead->processes->arrival;
   
   if (cHead->processes->start == 0)
      cHead->processes->start = cHead->clock;
   else
      cHead->processes->restart = cHead->clock;
}

void departure() {
   cHead->clock = eHead->time;
   cHead->processes->finish = cHead->clock;
   pHead->finish = cHead->processes->finish;
   cHead->processes->remaining = 0.0;
   cHead->processes = NULL;
   cHead->busy = false;

   popEvent();
}

void preempt() {
   processNode *prepro = cHead->processes;
   cHead->processes->remaining = finishTime() - eHead->time;
   cHead->processes = eHead->processes;
   cHead->clock = eHead->time;
   if (cHead->processes->restart == 0.0)
      cHead->processes->start = eHead->time;
   else
      cHead->processes->restart = eHead->time;

   eventQueue *preeve = new eventQueue;
   preeve->time = eHead->time;
   preeve->type = 1;
   preeve->next = 0;
   preeve->processes = prepro;

   popEvent();
   insertEvent(preeve);
}

float nextQuantum() {
   float nextQuantum = quantumClock;
   while (nextQuantum < rHead->processes->arrival)
      nextQuantum += quantum;
   
   return nextQuantum;
}

void insertEvent(eventQueue *eve) {
   if (eHead == 0)
      eHead = eve;
   else if (eHead->time > eve->time) {
      eve->next = eHead;
      eHead = eve;
   } else {
      eventQueue *eve2 = eHead;
      while (eve2 != 0) {
         if ((eve2->time < eve->time) && (eve2->next == 0)) {
            eve2->next = eve;
            break;
         } else if ((eve2->time < eve->time) && (eve2->next->time > eve->time)) {
            eve->next = eve2->next;
            eve2->next = eve;
            break;
         } else
            eve2 = eve2->next;
      }
   }
}

void popEvent() {
  eventQueue* tempPtr = eHead;
  eHead = eHead->next;
  delete tempPtr;
}

void popReadyQueue() {
  readyQueue* tempPtr = rHead;
  rHead = rHead->next;
  delete tempPtr;
}

float avgTurnaroundTime() {
   float turnaroundTime = 0.0;
   processNode *proc = pHead;
   while (proc->next != NULL) {
      if (proc->finish == 0) {

      } else
         turnaroundTime += (proc->finish - proc->arrival);

      proc = proc->next;
   }
   return turnaroundTime / limit;
}

float throughput() {
   processNode *proc = pHead;
   float finish = 0.0;

   while (proc->next != NULL) {
      if (proc->finish == 0) {
      }
      else
         finish = proc->finish;
      
      proc = proc->next;
   }
   return ((float)limit / finish);
}

float cpuUtilization() {
   processNode *proc = pHead;
   float busy = 0.0;
   float finish = 0.0;
   while (proc->next != NULL) {
      if (proc->finish == 0){ 
      }
      else {
         busy += proc->service;
         finish = proc->finish;
      }
      proc = proc->next;
   }
   return busy / finish;
}

float getAvgNumProcInQ() {
   float min = 0.0;
   processNode *proc = pHead;
   while (proc->next != NULL){
      if (proc->finish == 0){
      }
      else
         min = proc->finish;
      
      proc = proc->next;
   }
   proc = pHead;
   int time1 = 0;
   int processes = 0;
   int time = static_cast<int>(min) + 1;

   for (time1 = 0; time1 < time; time1++) {
      while (proc->finish != 0) {
         if ((proc->arrival < time1 && proc->start > time1) || (proc->arrival > time1 && proc->arrival < (time1 + 1)))
            processes++;
      
         proc = proc->next;
      }
      proc = pHead;
   }
   return ((float)processes / time);
}

void run_sim() {
  switch (scheduler) {
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
}

void FCFS() {
   int departed = 0;

   while (departed < limit) {
      if (cHead->busy == false) {
         scheduleArrival();
         if (rHead != NULL)
            scheduleAllocation();
      } else
         scheduleDeparture();
      if (eHead->type == 1) 
         arrival();
      else if (eHead->type == 2) {
         departure();
         departed += 1;
      } else if (eHead->type == 3)
            allocate();
   }
}

processNode* srtfProcess() {
  readyQueue* process = rHead;
  processNode* srt = process->processes;
  float rem = process->processes->remaining;

  while (process) {
    if (process->processes->remaining < rem) {
      rem = process->processes->remaining;
      srt = process->processes;
    }
    process = process->next;
  }
  return srt;
}

void SRTF() {
   int arrived = 0;
   int departures = 0;

   while (departures < limit) {
      if (arrived < (limit * 1.20)) {
         scheduleArrival();
         arrived++;
      }
      if (cHead->busy == false) {
         if (rHead != NULL)
            scheduleAllocation();
      } else {
         if (eHead->type == 1) {
            if (eHead->time > finishTime())
               scheduleDeparture();
            else if (isPreemptive()) {
               eventQueue *preeve = new eventQueue;
               preeve->time = eHead->time;
               preeve->type = 4;
               preeve->next = 0;
               preeve->processes = eHead->processes;

               popEvent();
               insertEvent(preeve);
            }    
         }
      }
      if (eHead->type == 1)
         arrival();
      else if (eHead->type == 2) {
         departure();
         departures++;
      } else if (eHead->type == 3)
         allocate(); 
      else if (eHead->type == 4)
         preempt();
   }
}

float responseRatio(processNode* process) {
  return ((cHead->clock - process->arrival) + process->service) / process->service;
}

processNode* hrrnProcess() {
  readyQueue* proc = rHead;
  processNode* hrrn = proc->processes;
  float respRatio = responseRatio(hrrn);

  while (proc) {
    if (responseRatio(proc->processes) > respRatio) {
      respRatio = responseRatio(proc->processes);
      hrrn = proc->processes;
    }
    proc = proc->next;
  }
  return hrrn;
}

void HRRN() {
   int departures = 0;
   while (departures < limit) {
      if (cHead->busy == false) {
         scheduleArrival();
         if (rHead != NULL)
            scheduleAllocation();    
      } else
         scheduleDeparture();
      if (eHead->type == 1)
         arrival();
      else if (eHead->type == 2) {
         departure();
         departures++;
      } else if (eHead->type == 3)
         allocate();
   }
}

void RR() {
   int arrivals = 0;
   int departures = 0;
   while (departures < limit) {
      if (arrivals < (limit * 1.1)) {
         scheduleArrival();
         arrivals++;
      }                                                                   
      if (cHead->busy == false) {
         scheduleArrival();
         if (rHead != 0) {
            eventQueue *eve = new eventQueue;
            processNode *nextProc = rHead->processes;
            float quantumCount = quantumClock;

            if (rHead->processes != NULL) {
            if (rHead->processes->arrival < cHead->clock)
            eve->time = cHead->clock;
            else {
            cHead->clock = rHead->processes->arrival;
            while (quantumCount < cHead->clock)
            quantumCount += quantum;

            quantumClock = quantumCount;
            eve->time = nextQuantum();
            }
            } else
            cout << "Error - schedule quantum" << endl;

            eve->type = 3;
            eve->next = 0;
            eve->processes = nextProc;
            insertEvent(eve);
         }
      } else {
            if (finishTime() < quantumClock + quantum){
               eventQueue *eve = new eventQueue;
               eve->type = 2;
               eve->next = 0;
               eve->processes = cHead->processes;

               if (cHead->processes != NULL) {
                  if (cHead->processes->restart == 0)
                     eve->time = cHead->processes->start + cHead->processes->remaining;
                  else
                     eve->time = cHead->processes->restart + cHead->processes->remaining;

               insertEvent(eve);
            }
         } else {
            if (rHead) {
               if (rHead->processes->arrival > finishTime()) {
                  eventQueue *eve = new eventQueue;
                  eve->type = 2;
                  eve->next = 0;
                  eve->processes = cHead->processes;

                  if (cHead->processes != NULL) {
                     if (cHead->processes->restart == 0)
                        eve->time = cHead->processes->start + cHead->processes->remaining;
                     else
                        eve->time = cHead->processes->restart + cHead->processes->remaining;

                  insertEvent(eve);
                  }
               }
               else {
                  eventQueue *preeve = new eventQueue;
                  float quantumCount = quantumClock;
                  preeve->type = 4;
                  preeve->next = 0;
                  cHead->clock = rHead->processes->arrival;

                  while (quantumCount < cHead->clock)
                     quantumCount += quantum;

                  quantumClock = quantumCount;
                  preeve->time = quantumClock + quantum;
                  preeve->processes = rHead->processes;

                  insertEvent(preeve);
               }
            }
         }
      }
      if (eHead != NULL) {
         if (eHead->type == 1)
            arrival();
         else if (eHead->type == 2) {
            cHead->processes->finish = eHead->time;
            cHead->processes->remaining = 0.0;
            cHead->processes = 0;
            cHead->clock = eHead->time;
            cHead->busy = false;
            popEvent();
            departures++;
            if (rHead != 0 && (rHead->processes->arrival < cHead->clock)) {
               eventQueue* eve = new eventQueue;
               float quantumCount = quantumClock;

               if (rHead->processes != NULL) {
                  if (rHead->processes->arrival < cHead->clock)
                     eve->time = cHead->clock;
                  else {
                     cHead->clock = rHead->processes->arrival;
                     while (quantumCount < cHead->clock)
                        quantumCount += quantum;

                     quantumClock = quantumCount;
                     eve->time = nextQuantum();
                  }
               } else {
               cout << "issue w wuantum" << endl;
               }

               eve->type = 3;
               eve->next = 0;
               eve->processes = rHead->processes;
               insertEvent(eve);
            }
         }
         else if (eHead->type == 3) { 
            cHead->processes = eHead->processes;
            cHead->busy = true;

            if (cHead->processes != NULL) {
               if (cHead->processes->start == 0)
                  cHead->processes->start = eHead->time;
               else
                  cHead->processes->restart = eHead->time;
            } else 
            cHead->processes = eHead->processes;

            popReadyQueue();
            popEvent();
         } else if (eHead->type == 4) {
            processNode *prepro = cHead->processes;
            float quantumCount = quantumClock;
            cHead->processes = eHead->processes;
            cHead->clock = eHead->time;
            cHead->processes->remaining = finishTime() - eHead->time;

            if (cHead->processes->start == 0.0)
               cHead->processes->start = eHead->time;
            else
               cHead->processes->restart = eHead->time;

            while (quantumCount < eHead->time)
               quantumCount += quantum;

            quantumClock = quantumCount;
            eventQueue *preeve = new eventQueue;
            preeve->time = eHead->time;
            preeve->type = 1;
            preeve->next = 0;
            preeve->processes = prepro;

            popEvent();
            popReadyQueue();
            insertEvent(preeve);
            }     
      }
   }
}

void generate_report() {
   string schedulerType;

   if (scheduler == 1)
      schedulerType = "FCFS";
   else if (scheduler == 2)
      schedulerType = "SRTF";
   else if (scheduler == 3)
      schedulerType = "HRRN";
   else if (scheduler == 4)
      schedulerType = "RR";

   ofstream report("report.txt", ios::out | ios::app);
   report << schedulerType << "\t\t" << "Lambda: " << lambda << "\t\t" << "Avg T/A: "
          << avgTurnaroundTime() << "\t\t" << "Thru: " << throughput() << "\t\t" << "CPU: " << cpuUtilization() << "\t\t" << "Avg RQ: " << getAvgNumProcInQ() << "\t\t" << endl;

   report.close();
}

int main(int argc, char *argv[]) {
   scheduler = atoi(argv[1]);
   lambda = atoi(argv[2]);
   serviceTime = (float)atof(argv[3]);
   quantum = (float)atof(argv[4]);         // parse arguments from console
   init();                   // initialize the simulation
   run_sim();                // run the simulation
   generate_report();        // output sim stats to file
   cout << endl << "Program Complete";
   return 0;
}
