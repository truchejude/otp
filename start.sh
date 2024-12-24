sudo rmmod otp
sudo insmod otp.ko num_passwords=3 time=240
./get_mdp_list.sh