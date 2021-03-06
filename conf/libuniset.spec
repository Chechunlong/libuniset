%def_enable doc
%def_enable mysql
%def_enable sqlite
%def_enable python
%def_enable rrd

%define oname uniset

Name: libuniset
Version: 1.8
Release: alt10
Summary: UniSet - library for building distributed industrial control systems

License: GPL
Group: Development/C++
Url: http://wiki.etersoft.ru/UniSet

Packager: Pavel Vainerman <pv@altlinux.ru>

# Git: http://git.etersoft.ru/projects/asu/uniset.git
Source: %name-%version.tar

# manually removed: glibc-devel-static
# Automatically added by buildreq on Fri Nov 26 2010
BuildRequires: libcomedi-devel libcommoncpp2-devel libomniORB-devel libsigc++2.0-devel xsltproc

%if_enabled mysql
# Using old package name instead of libmysqlclient-devel it absent in branch 5.0 for yauza
BuildRequires: libMySQL-devel
%endif

%if_enabled sqlite
BuildRequires: libsqlite3-devel
%endif

%if_enabled rrd
BuildRequires: librrd-devel
%endif

%if_enabled python
BuildRequires: python-devel
BuildRequires(pre): rpm-build-python
Requires: python-module-libxml2

# swig
# add_findprov_lib_path %python_sitelibdir/%oname
%endif

%if_enabled doc
BuildRequires: doxygen
%endif

%set_verify_elf_method textrel=strict,rpath=strict,unresolved=strict

%description
UniSet is a library for distributed control systems.
There are set of base components to construct this kind of systems:
* base interfaces for your implementation of control algorithms.
* algorithms for the discrete and analog input/output based on COMEDI interface.
* IPC mechanism based on CORBA (omniORB).
* logging system based on MySQL database.
* Web interface to display logging and statistic information.
* utilities for system's configuration based on XML.

UniSet have been written in C++ and IDL languages but you can use another languages in your
add-on components. The main principle of the UniSet library's design is a maximum integration
with open source third-party libraries. UniSet provide the consistent interface for all
add-on components and third-party libraries.

More information in Russian:

%package devel
Group: Development/C
Summary: Libraries needed to develop for UniSet
Requires: %name = %version-%release

%description devel
Libraries needed to develop for UniSet.

%if_enabled mysql
%package mysql-dbserver
Group: Development/Databases
Summary: MySQL-dbserver implementatioin for UniSet
Requires: %name = %version-%release
Provides: %oname-mysql-dbserver
Obsoletes: %oname-mysql-dbserver

%description mysql-dbserver
MySQL dbserver for %name

%package mysql-devel
Group: Development/Databases
Summary: Libraries needed to develop for uniset MySQL
Requires: %name = %version-%release
Provides: %oname-mysql-devel
Obsoletes: %oname-mysql-devel

%description mysql-devel
Libraries needed to develop for uniset MySQL
%endif

%if_enabled sqlite
%package sqlite-dbserver
Group: Development/Databases
Summary: SQLite-dbserver implementatioin for UniSet
Requires: %name = %version-%release
Provides: %oname-sqlite-dbserver
Obsoletes: %oname-sqlite-dbserver

%description sqlite-dbserver
SQLite dbserver for %name

%package sqlite-devel
Group: Development/Databases
Summary: Libraries needed to develop for uniset SQLite
Requires: %name = %version-%release
Provides: %oname-sqlite-devel
Obsoletes: %oname-sqlite-devel

%description sqlite-devel
Libraries needed to develop for uniset SQLite
%endif

%if_enabled python
%package -n python-module-%oname
Group: Development/Python
Summary: python interface for libuniset
Requires: %name = %version-%release

# py_provides UGlobal UInterface UniXML uniset

%description -n python-module-%oname
Python interface for %name
%endif

%package utils
Summary: UniSet utilities
Group: Development/Tools
Requires: %name = %version-%release
Provides: %oname-utils
Obsoletes: %oname-utils

%description utils
UniSet utilities

%if_enabled doc

%package doc
Group: Development/C++
Summary: Documentations for developing with UniSet
Requires: %name = %version-%release
BuildArch: noarch

%description doc
Documentations for developing with UniSet
%endif


%package extensions
Group: Development/C++
Summary: libUniSet extensions
Requires: %name = %version-%release
Provides: %oname-extentions
Obsoletes: %oname-extentions
Provides: %name-extentions
Obsoletes: %name-extentions

%description extensions
Extensions for libuniset

%package extensions-devel
Group: Development/C++
Summary: Libraries needed to develop for uniset extensions
Requires: %name-extensions = %version-%release
Provides: %name-extentions-devel
Obsoletes: %name-extentions-devel

%description extensions-devel
Libraries needed to develop for uniset extensions

%prep
%setup

%build
%autoreconf
%configure %{subst_enable doc} %{subst_enable mysql} %{subst_enable sqlite} %{subst_enable python} %{subst_enable rrd}
%make

%install
%makeinstall_std
rm -f %buildroot%_libdir/*.la

%if_enabled python
mkdir -p %buildroot%python_sitelibdir/%oname
mv -f %buildroot%python_sitelibdir/*.* %buildroot%python_sitelibdir/%oname/

%ifarch x86_64
mv -f %buildroot%python_sitelibdir_noarch/* %buildroot%python_sitelibdir/%oname
%endif

%endif

%files utils
%_bindir/%oname-admin
%_bindir/%oname-infoserver
%_bindir/%oname-mb*
%_bindir/%oname-nullController
%_bindir/%oname-sviewer-text
%_bindir/%oname-smonit
%_bindir/%oname-simitator
%_bindir/%oname-start*
%_bindir/%oname-stop*
%_bindir/%oname-func*
%_bindir/%oname-codegen
%dir %_datadir/%oname/
%dir %_datadir/%oname/xslt/
%_datadir/%oname/xslt/*.xsl
%_datadir/%oname/xslt/skel*

%files
%_libdir/libUniSet.so.*

%files devel
%dir %_includedir/%oname/
%_includedir/%oname/*.h
%_includedir/%oname/*.hh
%_includedir/%oname/*.tcc
%_includedir/%oname/IOs/
%_includedir/%oname/modbus/
%if_enabled mysql
%_includedir/%oname/mysql/
%endif
%if_enabled sqlite
%_includedir/%oname/sqlite/
%endif

%_libdir/libUniSet.so
%_datadir/idl/%oname/
%_pkgconfigdir/libUniSet.pc

%if_enabled mysql
%files mysql-dbserver
%_bindir/%oname-mysql-*dbserver
%_libdir/*-mysql.so*

%files mysql-devel
%_pkgconfigdir/libUniSetMySQL.pc
%endif

%if_enabled sqlite

%files sqlite-dbserver
%_bindir/%oname-sqlite-*dbserver
%_libdir/*-sqlite.so*

%files sqlite-devel
%_pkgconfigdir/libUniSetSQLite.pc
%endif

%if_enabled python
%files -n python-module-%oname
%dir %python_sitelibdir/%oname
%python_sitelibdir/*
%python_sitelibdir/%oname/*

%endif

%if_enabled doc
%files doc
%_docdir/%oname
%endif

%files extensions
%_bindir/%oname-iocontrol
%_bindir/%oname-iotest
%_bindir/%oname-iocalibr
%_bindir/%oname-logicproc
%_bindir/%oname-plogicproc
%_bindir/%oname-mtr-conv
%_bindir/%oname-mtr-setup
%_bindir/%oname-mtr-read
%_bindir/%oname-vtconv
%_bindir/%oname-rtu188-state
%_bindir/%oname-rtuexchange
%_bindir/%oname-smemory
%_bindir/%oname-smemory-plus
%_bindir/%oname-smviewer
%_bindir/%oname-network
%_bindir/%oname-unet*
#%_bindir/%oname-smdbserver

%_libdir/*Extensions.so.*
%_libdir/libUniSetIO*.so.*
%_libdir/libUniSetLP*.so.*
%_libdir/libUniSetMB*.so.*
%_libdir/libUniSetRT*.so.*
%_libdir/libUniSetShared*.so.*
%_libdir/libUniSetNetwork*.so.*
%_libdir/libUniSetUNetUDP*.so.*
#%_libdir/libUniSetSMDBServer*.so.*

%if_enabled rrd
%_bindir/%oname-rrd*
%_libdir/libUniSetRRD*.so.*
%endif


%files extensions-devel
%_includedir/%oname/extensions/
%_libdir/*Extensions.so
%_libdir/libUniSetIO*.so
%_libdir/libUniSetLP*.so
%_libdir/libUniSetMB*.so
%_libdir/libUniSetRT*.so
%_libdir/libUniSetShared*.so
%_libdir/libUniSetNetwork.so
%_libdir/libUniSetUNetUDP.so
#%_libdir/libUniSetSMDBServer.so
%_pkgconfigdir/*Extensions.pc
%_pkgconfigdir/libUniSetIO*.pc
%_pkgconfigdir/libUniSetLog*.pc
%_pkgconfigdir/libUniSetMB*.pc
%_pkgconfigdir/libUniSetRT*.pc
%_pkgconfigdir/libUniSetShared*.pc
%_pkgconfigdir/libUniSetNetwork*.pc
%_pkgconfigdir/libUniSetUNet*.pc

%if_enabled rrd
%_pkgconfigdir/libUniSetRRD*.pc
%_libdir/libUniSetRRD*.so
%endif

#%_pkgconfigdir/libUniSetSMDBServer.pc
#%_pkgconfigdir/libUniSet*.pc
%exclude %_pkgconfigdir/libUniSet.pc





%changelog
* Thu May 21 2015 Pavel Vainerman <pv@altlinux.ru> 1.8-alt10
- (ModbusMaster): add init timer for respond sensor 

* Tue May 19 2015 Pavel Vainerman <pv@altlinux.ru> 1.8-alt9
- (DelayTimer): fixed critical bug in DelayTimer (thanks ilyap@etersoft.ru)

* Sun May 17 2015 Pavel Vainerman <pv@altlinux.ru> 1.8-alt8
- (ModbusMaster): refactoring 'set respond sensor mechanism'

* Fri May 15 2015 Pavel Vainerman <pv@altlinux.ru> 1.8-alt7
- (ModbusMaster): fixed bug in initialization mechanism..(again setbug #5583)

* Wed May 13 2015 Pavel Vainerman <pv@altlinux.ru> 1.8-alt6
- ModbusMaster: fixed bug: unstable timeout...( thank`s ilyap@etersoft.ru)

* Thu Mar 19 2015 Pavel Vainerman <pv@altlinux.ru> 1.8-alt5
- unetudp: fxied critical bug in "switching channels" (thank`s Alexey Surov)

* Fri Mar 13 2015 Pavel Vainerman <pv@altlinux.ru> 1.8-alt4
- unetudp: minor optimization (thank`s thank`s Alexey Vinogradov)

* Sun Mar 01 2015 Pavel Vainerman <pv@altlinux.ru> 1.8-alt3
- ModbusMaster: fixed bug in initialization (close setbug #5583)

* Wed Nov 26 2014 Pavel Vainerman <pv@altlinux.ru> 1.8-alt2.1
- update for alt/sisyphus..
- fixed changelog (save for history):
  - Thu Dec 06 2012 Eugeny A. Rostovtsev (REAL) <real at altlinux.org> 1.4-alt1.2
    - Fixed build with glibc 2.16
  - Fri Sep 14 2012 Eugeny A. Rostovtsev (REAL) <real at altlinux.org> 1.4-alt1.1
    - Rebuilt with updated libomniORB

* Thu Nov 13 2014 Pavel Vainerman <pv@altlinux.ru> 1.8-alt2
- fixed bug: Now ObjectID=0 does not cause an error

* Thu Oct 30 2014 Pavel Vainerman <pv@altlinux.ru> 1.8-alt1
- (IOBase): added the ability to override the basic settings using the prefix.
- (SQLiteInterface): fixed bug in SQLiteResult

* Mon Oct 20 2014 Pavel Vainerman <pv@altlinux.ru> 1.7-alt16
- fixed bug in UniXML::iteartor getPIntProp() for prop <= 0

* Sun Jun 22 2014 Pavel Vainerman <pv@altlinux.ru> 1.7-alt15
- optimization: add insert-buffer for DBServer_MySQL (see uniset-docs)

* Sat Jun 14 2014 Pavel Vainerman <pv@altlinux.ru> 1.7-alt14
- backport UTCPStream for MBTCPMultiMaster

* Tue Apr 29 2014 Pavel Vainerman <pv@altlinux.ru> 1.7-alt13
- (uniset-codegen): add "force" parameters for vartype="out"

* Thu Apr 24 2014 Pavel Vainerman <pv@altlinux.ru> 1.7-alt12
- remove deprecated Extensions:getHeartBeatTime(). Use conf->getHeartBeatTime()
- rename parameter:  "time_msec" --> "msec" for <HeartBeatTime msec=""/>

* Thu Apr 24 2014 Pavel Vainerman <pv@server> 1.7-alt11
- minor fixes IOControl::setValue

* Tue Apr 01 2014 Pavel Vainerman <pv@altlinux.ru> 1.7-alt10
- add new vtype:  F2r, I2r, U2r  (reverse data order)

* Wed Mar 19 2014 Pavel Vainerman <pv@altlinux.ru> 1.7-alt9
- add thresholds processing for ModbusMaster (TCP and RTU)
- minor fixes

* Fri Mar 07 2014 Pavel Vainerman <pv@altlinux.ru> 1.7-alt8
- fixed bug in ComPort class (getTimeout.  eterbug #9890)

* Wed Feb 12 2014 Pavel Vainerman <pv@altlinux.ru> 1.7-alt7
- revert last changes ("ModbusMaster: add thread for check connection")

* Wed Feb 12 2014 Pavel Vainerman <pv@altlinux.ru> 1.7-alt6
- ModbusMaster: add thread for check connection

* Mon Feb 10 2014 Pavel Vainerman <pv@altlinux.ru> 1.7-alt5
- ModbusMaster: minor fixes

* Thu Feb 06 2014 Pavel Vainerman <pv@altlinux.ru> 1.7-alt4
- ModbusExchange: fixed bug:  deadlock for pollMutex..

* Tue Dec 10 2013 Pavel Vainerman <pv@altlinux.ru> 1.7-alt3
- add RRDServer

* Fri Dec 06 2013 Pavel Vainerman <pv@altlinux.ru> 1.7-alt2
- (unetexchange): add 'prefix'

* Wed Dec 04 2013 Pavel Vainerman <pv@altlinux.ru> 1.7-alt1
- (modbus): add ModbusMultiChannel

* Fri Nov 29 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt17
- (uniset-codegen): move 'arg-prefix' from <variables> to <settings>

* Tue Nov 05 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt16
- (uniset-codegen): add 'loglevel' parameters for src.xml

* Mon Oct 28 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt15
- (python): fixed bug in UInterface

* Sat Oct 26 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt14
- (python): fixed bug in UInterface

* Thu Oct 24 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt13
- (python): add getArgParam, getArgInt and checkArg functions for UGlobal.py

* Thu Sep 19 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt12
- (Modbus): Added ability to set the sensor mode (modeSensor) for each device
- fixed bug in MTR types: T_Str16 and T_Str8 (tnx ilyap)
- fixed bug in MTR::send_param

* Thu Jun 13 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt11
- fixed after cppcheck checking

* Wed Jun 05 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt10
- add for ModbusMaster (RTU|TCP) --xxx--aftersend-pause 

* Tue May 14 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt9
- add for Modbus (RTU|TCP) exchange  --xxx-reopen-timeout msec. (eterbug #9296)

* Wed May 08 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt8
- fixed minor bug in uniset-codegen (getValue) 

* Wed Mar 20 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt7
- modbus: add new function 0x2B/0x0E(43/14)"Read device identification"

* Tue Mar 05 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt6
- python: add __init__.py

* Tue Mar 05 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt5
- force add python provides

* Tue Mar 05 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt4
- restote UInterface for python

* Tue Mar 05 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt3
- add doc for python bindings
- rebuild wrapper files with new swig

* Tue Mar 05 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt2
- fixed build for x86_64

* Tue Mar 05 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt1
- python: final build

* Tue Mar 05 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt0.5
- python: test build

* Tue Mar 05 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt0.4
- python: test build

* Tue Mar 05 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt0.3
- python: test build

* Tue Mar 05 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt0.2
- python: test build

* Mon Mar 04 2013 Pavel Vainerman <pv@altlinux.ru> 1.6-alt0.1
- add python interface

* Mon Jan 14 2013 Pavel Vainerman <pv@altlinux.ru> 1.5-alt10
- add error code for MTR (eterbug #8659)
- (uniset-codegen): add generate class Skeleton (--make-skel)

* Sat Jan 05 2013 Pavel Vainerman <pv@altlinux.ru> 1.5-alt9
- add SQLite support

* Mon Dec 03 2012 Pavel Vainerman <pv@altlinux.ru> 1.5-alt8
- add uniset-smemory-plus

* Fri Nov 30 2012 Pavel Vainerman <pv@altlinux.ru> 1.5-alt7
- returned file 'SandClock.h' back and declared it obsolete

* Thu Nov 29 2012 Pavel Vainerman <pv@altlinux.ru> 1.5-alt6
- add DelayTimer class
- rename SandClock --> HourGlass

* Fri Nov 23 2012 Pavel Vainerman <pv@altlinux.ru> 1.5-alt5
- (Calibration): add getLeftRaw(),getRightRaw(),getLeftVal(),getRightVal()
- (Calibration): fixed bugs

* Fri Nov 23 2012 Pavel Vainerman <pv@altlinux.ru> 1.5-alt4
- (uniset-codegen): fixed minor bug

* Thu Nov 22 2012 Pavel Vainerman <pv@altlinux.ru> 1.5-alt3
- (Calibration): add getMinRaw(),getMaxRaw(),getMinVal(),getMaxVal()

* Wed Nov 21 2012 Pavel Vainerman <pv@altlinux.ru> 1.5-alt2
- new ConfirmMessage format (eterbug #8842)

* Tue Nov 06 2012 Pavel Vainerman <pv@altlinux.ru> 1.5-alt1
- add depends for IOBase

* Tue Sep 04 2012 Pavel Vainerman <pv@altlinux.ru> 1.4-alt11
- minor fixes (IOControl::getState, isExist)

* Wed Aug 29 2012 Pavel Vainerman <pv@altlinux.ru> 1.4-alt10
- (UDPNet): increase the resolution of the sensors over the network (600 analog, 600 digital)

* Mon Aug 20 2012 Pavel Vainerman <pv@altlinux.ru> 1.4-alt9
- fixed bug in previous commit (bug in UniXML::iterator::find)

* Tue Aug 07 2012 Pavel Vaynerman <pv@server> 1.4-alt8
- fixed bug in UniXML::iterator::find

* Wed Jul 25 2012 Pavel Vainerman <pv@altlinux.ru> 1.4-alt7
- (codegen): added support type 'long'
- add setThreadPriority(..) for UniSetObject

* Tue Jul 10 2012 Pavel Vaynerman <pv@server> 1.4-alt6
- (unetudp): fixed bug in the logic of switching channels

* Thu Jun 14 2012 Pavel Vainerman <pv@altlinux.ru> 1.4-alt5
- (codegen): fixed bug in validation 'iotype'

* Sun Jun 10 2012 Pavel Vainerman <pv@altlinux.ru> 1.4-alt4
- (codegen): added validation 'iotype'

* Sun Jun 10 2012 Pavel Vainerman <pv@altlinux.ru> 1.4-alt3
- (DBServer_MySQL): buffer is added to query

* Fri Jun 08 2012 Pavel Vainerman <pv@altlinux.ru> 1.4-alt2
- added support type 'double' for uniset-codegen (<variables>)


* Thu May 31 2012 Pavel Vainerman <pv@altlinux.ru> 1.4-alt1
- rename unet2 -->unetudp
- release version 1.4

* Thu May 31 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt19
- DBServer: set log info level - LEVEL9
- minor fixies for linker errors (new gcc)

* Tue Apr 10 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt18
- fixed bug in ComPort485F (reinit function)

* Fri Mar 16 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt17
- rebuild

* Fri Mar 16 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt16
- (unet2): fixed bug in respond sensors (again)

* Thu Mar 15 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt15
- (unet2): fixed bug in respond sensors

* Thu Mar 15 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt14
- (unet2): add 'unet_respond_invert' parameter

* Sun Mar 11 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt13
- minor fixes in uniset-codegen (add "preAskSensors")

* Fri Mar 02 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt12
- fixed bug in DigitalFilter 
- fixed bug in RTU188 exchange

* Tue Feb 28 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt11
- (iocontrol): fixed bug in configuring UNIO96

* Fri Feb 24 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt10
- (modbus): realized exchange with RTU188

* Wed Feb 22 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt9
- (modbus): fixed bug in modbus exchange for RTU188 (initialization)

* Tue Feb 21 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt8
- (modbus): fixed bug in modbus exchange for RTU188

* Sat Feb 18 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt7
- changed implementation SharedMemory::History (optimization)

* Fri Feb 17 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt6
- (io): corrected a mistake in configuring analog I/O

* Fri Feb 10 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt5
- (IOControl): Added support for setting boards such as 'Grayhill'
- (Modbus): Fixed minor bug in configuration with RTU188

* Fri Feb 03 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt4
- add commmon (respond and lostpackets) sensors for UNet2

* Tue Jan 31 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt3
- minor fixes in simitator

* Tue Jan 24 2012 Pavel Vainerman <pv@altlinux.ru> 1.3-alt2.1
- rebuild

* Mon Dec 26 2011 Pavel Vainerman <pv@altlinux.ru> 1.3-alt2
- fixed buf in Configuration

* Mon Dec 26 2011 Pavel Vainerman <pv@altlinux.ru> 1.3-alt1
- Added support for multiple profiles(Configuration) simultaneously

* Fri Dec 23 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt62
- fixed bug in UniversalInterface

* Fri Dec 23 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt61
- minor fixes in LogicProcessor

* Wed Dec 21 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt60
- fixed bug in LogicProcessor

* Mon Dec 19 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt59
- add precision for output variables

* Wed Nov 30 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt58
- fixed bug in ModbusSlave::readInputStatus(0x02)

* Sun Nov 27 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt57
- add uniset-mtr-read utility

* Sat Nov 26 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt56
- (modbus): fixed bug (again) in ModbusSlave::readInputStatus(0x02)

* Sat Nov 26 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt55
- (modbus): fixed bug in ModbusSlave::readInputStatus(0x02)

* Fri Nov 25 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt54
- (modbus): added 'const-reply' for modbustcptester

* Thu Nov 24 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt53
- (modbus): added information log

* Wed Nov 16 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt52
- (modbus): An opportunity to change the prefix is added to the properties

* Wed Nov 02 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt51
- (unet2): added reserv channel exchange

* Mon Oct 31 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt50
- ModbusMaster extensions code refactoring

* Tue Oct 25 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt49
- added support 'const' and [private|protecte|public] 
for <variables> in uniset-codegen

* Sat Oct 22 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt48
- added support for list of variables (<variables>) in uniset-codegen

* Tue Oct 11 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt47
- new build

* Sat Oct 08 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt46
- add reopen() for ComPort

* Tue Oct 04 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt45
- dependence on mandatory disabled launching a local omninames service

* Mon Oct 03 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt44
- add nodes filter for UNet2
- minor optimization

* Thu Jul 14 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt42
- fixed bug in uniset-codegen

* Sun Jun 26 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt41
- fixed bug in ComediControl::cinfugureSubdev
- and other minor fixes

* Thu Jun 09 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt38
- fied bug in ComPort::cleanupChannel()

* Sun Jun 05 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt37
- add cleanup before send for ComPort

* Tue May 24 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt36
- add 'node' param processing for uniset-codegen

* Fri May 20 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt35
- minor fixed in UniXML::getProp()

* Thu May 19 2011 Pavel Vainerman <pv@etersoft.ru> 1.0-alt34
- fixed bug in IOControl

* Thu May 19 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt33
- fixed bug in DBInterface::ping (again). Many thanks uzum

* Thu May 19 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt32
- fixed bug in DBInterface::nextRow function
- fixed bug in DBInterface::ping

* Fri May 13 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt31
- move DBServer-MySQL to extensions directory
- add pc-file for libUniSet-mysql
- create new devel package - "libuniset-mysql-devel"
- minor fixes

* Wed May 11 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt30
- add new function to UniversalInterface

* Sat May 07 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt29
- (unet2): new protocol implementation

* Thu May 05 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt28
- add setup function for ModbusTCPMaster and ModbusTCPServer

* Wed May 04 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt27
- fixed bug in ModbusTCPMaster and ModbusTCPServer

* Wed May 04 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt26
- minor fixes

* Wed May 04 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt25
- (unet2): minor fixes

* Sun May 01 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt24
- build for new uniset-unet2 version

* Sun May 01 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt23
- (uniset-unet2): fixed bug (SEGFAULT with a large number of items)

* Wed Apr 20 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt22
- (uniset-unet2-tester): fixed minor bugs 

* Wed Apr 20 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt21
- (uniset-unet2-tester): add new parameter 
   -l | --check-lost   - Check the lost packets.

* Wed Apr 20 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt20
- (uniset-unet2-tester): rename command line parameters

* Tue Apr 19 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt19
- a little cleaning

* Tue Apr 19 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt18
- add unet2-tester

* Tue Mar 29 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt17
- set encoding="utf-8" for codegen templates

* Tue Mar 29 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt16
- fixed minor bug in codegen

* Sat Mar 26 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt15
- fixed return value in some utilities

* Thu Mar 24 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt14
- fixed bug in MBSlave

* Thu Mar 24 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt13
- rename some utilities (mtr-xxx --> uniset-mtr-xxx, vtconv --> uniset-vtconv)

* Wed Mar 23 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt12
- fixed bug in TableBlockStorage interface

* Thu Mar 17 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt11
- fixed bug in MBTCPMaster (query optimization)

* Sun Mar 13 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt10
- fixed bug in uniset-codegen (again)

* Sun Mar 13 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt9
- fixed bug in uniset-codegen

* Fri Mar 11 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt8
- fixed bug in conf->getArgPInt function (new libUniSet revision)

* Wed Mar 02 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt7
- add UNet2 to extensions

* Tue Mar 01 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt6
- MBTCPMaster new version (fixed any bugs)

* Thu Feb 24 2011 Pavel Vainerman <pv@server> 1.0-alt5
- new build (optimization local timers)

* Thu Feb 24 2011 Pavel Vainerman <pv@server> 1.0-alt4
- new build

* Mon Feb 14 2011 Pavel Vainerman <pv@server> 1.0-alt3
- fixed bug in ModbusTCPMaster

* Mon Feb 14 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt2
- minor fixes
- new version ModbusTCPMaster

* Wed Feb 09 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt1
- fixed bug in VERSION (bad pc-files)

* Wed Jan 26 2011 Pavel Vainerman <pv@altlinux.ru> 1.0-alt0.1
- pre-release build

* Sat Jan 22 2011 Pavel Vainerman <pv@altlinux.ru> 0.99-alt30
- add "prefix" for IOControl

* Fri Jan 14 2011 Ilya Shpigor <elly@altlinux.org> 0.99-alt27
- initial build for ALT Linux Sisyphus

* Fri Jan 14 2011 Pavel Vainerman <pv@etersoft.ru> 0.99-eter26
- fixed bug in ModbusTCP Master. Set default signed type for data

* Sat Dec 04 2010 Pavel Vainerman <pv@altlinux.ru> 0.99-eter25
- fixed bug in uniset-mbtcptest (writexx)
- minor fixes ( add 'm'-parametes for set value < 0 )

* Tue Nov 30 2010 Pavel Vainerman <pv@altlinux.ru> 0.99-eter24
- ModbusRTU::mbException: public Exception

* Mon Nov 29 2010 Pavel Vainerman <pv@altlinux.ru> 0.99-eter23
- fixed bug in Modbus<-->SM (signed and unsigned value)

* Mon Nov 29 2010 Pavel Vainerman <pv@altlinux.ru> 0.99-eter22
- smonitor new format (id@node)

* Tue Nov 23 2010 Pavel Vainerman <pv@altlinux.ru> 0.99-eter20
- new build

* Tue Nov 16 2010 Pavel Vainerman <pv@etersoft.ru> 0.99-eter17
- fixed bug in MBSlave

* Mon Nov 15 2010 Pavel Vainerman <pv@etersoft.ru> 0.99-eter16
- Modbus: add test for query count

* Mon Nov 15 2010 Pavel Vainerman <pv@etersoft.ru> 0.99-eter15
- new build

* Fri Nov 12 2010 Pavel Vainerman <pv@etersoft.ru> 0.99-eter14
- new build

* Thu Nov 11 2010 Pavel Vainerman <pv@altlinux.ru> 0.99-eter13
- uniset-admin refactor. ( [get|set]Value, call remote sensors)

* Mon Nov 08 2010 Pavel Vainerman <pv@altlinux.ru> 0.99-eter12
- fixed minor bug in uniset-codegen

* Mon Oct 18 2010 Pavel Vainerman <pv@altlinux.ru> 0.99-eter9
- new build

* Wed Oct 13 2010 Ilya Shpigor <elly@altlinux.org> 0.99-eter8
- fix bug in ModbusTCPServer
- add gateway imitation to uniset-mbtcpserver-echo

* Sat Oct 09 2010 Pavel Vainerman <pv@altlinux.ru> 0.99-eter7
- fixed bug in MBTCPMaster

* Sun Oct 03 2010 Pavel Vainerman <pv@altlinux.ru> 0.99-eter6
- test codegen build

* Tue Sep 28 2010 Pavel Vainerman <pv@etersoft.ru> 0.99-eter5
- MBSlave (RTU|TCP) optimization

* Mon Sep 20 2010 Ilya Shpigor <elly@altlinux.org> 0.99-eter4
- new build 0.99-eter4

* Fri Sep 17 2010 Pavel Vainerman <pv@etersoft.ru> 0.99-eter3
- add new value types (I2,U2) in to MBTCPMaster

* Fri Sep 17 2010 Pavel Vainerman <pv@etersoft.ru> 0.99-eter2
- build for new MBSlave

* Tue Sep 14 2010 Pavel Vainerman <pv@altlinux.ru> 0.99-eter1
- test UDP build

* Tue Sep 07 2010 Evgeny Sinelnikov <sin@altlinux.ru> 0.98-eter9
- Build for Sisyphus Etersoft addon:
  commit fc7fb2eefaf900088d7e6583df440c914aeb9560

* Wed Aug 25 2010 Pavel Vainerman <pv@server> 0.98-eter8
- fixed bug for install IDL-files

* Wed Aug 11 2010 Pavel Vainerman <pv@altlinux.ru> 0.98-eter7
- add new types for MTR
- minor fixes in SharedMemory::HistoryInfo (add timestamp)

* Fri Jul 30 2010 Ilya Shpigor <elly@altlinux.org> 0.98-eter6
- add MTR support
- add db_ignore parameter for DBServer

* Tue Jul 13 2010 Vitaly Lipatov <lav@altlinux.ru> 0.98-eter4
- iterator for CycleStorage
- some fixes for CycleStorage iterator

* Wed Jun 16 2010 Ivan Donchevskiy <yv@etersoft.ru> 0.98-eter3
- new build

* Tue Jun 01 2010 Vitaly Lipatov <lav@altlinux.ru> 0.98-eter2
- fixed bug in uniset-codegen
- minor fixes in SM (add virtual function)
- fixed bug in SandClock interface
- fixed bug in ModbuRTU::autedetect slave adress function
- minor fixes in MTR setup API
- add MTR setup (API and utility)

* Wed Mar 03 2010 Ilya Shpigor <elly@altlinux.org> 0.98-eter1
- new build for Sisyphus with utf8 support

* Sat Feb 13 2010 Pavel Vainerman <pv@altlinux.ru> 0.97-eter54
- fixed bug in codegen

* Thu Feb 04 2010 Pavel Vainerman <pv@altlinux.ru> 0.97-eter53
- new build

* Tue Feb 02 2010 Larik Ishkulov <gentro@etersoft.ru> 0.97-eter52
- new build

* Fri Jan 29 2010 Pavel Vainerman <pv@altlinux.ru> 0.97-eter50
- add simitator

* Tue Jan 26 2010 Pavel Vainerman <pv@altlinux.ru> 0.97-eter49
- fixed bug in ModbusTCPMaster

* Mon Dec 28 2009 Alexander Morozov <amorozov@etersoft.ru> 0.97-eter48
- added new filters
- fixed some bugs

* Thu Dec 03 2009 Pavel Vainerman <pv@altlinux.ru> 0.97-eter46
- fixed bug in MBTCPMaster

* Wed Dec 02 2009 Pavel Vainerman <pv@altlinux.ru> 0.97-eter45
- new build (for builder50)
* Mon Nov 23 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter44
- exclude SMDBServer

* Thu Nov 19 2009 Larik Ishkulov <gentro@etersoft.ru> 0.97-eter37
- new build

* Wed Nov 18 2009 Pavel Vainerman <pv@altlinux.ru> 0.97-eter36
- new build

* Mon Nov 16 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter35
- fixed bug in uniset-mysql-dbserver

* Thu Nov 12 2009 Pavel Vainerman <pv@altlinux.ru> 0.97-eter34
- new build

* Tue Oct 27 2009 Pavel Vainerman <pv@altlinux.ru> 0.97-eter33
- fixed bug (int previous build) in SM
- fixed bug in uniset-stop.sh

* Tue Oct 27 2009 Pavel Vainerman <pv@altlinux.ru> 0.97-eter32
- new build

* Sat Oct 24 2009 Pavel Vainerman <pv@altlinux.ru> 0.97-eter31
- new build

* Wed Oct 21 2009 Pavel Vainerman <pv@altlinux.ru> 0.97-eter30
- new build

* Sun Oct 18 2009 Pavel Vainerman <pv@altlinux.ru> 0.97-eter29
- new build

* Fri Oct 09 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter28
- minor optimization

* Mon Oct 05 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter27
- new build

* Sun Oct 04 2009 Vitaly Lipatov <lav@altlinux.ru> 0.97-eter26
- new build

* Thu Oct 01 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter25
- return old mutex

* Thu Oct 01 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter24
- new mutex

* Wed Sep 30 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter23
- control (on/off) new ComPort

* Wed Sep 30 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter22
- new build

* Tue Sep 29 2009 Vitaly Lipatov <lav@altlinux.ru> 0.97-eter21
- new build

* Tue Sep 29 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter20
- new vtypes for Modbus

* Mon Sep 28 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter19
- new comport

* Mon Sep 28 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter18
- restore mutex

* Mon Sep 28 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter17
- new mbtcpmaster

* Mon Sep 28 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter16
- new mutex

* Sat Sep 26 2009 Pavel Vainerman <pv@altlinux.ru> 0.97-eter15
- new build

* Sat Sep 26 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter14
- add default heartbeat time to Configuration

* Sat Sep 26 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter13
- add heartbeat logic to uniset-codegen

* Sat Sep 26 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter12
- minor fixes in IONotifyController

* Fri Sep 25 2009 Pavel Vainerman <pv@altlinux.ru> 0.97-eter11
- return old mutex

* Wed Sep 23 2009 Pavel Vainerman <pv@altlinux.ru> 0.97-eter10
- new build

* Wed Sep 23 2009 Pavel Vainerman <pv@altlinux.ru> 0.97-eter9
- new mutex

* Tue Sep 22 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter8
- new build

* Tue Sep 22 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter7
- new build

* Tue Sep 22 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter6
- new build

* Tue Sep 22 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter5
- minor fixes in SM

* Tue Sep 22 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter4
- minor fixes in IOBase

* Tue Sep 22 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter3
- fixed bugs in IOControl

* Sat Sep 19 2009 Vitaly Lipatov <lav@altlinux.ru> 0.97-eter2
- fix build

* Wed Sep 16 2009 Vitaly Lipatov <lav@altlinux.ru> 0.97-eter1
- add getProp and getInt into generated _SK class for use default cnode
- UniXML: make xml2local, local2xml protected
- UniXML: add getContent for iterator (via xmlNodeGetContent)
- use logFile with string, without c_str
- UniXML: add getPropUtf8, findNodeUtf8, extFindNodeUtf8
- UniSetTypes: add getArgInt, getArgPInt
- forbid direct use atoi function in uniset and uniset related projects
- add string support for getIdByName
- use appropriate getArg(P)Int, get(P)IntProp instead direct atoi using
- Configuration: add getPIntField, getIntProp, getPIntProp, getArgPint
- Added byte size check in CycleStorage and TableBlockStorage
- add uniset-network (new component - UniNetwork)
- add new interface: getSensors()
- add new realization MBTCPMaster
- introduce getPintProp for get positive only values (returns def if the value zero or negative). Note: def may be negative if needed.

* Tue Sep 15 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter63
- new build

* Mon Sep 07 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter54
- rebuild for new ModbusType parameters

* Mon Sep 07 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter53
- rebuild for new MBTCPMaster

* Sun Sep 06 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter52
- minor fixes in MBTCPMAster

* Fri Aug 21 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter51
- minor fixes in RTUExchange

* Wed Aug 19 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter50
- fixed bug in IOControl (blink mechanic)

* Wed Aug 19 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter49
- add BLINK2, BLINK3 to IOControl

* Tue Aug 18 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter48
- fixed bug in PassiveTimer

* Wed Aug 05 2009 Vitaly Lipatov <lav@altlinux.ru> 0.96-eter37
- fixed smp build

* Mon Aug 03 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter36
- new build

* Mon Aug 03 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter35
- new  RS  properties

* Sat Aug 01 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter30
- new build

* Tue Jul 14 2009 Vitaly Lipatov <lav@altlinux.ru> 0.96-eter29
- build from gear repo, rewrote spec
- rename extentions to extensions (see eterbug #4008)
- update buildreq

* Mon Apr 06 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter4
- new ComediInterface

* Mon Apr 06 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter3
- merge with mutabor/master

* Mon Apr 06 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter2
- fixed bugs in uniset-codegen

* Sat Jan 17 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-alt7
- new version

* Tue Dec 16 2008 Pavel Vainerman <pv@altlinux.ru> 0.96-alt1
- new version (+extensions)

* Tue Nov 27 2007 Pavel Vainerman <pv@altlinux.ru> 0.93-alt13
- new version

* Tue Nov 27 2007 Pavel Vainerman <pv@altlinux.ru> 0.93-alt11
- new version

* Sun Nov 04 2007 Pavel Vainerman <pv@altlinux.ru> 0.92-alt5
- new version

* Tue Oct 23 2007 Pavel Vainerman <pv@altlinux.ru> 0.92-alt4
- build for C30

* Wed Oct 17 2007 Pavel Vainerman <pv@altlinux.ru> 0.91-alt8
- new version

* Wed Oct 17 2007 Pavel Vainerman <pv@altlinux.ru> 0.91-alt7
- new version

* Fri Sep 14 2007 Pavel Vainerman <pv@altlinux.ru> 0.91-alt6
- new version

* Fri Aug 03 2007 Pavel Vainerman <pv@altlinux.ru> 0.91-alt5
- new build for C30

* Thu Aug 02 2007 Pavel Vainerman <pv@altlinux.ru> 0.91-alt4
- new version for C30

* Thu Aug 02 2007 Pavel Vainerman <pv@altlinux.ru> 0.91-alt3
- new version for C30

* Mon Jul 30 2007 Pavel Vainerman <pv@altlinux.ru> 0.91-alt1
- build for C30

* Fri Jul 13 2007 Pavel Vainerman <pv@altlinux.ru> 0.9-alt14.C30
- buid for C30

* Sun Jul 08 2007 Pavel Vainerman <pv@altlinux.ru> 0.9-alt13
- new build

* Sat Jul 07 2007 Pavel Vainerman <pv@altlinux.ru> 0.9-alt0.C30.1
- build for C30

* Tue Jun 19 2007 Pavel Vainerman <pv@altlinux.ru> 0.8-alt10.M30
- new version

* Mon Jun 18 2007 Pavel Vainerman <pv@altlinux.ru> 0.8-alt9.M30
- new version (for M30)

* Mon Jun 18 2007 Pavel Vainerman <pv@altlinux.ru> 0.8-alt8.M30
- new version (for M30)

* Mon Jun 18 2007 Pavel Vainerman <pv@altlinux.ru> 0.8-alt7
- new version (for M30)

* Mon Jun 18 2007 Pavel Vainerman <pv@altlinux.ru> 0.8-alt1
- new build

* Mon Jun 18 2007 Pavel Vainerman <pv@altlinux.ru> 0.7-alt5
- new build

* Sat Feb 17 2007 Pavel Vainerman <pv@altlinux.ru> 0.7-alt2
- new build

* Mon Feb 05 2007 Pavel Vainerman <pv@altlinux.ru> 0.7-alt1
- new build

* Mon Dec 25 2006 Pavel Vainerman <pv@altlinux.ru> 0.7-alt0.1
- new build

* Tue Jun 13 2006 Pavel Vainerman <pv@altlinux.ru> 0.6-alt2
- fix bug for gcc 4.1.0

* Tue Jun 13 2006 Pavel Vainerman <pv@altlinux.ru> 0.6-alt1
- new version

* Fri Sep 09 2005 Pavel Vainerman <pv@altlinux.ru> 0.5.RC5-alt2
- fixed bug ind AskDUmper::SInfo::operator=

* Mon Aug 29 2005 Pavel Vainerman <pv@altlinux.ru> 0.5.RC5-alt1
- detach mysql-dbserver
- add --disable-mysql for configure script

* Fri Jun 24 2005 Pavel Vainerman <pv@altlinux.ru> 0.5.RC1-alt1
- build new version
- fixed bugs

* Sat Mar 26 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.9-alt4
- fixed bug in IOController: not registration child Objects
- add docs section for IOConfigure

* Sun Feb 27 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.9-alt3
- change createNext in UniXML
- add copyNode (new function to UniXML)

* Tue Feb 22 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.9-alt2
- bug fix for AskDumperXML1

* Mon Feb 21 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.9-alt1
- add ClassGen utility

* Fri Feb 04 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.8-alt1
- correct uniset-admin --logrotate function

* Sat Jan 29 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.7-alt1
- compiled for gcc3.4

* Thu Jan 06 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.6-alt1
- move idl files to /%_datadir/idl/%name
- remove old files

* Mon Jan 03 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.4-alt1
- new version
- rename TimeService --> TimerService
- new start/stop scripts for local run

* Fri Dec 24 2004 Pavel Vainerman <pv@altlinux.ru> 0.4.1-alt1
- new version
- add analog and digital filters
- add sensibility for analog sensor

* Wed Dec 22 2004 Pavel Vainerman <pv@altlinux.ru> 0.0.4-alt1
- build new version

* Tue Nov 09 2004 Pavel Vainerman <pv@altlinux.ru> 0.0.2-alt1
- new version
- disable uniset.xxx-xxx.xxx.rpm

* Mon Nov 08 2004 Pavel Vainerman <pv@altlinux.ru> 0.0.1-alt1
- first build
