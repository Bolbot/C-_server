#script for mail.ru archaic machine that does not even have a boost and uses g++-4.8.4 which alas does not support <regex>

sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-4.9 g++-4.9
sudo apt-get install build-essential
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 60 --slave /usr/bin/g++ g++ /usr/bin/g++-4.9
sudo apt-get install gcc-5 g++-5
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 70 --slave /usr/bin/g++ g++ /usr/bin/g++-5

sudo apt-get install libboost-all-dev -y
sudo apt-get install telnet -y
sudo apt-get install htop -y
sudo apt-get update
