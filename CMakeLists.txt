# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

cmake_minimum_required(VERSION 3.9)

project(si470x_examples VERSION 0.8 LANGUAGES C CXX)

set(RDS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rds")
set(SI470X_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../si470x")

if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
  set(CMAKE_C_STANDARD_REQUIRED OFF)
  set(CMAKE_C_EXTENSIONS OFF)
endif(NOT CMAKE_C_STANDARD)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif(NOT CMAKE_CXX_STANDARD)

include(CheckIncludeFile)
check_include_file("wiringPi.h" HAVE_WIRING_PI_H)
check_include_file("linux/i2c-dev.h" HAVE_I2C_DEV_H)

include(CheckLibraryExists)
check_library_exists(wiringPi wiringPiSetupGpio "" HAVE_WIRINGPI)

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(usleep "unistd.h" HAVE_USLEEP)
check_cxx_symbol_exists(nanosleep "time.h" HAVE_NANOSLEEP)

if (HAVE_WIRING_PI_H)
  add_definitions(-DHAVE_WIRING_PI)
endif (HAVE_WIRING_PI_H)
if (HAVE_I2C_DEV_H)
  add_definitions(-DHAVE_I2C_DEV)
endif (HAVE_I2C_DEV_H)
if (HAVE_USLEEP)
  add_definitions(-DHAVE_USLEEP)
endif (HAVE_USLEEP)
if (HAVE_NANOSLEEP)
  add_definitions(-DHAVE_NANOSLEEP)
endif (HAVE_NANOSLEEP)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(rds_util "")
target_sources(rds_util
  PRIVATE
  "util/oda_decode.c"
  "util/oda_decode.h"
  "util/port_noop.c"
  "util/port_noop.h"
  "util/port_unix.c"
  "util/port_unix.h"
  "util/rds_spy_log_reader.cc"
  "util/rds_spy_log_reader.h"
  "util/rds_util.c"
  "util/rds_util.h"
)
target_include_directories(rds_util
  PUBLIC
    $<BUILD_INTERFACE:${RDS_LIB_DIR}/include>
    $<BUILD_INTERFACE:${SI470X_LIB_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/util>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_options(rds_util PRIVATE -Werror -Wall -Wextra)

link_directories(
  ${RDS_LIB_DIR}/build
  ${SI470X_LIB_DIR}/build
)

add_executable(rdsdisplay
  "example/unix/rdsdisplay.cc"
)
target_include_directories(rdsdisplay
  PUBLIC
    $<BUILD_INTERFACE:${RDS_LIB_DIR}/include>
    $<BUILD_INTERFACE:${SI470X_LIB_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/util>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(rdsdisplay rds_util)
target_link_libraries(rdsdisplay si470x)
target_link_libraries(rdsdisplay rds)
target_link_libraries(rdsdisplay ncurses)
target_link_libraries(rdsdisplay Threads::Threads)
target_compile_options(rdsdisplay PRIVATE -Werror -Wall -Wextra)
if(HAVE_WIRINGPI)
  target_link_libraries(rdsdisplay wiringPi)
endif(HAVE_WIRINGPI)


add_executable(rdsstats
  "example/unix/rdsstats.cc"
)
target_include_directories(rdsstats
  PUBLIC
    $<BUILD_INTERFACE:${RDS_LIB_DIR}/include>
    $<BUILD_INTERFACE:${SI470X_LIB_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/util>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(rdsstats rds_util)
target_link_libraries(rdsstats si470x)
target_link_libraries(rdsstats rds)
target_link_libraries(rdsstats ncurses)
target_link_libraries(rdsstats Threads::Threads)
target_compile_options(rdsstats PRIVATE -Werror -Wall -Wextra)
if(HAVE_WIRINGPI)
  target_link_libraries(rdsstats wiringPi)
endif(HAVE_WIRINGPI)
