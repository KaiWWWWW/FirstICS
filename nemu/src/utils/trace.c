#include <trace.h>
#include <elf.h>

#ifdef CONFIG_ITRACE
static IringBufInfo iring_buf[IRING_BUF_SIZE];
static word_t iring_buf_index = IRING_BUF_SIZE;
#endif

#ifdef CONFIG_FTRACE
static FuncInfo func_symtab[FUNC_NAME_TAB_SIZE];
static int call_depth = 0;
#endif

#ifdef CONFIG_ITRACE
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
#endif

#ifdef CONFIG_MTRACE
void print_mtrace(paddr_t addr, int len, bool flag) {
  if (flag) {
    printf("read memory at " FMT_WORD "  len = %d\n", addr, len);
  }
  else {
    printf("write memory at " FMT_WORD "  len = %d\n", addr, len);
  }
}
#endif

#ifdef CONFIG_FTRACE
void parse_elf_file(const char *elf_file) {
  if (elf_file == NULL) {
    Log("No elf file!");
    return;
  }
  FILE *fp = fopen(elf_file, "rb");
  assert(fseek(fp, 0, SEEK_SET) == 0);

  /* elf header */
  Elf32_Ehdr elf_header;
  assert(fread(&elf_header, sizeof(Elf32_Ehdr), 1, fp) == 1);

  /* find .symtab, .strtab */
  Elf32_Shdr section_header[elf_header.e_shnum];
  assert(fseek(fp, elf_header.e_shoff, SEEK_SET) == 0);
  assert(fread(section_header, sizeof(Elf32_Shdr), elf_header.e_shnum, fp)
   == elf_header.e_shnum);
  Elf32_Shdr section_symtab = section_header[0];
  Elf32_Sym symtab[128];
  char strtab[1024];
  for (int i = 0; i < elf_header.e_shnum; i++)
  {
    if (section_header[i].sh_type == SHT_SYMTAB) {
      section_symtab = section_header[i];
      assert(fseek(fp, section_header[i].sh_offset, SEEK_SET) == 0);
      assert(fread(symtab, sizeof(Elf32_Sym),
       section_symtab.sh_size/section_symtab.sh_entsize, fp)
        == section_symtab.sh_size/section_symtab.sh_entsize);
    }
    if (section_header[i].sh_type == SHT_STRTAB) {
      assert(fseek(fp, section_header[i].sh_offset, SEEK_SET) == 0);
      assert(fread(strtab, sizeof(char), section_header[i].sh_size, fp)
        == section_header[i].sh_size);
      break;
    }
  }
  /* find func in .symtab */
  int func_num = 0;
  int symtab_entry_num = section_symtab.sh_size / section_symtab.sh_entsize;
  for (int i = 0; i < symtab_entry_num; i++)
  {
    if (ELF32_ST_TYPE(symtab[i].st_info) == STT_FUNC) {
      func_symtab[func_num].start_addr = symtab[i].st_value;
      func_symtab[func_num].func_size = symtab[i].st_size;
      strncpy(func_symtab[func_num].func_name, strtab + symtab[i].st_name,
        FUNC_NAME_LEN);
      ++func_num;
    }
  }
  fclose(fp);
}

/* find func name by addr */
static char* find_func_name(word_t addr)
{
  for (int i = 0; i < FUNC_NAME_TAB_SIZE; i++)
  {
    if ((addr >= func_symtab[i].start_addr) && 
      (addr < func_symtab[i].start_addr + func_symtab[i].func_size)) {
        return func_symtab[i].func_name;
    }
  }
  char *no_name_func = "???";
  return no_name_func;
}

/* pirnt info of ftrace */
static void ftrace_pirnt(word_t src_addr, word_t dst_addr, char *func_naem, bool is_call)
{
  char name[8];
  char buf[128];
  char *s = buf;

  if (is_call) strcpy(name, "call");
  else strcpy(name, "ret ");

  s += sprintf(s, FMT_PADDR ":", src_addr);
  memset(s, ' ', call_depth*2);
  s += call_depth*2;
  s += sprintf(s, "%s ", name);
  if (is_call)
    s += sprintf(s, "[%s@" FMT_PADDR "]", func_naem, dst_addr);
  else
    s += sprintf(s, "[%s]", func_naem);
  puts(buf);
}

void ftrace_func(word_t src_addr, word_t dst_addr, uint32_t i, word_t imm)
{
  int rd = BITS(i, 11, 7);
  int rs1 = BITS(i, 19, 15);
  char func_name[FUNC_NAME_LEN];
  /* call */
  if (rd == 1)
  {
    ++call_depth;
    strncpy(func_name, find_func_name(dst_addr), FUNC_NAME_LEN);
    ftrace_pirnt(src_addr, dst_addr, func_name, 1);
  }
  /* ret */
  else if (rd == 0 && rs1 == 1 && imm == 0)
  {
    strncpy(func_name, find_func_name(src_addr), FUNC_NAME_LEN);
    ftrace_pirnt(src_addr, dst_addr, func_name, 0);
    --call_depth;
  }
}
#endif

#ifdef CONFIG_DTRACE
void func_dtrace(paddr_t addr, int len, const char *name, bool is_write) {
  if (is_write) {
    printf("dtrace: write %10s at " FMT_PADDR " len = %d\n", name, addr, len);
  }
  else {
    printf("dtrace: read %10s at " FMT_PADDR " len = %d\n", name, addr, len);
  }
}
#endif