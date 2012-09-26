/*
 *  Logger.h
 *
 *  Copyright 2012 by Christian Hardmeier. All rights reserved.
 *
 *  This file is part of Docent, a document-level decoder for phrase-based
 *  statistical machine translation.
 *
 *  Docent is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  Docent is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  Docent. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef docent_Logger_h
#define docent_Logger_h

#include "Docent.h"

#include <iostream>

#include <boost/unordered_map.hpp>

enum LogLevel {
	debug,
	verbose,
	normal,
	error
};

class Logger {
private:
	typedef boost::unordered_map<std::string,uint> IndexMap_;
	static IndexMap_ indices_;
	static std::vector<LogLevel> levels_;

	uint index_;

	static uint findChannel(const std::string &channel);

public:
	static void setLogLevel(const std::string &channel, LogLevel level);

	Logger(const std::string &channel);

	bool loggable(LogLevel l) const {
		return l >= levels_[index_];
	}

	std::ostream &getLogStream() const {
		return std::cerr;
	}
};

// beware of double evaluation in the following macro
#define LOG(logger, level) \
	for(bool flagInLoggerMacro = (logger).loggable(level); flagInLoggerMacro; flagInLoggerMacro = false) \
		(logger).getLogStream()

#endif
