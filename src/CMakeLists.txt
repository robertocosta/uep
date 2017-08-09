set(cpp_files
  block_decoder
  block_encoder
  block_queues
  decoder
  log
  packets
  packets_rw
  rng
)

foreach(cppfile IN LISTS cpp_files)
  add_library(${cppfile} STATIC ${cppfile}.cpp)
endforeach(cppfile)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS controlMessage.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library(controlMessage.pb STATIC ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(controlMessage.pb ${PROTOBUF_LIBRARIES})

target_link_libraries(packets_rw
  packets
)
target_link_libraries(block_queues packets)
target_link_libraries(block_encoder rng packets)
target_link_libraries(block_decoder
  rng
  packets
  log
)
target_link_libraries(decoder
  block_decoder
  block_queues
  ${Boost_LIBRARIES}
)
target_link_libraries(log
  ${Boost_LIBRARIES}
)

add_executable(demo_ds demo_ds.cpp)
target_link_libraries(demo_ds
  block_encoder
  block_queues
  packets_rw
  log
  ${Boost_LIBRARIES}
)

add_executable(demo_dc demo_dc.cpp)
target_link_libraries(demo_dc
  decoder
  packets_rw
  log
)

add_executable(server server.cpp)
target_link_libraries(server
  controlMessage.pb
  block_encoder
  block_queues
  packets_rw
  ${Boost_LIBRARIES}
)

add_executable(client client.cpp)
target_link_libraries(client
  controlMessage.pb
  decoder
  packets_rw
  ${Boost_LIBRARIES}
)

add_executable(mp_plots mp_plots.cpp)
target_link_libraries(mp_plots
  decoder
  block_encoder
  log
)

add_custom_target(plot_required ALL
  ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_SOURCE_DIR}/plot_required.py
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plot_required
  COMMENT "Copying plot_required.py into build dir"
)

add_custom_target(plot_decodeable ALL
  ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_SOURCE_DIR}/plot_decodeable.py
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plot_decodeable
  COMMENT "Copying plot_decodeable.py into build dir"
)

add_custom_target(plot_time ALL
  ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_SOURCE_DIR}/plot_time.py
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plot_time
  COMMENT "Copying plot_time.py into build dir"
)
