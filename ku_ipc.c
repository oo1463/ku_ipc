#include "ku_ipc.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/string.h>


MODULE_LICENSE("GPL");
#define DEV_NAME "ku_ipc_dev" // 캐릭터 디바이스 이름

struct params{
    int msqid, msgsz, msgflg;
    void *msgp;
    long msgtyp;
};

struct msg_queue {
    struct list_head list;
    int ref_counter;
    int msg_vol;
    int msg_counter;
    struct msgbuf msg;
};

static struct msg_queue queue[10];

spinlock_t my_lock;

void delay(int sec){

    int i,j;
    for(j=0; j<sec; j++){
        for(i=0; i<1000 ; i++){
            udelay(1000);
        }
    }
}

static int kernel_get(int key, int msgflg){
    if(msgflg & KU_IPC_CREAT){
        queue[key].ref_counter++;
        return key;
    }
    else if(msgflg & KU_IPC_EXCL){
        if(queue[key].ref_counter == 0){
            queue[key].ref_counter++;
            return key;
        }
        else{
            return -1;
        }
    }
    else{
        return -1;
    }

}

static int kernel_close(int msqid){
    if(msqid >= 10 || msqid < 0) return -1;
    else if(queue[msqid].ref_counter <= 0) return -1;
    else {
        queue[msqid].ref_counter--;
        return 0;
    }
}


static int kernel_snd(int msqid, void *msgp, int msgsz, int msgflg){  // write 함수
    int ret;

    struct msg_queue *tmp = 0;
    printk("ku_ipc : snd started, nowait & return / queue[%d].msg_counter = %d\n",msqid, queue[msqid].msg_counter);

    if(queue[msqid].msg_counter >= KUIPC_MAXMSG || queue[msqid].msg_vol + msgsz > KUIPC_MAXVOL){
        if(msgflg & KU_IPC_NOWAIT){
            printk("ku_ipc : nowait & return / queue[%d].msg_counter = %d\n",msqid, queue[msqid].msg_counter);
            
            return -1;
        }
        else{
            delay(1);
            ret = kernel_snd(msqid, msgp, msgsz, msgflg);
        }
    }
    else{
        spin_lock(&my_lock);  // lock

        tmp = (struct msg_queue*)kmalloc(sizeof(struct msg_queue), GFP_KERNEL);  //노드 할당
        ret = copy_from_user(&(tmp->msg), (struct msgbuf *)msgp, msgsz + sizeof(long));  // msgsz 만큼 할당
        list_add_tail(&tmp->list, &queue[msqid].list);  // list에 노드 추가
        queue[msqid].msg_counter++;
        queue[msqid].msg_vol += msgsz + sizeof(long);
        spin_unlock(&my_lock); // unlock
        printk("ku_ipc : volume = %d / msgcounter = %d\n", queue[msqid].msg_vol, queue[msqid].msg_counter);

        return ret;
    }
    
    return ret;
}

static int kernel_rcv(int msqid, void *msgp, int msgsz, long msgtyp, int msgflg) {  // read 함수 부분
    int ret = -1;
    struct msg_queue *tmp = 0;
    struct list_head *pos = &queue[msqid].list;

    if(list_empty(pos)){  // 큐에 메시지가 없는 경우
        printk("ku_ipc : queue is empty\n");

        if(msgflg & KU_IPC_NOWAIT){
            ret = -1;
        }
        else {
            delay(1);
            ret = kernel_rcv(msqid, msgp, msgsz, msgtyp, msgflg);
        }
    }
    else{ // 큐에 메시지가 있는 경우       
        
        list_for_each(pos,&queue[msqid].list){
            tmp = list_entry(pos, struct msg_queue, list);
            if(tmp->msg.type == msgtyp){

                if(strlen(tmp->msg.text)+1 > msgsz) // 큐에 있는 메시지가 receive 할 메시지보다 클 때
                {
                    if(msgflg & KU_MSG_NOERROR){ // 메시지 컷팅
                        spin_lock(&my_lock);
                        tmp->msg.text[msgsz] = '\0';
                        copy_to_user((struct msgbuf *)msgp, &(tmp -> msg), msgsz + 1 + sizeof(long));
                        list_del(pos);
                        kfree(tmp);
                        queue[msqid].msg_counter--;
                        queue[msqid].msg_vol = queue[msqid].msg_vol - strlen(tmp->msg.text) - sizeof(long) - 1;
                        spin_unlock(&my_lock);
                        printk("ku_ipc: node is deleting\n");
                        ret = msgsz + sizeof(long);
                        break;
                    }
                    else{
                        printk("ku_ipc: Message in queue is bigger than receiving message\n");
                        ret = -1;
                    }
                }
                else{
                    spin_lock(&my_lock);
                    copy_to_user((struct msgbuf *)msgp, &(tmp -> msg), strlen(tmp->msg.text) + 1 + sizeof(long));
                    list_del(pos);
                    queue[msqid].msg_counter--;
                    queue[msqid].msg_vol = queue[msqid].msg_vol - sizeof(long) - strlen(tmp->msg.text) - 1;
                    kfree(tmp);
                    spin_unlock(&my_lock);
                    printk("ku_ipc: node is deleting\n");
                    ret = sizeof(long) + strlen(tmp->msg.text) + 1;
                    break;
                }
                printk("ku_ipc : volume = %d / msgcounter = %d\n", queue[msqid].msg_vol, queue[msqid].msg_counter);

            }
            
            if(list_is_last(pos,&queue[msqid].list)){ // type에 맞는 list가 없는 경우
                if(msgflg & KU_IPC_NOWAIT){
                    ret = -1;
                }
                else {
                    delay(1);
                    ret = kernel_rcv(msqid, msgp, msgsz, msgtyp, msgflg);
                }
            }

        }

    }
    return ret;
}


static long ku_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {

    struct params user_param;
    int ret;

    copy_from_user(&user_param, (struct params*)arg, sizeof(struct params));
    switch(cmd) {
        case KU_MSGGET:
            ret = kernel_get(user_param.msqid, user_param.msgflg);
            break;
        case KU_MSGCLOSE:
            ret = kernel_close(user_param.msqid);
            break;
        case KU_MSGSND:
            ret = kernel_snd(user_param.msqid, user_param.msgp, user_param.msgsz, user_param.msgflg);
            break;
        case KU_MSGRCV:
            ret = kernel_rcv(user_param.msqid, user_param.msgp, user_param.msgsz, user_param.msgtyp, user_param.msgflg);
            break;
    }
    return 0;
}


static int ku_ipc_open(struct inode *inode, struct file *file) {
    return 0;
}

static int ku_ipc_release(struct inode *inode, struct file *file) {
    return 0;
}


struct file_operations ku_ipc_fops = { //fops구조체 초기화, 함수포인터에 연결
    .open = ku_ipc_open,
    .release = ku_ipc_release,
    .unlocked_ioctl = ku_ioctl,
};

static dev_t dev_num;
static struct cdev *cd_cdev;


static int __init ku_ipc_init(void){

    int ret;
    int i;

    printk("ku_ipc : Init Module\n");

    for(i=0; i<10;i++){
        INIT_LIST_HEAD(&queue[i].list); // 리스트 init
        queue[i].ref_counter = 0;
        queue[i].msg_vol = 0;
        queue[i].msg_counter = 0;
    }

    alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
    cd_cdev = cdev_alloc();
    cdev_init(cd_cdev, &ku_ipc_fops); // cd_cdev 드라이버와 fops 연결
    ret = cdev_add(cd_cdev, dev_num, 1); // cd_cdev 메이저 넘버 연결

    if(ret < 0){
        printk("ku_ipc : fail to add character device \n");
        return -1;
    }

    return 0;
}

static void __exit ku_ipc_exit(void){
    struct msg_queue *tmp =0;
    struct list_head *pos =0;
    struct list_head *q =0;
    int i;

    printk("ku_ipc : Exit Module\n");

    for(i=0 ; i<10 ;i++){
        list_for_each_safe(pos, q, &queue[i].list) {
                tmp = list_entry(pos, struct msg_queue, list);
                list_del(pos);
                kfree(tmp);
            }
    }
    
    cdev_del(cd_cdev);
    unregister_chrdev_region(dev_num,1);
}

module_init(ku_ipc_init);
module_exit(ku_ipc_exit);

