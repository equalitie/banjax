/**
 * Copyright (c) 2015 eQualit.ie under GNU AGPL v3.0 or later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef SRC_EXCEPTIONS_H_
#define SRC_EXCEPTIONS_H_

#include <exception>

class EncryptionException : public std::exception
{
    virtual const char* what() const throw() { return "Errorr occured during encryption operation"; }
};

class InvalidConfException : public std::exception
{
    virtual const char* what() const throw() { return "Bad configuration errror"; }
};


#endif // SRC_EXCEPTIONS_H_
