#ifndef SWTELECTRONICS_H
#define SWTELECTRONICS_H

#include "IPbusInterface.h"
#include "Swt.h"
#include "dimrpcparallel.h"
#include <string>
#include <boost/asio.hpp>
#include <map>
#include <boost/log/trivial.hpp>

namespace fit_swt
{

class SwtLink : public ipbus::IPbusTarget, DimRpcParallel
{
 public:
  SwtLink(std::string rpc, boost::asio::io_context& ioContext, std::string address = "172.20.75.175", uint16_t rport = 50001, uint16_t lport = 0) : IPbusTarget(ioContext, address, lport, rport),
                                                                                                                                                    DimRpcParallel(rpc.c_str(), "C", "C", 0)
  {
    BOOST_LOG_TRIVIAL(trace) << "Trace log test";
    BOOST_LOG_TRIVIAL(debug) << "Debug log test";
    BOOST_LOG_TRIVIAL(info) << "Info log test";
    BOOST_LOG_TRIVIAL(warning) << "Warning log test";
    BOOST_LOG_TRIVIAL(error) << "Error log test";
    BOOST_LOG_TRIVIAL(fatal) << "Fatal log test";
  }

  void rpcHandler();
  void processRequest(const char* swtSequence);
  void writeFrame(Swt frame);
  void sendFailure();

  void splitLines(const char* swtSequence);
  bool parseFrames();
  bool interpretFrames();

  void execute();
  void createResponse();

 private:
  ipbus::IPbusControlPacket m_packet;
  std::vector<std::string> m_lines;
  std::vector<Swt> m_frames;
  std::string m_response;
};

} // namespace fit_swt

#endif // SWTELECTRONICS_H
