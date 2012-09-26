/*
 *  Logger.cpp
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

#include "Logger.h"

Logger::IndexMap_ Logger::indices_;
std::vector<LogLevel> Logger::levels_;

uint Logger::findChannel(const std::string &channel) {
	uint idx;

	IndexMap_::const_iterator it = indices_.find(channel);
	if(it == indices_.end()) {
		idx = levels_.size();
		levels_.push_back(normal);
		indices_.insert(std::make_pair(channel, idx));
	} else
		idx = it->second;

	return idx;
}

Logger::Logger(const std::string &channel) : index_(findChannel(channel)) {}

void Logger::setLogLevel(const std::string &channel, LogLevel level) {
	uint idx = findChannel(channel);
	levels_[idx] = level;
}
