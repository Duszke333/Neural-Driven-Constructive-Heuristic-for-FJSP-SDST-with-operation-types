# Neural Driven Constructive Heuristic for FJSP with operation types
## Description
Implementation of the algorithm described in the paper Kaleta,M. and Śliwiński, T.: "A neural-driven constructive heuristic for the flexible
job shop scheduling problem: an efficient alternative to complex deep learning methods".

This is a novel neural-driven constructive heuristic that replaces static priority dispatching rules with a compact, feed-forward neural network. The network evaluates potential operation-machine assignments during the schedule construction process based on features derived from the current partial schedule and job state. 

The version of FJSP considered here, assumes the operations may be grouped into Operation Types, which denote collections of operations that share identical sets of eligible machines
and corresponding processing times. Also, operations in a single job need to be of different Types. 

Operation Types are not as strong requirement as it seems. Actually, as long as the factory setup remains the same, one can assume Operation Types also do not change, 
as they describe set of eligible machines and respective processing times.

## Testing data

The generated synthetic test data used as test instances in the paper are stored in the test_data folder. The training data can be generated using the program. 

## Functionality

The program allows:
* generating fully random sythetic data 
* generating random synthetic training data for a factory setup described by a given Brandimarte test instance
* training the neural network - using training and validation files previously generated into the specified dir 
* testing - solving the test problems in the specified dir using the previously trained neural network 

## Installation

The program uses CMAES library, which should be installed beforehand.

The following installation was tested in Ubuntu 24.04.

```bash
sudo apt-get install git build-essential autoconf automake make libtool libgoogle-glog-dev libgflags-dev libeigen3-dev ninja-build libeigen3-dev git gdb valgrind clang-tidy libboost-all-dev nlohmann-json3-dev libsfml-dev

# install libcmaes
git clone https://github.com/CMA-ES/libcmaes.git

# for cmaes do not use cmake, instead compile with basic options enabled
cd libcmaes
./autogen.sh
echo "#define CMAES_EXPORT" > include/libcmaes/cmaes_export.h
./configure
make

# download and install the program
git clone https://github.com/Decision-Support-and-OR-Group/Neural-Driven-Constructive-Heuristic-for-FJSP-with-operation-types.git

cd Neural-Driven-Constructive-Heuristic-for-FJSP-with-operation-types

# by default CMakeLists.txt expects libcmaes in the parent directory (this repo cloned inside the libcmaes checkout);
# otherwise pass -DCMAES_INCLUDE_DIR=... -DCMAES_LIB_DIR=... to cmake

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -G "Ninja" ..
# cmake -DCMAKE_BUILD_TYPE=Debug -G "Ninja" .. # for debuggable (slow) version
cmake --build .
cd ..
```


### Program usage
Executing:
```bash
./nd-ch-fjsp --help
```

results in the following description of the basic program usage:

```
Usage: ./nd-ch-fjsp [mode] [options]

Global Options:
  -h [ --help ]         Produce help message
  --generate            Mode: Synthetic Data Generation
  --train               Mode: Training
  --test                Mode: Testing

Mode 1 Options (--generate):
  --output_dir arg       Output directory
  --machines arg         Number of machines
  --operation_types arg  Number of operation types
  --jobs_min arg         Min number of jobs
  --jobs_max arg         Max number of jobs
  --job_len_min arg      Min job length
  --job_len_max arg      Max job length
  --num_alt_min arg      Min no of alternative machines
  --num_alt_max arg      Max no of alternative machines
  --t_min arg            Min processing time on a single machine
  --t_max arg            Max processing time on a single machine
  --set_size arg         Number of problems to generate
  --common_seed arg (=1) Seed for common factory setup, shared among all 
                         generated problems
  --seed arg (=1)        Random seed

Mode 2 Options (--generate --brandimarte=ID):
  --brandimarte arg     Brandimarte ID (1-15)
  --output_dir arg      Output directory
  --set_size arg        Number of problems to generate
  --seed arg (=1)       Random seed

Mode 3 Options (--train):
  --files_dir arg                    Directory containing files for training
  --output_dir arg                   Output directory for statistics and NN 
                                     weights
  --val_set_size arg (=10000)        Validation set size
  --layer1 arg (=32)                 Layer 1 size
  --layer2 arg (=16)                 Layer 2 size
  --batch_size arg (=50)             Batch size
  --population arg (=192)            Population size
  --max_evals arg (=500000)          Max evaluations
  --sigma arg (=0.10000000000000001) Sigma
  --seed arg (=1)                    Random seed

Mode 4 Options (--test):
  --files_dir arg                    Directory containing files for testing
  --training_output_dir arg          Directory with saved NN weights
  --output_dir arg                   Output directory
  --population arg (=192)            Population size
  --max_evals arg (=500000)          Max evaluations
  --sigma arg (=0.10000000000000001) Sigma
  --schedules                        Save schedules
  --graphics                         Show graphics
  --time_limit arg (=60)             Time limit (s)
  --seed arg (=1)                    Random seed
```
### Examples
Typical workflow with the program looks as follows:

1. Generate random synthetic training problems and save them into 'training-problems' directory.
```
./nd-ch-fjsp --generate --output_dir=training-problems --machines=10 --operation_types=15 --jobs_min=20 --jobs_max=30 --job_len_min=5 --job_len_max=10 --num_alt_min=1 --num_alt_max=3 --t_min=10 --t_max=100 --set_size=100000 --common_seed=7 --seed=5000
```

2. Train new network on generated problems. Save the network in the 'trained-network' directory.
```
./nd-ch-fjsp --train --files_dir=training-problems --output_dir=trained-network --val_set_size=10000 --max_evals=500000
```

3. Generate smaller set of test problems and save them into 'test-problems' directory. Important is, that most options, with exception to 'seed' (but also 'jobs_min' and 'jobs_max') are the same as in the training set, as those values describe unchanged factory setup.
```
./nd-ch-fjsp --generate --output_dir=test-problems --machines=10 --operation_types=15 --jobs_min=20 --jobs_max=30 --job_len_min=5 --job_len_max=10 --num_alt_min=1 --num_alt_max=3 --t_min=10 --t_max=100 --set_size=100 --common_seed=7 --seed=100000
```

4. Test the constructive heuristic utilizing previously trained network in the 'trained-network' directory on a set of test problems in the 'test-problems' directory. Store results in the 'test-results' directory.
```
./nd-ch-fjsp --test --output_dir=test-results --files_dir=test-problems --training_output_dir=trained-network --time_limit=100 --schedules --graphics
```
