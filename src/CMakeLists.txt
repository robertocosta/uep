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

add_executable(nal_overhead nal_overhead.cpp)
target_link_libraries(nal_overhead
  ${Boost_LIBRARIES}
  log
  nal_reader
)

# add_executable(mp_plots mp_plots.cpp)
# target_link_libraries(mp_plots
#   decoder
#   block_encoder
#   log
# )

# add_executable(uep_mp_plots uep_mp_plots.cpp)
# target_link_libraries(uep_mp_plots
#   uep_decoder
#   block_encoder
#   log
# )

set_property(SOURCE uep_fast_run.i PROPERTY CPLUSPLUS ON)
swig_add_module(uep_fast_run python
  uep_fast_run.i
  uep_fast_run.cpp
  )
swig_link_libraries(uep_fast_run
  block_encoder
  log
  uep_decoder
)

set_target_properties(${SWIG_MODULE_uep_fast_run_REAL_NAME}
  PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SWIG_OUTDIR}
)

set(python_files
  avg_nal_segsizes
  ber
  overhead
  plot_ber_iid
  plot_ber_markov2_vs_ef
  plot_ber_markov2_vs_k
  plot_decodeable
  plot_fast_iid
  plot_fast_markov
  plot_overhead
  plot_required
  plot_time
  separate_markov
  uep_dec_time
  uep_enc_time
  utils
)

foreach(pyf IN LISTS python_files)
  add_custom_target(${pyf}.py ALL
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/${pyf}.py
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${pyf}.py
    COMMENT "Copying ${pyf}.py to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${pyf}.py"
  )

  add_custom_target(${pyf} ALL
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/${pyf}.py
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${pyf}
    COMMENT "Copying ${pyf}.py to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${pyf}"
  )
endforeach(pyf)
