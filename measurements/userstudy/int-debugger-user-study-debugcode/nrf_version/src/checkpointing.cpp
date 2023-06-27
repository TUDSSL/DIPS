#include "checkpointing.h"
#include "nrf.h"
#include "EEPROM.h"


/**
 * You can assume this function is correct
 */
int restore_checkpoint(){
    return EEPROM.read(0);
}


/**
 * You can assume this function is correct
 */
void store_checkpoint(int taskID){
    EEPROM.write(0, taskID);
};


/**
 * You can assume this function is correct
 */
void clear_mem(){
    // We just remove it so we can calculate it again next round :)
    EEPROM.write(0, 0);
    EEPROM.write(1, 0);
    EEPROM.write(2, 0);
    EEPROM.write(3, 0);
    EEPROM.write(4, 0);
    EEPROM.write(5, 0);
}
