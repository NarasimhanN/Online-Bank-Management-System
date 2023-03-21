
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <crypt.h>
#include "./dataStruct.h"
#include "./admin-credentials.h"
#include "./constants.h"

//-------------------definition of function-----------------------------------
bool login_handler(bool isAdmin, int connFD, struct Customer *ptrToCustomer);
bool get_account_details(int connFD, struct Account *customerAccount);
bool get_customer_details(int connFD, int customerID);

//For Login verification of Admin and Customer 
bool login_handler(bool isAdmin, int connFD, struct Customer *ptrToCustomerID)
{
    ssize_t readBytes, writeBytes;            // Number of bytes written to / read from the socket
    char readBuffer[1000], writeBuffer[1000]; // Buffer for reading from / writing to the client
    char tempBuffer[1000];
    struct Customer customer;

    int cid;

    bzero(readBuffer, sizeof(readBuffer));
    bzero(writeBuffer, sizeof(writeBuffer));

    // Get login message for respective user type
    if (isAdmin)
        strcpy(writeBuffer, "\n\n\n\n\t\tAre you an Admin??\n\n\t\tEnter your credentials to Prove yourself :) \n\n\n\t\t");
    else
        strcpy(writeBuffer, "\n\n\n\n\t\tWelcome Customer!!!\n\n\t\tEnter your credentials to login to your account :) \n\n\n\t\t");

    // Append the request for LOGIN ID message
    //strcat(writeBuffer, "\n");
    strcat(writeBuffer, LOGIN_ID);

    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("\nERRRO_LOG: Error writing message to the client!");
        return false;
    }

    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("\nERROR_LOG : Error reading login ID from client!");
        return false;
    }

    bool userFound = false;
    int x = (strchr(readBuffer, '-') == NULL) ? 0 : 1;
    if (x == 0 && strcmp(readBuffer, ADMIN_LOGIN_ID) != 0)
    {
        return false;
    }
    if (isAdmin)
    {
        if (strcmp(readBuffer, ADMIN_LOGIN_ID) == 0)
            userFound = true;
    }
    else
    {

       // printf("%d\n", x);
       if(strcmp(readBuffer,ADMIN_LOGIN_ID)==0)
       {
        userFound=false;
        return false;
       }
        bzero(tempBuffer, sizeof(tempBuffer));
        strcpy(tempBuffer, readBuffer);
        strtok(tempBuffer, "-");

        cid = atoi(strtok(NULL, "-"));

        int customerFileFD = open(CUSTOMER_FILE, O_RDONLY);
        if (customerFileFD == -1)
        {
            perror("\nERROR_LOG : Error opening customer file in read mode!");
            return false;
        }

        off_t offset = lseek(customerFileFD, cid * sizeof(struct Customer), SEEK_SET);
        if (offset >= 0)
        {
            struct flock lock = {F_RDLCK, SEEK_SET, cid * sizeof(struct Customer), sizeof(struct Customer), getpid()};

            int lockingStatus = fcntl(customerFileFD, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("\nERROR_LOG : Error obtaining read lock on customer record!");
                return false;
            }

            readBytes = read(customerFileFD, &customer, sizeof(struct Customer));
            printf("%ld", readBytes);
            if (readBytes == -1)
            {

                perror("\nERROR_LOG : Error reading customer record from file!");
            }

            lock.l_type = F_UNLCK;
            fcntl(customerFileFD, F_SETLK, &lock);

            if (strcmp(customer.login, readBuffer) == 0)
                userFound = true;

            close(customerFileFD);
        }
        else
        {
            writeBytes = write(connFD, "\n***\n\n\t\t\tCustomer not found for the given login ID***\n^$", strlen("\n***Customer not found for the given login ID***\n^$"));
        }
    }

    if (userFound)
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        writeBytes = write(connFD, PASSWORD, strlen(PASSWORD));
        if (writeBytes == -1)
        {
            perror("ERROR_LOG :Error writing PASSWORD message to client!");
            return false;
        }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == 1)
        {
            perror("Error reading password from the client!");
            return false;
        }

        char hashedPassword[1000];
        strcpy(hashedPassword, crypt(readBuffer, SALT_BAE));

        if (isAdmin)
        {
            if (strcmp(hashedPassword, ADMIN_PASSWORD) == 0)
                return true;
        }
        else
        {
            if (strcmp(hashedPassword, customer.password) == 0)
            {
                *ptrToCustomerID = customer;
                return true;
            }
        }

        bzero(writeBuffer, sizeof(writeBuffer));
        writeBytes = write(connFD, INVALID_PASSWORD, strlen(INVALID_PASSWORD));
    }
    else
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        writeBytes = write(connFD, INVALID_LOGIN, strlen(INVALID_LOGIN));
    }

    return false;
}

bool get_account_details(int connFD, struct Account *customerAccount)
{
    ssize_t readBytes, writeBytes;            // Number of bytes read from / written to the socket
    char readBuffer[1000], writeBuffer[1000]; // A buffer for reading from / writing to the socket
    char tempBuffer[1000];

    int accountNumber;
    struct Account account;
    int accountFileDescriptor;

    if (customerAccount == NULL)
    {

        writeBytes = write(connFD, "\n\n\t\tEnter the account number of the account you're searching for : ", strlen("\n\n\t\tEnter the account number of the account you're searching for : "));
        if (writeBytes == -1)
        {
            perror("Error writing GET_ACCOUNT_NUMBER message to client!");
            return false;
        }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error reading account number response from client!");
            return false;
        }
        //printf("%ld\n", readBytes);
        accountNumber = atoi(readBuffer);

        if(((strcmp(readBuffer,"0")!=0)&&accountNumber==0)){
              bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\t\t***No account could be found for the given account number*** \n");
        strcat(writeBuffer, "^");
        perror("Error opening account file in get_account_details!");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ACCOUNT_ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
        }
    }
    
    else
    {
        accountNumber = customerAccount->accountNumber;
    }
    //printf("acc:%d\n", accountNumber);
    accountFileDescriptor = open(ACCOUNT_FILE, O_RDONLY);
    if (accountFileDescriptor == -1 )
    {
        // Account record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\t\t***No account could be found for the given account number*** \n");
        strcat(writeBuffer, "^");
        perror("Error opening account file in get_account_details!");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ACCOUNT_ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }

    int offset = lseek(accountFileDescriptor, accountNumber * sizeof(struct Account), SEEK_SET);
    if (offset == -1 && errno == EINVAL)
    {
        // Account record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\t\t***No account could be found for the given account number ***\n");
        strcat(writeBuffer, "^");
        perror("Error seeking to account record in get_account_details!");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ACCOUNT_ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required account record!");
        return false;
    }

    struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Account), getpid()};

    int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error obtaining read lock on account record!");
        return false;
    }

    readBytes = read(accountFileDescriptor, &account, sizeof(struct Account));
    if (readBytes == -1)
    {
        perror("Error reading account record from file!");
        return false;
    }
    printf("read bytes:%ld", readBytes);
    lock.l_type = F_UNLCK;
    fcntl(accountFileDescriptor, F_SETLK, &lock);

    if (customerAccount != NULL)
    {
        printf("%d", account.active);
        *customerAccount = account;
        printf("%d", customerAccount->active);
        return true;
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    if (readBytes > 0 && account.active)
    {
        sprintf(writeBuffer, "\n\n\t\t--------------Account Details --------------\n\n\t\tAccount Number : %d\n\t\tAccount Type : %s\n\t\tAccount Status : %s", account.accountNumber, (account.isRegularAccount ? "Regular" : "Joint"), (account.active) ? "Active" : "Deactived");
        if (account.active)
        {
            sprintf(tempBuffer, "\n\t\tAccount Balance:₹ %ld", account.balance);
            strcat(writeBuffer, tempBuffer);
        }

        sprintf(tempBuffer, "\n\t\tPrimary Owner ID: %d", account.owners[0]);
        strcat(writeBuffer, tempBuffer);
        if (account.owners[1] != -1)
        {
            sprintf(tempBuffer, "\n\t\tSecondary Owner ID: %d", account.owners[1]);
            strcat(writeBuffer, tempBuffer);
        }

        strcat(writeBuffer, "\n^");
    }
    else
    {
        strcpy(writeBuffer, "\n\n\t\t***No account could be found for the given ID***\n");
        strcat(writeBuffer, "^");
    }

    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    return true;
}

bool get_customer_details(int connFD, int customerID)
{
    ssize_t readBytes, writeBytes;             // Number of bytes read from / written to the socket
    char readBuffer[1000], writeBuffer[10000]; // A buffer for reading from / writing to the socket
    char tempBuffer[1000];

    struct Customer customer;
    int customerFileDescriptor;
    struct flock lock = {F_RDLCK, SEEK_SET, 0, sizeof(struct Account), getpid()};

    if (customerID == -1)
    {
        writeBytes = write(connFD, "\n\n\n\t\t Enter the customer ID of the customer you're searching for : ", strlen("\n\n\n\t\tEnter the customer ID of the customer you're searching for : "));
        if (writeBytes == -1)
        {
            perror("ERROR_LOG : Error while writing GET_CUSTOMER_ID message to client!");
            return false;
        }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("ERROR_LOG : Error getting customer ID from client!");
            ;
            return false;
        }

        customerID = atoi(readBuffer);
        if((strcmp(readBuffer,"0")!=0) && customerID == 0){
              bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\n\t\t***No customer could be found for the given ID***\n");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("ERROR_LOG :Error while writing CUSTOMER_ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
        }
    }

    customerFileDescriptor = open(CUSTOMER_FILE, O_RDONLY);
    if (customerFileDescriptor == -1)
    {
        // Customer File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\n\t\t***No customer could be found for the given ID***\n");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("ERROR_LOG :Error while writing CUSTOMER_ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    int offset = lseek(customerFileDescriptor, customerID * sizeof(struct Customer), SEEK_SET);
    if (errno == EINVAL)
    {
        // Customer record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\n\t\t***No customer could be found for the given ID***\n");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("ERROR_LOG : Error while writing CUSTOMER_ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("ERROR_LOG : Error while seeking to required customer record!");
        return false;
    }
    lock.l_start = offset;

    int lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("ERROR_LOG : Error while obtaining read lock on the Customer file!");
        return false;
    }

    readBytes = read(customerFileDescriptor, &customer, sizeof(struct Customer));
    if (readBytes == -1)
    {
        perror("ERROR_LOG : Error reading customer record from file!");
        return false;
    }

    lock.l_type = F_UNLCK;
    fcntl(customerFileDescriptor, F_SETLK, &lock);

    bzero(writeBuffer, sizeof(writeBuffer));
    if (readBytes > 0)
    {
        sprintf(writeBuffer, "\n\n\t\t-----------Customer Details ---------- \n\n\t\tCID : %d\n\t\tName : %s\n\t\tGender : %c\n\t\tAge: %d\n\t\tAccount Number : %d\n\t\tLoginID : %s\n", customer.id, customer.name, customer.gender, customer.age, customer.account, customer.login);
        strcat(writeBuffer, "\n\n\t\tPress Enter to redirect to the main menu...^");
    }
    else
    {
        strcpy(writeBuffer, "\n\n\t\t***No customer could be found for the given ID***\n");
        strcat(writeBuffer, "^");
    }

    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("\n\t ERROR_LOG : Error writing customer info to client!");
        return false;
    }

    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
    return true;
}

bool get_transaction_details(int connFD, int accountNumber)
{

    ssize_t readBytes, writeBytes;                                 // Number of bytes read from / written to the socket
    char readBuffer[10000], writeBuffer[10000], tempBuffer[10000]; // A buffer for reading from / writing to the socket

    struct Account account;

    if (accountNumber == -1)
    {
        // Get the accountNumber
        writeBytes = write(connFD, "\n\n\t\tEnter the account number of the account you're searching for : ", strlen("\n\n\t\tEnter the account number of the account you're searching for : "));
        if (writeBytes == -1)
        {
            perror("Error writing GET_ACCOUNT_NUMBER message to client!");
            return false;
        }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error reading account number response from client!");
            return false;
        }

        account.accountNumber = atoi(readBuffer);
    }
    else
        account.accountNumber = accountNumber;

    if (get_account_details(connFD, &account))
    {
        int iter;

        struct Transaction transaction;
        struct tm transactionTime;

        bzero(writeBuffer, sizeof(readBuffer));

        int transactionFileDescriptor = open(TRANSACTION_FILE, O_RDONLY);
        if (transactionFileDescriptor == -1)
        {
            perror("Error while opening transaction file!");
            write(connFD, "\n\t\t***No transactions were performed on this account by the customer!***\n^", strlen("\n\t\t***No transactions were performed on this account by the customer!***\n^"));
            read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
            return false;
        }

        for (iter = 0; iter < MAX_TRANSACTIONS && account.transactions[iter] != -1; iter++)
        {

            int offset = lseek(transactionFileDescriptor, account.transactions[iter] * sizeof(struct Transaction), SEEK_SET);
            if (offset == -1)
            {
                perror("Error while seeking to required transaction record!");
                return false;
            }

            struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Transaction), getpid()};

            int lockingStatus = fcntl(transactionFileDescriptor, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining read lock on transaction record!");
                return false;
            }

            readBytes = read(transactionFileDescriptor, &transaction, sizeof(struct Transaction));
            if (readBytes == -1)
            {
                perror("Error reading transaction record from file!");
                return false;
            }

            lock.l_type = F_UNLCK;
            fcntl(transactionFileDescriptor, F_SETLK, &lock);

            transactionTime = *localtime(&(transaction.transactionTime));

            bzero(tempBuffer, sizeof(tempBuffer));
            sprintf(tempBuffer, "\n\n\t\tDetails of transaction %d - \n\t\t\t Date : %d:%d %d/%d/%d \n\t\t\t Operation : %s \n\t\t\t Balance - \n\t\t\t\t Before : %ld \n\t\t\t\t After : %ld \n\t\t\t\t Difference : %ld", (iter + 1), transactionTime.tm_hour, transactionTime.tm_min, transactionTime.tm_mday, transactionTime.tm_mon, transactionTime.tm_year, (transaction.operation ? "Deposit" : "Withdraw"), transaction.oldBalance, transaction.newBalance, (transaction.newBalance - transaction.oldBalance));

            if (strlen(writeBuffer) == 0)
                strcpy(writeBuffer, tempBuffer);
            else
                strcat(writeBuffer, tempBuffer);
        }

        close(transactionFileDescriptor);

        if (strlen(writeBuffer) == 0)
        {
            write(connFD, "\n\n\t\t***No transactions were performed on this account by the customer!***\n^", strlen("\n\n\t\t***No transactions were performed on this account by the customer!***\n^"));
            read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
            return false;
        }
        else
        {
            strcat(writeBuffer, "^");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        }
    }
}
