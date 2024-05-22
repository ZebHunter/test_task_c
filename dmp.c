#include <linux/init.h>
#include <linux/module.h>
#include <linux/bio.h>
#include <linux/kernel.h>
#include <linux/device-mapper.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/types.h>

#define DM_MSG_PREFIX "dmp"


struct statistics{
    __u64 read_requests;
    __u64 write_requests;
    __u64 read_sum_blk;
    __u64 write_sum_blk;
};

struct dmp_target {
    struct dm_dev* dev;
    sector_t start;
    struct statistics statistics;
    struct kobject ko_stat;
};

static ssize_t show_statistics(struct kobject* ko, struct attribute* attr, char* buf){
    struct dmp_target* dmp = container_of(ko, struct dmp_target, ko_stat);
    __u64 total_req = dmp->statistics.read_requests + dmp->statistics.write_requests;
    return sprintf(buf, "read:\n"
                        "\treqs: %llu\n"
                        "\tavg size: %llu\n"
                        "write:\n"
                        "\treqs: %llu\n"
                        "\tavg size: %llu\n"
                        "total:\n"
                        "\treqs: %llu\n"
                        "\tavg size: %llu\n",
                        dmp->statistics.read_requests,
                        dmp->statistics.read_sum_blk / dmp->statistics.read_requests,

                        dmp->statistics.write_requests,
                        dmp->statistics.write_sum_blk / dmp->statistics.write_requests,

                        total_req,
                        (dmp->statistics.write_sum_blk + dmp->statistics.read_sum_blk) / total_req);

}

static struct attribute volumes_attribute = {
    .name = "volumes",
    .mode = 0444,
};

static struct sysfs_ops volumes_sysfs_ops = {
    .show = show_statistics,
};

static const struct kobj_type stat_ktype = {
    .sysfs_ops = &volumes_sysfs_ops,
};


static int dmp_ctr(struct dm_target* ti, unsigned int argc, char** argv){

    if (argc != 1) {
        printk("dmp: error arg");
        ti->error = "Invalid argument count";
        return -EINVAL;
    }

    struct dmp_target* dmp = kmalloc(sizeof(struct dmp_target), GFP_KERNEL);

    if(dmp == NULL){
        printk("dmp: error alloc");
        ti->error = "dmp: Cannot allocate linear context";
        return -ENOMEM;
    }

    int error = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &dmp->dev);

    if (error) {
                printk("dmp: Device lookup failed");
                ti->error = "dmp: Device lookup failed";
                goto bad;
    }

    dmp->start = 0;

    dmp->statistics.read_requests = 0;
    dmp->statistics.write_requests = 0;
    dmp->statistics.read_sum_blk = 0;
    dmp->statistics.write_sum_blk = 0;

    error = kobject_init_and_add(&dmp->ko_stat, &stat_ktype, &THIS_MODULE->mkobj.kobj, "stat");

    if(error){
        printk("dmp: statistics kobject is null");
        ti->error = "dmp: statistics kobject is null";
        goto bad;
    }

    error = sysfs_create_file(&dmp->ko_stat, &volumes_attribute);

    if(error){
        printk("dmp: error to creating sysfs volumes file");
        ti->error = "dmp: error to creating sysfs volumes file";
        kobject_put(&dmp->ko_stat);
        goto bad;
    }

    ti->private = dmp;
    return 0;
    
  bad:
    kfree(dmp);         
    return error;

}

static int dmp_map(struct dm_target* ti, struct bio* bio){
    struct dmp_target* dmp = (struct dmp_target*) ti->private;
    bio_set_dev(bio, dmp->dev->bdev);
    switch(bio_op(bio)){
        case REQ_OP_READ:
            dmp->statistics.read_requests++;
            dmp->statistics.read_sum_blk += bio_sectors(bio) * SECTOR_SIZE;
            break;
        case REQ_OP_WRITE:
            dmp->statistics.write_requests++;
            dmp->statistics.write_sum_blk += bio_sectors(bio) * SECTOR_SIZE;
            break;
        default:
            return DM_MAPIO_KILL;
    }

    submit_bio(bio);
    
    return DM_MAPIO_SUBMITTED;
}

static void dmp_dtr(struct dm_target* ti){
    struct dmp_target* dmp = (struct dmp_target*) ti->private;
    kobject_put(&dmp->ko_stat);
    dm_put_device(ti, dmp->dev);
    kfree(dmp);
}

static struct target_type dmp = {
    .name = "dmp",
    .version = {1,0,0},
    .module = THIS_MODULE,
    .ctr = dmp_ctr,
    .dtr = dmp_dtr,
    .map = dmp_map,
};

/*-----------------------------Module Functions---------------------*/

static int __init init_dmp(void){
    int error = dm_register_target(&dmp);
    if(error) printk(KERN_CRIT "\n Error in registering target \n");
    return 0;
}

static void __exit exit_dmp(void){
    dm_unregister_target(&dmp);
}

module_init(init_dmp);
module_exit(exit_dmp);
MODULE_AUTHOR("Mikhail Rogachev");
MODULE_DESCRIPTION("device mapper proxy");
MODULE_LICENSE("GPL");
