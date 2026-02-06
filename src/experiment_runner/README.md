# Modular EM Experiment Framework

This directory contains the modular experiment framework for running EM algorithms on graph parsing tasks.

## Architecture

### Core Components

1. **ExperimentConfig** (`experiment_config.hpp/cpp`)
   - Configuration class for single experiments
   - Auto-detects input files based on directory structure
   - Manages output paths and algorithm parameters

2. **ExperimentRunner** (`experiment_runner.hpp/cpp`)
   - Main experiment execution engine
   - Supports all EM algorithm variants (batch, viterbi, online, full)
   - Handles data loading, algorithm execution, evaluation, and result saving
   - Reusable across different experiment configurations

### Directory Structure Expected

The framework expects experiment directories with this structure:
```
experiment_dir/
├── {name}.mapping.txt     # SHRG rules file
├── {name}.graphs.txt      # Graphs file
└── induced_outputs/       # Created automatically for results
    ├── {algorithm}_derivations.txt
    ├── {algorithm}_evaluation.json
    └── {algorithm}_log.txt
```

For example:
```
/path/to/incremental/.../by_ages/10/
├── 10.mapping.txt
├── 10.graphs.txt
└── induced_outputs/
    ├── batch_em_derivations.txt
    ├── batch_em_evaluation.json
    ├── viterbi_em_derivations.txt
    └── viterbi_em_evaluation.json
```

## Main Executables (in parser/src/)

### 1. `run_em_experiment`
Run a single EM experiment on one directory.

```bash
# Run batch EM on experiment folder 10
./run_em_experiment batch_em /path/to/incremental/.../by_ages/10

# Run all algorithms
./run_em_experiment all /path/to/incremental/.../by_ages/10

# With custom parameters
./run_em_experiment batch_em /path/to/experiment --batch-size 10 --max-iterations 100
```

**Usage:**
```
./run_em_experiment <algorithm> <experiment_directory> [options]

Algorithms:
  batch_em     - Batch EM algorithm
  viterbi_em   - Viterbi EM algorithm
  online_em    - Online EM algorithm
  full_em      - Full EM algorithm
  all          - Run all algorithms

Options:
  --batch-size <size>      Batch size for batch EM (default: 5)
  --max-iterations <num>   Maximum iterations (default: 50)
  --threshold <value>      Convergence threshold (default: 0.01)
  --parser-type <type>     Parser type (default: tree_v2)
  --timeout <seconds>      Timeout in seconds (default: 300)
  --no-evaluation          Skip evaluation step
  --quiet                  Reduce output verbosity
```

### 2. `run_all_experiments`
Run experiments for all folders in a base directory.

```bash
# Run batch EM on all experiment folders
./run_all_experiments /path/to/incremental/.../by_ages batch_em

# Run all algorithms on all folders
./run_all_experiments /path/to/incremental/.../by_ages all
```

**Usage:**
```
./run_all_experiments <base_directory> [algorithm]

Arguments:
  base_directory    Path to directory containing experiment folders
  algorithm         Algorithm to run (default: all)
```

### 3. `run_evaluation_only`
Run evaluation on experiments that have already completed EM training.

```bash
# Run evaluation on existing results
./run_evaluation_only /path/to/incremental/.../by_ages/10
```

## Key Features

1. **Modular Design**: Algorithms, configuration, and execution are separated
2. **Reusable Components**: Same framework works for all EM variants
3. **Auto-Discovery**: Automatically detects input files based on naming convention
4. **Comprehensive Evaluation**: BLEU scores, F1 scores, derivation comparison
5. **Error Handling**: Robust error checking and reporting
6. **Flexible Configuration**: Command-line parameter override
7. **Clean Output**: Organized results in `induced_outputs/` subdirectories

## Algorithm Support

- **Batch EM**: `em_framework/em_batch.hpp`
- **Viterbi EM**: `em_framework/em_viterbi.hpp`
- **Online EM**: `em_framework/em_online.hpp`
- **Full EM**: `em_framework/em.hpp`

Each algorithm is created through factory methods in `ExperimentRunner` and follows the same `EMBase` interface.

## Output Files

For each algorithm, the framework generates:

1. **Derivation Files**: `{algorithm}_derivations.txt`
   - Rule indices for each graph in the format expected by evaluation

2. **Evaluation Files**: `{algorithm}_evaluation.json`
   - BLEU scores, F1 scores, tree comparison metrics

3. **Log Files**: `{algorithm}_log.txt`
   - Execution logs and timing information

## Integration with Existing Evaluation

The framework generates derivation files in the same format as the existing evaluation pipeline:
- Compatible with `fla/src/eval_deriv.py`
- Uses same rule index format for parsing accuracy evaluation
- Supports both greedy and inside decoding evaluation