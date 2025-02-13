// Simulation of users depositing, withdrawing and transferring money from diffrent accounts. You can choose how much users and accounts to create 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#define SUCCESS 0
#define FAILURE 1

// Data typ describing the data returned by bank operations (deposits, withdrawls and transfers)
struct operation_data {
    int send_money;
    int src_user;
    int dst_account;
    int source_account_balance;
    int destination_account_balance;
    int success_or_failure;
};

// Initialization of users, accounts and their data
int usersAmount, accountsAmount, operationAmout;
int *usersIdArray, *usersTransferArray, *accountsIdArray, *accountsBalanceArray;

// Initialization of mutex'es 
pthread_mutex_t *mutexes;

// InitializationBarrier for threads
pthread_barrier_t barrier; 

// Initialization of an array containing bank users
pthread_t *bankUsers;

// Generating users, accounts and filling their data
void bank_data_generation() {
    if ((accountsAmount > usersAmount) || accountsAmount < 2) {
        printf("Amount of accounts has to be smaller than amount of bank users and grater than 2.\n");
        exit(EXIT_FAILURE);
    }
    else {
        // Filling arrays with data
        printf("You've choosen to create %d users and %d accounts.\n", usersAmount, accountsAmount);
        for (int i = 0; i < usersAmount; i++) {
            usersIdArray[i] = i; 
        }
        for (int i = 0; i < accountsAmount; i++) {
            accountsIdArray[i] = i; 
            accountsBalanceArray[i] = 0;
        }
    }
}

// Random intiger from 0 to amout of accounts
int random_int() {
    int random_intiger = rand();
    int random_from_0_to_AccAmount = (random_intiger % accountsAmount);
    return random_from_0_to_AccAmount;
}

// Function describing deposit operation 
struct operation_data depositing_money(int srcUser) { 
    int transferedMoney = (rand() % 1000) + 1;
    int dstAccount = random_int();
    pthread_mutex_lock(&mutexes[dstAccount]);
    accountsBalanceArray[dstAccount] += transferedMoney;
    struct operation_data deposit_data =  {.send_money = transferedMoney, 
                                            .src_user = srcUser, 
                                            .dst_account = accountsIdArray[dstAccount], 
                                            .source_account_balance = accountsBalanceArray[srcUser], 
                                            .destination_account_balance = accountsBalanceArray[dstAccount],
                                            .success_or_failure = SUCCESS};
    pthread_mutex_unlock(&mutexes[dstAccount]);
    return deposit_data;
}

// Function describing withdraw operation
struct operation_data withdrawing_money(int srcUser) {  
    while(1) {
        int transferedMoney = (rand() % 1000) + 1;
        int dstAccount = random_int();
        // User can't withdraw money from account which will have negative balance
        if (accountsBalanceArray[dstAccount] - transferedMoney >= 0) { 
            pthread_mutex_lock(&mutexes[dstAccount]);
            accountsBalanceArray[dstAccount] -= transferedMoney;
            struct operation_data withdraw_data = {.send_money = transferedMoney, 
                                                .src_user = srcUser, 
                                                .dst_account = accountsIdArray[dstAccount], 
                                                .source_account_balance = accountsBalanceArray[srcUser], 
                                                .destination_account_balance = accountsBalanceArray[dstAccount],
                                                .success_or_failure = SUCCESS};
            pthread_mutex_unlock(&mutexes[dstAccount]);
            return withdraw_data;
        }
        else {
            continue;
        }
    }
}

// Function describing transfer operation
struct operation_data transferring_money(int srcUser) {  
    while(1) {
        int transferedMoney = (rand() % 1000) + 1;
        int srcAccount = random_int();
        int dstAccount = random_int();
        // User can't transfer money from account which will have negative balance, also it can't perform transfer operation on single account 
        if ((dstAccount != srcAccount) && (accountsBalanceArray[srcAccount] - transferedMoney) >= 0) { 
            int lock1 = pthread_mutex_trylock(&mutexes[dstAccount]);
            int lock2 = pthread_mutex_trylock(&mutexes[srcAccount]);
            if (lock1 == 0 && lock2 == 0) {
                accountsBalanceArray[dstAccount] += transferedMoney;
                accountsBalanceArray[srcAccount] -= transferedMoney;
                struct operation_data transfer_data = {.send_money = transferedMoney, 
                                                    .src_user = srcUser, 
                                                    .dst_account = accountsIdArray[dstAccount], 
                                                    .source_account_balance = accountsBalanceArray[srcAccount], 
                                                    .destination_account_balance = accountsBalanceArray[dstAccount],
                                                    .success_or_failure = SUCCESS};
                pthread_mutex_unlock(&mutexes[dstAccount]);
                pthread_mutex_unlock(&mutexes[srcAccount]);      
                return transfer_data;
            }
            else {
                if (lock1 == 0) pthread_mutex_unlock(&mutexes[dstAccount]);
                if (lock2 == 0) pthread_mutex_unlock(&mutexes[srcAccount]);   
                continue;    
            }
        }
        else {       
            continue; 
        }    
    }
}

// Function describing what every thread will do 
void* bank_user_activity(void* ids) {
    int id = *(int *)ids; 
    int amount_of_deposits = 0;
    int amount_of_withdraws = 0; 
    int amount_of_transfers = 0; 
    // Waiting for all threads
    pthread_barrier_wait(&barrier); 
    while(1) {
        // User identification 
        if ((id % 3) == 0) { // Users of id modulo 3 equal to 0 will perform only deposit operation
            struct operation_data deposit = depositing_money(id);
            if (deposit.success_or_failure == SUCCESS) {
                amount_of_deposits++;
                printf("User %d deposited %d $ to account no. %d. Deposited account balance: %d.\n",
                                                                                deposit.src_user, 
                                                                                deposit.send_money, 
                                                                                deposit.dst_account, 
                                                                                deposit.destination_account_balance);
                usleep(100);
            }
                        else {
                continue;
            }
            // if the user performs the operation the selected number of times, we stop the loop
            if (amount_of_deposits == operationAmout) {
                break;
            }
        }
        if ((id % 3) == 1) { // Users of id modulo 3 equal to 1 will perform only withdraw operation
            struct operation_data withdraw = withdrawing_money(id);
            if (withdraw.success_or_failure == SUCCESS) {
                amount_of_withdraws++; 
                printf("User %d withdrawed %d $ from account no. %d. Withdrawed account balance: %d.\n",
                                                                                withdraw.src_user, 
                                                                                withdraw.send_money, 
                                                                                withdraw.dst_account, 
                                                                                withdraw.destination_account_balance);               
                usleep(100);
            }
            else {
                continue;
            }
            // if the user performs the operation the selected number of times, we stop the loop
            if (amount_of_withdraws == operationAmout) {
                break;  
            }
        }
        if ((id % 3) == 2) { // Users of id modulo 3 equal to 2 will perform only transfer operation
            struct operation_data transfer = transferring_money(id);
            if (transfer.success_or_failure == SUCCESS) {
                amount_of_transfers++;
                printf("User %d transfered %d $ from account no. %d to account no. %d. Source account balance: %d, destination account balance: %d.\n",
                                                                            transfer.src_user, 
                                                                            transfer.send_money, 
                                                                            transfer.src_user,
                                                                            transfer.dst_account, 
                                                                            transfer.source_account_balance,
                                                                            transfer.destination_account_balance);
            }
            else {
                continue;
            }
            // if the user performs the operation the selected number of times, we stop the loop
            if (amount_of_transfers == operationAmout) {
                break;
            }
            usleep(100);
        }
    }
    return NULL;
}

int main(void) {
    printf("Choose amount of bank users.\n");
    scanf("%d", &usersAmount);
    printf("Choose amount of accounts (amount of accounts has to be smaller than amount of bank users and greater than 2).\n");
    scanf("%d", &accountsAmount);
    printf("Choose amount of operations that single user is supposed to do.\n");
    scanf("%d", &operationAmout);
    // Dynamic memory allocation for all arrays
    usersIdArray = malloc(usersAmount * sizeof(int));
    usersTransferArray = malloc(usersAmount * sizeof(int));
    accountsIdArray = malloc(accountsAmount * sizeof(int));
    accountsBalanceArray = malloc(accountsAmount * sizeof(int));
    bank_data_generation();
    mutexes = malloc(accountsAmount * sizeof(pthread_mutex_t));
    bankUsers = malloc(usersAmount * sizeof(pthread_t));
    // Mutexes initjialization 
    for (int i = 0; i < accountsAmount; i++) {
        pthread_mutex_init(&mutexes[i], NULL);
    }
    // Threads synchroniztion with barrier
    pthread_barrier_init(&barrier, NULL, usersAmount);
    // Creating threads
    for (int i = 0; i < usersAmount; i++) {
        pthread_create(&bankUsers[i], NULL, bank_user_activity, &usersIdArray[i]);
    }
    // Closing of threads
    for (int i = 0; i < usersAmount; i++) {
        pthread_join(bankUsers[i], NULL);
    }
    // Deleting of mutexes 
    for (int i = 0; i < accountsAmount; i++){
        pthread_mutex_destroy(&mutexes[i]);
    }
    // Deleting barrier 
    pthread_barrier_destroy(&barrier);
    // Freeing alocated memory
    free(usersIdArray);
    free(usersTransferArray);
    free(accountsIdArray);
    free(accountsBalanceArray);
    free(bankUsers);
    free(mutexes);
    return 0;
}