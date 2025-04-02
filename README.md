# UAV Network Simulation with ns-3 and ZeroMQ

## Prerequisites
Ensure you have the following dependencies installed:
- **Git**
- **CMake**
- **GCC** (or Clang)
- **Python** (for ns-3 build system)

## Installing vcpkg
Clone the vcpkg repository and bootstrap it:
```sh
# Clone vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg (Linux/macOS)
./bootstrap-vcpkg.sh

# Bootstrap vcpkg (Windows)
bootstrap-vcpkg.bat
```

## Configuring Environment Variables
Add vcpkg to your shell configuration file for persistent usage:

```sh
# Open .bashrc or .zshrc and add the following lines:
export VCPKG_ROOT=/path/to/vcpkg
export PATH=$VCPKG_ROOT:$PATH
```

Apply the changes:
```sh
source ~/.bashrc  # or source ~/.zshrc
```

## Installing Required Packages
Install the necessary libraries using vcpkg:
```sh
vcpkg install zeromq:x64-linux cppzmq:x64-linux nlohmann-json:x64-linux
```

## Verifying Installation
To check if vcpkg has successfully installed the required libraries, run:
```sh
vcpkg list | grep -E "zeromq|cppzmq|nlohmann-json"
```

If any library is missing, reinstall it using `vcpkg install <package>:x64-linux`.

## Cloning and Integrating UAV Network Simulation
Navigate to your ns-3 installation directory and create a new `zmq` folder in the `scratch/` directory:
```sh
cd ns-3-${VERSION}/scratch
mkdir zmq
cd zmq

# Clone the UAV network simulation repository
git clone https://github.com/dukeb1212/uav-netsim-ns3 .
```

## Updating ns-3 CMake Configuration
Modify `ns-3-${VERSION}/CMakeLists.txt` by adding the following lines to include vcpkg dependencies:

```cmake
# Include vcpkg build system
include(/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake)

# Find required packages
find_package(cppzmq REQUIRED)
find_package(ZeroMQ CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Include necessary headers
include_directories(/path/to/vcpkg/installed/x64-linux/include)
```

## Building ns-3 with ZeroMQ Support
Navigate to the ns-3 root directory and build the project:
```sh
cd ns-3-${VERSION}
./ns3 build
```

## Running the Simulation
After building, navigate to the `ns-3-${VERSION}` folder and execute the test application:
```sh
./build/test
```

## Configuration in `test.cc`
### ZMQ Publisher Address for others
```cpp
std::string zmqAddress = "tcp://192.168.1.62:5555"; // IP address to publish network metrics
```

### ZMQ Subscriber Configuration
```cpp
Ptr<ZmqReceiverApp> app = CreateObject<ZmqReceiverApp>();
nodes.Get(0)->AddApplication(app);
app->SetAttribute("Address", StringValue("192.168.1.61")); // Address of the publisher
app->SetAttribute("Port", UintegerValue(5555));            // Port
app->SetAttribute("ID", StringValue("ns3"));               // Subscribe topic
app->SetStartTime(Seconds(1));                             // Start application at 1s in simulation, end after 5 mins
app->SetStopTime(Seconds(300.0));
```

## Notes
- Ensure that **`ns-3` is built with CMake** and correctly detects the vcpkg dependencies.
- If you encounter issues, verify that the paths to vcpkg and ns-3 are correctly set in `CMakeLists.txt` and your environment variables.

---
This README provides step-by-step instructions to set up ns-3 with ZeroMQ using vcpkg. If you need additional support, refer to the official [vcpkg](https://github.com/microsoft/vcpkg) and [ns-3](https://www.nsnam.org/) documentation.

