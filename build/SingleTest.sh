./destor /home/zxy/dedup_datasets/$1 -p"chunk-algorithm $2" -p"chunk-avg-size $3" -p"chunk-max-size $4" -p"chunk-min-size $5" && 

./destor -s && 
rm -rf /home/zxy/DestorHome/container.pool /home/zxy/DestorHome/destor.stat /home/zxy/DestorHome/manifest /home/zxy/DestorHome/index/* /home/zxy/DestorHome/recipes/* &&
echo "epoch done."
