#Copyright 2011 Iouri Khramtsov.
#
# This software is available under Apache License, Version 
# 2.0 (the "License"); you may not use this file except in 
# compliance with the License. You may obtain a copy of the
# License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

# A generic makefile.

# --- Locations of things
CXX := g++
BOOST_LIB_DIR := /usr/local/lib
LIBEVENT := /usr/local/lib/libevent-2.0.so.5
BIN_DIR := bin/
TEST_DIR := test

# --- Main components.
BIN := isawordd
SRC := http_server.cpp http_utils.cpp file_handler.cpp views.cpp file_cache.cpp word_picker.cpp

# --- Settings
CFLAGS := -W -Wall -g -L$(BOOST_LIB_DIR)
LDFLAGS := -Wall
LIBS := -lboost_regex $(LIBEVENT)
TEST_LIBS := -lboost_unit_test_framework -lboost_filesystem

# --- Ingredients
TEST_BIN := $(TEST_DIR)/test_$(BIN)
MAIN_SRC := $(BIN).cpp
TEST_SRC := $(TEST_BIN).cpp
OBJS := $(subst .cpp,.o,$(SRC))
MAIN_OBJ := $(BIN).o
TEST_OBJ := test_$(MAIN_OBJ)

# --- Rules

all: release

clean:
	rm -f *.o $(BIN_DIR)* $(TEST_BIN)

# General build rules
build: $(OBJS) $(MAIN_OBJ) 
	$(CXX) -o $(BIN_DIR)$(BIN) $(LDFLAGS) $(OBJS) $(MAIN_OBJ) $(LIBS)
	
%.o: %.cpp
	$(CXX) -c $(CFLAGS) $<
	
# Release:
release: $(CFLAGS) += -O2
release: $(LDFLAGS) += -O2
release: build

# Debug:
debug: CFLAGS += -O0
debug: LDFLAGS += -O0
debug: build

# Test:
test: CFLAGS += -O2
test: LDFLAGS += -O2
test: build_test run_test

test_debug: CFLAGS += -O0
test_debug: LDFLAGS += -O0
test_debug: build_test run_test

build_test: $(OBJS) $(TEST_OBJ)
	$(CXX) -o $(TEST_BIN) $(CFLAGS) $(OBJS) $(TEST_OBJ) $(LIBS) $(TEST_LIBS)

run_test:
	@echo ==================================
	@echo Running tests
	@echo ==================================
	cd $(TEST_DIR);./test_$(BIN)
