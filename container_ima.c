/*
 * Project for comsE6118:
 * Container IMA using eBPF
 * Fall 2022
 *
 * Avery Blanchard, agb2178
 */
#include <linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/ima.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <keyutils.h>
#include <keys/system_keyring.h>

#define MODULE_NAME "ContainerIMA"
#define INTEGRITY_KEYRING_IMA 1

const char *measure_log_dir = "/secure/container_ima/"; // in this dir, per container measurement logs 
struct vtpm_proxy_new_dev *container_vtpms;
struct container_data *head;
struct container_data *cur;
struct tpm_chip *ima_tpm_chip;
int host_inum;

/*
notes 
use integrity_iint_cache, not namespace specific
ima_collect_measurement
write own store measurement
*/
struct mmap_args_t {
	void *addr;
	size_t length;
	int prot;
	int flags;
	int fd;
	off_t offset
};
struct container_ima_hash {
	u8 algo;
	u8 length;
};
struct container_ima_hash_data {
	u8 algo;
	u8 length;
	u8 digest[HASH_MAX_DIGESTSIZE];
};
struct container_ima_entry {
	int pcr;
	struct tpm_digest *digests;
	int container_id;
	u32 data_len;
};
struct inode_ima_data {
	struct mutex mut;
	struct inode *inode;
	unsigned long flags;
	int container_id
	struct container_ima_hash_data *hash;
};
struct container_ima_inode_data {
	struct inode_ima_data iiam;
	struct vtpm_proxy_new_dev vtpm;
	int container_id;
	struct file *file;
	const char *filename;
	const void *buf;
	int len;
};
struct container_data {
	struct vtpm_proxy_new_dev vtpm;
	int container_id;
	inr keyring;
	struct file *ml;
	u8 algo; 
	int policy_num; 
	struct container_data *next;
};

int container_ima_fs_init() 
{
	int res;
	mode_t dir_mode= 0755;

	res = mkdir(measure_log_dir, dir_mode);

	return res;
}
/*
 * container_keyring_init 
 * 
 * 
 * https://man7.org/linux/man-pages/man7/keyrings.7.html
 * https://man7.org/linux/man-pages/man2/add_key.2.html 
 */
int contain_keyring_init()
{
	return 0;
}
/*
 * container_keyring_add_key
 * create key from loading the vTPM x.509 cert
 */
int container_keyring_add_key() 
{
	return 0;
}
/*
 * ima_vtpm_setup 
 *
 * Set up per container vTPM, PCR 10 for IMA
 * https://www.kernel.org/doc/html/v4.13/security/tpm/tpm_vtpm_proxy.html
 * https://elixir.bootlin.com/linux/v6.0.5/source/drivers/char/tpm/tpm_vtpm_proxy.c#L624 
 */
long ima_vtpm_setup(int container_id, struct tpm_chip *ima_tpm_chip, struct container_data *data) 
{
	struct vtpm_proxy_new_dev *new_vtpm;
	long ret;
	int ioctl; 
	struct file *vtpm_file;
	const char *vtpm_fd_name;
	char id[10];
	int check;
	
	new_vtpm = kmalloc(sizeof(struct vtpm_proxy_new_dev), GFP_KERNEL);
	if (!new_vtpm) {
		pr_err("kmalloc failed\n");
	}

	check = sprintf(id, "%d", container_id);
	if (check < 0)
		pr_err("sprintf fails in vtpm setup \n");
	
	check = strcat_s("/dev/vtpm", id);
	if (check == -1)
		pr_err("strcat_s fails in vtpm setup\n");

	new_vtpm.flags = VTPM_PROXY_FLAG_TPM2;
	new_vtpm.tpm_num = container_id;
	new_vtpm.fd = "/dev/vtpm";
	new_vtpm.major = MAJOR(ima_tpm_chip->device->devt); // MAJOR(dev_t dev); major number of the TPM device
	new_vtp.minor = MINOR(ima_tpm_chip->device->devt); // MINOR(dev_t dev); minor number of the TPM device


	ret = vtpmx_ioc_new_dev(vtpm_file, ioctl, (unsigned long)&new_vtpm);
	
	if (ret != 0) {
		pr_err("Failed to create a new vTPM device\n");
	}

	data->vtpm = new_vtpm;
	return ret;
	
}
/*
 * container_ima_setup
 *
 * Set up environment to initalize container IMA
 * Malloc structure to hold container ids and other data to preserve state
 */
void container_ima_setup()
{
	ima_hash_setup();

}
/*
 * container_ima_crypto_init
 * 
 * Iterate over PCRs, check algorithm for PCR10 and record
 */
int container_ima_crypto_init(struct container_data *data)
{
	int ret;
	int i;


	return 0;

}
/*
 * container_ima_init
 *
 * Initalize container IMA
 * Create vTPM proxy using container_id as its number
 * Create measurment log 
 * Default policy 
 * 
 * To do:
 * - Seperate policies per container 
 * - Seperate key ring? 
 * - Secure FS instance per container or store all logs in the same instance?
 * - Would having the vtpm device number relate to the container namespace ID be a problem for keylime?
 * - Visability of vTPM to the container? Require container-side config?
 */
int container_ima_init(int container_id) 
{
	int ret;
	struct container_data *data;

	data = kmalloc(size_of(struct container_data), GFP_KERNEL);
	if (!data) {
		pr_error("kmalloc failed\n");
		return -1;
	}
	ima_tpm_chip = tpm_default_chip();
	if (!ima_tpm_chip)
		pr_info("No TPM chip found, activating TPM-bypass!\n");

	container_ima_vtpm = ima_vtpm_setup(container_id, ima_tpm_chip, data); // per container vTPM

	//ret = integrity_init_keyring(INTEGRITY_KEYRING_IMA); // per container key ring

	data->keyring = INTEGRITY_KEYRING_IMA;

	if (ret)
		return ret;
	ret = container_ima_crypto_init(container_id, data); // iterate over PCR banks and init the algorithms per bank  

	if (ret)
		return ret;

	ret = container_ima_ml_init(container_id); // set up directory for per container Measurment Log

	if (ret) 
		return ret;

	container_ima_policy_init(container_id); // start with default policy for all containers

	return ret;
}

/* mapping of id to system call arguments */
BPF_HASH(active_mmap_args_map, uint64, struct mmap_args_t);

/*
 * syscall__probe_entry_mmap
 * 
 * void *mmap(void *addr, size_t length, int prot, int flags,
 * 		int fd, off_t offset);
 *
 * https://man7.org/linux/man-pages/man2/mmap.2.html
 *
 * Entry hook for mmap system call 
 */
int syscall__probe_entry_mmap(struct pt_regs *ctx, void *addr, size_t length, int prot, int flags, int fd, off_t offset) 
{
	uint64_t id = bpf_get_current_pid_tgid();
	
	struct mmap_args_t args = {};

	args.addr = addr;
	args.length = length;
	args.prot = prot;
	args.flags = flags;
	args.fd = fd;
	args.offset = offset;

	active_mmap_args_map.update(&id, &args);

	return 0;

}
/*
 * syscall__probe_ret_mmap 
 *
 * Exit hook for mmap system call
 */
int syscall__probe_ret_mmap(struct pt_regs *ctx) 
{
	/* if system call was 
	 * 	1. orginating from the container
	 * 	2. maps an executable page
	 * 	3. was successful
	 * then 
	 * 	1. access argument cache
	 * 	2. call functions to create hash digest, extend,
	 * 		and send to TPM for IMA per container.
	 */
	int ret;
	struct task_struct *task;
	unsigned int inum;
	struct mmap_args_t *args = {};
	
	ret = 0;
	task =  bpf_get_current_task();
    inum = task->nsproxy->cgroup_ns->ns_common->inum;

	active_mmap_args_map.pop(&args);

	if (inum == host_inum) {
		return ret;
	}

	if (args->prot != PROT_READ && args->prot != PROR_EXEC) {
		return ret;
	}

	/* Check if container already has an active ML, create hash of page and add to ML */


	/* If not, create vTPM and key ring, create hash of page and add to ML */
	ret = container_ima_init(inum); 
	queue_measurement();

	return ret;


}
int container_ima_cleanup() {
	
	return 0;
}
static int container_ima_init(void)
{
	/* Start container IMA */
	int ret;
	struct task_struct *task;

	task = current;
	host_inum = task->nsproxy->cgroup_ns->ns_common->inum;

	head = NULL;
	cur = NULL;
	container_ima_setup();
	ret = container_ima_init();


	return ret;
}

static int container_ima_init(void)
{
	/* Clean up 
	 * Free keyring and vTPMs
	 */
	int ret;
	ret = container_ima_cleanup();
	return ret;
}


module_init(container_ima_init);
module_exit(container_ima_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(MODULE_NAME);
MODULE_AUTHOR("Avery Blanchard");

