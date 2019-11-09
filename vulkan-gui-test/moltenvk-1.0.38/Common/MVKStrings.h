/*
 * MVKStrings.h
 *
 * Copyright (c) 2014-2019 The Brenwill Workshop Ltd. (http://www.brenwill.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MVKStrings_h_
#define __MVKStrings_h_ 1

#include <string>
#include <streambuf>

namespace mvk {

    static std::string _mvkDefaultWhitespaceChars = " \f\n\r\t\v";

    /** Returns a string with whitespace trimmed from the right end of the specified string. */
    inline std::string trim_right(const std::string& s, const std::string& delimiters = _mvkDefaultWhitespaceChars) {
        size_t endPos = s.find_last_not_of(delimiters);
        return (endPos != std::string::npos) ? s.substr(0, endPos + 1) : "";
    }

    /** Returns a string with whitespace trimmed from the left end of the specified string. */
    inline std::string trim_left(const std::string& s, const std::string& delimiters = _mvkDefaultWhitespaceChars) {
        size_t startPos = s.find_first_not_of(delimiters);
        return (startPos != std::string::npos) ? s.substr(startPos) : "";
    }

    /** Returns a string with whitespace trimmed from both ends of the specified string. */
    inline std::string trim(const std::string& s, const std::string& delimiters = _mvkDefaultWhitespaceChars) {
        size_t startPos = s.find_first_not_of(delimiters);
        size_t endPos = s.find_last_not_of(delimiters);
        return ( (startPos != std::string::npos) && (endPos != std::string::npos) ) ? s.substr(startPos, endPos + 1) : "";
    }

	/** A memory-based stream buffer. */
	class membuf : public std::streambuf {
	public:
		membuf(char* p, size_t n) {
			setg(p, p, p + n);
			setp(p, p + n);
		}
	};

	/** A character counting stream buffer. */
	class countbuf : public std::streambuf {
	public:
		size_t buffSize = 0;
	private:
		std::streamsize xsputn (const char* /* s */, std::streamsize n) override {
			buffSize += n;
			return n;
		}
	};

}

#endif
