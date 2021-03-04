#============================================================================
# Name        : Makefile
# Author      : Benjamin T. Cecchetto
# Version     :
# Copyright   : (c) 2016 Benjamin T. Cecchetto
# Description : Makefile for ScatteringExplorer Project
#============================================================================

CC=gcc
CXX=g++
RM=rm -f

INC=-I../glew-1.13.0/include  -I../soil/src -I../glm/glm
CPPFLAGS=-g -Werror -Wall -Wextra -std=c++11 $(shell root-config --cflags) $(INC)

LDFLAGS=-g $(shell root-config --ldflags)
LDPATHS=-L../soil/lib -L/usr/local/lib
LDLIBS=$(shell root-config --libs) $(LDPATHS) -lSOIL -lSDL2 -lSDL2_ttf -lGLEW -lGLU -lGL

SRCS := $(shell find . -name "*.cpp")

OBJS=$(subst .cpp,.o,$(SRCS))

all: visualgl

visualgl: $(OBJS)
	$(CXX) $(LDFLAGS) -o scatterex $(OBJS) $(LDLIBS)

depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS) visualgl

dist-clean: clean
	$(RM) *~ .depend
	find . -name "*~" -delete

include .depend
