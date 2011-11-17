#!/bin/sh

qemuopts="-hda obj/kern/kernel.img -hdb obj/fs/fs.img"
. ./grade-functions.sh

TCPDUMP=`PATH=$PATH:/usr/sbin:/sbin which tcpdump`
if [ x$TCPDUMP = x ]; then
	echo "Unable to find tcpdump in path, /usr/sbin, or /sbin" >&2
	exit 1
fi

$make

rand() {
	perl -e "my \$r = int(1024 + rand() * (65535 - 1024));print \"\$r\\n\";"
}

check_testoutput() {
	num=$1
	$TCPDUMP -XX -r qemu.pcap 2>/dev/null | egrep 0x0000 | (
		n=0
		while read line; do
			if [ $n -eq $num ]; then
				fail "extra packets sent"
				return 1
			fi
			if ! echo $line | egrep ": 5061 636b 6574 203. 3. Packet.0?$n$" > /dev/null; then
				fail "incorrect packet $n of $num"
				return 1
			fi
			n=`expr $n + 1`
		done
		if [ $n -ne $num ]; then
			fail "only got $n of $num packets"
			return 1
		fi
	)
	if [ $? = 0 ]; then
		pass
	else
		# The failure happened in a subshell, and thus wasn't
		# reflected in our tally
		fail > /dev/null
	fi
}

wait_for_line() {
	found=0
	for tries in 1 2 3 4 5 6 7 8 9 10; do
		if egrep "$1" jos.out >/dev/null; then
			found=1
			break
		fi
		sleep 1
	done

	if [ $found -eq 0 ]; then
		kill $PID
		wait 2> /dev/null

		echo "missing '$1'"
		fail
		return 1
	fi
}

check_testinput() {
	num=$1

	# Construct the sequence of packet numbers
	nums=""
	i=1
	while [ $i -le $num ]; do
		nums="$nums $(printf '%03d' $i)"
		i=$(expr $i + 1)
	done

	# Wait until it's ready to receive packets
	if ! wait_for_line 'Waiting for packets'; then
		return
	fi

	# Send num UDP packets
	for m in $nums; do
		# Don't use "localhost" here or some versions of
		# netcat will use UDP6, which qemu isn't listening on.
		echo "Packet $m" | nc -u -q 0 127.0.0.1 $echosrv_port
	done

	# Wait for the packets to be processed (1 second is usually
	# enough; if it takes more than 4, something's probably wrong)
	sleep 4

	kill $PID
	wait 2> /dev/null

	egrep '^input: ' jos.out | (
		expect() {
			if ! read line; then
				fail "$name not received"
				exit 1
			fi
			if ! echo "$line" | egrep "$1$" >/dev/null; then
				fail "receiving $name"
				echo "expected input: $1"
				echo "got      $line"
				exit 1
			fi
		}

		# ARP reply (QEMU 0.13.0 padded this out to 64 bytes)
		name="ARP reply"
		expect "0000   5254 0012 3456 ....  .... .... 0806 0001"
		expect "0010   0800 0604 0002 ....  .... .... 0a00 0202"
		expect "0020   5254 0012 3456 0a00  020f 0000 0000 0000"
		expect "0030   0000 0000 0000 0000  0000 0000 0000 0000"

		for m in $nums; do
			name="packet $m/$num"
			hex=$(echo $m | sed -re 's/(.)(.)(.)/3\1 3\23\3/')
			expect "0000   5254 0012 3456 ....  .... .... 0800 4500"
			expect "0010   0027 .... 0000 ..11  .... .... .... 0a00"
			expect "0020   020f .... 0007 0013  .... 5061 636b 6574"
			expect "0030   20$hex 0a00 0000  0000 0000"
		done
	)
	if [ $? = 0 ]; then
		pass
	else
		# The failure happened in a subshell, and thus wasn't
		# reflected in our tally
		fail > /dev/null
	fi
}

check_echosrv() {
	if ! wait_for_line 'bound'; then
		return
	fi

	str="$t0: network server works"
	echo $str | nc -q 3 localhost $echosrv_port > qemu.out

	kill $PID
	wait 2> /dev/null

	if egrep "^$str\$" qemu.out > /dev/null
	then
		pass
	else
		fail
	fi
}

check_httpd() {
	if ! wait_for_line 'Waiting for http connections'; then
		return
	fi

	echo ""

	# Each of the three tests is worth a third of the points
	pts=$(expr $pts / 3)

	perl -e "print '    wget localhost:$http_port/: '"
	if wget -o wget.log -O /dev/null localhost:$http_port/; then
		fail "got back data";
	else
		if egrep "ERROR 404" wget.log >/dev/null; then
			pass;
		else
			fail "did not get 404 error";
		fi
	fi

	perl -e "print '    wget localhost:$http_port/index.html: '"
	if wget -o /dev/null -O qemu.out localhost:$http_port/index.html; then
		if diff qemu.out fs/index.html > /dev/null; then
			pass;
		else
			fail "returned data does not match index.html";
		fi
	else
		fail "got error";
	fi

	perl -e "print '    wget localhost:$http_port/random_file.txt: '"
	if wget -o wget.log -O /dev/null localhost:$http_port/random_file.txt; then
		fail "got back data";
	else
		if egrep "ERROR 404" wget.log >/dev/null; then
			pass;
		else
			fail "did not get 404 error";
		fi
	fi

	kill $PID
	wait 2> /dev/null
}

http_port=`rand`
echosrv_port=`rand`
echo "using http port: $http_port"
echo "using echo server port: $echosrv_port"

qemuopts="$qemuopts -net user -net nic,model=e1000"
qemuopts="$qemuopts -redir tcp:$echosrv_port::7 -redir tcp:$http_port::80"
qemuopts="$qemuopts -redir udp:$echosrv_port::7"
qemuopts="$qemuopts -net dump,file=qemu.pcap"

pts=5
runtest1 -tag 'testtime' testtime -DTEST_NO_NS \
	'starting count down: 5 4 3 2 1 0 ' \

pts=5
runtest1 -tag 'pci attach' hello -DTEST_NO_NS \
	'PCI function 00:03.0 .8086:100e. enabled'

pts=15
rm -f obj/net/testoutput*
rm -f qemu.pcap
runtest1 -tag 'testoutput [5 packets]' -dir net testoutput \
	-DTEST_NO_NS -DTESTOUTPUT_COUNT=5 \
	-check check_testoutput 5

pts=10
rm -f obj/net/testoutput*
rm -f qemu.pcap
runtest1 -tag 'testoutput [100 packets]' -dir net testoutput \
	-DTEST_NO_NS -DTESTOUTPUT_COUNT=100 \
	-check check_testoutput 100

showpart A

# From here on, we need to drive the network while QEMU is running, so
# switch into asynchronous mode.
brkfn=

pts=15
runtest1 -tag "testinput [5 packets]" -dir net testinput -DTEST_NO_NS \
	-check check_testinput 5

pts=10
runtest1 -tag "testinput [100 packets]" -dir net testinput -DTEST_NO_NS \
	-check check_testinput 100

pts=15
runtest1 -tag 'tcp echo server [echosrv]' echosrv \
	-check check_echosrv

pts=30   # Actually 3 tests
runtest1 -tag 'web server [httpd]' httpd \
	-check check_httpd

showpart B

showfinal
