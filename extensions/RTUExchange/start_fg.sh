#!/bin/sh

uniset-start.sh -f ./uniset-rtuexchange --confile test.xml \
	--rs-dev /dev/cbsideA0 \
	--rs-name RSExchange \
	--rs-speed 38400 \
	--rs-filter-field rs \
	--rs-filter-value 3 \
	--dlog-add-levels crit,warn \
	--rs-force 0 \
	--rs-force-out 0 \
#,level3
#	--rs-force 1 \