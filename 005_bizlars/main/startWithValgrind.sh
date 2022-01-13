#!/bin/sh

valgrind --tool=memcheck --leak-check=full --db-attach=no ./adctest.x