#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>


void main()
{
    int status;

    int semid; /* Identifikator der Semaphorgruppe */

    unsigned short init_array[1]; /* zur Initialisierung */

    struct sembuf sem_p[2], sem_v[2]; /* Beschreibung von P- und V-Operationen */

    /* Erzeugung einer Semaphorgruppe mit einem Semaphor */

    semid = semget(IPC_PRIVATE,2,IPC_CREAT|0777);

    /* Initialisierung des Semaphors mit dem Wert 0 */

    init_array[0] = 0;
    init_array[1] = 0;
    semctl(semid,0,SETALL,init_array);

    /* Vorbereitung der P- und der V-Operation */

    sem_p[0].sem_num = 0;   sem_v[0].sem_num = 0;
    sem_p[0].sem_op  = -1;  sem_v[0].sem_op  = 1;
    sem_p[0].sem_flg = 0;   sem_v[0].sem_flg = 0;

    sem_p[1].sem_num = 1;   sem_v[1].sem_num = 1;
    sem_p[1].sem_op  = -1;  sem_v[1].sem_op  = 1;
    sem_p[1].sem_flg = 0;   sem_v[1].sem_flg = 0;




    int i,cnt=0;

    if(fork()==0)
    {
        semop(semid,&sem_p[1],1);

        for(i=6;i<=10;i++)
            printf("%d\n",i);

        semop(semid,&sem_v[0],1);
        semop(semid,&sem_p[1],1);

        for(i=16;i<=20;i++)
            printf("%d\n",i);
        semop(semid,&sem_v[0],1);
        semop(semid,&sem_p[1],1);

        for(i=26;i<=30;i++)
            printf("%d\n",i);

        exit(0);
    }

    for(i=1;i<=5;i++)
    {
        printf("%d\n",i);
        sleep(1);
    }
    semop(semid,&sem_v[1],1);
    semop(semid,&sem_p[0],1);

    for(i=11;i<=15;i++)
    {
        printf("%d\n",i);
        sleep(1);
    }
    semop(semid,&sem_v[1],1);
    semop(semid,&sem_p[0],1);

    for(i=21;i<=25;i++)
    {
        printf("%d\n",i);
        sleep(1);
    }
    semop(semid,&sem_v[1],1);

    wait(&status);
    wait(&status);
    semctl(semid,0,IPC_RMID,0);

}