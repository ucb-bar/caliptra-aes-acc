#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#include "accellib.h"
#include "encoding.h"
#include "rerocc.h"

#define AES_BLOCK_BITS (128)
#define AES_BLOCK_BYTES (AES_BLOCK_BITS / 8)

#define MAX_DATA_LEN_BYTES (8192)

void print_blocks(unsigned char* data, size_t blk_cnt) {
  uint64_t* data64 = (uint64_t*)data;
  for (size_t i = 0; i < blk_cnt; i++) {
    uint64_t* data64_1 = data64 + (2*i) + 1;
    uint64_t* data64_0 = data64 + (2*i) + 0;

    printf("Block[%" PRIu64 "]: 0x%016" PRIx64 "%016" PRIx64 " (%p,%p)\n", i, *data64_1, *data64_0, data64_1, data64_0);
  }
}

int main() {
  srand(0);

  // initialize large array of data to process
  unsigned char data[MAX_DATA_LEN_BYTES];
  for (size_t i = 0; i < MAX_DATA_LEN_BYTES; ++i) {
    //data[i] = i;
    //data[i] = 0;
    data[i] = rand();
  }

  // initialize random key
  uint64_t key[4];
  for (size_t i = 0; i < 4; ++i) {
    //key[i] = i * 2;
    //key[i] = 0;
    key[i] = rand();
  }
  printf("Key:\n");
  print_blocks((unsigned char*)key, 1);

  // initialize random iv
  uint64_t iv[2];
  for (size_t i = 0; i < 2; ++i) {
    iv[i] = (i + 1) * 4;
    //iv[i] = 0;
    //iv[i] = rand();
  }
  printf("IV:\n");
  print_blocks((unsigned char*)iv, 1);

  AESCBCPinPages();

#define CONFIG_ACC_ID 0
#define CONFIG_CFG_ID 15
  if (!rr_acquire_single(CONFIG_CFG_ID, CONFIG_ACC_ID)) {
    printf("Failed acquire\n");
    return 1;
  }

  rr_set_opc(AES256_OPCODE, CONFIG_CFG_ID);
  rr_fence(CONFIG_CFG_ID);
  printf("Done with ReRoCC setup\n");

  // dont' care about tlb
  uint8_t* ciphertext_area = AESCBCAccelSetup(MAX_DATA_LEN_BYTES); // fence, write zero
  uint8_t* plaintext_area = AESCBCAccelSetup(MAX_DATA_LEN_BYTES); // fence, write zero

  size_t cur_sz_bytes = AES_BLOCK_BYTES;
  while (cur_sz_bytes <= MAX_DATA_LEN_BYTES) {
    uint64_t data_len = cur_sz_bytes;

    //print_blocks(data, r);

    printf(">> Encrypt start: L:%lu\n", data_len);


    // printf("src start addr: 0x%016" PRIx64 "\n", (uint64_t)data);
    // printf("dest start addr: 0x%016" PRIx64 "\n", (uint64_t)ciphertext_area);
    #define ITERS (5)
    uint64_t sum = 0;
    for (size_t i = 0; i < ITERS; ++i) {
      uint64_t t1 = rdcycle();
      // encrypt
      AESCBCAccel(true,
                  data,
                  data_len,
                  key[0],
                  key[1],
                  0,
                  0,
                  iv[0],
                  iv[1],
                  ciphertext_area);
      uint64_t t2 = rdcycle();
      printf("Start cycle: %" PRIu64 ", End cycle: %" PRIu64 ", Took: %" PRIu64 "\n",
              t1, t2, t2 - t1);
      sum += t2 - t1;
    }
    printf("Avg: %" PRIu64 "\n", sum / ITERS);


    //print_blocks(ciphertext_area, r);

    // decryption start area
    printf(">> Decrypt start: L:%lu\n", data_len);


    // printf("src start addr: 0x%016" PRIx64 "\n", (uint64_t)ciphertext_area);
    // printf("dest start addr: 0x%016" PRIx64 "\n", (uint64_t)plaintext_area);

    sum = 0;
    for (size_t i = 0; i < ITERS; ++i) {
      uint64_t t1 = rdcycle();

      // decrypt
      AESCBCAccel(false,
                  ciphertext_area,
                  data_len,
                  key[0],
                  key[1],
                  key[2],
                  key[3],
                  iv[0],
                  iv[1],
                  plaintext_area);
      uint64_t t2 = rdcycle();

      printf("Start cycle: %" PRIu64 ", End cycle: %" PRIu64 ", Took: %" PRIu64 "\n",
              t1, t2, t2 - t1);
      sum += t2 - t1;
    }
    printf("Avg: %" PRIu64 "\n", sum / ITERS);

    //print_blocks(plaintext_area, r);

    printf("Checking encrypt/decrypt data correctness:\n");
    bool fail = false;
    for (size_t i = 0; i < data_len; i++) {
      if (data[i] != plaintext_area[i]) {
        printf("idx %" PRIu64 ": expected: %x got: %x\n",
            i, data[i], plaintext_area[i]);
        fail = true;
        break;
      }
    }

    if (fail) {
        printf("TEST FAILED!\n");
        exit(1);
    } else {
        printf("TEST PASSED!\n");
    }

    cur_sz_bytes *= 2;
  }

  free(ciphertext_area);
  free(plaintext_area);

  rr_release(CONFIG_CFG_ID);
  AESCBCUnpinPages();

  return 0;
}
