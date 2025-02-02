/*
 * File: container_ima_init.c
 *      Functions for initialization and cleanup
 */
#ifndef __CONTAINER_IMA_INIT_H__
#define __CONTAINER_IMA_INIT_H__

#include <linux/ima.h>
#include <linux/hugetlb.h>
#include <linux/vtpm_proxy.h>
#include <linux/bpf.h>
#include <linux/spinlock.h>
#include <linux/integrity.h>
#include "container_ima.h"
#include "container_ima.h"
#include "container_ima.h"
#include "container_ima_init.h"
#include "container_ima_fs.h"
#include "container_ima_api.h"

#define MMAP_MAX_MAPPINGS 100 //adjust as needed

struct tpm_chip *ima_tpm_chip;
static struct kmem_cache *c_ima_cache;

/*
 * init_container_ima_data
 * 	
 */
struct container_ima_data *init_container_ima_data(unsigned int container_id) 
{
	struct container_ima_data *data;
	pr_info("Creating container data for IMA\n");
	data = create_container_ima_data();
	
	/* init policy lists */
	pr_info("Init policy lists\n");
	INIT_LIST_HEAD(&data->c_ima_default_rules);

	data->container_id = container_id;

	/* init hash table */
	pr_info("Init hash table\n");
	atomic_long_set(&data->c_ima_write_mutex.owner, 0);
	//atomic_long_set(&data->hash_tbl->violations, 0);
	//memset(&data->hash_tbl->queue, 0, sizeof(data->hash_tbl->queue));

	/* init ML */
	pr_info("Init list of measurements\n");
	INIT_LIST_HEAD(&data->c_ima_measurements);
	mutex_init(&data->c_ima_write_mutex);

	data->container_integrity_iint_tree = RB_ROOT;
	//DEFINE_RWLOCK(data->container_integrity_iint_lock);

	return data;
}
struct container_ima_data *create_container_ima_data(void) 
{
	struct container_ima_data *data;

	//data = kmem_cache_zalloc(c_ima_cache, GFP_KERNEL);
	data = kmalloc(sizeof(data), GFP_KERNEL);
	if (!data) {
		return ERR_PTR(-ENOMEM);
		pr_err("Allocation failed in cache\n");
	}
	return data;
}
void container_ima_free_data(struct container_ima_data *data)
{
	/* Free policy, tree, hash table, vtpm, etc.... here */
	kmem_cache_free(c_ima_cache, data);

}
/*
 * container_ima_init
 * 		Initalize container IMA
 * 		Create vTPM proxy using container_id as its number
 *		Create measurment log 
 * 		Default policy
 */
struct container_ima_data *init_container_ima(unsigned int container_id, struct dentry *c_ima_dir, struct dentry *c_ima_symlink) 
{
	int ret;
	struct container_ima_data *data;
	/* check if container exist, then return container_data here */
	data = ima_data_exists(container_id);
	if (data) {
		pr_info("Data exists\n");
		return data;
	}
	pr_info("Data does not exist, init\n");
	data = init_container_ima_data(container_id);

	ima_tpm_chip = tpm_default_chip();
	if (!ima_tpm_chip)
		pr_info("No TPM chip found, activating TPM-bypass!\n");

	//ret = container_ima_fs_init(data, c_ima_dir, c_ima_symlink);
	//ret = integrity_init_keyring(INTEGRITY_KEYRING_IMA); // per container key ring

	//data->keyring = INTEGRITY_KEYRING_IMA;

	//ret = container_ima_crypto_init(data); // iterate over PCR banks and init the algorithms per bank  

	container_ima_add_data_entry(data, container_id);
	return data;
}

/*
 * container_keyring_init 
 *  TODO 
 * https://man7.org/linux/man-pages/man7/keyrings.7.html
 * https://man7.org/linux/man-pages/man2/add_key.2.html 
 */
int container_keyring_init()
{
	return 0;
}
/*
 * TODO
 */
int container_ima_cleanup() {
	
	return 0;
}
/*
 * container_ima_crypto_init
 * 
 * Iterate over PCRs, check algorithm for PCR10 and record
 */
int container_ima_crypto_init(struct container_ima_data *data)
{
	int ret;
	int i;


	return 0;

}
#endif