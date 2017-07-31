include_directories(../src)
link_libraries(${Boost_LIBRARIES})

add_executable(test_rng test_rng.cpp)
target_link_libraries(test_rng rng)
add_test(test_rng test_rng)

add_executable(test_data_client_server test_data_client_server.cpp)
target_link_libraries(test_data_client_server
  decoder
  block_encoder
  packets_rw
)
add_test(test_data_client_server test_data_client_server)

add_executable(test_packets test_packets.cpp)
target_link_libraries(test_packets packets)
add_test(test_packets test_packets)

add_executable(test_counters test_counters.cpp)
add_test(test_counters test_counters)

add_executable(test_block_decoder test_block_decoder.cpp)
target_link_libraries(test_block_decoder block_decoder)
add_test(test_block_decoder test_block_decoder)

add_executable(test_block_encoder test_block_encoder.cpp)
target_link_libraries(test_block_encoder block_encoder)
add_test(test_block_encoder test_block_encoder)

add_executable(test_encoder_decoder test_encoder_decoder.cpp)
target_link_libraries(test_encoder_decoder
  block_encoder
  decoder
)
add_test(test_encoder_decoder test_encoder_decoder)

add_executable(test_message_passing test_message_passing.cpp)
target_link_libraries(test_message_passing packets)
add_test(test_message_passing test_message_passing)

add_executable(test_packets_rw test_packet_rw.cpp)
target_link_libraries(test_packets_rw packets_rw)
add_test(test_packets_rw test_packets_rw)
