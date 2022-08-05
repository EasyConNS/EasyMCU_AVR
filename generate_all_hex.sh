# your EasyCon/Firmware path
EasyCon_Firmware_path=../EasyCon/Firmware

# add new board here
Boards=(UNO Teensy2 Leonardo Teensy2pp Beetle)
# increase new verrsion here
Version=6

for board in ${Boards[@]};
do
	echo $board
	echo "make clean it" $board
	make -f mk/$board.mk clean
	echo "make" $board
	make -f mk/$board.mk
	cp -f ./$board/$board.hex  ${EasyCon_Firmware_path}/$board\ v${Version}.hex
	echo "make" $board "completed"
done