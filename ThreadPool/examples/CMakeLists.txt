add_executable(thread_pool_test main.cpp)
target_link_libraries(thread_pool_test thread_pool)
target_compile_features(thread_pool_test PRIVATE cxx_std_17)

# install(TARGETS thread_pool_test
#         RUNTIME DESTINATION bin)
install(TARGETS thread_pool_test
    DESTINATION bin
)