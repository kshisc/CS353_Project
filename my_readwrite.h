/******************************************************************************/
/* Important CSCI 353 usage information:                                      */
/*                                                                            */
/* This fils is part of CSCI 353 programming assignments at USC.              */
/*         53616c7465645f5faf454acd8de2536dc9ae92d5904738f3b41eed2fa2af       */
/*         35d28e0a435e5f50414281de61958c65444cbd5b46aa7eb42f2c0258ca8a       */
/*         cf3ad79d6e72cd90806987130037a9068c552efff12cec1996639568           */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/*         You are also NOT permitted to distribute or publically display     */
/*         any file (or ANY PART of it) that uses/includes this file.         */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block if you    */
/*         submit this file for grading.                                      */
/******************************************************************************/

/*
 * Author:      William Chia-Wei Cheng (bill.cheng@usc.edu)
 *
 * @(#)$Id: my_readwrite.h,v 1.2 2024/01/06 01:08:48 william Exp $
 */

#ifndef _MY_READWRITE_H_
#define _MY_READWRITE_H_

/**
 * Read a line from the socket and return a C++ string.
 *         Can call this function repeatedly to read one line at a time.
 *         A line ends with a '\n'.
 *         The last line may be a "partial line", i.e., not ends with a '\n'.
 * Return -1 if there is an error (must ignore the returned C++ string).
 *         End-of-file/end-of-input is considered an error.
 *         After this function returns -1, should continue to return -1 if called again.
 * Otherwise, return the length of the returned C++ string.
 *
 * You should be able to use this function as it.
 * You should only call this function if you are expecting a line of text from socket_fd.
 *
 * @param socket_fd - client socket created by create_client_socket().
 * @param line - returned C++ string.
 */
int read_a_line(int socket_fd, string& line);

/**
 * Use this code to write to a socket (not a good idea to call write() directly).
 * Return 0 if successful.
 * Return -1 if there is an error.
 *
 * You should be able to use this function as it.
 *
 * @param fd - socket file descriptor or a regular file descriptor.
 * @param buf - buffer address.
 * @param bytes_to_wrte - number of bytes to write, starting at buf.
 */
int better_write(int fd, const char *buf, int bytes_to_write);

/**
 * Call this function instead of better_write() when you are writing any LINE of a message HEADER into the socket (or the entire message HEADER).
 *         This function will verify that bytes_to_write is > 1, buf[bytes_to_write-2] is '\r', and buf[bytes_to_write-1] is '\n'.
 *         If debugging is not turned on, this function would just check the above and then call better_write().
 * Must not use this function to write any part of a message BODY into the socket because we are supposed to treat message body as binary data!
 */
int better_write_header(int fd, const char *buf, int bytes_to_write);

/**
 * Call better_write_header_debug(1) to turn on debugging in better_write_header().
 * Call better_write_header_debug(0) to turn off debugging in better_write_header().
 */
void better_write_header_debug(int turn_on);

/**
 * Open in_filename_string for reading.
 *
 * @param in_filename_string - file name to open for reading.
 * @return (-1) if file cannot be opened; otherwise, return a file descriptor.
 */
int open_file_for_reading(string in_filename_string);

/**
 * Open out_filename_string for writing (create the file if it doesn't already exist).
 *
 * @param out_filename_string - file name to open for writing.
 * @return (-1) if file cannot be opened; otherwise, return a file descriptor.
 */
int open_file_for_writing(string out_filename_string);

extern int my_debug_header_lines;

#endif /*_MY_READWRITE_H_*/
