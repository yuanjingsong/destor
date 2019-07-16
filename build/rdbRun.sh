#!/bin/bash

function timediff() {

# time format:date +"%s.%N", such as 1502758855.907197692
    start_time=$1
    end_time=$2
    
    start_s=${start_time%.*}
    start_nanos=${start_time#*.}
    end_s=${end_time%.*}
    end_nanos=${end_time#*.}
    
    # end_nanos > start_nanos? 
    # Another way, the time part may start with 0, which means
    # it will be regarded as oct format, use "10#" to ensure
    # calculateing with decimal
    if [ "$end_nanos" -lt "$start_nanos" ];then
        end_s=$(( 10#$end_s - 1 ))
        end_nanos=$(( 10#$end_nanos + 10**9 ))
    fi
    
# get timediff
    time=$(( 10#$end_s - 10#$start_s )).$(( (10#$end_nanos - 10#$start_nanos)/10**6 ))
    
    echo $time
}


rm -rf /home/zxy/DestorHome/container.pool /home/zxy/DestorHome/destor.stat /home/zxy/DestorHome/manifest /home/zxy/DestorHome/index/* /home/zxy/DestorHome/recipes/*
start_pos=$(date +"%s.%N")

for file in `ls /data/datasets/rdb`
do
	./destor /data/datasets/rdb/$file -p"chunk-algorithm $1" -p"chunk-avg-size 8192" -p"chunk-max-size 65536" -p"chunk-min-size 2048" > log_$1/log_$file
done

end_pos=$(date +"%s.%N")
./destor -s

timediff $start_pos $end_pos


