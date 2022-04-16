if [ -e hd80MB.img ]; then
  rm hd80MB.img;
fi
truncate -s 80M hd80MB.img
gdisk hd80MB.img << EOF
n
1
2048
+20M
8300
w

Y

EOF
