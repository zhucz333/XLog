#include "IXLog.h"
#include "XLog.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <direct.h>
#include <chrono>


#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

const char* FMTTIME = "%02d-%02d %02d:%02d:%02d.%d|%lld";
const char* FMTDATE = "%04d%02d%02d";
const char* FMTLOG = "%s %s [%u][%s:%zu]";

static IXLog* g_log = nullptr;

const static std::unordered_map<LogLevelType, std::string> g_log_level_name_map = {
	{LogLevel::LEVEL_DEBUG,		"DEBUG  :"},
	{LogLevel::LEVEL_INFO,		"INFO   :"},
	{LogLevel::LEVEL_NOTICE,	"NOTICE :"},
	{LogLevel::LEVEL_WARNING,	"WARNING:"},
	{LogLevel::LEVEL_ERROR,		"ERROR  :"},
	{LogLevel::LEVEL_FATAL,		"FATAL  :"}
};

#ifdef WIN32
static	DWORD						g_log_tls_key;
#define GET_TLS_KEY(key)			(key = TlsAlloc())
#define FREE_TLS_KEY(key)			(TlsFree(key))
#define SET_TLS_VALUE(key,val)		(TlsSetValue(key, val))
#define GET_TLS_VALUE(key,val)		(val = (decltype(val))TlsGetValue(key))
#define LOCAL_TIME(tm,s)			(localtime_s(tm,s))
#else
static	pthread_key_t				g_log_tls_key;
#define GET_TLS_KEY(key)			(pthread_key_create(&key, NULL))
#define FREE_TLS_KEY(key)			(pthread_setspecific(key, NULL))
#define SET_TLS_VALUE(key,val)		(pthread_setspecific(key, val))
#define GET_TLS_VALUE(key,val)		(val = pthread_getspecific(key))
#define LOCAL_TIME(tm,s)			(localtime_r(s,tm))
#endif

int get_local_time(char* time_buffer, size_t len1, char* date_buffer, size_t len2)
{
	struct tm vtm;

	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);
	auto micro_seconds = std::chrono::duration_cast<std::chrono::microseconds>((now - std::chrono::system_clock::from_time_t(time))).count();
	auto micro_steady = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	LOCAL_TIME(&vtm, &time);

	memset(time_buffer, 0, len1);
	snprintf(time_buffer, len1, FMTTIME, vtm.tm_mon + 1, vtm.tm_mday, vtm.tm_hour, vtm.tm_min, vtm.tm_sec, micro_seconds, micro_steady);

	memset(date_buffer, 0, len2);
	snprintf(date_buffer, len2, FMTDATE, vtm.tm_year + 1900, vtm.tm_mon + 1, vtm.tm_mday);

	return 0;
}

void IXLog::config_instance(const char* prefix, LogLevelType type, size_t log_file_size)
{
	if (nullptr == g_log) {
		g_log = new XLog(prefix, type, log_file_size);
	}
}

IXLog* IXLog::get_instance()
{
	if (nullptr == g_log) {
		g_log = new XLog("./log", LogLevel::LEVEL_DEBUG, 10);
	}

	return g_log;
}

void IXLog::destroy_instance()
{
	if (g_log) {
		delete g_log;
		g_log = nullptr;
	}
}

XLog::XLog(const char *logFilePrefix, const LogLevelType level, int log_file_size) : IXLog(logFilePrefix, level, log_file_size), 
	m_log_level(level), m_file_size(log_file_size * 1024 * 1024)
{
	char time_buff[64] = { 0 };
	char date_buff[16] = { 0 };

	char const* folder = strrchr(logFilePrefix, '/');
	if (folder) {
		std::string path(logFilePrefix, folder - logFilePrefix);
		_mkdir(path.c_str());
	}
	m_debug_file= std::string(logFilePrefix) + ".log";
	m_warn_file = std::string(logFilePrefix) + ".log.wf";

	m_fp_debug = fopen(m_debug_file.c_str(), "a");
	assert(NULL != m_fp_debug);

	m_fp_warn = fopen(m_warn_file.c_str(), "a");
	assert(NULL != m_fp_warn);

	get_local_time(time_buff, sizeof(time_buff), date_buff, sizeof(date_buff));
	m_date_now.append(date_buff);

	g_log = this;

	m_flush_thread = std::make_shared<MThread>();
	m_flush_thread->Start(1);
}

XLog::~XLog()
{
	m_flush_thread->Stop();

	fclose(m_fp_debug);
	fclose(m_fp_warn);

	g_log = nullptr;
}

#define LOG_FUNC(FUNC,LEVEL) \
	bool FUNC(const char *file, size_t line, const char *fmt, ...) { \
		va_list args; \
		char head[256] = { 0 }; \
		char time_buff[64] = {0}; \
		char date_buff[16] = {0}; \
		std::string log_debug, log_warn; \
		if (m_log_level > LEVEL) { \
			return true; \
		} \
		get_local_time(time_buff, sizeof(time_buff), date_buff, sizeof(date_buff)); \
		snprintf(head, sizeof(head), FMTLOG, time_buff, g_log_level_name_map.find(LEVEL)->second.c_str(), std::this_thread::get_id(), file, line); \
		if (LEVEL>LogLevel::LEVEL_NOTICE) log_warn.append(head); \
		else log_debug.append(head); \
		va_start(args, fmt); \
		int size = _vscprintf(fmt, args); \
		char *log = (char*)calloc(1, size + 1); \
		vsnprintf(log, size + 1, fmt, args); \
		va_end(args); \
		if (LEVEL>=LogLevel::LEVEL_WARNING) { \
			log_warn.append(log).append("\n"); \
		} \
		else { \
			log_debug.append(log).append("\n"); \
		} \
		free(log); \
		if (LEVEL < LogLevel::LEVEL_WARNING) { \
			m_flush_thread->Post(std::bind(&XLog::flush_internal, this, true, log_debug, std::string(date_buff))); \
		} else { \
			m_flush_thread->Post(std::bind(&XLog::flush_internal, this, false, log_warn, std::string(date_buff))); \
		} \
		return true; \
	} \

LOG_FUNC(XLog::debug, LogLevel::LEVEL_DEBUG)
LOG_FUNC(XLog::info, LogLevel::LEVEL_INFO)
LOG_FUNC(XLog::notice, LogLevel::LEVEL_NOTICE)
LOG_FUNC(XLog::warning, LogLevel::LEVEL_WARNING)
LOG_FUNC(XLog::error, LogLevel::LEVEL_ERROR)
LOG_FUNC(XLog::fatal, LogLevel::LEVEL_FATAL)

#undef LOG

void XLog::flush_internal(bool is_debug, std::string buffer, std::string date)
{
	struct _stat state = {0};

	if (0 != m_date_now.compare(date)) {
		fclose(m_fp_debug);
		rename(m_debug_file.c_str(), (m_debug_file + "." + m_date_now + std::to_string(time(NULL))).c_str());
		m_fp_debug = fopen(m_debug_file.c_str(), "a");

		fclose(m_fp_warn);
		rename(m_warn_file.c_str(), (m_warn_file + "." + m_date_now + std::to_string(time(NULL))).c_str());
		m_fp_warn = fopen(m_warn_file.c_str(), "a");

		m_date_now = date;
	}

	if (is_debug) {
		if (0 == _stat(m_debug_file.c_str(), &state) && (state.st_size > m_file_size)) {
			fclose(m_fp_debug);
			rename(m_debug_file.c_str(), (m_debug_file + "." + m_date_now + std::to_string(time(NULL))).c_str());
			m_fp_debug = fopen(m_debug_file.c_str(), "a");
		}

		fwrite(buffer.c_str(), buffer.length(), 1, m_fp_debug);
		fflush(m_fp_debug);
		buffer.clear();
	} else {
		if (0 == _stat(m_warn_file.c_str(), &state) && (state.st_size > m_file_size)) {
			fclose(m_fp_warn);
			rename(m_warn_file.c_str(), (m_warn_file + "." + m_date_now + std::to_string(time(NULL))).c_str());
			m_fp_warn = fopen(m_warn_file.c_str(), "a");
		}

		fwrite(buffer.c_str(), buffer.length(), 1, m_fp_warn);
		fflush(m_fp_warn);
		buffer.clear();
	}
}
