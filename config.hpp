#ifndef CONFIG_HPP
#define CONFIG_HPP

// contains many program code related configurations

#include <cstdint>

//********************************************************************//
// ProbSAT related:
//********************************************************************//

// for correctness:

// check for common problems related to the input data
#define VERIFY_INPUT 1

// additional sanity checks used while developing to ensure correctness
#define ADDITIONAL_CHECKS 0


// implementation/performance:

// wheter to use a bitfield to store the current cnf formula assignment
#define USE_BITFIELD 0
// affects mainly memory usage

// wheter to cache the critical literal in a clause
#define USE_CRIT_LITERAL_CACHING 1
// important for performance

// wheter to use an alternative datastructure with continuous memory layout
#define USE_CONT_DATASTRUCT 1
// performance

// wheter a literal may appear multiple times in a clause
// not fully compatible with caching, use carefully
#define ALLOW_UNCLEAN_CLAUSES 0

//********************************************************************//
// Parallel implementation only:
//********************************************************************//

// implementation/performance:

// number of probsat threads per worker, was 12 on bwUniCluster
#define NUM_PROBSAT_THREADS_PER_WORKER 2

// usage of shared memory for cnf formula
#define USE_CNF_MULTITHREAD_SHARING 1

// bugfixes:

// if USE_CNF_MULTITHREAD_SHARING is enabled, this fix might be required
// so that the data is loaded from the main thread as memory allocations
// dont work as expected (& specified!) over multiple threads... bad for performance!
#define USE_FIX_CNF_GLOBAL_INPUT_READING 1
// enable if you get Segmentation fault becauso of Address not mapped, like this:
// [BPC:10798] *** Process received signal ***
// [BPC:10798] Signal: Segmentation fault (11)
// [BPC:10798] Signal code: Address not mapped (1)
// [BPC:10798] Failing at address: 0x630f17f38918

#if USE_CNF_MULTITHREAD_SHARING
	// use this config combination at own risk, to compile comment out the following line:
	static_assert(USE_FIX_CNF_GLOBAL_INPUT_READING == 1);
#endif


// unperformant fix for deadlock in OpenMPI when multiple connection attempts on one port happen simultaneos
#define MAX_SIMULTANEOUS_CONNECTION_ATTEMPTS 1

#define USE_CACHED_PROB_FUNC 0
#define USE_PROB_FUNC_LOCAL_CACHE 0


// additional:

// write statistic to stdout
#define OUTPUT_STATISTIC 1

// debug output useful for development, enable from top to bottom
#define DEBUG_COMMUNICATION 0
#define DEBUG_WORKER 0
#define DEBUG_WORKER_OUTPUT 0
#define DEBUG_OUTPUT 0

#endif
