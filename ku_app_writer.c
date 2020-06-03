#include "ku_ipc_lib.c"

void main(){

    int ret;
    int key = 0; // 요청하는 queue 번호
    int msqid;
    int msgsize;
    struct msgbuf user_msg = {1,"Empty"};

    printf("msg 내용을 입력하세요\n: ");
    fgets(user_msg.text, 128, stdin);
    user_msg.text[strlen(user_msg.text) - 1] = '\0';

    printf("msg type을 입력하세요 : ");
    scanf("%ld",&user_msg.type);
    
    msgsize = strlen(user_msg.text)+1;
    msqid = ku_msgget(key, KU_IPC_CREAT);
    printf("key : %d\n",key);

    ret = ku_msgsnd(msqid, &user_msg, msgsize, KU_IPC_NOWAIT);

}
