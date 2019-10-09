clear && g++ --version && (cd final && cmake . && make) && /home/box/final/final -h 127.0.0.1 -p 11111 -d /tmp/ && echo "<p>test</p>" > /tmp/test.txt && telnet 127.0.0.1 11111
