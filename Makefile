CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -MMD -MP
TARGET   = brave_tour
SRCDIR   = src

SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(OBJS:.o=.d)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)

.PHONY: clean
