host=`cat ~/.config/departures/host.txt`
dir=~/src/xtree/ctests/departures

if [ x$1 == "x-l" ]; then
	ssh -t $host "
		echo == executing install on host: \$HOSTNAME ==
		sudo cp ~/src/xtree/ctests/departures/departures /usr/local/bin/
		/usr/local/bin/departures -v"
elif [ x$1 == "x-i" ]; then
	ssh -t $host "cd $dir; source ./deploy-funcs.sh; info"
elif [ x$1 == "x-c" ]; then
	shift
	ssh -t $host "
		echo == executing command on host: \$HOSTNAME ==
		$*"
elif [ x$1 == "x-t" ]; then
	ssh -t $host "
		echo == executing test on host: \$HOSTNAME ==
		crontab -l|grep departures
		/usr/local/bin/departures -v
		/usr/local/bin/departures -f XG -t HB -m"
elif [ x$1 == "x-b" ]; then
	ssh $host "
		if [ ! -d ~/src/xtree ]; then
			mkdir -p ~/src/xtree
			cd ~/src/xtree
			git clone https://github.com/serge-v/ctests
			cd ctests
		fi
		cd ~/src/xtree/ctests/departures
		cmake .
		echo == executing buld on host: \$HOSTNAME ==
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
	echo '    ' -c       run command on remote server
fi

