#include <string>
#include <boost/exception/diagnostic_information.hpp>

#include "SwtLink.h"
#include "utils.h"

namespace fit_swt
{

void SwtLink::rpcHandler()
{
  processRequest(getString());
}

void SwtLink::processRequest(const char* swtSequence)
{
  splitLines(swtSequence);

  if (!parseFrames()) {
    sendFailure();
    return;
  }

  if (interpretFrames()) {
    execute();
  } else {
    std::cerr << "Sequence failed!" << std::endl;
    sendFailure();
  }
}

void SwtLink::splitLines(const char* swtSequence)
{
  std::string swtStr = swtSequence;
  m_lines = utils::splitString(swtStr, "\n");
}

bool SwtLink::parseFrames()
{
  m_frames.clear();
  if (m_lines[0] != "reset") {
    return false;
  }
  m_lines.erase(m_lines.begin());

  for (auto line : m_lines) {
    if (line.find("read") != std::string::npos) {
      m_frames.push_back({ 0, 0, 0 });
      continue;
    }

    line = line.substr(line.find("0x") + 2);
    line = line.substr(0, line.find(','));

    try {
      m_frames.emplace_back(stringToSwt(line.c_str()));
    } catch (const std::exception& e) {
      std::cerr << boost::diagnostic_information(e) << '\n';
      return false;
    }
  }

  return true;
}

bool SwtLink::interpretFrames()
{
  uint32_t buffer[2];
  int currentPacket = 0;
  m_packetsNumber = 1;
  const int packetSizePadding = 128;

  for (int i = 0; i < m_frames.size(); i++) {

    if (m_packets[currentPacket].requestSize + packetSizePadding >= ipbus::maxPacket) {
      std::cerr << "Max packet size exceeds, splitting packet...\n";
      currentPacket++;
      m_packetsNumber++;
    }

    if (m_frames[i].data == 0 && m_frames[i].address == 0 && m_frames[i].mode == 0) {
      continue;
    }

    switch (m_frames[i].getTransactionType()) {
      case Swt::TransactionType::Read:
        // std::cerr << "Read operation...\n";
        m_packets[currentPacket].addTransaction(ipbus::data_read, m_frames[i].address, &m_frames[i].data, &m_frames[i].data, 1);
        break;

      case Swt::TransactionType::Write:
        // std::cerr << "Write operation...\n";
        m_packets[currentPacket].addTransaction(ipbus::data_write, m_frames[i].address, &m_frames[i].data, &m_frames[i].data, 1);
        break;

      case Swt::TransactionType::RMWbits:
        // std::cerr << "RMWbits operation...\n";
        if (m_frames[i].mode != 2) {
          std::cerr << "RMWbits failed: first frame is not the AND frame" << std::endl;
          return false;
        }
        if (i + 1 >= m_frames.size()) {
          std::cerr << "RMWbits failed: second frame has been not received" << std::endl;
          return false;
        }
        if (m_frames[i + 1].data == 0 && m_frames[i + 1].address == 0 && m_frames[i + 1].mode == 0) {
          if (i + 2 >= m_frames.size()) {
            std::cerr << "RMWbits failed: second frame has been not received" << std::endl;
            return false;
          }
          if ((m_frames[i + 2].mode) != 3) {
            std::cerr << "RMWbits failed: invalid second frame - mode: " << m_frames[i + 2].mode << std::endl;
            return false;
          }
          buffer[0] = m_frames[i].data;
          buffer[1] = m_frames[i + 2].data;
          m_packets[currentPacket].addTransaction(ipbus::RMWbits, m_frames[i].address, buffer, &m_frames[i].data);
          i += 2;
        } else {
          if ((m_frames[i + 1].mode) != 3) {
            std::cerr << "RMWbits failed: invalid second frame - mode: " << m_frames[i + 1].mode << std::endl;
            return false;
          }
          buffer[0] = m_frames[i].data;
          buffer[1] = m_frames[i + 1].data;
          m_packets[currentPacket].addTransaction(ipbus::RMWbits, m_frames[i].address, buffer, &m_frames[i].data);
          i += 1;
        }

        break;

      case Swt::TransactionType::RMWsum:
        m_packets[currentPacket].addTransaction(ipbus::RMWsum, m_frames[i].address, &m_frames[i].data, &m_frames[i].data);
        break;

      default:
        break;
    }
  }

  return true;
}

void SwtLink::execute()
{
  for (int i = 0; i < m_packetsNumber; i++) {
    if (transcieve(m_packets[i]) == false) {
      sendFailure();
      return;
    }
  }
  createResponse();
}

void SwtLink::createResponse()
{
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
}

void SwtLink::writeFrame(Swt frame)
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

void SwtLink::sendFailure()
{
  setData("failure");
}

} // namespace fit_swt