/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#pragma once

#include <istream>
#include <string>

#include "sc-memory/sc_defines.hpp"

/// Class to create and execute system commands
class ScExec : public std::istream
{
public:
  /*! Creates command subprocess and execute it
   * @content Command content
   */
  _SC_EXTERN explicit ScExec(std::vector<std::string> const & content);

  _SC_EXTERN ~ScExec() override;

protected:
  class ScExecBuffer * m_buffer;
};
