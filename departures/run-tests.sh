function server() {
	subdir=$1
	cd ../tests/$subdir
	python ../server.py 2>&1 >/dev/null  &
	PID=$!
	echo subdir: $subdir, server pid: $PID
	cd -
}

function check() {
	if [[ $? == 1 ]] ; then
		echo -e '\x1b[31m'test failed:'\x1b[0m' $_
	else
		echo test passed: $_
	fi
}

server "01"
sleep 1

# test 1

./departures -f XG -t PO -s > 1~.txt
colordiff -u 1~.txt ../tests/1.txt
check

kill $PID
wait 2> /dev/null




