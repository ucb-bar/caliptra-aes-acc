#ifndef __ACCEL_H
#define __ACCEL_H

#include <stdint.h>
#include <stddef.h>
#define USING_OPCODE8
#ifdef USING_OPCODE8
#include "rocc_opcode8.h"
#else
#include "rocc.h"
#endif
#include <stdbool.h>

#define AES256_OPCODE 1

#define FUNCT_SFENCE 0
#define FUNCT_SRC_INFO 1
#define FUNCT_MODE 4
#define FUNCT_KEY_0 5
#define FUNCT_KEY_1 6
#define FUNCT_DEST_INFO 2
#define FUNCT_CHECK_COMPLETION 3

unsigned char * Aes256AccelSetup(size_t write_region_size);

void Aes256AccelNonblocking(bool encrypt,
                            const unsigned char* data,
                            size_t data_length,
                            uint64_t key0,
                            uint64_t key1,
                            uint64_t key2,
                            uint64_t key3,
                            unsigned char* result,
                            int64_t* success_flag);

int64_t Aes256Accel(bool encrypt,
                const unsigned char* data,
                size_t data_length,
                uint64_t key0,
                uint64_t key1,
                uint64_t key2,
                uint64_t key3,
                unsigned char* result);

volatile int64_t Aes256BlockOnCompletion(volatile int64_t * completion_flag);

#endif //__ACCEL_H
