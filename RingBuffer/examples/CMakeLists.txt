add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example ring_buffer)
target_compile_features(basic_example PRIVATE cxx_std_11)

install(TARGETS basic_example
        RUNTIME DESTINATION bin)