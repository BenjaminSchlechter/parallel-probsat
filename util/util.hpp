#ifndef UTIL_HPP
#define UTIL_HPP

// Hilfsfunktionen

#include <vector>
#include <sstream>
#include <iterator>
#include <string>
#include <iostream>

bool starts_with(std::string a, std::string b) {
	#if __cplusplus > 201703L
		return a.starts_with(b);
	#else 
		if (a.find(b) == 0) {
			return true;
		}
		
		return false;
	#endif
}

bool ends_with(std::string a, std::string b) {
	#if __cplusplus > 201703L
		return a.ends_with(b);
	#else 
		if (a.find(b) == a.length()-b.length()) {
			return true;
		}
		
		return false;
	#endif
}

template<class T, typename... Args> void ignore(const T&, Args...) { }
void print_exception(std::string info, const std::exception& e, std::size_t level = 0) {
    std::cerr << std::string(level, '\t') << info << " exception: " << e.what() << '\n';
    try {
        std::rethrow_if_nested(e);
    } catch(const std::exception& e) {
        print_exception(info, e, level+1);
    } catch(...) {
		throw std::runtime_error("found unexpected exception while recursing nested exceptions");
	}
}

void split(const std::string &string, std::vector<std::string> &result)
{
    std::istringstream iss(string);
    std::copy(std::istream_iterator<std::string>(iss),
		std::istream_iterator<std::string>(), std::back_inserter(result));
}

#endif
