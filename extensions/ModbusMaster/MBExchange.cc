// -----------------------------------------------------------------------------
#include <cmath>
#include <limits>
#include <sstream>
#include <Exceptions.h>
#include <UniSetTypes.h>
#include <extensions/Extensions.h>
#include "MBExchange.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// -----------------------------------------------------------------------------
MBExchange::MBExchange( UniSetTypes::ObjectId objId, UniSetTypes::ObjectId shmId, 
							SharedMemory* ic, const std::string prefix ):
UniSetObject_LT(objId),
allInitOK(false),
shm(0),
initPause(0),
force(false),
force_out(false),
mbregFromID(false),
sidExchangeMode(DefaultObjectId),
exchangeMode(emNone),
activated(false),
noQueryOptimization(false),
no_extimer(false),
prefix(prefix),
poll_count(0),
prop_prefix(""),
mb(0),
pollActivated(false)
{
//	cout << "$ $" << endl;

	if( objId == DefaultObjectId )
		throw UniSetTypes::SystemError("(MBExchange): objId=-1?!! Use --" + prefix + "-name" );

//	xmlNode* cnode = conf->getNode(myname);
	string conf_name = conf->getArgParam("--" + prefix + "-confnode",myname);

	cnode = conf->getNode(conf_name);
	if( cnode == NULL )
		throw UniSetTypes::SystemError("(MBExchange): Not found node <" + conf_name + " ...> for " + myname );

	shm = new SMInterface(shmId,&ui,objId,ic);

	UniXML_iterator it(cnode);

	// определяем фильтр
	s_field = conf->getArgParam("--" + prefix + "-filter-field");
	s_fvalue = conf->getArgParam("--" + prefix + "-filter-value");
	dlog[Debug::INFO] << myname << "(init): read fileter-field='" << s_field
						<< "' filter-value='" << s_fvalue << "'" << endl;

	stat_time = conf->getArgPInt("--" + prefix + "-statistic-sec",it.getProp("statistic_sec"),0);
	if( stat_time > 0 )
		ptStatistic.setTiming(stat_time*1000);

	recv_timeout = conf->getArgPInt("--" + prefix + "-recv-timeout",it.getProp("recv_timeout"), 500);

	int tout = conf->getArgPInt("--" + prefix + "-timeout",it.getProp("timeout"), 5000);
	// для совместимости со старым RTUExchange
	// надо обратывать и all-timeout
	tout = conf->getArgPInt("--" + prefix + "-all-timeout",it.getProp("all_timeout"), tout);
	ptTimeout.setTiming(tout);

	tout = conf->getArgPInt("--" + prefix + "-reopen-timeout",it.getProp("reopen_timeout"), 10000);
	ptReopen.setTiming(tout);

	aftersend_pause = conf->getArgPInt("--" + prefix + "-aftersend-pause",it.getProp("aftersend_pause"),0);

    defaultMBinitOK = conf->getArgPInt("--" + prefix + "-default-mbinit-ok", it.getProp("default_mbinitOK"), 0);
	noQueryOptimization = conf->getArgInt("--" + prefix + "-no-query-optimization",it.getProp("no_query_optimization"));

	mbregFromID = conf->getArgInt("--" + prefix + "-reg-from-id",it.getProp("reg_from_id"));
	dlog[Debug::INFO] << myname << "(init): mbregFromID=" << mbregFromID << endl;

	polltime = conf->getArgPInt("--" + prefix + "-polltime",it.getProp("polltime"), 100);

	initPause = conf->getArgPInt("--" + prefix + "-initPause",it.getProp("initPause"), 3000);

	sleepPause_usec = conf->getArgPInt("--" + prefix + "-sleepPause-usec",it.getProp("sleepPause"), 100);

	force = conf->getArgInt("--" + prefix + "-force",it.getProp("force"));
	force_out = conf->getArgInt("--" + prefix + "-force-out",it.getProp("force_out"));


	// ********** HEARTBEAT *************
	string heart = conf->getArgParam("--" + prefix + "-heartbeat-id",it.getProp("heartbeat_id"));
	if( !heart.empty() )
	{
		sidHeartBeat = conf->getSensorID(heart);
		if( sidHeartBeat == DefaultObjectId )
		{
			ostringstream err;
			err << myname << ": ID not found ('HeartBeat') for " << heart;
			dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}

		int heartbeatTime = conf->getArgPInt("--" + prefix + "-heartbeat-time",it.getProp("heartbeatTime"),conf->getHeartBeatTime());
		if( heartbeatTime )
			ptHeartBeat.setTiming(heartbeatTime);
		else
			ptHeartBeat.setTiming(UniSetTimer::WaitUpTime);

		maxHeartBeat = conf->getArgPInt("--" + prefix + "-heartbeat-max",it.getProp("heartbeat_max"), 10);
		test_id = sidHeartBeat;
	}
	else
	{
		test_id = conf->getSensorID("TestMode_S");
		if( test_id == DefaultObjectId )
		{
			ostringstream err;
			err << myname << "(init): test_id unknown. 'TestMode_S' not found...";
			dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}
	}

	dlog[Debug::INFO] << myname << "(init): test_id=" << test_id << endl;

	string emode = conf->getArgParam("--" + prefix + "-exchange-mode-id",it.getProp("exchangeModeID"));
	if( !emode.empty() )
	{
		sidExchangeMode = conf->getSensorID(emode);
		if( sidExchangeMode == DefaultObjectId )
		{
			ostringstream err;
			err << myname << ": ID not found ('ExchangeMode') for " << emode;
			dlog[Debug::CRIT] << myname << "(init): " << err.str() << endl;
			throw SystemError(err.str());
		}
	}

	activateTimeout	= conf->getArgPInt("--" + prefix + "-activate-timeout", 20000);
}
// -----------------------------------------------------------------------------
void MBExchange::help_print( int argc, const char* const* argv )
{
	cout << "--prefix-name name              - ObjectId (имя) процесса. По умолчанию: MBExchange1" << endl;
	cout << "--prefix-confnode name          - Настроечная секция в конф. файле <name>. " << endl;
	cout << "--prefix-polltime msec          - Пауза между опросаом карт. По умолчанию 200 мсек." << endl;
	cout << "--prefix-recv-timeout msec      - Таймаут на приём одного сообщения" << endl;
	cout << "--prefix-timeout msec           - Таймаут для определения отсутствия соединения" << endl;
	cout << "--prefix-reopen-timeout msec    - Таймаут для 'переоткрытия соединения' при отсутсвия соединения msec милисекунд. По умолчанию 10 сек." << endl;
	cout << "--prefix-heartbeat-id  name     - Данный процесс связан с указанным аналоговым heartbeat-дачиком." << endl;
	cout << "--prefix-heartbeat-max val      - Максимальное значение heartbeat-счётчика для данного процесса. По умолчанию 10." << endl;
	cout << "--prefix-ready-timeout msec     - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;
	cout << "--prefix-force 0,1              - Сохранять значения в SM, независимо от, того менялось ли значение" << endl;
	cout << "--prefix-force-out 0,1          - Считывать значения 'выходов' кажый раз SM (а не по изменению)" << endl;
	cout << "--prefix-initPause msec         - Задержка перед инициализацией (время на активизация процесса)" << endl;
	cout << "--prefix-no-query-optimization 0,1 - Не оптимизировать запросы (не объединять соседние регистры в один запрос)" << endl;
	cout << "--prefix-reg-from-id 0,1        - Использовать в качестве регистра sensor ID" << endl;
	cout << "--prefix-filter-field name      - Считывать список опрашиваемых датчиков, только у которых есть поле field" << endl;
	cout << "--prefix-filter-value val       - Считывать список опрашиваемых датчиков, только у которых field=value" << endl;
	cout << "--prefix-statistic-sec sec      - Выводить статистику опроса каждые sec секунд" << endl;
	cout << "--prefix-sm-ready-timeout       - время на ожидание старта SM" << endl;
	cout << "--prefix-exchange-mode-id       - Идентификатор (AI) датчика, позволяющего управлять работой процесса" << endl;
	cout << "--prefix-set-prop-prefix val    - Использовать для свойств указанный или пустой префикс." << endl;
}
// -----------------------------------------------------------------------------
MBExchange::~MBExchange()
{
	for( RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		if( it1->second->rtu )
		{
			delete it1->second->rtu;
			it1->second->rtu = 0;
		}
		
		RTUDevice* d(it1->second);
		for( RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
			delete it->second;

		delete it1->second;
	}
	
	delete shm;
}
// -----------------------------------------------------------------------------
void MBExchange::waitSMReady()
{
	// waiting for SM is ready...
	int ready_timeout = conf->getArgInt("--" + prefix +"-sm-ready-timeout","15000");
	if( ready_timeout == 0 )
		ready_timeout = 15000;
	else if( ready_timeout < 0 )
		ready_timeout = UniSetTimer::WaitUpTime;

	if( !shm->waitSMready(ready_timeout, 50) )
	{
		ostringstream err;
		err << myname << "(waitSMReady): failed waiting SharedMemory " << ready_timeout << " msec";
		dlog[Debug::CRIT] << err.str() << endl;
		throw SystemError(err.str());
	}
}
// -----------------------------------------------------------------------------
void MBExchange::step()
{
	if( !checkProcActive() )
		return;

	updateRespondSensors();

	if( sidHeartBeat!=DefaultObjectId && ptHeartBeat.checkTime() )
	{
		try
		{
			shm->localSaveValue(aitHeartBeat,sidHeartBeat,maxHeartBeat,getId());
			ptHeartBeat.reset();
		}
		catch(Exception& ex)
		{
			dlog[Debug::CRIT] << myname
				<< "(step): (hb) " << ex << std::endl;
		}
	}
}

// -----------------------------------------------------------------------------
bool MBExchange::checkProcActive()
{
	uniset_mutex_lock l(actMutex, 300);
	return activated;
}
// -----------------------------------------------------------------------------
void MBExchange::setProcActive( bool st )
{
	uniset_mutex_lock l(actMutex, 400);
	activated = st;
}
// -----------------------------------------------------------------------------
void MBExchange::sigterm( int signo )
{
	dlog[Debug::WARN] << myname << ": ********* SIGTERM(" << signo << ") ********" << endl;
	setProcActive(false);
	UniSetObject_LT::sigterm(signo);
}
// ------------------------------------------------------------------------------------------
void MBExchange::readConfiguration()
{
//	readconf_ok = false;
	xmlNode* root = conf->getXMLSensorsSection();
	if(!root)
	{
		ostringstream err;
		err << myname << "(readConfiguration): не нашли корневого раздела <sensors>";
		throw SystemError(err.str());
	}

	UniXML_iterator it(root);
	if( !it.goChildren() )
	{
		dlog[Debug::CRIT] << myname << "(readConfiguration): раздел <sensors> не содержит секций ?!!\n";
		return;
	}

	for( ;it.getCurrent(); it.goNext() )
	{
		if( UniSetTypes::check_filter(it,s_field,s_fvalue) )
			initItem(it);
	}
	
//	readconf_ok = true;
}
// ------------------------------------------------------------------------------------------
bool MBExchange::readItem( UniXML& xml, UniXML_iterator& it, xmlNode* sec )
{
	if( UniSetTypes::check_filter(it,s_field,s_fvalue) )
		initItem(it);
	return true;
}

// ------------------------------------------------------------------------------------------
MBExchange::DeviceType MBExchange::getDeviceType( const std::string& dtype )
{
	if( dtype.empty() )
		return dtUnknown;
	
	if( dtype == "mtr" || dtype == "MTR" )
		return dtMTR;
	
	if( dtype == "rtu" || dtype == "RTU" )
		return dtRTU;

	if( dtype == "rtu188" || dtype == "RTU188" )
		return dtRTU188;
	
	return dtUnknown;
}
// ------------------------------------------------------------------------------------------
void MBExchange::initIterators()
{
	shm->initAIterator(aitHeartBeat);
	shm->initAIterator(aitExchangeMode);
	for( MBExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		shm->initDIterator(d->resp_dit);
		shm->initAIterator(d->mode_ait);
		for( MBExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			for( PList::iterator it2=it->second->slst.begin();it2!=it->second->slst.end(); ++it2 )
			{
				shm->initDIterator(it2->dit);
				shm->initAIterator(it2->ait);
                shm->initAIterator(it2->t_ait);
			}
		}
	}
	
	for( ThresholdList::iterator t=thrlist.begin(); t!=thrlist.end(); ++t )
	{
		shm->initDIterator(t->dit);
		shm->initAIterator(t->ait);
		shm->initAIterator(t->t_ait);
	}
}
// -----------------------------------------------------------------------------
void MBExchange::initValues()
{
	for( MBExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		for( MBExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			
			for( PList::iterator it2=it->second->slst.begin();it2!=it->second->slst.end(); ++it2 )
			{
				if( it2->stype == UniversalIO::DigitalInput || it2->stype == UniversalIO::DigitalOutput )
					it2->value = shm->localGetState(it2->dit,it2->si.id);
				else if( it2->stype == UniversalIO::AnalogInput || it2->stype == UniversalIO::AnalogOutput )
					it2->value = shm->localGetValue(it2->ait,it2->si.id);
			}
			it->second->sm_initOK = true;
		}
	}

	for( ThresholdList::iterator t=thrlist.begin(); t!=thrlist.end(); ++t )
	{
		if( t->stype == UniversalIO::DigitalInput || t->stype == UniversalIO::DigitalOutput )
			t->value = shm->localGetState(t->dit,t->si.id);
		else if( t->stype == UniversalIO::AnalogInput || t->stype == UniversalIO::AnalogOutput )
			t->value = shm->localGetValue(t->ait,t->si.id);
	}
}
// -----------------------------------------------------------------------------
bool MBExchange::checkUpdateSM( bool wrFunc, long mdev )
{
	if( exchangeMode == emSkipExchange || mdev == emSkipExchange )
	{
		if( wrFunc )
			return true; // данные для посылки, должны обновляться всегда (чтобы быть актуальными)

		if( dlog.debugging(Debug::LEVEL3) )
			dlog[Debug::LEVEL3] << "(checkUpdateSM):"
				<< " skip... mode='emSkipExchange' " << endl;
		return false;
	}

	if( wrFunc && (exchangeMode == emReadOnly || mdev == emReadOnly) )
	{
		if( dlog.debugging(Debug::LEVEL3) )
			dlog[Debug::LEVEL3] << "(checkUpdateSM):"
				<< " skip... mode='emReadOnly' " << endl;
		return false;
	}

	if( !wrFunc && (exchangeMode == emWriteOnly || mdev == emWriteOnly) )
	{
		if( dlog.debugging(Debug::LEVEL3) )
			dlog[Debug::LEVEL3] << "(checkUpdateSM):"
				<< " skip... mode='emWriteOnly' " << endl;
		return false;
	}
	
	if( wrFunc && (exchangeMode == emSkipSaveToSM || mdev == emSkipSaveToSM) )
	{
		if( dlog.debugging(Debug::LEVEL3) )
			dlog[Debug::LEVEL3] << "(checkUpdateSM):"
				<< " skip... mode='emSkipSaveToSM' " << endl;
		return false;
	}

	return true;
}
// -----------------------------------------------------------------------------
bool MBExchange::checkPoll( bool wrFunc )
{
	if( exchangeMode == emWriteOnly && !wrFunc )
	{
		if( dlog.debugging(Debug::LEVEL3)  )
			dlog[Debug::LEVEL3] << myname << "(checkPoll): skip.. mode='emWriteOnly'" << endl;
		return false;
	}
	
	if( exchangeMode == emReadOnly && wrFunc )
	{
		if( dlog.debugging(Debug::LEVEL3)  )
			dlog[Debug::LEVEL3] << myname << "(checkPoll): skip.. poll mode='emReadOnly'" << endl;

		return false;
	}

	return true;
}
// -----------------------------------------------------------------------------
MBExchange::RegID MBExchange::genRegID( const ModbusRTU::ModbusData mbreg, const int fn )
{
	// формула для вычисления ID
	// требования:
	//  1. ID > диапазона возможных регистров
	//  2. одинаковые регистры, но разные функции должны давать разный ID
	//  3. регистры идущие подряд, должна давать ID идущие тоже подряд
	
	// Вообще диапазоны: 
	// mbreg: 0..65535
	// fn: 0...255
	int max = numeric_limits<ModbusRTU::ModbusData>::max(); // по идее 65535
	int fn_max = numeric_limits<ModbusRTU::ModbusByte>::max(); // по идее 255

	// fn необходимо привести к диапазону 0..max
	return max + mbreg + max + UniSetTypes::lcalibrate(fn,0,fn_max,0,max,false);
}
// ------------------------------------------------------------------------------------------
void MBExchange::printMap( MBExchange::RTUDeviceMap& m )
{
	cout << "devices: " << endl;
	for( MBExchange::RTUDeviceMap::iterator it=m.begin(); it!=m.end(); ++it )
	{
		cout << "  " <<  *(it->second) << endl;
	}
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, MBExchange::RTUDeviceMap& m )
{
	os << "devices: " << endl;
	for( MBExchange::RTUDeviceMap::iterator it=m.begin(); it!=m.end(); ++it )
	{
		os << "  " <<  *(it->second) << endl;
	}
	return os;
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, MBExchange::RTUDevice& d )
{
  	os 	<< "addr=" << ModbusRTU::addr2str(d.mbaddr)
  		<< " type=" << d.dtype
  		<< " respond_id=" << d.resp_id
  		<< " respond_timeout=" << d.resp_Delay.getOnDelay()
  		<< " respond_state=" << d.resp_state
  		<< " respond_invert=" << d.resp_invert
  		<< endl;
  			

	os << "  regs: " << endl;
	for( MBExchange::RegMap::iterator it=d.regmap.begin(); it!=d.regmap.end(); ++it )
		os << "     " << *(it->second) << endl;
	
	return os;
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, MBExchange::RegInfo* r )
{
	return os << (*r);
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, MBExchange::RegInfo& r )
{
	os << " id=" << r.id
		<< " mbreg=" << ModbusRTU::dat2str(r.mbreg)
		<< " mbfunc=" << r.mbfunc
		<< " q_num=" << r.q_num
		<< " q_count=" << r.q_count
		<< " value=" << ModbusRTU::dat2str(r.mbval) << "(" << (int)r.mbval << ")"
		<< " mtrType=" << MTR::type2str(r.mtrType)
		<< endl;

	for( MBExchange::PList::iterator it=r.slst.begin(); it!=r.slst.end(); ++it )
		os << "         " << (*it) << endl;

	return os;
}
// -----------------------------------------------------------------------------
void MBExchange::rtuQueryOptimization( RTUDeviceMap& m )
{
	if( noQueryOptimization )
		return;

	dlog[Debug::INFO] << myname << "(rtuQueryOptimization): optimization..." << endl;

	for( MBExchange::RTUDeviceMap::iterator it1=m.begin(); it1!=m.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		
		// Вообще в map они уже лежат в нужном порядке, т.е. функция genRegID() гарантирует
		// что регистры идущие подряд с одниковой функцией чтения/записи получат подряд идущие ID.
		// так что оптимтизация это просто нахождение мест где id идут не подряд...
		for( MBExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			MBExchange::RegMap::iterator beg = it;
			RegID id = it->second->id; // или собственно it->first
			beg->second->q_num = 1;
			beg->second->q_count = 1;
			it++;
			for( ;it!=d->regmap.end(); ++it )
			{
				if( (it->second->id - id) > 1 )
				{
					it--;  // раз это регистр уже следующий, то надо вернуть на шаг обратно..
					break;
				}
				
				beg->second->q_count++;

				if( beg->second->q_count >= ModbusRTU::MAXDATALEN  )
					break;

				id = it->second->id;
				it->second->q_num = beg->second->q_count;
				it->second->q_count = 0;
			}

			// check correct function...
			if( beg->second->q_count>1 && beg->second->mbfunc==ModbusRTU::fnWriteOutputSingleRegister )
			{
				dlog[Debug::WARN] << myname << "(rtuQueryOptimization): "
					<< " optimization change func=" << ModbusRTU::fnWriteOutputSingleRegister
					<< " <--> func=" << ModbusRTU::fnWriteOutputRegisters
					<< " for mbaddr=" << ModbusRTU::addr2str(d->mbaddr)
					<< " mbreg=" << ModbusRTU::dat2str(beg->second->mbreg);
				
				beg->second->mbfunc = ModbusRTU::fnWriteOutputRegisters;
			}
			else if( beg->second->q_count>1 && beg->second->mbfunc==ModbusRTU::fnForceSingleCoil )
			{
				dlog[Debug::WARN] << myname << "(rtuQueryOptimization): "
					<< " optimization change func=" << ModbusRTU::fnForceSingleCoil
					<< " <--> func=" << ModbusRTU::fnForceMultipleCoils
					<< " for mbaddr=" << ModbusRTU::addr2str(d->mbaddr)
					<< " mbreg=" << ModbusRTU::dat2str(beg->second->mbreg);
				
				beg->second->mbfunc = ModbusRTU::fnForceMultipleCoils;
			}
			
			if( it==d->regmap.end() )
				break;
		}
	}
}
// -----------------------------------------------------------------------------
//std::ostream& operator<<( std::ostream& os, MBExchange::PList& lst )
std::ostream& MBExchange::print_plist( std::ostream& os, MBExchange::PList& lst )
{
	os << "[ ";
	for( MBExchange::PList::const_iterator it=lst.begin(); it!=lst.end(); ++it )
		os << "(" << it->si.id << ")" << conf->oind->getBaseName(conf->oind->getMapName(it->si.id)) << " ";
	os << "]";

	return os;
}
// -----------------------------------------------------------------------------
void MBExchange::firstInitRegisters()
{
	// если все вернут TRUE, значит OK.
	allInitOK = true;
	for( InitList::iterator it=initRegList.begin(); it!=initRegList.end(); ++it )
	{
		try
		{
			it->initOK = preInitRead(it);
			allInitOK = it->initOK;
		}
		catch( ModbusRTU::mbException& ex )
		{
			allInitOK = false;
			if( dlog.debugging(Debug::LEVEL3) )
			{
				dlog[Debug::LEVEL3] << myname << "(preInitRead): FAILED ask addr=" << ModbusRTU::addr2str(it->dev->mbaddr)
					<< " reg=" << ModbusRTU::dat2str(it->mbreg)
					<< " err: " << ex << endl;
			}

			if( !it->dev->ask_every_reg )
				break;
		}
	}
}
// -----------------------------------------------------------------------------
bool MBExchange::preInitRead( InitList::iterator& p )
{
	if( p->initOK )
		return true;

	RTUDevice* dev = p->dev;
	int q_count = p->p.rnum;

	if( dlog.debugging(Debug::LEVEL3)  )
	{
		dlog[Debug::LEVEL3] << myname << "(preInitRead): poll "
			<< " mbaddr=" << ModbusRTU::addr2str(dev->mbaddr)
			<< " mbreg=" << ModbusRTU::dat2str(p->mbreg)
			<< " mbfunc=" << p->mbfunc
			<< " q_count=" << q_count
			<< endl;

			if( q_count > ModbusRTU::MAXDATALEN )
			{
					dlog[Debug::LEVEL3] << myname << "(preInitRead): count(" << q_count
					<< ") > MAXDATALEN(" << ModbusRTU::MAXDATALEN
					<< " ..ignore..."
					<< endl;
			}
	}

	switch( p->mbfunc )
	{
		case ModbusRTU::fnReadOutputRegisters:
		{
			ModbusRTU::ReadOutputRetMessage ret = mb->read03(dev->mbaddr,p->mbreg,q_count);
			p->initOK = initSMValue(ret.data, ret.count,&(p->p));
		}
		break;

		case ModbusRTU::fnReadInputRegisters:
		{
			ModbusRTU::ReadInputRetMessage ret1 = mb->read04(dev->mbaddr,p->mbreg,q_count);
			p->initOK = initSMValue(ret1.data,ret1.count, &(p->p));
		}
		break;
		case ModbusRTU::fnReadInputStatus:
		{
			ModbusRTU::ReadInputStatusRetMessage ret = mb->read02(dev->mbaddr,p->mbreg,q_count);
			ModbusRTU::ModbusData* dat = new ModbusRTU::ModbusData[q_count];
			int m=0;
			for( int i=0; i<ret.bcnt; i++ )
			{
				ModbusRTU::DataBits b(ret.data[i]);
				for( int k=0;k<ModbusRTU::BitsPerByte && m<q_count; k++,m++ )
					dat[m] = b[k];
			}

			p->initOK = initSMValue(dat,q_count, &(p->p));
			delete[] dat;
		}
		break;

		case ModbusRTU::fnReadCoilStatus:
		{
			ModbusRTU::ReadCoilRetMessage ret = mb->read01(dev->mbaddr,p->mbreg,q_count);
			ModbusRTU::ModbusData* dat = new ModbusRTU::ModbusData[q_count];
			int m = 0;
			for( int i=0; i<ret.bcnt; i++ )
			{
				ModbusRTU::DataBits b(ret.data[i]);
				for( int k=0;k<ModbusRTU::BitsPerByte && m<q_count; k++,m++ )
					dat[m] = b[k];
			}

			p->initOK = initSMValue(dat,q_count, &(p->p));
			delete[] dat;
		}
		break;

		default:
			p->initOK = false;
			break;
	}

	if( p->initOK )
	{
		bool f_out = force_out;
		// выставляем флаг принудительного обновления
		force_out = true; 
		p->ri->mb_initOK = true;
		p->ri->sm_initOK = false;
		updateRTU(p->ri->rit);
		force_out = f_out;
	}

	return p->initOK;
}
// -----------------------------------------------------------------------------
bool MBExchange::initSMValue( ModbusRTU::ModbusData* data, int count, RSProperty* p )
{
	using namespace ModbusRTU;
	try
	{
		if( p->vType == VTypes::vtUnknown )
		{
			ModbusRTU::DataBits16 b(data[0]);
			if( p->nbit >= 0 )
			{
				bool set = b[p->nbit];
				IOBase::processingAsDI( p, set, shm, true );
				return true;
			}

			if( p->rnum <= 1 )
			{
				if( p->stype == UniversalIO::DigitalInput ||
					p->stype == UniversalIO::DigitalOutput )
				{
					IOBase::processingAsDI( p, data[0], shm, true );
				}
				else
					IOBase::processingAsAI( p, (signed short)(data[0]), shm, true );

				return true;
			}

			dlog[Debug::CRIT] << myname << "(initSMValue): IGNORE item: rnum=" << p->rnum
					<< " > 1 ?!! for id=" << p->si.id << endl;

			return false;
		}
		else if( p->vType == VTypes::vtSigned )
		{
			if( p->stype == UniversalIO::DigitalInput ||
				p->stype == UniversalIO::DigitalOutput )
			{
				IOBase::processingAsDI( p, data[0], shm, true );
			}
			else
				IOBase::processingAsAI( p, (signed short)(data[0]), shm, true );

			return true;
		}
		else if( p->vType == VTypes::vtUnsigned )
		{
			if( p->stype == UniversalIO::DigitalInput ||
				p->stype == UniversalIO::DigitalOutput )
			{
				IOBase::processingAsDI( p, data[0], shm, true );
			}
			else
				IOBase::processingAsAI( p, (unsigned short)data[0], shm, true );

			return true;
		}
		else if( p->vType == VTypes::vtByte )
		{
			if( p->nbyte <= 0 || p->nbyte > VTypes::Byte::bsize )
			{
				dlog[Debug::CRIT] << myname << "(initSMValue): IGNORE item: sid=" << ModbusRTU::dat2str(p->si.id)
						<< " vtype=" << p->vType << " but nbyte=" << p->nbyte << endl;
				return false;
			}

			VTypes::Byte b(data[0]);
			IOBase::processingAsAI( p, b.raw.b[p->nbyte-1], shm, true );
			return true;
		}
		else if( p->vType == VTypes::vtF2 )
		{
			VTypes::F2 f(data,VTypes::F2::wsize());
			IOBase::processingFasAI( p, (float)f, shm, true );
		}
		else if( p->vType == VTypes::vtF2r )
		{
			VTypes::F2r f(data,VTypes::F2r::wsize());
			IOBase::processingFasAI( p, (float)f, shm, true );
		}
		else if( p->vType == VTypes::vtF4 )
		{
			VTypes::F4 f(data,VTypes::F4::wsize());
			IOBase::processingFasAI( p, (float)f, shm, true );
		}
		else if( p->vType == VTypes::vtI2 )
		{
			VTypes::I2 i2(data,VTypes::I2::wsize());
			IOBase::processingAsAI( p, (int)i2, shm, true );
		}
		else if( p->vType == VTypes::vtI2r )
		{
			VTypes::I2r i2(data,VTypes::I2::wsize());
			IOBase::processingAsAI( p, (int)i2, shm, true );
		}
		else if( p->vType == VTypes::vtU2 )
		{
			VTypes::U2 u2(data,VTypes::U2::wsize());
			IOBase::processingAsAI( p, (unsigned int)u2, shm, true );
		}
		else if( p->vType == VTypes::vtU2r )
		{
			VTypes::U2r u2(data,VTypes::U2::wsize());
			IOBase::processingAsAI( p, (unsigned int)u2, shm, true );
		}

		return true;
	}
	catch(IOController_i::NameNotFound &ex)
	{
		dlog[Debug::LEVEL3] << myname << "(initSMValue):(NameNotFound) " << ex.err << endl;
	}
	catch(IOController_i::IOBadParam& ex )
	{
		dlog[Debug::LEVEL3] << myname << "(initSMValue):(IOBadParam) " << ex.err << endl;
	}
	catch(IONotifyController_i::BadRange )
	{
		dlog[Debug::LEVEL3] << myname << "(initSMValue): (BadRange)..." << endl;
	}
	catch( Exception& ex )
	{
		dlog[Debug::LEVEL3] << myname << "(initSMValue): " << ex << endl;
	}
	catch(CORBA::SystemException& ex)
	{
		dlog[Debug::LEVEL3] << myname << "(initSMValue): CORBA::SystemException: "
			<< ex.NP_minorString() << endl;
	}
	catch(...)
	{
		dlog[Debug::LEVEL3] << myname << "(initSMValue): catch ..." << endl;
	}

	return false;
}
// -----------------------------------------------------------------------------
bool MBExchange::pollRTU( RTUDevice* dev, RegMap::iterator& it )
{
	RegInfo* p(it->second);

 	if( dev->mode == emSkipExchange )
	{
		if( dlog.debugging(Debug::LEVEL3)  )
			dlog[Debug::LEVEL3] << myname << "(pollRTU): SKIP EXCHANGE (mode=emSkipExchange) "
				<< " mbaddr=" << ModbusRTU::addr2str(dev->mbaddr)
				<< endl;
		return true;
	}

	if( dlog.debugging(Debug::LEVEL3)  )
	{
		dlog[Debug::LEVEL3] << myname << "(pollRTU): poll "
			<< " mbaddr=" << ModbusRTU::addr2str(dev->mbaddr)
			<< " mbreg=" << ModbusRTU::dat2str(p->mbreg)
			<< " mbfunc=" << p->mbfunc
			<< " q_count=" << p->q_count
			<< " mb_initOK=" << p->mb_initOK
			<< " sm_initOK=" << p->sm_initOK
			<< " mbval=" << p->mbval
			<< endl;
			
			if( p->q_count > ModbusRTU::MAXDATALEN )
			{
					dlog[Debug::LEVEL3] << myname << "(pollRTU): count(" << p->q_count 
					<< ") > MAXDATALEN(" << ModbusRTU::MAXDATALEN 
					<< " ..ignore..."
					<< endl;
					
			}
	}

	if( !checkPoll(ModbusRTU::isWriteFunction(p->mbfunc)) )
		return true;

	if( p->q_count == 0 )
	{
		if( dlog.debugging(Debug::INFO) )
			dlog[Debug::INFO] << myname << "(pollRTU): q_count=0 for mbreg=" << ModbusRTU::dat2str(p->mbreg) 
					<< " IGNORE register..." << endl;
		return false;
	}
	
	switch( p->mbfunc )
	{
		case ModbusRTU::fnReadInputRegisters:
		{
			ModbusRTU::ReadInputRetMessage ret = mb->read04(dev->mbaddr,p->mbreg,p->q_count);

			for( unsigned int i = 0; i < p->q_count; i++, it++ )
			{
				it->second->mbval = ret.data[i];
				it->second->mb_initOK = true;
			}

			it--;
		}
		break;

		case ModbusRTU::fnReadOutputRegisters:
		{
			ModbusRTU::ReadOutputRetMessage ret = mb->read03(dev->mbaddr,p->mbreg,p->q_count);

			for( unsigned int i = 0; i < p->q_count; i++, it++ )
			{
				it->second->mbval = ret.data[i];
				it->second->mb_initOK = true;
			}

			it--;
		}
		break;
		
		case ModbusRTU::fnReadInputStatus:
		{
			ModbusRTU::ReadInputStatusRetMessage ret = mb->read02(dev->mbaddr,p->mbreg,p->q_count);
			int m=0;
			for( int i=0; i<ret.bcnt; i++ )
			{
				ModbusRTU::DataBits b(ret.data[i]);

				for( unsigned int k = 0; k < ModbusRTU::BitsPerByte && m < p->q_count; k++, it++, m++ )
				{
					it->second->mbval = b[k];
					it->second->mb_initOK = true;
				}
			}

			it--;
		}
		break;
		
		case ModbusRTU::fnReadCoilStatus:
		{
			ModbusRTU::ReadCoilRetMessage ret = mb->read01(dev->mbaddr,p->mbreg,p->q_count);
			int m = 0;
			for( int i=0; i<ret.bcnt; i++ )
			{
				ModbusRTU::DataBits b(ret.data[i]);

				for( unsigned int k = 0; k < ModbusRTU::BitsPerByte && m < p->q_count; k++, it++, m++ )
				{
					it->second->mbval = b[k] ? 1 : 0;
					it->second->mb_initOK = true;
				}
			}

			it--;
		}
		break;
		
		case ModbusRTU::fnWriteOutputSingleRegister:
		{
			if( p->q_count != 1 )
			{
				dlog[Debug::CRIT] << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg) 
					<< " IGNORE WRITE SINGLE REGISTER (0x06) q_count=" << p->q_count << " ..." << endl;
				return false;
			}
			
			if( !p->sm_initOK )
			{
				if( dlog.debugging(Debug::LEVEL3) )
					dlog[Debug::LEVEL3] << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg)
					    << " slist=" << (*p)
						<< " IGNORE...(sm_initOK=false)" << endl;
				return true;
			}	

//			cerr << "**** mbreg=" << ModbusRTU::dat2str(p->mbreg) << " val=" << ModbusRTU::dat2str(p->mbval) << endl;
			ModbusRTU::WriteSingleOutputRetMessage ret = mb->write06(dev->mbaddr,p->mbreg,p->mbval);
		}
		break;

		case ModbusRTU::fnWriteOutputRegisters:
		{
			if( !p->sm_initOK )
			{
				// может быть такая ситуация, что 
				// некоторые регистры уже инициализированы, а другие ещё нет
				// при этом после оптимизации они попадают в один запрос
				// поэтому здесь сделано так:
				// если q_num > 1, значит этот регистр относится к предыдущему запросу
				// и его просто надо пропустить..
				if( p->q_num > 1 )
					return true;
			  
				// смещаем итератор, если данный запрос содержит много регистров
//				if( q->count > 1 )
//				{
//  				for( int i=0; i<p->q_count; i++ )
//					it++;
//					return true;
//				}
				
				if( dlog.debugging(Debug::LEVEL3) )
					dlog[Debug::LEVEL3] << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg)
						<< " IGNORE...(sm_initOK=false)" << endl;
				return true;
			}			  

			ModbusRTU::WriteOutputMessage msg(dev->mbaddr,p->mbreg);
			for( int i=0; i<p->q_count; i++,it++ )
				msg.addData(it->second->mbval);

			it--;
			ModbusRTU::WriteOutputRetMessage ret = mb->write10(msg);
		}
		break;

		case ModbusRTU::fnForceSingleCoil:
		{
			if( p->q_count != 1 )
			{
				dlog[Debug::CRIT] << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg) 
					<< " IGNORE FORCE SINGLE COIL (0x05) q_count=" << p->q_count << " ..." << endl;
				return false;
			}
			if( !p->sm_initOK )
			{
				if( dlog.debugging(Debug::LEVEL3) )
					dlog[Debug::LEVEL3] << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg)
						<< " IGNORE...(sm_initOK=false)" << endl;
				return true;
			}	

//			cerr << "****(coil) mbreg=" << ModbusRTU::dat2str(p->mbreg) << " val=" << ModbusRTU::dat2str(p->mbval) << endl;
			ModbusRTU::ForceSingleCoilRetMessage ret = mb->write05(dev->mbaddr,p->mbreg,p->mbval);
		}
		break;

		case ModbusRTU::fnForceMultipleCoils:
		{
			if( !p->sm_initOK )
			{
				if( dlog.debugging(Debug::LEVEL3) )
					dlog[Debug::LEVEL3] << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg)
						<< " IGNORE...(sm_initOK=false)" << endl;
				return true;
			}	

			ModbusRTU::ForceCoilsMessage msg(dev->mbaddr,p->mbreg);

			for( unsigned i = 0; i < p->q_count; i++, it++ )
			{
				msg.addBit( (it->second->mbval ? true : false) );
				it->second->mb_initOK = true;
			}

			it--;
//			cerr << "*********** (write multiple): " << msg << endl;
			ModbusRTU::ForceCoilsRetMessage ret = mb->write0F(msg);
		}
		break;
		
		default:
		{
			if( dlog.debugging(Debug::WARN) )
				dlog[Debug::WARN] << myname << "(pollRTU): mbreg=" << ModbusRTU::dat2str(p->mbreg) 
					<< " IGNORE mfunc=" << (int)p->mbfunc << " ..." << endl;
			return false;
		}
		break;
	}
	
	return true;
}
// -----------------------------------------------------------------------------
void MBExchange::updateSM()
{
	for( MBExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);

		if( d->mode_id != DefaultObjectId )
		{
			try
			{
				if( !shm->isLocalwork() )
					d->mode = shm->localGetValue(d->mode_ait,d->mode_id);
			}
			catch(IOController_i::NameNotFound &ex)
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM):(NameNotFound) " << ex.err << endl;
			}
			catch(IOController_i::IOBadParam& ex )
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM):(IOBadParam) " << ex.err << endl;
			}
			catch(IONotifyController_i::BadRange )
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM): (BadRange)..." << endl;
			}
			catch( Exception& ex )
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM): " << ex << endl;
			}
			catch(CORBA::SystemException& ex)
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM): CORBA::SystemException: "
					<< ex.NP_minorString() << endl;
			}
			catch(...)
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM): check modeSensor..catch ..." << endl;
			}
		}

		// обновление датчиков связи происходит в другом потоке
		// чтобы не зависеть от TCP таймаутов
		// см. updateRespondSensors()

		// update values...
		for( MBExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			try
			{
				if( d->dtype == dtRTU )
					updateRTU(it);
				else if( d->dtype == dtMTR )
					updateMTR(it);
				else if( d->dtype == dtRTU188 )
					updateRTU188(it);
			}
			catch(IOController_i::NameNotFound &ex)
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM):(NameNotFound) " << ex.err << endl;
			}
			catch(IOController_i::IOBadParam& ex )
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM):(IOBadParam) " << ex.err << endl;
			}
			catch(IONotifyController_i::BadRange )
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM): (BadRange)..." << endl;
			}
			catch( Exception& ex )
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM): " << ex << endl;
			}
			catch(CORBA::SystemException& ex)
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM): CORBA::SystemException: "
					<< ex.NP_minorString() << endl;
			}
			catch(...)
			{
				dlog[Debug::LEVEL3] << myname << "(updateSM): catch ..." << endl;
			}
			
			if( it==d->regmap.end() )
				break;
		}
	}
}

// -----------------------------------------------------------------------------
void MBExchange::updateRTU( RegMap::iterator& rit )
{
	RegInfo* r(rit->second);
	for( PList::iterator it=r->slst.begin(); it!=r->slst.end(); ++it )
		updateRSProperty( &(*it),false );

}
// -----------------------------------------------------------------------------
void MBExchange::updateRSProperty( RSProperty* p, bool write_only )
{
	using namespace ModbusRTU;
	RegInfo* r(p->reg->rit->second);

	bool save = isWriteFunction( r->mbfunc );
	
	if( !save && write_only )
		return;

	if( !checkUpdateSM(save,r->dev->mode) )
		return;
	
	// если требуется инициализация и она ещё не произведена,
	// то игнорируем
//	if( save && !r->mb_initOK )
//		return;

	// если мы ещё не разу не считали значение из устройства, то обновлять в SM
	// не нужно!
	if( !r->mb_initOK )
		return;

	if( dlog.debugging(Debug::LEVEL3) )
		dlog[Debug::LEVEL3] << myname << "(updateP): sid=" << p->si.id 
				<< " mbval=" << r->mbval 
				<< " vtype=" << p->vType
				<< " rnum=" << p->rnum
				<< " nbit=" << p->nbit
				<< " save=" << save
				<< " ioype=" << p->stype
				<< " mb_initOK=" << r->mb_initOK
				<< " sm_initOK=" << r->sm_initOK
				<< endl;

		try
		{
			if( p->vType == VTypes::vtUnknown )
			{
				ModbusRTU::DataBits16 b(r->mbval);
				if( p->nbit >= 0 )
				{
					if( save )
					{
						if( r->mb_initOK )
						{
							bool set = IOBase::processingAsDO( p, shm, force_out );
							b.set(p->nbit,set);
							r->mbval = b.mdata();
							r->sm_initOK = true;
						}
					}
					else
					{
						bool set = b[p->nbit];
						IOBase::processingAsDI( p, set, shm, force );
					}

					return;
				}
			
				if( p->rnum <= 1 )
				{
					if( save )
					{
						  if(  r->mb_initOK )
						  {
								if( p->stype == UniversalIO::DigitalInput ||
									p->stype == UniversalIO::DigitalOutput )
								{
									r->mbval = IOBase::processingAsDO( p, shm, force_out );
								}
								else  
									r->mbval = IOBase::processingAsAO( p, shm, force_out );

								r->sm_initOK = true;
						  }
					}
					else
					{
						if( p->stype == UniversalIO::DigitalInput ||
							p->stype == UniversalIO::DigitalOutput )
						{
							IOBase::processingAsDI( p, r->mbval, shm, force );
						}
						else
							IOBase::processingAsAI( p, (signed short)(r->mbval), shm, force );
					}

					return;
				}

				dlog[Debug::CRIT] << myname << "(updateRSProperty): IGNORE item: rnum=" << p->rnum 
						<< " > 1 ?!! for id=" << p->si.id << endl;
				return;
			}
			else if( p->vType == VTypes::vtSigned )
			{
				if( save )
				{
					if( r->mb_initOK )
					{
						  if( p->stype == UniversalIO::DigitalInput ||
							  p->stype == UniversalIO::DigitalOutput )
						  {
							  r->mbval = (signed short)IOBase::processingAsDO( p, shm, force_out );
						  }
						  else  
							  r->mbval = (signed short)IOBase::processingAsAO( p, shm, force_out );

						  r->sm_initOK = true;
					}
				}
				else
				{
					if( p->stype == UniversalIO::DigitalInput ||
						p->stype == UniversalIO::DigitalOutput )
					{
						IOBase::processingAsDI( p, r->mbval, shm, force );
					}
					else
					{
						IOBase::processingAsAI( p, (signed short)(r->mbval), shm, force );
					}
				}

				return;
			}
			else if( p->vType == VTypes::vtUnsigned )
			{
				if( save )
				{
					  if( r->mb_initOK )
					  {
						  if( p->stype == UniversalIO::DigitalInput ||
							  p->stype == UniversalIO::DigitalOutput )
						  {
							  r->mbval = (unsigned short)IOBase::processingAsDO( p, shm, force_out );
						  }
						  else  
							  r->mbval = (unsigned short)IOBase::processingAsAO( p, shm, force_out );

						  r->sm_initOK = true;
					  }
				}
				else
				{
					if( p->stype == UniversalIO::DigitalInput ||
						p->stype == UniversalIO::DigitalOutput )
					{
						IOBase::processingAsDI( p, r->mbval, shm, force );
					}
					else
					{
						IOBase::processingAsAI( p, (unsigned short)r->mbval, shm, force );
					}
				}

				return;
			}
			else if( p->vType == VTypes::vtByte )
			{
				if( p->nbyte <= 0 || p->nbyte > VTypes::Byte::bsize )
				{
					dlog[Debug::CRIT] << myname << "(updateRSProperty): IGNORE item: reg=" << ModbusRTU::dat2str(r->mbreg) 
							<< " vtype=" << p->vType << " but nbyte=" << p->nbyte << endl;
					return;
				}

				if( save )
				{
					  if( r->mb_initOK )
					  {
						  long v = IOBase::processingAsAO( p, shm, force_out );
						  VTypes::Byte b(r->mbval);
						  b.raw.b[p->nbyte-1] = v;
						  r->mbval = b.raw.w;
						  r->sm_initOK = true;
					  }
				}
				else
				{
					VTypes::Byte b(r->mbval);
					IOBase::processingAsAI( p, b.raw.b[p->nbyte-1], shm, force );
				}
				
				return;
			}
			else if( p->vType == VTypes::vtF2 || p->vType == VTypes::vtF2r )
			{
				RegMap::iterator i(p->reg->rit);
				if( save )
				{
					if( r->mb_initOK )
					{
						float f = IOBase::processingFasAO( p, shm, force_out );
						if( p->vType == VTypes::vtF2 )
						{
							VTypes::F2 f2(f);
							for( int k=0; k<VTypes::F2::wsize(); k++, i++ )
								i->second->mbval = f2.raw.v[k];
						}
						else if( p->vType == VTypes::vtF2r )
						{
							VTypes::F2r f2(f);
							for( int k=0; k<VTypes::F2r::wsize(); k++, i++ )
								i->second->mbval = f2.raw.v[k];
						}

						r->sm_initOK = true;
					}
				}
				else
				{
					ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[VTypes::F2::wsize()];
					for( int k=0; k<VTypes::F2::wsize(); k++, i++ )
						data[k] = i->second->mbval;
	
					float f=0;
					if( p->vType == VTypes::vtF2 )
					{
						VTypes::F2 f1(data,VTypes::F2::wsize());
						f = (float)f1;
					}
					else if( p->vType == VTypes::vtF2r )
					{
						VTypes::F2r f1(data,VTypes::F2r::wsize());
						f = (float)f1;
					}
	
					delete[] data;
				
					IOBase::processingFasAI( p, f, shm, force );
				}
			}
			else if( p->vType == VTypes::vtF4 )
			{
				RegMap::iterator i(p->reg->rit);
				if( save )
				{
					if( r->mb_initOK )
					{
						float f = IOBase::processingFasAO( p, shm, force_out );
						VTypes::F4 f4(f);
						for( int k=0; k<VTypes::F4::wsize(); k++, i++ )
							i->second->mbval = f4.raw.v[k];

						r->sm_initOK = true;
					}
				}
				else
				{
					ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[VTypes::F4::wsize()];
					for( int k=0; k<VTypes::F4::wsize(); k++, i++ )
						data[k] = i->second->mbval;
				
					VTypes::F4 f(data,VTypes::F4::wsize());
					delete[] data;
				
					IOBase::processingFasAI( p, (float)f, shm, force );
				}
			}
			else if( p->vType == VTypes::vtI2 || p->vType == VTypes::vtI2r )
			{
				RegMap::iterator i(p->reg->rit);
				if( save )
				{
					if( r->mb_initOK )
					{
						long v = IOBase::processingAsAO( p, shm, force_out );
						if( p->vType == VTypes::vtI2 )
						{
							VTypes::I2 i2(v);
							for( int k=0; k<VTypes::I2::wsize(); k++, i++ )
								i->second->mbval = i2.raw.v[k];
						}
						else if( p->vType == VTypes::vtI2r )
						{
							VTypes::I2r i2(v);
							for( int k=0; k<VTypes::I2::wsize(); k++, i++ )
								i->second->mbval = i2.raw.v[k];
						}
						r->sm_initOK = true;
					}
				}
				else
				{
					ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[VTypes::I2::wsize()];
					for( int k=0; k<VTypes::I2::wsize(); k++, i++ )
						data[k] = i->second->mbval;
				
					int v = 0;
					if( p->vType == VTypes::vtI2 )
					{
						VTypes::I2 i2(data,VTypes::I2::wsize());
						v = (int)i2;
					}
					else if( p->vType == VTypes::vtI2r )
					{
						VTypes::I2r i2(data,VTypes::I2::wsize());
						v = (int)i2;
					}

					delete[] data;
					IOBase::processingAsAI( p, v, shm, force );
				}
			}
			else if( p->vType == VTypes::vtU2 || p->vType == VTypes::vtU2r )
			{
				RegMap::iterator i(p->reg->rit);
				if( save )
				{
					if( r->mb_initOK )
					{
						long v = IOBase::processingAsAO( p, shm, force_out );
						if( p->vType == VTypes::vtU2 )
						{
							VTypes::U2 u2(v);
							for( int k=0; k<VTypes::U2::wsize(); k++, i++ )
								i->second->mbval = u2.raw.v[k];
						}
						else if( p->vType == VTypes::vtU2r )
						{
							VTypes::U2r u2(v);
							for( int k=0; k<VTypes::U2::wsize(); k++, i++ )
								i->second->mbval = u2.raw.v[k];
						}

						r->sm_initOK = true;
					}
				}
				else
				{
					ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[VTypes::U2::wsize()];
					for( int k=0; k<VTypes::U2::wsize(); k++, i++ )
						data[k] = i->second->mbval;
				
					unsigned int v = 0;
					if( p->vType == VTypes::vtU2 )
					{
						VTypes::U2 u2(data,VTypes::U2::wsize());
						v = (unsigned int)u2;
					}
					else if( p->vType == VTypes::vtU2r )
					{
						VTypes::U2r u2(data,VTypes::U2::wsize());
						v = (unsigned int)u2;
					}

					delete[] data;
					IOBase::processingAsAI( p, v, shm, force );
				}
			}

			return;
		}
		catch(IOController_i::NameNotFound& ex)
		{
			dlog[Debug::LEVEL3] << myname << "(updateRSProperty):(NameNotFound) " << ex.err << endl;
		}
		catch(IOController_i::IOBadParam& ex )
		{
			dlog[Debug::LEVEL3] << myname << "(updateRSProperty):(IOBadParam) " << ex.err << endl;
		}
		catch(IONotifyController_i::BadRange )
		{
			dlog[Debug::LEVEL3] << myname << "(updateRSProperty): (BadRange)..." << endl;
		}
		catch( Exception& ex )
		{
			dlog[Debug::LEVEL3] << myname << "(updateRSProperty): " << ex << endl;
		}
		catch(CORBA::SystemException& ex)
		{
			dlog[Debug::LEVEL3] << myname << "(updateRSProperty): CORBA::SystemException: "
				<< ex.NP_minorString() << endl;
		}
		catch(...)
		{
			dlog[Debug::LEVEL3] << myname << "(updateRSProperty): catch ..." << endl;
		}

	// Если SM стала (или была) недоступна
	// то флаг инициализации надо сбросить
	r->sm_initOK = false;
}
// -----------------------------------------------------------------------------
void MBExchange::updateMTR( RegMap::iterator& rit )
{
	RegInfo* r(rit->second);
	if( !r || !r->dev )
		return;

	using namespace ModbusRTU;
	bool save = isWriteFunction( r->mbfunc );

	if( !checkUpdateSM(save,r->dev->mode) )
		return;

	{
		for( PList::iterator it=r->slst.begin(); it!=r->slst.end(); ++it )
		{
			try
			{
				if( r->mtrType == MTR::mtT1 )
				{
					if( save )
						r->mbval = IOBase::processingAsAO( &(*it), shm, force_out );
					else
					{
						MTR::T1 t(r->mbval);
						IOBase::processingAsAI( &(*it), t.val, shm, force );
					}
					continue;
				}
			
				if( r->mtrType == MTR::mtT2 )
				{
					if( save )
					{
						MTR::T2 t(IOBase::processingAsAO( &(*it), shm, force_out ));
						r->mbval = t.val; 
					}
					else
					{
						MTR::T2 t(r->mbval);
						IOBase::processingAsAI( &(*it), t.val, shm, force );
					}
					continue;
				}
		
				if( r->mtrType == MTR::mtT3 )
				{
					RegMap::iterator i(rit);
					if( save )
					{
						MTR::T3 t(IOBase::processingAsAO( &(*it), shm, force_out ));
						for( int k=0; k<MTR::T3::wsize(); k++, i++ )
							i->second->mbval = t.raw.v[k];
					}
					else
					{
						ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[MTR::T3::wsize()];
						for( int k=0; k<MTR::T3::wsize(); k++, i++ )
							data[k] = i->second->mbval;
		
						MTR::T3 t(data,MTR::T3::wsize());
						delete[] data;
						IOBase::processingAsAI( &(*it), (long)t, shm, force );
					}
					continue;
				}
		
				if( r->mtrType == MTR::mtT4 )
				{
					if( save )
						dlog[Debug::WARN] << myname << "(updateMTR): write (T4) reg(" << dat2str(r->mbreg) << ") to MTR NOT YET!!!" << endl;
					else
					{
						MTR::T4 t(r->mbval);
						IOBase::processingAsAI( &(*it), uni_atoi(t.sval), shm, force );
					}
					continue;
				}
		
				if( r->mtrType == MTR::mtT5 )
				{
					RegMap::iterator i(rit);
					if( save )
					{
						MTR::T5 t(IOBase::processingAsAO( &(*it), shm, force_out ));
						for( int k=0; k<MTR::T5::wsize(); k++, i++ )
							i->second->mbval = t.raw.v[k];
					}
					else
					{
						ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[MTR::T5::wsize()];
						for( int k=0; k<MTR::T5::wsize(); k++, i++ )
							data[k] = i->second->mbval;
		
						MTR::T5 t(data,MTR::T5::wsize());
						delete[] data;
					
						IOBase::processingFasAI( &(*it), (float)t.val, shm, force );
					}
					continue;
				}
		
				if( r->mtrType == MTR::mtT6 )
				{
					RegMap::iterator i(rit);
					if( save )
					{
						MTR::T6 t(IOBase::processingAsAO( &(*it), shm, force_out ));
						for( int k=0; k<MTR::T6::wsize(); k++, i++ )
							i->second->mbval = t.raw.v[k];
					}
					else
					{
						ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[MTR::T6::wsize()];
						for( int k=0; k<MTR::T6::wsize(); k++, i++ )
							data[k] = i->second->mbval;
		
						MTR::T6 t(data,MTR::T6::wsize());
						delete[] data;
					
						IOBase::processingFasAI( &(*it), (float)t.val, shm, force );
					}
					continue;
				}
		
				if( r->mtrType == MTR::mtT7 )
				{
					RegMap::iterator i(rit);
					if( save )
					{
						MTR::T7 t(IOBase::processingAsAO( &(*it), shm, force_out ));
						for( int k=0; k<MTR::T7::wsize(); k++, i++ )
							i->second->mbval = t.raw.v[k];
					}
					else
					{
						ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[MTR::T7::wsize()];
						for( int k=0; k<MTR::T7::wsize(); k++, i++ )
							data[k] = i->second->mbval;
		
						MTR::T7 t(data,MTR::T7::wsize());
						delete[] data;
					
						IOBase::processingFasAI( &(*it), (float)t.val, shm, force );
					}
					continue;
				}
				
				if( r->mtrType == MTR::mtT16 )
				{
					if( save )
					{
						MTR::T16 t(IOBase::processingFasAO( &(*it), shm, force_out ));
						r->mbval = t.val;
					}
					else
					{
						MTR::T16 t(r->mbval);
						IOBase::processingFasAI( &(*it), t.fval, shm, force );
					}
					continue;
				}

				if( r->mtrType == MTR::mtT17 )
				{
					if( save )
					{
						MTR::T17 t(IOBase::processingFasAO( &(*it), shm, force_out ));
						r->mbval = t.val;
					}
					else
					{
						MTR::T17 t(r->mbval);
						IOBase::processingFasAI( &(*it), t.fval, shm, force );
					}
					continue;
				}		
				
				if( r->mtrType == MTR::mtF1 )
				{
					RegMap::iterator i(rit);
					if( save )
					{
						float f = IOBase::processingFasAO( &(*it), shm, force_out );
						MTR::F1 f1(f);
						for( int k=0; k<MTR::F1::wsize(); k++, i++ )
							i->second->mbval = f1.raw.v[k];
					}
					else
					{
						ModbusRTU::ModbusData* data = new ModbusRTU::ModbusData[MTR::F1::wsize()];
						for( int k=0; k<MTR::F1::wsize(); k++, i++ )
							data[k] = i->second->mbval;
		
						MTR::F1 t(data,MTR::F1::wsize());
						delete[] data;
					
						IOBase::processingFasAI( &(*it), (float)t, shm, force );
					}
					continue;
				}
			}
			catch(IOController_i::NameNotFound &ex)
			{
				dlog[Debug::LEVEL3] << myname << "(updateMTR):(NameNotFound) " << ex.err << endl;
			}
			catch(IOController_i::IOBadParam& ex )
			{
				dlog[Debug::LEVEL3] << myname << "(updateMTR):(IOBadParam) " << ex.err << endl;
			}
			catch(IONotifyController_i::BadRange )
			{
				dlog[Debug::LEVEL3] << myname << "(updateMTR): (BadRange)..." << endl;
			}
			catch( Exception& ex )
			{
				dlog[Debug::LEVEL3] << myname << "(updateMTR): " << ex << endl;
			}
			catch(CORBA::SystemException& ex)
			{
				dlog[Debug::LEVEL3] << myname << "(updateMTR): CORBA::SystemException: "
					<< ex.NP_minorString() << endl;
			}
			catch(...)
			{
				dlog[Debug::LEVEL3] << myname << "(updateMTR): catch ..." << endl;
			}
		}
	}
}
// -----------------------------------------------------------------------------
void MBExchange::updateRTU188( RegMap::iterator& rit )
{
	RegInfo* r(rit->second);
	if( !r || !r->dev || !r->dev->rtu )
		return;

	using namespace ModbusRTU;

	bool save = isWriteFunction( r->mbfunc );

	// пока-что функции записи в обмене с RTU188 
	// не реализованы
	if( isWriteFunction(r->mbfunc) )
	{
		cerr << myname << "(updateRTU188): write reg(" << dat2str(r->mbreg) << ") to RTU188 NOT YET!!!" << endl;
		return;
	}

	if( exchangeMode == emSkipExchange || r->dev->mode == emSkipExchange )
	{
		if( dlog.debugging(Debug::LEVEL3) )
			dlog[Debug::LEVEL3] << myname << "(updateRTU188):"
				<< " skip... mode=emSkipExchange " << endl;
		return;
	}

	if( save && (exchangeMode == emReadOnly || r->dev->mode == emReadOnly) )
	{
		if( dlog.debugging(Debug::LEVEL3) )
			dlog[Debug::LEVEL3] << myname << "(updateRTU188):"
				<< " skip... mode=emReadOnly " << endl;
		return;
	}

	if( !save && ( exchangeMode == emWriteOnly || r->dev->mode == emWriteOnly) )
	{
		if( dlog.debugging(Debug::LEVEL3) )
			dlog[Debug::LEVEL3] << myname << "(updateRTU188):"
				<< " skip... mode=emWriteOnly " << endl;
		return;
	}
	
	if( save && ( exchangeMode == emSkipSaveToSM || r->dev->mode == emSkipSaveToSM) )
	{
		if( dlog.debugging(Debug::LEVEL3) )
			dlog[Debug::LEVEL3] << myname << "(updateRT188):"
				<< " skip... mode=emSkipSaveToSM " << endl;
		return;
	}

	for( PList::iterator it=r->slst.begin(); it!=r->slst.end(); ++it )
	{
		try
		{
			if( it->stype == UniversalIO::DigitalInput )
			{
				bool set = r->dev->rtu->getState(r->rtuJack,r->rtuChan,it->stype);
				IOBase::processingAsDI( &(*it), set, shm, force );
			}
			else if( it->stype == UniversalIO::AnalogInput )
			{
				long val = r->dev->rtu->getInt(r->rtuJack,r->rtuChan,it->stype);
				IOBase::processingAsAI( &(*it), val, shm, force );
			}
		}
		catch(IOController_i::NameNotFound &ex)
		{
			dlog[Debug::LEVEL3] << myname << "(updateRTU188):(NameNotFound) " << ex.err << endl;
		}
		catch(IOController_i::IOBadParam& ex )
		{
			dlog[Debug::LEVEL3] << myname << "(updateRTU188):(IOBadParam) " << ex.err << endl;
		}
		catch(IONotifyController_i::BadRange )
		{
			dlog[Debug::LEVEL3] << myname << "(updateRTU188): (BadRange)..." << endl;
		}
		catch( Exception& ex )
		{
			dlog[Debug::LEVEL3] << myname << "(updateRTU188): " << ex << endl;
		}
		catch(CORBA::SystemException& ex)
		{
			dlog[Debug::LEVEL3] << myname << "(updateRTU188): CORBA::SystemException: "
				<< ex.NP_minorString() << endl;
		}
		catch(...)
		{
			dlog[Debug::LEVEL3] << myname << "(updateRTU188): catch ..." << endl;
		}
	}
}
// -----------------------------------------------------------------------------

MBExchange::RTUDevice* MBExchange::addDev( RTUDeviceMap& mp, ModbusRTU::ModbusAddr a, UniXML_iterator& xmlit )
{
	RTUDeviceMap::iterator it = mp.find(a);
	if( it != mp.end() )
	{
		DeviceType dtype = getDeviceType(xmlit.getProp(prop_prefix + "mbtype"));
		if( it->second->dtype != dtype )
		{
			dlog[Debug::CRIT] << myname << "(addDev): OTHER mbtype=" << dtype << " for " << xmlit.getProp("name")
				<< ". Already used devtype=" <<  it->second->dtype 
				<< " for mbaddr=" << ModbusRTU::addr2str(it->second->mbaddr)
				<< endl;
			return 0;
		}

		dlog[Debug::INFO] << myname << "(addDev): device for addr=" << ModbusRTU::addr2str(a) 
			<< " already added. Ignore device params for " << xmlit.getProp("name") << " ..." << endl;
		return it->second;
	}

	MBExchange::RTUDevice* d = new MBExchange::RTUDevice();
	d->mbaddr = a;

	if( !initRTUDevice(d,xmlit) )
	{	
		delete d;
		return 0;
	}

	mp.insert(RTUDeviceMap::value_type(a,d));
	return d;
}
// ------------------------------------------------------------------------------------------
MBExchange::RegInfo* MBExchange::addReg( RegMap& mp, RegID id, ModbusRTU::ModbusData r,
											UniXML_iterator& xmlit, MBExchange::RTUDevice* dev,
											MBExchange::RegInfo* rcopy )
{
	RegMap::iterator it = mp.find(id);
	if( it != mp.end() )
	{
		if( !it->second->dev )
		{
			dlog[Debug::CRIT] << myname << "(addReg): for " << xmlit.getProp("name")
				<< " dev=0!!!! " << endl;
			return 0;
		}
	
		if( it->second->dev->dtype != dev->dtype )
		{
			dlog[Debug::CRIT] << myname << "(addReg): OTHER mbtype=" << dev->dtype << " for " << xmlit.getProp("name")
				<< ". Already used devtype=" <<  it->second->dev->dtype << " for " << it->second->dev << endl;
			return 0;
		}
	
		if( dlog.debugging(Debug::INFO) )
		{
			dlog[Debug::INFO] << myname << "(addReg): reg=" << ModbusRTU::dat2str(r) 
			    << "(id=" << id << ")"
				<< " already added for " << (*it->second) 
				<< " Ignore register params for " << xmlit.getProp("name") << " ..." << endl;
		}

		it->second->rit = it;
		return it->second;
	}
	
	MBExchange::RegInfo* ri;
	if( rcopy )
	{
		ri = new MBExchange::RegInfo(*rcopy);
		ri->slst.clear();
		ri->mbreg = r;
	}
	else
	{
		ri = new MBExchange::RegInfo();
		if( !initRegInfo(ri,xmlit,dev) )
		{
			delete ri;
			return 0;
		}
		ri->mbreg = r;
	}

	ri->id = id;
	
	mp.insert(RegMap::value_type(id,ri));
	ri->rit = mp.find(id);
	
	return ri;
}
// ------------------------------------------------------------------------------------------
MBExchange::RSProperty* MBExchange::addProp( PList& plist, RSProperty& p )
{
	for( PList::iterator it=plist.begin(); it!=plist.end(); ++it )
	{
		if( it->si.id == p.si.id && it->si.node == p.si.node )
			return &(*it);
	}
	
	plist.push_back(p);
	PList::iterator it = plist.end();
	it--;
	return &(*it);
}
// ------------------------------------------------------------------------------------------
bool MBExchange::initRSProperty( RSProperty& p, UniXML_iterator& it )
{
	if( !IOBase::initItem(&p,it,shm,prop_prefix,false,&dlog,myname) )
		return false;

	// проверяем не пороговый ли это датчик (т.е. не связанный с обменом)
	// тогда заносим его в отдельный список
	if( p.t_ai != DefaultObjectId )
	{
		thrlist.push_back(p);
		return true;
	}

	if( it.getIntProp(prop_prefix + "rawdata") )
	{
		p.cal.minRaw = 0;
		p.cal.maxRaw = 0;
		p.cal.minCal = 0;
		p.cal.maxCal = 0;
		p.cal.precision = 0;
		p.cdiagram = 0;
	}

	string sbit(it.getProp(prop_prefix + "nbit"));
	if( !sbit.empty() )
	{
		p.nbit = UniSetTypes::uni_atoi(sbit.c_str());
		if( p.nbit < 0 || p.nbit >= ModbusRTU::BitsPerData )
		{
			dlog[Debug::CRIT] << myname << "(initRSProperty): BAD nbit=" << p.nbit 
				<< ". (0 >= nbit < " << ModbusRTU::BitsPerData <<")." << endl;
			return false;
		}
	}
	
	if( p.nbit > 0 && 
		( p.stype == UniversalIO::AnalogInput ||
			p.stype == UniversalIO::AnalogOutput ) )
	{
		dlog[Debug::WARN] << "(initRSProperty): (ignore) uncorrect param`s nbit>1 (" << p.nbit << ")"
			<< " but iotype=" << p.stype << " for " << it.getProp("name") << endl;
	}

	string sbyte(it.getProp(prop_prefix + "nbyte"));
	if( !sbyte.empty() )
	{
		p.nbyte = UniSetTypes::uni_atoi(sbyte.c_str());
		if( p.nbyte < 0 || p.nbyte > VTypes::Byte::bsize )
		{
			dlog[Debug::CRIT] << myname << "(initRSProperty): BAD nbyte=" << p.nbyte 
				<< ". (0 >= nbyte < " << VTypes::Byte::bsize << ")." << endl;
			return false;
		}
	}
	
	string vt(it.getProp(prop_prefix + "vtype"));
	if( vt.empty() )
	{
		p.rnum = VTypes::wsize(VTypes::vtUnknown);
		p.vType = VTypes::vtUnknown;
	}
	else
	{
		VTypes::VType v(VTypes::str2type(vt));
		if( v == VTypes::vtUnknown )
		{
			dlog[Debug::CRIT] << myname << "(initRSProperty): Unknown tcp_vtype='" << vt << "' for " 
					<< it.getProp("name") 
					<< endl;

			return false;
		}

		p.vType = v;
		p.rnum = VTypes::wsize(v);
	}

	return true;
}
// ------------------------------------------------------------------------------------------
bool MBExchange::initRegInfo( RegInfo* r, UniXML_iterator& it,  MBExchange::RTUDevice* dev )
{
	r->dev = dev;
	r->mbval = IOBase::initIntProp(it,"default",prefix,false);
	
	if( dev->dtype == MBExchange::dtRTU )
	{
//		dlog[Debug::INFO] << myname << "(initRegInfo): init RTU.." 
	}
	else if( dev->dtype == MBExchange::dtMTR )
	{
		// only for MTR
		if( !initMTRitem(it,r) )
			return false;
	}
	else if( dev->dtype == MBExchange::dtRTU188 )
	{
		// only for RTU188
 		if( !initRTU188item(it,r) )
			return false;

		UniversalIO::IOTypes t = UniSetTypes::getIOType( IOBase::initProp(it,"iotype",prefix,false) );
		r->mbreg = RTUStorage::getRegister(r->rtuJack,r->rtuChan,t);
		r->mbfunc = RTUStorage::getFunction(r->rtuJack,r->rtuChan,t);

		// т.к. с RTU188 свой обмен
		// mbreg и mbfunc поля не используются	
		return true;

	}
	else
	{
		dlog[Debug::CRIT] << myname << "(initRegInfo): Unknown mbtype='" << dev->dtype
				<< "' for " << it.getProp("name") << endl;
		return false;
	}
	
	if( mbregFromID )
	{
		// всё-таки несмотря на "mbregFromID" разрешим переопределить регистр..
		string sr = IOBase::initProp(it,"id",prop_prefix,false);
		if( sr.empty() )
		{
			r->mbreg = conf->getSensorID(it.getProp("name"));
			if( r->mbreg == DefaultObjectId )
			{
				dlog[Debug::CRIT] << myname << "(initItem): Unknown 'id' for " << it.getProp("name") << endl;
				return false;
			}
		}
		r->mbreg = ModbusRTU::str2mbData(sr);
	}
	else
	{
		string sr = IOBase::initProp(it,"mbreg",prop_prefix,false);
		if( sr.empty() )
		{
			dlog[Debug::CRIT] << myname << "(initItem): Unknown 'mbreg' for " << it.getProp("name") << endl;
			return false;
		}
		r->mbreg = ModbusRTU::str2mbData(sr);
	}

	r->mbfunc 	= ModbusRTU::fnUnknown;
	string f = IOBase::initProp(it,"mbfunc",prop_prefix,false);
	if( !f.empty() )
	{
		r->mbfunc = (ModbusRTU::SlaveFunctionCode)UniSetTypes::uni_atoi(f.c_str());
		if( r->mbfunc == ModbusRTU::fnUnknown )
		{
			dlog[Debug::CRIT] << myname << "(initRegInfo): Unknown mbfunc ='" << f
					<< "' for " << it.getProp("name") << endl;
			return false;
		}
	}

	return true;
}
// ------------------------------------------------------------------------------------------
bool MBExchange::initRTUDevice( RTUDevice* d, UniXML_iterator& it )
{
	std::string mbtype(IOBase::initProp(it,"mbtype",prop_prefix,false));
	d->dtype = getDeviceType(mbtype);

	if( d->dtype == dtUnknown )
	{
		dlog[Debug::CRIT] << myname << "(initRTUDevice): Unknown tcp_mbtype='" << mbtype << "'"
			<< ". Use: rtu " 
			<< " for " << it.getProp("name") << endl;
		return false;
	}

	string addr = IOBase::initProp(it,"mbaddr",prop_prefix,false);
	if( addr.empty() )
	{
		dlog[Debug::CRIT] << myname << "(initRTUDevice): Unknown mbaddr for " << it.getProp("name") << endl;
		return false;
	}

	d->mbaddr = ModbusRTU::str2mbAddr(addr);

	if( d->dtype == MBExchange::dtRTU188 )
	{
		if( !d->rtu )
			d->rtu = new RTUStorage(d->mbaddr);
	}

	return true;
}
// ------------------------------------------------------------------------------------------
bool MBExchange::initItem( UniXML_iterator& it )
{
	RSProperty p;
	if( !initRSProperty(p,it) )
		return false;

	string addr = IOBase::initProp(it,"mbaddr",prop_prefix,false);
	if( addr.empty() )
	{
		dlog[Debug::CRIT] << myname << "(initItem): Unknown mbaddr(" << IOBase::initProp(it,"mbaddr",prop_prefix,false) << ")='" << addr << "' for " << it.getProp("name") << endl;
		return false;
	}

	ModbusRTU::ModbusAddr mbaddr = ModbusRTU::str2mbAddr(addr);

	RTUDevice* dev = addDev(rmap,mbaddr,it);
	if( !dev )
	{
		dlog[Debug::CRIT] << myname << "(initItem): " << it.getProp("name") << " CAN`T ADD for polling!" << endl;
		return false;
	}

	ModbusRTU::ModbusData mbreg = 0;
	int fn = IOBase::initIntProp(it,"mbfunc",prop_prefix,false);

	if( dev->dtype == dtRTU188 )
	{
		RegInfo r_tmp;
		if( !initRTU188item(it, &r_tmp) )
		{
			dlog[Debug::CRIT] << myname << "(initItem): init RTU188 failed for " << it.getProp("name") << endl;
			return false;
		}

		mbreg = RTUStorage::getRegister(r_tmp.rtuJack,r_tmp.rtuChan,p.stype);
		fn = RTUStorage::getFunction(r_tmp.rtuJack,r_tmp.rtuChan,p.stype);
	}
	else
	{
		if( mbregFromID )
			mbreg = p.si.id; // conf->getSensorID(it.getProp("name"));
		else
		{
			string reg = IOBase::initProp(it,"mbreg",prop_prefix,false);
			if( reg.empty() )
			{	
				dlog[Debug::CRIT] << myname << "(initItem): unknown mbreg(" << IOBase::initProp(it,"mbreg",prop_prefix,false) << ") for " << it.getProp("name") << endl;
				return false;
			}
			mbreg = ModbusRTU::str2mbData(reg);
		}

		if( p.nbit != -1 )
		{
			if( fn == ModbusRTU::fnReadCoilStatus || fn == ModbusRTU::fnReadInputStatus )
			{
				dlog[Debug::CRIT] << myname << "(initItem): MISMATCHED CONFIGURATION!  nbit=" << p.nbit << " func=" << fn
									<< " for " << it.getProp("name") << endl;
				return false;
			}
		}
	}

	// формула для вычисления ID
	// требования:
	//  - ID > диапазона возможных регитров
	//  - разные функции должны давать разный ID
	RegID rID = genRegID(mbreg,fn);

	RegInfo* ri = addReg(dev->regmap,rID,mbreg,it,dev);
	
	if( dev->dtype == dtMTR )
	{
		p.rnum = MTR::wsize(ri->mtrType);
		if( p.rnum <= 0 )
		{
			dlog[Debug::CRIT] << myname << "(initItem): unknown word size for " << it.getProp("name") << endl;
			return false;
		}
	}
	
	if( !ri )
		return false;

	ri->dev = dev;

	// ПРОВЕРКА!
    // если функция на запись, то надо проверить
    // что один и тотже регистр не перезапишут несколько датчиков
    // это возможно только, если они пишут биты!!
    // ИТОГ:
    // Если для функций записи список датчиков для регистра > 1
    // значит в списке могут быть только битовые датчики
    // и если идёт попытка внести в список не битовый датчик то ОШИБКА!
    // И наоборот: если идёт попытка внести битовый датчик, а в списке
    // уже сидит датчик занимающий целый регистр, то тоже ОШИБКА!
	if(	ModbusRTU::isWriteFunction(ri->mbfunc) )
	{
		if( p.nbit<0 &&  ri->slst.size() > 1 )
		{
			dlog[Debug::CRIT] << myname << "(initItem): FAILED! Sharing SAVE (not bit saving) to "
				<< " tcp_mbreg=" << ModbusRTU::dat2str(ri->mbreg)
				<< " for " << it.getProp("name") << endl;

			abort(); 	// ABORT PROGRAM!!!!
			return false;
		}
		
		if( p.nbit >= 0 && ri->slst.size() == 1 )
		{
			PList::iterator it2 = ri->slst.begin();
			if( it2->nbit < 0 )
			{
					dlog[Debug::CRIT] << myname << "(initItem): FAILED! Sharing SAVE (mbreg=" 
						<< ModbusRTU::dat2str(ri->mbreg) << "  already used)!"
						<< " IGNORE --> " << it.getProp("name") << endl;
					abort(); 	// ABORT PROGRAM!!!!
					return false;
			}
		}

		// Раз это регистр для записи, то как минимум надо сперва
		// инициализировать значением из SM
		ri->sm_initOK = IOBase::initIntProp(it,"sm_initOK",prop_prefix,false);
		ri->mb_initOK = true;  // может быть переопределён если будет указан tcp_preinit="1" (см. ниже)
	}
	else
	{
		ri->mb_initOK = defaultMBinitOK;
		ri->sm_initOK = false;
	}
	

	RSProperty* p1 = addProp(ri->slst,p);
	if( !p1 )
		return false;

	p1->reg = ri;
	
	
	if( p1->rnum > 1 )
	{
		ri->q_count = p1->rnum;
		ri->q_num = 1;
		for( int i=1; i<p1->rnum; i++ )
		{
			RegID id1 = genRegID(mbreg+i,ri->mbfunc);
			RegInfo* r = addReg(dev->regmap,id1,mbreg+i,it,dev,ri);
			r->q_num=i+1;
			r->q_count=1;
			r->mbfunc = ri->mbfunc;
			r->mb_initOK = defaultMBinitOK;
			r->sm_initOK = false;

			if( ModbusRTU::isWriteFunction(ri->mbfunc) )
			{
				// Если занимает несколько регистров, а указана функция записи "одного",
				// то это ошибка..
				if( ri->mbfunc != ModbusRTU::fnWriteOutputRegisters && 
					ri->mbfunc != ModbusRTU::fnForceMultipleCoils )
				{
					dlog[Debug::CRIT] << myname << "(initItem): Bad write function ='" << ModbusRTU::fnWriteOutputSingleRegister
						<< "' for vtype='" << p1->vType << "'"
						<< " tcp_mbreg=" << ModbusRTU::dat2str(ri->mbreg)
						<< " for " << it.getProp("name") << endl;

					abort(); 	// ABORT PROGRAM!!!!
					return false;				  
				}
			}
		}
	}

	// Формируем список инициализации
	bool need_init = IOBase::initIntProp(it,"preinit",prop_prefix,false);
	if( need_init && ModbusRTU::isWriteFunction(ri->mbfunc) )
	{
		InitRegInfo	ii;
		ii.p = p;
		ii.dev = dev;
		ii.ri = ri;

		string s_reg( IOBase::initProp(it,"init_mbreg",prop_prefix,false) );
		if( !s_reg.empty() )
			ii.mbreg = ModbusRTU::str2mbData(s_reg);
		else
			ii.mbreg = ri->mbreg;

		string s_mbfunc(  IOBase::initProp(it,"init_mbfunc",prop_prefix,false) );
		if( !s_mbfunc.empty() )
		{
			ii.mbfunc = (ModbusRTU::SlaveFunctionCode)UniSetTypes::uni_atoi(s_mbfunc);
			if( ii.mbfunc == ModbusRTU::fnUnknown )
			{
				dlog[Debug::CRIT] << myname << "(initItem): Unknown tcp_init_mbfunc ='" << s_mbfunc
						<< "' for " << it.getProp("name") << endl;
				return false;
			}
		}
		else
		{
			switch(ri->mbfunc)
			{
				case ModbusRTU::fnWriteOutputSingleRegister:
					ii.mbfunc = ModbusRTU::fnReadOutputRegisters;
				break;

				case ModbusRTU::fnForceSingleCoil:
					ii.mbfunc = ModbusRTU::fnReadCoilStatus;
				break;
				
				case ModbusRTU::fnWriteOutputRegisters:
					ii.mbfunc = ModbusRTU::fnReadOutputRegisters;
				break;

				case ModbusRTU::fnForceMultipleCoils:
					ii.mbfunc = ModbusRTU::fnReadCoilStatus;
				break;

				default:
					ii.mbfunc = ModbusRTU::fnReadOutputRegisters;	  
				break;
			}
		}

		initRegList.push_back(ii);
		ri->mb_initOK = false;
		ri->sm_initOK = false;
	}
	
	return true;
}
// ------------------------------------------------------------------------------------------
bool MBExchange::initMTRitem( UniXML_iterator& it, RegInfo* p )
{
	p->mtrType = MTR::str2type(it.getProp(prop_prefix + "mtrtype"));
	if( p->mtrType == MTR::mtUnknown )
	{
		dlog[Debug::CRIT] << myname << "(readMTRItem): Unknown mtrtype '" 
					<< it.getProp(prop_prefix + "mtrtype")
					<< "' for " << it.getProp("name") << endl;

		return false;
	}

	return true;
}
// ------------------------------------------------------------------------------------------
bool MBExchange::initRTU188item( UniXML_iterator& it, RegInfo* p )
{
	string jack( IOBase::initProp(it,"jack",prop_prefix,false) );
	string chan( IOBase::initProp(it,"channel",prop_prefix,false) );
	
	if( jack.empty() )
	{
		dlog[Debug::CRIT] << myname << "(readRTU188Item): Unknown " << prop_prefix << "jack='' "
					<< " for " << it.getProp("name") << endl;
		return false;
	}
	p->rtuJack = RTUStorage::s2j(jack);
	if( p->rtuJack == RTUStorage::nUnknown )
	{
		dlog[Debug::CRIT] << myname << "(readRTU188Item): Unknown " << prop_prefix << "jack=" << jack
					<< " for " << it.getProp("name") << endl;
		return false;
	}

	if( chan.empty() )
	{
		dlog[Debug::CRIT] << myname << "(readRTU188Item): Unknown channel='' "
					<< " for " << it.getProp("name") << endl;
		return false;
	}
	
	p->rtuChan = UniSetTypes::uni_atoi(chan);

	if( dlog.debugging(Debug::LEVEL2) )
		dlog[Debug::LEVEL2] << myname << "(readRTU188Item): add jack='" << jack << "'" 
			<< " channel='" << p->rtuChan << "'" << endl; 

	return true;
}
// ------------------------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, const MBExchange::DeviceType& dt )
{
	switch(dt)
	{
		case MBExchange::dtRTU:
			os << "RTU";
		break;
		
		case MBExchange::dtMTR:
			os << "MTR";
		break;

		case MBExchange::dtRTU188:
			os << "RTU188";
		break;
		
		default:
			os << "Unknown device type (" << (int)dt << ")";
		break;
	}
	
	return os;
}
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, const MBExchange::RSProperty& p )
{
	os 	<< " (" << ModbusRTU::dat2str(p.reg->mbreg) << ")"
		<< " sid=" << p.si.id
		<< " stype=" << p.stype
	 	<< " nbit=" << p.nbit
	 	<< " nbyte=" << p.nbyte
		<< " rnum=" << p.rnum
		<< " safety=" << p.safety
		<< " invert=" << p.invert;

	if( p.stype == UniversalIO::AnalogInput || p.stype == UniversalIO::AnalogOutput )
	{
		os 	<< p.cal
			<< " cdiagram=" << ( p.cdiagram ? "yes" : "no" );
	}		
	
	return os;
}
// -----------------------------------------------------------------------------
void MBExchange::initDeviceList()
{
	xmlNode* respNode = 0;
	UniXML* xml = conf->getConfXML();
	if( xml )
		respNode = xml->extFindNode(cnode,1,1,"DeviceList");

	if( respNode )
	{
		UniXML_iterator it1(respNode);
		if( it1.goChildren() )
		{
			for(;it1.getCurrent(); it1.goNext() )
			{
				ModbusRTU::ModbusAddr a = ModbusRTU::str2mbAddr(it1.getProp("addr"));
				initDeviceInfo(rmap,a,it1);
			}
		}
		else
			dlog[Debug::WARN] << myname << "(init): <DeviceList> empty section..." << endl;
	}
	else
		dlog[Debug::WARN] << myname << "(init): <DeviceList> not found..." << endl;
}
// -----------------------------------------------------------------------------
bool MBExchange::initDeviceInfo( RTUDeviceMap& m, ModbusRTU::ModbusAddr a, UniXML_iterator& it )
{
	RTUDeviceMap::iterator d = m.find(a);
	if( d == m.end() )
	{
		dlog[Debug::WARN] << myname << "(initDeviceInfo): not found device for addr=" << ModbusRTU::addr2str(a) << endl;
		return false;
	}

	d->second->ask_every_reg = it.getIntProp("ask_every_reg");

	dlog[Debug::INFO] << myname << "(initDeviceInfo): add addr=" << ModbusRTU::addr2str(a) 
			<< " ask_every_reg=" << d->second->ask_every_reg << endl;

	string s(it.getProp("respondSensor"));
	if( !s.empty() )
	{
		d->second->resp_id = conf->getSensorID(s);
		if( d->second->resp_id == DefaultObjectId )
		{
			dlog[Debug::CRIT] << myname << "(initDeviceInfo): not found ID for respondSensor=" << s << endl;
			return false;
		}
    }

	string mod(it.getProp("modeSensor"));
	if( !mod.empty() )
	{
		d->second->mode_id = conf->getSensorID(mod);
		if( d->second->mode_id == DefaultObjectId )
		{
			dlog[Debug::CRIT] << myname << "(initDeviceInfo): not found ID for modeSensor=" << mod << endl;
			return false;
		}

		UniversalIO::IOTypes m_iotype = conf->getIOType(d->second->mode_id);
		if( m_iotype != UniversalIO::AnalogInput )
		{
			dlog[Debug::CRIT] << myname << "(initDeviceInfo): modeSensor='" << mod << "' must be 'AI'" << endl;
			return false;
		}
    }
 
	dlog[Debug::INFO] << myname << "(initDeviceInfo): add addr=" << ModbusRTU::addr2str(a) << endl;
	int tout = it.getPIntProp("timeout",ptTimeout.getInterval());
	d->second->resp_Delay.set(tout,0);
	d->second->resp_invert = it.getIntProp("invert");
	return true;
}
// -----------------------------------------------------------------------------
bool MBExchange::activateObject()
{
	// блокирование обработки Starsp 
	// пока не пройдёт инициализация датчиков
	// см. sysCommand()
	{
		setProcActive(false);
		UniSetTypes::uniset_mutex_lock l(mutex_start, 5000);
		UniSetObject_LT::activateObject();
		if( !shm->isLocalwork() )
			rtuQueryOptimization(rmap);
		initIterators();
		initValues();
		setProcActive(true);
	}

	return true;
}
// ------------------------------------------------------------------------------------------
void MBExchange::processingMessage(UniSetTypes::VoidMessage *msg)
{
	try
	{
		switch(msg->type)
		{
			case UniSetTypes::Message::SysCommand:
			{
				UniSetTypes::SystemMessage sm( msg );
				sysCommand( &sm );
			}
			break;

			case Message::Timer:
			{
				TimerMessage tm(msg);
				timerInfo(&tm);
			}
			break;
		
			case Message::SensorInfo:
			{
				SensorMessage sm( msg );
				sensorInfo(&sm);
			}
			break;

			default:
				break;
		}
	}
	catch( SystemError& ex )
	{
		dlog[Debug::CRIT] << myname << "(SystemError): " << ex << std::endl;
//		throw SystemError(ex);
	}
	catch( Exception& ex )
	{
		dlog[Debug::CRIT] << myname << "(processingMessage): " << ex << std::endl;
	}
	catch(...)
	{
		dlog[Debug::CRIT] << myname << "(processingMessage): catch ...\n";
	}
}
// -----------------------------------------------------------------------------
void MBExchange::sysCommand( UniSetTypes::SystemMessage *sm )
{
	switch( sm->command )
	{
		case SystemMessage::StartUp:
		{
			if( rmap.empty() )
			{
				dlog[Debug::CRIT] << myname << "(sysCommand): ************* ITEM MAP EMPTY! terminated... *************" << endl;
				raise(SIGTERM);
				return; 
			}

			if( dlog.debugging(Debug::INFO) )
				dlog[Debug::INFO] << myname << "(sysCommand): device map size= " << rmap.size() << endl;

			if( !shm->isLocalwork() )
				initDeviceList();
		
			waitSMReady();

			// подождать пока пройдёт инициализация датчиков
			// см. activateObject()
			msleep(initPause);
			PassiveTimer ptAct(activateTimeout);
			while( !activated && !ptAct.checkTime() )
			{	
				cout << myname << "(sysCommand): wait activate..." << endl;
				msleep(300);
				if( activated )
					break;
			}
			
			if( !activated )
				dlog[Debug::CRIT] << myname << "(sysCommand): ************* don`t activate?! ************" << endl;
			
			{
				UniSetTypes::uniset_mutex_lock l(mutex_start, 10000);
				askSensors(UniversalIO::UIONotify);
				initOutput();
			}

			initValues();
			updateSM();
			askTimer(tmExchange,polltime);
			break;
		}

		case SystemMessage::FoldUp:
		case SystemMessage::Finish:
			askSensors(UniversalIO::UIODontNotify);
			break;
		
		case SystemMessage::WatchDog:
		{
			// ОПТИМИЗАЦИЯ (защита от двойного перезаказа при старте)
			// Если идёт локальная работа 
			// (т.е. MBExchange  запущен в одном процессе с SharedMemory2)
			// то обрабатывать WatchDog не надо, т.к. мы и так ждём готовности SM
			// при заказе датчиков, а если SM вылетит, то вместе с этим процессом(MBExchange)
			if( shm->isLocalwork() )
				break;

			askSensors(UniversalIO::UIONotify);
			initOutput();
			initValues();

			if( !force )
			{
				force = true;
				poll();
				force = false;
			}
		}
		break;

		case SystemMessage::LogRotate:
		{
			// переоткрываем логи
			unideb << myname << "(sysCommand): logRotate" << std::endl;
			string fname = unideb.getLogFile();
			if( !fname.empty() )
			{
				unideb.logFile(fname);
				unideb << myname << "(sysCommand): ***************** UNIDEB LOG ROTATE *****************" << std::endl;
			}
			dlog << myname << "(sysCommand): logRotate" << std::endl;
			fname = dlog.getLogFile();
			if( !fname.empty() )
			{
				dlog.logFile(fname);
				dlog << myname << "(sysCommand): ***************** dlog LOG ROTATE *****************" << std::endl;
			}
		}
		break;

		default:
			break;
	}
}
// ------------------------------------------------------------------------------------------
void MBExchange::initOutput()
{
}
// ------------------------------------------------------------------------------------------
void MBExchange::askSensors( UniversalIO::UIOCommand cmd )
{
	if( !shm->waitSMworking(test_id,activateTimeout,50) )
	{
		ostringstream err;
		err << myname 
			<< "(askSensors): Не дождались готовности(work) SharedMemory к работе в течение " 
			<< activateTimeout << " мсек";
	
		dlog[Debug::CRIT] << err.str() << endl;
		kill(SIGTERM,getpid());	// прерываем (перезапускаем) процесс...
		throw SystemError(err.str());
	}

	try
	{
		if( sidExchangeMode != DefaultObjectId )
			shm->askSensor(sidExchangeMode,cmd);
	}
	catch( UniSetTypes::Exception& ex )
	{
		dlog[Debug::WARN] << myname << "(askSensors): " << ex << std::endl;
	}
	catch(...)
	{
		dlog[Debug::WARN] << myname << "(askSensors): 'sidExchangeMode' catch..." << std::endl;
	}
	
	for( MBExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);

		try
		{
			if( d->mode_id != DefaultObjectId )
				shm->askSensor(d->mode_id,cmd);
		}
		catch( UniSetTypes::Exception& ex )
		{
			dlog[Debug::WARN] << myname << "(askSensors): " << ex << std::endl;
		}
		catch(...)
		{
			dlog[Debug::WARN] << myname << "(askSensors): (mode_id=" << d->mode_id << ").. catch..." << std::endl;
		}

		if( force_out )
			return;

		for( MBExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			if( !isWriteFunction(it->second->mbfunc) )
				continue;
		
			for( PList::iterator i=it->second->slst.begin(); i!=it->second->slst.end(); ++i )
			{
				try
				{
					shm->askSensor(i->si.id,cmd);
				}
				catch( UniSetTypes::Exception& ex )
				{
					dlog[Debug::WARN] << myname << "(askSensors): " << ex << std::endl;
				}
				catch(...)
				{
					dlog[Debug::WARN] << myname << "(askSensors): id=" << i->si.id << " catch..." << std::endl;
				}
			}
		}
	}
}
// ------------------------------------------------------------------------------------------
void MBExchange::sensorInfo( UniSetTypes::SensorMessage* sm )
{
	if( sm->id == sidExchangeMode )
	{
		exchangeMode = sm->value;
		if( dlog.debugging(Debug::LEVEL3) )
			dlog[Debug::LEVEL3] << myname << "(sensorInfo): exchange MODE=" << sm->value << std::endl;
		//return; // этот датчик может встречаться и в списке обмена.. поэтому делать return нельзя.
	}

	for( MBExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);

		if( sm->id == d->mode_id )
			d->mode = sm->value;

		if( force_out )
			continue;

		for( MBExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			if( !isWriteFunction(it->second->mbfunc) )
				continue;
		
			for( PList::iterator i=it->second->slst.begin(); i!=it->second->slst.end(); ++i )
			{
				if( sm->id == i->si.id && sm->node == i->si.node )
				{
					if( dlog.debugging(Debug::INFO) )
					{
						dlog[Debug::INFO] << myname<< "(sensorInfo): si.id=" << sm->id 
							<< " reg=" << ModbusRTU::dat2str(i->reg->mbreg)
							<< " val=" << sm->value 
							<< " mb_initOK=" << i->reg->mb_initOK << endl;
					}

					if( !i->reg->mb_initOK )
						continue;

					i->value = sm->value;
					updateRSProperty( &(*i),true );
					return;
				}
			}
		}
	}
}
// ------------------------------------------------------------------------------------------
void MBExchange::timerInfo( TimerMessage *tm )
{
	if( tm->id == tmExchange )
	{
		if( no_extimer )
			askTimer(tm->id,0);
		else
			step();
	}
}
// -----------------------------------------------------------------------------
void MBExchange::poll()
{
	if( !mb )
	{
		{
			uniset_mutex_lock l(pollMutex, 300);
			pollActivated = false;
			mb = initMB(false);
		}

		if( !checkProcActive() )
			return;

		updateSM();
		allInitOK = false;
		return;
	}

	{
		uniset_mutex_lock l(pollMutex,200);
		pollActivated = true;
		ptTimeout.reset();
	}

	if( !allInitOK )
		firstInitRegisters();

	if( !checkProcActive() )
		return;

	bool allNotRespond = true;

	for( MBExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);

		if( d->mode_id != DefaultObjectId && d->mode == emSkipExchange )
			continue;

		if( dlog.debugging(Debug::LEVEL3) )
			dlog[Debug::LEVEL3] << myname << "(poll): ask addr=" << ModbusRTU::addr2str(d->mbaddr) 
				<< " regs=" << d->regmap.size() << endl;

		d->prev_numreply = d->numreply;

		for( MBExchange::RegMap::iterator it=d->regmap.begin(); it!=d->regmap.end(); ++it )
		{
			if( !checkProcActive() )
				return;
		  
			try
			{
				if( d->dtype==MBExchange::dtRTU || d->dtype==MBExchange::dtMTR )
				{
					if( pollRTU(d,it) )
					{
						d->numreply +=1;
						allNotRespond = false;
					}
				}
			}
			catch( ModbusRTU::mbException& ex )
			{ 
//				if( d->resp_real )
//				{
					if( dlog.debugging(Debug::LEVEL3) )
					{
						dlog[Debug::LEVEL3] << myname << "(poll): FAILED ask addr=" << ModbusRTU::addr2str(d->mbaddr)
							<< " reg=" << ModbusRTU::dat2str(it->second->mbreg)
							<< " for sensors: "; print_plist(dlog(Debug::LEVEL3),it->second->slst);
							dlog(Debug::LEVEL3) << " err: " << ex << endl;
					}
//				}

				// d->resp_real = false;
				if( ex.err == ModbusRTU::erTimeOut && !d->ask_every_reg )
					break;
			}

			if( it==d->regmap.end() )
				break;

			if( !checkProcActive() )
				return;
		}

		if( stat_time > 0 )
			poll_count++;
	}


	if( stat_time > 0 && ptStatistic.checkTime() )
	{
		cout << endl << "(poll statistic): number of calls is " << poll_count << " (poll time: " << stat_time << " sec)" << endl << endl;
		ptStatistic.reset();
		poll_count=0;
	}



	{
		uniset_mutex_lock l(pollMutex,120);
		pollActivated = false;
	}

	if( !checkProcActive() )
		return;

	// update SharedMemory...
	updateSM();
	
	// check thresholds
	for( ThresholdList::iterator t=thrlist.begin(); t!=thrlist.end(); ++t )
	{
		if( !checkProcActive() )
			return;
        
		IOBase::processingThreshold(&(*t),shm,force);
	}
	
	if( trReopen.hi(allNotRespond) )
		 ptReopen.reset();

	if( allNotRespond && ptReopen.checkTime() )
	{
		uniset_mutex_lock l(pollMutex, 300);
		if( dlog.debugging(Debug::WARN) )
			dlog[Debug::WARN] << myname << ": REOPEN timeout..(" << ptReopen.getInterval() << ")" << endl;

		mb = initMB(true);
		ptReopen.reset();
	}

//	printMap(rmap);
}
// -----------------------------------------------------------------------------
bool MBExchange::RTUDevice::checkRespond()
{
	bool prev = resp_state;
	resp_state = !resp_Delay.check( prev_numreply==numreply );

	if( dlog.debugging(Debug::LEVEL4) )
	{
		dlog[Debug::LEVEL4] << "(checkRespond): addr=" << ModbusRTU::addr2str(mbaddr)
			<< " respond_id=" << resp_id
			<< " state=" << resp_state
			<< " [timeout=" << resp_Delay.getOnDelay()
			<< " numreply=" << numreply
			<< " prev_numreply=" << prev_numreply
			<< " ]"
			<< endl;
	}

	return (prev!=resp_state);
}
// -----------------------------------------------------------------------------
void MBExchange::updateRespondSensors()
{
	bool chanTimeout = false;
	{
		uniset_mutex_lock l(pollMutex,240);
		chanTimeout = pollActivated && ptTimeout.checkTime();
	}

	for( MBExchange::RTUDeviceMap::iterator it1=rmap.begin(); it1!=rmap.end(); ++it1 )
	{
		RTUDevice* d(it1->second);
		
		if( d->resp_id != DefaultObjectId && d->checkRespond() )
		{
			try
			{
				bool set = d->resp_invert ? !d->resp_state : d->resp_state;
		
				if( dlog.debugging(Debug::LEVEL4) )
					dlog[Debug::LEVEL4] << myname << ": SAVE new respond state addr=" << ModbusRTU::addr2str(d->mbaddr)
					<< " set=" << set
					<< " [ invert=" << d->resp_invert
					<< "  resp_state=" << d->resp_state 
					<< " timeout=" << d->resp_Delay.getOnDelay()
					<< endl;

				shm->localSaveState(d->resp_dit,d->resp_id,set,getId());
			}
			catch( Exception& ex )
			{
				dlog[Debug::CRIT] << myname << "(step): (respond) " << ex << std::endl;
			}
		}
	}
}
// -----------------------------------------------------------------------------
void MBExchange::execute()
{
	no_extimer = true;

	try
	{
		askTimer(tmExchange,0);
	}
	catch(...){}
	
	initMB(false);

	while(1)
	{
		try
		{
			step();
		}
		catch( Exception& ex )
		{
			dlog[Debug::CRIT] << myname << "(execute): " << ex << std::endl;
		}
		catch(...)
		{
			dlog[Debug::CRIT] << myname << "(execute): catch ..." << endl;
		}

		msleep(polltime);
	}
}
// -----------------------------------------------------------------------------
