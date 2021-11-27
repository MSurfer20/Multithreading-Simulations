# OS and Networking Assigment - 5
**Name:** Suyash Vardhan Mathur  
**Roll no:** 2019114006
## Directory Strucuture
```
.  
├── README.md
├── q3
│  ├── server.cpp       Main file of the server
│  ├── server           Executable of the server
│  ├── REPORT.md        Report for q3
│  ├── Makefile         Makefile for q3
│  ├── headers.h        Common header file for server and client
│  ├── client.cpp       Main file of the client
│  └── client           Executable of  the client
├── q2
│  ├── REPORT.md        Report for q2
│  ├── q2.c             Main code for q2
│  ├── q2               Executable for q2
│  └── Makefile         Makefile of q2
└── q1
   ├── Report.md        Report for q1
   ├── q1.c             Main file of q1
   ├── q1               Executable of q1
   └── Makefile         Makefile of q1
```
## Instructions for running the code
### Question 1
```
cd q1
make
./q1
```
### Question 2
```
cd q2
make
./q2
```
### Question 3
```
cd q3
make
./client -> Run the client
./server [NUMER OF THREAD POOLS] -> Run the server
```