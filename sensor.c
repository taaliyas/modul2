#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

struct msg_buffer {
    long msg_type;
    int s_id;
    char msg_text[200];
};

int q1, q2;
int id;
int running = 1;

pthread_mutex_t lock;

void* listener_thread(void* arg) {
    struct msg_buffer msg;

    while (1) {
        if (msgrcv(q2, &msg, sizeof(msg) - sizeof(long),
                   getpid(), 0) != -1) {

            pthread_mutex_lock(&lock);

            if (strcmp(msg.msg_text, "SHUTDOWN") == 0) {
                printf("\n[INFO] Another sensor has exited\n");
                printf("[INFO] Cancelling current input...\n");
                printf("[INFO] System shutting down...\n");

                running = 0;
                pthread_mutex_unlock(&lock);
                break;
            }

            printf("\n[INFO] Current city status: %s\n\n", msg.msg_text);
            printf("Masukkan data:\n\n");

            pthread_mutex_unlock(&lock);
        }
    }
    return NULL;
}

int main() {
    key_t k1 = ftok("progfile1", 65);
    key_t k2 = ftok("progfile2", 75);

    q1 = msgget(k1, 0666 | IPC_CREAT);
    q2 = msgget(k2, 0666 | IPC_CREAT);

    struct msg_buffer msg;

    pthread_mutex_init(&lock, NULL);

    msg.msg_type = 1;
    msg.s_id = getpid();
    strcpy(msg.msg_text, "REGISTER");
    msgsnd(q1, &msg, sizeof(msg) - sizeof(long), 0);

    msgrcv(q2, &msg, sizeof(msg) - sizeof(long), getpid(), 0);
    id = msg.s_id;

    pthread_t tid;
    pthread_create(&tid, NULL, listener_thread, NULL);

    while (running) {
        char d1[50], d2[50];
        int vcount = 0;

        pthread_mutex_lock(&lock);
        printf("Masukkan data:\n\n");
        pthread_mutex_unlock(&lock);

        while (vcount < 2 && running) {
            char line[100];
            int input;
            char loc[10], stat[10];

            scanf(" %[^\n]", line);

            if (!running) break;

            if (strcmp(line, "exit") == 0) {
                pthread_mutex_lock(&lock);
                printf("[SENSOR] Sending exit signal...\n");
                pthread_mutex_unlock(&lock);

                msg.msg_type = 3;
                msg.s_id = getpid();
                strcpy(msg.msg_text, "EXIT");
                msgsnd(q1, &msg, sizeof(msg) - sizeof(long), 0);

                break;
            }

            if (sscanf(line, "%d %s %s", &input, loc, stat) != 3) {
                pthread_mutex_lock(&lock);
                printf("[ERROR]\n");
                pthread_mutex_unlock(&lock);
                continue;
            }

            if (input != id) {
                pthread_mutex_lock(&lock);
                printf("[ERROR]\n");
                pthread_mutex_unlock(&lock);
                continue;
            }

            if (!(strcmp(loc,"A")==0 || strcmp(loc,"B")==0 ||
                  strcmp(loc,"C")==0 || strcmp(loc,"D")==0) ||
                !(strcmp(stat,"L")==0 || strcmp(stat,"H")==0)) {

                pthread_mutex_lock(&lock);
                printf("[ERROR]\n");
                pthread_mutex_unlock(&lock);
                continue;
            }

            if (vcount == 0)
                sprintf(d1, "%d %s %s", input, loc, stat);
            else
                sprintf(d2, "%d %s %s", input, loc, stat);

            vcount++;
        }

        if (!running) break;

        if (vcount == 2) {
            sprintf(msg.msg_text, "%s | %s", d1, d2);
            msg.msg_type = 2;
            msg.s_id = getpid();
            msgsnd(q1, &msg, sizeof(msg) - sizeof(long), 0);
        }
    }
	
    pthread_join(tid, NULL);

    pthread_mutex_destroy(&lock);

    printf("[SENSOR] Shutting down...\n");
    return 0;
}
