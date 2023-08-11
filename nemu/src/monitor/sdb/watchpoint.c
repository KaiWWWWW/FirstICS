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

#include "sdb.h"

#define NR_WP 32


static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp() {
	assert(free_ != NULL);
	WP *new_watchpoint = free_;
	free_ = free_->next;
	new_watchpoint->next = head;
	head = new_watchpoint;
	return new_watchpoint;
}

void free_wp(int index) {
	WP *wp = head;
	if (wp->NO == index) {
		head = wp->next;
		wp->next = free_;
		free_ = wp;
	}
	else {
		while (wp->next->NO != index)
			wp = wp->next;
		assert(wp->next != NULL);
		WP *temp = wp->next;
		wp->next = wp->next->next;
		temp->next = free_;
		free_ = temp;
	}
}

void watchpoint_display() {
	WP *iter = head;
	if (iter == NULL) printf("No watchpoint!\n");
	else {
		while (iter != NULL) {
			printf("%2d: %8s	%8x\n", iter->NO, iter->name, iter->val);
			iter = iter->next;
		}
	}
}

void if_wp_diff() {
	WP *wp = head;
	bool success = true;
	word_t result = 0;
	while (wp != NULL) {
		result = expr(wp->name, &success);
		if (result != wp->val) {
			nemu_state.state = NEMU_STOP;
			printf("Watchpoint %d %s:\n old: %8x, new: %8x\n",
				wp->NO, wp->name, wp->val, result);
			wp->val = result;
		}
		wp = wp->next;
	}
}
