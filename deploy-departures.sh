host=`cat ~/.config/departures/host.txt`

if [ x$1 == "x-i" ]; then
	ssh $host "sudo cp ~/src/xtree/ctests/departures /usr/local/bin/; /usr/local/bin/departures -v;"
else
	ssh $host "
		cd ~/src/xtree/ctests
		git pull
		make
		echo == new version ==
		./departures -v
		echo == deployed version ==
		/usr/local/bin/departures -v
		echo == crontab ==
		crontab -l|grep departures"

	echo
	echo Project was build on $host. For install run script with -i parameter.
fi

