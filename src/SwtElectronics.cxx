#include "SwtElectronics.h"
#include "utils.h"
#include <string>

namespace fit_swt
{

void SwtElectronics::rpcHandler()
{
  processRequest(getString());
}

void SwtElectronics::processRequest(const char* swtSequence)
{
  splitLines(swtSequence);
  parseFrames();
  execute();
}

void SwtElectronics::splitLines(const char* swtSequence)
{
  std::string swtStr = swtSequence;
  m_lines = utils::splitString(swtStr, "\n");
}

void SwtElectronics::parseFrames()
{
  m_frames.clear();
  m_lines.erase(m_lines.begin());

  for (auto frame : m_lines) {
    if (frame.find("write") == std::string::npos)
      continue;

    frame = frame.substr(frame.find("0x") + 2);
    int size = frame.size();
    for (int i = frame.find(','); i < size; i++)
      frame.pop_back();

    try {
      m_frames.emplace_back(stringToSwt(frame.c_str()));
      ipbus::TransactionType type = (m_frames.back().getTransactionType() == Swt::TransactionType::Read) ? ipbus::data_read : ipbus::data_write;
      m_packet.addTransaction(type, m_frames.back().address, &m_frames.back().data, 1);
    } catch (const std::exception& e) {
      std::cerr << e.what() << '\n';
    }
  }
}

void SwtElectronics::execute()
{
  if (transcieve(m_packet)) {
    m_response = "success ";
    for (int i = 0; i < m_lines.size(); i++) {
      if (m_lines[i] == "read") {
        writeFrame(m_frames[i - 1]);
        m_response += "\n";
        continue;
      } else if (m_lines[i].find("write") != std::string::npos) {
        m_response += "0\n";
      }
    }
    setData(m_response.c_str());
  } else {
    setData("failure");
  }
}

void SwtElectronics::writeFrame(Swt frame)
{
  m_response += "0x";
  HalfWord h;
  h.data = frame.mode;

  std::string mode = halfWordToString(h);
  mode = mode.substr(1);
  m_response += mode;

  Word w;
  w.data = frame.address;
  m_response += wordToString(w);
  w.data = frame.data;
  m_response += wordToString(w);
}

}