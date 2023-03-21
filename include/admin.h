
#include "./common.h"
#include <ctype.h>
// declarations of functions

bool admin_operation_handler(int connFD);
bool add_account(int connFD);
int add_customer(int connFD, bool isPrimary, int newAccountNumber);
bool delete_account(int connFD);
bool modify_customer_info(int connFD);

// admin operation
bool admin_operation_handler(int connFD)
{
    ssize_t writeBytes, readBytes;            // Number of bytes read from / written to the client
    char readBuffer[1000], writeBuffer[1000]; // A buffer used for reading & writing to the client
    if (login_handler(true, connFD, NULL))
    {

        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\n\n\t\t------------------------------------------------------\n\t\t\t   Dear Admin, \n\t\t\t\tWELCOME to WORLD BANK Management \n\n\t\t----------------------------------------------------\n");
        while (1)
        {
            strcat(writeBuffer, "\n\n\n\t\t------------------------Admin Menu-------------------------\n");
            strcat(writeBuffer, "\n\n\t\t Press 1 : Add Account\n\t\t Press 2 : Delete Accounts\n\t\t Press 3 : Modify Customer Information\n\t\t Press 4 : Search for Customer details\n\t\t Press 5 : Get Account Details\n\t\t Press 6 : Logout\n");
            strcat(writeBuffer, "\n\t\t------------------------------------------------------------\n\t\tEnter your choice : ");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing ADMIN_MENU to client!");
                return false;
            }
            bzero(writeBuffer, sizeof(writeBuffer));

            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("Error while reading client's choice for ADMIN_MENU");
                return false;
            }

            int choice = atoi(readBuffer);
            switch (choice)
            {
            case 1:
                add_account(connFD);
                break;

            case 2:
                delete_account(connFD);
                break;
            case 3:
                modify_customer_info(connFD);
                break;

            case 4:
                get_customer_details(connFD, -1);
                break;

            case 5:
                get_account_details(connFD, NULL);
                break;

            case 6:

                writeBytes = write(connFD, "\n\t\tLogging you out!$", strlen("\n\t\tLogging you out!$"));
                return false;
            default:
                writeBytes = write(connFD, "\n\t\tLogging you out!$", strlen("\n\t\tLogging you out!$"));
                return false;
            }
        }
    }
    else
    {
        // ADMIN LOGIN FAILED
        bzero(writeBuffer, sizeof(writeBuffer));
        sprintf(writeBuffer, "%s\n", "No account found!!$");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        return false;
    }
    return true;
}
//____________________________________________________________________________________________________________________________

//---------------------------ADD ACCOUNT ---------------------------------------------------
bool add_account(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

    struct Account newAccount, prevAccount;

    int accountFileDescriptor = open(ACCOUNT_FILE, O_RDONLY);
    if (accountFileDescriptor == -1 && errno == ENOENT)
    {
        // Account file was never created
        newAccount.accountNumber = 0;
    }
    else if (accountFileDescriptor == -1)
    {
        perror("\nERROR_LOG : Error while opening account file");
        return false;
    }
    else
    {
        int offset = lseek(accountFileDescriptor, -sizeof(struct Account), SEEK_END);
        if (offset == -1)
        {
            perror("\nERROR_LOG : Error seeking to last Account record!");
            return false;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Account), getpid()};
        int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("ERROR_LOG : Error obtaining read lock on Account record!");
            return false;
        }

        readBytes = read(accountFileDescriptor, &prevAccount, sizeof(struct Account));
        if (readBytes == -1)
        {
            perror("ERROR_LOG : Error while reading Account record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(accountFileDescriptor, F_SETLK, &lock);

        close(accountFileDescriptor);

        newAccount.accountNumber = prevAccount.accountNumber + 1;
    }
    writeBytes = write(connFD, "\n\n\t\t Enter the type of account:\n\n\t\t   1. Normal account\n\t\t   2. Joint account\n\n\n\t\t Enter the choice : ", strlen("\n\n\t\t Enter the type of account:\n\n\t\t   1. Normal account\n\t\t   2. Joint account\n\n\n\t\t Enter the choice : "));
    if (writeBytes == -1)
    {
        perror("Error writing ADMIN_ADD_ACCOUNT_TYPE message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading account type response from client!");
        return false;
    }
    int i = atoi(readBuffer);
    printf("%d\n", i);
    if (i != 1 && i != 2)
    {
        return false;
    }
    newAccount.isRegularAccount = atoi(readBuffer) == 1 ? true : false;

    newAccount.owners[0] = add_customer(connFD, true, newAccount.accountNumber);
    if (newAccount.owners[0] != -2)
    {
        if (newAccount.isRegularAccount)
            newAccount.owners[1] = -1;
        else
            newAccount.owners[1] = add_customer(connFD, false, newAccount.accountNumber);

        newAccount.active = true;
        newAccount.balance = 0;

        memset(newAccount.transactions, -1, MAX_TRANSACTIONS * sizeof(int));
        if (newAccount.owners[0] != -2 && newAccount.owners[1] != -2)
        {
            accountFileDescriptor = open(ACCOUNT_FILE, O_CREAT | O_APPEND | O_WRONLY, 0777);
            if (accountFileDescriptor == -1)
            {
                perror("Error while creating / opening account file!");
                return false;
            }
            // write record in the file
            writeBytes = write(accountFileDescriptor, &newAccount, sizeof(struct Account));
            if (writeBytes == -1)
            {
                perror("Error while writing Account record to file!");
                return false;
            }

            close(accountFileDescriptor);

            bzero(writeBuffer, sizeof(writeBuffer));
            sprintf(writeBuffer, "\n%s%d", "\n\n\n\t\t The newly created account's number is : ", newAccount.accountNumber);
        }
        else
        {
            strcpy(writeBuffer, "\nError Receveing the appropriate values for customers!!\n");
        }
    }
    else
    {
        strcpy(writeBuffer, "\nError Receveing the appropriate values for customers!!\n");
    }

    strcat(writeBuffer, "\n\n\t\tPress Enter to Redirect to the main menu ...\n^");
    writeBytes = write(connFD, writeBuffer, sizeof(writeBuffer));
    readBytes = read(connFD, readBuffer, sizeof(read)); // Dummy read
    return true;
}

int add_customer(int connFD, bool isPrimary, int newAccountNumber)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

    struct Customer newCustomer, previousCustomer;

    int customerFileDescriptor = open(CUSTOMER_FILE, O_RDONLY);
    if (customerFileDescriptor == -1 && errno == ENOENT)
    {
        // Customer file was never created
        newCustomer.id = 0;
    }
    else if (customerFileDescriptor == -1)
    {
        perror("Error while opening customer file");
        return -2;
    }
    else
    {
        int offset = lseek(customerFileDescriptor, -sizeof(struct Customer), SEEK_END);
        if (offset == -1)
        {
            perror("Error seeking to last Customer record!");
            return -2;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Customer), getpid()};
        int lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Customer record!");
            return -2;
        }

        readBytes = read(customerFileDescriptor, &previousCustomer, sizeof(struct Customer));
        if (readBytes == -1)
        {
            perror("Error while reading Customer record from file!");
            return -2;
        }

        lock.l_type = F_UNLCK;
        fcntl(customerFileDescriptor, F_SETLK, &lock);

        close(customerFileDescriptor);

        newCustomer.id = previousCustomer.id + 1;
    }

    if (isPrimary)
        sprintf(writeBuffer, "%s%s", "\n\n\t\tEnter the details for the primary customer : \n\n\t\t", "\t\tWhat is the customer's name? - ");
    else
        sprintf(writeBuffer, "%s%s", "\n\t\tEnter the details for the secondary customer : \n\n\t\t", "\t\tWhat is the customer's name? - ");

    writeBytes = write(connFD, writeBuffer, sizeof(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error writing ADMIN_ADD_CUSTOMER_NAME message to client!");
        return -2;
    }

    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading customer name response from client!");
        ;
        return -2;
    }

    strcpy(newCustomer.name, readBuffer);

    writeBytes = write(connFD, "\n\n\n\n\t\tWhat is the customer's gender?\n\t\t 1. Male(M)\n\t\t 2. Female(F)\n\t\t 3. Others(O)\n\t\t Enter the choice(M/F/O) : ", strlen("\n\n\n\n\t\tWhat is the customer's gender?\n\t\t 1. Male(M)\n\t\t 2. Female(F)\n\t\t 3. Others(O)\n\t\t Enter the choice(M/F/O) : "));
    if (writeBytes == -1)
    {
        perror("\n\t ERROR_LOG : Error writing ADMIN_ADD_CUSTOMER_GENDER message to client!");
        return -2;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("\nERROR_LOG : Error reading customer gender response from client!");
        return -2;
    }

    if (readBuffer[0] == 'M' || readBuffer[0] == 'F' || readBuffer[0] == 'O' || readBuffer[0] == 'm' || readBuffer[0] == 'f' || readBuffer[0] == 'o')
        newCustomer.gender = toupper(readBuffer[0]);
    else
    {
        writeBytes = write(connFD, "\n\n\t\tIt seems you've entered a wrong gender choice! :(\n^", strlen("\n\n\t\tIt seems you've entered a wrong gender choice! :(\n^"));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return -2;
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "\n\n\t\tWhat is the customer's age ? : ");
    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error writing ADMIN_ADD_CUSTOMER_AGE message to client!");
        return -2;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading customer age response from client!");
        return -2;
    }

    int customerAge = atoi(readBuffer);
    if (customerAge <= 0)
    {
        // Either client has sent age as 0 (which is invalid) or has entered a non-numeric string
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\t\tInvalid input!\n\n\t\tYou'll now be redirected to the main menu!^\n\n");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
            return -2;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return -2;
    }
    newCustomer.age = customerAge;

    newCustomer.account = newAccountNumber;

    strcpy(newCustomer.login, newCustomer.name);
    strcat(newCustomer.login, "-");
    sprintf(writeBuffer, "%d", newCustomer.id);
    strcat(newCustomer.login, writeBuffer);

    char hashedPassword[1000];
    strcpy(hashedPassword, crypt(AUTOGEN_PASSWORD, SALT_BAE));
    strcpy(newCustomer.password, hashedPassword);

    customerFileDescriptor = open(CUSTOMER_FILE, O_CREAT | O_APPEND | O_WRONLY, 0777);
    if (customerFileDescriptor == -1)
    {
        perror("Error while creating / opening customer file!");
        return -2;
    }
    writeBytes = write(customerFileDescriptor, &newCustomer, sizeof(newCustomer));
    if (writeBytes == -1)
    {
        perror("Error while writing Customer record to file!");
        return -2;
    }

    close(customerFileDescriptor);

    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "%s%s-%d\n%s%s\n", "\n\n\t\tThe autogenerated login ID for the customer is : ", newCustomer.name, newCustomer.id, "\n\n\t\tThe autogenerated password for the customer is : ", AUTOGEN_PASSWORD);
    strcat(writeBuffer, "^");
    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error sending customer loginID and password to the client!");
        return -2;
    }

    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    return newCustomer.id;
}

//---------------------------DELETE ACCOUNT ---------------------------------------------------
bool delete_account(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

    struct Account account;

    writeBytes = write(connFD, "\n\n\t\t What is the account number of the account you want to delete ? : ", strlen("\n\n\t\t What is the account number of the account you want to delete ? : "));
    if (writeBytes == -1)
    {
        perror("Error writing ADMIN_DEL_ACCOUNT_NO to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading account number response from the client!");
        return false;
    }

    int accountNumber = atoi(readBuffer);

    int accountFileDescriptor = open(ACCOUNT_FILE, O_RDONLY);
    if (accountFileDescriptor == -1)
    {
        // Account record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\t\t***No account could be found for the given account number***\n");
        strcat(writeBuffer, "^");
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
    if (errno == EINVAL)
    {
        // Customer record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\t\t***No account could be found for the given account number***\n");
        strcat(writeBuffer, "^");
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
        perror("Error obtaining read lock on Account record!");
        return false;
    }

    readBytes = read(accountFileDescriptor, &account, sizeof(struct Account));
    if (readBytes == -1)
    {
        perror("Error while reading Account record from file!");
        return false;
    }
    // printf("readBytes:%ld\n", account.active);
    lock.l_type = F_UNLCK;
    fcntl(accountFileDescriptor, F_SETLK, &lock);

    close(accountFileDescriptor);

    bzero(writeBuffer, sizeof(writeBuffer));
    if (account.balance == 0 && accountNumber == account.accountNumber && account.active == 1)
    {
        // No money, hence can close account
        account.active = false;
        accountFileDescriptor = open(ACCOUNT_FILE, O_WRONLY);
        if (accountFileDescriptor == -1)
        {
            perror("Error opening Account file in write mode!");
            return false;
        }

        offset = lseek(accountFileDescriptor, accountNumber * sizeof(struct Account), SEEK_SET);
        if (offset == -1)
        {
            perror("Error seeking to the Account!");
            return false;
        }

        lock.l_type = F_WRLCK;
        lock.l_start = offset;

        int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining write lock on the Account file!");
            return false;
        }

        writeBytes = write(accountFileDescriptor, &account, sizeof(struct Account));
        if (writeBytes == -1)
        {
            perror("Error deleting account record!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(accountFileDescriptor, F_SETLK, &lock);

        strcpy(writeBuffer, "\n\n\t\tThis account has been successfully deleted\n\t\tRedirecting you to the main menu ...");
    }
    else if (accountNumber != account.accountNumber || account.active != 1)
    {
        strcpy(writeBuffer, "\n\n\t\t***No account could be found for the given account number***\n");

    }
    else
        // Account has some money ask customer to withdraw it
        strcpy(writeBuffer, "\n\n\t\tThis account cannot be deleted since it still has some money\n\t\tRedirecting you to the main menu ...");
    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error while writing final DEL message to client!");
        return false;
    }
    // readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    return true;
}

//---------------------------MODIFY ACCOUNT ---------------------------------------------------
bool modify_customer_info(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

    struct Customer customer;

    int customerID;

    off_t offset;
    int lockingStatus;

    writeBytes = write(connFD, "\n\n\t\tEnter the ID of the customer who's information you want to edit : ", strlen("\n\n\t\tEnter the ID of the customer who's information you want to edit : "));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_MOD_CUSTOMER_ID message to client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error while reading customer ID from client!");
        return false;
    }

    customerID = atoi(readBuffer);

    int customerFileDescriptor = open(CUSTOMER_FILE, O_RDONLY);
    if (customerFileDescriptor == -1)
    {
        // Customer File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\t\t***No customer could be found for the given ID***\n");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing CUSTOMER_ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }

    offset = lseek(customerFileDescriptor, customerID * sizeof(struct Customer), SEEK_SET);
    if (errno == EINVAL)
    {
        // Customer record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\t\t***No customer could be found for the given ID***\n");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing CUSTOMER_ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required customer record!");
        return false;
    }

    struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Customer), getpid()};

    // Lock the record to be read
    lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Couldn't obtain lock on customer record!");
        return false;
    }

    readBytes = read(customerFileDescriptor, &customer, sizeof(struct Customer));

    if (readBytes == -1)
    {
        perror("Error while reading customer record from the file!");
        return false;
    }

    // Unlock the record
    lock.l_type = F_UNLCK;
    fcntl(customerFileDescriptor, F_SETLK, &lock);

    close(customerFileDescriptor);
    if (readBytes == 0)
    {
        // Customer record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\t\t***No customer could be found for the given ID***\n");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing CUSTOMER_ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }

    writeBytes = write(connFD, "\n\t\tWhich information would you like to modify?\n\t\t1. Name 2. Age 3. Gender 4. Login ID 5. Password \n\n\n\t\tPress any other key and enter to cancel : ", strlen("\n\t\tWhich information would you like to modify?\n\t\t1. Name 2. Age 3. Gender 4. Login ID 5. Password \n\n\n\t\tPress any other key and enter to cancel : "));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_MOD_CUSTOMER_MENU message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error while getting customer modification menu choice from client!");
        return false;
    }

    int choice = atoi(readBuffer);
    if (choice == 0)
    { // A non-numeric string was passed to atoi
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\t\tInvalid input!\n\t\tYou'll now be redirected to the main menu!^\n");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    switch (choice)
    {
    case 1:
        // UPDATE NAME
        writeBytes = write(connFD, "\n\n\t\tWhat's the updated value for name ? : ", strlen("\n\n\t\tWhat's the updated value for name ? : "));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_MOD_CUSTOMER_NEW_NAME message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for customer's new name from client!");
            return false;
        }
        strcpy(customer.name, readBuffer);
        break;
    case 2:
        // update age
        writeBytes = write(connFD, "\n\n\t\tWhat's the updated value for age ? ", strlen("\n\n\t\tWhat's the updated value for age ? "));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_MOD_CUSTOMER_NEW_AGE message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for customer's new age from client!");
            return false;
        }
        int updatedAge = atoi(readBuffer);
        if (updatedAge == 0)
        {
            // Either client has sent age as 0 (which is invalid) or has entered a non-numeric string
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Invalid input!\nYou'll now be redirected to the main menu!\n^");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
                return false;
            }
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
            return false;
        }
        customer.age = updatedAge;
        break;
    case 3:
        // update gender
        writeBytes = write(connFD, "\n\n\t\tWhat's the updated value for gender ? ", strlen("\n\n\t\tWhat's the updated value for gender ? "));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_MOD_CUSTOMER_NEW_GENDER message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for customer's new gender from client!");
            return false;
        }
        customer.gender = readBuffer[0];
        break;

    case 4:
        // UPDATE login id
        writeBytes = write(connFD, "\n\n\t\tWhat's the updated value for LoginId ? : ", strlen("\n\n\t\tWhat's the updated value for LoginId ? : "));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_MOD_CUSTOMER_NEW_NAME message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for customer's new name from client!");
            return false;
        }
        strcat(readBuffer, "-");
        // strcat(readBuffer, customer.id);

        sprintf(readBuffer, "%s%d", readBuffer, customer.id);

        strcpy(customer.login, readBuffer);
        bzero(writeBuffer, sizeof(writeBuffer));
        sprintf(writeBuffer, "%s %s^", "\nThe new customer login  id is : ", readBuffer);

        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        bzero(writeBuffer, sizeof(writeBuffer));

        break;
    case 5:
        // UPDATE password
        writeBytes = write(connFD, "\n\n\t\tWhat's the updated value for Password ?: ", strlen("\n\n\t\tWhat's the updated value for Password ?: "));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_MOD_CUSTOMER_NEW_NAME message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for customer's new name from client!");
            return false;
        }

        strcpy(customer.password, crypt(readBuffer, SALT_BAE));
        break;
    default:
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\n\n\t\tExiting..\n\t\tYou'll now be redirected to the main menu!^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing INVALID_MENU_CHOICE message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }

    customerFileDescriptor = open(CUSTOMER_FILE, O_WRONLY);
    if (customerFileDescriptor == -1)
    {
        perror("Error while opening customer file");
        return false;
    }
    offset = lseek(customerFileDescriptor, customerID * sizeof(struct Customer), SEEK_SET);
    if (offset == -1)
    {
        perror("Error while seeking to required customer record!");
        return false;
    }

    lock.l_type = F_WRLCK;
    lock.l_start = offset;
    lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining write lock on customer record!");
        return false;
    }

    writeBytes = write(customerFileDescriptor, &customer, sizeof(struct Customer));
    if (writeBytes == -1)
    {
        perror("Error while writing update customer info into file");
    }

    lock.l_type = F_UNLCK;
    fcntl(customerFileDescriptor, F_SETLKW, &lock);

    close(customerFileDescriptor);

    writeBytes = write(connFD, "\n\n\t\tThe required modification was successfully made!\n\t\tYou'll now be redirected to the main menu!^", strlen("\n\n\t\tThe required modification was successfully made!\n\t\tYou'll now be redirected to the main menu!^"));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_MOD_CUSTOMER_SUCCESS message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    return true;
}
