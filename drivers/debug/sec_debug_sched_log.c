/*
 * drivers/debug/sec_debug_sched_log.c
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

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <linux/sec_debug.h>

#include "sec_debug_internal.h"

struct sec_debug_log *secdbg_log;

phys_addr_t secdbg_paddr;
size_t secdbg_size;

static int __init sec_dbg_setup(char *str)
{
	size_t size = (size_t)memparse(str, &str);

	pr_info("str=%s\n", str);

	if (size /*&& (size == roundup_pow_of_two(size))*/ && (*str == '@')) {
		secdbg_paddr = (phys_addr_t)memparse(++str, NULL);
		secdbg_size = size;
	}

	pr_info("secdbg_paddr = 0x%llx\n", (unsigned long long)secdbg_paddr);
	pr_info("secdbg_size = 0x%zx\n", secdbg_size);

	return 0;
}
__setup("sec_dbg=", sec_dbg_setup);

static inline long get_switch_state(bool preempt, struct task_struct *p)
{
	return preempt ? TASK_RUNNING | TASK_STATE_MAX : p->state;
}

static __always_inline void __sec_debug_task_sched_log(int cpu, bool preempt,
		struct task_struct *task, struct task_struct *prev,
		char *msg)
{
	struct sched_log *sched_log;
	int i;

	if (unlikely(!secdbg_log))
		return;

	if (unlikely(!task && !msg))
		return;

#ifdef NO_ATOMIC_IDX
	i = ++(secdbg_log->idx_sched[cpu]) & (SCHED_LOG_MAX - 1);
#else
	i = atomic_inc_return(&(secdbg_log->idx_sched[cpu]))
		& (SCHED_LOG_MAX - 1);
#endif
	sched_log = &secdbg_log->sched[cpu][i];

	sched_log->time = cpu_clock(cpu);
	if (task) {
		strlcpy(sched_log->comm, task->comm, sizeof(sched_log->comm));
		sched_log->pid = task->pid;
		sched_log->pTask = task;
		sched_log->prio = task->prio;
		strlcpy(sched_log->prev_comm, prev->comm,
				sizeof(sched_log->prev_comm));

		sched_log->prev_pid = prev->pid;
		sched_log->prev_state = get_switch_state(preempt, prev);
		sched_log->prev_prio = prev->prio;
	} else {
		strlcpy(sched_log->comm, msg, sizeof(sched_log->comm));
		sched_log->pid = current->pid;
		sched_log->pTask = NULL;
	}
}

void sec_debug_irq_enterexit_log(unsigned int irq, u64 start_time)
{
	struct irq_exit_log *irq_exit_log;
	int cpu = smp_processor_id();
	int i;

	if (unlikely(!secdbg_log))
		return;

#ifdef NO_ATOMIC_IDX
	i = ++(secdbg_log->idx_irq_exit[cpu]) & (SCHED_LOG_MAX - 1);
#else
	i = atomic_inc_return(&(secdbg_log->idx_irq_exit[cpu]))
			& (SCHED_LOG_MAX - 1);
#endif
	irq_exit_log = &secdbg_log->irq_exit[cpu][i];

	irq_exit_log->time = start_time;
	irq_exit_log->end_time = cpu_clock(cpu);
	irq_exit_log->irq = irq;
	irq_exit_log->elapsed_time = irq_exit_log->end_time - start_time;
	irq_exit_log->pid = current->pid;
}

void sec_debug_task_sched_log_short_msg(char *msg)
{
	__sec_debug_task_sched_log(raw_smp_processor_id(),
			false,  NULL, NULL, msg);
}

void sec_debug_task_sched_log(int cpu, bool preempt,
		struct task_struct *task, struct task_struct *prev)
{
	__sec_debug_task_sched_log(cpu, false, task, prev, NULL);
}

void sec_debug_timer_log(unsigned int type, int int_lock, void *fn)
{
	struct timer_log *timer_log;
	int cpu = smp_processor_id();
	int i;

	if (unlikely(!secdbg_log))
		return;

#ifdef NO_ATOMIC_IDX
	i = ++(secdbg_log->idx_timer[cpu]) & (SCHED_LOG_MAX - 1);
#else
	i = atomic_inc_return(&(secdbg_log->idx_timer[cpu]))
			& (SCHED_LOG_MAX - 1);
#endif
	timer_log = &secdbg_log->timer_log[cpu][i];

	timer_log->time = cpu_clock(cpu);
	timer_log->type = type;
	timer_log->int_lock = int_lock;
	timer_log->fn = (void *)fn;
	timer_log->pid = current->pid;
}

void sec_debug_secure_log(u32 svc_id, u32 cmd_id)
{
	struct secure_log *secure_log;
	static DEFINE_SPINLOCK(secdbg_securelock);
	unsigned long flags;
	int cpu;
	int i;

	if (unlikely(!secdbg_log))
		return;

	spin_lock_irqsave(&secdbg_securelock, flags);

	cpu = smp_processor_id();
#ifdef NO_ATOMIC_IDX
	i = ++(secdbg_log->idx_secure[cpu]) & (TZ_LOG_MAX - 1);
#else
	i = atomic_inc_return(&(secdbg_log->idx_secure[cpu]))
			& (TZ_LOG_MAX - 1);
#endif
	secure_log = &secdbg_log->secure[cpu][i];

	secure_log->time = cpu_clock(cpu);
	secure_log->svc_id = svc_id;
	secure_log->cmd_id = cmd_id;
	secure_log->pid = current->pid;

	spin_unlock_irqrestore(&secdbg_securelock, flags);
}

void sec_debug_irq_sched_log(unsigned int irq, void *fn,
		char *name, unsigned int en)
{
	struct irq_log *irq_log;
	int cpu = smp_processor_id();
	int i;

	if (unlikely(!secdbg_log))
		return;

#ifdef NO_ATOMIC_IDX
	i = ++(secdbg_log->idx_irq[cpu]) & (SCHED_LOG_MAX - 1);
#else
	i = atomic_inc_return(&(secdbg_log->idx_irq[cpu]))
			& (SCHED_LOG_MAX - 1);
#endif
	irq_log = &secdbg_log->irq[cpu][i];

	irq_log->time = cpu_clock(cpu);
	irq_log->irq = irq;
	irq_log->fn = (void *)fn;
	irq_log->name = name;
	irq_log->en = irqs_disabled();
	irq_log->preempt_count = preempt_count();
	irq_log->context = &cpu;
	irq_log->pid = current->pid;
	irq_log->entry_exit = en;
}

static int __init sec_debug_sched_log_init(void)
{
	size_t i;
	struct sec_debug_log *vaddr;
	size_t size;

	if (secdbg_paddr == 0 || secdbg_size == 0) {
		pr_info("sec debug buffer not provided. Using kmalloc..\n");
		size = sizeof(struct sec_debug_log);
		vaddr = kzalloc(size, GFP_KERNEL);
	} else {
		size = secdbg_size;
		vaddr = ioremap_wc(secdbg_paddr, secdbg_size);
	}

	pr_info("vaddr=0x%p paddr=0x%llx size=0x%zx sizeof(struct sec_debug_log)=0x%zx\n",
			vaddr, (uint64_t)secdbg_paddr,
			secdbg_size, sizeof(struct sec_debug_log));

	if ((!vaddr) || (sizeof(struct sec_debug_log) > size)) {
		pr_err("ERROR! init failed!\n");
		return -EFAULT;
	}

	memset_io(vaddr->sched, 0x0, sizeof(vaddr->sched));
	memset_io(vaddr->irq, 0x0, sizeof(vaddr->irq));
	memset_io(vaddr->irq_exit, 0x0, sizeof(vaddr->irq_exit));
	memset_io(vaddr->timer_log, 0x0, sizeof(vaddr->timer_log));
	memset_io(vaddr->secure, 0x0, sizeof(vaddr->secure));

	for (i = 0; i < num_possible_cpus(); i++) {
#ifdef NO_ATOMIC_IDX
		vaddr->idx_sched[i] = -1;
		vaddr->idx_irq[i] = -1;
		vaddr->idx_secure[i] = -1;
		vaddr->idx_irq_exit[i] = -1;
		vaddr->idx_timer[i] = -1;

#else
		atomic_set(&(vaddr->idx_sched[i]), -1);
		atomic_set(&(vaddr->idx_irq[i]), -1);
		atomic_set(&(vaddr->idx_secure[i]), -1);
		atomic_set(&(vaddr->idx_irq_exit[i]), -1);
		atomic_set(&(vaddr->idx_timer[i]), -1);

#endif
	}

	secdbg_log = vaddr;

	pr_info("init done\n");

	return 0;
}
arch_initcall_sync(sec_debug_sched_log_init);
