#include "philosopher.h"

// TODO: define some sem if you need
int eatnum;
int chops[5];

void init() {
  // init some sem if you need
  eatnum = sem_open(4);
  chops[0] = sem_open(1);
  chops[1] = sem_open(1);
  chops[2] = sem_open(1);
  chops[3] = sem_open(1);
  chops[4] = sem_open(1);
}

void philosopher(int id) {
  // implement philosopher, remember to call `eat` and `think`
  while (1) {
    P(eatnum);
    P(chops[id]);
    P(chops[(id+1)%5]);
    eat(id);
    V(chops[(id+1)%5]);
    V(chops[id]);
    V(eatnum);
  }
}
