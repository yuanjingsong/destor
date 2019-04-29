echo "******************LNX********************"
rm -rf /dev/shm/dataset/*
cp /home/xiawen/test/LNX /dev/shm/dataset/
./ChunkLader.sh LNX fastcdc
./ChunkLader.sh LNX rabin
./ChunkLader.sh LNX normalized_rabin
./ChunkLader.sh LNX ae
echo "******************RDB********************"
rm -rf /dev/shm/dataset/*
cp /home/xiawen/test/RDB /dev/shm/dataset/
./ChunkLader.sh RDB fastcdc
./ChunkLader.sh RDB rabin
./ChunkLader.sh RDB normalized_rabin
./ChunkLader.sh RDB ae
echo "******************SYN********************"
rm -rf /dev/shm/dataset/*
cp /home/xiawen/test/SYN /dev/shm/dataset/
./ChunkLader.sh SYN fastcdc
./ChunkLader.sh SYN rabin
./ChunkLader.sh SYN normalized_rabin
./ChunkLader.sh SYN ae
echo "******************TAR********************"
rm -rf /dev/shm/dataset/*
cp /home/xiawen/test/TAR /dev/shm/dataset/
./ChunkLader.sh TAR fastcdc
./ChunkLader.sh TAR rabin
./ChunkLader.sh TAR normalized_rabin
./ChunkLader.sh TAR ae
echo "******************VMA********************"
rm -rf /dev/shm/dataset/*
cp /home/xiawen/test/VMA /dev/shm/dataset/
./ChunkLader.sh VMA fastcdc
./ChunkLader.sh VMA rabin
./ChunkLader.sh VMA normalized_rabin
./ChunkLader.sh VMA ae
echo "******************VMB********************"
rm -rf /dev/shm/dataset/*
cp /home/xiawen/test/VMB /dev/shm/dataset/
./ChunkLader.sh VMB fastcdc
./ChunkLader.sh VMB rabin
./ChunkLader.sh VMB normalized_rabin
./ChunkLader.sh VMB ae
echo "******************WEB********************"
rm -rf /dev/shm/dataset/*
cp /home/xiawen/test/WEB /dev/shm/dataset/
./ChunkLader.sh WEB fastcdc
./ChunkLader.sh WEB rabin
./ChunkLader.sh WEB normalized_rabin
./ChunkLader.sh WEB ae

