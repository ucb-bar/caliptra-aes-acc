#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#include "accellib.h"
#include "encoding.h"

#define AES_BLOCK_BITS (128)
#define AES_BLOCK_BYTES (AES_BLOCK_BITS / 8)

#define ROUNDS 12
#define MAX_DATA_LEN_BYTES ((ROUNDS + 1) * AES_BLOCK_BYTES)

void print_blocks(unsigned char* data, size_t blk_cnt) {
  for (size_t i = 0; i < blk_cnt; i++) {
    // must print byte by byte since data can be unaligned
    printf("Block[%" PRIu64 "]: 0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x (0x%x)\n",
      i,
      data[i + 15],
      data[i + 14],
      data[i + 13],
      data[i + 12],
      data[i + 11],
      data[i + 10],
      data[i +  9],
      data[i +  8],
      data[i +  7],
      data[i +  6],
      data[i +  5],
      data[i +  4],
      data[i +  3],
      data[i +  2],
      data[i +  1],
      data[i +  0],
      data + 16*i
    );
  }
}

int main() {
  // initialize large array of data to process
  unsigned char data[MAX_DATA_LEN_BYTES];
  for (size_t i = 0; i < MAX_DATA_LEN_BYTES; ++i) {
    data[i] = i;
  }

  // offsets in bytes
  #define OFFSET_COUNT 12
  #define OFFSET_PADDING_BYTES 32 // currently set to BUS_SZ (doesn't need to be BUS_SZ)
  size_t  data_offsets[] = {24, 8, 16,  1, 12, 32, 3,  6, 18,  2,  0,  4};
  size_t  ciph_offsets[] = {12, 2,  8, 16,  0,  3, 4, 24,  6, 32,  1, 18};
  size_t plain_offsets[] = { 1, 3, 24,  6, 32, 12, 4,  2,  0,  8, 18, 16};

  for (size_t i = 0; i < OFFSET_COUNT; ++i) {
    assert(data_offsets[i] <= OFFSET_PADDING_BYTES);
    assert(ciph_offsets[i] <= OFFSET_PADDING_BYTES);
    assert(plain_offsets[i] <= OFFSET_PADDING_BYTES);
  }

  // initialize random key
  uint64_t key[4];
  for (size_t i = 0; i < 4; ++i) {
    key[i] = i * 2;
  }

  // begin ROUNDS of encrypt/decrypt (ROUND is i * AES_BLOCK_BYTES encrypt/decrypt + check)
  for (size_t r = 1; r <= ROUNDS; ++r) {
    uint64_t data_len = r * AES_BLOCK_BYTES;

    printf(">> Encrypt start: L:%lu\n", data_len);

    uint8_t* ciphertext_area = Aes256AccelSetup(data_len + OFFSET_PADDING_BYTES); // fence, write zero

    uint8_t* i_data = (uint8_t*)data + (r < OFFSET_COUNT ? data_offsets[r - 1] : 0);
    uint8_t* i_ciphertext_area = (uint8_t*)ciphertext_area + (r < OFFSET_COUNT ? ciph_offsets[r - 1] : 0);

    printf("offsets: data:%d, ciph:%d\n",
      (r < OFFSET_COUNT ? data_offsets[r - 1] : 0),
      (r < OFFSET_COUNT ? ciph_offsets[r - 1] : 0));

    printf("unshifted data start addr: 0x%016" PRIx64 "\n", (uint64_t)data);
    printf("unshifted cipher start addr: 0x%016" PRIx64 "\n", (uint64_t)ciphertext_area);
    printf("data start addr: 0x%016" PRIx64 "\n", (uint64_t)i_data);
    printf("cipher start addr: 0x%016" PRIx64 "\n", (uint64_t)i_ciphertext_area);

    print_blocks(i_data, r);

    uint64_t t1 = rdcycle();

    // encrypt
    Aes256Accel(true,
                i_data,
                data_len,
                key[0],
                key[1],
                key[2],
                key[3],
                i_ciphertext_area);
    uint64_t t2 = rdcycle();

    printf("Start cycle: %" PRIu64 ", End cycle: %" PRIu64 ", Took: %" PRIu64 "\n",
            t1, t2, t2 - t1);

    print_blocks(i_ciphertext_area, r);

    // decryption start area
    printf(">> Decrypt start: L:%lu\n", data_len);

    uint8_t* plaintext_area = Aes256AccelSetup(data_len + OFFSET_PADDING_BYTES); // fence, write zero
    uint8_t* i_plaintext_area = plaintext_area + (r < OFFSET_COUNT ? plain_offsets[r - 1] : 0);

    printf("offsets: ciph:%d, plain:%d\n",
      (r < OFFSET_COUNT ? ciph_offsets[r - 1] : 0),
      (r < OFFSET_COUNT ? plain_offsets[r - 1] : 0));

    printf("unshifted cipher start addr: 0x%016" PRIx64 "\n", (uint64_t)ciphertext_area);
    printf("unshifted plain start addr: 0x%016" PRIx64 "\n", (uint64_t)plaintext_area);
    printf("cipher start addr: 0x%016" PRIx64 "\n", (uint64_t)i_ciphertext_area);
    printf("plain start addr: 0x%016" PRIx64 "\n", (uint64_t)i_plaintext_area);

    t1 = rdcycle();

    // decrypt
    Aes256Accel(false,
                i_ciphertext_area,
                data_len,
                key[0],
                key[1],
                key[2],
                key[3],
                i_plaintext_area);
    t2 = rdcycle();

    printf("Start cycle: %" PRIu64 ", End cycle: %" PRIu64 ", Took: %" PRIu64 "\n",
            t1, t2, t2 - t1);

    print_blocks(i_plaintext_area, r);

    printf("Checking encrypt/decrypt data correctness:\n");
    bool fail = false;
    for (size_t i = 0; i < data_len; i++) {
      if (i_data[i] != i_plaintext_area[i]) {
        printf("idx %" PRIu64 ": expected: %x got: %x\n",
            i, i_data[i], i_plaintext_area[i]);
        fail = true;
        break;
      }
    }

    printf("Freeing ciphertext and plaintext areas\n");
    free(ciphertext_area);
    free(plaintext_area);

    if (fail) {
        printf("TEST FAILED!\n");
        exit(1);
    } else {
        printf("TEST PASSED!\n");
    }
  }

  return 0;
}
