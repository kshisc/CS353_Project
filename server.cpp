/* C++ standard include files first */
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <memory> //for shared pointer
#include <sys/time.h>
#include <mutex>
#include <condition_variable>
#include <queue>

using namespace std;

/* C system include files next */
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>

/* C standard include files next */
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

/* your own include last */
#include "my_socket.h"
#include "my_readwrite.h"
#include "my_timestamp.h"
#include "logging.h"


static int listen_socket_fd = (-1); /* there is nothing wrong with using a global variable */
static mutex m; //first-level

class Connection {
public:
    int conn_number; /* -1 means that the connection is not initialized properly */
    int socket_fd; /* -1 means closed by connection-handling thread, -2 means close by console thread and connection may still be active */
    int orig_socket_fd; /* copy of the original socket_fd */
    shared_ptr<thread> thread_ptr; /* shared pointer to a thread object */
    int kb_sent; /* number of KB (including partial KB at the end) of response body written into the socket */
    Connection() : conn_number(-1), socket_fd(-1), thread_ptr(NULL), kb_sent(0) { }
    Connection(int c, int s, shared_ptr<thread> p) { conn_number = c; socket_fd = orig_socket_fd = s; thread_ptr = p; kb_sent = 0; }
};

vector<shared_ptr<Connection>> connection_list;
static int next_conn_number = 1;

queue<shared_ptr<Connection>> reaper_q;
static condition_variable cv;

void reaper_add_work(shared_ptr<Connection> c)
{
    m.lock();
    reaper_q.push(c);
    cv.notify_all();
    m.unlock();
}

shared_ptr<Connection> reaper_wait_for_work()
{
    unique_lock<mutex> l(m);
    while (reaper_q.empty()) 
        cv.wait(l);
    shared_ptr<Connection> c = reaper_q.front();
    reaper_q.pop();
    return c;
}

static int get_file_size(const string &path)
{
    struct stat sb;
    if (stat(path.c_str(), &sb) != 0) return -1;
    return (int)sb.st_size;
}

void log_with_timestamp(string msg)
{
    m.lock();
    string line = "[" + get_timestamp_now() + "] " + msg;
    LogALineVersion3(line);
    m.unlock();
}

static
void talk_to_client(shared_ptr<Connection> conn_ptr)
{
    m.lock();
    m.unlock();
    
    string client_ip_and_port = get_ip_and_port_for_server(conn_ptr->socket_fd, 0);

    for (;;) {
        //parse header
        vector<string> req_lines; 
        string req_line;
        for (;;) {
            int bytes_received = read_a_line(conn_ptr->socket_fd, req_line);
            if (bytes_received <= 0) goto done;
            req_lines.push_back(req_line);
            if (req_line == "\r\n") break; //end of header
        }

        //extract first line
        string method, uri, version;
        string first = req_lines[0];
        while (!first.empty() && (first.back() == '\r' || first.back() == '\n')) { //remove "\r\n"
            first.pop_back();
        }
        stringstream ss1(first);
        ss1 >> method >> uri >> version;

        log_with_timestamp("[" +
            to_string(conn_ptr->conn_number) + "]\tClient connected from " +
            client_ip_and_port + " and requesting " + uri + "\n");

        //content type
        string content_type = "application/octet-stream";
        size_t dot_pos = uri.find_last_of('.');
        string ext = uri.substr(dot_pos+1);
        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == "html") content_type = "text/html";

        //check validity
        bool valid = (method == "GET") && (version.find("HTTP/1.") == 0);

        //get file size
        string path = "lab4data" + uri;
        int file_size = -1;
        if (valid) file_size = get_file_size(path);

        //send response
        bool sendFile = valid && (file_size >= 0);

        string respHeader = sendFile ? //header
            "HTTP/1.1 200 OK\r\n"
            "Server: lab4a\r\n"
            "Content-Type: " + content_type + "\r\n"
            "Content-Length: " + to_string(file_size) + "\r\n"
            "\r\n"
            :
            "HTTP/1.1 404 Not Found\r\n"
            "Server: lab4a\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 63\r\n"
            "\r\n"
        ;
        int w1 = better_write_header(conn_ptr->socket_fd, respHeader.c_str(), respHeader.size()); 
        if (w1 < 0) goto done;

        if (sendFile) { //body
            int fd = open(path.c_str(), O_RDONLY);
            if (fd >= 0) {
                char buf[1024];
                int bytes_left = file_size;

                while (bytes_left > 0) {
                    int buf_size = (bytes_left > 1024) ? 1024 : bytes_left;
                    int bytes_read = read(fd, buf, buf_size);
                    if (bytes_read <= 0) break;

                    for (int i = 0; i < bytes_read; i++) { //1 byte at a time
                        m.lock();
                        if ( (conn_ptr->socket_fd == -2) || (listen_socket_fd == -1) ) {
                            m.unlock();
                            break;
                        }
                        m.unlock();

                        int w2 = better_write(conn_ptr->socket_fd, &buf[i], 1);
                        if (w2 < 0) break;
                        usleep(250);
                    }

                    bytes_left -= bytes_read;
                    m.lock();
                    conn_ptr->kb_sent++;
                    m.unlock();
                }
                close(fd);
            }
        }
        else{
            string body = "<html><head></head><body><h1>404 Not Found</h1></body></html>\r\n";
            better_write(conn_ptr->socket_fd, body.c_str(), body.size());
        }
    }

    done:
        m.lock();
        if (conn_ptr->socket_fd >= 0)
            shutdown(conn_ptr->socket_fd, SHUT_RDWR);
        conn_ptr->socket_fd = -1;
        m.unlock();

        reaper_add_work(conn_ptr);
        
        log_with_timestamp("[" +
            to_string(conn_ptr->conn_number) + "]\tConnection closed with client at " +
            client_ip_and_port + "\n");
}

void console_thread_func()
{
    for (;;) {
        cout << "> ";
        cout.flush();

        string cmd;
        getline(cin, cmd);

        if (cmd == "") continue;

        if (cmd.substr(0,5) == "close") {
            istringstream iss(cmd);
            string word;
            int num;
            iss >> word >> num;

            m.lock();
            bool found = false;
            for (auto &c : connection_list) {
                if( (c->conn_number == num) && (c->socket_fd >= 0) ){
                    found = true;
                    shutdown(c->socket_fd, SHUT_RDWR);
                    c->socket_fd = -2;
                    break;
                }
            }
            m.unlock();

            if (found) 
                cout << "[" << num << "]\tClose connection requested.\n";
            else
                cout << "Invalid or inactive connection number: " << num << "\n";
        }

        else if (cmd == "status") {
            m.lock();
            bool any = false;
            for (auto &c : connection_list) {
                if (c->socket_fd >= 0){
                    any = true;
                    cout << "[" << c->conn_number << "]\tClient: "
                        << get_ip_and_port_for_server(c->orig_socket_fd, 0)
                        << ", Socket: " << c->socket_fd
                        << ", KB sent: " << c->kb_sent << "\n";
                }
            }
            m.unlock();
            
            if (!any)
                cout << "No active connections.\n";
        }

        else if (cmd == "quit") {
            m.lock();
            for (auto &c : connection_list) {
                if (c->socket_fd >= 0) {
                    shutdown(c->socket_fd, SHUT_RDWR);
                    c->socket_fd = -2;
                }
            }

            shutdown(listen_socket_fd, SHUT_RDWR);
            close(listen_socket_fd);
            listen_socket_fd = -1;
            m.unlock();
            break;
        }

        else { //help or unknown
            cout << "Available commands are:\n"
            << "\tclose #\n"
            << "\thelp\n"
            << "\tstatus\n"
            << "\tquit\n";
        }
    }
}

void reaper_thread_func()
{
    for (;;) {
        shared_ptr<Connection> c = reaper_wait_for_work();
        if (c == NULL)
            break;

        shared_ptr<thread> t = c->thread_ptr;
        if (t && t->joinable())
            t->join();

        log_with_timestamp("[" + to_string(c->conn_number) +
            "]\tReaper thread has joined with socket-reading thread\n");

        m.lock();
        close(c->orig_socket_fd);
        auto itr = find(connection_list.begin(), connection_list.end(), c); 
        if (itr != connection_list.end())
            connection_list.erase(itr);
        m.unlock();
    }

    for (;;) {
        m.lock();
        if (connection_list.empty()) {
            m.unlock();
            break;
        }

        shared_ptr<Connection> c2 = connection_list.front();
        m.unlock();

        shared_ptr<thread> t2 = c2->thread_ptr;
        if (t2 && t2->joinable())
            t2->join();

        log_with_timestamp("[" + to_string(c2->conn_number) +
            "]\tReaper thread has joined with socket-reading thread\n");

        m.lock();
        close(c2->orig_socket_fd);
        connection_list.erase(connection_list.begin());
        m.unlock();
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3) return 1;
    Init(argc, argv);
    
    listen_socket_fd = create_listening_socket(argv[1]);
    if (listen_socket_fd == -1) return 1;

    string ip_port  = get_ip_and_port_for_server(listen_socket_fd, 1);
    log_with_timestamp("Server " + ip_port + " started\n");

    shared_ptr<thread> console_thr = make_shared<thread>(console_thread_func);
    shared_ptr<thread> reaper_thr = make_shared<thread>(reaper_thread_func);

    for (;;) {
        int newsockfd = my_accept(listen_socket_fd);
        if (newsockfd == -1) break;

        m.lock();
        if (listen_socket_fd == -1) {
            shutdown(newsockfd, SHUT_RDWR);
            close(newsockfd);
            m.unlock();
            break;
        }

        shared_ptr<Connection> conn_ptr = make_shared<Connection>(Connection(next_conn_number++, newsockfd, NULL));
        shared_ptr<thread> thr_ptr = make_shared<thread>(thread(talk_to_client, conn_ptr));
        conn_ptr->thread_ptr = thr_ptr;
        connection_list.push_back(conn_ptr);
        m.unlock();
    }

    console_thr->join();
    reaper_add_work(NULL);
    reaper_thr->join();
    
    log_with_timestamp("Server " + ip_port + " stopped\n");
    CleanUp(argc, argv);
    
    return 0;
}
