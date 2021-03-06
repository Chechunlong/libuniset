// -------------------------------------------------------------------------
#ifndef UTCPStream_H_
#define UTCPStream_H_
// -------------------------------------------------------------------------
#include <string>
#include <cc++/socket.h>
// -------------------------------------------------------------------------
class UTCPStream:
    public ost::TCPStream
{
    public:

        UTCPStream();
        virtual ~UTCPStream();

        void create( const std::string& hname, int port, bool throwflag=false, timeout_t timer=0 );

    protected:
    private:

};
// -------------------------------------------------------------------------
#endif // UTCPStream_H_
// -------------------------------------------------------------------------
