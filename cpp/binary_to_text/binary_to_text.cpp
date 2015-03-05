/**
  Read Motion binary take stream files and output a plain text file.
  The intended purpose is to provide raw and sensor data export to
  programs like Excel with using the Motion Service to export the data.

  @file    tools/sdk/cpp/binary_to_text/binary_to_text.cpp
  @author  Luke Tokheim, luke@motionnode.com
  @version 2.0

  Copyright (c) 2014, Motion Workshop
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
#include <File.hpp>
#include <Format.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <stdexcept>

#include <cstring>


const std::size_t MaxOptionLength = 1024;
const std::size_t MinChannel = 9;
const std::size_t MaxChannel = 10;
const std::string ChannelName[MaxChannel] = {
  "ax", "ay", "az",
  "mx", "my", "mz",
  "gx", "gy", "gz",
  "temp"
};

template <typename T>
void print_fields(std::ostream &out,
                  const T &data,
                  const std::size_t &data_size,
                  const std::string &separator,
                  bool is_accel)
{
  if ((data_size >= MinChannel) && (data_size <= MaxChannel)) {
    for (std::size_t i=0; i<data_size; ++i) {
      if (!is_accel || (i < 3) || (i >= 9)) {
        out
          << data[i];
        if (i < data_size - 1) {
          out << separator;
        }
      }
    }

    out << std::endl;
  }
}

void print_header(std::ostream &out,
                  const std::size_t &data_size,
                  const std::string &separator,
                  bool is_accel)
{
  print_fields(out, ChannelName, data_size, separator, is_accel);
}

template <typename T>
void print_container(std::ostream &out,
                     const T &data,
                     const std::string &separator,
                     bool is_accel)
{
  print_fields(out, data, data.size(), separator, is_accel);
}

template <typename ElementT>
bool binary_to_text(const std::string &pathname,
                    std::ostream &out,
                    bool show_channel_names,
                    const std::string & separator)
{
  bool result = true;

  try {
    using Motion::SDK::File;
    using Motion::SDK::Format;

    File file(pathname);

    typename ElementT::data_type data(ElementT::Length + 1);
    if (file.readData(data)) {
      // Detect MotionNode Accel data streams.
      bool is_accel = ElementT::Length > 3;
      for (std::size_t i=3; i<ElementT::Length; ++i) {
        if (0 != data[i]) {
          is_accel = false;
          break;
        }
      }

      if (0 == data.back()) {
        if (show_channel_names) {
          print_header(out, data.size(), separator, is_accel);
        }
      } else {
        typename ElementT::data_type::value_type tmp = data.back();
        data.resize(ElementT::Length);

        if (show_channel_names) {
          print_header(out, data.size(), separator, is_accel);
        }

        print_container(out, data, separator, is_accel);

        typename ElementT::data_type next_data(ElementT::Length - 1);
        if (file.readData(next_data)) {
          data.front() = tmp;
          std::copy(
            next_data.begin(), next_data.end(),
            data.begin() + 1);
        } else {
          data.clear();
        }
      }

      if (!data.empty()) {
        do {
          print_container(out, data, separator, is_accel);
        } while (file.readData(data));
      }
    }

  } catch (std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
    result = false;
  }

  return result;
}

bool binary_to_text(const std::string &input_file,
                    std::ostream &out,
                    bool raw_format,
                    bool show_channel_names,
                    const std::string &separator)
{
  using Motion::SDK::Format;

  bool result = false;
  if (raw_format) {
    if (binary_to_text<Format::RawElement>(input_file, out, show_channel_names, separator)) {
      result = true;
    }
  } else {
    if (binary_to_text<Format::SensorElement>(input_file, out, show_channel_names, separator)) {
      result = true;
    }
  }

  return result;
}

void print_usage(const std::string &name, std::ostream &out)
{
  // Print usage.
  out
    << "Usage: " << name << " [OPTION]... FILENAME[...]" << std::endl
    << "Read a Motion Take binary sensor or raw stream file and output a plain text, comma separated version." << std::endl
    << std::endl
    << "Options" << std::endl
    << "-f, --file FILENAME       output results to a file, use - for standard output" << std::endl
    << "-h, --help                prints this message" << std::endl
    << "-r, --raw                 input files are raw format data files, default is sensor format" << std::endl
    << "-n, --nonames             do not print the channel name headers" << std::endl
    << "-s, --separator STRING    element delimiter string, default is \",\" (CSV)" << std::endl
    << std::endl;
}

// Entry point. Parse arguments and call worker function
// for the intended input and output data formats.
int main(int argc, char **argv)
{
  int result = 1;

  // Initialize all options.
  std::string name;
  if (argc > 0) {
    name.assign(argv[0]);
  }

  std::vector<std::string> input_file;
  std::string separator = ",";
  bool raw_format = false;
  bool show_channel_names = true;
  bool output_stdout = false;
  std::string output_file;

  // Parse command line options. Override the defaults we
  // just set above.
  bool valid_command_line = true;
  for (int i=1; i<argc; i++) {
    const std::size_t length = strlen(argv[i]);

    // Look for an option of the form:
    // /option, --option, -option, or -o
    if ((length > 1) && (('-' == *argv[i]) || ('/' == *argv[i]))) {
      std::string option(argv[i] + 1, length-1);
      std::transform(
        option.begin(), option.end(),
        option.begin(),
        tolower);

      // Remove a leading '-' character if it exists.
      if (!option.empty() && ('-' == option[0])) {
        option = option.substr(1);
      }

      // Do the actual option handling and assign
      // local flags.
      if ("file" == option || "f" == option) {
        if (argc >= i) {
          i++;
          std::string filename(argv[i]);
          if ("-" == filename) {
            output_stdout = true;
          } else {
            output_file.assign(argv[i]);
          }
        } else {
          std::cerr
            << "invalid option, missing argument: " << std::string(argv[i], length) << std::endl;
          valid_command_line = false;
        }
      } else if ("separator" == option || "s" == option) {
        if (argc >= i) {
          i++;
          separator.assign(argv[i]);
        } else {
          std::cerr
            << "invalid option, missing argument: " << std::string(argv[i], length) << std::endl;
          valid_command_line = false;
        }
      } else if ("raw" == option || "r" == option) {
        raw_format = true;
      } else if ("nonames" == option || "n" == option) {
        show_channel_names = false;
      } else if ("help" == option || "h" == option) {
        valid_command_line = false;
      } else {
        std::cerr
          << "unknown option: " << std::string(argv[i], length) << std::endl;
        valid_command_line = false;
      }

    } else if (length > 0) {
      // Anything no captured by an option is considered to be a file name.
      input_file.push_back(std::string(argv[i], length));
    }
  }

  // If we have valid options and at least one input file, run the conversion loop.
  // Otherwise, print out the usage information.
  if (valid_command_line && !input_file.empty()) {

    std::ofstream *fout = NULL;
    if (!output_stdout && !output_file.empty()) {
      fout = new std::ofstream(output_file.c_str(), std::ios_base::binary | std::ios_base::out);
      if (!fout->is_open()) {
        delete fout;
        fout = NULL;
      
        std::cerr
          << "failed to open output file, \"" << output_file << "\""
          << std::endl;
      }
    }

    // There is no output file name list and we did not select the
    // standard output. Generate output file names based on the input
    // file names.
    const bool auto_file_name = !output_stdout && (NULL == fout);

    result = 0;
    for (std::vector<std::string>::const_iterator itr=input_file.begin(); itr!=input_file.end(); ++itr) {
      if (auto_file_name) {
        output_file = (*itr) + ".csv";

        fout = new std::ofstream(output_file.c_str(), std::ios_base::binary | std::ios_base::out);
        if (!fout->is_open()) {
          delete fout;
          fout = NULL;
        }
      }

      if (!binary_to_text(*itr, (NULL != fout) ? *fout : std::cout, raw_format, show_channel_names, separator)) {
        result = 1;
      }

      if (auto_file_name) {
        if (NULL != fout) {
          fout->close();
          delete fout;
          fout = NULL;
        }
      }
    }

    if (NULL != fout) {
      fout->close();
      delete fout;
      fout = NULL;
    }

  } else {
    print_usage(name, std::cerr);
  }

  return result;
}

