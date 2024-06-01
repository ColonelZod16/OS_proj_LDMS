#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <asm-generic/socket.h>
#include <fcntl.h>
#define PORT 8080
#define MAX_CLIENTS 5
pthread_mutex_t book_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;
// Admin credentials (for demonstration purposes)
#define ADMIN_USERNAME "admin"
#define ADMIN_PASSWORD "adminpass"

// User credentials (for demonstration purposes)
#define USER_USERNAME "user"
#define USER_PASSWORD "userpass"
FILE * file;
FILE * file1;
int num_entries=0;
char name[100];
char pwd[100];
struct Entry{
    char name[100];
    char password[100];
}entries[100];
struct Book{
    char name[100];
    char author[100];
    char quant[100];
}books[100];
typedef struct {
    char name[100];
    int status;  // 0 for returned, 1 for borrowed
} User;
int read_lock(int fd) {
    struct flock lock;
    lock.l_type = F_RDLCK;  // Set read lock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;       // Lock the whole file
    lock.l_len = 0;         // 0 means until EOF

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("fcntl - read lock");
        return -1;
    }
    return 0;
}

// Function to acquire a write lock
int write_lock(int fd) {
    struct flock lock;
    lock.l_type = F_WRLCK;  // Set write lock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;       // Lock the whole file
    lock.l_len = 0;         // 0 means until EOF

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("fcntl - write lock");
        return -1;
    }
    return 0;
}

// Function to unlock
int unlock(int fd) {
    struct flock lock;
    lock.l_type = F_UNLCK;  // Release lock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;       // Unlock the whole file
    lock.l_len = 0;         // 0 means until EOF

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        perror("fcntl - unlock");
        return -1;
    }
    return 0;
}
User users[5];
int num_users = 0;
void load_users(){
    int fd = open("users.txt", O_RDWR, 0666);
    FILE *file = fdopen(fd,"r");
    //read_lock(fd);
    if (file == NULL) {
        perror("fopen");
        return;
    }

    num_users = 0;
    while (fscanf(file, "%s %d", users[num_users].name, &users[num_users].status) == 2) {
        num_users++;
        if (num_users >= 5) {
            break;  // Prevent overflow
        }
    }
   // unlock(fd);
    fclose(file);
}
void save_users() {
    int fd = open("users.txt", O_RDWR, 0666);
    FILE *file = fdopen(fd, "w");
    //write_lock(fd);
    if (file == NULL) {
        perror("fopen");
        return;
    }

    for (int i = 0; i < num_users; i++) {
        fprintf(file, "%s %d\n", users[i].name, users[i].status);
    }
   // unlock(fd);
    fflush(file);
    fclose(file);
}
void update_user_status(const char *username, int status) {
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].name, username) == 0) {
            users[i].status = status;
            save_users();
            return;
        }
    }
}
// Function to handle admin login
int read_books_data() {
    int num = 0;
    int fd = open("books.txt", O_RDWR, 0666);
    FILE *file = fdopen(fd, "r");
    read_lock(fd);
    if (file != NULL) {
        while (fscanf(file, "%s %s %s", books[num].name, books[num].author, books[num].quant) == 3) {
            num++;
            if (num >= 200) {
                break; // Prevent overflow of books array
            }
        }
        unlock(fd);
        fclose(file);
    }
    return num;
}
void write_books_data(int num_books) {
    int fd = open("books.txt", O_RDWR, 0666);
    write_lock(fd);
    FILE *file = fdopen(fd, "w");
    if (file != NULL) {
        for (int i = 0; i < num_books; i++) {
            fprintf(file, "%s %s %s\n", books[i].name, books[i].author, books[i].quant);
        }
        unlock(fd);
        fclose(file);
    }
}
int book_exists(char *book_name , int num_books) {
    for (int i = 0; i < num_books; i++) {
        if (strcmp(books[i].name,book_name)==0 ) {
            return 1+i; // Book found
        }
    }
    return 0; // Book not found
}
void delete_book(const char *book_name,int num_books) {
    // Find the index of the book to be deleted
    int index = -1;
    for (int i = 0; i < num_books; i++) {
        if (strcmp(books[i].name, book_name) == 0) {
            index = i;
            break;
        }
    }

    // If the book is found
    if (index != -1) {
        // Shift subsequent books one position back in the array
        for (int i = index; i < num_books - 1; i++) {
            strcpy(books[i].name, books[i + 1].name);
            strcpy(books[i].author, books[i + 1].author);
            strcpy(books[i].quant, books[i + 1].quant);
        }
        num_books--; // Decrement the total number of books
    } else {
        printf("Book not found.\n");
    }
}
void delete_book_by_name(const char *book_name) {
    pthread_mutex_lock(&book_mutex);

    // Open the file for reading
    FILE *file = fopen("books.txt", "r");
    if (file == NULL) {
        perror("fopen");
        pthread_mutex_unlock(&book_mutex);
        return;
    }

    // Read all books into an array  // Assuming a maximum of 1000 books
    int book_count = 0;
    char line[512];

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%s %s %s]", books[book_count].name, books[book_count].author, books[book_count].quant) == 3) {
            book_count++;
        }
    }
    fclose(file);

    // Search for the book to be deleted
    int index_to_delete = -1;
    for (int i = 0; i < book_count; i++) {
        if (strcmp(books[i].name, book_name) == 0) {
            index_to_delete = i;
            break;
        }
    }

    // If the book is found, remove it from the array
    if (index_to_delete != -1) {
        for (int i = index_to_delete; i < book_count - 1; i++) {
            books[i] = books[i + 1];
        }
        book_count--;

        // Write the modified array back to the file
        file = fopen("books.txt", "w");
        if (file == NULL) {
            perror("fopen");
            pthread_mutex_unlock(&book_mutex);
            return;
        }

        for (int i = 0; i < book_count; i++) {
            fprintf(file, "%s %s %s\n", books[i].name, books[i].author, books[i].quant);
        }
        fclose(file);

        printf("Book deleted successfully\n");
    } else {
        printf("Book not found\n%s",book_name);
    }

    pthread_mutex_unlock(&book_mutex);
}
void modify_book_quantity(const char *book_name, const char *new_quantity,int num_books) {
    // Find the index of the book
    int index = -1;
    for (int i = 0; i < num_books; i++) {
        if (strcmp(books[i].name, book_name) == 0) {
            index = i;
            break;
        }
    }

    // If the book is found
    if (index != -1) {
        // Update the quantity
        strcpy(books[index].quant, new_quantity);
    } else {
        printf("Book not found.\n");
    }
}
void separate_name_and_quantity(char *combined_str, char *name, char *quantity) {
    char *token = strtok(combined_str, ":");
    if (token != NULL) {
        strcpy(name, token);
    }

    token = strtok(NULL, ":");
    if (token != NULL) {
        strcpy(quantity, token);
    }
}
void log_transaction(char *username, char *book_name, char *transaction_type) {
    FILE *file = fopen("transactions.txt", "a");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    // Get the current date and time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    // Write the transaction details to the file
    fprintf(file, "%s - User: %s, Book: %s, Transaction: %s\n", timestamp, username, book_name, transaction_type);

    fflush(file);
    fclose(file);
}

void view_transactions(int client_socket) {
    pthread_mutex_lock(&book_mutex);
    FILE *file = fopen("transactions.txt", "r");
    if (file == NULL) {
        perror("fopen");
        pthread_mutex_unlock(&book_mutex);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        send(client_socket, line, strlen(line), 0);
    }
    fclose(file);
    pthread_mutex_unlock(&book_mutex);
}

void handle_admin_login(int client_socket) {
    char buffer[1024] = {0};
    char buffer1[1024] = {0};
    char name[1024] = {0};
    char author[1024] = {0};
    char quant[1024] = {0};
    char data[1024] = {0};
    //char *welcome_message = "Welcome, Admin!\n";
    char *login_prompt = "Please enter your username and password (e.g., username:password): ";

    // Send welcome message to admin
    //send(client_socket, welcome_message, strlen(welcome_message), 0);
    memset(buffer,0,sizeof(buffer));
    // Receive login credentials from admin
    int valread = read(client_socket, buffer, 1024);
    if (valread <= 0) {
        close(client_socket);
        pthread_exit(NULL);
    }
    // Verify admin login credentials
    if (strcmp(buffer, ADMIN_USERNAME ":" ADMIN_PASSWORD) == 0) {
        // Authentication successful
        char *success_message = "Authentication successful. You are now logged in as admin.\n";
        send(client_socket, success_message, strlen(success_message), 0);
        while (1) {
            // Receive message from admin
            // char *menu = "1.Add Book\n 2.Delete Book\n 3.Modify Books\n 4.See transactions\n 5.View Member details\n 6.Exit\n";
            // send(client_socket,menu,strlen(menu),0);
            int valread2 = read(client_socket, buffer1, 1024);
            if (valread2 <= 0) {
                break; // Exit loop if no data received or connection closed
            }
            if(strcmp(buffer1,"Add Books")==0){
               pthread_mutex_lock(&book_mutex);
               int num_books=read_books_data();
               memset(data,0,sizeof(data));
               int valread5 = read(client_socket, data, 1024);
               char *token = strtok(data, ":");
               if (token != NULL) {
                  strncpy(name, token, sizeof(name) - 1);
                  name[sizeof(name) - 1] = '\0';
               }

               token = strtok(NULL, ":");
               if (token != NULL) {
                  strncpy(author, token, sizeof(author) - 1);
                  author[sizeof(author) - 1] = '\0';
               }
           
               token = strtok(NULL, ":");
               if (token != NULL) {
                 strncpy(quant, token, sizeof(quant) - 1);
                 quant[sizeof(quant) - 1] = '\0';
               }
               if(book_exists(name,num_books)!=0 && num_books!=0){
                char * error = "Book already exists";
                send(client_socket,error,strlen(error),0);
               }else{

                 strcpy(books[num_books].name,name);
                 strcpy(books[num_books].author,author);
                 strcpy(books[num_books].quant,quant);
                 num_books+=1;
                 write_books_data(num_books);
                 char * no_error = "Book added to database";
                 send(client_socket,no_error,strlen(no_error),0);
                 memset(buffer1,0,sizeof(buffer1));
                 memset(name,0,sizeof(name));
                 memset(author,0,sizeof(author));
                 memset(author,0,sizeof(author));
               }
               pthread_mutex_unlock(&book_mutex);
            }else if(strcmp(buffer1,"Delete Book\n")==0){
                memset(data,0,sizeof(data));
                int valread4=read(client_socket,data,1024);
                int num_books=read_books_data();
                if(book_exists(data,num_books)==0){
                    char * error = "Book Doesnt Exist in database\n";
                    send(client_socket,error,1024,0);
                }else{
                     //pthread_mutex_lock(&book_mutex);
                     int num_books=read_books_data();
                    //  delete_book(data,num_books);
                    //  write_books_data(num_books-1);
                     delete_book_by_name(data);
                     char * no_error = "Book deleted from database\n";
                     send(client_socket,no_error,1024,0);
                     //pthread_mutex_unlock(&book_mutex);
                }
            }else if(strcmp(buffer1,"Modify Book\n")==0){
                memset(data,0,sizeof(data));
                int valread4=read(client_socket,data,1024);
                int num_books=read_books_data();
                separate_name_and_quantity(data,name,quant);
                if(book_exists(name,num_books)==0){
                    char * error = "Book Doesnt Exist in database\n";
                    send(client_socket,error,1024,0);
                }else{
                    pthread_mutex_lock(&book_mutex);
                    modify_book_quantity(name,quant,num_books);
                    write_books_data(num_books);
                    char * no_error = "Book Data Modified\n";
                    send(client_socket,no_error,1024,0);
                    pthread_mutex_unlock(&book_mutex);
                }
            }else if(strcmp(buffer1,"Search For a book\n")==0){
                char bname[1024]={0};
                int valr=read(client_socket,bname,1024);
                int num_books=read_books_data();
                if(book_exists(bname,num_books)==0){
                    char * error = "Book not in database\n";
                    send(client_socket,error,strlen(error),0);
                }else{
                    pthread_mutex_lock(&book_mutex);
                    FILE *file = fopen("books.txt", "r");
                    if (file == NULL) {
                       perror("fopen");
                       pthread_mutex_unlock(&book_mutex);
                       return;
                    }
                    char line[1024];
                    int found = 0;
                    while (fgets(line, sizeof(line), file)) {
                        char name[256], author[256], quantity[256];
                        sscanf(line, "%s %s %s", name, author, quantity);
                        if (strcmp(name,bname)==0) {
                            char comb[1024];
                            snprintf(comb,sizeof(comb),"BookName:%s   Author:%s   Quantity:%s",name,author,quantity);
                            send(client_socket,comb, strlen(comb), 0);
                             found = 1;
                             break;
                           }
                    }
                    pthread_mutex_unlock(&book_mutex);
                }
            }
            memset(buffer1, 0, sizeof(buffer1));
        }
    } else {
        // Authentication failed
        char *failure_message = "Authentication failed. Invalid username or password.\n";
        send(client_socket, failure_message, strlen(failure_message), 0);
        close(client_socket);
        pthread_exit(NULL);
    }
}
char name_pwd_separate(char input[]){
    char *token = strtok(input, ":");
    if (token != NULL) {
        strncpy(name, token, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
    }
    token = strtok(NULL, ":");
    if (token != NULL) {
        strncpy(pwd, token, sizeof(pwd) - 1);
        pwd[sizeof(pwd) - 1] = '\0';
    }
}int get_user_status(char *username) {
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].name, username) == 0) {
            return users[i].status;
        }
    }
    // User not found
    return -1; // Indicates user not found
}
// Function to handle user login
void handle_user_login(int client_socket) {
    char buffer[1024] = {0};
    char buffer1[1024] = {0};
    char *welcome_message = "Welcome, User!\n";
    char *login_prompt = "Please enter your username and password (e.g., username:password): ";
    char username[100];
    // Send welcome message to user
    //send(client_socket, welcome_message, strlen(welcome_message), 0);

    // Receive login credentials from user
    int valread = read(client_socket, buffer, 1024);
    if (valread <= 0) {
        close(client_socket);
        pthread_exit(NULL);
    }
    int authenticated = 0;
    for (int i = 0; i < num_entries; i++) {
        name_pwd_separate(buffer);
        if (strcmp(name,entries[i].name) == 0  && strcmp(pwd,entries[i].password)==0 ){
            strcpy(username,entries[i].name);
            authenticated = 1;
            break;
        }
    } 
    // Verify user login credentials
    if (authenticated==1) {
        // Authentication successful
        char *success_message = "Authentication successful. You are now logged in as user.\n";
        send(client_socket, success_message, strlen(success_message), 0);

        // Implement user functionality here
        // For demonstration, let's just echo back user's messages

        // Handle client communication
        while (1) {
            load_users();
            // Receive message from user
            char data[100];
            int mode;
            int valread2 = read(client_socket, &mode, sizeof(mode));
            printf("%d",mode);
            if (valread2 <= 0) {
                break; // Exit loop if no data received or connection closed
            }
            if(mode==1){
                pthread_mutex_lock(&book_mutex);
                int valread3 = read(client_socket,data,sizeof(data));
                int num_books=read_books_data();
                int index = book_exists(data,num_books)-1;
                char quant[100];
                strcpy(quant,books[index].quant);
                int quantity = atoi(quant);
                if(get_user_status(username)==0){
                      char * error = "Already borrowed a book , return the previous book the borrow a new one\n";
                      send(client_socket,error,strlen(error),0);
                }
                else if(book_exists(data,num_books)==0 || quantity==0){
                    char * error = "Book not available\n";
                    send(client_socket,error,strlen(error),0);
                }else{
                    char new_quant[100];
                    int num_chars_written = sprintf(new_quant, "%d", quantity-1);
                    modify_book_quantity(data,new_quant,num_books);
                    write_books_data(num_books);
                    char * no_error = "Book borrowed\n";
                    send(client_socket,no_error,strlen(no_error),0);
                    log_transaction(username,data,"Borrowed");
                    update_user_status(username, 0);
                }
                pthread_mutex_unlock(&book_mutex);
            }else if(mode==2){
                int valread3 = read(client_socket,data,sizeof(data));
                int num_books=read_books_data();
                int index = book_exists(data,num_books)-1;
                char quant[100];
                strcpy(quant,books[index].quant);
                int quantity = atoi(quant);
                if(get_user_status(username)==1){
                    char * error  = "Nothing to return for this particular user\n";
                    send(client_socket,error,strlen(error),0);
                }
                else if(book_exists(data,num_books)==0 || quantity==0){
                    char * error = "Book not in database\n";
                    send(client_socket,error,strlen(error),0);
                }else{
                    pthread_mutex_lock(&book_mutex);
                    char new_quant[100];
                    int num_chars_written = sprintf(new_quant, "%d", quantity+1);
                    modify_book_quantity(data,new_quant,num_books);
                    write_books_data(num_books);
                    char * no_error = "Book returned\n";
                    send(client_socket,no_error,strlen(no_error),0);
                    log_transaction(username,data,"Returned");
                    update_user_status(username, 1);
                    pthread_mutex_unlock(&book_mutex);
                }
            }
            memset(buffer1, 0, sizeof(buffer1));
        }
    } else {
        // Authentication failed
        char *failure_message = "Authentication failed. Invalid username or password.\n";
        send(client_socket, failure_message, strlen(failure_message), 0);
        close(client_socket);
        pthread_exit(NULL);
    }
}

// Function to handle client communication
void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[1024] = {0};
    char *mode_prompt = "Please select login mode:\n1) User Mode\n2) Admin Mode\n";

    // Send mode prompt to client
    send(client_socket, mode_prompt, strlen(mode_prompt), 0);

    // Receive mode selection from client
    int mode;
    int valread = read(client_socket, &mode, sizeof(mode));
    if (valread <= 0) {
        printf("hi\n");
        close(client_socket);
        pthread_exit(NULL);
    }
    // Handle client based on selected mode
    switch (mode){
        case 1:
            char *user_message = "User mode selection.\n";
            send(client_socket, user_message, strlen(user_message), 0);
            handle_user_login(client_socket);
            break;
        case 2:
            char *admin_message = "Admin mode selection.\n";
            send(client_socket, admin_message, strlen(admin_message), 0);
            handle_admin_login(client_socket);
            break;
        case 3:
        pthread_mutex_lock(&book_mutex);
            char comb[100];
            memset(comb,0,strlen(comb));
            int valread5=read(client_socket,comb,strlen(comb));
            char name1[100];
            char pwd1[100];
            printf("%s",comb);
            //add_user(name1,pwd1);
            char messaga[100];
            send(client_socket,comb,strlen(comb),0);
            handle_user_login(client_socket);
            pthread_mutex_unlock(&book_mutex);
            break;
        default:
            // Invalid mode selection
            char *invalid_message = "Invalid mode selection.\n";
            send(client_socket, invalid_message, strlen(invalid_message), 0);
            close(client_socket);
            pthread_exit(NULL);
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    file = fopen("members.txt","r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    while (fscanf(file, "%s %s", entries[num_entries].name, entries[num_entries].password) == 2) {
        num_entries++;
    }
    // Close the file
    fclose(file);
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Accept incoming connections and handle them in separate threads
    while (1) {
     if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Create thread to handle client communication
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, &new_socket) != 0) {
            perror("pthread_create");
            close(new_socket);
        }
    }

    return 0;
}