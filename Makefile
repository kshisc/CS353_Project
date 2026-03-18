MYDEFS = -g -Wall -std=c++11 -DLOCALHOST=\"127.0.0.1\"

server: server.cpp my_socket.cpp my_socket.h my_readwrite.cpp my_readwrite.h my_timestamp.cpp my_timestamp.h logging.cpp logging.h
	g++ ${MYDEFS} -o server server.cpp my_socket.cpp my_readwrite.cpp my_timestamp.cpp logging.cpp -pthread

clean:
	rm -f *.o server

