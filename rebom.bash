#!/bin/bash

for i in $(find src -name "*.[ch]pp")
do
	echo ${i}
	uconv -f utf-8 -t utf-8 --remove-signature ${i} > tmp.new
	mv tmp.new ${i}
done
