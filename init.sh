
rm ./files/account.bank ./files/customer.bank ./files/transactions.bank
gcc ./src/client.c -o client
gcc ./src/server.c -lcrypt -o server

