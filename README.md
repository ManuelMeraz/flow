[![Build status](https://ci.appveyor.com/api/projects/status/ro4lbfoa7n0sy74c/branch/master?svg=true)](https://ci.appveyor.com/project/manuelmeraz/flow/branch/master)

![CMake](https://github.com/manuelmeraz/flow/workflows/CMake/badge.svg)

# Flow

The ideal autonomous system is able to react and adapt to its environment without delay. 
To facilitate this behavior data flow from the moment it's captured from it's environment to 
the moment of actuation must be as close to instantaneous as possible. The transformation from 
perception data to execution should be performed without overhead, while being correct. Flow 
aims to provide this framework.

#### Quick Start

##### Docker

The recommended to play with the library is to use the docker image. It will mount your home directory to the image.

The image will have all dependencies installed for this library. 

```bash
./docker/build.sh # pull the image and make a user
./docker/start.sh # run the container and attach to it
```

The `bash.sh` script may be used to re-attach to the container if you have exited. 

You may now build the library within the docker container and install it if you'd like. 

```bash
./scripts/build.py --enable-testing --enable-examples
```

If the build script doesn't build the project because of a crash try using the `--clear-cache` flash. 

Use the `--help` option for more information.

##### Building

Requires Ubuntu 20.04 for GCC >= 10.2 to install. 

```bash
# Install GCC >= 10.2 and make it the default
sudo apt install build-essential gcc-10 g++-10 
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100 --slave /usr/bin/g++ g++ /usr/bin/g++-10 --slave /usr/bin/gcov gcov /usr/bin/gcov-10

# install conan
sudo apt install python-pip 
pip install --user conan

# install Flow
git clone https://github.com/ManuelMeraz/Flow.git && cd Flow
sudo ./scripts/install_libraries.sh
mkdir build && cd build
cmake -DENABLE_TESTING=ON -DENABLE_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release ..
make -j
ctest -j
sudo make install 
```

## Table of Contents

1. [Core Concepts](#core-concepts)
   1. [Overview](#1-overview)
      1. [Functions](#1-1-functions)
      2. [Communication](#1-2-communication)
   2. [Examples](#2-examples)
2. [Milestones](#milestones)
3. [Dependencies](#dependencies)
    1. [Necessary Dependencies ](#1-necessary-dependencies)
    2. [Library Dependencies ](#2-library-dependencies)
    3. [Optional Dependencies ](#3-optional-dependencies)
       1. [Tools](#3-1-tools")
4. [Build Instructions](#build-instructions)
    1. [Build Directory](#1-build-directory)
    2. [Environment Variables](#2-environment-variables)
    3. [Configure](#3-configure)
        1. [Via GUI](#3-1-configure-via-gui)
        2. [Via CCmake](#3-2-configure-via-ccmake)
    4. [Build](#4-build)
5. [Troubleshooting](#troubleshooting)
    1. [Update Conan](#1-update-conan)
    2. [Clear Conan Cache](#2-clear-conan-cache)
    3. [Misconfiguration](#2-misconfiguration)
6. [Testing](#testing)



<a name="core-concepts"></a>
## Core Concepts

When I first learned about asynchrony and what its purpose was there
was confusion between [concurrency and asynchronous processes](https://stackoverflow.com/questions/4844637/what-is-the-difference-between-concurrency-parallelism-and-asynchronous-methods). 

After becoming more aware of asynchrony the rabbit holes led me down the world of [monads](https://www.youtube.com/watch?v=t1e8gqXLbsU) and functional programming
with haskell. I was looking at how asynchrony was being used by libraries, where at the time they were implemented with monadic interfaces. . 

It was difficult to grasp, but it was starting to make sense. I never built a monadic library because it seemed mentally tedious.
I started looking into C++ 20 and found out about coroutines. After diving deeper and deeper into coroutines, 
these were the solution to all the problems in robotics; really, [real time systems](https://gridem.blogspot.com/2018/07/heterogeneous-concurrent-exactly-once.html).

Most of the callback base frameworks like ROS (Robot Operating System) and even ROS2 are severely lacking in
real time capabilities because they ignore performance in their design. I learned C++ using ROS, so I'm not putting it down.
I think it's great for what it is. 

In ROS-land the major forms of communication are done through publishers and subscribers (producers and consumers) where
some loop is being done by a node object, or a while loop in the main program. This is strictly a multi-threaded form
of concurrency and strong synchronization is required to communicate within callbacks. We all know this is not
what we want with asynchronous functions.

**Note on the current state of the project**: Right now this is an pre-pre-pre-alpha. I've been working on this in my spare time
over the last few months. I've had to make a couple of redesigns, but I think this is the one that will work to server
as a base.

I have many more additions I want to add, such as support for TCP/IP and UDP, performance optimizations, and ergonomics
such as adding in a when_all to subscribe to multiple channels at once with a receiver or transformer.

<a name="1-overview"></a>
### Overview

The philosophy behind this framework is dependency management by preventing cyclical dependencies and creating
maximum flow in a network and thereby minimizing latency in the system (and probably increase throughput). This is 
meant to be used for closed feedback systems.

<a name="1-1-functions"></a>
### Functions
Each node in this graph represents an function, and the specific type of function is defined by the dependencies it
has.

1. *Spinner* - A spinner is an function with no dependencies and nothing depends on it. it's a closed system. 
    - Notation:  `()`
    - Example: In C++ this is a `()->void` function, or any other process that 
    
2. *Producer* - A producer is an function with no dependencies and some other function must depend on it. 
    - Notation:  `()->R`
    - Example: In C++ this is a `()->R` function, or any other process that emulates the behavior
    
3. *Consumer* - A receiver is an function with at least one dependency and nothing depends on it.
    - Notation:  `(A)`
    - Example: In C++ this is a `(A&&... a)->void` function, or any other process that emulates the behavior
    
3. *Transformer* - A transformer is an function with at least one dependency and at least one function depends on it.
    - Notation:  `(A)->R`
    - Example: In C++ this is a `(A&&... a)->R` function, or any other process that emulates the behavior
    
The flow network is composed of these 4 types of functions, and any functions where the dependencies are not satisfied
is an invalid network.

<a name="1-2-communication"></a>
### Communication
Each of these functions are connected to each other through a `channel`. Each channel needs to have
at least one producer and one receiver on the other end. A transformer doubles as a producer and receiver, 
so a path through the network may look something like this

`{()->A , (A)->B, (B)->C, (C)}` This network contains a producer, two transformers, and a receiver. It is complete 
and and closed. There will be three channels in between; each with its own channel name. If no channel name is provided,
then an empty string will be used; you can think of this as a *global channel*. 

A global channel is a channel that is available globally for that specific message type. Publishing an
`int` without a channel name will publish to the global `int` channel.

**Not yet implemented**: At the moment channels use a multi-producer scheme, so if only one producer exists in that
channel, then it is inefficient due to synchronization of atomics. There will be a way to make channels that are
single producer single receiver in the future. These will be done by tightly linking multiple functions and generating
private channels that are inaccessible through the main network. Think of it as creating a subnet within the network.

Each of the functions in the network will begin and start to process data and eventually reach a frequency.

Looking at the original example: `{()->A , (A)->B, (B)->C, (C)}`

At t0 the two transformers and receiver at the end will be waiting for messages and the producer will begin to 
produce data. This could be through a network socket that has no local dependencies (e.g. sensor data). 

At t1 The first transformer receives the first message and transforms it, and at the same time the producer begins
producing a second piece of data. 

This keeps going until all 4 functions are constantly communicating information to the final receiver with some
frequency.

<a name="1-3-Cancellation"></a>
### Cancellation

Cancellation of coroutines is tricky, but there is a logical way to cancel this large network. 

The producers begin the chain of functions, and the way to end the chain is by beginning with the receiver
at the end of the chain. when a cancellation request is performed the consumers at the end of the
network flow will begin by exiting their main loop. 

At this point, transformers and producers down the chain will be awaiting compute time for their coroutine. The receiver
will then `flush` out the waiting transformer or producer that is next in line, once that transformer is free the 
receiver will end. Then the transformer will repeat this until the producer is reached at the beginning of the chain 
and then the producer coroutines will end and exit their scope.


<a name="2-examples"></a>
### Examples

`example/minimal_producer_consumer`
```c++
#include <flow/flow.hpp>

// This is a producer
std::string hello_world() { return "Hello World"; }

// This is a consumer. Values are passed in by rvalue, 
// implying that the caller is now the owner of the data.
void subscribe_hello(std::string&& message){  }

int main()
{
  using namespace std::literals;

  /**
   * The producer hello_world creates a private channel to 
   * subscribe_hello and sends messsages at 10hz
   */
  auto net = flow::network(flow::chain(10hz) | hello_world | subscribe_hello);

  /**
   * Note: cancellation begins in 1 ms, but cancellation
   * is non-deterministic. 
   */
  net.cancel_after(1ms);

  flow::spin(std::move(net));
}
```


Example with transformers and multiple chains
```c++
#include <random>
#include <flow/flow.hpp>

std::random_device random_device{};
std::mt19937 random_engine{ random_device() };
std::uniform_int_distribution<int> distribution{ 1, 100 };

// producer that publishes to "sensor"
class Sensor {
public:
  int operator()()
  {
    return distribution(random_engine);
  }

  std::string publish_to() { return "sensor"; }

private:
};

// transformer that consumes an integer and produces an integer
int low_pass_filter(int&& data)
{
  static int limit = 30;
  return std::min(data, limit);
}

// transformer that consumes an integer and produces an integer
int high_pass_filter(int&& data)
{
  static int limit = 70;
  return std::max(data, limit);
}

// consumer that consumes an integer
void consume_data(int&& data) { }

int main()
{
  using namespace std::literals;
  
  // transformers subscribe to sensor channel and create a private channel to consume data 
  auto low_pass = flow::chain(10hz) | flow::transform(low_pass_filter, "sensor") | consume_data;
  auto high_pass = flow::chain(10hz) | flow::transform(high_pass_filter, "sensor") | consume_data;

  auto network = flow::network(Sensor{}, std::move(low_pass), std::move(high_pass));

  network.cancel_after(1ms);
  flow::spin(std::move(network));
}

```

<a name="milestones"></a>
## Milestones
| Version | Description                                                                  | ETA                |
|---------|------------------------------------------------------------------------------|--------------------|
| 0.1.0   | Ability to create in-memory network, send messages, and shut down reliably.  | 1/25/2021          |
| 0.1.1   | Ability to set frequency of routines. Use fflat buffers as messages          |                    |
| 0.1.2   | TCP, UDP, ICP, etc support to send receive messages efficiently.             | Mid-February 20201 |
| 0.1.3   | Can generate custom messages. Single producer single consumer channels.      | 2/12/2021          |
| 0.1.4   | Collect performance metrics and show in documentation                        | Mid-March 2021     |
| 0.1.5   | Create tools to tweak performance                                            | April 2021         |
| 0.1.6   | Optimization of implementation and add memory pool/allocator options         | May 2021           |
| 0.2     | All Major features complete                                                  | June 2021          |
| 0.2.1   | Compose functions, when_all, when_any                                        | Mid-June 2021      |
| 1.0.0   | Embedded support (single-threaded, no heap allocatinons), security           | Mid 2022           |

<a name="dependencies"></a>
## Dependencies

**Note**: Flow has only been tested on Ubuntu 20.04 with GCC 10.2

<a name="1-necessary-dependencies"></a>
### Necessary Dependencies
1. GCC >= 10.2 (Even the latest clang does not work)
See [cppreference.com](https://en.cppreference.com/w/cpp/compiler_support)
to see which features are supported by each compiler.
The following compilers should work:

  * [gcc 10.2+](https://gcc.gnu.org/)
    <details>
    <summary>Install command</summary>

    - Ubuntu 20.04:

        sudo apt install build-essential gcc-10 g++-10
        
        sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100 --slave /usr/bin/g++ g++ /usr/bin/g++-10 --slave /usr/bin/gcov gcov /usr/bin/gcov-10
      
    </details>

2. [Conan](https://conan.io/)
    <details>
    <summary>Install Command</summary>
    
    - Via pip - https://docs.conan.io/en/latest/installation.html#install-with-pip-recommended
        
            pip install --user conan
    
     - Windows:

        choco install conan -y

    - MacOS:

        brew install conan

   </details>

3. [CMake 3.15+](https://cmake.org/)
   <details>
   <summary>Install Command</summary>

    - Debian/Ubuntu:

        sudo apt-get install cmake

    - Windows:

        choco install cmake -y

    - MacOS:

        brew install cmake

   </details>

<a name="2-library-dependencies"></a>
### Library Dependencies
1. [liburing](https://github.com/axboe/liburing)
   <details>
   <summary>Install Command</summary>
        sudo ./scripts/install_liburing.sh
   </details>
2. [cppcoro](https://github.com/Garcia6l20/cppcoro)
   <details>
   <summary>Install Command</summary>
        sudo ./scripts/install_libcppcoro.sh
   </details>

<a name="3-optional-dependencies"></a>
### Optional Dependencies

<a name="3-1-tools"></a>
#### C++ Tools
  * [Doxygen](http://doxygen.nl/)
    <details>
    <summary>Install Command</summary>

    - Debian/Ubuntu:
        
            sudo apt-get install doxygen
            sudo apt-get install graphviz

    - Windows:
        
            choco install doxygen.install -y
            choco install graphviz -y

    - MacOS:
        
            brew install doxygen
            brew install graphviz

    </details>


  * [ccache](https://ccache.dev/)
    <details>
    <summary>Install Command</summary>

    - Debian/Ubuntu:
        
            sudo apt-get install ccache

    - Windows:
        
            choco install ccache -y

    - MacOS:
        
            brew install ccache

    </details>


  * [Cppcheck](http://cppcheck.sourceforge.net_impl/)
    <details>
    <summary>Install Command</summary>

    - Debian/Ubuntu:
        
            sudo apt-get install cppcheck

    - Windows:
        
            choco install cppcheck -y

    - MacOS:
        
            brew install cppcheck

    </details>


  * [include-what-you-use](https://include-what-you-use.org/)
    <details>
    <summary>Install Command</summary>

    Follow instructions here:
    https://github.com/include-what-you-use/include-what-you-use#how-to-install
    </details>


<a name="build-instructions"></a>
## Build Instructions

<a name="1-build-directory"></a>
### Build directory
Make a build directory:
```bash
mkdir build
```
<a name="2-environment-variables"></a>
### Specify the compiler using environment variables

By default (if you don't set environment variables `CC` and `CXX`), the system default compiler will be used.

Conan and CMake use the environment variables CC and CXX to decide which compiler to use. So to avoid the conflict issues only specify the compilers using these variables.

CMake will detect which compiler was used to build each of the Conan targets. If you build all of your Conan targets with one compiler, and then build your CMake targets with a different compiler, the project may fail to build.

<details>
<summary>Commands for setting the compilers </summary>

- Debian/Ubuntu/MacOS:
    
    Set your desired compiler (`clang`, `gcc`, etc):
        
    - Temporarily (only for the current shell)
    
        Run one of the followings in the terminal:
    
        - clang
        
                CC=clang CXX=clang++
            
        - gcc
        
                CC=gcc CXX=g++
    
    - Permanent:

        Open `~/.bashrc` using your text editor:
            
            gedit ~/.bashrc
            
        Add `CC` and `CXX` to point to the compilers:
            
            export CC=clang
            export CXX=clang++
            
        Save and close the file.

- Windows:

    - Permanent:
    
        Run one of the followings in PowerShell:
                
        - Visual Studio generator and compiler (cl)
            
                [Environment]::SetEnvironmentVariable("CC", "cl.exe", "User")
                [Environment]::SetEnvironmentVariable("CXX", "cl.exe", "User")
                refreshenv
            
          Set the architecture using [vsvarsall](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=vs-2019#vcvarsall-syntax):
            
                vsvarsall.bat x64

        - clang

                [Environment]::SetEnvironmentVariable("CC", "clang.exe", "User")
                [Environment]::SetEnvironmentVariable("CXX", "clang++.exe", "User")
                refreshenv
     
        - gcc

                [Environment]::SetEnvironmentVariable("CC", "gcc.exe", "User")
                [Environment]::SetEnvironmentVariable("CXX", "g++.exe", "User")
                refreshenv
     

  - Temporarily (only for the current shell):
        
            $Env:CC="clang.exe"
            $Env:CXX="clang++.exe"
            
</details>

<a name="3-configure"></a>
### Configure your build

To configure the project and write makefiles, you could use `cmake` with a bunch of command line options.
The easier option is to run cmake interactively:

<a name="3-1-configure-via-gui"></a>
#### **Configure via cmake-gui**:

1) Open cmake-gui from the project directory:
```
cmake-gui .
```
2) Set the build directory:

![build_dir](https://user-images.githubusercontent.com/16418197/82524586-fa48e380-9af4-11ea-8514-4e18a063d8eb.jpg)

3) Configure the generator:

In cmake-gui, from the upper menu select `Tools/Configure`.

**Warning**: if you have set `CC` and `CXX` always choose the `use default native compilers` option. This picks `CC` and `CXX`. Don't change the compiler at this stage!

<details>
<summary>Windows - MinGW Makefiles</summary>

Choose MinGW Makefiles as the generator:

<img src="https://user-images.githubusercontent.com/16418197/82769479-616ade80-9dfa-11ea-899e-3a8c31d43032.png" alt="mingw">

</details>

<details>
<summary>Windows - Visual Studio generator and compiler</summary>

You should have already set `C` and `CXX` to `cl.exe`.

Choose "Visual Studio 16 2019" as the generator:

<img src="https://user-images.githubusercontent.com/16418197/82524696-32502680-9af5-11ea-9697-a42000e900a6.jpg" alt="default_vs">

</details>

<details>

<summary>Windows - Visual Studio generator and Clang Compiler</summary>

You should have already set `C` and `CXX` to `clang.exe` and `clang++.exe`.

Choose "Visual Studio 16 2019" as the generator. To tell Visual studio to use `clang-cl.exe`:
- If you use the LLVM that is shipped with Visual Studio: write `ClangCl` under "optional toolset to use". 

<img src="https://user-images.githubusercontent.com/16418197/82781142-ae60ac00-9e1e-11ea-8c77-222b005a8f7e.png" alt="visual_studio">

- If you use an external LLVM: write [`LLVM_v142`](https://github.com/zufuliu/llvm-utils#llvm-for-visual-studio-2017-and-2019)
 under "optional toolset to use".

<img src="https://user-images.githubusercontent.com/16418197/82769558-b3136900-9dfa-11ea-9f73-02ab8f9b0ca4.png" alt="visual_studio">

</details>
<br/>

4) Choose the Cmake options and then generate:

![generate](https://user-images.githubusercontent.com/16418197/82781591-c97feb80-9e1f-11ea-86c8-f2748b96f516.png)

<a name="3-2-configure-via-ccmake"></a>
#### **Configure via ccmake**:
with the Cmake Curses Dialog Command Line tool:  

    ccmake .

Once `ccmake` has finished setting up, press 'c' to configure the project, 
press 'g' to generate, and 'q' to quit.

<a name="4-build"></a>
### Build
Once you have selected all the options you would like to use, you can build the 
project (all targets):

    cmake --build ./build

For Visual Studio, give the build configuration (Release, RelWithDeb, Debug, etc) like the following:

    cmake --build ./build -- /p:configuration=Release

<a name="troubleshooting"></a>
## Troubleshooting

<a name="1-update-conan"></a>
### Update Conan
Many problems that users have can be resolved by updating Conan, so if you are 
having any trouble with this project, you should start by doing that.

To update conan: 

    $ pip install --user --upgrade conan 

You may need to use `pip3` instead of `pip` in this command, depending on your 
platform.

<a name="2-clear-conan-cache"></a>
### Clear Conan cache
If you continue to have trouble with your Conan dependencies, you can try 
clearing your Conan cache:

    $ conan remove -f '*'
    
The next time you run `cmake` or `cmake --build`, your Conan dependencies will
be rebuilt. If you aren't using your system's default compiler, don't forget to 
set the CC, CXX, CMAKE_C_COMPILER, and CMAKE_CXX_COMPILER variables, as 
described in the 'Build using an alternate compiler' section above.

<a name="3-misconfiguration"></a>
### Identifying misconfiguration of Conan dependencies

If you have a dependency 'A' that requires a specific version of another 
dependency 'B', and your project is trying to use the wrong version of 
dependency 'B', Conan will produce warnings about this configuration error 
when you run CMake. These warnings can easily get lost between a couple 
hundred or thousand lines of output, depending on the size of your project. 

If your project has a Conan configuration error, you can use `conan info` to 
find it. `conan info` displays information about the dependency graph of your 
project, with colorized output in some terminals.

    $ cd build
    $ conan info .

In my terminal, the first couple lines of `conan info`'s output show all of the
project's configuration warnings in a bright yellow font. 

For example, the package `spdlog/1.5.0` depends on the package `fmt/6.1.2`.
If you were to modify the file `cmake/Conan.cmake` so that it requires an 
earlier version of `fmt`, such as `fmt/6.0.0`, and then run:

    $ conan remove -f '*'       # clear Conan cache
    $ rm -rf build              # clear previous CMake build
    $ mkdir build && cd build
    $ cmake ..                  # rebuild Conan dependencies
    $ conan info .

...the first line of output would be a warning that `spdlog` needs a more recent
version of `fmt`.

<a name="testing"></a>
## Testing
See [Catch2 tutorial](https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md)

To run the tests execute the `ctest` command from within the `build` directory
