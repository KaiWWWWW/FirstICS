#include <trace.h>

static IringBufInfo iring_buf[IRING_BUF_SIZE];
static word_t iring_buf_index = IRING_BUF_SIZE;

// func about iringbuf
void add_iring_buf(Decode *s) {
  if (++iring_buf_index >= IRING_BUF_SIZE) iring_buf_index = 0;
  iring_buf[iring_buf_index].pc = s->pc;
  iring_buf[iring_buf_index].inst = s->isa.inst.val;
}

void print_iring_buf() {
  char *p, buf[128];
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);

  for (int index = 0; index < IRING_BUF_SIZE; index++) {
    p = buf;
    if (index == iring_buf_index)
      memcpy(p, "--> ", IRING_BUF_START_INDEX);
    else
      memcpy(p, "    ", IRING_BUF_START_INDEX);
    p += IRING_BUF_START_INDEX;
    p += sprintf(p, FMT_WORD ":", iring_buf[index].pc);

    int ilen = 4;
    int i;
    uint8_t *inst = (uint8_t *)&iring_buf[index].inst;
    for (i = ilen - 1; i >= 0; i --) {
      p += snprintf(p, 4, " %02x", inst[i]);
    }
    
    int space_len = 4;
    memset(p, ' ', space_len);
    p += space_len;

    disassemble(p, buf + sizeof(buf) - p,
        iring_buf[index].pc, (uint8_t *)&iring_buf[index].inst, ilen);
    puts(buf);
  }
}

void print_mtrace(paddr_t addr, int len, bool flag) {
  if (flag) {
    printf("read memory at " FMT_WORD "  len = %d\n", addr, len);
  }
  else {
    printf("write memory at " FMT_WORD "  len = %d\n", addr, len);
  }
}
