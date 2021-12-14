#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <pthread.h>
#include <signal.h>
enum NUMS{
    NUM_PROGRAMMERS = 3,//число программистов. От 2 до 15
    PENDING = 16,//Статусы програм
    IN_PROCESS = 32,//Програма пишется или проверяется
    READY = 64,//Программист проверил программу
    MASK_STATES = 0xf0,//Маска статусов выполнения программы
    MASK_PENDS = 0xf

};
int semid;
pthread_t threads[NUM_PROGRAMMERS];
void up(int semid,int v) {
    semop(semid, (struct sembuf[]){{v, 1, 0}}, 1);
}
void down(int semid,int v) {
    semop(semid, (struct sembuf[]){{v, -1, 0}}, 1);

}
typedef struct Args {
    int number;
    int semid;
} someArgs_t;

int reviews[NUM_PROGRAMMERS];
void write_prog() {
    // Программист пишет программу от 0 до 4 секунд
    long long  time_to_write=(4000000*(long long)rand())/RAND_MAX;
    usleep(time_to_write);
}
int checking() {
    long long int time_to_check=(4000000*(long long)rand())/RAND_MAX;
    usleep(time_to_check);
    return rand()%2;

}
void* son(void* v) {
    someArgs_t *arg = (someArgs_t*)v;
    int number=arg->number;
    int semid=arg->semid;
    int n=1;
    int prevval=NUM_PROGRAMMERS;
    while(1) {
        //пишем программу
        write_prog();

            printf("programmer %d %s his %d program!\n",number,(prevval==NUM_PROGRAMMERS)?"wrote":"rewrited",n);
            fflush(stdout);

        down(semid,NUM_PROGRAMMERS);
        reviews[number]=prevval|PENDING;
        if(prevval==NUM_PROGRAMMERS) {
            //если prevval==NUM_PROGRAMMERS то лююой другой программист может проверить программу. Иначе только тот, что проверял в прошлый раз
            for(int i=0;i<NUM_PROGRAMMERS;i++) {
                if(i==number) continue;
                else up(semid,i);
            }

        }
        else up(semid,prevval);

        up(semid,NUM_PROGRAMMERS);





        while(1) {
            //пока наша программа не проверена, проверяем чужие или спим
            semop(semid,(struct sembuf[]){{number, -1, 0},{NUM_PROGRAMMERS, -1, 0}}, 2);


            if((reviews[number]&MASK_STATES)==READY) {


                prevval=reviews[number]&MASK_PENDS;
                if(prevval==NUM_PROGRAMMERS) n++;
                reviews[number]&=MASK_PENDS;
                reviews[number]|=IN_PROCESS;
                up(semid,NUM_PROGRAMMERS);
                break;
            }
            else {
                for(int i=0;i<NUM_PROGRAMMERS;i++) {
                    if(i==number) continue;

                    if(((reviews[i]&MASK_STATES)==PENDING)&&(((reviews[i]&MASK_PENDS)==NUM_PROGRAMMERS)||((reviews[i]&MASK_PENDS)==number))) {
                        if((reviews[i]&MASK_PENDS)==NUM_PROGRAMMERS) {
                            for(int j=0;j<NUM_PROGRAMMERS;j++) {
                                if((j==number)||(j==i)) continue;
                                down(semid,j);
                            }
                        }



                        reviews[i]&=MASK_PENDS;
                        reviews[i]|=IN_PROCESS;
                        up(semid,NUM_PROGRAMMERS);
                        int result =checking();
                        printf("programmer %d checked programmer %d:program is %s\n",number,i,(result==1)?"correct":"incorrect");
                        fflush(stdout);
                        down(semid,NUM_PROGRAMMERS);
                        if(result==1) reviews[i]=NUM_PROGRAMMERS|READY;
                        else reviews[i]=number|READY;
                        up(semid,i);
                        up(semid,NUM_PROGRAMMERS);
                        break;


                    }
                }
            }
        }
    }
}
void handler(int s) {
    for (int i = 0; i < NUM_PROGRAMMERS; i++) {
        pthread_cancel(threads[i]);
    }
     semctl(semid, 0, IPC_RMID);
     exit(0);

}
int main()
{
    signal(SIGINT,handler);
    int key=100;
    for(int i=0;i<NUM_PROGRAMMERS;i++) {
        reviews[i]=IN_PROCESS;
    }
    semid = semget(key, NUM_PROGRAMMERS+1, 0600 | IPC_CREAT);
    semop(semid, (struct sembuf[]){{NUM_PROGRAMMERS, 1, 0}}, 1);

    someArgs_t args[NUM_PROGRAMMERS];
    for (int i = 0; i < NUM_PROGRAMMERS; i++) {
        args[i].number = i;
        args[i].semid=semid;
    }
    for (int i = 0; i < NUM_PROGRAMMERS; i++) {
         pthread_create(&threads[i], NULL, son, (void*) &args[i]);

    }
     for (int i = 0; i < NUM_PROGRAMMERS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
