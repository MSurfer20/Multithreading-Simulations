#include "headers.h"
using namespace std;

pthread_t client_threads[10000];
pthread_mutex_t print_lock;
struct Request
{
    int key1;
    int key2;
    string command;
    string value;
    int time_of_request;
    int id;
};

const char SERVER_ADDRESS[] = "127.0.0.1";
void* client_thread(void* arg)
{
    struct Request* req = (struct Request*)(arg);
    pthread_t thread_id = pthread_self();
    int req_no = req->id;
    sleep(req->time_of_request);

    int listenfd, connection_fd;
    char client_address[MAX_BUF_SIZE+1], readable_client_address[MAX_BUF_SIZE+1], send_data[MAX_BUF_SIZE+1], receive_data[MAX_BUF_SIZE+1];
    struct sockaddr_in servaddr;

    //INITIALIZING SOCKET
    listenfd=socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd<0)
    {
        perror("ERROR IN SOCKET");
        exit(-1);
    }

    //INITIALIZING ADDRESS
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, SERVER_ADDRESS, &servaddr.sin_addr)<=0)
    {
        perror("Error in connecting");
        return NULL;
    }

    int connect_res=connect(listenfd, (SA*)&servaddr, sizeof(servaddr));
    if(connect_res<0)
    {
        perror("Error in connecting");
        return NULL;
    }

    if(req->command=="insert" || req->command=="update")
    sprintf(send_data, "%s %d %s\n", req->command.c_str(), req->key1, req->value.c_str());
    else if(req->command=="delete" || req->command=="fetch")
    sprintf(send_data, "%s %d\n", req->command.c_str(), req->key1);
    else if(req->command=="concat")
    sprintf(send_data, "%s %d %d\n", req->command.c_str(), req->key1, req->key2);

    int num_bytes=strlen(send_data);
    int write_result=write(listenfd, send_data, num_bytes);
    if(write_result != num_bytes)
    {
        printf("Error sending data.");
        return NULL;
    }

    memset(receive_data, 0, MAX_BUF_SIZE+1);

    pthread_mutex_lock(&print_lock);
    cout<<req_no<<":"<<thread_id<<":";
    while((num_bytes = read(listenfd, receive_data, MAX_BUF_SIZE))>0)
    {
        cout<<receive_data;
        memset(receive_data, 0, MAX_BUF_SIZE+1);
    }
    cout<<endl;
    pthread_mutex_unlock(&print_lock);

    if(num_bytes<0)
    {
        perror("Error reading bytes.");
        return NULL;
    }
    
    return NULL;
}

int main(int argc, char** argv)
{
    int socket_fd, connection_fd, n, requests_count;
    struct sockaddr_in servaddr;
    char client_address[MAX_BUF_SIZE+1], sendline[MAX_BUF_SIZE+1], recvline[MAX_BUF_SIZE+1];
    pthread_mutex_init(&print_lock, NULL);
    cin>>requests_count;
    


    for(int i=0;i<requests_count;i++)
    {
        struct Request* req = new Request();
        req->id=i;
        
        cin>>req->time_of_request >> req->command >> req->key1;
        if(req->command == "insert")
        {
            cin>>req->value;
        }
        else if(req->command=="update")
        cin>>req->value;
        else if(req->command=="concat")
        cin>>req->key2;

        //take more input

        pthread_create(&client_threads[i], NULL, client_thread, (void*)req);
    }

    for(int i=0;i<requests_count;i++)
    {
        pthread_join(client_threads[i], NULL);
    }
}