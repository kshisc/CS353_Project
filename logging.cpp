/*
 * g++ -g -Wall -std=c++11 logging.cpp
 */

/* C++ standard include files first */
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

/* C system include files next - none for this program */
/* C standard include files next - none for this program */
/* your own include last - none for this program */

#include "logging.h"

/* global variables */
ostream *mylog=NULL;

static ofstream myfile;

void Init(int argc, char *argv[]) //changed
{
    if (argc > 2) {
        myfile.open(argv[2], ofstream::out|ofstream::app);
        mylog = &myfile;
    } else {
        mylog = &cout;
    }
}

/*
 * This function demonstrate that you can use the ostream to write to cout or a file.
 */
// static
// void LogALine(ostream& logfile, string a_line_of_msg)
// {
//     logfile << a_line_of_msg;
//     logfile.flush();
// }

// /*
//  * Here's another way to use the same function to log to either cout or a log file, depending on the value of the first argument.
//  */
// static
// void LogALineVersion2(int log_to_file, string a_line_of_msg)
// {
//     if (log_to_file) {
//         *mylog << a_line_of_msg;
//         mylog->flush();
//     } else {
//         cout << a_line_of_msg;
//         cout.flush();
//     }
// }

/*
 * This is the same idea as LogALine().  The only difference is that you always use the mylog global variable.
 */
void LogALineVersion3(string a_line_of_msg)
{
    *mylog << a_line_of_msg;
    mylog->flush();
}

void CleanUp(int argc, char *argv[])
{
    if (argc > 2) {
        myfile.close();
        // cout << "Some messages were written into file '" << (char*)(argv[1]) << "' (and this message is written to cout)" << endl;
    } else {
        // cout << "All messages were written into cout (including this message)" << endl;
    }
}

// int main(int argc, char *argv[])
// {
//     Init(argc, argv);
//     LogALine(*mylog, "Line 1.\r\n");
//     LogALine(*mylog, "Line 2.\r\n");
//     LogALine(*mylog, "Line 3.\r\n");
//     LogALineVersion2(1, "Line 4.\r\n");
//     LogALineVersion2(1, "Line 5.\r\n");
//     LogALineVersion2(1, "Line 6.\r\n");
//     LogALineVersion2(0, "Line 4.\r\n");
//     LogALineVersion2(0, "Line 5.\r\n");
//     LogALineVersion2(0, "Line 6.\r\n");
//     CleanUp(argc, argv);

//     return 0;
// }
