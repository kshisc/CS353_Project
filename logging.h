#ifndef LOGGING_H
#define LOGGING_H

/* Global log stream */
extern ostream *mylog;

/* Initialize logging system
 * If argc > 1, argv[1] is logfile name
 */
void Init(int argc, char *argv[]);

/* Log a line using global mylog */
void LogALineVersion3(string a_line_of_msg);

/* Cleanup logging system */
void CleanUp(int argc, char *argv[]);

#endif