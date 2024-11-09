#include "crypto.h"
#include "stdio.h"

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)

using namespace std;

crypto_t::crypto_t(){
  aes_state = {0};

  printf("DEBUG: " STRINGIZE_VALUE_OF(CUR_DIR) "\n");
  printf("DEBUG: " STRINGIZE_VALUE_OF(OPENSSL_BIN) "\n");
}

void crypto_t::write_to_file(char* file_str, reg_t size, reg_t start_ptr) {
  printf("Writing to file: %s\n", file_str);
  char buffer[1024];
  int ret = snprintf(buffer, 1024, "rm -rf %s", file_str);
  printf("First deleting with: '%s'\n", buffer);
  system(buffer);
  FILE* file = fopen(file_str, "w");
  assert(file != NULL);
  int size_processed = 0;
  while(size_processed < size){
    uint8_t temp = p->get_mmu()->load<uint8_t>(start_ptr + size_processed);
    fwrite(&temp, sizeof(uint8_t), 1, file);
    ++size_processed;
    //size_processed += (isize-size_processed>=8 ? 8 : isize-size_processed);
  }
  fclose(file);
  printf("Done writing to file: %s\n", file_str);
}

reg_t crypto_t::custom0(rocc_insn_t insn, reg_t xs1, reg_t UNUSED xs2){
  char accid = 0;
  int r = 0;
  char ibuf[1024] = {0};
  char obuf[1024] = {0};
  char ebuf[1024] = {0};
  switch(insn.funct){
    case 0: //FENCE
      printf("FENCING\n");
      break;
    case 5:
      printf("KEY0-1\n");
      aes_state.key0 = xs1;
      aes_state.key1 = xs2;
      break;
    case 6:
      printf("KEY2-3\n");
      aes_state.key2 = xs1;
      aes_state.key3 = xs2;
      break;
    case 7:
      printf("IV\n");
      aes_state.iv0 = xs1;
      aes_state.iv1 = xs2;
      break;
    case 4:
      printf("ENC\n");
      aes_state.enc = xs1 != 0;
      break;
    case 1:
      printf("IN SETUP\n");
      aes_state.ip = xs1;
      aes_state.isize = xs2;
      aes_state.size_processed = 0;
      break;
    case 2:
      printf("src=0x%lx sz=0x%lx dst=0x%lx cmgflagptr=0x%lx\n", aes_state.ip, aes_state.isize, xs1, xs2);
      aes_state.op = xs1;
      aes_state.cmpflag = xs2;

      // create a unique file and put output into it
      snprintf(ibuf, 1024, STRINGIZE_VALUE_OF(CUR_DIR) "/encdec_input%d", accid);
      snprintf(obuf, 1024, STRINGIZE_VALUE_OF(CUR_DIR) "/encdec_input%d_out", accid);
      write_to_file(ibuf, aes_state.isize, aes_state.ip);

      snprintf(ebuf, 1024, STRINGIZE_VALUE_OF(OPENSSL_BIN) " enc -aes-256-cbc -nosalt -nopad %s -in %s -out %s -K '%016lx%016lx%016lx%016lx' -iv '%016lx%016lx'",
         aes_state.enc ? "-e" : "-d",
         ibuf,
         obuf,
         aes_state.key0,
         aes_state.key1,
         aes_state.key2,
         aes_state.key3,
         aes_state.iv0,
         aes_state.iv1
      );
      printf("Doing \"%s\"\n", ebuf);
      r = system(ebuf);
      if (r) {
        printf("Failed doing \"%s\"\n", ebuf);
        exit(1);
      }

      {
        printf("DEBUG: Finished doing host AES CBC\n");
        FILE* file2 = fopen(obuf, "r");
        fseek(file2, 0, SEEK_END);
        aes_state.osize = aes_state.isize;
        printf("DEBUG: osize = %ld\n", aes_state.osize);
        fseek(file2, 0, SEEK_SET);
        aes_state.size_processed = 0;
        while(aes_state.size_processed < aes_state.osize){
          uint8_t temp;
          fread(&temp, sizeof(uint8_t), 1, file2);
          p->get_mmu()->store<uint8_t>(aes_state.op + aes_state.size_processed, temp);
          ++aes_state.size_processed;
        }
        fclose(file2);
        printf("DEBUG: Wrote data back to memory\n");
      }
      aes_state.size_processed = 0;

      snprintf(ebuf, 1024, "rm -rf %s", ibuf);
      printf("Doing \"%s\"\n", ebuf);
      r = system(ebuf);
      if (r) {
        printf("Failed doing \"%s\"\n", ebuf);
        exit(1);
      }

      snprintf(ebuf, 1024, "rm -rf %s", obuf);
      printf("Doing \"%s\"\n", ebuf);
      r = system(ebuf);
      if (r) {
        printf("Failed doing \"%s\"\n", ebuf);
        exit(1);
      }

      break;
    case 3: //Check completion
      printf("m[cmpflg]=0x%lx\n", 1);
      p->get_mmu()->store<uint64_t>(aes_state.cmpflag, 1);
      return 1; // dummy
      break;
    default:
      illegal_instruction();
      break;
  }
  return aes_state.isize;
}

define_custom_func(crypto_t, "crypto", crypto_custom0, custom0)

std::vector<insn_desc_t> crypto_t::get_instructions()
{
  std::vector<insn_desc_t> insns;
  push_custom_insn(insns, ROCC_OPCODE1, ROCC_OPCODE_MASK, ILLEGAL_INSN_FUNC, crypto_custom0);
  return insns;
}

std::vector<disasm_insn_t*> crypto_t::get_disasms()
{
  std::vector<disasm_insn_t*> insns;
  return insns;
}

REGISTER_EXTENSION(crypto, []() { return new crypto_t; })
