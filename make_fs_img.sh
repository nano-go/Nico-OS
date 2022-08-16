if [ -e hd80MB.img ]; then
  rm hd80MB.img;
fi
truncate -s 80M hd80MB.img
