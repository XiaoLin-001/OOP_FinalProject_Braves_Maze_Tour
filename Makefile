CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
TARGET   = brave_tour
OBJS     = main.o Block.o Player.o Maze.o

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

main.o: main.cpp Maze.h Player.h Block.h
	$(CXX) $(CXXFLAGS) -c main.cpp

Block.o: Block.cpp Block.h Player.h Maze.h
	$(CXX) $(CXXFLAGS) -c Block.cpp

Player.o: Player.cpp Player.h Block.h Maze.h
	$(CXX) $(CXXFLAGS) -c Player.cpp

Maze.o: Maze.cpp Maze.h Block.h Player.h
	$(CXX) $(CXXFLAGS) -c Maze.cpp

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
