Ajimu Script Language
======================

[![Build Status](https://travis-ci.org/emptyland/ajimu.png)](https://travis-ci.org/emptyland/ajimu])

Ajimu - A simple scheme implement. The name from Ajimu Najimi.

Build
-----

Require C++11 supports.

	$ git clone --branch=master --depth=100 --quiet git://github.com/emptyland/ajimu.git ajimu
	$ cd ajimu
	$ scons -Q

if you don't have clang:

	$ export CC=gcc
	$ export CXX=g++
	$ scons -Q

Test
----

	$ cd ajimu
	$ src/all_test
