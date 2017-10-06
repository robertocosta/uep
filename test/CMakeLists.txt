#include_directories(../src)
link_libraries(${Boost_LIBRARIES})

set(tests
  test_block_decoder
  test_block_encoder
  test_counters
  test_data_client_server
  test_encoder_decoder
  test_lazy_xor
  test_message_passing
  test_nal_rw
  test_packets
  test_packet_rw
  test_protobuf_rw
  test_rng
  test_uep_encdec
)

foreach(t IN LISTS tests)
  add_executable(${t} ${t}.cpp)
  add_test(
    NAME ${t}
    COMMAND ${t}
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  )
endforeach(t)

target_link_libraries(test_rng rng)
target_link_libraries(test_data_client_server
  block_encoder
  decoder
  log
  packets_rw
  uep_decoder
)
target_link_libraries(test_packets packets)
target_link_libraries(test_block_decoder block_decoder)
target_link_libraries(test_block_encoder block_encoder)
target_link_libraries(test_encoder_decoder
  block_encoder
  decoder
)
target_link_libraries(test_message_passing packets log)
target_link_libraries(test_packet_rw packets_rw)
target_link_libraries(test_lazy_xor packets)
target_link_libraries(test_uep_encdec
  block_encoder
  decoder
  uep_decoder
)
target_link_libraries(test_nal_rw
  packets
  nal_reader
  nal_writer
)
target_link_libraries(test_protobuf_rw
  packets
  protobuf_rw
  controlMessage.pb
  log
)
