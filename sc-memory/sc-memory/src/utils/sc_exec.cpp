/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc-memory/utils/sc_exec.hpp"

#include <istream>
#include <streambuf>
#include <string>
#include <algorithm>

#include "sc-memory/sc_debug.hpp"

class ScExecBuffer : public std::streambuf
{
public:
  explicit ScExecBuffer(sc_char const * command);

protected:
  std::string output;
};

ScExecBuffer::ScExecBuffer(sc_char const * command)
{
  std::array<sc_char, 256> buffer{};
  std::unique_ptr<FILE, int (*)(FILE *)> pipe(popen(command, "r"), pclose);
  if (!pipe)
    SC_THROW_EXCEPTION(utils::ExceptionInvalidState, "Invalid ScExec body");

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    output += buffer.data();

  setg((sc_char *)output.c_str(), (sc_char *)output.c_str(), (sc_char *)(output.c_str() + output.size()));
}

ScExec::ScExec(std::vector<std::string> const & content)
  : std::istream(nullptr)
  , m_buffer(nullptr)
{
  std::stringstream stream;

  if (!content.empty())
  {
    std::for_each(
        content.cbegin(),
        --content.cend(),
        [&stream](auto const & item)
        {
          stream << item << " ";
        });
    stream << *--content.cend();
  }

  m_buffer = new ScExecBuffer(stream.str().c_str());
  rdbuf(m_buffer);
}

ScExec::~ScExec()
{
  delete m_buffer;
}
