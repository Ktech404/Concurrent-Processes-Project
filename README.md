# Concurrent-Processes-Project

## 1. Project Directory Structure

```text
project/
├── bin/                 # compiled executables
├── src/                 # main program source files
├── checker/             # trace checker source file
├── config/              # workload configuration files
├── evidence/            # testing evidence and debugging investigations
├── traces/              # generated CSV execution traces
├── .gitignore
└── README.md
```

## 2. Requirements

The project requires:

- GCC (GNU Compiler Collection) version 11.4.0
- POSIX threads (`pthread`).
- A POSIX-compatible environment supporting the required threading and synchronization facilities.
- A terminal or shell for compiling and running the programs.

The compiler version used for development and testing should be recorded here.

## 3. Compilation and Clean Builds

To compile the executable, run from the project root directory:

```bash
gcc -Wall -Wextra -std=c11 -pthread src/*.c -o bin/program
```


## 4. Running the Programs

Run the compiled program from the project root:

```bash
./bin/program
```

From there, you will get a numbered command-line menu, where you can change the modes(but not the config and/or trace files), run the race condition, and run the checker. Or quit. There is no other commands needed. If you would like to change the config files the different queue modes run on, on the config+trace files the checker runs on, you will need to change the file paths in the source code, the the intruction for doing so are outlined below.

Please consider them very careful. The procress is easy, but the names must match carefully to avoid errors.

### Changing the config/trace files

**CV/Sem modes:**

The compiled program uses the configuration file selected in its source code. If you would like to change the input config file for either sem/cv mode, you will have to edit the relevant configuration-file path in the source code and recompile the program. *You will also need to change the name of the output trace file, to avoid confusion!*

Below, I will put the corresponding files/lines to change for both modes, if you would like to test something:

For the cv mode, change the input file in main.c line 40:

```c
if (!read_config("config/public_case.txt", &config))
```

Change this to one of the other config files in the config/folder, or add your own file to that folder. But be sure to change the output trace file name to match, so you know what input corresponds to what trace.

You can change this in cv_mode.c line 298:

```c
write_trace("trace/cv_public_case_events.csv", queue.events, queue.event_count);
```

The same directions apply for semaphores mode. To change the input config file, change line 55 in main.c, and to change the output trace file, change line 298 in sem_mode.c. Make sure these name correspond, you can correctly correspond the input configuration to the output trace.

**Checker:**
The same approach applies when selecting a different configuration for trace testing. In checker/checker.c, the relevant lines are 8-10:

```c
// input files
const char *config_file = "config/public_case.txt";
const char *trace_file = "traces/cv_public_case_events.csv";
```

As you can see, the checker file takes the config and trace files as inputs. I have already run the sem+cv mode for all of the config files, and named the corresponding checker files so that you can easily see which ones match.

It is my strong recommendation that you only, if you would like to, change the file paths/names above to the ones that are already found in the config/ and trace/ folder, instead of running the commands again and printing new trace files, unless, as outlined in the above section, **you make sure to name the output trace file in cv_mode.c and sem_mode.c to match whatever you change the input config file to**. The config/trace files work as pairs for the input in checker.c.

The default input config file and output trace files correspond to the public case.

## 5. Configuration Files

Configuration files specify the workload parameters:

| Parameter | Description |
|---|---|
| `seed` | Seed used to generate deterministic job values |
| `jobs` | Number of jobs to be produced |
| `producers` | Number of producer threads |
| `consumers` | Number of consumer threads |
| `capacity` | Maximum number of items in the bounded queue |
| `delay_ms` | Configured delay in milliseconds |

All configuration files must contain valid values for the required parameters.

The config/ folder contains configs corresponding to all of the required tests cases outline in the test matrix. They are all named according to their contents, so it should be pretty intuitive, ex. p_workload.txt = "personal workload".

The folder also includes a folder invalids/, which includes 4 invalid test cases the program should reject, which were inlcuded in the project instructions. Keep this in mind as you are/if you are changing the file paths.

Changing the input config files for the different modes is outlined above in section 4.

## 7. Trace Generation

The program generates a CSV execution trace recording queue and worker events.

The trace contains the following columns:

| Column | Description |
|---|---|
| `seq` | Sequential event number |
| `actor` | Producer, consumer, or MAIN responsible for the event |
| `event` | Event type |
| `job_id` | Job identifier or negative STOP identifier |
| `occupancy` | Queue occupancy after the event |
| `result` | Completed job result, or `-` when not applicable |

The trace files are stored in the `traces/` directory.

Similar to the config files, they are all names accordingly. I already ran both program modes on all of the input config files(the config files in "invalid/" do not having corresponding output trace files because the program will not, and is not supposed to, run malformed input files.), and you can clearly see the mode and input config file in the name. Ex:

- input config file = p_workload.txt
- output trace file for CV mode = cv_p_workload_events.csv
- output trace file for SEM mode = sem_p_workload_events.csv

Notice how the input and output file names corresponding, so you know which two to feed into the checker, and also which mode ran them. More detailed instructions for changing the file paths are outlined above in section 4.

## 8. Independent Trace Checker

The checker validates a CSV trace against its selected configuration file.

It checks the event fields, actor permissions, sequence numbers, job ownership, result calculations, duplicate and missing job events, consumer processing order, FIFO queue behaviour, queue occupancy, and shutdown protocol.

In the traces/ folder, is the corrupted/ folder. This folder contain 2 trace files that are based off the public_case.txt config file(ran with cv mode), and they are intentionaly are malformed, according to the project instructions. The deliberately introduced errors are indicated by the file names. If you run these files in the checker(paired with public_case.txt as the config file), you will see the corresponding error messages telling you what is wrong with them

To change the configuration or trace file being checked, edit the corresponding file paths in the checker source code and recompile it, this process is outlined above in section 4.

### Running the checker

A successful validation produces a `PASS` message and exits with status `0`. An invalid trace produces a `FAIL` message identifying the event or end-of-file condition and exits with a non-zero status.

## 9. Testing

The project was tested using multiple workload configurations and both queue mode implementations, as mentioned earlier.

Testing included:

- Different job counts and queue capacities.
- Different numbers of producers and consumers.
- Small queue capacities, including capacity one.
- Zero-job workloads.
- Workloads with more producers or consumers than jobs.
- Validation of traces generated by both synchronization implementations.
- Deliberately corrupted traces to verify that the independent checker rejects invalid events.

The original valid traces were retained as testing evidence, this is found in the traces/ folder. Furthermore, the evidence/ folder contains important testing results!

The required_tests_printed_output.pdf is a file containing the output printed to the terminal for each config file and queue mode. This is important because some of the invalid config files DO NOT HAVE CORRESPONDING TRACE FILES because they are malformed and we never run the modes on them.

PLEASE NOTE: This document also contains the two required checks for the checker.c file, which comes from the corrupted folder in the traces/ folder.

This document contains screenshots of the output that is printed to the terminal, although not neccesarily to a file as well. You are able to confirm everything is correct by running the code and files yourself. 

## 10. Limitations

- Configuration and trace file paths are selected by editing source-code constants; they are not supplied as command-line arguments.
- The programs must be saved AND recompiled after changing the selected file paths.
- The trace checker validates a recorded execution. A passing trace demonstrates that the recorded execution satisfies the checker rules, but does not prove correctness under every possible thread schedule. It also does not print the pass/failure to an external file
- The implementation depends on POSIX threading and synchronization facilities and is not intended to be directly portable to environments without them.
