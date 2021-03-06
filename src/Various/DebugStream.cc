// Created by Lars Gullik Bj�nnes
// Copyright 1999 Lars Gullik Bj�nnes (larsbj@lyx.org)
// Released into the public domain.

// Primarily developed for use in the LyX Project http://www.lyx.org/
// but should be adaptable to any project.

// (c) 2002 adapted for UniSet by Lav, GNU GPL license

//#define TEST_DEBUGSTREAM


#ifdef __GNUG__
#pragma implementation
#endif

//#include "DebugStream.h"
#include "Debug.h"

//�Since the current C++ lib in egcs does not have a standard implementation
// of basic_streambuf and basic_filebuf we don't have to include this
// header.
//#define MODERN_STL_STREAMS
#ifdef MODERN_STL_STREAMS
#include <fstream>
#endif
#include <iostream>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <iomanip>

using std::ostream;
using std::streambuf;
using std::streamsize;
using std::filebuf;
using std::cerr;
using std::ios;

/*
ostream & operator<<(ostream & o, Debug::type t)
{
	return o << int(t);
}
*/

/** This is a streambuffer that never prints out anything, at least
    that is the intention. You can call it a no-op streambuffer, and
    the ostream that uses it will be a no-op stream.
*/
class nullbuf : public streambuf {
protected:
#ifndef MODERN_STL_STREAMS
	typedef char char_type;
	typedef int int_type;
	///
	virtual int sync() { return 0; }
#endif
	///
	virtual streamsize xsputn(char_type const *, streamsize n) {
		// fakes a purge of the buffer by returning n
		return n;
	}
#ifdef MODERN_STL_STREAMS
	///
	virtual int_type overflow(int_type c = traits_type::eof()) {
		// fakes success by returning c
		return c == traits_type::eof() ? ' ' : c;
	}
#else
	///
	virtual int_type overflow(int_type c = EOF) {
		// fakes success by returning c
		return c == EOF ? ' ' : c;
	}
#endif
};

/** A streambuf that sends the output to two different streambufs. These
    can be any kind of streambufs.
*/
class teebuf : public streambuf {
public:
	///
	teebuf(streambuf * b1, streambuf * b2)
		: streambuf(), sb1(b1), sb2(b2) {}
protected:
#ifdef MODERN_STL_STREAMS
	///
	virtual int sync() {
		sb2->pubsync();
		return sb1->pubsync();
	}
	///
	virtual streamsize xsputn(char_type const * p, streamsize n) {
		sb2->sputn(p, n);
		return sb1->sputn(p, n);
	}
	///
	virtual int_type overflow(int_type c = traits_type::eof()) {
		sb2->sputc(c);
		return sb1->sputc(c);
	}
#else
	typedef char char_type;
	typedef int int_type;
	///
	virtual int sync() {
		sb2->sync();
		return sb1->sync();
	}
	///
	virtual streamsize xsputn(char_type const * p, streamsize n) {
		sb2->xsputn(p, n);
		return sb1->xsputn(p, n);
	}
	///
	virtual int_type overflow(int_type c = EOF) {
		sb2->overflow(c);
		return sb1->overflow(c);
	}
#endif
private:
	///
	streambuf * sb1;
	///
	streambuf * sb2;
};

///
class debugbuf : public streambuf {
public:
	///
	debugbuf(streambuf * b)
		: streambuf(), sb(b) {}
protected:
#ifdef MODERN_STL_STREAMS
	///
	virtual int sync() {
		return sb->pubsync();
	}
	///
	virtual streamsize xsputn(char_type const * p, streamsize n) {
		return sb->sputn(p, n);
	}
	///
	virtual int_type overflow(int_type c = traits_type::eof()) {
		return sb->sputc(c);
	}
#else
	typedef char char_type;
	typedef int int_type;
	///
	virtual int sync() {
		return sb->sync();
	}
	///
	virtual streamsize xsputn(char_type const * p, streamsize n) {
		return sb->xsputn(p, n);
	}
	///
	virtual int_type overflow(int_type c = EOF) {
		return sb->overflow(c);
	}
#endif
private:
	///
	streambuf * sb;
};

//--------------------------------------------------------------------------
/// So that public parts of DebugStream does not need to know about filebuf
struct DebugStream::debugstream_internal {
	/// Used when logging to file.
	filebuf fbuf;
};
//--------------------------------------------------------------------------

/// Constructor, sets the debug level to t.
DebugStream::DebugStream(Debug::type t)
	: ostream(new debugbuf(cerr.rdbuf())),
	  dt(t), nullstream(new nullbuf), internal(0),
	  show_datetime(true),
	  fname(""){}

//--------------------------------------------------------------------------
/// Constructor, sets the log file to f, and the debug level to t.
DebugStream::DebugStream(char const * f, Debug::type t)
	: ostream(new debugbuf(cerr.rdbuf())),
	  dt(t), nullstream(new nullbuf),
	  internal(new debugstream_internal),
	  show_datetime(true),
	  fname("")
{
	internal->fbuf.open(f, ios::out|ios::app);
	delete rdbuf(new teebuf(cerr.rdbuf(),
				&internal->fbuf));
}
//--------------------------------------------------------------------------

DebugStream::~DebugStream()
{
	delete nullstream.rdbuf(0); // Without this we leak
	delete rdbuf(0);            // Without this we leak
	delete internal;
}

//--------------------------------------------------------------------------
const DebugStream& DebugStream::operator=( const DebugStream& r )
{
	if( r == *this )
		return *this;

	dt = r.dt;
	show_datetime = r.show_datetime;
	fname = r.fname;
	if( !r.fname.empty() )
		logFile(fname);
	
	return *this;
}
//--------------------------------------------------------------------------
/// Sets the debugstreams' logfile to f.
void DebugStream::logFile(const std::string f)
{
	fname = f;
	if (internal) {
		internal->fbuf.close();
	} else {
		internal = new debugstream_internal;
	}
	internal->fbuf.open(f.c_str(), ios::out|ios::app);
	delete rdbuf(new teebuf(cerr.rdbuf(),
				&internal->fbuf));
}
//--------------------------------------------------------------------------
std::ostream & DebugStream::debug(Debug::type t) 
{
	if(dt & t)
	{
		if( show_datetime )
			print_datetime(t);
		*this << "(" << std::setfill(' ') << std::setw(6) << t << "):  "; // "):\t";
		return *this;
	}
	
	return nullstream;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::operator()(Debug::type t)
{
	if(dt & t)
		return *this;
		
	return nullstream;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::print_date(Debug::type t, char brk)
{
	if(dt && t)
	{
		time_t GMTime = time(NULL);
		struct tm *tms = localtime(&GMTime);
		return *this << std::setw(2) << std::setfill('0') << tms->tm_mday << brk
				 	 << std::setw(2) << std::setfill('0') << tms->tm_mon+1 << brk
					 << std::setw(4) << std::setfill('0') << tms->tm_year+1900;
	}
	
	return nullstream;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::print_time(Debug::type t, char brk)
{
	if(dt && t)
	{
		time_t GMTime = time(NULL);
		struct tm *tms = localtime(&GMTime);
		return *this << std::setw(2) << std::setfill('0') << tms->tm_hour << brk
					 << std::setw(2) << std::setfill('0') << tms->tm_min << brk
					 << std::setw(2) << std::setfill('0') << tms->tm_sec;
	}
	
	return nullstream;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::print_datetime(Debug::type t)
{
	if(dt & t)
	{
		time_t GMTime = time(NULL);
		struct tm *tms = localtime(&GMTime);
		return *this << std::setw(2) << std::setfill('0') << tms->tm_mday << "/"
					 << std::setw(2) << std::setfill('0') << tms->tm_mon+1 << "/"
					 << std::setw(4) << std::setfill('0') << tms->tm_year+1900 << " "
					 << std::setw(2) << std::setfill('0') << tms->tm_hour << ":"
					 << std::setw(2) << std::setfill('0') << tms->tm_min << ":"
					 << std::setw(2) << std::setfill('0') << tms->tm_sec;
	}
	
	return nullstream;
}
//--------------------------------------------------------------------------
std::ostream& DebugStream::pos(int x, int y)
{
	if( !dt )
		return nullstream;

	return *this << "\033[" << y << ";" << x << "f";
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#ifdef TEST_DEBUGSTREAM

// Example debug stream
DebugStream debugstream;

int main(int, char **)
{
	/**
	   I have been running some tests on this to see how much overhead
	   this kind of permanent debug code has. My conclusion is: not
	   much. In all, but the most time critical code, this will have
	   close to no impact at all.

	   In the tests that I have run the use of
	   if (debugstream.debugging(DebugStream::INFO))
	   debugstream << "some debug\n";
	   has close to no overhead when the debug level is not
	   DebugStream::INFO.

	   The overhead for
	   debugstream.debug(DebugStream::INFO) << "some debug\n";
	   is also very small when the debug level is not
	   DebugStream::INFO. However the overhead for this will increase
	   if complex debugging information is output.

	   The overhead when the debug level is DebugStream::INFO can be
	   significant, but since we then are running in debug mode it is
	   of no concern.

	   Why should we use this instead of the class Error that we already
	   have? First of all it uses C++ iostream and constructs, secondly
	   it will be a lot easier to output the debug info that we need
	   without a lot of manual conversions, thirdly we can now use
	   iomanipulators and the complete iostream formatting functions.
	   pluss it will work for all types that have a operator<<
	   defined, and can be used in functors that take a ostream & as
	   parameter. And there should be less need for temporary objects.
	   And one nice bonus is that we get a log file almost for
	   free.

	   Some of the names are of course open to modifications. I will try
	   to use the names we already use in LyX.
	*/
	// Just a few simple debugs to show how it can work.
	debugstream << "Debug level set to Debug::NONE\n";
	if (debugstream.debugging()) {
		debugstream << "Something must be debugged\n";
	}
	debugstream.debug(Debug::WARN) << "more debug(WARN)\n";
	debugstream.debug(Debug::INFO) << "even more debug(INFO)\n";
	debugstream.debug(Debug::CRIT) << "even more debug(CRIT)\n";
	debugstream.level(Debug::value("INFO"));
	debugstream << "Setting debug level to Debug::INFO\n";
	if (debugstream.debugging()) {
		debugstream << "Something must be debugged\n";
	}
	debugstream.debug(Debug::WARN) << "more debug(WARN)\n";
	debugstream.debug(Debug::INFO) << "even more debug(INFO)\n";
	debugstream.debug(Debug::CRIT) << "even more debug(CRIT)\n";
	debugstream.addLevel(Debug::type(Debug::CRIT |
					 Debug::WARN));
	debugstream << "Adding Debug::CRIT and Debug::WARN\n";
	debugstream[Debug::WARN] << "more debug(WARN)\n";
	debugstream[Debug::INFO] << "even more debug(INFO)\n";
	debugstream[Debug::CRIT] << "even more debug(CRIT)\n";
	debugstream.delLevel(Debug::INFO);
	debugstream << "Removing Debug::INFO\n";
	debugstream[Debug::WARN] << "more debug(WARN)\n";
	debugstream[Debug::INFO] << "even more debug(INFO)\n";
	debugstream[Debug::CRIT] << "even more debug(CRIT)\n";
	debugstream.logFile("logfile");
	debugstream << "Setting logfile to \"logfile\"\n";
	debugstream << "Value: " << 123 << " " << "12\n";
	int i = 0;
	int * p = new int;
	// note: the (void*) is needed on g++ 2.7.x since it does not
	// support partial specialization. In egcs this should not be
	// needed.
	debugstream << "automatic " << &i
		    << ", free store " << p << endl;
	delete p;
	/*
	for (int j = 0; j < 200000; ++j) {
		DebugStream tmp;
		tmp << "Test" << endl;
	}
	*/
}
#endif
