CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude
LDFLAGS  :=

TARGET          := cpp-lob
PROFILE_TARGET  := cpp-lob-profile
SOURCES         := src/main.cpp src/order_processor.cpp src/matching_engine.cpp src/output_engine.cpp

PROFILE_CXXFLAGS := -g -fno-omit-frame-pointer
PROFILE_OBJDIR   := .build-profile
PROFILE_OBJECTS  := $(SOURCES:%.cpp=$(PROFILE_OBJDIR)/%.o)

.PHONY: all profile clean run run-profile test

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

$(PROFILE_OBJDIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(PROFILE_CXXFLAGS) -c $< -o $@

profile: $(PROFILE_TARGET)

$(PROFILE_TARGET): $(PROFILE_OBJECTS)
	$(CXX) $(PROFILE_OBJECTS) -o $@ $(LDFLAGS)
	dsymutil $(PROFILE_TARGET)

TEST_SOURCES := tests/cancel_test.cpp src/matching_engine.cpp src/order_processor.cpp src/output_engine.cpp

test: $(TEST_SOURCES)
	$(CXX) $(CXXFLAGS) -g -fsanitize=address,undefined -o cancel-test $(TEST_SOURCES) $(LDFLAGS)
	./cancel-test

clean:
	rm -rf $(PROFILE_OBJDIR) $(PROFILE_TARGET).dSYM
	rm -f $(TARGET) $(PROFILE_TARGET) cancel-test

run: $(TARGET)
	./$(TARGET) -f data/test1.txt

run-profile: $(PROFILE_TARGET)
	./$(PROFILE_TARGET) -f data/test1.txt
