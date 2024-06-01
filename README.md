# OS_proj_LDMS
All the codes and text files used in the project


Run the server code , it opens a socket always listening for connections , multiple clients can connect to the server locally.
Then run the client code , and follow the options on the screen to test out the admin and user modes.
User can borrow only one book at a time , amdin can perform operations such as Delete , modify and add books.
Used simple mutex lock and file locking to achieve concurrency.
Used multi-threading to allow multiple clients to access the server.
All the transactions can be seen in transactions.txt
users.txt is used to maintain the status of borrowed or returned.
members.txt includes the user data and password.
All login modes are password protected.
You can add any username and password by just editing this file.This was just for demonstatration purposes.
books.txt is the database for storing all the book details.

Enjoy!!
