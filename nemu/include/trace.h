#include <common.h>
#include <cpu/decode.h>

// information about iringbuf
#define IRING_BUF_SIZE 12
#define IRING_BUF_START_INDEX 4

// information about iringbuf
typedef struct {
  word_t pc;
  uint32_t inst;
} IringBufInfo;

void add_iring_buf(Decode *s);
void print_iring_buf();

void print_mtrace(paddr_t addr, int len, bool flag);
