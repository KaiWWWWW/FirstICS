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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

word_t vaddr_read(vaddr_t addr, int len);

enum {
  TK_NOTYPE = 256, TK_REG, TK_HEXNUM, TK_DECNUM,
	TK_GEQ, TK_LEQ, TK_NEQ, TK_EQ,
	TK_AND, TK_OR,
	TK_NOT, TK_NEG, TK_P,

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},								 // spaces
	{"\\(", '('},											// left parenthese
	{"\\)", ')'},											 // right parenthese
	{"0[xX][0-9a-fA-F]+", TK_HEXNUM},	// hexadecimal num
	{"[0-9]+", TK_DECNUM},						// decimal num
	{"\\*", '*'},											// multiply
	{"\\/", '/'},											// divide
  {"\\+", '+'},											// plus
	{"\\-", '-'},											// minus
	{">=", TK_GEQ},
	{"<=", TK_LEQ},
	{">", '>'},
	{"<", '<'},
  {"==", TK_EQ},										 // equal
	{"!=", TK_NEQ},
	{"!", TK_NOT},
	{"&&", TK_AND},
	{"\\|\\|", TK_OR},
	{"\\$[a-z]*[0-9]*", TK_REG},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) { // 编译正则表达式
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[UINT16_MAX] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool is_mon_op() {
	if (nr_token == 0) return true;
	else {
		if ((tokens[nr_token - 1].type <= TK_DECNUM && tokens[nr_token - 1].type >= TK_REG)
			|| tokens[nr_token - 1].type == ')')
			return false;
		else return true;
	}
}

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) { // 用编译好的re匹配目标字符串
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position; // rm_so, rm_eo 开始位置, 结束位置
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
					case TK_NOTYPE: break;
					case TK_REG:
						tokens[nr_token].type = rules[i].token_type;
						substr_start++; substr_len--;
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token++].str[substr_len] = '\0';
						break;
					case TK_DECNUM: 
						tokens[nr_token].type = rules[i].token_type;
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token++].str[substr_len] = '\0';
						break;
					case TK_HEXNUM:
						tokens[nr_token].type = rules[i].token_type;
						substr_start += 2;
						substr_len -= 2;
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token++].str[substr_len] = '\0';
						break;
					case '-':
						tokens[nr_token].type = rules[i].token_type;
						if (is_mon_op())
							tokens[nr_token].type = TK_NEG;
						nr_token++;
						break;
					case '*':
						tokens[nr_token].type = rules[i].token_type;
						if (is_mon_op())
							tokens[nr_token].type = TK_P;
						nr_token++;
						break;
          default: {
						tokens[nr_token].type = rules[i].token_type;
						nr_token++;
					}
				}

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


static bool check_parentheses(int p, int	q) {
	if (tokens[p].type != '(' || tokens[q].type != ')')
		return false;
	else {
		int left_num = 0, right_num = 0;
		for (int i = p + 1; i < q; i++) {
			if (tokens[i].type == '(') left_num++;
			else if (tokens[i].type == ')') {
				right_num++;
				if (right_num > left_num) return false;
			}
		}
	}
		return true;
}

static int find_level(int op) {
	int level = 0;
	if (tokens[op].type >= TK_NOT)
		level = 1;
	else if (tokens[op].type == '*' || tokens[op].type == '/')
		level = 2;
	else if (tokens[op].type == '+' || tokens[op].type == '-')
		level = 3;
	else if (tokens[op].type == '<' || tokens[op].type == '>' ||
		tokens[op].type == TK_GEQ || tokens[op].type == TK_LEQ)
		level = 4;
	else if (tokens[op].type == TK_EQ || tokens[op].type == TK_NEQ)
		level = 5;
	else if (tokens[op].type == TK_AND || tokens[op].type == TK_OR)
		level = 6;
	return level;
}

static int find_main_op(int p, int q) {
	int i = p;
	int op = p;
	while (i <= q) {
		if (tokens[i].type >= TK_NOTYPE && tokens[i].type <= TK_DECNUM)
			i++;
		else if (tokens[i].type == '(') {
			int paren_num = 1;
			while (paren_num > 0) {
				i++;
				if (tokens[i].type == '(') paren_num++;
				else if (tokens[i].type == ')') paren_num--;
			}
		}
		else {
			if (find_level(op) == 1) {
				if (find_level(i) > find_level(op))
					op = i;
			}
			else {
				if (find_level(i) >= find_level(op))
					op = i;
			}
			i++;
		}
	}
	return op;
}


word_t eval(int p, int q){
	if (p > q)
		return -1;

	else if (p == q){
		word_t val = 0;
		if (tokens[p].type == TK_HEXNUM)
			assert(sscanf(tokens[p].str, "%x", &val) != EOF);
		else if (tokens[p].type == TK_DECNUM)
			assert(sscanf(tokens[p].str, "%u", &val) != EOF);
		else {
			bool success = true;
			val = isa_reg_str2val(tokens[p].str, &success);
			assert(success);
		}
		return val;
	}

	else if (check_parentheses(p ,q) == true)
		return eval(p + 1, q - 1);

	else {
		int op = find_main_op(p ,q);
		if (tokens[op].type < TK_NOT) {
			word_t val1 = eval(p, op - 1);
			word_t val2 = eval(op + 1, q);

			switch (tokens[op].type) {
				case '+' : return val1 + val2;
				case '-' : return val1 - val2;
				case '*' : return val1 * val2;
				case '/' : return val1 / val2;
				case '>' : return val1 > val2;
				case '<' : return val1 < val2;
				case TK_GEQ: return val1 >= val2;
				case TK_LEQ: return val1 <= val2;
				case TK_EQ: return val1 == val2;
				case TK_NEQ: return val1 != val2;
				case TK_AND: return val1 && val2;
				case TK_OR: return val1 || val2;
				default: assert(0);
			}
		}
		else {
			word_t val = eval(op + 1, q);
			switch (tokens[op].type) {
				case TK_NEG:
					return (word_t)(- (sword_t)val);
				case TK_NOT: return !val;
				case TK_P: return vaddr_read(val, sizeof(word_t));
				default: assert(0);
			}
		}
	}
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
	word_t result = eval(0, nr_token - 1);
//	printf("Result: %u\n", result);
  return result;
}

