// --------------------------------------------------------------------------
//!  \version $Id: MTR.h,v 1.1 2008/12/14 21:57:50 vpashka Exp $
// --------------------------------------------------------------------------
#ifndef _MTR_H_
#define _MTR_H_
// -----------------------------------------------------------------------------
#include <string>
#include <cstring>
#include <cmath>
#include "modbus/ModbusTypes.h"
// -----------------------------------------------------------------------------
class ModbusRTUMaster;
// -----------------------------------------------------------------------------
namespace MTR
{
	// ������������� � ������ ���������� ���� ������
	enum MTRType
	{
		mtUnknown,
		mtT1,
		mtT2,
		mtT3,
		mtT4,
		mtT5,
		mtT6,
		mtT7,
		mtT8,
		mtT9,
		mtF1,
		mtT_Str16,
		mtT_Str8
	};
	// -------------------------------------------------------------------------
	std::string type2str( MTRType t );			/*!< ������������� ������ � ��� */
	MTRType str2type( const std::string s );	/*!< �������������� �������� � ������ */
	int wsize( MTRType t ); 					/*!< ����� ������ � ������ */
	// -------------------------------------------------------------------------
	// ����������
	const ModbusRTU::ModbusData regModelNumber	= 30001;
	const ModbusRTU::ModbusData regSerialNumber	= 30009;
	
	std::string getModelNumber( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr );
	std::string getSerialNumber( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr );
	// -------------------------------------------------------------------------
	// ��������� �����
	const ModbusRTU::ModbusData regAddress		= 40055;
	const ModbusRTU::ModbusData regBaudRate		= 40056;
	const ModbusRTU::ModbusData regStopBit		= 40057; /* 0 - Stop bit, 1 - Stop bits */
	const ModbusRTU::ModbusData regParity		= 40058;
	const ModbusRTU::ModbusData regDataBits		= 40059;

	enum mtrBaudRate
	{
		br1200 		= 0,
		br2400 		= 1,
		br4800 		= 2,
		br9600 		= 3,
		br19200 	= 4,
		br38400 	= 5,
		br57600		= 6,
		br115200	= 7
	};
	
	enum mtrParity
	{
		mpNoParity		= 0,
		mpOddParity 	= 1,
		mpEvenParity 	= 2
	};

	enum mtrDataBits
	{
		db8Bits	= 0,
		db7Bits	= 1
	};

	bool setAddress( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, ModbusRTU::ModbusAddr newAddr );
	bool setBaudRate( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, mtrBaudRate br );
	bool setStopBit( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, bool state );
	bool setParity( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, mtrParity p );
	bool setDataBits( ModbusRTUMaster* mb, ModbusRTU::ModbusAddr addr, mtrDataBits d );
	
	// -------------------------------------------------------------------------
	static const int u2size = 2;
	// -------------------------------------------------------------------------
	class T1
	{
		public:
			T1():val(0){}
			T1( unsigned short v ):val(v){}
			T1( const ModbusRTU::ModbusData* data ):val(data[0]){}
			~T1(){}
			// ------------------------------------------
			/*! ������ � ������ */
			static int wsize(){ return 1; }
			/*! ��� �������� */
			static MTRType type(){ return mtT1; }
			// ------------------------------------------
			unsigned short val;
	};	
	// -------------------------------------------------------------------------
	class T2
	{
		public:
			T2():val(0){}
			T2( signed short v ):val(v){}
			T2( const ModbusRTU::ModbusData* data ):val(data[0]){}
			~T2(){}
			// ------------------------------------------
			/*! ������ � ������ */
			static int wsize(){ return 1; }
			/*! ��� �������� */
			static MTRType type(){ return mtT2; }
			// ------------------------------------------
			signed short val;
	};
	// -------------------------------------------------------------------------
	class T3
	{
		public:
			// ------------------------------------------
			/*! ��� �������� � ������ */
			typedef union
			{
				unsigned short v[u2size];
				signed int val; // :32
			} T3mem; 
			// ------------------------------------------
			// ������������ �� ������ ������...
			T3(){ memset(raw.v,0,sizeof(raw.v)); }
			
			T3( signed int i ){ raw.val = i; }

			T3( unsigned short v1, unsigned short v2 )
			{
				raw.v[0] = v1;
				raw.v[1] = v2;
			}
			
			T3( const ModbusRTU::ModbusData* data )
			{
				if( sizeof(data) >=u2size )
				{
					for( int i=0; i<u2size; i++ )
						raw.v[i] = data[i];
				}
			}

			~T3(){}
			// ------------------------------------------
			/*! ������ � ������ */
			static int wsize(){ return u2size; }
			/*! ��� �������� */
			static MTRType type(){ return mtT3; }
			// ------------------------------------------
			// ������� �������������� � ������ �����
			operator long() { return raw.val; }

			T3mem raw;
	};
	// --------------------------------------------------------------------------
	class T4
	{
		public:
			// ------------------------------------------
			// ������������ �� ������ ������...
			T4():sval(""),raw(0){}
			T4( unsigned short v1 ):raw(v1)
			{
				char c[3];
				memcpy(c,&v1,sizeof(c));
				sval = std::string(c);
			}
			
			T4( const ModbusRTU::ModbusData* data ):
				raw(data[0])
			{
				char c[3];
				memcpy(c,&(data[0]),sizeof(c));
				sval 	= std::string(c);
			}
			
			~T4(){}
			// ------------------------------------------
			/*! ������ � ������ */
			static int wsize(){ return 1; }
			/*! ��� �������� */
			static MTRType type(){ return mtT4; }
			// ------------------------------------------
			std::string sval;
			unsigned short raw;
	};
	// --------------------------------------------------------------------------
	class T5
	{
		public:
			// ------------------------------------------
			/*! ��� �������� � ������ */
			typedef union
			{
				unsigned short v[u2size];
				struct u_T5
				{
					unsigned int val:24;
					signed char exp; // :8
				} __attribute__( ( packed ) ) u2;
				long lval;
			} T5mem;
			// ------------------------------------------
			// ������������ �� ������ ������...
			T5():val(0){ memset(raw.v,0,sizeof(raw.v)); }
			T5( unsigned short v1, unsigned short v2 )
			{
				raw.v[0] = v1;
				raw.v[1] = v2;
				val = raw.u2.val * pow(10,raw.u2.exp);
			}
			
			T5( long v )
			{
				raw.lval = v;
			}
			
			T5( const ModbusRTU::ModbusData* data )
			{
				if( sizeof(data) >=u2size )
				{
					for( int i=0; i<u2size; i++ )
						raw.v[i] = data[i];
					val = raw.u2.val * pow(10,raw.u2.exp);
				}
			}

			~T5(){}
			// ------------------------------------------
			/*! ������ � ������ */
			static int wsize(){ return u2size; }
			/*! ��� �������� */
			static MTRType type(){ return mtT5; }
			// ------------------------------------------
			double val;
			T5mem raw;
	};
	// --------------------------------------------------------------------------
	class T6
	{
		public:
			// ------------------------------------------
			/*! ��� �������� � ������ */
			typedef union
			{
				unsigned short v[u2size];
				struct u_T6
				{
					signed int val:24;
					signed char exp; // :8
				} u2;
				long lval;
			} T6mem;
			// ------------------------------------------
			// ������������ �� ������ ������...
			T6():val(0){ memset(raw.v,0,sizeof(raw.v)); }
			T6( unsigned short v1, unsigned short v2 )
			{
				raw.v[0] = v1;
				raw.v[1] = v2;
				val = raw.u2.val * pow(10,raw.u2.exp);
			}

			T6( long v )
			{
				raw.lval = v;
			}
			
			T6( const ModbusRTU::ModbusData* data )
			{
				if( sizeof(data) >=u2size )
				{
					for( int i=0; i<u2size; i++ )
						raw.v[i] = data[i];
					val = raw.u2.val * pow(10,raw.u2.exp);
				}
			}

			~T6(){}
			// ------------------------------------------
			/*! ������ � ������ */
			static int wsize(){ return u2size; }
			/*! ��� �������� */
			static MTRType type(){ return mtT6; }
			// ------------------------------------------
			double val;
			T6mem raw;
	};
	// --------------------------------------------------------------------------
	class T7
	{
		public:
			// ------------------------------------------
			/*! ��� �������� � ������ */
			typedef union
			{
				unsigned short v[u2size];
				struct u_T7
				{
					unsigned int val:16;
					unsigned char ic; // :8 - Inductive/capacitive
					unsigned char ie; // :8 - Import/export 
				}__attribute__( ( packed ) ) u2;
				long lval;
			} T7mem;
			// ------------------------------------------
			// ������������ �� ������ ������...
			T7():val(0){ memset(raw.v,0,sizeof(raw.v)); }
			T7( unsigned short v1, unsigned short v2 )
			{
				raw.v[0] = v1;
				raw.v[1] = v2;
				val = raw.u2.val * pow(10,-4);
			}
			T7( const long v )
			{
				raw.lval = v;
			}
			
			T7( const ModbusRTU::ModbusData* data )
			{
				if( sizeof(data) >=u2size )
				{
					for( int i=0; i<u2size; i++ )
						raw.v[i] = data[i];
					val = raw.u2.val * pow(10,-4);
				}
			}

			~T7(){}
			// ------------------------------------------
			/*! ������ � ������ */
			static int wsize(){ return u2size; }
			/*! ��� �������� */
			static MTRType type(){ return mtT7; }
			// ------------------------------------------
			double val;
			T7mem raw;
	};
	// --------------------------------------------------------------------------
	class T8
	{
		public:
			// ------------------------------------------
			/*! ��� �������� � ������ */
			typedef union
			{
				unsigned short v[u2size];
				struct u_T8
				{
					unsigned short mon:8;
					unsigned short day:8;
					unsigned short hour:8;
					unsigned short min:8;
				}__attribute__( ( packed ) ) u2;
			} T8mem;
			// ------------------------------------------
			// ������������ �� ������ ������...
			T8(){ memset(raw.v,0,sizeof(raw.v)); }
			T8( unsigned short v1, unsigned short v2 )
			{
				raw.v[0] = v1;
				raw.v[1] = v2;
			}

			T8( const ModbusRTU::ModbusData* data )
			{
				if( sizeof(data) >=u2size )
				{
					for( int i=0; i<u2size; i++ )
						raw.v[i] = data[i];
				}
			}
			
			inline unsigned short day(){ return raw.u2.day; }
			inline unsigned short mon(){ return raw.u2.mon; }
			inline unsigned short hour(){ return raw.u2.hour; }
			inline unsigned short min(){ return raw.u2.min; }

			~T8(){}
			// ------------------------------------------
			/*! ������ � ������ */
			static int wsize(){ return u2size; }
			/*! ��� �������� */
			static MTRType type(){ return mtT8; }
			// ------------------------------------------
			T8mem raw;
	};
	// --------------------------------------------------------------------------
	class T9
	{
		public:
			// ------------------------------------------
			/*! ��� �������� � ������ */
			typedef union
			{
				unsigned short v[u2size];
				struct u_T9
				{
					unsigned short hour:8;
					unsigned short min:8;
					unsigned short sec:8;
					unsigned short ssec:8;
				}__attribute__( ( packed ) ) u2;
			} T9mem;
			// ------------------------------------------
			// ������������ �� ������ ������...
			T9(){ memset(raw.v,0,sizeof(raw.v)); }
			T9( unsigned short v1, unsigned short v2 )
			{
				raw.v[0] = v1;
				raw.v[1] = v2;
			}

			T9( const ModbusRTU::ModbusData* data )
			{
				if( sizeof(data) >=u2size )
				{
					for( int i=0; i<u2size; i++ )
						raw.v[i] = data[i];
				}
			}

			inline unsigned short hour(){ return raw.u2.hour; }
			inline unsigned short min(){ return raw.u2.min; }
			inline unsigned short sec(){ return raw.u2.sec; }
			inline unsigned short ssec(){ return raw.u2.ssec; }

			~T9(){}
			// ------------------------------------------
			/*! ������ � ������ */
			static int wsize(){ return u2size; }
			/*! ��� �������� */
			static MTRType type(){ return mtT9; }
			// ------------------------------------------
			T9mem raw;
	};
	// --------------------------------------------------------------------------
	class F1
	{
		public:
			// ------------------------------------------
			/*! ��� �������� � ������ */
			typedef union
			{
				unsigned short v[2];
				float val; // 
			} F1mem;
			// ------------------------------------------
			// ������������ �� ������ ������...
			F1(){ memset(raw.v,0,sizeof(raw.v)); }
			F1( unsigned short v1, unsigned short v2 )
			{
				raw.v[0] = v1;
				raw.v[1] = v2;
			}

			F1( float f )
			{
				raw.val = f;
			}

			F1( const ModbusRTU::ModbusData* data )
			{
				if( sizeof(data) >=u2size )
				{
					for( int i=0; i<u2size; i++ )
						raw.v[i] = data[i];
				}
			}

			~F1(){}
			// ------------------------------------------
			/*! ������ � ������ */
			static int wsize(){ return u2size; }
			/*! ��� �������� */
			static MTRType type(){ return mtF1; }
			// ------------------------------------------
			operator float(){ return raw.val; }
			operator long(){ return lroundf(raw.val); }
			
			F1mem raw;
	};
	// --------------------------------------------------------------------------
	class T_Str16
	{
		public:
			// ------------------------------------------
			// ������������ �� ������ ������...
			T_Str16():sval(""){}
			T_Str16( const ModbusRTU::ReadInputRetMessage& ret )
			{
				char c[16];
				memcpy(c,&ret.data,sizeof(c));
				sval = std::string(c);
			}

			~T_Str16(){}
			// ------------------------------------------
			/*! ������ � ������ */
			static int wsize(){ return 8; }
			/*! ��� �������� */
			static MTRType type(){ return mtT_Str16; }
			// ------------------------------------------
			std::string sval;
	};
	// --------------------------------------------------------------------------
	class T_Str8
	{
		public:
			// ------------------------------------------
			// ������������ �� ������ ������...
			T_Str8():sval(""){}
			T_Str8( const ModbusRTU::ReadInputRetMessage& ret )
			{
				char c[8];
				memcpy(c,&ret.data,sizeof(c));
				sval = std::string(c);
			}

			~T_Str8(){}
			// ------------------------------------------
			/*! ������ � ������ */
			static int wsize(){ return 4; }
			/*! ��� �������� */
			static MTRType type(){ return mtT_Str8; }
			// ------------------------------------------
			std::string sval;
	};
	// --------------------------------------------------------------------------
} // end of namespace MTR
// --------------------------------------------------------------------------
#endif // _MTR_H_
// -----------------------------------------------------------------------------