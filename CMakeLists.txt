add_executable(scratch_zmq_test_zmq
  test_zmq.cc          # Main test file
  zmq_receiver_app.cc  # ZMQ receiver implementation
  priority/priority-tag.cc
  priority/priority-tx-queue.cc
  priority/qos-config.cc
  uav/uav-application.cc
  uav/uav-telemetry.cc
  uav/uav-command.cc
)
add_executable(test test.cc zmq_receiver_app.cc priority/priority-tag.cc
priority/priority-tx-queue.cc
priority/qos-config.cc
uav/uav-application.cc
uav/uav-telemetry.cc
uav/uav-command.cc)

add_executable(video_stream videoStreamTest.cc)

add_executable(uav 
  test_uav.cc
  priority/priority-tag.cc
  priority/priority-tx-queue.cc
  priority/qos-config.cc
  uav/uav-application.cc
  uav/uav-telemetry.cc)

add_executable(congestion
  test_congestion.cc
  priority/priority-tag.cc
  priority/priority-tx-queue.cc
  priority/qos-config.cc
  uav/uav-application.cc
  uav/uav-telemetry.cc
  uav/uav-video-client.cc
  uav/uav-video-server.cc
  uav/uav-qos-config.cc)
#add_executable(indoor indoor.cc)
# Link ZeroMQ and cppzmq libraries
target_link_libraries(scratch_zmq_test_zmq PRIVATE libzmq libzmq-static nlohmann_json::nlohmann_json ${ns3-libs})
target_link_libraries(test PRIVATE libzmq libzmq-static nlohmann_json::nlohmann_json ${ns3-libs})
target_link_libraries(video_stream PRIVATE ${ns3-libs})
target_link_libraries(uav PRIVATE ${ns3-libs})
target_link_libraries(congestion PRIVATE ${ns3-libs})
#target_link_libraries(indoor PRIVATE libzmq libzmq-static nlohmann_json::nlohmann_json ${ns3-libs})