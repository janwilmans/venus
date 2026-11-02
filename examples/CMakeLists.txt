# the examples are also used in the documentation (as Compiler Explorer links) and run as tests
foreach (example executor_example pool_executor_example dispatcher_example)
  add_executable(${example} ${example}.cpp)
  target_link_libraries(${example} PRIVATE venus::executor)
  add_test(NAME ${example} COMMAND ${example})
  set_tests_properties(${example} PROPERTIES TIMEOUT 60)
endforeach()
