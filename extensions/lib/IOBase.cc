// $Id: IOBase.cc,v 1.3 2009/01/23 23:56:54 vpashka Exp $
// -----------------------------------------------------------------------------
#include "Configuration.h"
#include "Extensions.h"
#include "IOBase.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// -----------------------------------------------------------------------------
std::ostream& operator<<( std::ostream& os, IOBase& inf )
{
	return os << "(" << inf.si.id << ")" << conf->oind->getMapName(inf.si.id)
		<< " default=" << inf.defval << " safety=" << inf.safety;
}
// -----------------------------------------------------------------------------
bool IOBase::check_channel_break( long val )
{
	// ����� �� �����... (�������� ���������)
	if( breaklim <=0 )
		return false;
		
	return ( val < breaklim );
}
// -----------------------------------------------------------------------------
bool IOBase::check_jar( bool val )
{
	// ��� ������ �� ��������
	if( ptJar.getInterval() <= 0 )
	{
		jar_state = val;
		return val;
	}

	if( trJar.change(val) )
	{
		if( !jar_pause )
		{	
			// �������� �����...
			jar_pause = true;
			ptJar.reset();
		}
	}
	
	if( jar_pause && ptJar.checkTime() )
	{
		// ����� �� ������� ���������
		// ��������� ��������
		jar_state = val;
		jar_pause = false;
	}

	// ���������� �������, � �� ����� ��������
	return jar_state;
}
// -----------------------------------------------------------------------------
bool IOBase::check_on_delay( bool val )
{
	// ��� �������� �� ���������
	if( ptOnDelay.getInterval() <= 0 )
	{
		ondelay_state = val;
		return val;
	}

	if( trOnDelay.hi(val) )
		ptOnDelay.reset();

	// ��������� �������� ������ ���� ��������� �����
	// ��� ���� ��� "0"...
	if( !val || (val && ptOnDelay.checkTime()) )
		ondelay_state = val;

	// ���������� �������, � �� ����� ��������
	return ondelay_state;
}
// -----------------------------------------------------------------------------
bool IOBase::check_off_delay( bool val )
{
	if( ptOffDelay.getInterval() <= 0 )
	{
		offdelay_state = val;
		return val;
	}

	if( trOffDelay.low(val) )
		ptOffDelay.reset();

	// ��������� �������� ������ ���� ��������� �����
	// ��� ���� ��� "1"...
	if( val || (!val && ptOffDelay.checkTime()) )
		offdelay_state = val;

	// ���������� �������, � �� ����� ��������
	return offdelay_state;
}
// -----------------------------------------------------------------------------
void IOBase::processingAsAI( IOBase* it, long val, SMInterface* shm, bool force )
{
	// �������� �� �����
	if( it->check_channel_break(val) )
	{
		uniset_spin_lock lock(it->val_lock);
		it->value = ChannelBreakValue;
		shm->localSetUndefinedState(it->ait,true,it->si.id);
		return;
	}

	if( it->cdiagram )	// ������ ����������� ������������� ���������
	{
		// ���� ���� ���������, �� ����������� ���������� 
		// ������, �� ���������
		if( !it->nofilter && it->df.size() > 1 )
		{
			if( it->f_median )
				val = it->df.median(val);
			else
				val = it->df.filterRC(val);
		}

		if( it->craw != val )
		{	
			it->craw = val;
			val = it->cdiagram->getValue(val);
			it->cprev = val;
		}
		else
			val = it->cprev;	// ������ �������� ���������� ��������
	}
	else
	{
		IOController_i::CalibrateInfo* cal( &(it->cal) );
		if( cal->maxRaw!=0 && cal->maxRaw!=cal->minRaw ) // ������ ������� ����������
			val = UniSetTypes::lcalibrate(val,cal->minRaw,cal->maxRaw,cal->minCal,cal->maxCal,true);
		
		// ���� �� ���������, �� ��������� 
		// ������, ����� ���������
		if( !it->nofilter && it->df.size() > 1 )
		{
			if( it->f_median )
				val = it->df.median(val);
			else
				val = it->df.filterRC(val);
		}
	}

	if( !it->noprecision && it->cal.precision > 0 )
		val *= lround(pow10(it->cal.precision));


	// ���� ���������� �������� "�����",
	// �� ���������� ������� 
	{
		uniset_spin_lock lock(it->val_lock);
		if( it->value == ChannelBreakValue )
			shm->localSetUndefinedState(it->ait,false,it->si.id);

		if( force || it->value != val )
		{
			if( it->stype == UniversalIO::AnalogInput )
				shm->localSaveValue( it->ait,it->si.id,val,shm->ID() );
			else if( it->stype == UniversalIO::AnalogOutput )
				shm->localSetValue( it->ait,it->si.id,val,shm->ID() );
			else if( it->stype == UniversalIO::DigitalOutput )
				shm->localSetState( it->dit,it->si.id,(bool)val,shm->ID() );
			else if( it->stype == UniversalIO::DigitalInput )
				shm->localSaveState( it->dit,it->si.id,(bool)val,shm->ID() );

			it->value = val;
		}
	}
}
// -----------------------------------------------------------------------------
void IOBase::processingFasAI( IOBase* it, float fval, SMInterface* shm, bool force )
{
	long val = lroundf(fval);

	if( it->cal.precision > 0 )
		val = lroundf( fval * pow10(it->cal.precision) );

	// �������� �� �����
	if( it->check_channel_break(val) )
	{
		uniset_spin_lock lock(it->val_lock);
		it->value = ChannelBreakValue;
		shm->localSetUndefinedState(it->ait,true,it->si.id);
		return;
	}

	// ������ � �������������� �������...
	if( !it->nofilter )
	{
		if( it->df.size() > 1 )
			it->df.add(val);

		val = it->df.filterRC(val);
	}

	IOController_i::CalibrateInfo* cal( &(it->cal) );
	if( cal->maxRaw!=0 && cal->maxRaw!=cal->minRaw ) // ������ ������� ����������
		val = UniSetTypes::lcalibrate(val,cal->minRaw,cal->maxRaw,cal->minCal,cal->maxCal,true);

	// ���� ���������� �������� "�����",
	// �� ���������� ������� 
	{
		uniset_spin_lock lock(it->val_lock);
		if( it->value == ChannelBreakValue )
			shm->localSetUndefinedState(it->ait,false,it->si.id);

		if( force || it->value != val )
		{
			if( it->stype == UniversalIO::AnalogInput )
				shm->localSaveValue( it->ait,it->si.id,val,shm->ID() );
			else if( it->stype == UniversalIO::AnalogOutput )
				shm->localSetValue( it->ait,it->si.id,val,shm->ID() );
			else if( it->stype == UniversalIO::DigitalOutput )
				shm->localSetState( it->dit,it->si.id,(bool)val,shm->ID() );
			else if( it->stype == UniversalIO::DigitalInput )
				shm->localSaveState( it->dit,it->si.id,(bool)val,shm->ID() );

			it->value = val;
		}
	}
}
// -----------------------------------------------------------------------------
void IOBase::processingAsDI( IOBase* it, bool set, SMInterface* shm, bool force )
{
//	cout  << "subdev: " << it->subdev << " chan: " << it->channel << " state=" << set << endl;
	if( it->invert )
		set ^= true;
//	cout  << "subdev: " << it->subdev << " chan: " << it->channel << " (inv)state=" << set << endl;

	// ��������� ������ � ����� ������������������!
	set = it->check_jar(set);		// ������ ��������
//	cout  << "subdev: " << it->subdev << " chan: " << it->channel << " (jar)state=" << set << endl;
	set = it->check_on_delay(set);	// ������ �� ������������
//	cout  << "subdev: " << it->subdev << " chan: " << it->channel << " (on_delay)state=" << set << endl;
	set = it->check_off_delay(set);	// ������ �� ����������
//	cout  << "subdev: " << it->subdev << " chan: " << it->channel << " (off_delay)state=" << set << endl;

	{
		uniset_spin_lock lock(it->val_lock);
		if( force || (bool)it->value!=set )
		{
			if( it->stype == UniversalIO::DigitalInput )
				shm->localSaveState(it->dit,it->si.id,set,shm->ID());
			else if( it->stype == UniversalIO::DigitalOutput )
				shm->localSetState(it->dit,it->si.id,set,shm->ID());
			else if( it->stype == UniversalIO::AnalogInput )
				shm->localSaveValue( it->ait,it->si.id,(set ? 1:0),shm->ID() );
			else if( it->stype == UniversalIO::AnalogOutput )
				shm->localSetValue( it->ait,it->si.id,(set ? 1:0),shm->ID() );
			
			it->value = set ? 1 : 0;
		}
	}
}
// -----------------------------------------------------------------------------
long IOBase::processingAsAO( IOBase* it, SMInterface* shm, bool force )
{
	uniset_spin_lock lock(it->val_lock);
	long val = it->value;
	
	if( force )
	{
		if( it->stype == UniversalIO::DigitalInput || it->stype == UniversalIO::DigitalOutput )
			val = shm->localGetState(it->dit,it->si.id) ? 1 : 0;
		else if( it->stype == UniversalIO::AnalogInput || it->stype == UniversalIO::AnalogOutput )
			val = shm->localGetValue(it->ait,it->si.id);

		it->value = val;
	}

	if( it->stype == UniversalIO::AnalogOutput ||
		it->stype == UniversalIO::AnalogInput )
	{
		if( it->cdiagram )	// ������ ����������� ������������� ���������
		{
			if( it->cprev != it->value )
			{	
				it->cprev = it->value;
				val = it->cdiagram->getRawValue(val);
				it->craw = val;
			}
			else
				val = it->craw; // ������ �������� ���������� ��������
		}
		else
		{
			IOController_i::CalibrateInfo* cal( &(it->cal) );
			if( cal && cal->maxRaw!=0 && cal->maxRaw!=cal->minRaw ) // ������ ����������
			{
				// ��������� � �������� �������!!!
				val = UniSetTypes::lcalibrate(it->value,
							cal->minCal, cal->maxCal, cal->minRaw, cal->maxRaw, true );
			}
			else
				val = it->value;
#warning Precision!!!
//			if( it->cal.precision > 0 )
//				val = it->value / lround(pow10(it->cal.precision));
		}
	}
	
	return val;
}
// -----------------------------------------------------------------------------
bool IOBase::processingAsDO( IOBase* it, SMInterface* shm, bool force )
{
		uniset_spin_lock lock(it->val_lock);
		bool set = it->value;

		if( force )
		{
			if( it->stype == UniversalIO::DigitalInput || it->stype == UniversalIO::DigitalOutput )
				set = shm->localGetState(it->dit,it->si.id);
			else if( it->stype == UniversalIO::AnalogInput || it->stype == UniversalIO::AnalogOutput )
				set = shm->localGetValue(it->ait,it->si.id) ? true : false;
		}
		
		set = it->invert ? !set : set;
		return set; 
}
// -----------------------------------------------------------------------------
float IOBase::processingFasAO( IOBase* it, SMInterface* shm, bool force )
{
	uniset_spin_lock lock(it->val_lock);

	long val = it->value;
	
	if( force )
	{
		val = shm->localGetValue(it->ait,it->si.id);
		it->value = val;
	}

	if( it->stype == UniversalIO::AnalogOutput ||
		it->stype == UniversalIO::AnalogInput )
	{
		if( it->cdiagram )	// ������ ����������� ������������� ���������
		{
			if( it->cprev != it->value )
			{	
				it->cprev = it->value;
				val = it->cdiagram->getRawValue(val);
				it->craw = val;
			}
			else
				val = it->craw; // ������ �������� ���������� ��������
		}
		else
		{
			float fval = val;
			IOController_i::CalibrateInfo* cal( &(it->cal) );
			if( cal->maxRaw!=0 && cal->maxRaw!=cal->minRaw ) // ������ ����������
			{
				// ��������� � �������� �������!!!
				fval = UniSetTypes::fcalibrate(fval,
							cal->minCal, cal->maxCal, cal->minRaw, cal->maxRaw, true );
			}

			if( it->cal.precision > 0 )
				return ( fval / pow10(it->cal.precision) );
		}
	}
	
	return val;
}
// -----------------------------------------------------------------------------
void IOBase::processingThreshold( IOBase* it, SMInterface* shm, bool force )
{
	if( it->t_ai == DefaultObjectId )
		return;
	
	long val = shm->localGetValue(it->ait,it->t_ai);
	bool set = it->value ? true : false;

//	cout  << "val=" << val << " set=" << set << endl;
	// �������� ������� �������
	// �������� ������ ���� ������ lowLimit-���������������
	if( val <= (it->ti.lowlimit-it->ti.sensibility) )
		set = false;
	else if( val >= (it->ti.hilimit+it->ti.sensibility) )
		set = true;

//	cout  << "thresh: set=" << set << endl;
	processingAsDI(it,set,shm,force);
}
// -----------------------------------------------------------------------------

bool IOBase::initItem( IOBase* b, UniXML_iterator& it, SMInterface* shm,  
						DebugStream* dlog, std::string myname,
						int def_filtersize, float def_filterT )
{
	string sname( it.getProp("name") );

	ObjectId sid = DefaultObjectId;
	if( it.getProp("id").empty() )
		sid = conf->getSensorID(sname);
	else
	{
		sid = UniSetTypes::uni_atoi(it.getProp("id").c_str());
		if( sid <=0 )
			sid = DefaultObjectId;
	}
	
	if( sid == DefaultObjectId )
	{
		if( dlog )
			dlog[Debug::CRIT] << myname << "(readItem): (-1) �� ������� �������� ID ��� �������: "
						<< sname << endl;
		return false;
	}

	b->si.id 	= sid;
	b->si.node 	= conf->getLocalNode();

	b->nofilter = atoi( it.getProp("nofilter").c_str() );
	b->ignore	= atoi( it.getProp("ioignore").c_str() );
	b->invert	= atoi( it.getProp("ioinvert").c_str() );
	b->defval 	= atoi( it.getProp("default").c_str() );
	b->noprecision	= atoi( it.getProp("noprecision").c_str() );
	b->value	= b->defval;
	b->breaklim = atoi( it.getProp("breaklim").c_str() );
	
	long msec = atoi( it.getProp("jardelay").c_str() );
	b->ptJar.setTiming(msec);
	if( msec<=0 )
		b->ptJar.setTiming(UniSetTimer::WaitUpTime);

	msec = atoi( it.getProp("ondelay").c_str() );
	b->ptOnDelay.setTiming(msec);
	if( msec<=0 )
		b->ptOnDelay.setTiming(UniSetTimer::WaitUpTime);

	msec = atoi( it.getProp("offdelay").c_str() );
	b->ptOffDelay.setTiming(msec);
	if( msec<=0 )
		b->ptOffDelay.setTiming(UniSetTimer::WaitUpTime);
	
	string saf = it.getProp("safety");
	if( !saf.empty() )
		b->safety = atoi(saf.c_str());
	else
		b->safety = NoSafety;

	b->stype = UniSetTypes::getIOType(it.getProp("iotype"));
	if( b->stype == UniversalIO::UnknownIOType )
	{
		if( dlog )
			dlog[Debug::CRIT] << myname << "(IOBase::readItem): ����������� iotype=: " 
				<< it.getProp("iotype") << " ��� " << sname << endl;
		return false;
	}

	b->cal.minRaw = 0;
	b->cal.maxRaw = 0;
	b->cal.minCal = 0;
	b->cal.maxCal = 0;
	b->cal.sensibility = 0;
	b->cal.precision = 0;
	b->cdiagram = 0;
	b->f_median = false;
		
	shm->initAIterator(b->ait);
	shm->initDIterator(b->dit);

	if( b->stype == UniversalIO::AnalogInput || b->stype == UniversalIO::AnalogOutput )
	{
		b->cal.minRaw = atoi( it.getProp("rmin").c_str() );
		b->cal.maxRaw = atoi( it.getProp("rmax").c_str() );
		b->cal.minCal = atoi( it.getProp("cmin").c_str() );
		b->cal.maxCal = atoi( it.getProp("cmax").c_str() );
		b->cal.sensibility = atoi( it.getProp("sensibility").c_str() );
		b->cal.precision = atoi( it.getProp("precision").c_str() );

		int f_size 	= def_filtersize;
		float f_T 	= def_filterT;
		int f_median = atoi( it.getProp("filtermedian").c_str() );
		
		if( f_median > 0 )
		{
			f_size = f_median;
			b->f_median = true;
		}
		else
		{
			if( !it.getProp("filtersize").empty() )
			{
				f_size = atoi(it.getProp("filtersize").c_str());
				if( f_size < 0 )
					f_size = 0;
			}
		}
		
		if( !it.getProp("filterT").empty() )
		{
			f_T = atof(it.getProp("filterT").c_str());
			if( f_T < 0 )
				f_T = 0.0;
		}

		if( b->stype == UniversalIO::AnalogInput )
			b->df.setSettings( f_size, f_T );

		b->df.init(b->defval);

		std::string caldiagram( it.getProp("caldiagram") );
		if( !caldiagram.empty() )
			b->cdiagram = UniSetExtensions::buildCalibrationDiagram(caldiagram);
	}
	else if( b->stype == UniversalIO::DigitalInput || b->stype == UniversalIO::DigitalOutput )
	{
		string tai(it.getProp("threshold_aid"));
		if( !tai.empty() )
		{
			b->t_ai = conf->getSensorID(tai);
			if( b->t_ai == DefaultObjectId )
			{
				if( dlog )
					dlog[Debug::CRIT] << myname << "(IOBase::readItem): unknown ID for threshold_ai "
						<< tai << endl;
				return false;
			}
		
			b->ti.lowlimit 	= uni_atoi( it.getProp("lowlimit").c_str() );
			b->ti.hilimit 		= uni_atoi( it.getProp("hilimit").c_str() );
			b->ti.sensibility 	= uni_atoi( it.getProp("sensibility").c_str() );
		}
	}
//	else
//	{
//		dlog[Debug::CRIT] << myname << "(IOBase::readItem): ����������� iotype=: " << stype << " ��� " << sname << endl;
//		return false;
//	}

	return true;
}
// -----------------------------------------------------------------------------