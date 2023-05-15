server: ./base/webserver.cpp ./base/http/http_base.cpp ./base/http/http_read.cpp ./base/http/http_write.cpp ./base/timer/timer.cpp ./base/log/log.cpp main.cpp
	g++ -g $^ -o server
clean:
	rm server