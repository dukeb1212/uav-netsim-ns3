add_executable(scratch_zmq_test_zmq
  test_zmq.cc          # Main test file
  zmq_receiver_app.cc  # ZMQ receiver implementation
)
add_executable(test test.cc zmq_receiver_app.cc)
#add_executable(indoor indoor.cc)
# Link ZeroMQ and cppzmq libraries
target_link_libraries(scratch_zmq_test_zmq PRIVATE libzmq libzmq-static nlohmann_json::nlohmann_json ${ns3-libs})
target_link_libraries(test PRIVATE libzmq libzmq-static nlohmann_json::nlohmann_json ${ns3-libs})
#target_link_libraries(indoor PRIVATE libzmq libzmq-static nlohmann_json::nlohmann_json ${ns3-libs})