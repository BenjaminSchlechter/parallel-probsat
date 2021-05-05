
#include "probsat.hpp"

int main(int argc, char **argv)
{
	auto start = std::chrono::high_resolution_clock::now();
	
	try {
		param_parse_registry["--help"] = &parse_param_help;
		param_parse_registry["-h"] = &parse_param_help;

		param_parse_registry["--maxflips"] = &parse_maxflips_and_seed;
		param_parse_registry["--seed"] = &parse_maxflips_and_seed;

		param_parse_registry["--cb"] = &parse_cb_eps;
		param_parse_registry["--eps"] = &parse_cb_eps;

		param_parse_registry["--runs"] = &parse_num_iterations;

		param_parse_registry["-a"] = &parse_output_solution;
		param_parse_registry["--printSolution"] = &parse_output_solution;

		seed = random_generator_t().get_seed();

		if (2 > argc) {
			std::cerr << "usage: " << argv[0] << " [parameters like --help] boolean_formula.cnf" << std::endl;
			exit(EXIT_FAILURE);
		}

		int params_left = parse_params(argc - 1, &argv[1]);

		if (0 > params_left) {
			exit(EXIT_FAILURE);
		} else if (1 > params_left) {
			std::cerr << "at least one boolean formula is required" << std::endl;
			std::cerr << "usage: " << argv[0] << " [parameters like --help] boolean_formula.cnf" << std::endl;
			exit(EXIT_FAILURE);
		} else if (1 < params_left) {
			std::cerr << "privision of multiple boolean formulas not supported" << std::endl;
			std::cerr << "parsed arguments before: " << argv[argc - params_left] << std::endl;
			std::cerr << "usage: " << argv[0] << " [parameters like --help] boolean_formula.cnf" << std::endl;
			exit(EXIT_FAILURE);
		}

		std::string fname = argv[argc - 1];

		std::cout << "c processing: " << fname << std::endl;
		cnf_formula_t bformula = cnf_formula_t(fname);
		std::cout << "c num vars = " << std::to_string(bformula.num_vars()) << std::endl;
		std::cout << "c clauses = " << std::to_string(bformula.num_clauses()) << std::endl;

		// prob_func_t pfi = poly_prob_func_t();
		prob_func_t pfi = prob_func_t(poly_prob_func_t(eps, cb));
		std::cout << "c algorithm: polynomial with eps = " << std::to_string(eps)
			<< " and cb = " << std::to_string(cb) << std::endl;
		
		auto belegung = serial_instance_t::configuration_type(bformula.num_vars());

		auto imported = std::chrono::high_resolution_clock::now();
		auto import_duration = std::chrono::duration_cast<std::chrono::milliseconds>(imported - start);
		std::cout << "c import duration: " << import_duration.count() << " ms" << std::endl;


		for (std::size_t it = 1; it <= num_iterations; it++) {
			random_generator_t rgen = random_generator_t(seed);
			
			std::cout << "c iteration " << it << " / " << num_iterations << std::endl;
			std::cout << "c seed = " << std::to_string(rgen.get_seed()) << std::endl;

			do_init_configuration(bformula, belegung, rgen);			
			serial_instance_t solver = serial_instance_t(bformula, pfi, belegung, rgen);
			
			auto start_solving = std::chrono::high_resolution_clock::now();
			
			while (!solver.found_solution()) {
				solver.do_flip();
				
				if ((0 < max_flips) && (max_flips <= solver.get_num_flips())) {
					std::cout << "c FOUND NO SOLUTION AFTER " << solver.get_num_flips() << " FLIPS!" << std::endl;
					break;
				}
			}
			
			auto done_solving = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(done_solving - start_solving).count();
			
			auto num_flips = solver.get_num_flips();
			double fps = ((float) num_flips) / duration * 1000000.;
			double fpv = ((float) num_flips) / bformula.num_vars();
			double fpc = ((float) num_flips) / bformula.num_clauses();
			
			std::cout << "c done after " << num_flips << " flips and " << round(duration/1000.) << " milliseconds" << std::endl;
			std::cout << "c => " << round(fps) << " flips per seconds" << std::endl;
			std::cout << "c => " << fpv << " flips per variable" << std::endl;
			std::cout << "c => " << fpc << " flips per clause" << std::endl;
			
			if (solver.found_solution()) {
				assert(solver.check_assignment());
				std::cout << "s SATISFIABLE" << std::endl;

				if (0 < output_solution) {
					for (auto i = 0; i < bformula.num_vars(); i++) {
						if (0 == i % output_solution) {
							if (0 < i) {
								std::cout << std::endl;
							}

							std::cout << "v";
						}

						std::cout << " " << (belegung[i] ? i : -i);
					}
					
					std::cout << std::endl;
				}

				break;
			}
			
			seed = random_generator_t().get_seed();
		}
	} catch (const std::exception& ex) {
		print_exception("main", ex);
	}

	auto done = std::chrono::high_resolution_clock::now();
	auto overall_duration = std::chrono::duration_cast<std::chrono::seconds>(done - start);
	std::cout << "c overall duration: " << overall_duration.count() << " seconds" << std::endl;
	
	return EXIT_SUCCESS;
}

