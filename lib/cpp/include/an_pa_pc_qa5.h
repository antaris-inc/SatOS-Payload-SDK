// This should go in the header
#ifndef AN_PA_PC_QA5_H
#define AN_PA_PC_QA5_H

/*---------------
 * Include files
 *---------------*/
#include "OsAL.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#undef UNICODE
#include "Cgos.h"
#include "CgosPriv.h"

#include "an_pa_pc_qa5.h"

// GPIO functions
int write_pin(int port, int pin_number, int value);
int read_pin(int port, int pin_number);

// I2C functions
unsigned int get_i2c_count();
unsigned int write_i2c(unsigned short i2c_dev, unsigned char i2c_address, unsigned short index, unsigned char *data);
unsigned int read_i2c(unsigned short i2c_dev, unsigned char i2c_address, unsigned short index, unsigned char *data);

// Init functions
void init_interrupt_pin();
int init_qa7_lib();
int deinit_qa7_lib(void);

#endif 