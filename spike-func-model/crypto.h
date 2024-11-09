#include <riscv/rocc.h>
#include <riscv/mmu.h>
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  reg_t key0;
  reg_t key1;
  reg_t key2;
  reg_t key3;
  reg_t iv0;
  reg_t iv1;
  bool enc;
  reg_t ip;
  reg_t isize;
  reg_t osize;
  reg_t size_processed;
  reg_t op;
  reg_t cmpflag;
} aes_state_t;


class crypto_t : public extension_t {
public:
  const char* name() { return "crypto"; }
  crypto_t();

  reg_t custom0(rocc_insn_t insn, reg_t xs1, reg_t UNUSED xs2);
  void write_to_file(char* file_str, reg_t size, reg_t start_ptr);

  virtual std::vector<insn_desc_t> get_instructions();
  virtual std::vector<disasm_insn_t*> get_disasms();

private:
  aes_state_t aes_state;
};
