gtest_dep = dependency('gtest', main: false, required: false)
gmock_dep = dependency('gmock', required: false)

if not gtest_dep.found() or not gmock_dep.found()
    cmake = import('cmake')
    opt_var = cmake.subproject_options()
    opt_var.add_cmake_defines({'CMAKE_CXX_FLAGS': '-Wno-pedantic'})
    opt_var.set_override_option('warning_level', '3')
    opt_var.set_override_option('werror', 'true')

    gtest_proj = cmake.subproject('gtest', options: opt_var, required: true)
    if gtest_proj.found()
        gtest_dep = declare_dependency(
            dependencies: [
                dependency('threads'),
                gtest_proj.dependency('gtest')
            ]
        )
        gmock_dep = gtest_proj.dependency('gmock')
    else
        assert(
            not get_option('tests').enabled(),
            'Googletest is required if tests are enabled'
        )
    endif
endif

common_test_deps = [
    gtest_dep,
    gmock_dep,
    sdbusplus,
    phosphor_dbus_interfaces,
]

production_sources = [
    '../src/ssh-session-manager.cpp',
    '../src/kvm-session-manager.cpp',
    '../src/web-session-manager.cpp',
    '../src/vmedia-session-manager.cpp',
]

unit_test_exe = executable(
    'unit_test_session-manager',
    [
        'test_main.cpp',
        'test_utils.cpp',
        'test_ssh_session_manager.cpp',
        'test_kvm_session_manager.cpp',
        'test_web_session_manager.cpp',
        'test_vmedia_session_manager.cpp',
    ] + production_sources,
    include_directories: include_directories('..', '../include'),
    dependencies: common_test_deps,
)

test('unit_test_session-manager', unit_test_exe)
