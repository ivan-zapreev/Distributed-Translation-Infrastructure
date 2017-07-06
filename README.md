# **Distributed Translation Infrastructure**

**Author:** [Dr. Ivan S. Zapreev](https://nl.linkedin.com/in/zapreevis)

**Project pages:** [Git-Hub-Project](https://github.com/ivan-zapreev/Basic-Translation-Infrastructure)

## Introduction
This project contains a distributed phrase-based statistical machine translation infrastructure consisting of load ballancing, text pre/post-processing and translation services. The software is written in C++ 11 and follows the client/server architecture based on JSON over WebSockets. Along with scaling by introducing multiple services behind load ballancers, it fully utilizes multicore CPUs by employing multi-threading. The infrastructure consists of the following applications:

+ **bpbd-server** - the translation server consisting of the following main components:
    - *Decoder* - the decoder component responsible for translating text from one language into another
    - *LM* - the language model implementation allowing for seven different three implementations and responsible for estimating the target language phrase probabilities
    - *TM* - the translation model implementation required for providing source to target language phrase translation and the probabilities thereof
    - *RM* - the reordering model implementation required for providing the possible translation order changes and the probabilities thereof
+ **bpbd-balancer** - the load balancer that has the same WebSockets interface as **bpbd-server** and is supposed to distribute load balance between multiple translation server instances.
+ **bpbd-processor** - the text pre/post-processing server that allows to employ various third party text processing tools for different languages. Its purpose is to prepare text for translation and to post process the translated text to make it more readable.
+ **bpbd-client** - a thin client to send the translation job requests to the translation server and obtain results
+ **translate.html** - a thin web client to send the translation job requests to the translation server and obtain results
+ **lm-query** - a stand-alone language model query tool that allows to perform language model queries and estimate the joint phrase probabilities

### Introduction to phrase-based SMT

To keep a clear view of the used terminology further, we provide some details on the topic of phrase-based SMT, and to illustrate it, the following picture.

![Statistical Machine Translation Approach Image](./doc/images/smt/smt.jpg "Statistical Machine Translation Approach")

The entire phrase-based statistical machine translation relies on learning statistical correlations between words and phrases in an existing source/target translation text pair, also called parallel corpus or corpora. These correlations are learned by, generally speaking, three statistical models: TM - translation model; RM - reordering mode; and LM - language model; briefly mentioned above. If the training corpora is large enough, then these models will possess enough information to approximate a translation of an arbitrary text from the given source language to the given target language. Note that, before this information can be extracted, the parallel corpora undergoes the process called _word alignment_ which is aimed at estimating which words/phrases in the source language correspond to which words/phrases in the target language. Let us give a more precise definition of these models:

1. Translation model - provides phrases in the source language with learned possible target language translations and the probabilities thereof.
2. Reordering model - stores information about probable translation orders of the phrases within the source text, based on the observed source and target phrases and their alignments.
3. Language model - reflects the likelihood of this or that phrase in the target language to occur. In other words, it is used to evaluate the obtained translation for being _"sound"_ in the target language.

Note that, the language model is typically learned from a different corpus in a target language.

With these three models at hand one can perform decoding, which is a synonym to a translation process. SMT decoding is performed by exploring the state space of all possible translations and reordering of the source language phrases within one sentence. The purpose of decoding, as indicated by the maximization procedure at the bottom of the figure above, is to find a translation with the largest possible probability.

In order to obtain the best performance of the translation system one can employ Discriminative Training. The latter uses generated word lattice to optimize translation performance by reducing some measure of translation error. This is done by tuning the translation parameters such as feature lambda values of the model feature weights. Our software allows for word lattice generation and parameters' tuning, as discussed in sections [Building the project](#building-the-project) and [Using software](#using-software).

### Document structure
 
The rest of the document is organized as follows:

1. [Project structure](#project-structure) - Gives the file and folder structure of the project
2. [Supported platforms](#supported-platforms) - Indicates the project supported platforms
3. [Building the project](#building-the-project) - Describes the process of building the project
4. [Using software](#using-software) - Explains how the software is to be used
5. [Input file formats](#input-file-formats) - Provides examples of the input file formats
6. [Code documentation](#code-documentation) - Refers to the project documentation
7. [Third-party software](#third-party-software) - Lists the included third party software
8. [Performance evaluation](#performance-evaluation) - Contains performance evaluation results
9. [General design](#general-design) - Outlines the general software design
10. [Communication protocols](#communication-protocols) - Describes the application's communications
11. [Software details](#software-details) - Goes about some of the software details
12. [Literature and references](#literature-and-references) - Presents the list of used literature
13. [Licensing](#licensing) - States the licensing strategy of the project
14. [History](#history) - Stores a short history of this document

## Project structure
This is a Netbeans 8.0.2 project, based on cmake, and its top-level structure is as follows:

* **`[Project-Folder]`**/
    * **`doc/`** - project-related documentation
    * **`ext/`** - external libraries used in the project
    * **`inc/`** - C++ header files
    * **`src/`** - C++ source files
    * **`data/`** - stores the tests-related data
    * **`nbproject/`** - Netbeans project data
    * **`script/`** - stores the various scripts
    * **`script/web/`** - Web client for translation system
    * **`script/text/`** - Pre/post-processing scripts
    * **`script/text/truecase`** - True-casing scripts
    * **`script/text/truecase/moses`** - True-casing scripts from Moses
    * **`script/text/truecase/truecaser`** - True-casing scripts from Truecaser
    * **`script/test/`** - Scripts used for testing
    * **`script/tuning/`** - Decoder parameter's tuning scripts
    * **`script/tuning/PRO`** - PRO-14 optimization scripts
    * **`script/tuning/megam_0.92`** - MEGA Model Optimization Package (MegaM)
    * **`script/tuning/scripts`** - Core tuning scripts and utilities
    * `server.cfg` - example server configuration file
    * `balancer.cfg` - example load balancer configuration file
    * `processor.cfg` - example processor configuration file
    * `LICENSE` - code license (GPL 2.0)
    * `CMakeLists.txt` - cmake build script
    * `README.md` - this document
    * `Doxyfile` - Doxygen configuration file

## Supported platforms
This project supports two major platforms: Linux and Mac Os X. It has been successfully build and tested on:

* **Centos 6.6 64-bit** - Complete functionality.
* **Ubuntu 15.04 64-bit** - Complete functionality.
* **Mac OS X Yosemite 10.10 64-bit** - Limited by inability to collect memory-usage statistics.

**Notes:**

1. There was only a limited testing performed on 32-bit systems.
2. The project is **not possible** to build on Windows platform even under [Cygwin](https://www.cygwin.com/).

## Building the project
Building this project requires **gcc** version >= *4.9.1* and **cmake** version >= 2.8.12.2.

The first two steps before building the project, to be performed from the Linux command line console, are:

1. `cd [Project-Folder]`
2. `mkdir build`

After these are performed, the project can be build in two ways:

+ From the Netbeans environment by running Build in the IDE
    - In Netbeans menu: `Tools/Options/"C/C++"` make sure that the cmake executable is properly set.
    - Netbeans will always run cmake for the DEBUG version of the project
    - To build project in RELEASE version use building from Linux console
+ From the Linux command-line console by following the next steps
    - `cd [Project-Folder]/build`
    - `cmake -DCMAKE_BUILD_TYPE=Release ..` OR `cmake -DCMAKE_BUILD_TYPE=Debug ..`
    - `make -j [NUMBER-OF-THREADS]` add `VERBOSE=1` to make the compile-time options visible

The binaries will be generated and placed into *./build/* folder. In order to clean the project from the command line run `make clean`. Cleaning from Netbeans is as simple calling the `Clean and Build` from the `Run` menu.

In order to perform parameter tuning for the decoding server, one also needs to build the MegaM code delivered with this project. The steps needed for that are listed below. Please note that the first step is optional and is only needed in case *A)* the Ocam compiler is not present *B)* the present Ocam compiler can not build MegaM.

* Download and install the latest [Ocam compiler](http://www.ocaml.org/releases/4.04.html), we used version `4.04`. By default, installation will require root rights. If Ocam is installed locally then one needs to modify the MegaM's makefile to account for that, see below;

```
wget http://caml.inria.fr/pub/distrib/ocaml-4.04/ocaml-4.04.0.tar.gz;
tar -zxvf ocaml-4.04.0.tar.gz;
cd ocaml-4.04.0;
./configure;
make world;
make opt;
make opt.opt;
sudo make install;
```

* If the Ocam libraries are not installed into the `/usr/local/lib/ocaml` folder then update the value of the `OCAML_HOME` variable in the `[Project-Folder]/script/tuning/megam_0.92/Makefile` file with the proper Ocam library path. Also make sure that the compiler is in the `PATH`;

* Build the MegaM optimal executable by running:

```
cd [Project-Folder]/script/tuning/megam_0.92;
make opt;
```

### Project compile-time parameters
For the sake of performance optimizations, the project has a number of compile-time parameters that are to be set before the project is build and can not be modified in the run time. Let us consider the most important of them and indicate where all of them are to be found.

**Tuning mode:** The software can be compiled in the tuning mode, which supports the word lattice generation for the performance tuning of the translation system. The performance is measured in terms of the translation quality measure such as BLEU. When the software is compiled in the tuning mode, it is a number of times slower than in the regular, i.e. production, mode. Enabling of the tuning mode can be done by setting the value of the `IS_SERVER_TUNING_MODE` macro, in the `./inc/server/server_configs.hpp` file, to `true` and then re-compiling the software. The tuning mode only has impact on the **bpbd-server** executable. The lattice dumping is then not immediately enabled and configured. This is to be done via the server's [Configuration file](#server-config-file). For more details on word lattice see section [Word lattice generation](#word-lattice-generation).

**Logging level:** Logging is important when debugging software or providing an additional user information during the program's run time. Yet additional output actions come at a price and can negatively influence the program's performance. This is why it is important to be able to disable certain logging levels within the program not only during its run time but also at compile time. The possible range of project's logging levels, listed incrementally, is: ERROR, WARNING, USAGE, RESULT, INFO, INFO1, INFO2, INFO3, DEBUG, DEBUG1, DEBUG2, DEBUG3, DEBUG4. One can limit the logging level range available at run time by setting the `MAXIMUM_LOGGING_LEVEL` constant value in the `./inc/common/utils/logging/logger.hpp` header file. The default value is INFO3.

**Sanity checks:** When program is not running as expected, it could be caused by the internal software errors that are potentially detectable at run time. This software has a number of build-in sanity checks that can be enabled/disabled at compile time by setting the `DO_SANITY_CHECKS` boolean flag in the `./inc/common/utils/exceptions.hpp` header file. Note that enabling the sanity checks does not guarantee that the internal error will be found but will have a negative effect on the program's performance. Yet, it might help to identify some of the errors with e.g. input file formats and alike.

**Server configs:** There is a number of translation server common parameters used in decoding, translation, reordering and language models. Those are to be found in the `./inc/server/server_configs.hpp`:

* `UNKNOWN_LOG_PROB_WEIGHT` - The value used for the unknown probability weight _(log\_e scale)_
* `ZERO_LOG_PROB_WEIGHT` - The value used for the 'zero' probability weight _(log\_e scale)_
* `tm::MAX_NUM_TM_FEATURES` - Defines the maximum allowed number of the translation model features to be read, per translation target, from the model input file
* `tm::TM_MAX_TARGET_PHRASE_LEN` - The maximum length of the target phrase to be considered, this defines the maximum number of tokens to be stored per translation entry
* `lm::MAX_NUM_LM_FEATURES` - The maximum allowed number of language model features, the program currently supports only one value: `1`, which is also the minimum allowed number of features
* `lm::LM_M_GRAM_LEVEL_MAX` - The language model maximum level, the maximum number of words in the language model phrase
* `lm::LM_HISTORY_LEN_MAX` - **do not change** this parameter 
* `lm::LM_MAX_QUERY_LEN` - **do not change** this parameter 
* `lm::DEF_UNK_WORD_LOG_PROB_WEIGHT` - The default unknown word probability weight, for the case the `<unk>` entry is not present in the language model file _(log\_e scale)_
* `rm::MAX_NUM_RM_FEATURES` - Defines the maximum allowed number of the reordering model features to be read, per reordering entry, from the model input file. The number of features must be even, the maximum supported number of features is currently `8`.

**Decoder configs:** The decoder-specific parameters are located in `./inc/server/decoder/de_configs.hpp`:

* `MAX_WORDS_PER_SENTENCE` - The maximum allowed number of words/tokens per sentence to translate.

**LM configs:** The Language-model-specific parameters located in `./inc/server/lm/lm_configs.hpp`:

* `lm_word_index` - the word index type to be used, the possible values are:
     * `basic_word_index` - the basic word index that just loads the uni-grams in the same order as in the LM model file and gives them consecutive id values.
     * `counting_word_index` - the basic word index that counts the number of times the uni-gram occurs in the LM model file and gives lower ids to the more frequent uni-grams. This ensures some performance boost (within 10%) when querying certain types of language models but requires longer loading times.
     * `optimizing_word_index<basic_word_index>` - the optimizing word index is based on the linear probing hash map so it is the fastest, it uses a basic word index as a bootstrap word index for issuing the ids.
     * `optimizing_word_index<counting_word_index>` - the optimizing word index is based on the linear probing hash map so it is the fastest, it uses a counting word index as a bootstrap word index for issuing the ids.
     * `hashing_word_index` - the hashing word index is a discontinuous word index that does not issue the uni-gram ids consequently but rather associates each uni-gram with its hash value, the latter is taken to be a unique identifier. This is the only type of index supported by the hash-based `h2d_map_trie`.
* `lm_model_type` - the trie model type to be used, the possible values (trie types) are as follows, for a performance comparison thereof see [Performance Evaluation](#performance-evaluation):
     * `c2d_hybrid_trie<lm_word_index>` - contains the context-to-data mapping trie implementation based on `std::unordered` map and ordered arrays
     * `c2d_map_trie<lm_word_index>` - contains the context-to-data mapping trie implementation based on `std::unordered map`
     * `c2w_array_trie<lm_word_index>` - contains the context-to-word mapping trie implementation based on ordered arrays
     * `g2d_map_trie<lm_word_index>` - contains the m-gram-to-data mapping trie implementation based on self-made hash maps
     * `h2d_map_trie<lm_word_index>` - contains the hash-to-data mapping trie based on the linear probing hash map implementation
     * `w2c_array_trie<lm_word_index>` - contains the word-to-context mapping trie implementation based on ordered arrays
     * `w2c_hybrid_trie<lm_word_index>` - contains the word-to-context mapping trie implementation based on `std::unordered` map and ordered arrays
* `lm_model_reader` - the model reader is basically the file reader type one can use to load the model, currently there are three model reader types available, with `cstyle_file_reader` being the default:
     * `file_stream_reader` - uses the C++ streams to read from files, the slowest
     * `cstyle_file_reader` - uses C-style file reading functions, faster than `file_stream_reader`
     * `memory_mapped_file_reader` - uses memory-mapped files which are faster than the `cstyle_file_reader` but consume twice the file size memory (virtual RAM).
* `lm_builder_type` - currently there is just one builder type available: `lm_basic_builder<lm_model_reader>`.

Note that not all of the combinations of the `lm_word_index` and `lm_model_type` can work together, this is reported run time after the program is build. Some additional details on the preferred configurations can be also found in the `./inc/server/lm/lm_consts.hpp` header file comments. The default, and the most optimal performance/memory ratio configuration, is:

* `lm_word_index` being set to `hashing_word_index`
* `lm_model_type` begin set to `h2d_map_trie<lm_word_index>`.

**TM configs:** The Translation-model-specific parameters are located in `./inc/server/tm/tm_configs.hpp`:

* `tm_model_type` - currently there is just one model type available: `tm_basic_model`
* `tm_model_reader` - the same as `lm_model_reader` for _"LM configs"_
* `tm_builder_type` - currently there is just one builder type available: `tm_basic_builder<tm_model_reader>`

**RM configs:** The Reordering-model-specific parameters are located in `./inc/server/rm/rm_configs.hpp`:

* `rm_model_type` - currently there is just one model type available: `rm_basic_model`
* `rm_model_reader` - the same as `lm_model_reader` for _"LM configs"_
* `rm_builder_type` - currently there is just one builder type available: `rm_basic_builder<rm_model_reader>`

## Using software
This section briefly covers how the provided software can be used for performing text translations. We begin with **bpbd-server**, **bpbd-balancer** and **bpbd-processor**. Next, we talk about the client applications **bpbd-client** and Web UI. Further, we briefly talk about the language model query tool **lm-query**. Finally, we explain how the decoder's parameter tuning can be done. The latter is an iterative process allowing to obtain the best performance of the translation system in terms of the BLEU scores.

For information on the LM, TM and RM model file formats and others see section [Input file formats](#input-file-formats)

### Translation server: _bpbd-server_
The translation server is used for two things:
_(i)_ to load language, translation and reordering models (for a given source/target language pair); 
_(ii)_ to process the translation requests coming from the translation client.
The use of this executable is straightforward. When started from a command line without any parameters, **bpbd-server** reports on the available command-line options:

```
$ bpbd-server
<...>
PARSE ERROR:  
             Required argument missing: config

Brief USAGE: 
   bpbd-server  [-d <error|warn|usage|result|info|info1|info2|info3>] -c
                <server configuration file> [--] [--version] [-h]

For complete USAGE and HELP type: 
   bpbd-server --help
```
As one can see the only required command-line parameter of the translation server is a configuration file. The latter shall contain the necessary information for loading the models, and running the server. The configuration file content is covered in section [Configuration file](#server-config-file) below. Once the translation server is started there is still a way to change some of its run-time parameters. The latter can be done with a server console explained in the [Server console](#server-console) section below. In addition, for information on the LM, TM and RM model file formats see the [Input file formats](#input-file-formats)

#### Server config file
In order to start the server one must have a valid configuration file for it. The latter stores the minimum set of parameter values needed to run the translation server. Among other things, this config file specifies the location of the language, translation and reordering models, the number of translation threads, and the WebSocket port through which the server will accept requests. An example configuration file can be found in: `[Project-Folder]/server.cfg` and in `[Project-Folder]/data`. The content of this file is self explanatory and contains a significant amount of comments.

When run with a properly formed configuration file, **bpbd-server** gives the following output. Note the `-d info1` option ensuring additional information output during loading the models.

```
$ bpbd-server -c ../data/default-1-3.000.000.cfg -d info1
<...>
USAGE: The requested debug level is: 'INFO1', the maximum build level is 'INFO3' the set level is 'INFO1'
USAGE: Loading the server configuration option from: ../data/default-1-10.000.cfg
INFO: The configuration file has been parsed!
USAGE: Translation server from 'German' into 'English' on port: '9002' translation threads: '25'
INFO: LM parameters: [ conn_string = ../data/models/e_00_1000.lm, lm_feature_weights[1] = [ 0.200000 ] ]
INFO: TM parameters: [ conn_string = ../data/models/de-en-1-10.000.tm, tm_feature_weights[5] = [ 1.000000|1.000000|1.000000|1.000000|1.000000 ], tm_unk_features[5] = [ 1.000000|1.000000|0.000000|1.000000|1.000000 ], tm_trans_lim = 30, tm_min_trans_prob = 1e-20 ]
INFO: RM parameters: [ conn_string = ../data/models/de-en-1-10.000.rm, rm_feature_weights[6] = [ 1.000000|1.000000|1.000000|1.000000|1.000000|1.000000 ] ]
WARN: The de_is_gen_lattice is set to true in a non-training mode server compilation, re-setting to false!
INFO: DE parameters: [ de_dist_lim = 5, de_lin_dist_penalty = 1, de_pruning_threshold = 0.1, de_stack_capacity = 100, de_word_penalty = -0.3, de_max_source_phrase_length = 7, de_max_target_phrase_length = 7, de_is_gen_lattice = false ]
USAGE: --------------------------------------------------------
USAGE: Start creating and loading the Language Model ...
USAGE: Language Model is located in: ../data/models/e_00_1000.lm
USAGE: Using the <cstyle_file_reader.hpp> file reader!
USAGE: Using the <h2d_map_trie.hpp> model.
INFO: The <h2d_map_trie.hpp> model's buckets factor: 2
INFO: Expected number of M-grams per level: [ 4101 14605 19222 19930 19618 ]
INFO1: Pre-allocating memory:  0 hour(s) 0 minute(s) 0 second(s) 
INFO1: Reading ARPA 1-Grams:  0 hour(s) 0 minute(s) 0 second(s) 
INFO1: Reading ARPA 2-Grams:  0 hour(s) 0 minute(s) 0 second(s) 
INFO1: Reading ARPA 3-Grams:  0 hour(s) 0 minute(s) 0 second(s) 
INFO1: Reading ARPA 4-Grams:  0 hour(s) 0 minute(s) 0 second(s) 
INFO1: Reading ARPA 5-Grams:  0 hour(s) 0 minute(s) 0 second(s) 
USAGE: Reading the Language Model took 0.149301 CPU seconds.
USAGE: Action: 'Loading the Language Model' memory change:
USAGE: vmsize=+0 Mb, vmpeak=+0 Mb, vmrss=+0 Mb, vmhwm=+0 Mb
USAGE: --------------------------------------------------------
USAGE: Start creating and loading the Translation Model ...
USAGE: Translation Model is located in: ../data/models/de-en-1-10.000.tm
USAGE: Using the <cstyle_file_reader.hpp> file reader!
USAGE: Using the hash-based translation model: tm_basic_model.hpp
INFO1: Pre-loading phrase translations:  0 hour(s) 0 minute(s) 0 second(s) 
INFO: The number of loaded TM source entries is: 7055
INFO1: Storing the pre-loaded phrase translations:  0 hour(s) 0 minute(s) 0 second(s) 
INFO: The phrase-translations table is created and loaded
USAGE: Reading the Translation Model took 0.182227 CPU seconds.
USAGE: Action: 'Loading the Translation Model' memory change:
USAGE: vmsize=+0 Mb, vmpeak=+0 Mb, vmrss=+0 Mb, vmhwm=+0 Mb
USAGE: --------------------------------------------------------
USAGE: Start creating and loading the Reordering Model ...
USAGE: Reordering Model is located in: ../data/models/de-en-1-10.000.rm
USAGE: Using the <cstyle_file_reader.hpp> file reader!
USAGE: Using the hash-based reordering model: rm_basic_model.hpp
INFO1: Counting reordering entries:  0 hour(s) 0 minute(s) 0 second(s) 
INFO: The number of RM source/target entries matching TM is: 9694
INFO1: Building reordering model:  0 hour(s) 0 minute(s) 0 second(s) 
USAGE: Reading the Reordering Model took 0.161332 CPU seconds.
USAGE: Action: 'Loading the Reordering Model' memory change:
USAGE: vmsize=+0 Mb, vmpeak=+0 Mb, vmrss=+0 Mb, vmhwm=+0 Mb
USAGE: The server is started!
USAGE: --------------------------------------------------------
<...>
```
In the first seven lines we see information loaded from the configuration file. Further, the LM, TM, and RM, models are loaded and the information thereof is provided. Note that for less output one can simply run `bpbd-server -c ../data/default-1-10.000.cfg`.

There is a few important things to note about the configuration file at the moment:

* `[Translation Models]/tm_feature_weights` - the number of features must not exceed the value of `tm::MAX_NUM_TM_FEATURES`, see [Project compile-time parameters](#project-compile-time-parameters).
* `[Translation Models]/tm_unk_features` - the number of features must not exceed the value of `tm::MAX_NUM_TM_FEATURES`, see [Project compile-time parameters](#project-compile-time-parameters).
* `[Reordering Models]/rm_feature_weights` - the number of features must not exceed the value of `lm::MAX_NUM_RM_FEATURES`, see [Project compile-time parameters](#project-compile-time-parameters).
* `[Language Models]/lm_feature_weights` - the number of features must not exceed the value of `lm::MAX_NUM_LM_FEATURES`, see [Project compile-time parameters](#project-compile-time-parameters).

Note that, if there number of lambda weights specified in the configuration file is less than the actual number of features in the corresponding model then an error is reported.

#### Server console
Once the server is started it is not run as a Linux daemon but is a simple multi-threaded application that has its own interactive console allowing to manage some of the configuration file parameters and obtain some run-time information about the server. The list of available server console commands is given in the listing below:

```
$ bpbd-server -c ../data/default-1-10.000.cfg -d info2
<...>
USAGE: The server is started!
USAGE: --------------------------------------------------------
USAGE: General console commands: 
USAGE: 	'q & <enter>'  - to exit.
USAGE: 	'h & <enter>'  - print HELP info.
USAGE: 	'r & <enter>'  - run-time statistics.
USAGE: 	'p & <enter>'  - print program parameters.
USAGE: 	'set ll  <level> & <enter>'  - set log level.
USAGE: Specific console commands: 
USAGE: 	'set nt  <positive integer> & <enter>'  - set the number of translating threads.
USAGE: 	'set d  <integer> & <enter>'  - set the distortion limit.
USAGE: 	'set pt  <unsigned float> & <enter>'  - set pruning threshold.
USAGE: 	'set sc  <integer> & <enter>'  - set stack capacity.
USAGE: 	'set ldp  <float> & <enter>'  - set linear distortion penalty.
>> 
```
Note that, the commands allowing to change the translation process, e.g. the stack capacity, are to be used with great care. For the sake of memory optimization, **bpbd-server** has just one copy of the server run time parameters used from all the translation processes. So in case of active translation process, changing these parameters can cause disruptions thereof starting from an inability to perform translation and ending with memory leaks. All newly scheduled or finished translation tasks however will not experience any disruptions.

#### Word lattice generation

If the server is compiled in the [Tuning mode](#project-compile-time-parameters), then the word lattice generation can be enabled through the options in the server's [Configuration file](#server-config-file). The options influencing the lattice generation are as follows:

```
[Decoding Options]
    #The the tuning word lattice generation flag; <bool>:
    #This flag only works if the server is compiled with
    #the IS_SERVER_TUNING_MODE macro flag to true, otherwise
    #it is ignored, i.e. is internally re-set to false.
    de_is_gen_lattice=true

    #Stores the lattice data folder location for where the
    #generated lattice information is to be stored.
    de_lattices_folder=./lattices

    #The file name extension for the feature id to name mapping file
    #needed for tuning. The file will be generated if the lattice
    #generation is enabled. It will have the same name as the config
    #file plus this extension.
    de_lattice_id2name_file_ext=feature_id2name

    #The file name extension for the feature scores file needed for tuning.
    #The file will be generated if the lattice generation is enabled.
    #It will have the same name as the session id plus the translation
    #job id plus this extension.
    de_feature_scores_file_ext=feature_scores

    #The file name extension for the lattice file needed for tuning.
    #The file will be generated if the lattice generation is enabled.
    #It will have the same name as the session id plus the translation
    #job id plus this extension.
    de_lattice_file_ext=lattice
```

The lattice generation will be enabled if the value of the `de_is_gen_lattice` parameter is set to `true`. The word lattice is generated per source sentence and consists of a translation hypothesis graph and employed feature weights. The word lattice format is conformant to that of the Oister translation system. The lattice files, are dumped into the folder specified by the `de_lattices_folder` parameter.

The lattice files employ internal feature ids to identify the features used to compute a score of each hypothesis. To map these ids to the feature names found in the server config file, a global *id-to-feature-name* file mapping can be generated. This file is placed in the same folder where the translation server is being run. Also, the mapping is only generated if the server is started with the `-f` command line option/flag. The latter is exclusively enabled if the server is compiled in the tuning mode. If started with this flag, the server exits right after the mapping is generated and does not load the models or attempts to start the WebSockets server. The name of the *id-to-feature-name* file is defined by the server config file name, with the extension by the config file parameter: `de_lattice_id2name_file_ext`.

In the lattice files, each sentence gets a unique *sentence-id*, corresponding to its position in the test source file. The sentence ids start from *1*. Each sentence's lattice consists of two files with the name begin the sentence id and the extensions defined by the `de_feature_scores_file_ext` and `de_lattice_file_ext` parameters:
 
1. *\<sentence-id\>.*`de_lattice_file_ext` - stores the graph of the translation process: the partial hypothesis and the transitions between them attributed with source and target phrases and added costs.
2. *\<sentence-id\.*`de_feature_scores_file_ext`  - stores information about the feature weights values (without lambdas coefficients) that were used in the hypothesis expansion process.

For additional information on the lattice file formats see [Appendix: Word lattice files](#appendix-word-lattice-files).

Once the translation, with word lattice generation, is finished `de_lattices_folder` folder stores the lattice information files for each of the translated sentences. In order to combine them together into just two larger files, storing lattice graphs and feature scores for all sentences, one needs to use the **script/combine-lattices.sh** script. It's synopsis is self explanatory:

```
$ combine-lattices.sh
Usage: ./combine-lattices.sh <lattice-dir> <result-file-name> <sent-lattice-ext> <set-scores-ext>
    <lattice-dir> - the directory with the lattice files
    <result-file-name> - the file name to be used for the combined lattice data
    <sent-lattice-ext> - the lattice file extension for a sentence, default is 'lattice'
    <set-scores-ext> - the feature scores file extension for a sentence, default is 'feature_scores'
```

### Load balancer: _bpbd-balancer_
The load balancer server can be used for the next purposes:
_(i)_ Distribute load between multiple translation servers, for the same source-target language pair;
_(ii)_ Aggregate multiple translation servers for different source-target language pairs;
_(ii)_ Aggregate various balancer and translation servers ensuring a single client-communication point;
The use of this executable is straightforward. When started from a command line without any parameters, **bpbd-balancer** reports on the available command-line options:

```
$ bpbd-balancer
<...>
PARSE ERROR:  
             Required argument missing: config

Brief USAGE: 
   bpbd-balancer  [-d <error|warn|usage|result|info|info1|info2|info3>] -c
                  <balancer configuration file> [--] [--version] [-h]

For complete USAGE and HELP type: 
   bpbd-balancer --help
```
As one can see the only required command-line parameter of the server is a configuration file. The latter shall contain the necessary information for running the balancer server and connecting to translation servers. The configuration file content is covered in section [Configuration file](#balancer-config-file) below. Once the load balancer is started there is still a way to change some of its run-time parameters. The latter can be done with a balancer console explained in the [Balancer console](#balancer-console) section below. 

#### Balancer config file
In order to start the load balancer one must have a valid configuration file for it. The latter stores the minimum set of parameter values needed to run the balancer server. Among other things, this config file specifies the location of the translation servers to connect to. An example configuration file is: `[Project-Folder]/balancer.cfg`. The content of this file is self explanatory and contains a significant amount of comments.

When run with a properly formed configuration file, **bpbd-balancer** gives the following output. Note the `-d info3` option ensuring additional information output during starting up and connecting to translation servers.

```
$ bpbd-balancer -d info3 -c ../balancer.cfg 
<...> 
USAGE: The requested debug level is: 'INFO3', the maximum build level is 'INFO3' the set level is 'INFO3'
USAGE: Loading the server configuration option from: ../balancer.cfg
INFO: The configuration file has been parsed!
INFO: Balancer parameters: {server_port = 9000, num_req_threads = 10, num_resp_threads = 10,
translation servers:
[{SERVER_NAME_01, ws://localhost:9001, load weight=1}, {SERVER_NAME_02, ws://localhost:9002, load weight=2},
{SERVER_NAME_03, ws://localhost:9003, load weight=1}, {SERVER_NAME_04, ws://localhost:9004, load weight=1}, ]}
INFO3: Sanity checks are: OFF !
INFO3: Configuring the translation servers' manager
INFO3: Configuring 'SERVER_NAME_01' adapter...
INFO2: 'SERVER_NAME_01' adapter is configured
INFO3: Configuring 'SERVER_NAME_02' adapter...
INFO2: 'SERVER_NAME_02' adapter is configured
INFO3: Configuring 'SERVER_NAME_03' adapter...
INFO2: 'SERVER_NAME_03' adapter is configured
INFO3: Configuring 'SERVER_NAME_04' adapter...
INFO2: 'SERVER_NAME_04' adapter is configured
INFO2: The translation servers are configured
USAGE: Running the balancer server ...
USAGE: The balancer is started!
USAGE: --------------------------------------------------------
<...> 
```
Note that for less output one can simply run `bpbd-balancer -c ../balancer.cfg`.

#### Balancer console
Once the balancer is started it is not run as a Linux daemon but is a simple multi-threaded application that has its own interactive console allowing to manage some of the configuration file parameters and obtain some run-time information about the load balancer. The list of available console commands is given in the listing below:

```
$ bpbd-balancer -d info3 -c ../balancer.cfg 
<...>
USAGE: The balancer is started!
USAGE: --------------------------------------------------------
USAGE: General console commands: 
USAGE: 	'q & <enter>'  - to exit.
USAGE: 	'h & <enter>'  - print HELP info.
USAGE: 	'r & <enter>'  - run-time statistics.
USAGE: 	'p & <enter>'  - print program parameters.
USAGE: 	'set ll  <level> & <enter>'  - set log level.
USAGE: Specific console commands: 
USAGE: 	'set int  <positive integer> & <enter>'  - set the number of incoming pool threads.
USAGE: 	'set ont  <positive integer> & <enter>'  - set the number of outgoing pool threads.
>> 
```

### Text processor: _bpbd-processor_
The text processor server can be used for the following things:
_(i)_ Pre-process text for translation;
_(ii)_ Detect the source language;
_(iii)_ Post-process text after translation;
The use of this executable is straightforward. When started from a command line without any parameters, **bpbd-processor** reports on the available command-line options:

```
$ bpbd-processor
<...>
PARSE ERROR:  
             Required argument missing: config

Brief USAGE: 
   bpbd-processor  [-d <error|warn|usage|result|info|info1|info2|info3>] -c
                   <processor configuration file> [--] [--version] [-h]

For complete USAGE and HELP type: 
   bpbd-processor --help
```
As one can see the only required command-line parameter of the server is a configuration file. The latter shall contain the necessary information for running the processor server. The configuration file content is covered in section [Configuration file](#processor-config-file) below. Once the processor server is started there is still a way to change some of its run-time parameters. The latter can be done with a balancer console explained in the [Processor console](#processor-console) section below. 

#### Processor config file
In order to start the processor server one must have a valid configuration file for it. The latter stores the minimum set of parameter values needed to run the server. Among other things, this config file specifies the pre/post-processor scripts to be used. An example configuration file is: `[Project-Folder]/processor.cfg`. This file uses dummy versions of the pre/post-processor scripts located in: `[Project-Folder]/script/text/`:

   * `pre_process.sh` - copies input file to output, detects any text as German
   * `post_process.sh` - copies input file to output

Note that, the pre/post- processor scripts do not need to be bash scripts. They can be anything command-line executable that satisfies the scripts' interface. Run these scripts to get more details on the expected interface and functionality of the pre/post-processing scripts.

For convenience and the sake of example, we also provide pre-integrated third-party pre/post-processing software that can be invoked by using the `pre_process_nltk.sh` and  `post_process_nltk.sh` scripts. Please note that, these have a richer interface than the dummy scripts. Run them with no parameters to get more info. These two scripts require [python NLTK](http://www.nltk.org/) to be installed. At present `pre_process_nltk.sh`, with NLTK installed, supports languages such as: Dutch, Finnish, Greek, Polish, Spanish, Czech, English, French, Italian, Portuguese, Swedish, Danish, Estonian, German, Norwegian, Slovene, and Turkish. In case [Stanford Core NLP](http://stanfordnlp.github.io/CoreNLP/download.html) is also installed and the Chinese models jar is present, then `pre_process_nltk.sh` supports Chinese as well. Support for post processing, via `post_process_nltk.sh`, is twofold. For de-tokenization it is by default ensured for languages such as: English, French, Spanish, Italian, and Czech. True-casing is only supported for the languages with provided true-caser models. At present we support two true-casing scripts: [`Moses`](https://github.com/moses-smt/mosesdecoder) and [`Truecaser`](https://github.com/nreimers/truecaser). Both of these are included in the distribution and are located under: `./scripts/text/truecaser`. Note that, the true-caser model training scripts are included in this distribution and are located the corresponding true-casing script folders:

  * For Moses: `./scripts/text/truecaser/moses/`
  * For Truecaser: `./scripts/text/truecaser/truecaser/`

More details on these scripts will be provided in the next section. 


The content of the text processor configuration file is self explanatory and contains a significant amount of comments. When run with a properly formed configuration file, **bpbd-processor** gives the following output. Note the `-d info3` option ensuring additional information output during starting up and connecting to translation servers.

```
$ bpbd-processor -d info3 -c ../processor.cfg 
<...> 
USAGE: The requested debug level is: 'INFO3', the maximum build level is 'INFO3' the set level is 'INFO3'
USAGE: Loading the processor configuration option from: ../processor.cfg
INFO: The configuration file has been parsed!
USAGE: The lattice file folder is: ./proc_text
WARN: The directory: ./proc_text does not exist, creating!
INFO: Processor parameters: {server_port = 9000, num_threads = 20, work_dir = ./proc_text, pre_script_conf = {call_templ = ../script/text/pre_process.sh --work-dir=<WORK_DIR> --job-uid=<JOB_UID> --lang=<LANGUAGE>}, post_script_conf = {call_templ = ../script/text/post_process.sh --work-dir=<WORK_DIR> --job-uid=<JOB_UID> --lang=<LANGUAGE>}}
INFO3: Sanity checks are: OFF !
USAGE: Running the processor server ...
USAGE: The processor is started!
USAGE: --------------------------------------------------------
<...> 
```
Note that for less output one can simply run `bpbd-processor -c ../balancer.cfg`.

#### Pre-integrated third-party pre/post-processing scripts
To make our software complete and also to show how third-party pre/post-processing software can be integrated into our project we have created example pre/post-processing scripts, both of which are [Natural Language Toolkit (NLTK)](http://www.nltk.org/) based, and thus require NLTK for python to be installed. The installation instructions are simple and are to be found on the toolkit's website. Let's consider the created scripts:

   * `./script/text/pre_process_nltk.sh:`
      * Uses [stop-words analysis](https://en.wikipedia.org/wiki/Stop_words) to detect languages.
      * Allows language auto detection for [NLTK](http://www.nltk.org/) known stop-word sets.
      * Supports text template generation for text structure restoration.
      * Supports text tekenization and lowercasing as provided by [NLTK](http://www.nltk.org/).
   * `./script/text/post_process_nltk.sh:`
      * Supports sentence capitalization as a separate option.
      * Provides text de-tokenization based on [MTMonkey scripts](https://github.com/ufal/mtmonkey)
      * Allows text true-casing based on [`Moses`](https://github.com/moses-smt/mosesdecoder) or [`Truecaser`](https://github.com/nreimers/truecaser) scripts.
      * Supports text structure restoration from a pre-generated template.

These scripts call on python or Perl scripts delivered with the distribution. The latter are configurable by their command-line parameters which are typically well documented. In order to change the default scripts' behavior we expect our users to edit these parameters inside the `pre_process_nltk.sh` and `post_process_nltk.sh` scripts. It is also important to note that:

   * The text structure restoration is per default enabled but it is then expected that both pre- and post- processing scripts are to be run on the same file system with the same work directory.
   * The text structure restoration can be disabled by not providing `pre_process_nltk.py` and `post_process_nltk.py` with the `[-t TEMPL]` command-line parameter.
   * The interface of `pre_process_nltk.sh` script is the same as that of ``pre_process.sh``.
   * The list of `post_process_nltk.sh` script parameters is richer than that of ``post_process.sh``. The former requires the `<true_caser_type>` parameter to be specified and also has an optional parameter `<models-dir>`. Run `post_process_nltk.sh` with no parameters to get more info.
   * The `<true_caser_type>` parameter of `post_process_nltk.sh` allows to enable one of the two true-caser scripts: [`Moses`](https://github.com/moses-smt/mosesdecoder) or [`Truecaser`](https://github.com/nreimers/truecaser).
   * The  `<models-dir>` parameter of `post_process_nltk.sh` is optional but defines the folder where the true-caser model files are to be found. If not specified then `<models-dir>` is set to `.`.
   * The true-casing models are supposed to have file names as the lower-cased English names of the corresponding languages. The model file extensions are supposed to be `*.tcm` for Moses and `*.obj` for Truecaser, e.g.: `english.tcm` or `chinese.obj`.
   * Our project does not provide any default true caser models neither for Moses nor for Truecaser. So for `post_process_nltk.sh` to be used with the parameter `<true_caser_type>` other than `none` one needs to obtain such model(s) for used target language(s).
   * In order to generate new true-caser models, one can use the corresponding training software scripts provided with the distribution: `./script/text/truecase/moses/train-truecaser.perl` for Moses and `./script/text/truecase/truecaser/TrainTruecaser.py` for Truecaser. These scripts are taken "as-is" from the corresponding software sources. `TrainTruecaser.py` expects the training corpus to be located in the `train.txt` file.
   * Although `Truecaser` perhaps allows for better accuracy, its training script generates much larger models than those of `Moses`. Therefore if true-casing is needed, to minimize post-processing times, we suggest using `post_process_nltk.sh` with `<true_caser_type>` set to `moses`. 

#### Processor console
Once the processor is started it is not run as a Linux daemon but is a simple multi-threaded application that has its own interactive console allowing to manage some of the configuration file parameters and obtain some run-time information about the text processor. The list of available console commands is given in the listing below:

```
$ bpbd-processor -d info3 -c ../balancer.cfg 
<...>
USAGE: The processor is started!
USAGE: --------------------------------------------------------
USAGE: General console commands: 
USAGE: 	'q & <enter>'  - to exit.
USAGE: 	'h & <enter>'  - print HELP info.
USAGE: 	'r & <enter>'  - run-time statistics.
USAGE: 	'p & <enter>'  - print program parameters.
USAGE: 	'set ll  <level> & <enter>'  - set log level.
USAGE: Specific console commands: 
USAGE: 	'set nt  <positive integer> & <enter>'  - set the number of processor threads.
>> 
```

### Translation client: _bpbd-client_
The translation client is used to communicate with the server by sending translation job requests and receiving the translation results. When started from a command line without any parameters, **bpbd-client** reports on the available command-line options:

```
$bpbd-client
<...>
PARSE ERROR:  
             Required arguments missing: output-file, input-lang, input-file

Brief USAGE: 
   bpbd-client  [-d <error|warn|usage|result|info|info1|info2|info3>] [-c]
                [-s <the translation priority>] [-l <min #sentences per
                request>] [-u <max #sentences per request>] [-p
                <post-processor uri>] [-t <server uri>] [-r <pre-processor
                uri>] [-o <target language>] -O <target file name> -i
                <source language> -I <source file name> [--] [--version]
                [-h]

For complete USAGE and HELP type: 
   bpbd-client --help
```
One of the main required parameters of the translation client is the input file. The latter should contain text, in **UTF8** encoding - not checked upon, to be translated. In case no pre-processing is requested, *see the [-r] option*, the text is sent to the translation server as is. In that case it is expected that, the text contains one sentence per line and the sentences are lower-cased and tokenized (space-separated).

Each translation can be given a priority with the optional `-s` parameter. The higher the priority the sooner it will be processed by the server(s). This rule includes the translation, the balancer, and the pre/post-processor servers. The default priority value is zero - indicating normal or neutral priority. Jobs with equal priorities are handled at the first-come-first-serve basis. The translation jobs of a given priority are not served until all the jobs of the higher priorities are taken care of.

Once started, if pre-processing server is specified (the `-r` option), the source text is sent for pre-processing. In case the source language is to be detected, the value of the `-i` parameter must be set to `auto`. If pre-processing went without errors, the translation client makes a WebSocket connection to the translation server, reads text from the input file, splits it into a number of translation job requests (which are sent to the translation server) and waits for the reply. Each translation job sent to the server consists of a number of sentences called translation tasks. The maximum and minimum number of translation tasks per a translation job is configurable via additional client parameters. After the text is translated, the information on the translation process is placed into the logging file that has the same name as the output file, but is suffixed with `.log`. The latter contains information such as: if a job/task was canceled, or an error occurred. Next, if post-processing server is specified (the `-p` option), the text is sent to post-processing. After the post-processed text is received, the result is written to the output file. If the post-processing server was not specified then the target text is saved "as is". For more info run: `bpbd-client --help`.

Note that, the pre- and post- processing servers do not have to be the same. I.e. the values of the `-r` and `-p` parameters can be different. Yet in this case, the text post processing might be less efficient, depends on how the pre- and post- processing scripts are implemented, as if the post-processing script is located on a different server than the pre-processing script, it might not have access to the source text file, that can be used when giving the target text the same structure as the source one.

For the sake of better tuning the translation server's parameters, we introduce a special client-side option: `-c`. This optional parameter allows to request supplementary translation-process information per sentence. This information is also placed into the `.log` file. Currently, we only provide multi-stack level's load factors. For example, when translating from German into English the next sentence: `" wer ist voldemort ? "` with the `-c` option, we get an output:

```
----------------------------------------------------
Job id: 1, sentences [1:1], client status: 'replied'
Server response status: 'good', message: The text was fully translated!
--
Sentence: 1 translation status: 'good'
Multi-stack loads: [ 1% 5% 25% 45% 44% 28% 6% 1% ]
```
Where the line starting with `Multi-stack loads`, contains the stack level's load information, in percent relative to the stack level capacity. Note that, the number of tokens in the German source sentence is *6*. Yet, the number of stack levels is *8*. The latter is due to that the first and the last stack levels corresponds to the sentence's, implicitly introduced, begin and end tags: `<s>` and `</s>`. The latter are added to the sentence during the translation process. Clearly, it is important to tune the server's options in such a way that all the stack levels, except for the first and the last one, are `100%` loaded. If so, then we know that we ensure an exhaustive search through the translation hypothesis, for the given system parameters, thus ensuring for the best translation result.

Remember that, running **bpbd-client** with higher logging levels will give more insight into the translation process and functioning of the client. It is also important to note that, the source-language text in the input file is must be provided in the **UTF8** encoding.

### Web UI Translation client:
The web client for the translation system is just a web application that uses the WebSockets API of **HTML5** to sent JSON requests to the text processor, translation server or to a load balancer. The web client can be activated by opening `script/web/translate.html` in a web browser. At the moment the client was tested and proved to work in the following 64-bit browsers under **OS X EI Capitan**:

* Opera 38.0.2220.41,
* Safari 9.1.1 (11601.6.17)
* Google Chrome 51.0.2704.103
* Firefox 47.0.

The web interface looks as follows:
![Web Client Translation System Image](./doc/images/translator.png "Web Client Translation System")

As one can see its interface is simple and intuitive, its main purpose to allow to connect to a translation sever/balancer and to perform translations. Source text can be input into the text area on the left by hand or loaded from a file. The translated (target) text can be found in the text area on the right. It is annotated, with a pop-up information. The latter is visualized when a mouse pointer hovers over the sentence translation. Note that, the translation priorities in the web interface have the same meaning and functionality as in the command line translation client, see section [Translation client](#translation-client).

Most of the interface controls have tool tips. Yet for the sake of completeness below, we provide an annotated screen shot of the interface:
![Web Client Translation System Annotated Image](./doc/images/translator_annot.png "Web Client Translation System - Annotated")

### Language model query tool: _lm-query_
The language model query tool is used for querying stand alone language models to obtain the joint m-gram probabilities. When started from a command line without any parameters, **lm-query** reports on the available command-line options:

``` 
$ lm-query 
<...>
PARSE ERROR:  
             Required arguments missing: query, model

Brief USAGE: 
   lm-query  [-l <lm lambda weight>] [-d <error|warn|usage|result|info
             |info1|info2|info3>] -q <query file name> -m <model file name>
             [--] [--version] [-h]

For complete USAGE and HELP type: 
   lm-query --help
```
For information on the LM file format see section [Input file formats](#input-file-formats). The query file format is a text file in a **UTF8** encoding which, per line, stores one query being a space-separated sequence of tokens in the target language. The maximum allowed query length is limited by the compile-time constant `lm::LM_MAX_QUERY_LEN`, see section [Project compile-time parameters](#project-compile-time-parameters)

### Parameter Tuning
In order to obtain the best performance of the translation system one can employ Discriminative Training, see Chapter 9 of [Koe10](./doc/bibtex/Koehn_SMT_Book10.bib). The latter uses generated word lattice, c.f. Chapter 9.1.2 of [Koe10](./doc/bibtex/Koehn_SMT_Book10.bib), to optimize translation performance by reducing some measure of translation error. This is done by tuning the translation parameters such as feature lambda values of the model feature weights.

We measure the translation system performance in terms of the BLEU scores. Therefore, parameter tuning shall find such lambda values, to be used in the `bpbd-server` configuration files, c.f. section [Server config file](#server-config-file), that they maximize the BLEU scores of the translations provided by the system. As you already know from section [Word lattice generation](#word-lattice-generation), our software allows for word lattice generation.

In this section we explain other tools we have to perform parameter tuning. Please note that, tuning scripts are implemented using Perl and bash. We expect the Perl version to be `>=v5.10.1`. Also, in order to use the tuning scripts one must have MegaM compiled. See section [Building the project](#building-the-project) for the corresponding build instructions.

The rest of the section is organized as follows. First in section [Tuning test sets](#tuning-test-sets), we report on the test-sets that we use for tuning and their internal structure. Next in section [Run parameter tuning](#run-parameter-tuning), we explain how the tuning script can be run. Section [Check on tuning progress](#check-on-tuning-progress) reports on how the tuning progress can be monitored and the server configuration files can be generated for various tuning iterations. At last in section [Stop tuning](#stop-tuning) we explain how tuning can be stopped in an easy manner.

#### Tuning test sets
Parameter tuning is done wrt an [MT-Eval (OpenMT)](http://www.itl.nist.gov/iad/mig/tests/mt/) test set. Typically, we use [MT-04](http://www.itl.nist.gov/iad/mig/tests/mt/2004/). From these test sets we use two types of files:

* `Source file` - a pre-processed text in the source language, ready to be translated;
* `Reference translation files` - reference translation files, in the target language, not post-processed;

The source files are to be plain text files, pre-processed in the same way as it shall be done when running the translation infrastructure. I.e. using the same sentence splitting methods and tokenizers. Of course, the pre-processed text is expected to be lower-cased and to have one sentence per line. See section [Text processor: bpbd-processor](#text-processor-bpbd-processor) for more details on text pre-processing.

The reference files shall also be in plain text format. Moreover, they are expected to be lower-cased, tokenized and have one sentence per line. I.e. to have the same format as the text produced by the `bpbd-server`. We support multiple reference translations and therefore all the translation files shall have the same file name format: `<file_name_base>.<ref_index>`. Here '<file_name_base>' is the same file name used for all reference translation files of the given source text. The `<ref_index>` is the reference translation index, starting from `0`. Note that, even if there is just one reference translation then it shall still get the index in its file name. The maximum number of reference translation files is `100`.

#### Run parameter tuning
In order to start parameter tuning one shall use the `run_tuning.sh` script located in `[Project-Folder]/scripts/tuning/`. If run without parameters, it gives the following usage information:

```
$ ./run_tuning.sh 
------
USAGE: Allows to start the tuning process for the infrastructure
------
 ./run_tuning.sh --conf=<file-name> --src=<file-name> --src-language=<string> 
        --ref=<file-pref> --trg-language=<string> --no-parallel=<number> 
        --trace=<number> --nbest-size=<number> --mert-script=<string>
 Where:
    --conf=<file-name> the initial configuration file for the decoding server
    --src=<file-name> the source text file to use with tuning
    --src-language=<string> the language of the source text
    --ref=<file-pref> the reference translation text file name prefix.
                      The files names are constructed as:
                            <file-templ>.<index>
                      where <index> starts with 1.
    --trg-language=<string> the language of the reference text, default is english
    --no-parallel=<number> the number of parallel threads used for tuning, default is 1
    --trace=<number> the tracing level for the script, default is 1
    --nbest-size=<number> the number of best hypothesis to consider, default is 1500
    --mert-script=<string> the MERT script type to be used, default is 'PRO-14'
```

As one can see the script's interface is fairly simple. It requires the name of the config file, the source text file, the source language name, the target language name, and the reference file(s) prefix. Other parameters are optional and can be omitted. Note that, when run on a multi-core system, for the sake of faster tuning, it is advised to set the value of the `--no-parallel=` parameter to the number of cores present in the system. Further details on the source and reference files can be found in section [Tuning test sets](#tuning-test-sets).

Please note that for the tuning script to run, we need the initial `bpbd-server` configuration file. The latter can be obtained by copying the default `server.cfg` file from the project's folder to the work folder, where the tuning will be run, and then modifying it with the proper source and target language names, the model file locations, and modifying other values, if required. The tuning script itself can be run from any folder, chosen to be a work folder. Let us consider an example invocation of the tuning script:

```
$ [Project-Folder]/script/tuning/run_tuning.sh --conf=server.cfg --no-parallel=39 --trace=1 --nbest-size=1500 --src-language=chinese --src=$SMTAMS/data/translation_test/OpenMT/mt04/chinese-english//mt04.chinese-english.src.tok_stanford-ctb.txt --ref=$SMTAMS/data/translation_test/OpenMT/mt04/chinese-english//mt04.chinese-english.ref.tok.txt --trg-language=english
-----
Use '[Project-Folder]/script/tuning/tuning_progress.pl' to monitor the progress and retrieve optimum config.
Use '[Project-Folder]/script/tuning/kill_tuning.pl' to stop tuning.
-----
Starting tuning on: smt10.science.uva.nl at: Fri Dec 16 17:07:00 CET 2016
Writing the tuning log into file: tuning.log ...
```

Here for the sake of example, we explicitly specified the optional parameters: `--no-parallel=39`, `--trace=1`, and `--nbest-size=1500`. Once the script is started, the tuning process output is written into the `tuning.log` file. The output of the used `bpbd-client` is placed in the `client.log` and the output of the lattice combination script into the `combine.log`. Note that, the tuning script blocks the console and runs until one of the conditions is satisfied:

* The maximum allowed number of tuning iterations is done, this value is set to `30`;
* The tuning process is stopped by the `kill_tuning.pl` script, see section [Stop tuning](#stop-tuning);

#### Check on tuning progress
Tuning can takes several hours to a couple of days, depending on the number of features, the size of the models, the size of the development set, etc. Therefore, we have created the `tuning_progress.pl` script, located in `[Project-Folder]/script/tuning/`, that allows to monitor the tuning progress. When run without parameters this script provides the following output:

```
$ ./tuning_progress.pl
Smartmatch is experimental at ./tuning_progress.pl line 100.
-------
INFO:
    This script allows to monitor running/finished tuning process and
    to extract the configuration files for different tuning iterations.
    This script must be run from the same folder where tuning was/is run.
-------
USAGE:
    tuning_progress.pl --conf=<file_name> --err=<file_name> --select=<string>
        --conf=<file_name> - the configuration file used in tuning process
        --err=<file_name>  - the tuning.log file produced by the tuning script
        --select=<string>  - the iteration index for which the config file is to
                             be generated or 'best'/'last' values to get the config
                             files for the best-scoring or last iteration respectively.
                             This parameter is optional, if specified - the script
                             generates a corresponding iteration's config file
-------
ERROR: Provide required script arguments!
```
As on can notice this script can be used for two purposes:

1. Monitor the tuning progress;
2. Generate the configuration of a certain tuning iteration;

Let us consider both of these usages in more details.

##### Monitor Progress
Invoke the `tuning_progress.pl` script from the work folder where the tuning is run. Use the  `tuning.log` log file as the value of the `--err=` parameter and the used config file as the value of the `--conf=` parameter:

```
$ [Project-Folder]/script/tuning/tuning_progress.pl --conf=server.cfg --err=tuning.log
Avg. total = 0.00 sec.

Avg. total = 0.00 sec.

iteration=1     BLEU=0.297540050683629
iteration=2     BLEU=0.304940039112847  ^
iteration=3     BLEU=0.31246539393292   ^
iteration=4     BLEU=0.319738621056022  ^
iteration=5     BLEU=0.324859305442724  ^
iteration=6     BLEU=0.331238665525784  ^
iteration=7     BLEU=0.337603780346243  ^
iteration=8     BLEU=0.341755568020084  ^
iteration=9     BLEU=0.343932768784104  ^
iteration=10    BLEU=0.350114574042927  ^
iteration=11    BLEU=0.351529162662153  ^
iteration=12    BLEU=0.354527741244185  ^
iteration=13    BLEU=0.358353976275115  ^
iteration=14    BLEU=0.359557421035731  ^
iteration=15    BLEU=0.362479837703255  <--- best overall BLEU
iteration=16    BLEU=0.362085630174003  v
iteration=17    BLEU=0.36209376384702   ^
iteration=18    BLEU=0.359820528770433  v
iteration=19    BLEU=0.356861833879925  v
iteration=20    BLEU=0.354888166740936  v
iteration=21    BLEU=0.353153254195068  v
iteration=22    BLEU=0.355045301787933  ^
iteration=23    BLEU=0.351972501936527  v
iteration=24    BLEU=0.348495504435742  v

(1) de_lin_dist_penalty
(2) lm_feature_weights
(3) rm_feature_weights
(4) tm_feature_weights
(5) tm_word_penalty

...
```

This will print out a lot of information, but the most relevant part is shown above. It indicates the tuning iterations with the corresponding BLEU scores. Here tuning was running for a while already and we can see that for iterations `1-15` BLEU scores went up (the '^' sign), with iteration `15` yielding the best BLEU score. After that (iterations `16-24`), BLEU scores went (mostly) down.

As a rule of thumb, if you see that BLEU scores drop for two consecutive iterations after the optimal iteration, it's time to stop tuning. The latter can be done with the `kill_tuning.pl` script discussed in section [Stop tuning](#stop-tuning);

##### Generate config
When the `run_tuning.sh` script is still running, or after its execution is finished, one can generate the config file corresponding to any of the performed tuning iterations, as listed by the `tuning_progress.pl` script. In order to do so, in addition to the `--conf=` and `--err=` parameters one can just specify the third one: `--select=`, supplying the iteration number. For example, to generate the configuration script used in the best scoring tuning iteration of the example above, one can use the following script invocation:

```
$ [Project-Folder]/script/tuning/tuning_progress.pl --conf=server.cfg --err=tuning.log --select=best
```

or alternatively:
```
$ [Project-Folder]/script/tuning/tuning_progress.pl --conf=server.cfg --err=tuning.log --select=15
```

Since iteration `15` was the best scoring one, these will result in two identical configuration files being generated: `server.cfg.best` and `server.cfg.15`. Please note that, the `tuning_progress.pl` script must always be run from the same folder as the `run_tuning.sh` script as it uses some hidden temporary files stored in the work folder.

#### Stop tuning
If the best scoring tuning iteration has been reached one might want to stop tuning right away. This could be done by killing the `run_tuning.sh` script but things are a bit more complicated than that. The tuning script forks plenty of independent processes each of which log its PID value into the `tuning.log` file. Clearly, if `run_tuning.sh` is killed this will not affect the forked processes. This is why we have developed the `kill_tuning.pl` script located in the `[Project-Folder]/script/tuning/` folder. The only parameter required by the script is the `tuning.log` file used as a source of the related PID values. The script can be invoked as follows:

```
$ [Project-Folder]/script/tuning/kill_tuning.pl tuning.log

Are you sure you want to stop the tuning process?

Answer (yes/no)? yes
Starting killing the active tuning processes ...
----------------------------------------------------------------------
Starting a killing iteration, analyzing the error log for process ids.
The error log analysis is done, starting the killing.
 8380 pts/2    00:00:00 tuner.pl
 8385 pts/2    02:01:30 PRO-optimizatio
 4823 pts/2    01:57:44 bpbd-server
15260 pts/2    00:00:00 PRO-optimizer-p
15265 pts/2    00:05:37 PRO-optimizer-l
...
15297 pts/2    00:05:38 PRO-optimizer-l
15303 pts/2    00:05:38 PRO-optimizer-l
The number of (newly) killed processes is: 18
----------------------------------------------------------------------
Starting a killing iteration, analyzing the error log for process ids.
The error log analysis is done, starting the killing.
The number of (newly) killed processes is: 0
----------------------------------------------------------------------
The killing is over, we are finished!
```

Please note that, the script requires you to type in `yes` as a complete word and press enter to start the killing. Any other value will cause the killing to be canceled.

## Input file formats
In this section we briefly discuss the model file formats supported by the tools. We shall occasionally reference the other tools supporting the same file formats and external third-party web pages with extended format descriptions.

**WARNING:** The tooling only supports plain text `UTF8` encoded files! E.g. the decoder (translation server) doesn't accept compressed model files. __The software does not check on the file encoding or on that the file is uncompressed!__

###Translation model: `*.tm`
The translation-model file stores the phrase pairs in the source and target languages and the pre-computed probability weights in the following strict format:

```
<source-phrase> ||| <target-phrase> ||| <prob-1> <prob-2> <prob-3> <prob-4>
```

As generated by, e.g. [Moses](http://www.statmt.org/moses/?n=Moses.Tutorial). In general the source and target phrases and target phrase and probability weight sections are separated by five symbols: one space three vertical lines and one space. Source and target space words must be space separated, as well as the probability weights. At the moment, everything followed after the fourth probability, until the end of the line, is ignored. The tool supports `4` translation probabilities and the supported number of weights is defined by the `tm::NUM_TM_FEATURES` constant value, see [Project compile-time parameters](#project-compile-time-parameters). If the format is not followed, the program's behavior is not specified.

### Reordering model: `*.rm`
The reordering-model file stores the phrase pairs in the source and target languages and the reordering weights in the following strict format:

```
<source-phrase> ||| <target-phrase> ||| <weight-1> <weight-2> ... <weight-k>
```

As generated by, e.g. [Moses](http://www.statmt.org/moses/?n=FactoredTraining.BuildReorderingModel). In general the source and target phrases and target phrase and probability weight sections are separated by five symbols: one space three vertical lines and one space. Source and target space words must be space separated, as well as the probability weights. At the moment, everything followed after the last probability, until the end of the line, is ignored. The number weights `k` is fixed per model file. The tool supports `6` or `8` reordering weights and the supported number of weights is defined by the `rm::NUM_RM_FEATURES` constant value, see [Project compile-time parameters](#project-compile-time-parameters). If the format is not followed, the program's behavior is not specified.

### Language model: `*.lm`
The language model file is a UTF8 text file in a well known ARPA format, see e.g. details on [MSDN help](https://msdn.microsoft.com/en-us/library/office/hh378460%28v=office.14%29.aspx) or [Speech Technology and Research (STAR) Laboratory](http://www.speech.sri.com/projects/srilm/manpages/ngram-format.5.html). An example ARPA file is given below:

```
<header - information ignored by applications>

\data\
ngram 1=9
ngram 2=11
ngram 3=3

\1-grams:
-0.8953 <unk>        -0.7373
-0.7404 </s> -0.6515
-0.7861 <s>   -0.1764
-1.0414 When -0.4754
-1.0414 will -0.1315
-0.9622 the   0.0080
-1.4393 Stock        -0.3100
-1.0414 Go    -0.3852
-0.9622 Up    -0.1286

\2-grams:
-0.3626 <s> When     0.1736
-1.2765 <s> the      0.0000
-1.2765 <s> Up       0.0000
-0.2359 When will    0.1011
-1.0212 will </s>    0.0000
-0.4191 will the     0.0000
-1.1004 the </s>     0.0000
-1.1004 the Go       0.0000
-0.6232 Stock Go     0.0000
-0.2359 Go Up        0.0587
-0.4983 Up </s>      

\3-grams:
-0.4260 <s> When will      
-0.6601 When will the      
-0.6601 Go Up </s>   

\end\
```
Note that the format is expected to be followed in a very strict way. The headers can be skipped, the empty lines must be empty, the M-gram entry: 

```
<probability>    <word-1> <word-2> ... <word-m>    <back-off-weight>
```
Must have one _tabulation_ symbol after the `<probability>`, single space between any two words, and a single _tabulation_ symbol before the `<back-off-weight>`. If the format is not followed, the program's behavior is not specified. The maximum allowed language model level, the maximum value of N in the N-gram, is defined by the compile-time parameter `lm::LM_M_GRAM_LEVEL_MAX`, see [Project compile-time parameters](#project-compile-time-parameters).

## Code documentation
At present the documentation is done in the Java-Doc style that is successfully accepted by Doxygen with the Doxygen option *JAVADOC_AUTOBRIEF* set to *YES*. The generated documentation is located in two folders:

* `[Project-Folder]/doc/html.tar.gz`
    - Open the _index.html_ file located in the extracted folder with your favorite web browser.
* `[Project-Folder]/doc/latex.tar.gz`
    - Open the _refman.pdf_ file located in the extracted folder with your favorite pdf viewer.

The `[Project-Folder]/Doxyfile` can be used to re-generate the documentation at any given time, for more details see [Doxygen](www.doxygen.org/).

* To re-build the Latex documentation run the following commands from the Linux console:
	+ `cd [Project-Folder]/doc`
	+ `tar -zxvf latex.tar.gz`
	+ `cd ./latex`
	+ `make`

## Third-party software
At present this project includes the following external/third-party software:

| Library Name | Purpose | Website | Version | License |
|:------------|:--------:|:-------:|:-------:|:-------:|
|Feather ini parser|_Fast, lightweight, header, portable INI/configuration file parser for ANSI C++._|[link](https://github.com/Turbine1991/feather-ini-parser)|1.40|[MIT](https://opensource.org/licenses/MIT)|
|WebSocket++|_Is an open source, header only C++ library implementing RFC6455 (The WebSocket Protocol)._|[link](http://www.zaphoyd.com/websocketpp)|0.7.0|[BSD](http://www.linfo.org/bsdlicense.html)|
|Asio C++ Library|_A cross-platform C++ library for network and low-level I/O programming_|[link](http://think-async.com/)|1.10.6|[Boost](http://www.boost.org/users/license.html)|
|Tclap|_A small and flexible library that provides a simple interface for defining and accessing command line arguments_|[link](http://tclap.sourceforge.net/)|1.2.1|[MIT](https://opensource.org/licenses/MIT)|
|Rapid JSON|_An open source, header only C++ library implementing JSON for C++_|[link](https://github.com/miloyip/rapidjson)|1.0.2|[MIT](https://opensource.org/licenses/MIT)|
|jQuery|_A fast, small, and feature-rich JavaScript library_|[link](https://jquery.com/)|2.2.4|[MIT](https://opensource.org/licenses/MIT)|
|Bootstrap|_HTML, CSS, and JS framework for developing responsive, mobile first Web UIs_|[link](http://getbootstrap.com/)|3.3.6|[MIT](https://opensource.org/licenses/MIT)|
|MD5|_RSA Data Security, Inc. MD5 Message-Digest Algorithm_|[link](http://pajhome.org.uk/crypt/md5/index.html)|1.0|[BSD](https://opensource.org/licenses/BSD-3-Clause)|
|Download|_A library allowing to trigger a file download from JavaScript_|[link](http://danml.com/download.html)|4.2|[CCA4.0](https://creativecommons.org/licenses/by/4.0/)|
|MT Monkey|_An adapted de-tokenization script from the "Infrastructure for Machine Translation web services" project_|[link](https://github.com/ufal/mtmonkey/blob/master/worker/src/util/detokenize.py)|1.0|[Apache2.0](https://www.apache.org/licenses/LICENSE-2.0)|
|Moses|_Adapted true-casing scripts from the "Statistical machine translation system" project_|[link](https://github.com/moses-smt/mosesdecoder/tree/master/scripts/recaser)|0.12.1|[GPL2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html#SEC1)|
|Truecaser|_The "Language Independent Truecaser for Python" project_|[link](https://github.com/nreimers/truecaser)|1.0|[Apache2.0](https://www.apache.org/licenses/LICENSE-2.0)|
|MegaM|_MEGA Model Optimization Package_|[link](https://www.umiacs.umd.edu/~hal/megam/index.html)|0.92|[Free](./script/tuning/megam_0.92/README)|

## Performance evaluation
In this section we provide two performance evaluations done for the developed software. The first one is a comparison of [language-model query tools](#lm-query-tool-evaluation), relating our project's software performance with that of [SRILM](http://www.speech.sri.com/projects/srilm/) and [KenLM](https://kheafield.com/code/kenlm/). The section one is a comparison of [multi-threaded translation servers](#translation-server-evaluation), checking on how our software scales with increasing the number of decoding threads on a multi-core machine and compares that with a home-brewed decoding server called Oister and well known decoding servers [Moses](http://www.statmt.org/moses/index.php?n=Main.HomePage) and [Moses2](http://www.statmt.org/moses/?n=Site.Moses2).

### LM query tool evaluation
In this section we provide an empirical comparison of the developed LM query tool with two other well known tools, namely [SRILM](http://www.speech.sri.com/projects/srilm/) and [KenLM](https://kheafield.com/code/kenlm/), both of which provide language model implementations that can be queried.  The additional information on the compared tools is to be found in [Appendix: LM query tests](#appendix-lm-query-tests)

#### Test set-up
The main target of this experimental comparison is to evaluate memory consumption and query times of the implemented tries. For doing that we do not rely on the time and memory statistics reported by the tools but rather, for the sake of uniform and independent opinion, rely on the Linux standard time utility available in the `zsh` Linux shell. The latter provides system- measured statistics about the program run. We choose to measure:

* **MRSS** - the maximum resident memory usage of the program
* **CPU time** - the CPU time in seconds

We chose to measure maximum resident memory usage as this is what defines the amount of RAM needed to run the program. Also, the CPU times are the actual times that the program was executed on the CPU. Measuring CPU times allows for a fair comparison as excludes possible results influence by the other system processes.

The experiments were set up to be run with different-size 5-gram language models given in the ARPA format with two types of inputs:

1. The single 5-gram query that defines the baseline
2. The file input with 100,000,000 of 5-gram queries

The delta in execution CPU times between the baseline and the 100,000,000 query files defines the pure query execution time of the tool. Note that, the query files were produced from the text corpus different from the one used to produce the considered language models. The MRSS values are reported in gigabytes (Gb) and the CPU times are measured in seconds. The plots provide MRSS and CPU times relative to the input model size in Gb.

The test hardware configuration and the model/query files' data is to be found in [Appendix: LM query tests](#appendix-lm-query-tests)

#### Experimental results
The experimental results are present in the following two pictures. The first one indicates the changes in the MRSS depending on the model size: 

![MRSS Comparisons Image](./doc/images/experiments/lm/mem.jpg "MRSS Comparisons")

The second one shows the query CPU times depending on the model sizes:

![CPU Times Comparisons Image](./doc/images/experiments/lm/time.jpg "CPU Times Comparisons")

The results show that the developed LM model trie representations are highly compatible with the available state of the art tools. We also give the following usage guidelines for the implemented tries:

* **w2ca** and **c2wa** tries are beneficial for the machines with limited RAM. If low memory usage is very critical then bitmap hash caching can also be disabled.
* **c2dm** trie provides the fastest performance with moderate memory consumption. This is recommended when high performance is needed but one should be aware of possible m-gram id collisions.10
* **c2dh** trie is preferable if performance, as well as moderate memory consumption, is needed. This is the second-fastest trie which, unlike **c2dm**, is fully reliable.
* **w2ch** trie did not show itself useful and **g2dm** is yet to be re-worked and improved for better performance and memory usage.
* **h2dm** following the intuitions of the KenLM implementation, realizes the hash-map based trie using the linear probing hash map which turns to be the fastest trie with one of the best memory consumption. This tries type is used as a default one

### Translation server evaluation
In this section we provide an empirical comparison of the project's translation server with a home-brewed translation system called Oister and well known translation systems [Moses](http://www.statmt.org/moses/index.php?n=Main.HomePage) and [Moses2](http://www.statmt.org/moses/?n=Site.Moses2). Extended information on the compared tools and the test machine configuration can be found in [Appendix: Translation performance tests](#appendix-translation-performance-tests)

**Please note:** The system developed within this project, in this section, is referred as REMEDI.

#### Test set-up
In order to measure performance of the aforementioned systems we chose to perform Chinese to English translations based on the data of the [OpenMT MT-04 dataset](https://catalog.ldc.upenn.edu/LDC2010T12). Let us consider the experimental setup in more details. We shall first discuss the size of the used models, then go into the main translation parameters matching. Next, we indicate how we achieved the comparable BLEU performance of the systems. Finally, we explain how the decoding times were measured and on which machine configuration the experiments were run.

##### Models
It has been decided to take significant size models in order to make the timing aspects more vivid. The used model sizes are as follows:

* **Language Model** - 48.9 Gb (5-gram model);
* **Translation Model** - 1.3 Gb (5 features model);
* **Reordering Model** - 9.7 Gb (8 features model);

##### Parameters
We made sure that all of the system's parameters having high impact on systems' decoding times are matched in their values. We used:

* **Translation limit = 30** - the number of top translations per source phrase to consider;
* **Beam width = 0.1** - the maximum deviation from the best stack's hypothesis to consider;
* **Stack capacity = 100** - the maximum number of hypothesis per translation stack;
* **Source phrase length = 7** - the maximum source phrase length to consider;
* **Target phrase length = 7** - the maximum target phrase length to consider;
* **Distortion limit = 5** - the maximum distance for jumping within the source sentence;

Further, Moses and Moses2 were made sure not to use cube pruning as it can give a significant performance advantage and is not yet implemented in REMEDI. This does not limit the generality of the experiments as we are mostly interested in scaling of the systems' performance with the number of threads on a multi-core machine.

##### Tuning
All of the systems were individually tuned on the MT-04 dataset, using the same source corpus. We took a [CTB segmented Chinese](http://nlp.stanford.edu/software/segmenter.shtml) source text consisting of 1788 sentences and 49582 tokens. The same Chinese source text was used for translation during the performance experiments. The latter allowed us, in addition to performance measurements, to control the resulting translations' BLEU scores. Comparable BLEU values give us a higher confidence in proper performance comparison (Assuming correct system implementations, low BLEU scores could be a sign of the decoding algorithm considering too few translation hypothesis). The BLEU scores of the resulting translations, per system, are listed below:

* REMEDI: **36.72** BLEU
* Oister: **36.80** BLEU
* Moses: **35.53** BLEU
* Moses2: **35.53** BLEU

Note that, both REMEDI and Oister have very close BLEU scores, the difference is 0.08 which is considered to be negligible. Moses and Moses2 show exactly the same scores, which comes at is a bit of a surprise as Moses and Moses2 aren't exactly alike. Moses2 only implements a subset of the functionality of Moses, also there is some differences in pruning and stack configuration. The last thing to note is that the scores difference between REMEDI/Oister and Moses/Moses2 is of 1.27 BLEU points. This 3.5% difference implies a non-ideal but rather fair performance comparison. Note that, this is the best what the systems could achieve do on the given data as each of the systems was tuned independently to provide the best scores possible.

##### Measurements
All of the systems under consideration were taken as black-box systems. I.e. we did not reply on their timing outputs but rather measured the systems' run-time as given by the `time` command of the Linux shell. For each system we calculated the difference between the run-times needed to translate the Chinese source text, consisting of 1788 sentences and 49582 tokens, and a single Chinese word text. This difference gave us the pure translation times, excluding any model-loading/unloading, server-connection/disconnection or disk IO related times. Note that, each experiment was performed 10 times, which gave us the possibility to compute the average decoding times per experiment along with the standard deviation thereof. The exception was Oister. Due to its very long translation times, from 1.5 hours on 70 threads up to 20 hours on a single thread, we ran Oister complete-corpus translation experiments three times. However, we did 10 runs of the single-word text translations to properly measure its model-loading and unloading times. Considering the drastic difference in the Oister runtime and that of other tools, the fact of fewer test run repetitions done for Oister shall not give any impact on the obtained results.

##### Machine
To conclude the experimental setup section, let us note that each experiment was run independently on a dedicated machine. The used test machine runs Cent OS 6 and features 256 Gb RAM and a 64-bit, 40 core Intel Xeon, 2.50 GHz processor. The complete machine's configuration is given in [Appendix: Translation performance tests](#appendix-translation-performance-tests).

#### Experimental results
In this section we present several plots obtained from the measured data. First, we shall compare the systems' runtime and systems' throughput. Next, we consider systems' relative performance, scaling, and look at the speedups gained with increasing the number of decoding threads. At last, we investigate how REMEDI scales when increasing the workload. All of the plots presented in this section are based on the same data and just give different views on it for better analysis.

##### Systems' decoding times
Let us consider the four independent plots of the systems' decoding times:

![REMEDI: Decoding times, standard deviation](./doc/images/experiments/servers/stats.time.remedi.log.png "REMEDI: Decoding times, standard deviation")

![Oister: Decoding times, standard deviation](./doc/images/experiments/servers/stats.time.oister.log.png "Oister: Decoding times, standard deviation")

![Moses: Decoding times, standard deviation](./doc/images/experiments/servers/stats.time.moses.log.png "Moses: Decoding times, standard deviation")

![Moses2: Decoding times, standard deviation](./doc/images/experiments/servers/stats.time.moses2.log.png "Moses2: Decoding times, standard deviation")

Note that the *x* and *y* scales are log<sub>2</sub> and log<sub>10</sub> respectively. These plots show the average decoding time values plus the computed standard deviations. As one can see, Oister's deviations are rather small, compared to its large average decoding times. Next in line is REMEDI, its standard deviations are small, compared to those of Moses and especially Moses2 which indicates high application's stability in the multi-threading environment. This stability is also ensured by the fact that REMEDI is a client/server system, so the  model loading/unloading times, requiring unstable disk I/O operations, are fully excluded from the measurements.

The next figure presents the systems' average decoding times next to each other:

![Decoding times](./doc/images/experiments/servers/stats.time.tools.log.png "Decoding times")

Here, one shall make several observations. First of all, the performance of Moses and Moses2 are not as expected from explanations in [Appendix: Translation performance tests](#appendix-translation-performance-tests). On a single thread Moses is about 2 times slower than Moses2 and on 40-70 threads it is about 1.5 times slower. The more threads the less difference there is between Moses and Moses2. This contradicts the data obtained from the [official Moses2 web page](http://www.statmt.org/moses/?n=Site.Moses2), where the situation is as follows:

![Moses vs. Moses2 Scalability, 32 cores](./doc/images/experiments/servers/moses2-scalability.png "Moses vs. Moses2 Scalability, 32 cores")

It suggests that Moses2 scales better than Moses solely due to a better multi-threading implementation. Our findings indicate that Moses2 actually scales worse than Moses with the number of threads. We speculate that currently the speed improvement of Moses2 over Moses is mostly gained by the improved decoding algorithms or used data structures and not multi-threading improvements. Second, when the #threads >= #cores the decoding times of REMEDI and Moses are about the same. We see it is a great achievement from our side, considering that REMEDI was developed within two years by a single developer and Moses is being developed for 11 years by a research community. Despite this great difference in development investments, when fully utilizing the machine cores both of these tools exhibit the same decoding performance!

##### Systems' throughput
The average systems' throughput, in terms of words per second (wps), with standard deviation values, per number of threads, is given below:

![REMEDI: Words per second, standard deviation](./doc/images/experiments/servers/stats.wps.remedi.log.png "REMEDI: Words per second, standard deviation")

![Oister: Words per second, standard deviation](./doc/images/experiments/servers/stats.wps.oister.log.png "Oister: Words per second, standard deviation")

![Moses: Words per second, standard deviation](./doc/images/experiments/servers/stats.wps.moses.log.png "Moses: Words per second, standard deviation")

![Moses2: Words per second, standard deviation](./doc/images/experiments/servers/stats.wps.moses2.log.png "Moses2: Words per second, standard deviation")

These plots are functions of those in the previous section and give an insight into the wps data and its accuracy. The average throughput values for all of the systems are as follows:

![Words per second](./doc/images/experiments/servers/stats.wps.tools.log.png "Words per second")

These allow to compare and investigate the wps performance. For example, REMEDI has approximately 50 wps on a single thread which grows to approximately 1000 wps on 40 threads. This indicates an approximately 20 times performance increase when comparing the system being run on a single core vs 40 cores. Going in the number of threads beyond the number of available cores does not seem to bring any significant penalties.

##### Relative systems' performance
Remember that one of the main project goals was to get a faster SMT system than Oister. The next figure gives the translation time ratio of Oister vs. REMEDI, with standard deviations:

![Oister vs. REMEDI, standard deviation](./doc/images/experiments/servers/stats.ratio.r.vs.o.png "Oister vs. REMEDI, standard deviation")

As one can see, on a single thread REMEDI is approximately 70 times faster than Oister and on 25-40 threads it is >= 100 times faster. The other plots show the systems' performance relative to the fastest tool: Moses2:

![REMEDI vs. Moses2, standard deviation](./doc/images/experiments/servers/stats.ratio.m2.vs.r.png "REMEDI vs. Moses2, standard deviation")

![Oister vs. Moses2, standard deviation](./doc/images/experiments/servers/stats.ratio.m2.vs.o.png "Oister vs. Moses2, standard deviation")

![Moses vs. Moses2, standard deviation](./doc/images/experiments/servers/stats.ratio.m2.vs.m.png "Moses vs. Moses2, standard deviation")

An important thing here is that REMEDI is just approximately 2.5 to 1.5 times slower than Moses2. Also this seems to be mostly due to more efficient decoding algorithms and data structures of Moses2 and not the efficiency of its multi-threading. The average decoding time ratio plots are given in the figure below:

![Decoding times relative to Moses2](./doc/images/experiments/servers/stats.ratio.m2.vs.tools.png "Decoding times relative to Moses2")

This one shows that the difference between Moses and REMEDI is: *(i)* not that big; *(ii)* decreases with the increasing number of threads; *(iii)* is eliminated if #threads >= #cores.

##### Thread based scaling
One of the last but very important things to consider is the system's scaling factor with respect to its single threaded implementation. As before, below give us independent plots with standard deviations:

![REMEDI: Speed-up relative to a single thread, standard deviation](./doc/images/experiments/servers/stats.threads.remedi.png "REMEDI: Speed-up relative to a single thread, standard deviation")

![Oister: Speed-up relative to a single thread, standard deviation](./doc/images/experiments/servers/stats.threads.oister.png "Oister: Speed-up relative to a single thread, standard deviation")

![Moses: Speed-up relative to a single thread, standard deviation](./doc/images/experiments/servers/stats.threads.moses.png "Moses: Speed-up relative to a single thread, standard deviation")

![Moses2: Speed-up relative to a single thread, standard deviation](./doc/images/experiments/servers/stats.threads.moses2.png "Moses2: Speed-up relative to a single thread, standard deviation")

The next figure provides the average-value plots of all the systems:

![Speed-up relative to a single thread](./doc/images/experiments/servers/stats.threads.tools.png "Speed-up relative to a single thread")

As one can notice, the data of Moses and Moses2 is rather noisy, especially compared to that of REMEDI. Yet the results are representative and show clear trends. For example we see that Moses2 scales worst in the number of threads and a better scaling is exhibited by Oister that has batch-based parallelization strategy. A yet better system is Moses which goes up to 18 times efficiency on 30 cores. The best system is REMEDI, it shows clear and stable scalability until all of the cores are utilized. After that, the speed-up stays constant except for a small decline at 70 threads. However, looking at the increased standard deviation at the 70 threads point on the REMEDI's plot, we suspect that it is just a statistical outlier.

##### REMEDI load scaling
The figure below  shows how the REMEDI performance scales with the number of input sentences.

![REMEDI decoding times, 1788 vs 3576 sentences](./doc/images/experiments/servers/remedi-timing-1788-vs-3576.png "REMEDI decoding times, 1788 vs 3576 sentences")

Here, we took the translation times for the original Chinese corpus of 1788 sentences and the translation times of the new corpus obtained by copying the original one twice, to get 3576 sentences. As one can see the translation times for each number of threads for the double corpus have simply doubled. This indicates linear scaling in the number of sentences between these two experiments. The latter is a good sign of scalability, meaning that the translation tasks scheduling is efficient.

##### Conclusions
An extended experimental comparison between REMEDI, Oister, Moses and Moses2 gives an outstanding correlation with the state of the art systems, from which it follows that REMEDI:

* Has the best scaling capacity in the #threads;
* Is very stable in its decoding times;
* Is from 70 to 100 times faster than its predecessor: Oister;
* Reaches Moses in decoding times if #threads == #cores;
* Is rather close to Moses2 in its decoding times;

## General design
This section describes the designs of the provided software. Note that the designs below are schematic only and the actual implementation might deviate. Yet, they are sufficient to reflect the overall structure of the software.

Further, in [The general design](#the-general-design) section, we first provide the design design overview. Next, in [The translation server](#the-translation-server) section, we give some insights into the translation server's design. Later, in the [Multiple translation servers with load balancers](#multiple_translation_servers_with_load_balancers) section we talk about possible deployment configuration with the load balancer and its internal designs. At last, in [The text processing](#the-text-processing) section we give several examples of the deployments with text pre/post-processing server(s).

The designs were created using [Unified Modeling Language (UML)](http://www.uml.org/) with the help of the online UML tool called [UMLetino](http://www.umletino.com/).

### The general design
Consider the deployment diagram below. It shows the overview of the system design we have at the moment.

![The ultimate deployment Image](./doc/models/deployment/deployment_ideal.png "The ultimate deployment")

This design's main feature is that it is fully distributed. Let us discuss it in layers from left to right.

1. _The first layer_ - is the text processing layer, where can be zero, one or more text pre/post-processing application instances used by one or more clients in different configurations, see [The text processing](#the-text-processing) section for more details;
2. _The second layer_ - is the layer of translation clients which can be instances of the console and web clients provided with the project or some other third-party applications;
3. _The third layer_ - is the load balancing server layer used to distribute the workload between the translation servers and/or to aggregate those for multiple source/target language pairs. The Load Balancer component has the same interface as the Decoder component, allowing for hierarchical layer structure, see [Multiple translation servers with load balancers](#multiple_translation_servers_with_load_balancers) section below;
3. _The fourth layer_ - is a number of translation servers that execute translation jobs. Each translation server is responsible for translating one source language into one target language only;

The communication between the layers here are done using JSON-based communication protocol over WebSockets. JSON is a well established industrial format and from practice WebSockets is the fastest non-proprietary asynchronous communication protocol over TCP/IP. 

### The translation server
In this section we describe the internal design of the translation server application and its main components.

#### Single translation server
Let us consider the most trivial configuration to run our software. This configuration consists of a single translation server and multiple clients, as given on the picture below. 
![The current deployment Image](./doc/models/deployment/deployment_first.png "The current deployment")

As one can notice there is no load-balancing or text processing entity. This is the most trivial configuration to run the translation server. Let us now briefly consider the two most complicated components of the software: the _Decoder_ and the _Language model_.

##### The decoder component
The class diagram of the decoder component is given below. The decoder has a multi-threaded implementation, where each translation job (_a number of sentences to translate_) gets split into a number of translations tasks (_one task is one sentence_). Every translation task is executed in a separate thread. The number of translation threads is configurable at any moment of time.

![The decoder Image](./doc/models/decoder/decoder_component.png "The decoder")

Let us consider the main classes from the diagram:

The _translation\_server_ is responsible for: receiving the WebSocket session open and close requests; parsing the translation requests into translation jobs; scheduling the translation jobs to the _trans\_job\_pool; receiving the finished job notification; and sending the finished job reply to the client.

The _trans\_job\_pool_ stores all the scheduled translation jobs and splits them into the translation tasks scheduled by the _trans\_task\_pool_. Once all the translation tasks of a translation job are finished the _trans\_job_ notifies the _trans\_job\_pool and that, in its turn notifies the _translation\_server_.

The _trans\_task\_pool_ contains the queue of scheduled translation tasks and a limited number of translation worker threads to perform translations. In essence this is a thread pool entity with a queue of thread tasks.

The _trans\_task_ is a simple wrapper around the sentence translation entity _sentence\_decoder_. The latter's responsibility is to retrieve the preliminary information from the Language, Translation, and Reordering models and then to perform translations using the _multi\_tack_ class, and instances of _stack\_level_ and _stack\_state_ classes. The latter represents the translation expansion hypothesis. At present the translation algorithm supports:

* Beam search 
* Future cost estimates
* Threshold pruning of hypothesis
* Histogram pruning of hypothesis
* Hypothesis recombination

##### The LM component

Let us now consider the LM implementation class/package diagram on the figure below:

![The LM component Image](./doc/models/lm/lm_component.png "LM component")

The design of the Language model has not changed much since the split off from the [Back Off Language Model SMT](https://github.com/ivan-zapreev/Back-Off-Language-Model-SMT) project. So for more details we still refer to the [Implementation Details section](https://github.com/ivan-zapreev/Back-Off-Language-Model-SMT/blob/master/README.md#implementation-details) of the README.md thereof. For the most recent information on the LM component design please read the project's [Code documentation](#code-documentation).

### Multiple translation servers with load balancers
The Load Balancer (**bpbd-balancer**) has the same interface as the translation service (**bpbd-server**) so for the client (**bpbd-client** or **Web UI**) there is no difference with which of those to communicate. The Load Balancer serves two main purposes, it allows to:

* Distribute load between several translation services for the same source-target languages
* Aggregate services of several translation services for different/same source-target languages

The Figure below shows an example deployment of the multiple translation servers behind the two load balancer instances:

![The deployment with Load Balancer Image ](./doc/models/deployment/deployment_balancer.png "Load Balancer usage example")

Here the client application can be both console or web interface clients. Also a Load balancer can in its turn connect with another load balancer, as if it was a single translation service. This implements the composite design pattern of the distributed deployment.

The internal Load Balancer design is depicted in the Following figure. It's purpose it to introduce the implementation details of the load balancer with its main entities and their roles.

![The Load Balancer internals Image ](./doc/models/balancer/balancer_concepts.png "Load Balancer internals")

An example sequence diagram with several load balancing scenarios can be found below. This Figure shows the process of handling of an incoming translation job request. It also contains several optional scenarios for when s client session is dropped or the translation server closes the connection.

![The Load Balancer sequences Image ](./doc/models/balancer/balancer_sequence.png "Load Balancer sequences")

### The text processing
This section shows some deployment configurations in which the translation system, with text pre/post-processing, can be run. Note that, any pre/post-processing server in the figure below might support multiple languages and even language detection for pre-processing. Also, the decoders can be substituted with load balancer instances, spreading the translation load between multiple decoders.

![The Text Processor deployments Image ](./doc/models/deployment/processor_first.png "Text Processor deployments")

The reason why there is no load balancer capability for the pre/post-processing server at the moment is that these tasks are expected to be sufficiently less computation intensive, and are also optional, when compared to the text translation task itself.

Further, we shall briefly describe the possible translation system deployment configurations, specified on the figure above.

##### Type - 01: Only pre-processing, different physical servers
This is the situation when only the pre-processing is enabled. Whether the pre-processing script supports language identification or not will depend on the concrete pre-processing script implementation. If not, and it is requested, then an error is to be reported by the processing server. Since there is no post-processor, the resulting target text will be output by the translation system "as is", i.e. it will be tokenized and lower cased, and with just one sentence per line. This configuration can be recommended for systems with large load from the pre-processing script.

##### Type - 02: Only post-processing, different physical servers
This is the situation when only the post-processing is enabled. In this case the provided source text is supposed to be tokenized (words and punctuation marks are to be separated with single ASCII spaces), lower cased (all letters must be in lower case), unified (the longer UTF-8 character sequences are to be substituted with the equivalent but shorter ones), and there must be just one sentence per line in the source text file. The resulting target text will de untokenized and upper cased but one can still expect to get one target sentence per line in the output as there is no source text available to restore the original text structure. This configuration can be recommended for systems with large load from the post-processing scripts.

##### Type - 03: Pre- and post-processing, different physical servers
This is the situation when both pre- and post-processing are enabled. Yet, all of the servers are run on different physical computation nodes. Please note that, this complicates the situation when the post-processor script needs the source text for restoring the text structure. The complication comes from the fact that, even if the pre-processor script makes and stores a copy of the source text file, it is yet to be communicated to the server doing post-processing. This configuration can be recommended for systems with large load from the pre/post-processing scripts.

##### Type - 04: Pre- and post-processing, one physical server
This is the situation when both pre- and post-processing are enabled and are run together on one physical server. In this case, internal - temporary file sharing between the pre- and post-processing scripts becomes simpler. This configuration can be recommended for servers with multiple processors and shared hard drives or if the pre- and post- processing have low performance impact on the system. In the latter case, it might be easier to just run the configuration of *Type - 06*.

##### Type - 05: Separate applications on one physical server
This is the situation when both pre- and post-processing are enabled and are run together on one physical server with the translation system. In this case, internal - temporary file sharing between the pre- and post-processing scripts becomes simpler. This configuration can be recommended for servers with multiple processors and multiple shared hard drives or if the pre- and post- processing have low performance impact on the system or as a test configuration.

##### Type - 06: Pre- and post-processing, one application, one physical server
This is the situation when both pre- and post-processing are enabled and are run together within one application on one physical server. In this case, internal - temporary file sharing between the pre- and post-processing scripts becomes simpler. This configuration can be recommended for servers with multiple processors and multiple shared hard drives or if the pre- and post- processing have low performance impact on the system.

## Communication protocols
In this section we shall describe the communication protocols between the system applications mentioned in the section on [General design](#general-design). As it has already been noted, all the communications between the *bpbd-client*, *bpbd-server*, *bpbd-balancer*, *bpbd-processor*, and *translate.html* are based upon the [WebSockets](https://tools.ietf.org/html/rfc6455) communication protocol. The latter is a rather low-level messaging protocol allowing for asynchronous interaction between applications. In our framework it is used as a data transfer protocol we build our communications on. We chose the communicated message's payload to be formed using JSON - [JavaScript Object Notation](http://www.json.org/) which is a lightweight human-readable data-interchange format.

In essence, our applications interact by sending JSON message data objects to each other over WebSockets. There is just three types of communications (request-response messages) present in the system at the moment:

* **Supported languages - (SL)**: is used when one application needs to retrieve supported translation source/target language pairs from another application;
* **Translation - (T)**: is used when one application requires another application to perform some text translation from source language into a target one;
* **Pre-/Post-processing - (PP)**: is used when one application needs to pre/post-process some text, before/after translating it, in the given source/target language;

In the list above, we did not mention concrete application names as some of the communication types are common for multiple applications. To get a better insight into which communications are possible, consider the table below. The left-most column thereof contains application that can initiate this or that communication request. The top-most row contains application that can respond to this or that request with a proper response. The non-present relations or empty cells in the table mean no possible communications.

Request \ Response   | bpbd-server | bpbd-balancer | bpbd-processor |
---------------------|-------------|---------------|----------------|
**bpbd-client**      |  (SL), (T)  |  (SL), (T)    |      (PP)      |
**translate.html**   |  (SL), (T)  |  (SL), (T)    |      (PP)      |
**bpbd-balancer**    |  (SL), (T)  |  (SL), (T)    |                |

As indicated by the table above, **bpbd-client** and **translate.html** can only initiate requests, whereas **bpbd-server** and **bpbd-processor** can only produce responses. At the same time, **bpbd-balancer** does not only produce responses for **bpbd-client** and **translate.html**, but can also initiate requests for other instances of **bpbd-balancer** or **bpbd-server** applications.

Below we consider each of the aforementioned communication types in more details. We shall also provide the JSON format for each of them to facilitate development of/communication to the third-party client and/or server applications. We start by describing the common base class used for all communication messages.

### Common base class
In the actual C++ code design given below, one can see that the request and response message classes we have use intricate multiple inheritance.

![Messaging classes](./doc/models/messaging/messaging-3.0.png "Messaging classes")

Yet, when serialized into the JSON format the situation is much simpler. Consider the JSON object given below. It shall be seen as an instance of the common base class of all the communication messages we use in our software:

~~~json
{
	"prot_ver" : 0,
	"msg_type" : 5
}
~~~

Clearly, this base class has just two compulsory fields which are:

* **prot_ver** - *an unsigned integer* indicating the protocol version;
* **msg_type** - *an integer* indicating the message type;

The currently available message types are specified by the following C++ enumeration values:

~~~cpp
enum msg_type {
    //The message type is undefined
    MESSAGE_UNDEFINED = 0,
    //The supported languages request message
    MESSAGE_SUPP_LANG_REQ = 1,
    //The supported languages response message
    MESSAGE_SUPP_LANG_RESP = 2,
    //The translation job request message
    MESSAGE_TRANS_JOB_REQ = 3,
    //The translation job response message
    MESSAGE_TRANS_JOB_RESP = 4,
    //The pre-processor job request message
    MESSAGE_PRE_PROC_JOB_REQ = 5,
    //The pre-processor job response message
    MESSAGE_PRE_PROC_JOB_RESP = 6,
    //The post-processor job request message
    MESSAGE_POST_PROC_JOB_REQ = 7,
    //The post-processor job response message
    MESSAGE_POST_PROC_JOB_RESP = 8
};
~~~
All of the message classes discussed below inherit the **prot_ver** and **msg_type** fields.


### (SL) - Supported languages
The supported-languages request is used to retrieve the list of the supported source and target language pairs. At present, each **bpbd-server** only supports one source/target language pair. Being an aggregator of multiple translation services, each **bpbd-balancer** can support multiple language pairs.

#### JSON Request format
The request for supported languages does not extend the common base class, explained in section [Common base class](#common-base-class), with any new fields. All it takes to create an (SL) request is to instantiate the base class object with the proper value of its fields. See the example below which has the *msg_type* set to 1, being the message type of (SL) requests.

~~~json
{
	"prot_ver" : 0,
	"msg_type" : 1
}
~~~

#### JSON Response format
The (SL) response is supposed to return the list of supported language pairs. Clearly one source language can be translated into multiple target languages. This is illustrated with the following (SL) response example object.

~~~json
{
	"prot_ver" : 0,
	"msg_type" : 2,
	"langs" : {
               "german" : [ "french", "english"],
               "chinese" : [ "english" ]
             }
}
~~~

In this example, we see that *msg_type* is set to 2, indicating the this is an (SL) response and also there is an additional field **langs** which is *an object* storing the source to target language mappings. There, the object's field name indicates the source language and its value - which has to be *an array of strings* - stores the list of target languages. If the source language can not be translated in any language then it should not be present. In other words, an empty array of target languages is not supported.

Note that, the (SL) response does not support returning an error. This might be introduced in the latter protocol versions, but for now it is expected that each (SL) request should be handled properly. There is just two possibilities when it can fail:

1. The service does not support this type of request;
2. The service does support this type of requests but it is down;

Both of the above cases are currently considered as exceptional and only occur during development or setting up the infrastructure. Thus they are handled by simply returning a text error message through the WebSocket.

### (T) - Translation
Translation requests and responses are used to communicate the source text to the translation service and the resulting target text to its client. Clearly the source and target texts can be large and therefore our protocol supports splitting those texts into multiple translation requests and responses correspondingly.

#### JSON Request format
Each source text to be translated is split into a number of translation jobs, consisting of a number of translation tasks - being sentences. Within the client and server applications it is important to be able to keep track of all individual translation job requests, per client, to be able to identify which of them has been replied or not. There is also other things that need to be in the (T) request, such as: the request priority, the source and target language pair and etc. To see them all let us consider the (T) request example object below:

~~~json
{
	"prot_ver" : 0,
	"msg_type" : 3,
	"job_id" : 101,
	"priority" : 10,
	"source_lang" : "english",
	"target_lang" : "chinese",
	"is_trans_info" : true,
	"source_sent" : [ "how are you ?", "i was glad to see you ." , "let us meet again !"]
}
~~~

In the example above we have:

* **job_id** - *an unsigned integer* indicating the translation job id unique for the given client;
* **priority** - *an integer* storing the job priority where 0 means neutral and higher values mean higher priority;
* **source_lang** - *a string* with the source language;
* **target_lang** - *a string*  with the target language;
* **is_trans_info** - *a boolean flag* indicating whether we need to get an additional translation information with the job response. Such information includes but is not limited by the multi-stack loads and gets dumped into the translation log;
* **source_sent** - *an array of strings* which are sentences to be translated, appearing in the same order as they are present in the original text;

Note that, there is no limit on the number of sentences to be sent per request. However, the provided client implementations split the original text into a number of requests to improve the system's throughput in the multi-client environment.

#### JSON Response format
The translation job response message is supposed to indicate which translation request it corresponds to, contain the overall request status and the translated text - consisting of target sentences. The latter are attributed with translation status and (optionally) translation info, giving some insight into the translation process. Let us consider an example translation response below.

~~~json
{
	"prot_ver" : 0,
	"msg_type" : 4,
	"job_id" : 101,
	"stat_code" : 3,
	"stat_msg" : "Some translation tasks were canceled",
	"target_data" : [
							{
								"stat_code" : 2,
								"stat_msg" : "OK",
								"trans_text" : "你好吗 ？",
								"stack_load" : [ 3, 67, 90, 78, 40, 1 ]
							},
							{
								"stat_code" : 4,
								"stat_msg" : "The service is going down",
								"trans_text" : "i was glad to see you ."
							},
							{
								"stat_code" : 2,
								"stat_msg" : "OK",
								"trans_text" : "让我们再见面！",
								"stack_load" : [ 7, 76, 98, 90, 56, 47, 3 ]
							}
						]
}
~~~

In the example above, the status code is defined by the following C++ enumeration values:

~~~cpp
enum stat_code {
    //The status is not defined
    RESULT_UNDEFINED = 0,
    //The status is unknown
    RESULT_UNKNOWN = 1,
    //The translation was fully done
    RESULT_OK = 2,
    //Some sentences in the translation job were not translated
    RESULT_PARTIAL = 3,
    //The entire translation job/task was canceled
    RESULT_CANCELED = 4,
    //We failed to translate this entire translation job/task
    RESULT_ERROR = 5
};
~~~
Note that **stat_code** and **stat_msg**, storing the translation status, are given at the top level of a translation job  - indicating the overall status - and also at the level of each sentence. Also, **stack_load**, storing an array of stack loads in percent, is only present for a translated sentence if a translation info was requested. The latter is done by setting the **is_trans_info** flag in the corresponding translation job request. The order of translated sentence objects in the **target_data** array shall be the same as the order of the corresponding source sentences in the **source_sent** array of the translation job request.

### (PP) - Pre/Post processing
Text processing requests and responses are used to communicate the source/target texts to the text processing service for pre and post processing. Clearly the source and target texts can be large and therefore our protocol supports splitting those texts into multiple (PP) requests and responses. In case of text processing, we can not split a text in an arbitrary language into sentences at the client side. This would be too computationally intensive and would also require presence of corresponding language models at the client. Therefore, it has been decided to split text into UTF-8 character chunks of some fixed length. Let us consider the (PP) requests and responses in more details.

#### JSON Request format
An example (PP) request is given below. Here we give the pre-processor job request as indicated by the value of **msg_type**.

~~~json
{
	"prot_ver" : 0,
	"msg_type" : 5,
	"job_token" : "176393e9aae9b108767fafda109f4620",
	"priority" : 0,
	"num_chs" : 5,
	"ch_idx" : 1,
	"lang" : "auto",
	"text" : "學而時習之，不亦說乎？有朋自遠方來，不亦樂乎？人不知而不慍，不亦"
}
~~~

In the example object above we have the following fields:

* **job_token** - *a string* representing a semi-unique token specific for the text to be processed for the given client. For the pre-processor request (*msg_type == 5*) the initial value of **job_token** is chosen to be the MD5 sum of the entire text to be pre-processed. The pre-processor server can update the token to make it client-session specific. The (updated) token is always returned with the corresponding processor response. This new value is to be used for any subsequent post-processing request of the target text corresponding to the given source text.
* **priority** - *an integer* value of the job priority, the same meaning as for the (T) requests;
* **num_chs** - *an unsigned integer* giving the number of chunks the text is split into;
* **ch_idx** - *an unsigned integer* indicating the index of the current chunk, starts from zero;
* **lang** - *a string* storing the language of the text, or "**auto**" for the language auto detection. The latter is only allowed in case of the pre-processor job request, i.e. when *msg_type == 5*;
* **text** - *a string* storing text characters of the given chunk;

#### JSON Response format
An example (PP) response is given below. Here we give the pre-processor job response as indicated by the value of **msg_type**.

~~~json
{
	"prot_ver" : 0,
	"msg_type" : 6,
	"stat_code" : 2,
	"stat_msg" : "Fully pre-processed", 
	"job_token" : "176393e9aae9b108767fafda109f4620.1456",
	"num_chs" : 7,
	"ch_idx" : 1,
	"lang" : "chinese",
	"text" : "學而時習之 ，不亦說乎 ？有朋自遠方來 ，不亦樂乎 ？人不知而不"
}
~~~

In the example object above we have the following fields:

* **stat_code** - *an unsigned integer* indicating the status code of the response, the same meaning as for (T) responses;
* **stat_msg** - *a string* storing the response status message, the same meaning as for (T) responses;
* **job_token** - *a string* containing the job token. For the pre-processor response, it is updated by the text processing service: The original job token sent with the pre-processor request is made unique to the user session. This updated tokens is then to be used for any subsequent post-processing requests of the target text corresponding to this source text. For the post-processor response, the value of the job token does not change and is the same as sent with the post-processor request;
* **num_chs** - *an unsigned integer* indicating the number of chunks in which the pre/post-processed text is split for being sent back to the client;
* **ch_idx** - *an unsigned integer* value of the current chunk index, starts from zero;
* **lang** - *a string* storing the text's language. In case the pre-processor request was sent with the **lang** field value set to "auto", the pre-processor service supports language detection, and the language was successfully detected, this field will contains the name of the detected source language;
* **text** -  *a string* storing text characters of the given chunk;

## Software details
In this section we provide some additional details on the structure of the provided software. We shall begin with the common packages and then move on to the binary specific ones. The discussion will not go into details and will be kept at the level of source file folder, explaining their content.

Note that, to the possible extend the software is implemented via the header files located in the `[Project-Folder]/inc`. Due to the C++ language restrictions some of the header files do have corresponding C++ source files located in `[Project-Folder]/src`. The latter, to the necessary extend, follows the structure and file names found in `[Project-Folder]/inc`. Therefore, further we will only concentrate on the content of the `[Project-Folder]/inc` folder.

Additional information about the source code implementation classes can be found in the project's [Code documentation](#code-documentation).

### _common packages_
The project's common packages are located in `[Project-Folder]/inc/common`:

* `/messaging` - web-socket message related classes common for the bpbd balancer, server, processor, and client applications
* `/utils` - various utility classes and libraries needed for logging and etc.
    - `/cmd` - the common classed for the balancer/server console
    - `/containers` - some container type classes
    - `/file` - file-reading related classes
    - `/logging` - logging classes
    - `/monitor` - memory usage and CPU times monitor classes
    - `/text` - text utility classes and functions
    - `/threads` - the common classes used in multi-threading

### _bpbd-balancer_
All of the *bpbd-balancer* specific implementation classes are located in `[Project-Folder]/inc/balancer`.
Note that the balancer application, by nature, incorporates features of the client and server. Therefore, it uses the source code base of the _bpbd-client_ and _bpbd-server_, especially their messaging-related classes.

### _bpbd-processor_
All of the *bpbd-processor* specific implementation classes are located in `[Project-Folder]/inc/processor`.

* `/messaging` - client-server related communication classes

### _bpbd-client_
All of the *bpbd-client* specific implementation classes are located in `[Project-Folder]/inc/client`.

* `/messaging` - client-server related communication classes

### _bpbd-server_
All of the *bpbd-server* specific implementation classes are located in `[Project-Folder]/inc/server`:

* `/messaging` - client-server related communication classes
* `/common` - classes common to all server components
    - `/models` - model-related classes common to all server components
* `/decoder` - classes used in the decoder component
    - `sentence` - classes related to the top-level sentence decoding algorithms
    - `stack` - the multi-stack classes related to the stack-based decoding algorithms
* `/tm` - the translation model classes
    - `builders` - the model builder classes needed for reading the models
    - `models` - the model representation classes
    - `proxy` - the proxy objects implementing the local and/or remote model interface
* `/rm` - the reordering model classes
    - The same as for `/tm`.
* `/lm` - the language model classes
    - Similar to `/tm` and `/rm` but has some differences, see the next sub-section.

### _lm-query_
All of the *lm-query* specific implementation classes are located in `[Project-Folder]/inc/server/lm/`. The structure of this folder follows the general patters of that of `[Project-Folder]/inc/server/tm/` and `[Project-Folder]/inc/server/rm/` but has the following additional sub-folders:

* `/dictionaries` - dictionary/word-index related classes
* `/mgrams` - model and query m-gram related classes.

## Literature and references

This project is originally based on the following literature:

* Kenneth Heafield. "Kenlm: Faster and smaller language model queries." [BibTex](./doc/bibtex/Heafield_WMT11.bib)
* Philipp Koehn. "Statistical Machine Translation". [BibTex](./doc/bibtex/Koehn_SMT_Book10.bib)
* Mark Jan Nederhof, Giorgio Satta. "Prefix Probability for Probabilistic Synchronous Context-Free Grammars" [BibTex](./doc/bibtex/NederhofSatta_NLP11.bib)
* Adam Pauls, Dan Klein. "Faster and Smaller N-Gram Language Models" [BibTex](./doc/bibtex/PaulsKlein_ACL11.bib)
* Daniel Robenek, Jan Platos, Vaclav Snasel. "Efficient In-memory Data Structures for n-grams Indexing" [BibTex](./doc/bibtex/RobenekPlatosSnasel_DATESO13.bib)
* Andreas Stolcke, Jing Zheng, Wen Wang, Victor Abrash. "SRILM at Sixteen: Update and Outlook" [BibTex](./doc/bibtex/StolckeZhengWangAbrash_ASRU11.bib)
* Matthew Szudzik. "An Elegant Pairing Function" [BibTex](./doc/bibtex/Szudzik_NKS06.bib)

## Licensing
This is a free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This software is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

## History

* **21.04.2015** - Created
* **27.07.2015** - Changed project name and some to-do's
* **21.09.2015** - Updated with the latest developments preparing for the version 1, Owl release. 
* **11.03.2016** - Updated to reflect the latest project status. 
* **13.06.2016** - Added information on the `-c` option of the client. Introduced the server tuning mode for lattice generation. Updated description of model features and lambda's thereof. Prepared for the release 1.1.
* **22.07.2016** - Added information about the Web UI client and the Load Balancer. Updated the document with small changes in the existed text.
* **05.08.2016** - Added the description of the *bpbd-processor* and related script. Made the corresponding changes in the sections on the console client.
* **26.08.2016** - Updating the Web UI information, the interface was extended with the pre and post processor servers.
* **30.08.2016** - The information on translation clients was extended with translation priorities.
* **03.10.2016** - Removed an obsolete server console parameter description.
* **13.10.2016** - Added section about the communication protocols.
* **20.12.2016** - Updated with the decoder parameter's tuning scripts information.
* **15.03.2017** - Improved section on the software design. Added section on empirical comparison of various translation servers and how they scale with the increasing number of threads on multi-core machines.

## Appendix: LM query tests

### SRILM
Is a toolkit for building and applying statistical language models (LMs), primarily for use in speech recognition, statistical tagging and segmentation, and machine translation. It has been under development in the SRI Speech Technology and Research Laboratory since 1995. The employed tool version is **1.7.0**. The tool is run with the following command-line options:
```
% ngram -lm model-file -order 5 -ppl queries-file \
      -no-sos -no-eos -memuse -debug 0
```
No changes were done to the tool’s source code.

### KenLM
KenLM is a tool for estimating, filtering, and querying language models. The tool does not have clear version indication, so we used the tool’s GitHub snapshot of the Git revision:

_0f 306088c3d8b3a668c934f 605e21b693b959d4d_

KenLM does not allow to switch off the probability reports from the command line. Therefore we had to modify the tool’s code. In the `kenlm/lm/ngram query.hh` file we commented out the output code lines as follows:

```
struct BasicPrint {
  void Word(StringPiece, WordIndex, const FullScoreReturn &) const {}
  void Line(uint64_t oov, float total) const {
    //std::cout << "Total: " << total << " OOV: " << oov << ’\n’;
  }
  void Summary(double, double, uint64_t, uint64_t) {}
};
struct FullPrint : public BasicPrint {
  void Word(StringPiece surface, WordIndex vocab,
            const FullScoreReturn &ret) const {
    //std::cout << surface << ’=’ << vocab << ’ ’
    //<< static_cast<unsigned int>(ret.ngram_length)
    //<< ’ ’ << ret.prob << ’\t’;
}
  void Summary(double ppl_including_oov, double ppl_excluding_oov,
               uint64_t corpus_oov, uint64_t corpus_tokens) {
    std::cout <<
      "Perplexity including OOVs:\t" << ppl_including_oov << "\n"
      "Perplexity excluding OOVs:\t" << ppl_excluding_oov << "\n"
      "OOVs:\t" << corpus_oov << "\n"
      "Tokens:\t" << corpus_tokens << ’\n’
      ;
} };
```
After this change, the tool was run with the following command-line options: 18
``` 
% query -n model-file < queries-file
```

### Hardware configuration
The experiments were run on the following machine configuration:

```
[~ smt7 ~]$ lscpu
Architecture:          x86_64
CPU op-mode(s):        32-bit, 64-bit
Byte Order:            Little Endian
CPU(s):                40
On-line CPU(s) list:   0-39
Thread(s) per core:    2
Core(s) per socket:    10
Socket(s):             2
NUMA node(s):          2
Vendor ID:             GenuineIntel
CPU family:            6
Model:                 62
Stepping:              4
CPU MHz:               1200.000
BogoMIPS:              4999.23
Virtualization:        VT-x
L1d cache:             32K
L1i cache:             32K
L2 cache:              256K
L3 cache:              25600K
NUMA node0 CPU(s):     0-9,20-29
NUMA node1 CPU(s):     10-19,30-39
[~ smt7 ~]$ lsb_release -irc
Distributor ID:	CentOS
Release:	6.7
Codename:	Final
[~ smt7 ~]$ grep MemTotal /proc/meminfo
MemTotal:       264496688 kB
```

### Language models and query files
The considered language models and their sizes (in bytes) are:

```
[~ smt10~]$ ls -al *.lm
-rw-r--r-- 1     937792965 Sep 21 15:55 e_10_641093.lm
-rw-r--r-- 1    1708763123 Sep 21 17:36 e_20_1282186.lm
-rw-r--r-- 1    3148711562 Sep 21 17:45 e_30_2564372.lm
-rw-r--r-- 1    5880154140 Sep 21 18:09 e_40_5128745.lm
-rw-r--r-- 1   10952178505 Sep 21 18:29 e_50_10257490.lm
-rw-r--r-- 1   15667577793 Sep 21 20:22 e_60_15386235.lm
-rw-r--r-- 1   20098725535 Sep 21 20:37 e_70_20514981.lm
-rw-r--r-- 1   48998103628 Sep 21 21:08 e_80_48998103628.lm
```

The considered query files and their sizes are:

```
[~ smt10 ~]$ ls -al q_5_gram_1*.txt
-rw-r--r-- 1   2697064872 Sep 21 15:47 q_5_gram_100.000.000.txt
-rw-r--r-- 1           35 Sep 21 15:57 q_5_gram_1.txt
[~ smt10 ~]$ 
```

The number of m-grams per model is:

##### e\_10\_641093.lm

```
[~ smt10 ~]$ head -n 15 e_10_641093.lm
\data\
ngram 1=105682
ngram 2=1737132
ngram 3=5121040
ngram 4=7659442
ngram 5=8741158
```

##### e\_20\_1282186.lm

```
[~ smt10 ~]$ head -n 8 e_20_1282186.lm
\data\
ngram 1=143867
ngram 2=2707890
ngram 3=8886067
ngram 4=14188078
ngram 5=16757214
```

##### e\_30\_2564372.lm
```
[~ smt10 ~]$ head -n 8 e_30_2564372.lm
\data\
ngram 1=199164
ngram 2=4202658
ngram 3=15300577
ngram 4=26097321
ngram 5=31952150
```

##### e\_40\_5128745.lm

```
[~ smt10 ~]$ head -n 8 e_40_5128745.lm
\data\
ngram 1=298070
ngram 2=6675818
ngram 3=26819467
ngram 4=48897704
ngram 5=62194729
```

##### e\_50\_10257490.lm

```
[~ smt10 ~]$ head -n 8 e_50_10257490.lm
\data\
ngram 1=439499
ngram 2=10447874
ngram 3=46336705
ngram 4=90709359
ngram 5=120411272
```

##### e\_60\_15386235.lm

```
[~ smt10 ~]$ head -n 8 e_60_15386235.lm
\data\
ngram 1=568105
ngram 2=13574606
ngram 3=63474074
ngram 4=129430409
ngram 5=176283104
```

##### e\_70\_20514981.lm

```
[~ smt10 ~]$ head -n 8 e_70_20514981.lm
\data\
ngram 1=676750
ngram 2=16221298
ngram 3=78807519
ngram 4=165569280
ngram 5=229897626
```

##### e\_80\_48998103628.lm

```
[~ smt10 ~]$ head -n 8 e_80_48998103628.lm
\data\
ngram 1=2210728
ngram 2=67285057
ngram 3=183285165
ngram 4=396600722
ngram 5=563533665
```

## Appendix: Word lattice files
In this section we give detailed information on the format of word lattice files, discussed in Section [Word lattice generation](#word-lattice-generation).

Remember that *\<sentence-id\>* is an unsigned integer. For each new decoder instance the first received sentence gets an id zero. Also, `de_lattice_file_ext` and `de_feature_scores_file_ext` represent values defined by the server config file. *\<config-file-name\>* stands for the server configuration file name.

### Lattice file: *\<sentence-id\>.*`de_lattice_file_ext`

The structure of the word lattice file is then given by the following format:

```
TO_NODE_ID_1   FROM_NODE_ID_1|||TARGET_PHRASE|||SCORE_DELTA FROM_NODE_ID_2|||TARGET_PHRASE|||SCORE_DELTA ... FROM_NODE_ID_K|||TARGET_PHRASE|||SCORE_DELTA
...
<COVERVECS>TO_NODE_ID_1-FROM_NODE_ID_1:BEGIN_SOURCE_PHRASE_IDX:END_SOURCE_PHRASE_IDX TO_NODE_ID_1-FROM_NODE_ID_2:BEGIN_SOURCE_PHRASE_IDX:END_SOURCE_PHRASE_IDX …</COVERVECS>
```
The file consists of two distinct parts: the list of hypothesis nodes-to-node transitions, defining the translation graph, and the set of source phrase word cover indexes, enclosed in the single-line `<COVERVECS>` tag. Each nodes-to-node line begins with the id of the node we come to by an expansion, followed by the list of the hypothesis nodes from which we expand to the given one. Clearly, this list must contain at least one node and more nodes are possible in case of the to-node begin recombined with other nodes in the corresponding stack level. Note that, the `TO_NODE_ID_*` of the nodes that were recombined into another one are substituted with the id of the nodes they were recombined into.

More specifically, in the above:

* `TO_NODE_ID_*` - is a translation hypothesis node id for the hypothesis we expand into (a child hypothesis).
* `FROM_NODE_ID_*` - is a translation hypothesis node id for the hypothesis we expand from (a parent hypothesis).
* A three (3) space separated pair `TO_NODE_ID_*   FROM_NODE_ID_*` forms the begin of each graph transition line.
* `TARGET_PHRASE` - is a target translation phrase added to the translation result when expanding from one hypothesis to another. It can only be empty for the last dummy translation state, listed as first, in the first line of the sentence's  lattice file.
* `SCORE_DELTA` - the translation cost added to the translation result when expanding from one hypothesis to another.
* Each subsequent from-node in the list is single (1) space separated from the previous.
* The single-line `<COVERVECS>` tag contains a single (1) space separated list of the to- and from- node pairs attributed with the translated source phrase begin and end index.
* `BEGIN_SOURCE_PHRASE_IDX` - the begin word index of the phrase, in the source sentence, translated when expanding from one hypothesis to another.
* `END_SOURCE_PHRASE_IDX` - the end word index of the phrase, in the source sentence, translated when expanding from one hypothesis to another.

Note that the node ids begin with zero (0) and the lowest id is assigned to the sentence begin tag hypothesis **\<s\>**. The highest node id is assigned to the dummy hypothesis node which "aggregates" all the last-stack end tag hypothesis **\</s\>**. In the given format this dummy node is always present in the first line of the lattice file. The lattice dumping happens in a backwards fashion: from the end super state to the front of the stack.

Note that once the single sentence lattice file is combined with other sentence lattice files, the content of a single lattice file is wrapped with the `<SENT>` tag:

```
<SENT ID="sentence-id">
</SENT>
```
The latter has just one attribute `ID` which shall store the corresponding sentence id given by *\<sentence-id\>* from the original lattice file name.

### Scores file: *\<sentence-id\.*`de_feature_scores_file_ext`

The structure of the feature scores file is then given by the following format:

```
TO_NODE_ID_1 FEATURE_ID_A11=FEATURE_WEIGHT ... FEATURE_ID_A1N1=FEATURE_WEIGHT
...
TO_NODE_ID_Z FEATURE_ID_AZ1=FEATURE_WEIGHT ... FEATURE_ID_AZNZ=FEATURE_WEIGHT
```

Here each line contains the list of the corresponding non-zero feature weights that were added to the node's partial score by expanding from the parent hypothesis. The node ids and feature weights are single (1) space separated, and feature id values begin with zero. More specifically:

* `TO_NODE_ID_*` - is a translation hypothesis node id for the hypothesis we expand into (a child hypothesis).
* `FEATURE_ID_*` - the unique identifier, unsigned integer, of a model feature.
* `FEATURE_WEIGHT` - a non-zero value of the feature weight, not multiplied with the corresponding lambda.

The order of nodes in the file is not important. Let us also consider a few important notes:

1. The features used to compute the future costs of a hypothesis  are not taken into account. We only list features and weights thereof used in the current hypothesis partial score computation.
2. The feature weights are given as is in the model files, i.e. are not multiplied with corresponding lambda values from the server config file.
3. The feature weight of the Language model is taken to be the joint m-gram probability of the newly added target phrase with the given target translation history.
4. The word penalty is just the number of words in the target phrase.
5. The distortion penalty is the linear distortion cost without lambda.

Note that once the single sentence feature scores file is combined with other sentence feature score files, the content of a single feature scores file is wrapped with the `<SENT>` tag:

```
<SENT ID="sentence-id">
</SENT>
```
The latter has just one attribute `ID` which shall store the corresponding sentence id given by *\<sentence-id\>* from the original feature scores file name.

### Mapping file: *\<config-file-name\>.feature_id2name*

The structure of the id-to-feature-name mapping file is then given by the following format:

```
FEATURE_ID_1		FEATURE_NAME_1
...
FEATURE_ID_1		FEATURE_NAME_1
```

Here, each line contains a tab (`\t`) separated feature's global id  and feature name pair. The global feature ids are unsigned integers starting with zero. The feature names match those from the server's config file. In case a feature name corresponds to an array of values, we treat each array element as an individual feature. The name of a latter one is formed by the feature name plus a suffix of the array index in square brackets. Consider the following real-life example:

```
0       de_lin_dist_penalty
1       de_word_penalty
2       lm_feature_weights[0]
3       rm_feature_weights[0]
4       rm_feature_weights[1]
5       rm_feature_weights[2]
6       rm_feature_weights[3]
7       rm_feature_weights[4]
8       rm_feature_weights[5]
9       tm_feature_weights[0]
10      tm_feature_weights[1]
11      tm_feature_weights[2]
12      tm_feature_weights[3]
13      tm_feature_weights[4]
```

Note that in the example above, the phrase penalty is the translation model feature with index `4`, i.e. it corresponds to the global feature index `13`:

```
13      tm_feature_weights[4]
```

## Appendix: Translation performance tests

### REMEDI
This is the system developed within this project, called REMEDI, by Dr. Ivan S. Zapreev under the leadership of Dr. Christof Monz. The project is performed within the Information and Language Processing Systems group at the University of Amsterdam, The Netherlands.

This system is written mostly in C++ and supports multi-threading in the form of issuing a dedicated translation thread per source sentence. The employed Language, Translation and Reordering models are then shared in the multi-threaded environment between multiple sentences being translated.

Note that, REMEDI is made following a distributed client-server architecture. In our experiments a server is first started to load the models and prepare itself for performing translation tasks. Next a console client application is started. The latter sends the source text file, in batches, to the translation server and waits for all the translation to be received. 

For our experiments we used the git snapshot **949dc64f493aac4a2aede6a56d6d6f2ecc5cac0a** of the system. The first system's git repository commit dates as early as *Fri Apr 17 13:15:36 2015 +0200*. This means that the system exists for about 2 years and it is therefore relatively immature.

### Oister
This is a home-brewed system developed by Dr. Christof Monz et al. within the Information and Language Processing Systems group at the University of Amsterdam, The Netherlands.

This system is written mostly in Perl and supports multi-batching of the translated text corpus. The latter is done by splitting the text into a number of batches and translating each batch of sentences within a dedicated thread.

Oister does not provide a client-server architecture. It is a monolith system that, in order to do translations, each time has to load the required models. Moreover, Oister parallelizes translation process through batching: the sentences to be translated are split into a number of equally sized (in the number of sentences) chunks and then each chunk gets its translation thread to performance consequent sentences translation. This means that the translation time of Oister is always defined by the longest translation time among all the batches.

For our experiments we used the git snapshot **8e35f4a8bd3d54fe12b4b0aa36157248f34770ad** of the system. The first system's git repository commit dates as early as *Wed Sep 21 16:29:42 2011 +0200*. This means that the system exists for about 6 years now and this adds to its maturity.

### Moses/Moses2
This is the system developed by Prof. Dr. Philipp Koehn et al. at The Johns Hopkins University, Baltimore, US;

Since 2016, Moses comes in two versions. The first one, we shall keep calling Moses, is the evolutionary branch of the system. The second one, called Moses2, is a revolutionary drop-in replacement for the Moses decoder. The latter is specifically designed to be fast and scalable on multi-core machines. The only information available for Moses2 at the moment is present online on the [system's webpage](http://www.statmt.org/moses/?n=Site.Moses2).

Both Moses and Moses2 are written mostly in C++. The threading model adopted for multi-threaded Moses assigns each sentence to a distinct thread so that each thread works on its own decoding task, but shares models with the other threads. Moses2 contains an improved multi-threading model but a limited set of translation algorithms. It is claimed that Moses2 is faster than Moses on a multi-core machine using multiple translation threads due to its better multi-threading algorithms.

Both Moses and Moses2 can be run in a server mode but the implemented XML-RPC interface only allows to request a single sentence translation at a time and does not support asynchronous calls to the Moses servers (We considered all the provided Moses/Moses2 server client examples in Perl, Python and Java and none of them had multi-sentence or asynchronous calls implemented). For a single client, this situation completely eliminates the advantage of multi-threaded server implementation. Clearly, before the client can request the next sentence translation, via the provided XML-RPC interface, it has to wait until the previous sentence is translated and the result is returned. Therefore to get the maximum performance out of Moses/Moses2, we used these systems in their stand-alone console mode. I.e. we ran them in the same way as we did with Oister.
 
For our experiments we used the git snapshot **0af59a4cda442adb9dd3b04542292c61f70bb504** of the system. The first system's git repository commit dates as early as _Mon Jul 3 18:10:46 2006 +0000_. This means that the system exists for about 11 years now and this adds to its maturity.


### Test server
The machine's CPU is:

```
[~ smt10 ~]$ lscpu
  Architecture:          x86_64
  CPU op-mode(s):        32-bit, 64-bit
  Byte Order:            Little Endian
  CPU(s):                40
  On-line CPU(s) list:   0-39
  Thread(s) per core:    2
  Core(s) per socket:    10
  Socket(s):             2
  NUMA node(s):          2
  Vendor ID:             GenuineIntel
  CPU family:            6
  Model:                 62
  Model name:            Intel(R) Xeon(R) CPU E5-2670 v2 @ 2.50GHz
  Stepping:              4
  CPU MHz:               1200.000
  BogoMIPS:              4999.27
  Virtualization:        VT-x
  L1d cache:             32K
  L1i cache:             32K
  L2 cache:              256K
  L3 cache:              25600K
  ...
```

The machine features $16$ RAM modules:

```
  [~ smt10 ~]$ dmidecode --type memory
...
Memory Device
	Array Handle: 0x0029
	Error Information Handle: Not Provided
	Total Width: 72 bits
	Data Width: 64 bits
	Size: 16384 MB
	Form Factor: DIMM
	Set: None
	Locator: P1-DIMMA1
	Bank Locator: P0_Node0_Channel0_Dimm0
	Type: DDR3
	Type Detail: Registered (Buffered)
	Speed: 1333 MHz
	Manufacturer: Hynix Semiconductor
	Serial Number: 0B6D55E2     
	Asset Tag: DimmA1_AssetTag
	Part Number: HMT42GR7AFR4A-PB 
	Rank: 2
	Configured Clock Speed: 1333 MHz
...
```

![Markdown Logo](./doc/images/markdown.png "Markdown")

 _Powered by [Markdown-Cheatsheet](https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet)_
