// --------------------------------------------------------------------------
//! \version $Id: smemory.cc,v 1.1 2008/12/14 21:57:50 vpashka Exp $
// --------------------------------------------------------------------------
#include <string>
#include "Debug.h"
#include "Configuration.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace UniSetTypes;
// --------------------------------------------------------------------------
int main(int argc, const char **argv)
{   
	if( argc>1 && strcmp(argv[1],"--help")==0 )
	{
		cout << "--confile	- Configuration file. Default: test.xml" << endl;
		return 0;
	}

	try
	{
		string confile = UniSetTypes::getArgParam( "--confile", argc, argv, "test.xml" );
		conf = new Configuration(argc, argv, confile);

		string t(conf->oind->getTextName(1));

		cout << "**** check getTextName: " << ( t.empty() ?  "FAILED" : "OK" ) << endl;
		return 0;
	}
	catch(SystemError& err)
	{
		cerr << "(conftest): " << err << endl;
	}
	catch(Exception& ex)
	{
		cerr << "(conftest): " << ex << endl;
	}
	catch(...)
	{
		cerr << "(conftest): catch(...)" << endl;
	}
	
	return 1;
}