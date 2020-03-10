#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

int psvis_init(void) {
    return 0;
}

void psvis_exit(void) {

}

module_init(psvis_init);
module_exit(psvis_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PSVIS");
MODULE_AUTHOR("project");