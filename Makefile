CC=g++
PYTHON=python3
EXPANDER=python_scripts/expander.py
SRC_FILE=src/main.cpp
EXPANDED_FILE=build/submit.cpp
CXXFLAGS=-std=c++20 -Wall -Wextra -Werror
EXE_FILE=./build/bin/a.out
OFFICIAL_TOOL_DIR=official-tools/target/debug/
URL=https://atcoder.jp/contests/ahc027/tasks/ahc027_a

.PHONY: debug
debug: CXXFLAGS+=-g -O0 -fsanitize=address
debug: DEFINES=-D_FORTIFY_SOURCE=2 -D_GLIBCXX_DEBUG -DVSCODE -DLOCAL
debug: main

.PHONY: release
release: CXXFLAGS+=-O3
release: DEFINES=-DLOCAL
release: main

.PHONY: expand
expand:
	$(PYTHON) $(EXPANDER) $(SRC_FILE) $(EXPANDED_FILE)

.PHONY: main
main: SRCS=$(SRC_FILE)
main: build

.PHONY: build
build: expand $(SRCS)
build: $(SRCS)
	$(CC) $(CXXFLAGS) $(DEFINES) -o $(EXE_FILE) $(SRCS)

.PHONY: submit
submit: expand $(SRCS)
submit: $(SRCS)
	$(CC) $(CXXFLAGS) $(DEFINES) -o $(EXE_FILE) $(EXPANDED_FILE)
	oj submit $(URL) $(EXPANDED_FILE) -y

# usage make gen-testcases BG=1 ED=100 DIR=in/
.PHONY: gen-testcases
gen-testcases:
	seq $(BG) $(ED) > /tmp/seeds.txt
	mkdir -p $(DIR)/$(BG)-$(ED)
	$(OFFICIAL_TOOL_DIR)/gen /tmp/seeds.txt -d $(DIR)/$(BG)-$(ED)
