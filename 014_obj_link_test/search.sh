#!/bin/bash
sudo find . -type f -name "*.o" | xargs objdump -t | grep "\.text.*dev_warn" -B 20 | grep "\.o"
