add_executable(h2x_loco_client high_level/h2x_loco_client_example.cpp)
target_link_libraries(h2x_loco_client unitree_sdk2)

add_executable(h2x_arm_sdk_dds_example high_level/h2x_arm_sdk_dds_example.cpp)
target_link_libraries(h2x_arm_sdk_dds_example unitree_sdk2)

add_executable(h2x_ankle_swing_example low_level/h2x_ankle_swing_example.cpp)
target_link_libraries(h2x_ankle_swing_example unitree_sdk2)

add_executable(h2x_userctrl_dds_example high_level/h2x_userctrl_dds_example.cpp)
target_link_libraries(h2x_userctrl_dds_example unitree_sdk2)

add_executable(h2x_arm_action_example high_level/h2x_arm_action_example.cpp)
target_link_libraries(h2x_arm_action_example unitree_sdk2)

add_custom_target(h2x_examples
    DEPENDS h2x_loco_client h2x_arm_sdk_dds_example h2x_ankle_swing_example
            h2x_userctrl_dds_example h2x_arm_action_example)

find_package(yaml-cpp QUIET)
if(yaml-cpp_FOUND)
    add_executable(h2x_dual_arm_example low_level/h2x_dual_arm_example.cpp)
    target_include_directories(h2x_dual_arm_example PRIVATE ${YAML_CPP_INCLUDE_DIR})
    target_compile_definitions(h2x_dual_arm_example PUBLIC
        H2X_DUAL_ARM_BEHAVIOR_DIR="${CMAKE_CURRENT_SOURCE_DIR}/low_level/behavior_lib/")
    # Link the static archive, so the binary does not need libyaml-cpp.so at runtime.
    find_file(H2X_YAML_CPP_STATIC NAMES libyaml-cpp.a
        PATHS "${YAML_CPP_LIBRARY_DIR}" NO_DEFAULT_PATH)
    if(H2X_YAML_CPP_STATIC)
        target_link_libraries(h2x_dual_arm_example PRIVATE
            unitree_sdk2 "${H2X_YAML_CPP_STATIC}")
    else()
        target_link_libraries(h2x_dual_arm_example PRIVATE unitree_sdk2 yaml-cpp::yaml-cpp)
    endif()
    add_dependencies(h2x_examples h2x_dual_arm_example)
endif()

find_package(Eigen3 QUIET)
if(Eigen3_FOUND)
    add_executable(h2x_termination low_level/terminations.cpp)
    target_link_libraries(h2x_termination unitree_sdk2 Eigen3::Eigen)
    add_dependencies(h2x_examples h2x_termination)
endif()
