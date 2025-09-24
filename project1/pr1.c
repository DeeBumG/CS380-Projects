#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#define USERNAME_LEN 80

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
 * @brief Represents an account balance.
 * 
 * @param username The account holder's username.
 * @param amount The account balance.
 */
typedef struct balance_t {
    char username[USERNAME_LEN];
    uint64_t amount;
} balance_t;

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

int compare_times(const void *a, const void *b) {
    time_t *x = (time_t *)a;
    time_t *y = (time_t *)b;
    return *x - *y;
}

int calculate_balances(balance_t **dict, transaction_t **arr, int arrlen, int *dictlength){
    int i, j, k, dictlen = 0;
    char* user;

    //setting dictionary variable and allocating memory
    *dict = calloc((2 * arrlen), sizeof(balance_t));

    //loop through array of transactions to initialize balances to 0
    for (i=1; i < arrlen; i++){
        user = (*arr)[i].sender;
        for (j=0; j <= dictlen; j++){
                //check to see if the sender is system; don't track system balance
                if (!(strcmp(user, "system"))){
                    break;
                }
                //if end of dictionary is reached and account is not found, add account to dictionary
                //after adding account increment dictionary length
                else if (j == dictlen){
                    strncpy((*dict)[j].username, user, strnlen(user, 100)); 
                    (*dict)[j].amount = 0;
                    dictlen ++;
                    break;
                }
                //if recipient account is found, break
                else if (!(strcmp((*dict)[j].username, user))){
                    break;
                }
            }
        user = (*arr)[i].recipient;
        for (j=0; j <= dictlen; j++){
                //if end of dictionary is reached and account is not found, add account to dictionary
                //after adding account increment dictionary length
                if (j == dictlen){
                    strncpy((*dict)[j].username, user, strnlen(user, 100)); 
                    (*dict)[j].amount = 0;
                    dictlen ++;
                    break;
                }
                //if recipient account is found, break
                else if (!(strcmp((*dict)[j].username, user))){
                    break;
                }
            }
    }

    //looping through the array of transactions again to calculate balances
    for (i=1; i < arrlen; i++){
        //if the sender is system: add balance to recipient account
        if (!(strcmp((*arr)[i].sender, "system"))){
            //loop through dictionary to find recipient account
            for (j=0; j < dictlen; j++){
                //when recipient account is found, add amount to the account
                if (!(strcmp((*dict)[j].username, (*arr)[i].recipient))){
                    (*dict)[j].amount += (*arr)[i].amount;
                    break;
                }
            }
        }
        //if sender is not system: check that transaction is valid and execute transaction
        else {
            //loop through dictionary to find sender
            for (j=0; j < dictlen; j++){
                //if sender is found, check that balance is greater than transaction amount
                if (!(strcmp((*dict)[j].username, (*arr)[i].sender))){
                    if ((*dict)[j].amount >= (*arr)[i].amount){
                        //if valid, subtract amount from sender account
                        (*dict)[j].amount -= (*arr)[i].amount;
                        //loop through dictionary again to find recipient account
                        for(k=0; k < dictlen; k++){
                            if (!(strcmp((*dict)[k].username, (*arr)[i].recipient))){
                                //if recipient account is found, add amount to recipient
                                (*dict)[k].amount += (*arr)[i].amount;
                                break;
                            }
                        }
                    }
                    //break if sender is found
                    break;
                }
            }
        }
    }

    //set dictlength input parameter
    *dictlength = dictlen;
    //return
    return 0;
}

//checks for correct usage
int help(int argc, char *argv[]){
    if (argc == 1){
        printf("\nUsage: pr1 filename\n\nEnter pr1 -h for usage examples\n\n");
        return 0;
    }
    else if (argc > 2){
        printf("\nTakes one argument. (%d) arguments were given\n\nUsage: pr1 filename\n\nEnter pr1 -h for usage examples\n\n", (argc - 1));
        return 0;
    }
    else if ((!(strcmp(argv[1], "-h"))) || (!(strcmp(argv[1], "--help")))){
        printf("\npr1: Takes a CSV file as input and prints a list of sorted transactions and ending account balances.\n\n");
        printf("CSV file must be in the format: created_at,sender,recipient,amount.\n\n");
        printf("Usage: pr1 [filename]. Replace [filename] with the name of the CSV file.\n\n");
        return 0;
    }
    return 1;
}

//main is called with one argument: CSV filename
int main(int argc, char *argv[]) {
    //initializing array of transaction_t structs
    transaction_t *arr = NULL;
    //initializing dictionary (array) of blance_t structs
    balance_t *dict = NULL;
    //initializing array length and dictionary length
    int arrlength = 0, dictlength = 0, i;
    //calling help to ensure correct usage
    if (help(argc, argv)){
        //calling read_transactions passing in the initialized variables and the name of the CSV file
        read_transactions(argv[1], &arr, &arrlength);
        //sorting the transactions based on time, using the compare_times function and qsort
        qsort(arr, arrlength, sizeof(transaction_t), compare_times);
        //calculating balances based on trancactions array from above. dictlength and dict of balances will be set after calling
        calculate_balances(&dict, &arr, arrlength, &dictlength);
        //printing sorted transactions
        printf("created_at,sender,recipient,amount\n");
        for (i=1; i < arrlength; i++){
            printf("%ld,%s,%s,%lu\n", arr[i].created_at, arr[i].sender, arr[i].recipient, arr[i].amount);
        }
        //printing final account balances
        printf("username,balance\n");
        for (i=0; i < dictlength; i++){
            printf("%s,%lu\n", dict[i].username, dict[i].amount);
        }
    }
    //freeing memory used for dict and arr
    free(dict);
    free(arr);
    //return
    return 0;
}
