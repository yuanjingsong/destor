./destor /dev/shm/dataset/$1 -p"chunk-algorithm $2" -p"chunk-avg-size $3" -p"chunk-max-size $4" -p"chunk-min-size $5" && 

./destor -s && 
rm -rf /dev/shm/data/working/container.pool /dev/shm/data/working/destor.stat /dev/shm/data/working/manifest /dev/shm/data/working/index/* /dev/shm/data/working/recipes/* &&
echo "epoch done."
