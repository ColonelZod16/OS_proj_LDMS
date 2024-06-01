#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define MAX_MESSAGE_SIZE 1024
int is_username_present(const char *username) {
    FILE *file = fopen("members.txt", "r");
    if (file == NULL) {
        perror("Error opening members.txt");
        return 0; // Return 0 indicating username is not present
    }

    char line[256];
    int found = 0;

    // Read the file line by line
    while (fgets(line, sizeof(line), file)) {
        // Extract the username from the line
        char *token = strtok(line, " ");
        if (token && strcmp(token, username) == 0) {
            found = 1;
            break;
        }
    }

    // Close the file
    fclose(file);

    return found; // Return 1 if username is found, 0 otherwise
}
void print_file_content_line_by_line(char *file_content) {
    if (file_content == NULL) {
        printf("No content to display.\n");
        return;
    }

    const char *start = file_content;
    const char *end;

    while ((end = strchr(start, '\n')) != NULL) {
        // Print the current line
        printf("%.*s\n", (int)(end - start), start);
        // Move to the next line
        start = end + 1;
    }

    // Print the last line if it doesn't end with a newline character
    if (*start != '\0') {
        printf("%s\n", start);
    }
}
void view_transactions(int server_socket) {
    const int buffer_size = 4096;
    char buffer[buffer_size];
    char *file_content = NULL;
    size_t total_bytes_received = 0;
    int bytes_received;

    // Allocate initial memory for file_content
    file_content = malloc(1);
    if (file_content == NULL) {
        perror("malloc");
        return;
    }
    file_content[0] = '\0';  // Ensure it's null-terminated

    // Read data from the server in a loop
    while ((bytes_received = read(server_socket, buffer, buffer_size - 1)) > 0) {
        buffer[bytes_received] = '\0';  // Null-terminate the received data

        // Reallocate memory to accommodate the new data
        char *temp = realloc(file_content, total_bytes_received + bytes_received + 1);
        if (temp == NULL) {
            free(file_content);
            perror("realloc");
            return;
        }
        file_content = temp;

        // Append the new data to file_content
        strcat(file_content + total_bytes_received, buffer);
        total_bytes_received += bytes_received;
    }

    if (bytes_received < 0) {
        perror("read");
        free(file_content);
        return;
    }

    // Print the accumulated file content line by line
    char *line = strtok(file_content, "\n");
    while (line != NULL) {
        printf("%s\n", line);
        line = strtok(NULL, "\n");
    }

    free(file_content);
}
int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[MAX_MESSAGE_SIZE] = {0};
    char buffer1[MAX_MESSAGE_SIZE] = {0};
    char buffer2[MAX_MESSAGE_SIZE] = {0};
    char msg[MAX_MESSAGE_SIZE] = {0};
    char msg1[MAX_MESSAGE_SIZE] = {0};
    char username[100];
    char password[100];
    int logged_in = 0; // Flag to track whether the user is logged in

    // Create socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // Receive mode3 prompt from server
    valread = read(sock, buffer, MAX_MESSAGE_SIZE);

    int mode3;
    while(1) {
        // Select login mode3
        //printf("Enter login mode3 (1 for User Mode3, 2 for Admin Mode3): ");
        printf("%s\n", buffer);
        scanf("%d", &mode3);
        memset(buffer,0,sizeof(buffer));
        if(mode3!=1 && mode3!=2 && mode3!=3) {
            printf("Invalid input\n");
        } else {
            break;
        }
    }
    send(sock, &mode3, sizeof(mode3), 0);
    // if(mode3==3){
    //     char pwd[100];
    //     char name[100];
    //     while(1){
    //     printf("Enter a new username\n");
    //     scanf("%s",name);
    //     printf("Enter a new password\n");
    //     scanf("%s",pwd);
    //     int bit=is_username_present(name);
    //     if(bit==1){
    //         printf("UserName already exists\n");
    //     }else{
    //         break;
    //     }
    //     }
    //     char comb[200];
    //     sprintf(comb,"%s:%s",name,pwd);
    //     printf("%s",comb);
    //     send(sock,comb,strlen(comb),0);
    // }
    memset(msg,0,strlen(msg)); 
    valread = read(sock, msg, MAX_MESSAGE_SIZE);
    printf("%s\n",msg);
    printf("Enter your username: ");
    scanf("%s", username);
    printf("Enter your password: ");
    scanf("%s", password);

    // Send login credentials to server
    char credentials[200];
    sprintf(credentials, "%s:%s", username, password);
    send(sock, credentials, strlen(credentials), 0);

    // Receive authentication response from server
    //memset(buffer1,0,sizeof(buffer1));
    valread = read(sock, buffer2, MAX_MESSAGE_SIZE);
    printf("%s\n", buffer2);
    // Set flag to indicate successful login
    if (strcmp(buffer2, "Authentication successful. You are now logged in as user.\n") == 0 ||
        strcmp(buffer2, "Authentication successful. You are now logged in as admin.\n") == 0) {
        logged_in = 1;
    } else {
        close(sock);
        return -1;  // Exit program if authentication failed
    }
    // OLMS functionality after successful login
    while (logged_in && mode3==2) {
        // Implement OLMS functionality here
        // For demonstration, let's just send a message to the server and print the response
        printf("%s","1.Add Book\n2.Delete Book\n3.Modify Books\n4.Search For a book\n6.Exit\n");
        //int valread2 = read(sock,msg1,MAX_MESSAGE_SIZE);
        //printf("%s\n",msg1);
        char *message[MAX_MESSAGE_SIZE];
        int mode3;

        while(1){
          printf("Choose your option:");
          scanf("%d", &mode3);
          if(mode3>6 || mode3<1){
            printf("Invalid Option\n");
          }else{
            break;
          }
        }
        if(mode3 == 6) {
            printf("Exiting the program ,ending connection\n");
            close(sock);
            break; // Exit loop if user wants to quit
        }
        if(mode3 == 1){
            char *message = "Add Books";
            char name[100];
            char author[100];
            char quant[100];
            int books=0;
            send(sock, message, strlen(message), 0);
            printf("Enter Book Name:\n");
            scanf("%s",name);
            printf("Author:\n");
            scanf("%s",author);
            printf("Quantity:\n");
            scanf("%s",quant);
            char combined1[MAX_MESSAGE_SIZE];
            snprintf(combined1, sizeof(combined1), "%s:%s:%s", name, author, quant);
            //printf("%s\n",combined1);
            //int valread2 = read(sock,&books,sizeof(books));
            send(sock, combined1, strlen(combined1), 0);
            // send(sock, author, strlen(author), 0);
            char  feedback[MAX_MESSAGE_SIZE]={0};
            int valread45=read(sock,feedback,MAX_MESSAGE_SIZE);
            printf("%s\n",feedback);
        }else if(mode3==2){
            char *message = "Delete Book\n";
            char name[100];
            printf("Enter Book Name:\n");
            scanf("%s",name);
            send(sock, message, strlen(message), 0);
            send(sock, name, strlen(name), 0);
            char  feedback[MAX_MESSAGE_SIZE]={0};
            int valread4=read(sock,feedback,1024);
            printf("%s",feedback);
        }else if(mode3 == 3){
            char *message = "Modify Book\n";
            char name[100];
            char quant[100];
            send(sock, message, strlen(message), 0);
            printf("Enter Book Name:\n");
            scanf("%s",name);
            printf("Quantity:\n");
            scanf("%s",quant);     
            char combined1[MAX_MESSAGE_SIZE];        
            snprintf(combined1, sizeof(combined1), "%s:%s", name,quant);
            send(sock, combined1, strlen(combined1), 0);
            char  feedback[MAX_MESSAGE_SIZE]={0};
            int valread45=read(sock,feedback,MAX_MESSAGE_SIZE);
            printf("%s\n",feedback);
        }else if(mode3==4){
            char *message = "Search For a book\n";
            send(sock, message, strlen(message), 0);
            printf("%s\n","Enter a book name");
            char name[100];
            scanf("%s",name);
            send(sock,name,1024,0);
            char  feedback[MAX_MESSAGE_SIZE]={0};
            int valread45=read(sock,feedback,MAX_MESSAGE_SIZE);
            printf("%s\n",feedback);
        }else{
            char *message = "View Members\n";
            send(sock, message, strlen(message), 0);
        }
        
    }
    while(logged_in && mode3==1){
        int mode4=0;
        while(1){
         printf("1.Borrow Book\n2.Return Book\n3.Exit\n");
         scanf("%d",&mode4);
         if(mode4==3){
           printf("Exiting Program\n");
           close(sock);
           return 0;
         }
         else if(mode4!=1 && mode4!=2){
            printf("Invalid Option\n");
         }else{
            break;
         }
        }
        send(sock,&mode4,sizeof(mode4),0);
        if(mode4==1){
        char bookname[100];
        printf("Enter the name of the book:\n");
        scanf("%s",bookname);
        send(sock,bookname,strlen(bookname),0);
        char message[100];
        int valread5=read(sock,message,sizeof(message));
        printf("%s",message);
        memset(message,0,sizeof(message));
        }else{
            char name[100];
            printf("Enter the name of the book:\n");
            scanf("%s",name);
            send(sock,name,strlen(name),0);
            char message[100];
            int valread5=read(sock,message,sizeof(message));
            printf("%s",message);
            memset(message,0,sizeof(message));
        }

    }

    // Close socket
    close(sock);

   return 0;
}