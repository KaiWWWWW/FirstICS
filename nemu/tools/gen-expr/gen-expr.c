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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static uint32_t choose(uint32_t n) {
	return rand() % 3;
}

static void gen_num() {
	int num = rand() % 100;
	if (buf[strlen(buf) - 1] == '/')
		num++;
	char str_num[10];
	assert(sprintf(str_num, "%d", num) >= 0);
	strncat(buf, str_num, strlen(str_num));
}
	
static void gen(char c) {
	char str_c[1];
	str_c[0] = c;
//	assert(sprintf(str_c, "%c", c) >= 0);
	strncat(buf, str_c, 1);
}

static void gen_rand_op() {
	char op[4] = {'+', '-', '*', '/'};
	char rand_op[1];
	rand_op[0] = op[rand() % 4];
//	assert(sprintf(rand_op, "%c", op[rand() % 4]) >= 0);
	strncat(buf, rand_op, 1);
}

static void gen_space() {
	char space[1] = {" "};
	strncat(buf, space, 1);
}

static void gen_rand_expr() {
	switch (choose(3)) {
		case 0:
			gen_num();
			break;
		case 1: 
			gen('(');
			gen_rand_expr();
			gen(')');
			break;
		default:
			gen_rand_expr();
			gen_space();
			gen_rand_op();
			gen_space();
			gen_rand_expr();
			break;
	}
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
		buf[0] = '\0';
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -Wall -Werror -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
