#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

pthread_mutex_t **bufManip;  // array for the mutexes
int buffer_number;  // how many publisher types are there
char **package;  // books thats will be stored in this package
pthread_mutex_t package_mutex = PTHREAD_MUTEX_INITIALIZER;  // mutex for package buffer
int size_package;

typedef struct thread  // struct for publisher thread
{
    pthread_t tid;  // thread id
    int num_of_created_books;  // counter for how many books have been created
    int max_num_of_books;  // goal of book count for thread
}thread;

typedef struct publisher_type
{
    int type;
    int bookCount;
    int buffer_size;  // indiates the size of books buffer
    thread **threads_array;  // publisher threads for current publisher type
    char **buffer;  // holds the pubilshed books for packagers
    int is_finished;
}publisher_type;
publisher_type **publishers;

typedef struct packager{
    pthread_t tid;  // thread id 
    int number;  // id of packager
    publisher_type **publisher_list;
}packager;

// to pass publish book function as an arg
typedef struct book{
    int ptype;  // publisher type
    int pno;  // publisher number
    int total_no_book;
    publisher_type *pblshr_strct;  // publisher struct 
    char **book_name; // book string booki_j
}book;

void double_buffer_size(book *arg){  // double buffer size in case packagers are not fast enough 
    printf("Publisher %d of type %d\tBuffer is full. Resizing the buffer.\n", arg->pno, arg->ptype);  // may show up delayed but if sleep is added to publishers the order is correct
    arg->pblshr_strct->buffer_size *= 2;
    arg->pblshr_strct->buffer = realloc(arg->pblshr_strct->buffer, sizeof(char *)*arg->pblshr_strct->buffer_size);
    if (arg->pblshr_strct->buffer == NULL){
        printf("null dur\n");
    }
}

void *publish_book(void *arg){  // function for publisher threads
    /**here the lock or semaphore will start. we need publisher type thread no and buffer
     * First check whehter the publisher_type buffer is intiliazed if not create it then put in it
     * if publisher buffer is full double the size
     * for loop burada olacak max number of books to create
    */
    book *my_arg = (book *) arg;  // get the arguments for the books to be created
    int i;
    for (i = 0; i< my_arg->total_no_book; i++){  // loop until goal of book numbers is reached
        int a = my_arg->ptype -1;
        pthread_mutex_lock(bufManip[a]);  // lock the mutex for critical sextion
        char *book_str = (char *)malloc(sizeof(char)*20);
        if (my_arg->pblshr_strct->buffer == NULL){  // create the publisher types buffer if it doesn't exist yet
            my_arg->pblshr_strct->buffer =  malloc( my_arg->pblshr_strct->buffer_size *sizeof(char *));
        }

        sprintf(book_str,"Book%d_%d", (int) my_arg->ptype, (int) my_arg->pblshr_strct->bookCount);
        my_arg->pblshr_strct->bookCount++;
        int index, is_empty = 0;

        //checking for empty slot in the buffer
        for(index = 0; index < my_arg->pblshr_strct->buffer_size; index++){
            if (my_arg->pblshr_strct->buffer[index] != NULL){
                continue;
            }
            else
            {
                is_empty = 1;
                break;
            } 
        }
        
        //if there is no empty slot in buffer resize the buffer
        if (!is_empty){
            double_buffer_size(my_arg);
        }

        my_arg->pblshr_strct->buffer[index] = book_str;  // adding the book to our buffer

        printf("Publisher %d of type %d\t%s is published and put into the buffer %d\n", my_arg->pno, my_arg->ptype, book_str, my_arg->ptype);
        pthread_mutex_unlock(bufManip[a]);  // done with shared data access
    }
    printf("Publisher %d of type %d\tFinished publishing %d books. Exiting the system.\n",  my_arg->pno, my_arg->ptype, my_arg->total_no_book);
}

void *package_book(void *arg){  // function for the packager threads
    /*
    *   if packager is done -> Packager 3 There are no publishers left in the system. Only 1 of 6
    *   number of books could be packaged. The package
    *   contains Book2_15. Exiting the system.
    */
    packager *my_arg = (packager *) arg;  // get the arguments for the books to be created

    int counter = 0;
    while (1){
        if (counter == 60)
            break;
        counter++;
        pthread_mutex_lock(&package_mutex);
        if (my_arg->publisher_list[buffer_number-1]->is_finished == 1){
            // tbd add more detail to print
            printf("Packager %d\tThere are no publishers left in the system.\n\t\t", my_arg->number);
            int j;
            for (j=0; j<size_package;j++){
                if (package[j]==NULL){break;}
                printf("%s, ",package[j]);
            }
            printf(".\n");
            break;
        }
        else{
            //pthread_mutex_lock(&package_mutex);
            int i, is_full=1;
            for (i=0; i<size_package;i++){
                if (package[i] == NULL){
                    is_full = 0;
                    break;
                }
            }
            if (is_full){
                printf("Packager %d \t Finished preparing one package. The package contains: \n\t\t\t", my_arg->number);
                // int j;
                // for (j=0; j<size_package;j++){
                //     printf("%s, ",package[j]);
                //     package[j] = NULL;
                // }
                // printf(".\n");
            }
            else{
                int random_buffer_index = rand() % buffer_number;
                pthread_mutex_lock(bufManip[random_buffer_index]);
                
                char *pubBuff = my_arg->publisher_list[random_buffer_index]->buffer[0];
                if (pubBuff == NULL){
                    //if its null just pass
                }else{
                    package[i] = pubBuff;
                    printf("Packager %d\t Put %s into the package.\n", my_arg->number, package[i]);

                    int j;
                    for (j = 0; j<publishers[random_buffer_index]->buffer_size-1; j++){        
                        publishers[random_buffer_index]->buffer[j] = publishers[random_buffer_index]->buffer[j+1];
                    }
                    publishers[random_buffer_index]->buffer[j] = NULL;
                }
                pthread_mutex_unlock(bufManip[random_buffer_index]);
            }
            is_full = 1;
            for (i=0; i<size_package;i++){
                if (package[i] == NULL){
                    is_full = 0;
                    break;
                }
            }
            if (is_full){
                printf("Packager %d \t Finished preparing one package. The package contains: \n\t\t", my_arg->number);
                int j;
                for (j=0; j<size_package;j++){
                    printf("%s, ",package[j]);
                    package[j] = NULL;
                }
                printf(".\n");
            }

            //pthread_mutex_unlock(&package_mutex);
        }
        pthread_mutex_unlock(&package_mutex);
    } 
    pthread_mutex_unlock(&package_mutex);
}


int main(int argc, char* argv[]) {
    if ( argc != 10 ){return 1;}
    
    int pubTypeCount = atoi(argv[2]), pubCount = atoi(argv[3]), packCount = atoi(argv[4]);
    buffer_number = pubTypeCount;
    size_package = atoi(argv[8]);
    bufManip = (pthread_mutex_t **)malloc(sizeof(pthread_mutex_t *) * pubTypeCount);
    package = (char**) calloc( atoi(argv[8]), sizeof(char) *20);
    publishers = malloc(sizeof(publisher_type*) * pubTypeCount);
    packager *packager_list[packCount];
    int i, j , k;
    for (i = 1; i<=pubTypeCount; i++){  // create required number of publisherTypes
        publisher_type *publisher = (publisher_type *)malloc(sizeof(publisher_type));
        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
        bufManip[i-1] = &mutex;  // set mutex to global buffer according to publisherType number
        publishers[i-1] = publisher;
        publisher->bookCount = 1;
        publisher->threads_array = (thread **) malloc(sizeof(thread) * pubCount);  // set publisher threads array size of publisherType
        publisher->buffer_size = atoi(argv[9]);  // set initial size of publisherType buffer
        publisher->is_finished = 0;
        publisher->type = i;
        for (j = 1; j <= pubCount; j++){  // create publisher threads
            book *my_arg = (book *)malloc(sizeof(book));
            my_arg->ptype = i;my_arg->pno = j;my_arg->pblshr_strct = publisher, my_arg->total_no_book = atoi(argv[6]);  // set args for books of this publisherType and publisher thread
            pthread_t thread_no;  // initialize new publisher thread
            thread *thread_strc = (thread *) malloc(sizeof(thread));
            thread_strc->tid = thread_no;
            thread_strc->num_of_created_books = 0;
            thread_strc->max_num_of_books = atoi(argv[6]);  // set how many books the publisher needs to publish
            publisher->threads_array[j-1] = thread_strc;  // add publisher thread to publisherType threads array
            pthread_create(&thread_no, NULL, &publish_book, (void *)my_arg);  // start the newly created publisher thread
        }
    }
    for (k = 1; k <= packCount; k++){  // create packager threads
        pthread_t thread_no;  // init packager thread
        packager *packager_strc = (packager *) malloc(sizeof(packager));
        packager_strc->tid = thread_no; packager_strc->number = k;  // assign data to the packager
        packager_list[k-1] = packager_strc;
        packager_strc->publisher_list = publishers;
        pthread_create(&thread_no, NULL, &package_book, (void *)packager_strc);  // launch the packager thread
    }

    /* 
    pthread_join publisher threads in for loop
    if all threads from a publisher type have been joined remove publisher from publishers list
    */

    for (i = 0; i < pubTypeCount; i++){
        for (j = 0; j < pubCount; j++){
            pthread_join(publishers[i]->threads_array[j]->tid, NULL);
        }
        publishers[i]->is_finished = 1;
    }
    
    for (k = 0; k < packCount; k++){  // when all publishers have been joined, join packager threads with pthread_join
        pthread_join(packager_list[k]->tid, NULL);
    }

    pthread_exit(0);
    return 0;
}
