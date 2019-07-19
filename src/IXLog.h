//////////////////////////////////////////////////////////////////////////
///@file	IXLog.h														//
///@brief	class Log													//
///@author	__ZHUCZ__(zhucz333@163.com)									//
///@date	2019/04/08													//
//////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

#ifdef _WIN32
#ifdef LIB_LOG_EXPORT
#define LOG_EXPORT __declspec(dllexport)
#else
#define LOG_EXPORT __declspec(dllimport)
#endif
#else
#define LOG_EXPORT 
#endif


using LogLevelType = int;
namespace LogLevel 
{
	static const LogLevelType LEVEL_DEBUG	= 0;
	static const LogLevelType LEVEL_INFO	= 1;
	static const LogLevelType LEVEL_NOTICE	= 2;
	static const LogLevelType LEVEL_WARNING	= 3;
	static const LogLevelType LEVEL_ERROR	= 4;
	static const LogLevelType LEVEL_FATAL	= 5;
}

class LOG_EXPORT IXLog
{
public:
	static void config_instance(const char* prefix="./log", LogLevelType type = LogLevel::LEVEL_DEBUG, size_t log_file_size = 10);
	static IXLog* get_instance();
	static void destroy_instance();

	IXLog(const char *log_file_prefix, LogLevelType level, int log_file_size) {};
	virtual ~IXLog() {};

	virtual bool debug(const char *file, size_t line, const char *fmt, ...) = 0;
	virtual bool info(const char *file, size_t line, const char *fmt, ...) = 0;
	virtual bool notice(const char *file, size_t line, const char *fmt, ...) = 0;
	virtual bool warning(const char *file, size_t line, const char *fmt, ...) = 0;
	virtual bool error(const char *file, size_t line, const char *fmt, ...) = 0;
	virtual bool fatal(const char *file, size_t line, const char *fmt, ...) = 0;
};

#define __FILENAME__			(strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1) : __FILE__)
#define LOG_DEBUG(fmt, ...)		IXLog::get_instance()->debug(__FILENAME__, __LINE__, fmt, __VA_ARGS__)
#define LOG_INFO(fmt, ...)		IXLog::get_instance()->info(__FILENAME__, __LINE__, fmt, __VA_ARGS__)
#define LOG_NOTICE(fmt, ...)	IXLog::get_instance()->notice(__FILENAME__, __LINE__, fmt, __VA_ARGS__)
#define LOG_WARNING(fmt, ...)	IXLog::get_instance()->warning(__FILENAME__, __LINE__,fmt, __VA_ARGS__)
#define LOG_ERROR(fmt, ...)		IXLog::get_instance()->error(__FILENAME__, __LINE__, fmt, __VA_ARGS__)
#define LOG_FATAL(fmt, ...)		IXLog::get_instance()->fatal(__FILENAME__, __LINE__, fmt, __VA_ARGS__)		
