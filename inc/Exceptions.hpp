/* 
 * File:   Exceptions.h
 * Author: zapreevis
 *
 * Created on April 18, 2015, 1:31 PM
 */
#ifndef EXCEPTIONS_HPP
#define	EXCEPTIONS_HPP

#include <exception>    // std::exception
#include <string>       // std::string

using namespace std;

/**
 * This is an application exception class that is capable of storing an error message
 */
class Exception : public exception {
private:
    //The error message to be stored
    string msg;
    
public:
    
    explicit Exception(const char * message) : exception(), msg(message) {
    }
    
    explicit Exception(const string &message) : exception(), msg(message) {
    }

    /**
     * The copy constructor
     * @param other the other exception to copy from
     */
    Exception(Exception const & other) {
        this->msg = other.msg;
    }
    
    /**
     * This method returns the stored message
     * @return the reference to a constant error message string
     */
    string const & getMessage() const throw () {
        return msg;
    }
    
    /** Destructor.
     * Virtual to allow for subclassing.
     */
    virtual ~Exception() throw (){}
    
    /** Returns a pointer to the (constant) error description.
     *  @return A pointer to a \c const \c char*. The underlying memory
     *          is in posession of the \c Exception object. Callers \a must
     *          not attempt to free the memory.
     */
    virtual const char* what() const throw (){
       return msg.c_str();
    }
};

#endif	/* EXCEPTIONS_HPP */

