// --------------------------------------------------------------------------
//! \version $Id: smemory.cc,v 1.1 2008/12/14 21:57:50 vpashka Exp $
// --------------------------------------------------------------------------
#include <string>
#include "Debug.h"
#include "ObjectsActivator.h"
#include "SharedMemory.h"
#include "Extensions.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
using namespace UniSetExtensions;
// --------------------------------------------------------------------------
int main(int argc, char **argv)
{   
	if( argc>1 && strcmp(argv[1],"--help")==0 )
	{
		cout << "--confile	- ������������ ��������� ����. ����. �� ��������� configure.xml" << endl;
		SharedMemory::help_print(argc,argv);
		return 0;
	}

	try
	{
		string confile = UniSetTypes::getArgParam( "--confile", argc, argv, "configure.xml" );
		conf = new Configuration(argc, argv, confile);

		conf->initDebug(dlog,"dlog");
		string logfilename = conf->getArgParam("--logfile", "smemory.log");
		string logname( conf->getLogDir() + logfilename );
		unideb.logFile( logname.c_str() );
		dlog.logFile( logname.c_str() );

		SharedMemory* shm = SharedMemory::init_smemory(argc,argv);
		if( !shm )
			return 1;

		ObjectsActivator act;

		act.addObject(static_cast<class UniSetObject*>(shm));
		SystemMessage sm(SystemMessage::StartUp); 
		act.broadcast( sm.transport_msg() );
		act.run(false);

//		pause();	// �����, ����� �������� ������ ������ ��������� ������
		return 0;
	}
	catch(SystemError& err)
	{
		unideb[Debug::CRIT] << "(smemory): " << err << endl;
	}
	catch(Exception& ex)
	{
		unideb[Debug::CRIT] << "(smemory): " << ex << endl;
	}
	catch(...)
	{
		unideb[Debug::CRIT] << "(smemory): catch(...)" << endl;
	}
	
	return 1;
}