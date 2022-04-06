/*
 * Source File: server.h
 * Author: Ed Alegrid
 * Copyright (c) 2017 Ed Alegrid <ealegrid@gmail.com>
 * GNU General Public License v3.0
 */
#pragma once

#include <thread>
#include <future>
#include <sstream>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h> 
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include "socketerror.h"
#define READ_SIZE 512
#define MAX_EVENTS 5

namespace Tcp {

using namespace std;

class Server
{
    int sockfd, newsockfd, event_count, PORT;
    string IP;
    socklen_t clen;
    sockaddr_in server_addr{}, client_addr{};
    int listenF = false, ServerLoop = false;
    
    size_t bytes_read;

    struct epoll_event event, events[MAX_EVENTS];
    int epoll_fd = epoll_create1(0);
 
    int initSocket(const int &port, const string ip = "127.0.0.1")
    {
        PORT = port;
        IP = ip;
        try
        {
            if(port <= 0){
                throw SocketError("Invalid port");
            }
            sockfd =  socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                throw SocketError();
            }
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

            int reuse = 1;
            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
            if( bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
            {
                throw SocketError();
            }
            else
            {
                listen(sockfd, 5);
                clen = sizeof(client_addr);
            }
            return 0;
        }
        catch (SocketError& e)
        {
            cerr << "Server socket initialize error: " << e.what() << endl;
            closeHandler();
            exit(1);
        }
    }

    void closeHandler() const
    {
        if(close(epoll_fd))
        {
            cerr << "Failed to close epoll file descriptor"  << endl;
        } 
        socketClose();
        delete this;
        exit(1);
    }

    public:
        // use with createServer() method
        Server(){}
        // immediately initialize the server socket with the port provided
        Server(const int &port, const string ip = "127.0.0.1" ): PORT{port}, IP{ip} { initSocket(port, ip); }
        virtual ~Server() {}

        void createServer(const int &port, const string ip = "127.0.0.1")
        {
            initSocket(port, ip);
        }

        void socketListen(int serverloop = false)
        {
            ServerLoop = serverloop;
            try
            {
                auto l = [] (int fd, sockaddr_in client_addr, socklen_t clen)
                {
                    auto newfd = accept4(fd, (struct sockaddr *) &client_addr, &clen, SOCK_NONBLOCK);
                    if (newfd < 0){
                        cout << "Your server seems to be running in continous loop\nbut the socket listener is set to false like socketListen(false)!\nYou need to set it to true like socketListen(true) for continous loop. \n\n";
                        throw SocketError();
                    }
                    return newfd;
                };

                if(!listenF){
                    cout << "Server listening on: " << IP << ":" << PORT << "\n\n";
                    listenF = true;
                }

                auto nfd = std::async(l, sockfd, client_addr, clen);
                newsockfd = nfd.get();

                if(epoll_fd == -1)
                {
                  cerr << "Failed to create epoll file descriptor"  << endl;
                  return; 
                }

                event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP;
                event.data.fd = newsockfd;

                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newsockfd, &event))
                {
                  cerr << "Failed to add file descriptor to epoll"  << endl;
                  close(epoll_fd);
                  return;
                }

                event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

            }
            catch (SocketError& e)
            {
                std::cerr << "Server listen error: " << e.what() << std::endl;
                closeHandler();
            }
        }

        virtual const string readSync()
        {
            char read_buffer[READ_SIZE];
            try
            {
                if(!listenF){
                    throw SocketError("No listening socket!\n Did you forget to start the Listen() method!");
                }

                if(event_count < 0)
                {
                    cout << "Server read sync error: epoll failure" << endl;
                }

                for(int i = 0; i < event_count; i++)
                {
                    if(events[i].data.fd == newsockfd)
                    {
                        if(events[i].events & EPOLLIN) {

                            if (events[i].data.fd < 0){
                                continue;
                            }

                            bzero(read_buffer, sizeof(read_buffer));
                            int n = recv(events[i].data.fd, read_buffer, READ_SIZE - 1, 0);

                            if (n == 0){
                                cout << "Server read sync error, socket is closed or disconnected!\n";
                                break;
                            }
                            if (n < 0) {
                                cout << "Server read sync error: No data available" << endl;
                                break; 
                            }
                        }
                        else
                        {
                            cout << "Server read sync error: something unexpected happened" << endl;
                        }

                        if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
                            cout << "Server read sync error: connection is closed" << endl;
			                      epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
			                      close(events[i].data.fd);
			                      continue;
		                    }
                    }
                    
                }

            }
            catch (SocketError& e)
            {
                cerr << "Server read sync error: " << e.what() << endl;
                closeHandler();
            }
            return read_buffer;
        }

        virtual const string socketRead(const int read_bufsize = READ_SIZE)
        {
            if(!listenF){
              throw SocketError("No listening socket!\n Did you forget to start the Listen() method!");
            }
            string read_async_data = "no available data";
            try
            {
                auto l = [] (const int fd, const int &bufsize)
                {
                    string s;
                    char read_buffer[bufsize];
                    bzero(read_buffer, sizeof(read_buffer));
                    int n = recv(fd, read_buffer, bufsize - 1, 0);
                    if (n == 0){
                        cout << "Server read async error, socket is closed or disconnected" << endl;
                    }
                    if (n < 0) {
                        cout << "Server read async error: No data available" << endl;
                    }
                
                    //s = read_buffer;
                    s = &read_buffer[0];   
                    return s; 
                };

                if(event_count < 0)
                {
                    cout << "Server read async error: epoll failure" << endl;
                }

                int fd = event.data.fd;

                for(int i = 0; i < event_count; i++)
                {
                    if(events[i].data.fd == newsockfd)
                    {
                        if(events[i].events & EPOLLIN) {

                            fd = events[i].data.fd;

                            if (events[i].data.fd < 0){
                                continue;
                            }

                            auto rf = async(l, fd, read_bufsize);
                            read_async_data = rf.get();
                        }
                        else
                        {
                            cout << "Server read async error: something unexpected happened" << endl;
                        }

                        if(events[i].events & (EPOLLRDHUP | EPOLLHUP)){
                            cout << "Server read async error: connection is closed" << endl;
			                      epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
			                      close(events[i].data.fd);
			                      continue;
		                    }
                    }
                }
            }
            catch (SocketError& e)
            {
                cerr << "Server read async error: " << e.what() << endl;
                closeHandler();
            }
            return read_async_data;
        }

        virtual const string sendSync(const string &msg) const
        {
            try
            {
                if(!listenF){
                    throw SocketError("No listening socket!\n Did you forget to start the Listen() method!");
                }
                ssize_t n = send(sockfd, msg.c_str(), strlen(msg.c_str()), 0);
                if (n < 0) {
                    throw SocketError();
                }
            }
            catch (SocketError& e)
            {
                cerr << "Server send sync error: " << e.what() << endl;
                closeHandler();
            }
            return msg;
        }

        virtual const string socketWrite(const string &msg) const
        {
            try
            {
                auto l = [] (const int fd, const string &m)
                {
                    ssize_t n = send(fd, m.c_str(), strlen(m.c_str()), 0);
                    if (n < 0) {
	                    throw SocketError();
	                  }
                };

                auto sf = std::async(l, newsockfd, msg); 
	              sf.get();
            }
            catch (SocketError& e)
            {
                cerr << "Server send async error: " << e.what() << endl;
                closeHandler();
            }
            return msg;
        }

        virtual void socketClose() const
        {
            if(ServerLoop){
                close(newsockfd);
            }
            else{
                close(newsockfd);
                close(sockfd);
            }
        }
};

}
