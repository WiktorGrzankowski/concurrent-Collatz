all:
	g++ -std=c++17 -pthread -o main main.cpp teams.cpp
	g++ -std=c++17 -pthread -o new_process new_process.cpp
