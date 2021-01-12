#define DEBUG 0 //true/false
#define INPUT_SHOW 0//true/false
#define GEN_FILE 0 //true/false
#define THRD_NUM_TO_GEN 4
#define ARR_LEN_TO_GEN 10000
#define SUCCESS 0
#define MAX_COUNT_FOR_REC 1000
#include <pthread.h>
#include <semaphore.h> 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define SUM(x, y) (x + y)

typedef struct subarray {
    int index_from;
    int index_to;
    int id;
} SUBARRAY;

const char *inf = { "input.txt" };
const char *ouf = { "output.txt" };
const char *f_time = { "time.txt" };
FILE *f;

int thrds_qnt, num_qnt;
volatile int *array;
volatile SUBARRAY *subarr;
pthread_t *threads;
sem_t sem, additional_sem;

int error(const char *err);
int get_args(void);
void gen_inp(void);
void show_args(void);
void every_struct_init(void);
void every_struct_deinit(void);
int check_sem_value(void);
void task_divide(int border_u);
void create_threads(void);
void* thread_entry(void *param);
void do_merging(int border_l, int border_m, int border_u, int *a);
void do_merging_sort(int border_l, int border_u, int *a);
void sort(void);

int main(void) {
    if (get_args())
        return error("fopen failed.");

    every_struct_init();
    task_divide(0);
    create_threads();
    check_sem_value();
    sort();
    every_struct_deinit();

    return 0;
}

int error(const char *err) {
    printf("ERROR: %s", err);
    return -1;
}

void gen_inp(void) {
    if (DEBUG)
        printf("\nNow in -> gen_inp\n");

    int i;
    thrds_qnt = THRD_NUM_TO_GEN;
    num_qnt = ARR_LEN_TO_GEN;
    array = (int*)malloc(ARR_LEN_TO_GEN * sizeof(int));

    srand(time(NULL));
    for (i = 0; i < ARR_LEN_TO_GEN; i++)
        array[i] = -(RAND_MAX / 2) + rand();

    if (DEBUG)
        show_args();
}

int get_args(void) {
    if (DEBUG)
        printf("\nNow in -> get_args\n");

    if (GEN_FILE) {
        gen_inp();
    }
    else {
        int i = 0;
        char line1[20], line2[20];
        if ((f = fopen(inf, "rb")) == NULL)
            return 1;
        fscanf(f, "%s", line1);
        fscanf(f, "%s", line2);
        thrds_qnt = atoi(line1);
        num_qnt = atoi(line2);
        array = (int*)malloc(num_qnt * sizeof(int));

        while (i < num_qnt) {
            fscanf(f, "%s", line1);
            array[i] = atoi(line1);
            i++;
        }
        fclose(f);
        if (DEBUG)
            show_args();
    }
    return 0;
}

void show_args(void) {
    printf("\nArgs: thrds_qnt = %d, num_qnt = %d\n", thrds_qnt, num_qnt);
    if (INPUT_SHOW) {
        printf("with input arr:\n");
        for (int i = 0; i < num_qnt; i++)
            printf("%d ", array[i]);
    }
}

void every_struct_init(void) {
    if (DEBUG)
        printf("\nNow in -> every_struct_init\n");
    threads = (pthread_t*)malloc(sizeof(pthread_t) * thrds_qnt);
    subarr = (SUBARRAY*)malloc(sizeof(SUBARRAY) * (1 + thrds_qnt));
    sem_init(&sem, 0, 0);
    sem_init(&additional_sem, 0, thrds_qnt);
}

void every_struct_deinit(void) {
    sem_destroy(&sem);
    sem_destroy(&additional_sem);
    free((void*)subarr);
    free((void*)array);
    free(threads);
}

int check_sem_value(void) {
    int sem_value;

    while (true) {
        sem_getvalue(&additional_sem, &sem_value);

        if (0 == sem_value)
            break;

        pthread_yield();
    }

    return 0;
}

void inc_subarr(int index, int index_from, int index_to) {
    subarr[index].id = index;
    subarr[index].index_from = index_from;
    subarr[index].index_to = index_to;
}

void print_divide_debug_info() {
    int i;

    for (i = 1; i < thrds_qnt + 1; i++) {
        printf("\nThrd with num %d got some job in range %d:%d",
            i, subarr[i].index_from, subarr[i].index_to);    
    }

    if (thrds_qnt <= num_qnt) printf("\nAll %d threads got job!\n", thrds_qnt);
    else printf("\nOnly %d threads got job!\n", num_qnt);
}

void task_divide(int border_u) {
    int i, div, mod;
    div = mod = num_qnt;
    if (DEBUG)
        printf("\nNow in -> task_divide\n");

    border_u = 0;
    i = 1;
    div /= thrds_qnt;
    mod %= thrds_qnt;

    for (; i < thrds_qnt + 1; i++) {
        inc_subarr(i, border_u, div - 1 + border_u);
        border_u += div - 1;
        if (mod) {
            mod--;
            border_u++;
            subarr[i].index_to++;
        }
        border_u++;
    }

    if (DEBUG)
        print_divide_debug_info();
}

void create_threads(void) {
    int ind = 0, status;
    while (ind < thrds_qnt)
    {
        status = pthread_create(&threads[ind], 0, thread_entry, (void*)(ind + 1));
        if (status != SUCCESS)
            exit(printf("\nthread_creation ERROR!\n"));
        ind++;
    }
}

void sem_w(void) {
    sem_wait(&additional_sem);
    sem_wait(&sem);
}

void join(int id1, int id2) {
    if (DEBUG)
        printf("\nThread #%d merging with %d\n", id1, id2);
    pthread_join(threads[id2 - 1], 0);
}

void* thread_entry(void* param) {
    int border_l, border_u, border_m, step, mod, buf;
    long index = (long)param;
    step = 1;
    mod = step + 1;
    if (DEBUG)
        printf("\nThread #%d created.\nStart m_sort\n", long(param));
    sem_w();
    border_l = subarr[index].index_from;
    border_u = subarr[index].index_to;

    do_merging_sort(border_l, border_u, (int*)array);
    if (DEBUG)
        printf("\nFinish m_sort\n");

    while (true) {
        if (step + index > thrds_qnt)
            break;
        if (index % mod != 1)
            break;
        join(index, index + step);

        border_l = subarr[index].index_from;
        border_m = subarr[index].index_to;
        border_u = subarr[index + step].index_to;
        do_merging(border_l, border_m, border_u, (int*)array);
        subarr[index].index_to = subarr[index + step].index_to;

        step = step * 2;
        mod = mod * 2;

        if (DEBUG)
            printf("\nMerging complete.\nNew range is %d:%d", 
                subarr[index].index_from, subarr[index].index_to);
    }

    if (DEBUG)
        printf("\nThread #%d finished.\n", index);
}

void do_merging(int border_l, int border_m, int border_u, int *a) {
    int i, j, k, len1 = border_m - border_l + 1, len2 = border_u - border_m, *arr_L, *arr_R;
    bool dir;

    arr_R = (int*)malloc(len2 * sizeof(int));
    arr_L = (int*)malloc(len1 * sizeof(int));
    
    for (i = 0; i < len1; i++)
        arr_L[i] = a[border_l + i];
    for (i = 0; i < len2; i++)
        arr_R[i] = a[border_m + 1 + i];
    
    for (i = 0, j = 0, k = border_l; i < len1 && j < len2; k++) {
        dir = arr_L[i] <= arr_R[j];

        if (!dir) {
            a[k] = arr_R[j];
            j++;
        }
        else {
            a[k] = arr_L[i];
            i++;
        }
    }
    
    for (; i < len1; i++, k++)
        a[k] = arr_L[i];
    for (; j < len2; j++, k++)
        a[k] = arr_R[j];

    free(arr_R);
    free(arr_L);
}

void do_merging_sort(int border_l, int border_u, int *a) {
    int border_m;
    bool flag = border_l < border_u;
    
    if (flag) {
        border_m = (border_u - border_l) / 2 + border_l;

        do_merging_sort(border_l, border_m, a);

        do_merging_sort(border_m + 1, border_u, a);

        do_merging(border_l, border_m, border_u, a);
    }
}

void sort(void) {
    int i;
    clock_t time1, time2, time_spent;

    if (DEBUG)
        printf("\nStart sorting!\n");

    time1 = clock();
    for (i = 0; i < thrds_qnt; i++)
        sem_post(&sem);
    pthread_join(threads[0], 0);
    time2 = clock();

    if (DEBUG)
        printf("\nSorting finished!\n");

    f = fopen(ouf, "wb");
    fprintf(f, "%d\n", thrds_qnt);
    fprintf(f, "%d\n", num_qnt);
    i = 0;
    if (INPUT_SHOW && DEBUG) {
        printf("\nRes array: \n");
        while (i < num_qnt)
            printf("%d ", array[i++]);
        i = 0;
        printf("\n");
    }
    while (i < num_qnt) {
        fprintf(f, "%d ", array[i++]);
    }
    fclose(f);

    f = fopen(f_time, "wb");
    time_spent = (time2 - time1) / (CLOCKS_PER_SEC / 1000);
    if (DEBUG)
        printf("\nNow in out.\nSorting time: %u\n", time_spent);
    fprintf(f, "%u", time_spent);
    fclose(f); 
}
