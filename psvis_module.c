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
#include <linux/syscalls.h>
#include <linux/fcntl.h>

void print_children(struct task_struct *task, int level) {
  int i;
  struct list_head *list;
  struct task_struct *sibling;

  struct file *f;
  char buf[128];
  mm_segment_t fs;
  int j;

  for (j = 0; j < 128; j++) {
    buf[j] = 0;
  }

  f = filp_open("~/Desktop/304/project_01/psvis_output.txt", O_RDONLY, 0);
  fs = get_fs();
  set_fs(KERNEL_DS);
  f->f_op->read(f, buf, 128, &f->f_pos);
  set_fs(fs);
  printk(KERN_INFO "buf:%s\n", buf);
  filp_close(f, NULL);

  list_for_each(list, &task->children) {
    sibling = list_entry(list, struct task_struct, sibling);
    for (i = 0; i < level; i++) {
      printk(KERN_INFO "--");
    }
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
