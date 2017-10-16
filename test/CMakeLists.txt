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

add_custom_target(test_filter ALL
  ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_SOURCE_DIR}/test_filter.sh
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_filter
  COMMENT "Copying test_filter.sh to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_filter"
)
add_dependencies(test_filter filter_received)
add_test(
  NAME test_filter
  COMMAND ${CMAKE_COMMAND} -E env
  FILTER=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/filter_received
  BSEXTR=${CMAKE_SOURCE_DIR}/jsvm/bin/BitStreamExtractorStaticd
  bash test_filter
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
