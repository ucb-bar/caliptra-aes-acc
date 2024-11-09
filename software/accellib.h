#ifndef __ACCEL_H
#define __ACCEL_H

#include <stdint.h>
#include <stddef.h>
#include "rocc.h"
#include <stdbool.h>

#define AES256_OPCODE 1

#define FUNCT_SFENCE 0
#define FUNCT_SRC_INFO 1
#define FUNCT_MODE 4
#define FUNCT_KEY_0 5
#define FUNCT_KEY_1 6
#define FUNCT_IV 7
#define FUNCT_DEST_INFO 2
#define FUNCT_CHECK_COMPLETION 3

void AESCBCPinPages(void);
void AESCBCUnpinPages(void);

unsigned char * AESCBCAccelSetup(size_t write_region_size);

void AESCBCAccelNonblocking(bool encrypt,
                const unsigned char* data,
                size_t data_length,
                uint64_t key0,
                uint64_t key1,
                uint64_t key2,
                uint64_t key3,
                uint64_t iv0,
                uint64_t iv1,
                unsigned char* result,
                uint64_t* success_flag);

uint64_t AESCBCAccel(bool encrypt,
                const unsigned char* data,
                size_t data_length,
                uint64_t key0,
                uint64_t key1,
                uint64_t key2,
                uint64_t key3,
                uint64_t iv0,
                uint64_t iv1,
                unsigned char* result);

volatile uint64_t AESCBCBlockOnCompletion(volatile uint64_t * completion_flag);

#endif //__ACCEL_H
