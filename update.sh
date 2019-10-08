#script for mail.ru archaic machine that does not even have a boost

sudo apt-get update

sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-7.3.0 g++-7.3.0
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7.3.0 60 --slave /usr/bin/g++ g++ /usr/bin/g++-7.3.0

sudo apt-get install libboost-all-dev -y
sudo apt-get install telnet -y
sudo apt-get install htop -y
