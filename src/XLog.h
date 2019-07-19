//////////////////////////////////////////////////////////////////////////
///@file	XLog.h													//
///@brief	class XLog												//
///@author	__ZHUCZ__(zhucz333@163.com)									//
///@date	2019/04/08													//
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "IXLog.h"
#include "MThread.h"
#include <string>

class XLog : public IXLog
{
public:
	XLog(const char *log_file_prefix, LogLevelType level, int log_file_size);
	~XLog();

public:
	bool debug(const char *file, size_t line, const char *fmt, ...) override;
	bool info(const char *file, size_t line, const char *fmt, ...) override;
	bool notice(const char *file, size_t line, const char *fmt, ...) override;
	bool warning(const char *file, size_t line, const char *fmt, ...) override;
	bool error(const char *file, size_t line, const char *fmt, ...) override;
	bool fatal(const char *file, size_t line, const char *fmt, ...) override;

private:
	void flush_internal(bool is_debug, std::string buffer, std::string date);

private:
	FILE* m_fp_debug;
	FILE* m_fp_warn;
	LogLevelType m_log_level;
	std::string m_debug_file;
	std::string m_warn_file;
	std::string m_debug_buffer;
	std::string m_warn_buffer;
	std::string m_date_now;
	std::thread::id m_thread_id;
	int m_file_size;
	std::shared_ptr<MThread> m_flush_thread;
};
