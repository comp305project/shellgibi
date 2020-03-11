#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/moduleparam.h>
#include <linux/sched/signal.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

void print_children(struct task_struct *task, int level) {
  struct list_head *list;
  struct task_struct *sibling;
  list_for_each(list, &task->children) {
    sibling = list_entry(list, struct task_struct, sibling);
    printk(KERN_INFO "[%d] Child: %d\n", level, sibling->pid);

    print_children(sibling, level+1);
  }
}

int target_pid = 0;
module_param(target_pid, int, 0);

struct task_struct *task;

int psvis_init(void) {

  for_each_process(task) {
    if (task->pid == target_pid) {
      print_children(task, 0);
    }

    return 0;
  }

  return 0;
}

void psvis_exit(void) {

}

module_init(psvis_init);
module_exit(psvis_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PSVIS");
MODULE_AUTHOR("project");
