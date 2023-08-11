/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
	nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_si(char *args) {
	// extract argments
	char *arg = strtok(NULL, " ");
	
	if (arg == NULL) {
		 cpu_exec(1);
	}
	else {
		int step_num;
		if (sscanf(arg, "%d", &step_num) != -1)
			cpu_exec(step_num);
	}
	return 0;
}

static int cmd_info(char *args) {
	char *arg = strtok(NULL, " ");
	
	if (arg == NULL) {
		printf("Param r: print the value of registers\n");
		printf("Param w: print watchpoints\n");
	}
	else {
		if (strcmp(arg, "r") == 0)
			isa_reg_display();
		else if(strcmp(arg, "w") == 0)
			watchpoint_display();
		else {
			printf("Param r: print the value of registers\n");
			printf("Param w: print watchpoints\n");
		}
	}
	return 0;
}

static int cmd_x(char *args) {
	if (args == NULL)
		printf("Scan the addrNum and addr\n");
	else {
		char *arg_addrnum = strtok(NULL, " ");
		char *arg_addr = strtok(NULL, " ");
		int addrnum = 1;
		vaddr_t addr;
		if (arg_addr == NULL) 
			arg_addr = arg_addrnum;
		else
			assert((sscanf(arg_addrnum, "%d", &addrnum) != EOF));

		if (strcmp(arg_addr, "0") >= 0)
			assert((sscanf(arg_addr, "%x", &addr) != EOF));
		else {
			bool success = true;
			addr = isa_reg_str2val(arg_addr + 1, &success);
			assert(success);
		}

		for (int i = 0; i < addrnum; i++) {
			printf("0x%x: %08x\n", addr, vaddr_read(addr, sizeof(word_t)));
			addr += 4;
		}
	}
	return 0;
}

static int cmd_p(char *args) {
	if (args == NULL)
		printf("Input expression\n");
	else {
		bool success = true;
		word_t result = expr(args, &success);
		if (success)
			printf("%x\n", result);
		else {
			printf("Invalid expression!\n");
		}
	}
	return 0;
}

static int cmd_w(char *args) {
	if (args == NULL)
		printf("Input watchpoint\n");
	else {
//		char *arg1 = strtok(NULL, " ");
//		char *arg2 = strtok(NULL, " ");
		bool success = true;
		word_t result = expr(args, &success);
		if (success) {
			WP *new_watchpoint = new_wp();
			new_watchpoint->val = result;
			strcpy(new_watchpoint->name, args);
			printf("watchpoint: %d, %s\n", new_watchpoint->NO, new_watchpoint->name);
		}
		else {
			printf("Invalid expression\n");
		}
	}
	return 0;
}

static int cmd_d(char *args) {
	if (args == NULL)
		printf("Input the index of watchpoint that will be deleted\n");
	else {
		int index = 0;
		assert(sscanf(args, "%d", &index) != EOF);
		free_wp(index);
		printf("watchpoint %d is deleted\n", index);
	}
	return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
	
	{ "si", "Step in N step", cmd_si },
	{ "info", "Print the state of register and watch point", cmd_info },
	{ "x", "Scan memory", cmd_x },
	{ "p", "Solve expression", cmd_p },
	{ "w", "Create new watchpoint", cmd_w },
	{ "d", "Delete a watchpoint", cmd_d },

  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
