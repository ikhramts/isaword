# Copyright 2011 Iouri Khramtsov.
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

#Settings
CXX := g++
BOOST_LIB_DIR := /usr/local/lib

#Compiling and linking
COMPILE_OPTIONS := -W -Wall
RELEASE_COMPILE_OPTIONS := -O2 $(COMPILE_OPTIONS)
DEBUG_COMPILE_OPTIONS := -O0 -ggdb $(COMPILE_OPTIONS)

LINK_OPTIONS := -Wall --as-needed -shared-libgcc
LIBS := -lboost_regex
RELEASE_LINK_OPTIONS := -O2 $(LINK_OPTIONS)
DEBUG_LINK_OPTIONS := -O0 -ggdb $(LINK_OPTIONS)
TEST_LINK_OPTIONS := -O2 $(LINK_OPTIONS)

#Targets
OBJS := pseudoword_generator.o makewords.o
DEBUG_OBJS := $(addsuffix -debug, $(OBJS))
TEST_OBJS := pseudoword_generator.o-test tests.o-test

# Rules
all: release

clean:
	rm -f *.o *.o-debug *.o-test makewords test_makewords

# Release:
release: clean build_release

build_release: $(OBJS)
	$(CXX) -o makewords $(RELEASE_LINK_OPTIONS) $(OBJS) $(LIBS)
	
%.o: %.cpp
	$(CXX) -c $(RELEASE_COMPILE_OPTIONS) $<

# Debug:
debug: clean build_debug

build_debug: $(DEBUG_OBJS)
	$(CXX) -o makewords $(DEBUG_LINK_OPTIONS) $(DEBUG_OBJS) $(LIBS)

%.o-debug: %.cpp
	$(CXX) -o $@ -c $(DEBUG_COMPILE_OPTIONS) $<

# Test:
test: clean build_test run_test

build_test: $(TEST_OBJS)
	$(CXX) -o test_makewords $(DEBUG_LINK_OPTIONS) $(TEST_OBJS) $(LIBS) -lboost_unit_test_framework

%.o-test: %.cpp
	$(CXX) -o $@ -c $(DEBUG_COMPILE_OPTIONS) $<

run_test:
	@echo ==================================
	@echo Running tests
	@echo ==================================
	./test_makewords
