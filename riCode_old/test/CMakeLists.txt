# CmakeLists in test dir

# Make sure the compiler can find include from our RICPlib library
INCLUDE_DIRECTORIES(${IRCP_SOURCE_DIR}/src)

# Add binary called "ircpReader" that is built from the source file "test.cpp"
ADD_EXECUTABLE(ircpReader testReader)

# Link the executable to the RICPlib library
TARGET_LINK_LIBRARIES(ircpReader RICPlib z)

# Add binary called "ircpWriter" that is built from the source file "test.cpp"
ADD_EXECUTABLE(ircpWriter testWriter)

# Link the executable to the RICPlib library
TARGET_LINK_LIBRARIES(ircpWriter RICPlib z)


