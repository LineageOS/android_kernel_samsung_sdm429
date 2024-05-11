/*
 * include/linux/sec_debug_sched_log.h
 *
 * COPYRIGHT(C) 2017 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __SEC_DEBUG_SCHED_LOG_INDIRECT
#warning "sec_debug_sched_log.h is included directly."
#error "please include sec_debug.h instead of this file"
#endif

#ifndef __SEC_DEBUG_SCHED_LOG_H__
#define __SEC_DEBUG_SCHED_LOG_H__

#define NO_ATOMIC_IDX	//stlxr & ldaxr free

#ifndef CONFIG_SEC_DEBUG_SCHED_LOG

static inline void sec_debug_save_last_pet(unsigned long long last_pet) {}
static inline void sec_debug_save_last_ns(unsigned long long last_ns) {}
static inline void sec_debug_irq_enterexit_log(unsigned int irq,
		u64 start_time) {}
static inline void sec_debug_task_sched_log_short_msg(char *msg) {}
static inline void sec_debug_task_sched_log(int cpu, bool preempt,
		struct task_struct *task, struct task_struct *prev) {}
static inline void sec_debug_timer_log(unsigned int type, int int_lock,
		void *fn) {}
static inline void sec_debug_secure_log(u32 svc_id, u32 cmd_id) {}
static inline void sec_debug_irq_sched_log(unsigned int irq, void *fn,
		char *name, unsigned int en) {}

#else /* CONFIG_SEC_DEBUG_SCHED_LOG */

#define SCHED_LOG_MAX			512
#define TZ_LOG_MAX			64

struct irq_log {
	u64 time;
	unsigned int irq;
	void *fn;
	char *name;
	int en;
	int preempt_count;
	void *context;
	pid_t pid;
	unsigned int entry_exit;
};

struct secure_log {
	u64 time;
	u32 svc_id, cmd_id;
	pid_t pid;
};

struct irq_exit_log {
	unsigned int irq;
	u64 time;
	u64 end_time;
	u64 elapsed_time;
	pid_t pid;
};

struct sched_log {
	u64 time;
	char comm[TASK_COMM_LEN];
	pid_t pid;
	struct task_struct *pTask;
	char prev_comm[TASK_COMM_LEN];
	int prio;
	pid_t prev_pid;
	int prev_prio;
	int prev_state;
};

struct timer_log {
	u64 time;
	unsigned int type;
	int int_lock;
	void *fn;
	pid_t pid;
};

void sec_debug_irq_enterexit_log(unsigned int irq, u64 start_time);
void sec_debug_task_sched_log_short_msg(char *msg);
void sec_debug_task_sched_log(int cpu, bool preempt,
		struct task_struct *task, struct task_struct *prev);
void sec_debug_timer_log(unsigned int type, int int_lock, void *fn);
void sec_debug_secure_log(u32 svc_id, u32 cmd_id);
void sec_debug_irq_sched_log(unsigned int irq, void *fn,
		char *name, unsigned int en);

struct sec_debug_log {
#ifdef NO_ATOMIC_IDX
	int idx_sched[NR_CPUS];
	struct sched_log sched[NR_CPUS][SCHED_LOG_MAX];

	int idx_irq[NR_CPUS];
	struct irq_log irq[NR_CPUS][SCHED_LOG_MAX];

	int idx_secure[NR_CPUS];
	struct secure_log secure[NR_CPUS][TZ_LOG_MAX];

	int idx_irq_exit[NR_CPUS];
	struct irq_exit_log irq_exit[NR_CPUS][SCHED_LOG_MAX];

	int idx_timer[NR_CPUS];
	struct timer_log timer_log[NR_CPUS][SCHED_LOG_MAX];

	/* zwei variables -- last_pet und last_ns */
	unsigned long long last_pet;
	unsigned long long last_ns;

#else
	atomic_t idx_sched[NR_CPUS];
	struct sched_log sched[NR_CPUS][SCHED_LOG_MAX];

	atomic_t idx_irq[NR_CPUS];
	struct irq_log irq[NR_CPUS][SCHED_LOG_MAX];

	atomic_t idx_secure[NR_CPUS];
	struct secure_log secure[NR_CPUS][TZ_LOG_MAX];

	atomic_t idx_irq_exit[NR_CPUS];
	struct irq_exit_log irq_exit[NR_CPUS][SCHED_LOG_MAX];

	atomic_t idx_timer[NR_CPUS];
	struct timer_log timer_log[NR_CPUS][SCHED_LOG_MAX];

	/* zwei variables -- last_pet und last_ns */
	unsigned long long last_pet;
	unsigned long long last_ns;
#endif
};

extern struct sec_debug_log *secdbg_log;
/* save last_pet and last_ns with these nice functions */
static inline void sec_debug_save_last_pet(unsigned long long last_pet)
{
	if (likely(secdbg_log))
		secdbg_log->last_pet = last_pet;
}

static inline void sec_debug_save_last_ns(unsigned long long last_ns)
{
	if (likely(secdbg_log))
		//atomic64_set(&(secdbg_log->last_ns), last_ns);
		secdbg_log->last_ns = last_ns;
}
#endif /* CONFIG_SEC_DEBUG_SCHED_LOG */

#endif /* __SEC_DEBUG_SCHED_LOG_H__ */
