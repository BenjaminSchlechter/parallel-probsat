#ifndef PARSE_PARAMS_HPP
#define PARSE_PARAMS_HPP

#include <queue>
#include <map>
#include <string>
#include <cassert>

typedef void (*param_parse_function)(std::queue<std::string> &);
std::map<std::string, param_parse_function> param_parse_registry;

int parse_params(int argc, char *argv[]) {
	std::queue<std::string> params;
	for (auto i = 0; i < argc; i++) {
		params.push(std::string(argv[i]));
	}

	while (!params.empty()) {
		std::string param = params.front();

		auto fit = param_parse_registry.find(param);
		if (param_parse_registry.end() == fit) {
			return params.size();
			// std::cerr << "unkown parameter " << param << "" << std::endl;
		} else {
			try {
				auto param_func = *fit->second;
				assert(param_func);
				
				param_func(params);
			} catch (const std::exception &e) {
				std::cerr << "error parsing parameter '" << param << "':\n" << e.what() << std::endl;
				return -1;
			}
		}
	}

	return 0;
}

#endif
