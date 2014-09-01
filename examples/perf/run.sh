WOO=woo-mt
TAG=iniConParallel
for j in 1 2 4 6; do
	for i in `seq 5`; do $WOO -xn -j$j inclined.py $TAG 10 1000; done
	for i in `seq 5`; do $WOO -xn -j$j inclined.py $TAG 20 1000; done
	for i in `seq 3`; do $WOO -xn -j$j inclined.py $TAG 25 1000; done
	for i in `seq 2`; do $WOO -xn -j$j inclined.py $TAG 30 1000; done
	$WOO -xn -j$j inclined.py $TAG 40 1000
	$WOO -xn -j$j inclined.py $TAG 50 1000
	$WOO -xn -j$j inclined.py $TAG 60 1000
done
