#include "headers.h"
using namespace std;

#define BACKLOG 2048

pthread_mutex_t pool_mutex, print_lock;
pthread_cond_t pool_cond;

queue<int*> connections;
string dict[100000];
pthread_mutex_t dict_lock[100000];

string dict_fetch(int key)
{
    pthread_mutex_lock(&dict_lock[key]);
    string ret_str;
    if(dict[key]=="")
    {
        ret_str="Key does not exist";
    }
    else
    {
        ret_str=dict[key];
    }
    pthread_mutex_unlock(&dict_lock[key]);
    return ret_str;
}

string dict_concat(int key1, int key2)
{
    pthread_mutex_lock(&dict_lock[key1]);
    pthread_mutex_lock(&dict_lock[key2]);
    string ret_str;
    if(dict[key1]=="" || dict[key2]=="")
    {
        ret_str = "Concat failed as at least one of the keys does not exist";
    }
    else
    {
        string t1=dict[key1];
        string t2=dict[key2];
        dict[key1]=t1+t2;
        dict[key2]=t2+t1;
        ret_str=t2+t1;
    }
    pthread_mutex_unlock(&dict_lock[key1]);
    pthread_mutex_unlock(&dict_lock[key2]);
    return ret_str;
}

string dict_delete(int key)
{
    pthread_mutex_lock(&dict_lock[key]);
    string ret_str;
    if(dict[key]=="")
    {
        ret_str="No such key exists";
    }
    else
    {
        dict[key]="";
        ret_str="Deletion successful";
    }
    pthread_mutex_unlock(&dict_lock[key]);
    return ret_str;
}

string dict_update(int key, string val)
{
    pthread_mutex_lock(&dict_lock[key]);
    string ret_str;
    if(dict[key]=="")
    {
        ret_str="Key does not exist";
    }
    else
    {
        dict[key]=val;
        ret_str=dict[key];
    }
    pthread_mutex_unlock(&dict_lock[key]);
    return ret_str;
}

string dict_insert(int key, string val)
{
    pthread_mutex_lock(&dict_lock[key]);
    string ret_str;
    if(dict[key]!="")
    {
        ret_str="Key already exists";
    }
    else
    {
        dict[key]=val;
        ret_str="Insertion successful";
    }
    pthread_mutex_unlock(&dict_lock[key]);
    return ret_str;
}


void* connection_handler(void* arg)
{
    // cout<<"AAAAAAAAAAAAAAAAAAAAAA";
    char buffer[MAX_BUF_SIZE+1],  sendline[MAX_BUF_SIZE+1];
    char recvline[MAX_BUF_SIZE+1];
    size_t n;
    string response;
    memset(recvline, 0, MAX_BUF_SIZE);
    int connection_fd = *(int*)arg;

    while((n=read(connection_fd, recvline, MAX_BUF_SIZE-1)) >0)
    {
        //MESSAGE
        printf("%s", recvline);
        if(recvline[n-1] == '\n')
        {
            break;
        }
        memset(recvline, 0, MAX_BUF_SIZE);
    }
    // cout<<"BBBBBBBBBBBBBBBBBBBBBBBBB";

    if(n<0)
    {
        printf("ERROR IN READING.\n");
        return NULL;
        // exit(-1);
    }

    string command, value;
    int key1, key2;
    stringstream tokenizer(recvline);
    
    tokenizer>>command;
    tokenizer>>key1;

    if(command=="insert")
    tokenizer>>value;
    else if(command=="update")
    tokenizer>>value;
    else if(command=="concat")
    tokenizer>>key2;

    if(command=="fetch")
    response = dict_fetch(key1);
    else if(command=="concat")
    response=dict_concat(key1, key2);
    else if(command=="insert")
    response=dict_insert(key1, value);
    else if(command=="delete")
    response=dict_delete(key1);
    else if(command=="update")
    response=dict_update(key1, value);
    //Send a response
    snprintf(sendline, MAX_BUF_SIZE+1, "%s", response.c_str());

    sleep(2);


    write(connection_fd, sendline, strlen(sendline));
    close(connection_fd);

    free(arg);
    return NULL;
}

void* thread_function(void* arg)
{
    while(true)
    {
        pthread_mutex_lock(&pool_mutex);
        while(connections.size() == 0)
        {
            pthread_cond_wait(&pool_cond, &pool_mutex);
        }
        int* conn = connections.front();
        // cout<<"SECCCCC";
        connections.pop();
        pthread_mutex_unlock(&pool_mutex);
        connection_handler(conn);
    }
}

int main(int argc, char** argv)
{
    if(argc!=2)
    {
        cout<<"ERROR: Invalid number of arguments."<<endl;
        exit(1);
    }
    pthread_mutex_init(&pool_mutex, NULL);
    pthread_cond_init(&pool_cond, NULL);
    pthread_mutex_init(&print_lock, NULL);

    for(int i=0;i<DICTIONARY_SIZE;i++)
    {
        pthread_mutex_init(&dict_lock[i], NULL);
        dict[i]="";
    }

    int pool_thread_count;
    pool_thread_count=atoi(argv[1]);
    pthread_t thread_pool[pool_thread_count];


    for(int i=0;i<pool_thread_count;i++)
    {
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }

    int listenfd, connection_fd;
    struct sockaddr_in servaddr;
    char client_address[MAX_BUF_SIZE+1], readable_client_address[MAX_BUF_SIZE+1];

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

    if(bind(listenfd, (SA*)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("Bind error");
        exit(-1);
    }

    if((listen(listenfd, BACKLOG)) <0)
    {
        perror("Listening error");
        exit(-1);
    }

    while(true)
    {
        struct sockaddr_in addr;
        socklen_t addr_length;

        printf("WAITING FOR A CONNECTION ON PORT %d\n", PORT);
        fflush(stdout);

        connection_fd = accept(listenfd, (SA*)&addr, &addr_length);

        inet_ntop(AF_INET, &addr, client_address, MAX_BUF_SIZE);
        cout<<client_address<<endl;

        
        pthread_t thrd;

        pthread_mutex_lock(&pool_mutex);
        int *pclient = (int*)malloc(sizeof(int));
        *pclient = connection_fd;
        connections.push(pclient);
        pthread_cond_signal(&pool_cond);
        pthread_mutex_unlock(&pool_mutex);
        // pthread_create(&thrd, NULL, connection_handler, (void*)pclient);
    }

    return 0;
}