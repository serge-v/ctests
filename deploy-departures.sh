host=`cat ~/.config/departures/host.txt`

if [ x$1 == "x-l" ]; then
	ssh -t $host "
		echo == executing install on host: \$HOSTNAME ==
		sudo cp ~/src/xtree/ctests/departures /usr/local/bin/
		/usr/local/bin/departures -v"
elif [ x$1 == "x-i" ]; then
	ssh -t $host "
		echo == executing info on host: \$HOSTNAME ==
		crontab -l|grep departures
		/usr/local/bin/departures -v"
elif [ x$1 == "x-t" ]; then
	ssh -t $host "
		echo == executing test on host: \$HOSTNAME ==
		crontab -l|grep departures
		/usr/local/bin/departures -v
		/usr/local/bin/departures -f XG -m"
elif [ x$1 == "x-b" ]; then
	echo == executing buld on host: \$HOSTNAME ==
	ssh $host "
		cd ~/src/xtree/ctests
		git pull
		make clean
		make
		echo == new version ==
		./departures -v
		echo == deployed version ==
		/usr/local/bin/departures -v
		echo == crontab ==
		crontab -l|grep departures"

	echo
	echo Project was build on $host. For install run script with -i parameter.
else
	echo usage: deploy-departures.h [-bitl]
	echo '    ' -i       info
	echo '    ' -b       build
	echo '    ' -l       install
	echo '    ' -t       test
fi

