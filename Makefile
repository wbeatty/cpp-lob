CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
LDFLAGS  :=

TARGET          := cpp-lob
PROFILE_TARGET  := cpp-lob-profile
SOURCES         := main.cpp market.cpp limit.cpp

PROFILE_CXXFLAGS := -g -fno-omit-frame-pointer
PROFILE_OBJDIR   := .build-profile
PROFILE_OBJECTS  := $(SOURCES:%.cpp=$(PROFILE_OBJDIR)/%.o)

.PHONY: all profile clean run run-profile

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

$(PROFILE_OBJDIR):
	mkdir -p $(PROFILE_OBJDIR)

$(PROFILE_OBJDIR)/%.o: %.cpp | $(PROFILE_OBJDIR)
	$(CXX) $(CXXFLAGS) $(PROFILE_CXXFLAGS) -c $< -o $@

profile: $(PROFILE_TARGET)

$(PROFILE_TARGET): $(PROFILE_OBJECTS)
	$(CXX) $(PROFILE_OBJECTS) -o $@ $(LDFLAGS)
	dsymutil $(PROFILE_TARGET)

clean:
	rm -rf $(PROFILE_OBJDIR) $(PROFILE_TARGET).dSYM
	rm -f $(TARGET) $(PROFILE_TARGET)

run: $(TARGET)
	./$(TARGET) < sample_input.txt

run-profile: $(PROFILE_TARGET)
	./$(PROFILE_TARGET) < sample_input.txt
