ctests
======

C language tests

Tests that may be converted to projects sometime
================================================

	departures -- Fetch NJ transit data and print next departing train
	mailto     -- console utility to send email to me
	weather    -- console utility to fetch weather forecast from NOAA

Building
========

	cmake .
	make

Generating project for Xcode
==========================

	mkdir ~/src/ctestsxcode
	cd ~/src/ctestsxcode
	cmake -G ~/src/xtree/ctests
