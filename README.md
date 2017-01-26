# vs_httpd

Very Simple HTTP server that can deliver static files very fast

## Install Prerequisiste Packages and Build

Please install Apache Portable Runtime (apr) core & its dev headers and Asynchronous event notification library (libevent) core & its dev headers if not yet installed on your environment

```
# for Ubuntu,Debian apt users
sudo apt-get install libapr1 libapr1-dev
sudo apt-get install libevent-dev

# for CentOS,Fedora,Oracle Linux,Red Hat Enterprise Linux
sudo yum install apr apr-devel
sudo yum install libevent-devel
```

Once all prerequisite packages are installed, you're ready to build vs_httpd. Just simply exec make like this:

```
$ make clean  (only if you have an existing vs_httpd binary)
    rm -f *.o vs_httpd
$ make
    gcc   -DLINUX -D_REENTRANT -D_GNU_SOURCE -I/usr/include/apr-1.0  -I/usr/include   -c -o vs_httpd.o vs_httpd.c
    gcc vs_httpd.o  -L/usr/lib/x86_64-linux-gnu -lapr-1 -levent -o vs_httpd   
```

## Usage

Assuming all required packages are installed and rightly configured, you're ready to run vs_httpd

```
$ vs_httpd -h

Usage: vs_httpd [-a address] [-p port] [-d documentroot]
               [-D] [-v] [-h]
Options:
  -a address      : define server address (default: "0.0.0.0")
  -p port         : define server port (default: 8080)
  -d documentroot : define document root with full path (default: "./")
  -D              : daemonize option 0-off,1-on (default: 0)
  -v              : verbose option 0-off,1-on (default: 0)
  -h              : list available command line options (this page)
```

For examples, you can run vs_httpd: 
```
# Running with documentroot, and verbose  (default port: 8080)
./vs_httpd -d /tmp/pages -v
# send test request
curl localhost:8080/index.html

# Running with port, documentroot, and daemonize option
./vs_httpd  -p 9999 -d /tmp/pages -D
# send test request
curl localhost:9999/index.html
```

## Benchmark with apache bench (ab -n 1000 -c 10)

### Apache/2.4.12
```
Concurrency Level:      10
Time taken for tests:   0.274 seconds
Complete requests:      1000
Failed requests:        0
Total transferred:      286000 bytes
HTML transferred:       40000 bytes
Requests per second:    3649.66 [#/sec] (mean)
Time per request:       2.740 [ms] (mean)
Time per request:       0.274 [ms] (mean, across all concurrent requests)
Transfer rate:          1019.34 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.2      0       2
Processing:     1    3   2.8      2      86
Waiting:        0    1   2.7      1      84
Total:          1    3   2.8      2      86

Percentage of the requests served within a certain time (ms)
  50%      2
  66%      3
  75%      3
  80%      3
  90%      3
  95%      3
  98%      4
  99%     10
 100%     86 (longest request)
```

### vs_httpd
```
Concurrency Level:      10
Time taken for tests:   0.194 seconds
Complete requests:      1000
Failed requests:        0
Total transferred:      102000 bytes
HTML transferred:       40000 bytes
Requests per second:    5149.70 [#/sec] (mean)
Time per request:       1.942 [ms] (mean)
Time per request:       0.194 [ms] (mean, across all concurrent requests)
Transfer rate:          512.96 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        1    1   0.3      1       5
Processing:     0    1   0.3      1       5
Waiting:        0    0   0.3      0       1
Total:          1    2   0.4      2       6

Percentage of the requests served within a certain time (ms)
  50%      2
  66%      2
  75%      2
  80%      2
  90%      2
  95%      2
  98%      2
  99%      3
 100%      6 (longest request)
```


## Issues

* [Current Issues, bugs, and requests](https://github.com/yokawasa/vs_httpd/issues)

## Change log

* [Changelog](CHANGELOG.md)

## Contributing

Bug reports and pull requests are welcome on GitHub at https://github.com/yokawasa/vs_httpd.
