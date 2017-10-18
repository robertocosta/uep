set(cpp_files
  base_types
  block_decoder
  block_encoder
  block_queues
  decoder
  log
  nal_reader
  nal_writer
  packets
  packets_rw
  protobuf_rw
  rng
  uep_decoder
)

foreach(cppfile IN LISTS cpp_files)
  add_library(${cppfile} STATIC ${cppfile}.cpp)
endforeach(cppfile)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS controlMessage.proto)
#include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library(controlMessage.pb STATIC ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(controlMessage.pb ${PROTOBUF_LIBRARIES})

target_link_libraries(packets
  base_types
)
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
target_link_libraries(uep_decoder
  decoder
  ${Boost_LIBRARIES}
)
target_link_libraries(log
  ${Boost_LIBRARIES}
)
target_link_libraries(nal_reader
  log
  packets
  ${Boost_LIBRARIES}
)
target_link_libraries(nal_writer
  nal_reader
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
  block_encoder
  block_queues
  controlMessage.pb
  nal_reader
  packets_rw
  protobuf_rw
  ${Boost_LIBRARIES}
)

add_executable(client client.cpp)
target_link_libraries(client
  controlMessage.pb
  decoder
  nal_writer
  packets_rw
  protobuf_rw
  uep_decoder
  ${Boost_LIBRARIES}
)

add_executable(filter_received filter_received.cpp)
target_link_libraries(filter_received
  log
  base_types
  ${Boost_LIBRARIES}
)

add_executable(mp_plots mp_plots.cpp)
target_link_libraries(mp_plots
  decoder
  block_encoder
  log
)

add_executable(uep_mp_plots uep_mp_plots.cpp)
target_link_libraries(uep_mp_plots
  uep_decoder
  block_encoder
  log
)

set(python_files
  avg_nal_segsizes
  ber
  plot_decodeable
  plot_required
  plot_time
)

foreach(pyf IN LISTS python_files)
  add_custom_target(${pyf} ALL
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/${pyf}.py
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${pyf}
    COMMENT "Copying ${pyf}.py to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${pyf}"
)
endforeach(pyf)
