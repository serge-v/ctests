ctests
======

C language tests

Tests that may be converted to projects sometime
================================================
	mailto     -- console utility to send email to me

Building
========

	cmake .
	make

Generating project for Xcode
==========================

	mkdir ~/src/ctestsxcode
	cd ~/src/ctestsxcode
	cmake -G ~/src/xtree/ctests
