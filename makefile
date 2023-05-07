server: ./base/http/http_base.cpp ./base/http/http_read.cpp ./base/http/http_write.cpp ./base/timer/timer.cpp main.cpp
	g++ -g $^ -o server
clean:
	rm server