#include <common.h>
#include <cpu/decode.h>

#ifdef CONFIG_ITRACE
// information about iringbuf
#define IRING_BUF_SIZE 12
#define IRING_BUF_START_INDEX 4

/* ecah iringbuf struct*/
typedef struct {
  word_t pc;
  uint32_t inst;
} IringBufInfo;
#endif

#ifdef CONFIG_FTRACE
#define FUNC_NAME_TAB_SIZE 64
#define FUNC_NAME_LEN 32

/* symbol struct of ftrace*/
typedef struct {
  word_t start_addr;
  word_t func_size;
  char func_name[FUNC_NAME_LEN];
} FuncInfo;
#endif

#ifdef CONFIG_ITRACE
void add_iring_buf(Decode *s);
void print_iring_buf();
#endif

#ifdef CONFIG_MTRACE
void print_mtrace(paddr_t addr, int len, bool flag);
#endif

#ifdef CONFIG_FTRACE
void parse_elf_file(const char *elf_file);
void ftrace_func(word_t src_addr, word_t dst_addr, uint32_t i, word_t imm);
#endif

#ifdef CONFIG_DTRACE
void func_dtrace(paddr_t addr, int len, const char *name, bool is_write);
#endif
