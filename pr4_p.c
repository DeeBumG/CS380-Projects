#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <openssl/sha.h>
#include <limits.h>
#include "uthash.h"

#define USERNAME_LEN 64
#define SHA256_DIGEST_LENGTH 32

/**
 * @brief Represents a hashtable for storing pending credit.
 * 
 * @param username name of user.
 * @param pending_credit amount of pending credit.
 * @param hh makes this structure hashable
 */

typedef struct hashtable_t {                   
    char username[USERNAME_LEN]; //key
    uint64_t pending_credit; //value
    UT_hash_handle hh;  
} hashtable_t;


/**
 * @brief Represents a transaction.
 * 
 * @param created_at The datetime at which the user created this transaction.
 * @param sender The sender's username.
 * @param recipient The recipient's username.
 * @param amount The amount transferred from sender to recipient.
 */
typedef struct transaction_t {
    time_t created_at;
    char sender[USERNAME_LEN];
    char recipient[USERNAME_LEN];
    uint64_t amount;
} transaction_t;

/**
 * @brief Represents a block in the blockchain.
 * 
 * @param transaction The transaction this block contains.
 * @param proof_of_work A number such that the hash of the block_t contains
 * some number of leading zeros. It has no meaning other than as part of the
 * hash.
 */
typedef struct block_t {
    transaction_t transaction;
    uint64_t proof_of_work;
} block_t;


//setting global variables ----

//number of threads used
int nthreads = 1;
//number of transactions (+ header)
int numelems = 0;
//array to store strings of completed blocks
char **res_arr;
//array of input transactions
transaction_t *arr = NULL;
//initializing hashtable
hashtable_t *hashtable = NULL;

//------------------------------

//assume each line in the CSV is fewer than 256 characters
int read_transactions(char *filename, transaction_t **arr, int *length) {
    // check for bad inputs.
    if (!filename || !arr || !length){return 1;}
    
    // open file:
    FILE *file = fopen(filename, "r");

    //check if there was an error opening the file (file would be NULL).
    if (!file){return 2;}

    //count the number of lines:
    int num_lines = 0;
    char line[256] = {0};
    while (NULL != fgets(line, sizeof(line), file)) {
        num_lines ++;
    }

    //go back to the beginning of the file:
    rewind(file);

    //create the result array and set the length:
    *arr = calloc(num_lines, sizeof(transaction_t));
    *length = num_lines;

    //fill the result array:
    //Need to convert each line to the transaction fields in transaction_t

    int idx = 0, created = 0, money = 0, i = 0, slen = 0;
    char sendr[USERNAME_LEN] = {0};
    char recvr[USERNAME_LEN] = {0};

    while (NULL != fgets(line, sizeof(line), file)) {

        //Reset variables for each line
        created = 0;
        money = 0;
        i = 0;
        slen = 0;

        //setting created (time) field
        created += atoi(&line[i]);

        //move i to end of field, which is the ','
        for (; i < 256; i++){
            if (line[i] == ','){break;}
        }

        //incrementing i to first index of the sender field
        i++;

        //i is now index of the first ','; now set the sender field; slen is used as index and length (at the end)
        for (; i < 256; i++){
            if (line[i] == ','){break;}
            sendr[slen] = (char)(line[i]);
            slen++;
        }

        //setting last null char for sendr
        sendr[slen] = '\0';

        //Setting sender using the length calculated above
        strncpy(((*arr)[idx]).sender, sendr, slen);

        //resetting slen for second string; receiver
        slen = 0;

        //incrementing i; is now at first index of the receiver field
        i++;

        //moving each character into the local recvr string
        for (; i < 256; i++){
            if (line[i] == ','){break;}
            recvr[slen] = (char)(line[i]);
            slen++;
        }

        //setting last null char for recvr
        recvr[slen] = '\0';

        //incrementing i, is now at the first index of the last csv field
        i++;

        //Setting money (amount)
        money += atoi(&line[i]);

        //Setting all remaining fields of the transaction_t equal to local variables; sender was set above
        ((*arr)[idx]).created_at = (time_t)created;
        //setting recipient using the slen calculated above
        strncpy(((*arr)[idx]).recipient, recvr, slen);
        ((*arr)[idx]).amount = (uint64_t)money;
        //incrementing index for the array as we are moving to a new line in the csv
        idx++;
    }

    // Closing the file
    fclose(file);
    return 0;
}

//mines a block for each transaction
void * mine_blocks(void * rank) {
    //thread number
    long myrank = (long)(rank);
    //work each thread will do
    long mywork = numelems / nthreads;
    //start index in transaction array
    long start = myrank * mywork;
    //end index in transaction array
    long end = start + mywork;
    //counter for iteration
    long k;
    //account for uneven distribution of work
    if (myrank == nthreads - 1){
        end = numelems;
    }
    
    //outer loop (mines a block for each transaction in mywork)
    for (k = start; k < end; k++){

        //initialize block
        block_t block;
        //initialize counter variables
        uint64_t i, j, max = UINT64_MAX;
        //variable to determine if valid hash exists
        int exhausted = 1;

        //set block to 0s
        memset(&block, 0, sizeof(block_t));
    
        //set block's transaction to the current transaction
        block.transaction = arr[k];
        
        //set proof of work to 0
        block.proof_of_work = 0;

        //inner for loop (iterates through proof of work until block is mined)
        for (i=0; i < max; i++){
            //increment proof of work
            block.proof_of_work = i;
            //initialize hash digest
            unsigned char digest[SHA256_DIGEST_LENGTH];
            //calculate hash digest of transaction + proof of work
            SHA256((unsigned char *)&block, sizeof(block_t), digest);

            //if valid digest is found...
            if (digest[0] == 0 && digest[1] == 0 && digest[2] == 0){
                //create temporary string to store the hash digest
                char *digest_str = malloc(SHA256_DIGEST_LENGTH * 2 + 1);
                //allocate space for string in the result array to store transaction + digest
                res_arr[k] = (char *)malloc(sizeof(block) + sizeof(digest_str) + 1);

                //iterate through digest and convert to string
                for (j=0; j < SHA256_DIGEST_LENGTH; j++){
                    sprintf(digest_str + j * 2, "%02hhx", digest[j]);
                }
                //set last char to \0
                digest_str[SHA256_DIGEST_LENGTH * 2] = '\0';

                //add all transaction + proof of work + digest string to result array
                sprintf(
                    res_arr[k],
                    "%ld,%s,%s,%lu,%lu,%s", 
                    block.transaction.created_at, 
                    block.transaction.sender, 
                    block.transaction.recipient, 
                    block.transaction.amount,
                    block.proof_of_work,
                    digest_str
                );

                //free temporary digest string
                exhausted = 0;
                free(digest_str);
                break;
            }
        }

        //error handling (if no valid digest is found)
        if (exhausted){
            res_arr[k] = (char *)malloc(sizeof(arr[k]) + 44);
            sprintf(
                res_arr[k],
                "%s%ld,%s,%s,%lu",
                "block mining unsuccessful for transaction: ",
                arr[k].created_at,
                arr[k].sender,
                arr[k].recipient,
                arr[k].amount
            );
        }
    }
    return NULL;
}


//checks for correct usage
int help(int argc, char *argv[]){
    if (argc == 1){
        printf("\nUsage: pr4 filename numthreads\n\nEnter pr4 -h for usage examples\n\n");
        return 0;
    }
    else if (argc > 3){
        printf("\nTakes two arguments. (%d) arguments were given\n\nUsage: pr4 filename numthreads\n\nEnter pr4 -h for usage examples\n\n", (argc - 1));
        return 0;
    }
    else if ((!(strncmp(argv[1], "-h", 2))) || (!(strncmp(argv[1], "--help", 6)))){
        printf("\npr4: Takes a CSV file and the number of threads to use as input and prints a list of mined blocks for each transaction, as well as pending balances.\n\n");
        printf("CSV file must be in the format: created_at,sender,recipient,amount.\n\n");
        printf("It is recommended the number of threads used be less than or equal to the available cores on your computer.\n\n");
        printf("Usage: pr4 [filename] [numthreads]. Replace [filename] with the name of the CSV file and [numthreads] with the number of threads to use.\n\n");
        return 0;
    }
    return 1;
}

//calculate pending credit using hashtable
void calculate_pending_credit(transaction_t *arr, int *arrlength) {
    int i;
    for (i=1; i < *arrlength; i++){
        hashtable_t *s;
        HASH_FIND_STR(hashtable, arr[i].recipient, s);
        if (s == NULL) {
            s = (hashtable_t *)malloc(sizeof *s);
            strncpy(s->username, arr[i].recipient, sizeof(arr[i].recipient) - 1);
            s->pending_credit = arr[i].amount;
            HASH_ADD_STR(hashtable, username, s);
        }
        else {
            s->pending_credit += arr[i].amount;
        }
    }
}

//iterate through hashtable to print pending credit and free hashtable memory
void iterate_hashtable() {
    hashtable_t *s;
    printf("%s\n", "username,pending_credit");
    for (s = hashtable; s != NULL; s = s->hh.next) {
        printf("%s,%lu\n", s->username, s->pending_credit);
        HASH_DEL(hashtable, s);
        free(s);
    }
}

//main is called with two arguments: CSV filename, num threads
int main(int argc, char *argv[]) {

    //initializing counter variable
    int i;

    //call help to ensure correct usage
    if (help(argc, argv)){

        //set number of threads
        nthreads = strtol(argv[2], NULL, 10);

        //allocate thread array
        pthread_t *thread_array = malloc(nthreads * sizeof(pthread_t));

        //reads through provided CSV, builds array of transactions
        //sets numelems correctly
        read_transactions(argv[1], &arr, &numelems);
        
        //allocate space for result array (stores output strings)
        res_arr = (char **)malloc(numelems * sizeof(char *));

        //ensure only 1 thread per element at most
        if (numelems < nthreads){
            nthreads = numelems;
        }

        //print header
        printf("%s", "created_at,sender,recipient,amount,proof,digest\n");

        //create threads, each thread calls mine_blocks
        for (i=0; i < nthreads; i++){
            pthread_create(&thread_array[i], NULL, mine_blocks, (void *)i);
        }

        //join threads when work is complete
        for (i = 0; i < nthreads; i++) {
            pthread_join(thread_array[i], NULL);
        }

        //iterate through result array when complete, print out lines in order
        //frees memory for each string in the array
        free(res_arr[0]);
        for (i=1; i < numelems; i ++){
            printf("%s\n", res_arr[i]);
            free(res_arr[i]);
        }

        //builds hashtable of pending credit for recipients
        calculate_pending_credit(arr, &numelems);
        
        //iterates through, prints content, and deletes / frees hashtable
        iterate_hashtable();

        //free threads
        free(thread_array);
        //free output array
        free(res_arr);
    }

    //freeing memory used for arr
    free(arr);
   
    return 0;
}
