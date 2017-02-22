#!/bin/bash
OUTPUT=test_result.csv
rm $OUTPUT
echo -n "# children," >> $OUTPUT

for i in `seq 1 $1`;
do
	echo -n "trial $i," >> $OUTPUT
done

for i in `seq 1 10`;
do
	echo "Test with $i children"
	echo "" >> $OUTPUT
	echo -n "$i," >> $OUTPUT
	for j in `seq 1 $1`;
	do
		#time ./run.sh $i > /dev/null
		START=$(date +%s.%N)
		./run.sh $i > /dev/null
		END=$(date +%s.%N)
		DIFF=$(echo "$END - $START" | bc)
		echo -n "$DIFF," >> $OUTPUT
		echo $DIFF
	done
	echo "----------------------------"
	sleep 1
done
