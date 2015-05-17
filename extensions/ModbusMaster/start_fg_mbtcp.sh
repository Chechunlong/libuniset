#!/bin/sh

./uniset-start.sh -f ./uniset-mbtcpmaster \
--confile test.xml \
--mbtcp-name MBMaster1 \
--smemory-id SharedMemory \
--mbtcp-set-prop-prefix \
--mbtcp-filter-field rs \
--mbtcp-filter-value 5 \
--mbtcp-gateway-iaddr localhost \
--mbtcp-gateway-port 2048 \
--mbtcp-recv-timeout 5000 \
--mbtcp-force-disconnect 1 \
--mbtcp-polltime 3000 \
--mbtcp-force-out 1 \
--dlog-add-levels level4 \
$*

#--dlog-add-levels info,crit,warn,level4,level3,level9 \

#--mbtcp-exchange-mode-id MB1_Mode_AS \
#--mbtcp-filter-field mbtcp --mbtcp-filter-value 1
#--mbtcp-set-prop-prefix rs_ \

