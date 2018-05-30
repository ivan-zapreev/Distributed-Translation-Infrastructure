# **The basic translation infrastructure demo with TLS support**

**Author:** [Dr. Ivan S. Zapreev](https://nl.linkedin.com/in/zapreevis)

**Demo pages:** [Git-Hub-Demo-Pages](https://github.com/ivan-zapreev/Distributed-Translation-Infrastructure/tree/master/demo/tls)

**Parent project:** [Distributed Translation Infrastructure](https://github.com/ivan-zapreev/Distributed-Translation-Infrastructure)

## Introduction

This is a small demonstration set-up created for the sake of making a demonstration of the distributed translation infrastructure, assumed to be present in a `<DTI_HOME>` folder on the reader's hard drive. This demo features multiple translation services, basic text pre/post-processing, aggregation of multiple source language translation servers, and [TLS/SSL](https://en.wikipedia.org/wiki/Transport_Layer_Security)-based communications.

## Directories

Relative to the [Distributed Translation Infrastructure](https://github.com/ivan-zapreev/Distributed-Translation-Infrastructure) project home folder `<DTI_HOME>` the demo is located in `<DTI_HOME>/demo/tls/`. The structure of the demo is as follows:

```
$ tree .
├── README.md
├── certificates
│   ├── ca2048.crt
│   ├── ca2048.key
│   ├── ca2048.srl
│   ├── client2048.crt
│   ├── client2048.csr
│   ├── client2048.key
│   ├── dh1024.pem
│   ├── dh2048.pem
│   └── dh4096.pem
├── configs
│   ├── balancer-mixed-tls.cfg
│   ├── client-mixed-tls.cfg
│   ├── client-no-tls.cfg
│   ├── client-tls.cfg
│   ├── processor-no-tls.cfg
│   ├── processor-tls.cfg
│   ├── server-no-tls.cfg
│   └── server-tls.cfg
├── imgs
│   ├── browser_connected.png
│   ├── browser_exceptions.png
│   ├── browser_initial.png
│   └── browser_resulting.png
├── models
│   ├── chinese.english.head.10.rm.zip
│   ├── chinese.english.head.10.tm.zip
│   ├── de-en-1-10.000.rm.zip
│   ├── de-en-1-10.000.tm.zip
│   ├── e_00_1000.lm.zip
│   ├── english.bitext.lm.zip.aa
│   ├── english.bitext.lm.zip.ab
│   └── english.bitext.lm.zip.ac
├── start.sh
└── test
    ├── chinese.txt
    ├── english.txt
    └── german.txt
```

In the listing above:

- `./README.md` - the information file you are reading;
- `./certificates/` - the certificates and keys for SSL/TLS;
- `./configs/` - the server's configuration files; 
- `./imgs /` - the images used for this `README.md`
- `./models/` - the compressed model files;
- `./start.sh` - is the script to start-up and run the infrastructure;
- `./test/` - the source/target texts;

## SSL/TLS set-up

In order to run this demo one requires [OpenSSL](https://www.openssl.org/) installed on the DEMO machine as well as the [Distributed Translation Infrastructure](https://github.com/ivan-zapreev/Distributed-Translation-Infrastructure) being built with TLS support, see [Project's build instructions](../../../..#building-the-project) for more details. Note that, the SSL/TLS certificates used for all secure communications are stored in the `<DTI_HOME>/demo/tls/certificates` folder. These have been generated with the use of the `<DTI_HOME>/scripts/ssl` script and if expired can be easily re-generated.

## Deployment

All the servers are run on localhost but use different ports. The following deployment is ensured by the corresponding configuration files:

- **ws://localhost:9001** - `bpbd-server -c ./configs/server-tls.cfg` translating *Chinese* to * English*;
- **ws://localhost:9002** - `bpbd-server -c ./configs/server-no-tls.cfg` translating *German* to *English*;
- **wws://localhost:9003** - `bpbd-processor -c./configs/processor-tls.cfg` performs both *pre-* and *post-* processing;
- **ws://localhost:9004** - `bpbd-processor -c./configs/processor-no-tls.cfg` performs both *pre-* and *post-* processing;
- **wws://localhost:9005** - `bpbd-balancer -c ./configs/balancer-mixed-tls.cfg` balances load and distributes requests for all the translation servers;

## Important notes

* Text *pre-* and *post-* processing is done using the demonstration scripts `pre_process.sh` and `post_process.sh` located in `<DTI_HOME>/script/text/`;
* The `pre_process.sh` script has a dummy language detection, the language is always detected to be *German*;
* Both `pre_process.sh` and `post_process.sh` do not perform any actual text *pre* or *post* processing.
* One may get a proper text *pre* and *post* processing by:
    * Setting up the NLTK enabled *pre* and *post* processing scripts as described in [processor instructions](../../../..#pre-integrated-third-party-prepost-processing-scripts);
    * Changing the `./start.sh` script sources to soft-link `pre_process_nltk.sh` and `post_process_nltk.sh` instead of `pre_process.sh` and `post_process.sh`:

```
ln -f -s ${PROJECT_PATH}/script/text/pre_process_nltk.sh .pre_process.sh
ln -f -s ${PROJECT_PATH}/script/text/post_process_nltk.sh .post_process.sh
```
     
* The models used for the servers are either fake or have minimal size. This is due to size limitations of the repository;
* The translating from *German* to *English* will not provide any decent results due to used dummy models;
* The only remotely useful translation, enough for demonstration purposes, can be done from *Chinese* to *English*;
* The *Chinese* to *English* translation model is filtered to only provide decent translation for `./test/chinese.txt`;
* The reference *Chinese* to *English* translation of `./test/chinese.txt`, done on full-scale models, is located in `./test/english.txt`.

## Running

The demo is to be run in two modes, using:

- Console client (`<DTI_HOME>/build/bpbd-client`)
- Web client (`<DTI_HOME>/script/web/translate.html`)

Before these are used the servers infrastructure is to be started using the next steps:

* Open the Linux terminal and change directory to the `<DTI_HOME>/demo/tls/` folder;
* Run the `./start.sh` script from that same terminal;
* Wait until the following message appears in the terminal: 

```
INFO: Please press enter to STOP the servers and finish...
```

* From another terminal run `screen -r` to make sure there is *5* screens running. You may connect to each of them to get access to the corresponding server's command line.

After these steps are done the console and web clients may be run. The exact steps for doing these are given in the subsections below. To terminate the servers infrastructure and finish the demo:

* Return to the terminal running the `./start.sh` script and press **Enter**;

### Running Console client

There are four console client configurations to be run from terminal. To do that open a new terminal window and change directory to `<DTI_HOME>/demo/tls/` and run the next experiments:

#### Non-TLS improper translation request

```
../../build/bpbd-client -I ./test/chinese.txt -i Chinese -O ./english.txt -o English -c ./configs/client-no-tls.cfg
```

The client will use to the non-TLS *German* to *English* translation server running on port *9002* and request for *Chinese* to *English* translation. As a result the following error will be observed:

```
ERROR <trans_manager.hpp::process_job_result(...):413>: translation_server.hpp::check_source_target_languages(...):189: Unsupported source language: 'chinese' the server only supports: 'German'
```

The client also uses non-TLS *pre* and *post* processing server running on port *9004*.

#### Non-TLS proper translation request

```
../../build/bpbd-client -I ./test/german.txt -i German -O ./english.txt -o English -c ./configs/client-no-tls.cfg
```

The client will use the non-TLS *German* to *English* translation server running on port *9002* and request for * German* to *English* translation. Since the models are dummy the resulting file `./english.txt`, up to visible characters, will be equivalent to the original one (`./test/german.txt`) indicating that no text could be translated.

The client also uses non-TLS *pre* and *post* processing server running on port *9004*.

#### Mixed-TLS translation request

```
../../build/bpbd-client -I ./test/chinese.txt -i Chinese -O ./english.txt -o English -c ./configs/client-mixed-tls.cfg
```

The client will use to the TLS *Chinese* to *English* translation server running on port *9001* and will also use non-TLS *pre* and *post* processing server running on port *9004*. The run will produce the `./english.txt` file of the quality less but comparable with that of `./test/english.txt`.

#### Pure-TLS translation request

```
../../build/bpbd-client -I ./test/chinese.txt -i Chinese -O ./english.txt -o English -c ./configs/client-tls.cfg
```

The client will use the TLS *Chinese* to *English* and *German* to *English* aggregating translation service represented by the balancing server running on port *9005* and will also use TLS *pre* and *post* processing server running on port *9003*. The run will produce the `./english.txt` file of the quality less but comparable with that of `./test/english.txt`.

Note that, the balancing server, running on port *9005*, can only be reached via a secure (TLS) connection. However, it by itself aggregates two distinct type translation servers:

- A non-TLS *German* to *English* translation server running on port *9002*
- A TLS *Chinese* to *English* translation server running on port *9001*

### Running Web client

**ToDo:** Re-write this section.

* Open the `<DTI_HOME>/script/web/translate.html` document in one of the web-browsers: 

![The initial browser state](./imgs/browser_initial.png "The initial browser state")

* Note that the **Pre**, **Trans**, and **Post** connection indicators are all red, meaning the servers are not running;
* Open the Linux terminal and change directory to the `<DTI_HOME>/demo/tls/` folder;

* Keep refreshing the `<DTI_HOME>/script/web/translate.html` page in the browser, until it is in the following state:

![The connected browser state](./imgs/browser_connected.png "The connected browser state")

* Please note that here:
    * Getting this state shall not take longer than several minutes;
    * The **Pre**, **Trans**, and **Post** connection indicators will become green;
    * The **Source** language will be set to `--detect--`;
* Copy the content of the `<DTI_HOME>/demo/tls/test/chinese.txt` file into the **Source** text box of the `<DTI_HOME>/script/web/translate.html` document inside the browser;
* Select the **Source** language to be *Chinese* and click the **Translate** button;
* Wait until the translation process is finished, the resulting state shall be as follows:

![The resulting browser state](./imgs/browser_resulting.png "The resulting browser state")

* You may want compare the translation result to that stored in  `<DTI_HOME>/demo/tls/test/english.txt` and obtained using full-scale models;
