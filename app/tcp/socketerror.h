/*
 * Source File: socketerror.h
 * Author: Ed Alegrid
 * Copyright (c) 2017 Ed Alegrid <ealegrid@gmail.com>
 * GNU General Public License v3.0
 */

#pragma once
#include <errno.h>

class SocketError : public std::exception
{
     std::string erMsg {"null"};
     const char* ErrCode;

     public:
        // with no parameter, error message defaults to errno
        SocketError() : ErrCode(strerror(errno)) {
            if(errno == 0){
                ErrCode = "Can't find any error! Did you throw a SocketError?";
            }
        }
        // enter your own descriptive error message
        SocketError(const char* msg) : erMsg(msg) {}
        virtual ~SocketError() {}
        virtual const char* what() const noexcept override
        {
            if(erMsg == "null") {
                return ErrCode;
            }
            else{
                return erMsg.c_str();
            }
        }
};

