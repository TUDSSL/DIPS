
// Created by tomh on 7-6-22.
//

#ifndef USERTESTING_NRF_BASE_CHECKPOINTING_H
#define USERTESTING_NRF_BASE_CHECKPOINTING_H


int restore_checkpoint();

void store_checkpoint(int taskID);

void clear_mem();



#endif //USERTESTING_NRF_BASE_CHECKPOINTING_H
